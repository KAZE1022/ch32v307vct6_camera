/********************************** (C) COPYRIGHT *******************************
* File Name          : main.c
* Author             : WCH / 修改: OV2640 -> 1.8寸 128x160 ST7735S(SPI)
* Version            : V2.0.0
* Date               : 2025/04/06
* Description        : OV2640 视频流显示 + 参数诊断页 + 像素校准画面.
*
*  一、摄像头接线(与 WCH 原例程一致)
*    D0--PA9    D1--PA10   D2--PC8    D3--PC9    D4--PC11   D5--PB6    D6--PB8    D7--PB9
*    D8--PC10   D9--PC12   D10--PD6   D11--PD2
*    DPCLK--PA6  HERF--PA4  SYNC--PA5
*    SDCLK--PB10 (SCCB SCL)   SDA--PB11 (SCCB SDA)
*    RESET--PC13  DPWDN--PC3   (模块上的 NC 脚悬空即可)
*
*  二、LCD 接线(1.8寸 128x160 ST7735S, 8 脚 SPI 模块)
*    VCC--3.3V  GND--GND  SCL--PB13(SPI2_SCK)  SDA--PB15(SPI2_MOSI)
*    RST--PC4   DC--PB14  CS--PB12             BLK--PC5(高电平点亮)
*
*  三、上电流程
*    1. 像素校准画面(色条 + 灰阶 + 黑白块 + 边框)   约 2 秒
*    2. 参数诊断页(帧/行/丢行/FIFO/清晰度/数据样本)  约 6 秒
*    3. 摄像头实时画面(全屏)
*
*  四、串口: UART1(PA9/PA10)被 DVP 的 D0/D1 占用, 调试打印用 UART2(PA2),
*     工程里已定义 DEBUG=DEBUG_UART2; 未定义时 DBG 自动失效, 不会卡死.
*********************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/
#include "debug.h"
#include "lcd.h"
#include "ov.h"

/* 串口调试输出(UART2/PA2), 未定义 DEBUG=DEBUG_UART2 时自动关闭 */
#if (DEBUG == DEBUG_UART2)
#define DBG_INIT()   USART_Printf_Init(115200)
#define DBG(...)     printf(__VA_ARGS__)
#else
#define DBG_INIT()
#define DBG(...)
#endif

/*==============================================================================
 *  可调参数
 *============================================================================*/
/* 像素校准画面显示时长(ms) */
#ifndef CALIB_SHOW_MS
#define CALIB_SHOW_MS        2000
#endif

/* 参数诊断页显示时长(ms), 之后自动进入实时画面 */
#ifndef DIAG_SHOW_MS
#define DIAG_SHOW_MS         6000
#endif

/* 诊断页行数(每行的位置按屏高自动均分, 竖屏/横屏都能排下) */
#define DIAG_LINES           10

/* 实时画面左下角的帧率叠字: 1 = 显示, 0 = 不显示
 * 做法是把文字直接画进"片"缓冲, 跟着画面一起推给屏 —— 所以它不会被下一次
 * 刷新冲掉(用 LCD_ShowString 直接画在屏上是不行的, 几十毫秒后就被画面盖掉). */
#ifndef SHOW_FPS
#define SHOW_FPS             1
#endif

/* 帧率统计窗口(ms): 每积累这么多时间结算一次 */
#define FPS_WIN_MS           1000

/* 软件白平衡(灰世界): 1 = 自动把 R/G/B 拉平(画面发黄/发蓝时很有用), 0 = 不处理
 * 画面本来就正常时它不会乱动(系数停在 1.0); 只在明显偏色时才修, 且限制在 0.5~2 倍 */
#ifndef CAM_AWB
#define CAM_AWB              1
#endif

/* 摄像头像素字节序: OV2640 的 RGB565 是"低字节先发"(WCH 表里 0xDA=0x09 =
 * RGB565 | LBYTE_FIRST), 而 ST7735S 要求高字节先发, 所以必须交换一次.
 * 正常保持 1; 万一颜色变成另一种错法再试 0 */
#ifndef CAM_BYTE_SWAP
#define CAM_BYTE_SWAP       1
#endif

/* 画面左右镜像: 1 = 摄像头画面水平翻转(左右对调)显示, 0 = 正常
 * 只影响摄像头画面; 屏幕上的诊断文字/校准画面**不受影响**(保持可读).
 * 做法是在把每一行存进帧缓冲时顺便反过来, 不额外花时间、不影响帧率.
 * 注: 如果想让整屏(含文字)一起镜像, 用 lcd.h 里的 LCD_MIRROR_H = 1 */
#ifndef CAM_MIRROR_X
#define CAM_MIRROR_X        0
#endif

/*==============================================================================
 *  帧率开关(画面"延迟感"的真正大头)
 *  OV2640 内部没有 PLL, 帧率只由一个寄存器决定: 传感器组(0xFF=0x01)的 CLKRC(0x11):
 *      CLK  = XVCLK(模块上的 24MHz 晶振) x (bit7 ? 2 : 1) / (bit[5:0] + 1)
 *      帧率 = CLK / (tLINE x 每帧总行数)
 *      本机走的是 UXGA 栅格: tLINE = 1922, 每帧约 1560 行(含消隐)
 *      -> CLK=12MHz 约 4fps, 24MHz 约 8fps, 48MHz 约 16fps
 *  WCH 原始代码写的是 0x01 -> 24/2 = 12MHz -> 只有 4fps,
 *  这就是"人动一下、画面要等半天才跟上"的根本原因.
 *  (屏不是瓶颈: ST7735S 是 60Hz, 刷一屏 16.7ms, 比摄像头一帧快十几倍.)
 *
 *  注意: 帧率变快 N 倍, 同一个曝光值就只有 1/N 的曝光时间 -> 画面会暗 N 倍,
 *        所以下面会自动把曝光值也乘 N, 亮度基本不变.
 *============================================================================*/
#ifndef OV2640_CLKRC
#define OV2640_CLKRC         0x80   /* 0x01 = 12MHz ≈ 4fps (原样)
                                     * 0x00 = 24MHz ≈ 8fps
                                     * 0x80 = 48MHz ≈ 16fps (数据手册标称, 默认)
                                     * 画面花/不出图: 先改 0x00, 再不行改回 0x01 */
#endif

/* 相对"原来 12MHz"的倍率: 用来同步放大曝光值 */
#define OV2640_CLK_X         (2 * (((OV2640_CLKRC) & 0x80) ? 2 : 1) / (((OV2640_CLKRC) & 0x3F) + 1))

/* 摄像头输出时钟分频: PCLK = CLK / 本值
 *   默认让 PCLK 保持 12MHz(现在验证过没问题的值):
 *     CLKRC=0x01 -> 1     CLKRC=0x00 -> 2     CLKRC=0x80 -> 4
 *   例外: SVGA 栅格 + 48MHz 时, 一行只有约 25us, 而 240 像素 x2 字节在
 *         12MHz 下要 40us —— 塞不进一行, 所以这时自动改成 24MHz(PCLK_DIV=2).
 *   想让 DVP 采样更宽松(更慢但更稳)可以手动调大; 调太小会花屏/错位 */
#ifndef OV2640_PCLK_DIV
#if (OV2640_SVGA_MODE == 1) && (OV2640_CLK_X >= 4)
#define OV2640_PCLK_DIV      (OV2640_CLK_X / 2)      /* 24MHz */
#else
#define OV2640_PCLK_DIV      OV2640_CLK_X            /* 12MHz */
#endif
#endif

/*==============================================================================
 *  亮度: 软件自动曝光(默认开)
 *  摄像头自己的自动曝光在这里不好使(暗处会把帧时间拉到几百毫秒, 而且和
 *  "固定曝光保延迟"的做法冲突), 所以改成**我们自己测亮度、自己调**:
 *     每 200ms 测一次画面亮度(行缓冲高字节的平均值, 0~255),
 *       太暗: 先把曝光加到满(255), 还暗再加增益
 *       太亮: 先减增益, 减到最小还亮再减曝光
 *  这样任何光线下都能自己收敛, 诊断页能看到它当前用的曝光/增益和测到的亮度.
 *  想改回"固定曝光+固定增益"(不自动调): OV2640_AUTO_EXP 改成 0
 *============================================================================*/
#ifndef OV2640_AUTO_EXP
#define OV2640_AUTO_EXP      1
#endif

/* 目标亮度(0~255): 越大画面越亮. 100~130 比较自然; 嫌暗就加到 150 */
#ifndef OV2640_AE_TARGET
#define OV2640_AE_TARGET     110
#endif

/* 自动曝光的"死区"和步长 —— 防抖用:
 *   亮度落在 目标±AE_BAND 以内就**一点都不动**(不然会一会儿亮一会儿暗, 就是"呼吸")
 *   要动的时候每步也只走 AE_STEP_xxx, 慢慢靠过去, 不会一下跳一大截 */
#define AE_BAND              14      /* 死区(0~255): 调大更稳, 调小反应更快 */
#define AE_STEP_EXP          12      /* 曝光每步最大变化 */
#define AE_STEP_GAIN         8       /* 增益每步最大变化 */
#define AE_AVG_SHIFT         2       /* 亮度/颜色的低通滤波强度(2 = 平均最近约 4 次) */

/* 低延迟模式: 固定曝光(关掉摄像头自带的自动曝光)
 *   0 = 用原厂自动曝光(暗处延迟几百毫秒)    1 = 低延迟(默认) */
#ifndef OV2640_LOW_LATENCY
#define OV2640_LOW_LATENCY   1
#endif

/* 固定曝光时间(名义值), 单位微秒(µs) —— 只有 OV2640_AUTO_EXP=0 时才是"固定"的,
 * 开着自动曝光时它就是**起始值**(回路会自己往上/往下调).
 *   加曝光不提噪点, 但画面更容易糊(运动模糊), 太大还会拖慢帧率.
 *   注 1: "一行"的时间随"栅格模式 + 帧率时钟"变, 这里已经自动换算好了
 *   注 2: 真正写进摄像头 AEC(0x10) 的值 = 换算结果, 诊断页会显示出来.
 *         手册对 0x10 的刻度说法不一(Linux 驱动注释是 AEC[9:2], 即 1 个数 = 4 行),
 *         所以**绝对毫秒数不必较真, 以画面亮度为准**; 8 位寄存器写满 255 就到顶. */
#ifndef OV2640_AEC_US
#define OV2640_AEC_US        8000
#endif

/* 栅格模式(SVGA 开关) —— 帧率的"总开关"(帧率 = CLK / (tLINE x 每帧行数)):
 *   0 = UXGA 栅格: tLINE=1922, 约 1560 行/帧 -> 48MHz 时约 16fps  (画面最细)
 *   1 = SVGA 栅格: tLINE=1190, 约  672 行/帧 -> 帧率约 4 倍
 *       配 OV2640_CLKRC=0x00(24MHz) 约 30fps, 配 0x80(48MHz) 约 60fps
 *   ⚠ 帧率越高, 一帧时间越短 -> 曝光上限越小 -> 暗处越暗(30fps 时一帧只有 33ms).
 *     画面花了/比例不对就先改回 0. */
#ifndef OV2640_SVGA_MODE
#define OV2640_SVGA_MODE     0
#endif

/* 一行的时间(以时钟周期数表示): tLINE = 本值 / CLK, CLK = 12MHz x OV2640_CLK_X */
#if (OV2640_SVGA_MODE == 1)
#define OV2640_TLINE_TX      1190
#else
#define OV2640_TLINE_TX      1922
#endif

/* 换算成摄像头要的曝光值: 时间(µs) / 一行的时间(µs), 再限制在 8 位寄存器上限 */
#define OV2640_AEC_LINES     ((OV2640_AEC_US * 12 * OV2640_CLK_X) / OV2640_TLINE_TX)
#if (OV2640_AEC_LINES > 255)
#undef  OV2640_AEC_LINES
#define OV2640_AEC_LINES     255
#endif

/* 增益: 和曝光一起决定亮度
 *   OV2640_AUTO_EXP=1 时这是**起始值**(回路会自己调)
 *   OV2640_AUTO_EXP=0 时这是固定值: 0 = 增益自动(上限 128x), 0x01~0xFF = 手动固定
 *     太暗 -> 加大(0x80 / 0xCF / 0xFF)  过曝 -> 减小(0x20 / 0x10) */
#ifndef OV2640_GAIN_VALUE
#define OV2640_GAIN_VALUE    0x40
#endif

/*==============================================================================
 *  摄像头参数和屏幕的匹配(视野开关)
 *  摄像头输出固定 240x320(3:4), 屏是 128x160(4:5), 两者不是同一个比例, 两种做法:
 *
 *   0 = 【默认, 最清楚】上面的固定取景框: 中心 128x160 原样上屏(1:1).
 *       一个摄像头像素 = 一个屏像素, 不缩放、不模糊、不花 CPU、不影响帧率.
 *       代价: 只看得到摄像头视野中心约 1/4 的面积(四周被裁掉了).
 *
 *   1 = 全视野降采样: 忽略取景框, 把整幅 240x320 缩到屏上(横竖都按比例取).
 *       看到的是摄像头的**全部视野**, 一点不裁.
 *       代价: 画面变"糊/有锯齿"(最近邻取样, 没做平均). 帧率不受影响.
 *============================================================================*/
#ifndef CAM_FULL_VIEW
#define CAM_FULL_VIEW        0
#endif

/*==============================================================================
 *  取景框: 固定取摄像头输出画面(240x320)的**正中间** 128x160
 *
 *  以画面中心点算:
 *      横向 128  ->  左右各占一半: 左右各丢 56 列, 取第  56..183 列
 *      纵向 160  ->  上下各占一半: 上下各丢 80 行, 取第  80..239 行
 *  1 个摄像头像素 = 1 个屏像素(1:1), 不缩放、不花 CPU、不影响帧率.
 *  这两个数只跟"摄像头输出尺寸"有关, 和屏幕旋转/镜像/补色**无关**.
 *
 *  注: 只有屏也是 128x160(竖屏)时才能 1:1 铺满 —— 旋转 90/270 会变成 160x128,
 *      那时要把 CAM_VIEW_W/H 改成 160/128(否则屏两侧会空黑边), 代码里已做检查.
 *============================================================================*/
#ifndef CAM_VIEW_W
#define CAM_VIEW_W           128
#endif
#ifndef CAM_VIEW_H
#define CAM_VIEW_H           160
#endif

/*==============================================================================
 *  摄像头取图与裁剪
 *============================================================================*/
#define CAM_WIDTH             240                          /* 摄像头输出宽 */
#define CAM_HEIGHT            320                          /* 摄像头输出高 */
#define CAM_ROW_BYTES         (CAM_WIDTH * 2)              /* 480 */

/* 取景框必须比输出小, 而且差值必须是偶数, 才谈得上"上下/左右各占一半" */
#if ((CAM_WIDTH - CAM_VIEW_W) < 0) || ((CAM_HEIGHT - CAM_VIEW_H) < 0)
#error "取景框比摄像头输出画面还大: 请把 CAM_VIEW_W / CAM_VIEW_H 改小"
#endif
#if (((CAM_WIDTH - CAM_VIEW_W) % 2) != 0) || (((CAM_HEIGHT - CAM_VIEW_H) % 2) != 0)
#error "取景框和摄像头输出的差不是偶数, 做不到上下/左右各占一半"
#endif

/* 居中: 左边和右边各留 (240-128)/2 = 56 列, 上边和下边各留 (320-160)/2 = 80 行 */
#define CROP_X                ((CAM_WIDTH  - CAM_VIEW_W) / 2)      /* 56 */
#define CROP_Y                ((CAM_HEIGHT - CAM_VIEW_H) / 2)      /* 80 */

#if (CAM_FULL_VIEW == 0) && ((CAM_VIEW_W != LCD_WIDTH) || (CAM_VIEW_H != LCD_HEIGHT))
#error "取景框是固定的 128x160, 但当前屏尺寸不是 128x160(是不是把 LCD_ROTATE 设成 90/270 了?): 请把 CAM_VIEW_W/H 改成屏尺寸(横屏就是 160/128), 或者把 LCD_ROTATE 改回 0/180"
#endif

#if (CAM_FULL_VIEW == 1)
#define CAM_ROW_SRC_W         CAM_WIDTH                   /* 全视野模式: 整行 240 个源像素 */
#else
#define CAM_ROW_SRC_W         CAM_VIEW_W                  /* 取景框行 128 个源像素(1:1) */
#endif

#define ROW_BYTES             (LCD_WIDTH * 2)              /* 每行推给屏 256 字节 */

/*==============================================================================
 *  分片刷屏(降低"延迟感"的关键)
 *  以前是"整帧攒满才刷一次": 屏幕内容平均老半帧, 看起来就是跳一下、慢半拍.
 *  现在把画面切成 SLICE_NUM 个横条, 每攒满一条就立刻刷出去:
 *    屏幕连续更新(一帧分十几次刷新), 看到的永远是最新内容, 延迟感明显小.
 *  RAM: SLICE_NUM * SLICE_ROWS * 每行字节  (128x160 时 = 16 * 10 * 256 = 40KB)
 *  注: 旋转 90/270 后屏高是 128, 除不尽 10, 所以最后一片允许不满(见 SLICE_ROWS_OF).
 *============================================================================*/
#define SLICE_ROWS           10                                  /* 每片行数 */
#define SLICE_NUM            ((LCD_HEIGHT + SLICE_ROWS - 1) / SLICE_ROWS)
#define SLICE_BYTES          (SLICE_ROWS * ROW_BYTES)            /* 每片最大字节数 */

/* 第 s 片实际有多少行(最后一片可能不满) */
#define SLICE_ROWS_OF(s)     (((((s) + 1) * SLICE_ROWS) > LCD_HEIGHT) ? \
                              (LCD_HEIGHT - (s) * SLICE_ROWS) : SLICE_ROWS)

static u8 slice_buf[SLICE_NUM][SLICE_BYTES] __attribute__((aligned(4)));
static volatile u8  slice_ready[SLICE_NUM];       /* 这一片攒满了, 等主循环刷 */
static volatile u8  slice_busy[SLICE_NUM];        /* 这一片正在被 DMA 搬给屏, 不能改写 */
static volatile u16 slice_rows[SLICE_NUM];        /* 这一片已收行数 */
static volatile u8  slice_inflight = 0xFF;        /* 正在搬的那一片(0xFF=没有) */

/* DVP 行缓冲(乒乓), 每块装摄像头一整行 */
static u8 dvp_row_buf0[CAM_ROW_BYTES] __attribute__((aligned(4)));
static u8 dvp_row_buf1[CAM_ROW_BYTES] __attribute__((aligned(4)));

/*==============================================================================
 *  运行状态(供诊断页显示)
 *============================================================================*/
volatile u32 frame_cnt    = 0;      /* 收到的帧数(靠 VSYNC) */
volatile u32 href_cnt     = 0;      /* 收到的行数 */
volatile u32 row_cnt      = 0;      /* 当前帧内的行号 */
volatile u32 row_drop_cnt = 0;      /* 丢行次数(基本应该为 0) */
volatile u32 fifo_ov_cnt  = 0;      /* DVP FIFO 溢出次数 */
volatile u32 focus_sum    = 0;      /* 清晰度累计 */
volatile u32 focus_rows   = 0;      /* 清晰度统计的行数 */

volatile u8  video_on     = 0;      /* 0=显示诊断页, 1=摄像头画面 */
volatile u8  ae_luma      = 0;      /* 最近一次测到的画面亮度(0~255, 诊断页显示) */
volatile u8  ae_r = 0, ae_g = 0, ae_b = 0;   /* 最近一次测到的 R/G/B 平均值 */

#if (OV2640_AUTO_EXP == 1)
static u8 ae_exp  = (u8)OV2640_AEC_LINES;                    /* 当前曝光(写 AEC 0x10) */
static u8 ae_gain = (u8)((OV2640_GAIN_VALUE != 0) ? OV2640_GAIN_VALUE : 0x40);
static u8 ae_luma_f = 0;                                     /* 滤波后的亮度 */
static u8 ae_rf = 0, ae_gf = 0, ae_bf = 0;                   /* 滤波后的 R/G/B */
static u8 ae_inited = 0;                                     /* 第一次测量直接当初值 */
#endif

#if (CAM_AWB == 1)
#define WB_MIN     128      /* 白平衡系数下限 0.50 倍 */
#define WB_MAX     512      /* 上限 2.00 倍 */
#define WB_STEP    4        /* 每步最多变这么多(256 = 1.0 倍) */
#define WB_DEAD    6        /* 死区: 差这么点就不动, 免得颜色一直微调 */

static u16 wb_r = 256, wb_b = 256;                   /* 白平衡系数(8.8 定点, 256 = 1.0) */
static u8  wb_rlut[32], wb_blut[32];                 /* 系数查表(每像素只查表, 不做乘法) */
#endif

#if (SHOW_FPS == 1)
static u32 fps_row_per_ms = 0;      /* 启动时标定: 每毫秒收多少行(摄像头晶振决定, 很准) */
static u32 fps_val        = 0;      /* 最近一次算出来的帧率 */
static u32 fps_rps        = 0;      /* 最近一次算出来的行速率 */
#endif

void DVP_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

/*********************************************************************
 * @fn      CAM_MeasureStats
 *
 * @brief   量一下当前画面的 R/G/B 平均值和亮度(都是 0~255), 
 *          给软件自动曝光(看亮度)和软件白平衡(看 R/G/B 平不平衡)用.
 *          做法: 从行缓冲里隔 8 字节抽一个像素, 把 RGB565 拆成 R/G/B 累加.
 *          注: 读的同时 DVP 可能正在写这块缓冲 —— 对"平均值"没影响.
 *
 * @param   r/g/b - 输出: 三通道平均值(0~255)
 *          y     - 输出: 亮度(0.3R + 0.6G + 0.1B)
 *
 * @return  none
 */
static void CAM_MeasureStats(u8 *r, u8 *g, u8 *b, u8 *y)
{
    u32 sr = 0, sg = 0, sb = 0, n = 0;
    u16 i;

    for(i = 1; i < (u16)CAM_ROW_BYTES; i += 8)
    {
        u16 px = (u16)(((u16)dvp_row_buf0[i] << 8) | dvp_row_buf0[i - 1]);   /* 低字节先发 */

        sr += (u16)(((px >> 11) & 0x1F) << 3);      /* R: 5 位 -> 0~248 */
        sg += (u16)(((px >> 5) & 0x3F) << 2);       /* G: 6 位 -> 0~252 */
        sb += (u16)((px & 0x1F) << 3);              /* B: 5 位 -> 0~248 */
        n++;
    }

    if(n == 0)
    {
        *r = 0; *g = 0; *b = 0; *y = 0;
        return;
    }

    sr /= n;  sg /= n;  sb /= n;
    *r = (u8)sr;  *g = (u8)sg;  *b = (u8)sb;
    *y = (u8)((sr * 77 + sg * 150 + sb * 29) >> 8);
}

#if (CAM_AWB == 1)
/*********************************************************************
 * @fn      CAM_WbBuildLut
 *
 * @brief   把白平衡系数做成查表(32 个档位), 这样每个像素只要查两次表,
 *          比"每像素做乘法"快得多(中断里省时间).
 *
 * @return  none
 */
static void CAM_WbBuildLut(void)
{
    u8 i;

    for(i = 0; i < 32; i++)
    {
        u16 v = (u16)(((u16)i * wb_r) >> 8);

        wb_rlut[i] = (u8)((v > 31) ? 31 : v);

        v = (u16)(((u16)i * wb_b) >> 8);
        wb_blut[i] = (u8)((v > 31) ? 31 : v);
    }
}
#endif  /* CAM_AWB */

#if (CAM_AWB == 1) && (OV2640_AUTO_EXP == 1)
/*********************************************************************
 * @fn      CAM_AutoWhiteBalance
 *
 * @brief   软件白平衡(灰世界): 让 R/G/B 的平均值趋于相等.
 *          只动 R 和 B 的系数(0.5~2 倍), 每一步只挪一点, 所以不会来回抖.
 *          画面本来就正的时候 R/G/B 本来就差不多 -> 系数会停在 1.0 不动.
 *
 * @param   r/g/b - 刚量到的三通道平均值
 *
 * @return  none
 */
static void CAM_AutoWhiteBalance(u8 r, u8 g, u8 b)
{
    u32 target;
    u16 want_r, want_b, diff_r, diff_b;

    if((r < 8) || (b < 8))
    {
        return;                                     /* 太暗了, 先让自动曝光去提亮 */
    }

    target = ((u32)r + g + b) / 3;
    want_r = (u16)((target << 8) / r);              /* 想要的系数(256 = 1.0 倍) */
    want_b = (u16)((target << 8) / b);

    if(want_r > WB_MAX) want_r = WB_MAX;
    if(want_r < WB_MIN) want_r = WB_MIN;
    if(want_b > WB_MAX) want_b = WB_MAX;
    if(want_b < WB_MIN) want_b = WB_MIN;

    /* 死区: 差得不多就不动它(免得一直微调, 画面颜色看着在"呼吸") */
    diff_r = (want_r > wb_r) ? (u16)(want_r - wb_r) : (u16)(wb_r - want_r);
    diff_b = (want_b > wb_b) ? (u16)(want_b - wb_b) : (u16)(wb_b - want_b);
    if((diff_r <= WB_DEAD) && (diff_b <= WB_DEAD))
    {
        return;
    }

    /* 每步最多挪 WB_STEP, 慢慢收敛, 避免画面抖色 */
    if(want_r > (u16)(wb_r + WB_STEP))       wb_r = (u16)(wb_r + WB_STEP);
    else if((u16)(want_r + WB_STEP) < wb_r)  wb_r = (u16)(wb_r - WB_STEP);
    else                                     wb_r = want_r;

    if(want_b > (u16)(wb_b + WB_STEP))       wb_b = (u16)(wb_b + WB_STEP);
    else if((u16)(want_b + WB_STEP) < wb_b)  wb_b = (u16)(wb_b - WB_STEP);
    else                                     wb_b = want_b;

    CAM_WbBuildLut();
}
#endif  /* CAM_AWB && OV2640_AUTO_EXP */

#if (OV2640_AUTO_EXP == 1)
/*********************************************************************
 * @fn      CAM_AutoExposure
 *
 * @brief   软件自动曝光: 用测到的亮度反过来调"曝光值"和"增益"
 *            太暗 -> 先加曝光, 曝光到顶(255)了再加增益
 *            太亮 -> 先减增益, 增益到最小了再减曝光
 *            任何一个通道快饱和(过曝发白/发黄) -> 直接往下调, 不等亮度比较
 *          每调用一次走一小步(约 200ms 调一次), 所以不会来回闪.
 *          顺便把软件白平衡也走一步.
 *
 * @return  none
 */
static void CAM_AutoExposure(void)
{
    u8 r, g, b, y, peak;
#if (CAM_AWB == 1)
    static u8 awb_div = 0;                              /* 白平衡每 2 步走一次 */
#endif

    CAM_MeasureStats(&r, &g, &b, &y);

    /* 先低通滤波: 单行抽样本身有噪声, 不滤的话回路会跟着噪声来回动
     * (现象就是画面"一会儿白一会儿暗"的呼吸感) */
    if(ae_inited == 0)
    {
        ae_luma_f = y;
        ae_rf = r;  ae_gf = g;  ae_bf = b;
        ae_inited = 1;
    }
    else
    {
        ae_luma_f = (u8)(ae_luma_f + ((s16)((s16)y - (s16)ae_luma_f) >> AE_AVG_SHIFT));
        ae_rf     = (u8)(ae_rf     + ((s16)((s16)r - (s16)ae_rf)     >> AE_AVG_SHIFT));
        ae_gf     = (u8)(ae_gf     + ((s16)((s16)g - (s16)ae_gf)     >> AE_AVG_SHIFT));
        ae_bf     = (u8)(ae_bf     + ((s16)((s16)b - (s16)ae_bf)     >> AE_AVG_SHIFT));
    }

    ae_luma = y;            /* 诊断页显示"原始"值, 更直观 */
    ae_r = r;  ae_g = g;  ae_b = b;

#if (CAM_AWB == 1)
    if(++awb_div >= 2)
    {
        awb_div = 0;
        CAM_AutoWhiteBalance(ae_rf, ae_gf, ae_bf);
    }
#endif

    peak = (r > g) ? r : g;
    if(b > peak) peak = b;

    if((ae_luma_f < (u8)(OV2640_AE_TARGET - AE_BAND)) && (peak < 235))   /* 太暗, 而且没过曝 */
    {
        if(ae_exp < 255)
        {
            ae_exp = (u8)((ae_exp > (255 - AE_STEP_EXP)) ? 255 : (ae_exp + AE_STEP_EXP));
        }
        else if(ae_gain < 255)
        {
            ae_gain = (u8)((ae_gain > (255 - AE_STEP_GAIN)) ? 255 : (ae_gain + AE_STEP_GAIN));
        }
        else
        {
            return;                                     /* 已经到顶了, 再暗也没办法 */
        }
    }
    else if((ae_luma_f > (u8)(OV2640_AE_TARGET + AE_BAND)) || (peak >= 245))  /* 太亮/快饱和 */
    {
        if(ae_gain > 8)
        {
            ae_gain = (u8)((ae_gain < (8 + AE_STEP_GAIN)) ? 8 : (ae_gain - AE_STEP_GAIN));
        }
        else if(ae_exp > 2)
        {
            ae_exp = (u8)((ae_exp < (2 + AE_STEP_EXP)) ? 2 : (ae_exp - AE_STEP_EXP));
        }
        else
        {
            return;
        }
    }
    else
    {
        return;                                         /* 在死区里, 一点都不动 */
    }

    OV2640_SetExpGain(ae_exp, ae_gain);                 /* 写进摄像头 */
}
#endif

/*********************************************************************
 * @fn      CAM_PutPixel
 *
 * @brief   把一个摄像头像素写进片缓冲(片缓冲是"高字节在前", 屏要的顺序).
 *          开启软件白平衡(CAM_AWB)时顺便把 R 和 B 按系数缩放一下.
 *
 * @param   d - 目标(片缓冲里的 2 字节)
 *          s - 源(摄像头行缓冲里的 2 字节, 低字节在前)
 *
 * @return  none
 */
static inline void CAM_PutPixel(u8 *d, const u8 *s)
{
#if (CAM_AWB == 1)
    u16 px = (u16)(((u16)s[1] << 8) | s[0]);
    u16 out = (u16)(((u16)wb_rlut[(px >> 11) & 0x1F] << 11) |
                    (px & 0x07E0) |
                     (u16)wb_blut[px & 0x1F]);

    d[0] = (u8)(out >> 8);
    d[1] = (u8)(out & 0xFF);
#elif (CAM_BYTE_SWAP == 1)
    d[0] = s[1];
    d[1] = s[0];
#else
    d[0] = s[0];
    d[1] = s[1];
#endif
}

/*********************************************************************
 * @fn      CAM_StoreRow
 *
 * @brief   把摄像头的一行处理并存进"片"缓冲:
 *            1) 换像素字节序(OV2640 低字节先发 -> 屏要高字节先发)
 *            2) 可选左右镜像(CAM_MIRROR_X)
 *            3) 可选整幅缩到屏上(CAM_FULL_VIEW: 横向 240->128 最近邻)
 *            4) 可选软件白平衡(CAM_AWB)
 *          这些都在这一个循环里做完, 不会多花时间.
 *
 * @param   dst - 片缓冲里这一行的起点
 *          src - 摄像头行缓冲里对应的起点
 *
 * @return  none
 */
static void CAM_StoreRow(u8 *dst, const u8 *src)
{
    /* 横向取样步长(8.8 定点, 用累加代替每像素除法).
     *   src 指向的是"这一屏行对应的那一段源行":
     *     CAM_FULL_VIEW=0(中心裁剪) -> 这一段就是 128 个源像素  -> step = 256 (1:1)
     *     CAM_FULL_VIEW=1(全视野)   -> 这一段是整行 240 个源像素 -> step = 480 (1.875:1)
     *   ⚠ 这里如果用错(CAM_WIDTH 恒当步长基准), 中心裁剪模式会变成"整行压缩到屏上",
     *     而且取样下标会一路冲出 480 字节的行缓冲 -> 屏右侧 1/4 就是花边. */
    const u32 step = ((u32)CAM_ROW_SRC_W << 8) / LCD_WIDTH;
    u32 acc;
    u16 n;

#if (CAM_MIRROR_X == 1)
    acc = ((u32)CAM_ROW_SRC_W << 8);        /* 从最右边往回取 */
    for(n = 0; n < LCD_WIDTH; n++)
    {
        acc -= step;
        CAM_PutPixel(&dst[2 * n], src + ((acc >> 8) << 1));
    }
#else
    acc = 0;
    for(n = 0; n < LCD_WIDTH; n++)
    {
        CAM_PutPixel(&dst[2 * n], src + ((acc >> 8) << 1));
        acc += step;
    }
#endif
}

/*********************************************************************
 * @fn      RAM_Check
 *
 * @brief   检查分片缓冲所在的 RAM(32KB 以上)是否真的存在.
 *          芯片必须配置成 FLASH-192K + RAM-128K 才有这块 RAM,
 *          否则写进去读不回来(屏幕会提示 RAM NOT 128K!).
 *
 * @return  1 - 正常, 0 - 这块 RAM 不可用
 */
static u8 RAM_Check(void)
{
    u8  *p     = &slice_buf[0][0];
    u32  total = (u32)sizeof(slice_buf);        /* = SLICE_NUM * SLICE_ROWS * 256 */
    u32  i;

    for(i = 0; i < total; i += 4096)
    {
        p[i] = 0xA5;
    }
    for(i = 0; i < total; i += 4096)
    {
        if(p[i] != 0xA5)
        {
            return 0;
        }
    }
    for(i = 0; i < total; i += 4096)
    {
        p[i] = 0x5A;
    }
    for(i = 0; i < total; i += 4096)
    {
        if(p[i] != 0x5A)
        {
            return 0;
        }
    }
    return 1;
}

/*********************************************************************
 * @fn      LCD_CalibScreen
 *
 * @brief   像素校准画面(全部由本机生成, 和摄像头无关):
 *            上半屏: 8 条等宽竖彩条(白/黄/青/绿/品红/红/蓝/黑)
 *            中  部: 16 级灰阶竖条
 *            下  部: 白/黑方块 + 文字
 *            四周  : 1 像素白边框
 *          各段高度按屏高按比例算, 所以横屏(旋转90/270)也自动排得下.
 *          检查点:
 *            彩条颜色是否正确 -> 红蓝对调改 LCD_BGR_SWAP; 发白/发黑改 LCD_INVERT
 *            边框是否完整、有没有多余的一行/一列 -> 调 LCD_XSTART / LCD_YSTART
 *            8 条是否等宽 -> 证明水平方向 1:1 正常
 *            屏幕方向不对 -> 改 LCD_ROTATE / LCD_MIRROR_H / LCD_MIRROR_V
 *
 * @return  none
 */
static void LCD_CalibScreen(void)
{
    static const u16 bars[8] =
    {
        WHITE, YELLOW, CYAN, GREEN, MAGENTA, RED, BLUE, BLACK
    };
    u8 *pix;
    u16 s, r, rows, y, x, b;
    u16 y1 = (u16)((LCD_HEIGHT * 74) / 100);        /* 彩条区结束行  160 -> 118 */
    u16 y2 = (u16)((LCD_HEIGHT * 84) / 100);        /* 灰阶区结束行  160 -> 134 */
    u16 y3 = (u16)((LCD_HEIGHT * 91) / 100);        /* 方块区结束行  160 -> 145 */
    u16 y4 = (u16)(LCD_HEIGHT - 12);                /* 文字行        160 -> 148 */

    /* 一条一条画进"片", 画满一片立刻刷走(和实时画面走同一条路径,
     * 这样校准画面正常 = 刷屏链路正常) */
    for(s = 0; s < SLICE_NUM; s++)
    {
        pix  = slice_buf[s];
        rows = (u16)SLICE_ROWS_OF(s);

        for(r = 0; r < rows; r++)
        {
            y = (u16)(s * SLICE_ROWS + r);

            for(x = 0; x < LCD_WIDTH; x++)
            {
                if(y < y1)
                {
                    b = bars[(x * 8) / LCD_WIDTH];              /* 8 条等宽彩条 */
                }
                else if(y < y2)
                {
                    b = (u16)(((x * 16) / LCD_WIDTH) * 0x0841); /* 16 级灰阶 */
                }
                else if(y < y3)
                {
                    b = GRAY;                                   /* 黑白方块底色 */
                }
                else
                {
                    b = BLACK;
                }
                pix[(u32)r * ROW_BYTES + 2 * x]     = (u8)(b >> 8);
                pix[(u32)r * ROW_BYTES + 2 * x + 1] = (u8)(b & 0xFF);
            }
        }

        LCD_PushBlock((u16)(s * SLICE_ROWS), slice_buf[s], rows);
    }

    LCD_WaitFrameDone();                        /* 等全屏刷完再补画方块/边框/文字 */

    /* 白/黑方块: 用来判断反相(补色)开关 */
    LCD_Fill(8, y2, 31, (u16)(y3 - 1), WHITE);
    LCD_Fill(40, y2, 63, (u16)(y3 - 1), BLACK);

    /* 1 像素白边框: 用来判断显存偏移 */
    LCD_Fill(0, 0, LCD_WIDTH - 1, 0, WHITE);
    LCD_Fill(0, LCD_HEIGHT - 1, LCD_WIDTH - 1, LCD_HEIGHT - 1, WHITE);
    LCD_Fill(0, 0, 0, LCD_HEIGHT - 1, WHITE);
    LCD_Fill(LCD_WIDTH - 1, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, WHITE);

    LCD_ShowString(2, y4, "CALIB", WHITE, BLACK);
}

/*********************************************************************
 * @fn      LCD_DiagShow
 *
 * @brief   参数诊断页(不用接串口也能看出问题在哪)
 *
 * @return  none
 */
static void LCD_DiagShow(void)
{
    char s[28];
    u8   i;
    u16  k, y;
    u32  se, so;
    u32  fps, rps;
    static u32 last_frame = 0, last_href = 0;

    /* 诊断页每 500ms 刷新一次 -> 差值乘 2 就是每秒的值
     * (这里是准的: 调用它的循环就是 Delay_Ms(500); 实时画面的左下角叠字用的是
     *  另一套"按行数结算"的算法, 见 SHOW_FPS 那一段) */
    fps = (frame_cnt - last_frame) * 2;
    rps = (href_cnt - last_href) * 2;
    last_frame = frame_cnt;
    last_href  = href_cnt;

    /* 行距按屏高分: 竖屏 160 -> 14, 横屏 128 -> 13; 共 10 行(最后一行只画一行高) */
    y = (u16)((LCD_HEIGHT - 12) / (DIAG_LINES - 1));

    /* 顺便量一下三通道平均值(诊断用): 偏色时这三个数一眼能看出来 */
    {
        u8 rr, gg, bb, yy0;

        CAM_MeasureStats(&rr, &gg, &bb, &yy0);
        ae_r = rr;  ae_g = gg;  ae_b = bb;  ae_luma = yy0;
    }

    LCD_Clear(BLACK);
    LCD_ShowString(0, 0, "-- DVP DIAG --", CYAN, BLACK);

    sprintf(s, "fps:%d r/s:%d", (int)fps, (int)rps);
    LCD_ShowString(0, (u16)(y * 1), s, GREEN, BLACK);       /* 帧率 / 行速率 */

    sprintf(s, "f:%d r/f:%d", (int)frame_cnt,
            (frame_cnt != 0) ? (int)(href_cnt / frame_cnt) : 0);
    LCD_ShowString(0, (u16)(y * 2), s, WHITE, BLACK);       /* 累计帧数 / 每帧行数(必须 320) */

    /* 丢行 / 时钟档位 / 曝光 / 增益(自动曝光时 = 回路当前用的值, A = 增益自动) */
#if (OV2640_AUTO_EXP == 1)
    sprintf(s, "drop:%d clk%02X %d/%02X", (int)row_drop_cnt, (int)OV2640_CLKRC,
            (int)ae_exp, (int)ae_gain);
#elif (OV2640_GAIN_VALUE == 0)
    sprintf(s, "drop:%d clk%02X %d/A", (int)row_drop_cnt, (int)OV2640_CLKRC,
            (int)OV2640_AEC_LINES);
#else
    sprintf(s, "drop:%d clk%02X %d/%02X", (int)row_drop_cnt, (int)OV2640_CLKRC,
            (int)OV2640_AEC_LINES, (int)OV2640_GAIN_VALUE);
#endif
    LCD_ShowString(0, (u16)(y * 3), s, WHITE, BLACK);

    sprintf(s, "flex:%d foc:%d", (int)fifo_ov_cnt,
            (int)((focus_rows != 0) ? (focus_sum / focus_rows) : 0));
    LCD_ShowString(0, (u16)(y * 4), s, YELLOW, BLACK);      /* FIFO 溢出 / 清晰度 */

    /* 偶/奇字节平均值: 判断像素格式
     *   RGB565 -> 两个都随亮度变
     *   YUV422 -> 奇数(色度)基本停在 128 附近 */
    se = 0;
    so = 0;
    for(k = 0; k < CAM_ROW_BYTES; k += 2)
    {
        se += dvp_row_buf0[k];
        so += dvp_row_buf0[k + 1];
    }
    se /= (CAM_ROW_BYTES / 2);
    so /= (CAM_ROW_BYTES / 2);
    /* lum = 自动曝光量到的亮度(滤波前)   wb = 正在施加的白平衡系数(256 = 1.0 倍, 不修色) */
#if (CAM_AWB == 1)
    sprintf(s, "lum:%d wb:%d/%d", (int)ae_luma, (int)wb_r, (int)wb_b);
#else
    sprintf(s, "lum:%d e:%d", (int)ae_luma, (int)se);
#endif
    LCD_ShowString(0, (u16)(y * 5), s, MAGENTA, BLACK);

    /* 行缓冲前 6 字节: 应随镜头画面变化 */
    for(i = 0; i < 6; i++)
    {
        sprintf(s + i * 3, "%02X ", dvp_row_buf0[i]);
    }
    LCD_ShowString(0, (u16)(y * 6), s, GREEN, BLACK);

    /* 三通道平均值: 判断偏色(发黄 = R/G 比 B 大很多; 发蓝 = B 大) */
    sprintf(s, "rgb:%d/%d/%d", (int)ae_r, (int)ae_g, (int)ae_b);
    LCD_ShowString(0, (u16)(y * 7), s, CYAN, BLACK);

    /* 取景框: 尺寸 @ 左上角坐标(在摄像头 240x320 输出画面里的位置) */
#if (CAM_FULL_VIEW == 1)
    sprintf(s, "view full %dx%d", CAM_WIDTH, CAM_HEIGHT);
#else
    sprintf(s, "view %dx%d @%d,%d", CAM_VIEW_W, CAM_VIEW_H, CROP_X, CROP_Y);
#endif
    LCD_ShowString(0, (u16)(y * 8), s, WHITE, BLACK);

    /* 画面开关状态: 拍照发人看时, 一眼就知道当前是哪套设置 */
    sprintf(s, "rot%d mh%d mv%d iv%d bgr%d",
            (int)LCD_ROTATE, (int)LCD_MIRROR_H, (int)LCD_MIRROR_V, (int)LCD_INVERT, (int)LCD_BGR_SWAP);
    LCD_ShowString(0, (u16)(y * 9), s, YELLOW, BLACK);
}

/*********************************************************************
 * @fn      DVP_Init
 *
 * @brief   DVP 初始化(8 位数据模式, RGB565, 240x320)
 *
 * @return  none
 */
static void DVP_Init(void)
{
    NVIC_InitTypeDef NVIC_InitStructure = {0};

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DVP, ENABLE);

    DVP->CR0 &= ~RB_DVP_MSK_DAT_MOD;
    DVP->CR0 |= RB_DVP_D8_MOD | RB_DVP_V_POLAR;     /* 8位数据模式, VSYNC 高有效 */

    DVP->CR1 &= ~((RB_DVP_ALL_CLR) | RB_DVP_RCV_CLR);

    DVP->ROW_NUM = CAM_HEIGHT;                      /* 320 行 */
    DVP->COL_NUM = CAM_ROW_BYTES;                   /* 每行 480 字节 */

    DVP->DMA_BUF0 = (u32)dvp_row_buf0;
    DVP->DMA_BUF1 = (u32)dvp_row_buf1;

    DVP->CR1 &= ~RB_DVP_FCRC;
    DVP->CR1 |= DVP_RATE_100P;                      /* 每帧都采集 */

    DVP->IER |= RB_DVP_IE_STP_FRM;
    DVP->IER |= RB_DVP_IE_FIFO_OV;
    DVP->IER |= RB_DVP_IE_FRM_DONE;
    DVP->IER |= RB_DVP_IE_ROW_DONE;
    DVP->IER |= RB_DVP_IE_STR_FRM;

    NVIC_InitStructure.NVIC_IRQChannel                   = DVP_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    DVP->CR1 |= RB_DVP_DMA_EN;
    DVP->CR0 |= RB_DVP_ENABLE;
}

/*********************************************************************
 * @fn      DVP_IRQHandler
 *
 * @brief   每收完一行: 统计清晰度, 需要时把该行裁到屏上
 *
 * @return  none
 */
void DVP_IRQHandler(void)
{
    if(DVP->IFR & RB_DVP_IF_ROW_DONE)
    {
        DVP->IFR &= ~RB_DVP_IF_ROW_DONE;            /* 写0清标志 */

        /* 取"刚收完"的那一行所在的缓冲区:
         *   DVP 收完本行后立刻转去写"另一个"缓冲区, 要再过一整行时间(>1ms)
         *   才会回到这一块; 而我们推屏只要约 0.2ms, 所以读它绝对安全,
         *   而且拿到的是最新的一行.
         *   奇偶对应关系: 第 0 行收完在 BUF0, 第 1 行在 BUF1, 依次交替. */
        {
            const u8 *fb;
            u16 src_row = (u16)row_cnt;                 /* 刚收完的就是这一行 */

            fb = ((row_cnt & 1) ? dvp_row_buf1 : dvp_row_buf0) + (CROP_X * 2);

            /* 清晰度统计(和出不出图无关, 诊断页要显示) */
            {
                u16 n;
#if (CAM_BYTE_SWAP == 1)
                /* 低字节先发 -> 原始 buffer 里高字节(亮度)在奇数下标 */
                for(n = 1; (u16)(n + 8) < (u16)ROW_BYTES; n += 8)
#else
                for(n = 0; (u16)(n + 8) < (u16)ROW_BYTES; n += 8)
#endif
                {
                    u8 a = fb[n];
                    u8 b = fb[n + 8];

                    focus_sum += (a > b) ? (u32)(a - b) : (u32)(b - a);
                }
                focus_rows++;
            }

            /* 先换算: 这一源行要不要显示? 显示到屏上第几行?
             * (纵向映射写成通用的按比例映射, 这样竖屏/横屏、全视野/裁剪都能用) */
            {
                u16 y     = 0;
                u8  show  = 0;

                if(video_on)
                {
#if (CAM_FULL_VIEW == 1)
                    /* 整幅缩到屏上: 320 行 -> 屏高 行, 按比例取(横屏 128 行时就是 2.5:1) */
                    u32 ya = ((u32)src_row * LCD_HEIGHT) / CAM_HEIGHT;
                    u32 yb = (((u32)src_row + 1) * LCD_HEIGHT) / CAM_HEIGHT;

                    if((src_row < CAM_HEIGHT) && (yb > ya) && (ya < LCD_HEIGHT))
                    {
                        y    = (u16)ya;
                        show = 1;
                    }
#else
                    /* 中心裁剪: 源行 src_row 对应屏上的第 (src_row - CROP_Y) 行, 1:1 */
                    if((src_row >= CROP_Y) && (src_row < (u32)(CROP_Y + LCD_HEIGHT)))
                    {
                        y    = (u16)(src_row - CROP_Y);
                        show = 1;
                    }
#endif
                }

                /* 把这一行存进对应"片", 攒满一片就通知主循环立刻刷.
                 * 两种情况要放弃这一行(记一次丢行), 否则会把屏上的画面改坏:
                 *   1) 这一片正在被 DMA 搬到屏上(改写 = 花屏条)
                 *   2) 这一片还等着主循环刷(改写 = 半新半旧的撕裂条) */
                if(show)
                {
                    u16 s = (u16)(y / SLICE_ROWS);
                    u16 r = (u16)(y % SLICE_ROWS);

                    if((slice_busy[s] == 0) && (slice_ready[s] == 0))
                    {
                        CAM_StoreRow(&slice_buf[s][(u32)r * ROW_BYTES], fb);

                        slice_rows[s]++;

                        if((r == (SLICE_ROWS - 1)) || (y == (u16)(LCD_HEIGHT - 1)))
                        {
                            slice_ready[s] = 1;     /* 这一片的最后一行到了 */
                        }
                    }
                    else
                    {
                        row_drop_cnt++;
                    }
                }
            }
        }

        row_cnt++;
        if(row_cnt >= CAM_HEIGHT)
        {
            row_cnt = 0;
        }
        href_cnt++;
    }

    if(DVP->IFR & RB_DVP_IF_FRM_DONE)
    {
        DVP->IFR &= ~RB_DVP_IF_FRM_DONE;
        row_cnt = 0;                                /* 新的一帧, 从第 0 行重新开始 */

        /* 一帧结束: 还没攒满的片也交出去(帧尾兜底).
         * 不加这段的话, 万一某帧漏了几行, 那一片就永远等不到"最后一行",
         * 屏上会留下一小条不再更新的旧画面. */
        {
            u16 s;

            for(s = 0; s < SLICE_NUM; s++)
            {
                if((slice_rows[s] > 0) && (slice_ready[s] == 0) && (slice_busy[s] == 0))
                {
                    slice_ready[s] = 1;
                }
            }
        }
    }

    if(DVP->IFR & RB_DVP_IF_STR_FRM)
    {
        DVP->IFR &= ~RB_DVP_IF_STR_FRM;
        frame_cnt++;
    }

    if(DVP->IFR & RB_DVP_IF_STP_FRM)
    {
        DVP->IFR &= ~RB_DVP_IF_STP_FRM;
    }

    if(DVP->IFR & RB_DVP_IF_FIFO_OV)
    {
        DVP->IFR &= ~RB_DVP_IF_FIFO_OV;
        fifo_ov_cnt++;
    }
}

/*********************************************************************
 * @fn      main
 *
 * @brief   主程序
 *
 * @return  none
 */
int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    SystemCoreClockUpdate();
    Delay_Init();
    DBG_INIT();

    DBG("SystemClk:%d\r\n", (int)SystemCoreClock);
    DBG("ChipID:%08x\r\n", (unsigned int)DBGMCU_GetCHIPID());

    /* ---- 0. LCD 初始化 + 检查 32KB 以上那块 RAM 是否存在 ---- */
    LCD_Init();
    LCD_BackLight(1);

    if(RAM_Check() == 0)
    {
        /* 芯片没配置成 FLASH-192K + RAM-128K, 整帧缓冲用不了 */
        LCD_Clear(BLACK);
        LCD_ShowString(0, 34, "RAM NOT 128K !", RED, BLACK);
        LCD_ShowString(0, 50, "WCH-LinkUtility", WHITE, BLACK);
        LCD_ShowString(0, 62, "set FLASH/RAM", WHITE, BLACK);
        LCD_ShowString(0, 74, "= 192K / 128K", YELLOW, BLACK);
        LCD_ShowString(0, 90, "then reset", WHITE, BLACK);
        DBG("RAM 128K not available!\r\n");
        while(1)
        {
            Delay_Ms(1000);
            DBG("RAM 128K not available!\r\n");
        }
    }

    /* ---- 1. 像素校准画面 ---- */
    LCD_CalibScreen();
    DBG("LCD ST7735S 128x160 ready\r\n");
    Delay_Ms(CALIB_SHOW_MS);

    /* ---- 2. 摄像头初始化 ---- */
    while(OV2640_Init())
    {
        LCD_Clear(BLACK);
        LCD_ShowString(4, 74, "CAMERA ERR", RED, BLACK);
        DBG("Camera Err\r\n");
        Delay_Ms(1000);
    }
    DBG("Camera Suc\r\n");

    /* 摄像头: 只用 WCH 原例程验证过的配置(输出 240x320, 屏上取中间一块) */
    RGB565_Mode_Init();

#if (OV2640_SVGA_MODE == 1)
    /* 换成 SVGA 栅格: 帧率约 4 倍(30fps@24MHz / 60fps@48MHz), 但一帧时间变短 -> 暗处更暗 */
    OV2640_SetRaster(1);
#else
    OV2640_SetRaster(0);
#endif

    OV2640_Speed_Set(OV2640_PCLK_DIV, OV2640_CLKRC);   /* 输出 PCLK + 帧率时钟 */

#if (OV2640_LOW_LATENCY == 1)
    /* 初始曝光 + 增益: 之后由软件自动曝光(见 OV2640_AUTO_EXP)自己微调 */
    OV2640_LowLatency((UINT8)OV2640_AEC_LINES, OV2640_GAIN_VALUE);
#endif

#if (CAM_AWB == 1)
    CAM_WbBuildLut();       /* 白平衡系数表先按 1.0 倍初始化 */
#endif

    Delay_Ms(500);

    LCD_Clear(BLACK);
    LCD_ShowString(4, 74, "CAMERA OK", GREEN, BLACK);
    Delay_Ms(600);

    /* ---- 3. 启动 DVP, 先显示参数诊断页 ---- */
    DVP_Init();
    DBG("DVP streaming...\r\n");
    Delay_Ms(1200);                                     /* 等第一帧, 否则全是 0 */

#if (SHOW_FPS == 1)
    /* 标定"每毫秒收多少行"(摄像头的行速率由晶振决定, 用它当计时基准最准,
     * 这样算出来的 fps 不受主循环里做了多少事的影响) */
    {
        u32 r0 = href_cnt;

        Delay_Ms(500);
        fps_row_per_ms = (href_cnt - r0) / 500;
        if(fps_row_per_ms == 0)
        {
            fps_row_per_ms = 1;                         /* 没数据时别除零 */
        }
    }
#endif

    {
        u32 t;
        for(t = 0; t < (DIAG_SHOW_MS / 500); t++)
        {
#if (OV2640_AUTO_EXP == 1)
            CAM_AutoExposure();                         /* 诊断页上就能看到自动曝光在动 */
#endif
            LCD_DiagShow();
            Delay_Ms(500);
        }
    }

    /* ---- 4. 实时画面: 分片刷屏(每攒满 10 行就立刻刷出去) ----
     *   和"整帧攒满才刷"相比, 屏幕内容永远是刚收下来的,
     *   延迟从"半帧~一帧"降到"几十毫秒", 而且画面是连续更新的, 不会跳. */
    video_on = 1;

    {
        u16 tick = 0;
#if (OV2640_AUTO_EXP == 1)
        u16 ae_tick = 0;
#endif
#if (SHOW_FPS == 1)
        u32 fps_base_href  = href_cnt;      /* 上一次结算时的行数 */
        u32 fps_base_frame = frame_cnt;     /* 上一次结算时的帧数 */
        u32 fps_win_rows   = (u32)fps_row_per_ms * FPS_WIN_MS;   /* 一个窗口对应多少行 */
        char s1[28];
#endif

        while(1)
        {
            u16 s;

#if (SHOW_FPS == 1)
            /* 用"行数"当计时基准结算帧率: 每积累一个窗口(默认 1000ms 对应的行数)
             * 才算一次差. 之前那种"每次循环都算、还乘 2"是错的 —— 循环一次只有
             * 约 1ms, 帧数差基本是 0, 所以永远显示 0 或 2. */
            if((href_cnt - fps_base_href) >= fps_win_rows)
            {
                fps_base_href += fps_win_rows;
                fps_val = (frame_cnt - fps_base_frame) * 1000u / FPS_WIN_MS;
                fps_base_frame = frame_cnt;
                fps_rps = fps_row_per_ms * 1000u;
            }
#endif

            /* 每约 200ms 走一步软件自动曝光(测亮度 -> 调曝光/增益) */
#if (OV2640_AUTO_EXP == 1)
            if(++ae_tick >= 200)
            {
                ae_tick = 0;
                CAM_AutoExposure();
            }
#endif

            /* 上一片搬完了就把它标回"空闲", 中断才敢往里写 */
            if((slice_inflight != 0xFF) && (LCD_PushBusy() == 0))
            {
                slice_busy[slice_inflight] = 0;
                slice_inflight = 0xFF;
            }

            /* 顺序找攒满的片, 立刻刷. 同一时刻只允许一片在搬(DMA 只有一路),
             * 所以碰到"还在搬"就等下一轮(1ms 后), 不会丢片. */
            for(s = 0; s < SLICE_NUM; s++)
            {
                if(slice_ready[s])
                {
                    if(LCD_PushBusy())
                    {
                        break;
                    }

                    /* 先立"正在搬"的牌子再开 DMA: 中断就会主动避开这一片,
                     * 不会出现"边搬边改"的花屏条 */
                    slice_busy[s]  = 1;
                    slice_inflight = (u8)s;

                    slice_ready[s] = 0;
                    slice_rows[s]  = 0;     /* 这一片交出去了, 行数清零重新攒 */

#if (SHOW_FPS == 1)
                    /* 左下角帧率叠字: 直接画进这一片缓冲(马上要被 DMA 搬走),
                     * 所以它会跟着画面一起上屏, 不会被下一次刷新冲掉.
                     * 文字是 12 行高, 可能横跨两片 -> 两片各自画自己那部分
                     * (LCD_BufShowChar 会自动裁掉超出的行, 不会写到隔壁片去). */
                    {
                        s16 y0    = (s16)(LCD_HEIGHT - 12);         /* 叠字起始行(屏坐标) */
                        s16 first = (s16)(s * SLICE_ROWS);          /* 这一片的第一行 */
                        s16 yl    = (s16)(y0 - first);              /* 在这一片里的起始行 */
                        u16 rows  = (u16)SLICE_ROWS_OF(s);

                        if((yl > -12) && (yl < (s16)rows))
                        {
                            const char *p = s1;
                            u16 x = 0;

                            sprintf(s1, "fps:%d", (int)fps_val);
                            while(*p && ((x + 6) <= LCD_WIDTH))
                            {
                                LCD_BufShowChar(slice_buf[s], ROW_BYTES, rows, yl, x, *p, GREEN, BLACK);
                                x = (u16)(x + 6);
                                p++;
                            }
                        }
                    }
#endif

                    LCD_PushBlock((u16)(s * SLICE_ROWS), slice_buf[s], SLICE_ROWS_OF(s));
                }
            }

            if(++tick >= 1000)              /* 约 1 秒打印一次 */
            {
                tick = 0;
                DBG("frame:%d  href:%d  drop:%d  fifo_ov:%d\r\n",
                    (int)frame_cnt, (int)href_cnt, (int)row_drop_cnt, (int)fifo_ov_cnt);
            }

            Delay_Ms(1);
        }
    }
}
