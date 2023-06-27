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
#include "clk_gating.h"
#include "ccm.h"
#include "if_mngr.h"
#include "string.h"

#define ARM_DRIVER_VERSION_MAJOR_MINOR(major, minor) (((major) << 8) | (minor))
#define ARM_CCM_API_VERSION ARM_DRIVER_VERSION_MAJOR_MINOR(1, 0) /* driver version */
#define ARM_CCM_DRV_VERSION ARM_DRIVER_VERSION_MAJOR_MINOR(1, 0) /* driver version */

#define NSEC_PER_SEC (1000000000L)

#define MAX_CCM_TIMER_NCO_MAX (0xFFFF)
#define MAX_CCM_TIMER_NCO_STEP (0xFFFF)
#define MAX_CCM_COMPARE_REFVAL (0xFFFF)
#define MAX_CCM_TIMER_PRESCALAR (0xFFFF)

#define CCM_SLICE_BASE_OFFSET (0x10000)
#define BASE_ADDRESS_CCM BASE_ADDRESS_MCU_CC_PWM0

#define CCM_TIMER_PRESCALAR_MAX_MSK MCU_CC_PWM0_CC_PWM_TIMER_PRESCALAR_MAX_TIMER_PRESCALAR_MAX_MSK
#define CCM_TIMER_PRESCALAR_MAX_POS MCU_CC_PWM0_CC_PWM_TIMER_PRESCALAR_MAX_TIMER_PRESCALAR_MAX_POS
#define CCM_TIMER_NCO_MAX_MSK MCU_CC_PWM0_CC_PWM_TIMER_NCO_MAX_TIMER_NCO_MAX_MSK
#define CCM_TIMER_NCO_MAX_POS MCU_CC_PWM0_CC_PWM_TIMER_NCO_MAX_TIMER_NCO_MAX_POS
#define CCM_TIMER_NCO_STEP_MSK MCU_CC_PWM0_CC_PWM_TIMER_NCO_STEP_TIMER_NCO_STEP_MSK
#define CCM_TIMER_NCO_STEP_POS MCU_CC_PWM0_CC_PWM_TIMER_NCO_STEP_TIMER_NCO_STEP_POS
#define CCM_CONF_DONE_MSK MCU_CC_PWM0_CC_PWM0_CONF_DONE_CC_PWM_CONF_DONE_MSK
#define CCM_CONF_DONE_POS MCU_CC_PWM0_CC_PWM0_CONF_DONE_CC_PWM_CONF_DONE_POS
#define CCM_CONF_FORCE_STOP_MSK MCU_CC_PWM0_CC_PWM0_CONF_DONE_CC_PWM_FORCE_STOP_MSK
#define CCM_CONF_FORCE_STOP_POS MCU_CC_PWM0_CC_PWM0_CONF_DONE_CC_PWM_FORCE_STOP_POS
#define CCM_TIMER_CONF_DIRECTION_MSK MCU_CC_PWM0_CC_PWM_TIMER_CONF_TIMER_DIRECTION_MSK
#define CCM_TIMER_CONF_DIRECTION_POS MCU_CC_PWM0_CC_PWM_TIMER_CONF_TIMER_DIRECTION_POS
#define CCM_TIMER_CONF_INT_ENABLE_MSK MCU_CC_PWM0_CC_PWM_TIMER_CONF_TIMER_INTERRUPT_ENABLE_MSK
#define CCM_TIMER_CONF_INT_ENABLE_POS MCU_CC_PWM0_CC_PWM_TIMER_CONF_TIMER_INTERRUPT_ENABLE_POS
#define CCM_TIMER_CONF_ENABLE_MSK MCU_CC_PWM0_CC_PWM_TIMER_CONF_TIMER_ENABLE_MSK
#define CCM_TIMER_CONF_ENABLE_POS MCU_CC_PWM0_CC_PWM_TIMER_CONF_TIMER_ENABLE_POS
#define CCM_TIMER_CONF_HW_HALT_MASK_MSK MCU_CC_PWM0_CC_PWM_TIMER_CONF_TIMER_HW_HALT_MASK_MSK
#define CCM_TIMER_CONF_HW_HALT_MASK_POS MCU_CC_PWM0_CC_PWM_TIMER_CONF_TIMER_HW_HALT_MASK_POS
#define CCM_COMPARE_CONF_ENABLE_MSK MCU_CC_PWM0_CC_PWM_COMPARE_CONF_COMP_ENABLE_MSK
#define CCM_COMPARE_CONF_ENABLE_POS MCU_CC_PWM0_CC_PWM_COMPARE_CONF_COMP_ENABLE_POS
#define CCM_COMPARE_CONF_START_VAL_MSK MCU_CC_PWM0_CC_PWM_COMPARE_CONF_COMP_START_VAL_MSK
#define CCM_COMPARE_CONF_START_VAL_POS MCU_CC_PWM0_CC_PWM_COMPARE_CONF_COMP_START_VAL_POS
#define CCM_COMPARE_CONF_STOP_VAL_MSK MCU_CC_PWM0_CC_PWM_COMPARE_CONF_COMP_STOP_VAL_MSK
#define CCM_COMPARE_CONF_STOP_VAL_POS MCU_CC_PWM0_CC_PWM_COMPARE_CONF_COMP_STOP_VAL_POS
#define CCM_COMPARE_CONF_BYPASS_MSK MCU_CC_PWM0_CC_PWM_COMPARE_CONF_COMP_BYPASS_MSK
#define CCM_COMPARE_CONF_BYPASS_POS MCU_CC_PWM0_CC_PWM_COMPARE_CONF_COMP_BYPASS_POS
#define CCM_COMPARE_CONF_INT0_EN_MSK MCU_CC_PWM0_CC_PWM_COMPARE_CONF_COMP_INT0_EN_MSK
#define CCM_COMPARE_CONF_INT0_EN_POS MCU_CC_PWM0_CC_PWM_COMPARE_CONF_COMP_INT0_EN_POS
#define CCM_COMPARE_REFVAL0_MSK MCU_CC_PWM0_CC_PWM_COMPARE_REFVAL0_COMP_REFVAL0_MSK
#define CCM_COMPARE_REFVAL0_POS MCU_CC_PWM0_CC_PWM_COMPARE_REFVAL0_COMP_REFVAL0_POS

#define CCM_INT_TIMER_INT_MSK MCU_CC_PWM0_CC_PWM_INT_STATUS_CC_PWM_TIMER_INT_MSK
#define CCM_INT_TIMER_INT_POS MCU_CC_PWM0_CC_PWM_INT_STATUS_CC_PWM_TIMER_INT_POS
#define CCM_INT_COMPARE_INT0_MSK MCU_CC_PWM0_CC_PWM0_INT_STATUS_RC_CC_PWM_COMPARE_INT0_MSK
#define CCM_INT_COMPARE_INT0_POS MCU_CC_PWM0_CC_PWM0_INT_STATUS_RC_CC_PWM_COMPARE_INT0_POS
#define CCM_INT_MASK_TIMER_INT_MSK MCU_CC_PWM0_CC_PWM_INT_MASK_CC_PWM_TIMER_INT_MASK_MSK
#define CCM_INT_MASK_TIMER_INT_POS MCU_CC_PWM0_CC_PWM_INT_MASK_CC_PWM_TIMER_INT_MASK_POS
#define CCM_INT_MASK_COMPARE_INT_MSK MCU_CC_PWM0_CC_PWM_INT_MASK_CC_PWM_COMPARE_INT0_MASK_MSK
#define CCM_INT_MASK_COMPARE_INT_POS MCU_CC_PWM0_CC_PWM_INT_MASK_CC_PWM_COMPARE_INT0_MASK_POS
#define CCM_CHANNEL_BASE(cid) (BASE_ADDRESS_CCM + (CCM_SLICE_BASE_OFFSET * cid))

#define CCM_CONF_DONE_REG_ADDR(id) (CCM_CHANNEL_BASE(id) + 0x00)
#define CCM_CONF_DONE_REG(id) (REGISTER(CCM_CONF_DONE_REG_ADDR(id)))

#define CCM_TIMER_CONF_REG_ADDR(id) (CCM_CHANNEL_BASE(id) + 0x10)
#define CCM_TIMER_CONF_REG(id) (REGISTER(CCM_TIMER_CONF_REG_ADDR(id)))

#define CCM_TIMER_PRESCALAR_REG_ADDR(id) (CCM_CHANNEL_BASE(id) + 0x14)
#define CCM_TIMER_PRESCALAR_REG(id) (REGISTER(CCM_TIMER_PRESCALAR_REG_ADDR(id)))

#define CCM_TIMER_NCO_STEP_REG_ADDR(id) (CCM_CHANNEL_BASE(id) + 0x18)
#define CCM_TIMER_NCO_STEP_REG(id) (REGISTER(CCM_TIMER_NCO_STEP_REG_ADDR(id)))

#define CCM_TIMER_NCO_MAX_REG_ADDR(id) (CCM_CHANNEL_BASE(id) + 0x1C)
#define CCM_TIMER_NCO_MAX_REG(id) (REGISTER(CCM_TIMER_NCO_MAX_REG_ADDR(id)))

#define CCM_TIMER_VAL_REG_ADDR(id) (CCM_CHANNEL_BASE(id) + 0x20)
#define CCM_TIMER_VAL_REG(id) (REGISTER(CCM_TIMER_VAL_REG_ADDR(id)))

#define CCM_COMPARE_CONF_REG_ADDR(id) (CCM_CHANNEL_BASE(id) + 0x24)
#define CCM_COMPARE_CONF_REG(id) (REGISTER(CCM_COMPARE_CONF_REG_ADDR(id)))

#define CCM_INT_MASK_REG_ADDR(id) (CCM_CHANNEL_BASE(id) + 0x2C)
#define CCM_INT_MASK_REG(id) (REGISTER(CCM_INT_MASK_REG_ADDR(id)))

#define CCM_COMPARE_CONF_REFVAL0_REG_ADDR(id) (CCM_CHANNEL_BASE(id) + 0x28)
#define CCM_COMPARE_CONF_REFVAL0_REG(id) (REGISTER(CCM_COMPARE_CONF_REFVAL0_REG_ADDR(id)))

#define CCM_INT_STATUS_REG_ADDR(id) (CCM_CHANNEL_BASE(id) + 0x30)
#define CCM_INT_STATUS_REG(id) (REGISTER(CCM_INT_STATUS_REG_ADDR(id)))

#define CCM_INT_STATUS_RC_REG_ADDR(id) (CCM_CHANNEL_BASE(id) + 0x34)
#define CCM_INT_STATUS_RC_REG(id) (REGISTER(CCM_INT_STATUS_RC_REG_ADDR(id)))


extern uint32_t SystemCoreClock;
/**
\brief Driver Version
*/
typedef struct _CCM_DRIVER_VERSION {
  uint16_t api;  ///< API version
  uint16_t drv;  ///< Driver version
} CCM_DRIVER_VERSION;

typedef struct _CCM_DEVICE_STATE {
  CCM_POWER_STATE pwr_state;
  CCM_SignalEvent_t cb_event;
  uint32_t mode[CCM_NUM];
} CCM_DEV_STATE;

static CCM_DEV_STATE gCCMDev;

void CC_PWM0_IRQHandler(void) {
  uint32_t evt = 0;
  uint32_t int_status = CCM_INT_STATUS_RC_REG(0);
  if (gCCMDev.cb_event) {
    if (CCM_INT_TIMER_INT_MSK & int_status) evt |= CCM_ID_0_TIMER_EVT;
    if (CCM_INT_COMPARE_INT0_MSK & int_status) evt |= CCM_ID_0_COMPARE_EVT;
    gCCMDev.cb_event(evt);
  }
}

void CC_PWM1_IRQHandler(void) {
  uint32_t evt = 0;
  uint32_t int_status = CCM_INT_STATUS_RC_REG(1);
  if (gCCMDev.cb_event) {
    if (CCM_INT_TIMER_INT_MSK & int_status) evt |= CCM_ID_1_TIMER_EVT;
    if (CCM_INT_COMPARE_INT0_MSK & int_status) evt |= CCM_ID_1_COMPARE_EVT;
    gCCMDev.cb_event(evt);
  }
}

/* Driver Version */
static const CCM_DRIVER_VERSION DriverVersion = {ARM_CCM_API_VERSION, ARM_CCM_DRV_VERSION};

/* Driver Capabilities */
static const CCM_CAPABILITIES DriverCapabilities = {
    CCM_NUM, /**< Nb of One threshold PWM feature*/
    0,       /**< Nb of Two threshold PWM feature*/
    CCM_NUM, /**< Nb of Generate equal duty cycle clock out feature*/
    CCM_NUM, /**< Nb of Generate narrow pulse feature*/
    0,       /**< Nb of Led output feature*/
    0,       /**< While we want to combine few slots together in cascade so they drive each
                                        other with any mathematical operation (or/and/xor) */
    0,       /**< Nb of Input pin from external driver feature*/
    0,       /**< ???? */
    0        /**< ???? */
};

//
//  Functions
//

uint32_t CCM_GetVersion(void) { return (uint32_t)(DriverVersion.api << 16 | DriverVersion.drv); }
const CCM_CAPABILITIES *CCM_GetCapabilities(void) { return &DriverCapabilities; }

static int32_t _CCM_Apply_Clk_Out(CCM_ID cid, uint32_t max_timer, uint32_t step) {
  int32_t ret = CCM_DRIVER_OK;
  uint32_t cmp_refval = 0;
  uint8_t int_en = 1;

  if ((max_timer - 1) > MAX_CCM_TIMER_NCO_MAX) {
    ret = CCM_DRIVER_ERROR_PARAMETER;
    return ret;
  }

  if (step > MAX_CCM_TIMER_NCO_STEP) {
    ret = CCM_DRIVER_ERROR_PARAMETER;
    return ret;
  }

  cmp_refval = (max_timer / 2) - step;

  if (cmp_refval > MAX_CCM_COMPARE_REFVAL) {
    ret = CCM_DRIVER_ERROR_PARAMETER;
    return ret;
  }

  CCM_TIMER_PRESCALAR_REG(cid) = 0;

  CCM_TIMER_NCO_MAX_REG(cid) = ((max_timer - 1) << CCM_TIMER_NCO_MAX_POS) & CCM_TIMER_NCO_STEP_MSK;

  CCM_TIMER_NCO_STEP_REG(cid) = (step << CCM_TIMER_NCO_STEP_POS) & CCM_TIMER_NCO_STEP_MSK;

  CCM_TIMER_CONF_REG(cid) = (0 << CCM_TIMER_CONF_ENABLE_POS) | (0 << CCM_TIMER_CONF_DIRECTION_POS) |
                            (int_en << CCM_TIMER_CONF_INT_ENABLE_POS) |
                            (1 << CCM_TIMER_CONF_HW_HALT_MASK_POS);

  CCM_COMPARE_CONF_REG(cid) =
      (0 << CCM_COMPARE_CONF_ENABLE_POS) | (0 << CCM_COMPARE_CONF_START_VAL_POS) |
      (0 << CCM_COMPARE_CONF_STOP_VAL_POS) | (0 << CCM_COMPARE_CONF_BYPASS_POS) |
      (int_en << CCM_COMPARE_CONF_INT0_EN_POS);

  CCM_COMPARE_CONF_REFVAL0_REG(cid) =
      (cmp_refval << CCM_COMPARE_REFVAL0_POS) & CCM_COMPARE_REFVAL0_MSK;
  return ret;
}

static int32_t _CCM_Apply_Pwm_One_Bit_Dac(CCM_ID cid, uint32_t max_timer, uint32_t step) {
  int32_t ret = CCM_DRIVER_OK;
  uint8_t int_en = 1;

  if ((max_timer - 1) > MAX_CCM_TIMER_NCO_MAX) {
    ret = CCM_DRIVER_ERROR_PARAMETER;
    return ret;
  }

  if (step > MAX_CCM_TIMER_NCO_STEP) {
    ret = CCM_DRIVER_ERROR_PARAMETER;
    return ret;
  }

  CCM_TIMER_PRESCALAR_REG(cid) = 0;

  CCM_TIMER_NCO_MAX_REG(cid) = ((max_timer - 1) << CCM_TIMER_NCO_MAX_POS) & CCM_TIMER_NCO_MAX_MSK;

  CCM_TIMER_NCO_STEP_REG(cid) = (step << CCM_TIMER_NCO_STEP_POS) & CCM_TIMER_NCO_STEP_MSK;

  CCM_TIMER_CONF_REG(cid) = (0 << CCM_TIMER_CONF_ENABLE_POS) | (0 << CCM_TIMER_CONF_DIRECTION_POS) |
                            (int_en << CCM_TIMER_CONF_INT_ENABLE_POS) |
                            (1 << CCM_TIMER_CONF_HW_HALT_MASK_POS);

  CCM_COMPARE_CONF_REG(cid) =
      (0 << CCM_COMPARE_CONF_ENABLE_POS) | (1 << CCM_COMPARE_CONF_START_VAL_POS) |
      (0 << CCM_COMPARE_CONF_STOP_VAL_POS) | (0 << CCM_COMPARE_CONF_BYPASS_POS) |
      (int_en << CCM_COMPARE_CONF_INT0_EN_POS);

  CCM_COMPARE_CONF_REFVAL0_REG(cid) = 0;
  return ret;
}

static int32_t _CCM_Apply_Pwm_One_Threshold(CCM_ID cid, uint32_t period_ns, uint32_t duty_ns,
                                            uint32_t polarity) {
  uint32_t timer_precision, clk_precision;
  uint32_t fclk = SystemCoreClock;
  uint32_t timer_nco_max;
  uint32_t timer_prescalar_max = 0;
  uint32_t cmp_refval = 0;
  int32_t ret = CCM_DRIVER_OK;
  uint8_t int_en = 1;
  uint8_t start_val = (polarity != 0);

  clk_precision = NSEC_PER_SEC / fclk;
  timer_precision = clk_precision;
  timer_nco_max = (period_ns / timer_precision) - 1;

  while (timer_nco_max > MAX_CCM_TIMER_NCO_MAX) {
    timer_prescalar_max++;
    if (timer_prescalar_max > MAX_CCM_TIMER_PRESCALAR) {
      ret = CCM_DRIVER_ERROR_PARAMETER;
      goto pwm_config_err;
    }
    timer_precision = clk_precision * (timer_prescalar_max + 1);
    timer_nco_max = (period_ns / timer_precision) - 1;
  }
  cmp_refval = (duty_ns / timer_precision) - 1;

  if (cmp_refval == 0 || cmp_refval > MAX_CCM_COMPARE_REFVAL) {
    ret = CCM_DRIVER_ERROR_PARAMETER;
    goto pwm_config_err;
  }

  CCM_TIMER_PRESCALAR_REG(cid) = timer_prescalar_max;

  CCM_TIMER_NCO_MAX_REG(cid) = (timer_nco_max << CCM_TIMER_NCO_MAX_POS) & CCM_TIMER_NCO_MAX_MSK;

  CCM_TIMER_NCO_STEP_REG(cid) = (1 << CCM_TIMER_NCO_STEP_POS) & CCM_TIMER_NCO_STEP_MSK;

  CCM_TIMER_CONF_REG(cid) = (0 << CCM_TIMER_CONF_ENABLE_POS) | (0 << CCM_TIMER_CONF_DIRECTION_POS) |
                            (int_en << CCM_TIMER_CONF_INT_ENABLE_POS) |
                            (1 << CCM_TIMER_CONF_HW_HALT_MASK_POS);

  CCM_COMPARE_CONF_REG(cid) =
      (0 << CCM_COMPARE_CONF_ENABLE_POS) | (start_val << CCM_COMPARE_CONF_START_VAL_POS) |
      (0 << CCM_COMPARE_CONF_STOP_VAL_POS) | (0 << CCM_COMPARE_CONF_BYPASS_POS) |
      (int_en << CCM_COMPARE_CONF_INT0_EN_POS);

  CCM_COMPARE_CONF_REFVAL0_REG(cid) =
      (cmp_refval << CCM_COMPARE_REFVAL0_POS) & CCM_COMPARE_REFVAL0_MSK;

pwm_config_err:
  return ret;
}

static uint8_t _CCM_Channel_Is_Enabled(CCM_ID cid) {
  return (uint8_t)(
      ((CCM_TIMER_CONF_REG(cid) & CCM_TIMER_CONF_ENABLE_MSK) >> CCM_TIMER_CONF_ENABLE_POS));
}

static void _CCM_Disable_Channel(CCM_ID cid) {
  uint32_t reg_val;

  CCM_INT_MASK_REG(cid) = CCM_INT_MASK_TIMER_INT_MSK | CCM_INT_MASK_COMPARE_INT_MSK;

  reg_val = CCM_COMPARE_CONF_REG(cid);
  reg_val &= ~(CCM_COMPARE_CONF_ENABLE_MSK);
  CCM_COMPARE_CONF_REG(cid) = reg_val;

  reg_val = CCM_TIMER_CONF_REG(cid);
  reg_val &= ~(CCM_TIMER_CONF_ENABLE_MSK);
  CCM_TIMER_CONF_REG(cid) = reg_val;

  reg_val = CCM_CONF_DONE_REG(cid);
  reg_val |= (CCM_CONF_FORCE_STOP_MSK);
  CCM_CONF_DONE_REG(cid) = reg_val;
}

static void _CCM_Enable_Channel(CCM_ID cid) {
  uint32_t reg_val;

  reg_val = CCM_TIMER_CONF_REG(cid);
  reg_val |= (CCM_TIMER_CONF_ENABLE_MSK);
  CCM_TIMER_CONF_REG(cid) = reg_val;

  reg_val = CCM_COMPARE_CONF_REG(cid);
  reg_val |= (CCM_COMPARE_CONF_ENABLE_MSK);
  CCM_COMPARE_CONF_REG(cid) = reg_val;

  if (gCCMDev.cb_event)
    CCM_INT_MASK_REG(cid) = 0;
  else
    CCM_INT_MASK_REG(cid) = CCM_INT_MASK_TIMER_INT_MSK | CCM_INT_MASK_COMPARE_INT_MSK;

  reg_val = CCM_CONF_DONE_REG(cid);
  reg_val |= (CCM_CONF_DONE_MSK);
  CCM_CONF_DONE_REG(cid) = reg_val;
}

int32_t CCM_Initialize(CCM_SignalEvent_t cb_event) {
  NVIC_SetPriority(CC_PWM0_IRQn, 7);
  NVIC_SetPriority(CC_PWM1_IRQn, 7);
  gCCMDev.cb_event = cb_event;
  memset(gCCMDev.mode, 0x0, CCM_NUM * sizeof(uint32_t));
  /*We do not check the return code here since user may use this hw block for interrupt only*/
  (void)if_mngr_config_io(IF_MNGR_PWM0);
  (void)if_mngr_config_io(IF_MNGR_PWM1);
  return CCM_DRIVER_OK;
}

int32_t CCM_Uninitialize(void) {
  CCM_ID cid;
  for (cid = CCM_ID_0; cid < CCM_NUM; cid++) {
    if (_CCM_Channel_Is_Enabled(cid)) {
      _CCM_Disable_Channel(cid);
    }
  }
  gCCMDev.cb_event = NULL;
  memset(gCCMDev.mode, 0x0, CCM_NUM * sizeof(uint32_t));
  return CCM_DRIVER_OK;
}

int32_t CCM_PowerControl(CCM_POWER_STATE state) {
  CCM_ID cid;
  switch (state) {
    case POWER_OFF:
      for (cid = CCM_ID_0; cid < CCM_NUM; cid++) {
        if (_CCM_Channel_Is_Enabled(cid)) {
          _CCM_Disable_Channel(cid);
        }
      }
      NVIC_DisableIRQ(CC_PWM0_IRQn);
      NVIC_DisableIRQ(CC_PWM1_IRQn);
      clk_gating_source_disable(CLK_GATING_PWM);
      break;

    case POWER_FULL:
      clk_gating_source_enable(CLK_GATING_PWM);
      NVIC_EnableIRQ(CC_PWM0_IRQn);
      NVIC_EnableIRQ(CC_PWM1_IRQn);
      break;

    case POWER_LOW:
    default:
      return CCM_DRIVER_ERROR_UNSUPPORTED;
      break;
  }
  gCCMDev.pwr_state = state;
  return CCM_DRIVER_OK;
}

int32_t CCM_Control(uint32_t control, uint32_t arg) {
  CCM_ID cid;
  switch (control) {
    case CCM_MODE_PWM_ONE_THRESHOLD:
    case CCM_MODE_1BIT_DAC:
    case CCM_MODE_CLK_OUT:
      cid = (CCM_ID)arg;

      if (cid >= CCM_NUM) return CCM_DRIVER_ERROR_PARAMETER;

      if (gCCMDev.mode[cid] != control && _CCM_Channel_Is_Enabled(cid)) _CCM_Disable_Channel(cid);

      gCCMDev.mode[cid] = control;

      break;

    default:
      return CCM_DRIVER_ERROR_UNSUPPORTED;
  }
  return CCM_DRIVER_OK;
}

int32_t CCM_ConfigOutputChannel(CCM_ID CCM_id, uint32_t param1, uint32_t param2, uint32_t param3) {
  uint8_t need_reenable = 0;
  uint32_t current_mode;
  int32_t ret = CCM_DRIVER_OK;

  need_reenable = _CCM_Channel_Is_Enabled(CCM_id);

  if (need_reenable) _CCM_Disable_Channel(CCM_id);

  current_mode = gCCMDev.mode[CCM_id];
  switch (current_mode) {
    case CCM_MODE_PWM_ONE_THRESHOLD:
      ret = _CCM_Apply_Pwm_One_Threshold(CCM_id, param1, param2, param3);
      break;
    case CCM_MODE_1BIT_DAC:
      ret = _CCM_Apply_Pwm_One_Bit_Dac(CCM_id, param1, param2);
      break;
    case CCM_MODE_CLK_OUT:
      ret = _CCM_Apply_Clk_Out(CCM_id, param1, param2);
      break;
    default:
      ret = CCM_DRIVER_ERROR;
      break;
  }

  if (need_reenable) _CCM_Enable_Channel(CCM_id);

  return ret;
}

void CCM_EnableDisableOutputChannels(uint32_t channelMask) {
  CCM_ID cid;
  uint8_t en;
  for (cid = CCM_ID_0; cid < CCM_NUM; cid++) {
    en = (channelMask >> cid) & 1;
    if (en != _CCM_Channel_Is_Enabled(cid)) {
      if (en)
        _CCM_Enable_Channel(cid);
      else
        _CCM_Disable_Channel(cid);
    }
  }
}

int32_t CCM_GetChannelStatus(CCM_ID CCM_Id, CCM_CHANNEL_STATUS *status) {
  uint32_t timer_precision = 0;
  uint32_t fclk = SystemCoreClock;
  if (!status || (CCM_Id >= CCM_NUM)) return CCM_DRIVER_ERROR_PARAMETER;

  status->enable = _CCM_Channel_Is_Enabled(CCM_Id);
  status->mode = gCCMDev.mode[CCM_Id];
  status->param1 = 0;
  status->param2 = 0;
  status->param3 = 0;
  switch (status->mode) {
    case CCM_MODE_PWM_ONE_THRESHOLD:
      timer_precision = (NSEC_PER_SEC / fclk) * (CCM_TIMER_PRESCALAR_REG(CCM_Id) + 1);
      status->param1 = (CCM_TIMER_NCO_MAX_REG(CCM_Id) + 1) * timer_precision;
      status->param2 = (CCM_COMPARE_CONF_REFVAL0_REG(CCM_Id) + 1) * timer_precision;
      status->param3 = (CCM_COMPARE_CONF_REG(CCM_Id) & CCM_COMPARE_CONF_START_VAL_MSK) >>
                       CCM_COMPARE_CONF_START_VAL_POS;
      break;
    case CCM_MODE_1BIT_DAC:
    case CCM_MODE_CLK_OUT:
      status->param1 =
          ((CCM_TIMER_NCO_MAX_REG(CCM_Id) & CCM_TIMER_NCO_MAX_MSK) >> CCM_TIMER_NCO_MAX_POS) + 1;
      status->param2 =
          ((CCM_TIMER_NCO_STEP_REG(CCM_Id) & CCM_TIMER_NCO_STEP_MSK) >> CCM_TIMER_NCO_STEP_POS);
      break;
    default:
      break;
  }
  return CCM_DRIVER_OK;
}
