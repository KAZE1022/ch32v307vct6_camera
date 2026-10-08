/********************************** (C) COPYRIGHT *******************************
* File Name          : ov.c
* Author             : WCH
* Version            : V1.0.0
* Date               : 2021/06/06
* Description        : This file contains the headers of the OV2640.
*********************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/
#include "ov.h"

/* Start Camera list of initialization configuration registers */
const UINT8 OV2640_InitRegTbl[][2]=
{
	0xff, 0x00, 0x2c, 0xff,	0x2e, 0xdf,	0xff, 0x01,	0x3c, 0x32,

	0x11, 0x00,	0x09, 0x02,	0x04, 0xD8,	0x13, 0xe5,	0x14, 0x48,	0x2c, 0x0c,	0x33, 0x78,	0x3a, 0x33,	0x3b, 0xfB,

	0x3e, 0x00,	0x43, 0x11,	0x16, 0x10,

	0x39, 0x92,

	0x35, 0xda,	0x22, 0x1a,	0x37, 0xc3,	0x23, 0x00,	0x34, 0xc0,	0x36, 0x1a,	0x06, 0x88,	0x07, 0xc0,	0x0d, 0x87,	0x0e, 0x41,	0x4c, 0x00,

	0x48, 0x00,	0x5B, 0x00,	0x42, 0x03,

	0x4a, 0x81,	0x21, 0x99,

	0x24, 0x40,	0x25, 0x38,	0x26, 0x82,	0x5c, 0x00,	0x63, 0x00,	0x46, 0x00,	0x0c, 0x3c,

	0x61, 0x70,	0x62, 0x80,	0x7c, 0x05,

	0x20, 0x80,	0x28, 0x30,	0x6c, 0x00,	0x6d, 0x80,	0x6e, 0x00,	0x70, 0x02,	0x71, 0x94,	0x73, 0xc1,	0x3d, 0x34,	0x5a, 0x57,

	0x12, 0x00,

	0x17, 0x11, 0x18, 0x75,	0x19, 0x01,	0x1a, 0x97,	0x32, 0x36,	0x03, 0x0f,	0x37, 0x40,

	0x4f, 0xca,	0x50, 0xa8,	0x5a, 0x23,	0x6d, 0x00,	0x6d, 0x38,

	0xff, 0x00,	0xe5, 0x7f,	0xf9, 0xc0,	0x41, 0x24,	0xe0, 0x14,	0x76, 0xff,	0x33, 0xa0,	0x42, 0x20,	0x43, 0x18,	0x4c, 0x00,	0x87, 0xd5,	0x88, 0x3f,	0xd7, 0x03,	0xd9, 0x10,	0xd3, 0x82,

	0xc8, 0x08,	0xc9, 0x80,

	0x7c, 0x00,	0x7d, 0x00,	0x7c, 0x03,	0x7d, 0x48,	0x7d, 0x48,	0x7c, 0x08,	0x7d, 0x20,	0x7d, 0x10,	0x7d, 0x0e,

	0x90, 0x00,	0x91, 0x0e,	0x91, 0x1a,	0x91, 0x31,	0x91, 0x5a,	0x91, 0x69,	0x91, 0x75,	0x91, 0x7e,	0x91, 0x88,	0x91, 0x8f,	0x91, 0x96,	0x91, 0xa3,	0x91, 0xaf,	0x91, 0xc4,	0x91, 0xd7,	0x91, 0xe8,	0x91, 0x20,

	0x92, 0x00,	0x93, 0x06,	0x93, 0xe3,	0x93, 0x05,	0x93, 0x05,	0x93, 0x00,	0x93, 0x04,	0x93, 0x00,	0x93, 0x00,	0x93, 0x00,	0x93, 0x00,	0x93, 0x00,	0x93, 0x00,	0x93, 0x00,

	0x96, 0x00,	0x97, 0x08,	0x97, 0x19,	0x97, 0x02,	0x97, 0x0c,	0x97, 0x24,	0x97, 0x30,	0x97, 0x28,	0x97, 0x26,	0x97, 0x02,	0x97, 0x98,	0x97, 0x80,	0x97, 0x00,	0x97, 0x00,

	0xc3, 0xef,

	0xa4, 0x00,	0xa8, 0x00,	0xc5, 0x11,	0xc6, 0x51,	0xbf, 0x80,	0xc7, 0x10,	0xb6, 0x66,	0xb8, 0xA5,	0xb7, 0x64,	0xb9, 0x7C,	0xb3, 0xaf,	0xb4, 0x97,	0xb5, 0xFF,	0xb0, 0xC5,	0xb1, 0x94,	0xb2, 0x0f,	0xc4, 0x5c,

	0xc0, 0xc8,	0xc1, 0x96,	0x8c, 0x00,	0x86, 0x3d,	0x50, 0x00,	0x51, 0x90,	0x52, 0x2c,	0x53, 0x00,	0x54, 0x00,	0x55, 0x88,

	0x5a, 0x90,	0x5b, 0x2C,	0x5c, 0x05,

	0xd3, 0x02,

	0xc3, 0xed,	0x7f, 0x00,

	0xda, 0x09,

	0xe5, 0x1f,	0xe1, 0x67,	0xe0, 0x00,	0xdd, 0x7f,	0x05, 0x00,
};

/* YUV422 */
const UINT8 OV2640_YUV422RegTbl[][2]=
{
	0xFF, 0x00,	0xDA, 0x10,	0xD7, 0x03,	0xDF, 0x00,	0x33, 0x80,	0x3C, 0x40,	0xe1, 0x77,	0x00, 0x00,
};

/* JPEG */
const UINT8 OV2640_JPEGRegTbl[][2]=
{
	0xff, 0x01,	0xe0, 0x14,	0xe1, 0x77,	0xe5, 0x1f,	0xd7, 0x03,	0xda, 0x10,	0xe0, 0x00,
};

/* RGB565 */
const UINT8 OV2640_RGB565RegTbl[][2]=
{
	0xFF, 0x00,	0xDA, 0x09,	0xD7, 0x03,	0xDF, 0x02,	0x33, 0xa0,	0x3C, 0x00,	0xe1, 0x67,
	0xff, 0x01,	0xe0, 0x00,	0xe1, 0x00,	0xe5, 0x00,	0xd7, 0x00,	0xda, 0x00,	0xe0, 0x00,
};

/*********************************************************************
 * @fn      SCCB_GPIO_Init
 *
 * @brief   Init SCCB GPIO.
 *
 * @return  none
 */
void SCCB_GPIO_Init(void)
{
	IIC_SCL_OUT;
	IIC_SDA_OUT;
	IIC_SCL_SET;
	IIC_SDA_SET;
}

/*********************************************************************
 * @fn      SCCB_Start
 *
 * @brief   Start Signal
 *
 * @return  none
 */
void SCCB_Start(void)
{
	IIC_SDA_SET;
	IIC_SCL_SET;
	Delay_Us(50);
	IIC_SDA_CLR;
	Delay_Us(50);
	IIC_SCL_CLR;
}

/*********************************************************************
 * @fn      SCCB_Stop
 *
 * @brief   Stop Signal
 *
 * @return  none
 */
void SCCB_Stop(void)
{
	IIC_SDA_CLR;
	Delay_Us(50);
	IIC_SCL_SET;
	Delay_Us(50);
	IIC_SDA_SET;
	Delay_Us(50);
}

/*********************************************************************
 * @fn      SCCB_No_Ack
 *
 * @brief   NAK Signal
 *
 * @return  none
 */
void SCCB_No_Ack(void)
{
	Delay_Us(50);
	IIC_SDA_SET;
	IIC_SCL_SET;
	Delay_Us(50);
	IIC_SCL_CLR;
	Delay_Us(50);
	IIC_SDA_CLR;
	Delay_Us(50);
}

/*********************************************************************
 * @fn      SCCB_WR_Byte
 *
 * @brief   Write One Byte
 *
 * @param   data
 *
 * @return  0 - Success
 *          other - Err
 */
UINT8 SCCB_WR_Byte(UINT8 data)
{
	UINT8 i,t;

	for(i=0; i<8; i++){
		if(data & 0x80){
			IIC_SDA_SET;
		}
		else{
		    IIC_SDA_CLR;
		}

		data <<= 1;
		Delay_Us(50);
		IIC_SCL_SET;
		Delay_Us(50);
		IIC_SCL_CLR;
	}

	IIC_SDA_IN;
	Delay_Us(50);
	IIC_SCL_SET;
	Delay_Us(50);

	if(SDA_IN_R)t=1;
	else t=0;

	IIC_SCL_CLR;
	IIC_SDA_OUT;

	return t;
}

/*********************************************************************
 * @fn      SCCB_RD_Byte
 *
 * @brief   Read One Byte
 *
 * @return  Read one byte data
 */
UINT8 SCCB_RD_Byte(void)
{
	UINT8 t=0,i;

	IIC_SDA_IN;

	for(i=8; i>0; i--){
		Delay_Us(50);
		IIC_SCL_SET;
		t=t<<1;

		if(SDA_IN_R)t++;

		Delay_Us(50);
		IIC_SCL_CLR;
	}

	IIC_SDA_OUT;

	return t;
}

/*********************************************************************
 * @fn      SCCB_WR_Reg
 *
 * @brief   Write camera Register
 *
 * @param   Reg_Adr - Register address
 *          Reg_Val - Register value
 *
 * @return  0 - Success
 *          other - Err
 */
UINT8 SCCB_WR_Reg(UINT8 Reg_Adr,UINT8 Reg_Val)
{
	UINT8 res=0;

	SCCB_Start();
	if(SCCB_WR_Byte(SCCB_ID))res=1;
	Delay_Us(100);
  	if(SCCB_WR_Byte(Reg_Adr))res=1;
  	Delay_Us(100);
  	if(SCCB_WR_Byte(Reg_Val))res=1;
  	SCCB_Stop();

  	return	res;
}

/*********************************************************************
 * @fn      SCCB_RD_Reg
 *
 * @brief   Read camera Register
 *
 * @return  Camera Register value
 */
UINT8 SCCB_RD_Reg(UINT8 Reg_Adr)
{
	UINT8 val=0;

	SCCB_Start();
	SCCB_WR_Byte(SCCB_ID);
	Delay_Us(100);
  	SCCB_WR_Byte(Reg_Adr);
  	Delay_Us(100);
	SCCB_Stop();
	Delay_Us(100);

	SCCB_Start();
	SCCB_WR_Byte(SCCB_ID|0X01);
	Delay_Us(100);
  	val=SCCB_RD_Byte();
  	SCCB_No_Ack();
  	SCCB_Stop();

  	return val;
}

/*********************************************************************
 * @fn      DVP_GPIO_Init
 *
 * @brief   Init DVP GPIO.
 *
 * @return  none
 */
void DVP_GPIO_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOC|RCC_APB2Periph_GPIOD, ENABLE);     //ʹ����ض˿�ʱ��

    //PC3-PC13
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_3 | GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_9 | GPIO_Pin_10  ;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_6 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11  ;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12  ;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_2  ;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

}

/*********************************************************************
 * @fn      OV2640_Init
 *
 * @brief   Init OV2640
 *
 * @return  0 - Success
 *          1 - Err
 */
UINT8 OV2640_Init(void)
{
	UINT16 i=0;
	UINT16 reg;

	DVP_GPIO_Init();

	OV_PWDN_CLR;                //POWER ON
 	Delay_Ms(10);
 	OV_RESET_CLR;				//Reset OV2640
	Delay_Ms(10);
	OV_RESET_SET;				//Reset End

	SCCB_GPIO_Init();
	SCCB_WR_Reg(0xFF, 0x01);	//Register Bank Select Sensor address

 	SCCB_WR_Reg(0x12, 0x80);	//Reset All Register

 	Delay_Ms(50);

	//Read MID
	reg=SCCB_RD_Reg(0x1C);
	reg<<=8;
	reg|=SCCB_RD_Reg(0x1D);

	if(reg!=OV2640_MID)
	{
		printf("MID:%08x\r\n",reg);
		return 1;
	}

	//Read PID
	reg=SCCB_RD_Reg(0x0A);
	reg<<=8;
	reg|=SCCB_RD_Reg(0x0B);

	if(reg!=OV2640_PID)
	{
		printf("PID:%08x\r\n",reg);
		return 2;
	}

	for(i=0;i<sizeof(OV2640_InitRegTbl)/2;i++)
	{
	   	SCCB_WR_Reg(OV2640_InitRegTbl[i][0],OV2640_InitRegTbl[i][1]);
 	}

  	return 0;
}

/*********************************************************************
 * @fn      RGB565_Mode_Init
 *
 * @brief   Init RGB565 mode
 *
 * @return  none
 */
void RGB565_Mode_Init(void)
{
	OV2640_RGB565_Mode();
	OV2640_OutSize_Set(OV2640_RGB565_WIDTH,OV2640_RGB565_HEIGHT);
	OV2640_Speed_Set(28,1);
}

/*********************************************************************
 * @fn      JPEG_Mode_Init
 *
 * @brief   Init JPEG mode
 *
 * @return  none
 */
void JPEG_Mode_Init(void)
{
	OV2640_JPEG_Mode();
  	OV2640_OutSize_Set(OV2640_JPEG_WIDTH,OV2640_JPEG_HEIGHT);
  	OV2640_Speed_Set(30,1);
}

/*********************************************************************
 * @fn      OV2640_JPEG_Mode
 *
 * @brief   JPEG Mode
 *
 * @return  none
 */
void OV2640_JPEG_Mode(void)
{
	UINT16 i=0;

	//YUV422
	for(i=0;i<(sizeof(OV2640_YUV422RegTbl)/2);i++)
	{
		SCCB_WR_Reg(OV2640_YUV422RegTbl[i][0],OV2640_YUV422RegTbl[i][1]);
	}
	//JPEG
	for(i=0;i<(sizeof(OV2640_JPEGRegTbl)/2);i++)
	{
		SCCB_WR_Reg(OV2640_JPEGRegTbl[i][0],OV2640_JPEGRegTbl[i][1]);
	}
}

/*********************************************************************
 * @fn      OV2640_RGB565_Mode
 *
 * @brief   RGB565 Mode
 *
 * @return  none
 */
void OV2640_RGB565_Mode(void)
{
	UINT16 i=0;

	//RGB565
	for(i=0;i<(sizeof(OV2640_RGB565RegTbl)/2);i++)
	{
		SCCB_WR_Reg(OV2640_RGB565RegTbl[i][0],OV2640_RGB565RegTbl[i][1]);
	}
}

/*********************************************************************
 * @fn      OV2640_OutSize_Set
 *
 * @brief   Set Image Resolution
 *
 * @param   Image_width -
 *          Image_height -
 *
 * @return  none
 */
UINT8 OV2640_OutSize_Set(UINT16 Image_width,UINT16 Image_height)
{
	UINT16 Out_Size_Width;
	UINT16 Out_Size_Height;
	UINT8 t;

	if((Image_width%4) || (Image_height%4))return 0x01;

	Out_Size_Width=Image_width/4;
	Out_Size_Height=Image_height/4;

	SCCB_WR_Reg(0XFF,0X00);
	SCCB_WR_Reg(0XE0,0X04);
	SCCB_WR_Reg(0X5A,Out_Size_Width&0XFF);
	SCCB_WR_Reg(0X5B,Out_Size_Height&0XFF);
	t=(Out_Size_Width>>8)&0X03;
	t|=(Out_Size_Height>>6)&0X04;
	SCCB_WR_Reg(0X5C,t);
	SCCB_WR_Reg(0XE0,0X00);

	return 0;
}

/*********************************************************************
 * @fn      OV2640_LowLatency
 *
 * @brief   低延迟模式: 固定曝光 + 手动或自动增益
 *          画面延迟 ≈ 一帧时间 ≈ 曝光时间. 暗处自动曝光(AEC)会把帧时间
 *          拉长, 于是"晃动时明显拖尾/延迟". 这里关掉 AEC 用固定短曝光,
 *          亮度用增益补, 延迟就能压下来.
 *
 *          寄存器: 传感器组(0xFF=0x01)
 *            COM8(0x13) bit0=自动曝光, bit2=自动增益
 *            AEC (0x10)       = 曝光值(越小延迟越低、越暗)
 *            GAIN(0x00)       = 手动增益(越大越亮越噪)
 *            COM9(0x14)       = 自动增益上限
 *
 * @param   Aec_Val  - 曝光值, 建议 1~32
 *          Gain_Val - 手动增益 0x01~0xFF; 填 0 表示增益保持自动(上限拉满)
 *
 * @return  none
 */
void OV2640_LowLatency(UINT8 Aec_Val, UINT8 Gain_Val)
{
	SCCB_WR_Reg(0xFF, 0x01);            /* 传感器寄存器组 */

	if(Gain_Val != 0)
	{
		/* 曝光和增益都手动: 亮度完全由 Aec_Val / Gain_Val 决定 */
		SCCB_WR_Reg(0x13, 0xE0);        /* COM8: 关自动曝光, 保留防闪 */
		SCCB_WR_Reg(0x00, Gain_Val);    /* GAIN */
	}
	else
	{
		/* 只固定曝光, 增益仍自动(上限拉到 128x) */
		SCCB_WR_Reg(0x13, 0xE4);
		SCCB_WR_Reg(0x14, 0xC8);
	}

	SCCB_WR_Reg(0x10, Aec_Val);         /* AEC: 固定曝光值 */
	SCCB_WR_Reg(0xFF, 0x00);
}

/*********************************************************************
 * @fn      OV2640_OutSize_Set2
 *
 * @brief   更完整的输出尺寸设置(参考 Linux 内核 ov2640 驱动)
 *          Linux 驱动改尺寸的顺序是: CTRLI(含水平/垂直降采样) ->
 *          ZMOW/ZMOH/ZMHH(输出窗口) -> R_DVP_SP -> RESET,
 *          而 WCH 的精简版只写了 ZMOW/ZMOH/ZMHH.
 *          CTRLI 的 V_DIV/H_DIV 是"整数降采样"档位(0=1x,1=2x,2=4x,3=8x),
 *          ISP 的精细缩放器倍率有限, 大倍数缩小必须靠它, 否则画面会
 *          变成"放大 + 发虚"(只显示中间一小块, 还糊).
 *          参考 Linux 的取值: 640x480->0, 320x240->2, 176x144->3
 *
 * @param   Image_width  - 输出宽(4 的倍数)
 *          Image_height - 输出高(4 的倍数)
 *          V_Div        - 垂直降采样档位 0~3
 *          H_Div        - 水平降采样档位 0~3
 *
 * @return  0 - 成功
 */
UINT8 OV2640_OutSize_Set2(UINT16 Image_width, UINT16 Image_height,
                          UINT8 V_Div, UINT8 H_Div)
{
	UINT16 w, h;
	UINT8  t;

	if((Image_width % 4) || (Image_height % 4))
	{
		return 0x01;
	}

	w = Image_width / 4;
	h = Image_height / 4;

	SCCB_WR_Reg(0xFF, 0x00);                                    /* DSP 组 */
	SCCB_WR_Reg(0xE0, 0x04);                                    /* RESET_DVP */
	SCCB_WR_Reg(0x50, (UINT8)(0x80 | ((V_Div & 0x03) << 3) | (H_Div & 0x03)));  /* CTRLI */
	SCCB_WR_Reg(0x5A, (UINT8)(w & 0xFF));
	SCCB_WR_Reg(0x5B, (UINT8)(h & 0xFF));
	t  = (UINT8)(((w >> 8) & 0x03) | ((h >> 6) & 0x04));
	SCCB_WR_Reg(0x5C, t);
	SCCB_WR_Reg(0xE0, 0x00);

	return 0;
}

/*********************************************************************
 * @fn      OV2640_SetReadout
 *
 * @brief   设置传感器内部读出窗口(居中) + 对应的 DSP 输入尺寸.
 *          传感器默认读的是很大一块区域(读的行数多 -> 帧率被卡住),
 *          只读"真正要显示的那一块"就能把帧率按比例提上去.
 *          视野就等于这块窗口(越小 = 数码变焦越大).
 *
 *          寄存器参考 Linux 内核 ov2640 驱动的 size_change_preamble:
 *            传感器组: HSTART(0x17)/HEND(0x18) 单位 8 像素
 *                      VSTART(0x19)/VEND(0x1A) 单位 4 行
 *            DSP 组  : SIZEL/HSIZE8/VSIZE8/CTRL2/HSIZE/VSIZE/VHYX/TEST
 *
 * @param   WinW - 窗口宽(必须是 16 的倍数; 1600 表示全宽)
 *          WinH - 窗口高(必须是 8 的倍数;  1200 表示全高)
 *
 * @return  0 - 成功, 1 - 参数不合法
 */
UINT8 OV2640_SetReadout(UINT16 WinW, UINT16 WinH, UINT8 UpdateDsp)
{
	UINT16 hstart, vstart;

	if((WinW % 16) || (WinH % 8) || (WinW > 1600) || (WinH > 1200))
	{
		return 0x01;
	}

	/* 居中 */
	hstart = (1600 - WinW) / 2;
	vstart = (1200 - WinH) / 2;

	/* --- 传感器读出窗口 --- */
	SCCB_WR_Reg(0xFF, 0x01);
	SCCB_WR_Reg(0x17, (UINT8)(hstart >> 3));                /* HSTART */
	SCCB_WR_Reg(0x18, (UINT8)((hstart + WinW) >> 3));       /* HEND   */
	SCCB_WR_Reg(0x19, (UINT8)(vstart >> 2));                /* VSTART */
	SCCB_WR_Reg(0x1A, (UINT8)((vstart + WinH) >> 2));       /* VEND   */

	if(UpdateDsp == 0)
	{
		SCCB_WR_Reg(0xFF, 0x00);
		return 0;                                           /* 只改窗口, DSP 尺寸保持原样 */
	}

	/* --- DSP 输入尺寸 = 这块窗口(参考 Linux 驱动的计算公式) --- */
	SCCB_WR_Reg(0xFF, 0x00);
	SCCB_WR_Reg(0xE0, 0x04);                                /* RESET_DVP */
	SCCB_WR_Reg(0x8C, (UINT8)((((WinW >> 11) & 0x01) << 6) |
	                          (((WinW >> 3) & 0x07) << 3) |
	                           ((WinH >> 3) & 0x07)));      /* SIZEL  */
	SCCB_WR_Reg(0xC0, (UINT8)((WinW >> 3) & 0xFF));         /* HSIZE8 */
	SCCB_WR_Reg(0xC1, (UINT8)((WinH >> 3) & 0xFF));         /* VSIZE8 */
	SCCB_WR_Reg(0x86, 0x3D);                                /* CTRL2: 开 DCW/SDE/UV */
	SCCB_WR_Reg(0x51, (UINT8)((WinW >> 2) & 0xFF));         /* HSIZE  */
	SCCB_WR_Reg(0x52, (UINT8)((WinH >> 2) & 0xFF));         /* VSIZE  */
	SCCB_WR_Reg(0x53, 0x00);                                /* XOFFL  */
	SCCB_WR_Reg(0x54, 0x00);                                /* YOFFL  */
	SCCB_WR_Reg(0x55, (UINT8)((((WinH >> 10) & 0x01) << 7) |
	                           (((WinW >> 10) & 0x01) << 3)));  /* VHYX */
	SCCB_WR_Reg(0x57, (UINT8)(((WinW >> 11) & 0x01) << 7)); /* TEST   */
	SCCB_WR_Reg(0xE0, 0x00);

	return 0;
}

/*********************************************************************
 * @fn      OV2640_PickDiv
 *
 * @brief   按"降采样后尺寸必须 >= 输出尺寸"的约束, 自动挑最大的整数降采样档位
 *
 * @param   Win - 降采样前尺寸
 *          Out - 输出尺寸
 *
 * @return  档位 0~3
 */
static UINT8 OV2640_PickDiv(UINT16 Win, UINT16 Out)
{
	UINT8 d = 0;

	while((d < 3) && ((Win >> (d + 1)) >= Out))
	{
		d++;
	}
	return d;
}

/*********************************************************************
 * @fn      OV2640_Config
 *
 * @brief   一步到位: 读出窗口 + DSP 输入尺寸 + 输出尺寸(自动挑降采样档位)
 *
 * @param   WinW/WinH - 传感器读出窗口(居中)
 *          OutW/OutH - 摄像头输出尺寸
 *
 * @return  0 - 成功
 */
UINT8 OV2640_Config(UINT16 WinW, UINT16 WinH, UINT16 OutW, UINT16 OutH, UINT8 UpdateDsp)
{
	UINT8 vd, hd;

	if(OV2640_SetReadout(WinW, WinH, UpdateDsp))
	{
		return 0x01;
	}

	if(UpdateDsp == 0)
	{
		return 0;               /* 只改窗口: 输出尺寸/DSP 都保持原样 */
	}

	vd = OV2640_PickDiv(WinH, OutH);
	hd = OV2640_PickDiv(WinW, OutW);

	return OV2640_OutSize_Set2(OutW, OutH, vd, hd);
}

/*********************************************************************
 * @fn      OV2640_Speed_Set
 *
 * @brief   Set DVP PCLK
 *
 * @param   Pclk_Div - DVP output speed ctrl
 *          Xclk_Div - Crystal oscillator input frequency division
 *
 * @return  0 - Success
 *          other - Err
 */
void OV2640_Speed_Set(UINT8 Pclk_Div, UINT8 Xclk_Div)
{
	SCCB_WR_Reg(0XFF,0X00);
	SCCB_WR_Reg(0XD3,Pclk_Div);

	SCCB_WR_Reg(0XFF,0X01);
	SCCB_WR_Reg(0X11,Xclk_Div);
}



