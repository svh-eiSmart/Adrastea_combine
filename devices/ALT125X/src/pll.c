/* ---------------------------------------------------------------------------

(c) copyright 2019 Altair Semiconductor, Ltd. All rights reserved.

This software, in source or object form (the "Software"), is the
property of Altair Semiconductor Ltd. (the "Company") and/or its
licensors, which have all right, title and interest therein, You
may use the Software only in accordance with the terms of written
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

#include <stdint.h>
#include <stdlib.h>
#include "125X_mcu.h"

#define MAX_DPLL_VCO_IDIV 16
#define MAX_DPLL_VCO_MULT 64
#define REF_CLK 26000000
#define F_CPU REF_CLK

#define SET_BIT(reg, bit) ((reg) |= (1 << (bit)))
#define CLEAR_BIT(reg, bit) ((reg) &= ~(1 << (bit)))
#define TEST_BIT(reg, bit) (((reg) & (1 << (bit))) == (1 << (bit)))
#define TEST_NEG_BIT(reg, bit) (((reg) & (1 << (bit))) != (1 << (bit)))

void rcclk_init(char clk_div) {
  if (TEST_NEG_BIT(REGISTER(MCU_CLK_CONTROL_CTRL),
                   MCU_CLK_CONTROL_CTRL_GF_RST_POS)) {  // Move back to bootclk
    CLEAR_BIT(REGISTER(MCU_CLK_CONTROL_CTRL),
              MCU_CLK_CONTROL_CTRL_GF_SEL_POS); /* SW control PLL clock (not ref clock) */
    SET_BIT(REGISTER(MCU_CLK_CONTROL_CTRL),
            MCU_CLK_CONTROL_CTRL_GF_RST_POS); /* refclk/pllclk gf rst (rst=1) */
  }

  // UART is derived from MCU main clk
  CLEAR_BIT(REGISTER(MCU_CLK_CONTROL_CTRL), MCU_CLK_CONTROL_CTRL_MUX_UART0_EXT_CLK_SEL_POS);
#ifdef ALT1250
  CLEAR_BIT(REGISTER(MCU_CLK_CONTROL_CTRL), MCU_CLK_CONTROL_CTRL_MUX_UART1_EXT_CLK_SEL_POS);
#endif
  REGISTER(MCU_CLK_CONTROL_CTRL) |= clk_div
                                    << MCU_CLK_CONTROL_CTRL_DIV_POS;  // divide RC clk by parameter

  CLEAR_BIT(REGISTER(MCU_CLK_CONTROL_CTRL), MCU_CLK_CONTROL_CTRL_SEL1_POS);  // Select RC clk
  /* Switch in glitch free manner from boot clock to fast clock (MCU clock source) */
  CLEAR_BIT(REGISTER(MCU_CLK_CONTROL_CTRL),
            MCU_CLK_CONTROL_CTRL_GF_RST_POS); /* refclk/pllclk gf rst (rst=0) */
  SET_BIT(REGISTER(MCU_CLK_CONTROL_CTRL),
          MCU_CLK_CONTROL_CTRL_GF_SEL_POS); /* SW control PLL clock (not ref clock) */
}

/* Calculate the appropriate ratio of the VCO (Mult & Idiv) */
int alt1250_calc_pll_vco_mult_idiv(ulong vco, int *idiv, int *mult) {
  int i, delta = 0, curr_delta, idiv_delta, roundup, curr_roundup;
  int max_dpll_vco_idiv = (MAX_DPLL_VCO_MULT * REF_CLK) / vco;

  /* The maximum idiv is limited by the maximum multiplication value */
  if (max_dpll_vco_idiv > MAX_DPLL_VCO_IDIV) max_dpll_vco_idiv = MAX_DPLL_VCO_IDIV;

  *idiv = *mult = -1;
  for (i = 1; i <= max_dpll_vco_idiv; i++) {
    if ((curr_delta = (((vco / 1000) * i) % (REF_CLK / 1000))) == 0) {
      *mult = (((vco / 1000) * i) / (REF_CLK / 1000)) - 1;
      *idiv = i - 1;
      delta = 0; /* Ignore the fractional calculation */
      break;
    }

    /* Test for the minimum delta */
    if (((REF_CLK / 1000) - curr_delta) < curr_delta) {
      curr_delta = (REF_CLK / 1000) - curr_delta;
      /* Mult value should be rounded up */
      curr_roundup = 1;
    } else
      curr_roundup = 0;

    /* Use fractional calculation for Mult & Div */
    if (curr_delta < delta || delta == 0) {
      delta = curr_delta;
      idiv_delta = i;
      roundup = curr_roundup;
    }
  }

  if (delta) {
    *mult = (((vco / 1000) * idiv_delta) / (REF_CLK / 1000)) - 1 + roundup;
    *idiv = idiv_delta - 1;
  }

  return (*idiv >= 0 && *idiv < MAX_DPLL_VCO_IDIV && *mult >= 0 && *mult < MAX_DPLL_VCO_MULT);
}

static int alt1250_mcu_dpll_config(ulong vco_freq, ulong clk_freq) {
  int mult, idiv;

  /* Calculate the appropriate ratio of the VCO (MULT & IDIV) */
  if (alt1250_calc_pll_vco_mult_idiv(vco_freq, &idiv, &mult)) {

#ifdef MCU_CLK_CONTROL_DPLL_CFG
    uint32_t dpll_vco_val, out_div;

    /* Calculate the actual VCO frequency of the corresponding PLL */
    dpll_vco_val = (((REF_CLK / 1000) * (mult + 1)) / (idiv + 1)) * 1000;
    /* Minimum value for ODIV is "0", i.e. divided by "1" */
    out_div = (dpll_vco_val / clk_freq) ? ((dpll_vco_val / clk_freq) - 1) : 0;
    /* Enable PLL Reset */
    REGISTER(MCU_CLK_CONTROL_DPLL_RST) = (1 << MCU_CLK_CONTROL_DPLL_RST_DPLL_RST_POS);

    /* Power down the MCU DPLL*/
    REGISTER(MCU_CLK_CONTROL_DPLL_EN) = (1 << MCU_CLK_CONTROL_DPLL_EN_PWRDN_POS);

    /* Configure the MCU DPLL */
    REGISTER(MCU_CLK_CONTROL_DPLL_CFG) =
        ((0 << MCU_CLK_CONTROL_DPLL_CFG_SLIP_EN_POS) |        /*  */
         (0 << MCU_CLK_CONTROL_DPLL_CFG_TEST_POS) |           /*  */
         (1 << MCU_CLK_CONTROL_DPLL_CFG_INTFB_POS) |          /*  */
         (0 << MCU_CLK_CONTROL_DPLL_CFG_BYPSS_POS) |          /*  */
         (out_div << MCU_CLK_CONTROL_DPLL_CFG_OD_POS) |       /* Output division  */
         (idiv << MCU_CLK_CONTROL_DPLL_CFG_NR_POS) |          /* Input division */
         (mult << MCU_CLK_CONTROL_DPLL_CFG_NF_POS) |          /* Multiplication value  */
         ((mult - 1) << MCU_CLK_CONTROL_DPLL_CFG_BWADJ_POS)); /* Bandwidth ajusment */

    /* Power up the MCU DPLL*/
    REGISTER(MCU_CLK_CONTROL_DPLL_EN) = (0 << MCU_CLK_CONTROL_DPLL_EN_PWRDN_POS);
    delayus(REF_CLK, 10);
    /* Release PLL Reset */
    REGISTER(MCU_CLK_CONTROL_DPLL_RST) &= ~MCU_CLK_CONTROL_DPLL_RST_DPLL_RST_MSK;
#endif

    /* Wait for 500 cycles of REF_CLK/(NR + 1) */
    delayus(REF_CLK, (500 * (idiv + 1) * 1000000) / REF_CLK);

    return 1;
  } else
    return 0;
}

#define CONFIG_ALT1250_MCU_GF_CLKSRC \
  1 /* Add MCUCLK domain to rcclk divided  pll_clk or 4.19  per sel0 & sel1 */
#define CONFIG_ALT1250_MCU_SEL1_CLKSRC 1    /* Add MCUCLK domain to pll or 4.19 (per sel0) */
#define CONFIG_ALT1250_MCU_SEL0_CLKSRC 0    /* Add MCUCLK domain to pll_clk */
#define CONFIG_ALT1250_MCU_SEL_PLL_CLKSRC 0 /* Add MCUCLK domain to refclk */

static void alt1250_mcu_dpll_add_domain(ulong div_rcclk) {
  /* MCU clock should be switched in glitch free manner to boot clock */
  CLEAR_BIT(REGISTER(MCU_CLK_CONTROL_CTRL),
            MCU_CLK_CONTROL_CTRL_GF_RST_POS); /* refclk/pllclk gf rst (rst=0) */
  CLEAR_BIT(REGISTER(MCU_CLK_CONTROL_CTRL),
            MCU_CLK_CONTROL_CTRL_GF_SEL_POS); /* SW control ref clock (not PLL clock) */
  SET_BIT(REGISTER(MCU_CLK_CONTROL_CTRL),
          MCU_CLK_CONTROL_CTRL_GF_RST_POS); /* refclk/pllclk gf rst (rst=1) */

  if (CONFIG_ALT1250_MCU_SEL1_CLKSRC == 1)
    SET_BIT(REGISTER(MCU_CLK_CONTROL_CTRL),
            MCU_CLK_CONTROL_CTRL_SEL1_POS); /* pll or 4.19 (per sel0) */
  else
    CLEAR_BIT(REGISTER(MCU_CLK_CONTROL_CTRL), MCU_CLK_CONTROL_CTRL_SEL1_POS); /* rcclk divided */

  if (CONFIG_ALT1250_MCU_SEL0_CLKSRC == 1)
    SET_BIT(REGISTER(MCU_CLK_CONTROL_CTRL), MCU_CLK_CONTROL_CTRL_SEL0_POS); /* 4.19 */
  else
    CLEAR_BIT(REGISTER(MCU_CLK_CONTROL_CTRL), MCU_CLK_CONTROL_CTRL_SEL0_POS); /* mcu pll_clk */

  if (CONFIG_ALT1250_MCU_SEL_PLL_CLKSRC == 1) {
    SET_BIT(REGISTER(MCU_CLK_CONTROL_CTRL),
            MCU_CLK_CONTROL_CTRL_SEL_PLL_MCU_REFCLK_POS); /* rcclk */
    /* Set divide value to rcclk */
    REGISTER(MCU_CLK_CONTROL_CTRL) &= ~MCU_CLK_CONTROL_CTRL_DIV_MSK;
    REGISTER(MCU_CLK_CONTROL_CTRL) |= (div_rcclk << MCU_CLK_CONTROL_CTRL_DIV_POS);
  } else
    CLEAR_BIT(REGISTER(MCU_CLK_CONTROL_CTRL),
              MCU_CLK_CONTROL_CTRL_SEL_PLL_MCU_REFCLK_POS); /* refclk */

  if (CONFIG_ALT1250_MCU_GF_CLKSRC == 1) {
    /* Switch in glitch free manner from boot clock to fast clock (MCU clock source) */
    CLEAR_BIT(REGISTER(MCU_CLK_CONTROL_CTRL),
              MCU_CLK_CONTROL_CTRL_GF_RST_POS); /* refclk/pllclk gf rst (rst=0) */
    SET_BIT(REGISTER(MCU_CLK_CONTROL_CTRL),
            MCU_CLK_CONTROL_CTRL_GF_SEL_POS); /* SW control PLL clock (not ref clock) */
  }

  CLEAR_BIT(REGISTER(MCU_CLK_CONTROL_CTRL), MCU_CLK_CONTROL_CTRL_MUX_UART0_EXT_CLK_SEL_POS);
 #ifdef ALT1250
  CLEAR_BIT(REGISTER(MCU_CLK_CONTROL_CTRL), MCU_CLK_CONTROL_CTRL_MUX_UART1_EXT_CLK_SEL_POS);
 #endif
}

void pll_init(ulong vco_freq, ulong clk_freq) {
  if (TEST_NEG_BIT(REGISTER(MCU_CLK_CONTROL_CTRL),
                   MCU_CLK_CONTROL_CTRL_GF_RST_POS)) {  // Move back to bootclk
    CLEAR_BIT(REGISTER(MCU_CLK_CONTROL_CTRL),
              MCU_CLK_CONTROL_CTRL_GF_SEL_POS); /* SW control PLL clock (not ref clock) */
    SET_BIT(REGISTER(MCU_CLK_CONTROL_CTRL),
            MCU_CLK_CONTROL_CTRL_GF_RST_POS); /* refclk/pllclk gf rst (rst=1) */
  }

  alt1250_mcu_dpll_config(vco_freq, clk_freq);
  alt1250_mcu_dpll_add_domain(0); /* Add MCU domain */
}