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
/* Standard includes. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Platform Headers */
#include DEVICE_HEADER

#include "newlibPort.h"
#include "DRV_PMP_AGENT.h"
#include "DRV_ATOMIC_COUNTER.h"
#include "DRV_GPIO_WAKEUP.h"
#include "DRV_PM.h"
#include "DRV_IF_CFG.h"
#if (configUSE_ALT_SLEEP == 1)
#include "pwr_mngr.h"
#include "sleep_notify.h"
#include "sleep_mngr.h"
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define PM_ASSERT(x) \
  if ((x) == 0) {    \
    for (;;)         \
      ;              \
  } /**< Assertion check */

/****************************************************************************
 * Private Types
 ****************************************************************************/
typedef struct drv_pm_retentionsection {
  uint32_t magic;
  uint32_t crc;  /* CRC32 over data bytes    */
  char data[16]; /* data        */
} DRV_PM_RetentionSection;

typedef struct {
  DRV_PM_PwrMode mode;            /* sleep mode*/
  uint32_t duration;              /* sleep duration*/
  uint32_t memRetentionSecIdList; /* section ID list for SRAM memory Retention */
} DRV_PM_SleepConfig;

/****************************************************************************
 * Private Data
 ****************************************************************************/
static DRV_PM_Statistics sleep_statistics;
static uint32_t intr_masked_tab[64];
static uint32_t clkgating_reg_backup = 0;
static int32_t drv_pm_initialized = 0;

#define __GPMSEC__ __attribute__((section("GPMdata")))
DRV_PM_RetentionSection pm_retent_sec __GPMSEC__;

static CLK_CONTROL_Type *clk_ctrl_reg = CLK_CONTROL;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/
static void mark_stateful_sleep(void);
static void send_sleep_request(DRV_PM_SleepConfig *pwr_config);
static void gpm_boot_probe(void);
static DRV_PM_Status DRV_PM_WAKEUP_IO_Initialize(void);
static void update_wakeup_info(DRV_PM_WakeUpCause cause, uint32_t duration_left, uint32_t count);
static void update_wakeup_cause(void);
/****************************************************************************
 * Private Functions
 ****************************************************************************/
/* add a mark to retention memory for stateful sleep */
static void mark_stateful_sleep(void) {
  pm_retent_sec.magic = 0xdeadbeef;
  pm_retent_sec.crc = 0x1;
  strncpy(pm_retent_sec.data, "StaFul-retM", sizeof(pm_retent_sec.data) - 1);
}

/* Send go-to-sleep to PMP. */
static void send_sleep_request(DRV_PM_SleepConfig *pwr_config) {
  DRV_PMP_MCU2PMP_Message sent_msg;

  memset(&sent_msg, 0x0, sizeof(DRV_PMP_MCU2PMP_Message));
  sent_msg.header.msgType = MCU2PMP_MSG_GOING_TO_SLEEP;

  switch (pwr_config->mode) {
    case DRV_PM_MODE_STOP:
      sent_msg.body.goingToSleep.sleepType = MCU_STOP;
      break;
    case DRV_PM_MODE_STANDBY:
      sent_msg.body.goingToSleep.sleepType = MCU_STANDBY;
      break;
    case DRV_PM_MODE_SHUTDOWN:
      sent_msg.body.goingToSleep.sleepType = MCU_SHUTDOWN;
      break;
    default:
      /* error: wrong sleep mode */
      return;
  }
  sent_msg.body.goingToSleep.sleepDuration = pwr_config->duration;
  sent_msg.body.goingToSleep.memRetentionSecIdList =
      (pwr_config->memRetentionSecIdList & PWR_CON_RETENTION_SEC_MASK_ALL);

  /* Note: shall confirm the request is from IDLE task. */
  DRV_PMP_AGENT_SendMessageByIdleTask(&sent_msg);
}

/*-----------------------------------------------------------------------------
 * PURPOSE: This function would probe the boot type.
 *        Now this scheme is thru checking a magic string stil be kept in retention
 *        memory after sleep.
 *-----------------------------------------------------------------------------
 */
static void gpm_boot_probe(void) {
  /* if (magic # is match), retention data is kept */
  if (pm_retent_sec.magic == 0xdeadbeef) {
    sleep_statistics.wakeup_state = (DRV_PM_WAKEUP_STATEFUL);
  } else { /* else, retention data is not kept */
    sleep_statistics.wakeup_state = (DRV_PM_WAKEUP_STATELESS);
  }

  /* clear magic from retention memory */
  memset(&pm_retent_sec, 0x0, sizeof(DRV_PM_RetentionSection));
}

/*-----------------------------------------------------------------------------
 * DRV_PM_Status DRV_PM_WAKEUP_IO_Initialize(void)
 * PURPOSE: This function would initialize and configure default wakeup setting.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  DRV_PM_Status.
 *-----------------------------------------------------------------------------
 */
static DRV_PM_Status DRV_PM_WAKEUP_IO_Initialize(void) {
  Interface_Id wakeup_if;
  DRV_PM_WakeupConfig wakeup_cfg;

  for (wakeup_if = IF_CFG_WAKEUP01; wakeup_if <= IF_CFG_WAKEUP04; wakeup_if++) {
    if (DRV_IF_CFG_GetDefConfig(wakeup_if, &wakeup_cfg) == DRV_IF_CFG_OK) {
      DRV_PM_EnableWakeUpPin(wakeup_cfg.pin_set, wakeup_cfg.pin_pol);
    }
  }
  return DRV_PM_OK;
}

/*-----------------------------------------------------------------------------
 * void update_wakeup_info(DRV_PM_WakeUpCause cause,
 *          uint32_t duration_left, uint32_t count)
 * PURPOSE: This function would update the data of last wakeup.
 * PARAMs:
 *      INPUT:  DRV_PM_WakeUpCause cause - wakeup cause.
 *              uint32_t duration_left - sleep duration left.
 *              uint32_t count - wakeup count. only available in STOP mode.
 *      OUTPUT: None
 * RETURN:  None
 *-----------------------------------------------------------------------------
 */
void update_wakeup_info(DRV_PM_WakeUpCause cause, uint32_t duration_left, uint32_t count) {
  sleep_statistics.last_cause = cause;
  sleep_statistics.last_dur_left = duration_left;
  sleep_statistics.counter = count;

  if ((sleep_statistics.wakeup_state == DRV_PM_WAKEUP_STATELESS) &&
      (sleep_statistics.last_cause == DRV_PM_WAKEUP_CAUSE_NONE) &&
      (sleep_statistics.last_dur_left == 0) && (sleep_statistics.counter == 0)) {
    sleep_statistics.boot_type = DRV_PM_DEV_COLD_BOOT;
  } else {
    sleep_statistics.boot_type = DRV_PM_DEV_WARM_BOOT;
  }
}

void update_wakeup_cause(void) {
  uint32_t cause, time_left, count;
  DRV_PMP_AGENT_GetWakeInfo(&cause, &time_left, &count);
  if (cause < DRV_PM_WAKEUP_CAUSE_TIMEOUT || cause > DRV_PM_WAKEUP_CAUSE_MODEM) {
    sleep_statistics.last_cause = DRV_PM_WAKEUP_CAUSE_UNKNOWN;
  } else {
    update_wakeup_info((DRV_PM_WakeUpCause)cause, time_left, count);
  }
}
/****************************************************************************
 * Public Functions
 ****************************************************************************/
/*-----------------------------------------------------------------------------
 * void DRV_PM_ClearStatistics(void)
 * PURPOSE: This function would reset current sleep statistics.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  None
 *-----------------------------------------------------------------------------
 */
void DRV_PM_ClearStatistics(void) { sleep_statistics.counter = 0; }

/*-----------------------------------------------------------------------------
 * int32_t DRV_PM_GetStatistics(DRV_PM_Statistics *pwr_stat)
 * PURPOSE: This function would get sleep statistics of MCU.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: pointer of DRV_PM_Statistics
 * RETURN:  error code. 0-success; other-fail
 *-----------------------------------------------------------------------------
 */
int32_t DRV_PM_GetStatistics(DRV_PM_Statistics *pwr_stat) {
  if (pwr_stat == NULL) return (-1);

  update_wakeup_cause();
  memcpy(pwr_stat, &sleep_statistics, sizeof(DRV_PM_Statistics));

  return 0;
}

/*-----------------------------------------------------------------------------
 * DRV_PM_BootType DRV_PM_GetDevBootType(void)
 * PURPOSE: This function would get device boot type.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  enumeration value of device boot type.
 * Note:  it can't distinguish the boot is triggered by reset button.
 *-----------------------------------------------------------------------------
 */
DRV_PM_BootType DRV_PM_GetDevBootType(void) { return sleep_statistics.boot_type; }

/*-----------------------------------------------------------------------------
 * DRV_PM_Status DRV_PM_EnterStopMode(DRV_PM_StopModeConfig *pwr_stop_param)
 * PURPOSE: This function would configure to Stop mode.
 * PARAMs:
 *      INPUT:  DRV_PM_StopModeConfig *pwr_stop_param
 *      OUTPUT: None
 * RETURN:  DRV_PM_Status.
 *-----------------------------------------------------------------------------
 */
DRV_PM_Status DRV_PM_EnterStopMode(DRV_PM_StopModeConfig *pwr_stop_param) {
  DRV_PM_SleepConfig pwr_config;

  sleep_statistics.wakeup_state = (DRV_PM_WAKEUP_STATEFUL);

  memset(&pwr_config, 0x0, sizeof(DRV_PM_SleepConfig));
  pwr_config.mode = DRV_PM_MODE_STOP;
  pwr_config.duration = pwr_stop_param->duration;
  send_sleep_request(&pwr_config);

  SCB->ICSR = SCB->ICSR | (0x8000000);
  __asm volatile("dsb" ::: "memory");
  __asm volatile("wfi");
  __asm volatile("isb");

  return DRV_PM_OK;
}

/*-----------------------------------------------------------------------------
 * DRV_PM_Status DRV_PM_EnterStandbyMode(DRV_PM_StandbyModeConfig *pwr_standby_param)
 * PURPOSE: This function would configure to Standby mode.
 * PARAMs:
 *      INPUT:  DRV_PM_StandbyModeConfig *pwr_standby_param
 *      OUTPUT: None
 * RETURN:  DRV_PM_Status.
 *-----------------------------------------------------------------------------
 */
DRV_PM_Status DRV_PM_EnterStandbyMode(DRV_PM_StandbyModeConfig *pwr_standby_param) {
  DRV_PM_SleepConfig pwr_config;
  int32_t i = 0;

  /*check input */
#ifdef ALT1255
  if ((pwr_standby_param->memRetentionSecIdList != 0) &&
      (pwr_standby_param->memRetentionSecIdList != PWR_CON_RETENTION_SEC_2)) {
    return DRV_PM_GEN_ERROR;
  }
#else
  if (pwr_standby_param->memRetentionSecIdList > PWR_CON_RETENTION_SEC_MASK_ALL) {
    return DRV_PM_GEN_ERROR;
  }
#endif

  /* marks stateful sleep */
  mark_stateful_sleep();

  memset(&pwr_config, 0x0, sizeof(DRV_PM_SleepConfig));
  pwr_config.mode = DRV_PM_MODE_STANDBY;
  pwr_config.duration = pwr_standby_param->duration;
  pwr_config.memRetentionSecIdList = pwr_standby_param->memRetentionSecIdList;
  send_sleep_request(&pwr_config);

  SCB->ICSR = SCB->ICSR | (0x8000000);
  // waiting for pmp handling
  while (1) {
    __asm volatile("dsb" ::: "memory");
    __asm volatile("wfi");
    __asm volatile("isb");
    if (i >= 10) {
      /* we should never get here, assert it */
      PM_ASSERT(0);
    }
    i++;
  }

  return DRV_PM_GEN_ERROR;  // Unreachable, still keep it for API compatible
}

/*-----------------------------------------------------------------------------
 * DRV_PM_Status DRV_PM_EnterShutdownMode (DRV_PM_ShutdownModeConfig *pwr_shutdown_param)
 * PURPOSE: This function would configure to Shutdown mode.
 * PARAMs:
 *      INPUT:  DRV_PM_ShutdownModeConfig *pwr_shutdown_param
 *      OUTPUT: None
 * RETURN:  DRV_PM_Status.
 *-----------------------------------------------------------------------------
 */
DRV_PM_Status DRV_PM_EnterShutdownMode(DRV_PM_ShutdownModeConfig *pwr_shutdown_param) {
  DRV_PM_SleepConfig pwr_config;
  int32_t i = 0;

  memset(&pwr_config, 0x0, sizeof(DRV_PM_SleepConfig));
  pwr_config.mode = DRV_PM_MODE_SHUTDOWN;
  pwr_config.duration = pwr_shutdown_param->duration;
  send_sleep_request(&pwr_config);

  SCB->ICSR = SCB->ICSR | (0x8000000);
  // waiting for pmp handling
  while (1) {
    __asm volatile("dsb" ::: "memory");
    __asm volatile("wfi");
    __asm volatile("isb");
    if (i >= 10) {
      /* we should never get here, assert it */
      PM_ASSERT(0);
    }
    i++;
  }

  return DRV_PM_GEN_ERROR;  // Unreachable, still keep it for API compatible
}

/*-----------------------------------------------------------------------------
 * DRV_PM_Status DRV_PM_EnableWakeUpPin(uint32_t pinNum, DRV_PM_WAKEUP_PinPolarity polarity)
 * PURPOSE: This function would enable WakeUp Pin.
 * PARAMs:
 *      INPUT:  pinNum - GPIO Pin number.
 *              polarity.
 *      OUTPUT: None
 * RETURN:  DRV_PM_Status.
 *-----------------------------------------------------------------------------
 */
DRV_PM_Status DRV_PM_EnableWakeUpPin(uint32_t pinNum, DRV_PM_WAKEUP_PinPolarity polarity) {
  DRV_GPIO_WAKEUP_Configuration wkup_struct = {0}, temp_struct = {0};
  DRV_PM_Status ret_val = DRV_PM_OK;
  int32_t res;

  wkup_struct.gpio_pin = pinNum;
  wkup_struct.debounce_max_val = 1;
  wkup_struct.clk = GPIO_WAKEUP_RTC_CLK;
  wkup_struct.debounce_en = 1;
  wkup_struct.int_en = 0;
  wkup_struct.mode = (GPIO_WAKEUP_Polarity)polarity;
  wkup_struct.wakeup_en = 1;

  DRV_GPIO_WAKEUP_Initialize();

  DRV_GPIO_WAKEUP_GetSetup(&temp_struct, pinNum, DRV_GPIO_WAKEUP_REGISTER_LOOKUP);
  if ((temp_struct.gpio_pin >= 0) && (temp_struct.mode == wkup_struct.mode) &&
      (temp_struct.debounce_en == wkup_struct.debounce_en) &&
      (temp_struct.wakeup_en == wkup_struct.wakeup_en)) {
    // if set value is the same, return OK
    return DRV_PM_OK;
  }

  res = DRV_GPIO_WAKEUP_AddIO(&wkup_struct);
  if (res != 0) {
    ret_val = DRV_PM_GEN_ERROR;
  }

  return ret_val;
}

/*-----------------------------------------------------------------------------
 * DRV_PM_Status DRV_PM_DisableWakeUpPin(uint32_t pinNum)
 * PURPOSE: This function would disable WakeUp Pin.
 * PARAMs:
 *      INPUT:  pinNum - GPIO Pin number.
 *      OUTPUT: None
 * RETURN:  DRV_PM_Status.
 *-----------------------------------------------------------------------------
 */
DRV_PM_Status DRV_PM_DisableWakeUpPin(uint32_t pinNum) {
  if (DRV_GPIO_WAKEUP_DeleteIO(pinNum) != 0) return DRV_PM_GEN_ERROR;

  return DRV_PM_OK;
}

/*-----------------------------------------------------------------------------
 * DRV_PM_Status DRV_PM_Initialize(void)
 * PURPOSE: This function would configure and intialize Power Management module.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  DRV_PM_Status.
 *-----------------------------------------------------------------------------
 */
DRV_PM_Status DRV_PM_Initialize(void) {
  if (drv_pm_initialized) return 0;

  memset(&sleep_statistics, 0, sizeof(DRV_PM_Statistics));
  update_wakeup_cause();
  gpm_boot_probe();
  DRV_PM_ClearStatistics();
#if (configUSE_ALT_SLEEP == 1)
  smngr_init();
  sleep_notify_init();
#endif

  if (DRV_PM_WAKEUP_IO_Initialize() != DRV_PM_OK) {
    return DRV_PM_GEN_ERROR;
  }

#if (configUSE_ALT_SLEEP == 1)
  if (pwr_mngr_init() != PWR_MNGR_OK) {
    return DRV_PM_GEN_ERROR;
  }
#endif

  drv_pm_initialized = 1;
  return DRV_PM_OK;
}

/*-----------------------------------------------------------------------------
 * void DRV_PM_MaskInterrupts(DRV_PM_PwrMode pwr_mode)
 * PURPOSE: This function would disable interrupts before entering sleep.
 * PARAMs:
 *      INPUT:  DRV_PM_PwrMode mode
 *      OUTPUT: None
 * RETURN:  None.
 *-----------------------------------------------------------------------------
 */
void DRV_PM_MaskInterrupts(DRV_PM_PwrMode mode) {
  uint32_t cnt = 0, idx;

  memset(intr_masked_tab, 0x0, sizeof(intr_masked_tab));
  for (idx = 0; idx < 64; idx++) {
    if ((idx == ATMCTR_MAILBOX_IRQn) &&
        ((mode != DRV_PM_MODE_STANDBY) && (mode != DRV_PM_MODE_SHUTDOWN)))  // not mask pmp2mcu AC
      continue;

    if (NVIC_GetEnableIRQ((IRQn_Type)idx) == 1) {
      NVIC_DisableIRQ((IRQn_Type)idx);
      intr_masked_tab[cnt] = idx;
      cnt++;
    }
  }
  intr_masked_tab[cnt] = 255;
}

/*-----------------------------------------------------------------------------
 * void DRV_PM_RestoreInterrupts(void)
 * PURPOSE: This function would restore interrupts disabled for entering to sleep.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  None.
 *-----------------------------------------------------------------------------
 */
void DRV_PM_RestoreInterrupts(void) {
  uint32_t idx;

  for (idx = 0; idx < 64; idx++) {
    if (intr_masked_tab[idx] >= 64) break;
    NVIC_EnableIRQ((IRQn_Type)intr_masked_tab[idx]);
  }
}

/*-----------------------------------------------------------------------------
 * void pwr_store_set_gatedclks_before_sleep(void)
 * PURPOSE: This function would handle gated clocks IO before entering or leaving sleep.
 * PARAMs:
 *      INPUT:  bool enter_sleep(true); leave_sleep(false)
 *      OUTPUT: None
 * RETURN:  None
 *-----------------------------------------------------------------------------
 */
void DRV_PM_HandleClockGating(bool enter_sleep) {
  if (enter_sleep) {
    clkgating_reg_backup = clk_ctrl_reg->MCU_CLK_REQ_EN;
  } else {
    clk_ctrl_reg->MCU_CLK_REQ_EN = clkgating_reg_backup;
  }
}
