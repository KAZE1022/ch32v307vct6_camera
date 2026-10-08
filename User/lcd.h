/********************************** (C) COPYRIGHT *******************************
* File Name          : lcd.h
* Author             : WCH / 修改: 适配 1.8寸 128x160 ST7735S (SPI)
* Version            : V2.0.0
* Date               : 2025/04/06
* Description        : 1.8" 128x160 ST7735S (4线 SPI) 驱动头文件.
*
*  接线:  VCC->3.3V  GND->GND   SCL->PB13(SPI2_SCK)  SDA->PB15(SPI2_MOSI)
*         RST->PC4    DC->PB14   CS->PB12              BLK->PC5(高电平点亮)
*********************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/
#ifndef __LCD_H
#define __LCD_H

#include "debug.h"
#include "stdlib.h"

#define  delay_ms(x)  Delay_Ms(x)
#define  delay_us(x)  Delay_Us(x)

/*==============================================================================
 *  一、画面显示开关(整屏级)
 *  这一组全是"零成本"的: 只改面板寄存器(MADCTL/INVON), 不占 CPU、不影响帧率,
 *  而且连诊断页文字、校准画面一起变(整块屏一起转/翻/反相).
 *  只想翻摄像头画面、让屏上的文字保持可读 -> 用 main.c 里的 CAM_MIRROR_X.
 *============================================================================*/
/* 1) 旋转: 0 / 90 / 180 / 270 (度)
 *    90/270 会把屏幕变成横向(宽160 高128), 取图窗口/文字排版会自动跟着改 */
#ifndef LCD_ROTATE
#define LCD_ROTATE       0
#endif

/* 2) 左右镜像   3) 上下镜像   (0 = 不翻, 1 = 翻)
 *    是在"旋转之后"的画面基础上再翻.
 *    注: 旋转 90/270 时画面本身转了 90 度, 所以这两开关的"左右/上下"会互换,
 *        转完发现左右反了却像是上下反了, 就用另一个开关. */
#ifndef LCD_MIRROR_H
#define LCD_MIRROR_H     0
#endif
#ifndef LCD_MIRROR_V
#define LCD_MIRROR_V     0
#endif

/* 4) 补色(整屏反相, 像底片): 0 = 正常, 1 = 反相 */
#ifdef LCD_USE_INVON                        /* 兼容旧名字 */
#undef  LCD_INVERT
#define LCD_INVERT       LCD_USE_INVON
#endif
#ifndef LCD_INVERT
#define LCD_INVERT       0
#endif

/* 5) 红蓝互换(MADCTL 的 BGR 位): 红蓝对调时置 1 */
#ifndef LCD_BGR_SWAP
#define LCD_BGR_SWAP     0
#endif

/* 面板物理分辨率(竖着看)与旋转后的逻辑分辨率 */
#define LCD_PANEL_W      128
#define LCD_PANEL_H      160

#if ((LCD_ROTATE == 90) || (LCD_ROTATE == 270))
#define LCD_WIDTH        LCD_PANEL_H             /* 160, 横向 */
#define LCD_HEIGHT       LCD_PANEL_W             /* 128 */
#else
#define LCD_WIDTH        LCD_PANEL_W             /* 128, 竖向 */
#define LCD_HEIGHT       LCD_PANEL_H             /* 160 */
#endif

/* 显存偏移(画面整体偏 1~2 像素时调): 1.8寸 ST7735S 常见是 (0,0) 或 (2,1) */
#define LCD_XSTART       0
#define LCD_YSTART       0

/* 扫描方向(MADCTL 0x36)由上面 1)~5) 自动算出来.
 * 想直接写寄存器值(高级用法): 自己定义 LCD_MADCTL 即可覆盖.
 *   0x00=正常  0x40=左右镜像  0x80=上下翻转  0xC0=180度
 *   0x20=转置   0x60=顺时针90度  0xA0=逆时针90度
 * 位3(0x08) 是 BGR 位; 位2(0x04)/位4(0x10) 是刷新顺序, 一般不用动. */
#ifndef LCD_MADCTL
#if   (LCD_ROTATE == 90)
#define LCD_MADCTL_ROT   0x60                    /* MV|MX = 顺时针 90 度 */
#elif (LCD_ROTATE == 180)
#define LCD_MADCTL_ROT   0xC0                    /* MX|MY = 180 度 */
#elif (LCD_ROTATE == 270)
#define LCD_MADCTL_ROT   0xA0                    /* MV|MY = 逆时针 90 度 */
#else
#define LCD_MADCTL_ROT   0x00
#endif

#define LCD_MADCTL       (LCD_MADCTL_ROT                      | \
                          ((LCD_MIRROR_H  != 0) ? 0x40 : 0x00) | \
                          ((LCD_MIRROR_V  != 0) ? 0x80 : 0x00) | \
                          ((LCD_BGR_SWAP  != 0) ? 0x08 : 0x00))
#endif

/*==============================================================================
 *  二、引脚配置
 *============================================================================*/
#define LCD_SPIx          SPI2

#define LCD_SCK_PORT      GPIOB                   /* 模块 SCL */
#define LCD_SCK_PIN       GPIO_Pin_13             /* SPI2_SCK  */

#define LCD_MOSI_PORT     GPIOB                   /* 模块 SDA */
#define LCD_MOSI_PIN      GPIO_Pin_15             /* SPI2_MOSI */

#define LCD_CS_PORT       GPIOB                   /* 模块 CS  */
#define LCD_CS_PIN        GPIO_Pin_12

#define LCD_DC_PORT       GPIOB                   /* 模块 DC  */
#define LCD_DC_PIN        GPIO_Pin_14

#define LCD_RST_PORT      GPIOC                   /* 模块 RST */
#define LCD_RST_PIN       GPIO_Pin_4

#define LCD_BLK_PORT      GPIOC                   /* 模块 BLK */
#define LCD_BLK_PIN       GPIO_Pin_5

/* SPI 时钟 = PCLK1(48MHz) / 分频
 *   _2 -> 24MHz(默认; 整帧 128x160 推屏只要约 14ms, 小于面板扫一屏的 16.7ms,
 *              所以连细撕裂都不会有. 略超 ST7735S 手册的 15MHz, 但短线一般没问题)
 *   _4 -> 12MHz(整帧约 27ms; 出现花屏/错位就换回这个) */
#ifndef LCD_SPI_PRESCALER
#define LCD_SPI_PRESCALER SPI_BaudRatePrescaler_2
#endif

/*==============================================================================
 *  三、引脚操作宏
 *============================================================================*/
#define LCD_CS_L()        GPIO_ResetBits(LCD_CS_PORT, LCD_CS_PIN)
#define LCD_CS_H()        GPIO_SetBits(LCD_CS_PORT,   LCD_CS_PIN)
#define LCD_DC_L()        GPIO_ResetBits(LCD_DC_PORT, LCD_DC_PIN)
#define LCD_DC_H()        GPIO_SetBits(LCD_DC_PORT,   LCD_DC_PIN)
#define LCD_BLK_ON()      GPIO_SetBits(LCD_BLK_PORT,  LCD_BLK_PIN)
#define LCD_BLK_OFF()     GPIO_ResetBits(LCD_BLK_PORT,LCD_BLK_PIN)

/*==============================================================================
 *  四、颜色定义(RGB565)
 *============================================================================*/
#define WHITE            0xFFFF
#define BLACK            0x0000
#define BLUE             0x001F
#define RED              0xF800
#define GREEN            0x07E0
#define CYAN             0x07FF
#define MAGENTA          0xF81F
#define YELLOW           0xFFE0
#define GRAY             0x8430
#define BROWN            0xBC40
#define BRRED            0xFC07
#define DARKBLUE         0x01CF
#define LIGHTBLUE        0x7D7C
#define GRAYBLUE         0x5458
#define LIGHTGREEN       0x841F
#define LGRAY            0xC618
#define LGRAYBLUE        0xA651
#define LBBLUE           0x2B12

/*==============================================================================
 *  五、对外接口
 *============================================================================*/
void LCD_Init(void);
void LCD_BackLight(u8 on);
void LCD_WriteCmd(u8 cmd);
void LCD_WriteCmdParam(u8 cmd, const u8 *param, u16 len);
void LCD_WriteData16(u16 data);
void LCD_SetWindow(u16 xs, u16 ys, u16 xe, u16 ye);
void LCD_SetCursor(u16 x, u16 y);
void LCD_Clear(u16 color);
void LCD_Fill(u16 xs, u16 ys, u16 xe, u16 ye, u16 color);
void LCD_ShowChar(u16 x, u16 y, char ch, u16 fc, u16 bc);
void LCD_ShowString(u16 x, u16 y, const char *str, u16 fc, u16 bc);

/* 把每个像素的两个字节交换(OV2640 低字节先发 -> ST7735S 要高字节先发) */
void LCD_SwapPixelBytes(const u8 *src, u8 *dst, u16 pixels);

/* 刷一个"横条": 把 buf 里的 rows 行像素写到屏幕第 y 行开始的位置.
 * 用 SPI+DMA 发送, 不等发完就返回(内部会先等上一次发完).
 * 分片刷屏就是用这个: 每攒满一小段就立刻刷出去, 屏幕连续更新、延迟感小. */
void LCD_PushBlock(u16 y, const u8 *buf, u16 rows);

/* 那段横条还在搬吗? ==1 时 DMA 正在读 buf, 这块内存不能被改写(会花屏) */
u8   LCD_PushBusy(void);

/* 上一帧是否已发完; ==1 时才能再调用 LCD_PushFrame / 画其他东西 */
u8   LCD_FrameDone(void);
void LCD_WaitFrameDone(void);

#endif
