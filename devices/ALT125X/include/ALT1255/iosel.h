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
/**
 * @file iosel.h
 */
#ifndef IOSEL_H
#define IOSEL_H
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "if_cfg/general_config.h"
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/*! @cond Doxygen_Suppress */
#define IOMNGR_REG_OFS_UNDEFINED (0xFFFF)
#define IO_FUNC_OPT_SIZE 5
#define IOSEL_CFG_TB_SIZE ALT1255_GPIO_NUM
/*! @endcond */
/****************************************************************************
 * Data types
 ****************************************************************************/
/**
 * @defgroup iosel IO Select
 * @{
 */
/**
 * @defgroup iosel_data_types IO Select Types
 * @{
 */
/**
 * @typedef io_mngr_reg_ofs_t
 * Definition of IO MUX register offset type
 */
typedef unsigned short io_mngr_reg_ofs_t;
/**
 * @typedef eIoMngrFunc
 * Definition of IO interface function list.
 */
typedef enum {
  IO_FUNC_GPIO,
  IO_FUNC_EXT_DMA_RDY0,
  IO_FUNC_EXT_DMA_RDY1,
  IO_FUNC_EXT_DMA_RDY2,
  IO_FUNC_UART0_CTS_N,
  IO_FUNC_UART0_RTS_N,
  IO_FUNC_UART0_TXD,
  IO_FUNC_UART0_RXD,
  IO_FUNC_PWM0,
  IO_FUNC_PWM1,
  IO_FUNC_LED_CTRL_LED_0,
  IO_FUNC_LED_CTRL_LED_1,
  IO_FUNC_LED_CTRL_LED_2,
  IO_FUNC_CLKOUT,
  IO_FUNC_SWCLK,
  IO_FUNC_SWDAT_OUT,
  IO_FUNC_I2C0_SCL_OUT,
  IO_FUNC_I2C0_SDA_OUT,
  IO_FUNC_SPI0_SCK_OUT,
  IO_FUNC_SPI0_CS0_OUT,
  IO_FUNC_SPI0_CD_OUT,
  IO_FUNC_SPI0_MOSI_OUT,
  IO_FUNC_SPI_SLAVE_CLK,
  IO_FUNC_SPI_SLAVE_SRDY,
  IO_FUNC_SPI_SLAVE_MRDY,
  IO_FUNC_SPI_SLAVE_MISO,
  IO_FUNC_SPI_SLAVE_MOSI,
  IO_FUNC_UNDEFINED
} eIoMngrFunc;
/**
 * @typedef iosel_cfg_t
 * Definition of IO select config table
 */
typedef struct _iosel_cfg {
  io_mngr_reg_ofs_t reg_offset;
  eIoMngrFunc io_func_opt[IO_FUNC_OPT_SIZE];
} iosel_cfg_t;
/** @} iosel_data_types  */

/*******************************************************************************
 * API
 ******************************************************************************/
/** @} io_mngr */
#endif
