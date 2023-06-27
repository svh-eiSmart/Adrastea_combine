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
 * @file if_mngr.h
 */
#ifndef IF_MNGR_H
#define IF_MNGR_H
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "io_mngr.h"
#include "if_cfg/interfaces_config_alt1250.h"
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/*! @cond Doxygen_Suppress */
// clang-format off
#define IF_MNGR_UART_IS_DEFINED(IF) (IF##_RX_PIN_SET         != ALT1250_PIN_UNDEFINED && \
                                     IF##_TX_PIN_SET         != ALT1250_PIN_UNDEFINED && \
                                     IF##_BAUDRATE_DEFAULT   != ALT1250_CONFIG_UNDEFINED && \
                                     IF##_HWFLOWCTRL_DEFAULT != ALT1250_CONFIG_UNDEFINED && \
                                     IF##_PARITY_DEFAULT     != ALT1250_CONFIG_UNDEFINED && \
                                     IF##_STOPBITS_DEFAULT   != ALT1250_CONFIG_UNDEFINED && \
                                     IF##_WORDLENGTH_DEFAULT != ALT1250_CONFIG_UNDEFINED)

#define IF_MNGR_ANY_UART_IS_DEFINED (IF_MNGR_UART_IS_DEFINED(UARTF0) || IF_MNGR_UART_IS_DEFINED(UARTF1))

#define IF_MNGR_UART_RTS_CTS_IS_DEFINED(IF) (IF##_RTS_PIN_SET != ALT1250_PIN_UNDEFINED && \
                                             IF##_CTS_PIN_SET != ALT1250_PIN_UNDEFINED)

#define IF_MNGR_SPIM_IS_DEFINED(IF) (IF##_CLK_PIN_SET        != ALT1250_PIN_UNDEFINED && \
                                     IF##_MISO_PIN_SET       != ALT1250_PIN_UNDEFINED && \
                                     IF##_MOSI_PIN_SET       != ALT1250_PIN_UNDEFINED && \
                                     IF##_CPHA_DEFAULT       != ALT1250_CONFIG_UNDEFINED && \
                                     IF##_CPOL_DEFAULT       != ALT1250_CONFIG_UNDEFINED && \
                                     IF##_ENDIAN_DEFAULT     != ALT1250_CONFIG_UNDEFINED && \
                                     IF##_BITORDER_DEFAULT   != ALT1250_CONFIG_UNDEFINED && \
                                     IF##_BUSSPEED_DEFAULT   != ALT1250_CONFIG_UNDEFINED && \
                                     IF##_DATABITS_DEFAULT   != ALT1250_CONFIG_UNDEFINED && \
                                     IF##_SSMODE_DEFAULT     != ALT1250_CONFIG_UNDEFINED)

#define IF_MNGR_ANY_SPIM_IS_DEFINED (IF_MNGR_SPIM_IS_DEFINED(SPIM0) || IF_MNGR_SPIM_IS_DEFINED(SPIM1))

#define IF_MNGR_SPIS_IS_DEFINED(IF) (IF##_CLK_PIN_SET        != ALT1250_PIN_UNDEFINED && \
                                     IF##_EN_PIN_SET         != ALT1250_PIN_UNDEFINED && \
                                     IF##_MISO_PIN_SET       != ALT1250_PIN_UNDEFINED && \
                                     IF##_MOSI_PIN_SET       != ALT1250_PIN_UNDEFINED && \
                                     IF##_CPHA_DEFAULT       != ALT1250_CONFIG_UNDEFINED && \
                                     IF##_CPOL_DEFAULT       != ALT1250_CONFIG_UNDEFINED && \
                                     IF##_ENDIAN_DEFAULT     != ALT1250_CONFIG_UNDEFINED && \
                                     IF##_SSMODE_DEFAULT     != ALT1250_CONFIG_UNDEFINED)

#define IF_MNGR_ANY_SPIS_IS_DEFINED (IF_MNGR_SPIS_IS_DEFINED(SPIS0))

#define IF_MNGR_I2C_IS_DEFINED(IF)  (IF##_SDA_PIN_SET        != ALT1250_PIN_UNDEFINED && \
                                     IF##_SCL_PIN_SET        != ALT1250_PIN_UNDEFINED)

#define IF_MNGR_ANY_I2C_IS_DEFINED (IF_MNGR_I2C_IS_DEFINED(I2C0) || IF_MNGR_I2C_IS_DEFINED(I2C1))

#define IF_MNGR_GPIO_IS_DEFINED(N) ((GPIO_ID_##N##_PIN_SET     != ALT1250_PIN_UNDEFINED) && \
                                    (GPIO_ID_##N##_DIR_DEFAULT != ALT1250_CONFIG_UNDEFINED) && \
                                    (GPIO_ID_##N##_PU_DEFAULT  != ALT1250_CONFIG_UNDEFINED) && \
                                    (GPIO_ID_##N##_VAL_DEFAULT != ALT1250_CONFIG_UNDEFINED))

#define IF_MNGR_ANY_GPIO_IS_DEFINED  IF_MNGR_GPIO_IS_DEFINED(01) || \
                                     IF_MNGR_GPIO_IS_DEFINED(02) || \
                                     IF_MNGR_GPIO_IS_DEFINED(03) || \
                                     IF_MNGR_GPIO_IS_DEFINED(04) || \
                                     IF_MNGR_GPIO_IS_DEFINED(05) || \
                                     IF_MNGR_GPIO_IS_DEFINED(06) || \
                                     IF_MNGR_GPIO_IS_DEFINED(07) || \
                                     IF_MNGR_GPIO_IS_DEFINED(08) || \
                                     IF_MNGR_GPIO_IS_DEFINED(09) || \
                                     IF_MNGR_GPIO_IS_DEFINED(10) || \
                                     IF_MNGR_GPIO_IS_DEFINED(11) || \
                                     IF_MNGR_GPIO_IS_DEFINED(12) || \
                                     IF_MNGR_GPIO_IS_DEFINED(13) || \
                                     IF_MNGR_GPIO_IS_DEFINED(14) || \
                                     IF_MNGR_GPIO_IS_DEFINED(15)

#define IF_MNGR_LAST_GPIO IF_MNGR_GPIO15

#define IF_MNGR_ANY_WAKEUP_IS_DEFINED IF_MNGR_WAKEUP_IS_DEFINED(01) || \
                                      IF_MNGR_WAKEUP_IS_DEFINED(02) || \
                                      IF_MNGR_WAKEUP_IS_DEFINED(03) || \
                                      IF_MNGR_WAKEUP_IS_DEFINED(04)

#define IF_MNGR_WAKEUP_IS_DEFINED(N) ((WAKEUP_IO_ID_##N##_PIN_SET != ALT1250_PIN_UNDEFINED) && \
                                      (WAKEUP_IO_ID_##N##_PIN_POL != ALT1250_CONFIG_UNDEFINED))

#define IF_MNGR_LED_IS_DEFINED(IF) ( IF##_PIN_SET != ALT1250_PIN_UNDEFINED )

#define IF_MNGR_PWM_IS_DEFINED(IF)  (IF##_PIN_SET            != ALT1250_PIN_UNDEFINED && \
                                     IF##_CLK_DIV_DEFAULT    != ALT1250_CONFIG_UNDEFINED && \
                                     IF##_DUTY_CYCLE_DEFAULT != ALT1250_CONFIG_UNDEFINED)

#define IF_MNGR_ANY_PWM_IS_DEFINED (IF_MNGR_PWM_IS_DEFINED(PWM0) || \
                                    IF_MNGR_PWM_IS_DEFINED(PWM1) || \
                                    IF_MNGR_PWM_IS_DEFINED(PWM2) || \
                                    IF_MNGR_PWM_IS_DEFINED(PWM3))
// clang-format on
/*! @endcond */

/****************************************************************************
 * Data types
 ****************************************************************************/
/**
 * @defgroup if_mngr Interface Manager
 * @{
 */
/**
 * @defgroup if_mngr_data_types Interface Manager Types
 * @{
 */
/**
 * @typedef eIfMngrIf
 * Definition of interface ID.
 */
typedef enum {
  IF_MNGR_UARTF0,
  IF_MNGR_UARTF1,
  IF_MNGR_SPIM0,
  IF_MNGR_SPIM1,
  IF_MNGR_SPIS0,
  IF_MNGR_I2C0,
  IF_MNGR_I2C1,
  IF_MNGR_GPIO01,
  IF_MNGR_GPIO02,
  IF_MNGR_GPIO03,
  IF_MNGR_GPIO04,
  IF_MNGR_GPIO05,
  IF_MNGR_GPIO06,
  IF_MNGR_GPIO07,
  IF_MNGR_GPIO08,
  IF_MNGR_GPIO09,
  IF_MNGR_GPIO10,
  IF_MNGR_GPIO11,
  IF_MNGR_GPIO12,
  IF_MNGR_GPIO13,
  IF_MNGR_GPIO14,
  IF_MNGR_GPIO15, /* Make sure IF_MNGR_LAST_GPIO re-defined if number of GPIO changed */
  IF_MNGR_WAKEUP01,
  IF_MNGR_WAKEUP02,
  IF_MNGR_WAKEUP03,
  IF_MNGR_WAKEUP04,
  IF_MNGR_LED0,
  IF_MNGR_LED1,
  IF_MNGR_LED2,
  IF_MNGR_LED3,
  IF_MNGR_LED4,
  IF_MNGR_LED5,
  IF_MNGR_PWM0,
  IF_MNGR_PWM1,
  IF_MNGR_PWM2,
  IF_MNGR_PWM3,
  IF_MNGR_LAST_IF
} eIfMngrIf;
/**
 * @typedef eIfMngrRet
 * Definition of interface manager API return code.
 */
typedef enum {
  IF_MNGR_SUCCESS, /**< API returns with no error */
  IF_MNGR_FAILED   /**< API returns with error */
} eIfMngrRet;
/** @} if_mngr_data_types */

/*******************************************************************************
 * API
 ******************************************************************************/
/**
 * @defgroup if_mngr_apis Interface Manager APIs
 * @{
 */
/**
 * @brief Load default configuration defined in interface_config_alt1250.h
 *
 * @param [in]  intf: Specify interface ID with type @ref eIfMngrIf.
 * @param [out] config: Store the default configuration.
 *
 * @return @ref eIfMngrRet.
 */
eIfMngrRet if_mngr_load_defconfig(eIfMngrIf intf, void *config);
/**
 * @brief Set io mux configuration defined in interface_config_alt1250.h
 *
 * @param [in] intf: Specify interface ID with type @ref eIfMngrIf.
 *
 * @return @ref eIfMngrRet.
 */
eIfMngrRet if_mngr_config_io(eIfMngrIf intf);
/** @} if_mngr_apis */
/** @} if_mngr */
#endif
