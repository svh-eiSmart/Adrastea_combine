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
/* Standard includes. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include DEVICE_HEADER

#include "DRV_CLOCK_GATING.h"

static CLK_CONTROL_Type *clk_ctrl_reg = CLK_CONTROL;

void DRV_CLKGATING_Initialize(void) {
  /* ========== Set to hw controlled ========== */
  /* ERR-369 */
  // clk_ctrl_reg->MCU_CLK_REQ_BYP &= ~CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_REGMCU_Msk;
  // //[28] MCU_REGMCU

  /* ERR-304, ERR-306, ERR-339 */
  // clk_ctrl_reg->MCU_CLK_REQ_BYP &= ~CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_SSX_GPM_Msk;
  // //[27] MCU_SSX_GPM clk_ctrl_reg->MCU_CLK_REQ_BYP &=
  // ~CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_SSX_PMP_Msk;    //[26] MCU_SSX_PMP
  // clk_ctrl_reg->MCU_CLK_REQ_BYP &= ~CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_SSX_Msk;
  // //[3] MCU_SSX

#ifdef CLK_CONTROL_MCU_CLK_REQ_BYP_MSE_BRIDGE_Msk
  clk_ctrl_reg->MCU_CLK_REQ_BYP &= ~CLK_CONTROL_MCU_CLK_REQ_BYP_MSE_BRIDGE_Msk;  //[31] MSE_BRIDGE
#endif
  clk_ctrl_reg->MCU_CLK_REQ_BYP &= ~CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_IOSEL_Msk;  //[23] MCU_IOSEL
  clk_ctrl_reg->MCU_CLK_REQ_BYP &=
      ~CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_UARTF0_OCP_Msk;  //[19] MCU_UARTF0_OCP
#ifdef CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_UARTF1_OCP_Msk
  clk_ctrl_reg->MCU_CLK_REQ_BYP &=
      ~CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_UARTF1_OCP_Msk;  //[18] MCU_UARTF1_OCP
#endif
  clk_ctrl_reg->MCU_CLK_REQ_BYP &= ~CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_M4_Msk;  //[17] MCU_M4
  clk_ctrl_reg->MCU_CLK_REQ_BYP &=
      ~CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_SPI_MASTER0_Msk;  //[15] MCU_SPI_MASTER0
#ifdef CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_SPI_MASTER1_Msk
  clk_ctrl_reg->MCU_CLK_REQ_BYP &=
      ~CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_SPI_MASTER1_Msk;  //[14] MCU_SPI_MASTER1
#endif
  clk_ctrl_reg->MCU_CLK_REQ_BYP &=
      ~CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_SPI_SLAVE_Msk;  //[13] MCU_SPI_SLAVE
#ifdef CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_SF_Msk
  clk_ctrl_reg->MCU_CLK_REQ_BYP &= ~CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_SF_Msk;  //[7] MCU_SF
#endif
  clk_ctrl_reg->MCU_CLK_REQ_BYP &= ~CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_GDMA_Msk;  //[6] MCU_GDMA
  clk_ctrl_reg->MCU_CLK_REQ_BYP &= ~CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_I2C0_Msk;  //[5] MCU_I2C0
#ifdef MCU_CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_I2C1_Msk
  clk_ctrl_reg->MCU_CLK_REQ_BYP &= ~MCU_CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_I2C1_Msk;  //[4] MCU_I2C1
#endif
  clk_ctrl_reg->MCU_CLK_REQ_BYP &= ~CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_GPIO_RF_Msk;  //[1] MCU_GPIO_RF
  clk_ctrl_reg->MCU_CLK_REQ_BYP &= ~CLK_CONTROL_MCU_CLK_REQ_BYP_MCU_LED_Msk;      //[0] MCU_LED

  /* ========== Disable unused devices in order to reduce power consumption ========== */
#ifdef CLK_CONTROL_MCU_CLK_REQ_EN_MCU_PCM_Msk
  clk_ctrl_reg->MCU_CLK_REQ_EN &= ~CLK_CONTROL_MCU_CLK_REQ_EN_MCU_PCM_Msk;  //[30] MCU_PCM
#endif
  clk_ctrl_reg->MCU_CLK_REQ_EN &= ~CLK_CONTROL_MCU_CLK_REQ_EN_MCU_PWM_Msk;  //[29] MCU_PWM
  // clk_ctrl_reg->MCU_CLK_REQ_EN &=
  //    ~CLK_CONTROL_MCU_CLK_REQ_EN_MCU_SHADOW_32KHZ_Msk;  //[25] MCU_SHADOW_32KHZ
  // clk_ctrl_reg->MCU_CLK_REQ_EN &= ~CLK_CONTROL_MCU_CLK_REQ_EN_MCU_TIMERS_Msk;
  // //[24] MCU_TIMERS. SW & ON
#ifdef CLK_CONTROL_MCU_CLK_REQ_EN_MCU_SIC_Msk
  clk_ctrl_reg->MCU_CLK_REQ_EN &= ~CLK_CONTROL_MCU_CLK_REQ_EN_MCU_SIC_Msk;  //[22] MCU_SIC
#endif
  clk_ctrl_reg->MCU_CLK_REQ_EN &= ~CLK_CONTROL_MCU_CLK_REQ_EN_MCU_BIST_Msk;  //[21] MCU_BIST
  clk_ctrl_reg->MCU_CLK_REQ_EN &=
      ~CLK_CONTROL_MCU_CLK_REQ_EN_MCU_CLK32K_LED_Msk;                         //[20] MCU_CLK32K_LED
  clk_ctrl_reg->MCU_CLK_REQ_EN &= ~CLK_CONTROL_MCU_CLK_REQ_EN_MCU_DEBUG_Msk;  //[16] MCU_DEBUG
  // clk_ctrl_reg->MCU_CLK_REQ_EN &= ~CLK_CONTROL_MCU_CLK_REQ_EN_MCU_UARTI0_Msk;
  // //[12] MCU_UARTI0, HiFC
  clk_ctrl_reg->MCU_CLK_REQ_EN &= ~CLK_CONTROL_MCU_CLK_REQ_EN_MCU_UARTI1_Msk;  //[11] MCU_UARTI1
#ifdef CLK_CONTROL_MCU_CLK_REQ_EN_MCU_UARTI2_Msk
  clk_ctrl_reg->MCU_CLK_REQ_EN &= ~CLK_CONTROL_MCU_CLK_REQ_EN_MCU_UARTI2_Msk;  //[10] MCU_UARTI2
#endif
  // clk_ctrl_reg->MCU_CLK_REQ_EN &= ~CLK_CONTROL_MCU_CLK_REQ_EN_MCU_UARTF0_Msk;
  // //[9] MCU_UARTF0_MSK, SW & ON clk_ctrl_reg->MCU_CLK_REQ_EN &=
  // ~CLK_CONTROL_MCU_CLK_REQ_EN_MCU_UARTF1_Msk;       //[8] MCU_UARTF1
  clk_ctrl_reg->MCU_CLK_REQ_EN &= ~CLK_CONTROL_MCU_CLK_REQ_EN_MCU_GPIO_IF_Msk;  //[2] MCU_GPIO_IF
}

int32_t DRV_CLKGATING_EnableClkSource(Clock_Gating_Source source) {
  switch (source) {
#ifdef CLK_CONTROL_MCU_CLK_REQ_EN_MCU_PCM_Msk
    case CLK_GATING_PCM:
      clk_ctrl_reg->MCU_CLK_REQ_EN |= CLK_CONTROL_MCU_CLK_REQ_EN_MCU_PCM_Msk;
      break;
#endif
    case CLK_GATING_PWM:
      clk_ctrl_reg->MCU_CLK_REQ_EN |= CLK_CONTROL_MCU_CLK_REQ_EN_MCU_PWM_Msk;
      break;
    case CLK_GATING_SHADOW_32KHZ:
      clk_ctrl_reg->MCU_CLK_REQ_EN |= CLK_CONTROL_MCU_CLK_REQ_EN_MCU_SHADOW_32KHZ_Msk;
      break;
    case CLK_GATING_TIMERS:
      clk_ctrl_reg->MCU_CLK_REQ_EN |= CLK_CONTROL_MCU_CLK_REQ_EN_MCU_TIMERS_Msk;
      break;
#ifdef CLK_CONTROL_MCU_CLK_REQ_EN_MCU_SIC_Msk
    case CLK_GATING_SIC:
      clk_ctrl_reg->MCU_CLK_REQ_EN |= CLK_CONTROL_MCU_CLK_REQ_EN_MCU_SIC_Msk;
      break;
#endif
    case CLK_GATING_BIST:
      clk_ctrl_reg->MCU_CLK_REQ_EN |= CLK_CONTROL_MCU_CLK_REQ_EN_MCU_BIST_Msk;
      break;
    case CLK_GATING_CLK32K_LED:
      clk_ctrl_reg->MCU_CLK_REQ_EN |= CLK_CONTROL_MCU_CLK_REQ_EN_MCU_CLK32K_LED_Msk;
      break;
    case CLK_GATING_DEBUG:
      clk_ctrl_reg->MCU_CLK_REQ_EN |= CLK_CONTROL_MCU_CLK_REQ_EN_MCU_DEBUG_Msk;
      break;
    case CLK_GATING_UARTI0:
      clk_ctrl_reg->MCU_CLK_REQ_EN |= CLK_CONTROL_MCU_CLK_REQ_EN_MCU_UARTI0_Msk;
      break;
    case CLK_GATING_UARTI1:
      clk_ctrl_reg->MCU_CLK_REQ_EN |= CLK_CONTROL_MCU_CLK_REQ_EN_MCU_UARTI1_Msk;
      break;
#ifdef CLK_CONTROL_MCU_CLK_REQ_EN_MCU_UARTI2_Msk
    case CLK_GATING_UARTI2:
      clk_ctrl_reg->MCU_CLK_REQ_EN |= CLK_CONTROL_MCU_CLK_REQ_EN_MCU_UARTI2_Msk;
      break;
#endif
    case CLK_GATING_UARTF0:
      clk_ctrl_reg->MCU_CLK_REQ_EN |= CLK_CONTROL_MCU_CLK_REQ_EN_MCU_UARTF0_Msk;
      break;
#ifdef CLK_CONTROL_MCU_CLK_REQ_EN_MCU_UARTF1_Msk
    case CLK_GATING_UARTF1:
      clk_ctrl_reg->MCU_CLK_REQ_EN |= CLK_CONTROL_MCU_CLK_REQ_EN_MCU_UARTF1_Msk;
      break;
#endif
    case CLK_GATING_GPIO_IF:
      clk_ctrl_reg->MCU_CLK_REQ_EN |= CLK_CONTROL_MCU_CLK_REQ_EN_MCU_GPIO_IF_Msk;
      break;
    default:
      return (-1);
  }
  return (0);
}

int32_t DRV_CLKGATING_DisableClkSource(Clock_Gating_Source source) {
  switch (source) {
#ifdef CLK_CONTROL_MCU_CLK_REQ_EN_MCU_PCM_Msk
    case CLK_GATING_PCM:
      clk_ctrl_reg->MCU_CLK_REQ_EN &= ~CLK_CONTROL_MCU_CLK_REQ_EN_MCU_PCM_Msk;
      break;
#endif
    case CLK_GATING_PWM:
      clk_ctrl_reg->MCU_CLK_REQ_EN &= ~CLK_CONTROL_MCU_CLK_REQ_EN_MCU_PWM_Msk;
      break;
    case CLK_GATING_SHADOW_32KHZ:
      clk_ctrl_reg->MCU_CLK_REQ_EN &= ~CLK_CONTROL_MCU_CLK_REQ_EN_MCU_SHADOW_32KHZ_Msk;
      break;
    case CLK_GATING_TIMERS:
      clk_ctrl_reg->MCU_CLK_REQ_EN &= ~CLK_CONTROL_MCU_CLK_REQ_EN_MCU_TIMERS_Msk;
      break;
#ifdef CLK_CONTROL_MCU_CLK_REQ_EN_MCU_SIC_Msk
    case CLK_GATING_SIC:
      clk_ctrl_reg->MCU_CLK_REQ_EN &= ~CLK_CONTROL_MCU_CLK_REQ_EN_MCU_SIC_Msk;
      break;
#endif
    case CLK_GATING_BIST:
      clk_ctrl_reg->MCU_CLK_REQ_EN &= ~CLK_CONTROL_MCU_CLK_REQ_EN_MCU_BIST_Msk;
      break;
    case CLK_GATING_CLK32K_LED:
      clk_ctrl_reg->MCU_CLK_REQ_EN &= ~CLK_CONTROL_MCU_CLK_REQ_EN_MCU_CLK32K_LED_Msk;
      break;
    case CLK_GATING_DEBUG:
      clk_ctrl_reg->MCU_CLK_REQ_EN &= ~CLK_CONTROL_MCU_CLK_REQ_EN_MCU_DEBUG_Msk;
      break;
    case CLK_GATING_UARTI0:
      clk_ctrl_reg->MCU_CLK_REQ_EN &= ~CLK_CONTROL_MCU_CLK_REQ_EN_MCU_UARTI0_Msk;
      break;
    case CLK_GATING_UARTI1:
      clk_ctrl_reg->MCU_CLK_REQ_EN &= ~CLK_CONTROL_MCU_CLK_REQ_EN_MCU_UARTI1_Msk;
      break;
#ifdef CLK_CONTROL_MCU_CLK_REQ_EN_MCU_UARTI2_Msk
    case CLK_GATING_UARTI2:
      clk_ctrl_reg->MCU_CLK_REQ_EN &= ~CLK_CONTROL_MCU_CLK_REQ_EN_MCU_UARTI2_Msk;
      break;
#endif
    case CLK_GATING_UARTF0:
      clk_ctrl_reg->MCU_CLK_REQ_EN &= ~CLK_CONTROL_MCU_CLK_REQ_EN_MCU_UARTF0_Msk;
      break;
#ifdef CLK_CONTROL_MCU_CLK_REQ_EN_MCU_UARTF1_Msk
    case CLK_GATING_UARTF1:
      clk_ctrl_reg->MCU_CLK_REQ_EN &= ~CLK_CONTROL_MCU_CLK_REQ_EN_MCU_UARTF1_Msk;
      break;
#endif
    case CLK_GATING_GPIO_IF:
      clk_ctrl_reg->MCU_CLK_REQ_EN &= ~CLK_CONTROL_MCU_CLK_REQ_EN_MCU_GPIO_IF_Msk;
      break;
    default:
      return (-1);
  }
  return (0);
}
