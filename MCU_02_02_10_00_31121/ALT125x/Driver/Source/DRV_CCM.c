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
 * CCM hardware block is only available on ALT1255.
 * DO NOT use this on ALT1250 platform.
 *****************************************************************************/
#include "ALT1255.h"
#include "DRV_CLOCK_GATING.h"
#include "DRV_CCM.h"
#include "DRV_IF_CFG.h"
#if (configUSE_ALT_SLEEP == 1)
#include "DRV_PM.h"
#include "DRV_SLEEP.h"
#endif
#include "string.h"

#define ARM_DRIVER_VERSION_MAJOR_MINOR(major, minor) (((major) << 8) | (minor))
#define ARM_CCM_API_VERSION ARM_DRIVER_VERSION_MAJOR_MINOR(2, 0) /* driver version */
#define ARM_CCM_DRV_VERSION ARM_DRIVER_VERSION_MAJOR_MINOR(1, 0) /* driver version */

#define NSEC_PER_SEC (1000000000L)

#define MAX_CCM_TIMER_NCO_MAX (0xFFFF)
#define MAX_CCM_TIMER_NCO_STEP (0xFFFF)
#define MAX_CCM_COMPARE_REFVAL (0xFFFF)
#define MAX_CCM_TIMER_PRESCALAR (0xFFFF)

#define CCM_INT_TIMER_INT_MSK CC_PWM0_INT_STATUS_RC_CC_PWM_TIMER_INT_Msk
#define CCM_INT_COMPARE_INT0_MSK CC_PWM0_INT_STATUS_RC_CC_PWM_COMPARE_INT0_Msk

extern uint32_t SystemCoreClock;

typedef struct {
  uint16_t api;  ///< API version
  uint16_t drv;  ///< Driver version
} CCM_DriverVersion;

#if (configUSE_ALT_SLEEP == 1)
typedef struct {
  uint32_t timer_conf;
  uint32_t prescalar_max;
  uint32_t nco_step;
  uint32_t nco_max;
  uint32_t cmp_conf;
  uint32_t cmp_refval;
  uint32_t int_mask;
} CCM_Sleep_Ctx;
#endif

typedef struct {
  uint8_t init_done;
  CCM_PowerState pwr_state;
  DRV_CCM_EventCallback cb_event;
  uint32_t mode[CCM_ID_MAX];
  CC_PWM_Type *base_reg[CCM_ID_MAX];
#if (configUSE_ALT_SLEEP == 1)
  CCM_Sleep_Ctx sleep_ctx[CCM_ID_MAX];
  int32_t sleep_notify_inx;
#endif
} CCM_Peripheral;

static CCM_Peripheral ccm_peripheral = {
    .init_done = 0,
    .pwr_state = CCM_POWER_OFF,
    .cb_event = NULL,
    .mode = {CCM_MODE_PWM_ONE_THRESHOLD, CCM_MODE_PWM_ONE_THRESHOLD},
    .base_reg = {CC_PWM0, CC_PWM1}};

void CC_PWM0_IRQHandler(void) {
  uint32_t evt = 0;
  uint32_t int_status = ccm_peripheral.base_reg[0]->INT_STATUS_RC;
  if (ccm_peripheral.cb_event) {
    if (CCM_INT_TIMER_INT_MSK & int_status) evt |= CCM_ID_0_TIMER_EVT;
    if (CCM_INT_COMPARE_INT0_MSK & int_status) evt |= CCM_ID_0_COMPARE_EVT;
    ccm_peripheral.cb_event(evt);
  }
}

void CC_PWM1_IRQHandler(void) {
  uint32_t evt = 0;
  uint32_t int_status = ccm_peripheral.base_reg[1]->INT_STATUS_RC;
  if (ccm_peripheral.cb_event) {
    if (CCM_INT_TIMER_INT_MSK & int_status) evt |= CCM_ID_1_TIMER_EVT;
    if (CCM_INT_COMPARE_INT0_MSK & int_status) evt |= CCM_ID_1_COMPARE_EVT;
    ccm_peripheral.cb_event(evt);
  }
}

/* Driver Version */
static const CCM_DriverVersion DriverVersion = {ARM_CCM_API_VERSION, ARM_CCM_DRV_VERSION};

/* Driver Capabilities */
static const CCM_Capabilities DriverCapabilities = {
    CCM_ID_MAX, /**< Nb of One threshold PWM feature*/
    0,          /**< Nb of Two threshold PWM feature*/
    CCM_ID_MAX, /**< Nb of Generate equal duty cycle clock out feature*/
    CCM_ID_MAX, /**< Nb of Generate narrow pulse feature*/
    0,          /**< Nb of Led output feature*/
    0,          /**< While we want to combine few slots together in cascade so they drive each
                                           other with any mathematical operation (or/and/xor) */
    0,          /**< Nb of Input pin from external driver feature*/
    0,          /**< ???? */
    0           /**< ???? */
};

//
//  Functions
//

uint32_t CCM_GetVersion(void) { return (uint32_t)(DriverVersion.api << 16 | DriverVersion.drv); }
const CCM_Capabilities *CCM_GetCapabilities(void) { return &DriverCapabilities; }

static DRV_CCM_Status DRV_CCM_Apply_Clk_Out(CCM_Id cid, uint32_t max_timer, uint32_t step) {
  DRV_CCM_Status ret = DRV_CCM_OK;
  uint32_t cmp_refval = 0;
  uint8_t int_en = 1;

  if ((max_timer - 1) > MAX_CCM_TIMER_NCO_MAX) {
    ret = DRV_CCM_ERROR_PARAM;
    return ret;
  }

  if (step > MAX_CCM_TIMER_NCO_STEP) {
    ret = DRV_CCM_ERROR_PARAM;
    return ret;
  }

  cmp_refval = (max_timer / 2) - step;

  if (cmp_refval > MAX_CCM_COMPARE_REFVAL) {
    ret = DRV_CCM_ERROR_PARAM;
    return ret;
  }

  ccm_peripheral.base_reg[cid]->TIMER_PRESCALAR_MAX_b.TIMER_PRESCALAR_MAX = 0;

  ccm_peripheral.base_reg[cid]->TIMER_NCO_MAX_b.TIMER_NCO_MAX = max_timer - 1;

  ccm_peripheral.base_reg[cid]->TIMER_NCO_STEP_b.TIMER_NCO_STEP = step;

  ccm_peripheral.base_reg[cid]->TIMER_CONF_b.TIMER_ENABLE = 0;
  ccm_peripheral.base_reg[cid]->TIMER_CONF_b.TIMER_DIRECTION = 0;
  ccm_peripheral.base_reg[cid]->TIMER_CONF_b.TIMER_INTERRUPT_ENABLE = int_en;

  ccm_peripheral.base_reg[cid]->COMPARE_CONF_b.COMP_ENABLE = 0;
  ccm_peripheral.base_reg[cid]->COMPARE_CONF_b.COMP_START_VAL = 0;
  ccm_peripheral.base_reg[cid]->COMPARE_CONF_b.COMP_STOP_VAL = 0;
  ccm_peripheral.base_reg[cid]->COMPARE_CONF_b.COMP_BYPASS = 0;
  ccm_peripheral.base_reg[cid]->COMPARE_CONF_b.COMP_INT0_EN = int_en;

  ccm_peripheral.base_reg[cid]->COMPARE_REFVAL0_b.COMP_REFVAL0 = cmp_refval;
  return ret;
}

static DRV_CCM_Status DRV_CCM_Apply_Pwm_One_Bit_Dac(CCM_Id cid, uint32_t max_timer, uint32_t step) {
  DRV_CCM_Status ret = DRV_CCM_OK;
  uint8_t int_en = 1;

  if ((max_timer - 1) > MAX_CCM_TIMER_NCO_MAX) {
    ret = DRV_CCM_ERROR_PARAM;
    return ret;
  }

  if (step > MAX_CCM_TIMER_NCO_STEP) {
    ret = DRV_CCM_ERROR_PARAM;
    return ret;
  }

  ccm_peripheral.base_reg[cid]->TIMER_PRESCALAR_MAX_b.TIMER_PRESCALAR_MAX = 0;

  ccm_peripheral.base_reg[cid]->TIMER_NCO_MAX_b.TIMER_NCO_MAX = max_timer - 1;

  ccm_peripheral.base_reg[cid]->TIMER_NCO_STEP_b.TIMER_NCO_STEP = step;

  ccm_peripheral.base_reg[cid]->TIMER_CONF_b.TIMER_ENABLE = 0;
  ccm_peripheral.base_reg[cid]->TIMER_CONF_b.TIMER_DIRECTION = 0;
  ccm_peripheral.base_reg[cid]->TIMER_CONF_b.TIMER_INTERRUPT_ENABLE = int_en;

  ccm_peripheral.base_reg[cid]->COMPARE_CONF_b.COMP_ENABLE = 0;
  ccm_peripheral.base_reg[cid]->COMPARE_CONF_b.COMP_START_VAL = 1;
  ccm_peripheral.base_reg[cid]->COMPARE_CONF_b.COMP_STOP_VAL = 0;
  ccm_peripheral.base_reg[cid]->COMPARE_CONF_b.COMP_BYPASS = 0;
  ccm_peripheral.base_reg[cid]->COMPARE_CONF_b.COMP_INT0_EN = int_en;

  ccm_peripheral.base_reg[cid]->COMPARE_REFVAL0_b.COMP_REFVAL0 = 0;
  return ret;
}

static DRV_CCM_Status DRV_CCM_Apply_Pwm_One_Threshold(CCM_Id cid, uint32_t period_ns,
                                                      uint32_t duty_ns, uint32_t polarity) {
  uint32_t timer_precision, clk_precision;
  uint32_t fclk = SystemCoreClock;
  uint32_t timer_nco_max;
  uint32_t timer_prescalar_max = 0;
  uint32_t cmp_refval = 0;
  DRV_CCM_Status ret = DRV_CCM_OK;
  uint8_t int_en = 1;
  uint8_t start_val = (polarity != 0);

  clk_precision = NSEC_PER_SEC / fclk;
  timer_precision = clk_precision;
  timer_nco_max = (period_ns / timer_precision) - 1;

  while (timer_nco_max > MAX_CCM_TIMER_NCO_MAX) {
    timer_prescalar_max++;
    if (timer_prescalar_max > MAX_CCM_TIMER_PRESCALAR) {
      ret = DRV_CCM_ERROR_PARAM;
      goto pwm_config_err;
    }
    timer_precision = clk_precision * (timer_prescalar_max + 1);
    timer_nco_max = (period_ns / timer_precision) - 1;
  }
  cmp_refval = (duty_ns / timer_precision) - 1;

  if (cmp_refval == 0 || cmp_refval > MAX_CCM_COMPARE_REFVAL) {
    ret = DRV_CCM_ERROR_PARAM;
    goto pwm_config_err;
  }

  ccm_peripheral.base_reg[cid]->TIMER_PRESCALAR_MAX_b.TIMER_PRESCALAR_MAX = timer_prescalar_max;

  ccm_peripheral.base_reg[cid]->TIMER_NCO_MAX_b.TIMER_NCO_MAX = timer_nco_max;

  ccm_peripheral.base_reg[cid]->TIMER_NCO_STEP_b.TIMER_NCO_STEP = 1;

  ccm_peripheral.base_reg[cid]->TIMER_CONF_b.TIMER_ENABLE = 0;
  ccm_peripheral.base_reg[cid]->TIMER_CONF_b.TIMER_DIRECTION = 0;
  ccm_peripheral.base_reg[cid]->TIMER_CONF_b.TIMER_INTERRUPT_ENABLE = int_en;

  ccm_peripheral.base_reg[cid]->COMPARE_CONF_b.COMP_ENABLE = 0;
  ccm_peripheral.base_reg[cid]->COMPARE_CONF_b.COMP_START_VAL = start_val;
  ccm_peripheral.base_reg[cid]->COMPARE_CONF_b.COMP_STOP_VAL = 0;
  ccm_peripheral.base_reg[cid]->COMPARE_CONF_b.COMP_BYPASS = 0;
  ccm_peripheral.base_reg[cid]->COMPARE_CONF_b.COMP_INT0_EN = int_en;

  ccm_peripheral.base_reg[cid]->COMPARE_REFVAL0_b.COMP_REFVAL0 = cmp_refval;

pwm_config_err:
  return ret;
}

static uint8_t DRV_CCM_Channel_Is_Enabled(CCM_Id cid) {
  return (uint8_t)(ccm_peripheral.base_reg[cid]->TIMER_CONF_b.TIMER_ENABLE != 0);
}

static void DRV_CCM_Disable_Channel(CCM_Id cid) {
  ccm_peripheral.base_reg[cid]->INT_MASK_b.CC_PWM_TIMER_INT_MASK = 1;
  ccm_peripheral.base_reg[cid]->INT_MASK_b.CC_PWM_COMPARE_INT0_MASK = 1;

  ccm_peripheral.base_reg[cid]->COMPARE_CONF_b.COMP_ENABLE = 0;

  ccm_peripheral.base_reg[cid]->TIMER_CONF_b.TIMER_ENABLE = 0;

  ccm_peripheral.base_reg[cid]->CONF_DONE_b.CC_PWM_FORCE_STOP = 1;
}

static void DRV_CCM_Enable_Channel(CCM_Id cid) {
  ccm_peripheral.base_reg[cid]->TIMER_CONF_b.TIMER_ENABLE = 1;
  ccm_peripheral.base_reg[cid]->COMPARE_CONF_b.COMP_ENABLE = 1;

  if (ccm_peripheral.cb_event) {
    ccm_peripheral.base_reg[cid]->INT_MASK_b.CC_PWM_TIMER_INT_MASK = 0;
    ccm_peripheral.base_reg[cid]->INT_MASK_b.CC_PWM_COMPARE_INT0_MASK = 0;
  } else {
    ccm_peripheral.base_reg[cid]->INT_MASK_b.CC_PWM_TIMER_INT_MASK = 1;
    ccm_peripheral.base_reg[cid]->INT_MASK_b.CC_PWM_COMPARE_INT0_MASK = 1;
  }

  ccm_peripheral.base_reg[cid]->CONF_DONE_b.CC_PWM_CONF_DONE = 1;
}

#if (configUSE_ALT_SLEEP == 1)
static int32_t DRV_CCM_SleepHandler(DRV_SLEEP_NotifyType state, DRV_PM_PwrMode pwr_mode,
                                    void *arg) {
  CCM_Id cid;

  if (pwr_mode == DRV_PM_MODE_STANDBY || pwr_mode == DRV_PM_MODE_SHUTDOWN) {
    if (state == DRV_SLEEP_NTFY_SUSPENDING) {
      for (cid = CCM_ID_0; cid < CCM_ID_MAX; cid++) {
        ccm_peripheral.sleep_ctx[cid].timer_conf = ccm_peripheral.base_reg[cid]->TIMER_CONF;
        ccm_peripheral.sleep_ctx[cid].prescalar_max =
            ccm_peripheral.base_reg[cid]->TIMER_PRESCALAR_MAX;
        ccm_peripheral.sleep_ctx[cid].nco_step = ccm_peripheral.base_reg[cid]->TIMER_NCO_STEP;
        ccm_peripheral.sleep_ctx[cid].nco_max = ccm_peripheral.base_reg[cid]->TIMER_NCO_MAX;
        ccm_peripheral.sleep_ctx[cid].cmp_conf = ccm_peripheral.base_reg[cid]->COMPARE_CONF;
        ccm_peripheral.sleep_ctx[cid].cmp_refval = ccm_peripheral.base_reg[cid]->COMPARE_REFVAL0;
        ccm_peripheral.sleep_ctx[cid].int_mask = ccm_peripheral.base_reg[cid]->INT_MASK;
      }
    } else if (state == DRV_SLEEP_NTFY_RESUMING) {
      for (cid = CCM_ID_0; cid < CCM_ID_MAX; cid++) {
        ccm_peripheral.base_reg[cid]->TIMER_CONF = ccm_peripheral.sleep_ctx[cid].timer_conf;
        ccm_peripheral.base_reg[cid]->TIMER_PRESCALAR_MAX =
            ccm_peripheral.sleep_ctx[cid].prescalar_max;
        ccm_peripheral.base_reg[cid]->TIMER_NCO_STEP = ccm_peripheral.sleep_ctx[cid].nco_step;
        ccm_peripheral.base_reg[cid]->TIMER_NCO_MAX = ccm_peripheral.sleep_ctx[cid].nco_max;
        ccm_peripheral.base_reg[cid]->COMPARE_CONF = ccm_peripheral.sleep_ctx[cid].cmp_conf;
        ccm_peripheral.base_reg[cid]->COMPARE_REFVAL0 = ccm_peripheral.sleep_ctx[cid].cmp_refval;
        ccm_peripheral.base_reg[cid]->INT_MASK = ccm_peripheral.sleep_ctx[cid].int_mask;

        if (ccm_peripheral.base_reg[cid]->TIMER_CONF_b.TIMER_ENABLE)
          ccm_peripheral.base_reg[cid]->CONF_DONE_b.CC_PWM_CONF_DONE = 1;
      }
    }
  }
  return 0;
}
#endif

DRV_CCM_Status DRV_CCM_Initialize(DRV_CCM_EventCallback cb_event) {
  DRV_IF_CFG_Status ifcfg_ret;

  if (ccm_peripheral.init_done) return DRV_CCM_ERROR_INIT;

#if (configUSE_ALT_SLEEP == 1)
  if (DRV_SLEEP_RegNotification(DRV_CCM_SleepHandler, &ccm_peripheral.sleep_notify_inx, NULL) != 0)
    return DRV_CCM_ERROR_GENERIC;
#endif

  NVIC_SetPriority(CC_PWM0_IRQn, 7);
  NVIC_SetPriority(CC_PWM1_IRQn, 7);
  ccm_peripheral.cb_event = cb_event;
  memset(ccm_peripheral.mode, 0x0, CCM_ID_MAX * sizeof(uint32_t));
  /*Configure PIN mux. Do not return error if user choose to left configuration in interface header
   * undefined. User can use DRV_IO_SetMux to configure mux by itself*/
  ifcfg_ret = DRV_IF_CFG_SetIO(IF_CFG_PWM0);
  if (ifcfg_ret != DRV_IF_CFG_OK && ifcfg_ret != DRV_IF_CFG_ERROR_UNDEFINED)
    return DRV_CCM_ERROR_IF_CFG;
  /*Configure PIN mux. Do not return error if user choose to left configuration in interface header
   * undefined. User can use DRV_IO_SetMux to configure mux by itself*/
  ifcfg_ret = DRV_IF_CFG_SetIO(IF_CFG_PWM1);
  if (ifcfg_ret != DRV_IF_CFG_OK && ifcfg_ret != DRV_IF_CFG_ERROR_UNDEFINED)
    return DRV_CCM_ERROR_IF_CFG;
  ccm_peripheral.init_done = 1;
  return DRV_CCM_OK;
}

DRV_CCM_Status DRV_CCM_Uninitialize(void) {
  if (!ccm_peripheral.init_done) return DRV_CCM_ERROR_INIT;

#if (configUSE_ALT_SLEEP == 1)
  if (DRV_SLEEP_UnRegNotification(ccm_peripheral.sleep_notify_inx) != 0)
    return DRV_CCM_ERROR_GENERIC;
#endif

  CCM_Id cid;
  for (cid = CCM_ID_0; cid < CCM_ID_MAX; cid++) {
    if (DRV_CCM_Channel_Is_Enabled(cid)) {
      DRV_CCM_Disable_Channel(cid);
    }
  }
  ccm_peripheral.cb_event = NULL;
  memset(ccm_peripheral.mode, 0x0, CCM_ID_MAX * sizeof(uint32_t));

  ccm_peripheral.init_done = 0;
  return DRV_CCM_OK;
}

DRV_CCM_Status DRV_CCM_PowerControl(CCM_PowerState state) {
  CCM_Id cid;

  if (!ccm_peripheral.init_done) return DRV_CCM_ERROR_INIT;

  switch (state) {
    case CCM_POWER_OFF:
      for (cid = CCM_ID_0; cid < CCM_ID_MAX; cid++) {
        if (DRV_CCM_Channel_Is_Enabled(cid)) {
          DRV_CCM_Disable_Channel(cid);
        }
      }
      NVIC_DisableIRQ(CC_PWM0_IRQn);
      NVIC_DisableIRQ(CC_PWM1_IRQn);
      DRV_CLKGATING_DisableClkSource(CLK_GATING_PWM);
      break;

    case CCM_POWER_FULL:
      DRV_CLKGATING_EnableClkSource(CLK_GATING_PWM);
      NVIC_EnableIRQ(CC_PWM0_IRQn);
      NVIC_EnableIRQ(CC_PWM1_IRQn);
      break;

    case CCM_POWER_LOW:
    default:
      return DRV_CCM_ERROR_UNSUPPORTED;
      break;
  }
  ccm_peripheral.pwr_state = state;
  return DRV_CCM_OK;
}

DRV_CCM_Status DRV_CCM_Control(uint32_t control, uint32_t arg) {
  CCM_Id cid;

  if (!ccm_peripheral.init_done) return DRV_CCM_ERROR_INIT;

  if (ccm_peripheral.pwr_state == CCM_POWER_OFF) return DRV_CCM_ERROR_POWER;

  switch (control) {
    case CCM_MODE_PWM_ONE_THRESHOLD:
    case CCM_MODE_1BIT_DAC:
    case CCM_MODE_CLK_OUT:
      cid = (CCM_Id)arg;

      if (cid >= CCM_ID_MAX) return DRV_CCM_ERROR_PARAM;

      if (ccm_peripheral.mode[cid] != control && DRV_CCM_Channel_Is_Enabled(cid))
        DRV_CCM_Disable_Channel(cid);

      ccm_peripheral.mode[cid] = control;

      break;

    default:
      return DRV_CCM_ERROR_UNSUPPORTED;
      break;
  }
  return DRV_CCM_OK;
}

DRV_CCM_Status DRV_CCM_ConfigOutputChannel(CCM_Id ccm_id, uint32_t param1, uint32_t param2,
                                           uint32_t param3) {
  uint8_t need_reenable = 0;
  uint32_t current_mode;
  int32_t ret = DRV_CCM_OK;

  if (!ccm_peripheral.init_done) return DRV_CCM_ERROR_INIT;

  if (ccm_peripheral.pwr_state == CCM_POWER_OFF) return DRV_CCM_ERROR_POWER;

  need_reenable = DRV_CCM_Channel_Is_Enabled(ccm_id);

  if (need_reenable) DRV_CCM_Disable_Channel(ccm_id);

  current_mode = ccm_peripheral.mode[ccm_id];
  switch (current_mode) {
    case CCM_MODE_PWM_ONE_THRESHOLD:
      ret = DRV_CCM_Apply_Pwm_One_Threshold(ccm_id, param1, param2, param3);
      break;
    case CCM_MODE_1BIT_DAC:
      ret = DRV_CCM_Apply_Pwm_One_Bit_Dac(ccm_id, param1, param2);
      break;
    case CCM_MODE_CLK_OUT:
      ret = DRV_CCM_Apply_Clk_Out(ccm_id, param1, param2);
      break;
    default:
      ret = DRV_CCM_ERROR_GENERIC;
      break;
  }

  if (need_reenable) DRV_CCM_Enable_Channel(ccm_id);

  return ret;
}

void DRV_CCM_EnableDisableOutputChannels(uint32_t channel_mask) {
  CCM_Id cid;
  uint8_t en;

  for (cid = CCM_ID_0; cid < CCM_ID_MAX; cid++) {
    en = (channel_mask >> cid) & 1;
    if (en != DRV_CCM_Channel_Is_Enabled(cid)) {
      if (en)
        DRV_CCM_Enable_Channel(cid);
      else
        DRV_CCM_Disable_Channel(cid);
    }
  }
}

DRV_CCM_Status DRV_CCM_GetChannelStatus(CCM_Id ccm_id, CCM_ChannelStatus *status) {
  uint32_t timer_precision = 0;
  uint32_t fclk = SystemCoreClock;

  if (!ccm_peripheral.init_done) return DRV_CCM_ERROR_INIT;

  if (!status || (ccm_id >= CCM_ID_MAX)) return DRV_CCM_ERROR_PARAM;

  status->enable = DRV_CCM_Channel_Is_Enabled(ccm_id);
  status->mode = ccm_peripheral.mode[ccm_id];
  status->param1 = 0;
  status->param2 = 0;
  status->param3 = 0;
  switch (status->mode) {
    case CCM_MODE_PWM_ONE_THRESHOLD:
      timer_precision =
          (NSEC_PER_SEC / fclk) *
          (ccm_peripheral.base_reg[ccm_id]->TIMER_PRESCALAR_MAX_b.TIMER_PRESCALAR_MAX + 1);
      status->param1 =
          (ccm_peripheral.base_reg[ccm_id]->TIMER_NCO_MAX_b.TIMER_NCO_MAX + 1) * timer_precision;
      status->param2 =
          (ccm_peripheral.base_reg[ccm_id]->COMPARE_REFVAL0_b.COMP_REFVAL0 + 1) * timer_precision;
      status->param3 = ccm_peripheral.base_reg[ccm_id]->COMPARE_CONF_b.COMP_START_VAL;
      break;
    case CCM_MODE_1BIT_DAC:
    case CCM_MODE_CLK_OUT:
      status->param1 = ccm_peripheral.base_reg[ccm_id]->TIMER_NCO_MAX_b.TIMER_NCO_MAX + 1;
      status->param2 = ccm_peripheral.base_reg[ccm_id]->TIMER_NCO_STEP_b.TIMER_NCO_STEP;
      break;
    default:
      break;
  }
  return DRV_CCM_OK;
}
