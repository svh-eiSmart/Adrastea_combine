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
#include <stdio.h>

#include DEVICE_HEADER

#include "DRV_32KTIMER.h"
#include "DRV_CLOCK_GATING.h"

#if (configUSE_ALT_SLEEP == 1)
#include "hibernate.h"
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/
#if (configUSE_ALT_SLEEP == 1)
static UNCOMPRESSED struct drv_32ktimer_peripheral {
  SHADOW_32K_TIMER_INTR_Type *intr_reg;
  SHADOW_32K_TIMER_CTRL_Type *ctrl_reg;
  uint16_t is_config;
  uint32_t irq_enabled;
  DRV_32KTIMER_InterruptHandler irq_handler;
  int32_t user_param;
} DRV_32KTIMER_Peripheral;
#else
static struct drv_32ktimer_peripheral {
  SHADOW_32K_TIMER_INTR_Type *intr_reg;
  SHADOW_32K_TIMER_CTRL_Type *ctrl_reg;
  uint16_t is_config;
  uint32_t irq_enabled;
  DRV_32KTIMER_InterruptHandler irq_handler;
  int32_t user_param;
} DRV_32KTIMER_Peripheral = {SHADOW_32K_TIMER_INTR, SHADOW_32K_TIMER_CTRL, 0, 0, NULL, 0};
#endif
/****************************************************************************
 * Private Data
 ****************************************************************************/
#if (configUSE_ALT_SLEEP == 1)
static UNCOMPRESSED int shadow32ktimer_irq_initialized = 0;
#else
static int shadow32ktimer_irq_initialized = 0;
#endif
/****************************************************************************
 * Functions
 ****************************************************************************/
void shadow32ktimer_IRQhandler(void) {
  if (DRV_32KTIMER_Peripheral.irq_enabled && DRV_32KTIMER_Peripheral.irq_handler) {
    DRV_32KTIMER_Peripheral.irq_handler(DRV_32KTIMER_Peripheral.user_param);
  }
}

void DRV_32KTIMER_Initialize(void) {
  if (shadow32ktimer_irq_initialized) return;

#if (configUSE_ALT_SLEEP == 1)
  // Initialize here as uncompressed section is always treated as bss, not data
  DRV_32KTIMER_Peripheral =
      (struct drv_32ktimer_peripheral){SHADOW_32K_TIMER_INTR, SHADOW_32K_TIMER_CTRL, 0, 0, NULL, 0};
#endif
  // enable clock gating
  DRV_CLKGATING_EnableClkSource(CLK_GATING_SHADOW_32KHZ);

  // Disable shadow 32K Timer
  DRV_32KTIMER_Peripheral.ctrl_reg->CTRL = 0;

  // disable all interrupt
  DRV_32KTIMER_Peripheral.intr_reg->EN = 0;

  // clear database
  DRV_32KTIMER_Peripheral.irq_enabled = 0;
  DRV_32KTIMER_Peripheral.irq_handler = NULL;
  DRV_32KTIMER_Peripheral.user_param = 0;

  // Enable interrupt
  NVIC_SetPriority((IRQn_Type)Shadow_32K_Timer_IRQn, 7); /* set Interrupt priority */
  NVIC_EnableIRQ((IRQn_Type)Shadow_32K_Timer_IRQn);

  shadow32ktimer_irq_initialized = 1;
}

int32_t DRV_32KTIMER_RegisterInterrupt(DRV_32KTIMER_InterruptHandler irq_handler,
                                       int32_t user_param) {
  DRV_32KTIMER_Initialize();
  DRV_32KTIMER_Peripheral.irq_enabled = 0;
  DRV_32KTIMER_Peripheral.irq_handler = irq_handler;
  DRV_32KTIMER_Peripheral.user_param = user_param;

  return 0;
}

uint32_t DRV_32KTIMER_GetValue(void) { return DRV_32KTIMER_Peripheral.ctrl_reg->VALUE; }

void DRV_32KTIMER_AddOffset(uint32_t offset) {
  DRV_32KTIMER_Peripheral.ctrl_reg->UPDT_OFFSET = offset;
}

void DRV_32KTIMER_Enable(void) {
  DRV_32KTIMER_Peripheral.ctrl_reg->CTRL = SHADOW_32K_TIMER_CTRL_CTRL_EN_Msk;
}

void DRV_32KTIMER_Disable(void) { DRV_32KTIMER_Peripheral.ctrl_reg->CTRL = 0; }

void DRV_32KTIMER_Activate(uint32_t value) {
  DRV_32KTIMER_Initialize();
  DRV_32KTIMER_Peripheral.ctrl_reg->PRESET_VALUE = value;  // Preset value
  DRV_32KTIMER_Enable();
}

int32_t DRV_32KTIMER_SetInterruptOffset(uint32_t offset) {
  if (offset > 0xffff) {
    return -1;
  }

  DRV_32KTIMER_Peripheral.irq_enabled = 1;

  // Set target interrupt offset target
  DRV_32KTIMER_Peripheral.intr_reg->SET_OFFSET = offset;

  // Enable Interrupt
  DRV_32KTIMER_Peripheral.intr_reg->EN = SHADOW_32K_TIMER_INTR_EN_EN_Msk;

  return 0;
}

int32_t DRV_32KTIMER_SetInterruptValue(uint32_t value) {
  DRV_32KTIMER_Peripheral.irq_enabled = 1;

  // Set target interrupt offset target
  DRV_32KTIMER_Peripheral.intr_reg->SET_VALUE = value;

  // Enable Interrupt
  DRV_32KTIMER_Peripheral.intr_reg->EN = SHADOW_32K_TIMER_INTR_EN_EN_Msk;

  return 0;
}

int32_t DRV_32KTIMER_DisableInterrupt(void) {
  DRV_32KTIMER_Peripheral.irq_enabled = 0;

  // Disable Interrupt
  DRV_32KTIMER_Peripheral.intr_reg->EN = 0;

  return 0;
}

uint32_t DRV_32KTIMER_ReadTargetValue(void) { return DRV_32KTIMER_Peripheral.intr_reg->TARGET; }

uint32_t DRV_32KTIMER_ReadCurrentValue(void) { return DRV_32KTIMER_Peripheral.intr_reg->VALUE; }
