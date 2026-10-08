/********************************** (C) COPYRIGHT *******************************
* File Name          : ov.h
* Author             : WCH
* Version            : V1.0.0
* Date               : 2021/06/06
* Description        : This file contains the headers of the OV2640.
*********************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/
#ifndef __OV_H
#define __OV_H

#include "debug.h"

#ifndef NULL
#define NULL                    0
#endif

#ifndef VOID
#define VOID                    void
#endif
#ifndef CONST
#define CONST                   const
#endif
#ifndef BOOL
typedef unsigned char           BOOL;
#endif
#ifndef BOOLEAN
typedef unsigned char           BOOLEAN;
#endif
#ifndef CHAR
typedef char                    CHAR;
#endif
#ifndef INT8
typedef char                    INT8;
#endif
#ifndef INT16
typedef short                   INT16;
#endif
#ifndef INT32
typedef long                    INT32;
#endif
#ifndef UINT8
typedef unsigned char           UINT8;
#endif
#ifndef UINT16
typedef unsigned short          UINT16;
#endif
#ifndef UINT32
typedef unsigned long           UINT32;
#endif
#ifndef UINT8V
typedef unsigned char volatile  UINT8V;
#endif
#ifndef UINT16V
typedef unsigned short volatile UINT16V;
#endif
#ifndef UINT32V
typedef unsigned long volatile  UINT32V;
#endif

#ifndef PVOID
typedef void                    *PVOID;
#endif
#ifndef PCHAR
typedef char                    *PCHAR;
#endif
#ifndef PCHAR
typedef const char              *PCCHAR;
#endif
#ifndef PINT8
typedef char                    *PINT8;
#endif
#ifndef PINT16
typedef short                   *PINT16;
#endif
#ifndef PINT32
typedef long                    *PINT32;
#endif
#ifndef PUINT8
typedef unsigned char           *PUINT8;
#endif
#ifndef PUINT16
typedef unsigned short          *PUINT16;
#endif
#ifndef PUINT32
typedef unsigned long           *PUINT32;
#endif
#ifndef PUINT8V
typedef volatile unsigned char  *PUINT8V;
#endif
#ifndef PUINT16V
typedef volatile unsigned short *PUINT16V;
#endif
#ifndef PUINT32V
typedef volatile unsigned long  *PUINT32V;
#endif


/* 摄像头 RGB565 输出尺寸: 240x320
 * (就是 WCH 原例程验证过的配置; 屏上取中间 128x160 纯裁剪, 不做缩放) */
#define RGB565_ROW_NUM   320
#define RGB565_COL_NUM   480   //Col * 2
#define OV2640_RGB565_HEIGHT   320
#define OV2640_RGB565_WIDTH	   240

//JPEG PIXEL 1024 * 768
#define OV2640_JPEG_HEIGHT	768
#define OV2640_JPEG_WIDTH	1024


//PC13-RESET  PC3-PWDN
#define OV_RESET_SET   {GPIOC->BSHR = GPIO_Pin_13;}
#define OV_RESET_CLR   {GPIOC->BCR = GPIO_Pin_13;}
#define OV_PWDN_SET    {GPIOC->BSHR = GPIO_Pin_3;}
#define OV_PWDN_CLR    {GPIOC->BCR = GPIO_Pin_3;}

//OV2640��ID
#define SCCB_ID   			    0X60
#define OV2640_MID				0X7FA2
#define OV2640_PID				0X2642

#define IIC_SCL_IN  {GPIOB->CFGHR&=0XFFFFF0FF;GPIOB->CFGHR|=8<<8;}
#define IIC_SCL_OUT {GPIOB->CFGHR&=0XFFFFF0FF;GPIOB->CFGHR|=3<<8;}
#define IIC_SDA_IN  {GPIOB->CFGHR&=0XFFFF0FFF;GPIOB->CFGHR|=8<<12;}
#define IIC_SDA_OUT {GPIOB->CFGHR&=0XFFFF0FFF;GPIOB->CFGHR|=3<<12;}

#define IIC_SDA_SET   {GPIOB->BSHR = GPIO_Pin_11;}
#define IIC_SDA_CLR   {GPIOB->BCR = GPIO_Pin_11;}
#define IIC_SCL_SET   {GPIOB->BSHR = GPIO_Pin_10;}
#define IIC_SCL_CLR   {GPIOB->BCR = GPIO_Pin_10;}

//SDA In
#define SDA_IN_R      (GPIOB->INDR & GPIO_Pin_11)


void SCCB_GPIO_Init(void);
void SCCB_Start(void);
void SCCB_Stop(void);
void SCCB_No_Ack(void);
UINT8 SCCB_WR_Byte(UINT8 data);
UINT8 SCCB_RD_Byte(void);
UINT8 SCCB_WR_Reg(UINT8 Reg_Adr,UINT8 Reg_Val);
UINT8 SCCB_RD_Reg(UINT8 Reg_Adr);

void RGB565_Mode_Init(void);
void JPEG_Mode_Init(void);
UINT8 OV2640_Init(void);
void OV2640_JPEG_Mode(void);
void OV2640_RGB565_Mode(void);
UINT8 OV2640_OutSize_Set(UINT16 Image_width,UINT16 Image_height);
void OV2640_Speed_Set(UINT8 Pclk_Div, UINT8 Xclk_Div);

/* 输出尺寸设置(完整版): 除 zoom 窗口(0x5A/0x5B/0x5C) 外,
 * 还要设 CTRLI(0x50) 的整数降采样档位 V_Div/H_Div (0=1x,1=2x,2=4x,3=8x).
 * 约束: 降采样后的处理尺寸必须 >= 输出尺寸, 否则会被截短 */
UINT8 OV2640_OutSize_Set2(UINT16 Image_width, UINT16 Image_height,
                          UINT8 V_Div, UINT8 H_Div);

/* 设置传感器内部"读出窗口"(居中); UpdateDsp=1 时同时把 DSP 输入尺寸也改成这块窗口,
 * UpdateDsp=0 时只改窗口(DSP/输出尺寸保持原样, 用于单独验证窗口是否生效).
 *   WinW - 窗口宽(必须是 16 的倍数, 建议 >= 输出宽)
 *   WinH - 窗口高(必须是 8 的倍数,  建议 >= 输出高) */
UINT8 OV2640_SetReadout(UINT16 WinW, UINT16 WinH, UINT8 UpdateDsp);

/* 读出窗口 + (可选)DSP 输入尺寸 + 输出尺寸(自动挑整数降采样档位) */
UINT8 OV2640_Config(UINT16 WinW, UINT16 WinH, UINT16 OutW, UINT16 OutH, UINT8 UpdateDsp);

/* 低延迟模式: 固定曝光(关自动曝光) + 增益(手动或自动上限拉满)
 * 画面延迟约等于一帧时间, 而暗处自动曝光会把帧时间拉长, 所以固定短曝光
 * 能降低延迟, 亮度用增益补(代价是暗处噪点变多).
 *   Aec_Val  - 曝光值, 越小延迟越低但画面越暗 (建议 1~32)
 *   Gain_Val - 手动增益 0x01~0xFF, 越大越亮越噪; 填 0 = 增益保持自动 */
void OV2640_LowLatency(UINT8 Aec_Val, UINT8 Gain_Val);



#endif

