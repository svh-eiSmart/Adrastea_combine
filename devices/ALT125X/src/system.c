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
#include <string.h>
#include <FreeRTOS.h>
#include "task.h"
#include "125X_mcu.h"
#include "clk_gating.h"
#include "mcu2pmp_msg.h"
#include "pmp_agent.h"
#include "atomic_counter.h"
#include "serial_container.h"
#include "portmacro.h"

#ifndef WEAK
#if defined(__ICCARM__)
#define WEAK __weak
#else
#define WEAK __attribute__((weak))
#endif
#endif

#ifndef UNCACHED
#if defined(__ICCARM__)
#define UNCACHED _Pragma("location=\"UNCACHED\"")
#else
#define UNCACHED __attribute__((section(".uncached")))
#endif
#endif

/*----------------------------------------------------------------------------
  Externals
 *----------------------------------------------------------------------------*/
#if defined(__VTOR_PRESENT) && (__VTOR_PRESENT == 1U)
extern uint32_t __Vectors;
#endif

/*----------------------------------------------------------------------------
  System Core Clock Variable
 *----------------------------------------------------------------------------*/

volatile uint32_t Ticks; /* counts timeTicks */
volatile uint32_t GP_Timer_cnt;
#if 0
/*----------------------------------------------------------------------------
 * SysTick_Handler:
 *----------------------------------------------------------------------------*/
void SysTick_Handler(void) {
  Ticks++;
}
#endif

void Activate_GP_Timer_0(void) {
  GP_Timer_cnt = 0;

  // Enable NCO (no rate change)
  REGISTER(MCU_NCO_MCU_NCO_CFG) =
      (1 << MCU_NCO_MCU_NCO_CFG_MCU_NCO_RATE_POS) | MCU_NCO_MCU_NCO_CFG_MCU_NCO_ENABLE_MSK;

  // Enable GP Timer
  REGISTER(MCU_GP_TIMER_CTRL_GP_TIMER_CTRL) = MCU_GP_TIMER_CTRL_GP_TIMER_CTRL_GP_TIMER_EN_MSK;

  // Set target interrupt offset target
  REGISTER(MCU_GP_TIMER_INTR_0_GP_TIMER_INTR_SET_OFFSET) = 0x20;

  // Enable Interrupt
  REGISTER(MCU_GP_TIMER_INTR_0_GP_TIMER_INTR_EN) =
      MCU_GP_TIMER_INTR_0_GP_TIMER_INTR_EN_GP_TIMER_INTR_EN_MSK;

  NVIC_SetPriority(GP_Timer_0_IRQn, 0); /* set Priority for GP_Timer_0 Interrupt */
  NVIC_EnableIRQ(GP_Timer_0_IRQn);
}

void machine_reset(eMachineResetCause reset_cause) {
  mcu2pmp_mbox_msg_t msg;

  memset(&msg, 0x0, sizeof(mcu2pmp_mbox_msg_t));
  msg.header.msgType = MCU2PMP_MSG_RESET_REQUEST;
  switch (reset_cause) {
    case MACHINE_RESET_CAUSE_USER:
      msg.body.reset_request.reset_cause = MCU_RESET_CAUSE_USER_TRIGGER;
      break;
    case MACHINE_RESET_CAUSE_EXCEPTION:
      msg.body.reset_request.reset_cause = MCU_RESET_CAUSE_SW_FAILURE_EXCEPTION;
      break;
    case MACHINE_RESET_CAUSE_ASSERT:
      msg.body.reset_request.reset_cause = MCU_RESET_CAUSE_SW_FAILURE_ASSERT;
      break;
    default:
      msg.body.reset_request.reset_cause = MCU_RESET_CAUSE_UNKNOWN;
      break;
  }
  /*Make sure pmp message will be sent atomically*/
  if (xPortIsInsideInterrupt() != pdTRUE) taskENTER_CRITICAL();
  send_msg_to_pmp(&msg);
  while (1)
    ;
}

/*****************************************************************************
 *  Function: delayus
 *
 *  Parameters: freq  - CPU clock frequency in Hz
 *              d     - delay time in us
 *
 *  Returns:    none
 *
 *  Description: busy wait for approximate delay us.
 *
 *****************************************************************************/
void delayus(uint32_t freq, int32_t d) {
  uint32_t p;

  p = (d >> 1) * (freq >> 19);
  while (p--)
    ;
}

/*****************************************************************************
 *  Function: msleep
 *
 *  Parameters: milliseconds - Number of milliseconds to sleep
 *
 *  Returns:    none
 *
 *  Description: Sleep (not busy wait) for a number of milli seconds
 *
 *****************************************************************************/
void msleep(uint32_t milliseconds) {
  unsigned long int delay;
  delay = milliseconds / portTICK_PERIOD_MS;
  if (delay == 0) {
    delay = 1;
  }
  vTaskDelay(delay);
}

uint32_t SystemCoreClock;

extern void sysclk_init();

/*----------------------------------------------------------------------------
  System initialization function
 *----------------------------------------------------------------------------*/
UNCACHED void SystemInit(void) {
  // Invalidate cache lines
  REGISTER(MCU_REGFILE_ICACHE_CTRL) = MCU_REGFILE_ICACHE_CTRL_ICACHE_INVALIDATE_MSK;
  REGISTER(MCU_REGFILE_ICACHE_CTRL) = 0;
#if defined(__VTOR_PRESENT) && (__VTOR_PRESENT == 1U)
  SCB->VTOR = (uint32_t)&__Vectors;
#endif
}

/*-----------------------------------------------------------*/

WEAK void platform_init_hook(void) {}

static void init() {
  sysclk_init();

  clk_gating_init();

  /* Ensure all priority bits are assigned as preemption priority bits. */
  NVIC_SetPriorityGrouping(0);

  SysTick_Config(SystemCoreClock / 1000);

  serial_init();

  atomic_counter_init();

  pmpAgent_init();
  platform_init_hook();
}

#ifdef __GNUC__
int __real_main(void);
int __wrap_main(void) {
  init();
  return __real_main();
}
#endif

#ifdef __ICCARM__
int $Super$$main(void);
int $Sub$$main(void) {
  init();
  return $Super$$main();
}
#endif
