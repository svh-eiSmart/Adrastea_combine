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
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_I2C0_SDA_OUT, IO_FUNC_SWCLK, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1255_GPIO1, Ball No.: G15,  PAD Name: GPIO0*/
  {
    .reg_offset = 0x3C,
    .io_func_opt = {IO_FUNC_CLKOUT, IO_FUNC_LED_CTRL_LED_1,
                    IO_FUNC_PWM1, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1255_GPIO2, Ball No.: F14,  PAD Name: GPIO1*/
  {
    .reg_offset = 0x40,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_EXT_DMA_RDY0,
                    IO_FUNC_I2C0_SCL_OUT, IO_FUNC_SWDAT_OUT, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1255_GPIO3, Ball No.: E13,  PAD Name: GPIO2*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1255_GPIO4, Ball No.: N/A,  PAD Name: N/A*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1255_GPIO5, Ball No.: N/A,  PAD Name: N/A*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1255_GPIO6, Ball No.: N/A,  PAD Name: N/A*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1255_GPIO7, Ball No.: N/A,  PAD Name: N/A*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1255_GPIO8, Ball No.: N/A,  PAD Name: N/A*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1255_GPIO9, Ball No.: N/A,  PAD Name: N/A*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1255_GPIO10, Ball No.: N/A,  PAD Name: N/A*/
  {
    .reg_offset = 0x54,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1255_GPIO11, Ball No.: F8,  PAD Name: RFFE_SCLK*/
  {
    .reg_offset = 0x58,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1255_GPIO12, Ball No.: F10,  PAD Name: RFFE_SDATA*/
  {
    .reg_offset = 0x5C,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1255_GPIO13, Ball No.: J7,  PAD Name: SC_RST*/
  {
    .reg_offset = 0x60,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1255_GPIO14, Ball No.: L5,  PAD Name: SC_IO*/
  {
    .reg_offset = 0x64,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1255_GPIO15, Ball No.: K6,  PAD Name: SC_CLK*/
  {
    .reg_offset = 0x68,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_PWM1, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1255_GPIO16, Ball No.: F6,  PAD Name: SC_DET*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1255_GPIO17, Ball No.: N/A,  PAD Name: N/A*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1255_GPIO18, Ball No.: N/A,  PAD Name: N/A*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1255_GPIO19, Ball No.: N/A,  PAD Name: N/A*/
  {
    .reg_offset = 0x78,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_SPI0_MOSI_OUT, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1255_GPIO20, Ball No.: E11,  PAD Name: EJ_TRST*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1255_GPIO21, Ball No.: N/A,  PAD Name: N/A*/
  {
    .reg_offset = 0x80,
    .io_func_opt = {IO_FUNC_CLKOUT, IO_FUNC_LED_CTRL_LED_2,
                    IO_FUNC_PWM0, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1255_GPIO22, Ball No.: F12,  PAD Name: EJ_TDO*/
  {
    .reg_offset = 0x84,
    .io_func_opt = {IO_FUNC_UART0_RXD, IO_FUNC_UNDEFINED,
                    IO_FUNC_SPI_SLAVE_MOSI, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1255_GPIO23, Ball No.: K12,  PAD Name: UART0_RX*/
  {
    .reg_offset = 0x88,
    .io_func_opt = {IO_FUNC_UART0_TXD, IO_FUNC_UNDEFINED,
                    IO_FUNC_SPI_SLAVE_MISO, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1255_GPIO24, Ball No.: J13,  PAD Name: UART0_TX*/
  {
    .reg_offset = 0x8C,
    .io_func_opt = {IO_FUNC_UART0_CTS_N, IO_FUNC_I2C0_SDA_OUT,
                    IO_FUNC_SPI_SLAVE_MRDY, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1255_GPIO25, Ball No.: H12,  PAD Name: UART0_CTS*/
  {
    .reg_offset = 0x90,
    .io_func_opt = {IO_FUNC_UART0_RTS_N, IO_FUNC_I2C0_SCL_OUT,
                    IO_FUNC_SPI_SLAVE_CLK, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1255_GPIO26, Ball No.: G13,  PAD Name: UART0_RTS*/
  {
    .reg_offset = 0x94,
    .io_func_opt = {IO_FUNC_UART0_RXD, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1255_GPIO27, Ball No.: J11,  PAD Name: UART2_RX*/
  {
    .reg_offset = 0x98,
    .io_func_opt = {IO_FUNC_UART0_TXD, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1255_GPIO28, Ball No.: K10,  PAD Name: UART2_TX*/
  {
    .reg_offset = 0x9C,
    .io_func_opt = {IO_FUNC_UART0_CTS_N, IO_FUNC_UNDEFINED,
                    IO_FUNC_I2C0_SDA_OUT, IO_FUNC_LED_CTRL_LED_1, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1255_GPIO29, Ball No.: H10,  PAD Name: UART2_CTS*/
  {
    .reg_offset = 0xA0,
    .io_func_opt = {IO_FUNC_UART0_RTS_N, IO_FUNC_EXT_DMA_RDY1,
                    IO_FUNC_I2C0_SCL_OUT, IO_FUNC_LED_CTRL_LED_0, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1255_GPIO30, Ball No.: G11,  PAD Name: UART2_RTS*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1255_GPIO31, Ball No.: N/A,  PAD Name: N/A*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1255_GPIO32, Ball No.: N/A,  PAD Name: N/A*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1255_GPIO33, Ball No.: N/A,  PAD Name: N/A*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1255_GPIO34, Ball No.: N/A,  PAD Name: N/A*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1255_GPIO35, Ball No.: N/A,  PAD Name: N/A*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1255_GPIO36, Ball No.: N/A,  PAD Name: N/A*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1255_GPIO37, Ball No.: N/A,  PAD Name: N/A*/
  {
    .reg_offset = 0xC0,
    .io_func_opt = {IO_FUNC_SPI0_MOSI_OUT, IO_FUNC_UART0_RXD,
                    IO_FUNC_I2C0_SDA_OUT, IO_FUNC_SWDAT_OUT, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1255_GPIO38, Ball No.: L9,  PAD Name: SPIM1_MOSI*/
  {
    .reg_offset = 0xC4,
    .io_func_opt = {IO_FUNC_SPI0_CD_OUT, IO_FUNC_UART0_TXD,
                    IO_FUNC_I2C0_SCL_OUT, IO_FUNC_SWCLK, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1255_GPIO39, Ball No.: H8,  PAD Name: SPIM1_MISO*/
  {
    .reg_offset = 0xC8,
    .io_func_opt = {IO_FUNC_SPI0_CS0_OUT, IO_FUNC_UART0_CTS_N,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1255_GPIO40, Ball No.: J9,  PAD Name: SPIM1_EN*/
  {
    .reg_offset = 0xCC,
    .io_func_opt = {IO_FUNC_SPI0_SCK_OUT, IO_FUNC_UART0_RTS_N,
                    IO_FUNC_SPI_SLAVE_SRDY, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1255_GPIO41, Ball No.: K8,  PAD Name: SPIM1_CLK*/
  {
    .reg_offset = 0xD0,
    .io_func_opt = {IO_FUNC_I2C0_SDA_OUT, IO_FUNC_LED_CTRL_LED_2,
                    IO_FUNC_PWM1, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1255_GPIO42, Ball No.: M12,  PAD Name: I2C0_SDA*/
  {
    .reg_offset = 0xD4,
    .io_func_opt = {IO_FUNC_I2C0_SCL_OUT, IO_FUNC_EXT_DMA_RDY1,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1255_GPIO43, Ball No.: P12,  PAD Name: I2C0_SCL*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1255_GPIO44, Ball No.: N/A,  PAD Name: N/A*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1255_GPIO45, Ball No.: N/A,  PAD Name: N/A*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1255_GPIO46, Ball No.: N/A,  PAD Name: N/A*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1255_GPIO47, Ball No.: N/A,  PAD Name: N/A*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1255_GPIO48, Ball No.: N/A,  PAD Name: N/A*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1255_GPIO49, Ball No.: N/A,  PAD Name: N/A*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1255_GPIO50, Ball No.: N/A,  PAD Name: N/A*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1255_GPIO51, Ball No.: N/A,  PAD Name: N/A*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1255_GPIO52, Ball No.: N/A,  PAD Name: N/A*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1255_GPIO53, Ball No.: N/A,  PAD Name: N/A*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1255_GPIO54, Ball No.: N/A,  PAD Name: N/A*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1255_GPIO55, Ball No.: N/A,  PAD Name: N/A*/
  {
    .reg_offset = 0x108,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_LED_CTRL_LED_0,
                    IO_FUNC_PWM1, IO_FUNC_SPI0_CS0_OUT, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1255_GPIO56, Ball No.: N11,  PAD Name: FLASH1_IO0*/
  {
    .reg_offset = 0x10C,
    .io_func_opt = {IO_FUNC_CLKOUT, IO_FUNC_UNDEFINED,
                    IO_FUNC_SPI_SLAVE_SRDY, IO_FUNC_SPI0_SCK_OUT, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1255_GPIO57, Ball No.: L11,  PAD Name: FLASH1_IO1*/
  {
    .reg_offset = 0x110,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1255_GPIO58, Ball No.: M10,  PAD Name: FLASH1_IO2*/
  {
    .reg_offset = 0x114,
    .io_func_opt = {IO_FUNC_CLKOUT, IO_FUNC_LED_CTRL_LED_0,
                    IO_FUNC_PWM0, IO_FUNC_SPI0_CD_OUT, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1255_GPIO59, Ball No.: N9,  PAD Name: FLASH1_IO3*/
  {
    .reg_offset = 0x118,
    .io_func_opt = {IO_FUNC_CLKOUT, IO_FUNC_LED_CTRL_LED_1,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1255_GPIO60, Ball No.: G9,  PAD Name: CLKOUT*/
  {
    .reg_offset = 0x11C,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,  IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1255_GPIO61, Ball No.: J3,  PAD Name: WAKEUP*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1255_GPIO62, Ball No.: N/A,  PAD Name: N/A*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1255_GPIO63, Ball No.: N/A,  PAD Name: N/A*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1255_GPIO64, Ball No.: N/A,  PAD Name: N/A*/
  {
    .reg_offset = 0x0,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1255_GPIO65, Ball No.H14,  PAD Name: FLASH0_CS_N0*/
  {
    .reg_offset = 0x4,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_PWM0, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1255_GPIO66, Ball No.L13,  PAD Name: FLASH0_CS_N1*/
  {
    .reg_offset = 0x8,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1255_GPIO67, Ball No.K14,  PAD Name: FLASH0_SCK*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1255_GPIO68, Ball No.: N/A,  PAD Name: N/A*/
  {
    .reg_offset = IOMNGR_REG_OFS_UNDEFINED,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED}
  }, /*MCU PIN ID: ALT1255_GPIO69, Ball No.: N/A,  PAD Name: N/A*/
  {
    .reg_offset = 0x14,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1255_GPIO70, Ball No.: N15,  PAD Name: FLASH0_IO0*/
  {
    .reg_offset = 0x18,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1255_GPIO71, Ball No.: P14,  PAD Name: FLASH0_IO1*/
  {
    .reg_offset = 0x1C,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1255_GPIO72, Ball No.: N13,  PAD Name: FLASH0_IO2*/
  {
    .reg_offset = 0x20,
    .io_func_opt = {IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED,
                    IO_FUNC_UNDEFINED, IO_FUNC_UNDEFINED, IO_FUNC_GPIO}
  }, /*MCU PIN ID: ALT1255_GPIO73, Ball No.: M14,  PAD Name: FLASH0_IO3*/
    // clang-format on
};
