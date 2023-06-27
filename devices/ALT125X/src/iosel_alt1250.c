/****************************************************************************
 *
 *  (c) copyright 2019 Altair Semiconductor, Ltd. All rights reserved.
 *
 *  This software, in source or object form (the "Software"), is the
 *  property of Altair Semiconductor Ltd. (the "Company") and/or its
 *  licensors, which have all right, title and interest therein, You
 *  may use the Software only in  accordance with the terms of written
 *  license agreement between you and the Company (the "License").
 *  Except as expressly stated in the License, the Company grants no
 *  licenses by implication, estoppel, or otherwise. If you are not
 *  aware of or do not agree to the License terms, you may not use,
 *  copy or modify the Software. You may use the source code of the
 *  Software only for your internal purposes and may not distribute the
 *  source code of the Software, any part thereof, or any derivative work
 *  thereof, to any third party, except pursuant to the Company's prior
 *  written consent.
 *  The Software is the confidential information of the Company.
 *
 ****************************************************************************/
#include "iosel.h"

/*This table uses eMcuPinIds as index */
iosel_cfg_t iosel_cfg_table[IOSEL_CFG_TB_SIZE] = {
    // clang-format off
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*Dummy element. GPIO starts from 1*/
  {
    .reg_offset = 0x38,
    .io_func_opt = {IO_FUNC_PWM2, IO_FUNC_LED_CTRL_LED_4,
                    IO_FUNC_CC_IN0, IO_FUNC_EXT_DMA_RDY3, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO1, Ball No.: L13,  PAD Name: GPIO0*/
  {
    .reg_offset = 0x3C,
    .io_func_opt = {IO_FUNC_CLKOUT, IO_FUNC_LED_CTRL_LED_1,
                    IO_FUNC_CC_IN1, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO2, Ball No.: M14,  PAD Name: GPIO1*/
  {
    .reg_offset = 0x40,
    .io_func_opt = {IO_FUNC_SWCLK, IO_FUNC_I2C1_SDA_OUT,
                    IO_FUNC_CC_IN2, IO_FUNC_SPI1_CS0_OUT, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO3, Ball No.: K14,  PAD Name: GPIO2*/
  {
    .reg_offset = 0x44,
    .io_func_opt = {IO_FUNC_SWDAT_OUT, IO_FUNC_I2C1_SCL_OUT,
                    IO_FUNC_CC_IN3, IO_FUNC_SPI1_SCK_OUT, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO4, Ball No.: J13,  PAD Name: GPIO3*/
  {
    .reg_offset = 0x48,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_CC_OUT1,
                    IO_FUNC_LED_CTRL_LED_0, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO5, Ball No.: H12,  PAD Name: GPIO4*/
  {
    .reg_offset = 0x4C,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_CC_OUT2,
                    IO_FUNC_LED_CTRL_LED_2, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO6, Ball No.: K12,  PAD Name: GPIO5*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1250_GPIO7, Ball No.: N/A,  PAD Name: N/A*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1250_GPIO8, Ball No.: N/A,  PAD Name: N/A*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1250_GPIO9, Ball No.: N/A,  PAD Name: N/A*/
  {
    .reg_offset = 0x50,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO10, Ball No.: H10,  PAD Name: RFFE_VDDIO_OUT*/
  {
    .reg_offset = 0x54,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO11, Ball No.: H6,  PAD Name: RFFE_SCLK*/
  {
    .reg_offset = 0x58,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO12, Ball No.: H8,  PAD Name: RFFE_SDATA*/
  {
    .reg_offset = 0x5C,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO13, Ball No.: M8,  PAD Name: SC_RST*/
  {
    .reg_offset = 0x60,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO14, Ball No.: M10,  PAD Name: SC_IO*/
  {
    .reg_offset = 0x64,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO15, Ball No.: L9,  PAD Name: SC_CLK*/
  {
    .reg_offset = 0x68,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_PWM0,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO16, Ball No.: J11,  PAD Name: SC_DET*/
  {
    .reg_offset = 0x6C,
    .io_func_opt = {IO_FUNC_CLKOUT, IO_FUNC_CC_OUT3,
                    IO_FUNC_PWM3, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO17, Ball No.: J9,  PAD Name: SC_SWP*/
  {
    .reg_offset = 0x70,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO18, Ball No.: R5,  PAD Name: EJ_TCK*/
  {
    .reg_offset = 0x74,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO19, Ball No.: K4,  PAD Name: EJ_TMS*/
  {
    .reg_offset = 0x78,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO20, Ball No.: J5,  PAD Name: EJ_TRST*/
  {
    .reg_offset = 0x7C,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO21, Ball No.: L5,  PAD Name: EJ_TDI*/
  {
    .reg_offset = 0x80,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO22, Ball No.: N5,  PAD Name: EJ_TDO*/
  {
    .reg_offset = 0x84,
    .io_func_opt = {IO_FUNC_UART0_RXD, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO23, Ball No.: G11,  PAD Name: UART0_RX*/
  {
    .reg_offset = 0x88,
    .io_func_opt = {IO_FUNC_UART0_TXD, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO24, Ball No.: K10,  PAD Name: UART0_TX*/
  {
    .reg_offset = 0x8C,
    .io_func_opt = {IO_FUNC_UART0_CTS_N, IO_FUNC_I2C0_SCL_OUT,
                    IO_FUNC_UNDEFINED, IO_FUNC_SPI0_SCK_OUT, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO25, Ball No.: G9,  PAD Name: UART0_CTS*/
  {
    .reg_offset = 0x90,
    .io_func_opt = {IO_FUNC_UART0_RTS_N, IO_FUNC_I2C0_SDA_OUT,
                    IO_FUNC_UNDEFINED, IO_FUNC_SPI0_CS0_OUT, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO26, Ball No.: K8,  PAD Name: UART0_RTS*/
  {
    .reg_offset = 0x94,
    .io_func_opt = {IO_FUNC_SPI0_CD_OUT, IO_FUNC_SPI1_CD_OUT,
                    IO_FUNC_UART1_RXD, IO_FUNC_PWM2, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO27, Ball No.: H14,  PAD Name: UART2_RX*/
  {
    .reg_offset = 0x98,
    .io_func_opt = {IO_FUNC_SPI0_MOSI_OUT, IO_FUNC_SPI1_MOSI_OUT,
                    IO_FUNC_UART1_TXD, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO28, Ball No.: G13,  PAD Name: UART2_TX*/
  {
    .reg_offset = 0x9C,
    .io_func_opt = {IO_FUNC_SPI0_SCK_OUT, IO_FUNC_SPI1_SCK_OUT,
                    IO_FUNC_UART1_CTS_N, IO_FUNC_EXT_DMA_RDY2, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO29, Ball No.: G15,  PAD Name: UART2_CTS*/
  {
    .reg_offset = 0xA0,
    .io_func_opt = {IO_FUNC_SPI0_CS0_OUT, IO_FUNC_SPI1_CS0_OUT,
                    IO_FUNC_UART1_RTS_N, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO30, Ball No.: K6,  PAD Name: UART2_RTS*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1250_GPIO31, Ball No.: M4,  PAD Name: N/A*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1250_GPIO32, Ball No.: P4,  PAD Name: N/A*/
  {
    .reg_offset = 0xAC,
    .io_func_opt = {IO_FUNC_SPI_SLAVE_MISO, IO_FUNC_SWCLK,
                    IO_FUNC_PCM_IN, IO_FUNC_SPI0_MOSI_OUT, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO33, Ball No.: U13,  PAD Name: SPIM0_MOSI*/
  {
    .reg_offset = 0xB0,
    .io_func_opt = {IO_FUNC_SPI_SLAVE_MOSI, IO_FUNC_UART1_RXD,
                    IO_FUNC_PCM_FS, IO_FUNC_SPI0_CD_OUT, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO34, Ball No.: T12,  PAD Name: SPIM0_MISO*/
  {
    .reg_offset = 0xB4,
    .io_func_opt = {IO_FUNC_SPI_SLAVE_CLK, IO_FUNC_SWDAT_OUT,
                    IO_FUNC_PCM_OUT, IO_FUNC_SPI0_CS0_OUT, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO35, Ball No.: P12,  PAD Name: SPIM0_EN0*/
  {
    .reg_offset = 0xB8,
    .io_func_opt = {IO_FUNC_SF_CS_N1, IO_FUNC_SPI_SLAVE_SRDY,
                    IO_FUNC_CC_IN2, IO_FUNC_CC_OUT2, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO36, Ball No.: R13,  PAD Name: SPIM0_EN1*/
  {
    .reg_offset = 0xBC,
    .io_func_opt = {IO_FUNC_SPI_SLAVE_MRDY, IO_FUNC_UART1_TXD,
                    IO_FUNC_PCM_CLK, IO_FUNC_SPI0_SCK_OUT, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO37, Ball No.: V12,  PAD Name: SPIM0_CLK*/
  {
    .reg_offset = 0xC0,
    .io_func_opt = {IO_FUNC_SPI1_MOSI_OUT,IO_FUNC_SPI_SLAVE_MISO,
                    IO_FUNC_PCM_OUT, IO_FUNC_PWM2, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO38, Ball No.: T8,  PAD Name: SPIM1_MOSI*/
  {
    .reg_offset = 0xC4,
    .io_func_opt = {IO_FUNC_SPI1_CD_OUT, IO_FUNC_SPI_SLAVE_MOSI,
                    IO_FUNC_PCM_FS, IO_FUNC_PWM1, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO39, Ball No.: N9,  PAD Name: SPIM1_MISO*/
  {
    .reg_offset = 0xC8,
    .io_func_opt = {IO_FUNC_SPI1_CS0_OUT, IO_FUNC_SPI_SLAVE_CLK,
                    IO_FUNC_PCM_IN, IO_FUNC_PWM3, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO40, Ball No.: P8,  PAD Name: SPIM1_EN*/
  {
    .reg_offset = 0xCC,
    .io_func_opt = {IO_FUNC_SPI1_SCK_OUT, IO_FUNC_SPI_SLAVE_MRDY,
                    IO_FUNC_PCM_CLK, IO_FUNC_CC_OUT0, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO41, Ball No.: R9,  PAD Name: SPIM1_CLK*/
  {
    .reg_offset = 0xD0,
    .io_func_opt = {IO_FUNC_I2C0_SDA_OUT, IO_FUNC_SF_SCK,
                    IO_FUNC_PWM3, IO_FUNC_SPI1_SCK_OUT, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO42, Ball No.: J7,  PAD Name: I2C0_SDA*/
  {
    .reg_offset = 0xD4,
    .io_func_opt = {IO_FUNC_I2C0_SCL_OUT, IO_FUNC_SF_CS_N0,
                    IO_FUNC_LED_CTRL_LED_3, IO_FUNC_SPI1_CS0_OUT, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO43, Ball No.: L7,  PAD Name: I2C0_SCL*/
  {
    .reg_offset = 0xD8,
    .io_func_opt = {IO_FUNC_I2C1_SDA_OUT, IO_FUNC_SF_SCK,
                    IO_FUNC_PWM3, IO_FUNC_SPI1_SCK_OUT, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO44, Ball No.: H2,  PAD Name: I2C1_SDA*/
  {
    .reg_offset = 0xDC,
    .io_func_opt = {IO_FUNC_I2C1_SCL_OUT, IO_FUNC_SF_CS_N0,
                    IO_FUNC_LED_CTRL_LED_3, IO_FUNC_SPI1_CS0_OUT, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO45, Ball No.: H4,  PAD Name: I2C1_SCL*/
  {
    .reg_offset = 0xE0,
    .io_func_opt = {IO_FUNC_UART1_CTS_N, IO_FUNC_SF_IO0_OUT,
                    IO_FUNC_LED_CTRL_LED_2, IO_FUNC_PCM_CLK, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO46, Ball No.: V10,  PAD Name: PCM_CLK*/
  {
    .reg_offset = 0xE4,
    .io_func_opt = {IO_FUNC_UART1_RTS_N, IO_FUNC_SF_IO1_OUT,
                    IO_FUNC_LED_CTRL_LED_5, IO_FUNC_PCM_FS, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO47, Ball No.: R11,  PAD Name: PCM_FS*/
  {
    .reg_offset = 0xE8,
    .io_func_opt = {IO_FUNC_UART1_RXD, IO_FUNC_SF_IO2_OUT,
                    IO_FUNC_LED_CTRL_LED_4, IO_FUNC_PCM_IN, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO48, Ball No.: N11,  PAD Name: PCM_IN*/
  {
    .reg_offset = 0xEC,
    .io_func_opt = {IO_FUNC_UART1_TXD, IO_FUNC_SF_IO3_OUT,
                    IO_FUNC_PWM2, IO_FUNC_PCM_OUT, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO49, Ball No.: T10,  PAD Name: PCM_OUT*/
  {
    .reg_offset = 0xF0,
    .io_func_opt = {IO_FUNC_PWM0, IO_FUNC_CC_OUT0,
                    IO_FUNC_CC_IN0, IO_FUNC_CLKOUT, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO50, Ball No.: P10,  PAD Name: PWM0*/
  {
    .reg_offset = 0xF4,
    .io_func_opt = {IO_FUNC_PWM1, IO_FUNC_CC_OUT1,
                    IO_FUNC_CC_IN1, IO_FUNC_SPI1_CD_OUT, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO51, Ball No.: L11,  PAD Name: PWM1*/
  {
    .reg_offset = 0xF8,
    .io_func_opt = {IO_FUNC_PWM2, IO_FUNC_CC_OUT2,
                    IO_FUNC_CC_IN2, IO_FUNC_SPI1_MOSI_OUT, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO52, Ball No.: M12,  PAD Name: PWM2*/
  {
    .reg_offset = 0xFC,
    .io_func_opt = {IO_FUNC_PWM3, IO_FUNC_CC_OUT3,
                    IO_FUNC_CC_IN3, IO_FUNC_LED_CTRL_LED_3, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO53, Ball No.: N13,  PAD Name: PWM3*/
  {
    .reg_offset = 0x100,
    .io_func_opt = {IO_FUNC_SF_CS_N0, IO_FUNC_SF_CS_N1,
                    IO_FUNC_LED_CTRL_LED_0, IO_FUNC_PWM0, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO54, Ball No.: M6,  PAD Name: FLASH1_CS_N*/
  {
    .reg_offset = 0x104,
    .io_func_opt = {IO_FUNC_SF_SCK, IO_FUNC_UNDEFINED,
                    IO_FUNC_LED_CTRL_LED_1, IO_FUNC_PWM1, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO55, Ball No.: U7,  PAD Name: FLASH1_SCK*/
  {
    .reg_offset = 0x108,
    .io_func_opt = {IO_FUNC_SF_IO0_OUT, IO_FUNC_I2C0_SDA_OUT,
                    IO_FUNC_UART1_RXD, IO_FUNC_CC_OUT0, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO56, Ball No.: T6,  PAD Name: FLASH1_IO0*/
  {
    .reg_offset = 0x10C,
    .io_func_opt = {IO_FUNC_SF_IO1_OUT, IO_FUNC_I2C0_SCL_OUT,
                    IO_FUNC_UART1_TXD, IO_FUNC_CC_OUT1, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO57, Ball No.: R7,  PAD Name: FLASH1_IO1*/
  {
    .reg_offset = 0x110,
    .io_func_opt = {IO_FUNC_SF_IO2_OUT, IO_FUNC_I2C1_SDA_OUT,
                    IO_FUNC_UART1_CTS_N, IO_FUNC_CC_OUT2, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO58, Ball No.: P6,  PAD Name: FLASH1_IO2*/
  {
    .reg_offset = 0x114,
    .io_func_opt = {IO_FUNC_SF_IO3_OUT, IO_FUNC_I2C1_SCL_OUT,
                    IO_FUNC_UART1_RTS_N, IO_FUNC_CC_OUT3, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO59, Ball No.: N7,  PAD Name: FLASH1_IO3*/
  {
    .reg_offset = 0x118,
    .io_func_opt = {IO_FUNC_SF_CS_N1, IO_FUNC_CLKOUT,
                    IO_FUNC_CC_OUT1, IO_FUNC_PWM0, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO60, Ball No.: U9,  PAD Name: CLKOUT*/
  {
    .reg_offset = 0x11C,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_CC_IN0, IO_FUNC_UNDEFINED,  IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO61, Ball No.: P2,  PAD Name: WAKEUP*/
  {
    .reg_offset = 0x120,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_CC_IN1, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO62, Ball No.: K2,  PAD Name: PB*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1250_GPIO63, Ball No.: N/A,  PAD Name: N/A*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1250_GPIO64, Ball No.: N/A,  PAD Name: N/A*/
  {
    .reg_offset = 0x0,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO65, Ball No.W13: ,  PAD Name: FLASH0_CS_N0*/
  {
    .reg_offset = 0x4,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_EXT_DMA_RDY0, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO66, Ball No.V14: ,  PAD Name: FLASH0_CS_N1*/
  {
    .reg_offset = 0x8,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO67, Ball No.: W15,  PAD Name: FLASH0_SCK*/
  {
    .reg_offset = 0xC,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO68, Ball No.: F12,  PAD Name: FLASH0_RESETN*/
  {
    .reg_offset = 0x10,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO69, Ball No.: T14,  PAD Name: FLASH0_DQS*/
  {
    .reg_offset = 0x14,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO70, Ball No.: AA15,  PAD Name: FLASH0_IO0*/
  {
    .reg_offset = 0x18,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO71, Ball No.: Y14,  PAD Name: FLASH0_IO1*/
  {
    .reg_offset = 0x1C,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO72, Ball No.: AA13,  PAD Name: FLASH0_IO2*/
  {
    .reg_offset = 0x20,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO73, Ball No.: Y12,  PAD Name: FLASH0_IO3*/
  {
    .reg_offset = 0x24,
    .io_func_opt = {IO_FUNC_UART1_CTS_N, IO_FUNC_UNDEFINED,
                    IO_FUNC_PWM0, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO74, Ball No.: AA11,  PAD Name: FLASH0_IO4*/
  {
    .reg_offset = 0x28,
    .io_func_opt = {IO_FUNC_UART1_TXD, IO_FUNC_UNDEFINED,
                    IO_FUNC_PWM1, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO75, Ball No.: Y10,  PAD Name: FLASH0_IO5*/
  {
    .reg_offset = 0x2C,
    .io_func_opt = {IO_FUNC_UART1_RTS_N, IO_FUNC_UNDEFINED,
                    IO_FUNC_PWM2, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO76, Ball No.: U11,  PAD Name: FLASH0_IO6*/
  {
    .reg_offset = 0x30,
    .io_func_opt = {IO_FUNC_UART1_RXD, IO_FUNC_UNDEFINED,
                    IO_FUNC_PWM3, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO77, Ball No.: Y8,  PAD Name: FLASH0_IO7*/
  {
    .reg_offset = 0x34,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_LED_CTRL_LED_5,
                    IO_FUNC_EXT_DMA_RDY1, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1250_GPIO78, Ball No.: W11,  PAD Name: FLASH0_CS_N2*/
    // clang-format on
};
