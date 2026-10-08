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

/* 摄像头像素字节序: OV2640 的 RGB565 是"低字节先发"(WCH 表里 0xDA=0x09 =
 * RGB565 | LBYTE_FIRST), 而 ST7735S 要求高字节先发, 所以必须交换一次.
 * 正常保持 1; 万一颜色变成另一种错法再试 0 */
#ifndef CAM_BYTE_SWAP
#define CAM_BYTE_SWAP       1
#endif

/* 画面左右镜像: 1 = 摄像头画面水平翻转(左右对调)显示, 0 = 正常
 * 只影响摄像头画面; 屏幕上的诊断文字/校准画面**不受影响**(保持可读).
 * 做法是在把每一行存进帧缓冲时顺便反过来, 不额外花时间、不影响帧率.
 * 注: 如果想让整屏(含文字)一起镜像, 改成改 lcd.h 里的 LCD_MADCTL = 0x40 */
#ifndef CAM_MIRROR_X
#define CAM_MIRROR_X        0
#endif

/* ISP 整数降采样档位现在由 OV2640_Config() 按"降采样后尺寸 >= 输出尺寸"
 * 自动挑选, 不需要手工设置 */

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
 *   默认跟着上面自动走, 让 PCLK 恒等于 12MHz(现在验证过没问题的值):
 *     CLKRC=0x01 -> 1     CLKRC=0x00 -> 2     CLKRC=0x80 -> 4
 *   想让 DVP 采样更宽松(更慢但更稳)可以手动调大; 调太小会花屏/错位 */
#ifndef OV2640_PCLK_DIV
#define OV2640_PCLK_DIV      OV2640_CLK_X
#endif

/* 低延迟模式(固定短曝光 + 自动增益):
 *   画面延迟 ≈ 一帧时间 ≈ 曝光时间. 暗处自动曝光会把帧时间拉到几百毫秒,
 *   这就是"晃动时明显延迟"的主因. 打开本项后延迟可以压到几十毫秒,
 *   代价是暗处噪点变多(亮度由自动增益补).
 *   0 = 保持原厂自动曝光(暗处延迟大)      1 = 低延迟(默认) */
#ifndef OV2640_LOW_LATENCY
#define OV2640_LOW_LATENCY   1
#endif

/* 固定曝光值: 越小延迟越低、画面越暗. 建议范围 1~32
 *   太暗 -> 加大(16/24/32)   延迟还是大 -> 减小(4/2)
 *   注: 真正写进摄像头的值是这个数再乘时钟倍率(见上面 OV2640_CLK_X),
 *       因为帧率一快, 单位"行"的时间就短了, 必须按倍数补回来 */
#ifndef OV2640_AEC_VALUE
#define OV2640_AEC_VALUE     16
#endif

/* 实际写入摄像头的曝光值(自动乘倍数并限制在 8 位内) */
#define OV2640_AEC_OUT       ((((u32)OV2640_AEC_VALUE * OV2640_CLK_X) > 255) ? 255 : \
                              ((u32)OV2640_AEC_VALUE * OV2640_CLK_X))

/* 手动增益(和曝光配合决定亮度): 0x01~0xFF, 越大越亮也越噪
 *   画面太暗 -> 加大(0x80 / 0xC0 / 0xFF)
 *   画面发白/过曝 -> 减小(0x20 / 0x10)
 *   填 0 = 增益保持自动(只固定曝光) */
#ifndef OV2640_GAIN_VALUE
#define OV2640_GAIN_VALUE    0xCF
#endif

/* 读出窗口实验开关(帧率的潜在总开关, 但目前还没调通):
 *   0 = 【默认, 已知可用】用 WCH 原配置: 输出 240x320 + 屏上取中心 128x160
 *       row/f 应该是 320, 画面正常, 帧率约 4fps
 *   1 = 【实验】读出窗口 + DSP 输入尺寸 + 输出 128x160 全套
 *       实测 row/f 变成 14529 -> 说明设置没生效(摄像头还在输出整幅), 画面会花
 *   2 = 【实验, 最可能有用】只改传感器读出窗口, DSP 和输出尺寸都保持 WCH 原样
 *       (WCH 自己就是"窗口 800x600 + DSP 认为 1600x1200"的错配配置且能正常出图,
 *        所以只改窗口最可能生效: 读的行数少了, 帧率就该上去)
 * 先试 2, 记下 fps / row/f / 画面是否正常; 不行再试 1; 都能回到 0。 */
#ifndef OV2640_READOUT_OPT
#define OV2640_READOUT_OPT   0
#endif

#if (OV2640_READOUT_OPT == 0)
#define CAM_OUT_W            240      /* 老配置: 输出 240x320 ... */
#define CAM_OUT_H            320
#else
#define CAM_OUT_W            LCD_WIDTH   /* 新配置: 输出 = 屏尺寸 */
#define CAM_OUT_H            LCD_HEIGHT
#endif

/* 传感器内部读出窗口(仅 OPT=1/2 时使用), 宽必须是 16 的倍数, 高必须是 8 的倍数 */
#ifndef OV2640_READOUT_W
#define OV2640_READOUT_W     424
#endif
#ifndef OV2640_READOUT_H
#define OV2640_READOUT_H     300
#endif

/*==============================================================================
 *  摄像头参数和屏幕的匹配(视野开关)
 *  摄像头输出 240x320(3:4), 屏是 128x160(4:5), 两者不是同一个比例, 两种做法:
 *
 *   0 = 【默认, 最清楚】中心 1:1 裁剪: 取画面正中间的 128x160 原样上屏.
 *       一个摄像头像素 = 一个屏像素, 不缩放、不模糊、不花 CPU、不影响帧率.
 *       代价: 只看得到摄像头视野中心约 1/4 的面积(四周被裁掉了).
 *       (裁哪一块看 CROP_X / CROP_Y, 默认居中)
 *
 *   1 = 全视野 2:1 降采样: 把整幅 240x320 缩到屏上(横向 240->128, 纵向 320->160).
 *       看到的是摄像头的**全部视野**, 一点不裁.
 *       代价: 画面变"糊/有锯齿"(最近邻取样, 没做平均), 而且横向是 1.875:1
 *       的非整数缩放, 竖线会略有不均匀. 帧率不受影响.
 *============================================================================*/
#ifndef CAM_FULL_VIEW
#define CAM_FULL_VIEW        0
#endif

/*==============================================================================
 *  摄像头取图与裁剪
 *============================================================================*/
#define CAM_WIDTH             CAM_OUT_W                    /* 240 */
#define CAM_HEIGHT            CAM_OUT_H                    /* 320 */
#define CAM_ROW_BYTES         (CAM_WIDTH * 2)              /* 480 */

/* 屏上显示摄像头画面的"中间 128x160"(只有输出大于屏尺寸时才需要裁剪) */
#if (CAM_FULL_VIEW == 1)
#define CROP_X                0
#define CROP_Y                0
#define CAM_ROW_SRC_W         CAM_WIDTH                   /* 这一屏行横跨整行 240 个源像素 */
#else
/* 显示窗口在摄像头输出画面里的位置(默认居中).
 * 想自己指定就打开下面两行(单位: 摄像头输出像素, 0 = 最左/最上):
 *   #define CROP_X   0
 *   #define CROP_Y   0                                                       */
#ifndef CROP_X
#define CROP_X                ((CAM_WIDTH  > LCD_WIDTH ) ? ((CAM_WIDTH  - LCD_WIDTH ) / 2) : 0)
#endif
#ifndef CROP_Y
#define CROP_Y                ((CAM_HEIGHT > LCD_HEIGHT) ? ((CAM_HEIGHT - LCD_HEIGHT) / 2) : 0)
#endif
#define CAM_ROW_SRC_W         LCD_WIDTH                   /* 这一屏行横跨 128 个源像素(1:1) */
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
static volatile u16 frame_rows = 0;               /* 本帧已收行数(诊断用) */

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

void DVP_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

/*********************************************************************
 * @fn      CAM_StoreRow
 *
 * @brief   把摄像头的一行处理并存进"片"缓冲:
 *            1) 换像素字节序(OV2640 低字节先发 -> 屏要高字节先发)
 *            2) 可选左右镜像(CAM_MIRROR_X)
 *            3) 可选整幅缩到屏上(CAM_FULL_VIEW: 横向 240->128 最近邻)
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
        const u8 *s;

        acc -= step;
        s = src + ((acc >> 8) << 1);
#if (CAM_BYTE_SWAP == 1)
        dst[2 * n]     = s[1];
        dst[2 * n + 1] = s[0];
#else
        dst[2 * n]     = s[0];
        dst[2 * n + 1] = s[1];
#endif
    }
#else
    acc = 0;
    for(n = 0; n < LCD_WIDTH; n++)
    {
        const u8 *s = src + ((acc >> 8) << 1);

#if (CAM_BYTE_SWAP == 1)
        dst[2 * n]     = s[1];
        dst[2 * n + 1] = s[0];
#else
        dst[2 * n]     = s[0];
        dst[2 * n + 1] = s[1];
#endif
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
    static u32 last_frame = 0, last_href = 0;
    u32 fps, rps;

    /* 每 500ms 刷新一次 -> 差值乘 2 就是每秒的值(直接反映帧率/行速率) */
    fps = (frame_cnt - last_frame) * 2;
    rps = (href_cnt - last_href) * 2;
    last_frame = frame_cnt;
    last_href  = href_cnt;

    /* 行距按屏高分: 竖屏 160 -> 14, 横屏 128 -> 13; 共 10 行(最后一行只画一行高) */
    y = (u16)((LCD_HEIGHT - 12) / (DIAG_LINES - 1));

    LCD_Clear(BLACK);
    LCD_ShowString(0, 0, "-- DVP DIAG --", CYAN, BLACK);

    sprintf(s, "fps:%d r/s:%d", (int)fps, (int)rps);
    LCD_ShowString(0, (u16)(y * 1), s, GREEN, BLACK);       /* 帧率 / 行速率 */

    sprintf(s, "f:%d r/f:%d", (int)frame_cnt,
            (frame_cnt != 0) ? (int)(href_cnt / frame_cnt) : 0);
    LCD_ShowString(0, (u16)(y * 2), s, WHITE, BLACK);       /* 累计帧数 / 每帧行数(必须 320) */

    sprintf(s, "drop:%d clk%02X", (int)row_drop_cnt, (int)OV2640_CLKRC);
    LCD_ShowString(0, (u16)(y * 3), s, WHITE, BLACK);       /* 丢行数 / 摄像头时钟档位 */

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
    sprintf(s, "e:%d o:%d", (int)se, (int)so);
    LCD_ShowString(0, (u16)(y * 5), s, MAGENTA, BLACK);

    /* 行缓冲前 6 字节: 应随镜头画面变化 */
    for(i = 0; i < 6; i++)
    {
        sprintf(s + i * 3, "%02X ", dvp_row_buf0[i]);
    }
    LCD_ShowString(0, (u16)(y * 6), s, GREEN, BLACK);

    for(i = 0; i < 6; i++)
    {
        sprintf(s + i * 3, "%02X ", dvp_row_buf1[i]);
    }
    LCD_ShowString(0, (u16)(y * 7), s, GREEN, BLACK);

    sprintf(s, "%dx%d of %dx%d", LCD_WIDTH, LCD_HEIGHT, CAM_WIDTH, CAM_HEIGHT);
    LCD_ShowString(0, (u16)(y * 8), s, WHITE, BLACK);       /* 屏尺寸 / 摄像头输出尺寸 */

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
                        frame_rows++;

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
        frame_rows = 0;

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

    /* 摄像头: 三种配置(见文件顶部 OV2640_READOUT_OPT 的说明) */
#if (OV2640_READOUT_OPT == 0)
    RGB565_Mode_Init();                         /* WCH 原配置: 240x320 输出 + 中心裁剪 */
#elif (OV2640_READOUT_OPT == 1)
    OV2640_RGB565_Mode();
    OV2640_Config(OV2640_READOUT_W, OV2640_READOUT_H, CAM_WIDTH, CAM_HEIGHT, 1);
#else
    OV2640_RGB565_Mode();
    RGB565_Mode_Init();                         /* 先按原样配好... */
    OV2640_Config(OV2640_READOUT_W, OV2640_READOUT_H, 0, 0, 0);   /* 再只改读出窗口 */
#endif
    OV2640_Speed_Set(OV2640_PCLK_DIV, OV2640_CLKRC);   /* 输出 PCLK + 帧率时钟 */

#if (OV2640_LOW_LATENCY == 1)
    /* 固定曝光 + 增益: 把帧时间(也就是延迟)压下来, 亮度用曝光/增益补 */
    OV2640_LowLatency((UINT8)OV2640_AEC_OUT, OV2640_GAIN_VALUE);
#endif

    Delay_Ms(500);

    LCD_Clear(BLACK);
    LCD_ShowString(4, 74, "CAMERA OK", GREEN, BLACK);
    Delay_Ms(600);

    /* ---- 3. 启动 DVP, 先显示参数诊断页 ---- */
    DVP_Init();
    DBG("DVP streaming...\r\n");
    Delay_Ms(1200);                                     /* 等第一帧, 否则全是 0 */

    {
        u32 t;
        for(t = 0; t < (DIAG_SHOW_MS / 500); t++)
        {
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

        while(1)
        {
            u16 s;

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
