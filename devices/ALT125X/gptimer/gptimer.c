/*  ---------------------------------------------------------------------------

        (c) copyright 2018 Altair Semiconductor, Ltd. All rights reserved.

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

#include "FreeRTOS.h"
#include <stdio.h>
#include "portmacro.h"
#include "125X_mcu.h"
#include "mcu_exported.h"
#include "gptimer.h"
#include "clk_gating.h"

static int g_gptimer_irq_initialized = 0;

static struct alt1250_gptimer_cfg_sel {
  uint16_t is_config;
  uint32_t irq_enabled;
  gptimer_interrupt_handler irq_handler;
  int user_param;
} gptimer_cfg_sel[] = {{0, 0, NULL, 0}, {0, 0, NULL, 0}};

static void gptimer_irq_interrupt_handler(int gptimer_ins);

// check gptimer_int number
#define GPTIMER_CHECK_INT_NUMBER(p) \
  if (((p) != 0) && ((p) != 1)) {   \
    return -1;                      \
  }

/*----------------------------------------------------------------------------
 * GP_Timer_0_IRQHandler, GP_Timer_1_IRQHandler
 *----------------------------------------------------------------------------*/
void GP_Timer_0_IRQHandler(void) { gptimer_irq_interrupt_handler(0); }

void GP_Timer_1_IRQHandler(void) { gptimer_irq_interrupt_handler(1); }

static void gptimer_irq_interrupt_handler(int gptimer_ins) {
  if ((gptimer_ins == 0) || (gptimer_ins == 1)) {
    if (gptimer_cfg_sel[gptimer_ins].irq_enabled && gptimer_cfg_sel[gptimer_ins].irq_handler) {
      gptimer_cfg_sel[gptimer_ins].irq_handler(gptimer_cfg_sel[gptimer_ins].user_param);
    }
  } else {
    printf("Error - Ileagal GPTIMER interrupt\n\r");
  }
}

/****************************************************************************/
/* Function name: gptimer_init                                              */
/****************************************************************************/
/* Description: Initialize GP timer interrupt registrations DB if required  */
/*              This function must be called before activating GP timer.    */
/*              There is no effect in calling this function more than once. */
/* Parameters:  None                                                        */
/* Return value: None                                                       */
/****************************************************************************/
void gptimer_init(void) {
  if (g_gptimer_irq_initialized) return;

  size_t i;

  // enable clock for timer
  clk_gating_source_enable(CLK_GATING_TIMERS);

  // Enable NCO (no rate change)
  REGISTER(MCU_NCO_MCU_NCO_CFG) |= MCU_NCO_MCU_NCO_CFG_MCU_NCO_ENABLE_MSK;

  // Disable GP Timer
  REGISTER(MCU_GP_TIMER_CTRL_GP_TIMER_CTRL) = 0;

  // disable all interrupts
  REGISTER(MCU_GP_TIMER_INTR_0_GP_TIMER_INTR_EN) = 0;
  REGISTER(MCU_GP_TIMER_INTR_1_GP_TIMER_INTR_EN) = 0;

  // clear database
  for (i = 0; i < (sizeof(gptimer_cfg_sel) / sizeof(gptimer_cfg_sel[0])); i++) {
    gptimer_cfg_sel[i].irq_enabled = 0;
    gptimer_cfg_sel[i].irq_handler = NULL;
    gptimer_cfg_sel[i].user_param = 0;
  }

  // Enable interrupts
  for (i = GP_Timer_0_IRQn; i <= GP_Timer_1_IRQn; i++) {
    NVIC_SetPriority((IRQn_Type)i, 7); /* set Interrupt priority */
    NVIC_EnableIRQ((IRQn_Type)i);
  }

  g_gptimer_irq_initialized = 1;
}

/****************************************************************************/
/* Function name: gptimer_register_interrupt                                */
/****************************************************************************/
/* Description: Register an interrupt handler for GP timer interrupt        */
/*              When the GP timer interrupt occurs, the handler is called   */
/*              from interrupt context                                      */
/* Parameters:  gptimer     - interrupt number, 0 or 1                      */
/*              irq_handler - interrupt handler function                    */
/*              user_param  - parameter transfered to the interrupt handler */
/*                            function                                      */
/* Return value: 0                                                          */
/****************************************************************************/
int gptimer_register_interrupt(int gptimer, gptimer_interrupt_handler irq_handler, int user_param) {
  gptimer_init();

  gptimer_cfg_sel[gptimer].irq_enabled = 0;
  gptimer_cfg_sel[gptimer].irq_handler = irq_handler;
  gptimer_cfg_sel[gptimer].user_param = user_param;

  return 0;
}

/****************************************************************************/
/* Function name: gptimer_get_value                                         */
/****************************************************************************/
/* Description: Read GP timer current value (counter)                       */
/* Parameters:  None                                                        */
/* Return value: GP timer value                                             */
/****************************************************************************/
unsigned int gptimer_get_value(void) { return REGISTER(MCU_GP_TIMER_CTRL_GP_TIMER_VALUE); }

/****************************************************************************/
/* Function name: gptimer_add_offset                                        */
/****************************************************************************/
/* Description: Add offset to GP timer counter                              */
/* Parameters:  offset      - offset to be added                            */
/* Return value: None                                                       */
/****************************************************************************/
void gptimer_add_offset(unsigned int offset) {
  REGISTER(MCU_GP_TIMER_CTRL_GP_TIMER_UPDT_OFFSET) = offset;
}

/****************************************************************************/
/* Function name: gptimer_enable                                            */
/****************************************************************************/
/* Description: Enable the GP timer HW.                                     */
/* Parameters:  None                                                        */
/* Return value: None                                                       */
/****************************************************************************/
void gptimer_enable(void) {
  // enable clock for timer
  REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) |= MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_TIMERS_MSK;

  REGISTER(MCU_GP_TIMER_CTRL_GP_TIMER_CTRL) =
      MCU_GP_TIMER_CTRL_GP_TIMER_CTRL_GP_TIMER_EN_MSK;  // Enable timer
}

/****************************************************************************/
/* Function name: gptimer_disable                                           */
/****************************************************************************/
/* Description: Disable the GP timer HW.                                    */
/* Parameters:  None                                                        */
/* Return value: None                                                       */
/****************************************************************************/
void gptimer_disable(void) {
  REGISTER(MCU_GP_TIMER_CTRL_GP_TIMER_CTRL) = 0;

  REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) &= ~MCU_CLK_CONTROL_MCU_CLK_REQ_EN_MCU_TIMERS_MSK;
}

/****************************************************************************/
/* Function name: gptimer_activate                                          */
/****************************************************************************/
/* Description: Activate GP timer and initialize interrupt registrations DB */
/*              if required, enable GP timer.                               */
/* Parameters:  value - Initial value for GP timer                          */
/* Return value: None                                                       */
/****************************************************************************/
void gptimer_activate(unsigned int value) {
  gptimer_init();
  REGISTER(MCU_GP_TIMER_CTRL_GP_TIMER_PRESET_VALUE) = value;  // Preset value
  gptimer_enable();
}

/****************************************************************************/
/* Function name: gptimer_set_frequency                                     */
/****************************************************************************/
/* Description: Set GP timer frequency                                      */
/* Parameters:  frequency   - Requested frequency in Hz                     */
/* Return value: Actual frequency set, or -1 if fail                        */
/****************************************************************************/
int gptimer_set_frequency(int frequency) {
  // target_rate = nco_clock * (nco_rate / wrap_value)
  int nco_clock = SystemCoreClock;
  int nco_rate, wrap_value;
  int delta = 0, curr_delta, idiv_delta, roundup, curr_roundup;
  int max_wrap_value = 0x10000;

  nco_rate = -1;
  for (wrap_value = 1; wrap_value <= max_wrap_value; wrap_value++) {
    if ((curr_delta = (((long long)frequency * wrap_value) % nco_clock)) == 0) {
      nco_rate = (((long long)frequency * wrap_value) / nco_clock);
      delta = 0; /* Ignore the fractional calculation */
      break;
    }

    /* Test for the minimum delta */
    if ((nco_clock - curr_delta) < curr_delta) {
      curr_delta = nco_clock - curr_delta;
      /* Mult value should be rounded up */
      curr_roundup = 1;
    } else {
      curr_roundup = 0;
    }

    /* Use fractional calculation for Mult & Div */
    if (curr_delta < delta || delta == 0) {
      delta = curr_delta;
      idiv_delta = wrap_value;
      roundup = curr_roundup;
    }
  }

  if (delta) {
    nco_rate = (((long long)frequency * idiv_delta) / nco_clock) + roundup;
    wrap_value = idiv_delta;
  }

  //  printf ("nco_rate = %d, wrap_value = %d\n\r",nco_rate, wrap_value);

  if ((wrap_value > 0) && (wrap_value <= max_wrap_value) && (nco_rate > 0) &&
      (nco_rate <= MCU_NCO_MCU_NCO_CFG_MCU_NCO_RATE_MSK)) {
    REGISTER(MCU_NCO_MCU_NCO_WRAP_VALUE) = wrap_value;
    REGISTER(MCU_NCO_MCU_NCO_CFG) =
        nco_rate | (REGISTER(MCU_NCO_MCU_NCO_CFG) & MCU_NCO_MCU_NCO_CFG_MCU_NCO_ENABLE_MSK);
    return (long long)nco_clock * nco_rate / wrap_value;
  } else {
    return -1;
  }
}

/****************************************************************************/
/* Function name: gptimer_set_interrupt_offset                              */
/****************************************************************************/
/* Description: Set GP timer interrupt with offset from current count       */
/* Parameters:  gptimer  - interrupt number, 0 or 1                         */
/*              offset   - At which interrupt will occur (16 bits)          */
/* Return value: Pass (0) or Fail (-1)                                      */
/****************************************************************************/
int gptimer_set_interrupt_offset(int gptimer, unsigned int offset) {
  GPTIMER_CHECK_INT_NUMBER(gptimer);

  if (offset > 0xffff) {
    return -1;
  }

  gptimer_cfg_sel[gptimer].irq_enabled = 1;

  // Set target interrupt offset target
  REGISTER(gptimer == 0 ? MCU_GP_TIMER_INTR_0_GP_TIMER_INTR_SET_OFFSET
                        : MCU_GP_TIMER_INTR_1_GP_TIMER_INTR_SET_OFFSET) = offset;

  // Enable Interrupt
  REGISTER(gptimer == 0 ? MCU_GP_TIMER_INTR_0_GP_TIMER_INTR_EN
                        : MCU_GP_TIMER_INTR_1_GP_TIMER_INTR_EN) =
      MCU_GP_TIMER_INTR_0_GP_TIMER_INTR_EN_GP_TIMER_INTR_EN_MSK;

  return 0;
}

/****************************************************************************/
/* Function name: gptimer_set_interrupt_value                               */
/****************************************************************************/
/* Description: Set GP timer interrupt at absolute value                    */
/* Parameters:  gptimer  - interrupt number, 0 or 1                         */
/*              value    - At which interrupt will occur (16 bits)          */
/* Return value: Pass (0) or Fail (-1)                                      */
/****************************************************************************/
int gptimer_set_interrupt_value(int gptimer, unsigned int value) {
  GPTIMER_CHECK_INT_NUMBER(gptimer);

  gptimer_cfg_sel[gptimer].irq_enabled = 1;

  // Set target interrupt offset target
  REGISTER(gptimer == 0 ? MCU_GP_TIMER_INTR_0_GP_TIMER_INTR_SET_VALUE
                        : MCU_GP_TIMER_INTR_1_GP_TIMER_INTR_SET_VALUE) = value;

  // Enable Interrupt
  REGISTER(gptimer == 0 ? MCU_GP_TIMER_INTR_0_GP_TIMER_INTR_EN
                        : MCU_GP_TIMER_INTR_1_GP_TIMER_INTR_EN) =
      MCU_GP_TIMER_INTR_0_GP_TIMER_INTR_EN_GP_TIMER_INTR_EN_MSK;

  return 0;
}

/****************************************************************************/
/* Function name: gptimer_disable_interrupt                                 */
/****************************************************************************/
/* Description: Disable active interrupt of GP timer                        */
/* Parameters:  gptimer  - interrupt number, 0 or 1                         */
/* Return value: Pass (0) or Faile (-1)                                     */
/****************************************************************************/
int gptimer_disable_interrupt(int gptimer) {
  GPTIMER_CHECK_INT_NUMBER(gptimer);

  gptimer_cfg_sel[gptimer].irq_enabled = 0;

  // Disable Interrupt
  REGISTER(gptimer == 0 ? MCU_GP_TIMER_INTR_0_GP_TIMER_INTR_EN
                        : MCU_GP_TIMER_INTR_1_GP_TIMER_INTR_EN) = 0;

  return 0;
}

/****************************************************************************/
/* Function name: gptimer_read_target_value                                 */
/****************************************************************************/
/* Description: Read GP timer interrupt target                              */
/* Parameters:  gptimer  - interrupt number, 0 or 1                         */
/* Return value: Target                                                     */
/****************************************************************************/
unsigned int gptimer_read_target_value(int gptimer) {
  GPTIMER_CHECK_INT_NUMBER(gptimer);

  return REGISTER(gptimer == 0 ? MCU_GP_TIMER_INTR_0_GP_TIMER_INTR_TARGET
                               : MCU_GP_TIMER_INTR_1_GP_TIMER_INTR_TARGET);
}

/****************************************************************************/
/* Function name: gptimer_read_current_value                                */
/****************************************************************************/
/* Description: Read GP timer interrupt counter                             */
/* Parameters:  gptimer  - interrupt number, 0 or 1                         */
/* Return value: Counter                                                    */
/****************************************************************************/
unsigned int gptimer_read_current_value(int gptimer) {
  GPTIMER_CHECK_INT_NUMBER(gptimer);

  return REGISTER(gptimer == 0 ? MCU_GP_TIMER_INTR_0_GP_TIMER_INTR_VALUE
                               : MCU_GP_TIMER_INTR_1_GP_TIMER_INTR_VALUE);
}
