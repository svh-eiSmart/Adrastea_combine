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
#include "shadow32ktimer.h"
#include "clk_gating.h"

static int g_shadow32ktimer_irq_initialized = 0;

static struct alt1250_shadow32ktimer_cfg_sel {
  uint16_t is_config;
  uint32_t irq_enabled;
  shadow32ktimer_interrupt_handler irq_handler;
  int user_param;
} shadow32ktimer_cfg_sel = {0, 0, NULL, 0};

void shadow32ktimer_IRQhandler(void) {
  if (shadow32ktimer_cfg_sel.irq_enabled && shadow32ktimer_cfg_sel.irq_handler) {
    shadow32ktimer_cfg_sel.irq_handler(shadow32ktimer_cfg_sel.user_param);
  }
}

/****************************************************************************/
/* Function name: shadow32ktimer_init                                        */
/****************************************************************************/
/* Description: Initialize timer interrupt registrations DB if required     */
/*              This function must be called before activating timer.       */
/*              There is no effect in calling this function more than once. */
/* Parameters:  None                                                        */
/* Return value: None                                                       */
/****************************************************************************/
void shadow32ktimer_init(void) {
  if (g_shadow32ktimer_irq_initialized) return;

  // enable clock gating
  clk_gating_source_enable(CLK_GATING_SHADOW_32KHZ);

  // Disable shadow 32K Timer
  REGISTER(MCU_SHADOW_32K_TIMER_CTRL_SHADOW_32K_TIMER_CTRL) = 0;

  // disable all interrupt
  REGISTER(MCU_SHADOW_32K_TIMER_INTR_SHADOW_32K_TIMER_INTR_EN) = 0;

  // clear database
  shadow32ktimer_cfg_sel.irq_enabled = 0;
  shadow32ktimer_cfg_sel.irq_handler = NULL;
  shadow32ktimer_cfg_sel.user_param = 0;

  // Enable interrupt
  NVIC_SetPriority((IRQn_Type)Shadow32K_Timer_IRQn, 7); /* set Interrupt priority */
  NVIC_EnableIRQ((IRQn_Type)Shadow32K_Timer_IRQn);

  g_shadow32ktimer_irq_initialized = 1;
}

/*****************************************************************************/
/* Function name: shadow32ktimer_register_interrupt                          */
/*****************************************************************************/
/* Description: Register an interrupt handler for shadow 32k timer interrupt */
/*              When the shadow 32k timer interrupt occurs, the handler is   */
/*              called from interrupt context                                */
/* Parameters:  irq_handler - interrupt handler function                     */
/*              user_param  - parameter transfered to the interrupt handler  */
/*                            function                                       */
/* Return value: 0                                                           */
/*****************************************************************************/
int shadow32ktimer_register_interrupt(shadow32ktimer_interrupt_handler irq_handler,
                                      int user_param) {
  shadow32ktimer_init();

  shadow32ktimer_cfg_sel.irq_enabled = 0;
  shadow32ktimer_cfg_sel.irq_handler = irq_handler;
  shadow32ktimer_cfg_sel.user_param = user_param;

  return 0;
}

/****************************************************************************/
/* Function name: shadow32ktimer_get_value                                  */
/****************************************************************************/
/* Description: Read shadow 32k timer current value (counter)               */
/* Parameters:  None                                                        */
/* Return value: shadow 32k timer value                                     */
/****************************************************************************/
unsigned int shadow32ktimer_get_value(void) {
  return REGISTER(MCU_SHADOW_32K_TIMER_CTRL_SHADOW_32K_TIMER_VALUE);
}

/****************************************************************************/
/* Function name: shadow32ktimer_add_offset                                 */
/****************************************************************************/
/* Description: Add offset to shadow 32k timer counter                      */
/* Parameters:  offset      - offset to be added                            */
/* Return value: None                                                       */
/****************************************************************************/
void shadow32ktimer_add_offset(unsigned int offset) {
  REGISTER(MCU_SHADOW_32K_TIMER_CTRL_SHADOW_32K_TIMER_UPDT_OFFSET) = offset;
}

/****************************************************************************/
/* Function name: shadow32ktimer_enable                                     */
/****************************************************************************/
/* Description: Enable the shadow 32k timer HW.                             */
/* Parameters:  None                                                        */
/* Return value: None                                                       */
/****************************************************************************/
void shadow32ktimer_enable(void) {
  REGISTER(MCU_SHADOW_32K_TIMER_CTRL_SHADOW_32K_TIMER_CTRL) =
      MCU_SHADOW_32K_TIMER_CTRL_SHADOW_32K_TIMER_CTRL_SHADOW_32K_TIMER_EN_MSK;  // Enable timer
}

/****************************************************************************/
/* Function name: shadow32ktimer_disable                                    */
/****************************************************************************/
/* Description: Disable the shadow 32k timer HW.                            */
/* Parameters:  None                                                        */
/* Return value: None                                                       */
/****************************************************************************/
void shadow32ktimer_disable(void) { REGISTER(MCU_SHADOW_32K_TIMER_CTRL_SHADOW_32K_TIMER_CTRL) = 0; }

/****************************************************************************/
/* Function name: shadow32ktimer_activate                                   */
/****************************************************************************/
/* Description: Activate shadow 32k timer and initialize interrupt          */
/*               registrations DB if required, enable timer.                */
/* Parameters:  value - Initial value for timer                             */
/* Return value: None                                                       */
/****************************************************************************/
void shadow32ktimer_activate(unsigned int value) {
  shadow32ktimer_init();
  REGISTER(MCU_SHADOW_32K_TIMER_CTRL_SHADOW_32K_TIMER_PRESET_VALUE) = value;  // Preset value
  shadow32ktimer_enable();
}

/******************************************************************************/
/* Function name: shadow32ktimer_set_interrupt_offset                         */
/******************************************************************************/
/* Description: Set shadow 32k timer interrupt with offset from current count */
/* Parameters:  offset   - At which interrupt will occur (16 bits)            */
/* Return value: Pass (0) or Fail (-1)                                        */
/******************************************************************************/
int shadow32ktimer_set_interrupt_offset(unsigned int offset) {
  if (offset > 0xffff) {
    return -1;
  }

  shadow32ktimer_cfg_sel.irq_enabled = 1;

  // Set target interrupt offset target
  REGISTER(MCU_SHADOW_32K_TIMER_INTR_SHADOW_32K_TIMER_INTR_SET_OFFSET) = offset;

  // Enable Interrupt
  REGISTER(MCU_SHADOW_32K_TIMER_INTR_SHADOW_32K_TIMER_INTR_EN) =
      MCU_SHADOW_32K_TIMER_INTR_SHADOW_32K_TIMER_INTR_EN_SHADOW_32K_TIMER_INTR_EN_MSK;

  return 0;
}

/****************************************************************************/
/* Function name: shadow32ktimer_set_interrupt_value                        */
/****************************************************************************/
/* Description: Set shadow 32k timer interrupt at absolute value            */
/* Parameters:  value    - At which interrupt will occur (16 bits)          */
/* Return value: Pass (0)                                                   */
/****************************************************************************/
int shadow32ktimer_set_interrupt_value(unsigned int value) {
  shadow32ktimer_cfg_sel.irq_enabled = 1;

  // Set target interrupt offset target
  REGISTER(MCU_SHADOW_32K_TIMER_INTR_SHADOW_32K_TIMER_INTR_SET_VALUE) = value;

  // Enable Interrupt
  REGISTER(MCU_SHADOW_32K_TIMER_INTR_SHADOW_32K_TIMER_INTR_EN) =
      MCU_SHADOW_32K_TIMER_INTR_SHADOW_32K_TIMER_INTR_EN_SHADOW_32K_TIMER_INTR_EN_MSK;

  return 0;
}

/****************************************************************************/
/* Function name: shadow32ktimer_disable_interrupt                          */
/****************************************************************************/
/* Description: Disable active interrupt of shadow 32k timer                */
/* Parameters:  None                                                        */
/* Return value: Pass (0)                                                   */
/****************************************************************************/
int shadow32ktimer_disable_interrupt(void) {
  shadow32ktimer_cfg_sel.irq_enabled = 0;

  // Disable Interrupt
  REGISTER(MCU_SHADOW_32K_TIMER_INTR_SHADOW_32K_TIMER_INTR_EN) = 0;

  return 0;
}

/****************************************************************************/
/* Function name: shadow32ktimer_read_target_value                          */
/****************************************************************************/
/* Description: Read timer interrupt target                                 */
/* Parameters:  None                                                        */
/* Return value: Target                                                     */
/****************************************************************************/
unsigned int shadow32ktimer_read_target_value(void) {
  return REGISTER(MCU_SHADOW_32K_TIMER_INTR_SHADOW_32K_TIMER_INTR_TARGET);
}

/****************************************************************************/
/* Function name: shadow32ktimer_read_current_value                         */
/****************************************************************************/
/* Description: Read GP timer interrupt counter                             */
/* Parameters:  None                                                        */
/* Return value: Counter                                                    */
/****************************************************************************/
unsigned int shadow32ktimer_read_current_value(void) {
  return REGISTER(MCU_SHADOW_32K_TIMER_INTR_SHADOW_32K_TIMER_INTR_VALUE);
}