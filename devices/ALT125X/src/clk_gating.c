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

/* Standard includes. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "125X_mcu.h"
#include "clk_gating.h"
/*-----------------------------------------------------------------------------
 * void clk_gating_init(void)
 * PURPOSE: This function would initialize clock gating source.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  None
 *-----------------------------------------------------------------------------
 */
void clk_gating_init(void) {
  /* ========== Set to hw controlled ========== */
  /* ERR-369 */
  // REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_BYP) &= ~MCU_CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_REGMCU_MSK;
  // //[28] MCU_REGMCU

  /* ERR-304, ERR-306, ERR-339 */
  // REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_BYP) &= ~MCU_CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_SSX_GPM_MSK;
  // //[27] MCU_SSX_GPM REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_BYP) &=
  // ~MCU_CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_SSX_PMP_MSK;    //[26] MCU_SSX_PMP
  // REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_BYP) &= ~MCU_CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_SSX_MSK;
  // //[3] MCU_SSX

#ifdef MCU_CLK_CONTROL_MCU_CLK_REQ_BYP_MSE_BRIDGE_MSK
  REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_BYP) &=
      ~MCU_CLK_CONTROL_MCU_CLK_REQ_BYP_MSE_BRIDGE_MSK;  //[31] MSE_BRIDGE
#endif
  REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_BYP) &=
      ~MCU_CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_IOSEL_MSK;  //[23] MCU_IOSEL
  REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_BYP) &=
      ~MCU_CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_UARTF0_OCP_MSK;  //[19] MCU_UARTF0_OCP
#ifdef MCU_CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_UARTF1_OCP_MSK
  REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_BYP) &=
      ~MCU_CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_UARTF1_OCP_MSK;  //[18] MCU_UARTF1_OCP
#endif
  REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_BYP) &=
      ~MCU_CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_M4_MSK;  //[17] MCU_M4
  REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_BYP) &=
      ~MCU_CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_SPI_MASTER0_MSK;  //[15] MCU_SPI_MASTER0
#ifdef MCU_CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_SPI_MASTER1_MSK
  REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_BYP) &=
      ~MCU_CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_SPI_MASTER1_MSK;  //[14] MCU_SPI_MASTER1
#endif
  REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_BYP) &=
      ~MCU_CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_SPI_SLAVE_MSK;  //[13] MCU_SPI_SLAVE
#ifdef MCU_CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_SF_MSK
  REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_BYP) &=
      ~MCU_CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_SF_MSK;  //[7] MCU_SF
#endif
  REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_BYP) &=
      ~MCU_CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_GDMA_MSK;  //[6] MCU_GDMA
  REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_BYP) &=
      ~MCU_CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_I2C0_MSK;  //[5] MCU_I2C0
#ifdef MCU_CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_I2C1_MSK
  REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_BYP) &=
      ~MCU_CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_I2C1_MSK;  //[4] MCU_I2C1
#endif
  REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_BYP) &=
      ~MCU_CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_GPIO_RF_MSK;  //[1] MCU_GPIO_RF
  REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_BYP) &=
      ~MCU_CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_LED_MSK;  //[0] MCU_LED

  /* ========== Disable unused devices in order to reduce power consumption ========== */
#ifdef MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_PCM_MSK
  REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) &=
      ~MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_PCM_MSK;  //[30] MCU_PCM
#endif
  REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) &=
      ~MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_PWM_MSK;  //[29] MCU_PWM
  // REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) &=
  //    ~MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_SHADOW_32KHZ_MSK;  //[25] MCU_SHADOW_32KHZ
  // REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) &= ~MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_TIMERS_MSK;
  // //[24] MCU_TIMERS. SW & ON
#ifdef MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_SIC_MSK
  REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) &=
      ~MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_SIC_MSK;  //[22] MCU_SIC
#endif
  REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) &=
      ~MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_BIST_MSK;  //[21] MCU_BIST
  REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) &=
      ~MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_CLK32K_LED_MSK;  //[20] MCU_CLK32K_LED
  REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) &=
      ~MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_DEBUG_MSK;  //[16] MCU_DEBUG
  // REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) &= ~MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_UARTI0_MSK;
  // //[12] MCU_UARTI0, HiFC
  REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) &=
      ~MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_UARTI1_MSK;  //[11] MCU_UARTI1
#ifdef MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_UARTI2_MSK
  REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) &=
      ~MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_UARTI2_MSK;  //[10] MCU_UARTI2
#endif
  // REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) &= ~MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_UARTF0_MSK;
  // //[9] MCU_UARTF0_MSK, SW & ON REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) &=
  // ~MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_UARTF1_MSK;       //[8] MCU_UARTF1
  REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) &=
      ~MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_GPIO_IF_MSK;  //[2] MCU_GPIO_IF
}

/*-----------------------------------------------------------------------------
 * int clk_gating_source_enable(ClkGating_t source)
 * PURPOSE: This function would enable MCU clock gating SW contol clock source.
 * PARAMs:
 *      INPUT:  Clock gating source (SW).
 *      OUTPUT: None
 * RETURN:  error code. 0-success; other-fail
 *-----------------------------------------------------------------------------
 */
int clk_gating_source_enable(ClkGating_t source) {
  switch (source) {
#ifdef MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_PCM_MSK
    case CLK_GATING_PCM:
      REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) |= MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_PCM_MSK;
      break;
#endif
    case CLK_GATING_PWM:
      REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) |= MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_PWM_MSK;
      break;
    case CLK_GATING_SHADOW_32KHZ:
      REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) |=
          MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_SHADOW_32KHZ_MSK;
      break;
    case CLK_GATING_TIMERS:
      REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) |= MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_TIMERS_MSK;
      break;
#ifdef MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_SIC_MSK
    case CLK_GATING_SIC:
      REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) |= MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_SIC_MSK;
      break;
#endif
    case CLK_GATING_BIST:
      REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) |= MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_BIST_MSK;
      break;
    case CLK_GATING_CLK32K_LED:
      REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) |= MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_CLK32K_LED_MSK;
      break;
    case CLK_GATING_DEBUG:
      REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) |= MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_DEBUG_MSK;
      break;
    case CLK_GATING_UARTI0:
      REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) |= MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_UARTI0_MSK;
      break;
    case CLK_GATING_UARTI1:
      REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) |= MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_UARTI1_MSK;
      break;
#ifdef MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_UARTI2_MSK
    case CLK_GATING_UARTI2:
      REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) |= MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_UARTI2_MSK;
      break;
#endif
    case CLK_GATING_UARTF0:
      REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) |= MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_UARTF0_MSK;
      break;
#ifdef MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_UARTF1_MSK
    case CLK_GATING_UARTF1:
      REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) |= MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_UARTF1_MSK;
      break;
#endif
    case CLK_GATING_GPIO_IF:
      REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) |= MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_GPIO_IF_MSK;
      break;
    default:
      return (-1);
  }
  return (0);
}

/*-----------------------------------------------------------------------------
 * int clk_gating_source_disable(ClkGating_t source)
 * PURPOSE: This function would disable MCU clock gating SW contol clock source.
 * PARAMs:
 *      INPUT:  Clock gating source (SW).
 *      OUTPUT: None
 * RETURN:  error code. 0-success; other-fail
 *-----------------------------------------------------------------------------
 */
int clk_gating_source_disable(ClkGating_t source) {
  switch (source) {
#ifdef MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_PCM_MSK
    case CLK_GATING_PCM:
      REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) &= ~MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_PCM_MSK;
      break;
#endif
    case CLK_GATING_PWM:
      REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) &= ~MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_PWM_MSK;
      break;
    case CLK_GATING_SHADOW_32KHZ:
      REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) &=
          ~MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_SHADOW_32KHZ_MSK;
      break;
    case CLK_GATING_TIMERS:
      REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) &= ~MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_TIMERS_MSK;
      break;
#ifdef MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_SIC_MSK
    case CLK_GATING_SIC:
      REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) &= ~MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_SIC_MSK;
      break;
#endif
    case CLK_GATING_BIST:
      REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) &= ~MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_BIST_MSK;
      break;
    case CLK_GATING_CLK32K_LED:
      REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) &=
          ~MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_CLK32K_LED_MSK;
      break;
    case CLK_GATING_DEBUG:
      REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) &= ~MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_DEBUG_MSK;
      break;
    case CLK_GATING_UARTI0:
      REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) &= ~MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_UARTI0_MSK;
      break;
    case CLK_GATING_UARTI1:
      REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) &= ~MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_UARTI1_MSK;
      break;
#ifdef MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_UARTI2_MSK
    case CLK_GATING_UARTI2:
      REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) &= ~MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_UARTI2_MSK;
      break;
#endif
    case CLK_GATING_UARTF0:
      REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) &= ~MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_UARTF0_MSK;
      break;
#ifdef MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_UARTF1_MSK
    case CLK_GATING_UARTF1:
      REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) &= ~MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_UARTF1_MSK;
      break;
#endif
    case CLK_GATING_GPIO_IF:
      REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) &= ~MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_GPIO_IF_MSK;
      break;
    default:
      return (-1);
  }
  return (0);
}
