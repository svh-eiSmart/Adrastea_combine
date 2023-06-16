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
#ifndef DRV_CLKGATING_H_
#define DRV_CLKGATING_H_

#include <stdint.h>

/**
 * @defgroup drv_clk_gating Clock Gating Driver
 * @{
 */

/****************************************************************************
 * Data Type
 ****************************************************************************/
/**
 * @defgroup drv_clk_gating_data_type Clock Gating Data Types
 * @{
 */

/** @brief software clock gating source list.*/
typedef enum {
  CLK_GATING_PCM,
  CLK_GATING_PWM,
  CLK_GATING_SHADOW_32KHZ,
  CLK_GATING_TIMERS,
  CLK_GATING_SIC,
  CLK_GATING_BIST,
  CLK_GATING_CLK32K_LED,
  CLK_GATING_DEBUG,
  CLK_GATING_UARTI0,
  CLK_GATING_UARTI1,
  CLK_GATING_UARTI2,
  CLK_GATING_UARTF0,
  CLK_GATING_UARTF1,
  CLK_GATING_GPIO_IF
} Clock_Gating_Source;

/** @} drv_clk_gating_data_type */

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/
#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif
/**
 * @defgroup drv_clk_gating_apis Clock Gating APIs
 * @{
 */
/**
 * @brief Initialize clock gating source.
 *
 * @param [in] None.
 * @return None.
 */
void DRV_CLKGATING_Initialize(void);

/**
 * @brief This function would enable MCU clock gating SW contol clock source.
 *
 * @param [in] source: Clock gating source (SW).
 *
 * @return error code. 0-success; other-fail.
 */
int32_t DRV_CLKGATING_EnableClkSource(Clock_Gating_Source source);

/**
 * @brief This function would disable MCU clock gating SW contol clock source.
 *
 * @param [in] source: Clock gating source (SW).
 *
 * @return error code. 0-success; other-fail.
 */
int32_t DRV_CLKGATING_DisableClkSource(Clock_Gating_Source source);
/** @} drv_clk_gating_apis */
#undef EXTERN
#ifdef __cplusplus
}
#endif
/** @} drv_clk_gating */

#endif /* DRV_CLKGATING_H_ */
