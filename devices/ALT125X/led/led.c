/*  ---------------------------------------------------------------------------

        (c) copyright 2017 Altair Semiconductor, Ltd. All rights reserved.

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
#include "led.h"
#include "mcu_exported.h"
#include "if_mngr.h"
#include "clk_gating.h"

#if defined(ALT1250)
#define LED_CTRL_NUM (6)
#elif defined(ALT1255)
#define  LED_CTRL_NUM (3)
#else
#error "Wrong config"
#endif
#define SLEEP_CLOCK_32K (0x20)
#define SLEEP_CLOCK_19_2M (0x4B00)
#define SLEEP_CLOCK_26M (0x6590)
#define LED_CTRL_RESTART_LED_0 (0x1)
#define LED_CTRL_RESTART_DIM_0 (0x100)
#define LED_CTRL_RESTART_DIM_1 (0x200)
#define LED_CTRL_RESTART_CNTR (0x400)

#define MCU_LED_CTRL_DURATION_REG MCU_LED_CTRL_GRF_LED_CTRL_DURATION_0
#define MCU_LED_CTRL_ON_DURATION_POS \
  MCU_LED_CTRL_GRF_LED_CTRL_DURATION_0_GRF_LED_CTRL_ON_DURATION_POS
#define MCU_LED_CTRL_ON_DURATION_MSK \
  MCU_LED_CTRL_GRF_LED_CTRL_DURATION_0_GRF_LED_CTRL_ON_DURATION_MSK
#define MCU_LED_CTRL_OFF_DURATION_POS \
  MCU_LED_CTRL_GRF_LED_CTRL_DURATION_0_GRF_LED_CTRL_OFF_DURATION_POS
#define MCU_LED_CTRL_OFF_DURATION_MSK \
  MCU_LED_CTRL_GRF_LED_CTRL_DURATION_0_GRF_LED_CTRL_OFF_DURATION_MSK

#define MCU_LED_CTRL_CFG_REG MCU_LED_CTRL_GRF_LED_CTRL_CFG_0
#define MCU_LED_CTRL_CFG_OFFSET_POS MCU_LED_CTRL_GRF_LED_CTRL_CFG_0_GRF_LED_CTRL_OFFSET_POS
#define MCU_LED_CTRL_CFG_OFFSET_MSK MCU_LED_CTRL_GRF_LED_CTRL_CFG_0_GRF_LED_CTRL_OFFSET_MSK
#define MCU_LED_CTRL_CFG_DIM_SEL_POS MCU_LED_CTRL_GRF_LED_CTRL_CFG_0_GRF_LED_CTRL_DIM_SEL_POS
#define MCU_LED_CTRL_CFG_DIM_SEL_MSK MCU_LED_CTRL_GRF_LED_CTRL_CFG_0_GRF_LED_CTRL_DIM_SEL_MSK
#define MCU_LED_CTRL_CFG_POLARITY_POS MCU_LED_CTRL_GRF_LED_CTRL_CFG_0_GRF_LED_CTRL_POLARITY_POS
#define MCU_LED_CTRL_CFG_POLARITY_MSK MCU_LED_CTRL_GRF_LED_CTRL_CFG_0_GRF_LED_CTRL_POLARITY_MSK
#define MCU_LED_CTRL_CFG_CLK_EN_POS MCU_LED_CTRL_GRF_LED_CTRL_CFG_0_GRF_LED_CTRL_CLK_EN_POS
#define MCU_LED_CTRL_CFG_CLK_EN_MSK MCU_LED_CTRL_GRF_LED_CTRL_CFG_0_GRF_LED_CTRL_CLK_EN_MSK

#define MCU_LED_CTRL_DIM_CFG_DIM_BRIGHT_LEVELS_POS \
  MCU_LED_CTRL_GRF_LED_CTRL_DIM_CFG_0_GRF_LED_CTRL_DIM_BRIGHT_LEVELS_POS
#define MCU_LED_CTRL_DIM_CFG_DIM_BRIGHT_LEVELS_MSK \
  MCU_LED_CTRL_GRF_LED_CTRL_DIM_CFG_0_GRF_LED_CTRL_DIM_BRIGHT_LEVELS_MSK
#define MCU_LED_CTRL_DIM_CFG_DIM_BASE_UNIT_REPEATS_POS \
  MCU_LED_CTRL_GRF_LED_CTRL_DIM_CFG_0_GRF_LED_CTRL_DIM_BASE_UNIT_REPEATS_POS
#define MCU_LED_CTRL_DIM_CFG_DIM_BASE_UNIT_REPEATS_MSK \
  MCU_LED_CTRL_GRF_LED_CTRL_DIM_CFG_0_GRF_LED_CTRL_DIM_BASE_UNIT_REPEATS_MSK
#define MCU_LED_CTRL_DIM_CFG_DIM_HIGH_DELAY_POS \
  MCU_LED_CTRL_GRF_LED_CTRL_DIM_CFG_0_GRF_LED_CTRL_DIM_HIGH_DELAY_POS
#define MCU_LED_CTRL_DIM_CFG_DIM_HIGH_DELAY_MSK \
  MCU_LED_CTRL_GRF_LED_CTRL_DIM_CFG_0_GRF_LED_CTRL_DIM_HIGH_DELAY_MSK
#define MCU_LED_CTRL_DIM_CFG_DIM_LOW_DELAY_POS \
  MCU_LED_CTRL_GRF_LED_CTRL_DIM_CFG_0_GRF_LED_CTRL_DIM_LOW_DELAY_POS
#define MCU_LED_CTRL_DIM_CFG_DIM_LOW_DELAY_MSK \
  MCU_LED_CTRL_GRF_LED_CTRL_DIM_CFG_0_GRF_LED_CTRL_DIM_LOW_DELAY_MSK

static inline void insert_value(uint32_t* word, uint32_t value, uint32_t pos, uint32_t mask) {
  *word &= ~mask;
  value &= (mask >> pos);
  *word |= (value << pos);
};

void led_init(void) {
#define CLOCK_ENABLE (1)
  clk_gating_source_enable(CLK_GATING_CLK32K_LED);
  uint32_t reg_val =
      (SLEEP_CLOCK_32K << MCU_LED_CTRL_GRF_LED_CTRL_CNTR_CFG_GRF_LED_CTRL_CNTR_TARGET_POS) |
      CLOCK_ENABLE;
  REGISTER(MCU_LED_CTRL_GRF_LED_CTRL_CNTR_CFG) = reg_val;       // enable controller
  REGISTER(BASE_ADDRESS_MCU_LED_CTRL) = LED_CTRL_RESTART_CNTR;  // restart controller

  // init pin functionality according to IF config
  if_mngr_config_io(IF_MNGR_LED0);
  if_mngr_config_io(IF_MNGR_LED1);
  if_mngr_config_io(IF_MNGR_LED2);
#ifdef ALT1250
  if_mngr_config_io(IF_MNGR_LED3);
  if_mngr_config_io(IF_MNGR_LED4);
  if_mngr_config_io(IF_MNGR_LED5);
#endif
}

void set_led_mode(int led_channel, sLED_param* param) {
  int i;
  uint32_t reg_base;
  uint32_t reg_value;

  for (i = 0; i < LED_CTRL_NUM; i++) {
    if ((led_channel >> i) & 0x1) {
      reg_base = MCU_LED_CTRL_DURATION_REG + i * 4;
      REGISTER(reg_base) = 0;
      reg_value = 0;
      insert_value(&reg_value, param->on_duration, MCU_LED_CTRL_ON_DURATION_POS,
                   MCU_LED_CTRL_ON_DURATION_MSK);
      insert_value(&reg_value, param->off_duration, MCU_LED_CTRL_OFF_DURATION_POS,
                   MCU_LED_CTRL_OFF_DURATION_MSK);
      REGISTER(reg_base) = reg_value;

      reg_base = MCU_LED_CTRL_CFG_REG + i * 4;
      REGISTER(reg_base) = 0;
      reg_value = 0;
      insert_value(&reg_value, param->timing.offset, MCU_LED_CTRL_CFG_OFFSET_POS,
                   MCU_LED_CTRL_CFG_OFFSET_MSK);
      insert_value(&reg_value, param->timing.polarity, MCU_LED_CTRL_CFG_POLARITY_POS,
                   MCU_LED_CTRL_CFG_POLARITY_MSK);

      // disable dim mode
      insert_value(&reg_value, 0, MCU_LED_CTRL_CFG_DIM_SEL_POS, MCU_LED_CTRL_CFG_DIM_SEL_MSK);

      // enable clock
      insert_value(&reg_value, 1, MCU_LED_CTRL_CFG_CLK_EN_POS, MCU_LED_CTRL_CFG_CLK_EN_MSK);
      REGISTER(reg_base) = reg_value;
    }
  }
}

void set_dim_mode(int led_channel, sDIM_param* param) {
  int i;
  uint32_t reg_base;
  uint32_t reg_value = 0;

  for (i = 0; i < LED_CTRL_NUM; i++) {
    if ((led_channel >> i) & 0x1) {
      if (param->dim_sel == eDIM0)
        reg_base = MCU_LED_CTRL_GRF_LED_CTRL_DIM_CFG_0;
      else
        reg_base = MCU_LED_CTRL_GRF_LED_CTRL_DIM_CFG_1;

      REGISTER(reg_base) = 0;  // clear register
      reg_value = 0;
      insert_value(&reg_value, param->bright_level, MCU_LED_CTRL_DIM_CFG_DIM_BRIGHT_LEVELS_POS,
                   MCU_LED_CTRL_DIM_CFG_DIM_BRIGHT_LEVELS_MSK);
      insert_value(&reg_value, param->repeat_times, MCU_LED_CTRL_DIM_CFG_DIM_BASE_UNIT_REPEATS_POS,
                   MCU_LED_CTRL_DIM_CFG_DIM_BASE_UNIT_REPEATS_MSK);
      insert_value(&reg_value, param->brightest_hold, MCU_LED_CTRL_DIM_CFG_DIM_HIGH_DELAY_POS,
                   MCU_LED_CTRL_DIM_CFG_DIM_HIGH_DELAY_MSK);
      insert_value(&reg_value, param->darkest_hold, MCU_LED_CTRL_DIM_CFG_DIM_LOW_DELAY_POS,
                   MCU_LED_CTRL_DIM_CFG_DIM_LOW_DELAY_MSK);
      REGISTER(reg_base) = reg_value;

      reg_base = MCU_LED_CTRL_GRF_LED_CTRL_CFG_0 + i * 4;
      REGISTER(reg_base) = 0;
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
      REGISTER(reg_base) = reg_value;
    }
  }
}

void led_start(int led_channel) {
  int i;
  uint32_t reg_base = MCU_LED_CTRL_CFG_REG;
  uint32_t ctrl_cfg_value = 0;
  uint32_t reg_value = 0;

  for (i = 0; i < LED_CTRL_NUM; i++) {
    if ((led_channel >> i) & 0x1) {
      reg_value |= LED_CTRL_RESTART_LED_0 << i;
      ctrl_cfg_value = REGISTER(reg_base + i * 4);

      ctrl_cfg_value =
          (ctrl_cfg_value & MCU_LED_CTRL_CFG_DIM_SEL_MSK) >> MCU_LED_CTRL_CFG_DIM_SEL_POS;
      if (ctrl_cfg_value == eDIM0)
        reg_value |= LED_CTRL_RESTART_DIM_0;
      else if (ctrl_cfg_value == eDIM1)
        reg_value |= LED_CTRL_RESTART_DIM_1;
    }
  }
  REGISTER(MCU_LED_CTRL_GRF_LED_CTRL_RESTART) = reg_value;
}

void led_stop(int led_channel) {
  int i;
  uint32_t reg_base = MCU_LED_CTRL_CFG_REG;
  uint32_t reg_value = 0;

  for (i = 0; i < LED_CTRL_NUM; i++) {
    if ((led_channel >> i) & 0x1) {
      reg_value |= LED_CTRL_RESTART_LED_0 << i;
      REGISTER(reg_base + i * 4) = 0;
    }
  }
  REGISTER(MCU_LED_CTRL_GRF_LED_CTRL_RESTART) = reg_value;
}
