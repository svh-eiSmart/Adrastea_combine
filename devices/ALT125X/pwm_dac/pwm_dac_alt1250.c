/*  ---------------------------------------------------------------------------

    (c) copyright 2020 Altair Semiconductor, Ltd. All rights reserved.

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

#include "125X_mcu.h"
#include "pwm_dac.h"
#include "clk_gating.h"

#define PWM_UNDEF UINT_MAX

// clang-format off
#define PWM_MAX_CLK_DIV                  (MCU_PWM_DAC_OP_RATE0_OP_RATE_MSK + 1)
#define PWM_MAX_DUTY_CYCLE               (MCU_PWM_DAC_RATE0_RATE_MSK + 1)
#define PWM_MIN_CLK_DIV                  (1)
#define PWM_MIN_DUTY_CYCLE               (0)

#define PWM_DAC_OP_RATE_EN_MSK           MCU_PWM_DAC_OP_RATE0_EN_MSK
#define PWM_DAC_OP_RATE_EN_POS           MCU_PWM_DAC_OP_RATE0_EN_POS
#define PWM_DAC_OP_RATE_OP_RATE_MSK      MCU_PWM_DAC_OP_RATE0_OP_RATE_MSK
#define PWM_DAC_OP_RATE_OP_RATE_POS      MCU_PWM_DAC_OP_RATE0_OP_RATE_POS
#define PWM_DAC_RATE_RATE_MSK            MCU_PWM_DAC_RATE0_RATE_MSK
#define PWM_DAC_RATE_RATE_POS            MCU_PWM_DAC_RATE0_RATE_POS
#define PWM_DAC_OP_RATE_DISABLE_VAL_MSK  MCU_PWM_DAC_OP_RATE0_DISABLE_VAL_MSK
// clang-format on

static pwm_cfg_t pwm_defconfig[PWM_NUM] = {
    {PWM_UNDEF, PWM_UNDEF}, {PWM_UNDEF, PWM_UNDEF}, {PWM_UNDEF, PWM_UNDEF}, {PWM_UNDEF, PWM_UNDEF}};

static inline int pwm_is_init(eMcuPwmIds pwm_id) {
  return (pwm_defconfig[pwm_id].def_clk_div != PWM_UNDEF) &&
         (pwm_defconfig[pwm_id].def_duty_cycle != PWM_UNDEF);
}

static inline unsigned long pwm_op_rate_reg_addr(eMcuPwmIds pwm_id) {
  return BASE_ADDRESS_MCU_PWM_DAC + (0x008 * pwm_id);
}

static inline unsigned long pwm_rate_reg_addr(eMcuPwmIds pwm_id) {
  return pwm_op_rate_reg_addr(pwm_id) + 0x004;
}

static inline void pwm_disable(eMcuPwmIds pwm_id, unsigned int disable_value) {
  unsigned long op_rate_reg, v;

  op_rate_reg = pwm_op_rate_reg_addr(pwm_id);

  v = REGISTER(op_rate_reg);
  v &= ~(PWM_DAC_OP_RATE_EN_MSK);
  if (disable_value)
    v |= PWM_DAC_OP_RATE_DISABLE_VAL_MSK;
  else
    v &= ~(PWM_DAC_OP_RATE_DISABLE_VAL_MSK);
  REGISTER(op_rate_reg) = v;
}

static inline void pwm_enable(eMcuPwmIds pwm_id, unsigned int op_rate, unsigned int rate) {
  unsigned long op_rate_reg, rate_reg, v;

  op_rate_reg = pwm_op_rate_reg_addr(pwm_id);
  rate_reg = pwm_rate_reg_addr(pwm_id);

  v = REGISTER(rate_reg);
  v &= ~(PWM_DAC_RATE_RATE_MSK);
  v |= ((rate << PWM_DAC_RATE_RATE_POS) & PWM_DAC_RATE_RATE_MSK);
  REGISTER(rate_reg) = v;

  v = REGISTER(op_rate_reg);
  v |= PWM_DAC_OP_RATE_EN_MSK;
  v &= ~(PWM_DAC_OP_RATE_OP_RATE_MSK);
  v |= (op_rate << (PWM_DAC_OP_RATE_OP_RATE_POS));
  REGISTER(op_rate_reg) = v;
}

ePwmRet pwm_set(eMcuPwmIds pwm_id, unsigned int duty_cycle) {
  eIfMngrIf intf_id;
  unsigned long clk_div;

  if (pwm_id >= PWM_NUM) return PWM_RET_ID_ERR;

  if (!pwm_is_init(pwm_id)) {
    if (clk_gating_source_enable(CLK_GATING_PWM) != 0) return PWM_RET_IFCFG_ERR;

    intf_id = (eIfMngrIf)((int)IF_MNGR_PWM0 + (int)pwm_id);

    if ((if_mngr_load_defconfig(intf_id, &pwm_defconfig[pwm_id]) != IF_MNGR_SUCCESS) ||
        (if_mngr_config_io(intf_id) != IF_MNGR_SUCCESS)) {
      pwm_defconfig[pwm_id].def_clk_div = PWM_UNDEF;
      pwm_defconfig[pwm_id].def_duty_cycle = PWM_UNDEF;
      return PWM_RET_IFCFG_ERR;
    }
  }

  clk_div = pwm_defconfig[pwm_id].def_clk_div;

  if (duty_cycle == (unsigned int)PWM_DUTY_CYCLE_DEFCONFIG)
    duty_cycle = pwm_defconfig[pwm_id].def_duty_cycle;

  if (clk_div > PWM_MAX_CLK_DIV || duty_cycle > PWM_MAX_DUTY_CYCLE) return PWM_RET_PARAM_ERR;

  if (duty_cycle == PWM_MIN_DUTY_CYCLE)
    pwm_disable(pwm_id, 0);
  else if (duty_cycle == PWM_MAX_DUTY_CYCLE)
    pwm_disable(pwm_id, 1);
  else
    pwm_enable(pwm_id, clk_div - 1, duty_cycle);

  return PWM_RET_SUCCESS;
}

ePwmRet pwm_get(eMcuPwmIds pwm_id, unsigned int *pwm_en, unsigned int *clk_div,
                unsigned int *duty_cycle) {
  unsigned long op_rate_v, rate_v;

  if (pwm_id >= PWM_NUM) return PWM_RET_ID_ERR;

  op_rate_v = REGISTER(pwm_op_rate_reg_addr(pwm_id));
  *pwm_en = (op_rate_v & PWM_DAC_OP_RATE_EN_MSK) >> PWM_DAC_OP_RATE_EN_POS;
  *clk_div = ((op_rate_v & PWM_DAC_OP_RATE_OP_RATE_MSK) >> PWM_DAC_OP_RATE_OP_RATE_POS) + 1;

  rate_v = REGISTER(pwm_rate_reg_addr(pwm_id));
  if (*pwm_en == 0) {
    if (op_rate_v & PWM_DAC_OP_RATE_DISABLE_VAL_MSK)
      *duty_cycle = PWM_MAX_DUTY_CYCLE;
    else
      *duty_cycle = PWM_MIN_DUTY_CYCLE;
  } else {
    *duty_cycle = (rate_v & PWM_DAC_RATE_RATE_MSK) >> PWM_DAC_RATE_RATE_POS;
  }

  return PWM_RET_SUCCESS;
}
