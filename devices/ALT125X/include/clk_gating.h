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
#ifndef CLK_GATING_H
#define CLK_GATING_H

/* software clock gating source list */
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
} ClkGating_t;

/*-----------------------------------------------------------------------------
 * void clk_gating_init(void)
 * PURPOSE: This function would initialize clock gating source.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  None
 *-----------------------------------------------------------------------------
 */
void clk_gating_init(void);

/*-----------------------------------------------------------------------------
 * int clk_gating_source_enable(ClkGating_t source)
 * PURPOSE: This function would enable MCU clock gating SW contol clock source.
 * PARAMs:
 *      INPUT:  Clock gating source (SW).
 *      OUTPUT: None
 * RETURN:  error code. 0-success; other-fail
 *-----------------------------------------------------------------------------
 */
int clk_gating_source_enable(ClkGating_t source);

/*-----------------------------------------------------------------------------
 * int clk_gating_source_disable(ClkGating_t source)
 * PURPOSE: This function would disable MCU clock gating SW contol clock source.
 * PARAMs:
 *      INPUT:  Clock gating source (SW).
 *      OUTPUT: None
 * RETURN:  error code. 0-success; other-fail
 *-----------------------------------------------------------------------------
 */
int clk_gating_source_disable(ClkGating_t source);

#endif
