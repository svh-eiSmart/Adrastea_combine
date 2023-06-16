/*  ---------------------------------------------------------------------------

    (c) copyright 2021 Altair Semiconductor, Ltd. All rights reserved.

    This software, in source or object form (the "Software"), is the
    property of Altair Semiconductor Ltd. (the "Company") and/or its
    licensors, which have all right, title and interest therein, You
    may use the Software only in  accordance with the terms of written
    license agreement between you and the Company (the "License").
    Except as expressly stated in the License, the Company grants no
    licenses by implication, estoppel, or otherwise. If you are not
    aware of or do not agree to the License terms, you may not use,
    copy or modify the Software. You may use the source code of the
    Software only for your internal purposes and may not distribute the
    source code of the Software, any part thereof, or any derivative work
    thereof, to any third party, except pursuant to the Company's prior
    written consent.
    The Software is the confidential information of the Company.

   ------------------------------------------------------------------------- */
/**
 * @file DRV_IF_CFG.h
 */
#ifndef DRV_IF_CFG_H_
#define DRV_IF_CFG_H_
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "if_cfg/interfaces_config_alt1250.h"
#include <stdint.h>
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/*! @cond Doxygen_Suppress */
// clang-format off
#define IF_CFG_UART_IS_DEFINED(IF) (IF##_RX_PIN_SET         != ALT1250_PIN_UNDEFINED && \
                                    IF##_TX_PIN_SET         != ALT1250_PIN_UNDEFINED && \
                                    IF##_BAUDRATE_DEFAULT   != ALT1250_CONFIG_UNDEFINED && \
                                    IF##_HWFLOWCTRL_DEFAULT != ALT1250_CONFIG_UNDEFINED && \
                                    IF##_PARITY_DEFAULT     != ALT1250_CONFIG_UNDEFINED && \
                                    IF##_STOPBITS_DEFAULT   != ALT1250_CONFIG_UNDEFINED && \
                                    IF##_WORDLENGTH_DEFAULT != ALT1250_CONFIG_UNDEFINED)

#define IF_CFG_ANY_UART_IS_DEFINED (IF_CFG_UART_IS_DEFINED(UARTF0) || IF_CFG_UART_IS_DEFINED(UARTF1))

#define IF_CFG_UART_RTS_CTS_IS_DEFINED(IF) (IF##_RTS_PIN_SET != ALT1250_PIN_UNDEFINED && \
                                            IF##_CTS_PIN_SET != ALT1250_PIN_UNDEFINED)

#define IF_CFG_SPIM_IS_DEFINED(IF) (IF##_CLK_PIN_SET        != ALT1250_PIN_UNDEFINED && \
                                    IF##_MISO_PIN_SET       != ALT1250_PIN_UNDEFINED && \
                                    IF##_MOSI_PIN_SET       != ALT1250_PIN_UNDEFINED && \
                                    IF##_CPHA_DEFAULT       != ALT1250_CONFIG_UNDEFINED && \
                                    IF##_CPOL_DEFAULT       != ALT1250_CONFIG_UNDEFINED && \
                                    IF##_ENDIAN_DEFAULT     != ALT1250_CONFIG_UNDEFINED && \
                                    IF##_BITORDER_DEFAULT   != ALT1250_CONFIG_UNDEFINED && \
                                    IF##_BUSSPEED_DEFAULT   != ALT1250_CONFIG_UNDEFINED && \
                                    IF##_DATABITS_DEFAULT   != ALT1250_CONFIG_UNDEFINED && \
                                    IF##_SSMODE_DEFAULT     != ALT1250_CONFIG_UNDEFINED)

#define IF_CFG_ANY_SPIM_IS_DEFINED (IF_CFG_SPIM_IS_DEFINED(SPIM0) || IF_CFG_SPIM_IS_DEFINED(SPIM1))

#define IF_CFG_SPIS_IS_DEFINED(IF) (IF##_CLK_PIN_SET        != ALT1250_PIN_UNDEFINED && \
                                    IF##_MISO_PIN_SET       != ALT1250_PIN_UNDEFINED && \
                                    IF##_MOSI_PIN_SET       != ALT1250_PIN_UNDEFINED && \
                                    IF##_CPHA_DEFAULT       != ALT1250_CONFIG_UNDEFINED && \
                                    IF##_CPOL_DEFAULT       != ALT1250_CONFIG_UNDEFINED && \
                                    IF##_ENDIAN_DEFAULT     != ALT1250_CONFIG_UNDEFINED && \
                                    IF##_SSMODE_DEFAULT     != ALT1250_CONFIG_UNDEFINED)

#define IF_CFG_ANY_SPIS_IS_DEFINED (IF_CFG_SPIS_IS_DEFINED(SPIS0))

#define IF_CFG_I2C_IS_DEFINED(IF)  (IF##_SDA_PIN_SET        != ALT1250_PIN_UNDEFINED && \
                                    IF##_SCL_PIN_SET        != ALT1250_PIN_UNDEFINED)

#define IF_CFG_ANY_I2C_IS_DEFINED (IF_CFG_I2C_IS_DEFINED(I2C0) || IF_CFG_I2C_IS_DEFINED(I2C1))

#define IF_CFG_GPIO_IS_DEFINED(N) ((GPIO_ID_##N##_PIN_SET     != ALT1250_PIN_UNDEFINED) && \
                                   (GPIO_ID_##N##_DIR_DEFAULT != ALT1250_CONFIG_UNDEFINED) && \
                                   (GPIO_ID_##N##_PU_DEFAULT  != ALT1250_CONFIG_UNDEFINED) && \
                                   (GPIO_ID_##N##_VAL_DEFAULT != ALT1250_CONFIG_UNDEFINED))

#define IF_CFG_ANY_GPIO_IS_DEFINED  (IF_CFG_GPIO_IS_DEFINED(01) || \
                                     IF_CFG_GPIO_IS_DEFINED(02) || \
                                     IF_CFG_GPIO_IS_DEFINED(03) || \
                                     IF_CFG_GPIO_IS_DEFINED(04) || \
                                     IF_CFG_GPIO_IS_DEFINED(05) || \
                                     IF_CFG_GPIO_IS_DEFINED(06) || \
                                     IF_CFG_GPIO_IS_DEFINED(07) || \
                                     IF_CFG_GPIO_IS_DEFINED(08) || \
                                     IF_CFG_GPIO_IS_DEFINED(09) || \
                                     IF_CFG_GPIO_IS_DEFINED(10) || \
                                     IF_CFG_GPIO_IS_DEFINED(11) || \
                                     IF_CFG_GPIO_IS_DEFINED(12) || \
                                     IF_CFG_GPIO_IS_DEFINED(13) || \
                                     IF_CFG_GPIO_IS_DEFINED(14) || \
                                     IF_CFG_GPIO_IS_DEFINED(15))

#define IF_CFG_LAST_GPIO IF_CFG_GPIO15

#define IF_CFG_ANY_WAKEUP_IS_DEFINED (IF_CFG_WAKEUP_IS_DEFINED(01) || \
                                      IF_CFG_WAKEUP_IS_DEFINED(02) || \
                                      IF_CFG_WAKEUP_IS_DEFINED(03) || \
                                      IF_CFG_WAKEUP_IS_DEFINED(04))

#define IF_CFG_WAKEUP_IS_DEFINED(N) ((WAKEUP_IO_ID_##N##_PIN_SET != ALT1250_PIN_UNDEFINED) && \
                                      (WAKEUP_IO_ID_##N##_PIN_POL != ALT1250_CONFIG_UNDEFINED))

#define IF_CFG_LED_IS_DEFINED(IF) ( IF##_PIN_SET != ALT1250_PIN_UNDEFINED )

#define IF_CFG_PWM_IS_DEFINED(IF)  (IF##_PIN_SET            != ALT1250_PIN_UNDEFINED && \
                                    IF##_CLK_DIV_DEFAULT    != ALT1250_CONFIG_UNDEFINED && \
                                    IF##_DUTY_CYCLE_DEFAULT != ALT1250_CONFIG_UNDEFINED)

#define IF_CFG_ANY_PWM_IS_DEFINED (IF_CFG_PWM_IS_DEFINED(PWM0) || \
                                   IF_CFG_PWM_IS_DEFINED(PWM1) || \
                                   IF_CFG_PWM_IS_DEFINED(PWM2) || \
                                   IF_CFG_PWM_IS_DEFINED(PWM3))
// clang-format on
/*! @endcond */

/****************************************************************************
 * Data types
 ****************************************************************************/
/**
 * @defgroup if_cfg Interface Configuration Utility
 * @{
 */
/**
 * @defgroup if_cfg_data_types Interface Configuration Types
 * @{
 */
/**
 * @typedef Interface_Id
 * Definition of interface ID.
 */
typedef enum {
  IF_CFG_UARTF0,
  IF_CFG_UARTF1,
  IF_CFG_SPIM0,
  IF_CFG_SPIM1,
  IF_CFG_SPIS0,
  IF_CFG_I2C0,
  IF_CFG_I2C1,
  IF_CFG_GPIO01,
  IF_CFG_GPIO02,
  IF_CFG_GPIO03,
  IF_CFG_GPIO04,
  IF_CFG_GPIO05,
  IF_CFG_GPIO06,
  IF_CFG_GPIO07,
  IF_CFG_GPIO08,
  IF_CFG_GPIO09,
  IF_CFG_GPIO10,
  IF_CFG_GPIO11,
  IF_CFG_GPIO12,
  IF_CFG_GPIO13,
  IF_CFG_GPIO14,
  IF_CFG_GPIO15, /* Make sure IF_CFG_LAST_GPIO re-defined if number of GPIO changed */
  IF_CFG_WAKEUP01,
  IF_CFG_WAKEUP02,
  IF_CFG_WAKEUP03,
  IF_CFG_WAKEUP04,
  IF_CFG_LED0,
  IF_CFG_LED1,
  IF_CFG_LED2,
  IF_CFG_LED3,
  IF_CFG_LED4,
  IF_CFG_LED5,
  IF_CFG_PWM0,
  IF_CFG_PWM1,
  IF_CFG_PWM2,
  IF_CFG_PWM3,
  IF_CFG_LAST_IF
} Interface_Id;
/**
 * @typedef DRV_IF_CFG_Status
 * Definition of interface config API return code.
 */
typedef enum {
  DRV_IF_CFG_OK,               /**< API returns with no error */
  DRV_IF_CFG_ERROR_UNDEFINED,  /**< API returns with no definition in interface header */
  DRV_IF_CFG_ERROR_CONFIG,     /**< API returns with error configuration in interface header */
  DRV_IF_CFG_ERROR_IOPAR,      /**< API returns with iopar error */
  DRV_IF_CFG_ERROR_UNSUPPORTED /**< API returns with error not support */
} DRV_IF_CFG_Status;
/** @} if_cfg_data_types */

/*******************************************************************************
 * API
 ******************************************************************************/
#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif
/**
 * @defgroup if_cfg_apis Interface Configuration APIs
 * @{
 */
/**
 * @brief Load default configuration defined in interfaces_config_alt1250.h
 *
 * @param [in]  intf: Specify interface ID with type @ref IF_CFG_InterfaceId.
 * @param [out] config: Store the default configuration.
 *
 * @return @ref DRV_IF_CFG_Status.
 */
DRV_IF_CFG_Status DRV_IF_CFG_GetDefConfig(Interface_Id intf, void *config);
/**
 * @brief Set io mux configuration defined in interfaces_config_alt1250.h
 *
 * @param [in] intf: Specify interface ID with type @ref IF_CFG_InterfaceId.
 *
 * @return @ref DRV_IF_CFG_Status.
 */
DRV_IF_CFG_Status DRV_IF_CFG_SetIO(Interface_Id intf);
/** @} if_cfg_apis */
#undef EXTERN
#ifdef __cplusplus
}
#endif
/** @} if_cfg */
#endif /*DRV_IF_CFG_H_*/
