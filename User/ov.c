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

/* 下面两张寄存器表沿用 WCH 的写法(扁平初始化列表), 一行一个寄存器对.
 * 打开 -Wall 时 GCC 会提示 missing braces, 但数据本身是对的, 这里显式关掉这个提示. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-braces"

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

/* RGB565 */
const UINT8 OV2640_RGB565RegTbl[][2]=
{
	0xFF, 0x00,	0xDA, 0x09,	0xD7, 0x03,	0xDF, 0x02,	0x33, 0xa0,	0x3C, 0x00,	0xe1, 0x67,
	0xff, 0x01,	0xe0, 0x00,	0xe1, 0x00,	0xe5, 0x00,	0xd7, 0x00,	0xda, 0x00,	0xe0, 0x00,
};

#pragma GCC diagnostic pop

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
	OV2640_Speed_Set(28,1);         /* main.c 里会用 OV2640_CLKRC 再覆盖一次 */
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
 *          画面延迟 ≈ 一帧时间, 而暗处自动曝光(AEC)会把帧时间拉长,
 *          于是"晃动时明显拖尾/延迟". 这里把曝光固定成短的, 亮度交给增益.
 *
 *          寄存器: 传感器组(0xFF=0x01)
 *            COM8(0x13) bit0 = 自动曝光使能(1=自动), bit2 = 自动增益使能(1=自动),
 *                       bit5 = 防闪烁(banding)滤波, bit[7:6] 按参考配置固定为 1
 *            AEC (0x10)       = 曝光值(越大越亮; 8 位寄存器, 最大 255)
 *            GAIN(0x00)       = 手动增益(越大越亮越噪)
 *            COM9(0x14)       = 自动增益的**上限档位**, 在 bit[7:5]:
 *                               000=2x 001=4x 010=8x 011=16x 100=32x 101=64x 110=128x
 *                               bit3 按参考配置(0x48)固定置 1
 *
 * @param   Aec_Val  - 固定曝光值(写入 0x10, 最大 255)
 *          Gain_Val - 手动增益 0x01~0xFF; 填 0 = 增益保持自动(上限 128x)
 *
 * @return  none
 */
void OV2640_LowLatency(UINT8 Aec_Val, UINT8 Gain_Val)
{
	if(Gain_Val != 0)
	{
		/* 曝光和增益都手动: 亮度完全由 Aec_Val / Gain_Val 决定, 噪点稳定 */
		OV2640_SetExpGain(Aec_Val, Gain_Val);
	}
	else
	{
		/* 只固定曝光, 增益保持自动: 暗处自动提亮, 延迟不受影响(只是噪点变多).
		 * 0xC8 = 110 0 1000 -> bit[7:5]=110 是 128x 上限, bit3 保持参考配置的 1 */
		SCCB_WR_Reg(0xFF, 0x01);        /* 传感器寄存器组 */
		SCCB_WR_Reg(0x13, 0xE4);        /* COM8: 关自动曝光 + 开自动增益 */
		SCCB_WR_Reg(0x14, 0xC8);        /* COM9: 自动增益上限 = 128x */
		SCCB_WR_Reg(0x10, Aec_Val);     /* AEC: 固定曝光值 */
		SCCB_WR_Reg(0xFF, 0x00);
	}
}

/*********************************************************************
 * @fn      OV2640_SetExpGain
 *
 * @brief   手动设置"曝光 + 增益"(软件自动曝光回路就是用这个)
 *          COM8 把自动曝光、自动增益都关掉, 亮度完全由这两个值决定.
 *
 * @param   Aec_Val  - 曝光值(写 AEC 0x10, 0~255, 越大越亮)
 *          Gain_Val - 增益(写 GAIN 0x00, 0~255, 越大越亮越噪)
 *
 * @return  none
 */
void OV2640_SetExpGain(UINT8 Aec_Val, UINT8 Gain_Val)
{
	SCCB_WR_Reg(0xFF, 0x01);            /* 传感器寄存器组 */
	SCCB_WR_Reg(0x13, 0xE0);            /* COM8: 关自动曝光 + 关自动增益 */
	SCCB_WR_Reg(0x00, Gain_Val);        /* GAIN */
	SCCB_WR_Reg(0x10, Aec_Val);         /* AEC  */
	SCCB_WR_Reg(0xFF, 0x00);
}

/*********************************************************************
 * @fn      OV2640_DspInputSize
 *
 * @brief   告诉 ISP: "我要送进来的图是 W x H"(DSP 组)
 *          ISP 就按这个尺寸做缩放, 所以**必须和传感器真正输出的尺寸一致**,
 *          否则画面会被拉伸/花掉. 取值公式参考 Linux 内核 ov2640 驱动.
 *
 * @param   W - 输入宽(8 的倍数)   H - 输入高(8 的倍数)
 *
 * @return  none
 */
static void OV2640_DspInputSize(UINT16 W, UINT16 H)
{
	SCCB_WR_Reg(0xFF, 0x00);                                            /* DSP 组 */
	SCCB_WR_Reg(0xE0, 0x04);                                            /* RESET_DVP */
	/* SIZEL = "尺寸低位补全": 高 3 位是宽/高的像素数低 3 位(不是除以 8 之后再取!)
	 * 参考 Linux 驱动: SIZEL_HSIZE8_SET(x)=((x)&7)<<3, SIZEL_VSIZE8_SET(x)=(x)&7
	 * 1600x1200 和 800x600 都是 8 的倍数, 所以这两位应该是 0 */
	SCCB_WR_Reg(0x8C, (UINT8)((((W >> 11) & 0x01) << 6) |
	                          (((W     ) & 0x07) << 3) |
	                           ((H     ) & 0x07)));                     /* SIZEL  */
	SCCB_WR_Reg(0xC0, (UINT8)((W >> 3) & 0xFF));                        /* HSIZE8 */
	SCCB_WR_Reg(0xC1, (UINT8)((H >> 3) & 0xFF));                        /* VSIZE8 */
	SCCB_WR_Reg(0x86, 0x3D);                                            /* CTRL2  */
	SCCB_WR_Reg(0x51, (UINT8)((W >> 2) & 0xFF));                        /* HSIZE  */
	SCCB_WR_Reg(0x52, (UINT8)((H >> 2) & 0xFF));                        /* VSIZE  */
	SCCB_WR_Reg(0x53, 0x00);                                            /* XOFFL  */
	SCCB_WR_Reg(0x54, 0x00);                                            /* YOFFL  */
	SCCB_WR_Reg(0x55, (UINT8)((((H >> 10) & 0x01) << 7) |
	                           (((W >> 10) & 0x01) << 3)));             /* VHYX   */
	SCCB_WR_Reg(0x57, (UINT8)(((W >> 11) & 0x01) << 7));                /* TEST   */
	SCCB_WR_Reg(0xE0, 0x00);
}

/*********************************************************************
 * @fn      OV2640_SetRaster
 *
 * @brief   切"栅格模式" —— 帧率的总开关.
 *          帧率 = CLK / (tLINE x 每帧行数), 而 tLINE/行数只由栅格模式(COM7)决定:
 *            UXGA: tLINE=1922, 约 1560 行/帧     SVGA: tLINE=1190, 约 672 行/帧
 *          所以切成 SVGA 帧率大约翻 4 倍(画面源分辨率从 1600x1200 降到 800x600).
 *          切模式的同时要把 ISP 的"输入尺寸"也改成配套的值, 否则画面会花.
 *
 * @param   Svga - 0 = UXGA 栅格(慢, 画面细)   1 = SVGA 栅格(快)
 *
 * @return  none
 */
void OV2640_SetRaster(UINT8 Svga)
{
	SCCB_WR_Reg(0xFF, 0x01);                        /* 传感器寄存器组 */
	if(Svga)
	{
		SCCB_WR_Reg(0x03, 0x0A);                    /* COM1: 垂直窗口 LSB = SVGA */
		SCCB_WR_Reg(0x12, 0x40);                    /* COM7: SVGA 栅格 */
	}
	else
	{
		SCCB_WR_Reg(0x03, 0x0F);                    /* COM1: 垂直窗口 LSB = UXGA */
		SCCB_WR_Reg(0x12, 0x00);                    /* COM7: UXGA 栅格 */
	}

	/* ISP 的输入尺寸跟着改: UXGA=1600x1200, SVGA=800x600 */
	OV2640_DspInputSize(Svga ? 800 : 1600, Svga ? 600 : 1200);
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



