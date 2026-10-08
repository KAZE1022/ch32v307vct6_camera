/********************************** (C) COPYRIGHT *******************************
* File Name          : lcd.c
* Author             : WCH / 修改: 适配 1.8寸 128x160 ST7735S (SPI)
* Version            : V2.0.0
* Date               : 2025/04/06
* Description        : 1.8" 128x160 ST7735S (4线 SPI) 驱动.
*********************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/
#include "lcd.h"
#include "font.h"

/*==============================================================================
 *  底层: GPIO / SPI
 *============================================================================*/
static void LCD_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);

    /* SCK / MOSI: 复用推挽 */
    GPIO_InitStructure.GPIO_Pin   = LCD_SCK_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LCD_SCK_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin   = LCD_MOSI_PIN;
    GPIO_Init(LCD_MOSI_PORT, &GPIO_InitStructure);

    /* CS / DC / RST / BLK: 普通推挽输出 */
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;

    GPIO_InitStructure.GPIO_Pin   = LCD_CS_PIN;
    GPIO_Init(LCD_CS_PORT, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin   = LCD_DC_PIN;
    GPIO_Init(LCD_DC_PORT, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin   = LCD_RST_PIN;
    GPIO_Init(LCD_RST_PORT, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin   = LCD_BLK_PIN;
    GPIO_Init(LCD_BLK_PORT, &GPIO_InitStructure);

    LCD_CS_H();
    LCD_DC_H();
    LCD_BLK_OFF();
}

static void LCD_SPI_Init(void)
{
    SPI_InitTypeDef SPI_InitStructure = {0};

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

    /* 只用发送方向(MISO 没接, PB14 当 DC 用) */
    SPI_InitStructure.SPI_Direction         = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode              = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize          = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL              = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA              = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS               = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = LCD_SPI_PRESCALER;
    SPI_InitStructure.SPI_FirstBit          = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial     = 7;
    SPI_Init(LCD_SPIx, &SPI_InitStructure);

    /* 软件 NSS 必须内部置高, 否则主机模式会判定模式错误 */
    SPI_NSSInternalSoftwareConfig(LCD_SPIx, SPI_NSSInternalSoft_Set);

    SPI_Cmd(LCD_SPIx, ENABLE);
}

/*==============================================================================
 *  SPI 收发原语
 *============================================================================*/
static void LCD_SPI_SendByte(u8 data)
{
    while(SPI_I2S_GetFlagStatus(LCD_SPIx, SPI_I2S_FLAG_TXE) == RESET);
    SPI_I2S_SendData(LCD_SPIx, data);
}

/* 发送一个字节并等它彻底移出去(切换 DC/CS 之前必须用, 否则最后一个字节会被读错) */
static void LCD_SPI_SendByteSync(u8 data)
{
    LCD_SPI_SendByte(data);
    while(SPI_I2S_GetFlagStatus(LCD_SPIx, SPI_I2S_FLAG_BSY) != RESET);
}

static void LCD_SPI_WaitDone(void)
{
    while(SPI_I2S_GetFlagStatus(LCD_SPIx, SPI_I2S_FLAG_TXE) == RESET);
    while(SPI_I2S_GetFlagStatus(LCD_SPIx, SPI_I2S_FLAG_BSY) != RESET);
}

/*==============================================================================
 *  命令 / 数据
 *============================================================================*/
void LCD_WriteCmd(u8 cmd)
{
    LCD_WaitFrameDone();                        /* 别打断正在发送的整帧 */
    LCD_CS_L();
    LCD_DC_L();
    LCD_SPI_SendByteSync(cmd);
    LCD_CS_H();
}

void LCD_WriteCmdParam(u8 cmd, const u8 *param, u16 len)
{
    LCD_WaitFrameDone();
    LCD_CS_L();
    LCD_DC_L();
    LCD_SPI_SendByteSync(cmd);          /* 命令字节必须发完再切 DC */
    if(len)
    {
        LCD_DC_H();
        while(len--)
        {
            LCD_SPI_SendByte(*param++);
        }
    }
    LCD_SPI_WaitDone();
    LCD_CS_H();
}

void LCD_WriteData16(u16 data)
{
    LCD_WaitFrameDone();
    LCD_CS_L();
    LCD_DC_H();
    LCD_SPI_SendByte(data >> 8);
    LCD_SPI_SendByte(data & 0xFF);
    LCD_SPI_WaitDone();
    LCD_CS_H();
}

/*==============================================================================
 *  SPI 发送 DMA(把 CPU 从"搬像素"里解放出来, 这是帧率的关键)
 *  SPI2_TX 对应 DMA1_Channel5
 *============================================================================*/
#define LCD_DMA_CH        DMA1_Channel5
#define LCD_DMA_TC_FLAG   DMA1_FLAG_TC5

static u8 lcd_dma_started = 0;

static void LCD_DMA_Init(void)
{
    DMA_InitTypeDef DMA_InitStructure = {0};

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    DMA_DeInit(LCD_DMA_CH);

    DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&LCD_SPIx->DATAR;
    DMA_InitStructure.DMA_MemoryBaseAddr     = 0;
    DMA_InitStructure.DMA_DIR                = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_BufferSize         = LCD_WIDTH * 2;
    DMA_InitStructure.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode               = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority           = DMA_Priority_VeryHigh;
    DMA_InitStructure.DMA_M2M                = DMA_M2M_Disable;
    DMA_Init(LCD_DMA_CH, &DMA_InitStructure);

    /* 只开发送方向; DMA 通道默认是关的, 所以不影响 CPU 发命令字节 */
    SPI_I2S_DMACmd(LCD_SPIx, SPI_I2S_DMAReq_Tx, ENABLE);
    DMA_ClearFlag(LCD_DMA_TC_FLAG);
}

u8 LCD_FrameDone(void)
{
    if(lcd_dma_started == 0)
    {
        return 1;                               /* 还没发过 */
    }
    if(DMA_GetFlagStatus(LCD_DMA_TC_FLAG) == RESET)
    {
        return 0;                               /* 还在搬 */
    }

    LCD_SPI_WaitDone();                         /* 等最后几个字节移出去 */
    lcd_dma_started = 0;
    LCD_CS_H();
    return 1;
}

void LCD_WaitFrameDone(void)
{
    while(LCD_FrameDone() == 0)
    {
    }
}

/*********************************************************************
 * @fn      LCD_PushBusy
 *
 * @brief   上一段横条是否还在搬(DMA 还在读 buf).
 *          分片刷屏时用: 还在搬就不能改写那块内存, 否则屏上会出现花屏条.
 *          注: 只是查询, 不改变任何状态; 真正的收尾(等最后一个字节移出、
 *          拉高 CS)由下一次 LCD_PushBlock / LCD_WriteCmd 里的
 *          LCD_WaitFrameDone 完成.
 *
 * @return  1 - 还在搬, 0 - 已搬完
 */
u8 LCD_PushBusy(void)
{
    if(lcd_dma_started == 0)
    {
        return 0;
    }
    if(DMA_GetFlagStatus(LCD_DMA_TC_FLAG) == RESET)
    {
        return 1;
    }
    return 0;
}

/*==============================================================================
 *  ST7735S 初始化
 *============================================================================*/
typedef struct
{
    u8  cmd;
    u8  len;
    u8  delay;              /* 发完后延时(ms) */
    u8  data[16];
} lcd_init_cmd_t;

static const lcd_init_cmd_t st7735s_cmd_tbl[] =
{
    {0x01, 0, 150, {0}},                                        /* SWRESET  软复位 */
    {0x11, 0, 150, {0}},                                        /* SLPOUT   退出睡眠 */
    {0xB1, 3, 0,   {0x01, 0x2C, 0x2D}},                         /* FRMCTR1  帧率 */
    {0xB2, 3, 0,   {0x01, 0x2C, 0x2D}},                         /* FRMCTR2 */
    {0xB3, 6, 0,   {0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D}},       /* FRMCTR3 */
    {0xB4, 1, 0,   {0x07}},                                     /* INVCTR   全屏反相 */
    {0xC0, 3, 0,   {0xA2, 0x02, 0x84}},                         /* PWCTR1   power */
    {0xC1, 1, 0,   {0xC5}},                                     /* PWCTR2 */
    {0xC2, 2, 0,   {0x0A, 0x00}},                               /* PWCTR3 */
    {0xC3, 2, 0,   {0x8A, 0x2A}},                               /* PWCTR4 */
    {0xC4, 2, 0,   {0x8A, 0xEE}},                               /* PWCTR5 */
    {0xC5, 1, 0,   {0x0E}},                                     /* VMCTR1   VCOM */
    {0x36, 1, 0,   {LCD_MADCTL}},                               /* MADCTL   扫描方向 */
    {0x3A, 1, 0,   {0x05}},                                     /* COLMOD   16bit RGB565 */
    {0xE0, 16, 0,  {0x02, 0x1C, 0x07, 0x12, 0x37, 0x32, 0x29,   /* PVGAMCTRL 正极性伽马 */
                    0x2D, 0x29, 0x25, 0x2B, 0x39, 0x00, 0x01,
                    0x03, 0x10}},
    {0xE1, 16, 0,  {0x03, 0x1D, 0x07, 0x06, 0x2E, 0x2C, 0x29,   /* NVGAMCTRL 负极性伽马 */
                    0x2D, 0x2E, 0x2E, 0x37, 0x3F, 0x00, 0x00,
                    0x02, 0x10}},
#if (LCD_INVERT == 1)
    {0x21, 0, 0,   {0}},                                        /* INVON    整屏反相(补色) */
#else
    {0x20, 0, 0,   {0}},                                        /* INVOFF   显示正常 */
#endif
    {0x13, 0, 10,  {0}},                                        /* NORON    部分模式关闭 */
    {0x29, 0, 120, {0}},                                        /* DISPON   开显示 */
};

void LCD_Init(void)
{
    u16 i;

    LCD_GPIO_Init();

    /* 硬复位 */
    GPIO_SetBits(LCD_RST_PORT, LCD_RST_PIN);
    delay_ms(10);
    GPIO_ResetBits(LCD_RST_PORT, LCD_RST_PIN);
    delay_ms(20);
    GPIO_SetBits(LCD_RST_PORT, LCD_RST_PIN);
    delay_ms(120);

    LCD_SPI_Init();
    LCD_DMA_Init();

    for(i = 0; i < (sizeof(st7735s_cmd_tbl) / sizeof(st7735s_cmd_tbl[0])); i++)
    {
        LCD_WriteCmdParam(st7735s_cmd_tbl[i].cmd,
                          st7735s_cmd_tbl[i].data,
                          st7735s_cmd_tbl[i].len);
        if(st7735s_cmd_tbl[i].delay)
        {
            delay_ms(st7735s_cmd_tbl[i].delay);
        }
    }

    LCD_SetWindow(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);         /* 全屏窗口 */
}

void LCD_BackLight(u8 on)
{
    if(on)
    {
        LCD_BLK_ON();
    }
    else
    {
        LCD_BLK_OFF();
    }
}

/*==============================================================================
 *  坐标 / 画图
 *============================================================================*/
void LCD_SetWindow(u16 xs, u16 ys, u16 xe, u16 ye)
{
    u8 p[4];

    xs += LCD_XSTART;
    xe += LCD_XSTART;
    ys += LCD_YSTART;
    ye += LCD_YSTART;

    p[0] = xs >> 8;  p[1] = xs & 0xFF;  p[2] = xe >> 8;  p[3] = xe & 0xFF;
    LCD_WriteCmdParam(0x2A, p, 4);                      /* CASET 列地址 */

    p[0] = ys >> 8;  p[1] = ys & 0xFF;  p[2] = ye >> 8;  p[3] = ye & 0xFF;
    LCD_WriteCmdParam(0x2B, p, 4);                      /* RASET 行地址 */
}

void LCD_SetCursor(u16 x, u16 y)
{
    LCD_SetWindow(x, y, x, y);
}

void LCD_Fill(u16 xs, u16 ys, u16 xe, u16 ye, u16 color)
{
    u32 i, n;

    if((xs > xe) || (ys > ye) || (xe >= LCD_WIDTH) || (ye >= LCD_HEIGHT))
    {
        return;
    }

    LCD_SetWindow(xs, ys, xe, ye);
    LCD_WriteCmd(0x2C);                                 /* RAMWR */

    n = (u32)(xe - xs + 1) * (ye - ys + 1);

    LCD_CS_L();
    LCD_DC_H();
    for(i = 0; i < n; i++)
    {
        LCD_SPI_SendByte(color >> 8);
        LCD_SPI_SendByte(color & 0xFF);
    }
    LCD_SPI_WaitDone();
    LCD_CS_H();
}

void LCD_Clear(u16 color)
{
    LCD_Fill(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, color);
}

void LCD_ShowChar(u16 x, u16 y, char ch, u16 fc, u16 bc)
{
    u8  rows[12] = {0};                                 /* 12 行, 每行低 6 位为像素 */
    u8  c, r, b0, b1;
    u16 idx;

    if((ch < ' ') || (ch > '~'))
    {
        ch = ' ';
    }
    idx = (u16)(ch - ' ');

    /* 字库是"列优先, 每列 2 字节(上8行 + 下4行)" */
    for(c = 0; c < 6; c++)
    {
        b0 = asc2_1206[idx][2 * c];
        b1 = asc2_1206[idx][2 * c + 1];
        for(r = 0; r < 8; r++)
        {
            if(b0 & (0x80 >> r))
            {
                rows[r] |= (1 << c);
            }
        }
        for(r = 0; r < 4; r++)
        {
            if(b1 & (0x80 >> r))
            {
                rows[8 + r] |= (1 << c);
            }
        }
    }

    LCD_SetWindow(x, y, x + 5, y + 11);
    LCD_WriteCmd(0x2C);
    LCD_CS_L();
    LCD_DC_H();
    for(r = 0; r < 12; r++)
    {
        for(c = 0; c < 6; c++)
        {
            u16 pix = (rows[r] & (1 << c)) ? fc : bc;

            LCD_SPI_SendByte(pix >> 8);
            LCD_SPI_SendByte(pix & 0xFF);
        }
    }
    LCD_SPI_WaitDone();
    LCD_CS_H();
}

void LCD_ShowString(u16 x, u16 y, const char *str, u16 fc, u16 bc)
{
    while(*str)
    {
        if((x + 6) > LCD_WIDTH)
        {
            x = 0;
            y += 12;
        }
        if((y + 12) > LCD_HEIGHT)
        {
            break;
        }
        LCD_ShowChar(x, y, *str++, fc, bc);
        x += 6;
    }
}

/*==============================================================================
 *  视频流
 *============================================================================*/
void LCD_SwapPixelBytes(const u8 *src, u8 *dst, u16 pixels)
{
    u16 i;

    for(i = 0; i < pixels; i++)
    {
        dst[2 * i]     = src[2 * i + 1];
        dst[2 * i + 1] = src[2 * i];
    }
}

/*********************************************************************
 * @fn      LCD_PushBlock
 *
 * @brief   把一段横条(rows 行, 每行 LCD_WIDTH*2 字节)用 SPI+DMA 刷到屏幕
 *          第 y 行开始的位置. 不等发完就返回(内部会先等上一次发完).
 *          分片刷屏: 每攒满一小段就立刻刷出去, 屏幕就有连续的更新,
 *          不会出现"整帧攒满才跳一下"的延迟感, 也不会有瀑布感.
 *
 * @param   y    - 起始行号
 *          buf  - 像素数据(高字节在前, 按行连续排列)
 *          rows - 行数
 *
 * @return  none
 */
void LCD_PushBlock(u16 y, const u8 *buf, u16 rows)
{
    if((rows == 0) || ((u32)y + rows > LCD_HEIGHT))
    {
        return;
    }

    LCD_SetWindow(0, y, LCD_WIDTH - 1, y + rows - 1);       /* 内部会先等上一段发完 */

    LCD_CS_L();
    LCD_DC_L();
    LCD_SPI_SendByteSync(0x2C);                             /* RAMWR */
    LCD_DC_H();

    DMA_Cmd(LCD_DMA_CH, DISABLE);
    LCD_DMA_CH->MADDR = (u32)buf;
    DMA_SetCurrDataCounter(LCD_DMA_CH, (u16)((u32)rows * LCD_WIDTH * 2));
    DMA_ClearFlag(LCD_DMA_TC_FLAG);
    DMA_Cmd(LCD_DMA_CH, ENABLE);
    lcd_dma_started = 1;
}
