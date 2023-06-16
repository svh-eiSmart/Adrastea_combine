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
#include "stdio.h"
#include "stdlib.h"
#include DEVICE_HEADER
#include "DRV_LED.h"
#include "DRV_IF_CFG.h"
#include "DRV_CLOCK_GATING.h"

#if defined(ALT1250)
#define LED_CTRL_NUM (6)
#elif defined(ALT1255)
#define LED_CTRL_NUM (3)
#endif

#define SLEEP_CLOCK_32K (0x20)
#define SLEEP_CLOCK_19_2M (0x4B00)
#define SLEEP_CLOCK_26M (0x6590)
#define LED_CTRL_RESTART_LED_0 (0x1)
#define LED_CTRL_RESTART_DIM_0 (0x100)
#define LED_CTRL_RESTART_DIM_1 (0x200)
#define LED_CTRL_RESTART_CNTR (0x400)

#define MCU_LED_CTRL_DURATION_REG MCU_LED_CTRL_GRF_LED_CTRL_DURATION_0
#define MCU_LED_CTRL_ON_DURATION_POS LED_CTRL_DURATION_0_GRF_LED_CTRL_ON_DURATION_Pos
#define MCU_LED_CTRL_ON_DURATION_MSK LED_CTRL_DURATION_0_GRF_LED_CTRL_ON_DURATION_Msk
#define MCU_LED_CTRL_OFF_DURATION_POS LED_CTRL_DURATION_0_GRF_LED_CTRL_OFF_DURATION_Pos
#define MCU_LED_CTRL_OFF_DURATION_MSK LED_CTRL_DURATION_0_GRF_LED_CTRL_OFF_DURATION_Msk

#define MCU_LED_CTRL_CFG_REG MCU_LED_CTRL_GRF_LED_CTRL_CFG_0
#define MCU_LED_CTRL_CFG_OFFSET_POS LED_CTRL_CFG_0_GRF_LED_CTRL_OFFSET_Pos
#define MCU_LED_CTRL_CFG_OFFSET_MSK LED_CTRL_CFG_0_GRF_LED_CTRL_OFFSET_Msk
#define MCU_LED_CTRL_CFG_DIM_SEL_POS LED_CTRL_CFG_0_GRF_LED_CTRL_DIM_SEL_Pos
#define MCU_LED_CTRL_CFG_DIM_SEL_MSK LED_CTRL_CFG_0_GRF_LED_CTRL_DIM_SEL_Msk
#define MCU_LED_CTRL_CFG_POLARITY_POS LED_CTRL_CFG_0_GRF_LED_CTRL_POLARITY_Pos
#define MCU_LED_CTRL_CFG_POLARITY_MSK LED_CTRL_CFG_0_GRF_LED_CTRL_POLARITY_Msk
#define MCU_LED_CTRL_CFG_CLK_EN_POS LED_CTRL_CFG_0_GRF_LED_CTRL_CLK_EN_Pos
#define MCU_LED_CTRL_CFG_CLK_EN_MSK LED_CTRL_CFG_0_GRF_LED_CTRL_CLK_EN_Msk

#define MCU_LED_CTRL_DIM_CFG_DIM_BRIGHT_LEVELS_POS \
  LED_CTRL_DIM_CFG_0_GRF_LED_CTRL_DIM_BRIGHT_LEVELS_Pos
#define MCU_LED_CTRL_DIM_CFG_DIM_BRIGHT_LEVELS_MSK \
  LED_CTRL_DIM_CFG_0_GRF_LED_CTRL_DIM_BRIGHT_LEVELS_Msk
#define MCU_LED_CTRL_DIM_CFG_DIM_BASE_UNIT_REPEATS_POS \
  LED_CTRL_DIM_CFG_0_GRF_LED_CTRL_DIM_BASE_UNIT_REPEATS_Pos
#define MCU_LED_CTRL_DIM_CFG_DIM_BASE_UNIT_REPEATS_MSK \
  LED_CTRL_DIM_CFG_0_GRF_LED_CTRL_DIM_BASE_UNIT_REPEATS_Msk
#define MCU_LED_CTRL_DIM_CFG_DIM_HIGH_DELAY_POS LED_CTRL_DIM_CFG_0_GRF_LED_CTRL_DIM_HIGH_DELAY_Pos
#define MCU_LED_CTRL_DIM_CFG_DIM_HIGH_DELAY_MSK LED_CTRL_DIM_CFG_0_GRF_LED_CTRL_DIM_HIGH_DELAY_Msk
#define MCU_LED_CTRL_DIM_CFG_DIM_LOW_DELAY_POS LED_CTRL_DIM_CFG_0_GRF_LED_CTRL_DIM_LOW_DELAY_Pos
#define MCU_LED_CTRL_DIM_CFG_DIM_LOW_DELAY_MSK LED_CTRL_DIM_CFG_0_GRF_LED_CTRL_DIM_LOW_DELAY_Msk

#define CHECK_PARAM(x)      \
  if (!x) {                 \
    return LED_ERROR_PARAM; \
  }
struct LED_CONTROLLER {
  volatile uint32_t *cfg;
  volatile uint32_t *duration;
  Interface_Id if_id;
};
static struct LED_CONTROLLER LED_CNTR[] = {
    {
        .cfg = &LED_CTRL->CFG_0,
        .duration = &LED_CTRL->DURATION_0,
        .if_id = IF_CFG_LED0,
    },
    {
        .cfg = &LED_CTRL->CFG_1,
        .duration = &LED_CTRL->DURATION_1,
        .if_id = IF_CFG_LED1,
    },
    {
        .cfg = &LED_CTRL->CFG_2,
        .duration = &LED_CTRL->DURATION_2,
        .if_id = IF_CFG_LED2,
    },
#ifdef ALT1250
    {
        .cfg = &LED_CTRL->CFG_3,
        .duration = &LED_CTRL->DURATION_3,
        .if_id = IF_CFG_LED3,
    },
    {
        .cfg = &LED_CTRL->CFG_4,
        .duration = &LED_CTRL->DURATION_4,
        .if_id = IF_CFG_LED4,
    },
    {
        .cfg = &LED_CTRL->CFG_5,
        .duration = &LED_CTRL->DURATION_5,
        .if_id = IF_CFG_LED5,
    },
#endif
};

static inline void insert_value(uint32_t *word, uint32_t value, uint32_t pos, uint32_t mask) {
  *word &= ~mask;
  value &= (mask >> pos);
  *word |= (value << pos);
};

Led_Err_Code DRV_LED_Initialize(void) {
#define CLOCK_ENABLE (1)
  DRV_CLKGATING_EnableClkSource(CLK_GATING_CLK32K_LED);
  LED_CTRL->CNTR_CFG_b.GRF_LED_CTRL_CNTR_TARGET = SLEEP_CLOCK_32K;
  LED_CTRL->CNTR_CFG_b.GRF_LED_CTRL_CNTR_CLK_EN = 1;
  LED_CTRL->RESTART = LED_CTRL_RESTART_CNTR;
  return LED_ERROR_NONE;
}

Led_Err_Code DRV_LED_Uninitialize(void) { return LED_ERROR_NONE; }

Led_Err_Code DRV_LED_Set_Led_Mode(int led_channel, DRV_LED_Param *param) {
  CHECK_PARAM(param);

  int i;
  uint32_t reg_value;
  struct LED_CONTROLLER *led = NULL;

  for (i = 0; i < LED_CTRL_NUM; i++) {
    if ((led_channel >> i) & 0x1) {
      led = &LED_CNTR[i];
      CHECK_PARAM(led);
      *led->duration = 0;
      reg_value = 0;

      insert_value(&reg_value, param->on_duration, MCU_LED_CTRL_ON_DURATION_POS,
                   MCU_LED_CTRL_ON_DURATION_MSK);
      insert_value(&reg_value, param->off_duration, MCU_LED_CTRL_OFF_DURATION_POS,
                   MCU_LED_CTRL_OFF_DURATION_MSK);
      *led->duration = reg_value;

      reg_value = 0;
      insert_value(&reg_value, param->timing.offset, MCU_LED_CTRL_CFG_OFFSET_POS,
                   MCU_LED_CTRL_CFG_OFFSET_MSK);
      insert_value(&reg_value, param->timing.polarity, MCU_LED_CTRL_CFG_POLARITY_POS,
                   MCU_LED_CTRL_CFG_POLARITY_MSK);

      // disable dim mode
      insert_value(&reg_value, 0, LED_CTRL_CFG_0_GRF_LED_CTRL_DIM_SEL_Pos,
                   LED_CTRL_CFG_0_GRF_LED_CTRL_DIM_SEL_Msk);

      // enable clock
      insert_value(&reg_value, 1, MCU_LED_CTRL_CFG_CLK_EN_POS, MCU_LED_CTRL_CFG_CLK_EN_MSK);
      *led->cfg = reg_value;
    }
  }
  return LED_ERROR_NONE;
}

Led_Err_Code DRV_LED_Set_Dim_Mode(int led_channel, DRV_LED_Dim_Param *param) {
  CHECK_PARAM(param);

  int i;
  uint32_t reg_value;
  struct LED_CONTROLLER *led = NULL;

  for (i = 0; i < LED_CTRL_NUM; i++) {
    if ((led_channel >> i) & 0x1) {
      led = &LED_CNTR[i];
      CHECK_PARAM(led);
      reg_value = 0;

      insert_value(&reg_value, param->bright_level, MCU_LED_CTRL_DIM_CFG_DIM_BRIGHT_LEVELS_POS,
                   MCU_LED_CTRL_DIM_CFG_DIM_BRIGHT_LEVELS_MSK);
      insert_value(&reg_value, param->repeat_times, MCU_LED_CTRL_DIM_CFG_DIM_BASE_UNIT_REPEATS_POS,
                   MCU_LED_CTRL_DIM_CFG_DIM_BASE_UNIT_REPEATS_MSK);
      insert_value(&reg_value, param->brightest_hold, MCU_LED_CTRL_DIM_CFG_DIM_HIGH_DELAY_POS,
                   MCU_LED_CTRL_DIM_CFG_DIM_HIGH_DELAY_MSK);
      insert_value(&reg_value, param->darkest_hold, MCU_LED_CTRL_DIM_CFG_DIM_LOW_DELAY_POS,
                   MCU_LED_CTRL_DIM_CFG_DIM_LOW_DELAY_MSK);

      if (param->dim_sel == eDIM0) {
        LED_CTRL->DIM_CFG_0 = reg_value;
      } else if (param->dim_sel == eDIM1) {
        LED_CTRL->DIM_CFG_1 = reg_value;
      } else {
        return LED_ERROR_PARAM;
      }
      *led->cfg = 0;
      reg_value = 0;
      insert_value(&reg_value, param->timing.offset, MCU_LED_CTRL_CFG_OFFSET_POS,
                   MCU_LED_CTRL_CFG_OFFSET_MSK);
      insert_value(&reg_value, param->timing.polarity, MCU_LED_CTRL_CFG_POLARITY_POS,
                   MCU_LED_CTRL_CFG_POLARITY_MSK);

      // enable dim mode
      insert_value(&reg_value, (uint32_t)param->dim_sel, MCU_LED_CTRL_CFG_DIM_SEL_POS,
                   MCU_LED_CTRL_CFG_DIM_SEL_MSK);

      // diable clock
      insert_value(&reg_value, 0, MCU_LED_CTRL_CFG_CLK_EN_POS, MCU_LED_CTRL_CFG_CLK_EN_MSK);
      *led->cfg = reg_value;
    }
  }
  return LED_ERROR_NONE;
}

Led_Err_Code DRV_LED_Start(int led_channel) {
  int i;
  uint32_t ctrl_cfg_value = 0;
  uint32_t reg_value = 0;
  struct LED_CONTROLLER *led = NULL;

  for (i = 0; i < LED_CTRL_NUM; i++) {
    if ((led_channel >> i) & 0x1) {
      led = &LED_CNTR[i];
      CHECK_PARAM(led);
      DRV_IF_CFG_SetIO(led->if_id);

      reg_value |= LED_CTRL_RESTART_LED_0 << i;
      ctrl_cfg_value = *led->cfg;
      ctrl_cfg_value =
          (ctrl_cfg_value & MCU_LED_CTRL_CFG_DIM_SEL_MSK) >> MCU_LED_CTRL_CFG_DIM_SEL_POS;

      if (ctrl_cfg_value == eDIM0)
        reg_value |= LED_CTRL_RESTART_DIM_0;
      else if (ctrl_cfg_value == eDIM1)
        reg_value |= LED_CTRL_RESTART_DIM_1;
    }
  }
  LED_CTRL->RESTART = reg_value;
  return LED_ERROR_NONE;
}

Led_Err_Code DRV_LED_Stop(int led_channel) {
  int i;
  uint32_t reg_value = 0;
  struct LED_CONTROLLER *led = NULL;

  for (i = 0; i < LED_CTRL_NUM; i++) {
    if ((led_channel >> i) & 0x1) {
      led = &LED_CNTR[i];
      CHECK_PARAM(led);
      reg_value |= LED_CTRL_RESTART_LED_0 << i;
      *led->cfg = 0;
    }
  }
  LED_CTRL->RESTART = reg_value;
  return LED_ERROR_NONE;
}
