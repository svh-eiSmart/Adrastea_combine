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
/*****************************************************************************
 * PWM_DAC hardware block is only available on ALT1250.
 * DO NOT use this on ALT1255 platform.
 *****************************************************************************/
#include "ALT1250.h"
#include "DRV_COMMON.h"
#include "DRV_PWM_DAC.h"
#include "DRV_IF_CFG.h"
#include "DRV_CLOCK_GATING.h"
#if (configUSE_ALT_SLEEP == 1)
#include "DRV_PM.h"
#include "DRV_SLEEP.h"
#endif

#include <string.h>

#define PWM_MAX_CLK_DIV (PWM_DAC_OP_RATE0_OP_RATE_Msk + 1)
#define PWM_MAX_DUTY_CYCLE (PWM_DAC_RATE0_RATE_Msk + 1)
#define PWM_MIN_CLK_DIV (1)
#define PWM_MIN_DUTY_CYCLE (0)

#define PWM_DAC_OP_RATE_EN_MSK PWM_DAC_OP_RATE0_EN_Msk
#define PWM_DAC_OP_RATE_EN_POS PWM_DAC_OP_RATE0_EN_Pos
#define PWM_DAC_OP_RATE_OP_RATE_MSK PWM_DAC_OP_RATE0_OP_RATE_Msk
#define PWM_DAC_OP_RATE_OP_RATE_POS PWM_DAC_OP_RATE0_OP_RATE_Pos
#define PWM_DAC_OP_RATE_DISABLE_VAL_MSK PWM_DAC_OP_RATE0_DISABLE_VAL_Msk

#define PWM_DAC_RATE_RATE_MSK PWM_DAC_RATE0_RATE_Msk
#define PWM_DAC_RATE_RATE_POS PWM_DAC_RATE0_RATE_Pos

#define PWM_DAC_OP_RATE_REG(id) (REGISTER(PWM_DAC_BASE + (0x0008 * pwm_id)))
#define PWM_DAC_RATE_REG(id) (REGISTER((PWM_DAC_BASE + (0x0008 * pwm_id)) + 0x0004))

#if (configUSE_ALT_SLEEP == 1)
typedef struct {
  uint32_t op_rate;
  uint32_t rate;
} PWM_DAC_Sleep_Ctx;
#endif
typedef struct {
  PWM_DAC_PowerState pwr_state;
  uint8_t init_done;
#if (configUSE_ALT_SLEEP == 1)
  PWM_DAC_Sleep_Ctx sleep_ctx[PWM_DAC_ID_MAX];
  int32_t sleep_notify_inx;
#endif
} PWM_DAC_Peripheral;

static PWM_DAC_Peripheral pwm_peripheral = {.pwr_state = PWM_POWER_OFF, .init_done = 0};

static void DRV_PWM_DAC_Disable(PWM_DAC_Id pwm_id, uint32_t disable_value) {
  uint32_t v;

  v = PWM_DAC_OP_RATE_REG(pwm_id);
  v &= ~(PWM_DAC_OP_RATE_OP_RATE_MSK);
  v &= ~(PWM_DAC_OP_RATE_EN_MSK);
  if (disable_value)
    v |= PWM_DAC_OP_RATE_DISABLE_VAL_MSK;
  else
    v &= ~(PWM_DAC_OP_RATE_DISABLE_VAL_MSK);
  PWM_DAC_OP_RATE_REG(pwm_id) = v;

  v = PWM_DAC_RATE_REG(pwm_id);
  v &= ~(PWM_DAC_RATE_RATE_MSK);
  PWM_DAC_RATE_REG(pwm_id) = v;
}

static void DRV_PWM_DAC_Enable(PWM_DAC_Id pwm_id, uint32_t op_rate, uint32_t rate) {
  uint32_t v;

  v = PWM_DAC_RATE_REG(pwm_id);
  v &= ~(PWM_DAC_RATE_RATE_MSK);
  v |= ((rate << PWM_DAC_RATE_RATE_POS) & PWM_DAC_RATE_RATE_MSK);
  PWM_DAC_RATE_REG(pwm_id) = v;

  v = PWM_DAC_OP_RATE_REG(pwm_id);
  v |= PWM_DAC_OP_RATE_EN_MSK;
  v &= ~(PWM_DAC_OP_RATE_OP_RATE_MSK);
  v |= ((op_rate << (PWM_DAC_OP_RATE_OP_RATE_POS)) & PWM_DAC_OP_RATE_OP_RATE_MSK);
  PWM_DAC_OP_RATE_REG(pwm_id) = v;
}

#if (configUSE_ALT_SLEEP == 1)
static int32_t DRV_PWM_DAC_SleepHandler(DRV_SLEEP_NotifyType state, DRV_PM_PwrMode pwr_mode,
                                        void *arg) {
  PWM_DAC_Id pwm_id;
  if (pwr_mode == DRV_PM_MODE_STANDBY || pwr_mode == DRV_PM_MODE_SHUTDOWN) {
    if (state == DRV_SLEEP_NTFY_SUSPENDING) {
      for (pwm_id = PWM_DAC_ID_0; pwm_id < PWM_DAC_ID_MAX; pwm_id++) {
        pwm_peripheral.sleep_ctx[pwm_id].rate = PWM_DAC_RATE_REG(pwm_id);
        pwm_peripheral.sleep_ctx[pwm_id].op_rate = PWM_DAC_OP_RATE_REG(pwm_id);
      }
    } else if (state == DRV_SLEEP_NTFY_RESUMING) {
      for (pwm_id = PWM_DAC_ID_0; pwm_id < PWM_DAC_ID_MAX; pwm_id++) {
        PWM_DAC_RATE_REG(pwm_id) = pwm_peripheral.sleep_ctx[pwm_id].rate;
        PWM_DAC_OP_RATE_REG(pwm_id) = pwm_peripheral.sleep_ctx[pwm_id].op_rate;
      }
    }
  }
  return 0;
}
#endif

DRV_PWM_DAC_Status DRV_PWM_DAC_Initialize(void) {
  Interface_Id intf;
  DRV_IF_CFG_Status ifcfg_ret;
  PWM_DAC_Id pwm_id;
  if (pwm_peripheral.init_done) return DRV_PWM_DAC_ERROR_INIT;

#if (configUSE_ALT_SLEEP == 1)
  if (DRV_SLEEP_RegNotification(DRV_PWM_DAC_SleepHandler, &pwm_peripheral.sleep_notify_inx, NULL) !=
      0)
    return DRV_PWM_DAC_ERROR_PM;
#endif
  for (pwm_id = 0; pwm_id < PWM_DAC_ID_MAX; pwm_id++) {
    intf = (Interface_Id)IF_CFG_PWM0 + pwm_id;
    ifcfg_ret = DRV_IF_CFG_SetIO(intf);
    /*Configure PIN mux. Do not return error if user choose to left configuration in interface
     * header undefined. User can use DRV_IO_SetMux to configure mux by itself*/
    if (ifcfg_ret != DRV_IF_CFG_OK && ifcfg_ret != DRV_IF_CFG_ERROR_UNDEFINED)
      return DRV_PWM_DAC_ERROR_IF_CFG;
  }
  pwm_peripheral.init_done = 1;
  return DRV_PWM_DAC_OK;
}

DRV_PWM_DAC_Status DRV_PWM_DAC_Uninitialized(void) {
  if (!pwm_peripheral.init_done) return DRV_PWM_DAC_ERROR_INIT;
#if (configUSE_ALT_SLEEP == 1)
  if (DRV_SLEEP_UnRegNotification(pwm_peripheral.sleep_notify_inx) != 0)
    return DRV_PWM_DAC_ERROR_PM;
#endif
  DRV_PWM_DAC_Disable(PWM_DAC_ID_0, 0);
  DRV_PWM_DAC_Disable(PWM_DAC_ID_1, 0);
  DRV_PWM_DAC_Disable(PWM_DAC_ID_2, 0);
  DRV_PWM_DAC_Disable(PWM_DAC_ID_3, 0);
  pwm_peripheral.init_done = 0;
  return DRV_PWM_DAC_OK;
}

DRV_PWM_DAC_Status DRV_PWM_DAC_PowerControl(PWM_DAC_PowerState state) {
  switch (state) {
    case PWM_POWER_FULL:
      DRV_CLKGATING_EnableClkSource(CLK_GATING_PWM);
      break;
    case PWM_POWER_OFF:
      DRV_CLKGATING_DisableClkSource(CLK_GATING_PWM);
      break;
    case PWM_POWER_LOW:
    default:
      return DRV_PWM_DAC_ERROR_UNSUPPORTED;
      break;
  }
  pwm_peripheral.pwr_state = state;
  return DRV_PWM_DAC_OK;
}

DRV_PWM_DAC_Status DRV_PWM_DAC_GetDefConf(PWM_DAC_Id pwm_id, PWM_DAC_Param *param) {
  Interface_Id intf_id;
  PWM_DAC_Param intf_param;

  if (pwm_id >= PWM_DAC_ID_MAX) return DRV_PWM_DAC_ERROR_PARAM;

  intf_id = (Interface_Id)((int)IF_CFG_PWM0 + (int)pwm_id);
  if (DRV_IF_CFG_GetDefConfig(intf_id, &intf_param) != DRV_IF_CFG_OK)
    return DRV_PWM_DAC_ERROR_IF_CFG;

  memcpy(param, &intf_param, sizeof(PWM_DAC_Param));
  return DRV_PWM_DAC_OK;
}

DRV_PWM_DAC_Status DRV_PWM_DAC_ConfigOutputChannel(PWM_DAC_Id pwm_id, PWM_DAC_Param *param) {
  if (pwm_id >= PWM_DAC_ID_MAX) return DRV_PWM_DAC_ERROR_PARAM;

  if (!pwm_peripheral.init_done) return DRV_PWM_DAC_ERROR_INIT;

  if (pwm_peripheral.pwr_state == PWM_POWER_OFF) return DRV_PWM_DAC_ERROR_POWER;

  if (param->clk_div > PWM_MAX_CLK_DIV || param->duty_cycle > PWM_MAX_DUTY_CYCLE)
    return DRV_PWM_DAC_ERROR_PARAM;

  if (param->duty_cycle == PWM_MIN_DUTY_CYCLE)
    DRV_PWM_DAC_Disable(pwm_id, 0);
  else if (param->duty_cycle == PWM_MAX_DUTY_CYCLE)
    DRV_PWM_DAC_Disable(pwm_id, 1);
  else
    DRV_PWM_DAC_Enable(pwm_id, param->clk_div - 1, param->duty_cycle);
  return DRV_PWM_DAC_OK;
}

DRV_PWM_DAC_Status DRV_PWM_DAC_GetChannelStatus(PWM_DAC_Id pwm_id, uint32_t *enable,
                                                PWM_DAC_Param *status) {
  uint32_t op_rate_v;
  if (pwm_id >= PWM_DAC_ID_MAX) return DRV_PWM_DAC_ERROR_PARAM;

  op_rate_v = PWM_DAC_OP_RATE_REG(pwm_id);

  *enable = (op_rate_v & PWM_DAC_OP_RATE_EN_MSK) >> PWM_DAC_OP_RATE_EN_POS;
  status->clk_div = ((op_rate_v & PWM_DAC_OP_RATE_OP_RATE_MSK) >> PWM_DAC_OP_RATE_OP_RATE_POS) + 1;

  if (*enable == 0) {
    if (op_rate_v & PWM_DAC_OP_RATE_DISABLE_VAL_MSK)
      status->duty_cycle = PWM_MAX_DUTY_CYCLE;
    else
      status->duty_cycle = PWM_MIN_DUTY_CYCLE;
  } else {
    status->duty_cycle =
        (PWM_DAC_RATE_REG(pwm_id) & PWM_DAC_RATE_RATE_MSK) >> PWM_DAC_RATE_RATE_POS;
  }
  return DRV_PWM_DAC_OK;
}
