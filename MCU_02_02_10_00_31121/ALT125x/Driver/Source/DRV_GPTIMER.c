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

/* Platform Headers */
#include DEVICE_HEADER

#include "DRV_GPTIMER.h"
#include "DRV_CLOCK_GATING.h"

#if (configUSE_ALT_SLEEP == 1)
#include "pwr_mngr.h"
#include "DRV_PM.h"
#include "DRV_SLEEP.h"
#endif
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
// check gptimer_int number
#define GPTIMER_CHECK_INT_NUMBER(p) \
  if (((p) != 0) && ((p) != 1)) {   \
    return -1;                      \
  }

/****************************************************************************
 * Private Types
 ****************************************************************************/
static struct drv_gptimer_peripheral {
  GP_TIMER_INTR_Type *intr_reg;
  uint16_t is_config;
  uint32_t irq_enabled;
  DRV_GPTIMER_EventCallback callback;
  int32_t user_param;
} DRV_GPTIMER_Peripheral[] = {{GP_TIMER_INTR_0, 0, 0, NULL, 0}, {GP_TIMER_INTR_1, 0, 0, NULL, 0}};

static void DRV_GPTIMER_IrqHandler(int gptimer_ins);

/****************************************************************************
 * Private Data
 ****************************************************************************/
static GP_TIMER_CTRL_Type *gptimer_ctrl_reg = GP_TIMER_CTRL;
static NCO_Type *nco_reg = NCO;
static CLK_CONTROL_Type *clk_ctrl_reg = CLK_CONTROL;

static int32_t gptimer_irq_initialized = 0;

#if (configUSE_ALT_SLEEP == 1)
static uint8_t gptimer_registered_sleep_notify = 0;
#endif
/****************************************************************************
 * Functions
 ****************************************************************************/
void GP_Timer_0_IRQHandler(void) { DRV_GPTIMER_IrqHandler(0); }
void GP_Timer_1_IRQHandler(void) { DRV_GPTIMER_IrqHandler(1); }

static void DRV_GPTIMER_IrqHandler(int gptimer_ins) {
  if ((gptimer_ins == 0) || (gptimer_ins == 1)) {
    if (DRV_GPTIMER_Peripheral[gptimer_ins].irq_enabled &&
        DRV_GPTIMER_Peripheral[gptimer_ins].callback) {
      DRV_GPTIMER_Peripheral[gptimer_ins].callback(DRV_GPTIMER_Peripheral[gptimer_ins].user_param);
    }
  }
}

#if (configUSE_ALT_SLEEP == 1)
int32_t DRV_GPTIMER_HandleSleepNotify(DRV_SLEEP_NotifyType sleep_state, DRV_PM_PwrMode pwr_mode,
                                      void *ptr_ctx) {
  if (pwr_mode == DRV_PM_MODE_STANDBY || pwr_mode == DRV_PM_MODE_SHUTDOWN) {
    if (sleep_state == DRV_SLEEP_NTFY_SUSPENDING) {
      DRV_GPTIMER_Disable();
      DRV_GPTIMER_DisableInterrupt(0);
      DRV_GPTIMER_DisableInterrupt(1);
    } else if (sleep_state == DRV_SLEEP_NTFY_RESUMING) {
      gptimer_irq_initialized = 0;
    }
  }
  return 0;
}
#endif

void DRV_GPTIMER_Initialize(void) {
  if (gptimer_irq_initialized) return;

  size_t i;

  // enable clock for timer
  DRV_CLKGATING_EnableClkSource(CLK_GATING_TIMERS);

  // Enable NCO (no rate change)
  nco_reg->CFG |= NCO_CFG_MCU_NCO_ENABLE_Msk;

  // Disable GP Timer
  gptimer_ctrl_reg->CTRL = 0;

  // disable all interrupts
  DRV_GPTIMER_Peripheral[0].intr_reg->EN = 0;
  DRV_GPTIMER_Peripheral[1].intr_reg->EN = 0;

  // clear database
  for (i = 0; i < (sizeof(DRV_GPTIMER_Peripheral) / sizeof(DRV_GPTIMER_Peripheral[0])); i++) {
    DRV_GPTIMER_Peripheral[i].irq_enabled = 0;
    DRV_GPTIMER_Peripheral[i].callback = NULL;
    DRV_GPTIMER_Peripheral[i].user_param = 0;
    DRV_GPTIMER_Peripheral[i].intr_reg->EN = 0;
  }

  // Enable interrupts
  for (i = GP_Timer_0_IRQn; i <= GP_Timer_1_IRQn; i++) {
    NVIC_SetPriority((IRQn_Type)i, 7); /* set Interrupt priority */
    NVIC_EnableIRQ((IRQn_Type)i);
  }

#if (configUSE_ALT_SLEEP == 1)
  if (!gptimer_registered_sleep_notify) {
    int32_t temp_for_sleep_notify, ret_val;

    ret_val =
        DRV_SLEEP_RegNotification(&DRV_GPTIMER_HandleSleepNotify, &temp_for_sleep_notify, NULL);
    if (ret_val != 0) { /* failed to register sleep notify callback */
      ;
    } else {
      gptimer_registered_sleep_notify = 1;
    }
  }
#endif

  gptimer_irq_initialized = 1;
}

int32_t DRV_GPTIMER_RegisterInterrupt(int32_t gptimer, DRV_GPTIMER_EventCallback callback,
                                      int32_t user_param) {
  DRV_GPTIMER_Initialize();

  DRV_GPTIMER_Peripheral[gptimer].irq_enabled = 0;
  DRV_GPTIMER_Peripheral[gptimer].callback = callback;
  DRV_GPTIMER_Peripheral[gptimer].user_param = user_param;

  return 0;
}

uint32_t DRV_GPTIMER_GetValue(void) { return gptimer_ctrl_reg->VALUE; }

void DRV_GPTIMER_AddOffset(uint32_t offset) { gptimer_ctrl_reg->UPDT_OFFSET = offset; }

void DRV_GPTIMER_Enable(void) {
  // enable clock for timer
  clk_ctrl_reg->MCU_CLK_REQ_EN |= CLK_CONTROL_MCU_CLK_REQ_EN_MCU_TIMERS_Msk;
  // Enable timer
  gptimer_ctrl_reg->CTRL = GP_TIMER_CTRL_CTRL_EN_Msk;
}

void DRV_GPTIMER_Disable(void) {
  gptimer_ctrl_reg->CTRL = 0;

  clk_ctrl_reg->MCU_CLK_REQ_EN &= ~CLK_CONTROL_MCU_CLK_REQ_EN_MCU_TIMERS_Msk;
}

void DRV_GPTIMER_Activate(uint32_t value) {
  DRV_GPTIMER_Initialize();
  gptimer_ctrl_reg->PRESET_VALUE = value;  // Preset value
  DRV_GPTIMER_Enable();
}

int32_t DRV_GPTIMER_SetFrequency(int32_t frequency) {
  // target_rate = nco_clock * (nco_rate / wrap_value)
  int32_t nco_clock = SystemCoreClock;
  int32_t nco_rate, wrap_value;
  int32_t delta = 0, curr_delta, idiv_delta, roundup, curr_roundup;
  int32_t max_wrap_value = 0x10000;

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

  if ((wrap_value > 0) && (wrap_value <= max_wrap_value) && (nco_rate > 0) &&
      ((uint32_t)nco_rate <= NCO_CFG_MCU_NCO_RATE_Msk)) {
    nco_reg->WRAP_VALUE = wrap_value;
    nco_reg->CFG = nco_rate | (nco_reg->CFG & NCO_CFG_MCU_NCO_ENABLE_Msk);
    return (long long)nco_clock * nco_rate / wrap_value;
  } else {
    return -1;
  }
}

int32_t DRV_GPTIMER_SetInterruptOffset(int32_t gptimer, uint32_t offset) {
  GPTIMER_CHECK_INT_NUMBER(gptimer);

  if (offset > 0xffff) {
    return -1;
  }

  DRV_GPTIMER_Peripheral[gptimer].irq_enabled = 1;

  // Set target interrupt offset target
  DRV_GPTIMER_Peripheral[gptimer].intr_reg->SET_OFFSET = offset;

  // Enable Interrupt
  DRV_GPTIMER_Peripheral[gptimer].intr_reg->EN = GP_TIMER_INTR_0_EN_EN_Msk;

  return 0;
}

int32_t DRV_GPTIMER_SetInterruptValue(int32_t gptimer, uint32_t value) {
  GPTIMER_CHECK_INT_NUMBER(gptimer);

  DRV_GPTIMER_Peripheral[gptimer].irq_enabled = 1;

  // Set target interrupt offset target
  DRV_GPTIMER_Peripheral[gptimer].intr_reg->SET_VALUE = value;
  // Enable Interrupt
  DRV_GPTIMER_Peripheral[gptimer].intr_reg->EN = GP_TIMER_INTR_0_EN_EN_Msk;
  return 0;
}

int32_t DRV_GPTIMER_DisableInterrupt(int32_t gptimer) {
  GPTIMER_CHECK_INT_NUMBER(gptimer);
  DRV_GPTIMER_Peripheral[gptimer].irq_enabled = 0;
  DRV_GPTIMER_Peripheral[gptimer].intr_reg->EN = 0;
  return 0;
}

uint32_t DRV_GPTIMER_ReadTargetValue(int32_t gptimer) {
  GPTIMER_CHECK_INT_NUMBER(gptimer);
  return DRV_GPTIMER_Peripheral[gptimer].intr_reg->TARGET;
}

uint32_t DRV_GPTIMER_ReadCurrentValue(int32_t gptimer) {
  GPTIMER_CHECK_INT_NUMBER(gptimer);
  return DRV_GPTIMER_Peripheral[gptimer].intr_reg->VALUE;
}
