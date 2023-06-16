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
#include <stdbool.h>

/* Kernel includes. */
#include "DRV_GPIO.h"
#include "pwr_mngr.h"
#include "DRV_PM.h"
#include "DRV_IF_CFG.h"
#include "hifc_api.h"
#include "sleep_mngr.h"
#include "sleep_notify.h"
#include "alt_osal.h"

#include "hibernate.h"

/****************************************************************************
 * Definitions
 ****************************************************************************/

/* debug level: 0:critical; 1: warning; 2:information; 3: debug */
#define PWR_MNGR_LOG_LEVEL 0
#if defined(PWR_MNGR_ENABLE_DEBUG_PRINTS)
#define PWR_MNGR_LOG(lEVEL, fmt, args...)                 \
  {                                                       \
    if (lEVEL <= PWR_MNGR_LOG_LEVEL) printf(fmt, ##args); \
  }
#else
#define PWR_MNGR_LOG(lEVEL, fmt, args...) \
  {}
#endif

#define PWR_MNGR_LOCK(handle)                         \
  do {                                                \
    alt_osal_lock_mutex(handle, ALT_OSAL_TIMEO_FEVR); \
  } while (0)

#define PWR_MNGR_UNLOCK(handle)    \
  do {                             \
    alt_osal_unlock_mutex(handle); \
  } while (0)

/****************************************************************************
 * Private Types
 ****************************************************************************/
typedef struct pwr_mngr_mongpioactiveentry {
  int32_t gpio_num;
  int32_t pol_level;
} PWR_MNGR_MonGpioActiveEntry;

typedef struct pwr_mngr_mongpiolist {
  PWR_MNGR_MonGpioActiveEntry entry[PWR_MNGR_MON_GPIO_MAX_ENTRY];
} PWR_MNGR_MonGpioList;

typedef struct pwr_mngr_control {
  int32_t sleep_manager_counters_enable;
} PWR_MNGR_Control;

/****************************************************************************
 * Private Data
 ****************************************************************************/
static PWR_MNGR_Configuation sleep_conf = {0};
static PWR_MNGR_PwrCounters sleep_counters = {0};
static PWR_MNGR_Control pwrmngr_control = {0};
static PWR_MNGR_MonGpioList mon_gpio_list = {0};

/* UART inactive time */
static uint32_t uart_inactivity_time = 5; /* in units of second */
static volatile uint32_t last_uart_interupt;

static int32_t allow_to_sleep = 0;
static uint32_t sleep_cnt;
static uint32_t enter_sleep;
static bool is_initialized = false;

static alt_osal_mutex_handle pwr_mngr_mtx = NULL;
/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/
#if (configUSE_ALT_SLEEP == 1)
static void pm_conf_init(void);
#endif
static void pm_sleep_enable(void);
static void pm_sleep_disable(void);
static void update_sleep_manager_counter(unsigned long sync_mask, unsigned long async_mask);
static int32_t pwr_check_uart_inactive_time(void);
static int32_t pwr_mngr_check_mon_gpio_busy(void);

/****************************************************************************
 * Private Functions
 ****************************************************************************/
/*-----------------------------------------------------------------------------
 * static void pm_conf_init(void)
 * PURPOSE: This function would initialize default setting of power manager.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  None
 *-----------------------------------------------------------------------------
 */
#if (configUSE_ALT_SLEEP == 1)
static void pm_conf_init(void) {
  int32_t idx;

  memset(&sleep_conf, 0x0, sizeof(PWR_MNGR_Configuation));
  sleep_conf.duration = PWR_MNGR_DFT_SLEEP_DRUATION;  // unit:ms
  sleep_conf.section_id_list = (PWR_CON_RETENTION_SEC_MASK_ALL);
  sleep_conf.mode = PWR_MNGR_MODE_SLEEP;
  sleep_conf.threshold = PWR_MNGR_SHUTDOWN_THRESHOLD;
  sleep_conf.enable = 0;  // 1: enable
  allow_to_sleep = sleep_conf.enable;

  memset(&mon_gpio_list, 0x0, sizeof(PWR_MNGR_MonGpioList));
  memset(&pwrmngr_control, 0x0, sizeof(PWR_MNGR_Control));

  for (idx = 0; idx < PWR_MNGR_MON_GPIO_MAX_ENTRY; idx++) mon_gpio_list.entry[idx].gpio_num = 255;
}
#endif
/*-----------------------------------------------------------------------------
 * static int32_t pwr_check_uart_inactive_time(void)
 * PURPOSE: This function would check if UART inactivity time still less than
 *          upper limitation.
 *          If yes, block this cycle to sleep.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  0: over upper limitation, 1: less than setting.
 *-----------------------------------------------------------------------------
 */
static int32_t pwr_check_uart_inactive_time(void) {
  uint32_t now = alt_osal_get_tick_count() / portTICK_PERIOD_MS;
  uint32_t uart_interupt_delta = 0;
  int32_t ret_val = 0;

  if (uart_inactivity_time == 0) {
    ret_val = 0;
  } else {
    /* calculate UART inactivity time */
    if (last_uart_interupt == 0) {
      last_uart_interupt = now;
      uart_interupt_delta = 0;
    } else {
      uart_interupt_delta = ((now / 1000) - (last_uart_interupt / 1000));
    }

    if (uart_interupt_delta < uart_inactivity_time) {
      ret_val = 1;
    }
  }
  return ret_val;
}

/*-----------------------------------------------------------------------------
 * static void pm_sleep_enable(void)
 * PURPOSE: This function would enable MCU sleep.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  None
 *-----------------------------------------------------------------------------
 */
static void pm_sleep_enable(void) {
  sleep_conf.enable = 1;
  allow_to_sleep = 1;
}

/*-----------------------------------------------------------------------------
 * static void pm_sleep_disable(void)
 * PURPOSE: This function would disable MCU sleep.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  None
 *-----------------------------------------------------------------------------
 */
static void pm_sleep_disable(void) {
  sleep_conf.enable = 0;
  allow_to_sleep = 0;
  sleep_conf.mode = PWR_MNGR_MODE_SLEEP;
}

/*-----------------------------------------------------------------------------
 * static void update_sleep_manager_counter(unsigned long sync_mask, unsigned long async_mask)
 * PURPOSE: This function would set UART inactivity time.
 * PARAMs:
 *      INPUT:  uint32_t inact_time (unit: second)
 *      OUTPUT: None
 * RETURN:  error code. 0-success; other-fail
 *-----------------------------------------------------------------------------
 */
static void update_sleep_manager_counter(unsigned long sync_mask, unsigned long async_mask) {
  int32_t i = 0;

  if (sync_mask > 0) {
    for (i = 1; i < SMNGR_RANGE; i++) {
      if ((sync_mask >> i) & 1L) {
        sleep_counters.sleep_manager_devices[i]++;
      }
    }
  }

  if (async_mask > 0) {
    sleep_counters.sleep_manager_devices[async_mask]++;
  }
}

/*-----------------------------------------------------------------------------
 * static void pwr_mngr_check_mon_gpio_busy(void)
 * PURPOSE: This function would check if specified GPIO port is busy.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  0:not busy; 1:busy.
 *-----------------------------------------------------------------------------
 */
static int32_t pwr_mngr_check_mon_gpio_busy(void) {
  int idx;
  uint8_t gpio_val = 0;

  for (idx = 0; idx < PWR_MNGR_MON_GPIO_MAX_ENTRY; idx++) {
    if ((mon_gpio_list.entry[idx].gpio_num >= MCU_GPIO_ID_01) &&
        (mon_gpio_list.entry[idx].gpio_num < MCU_GPIO_ID_MAX)) {
      if ((DRV_GPIO_SetDirection((GPIO_Id)mon_gpio_list.entry[idx].gpio_num, GPIO_DIR_INPUT) ==
           DRV_GPIO_OK) &&
          (DRV_GPIO_GetValue((GPIO_Id)mon_gpio_list.entry[idx].gpio_num, &gpio_val) ==
           DRV_GPIO_OK) &&
          (gpio_val == mon_gpio_list.entry[idx].pol_level)) {
        /* specified gpio is busy */
        return 1;
      }
    }
  }
  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
/*-----------------------------------------------------------------------------
 * int32_t pwr_mngr_set_debug_mode(int32_t en_debug)
 * PURPOSE: This function would switch on/off power-manager debug mode.
 * PARAMs:
 *      INPUT:  en_debug (1:enable / 0:disable)
 *      OUTPUT: None
 * RETURN:  error code. 0-success; other-fail.
 *-----------------------------------------------------------------------------
 */
int32_t pwr_mngr_set_debug_mode(int32_t en_debug) {
  int32_t ret_val = 0;

  PWR_MNGR_LOCK(&pwr_mngr_mtx);
  if (en_debug == 1) {
    pwrmngr_control.sleep_manager_counters_enable = 1;
  } else if (en_debug == 0) {
    pwrmngr_control.sleep_manager_counters_enable = 0;
  } else {
    ret_val = (-1);
  }
  PWR_MNGR_UNLOCK(&pwr_mngr_mtx);

  return ret_val;
}

/*-----------------------------------------------------------------------------
 * int32_t pwr_mngr_conf_get_sleep(void)
 * PURPOSE: This function would get sleep setting.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  return value: 1 enable; 0 diable.
 *-----------------------------------------------------------------------------
 */
int32_t pwr_mngr_conf_get_sleep(void) { return sleep_conf.enable; }

/*-----------------------------------------------------------------------------
 * PWR_MNGR_Status pwr_mngr_enable_sleep(int32_t set_val)
 * PURPOSE: This function would set sleep setting.
 * PARAMs:
 *      INPUT:  set_val - 1:enable; 0:disable
 *      OUTPUT: None
 * RETURN:  error code. 0-success; other-fail
 *-----------------------------------------------------------------------------
 */
PWR_MNGR_Status pwr_mngr_enable_sleep(int32_t set_val) {
  int32_t ret_val = PWR_MNGR_OK;

  if (!is_initialized) return (PWR_MNGR_NOT_INIT);

  PWR_MNGR_LOCK(&pwr_mngr_mtx);
  if (set_val == 1)
    pm_sleep_enable();
  else if (set_val == 0)
    pm_sleep_disable();
  else
    ret_val = PWR_MNGR_GEN_ERROR;
  PWR_MNGR_UNLOCK(&pwr_mngr_mtx);

  return ret_val;
}

/*-----------------------------------------------------------------------------
 * PWR_MNGR_Status pwr_mngr_conf_set_mode(PWR_MNGR_PwrMode mode, uint32_t duration)
 * PURPOSE: This function would set power mode and duration.
 * PARAMs:
 *      INPUT: powr_mode, sleep_duration (unit: ms).
 *      OUTPUT: None
 * RETURN:  PWR_MNGR_Status.
 *-----------------------------------------------------------------------------
 */
PWR_MNGR_Status pwr_mngr_conf_set_mode(PWR_MNGR_PwrMode mode, uint32_t duration) {
  if (!is_initialized) return (PWR_MNGR_NOT_INIT);

  PWR_MNGR_LOCK(&pwr_mngr_mtx);
  sleep_conf.mode = mode;
  sleep_conf.duration = duration;
  PWR_MNGR_UNLOCK(&pwr_mngr_mtx);

  return PWR_MNGR_OK;
}

/*-----------------------------------------------------------------------------
 * PWR_MNGR_Status pwr_mngr_enable_retention_sec(uint32_t sec_id_mask)
 * PURPOSE: This function would enable SRAM Content Retention sections.
 * PARAMs:
 *      INPUT:  sec_id_mask - section-id mask.
 *      OUTPUT: None
 * RETURN:  PWR_MNGR_Status.
 *-----------------------------------------------------------------------------
 */
PWR_MNGR_Status pwr_mngr_enable_retention_sec(uint32_t sec_id_mask) {
  if (!is_initialized) return (PWR_MNGR_NOT_INIT);

    /* check input */
#ifdef ALT1255
  if ((sec_id_mask != 0) && (sec_id_mask != PWR_CON_RETENTION_SEC_2)) {
    PWR_MNGR_LOG(1, "Wrong Content Retention ID mask\n\r");
    return PWR_MNGR_GEN_ERROR;
  }
#else
  if (sec_id_mask > PWR_CON_RETENTION_SEC_MASK_ALL) {
    PWR_MNGR_LOG(1, "Wrong Content Retention ID mask\n\r");
    return PWR_MNGR_GEN_ERROR;
  }
#endif
  PWR_MNGR_LOCK(&pwr_mngr_mtx);
  sleep_conf.section_id_list = sec_id_mask;
  PWR_MNGR_UNLOCK(&pwr_mngr_mtx);

  return PWR_MNGR_OK;
}

/*-----------------------------------------------------------------------------
 * PWR_MNGR_Status pwr_mngr_conf_get_settings(PWR_MNGR_Configuation *pwr_conf)
 * PURPOSE: This function would get current power mode setting.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  PWR_MNGR_PwrMode mode.
 *-----------------------------------------------------------------------------
 */
PWR_MNGR_Status pwr_mngr_conf_get_settings(PWR_MNGR_Configuation *pwr_conf) {
  if (pwr_conf == NULL) return PWR_MNGR_GEN_ERROR;

  memcpy(pwr_conf, &sleep_conf, sizeof(PWR_MNGR_Configuation));
  return PWR_MNGR_OK;
}

/*-----------------------------------------------------------------------------
 * PWR_MNGR_Status pwr_mngr_conf_set_cli_inact_time(uint32_t inact_time)
 * PURPOSE: This function would set UART inactivity time.
 * PARAMs:
 *      INPUT:  uint32_t inact_time (unit: second)
 *      OUTPUT: None
 * RETURN:  error code. 0-success; other-fail
 *-----------------------------------------------------------------------------
 */
PWR_MNGR_Status pwr_mngr_conf_set_cli_inact_time(uint32_t inact_time) {
  if (!is_initialized) return (PWR_MNGR_NOT_INIT);

  PWR_MNGR_LOCK(&pwr_mngr_mtx);
  uart_inactivity_time = inact_time;
  PWR_MNGR_UNLOCK(&pwr_mngr_mtx);

  return PWR_MNGR_OK;
}

/*-----------------------------------------------------------------------------
 * void pwr_mngr_pre_sleep_process (uint32_t idle_time)
 * PURPOSE: This function would run pre-sleep process.
 * PARAMs:
 *      INPUT:  uint32_t idle_time
 *              PWR_MNGR_PwrMode pwr_mode
 *      OUTPUT: None
 * RETURN:  status code.
 *-----------------------------------------------------------------------------
 */
void pwr_mngr_pre_sleep_process(uint32_t idle_time, PWR_MNGR_PwrMode pwr_mode) {
  /*Notify register callback of suspending state*/
  if (idle_time > 0) {
    sleep_notify(SUSPENDING, pwr_mode);
    DRV_PM_PreSleepProcess((DRV_PM_PwrMode)pwr_mode);
    
    sleep_cnt++;
    enter_sleep = 1;
  }

  return;
}

/*-----------------------------------------------------------------------------
 * void pwr_mngr_post_sleep_process (uint32_t idle_time, PWR_MNGR_PwrMode pwr_mode)
 * PURPOSE: This function would run post-sleep process.
 * PARAMs:
 *      INPUT:  uint32_t idle_time
 *              PWR_MNGR_PwrMode pwr_mode
 *      OUTPUT: None
 * RETURN:  status code.
 *-----------------------------------------------------------------------------
 */
void pwr_mngr_post_sleep_process(uint32_t idle_time, PWR_MNGR_PwrMode pwr_mode) { 
  if (enter_sleep == 1) {
    DRV_PM_PostSleepProcess((DRV_PM_PwrMode)pwr_mode);

    /*Notify register callback of resuming state*/
    sleep_notify(RESUMING, pwr_mode);
    enter_sleep = 0;
  }

  return;
}

/*-----------------------------------------------------------------------------
 * void pwr_mngr_refresh_uart_inactive_time(void)
 * PURPOSE: This function would refresh last UART interrupt time.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  None
 *-----------------------------------------------------------------------------
 */
void pwr_mngr_refresh_uart_inactive_time(void) {
  volatile uint32_t now = alt_osal_get_tick_count() / portTICK_PERIOD_MS;

  last_uart_interupt = now;
}

/*-----------------------------------------------------------------------------
 * int32_t pwr_mngr_get_prevent_sleep_cntr(PWR_MNGR_PwrCounters *pwr_counters)
 * PURPOSE: This function would get sleep counters of MCU.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: pointer of PWR_MNGR_PwrCounters
 * RETURN:  error code. 0-success; other-fail
 *-----------------------------------------------------------------------------
 */
int32_t pwr_mngr_get_prevent_sleep_cntr(PWR_MNGR_PwrCounters *pwr_counters) {
  if (pwr_counters == NULL) return (-1);

  memcpy(pwr_counters, &sleep_counters, sizeof(PWR_MNGR_PwrCounters));

  return 0;
}

/*-----------------------------------------------------------------------------
 * int32_t pwr_mngr_clr_prevent_sleep_cntr(PWR_MNGR_PwrCounters *pwr_counters)
 * PURPOSE: This function would clear sleep counters of MCU.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  error code. 0-success; other-fail
 *-----------------------------------------------------------------------------
 */
int32_t pwr_mngr_clr_prevent_sleep_cntr(void) {
  memset(&sleep_counters, 0x0, sizeof(PWR_MNGR_PwrCounters));
  return 0;
}

/*-----------------------------------------------------------------------------
 * int32_t pwr_mngr_check_allow2sleep(void)
 * PURPOSE: This function would check if meets the conditions of allow-to-sleep.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  status code.
 *-----------------------------------------------------------------------------
 */
int32_t pwr_mngr_check_allow2sleep(void) {
  uint32_t sync_mask = 0, async_mask = 0;

  if (allow_to_sleep == 0) {
    sleep_counters.sleep_disable++;
    return 0;
  }

  if (pwr_check_uart_inactive_time() == 1) {
    sleep_counters.uart_incativity_time++;
    return 0;
  }

  if (smngr_is_dev_busy(&sync_mask, &async_mask)) {
    sleep_counters.sleep_manager++;

    /*only if sleep manager counters is enable - then look for it*/
    if (pwrmngr_control.sleep_manager_counters_enable) {
      update_sleep_manager_counter(sync_mask, async_mask);
    }
    return 0;
  }

  if (get_if_state() != IfDown) {
    sleep_counters.hifc_busy++;
    return 0;
  }

  if (pwr_mngr_check_mon_gpio_busy()) {
    sleep_counters.mon_gpio_busy++;
    return 0;
  }

  return 1;
}

/*-----------------------------------------------------------------------------
 * PWR_MNGR_Status pwr_mngr_enter_sleep(void)
 * PURPOSE: This function would send allow-to-sleep message to PMP.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  PWR_MNGR_Status.
 *-----------------------------------------------------------------------------
 */
PWR_MNGR_Status pwr_mngr_enter_sleep(void) {
  if (!is_initialized) return (PWR_MNGR_NOT_INIT);

  if (sleep_conf.enable == 0) return PWR_MNGR_GEN_ERROR;

  if (sleep_conf.mode == PWR_MNGR_MODE_STOP) {
    DRV_PM_StopModeConfig st_pwr_stop;

    memset(&st_pwr_stop, 0x0, sizeof(DRV_PM_StopModeConfig));
    PWR_MNGR_LOG(2, "Go to Stop mode...\n\r");
    st_pwr_stop.duration = sleep_conf.duration;
    if (DRV_PM_EnterStopMode(&st_pwr_stop) != DRV_PM_OK) return PWR_MNGR_GEN_ERROR;
  } else if (sleep_conf.mode == PWR_MNGR_MODE_STANDBY) {
    DRV_PM_StandbyModeConfig st_pwr_standby;

    memset(&st_pwr_standby, 0x0, sizeof(DRV_PM_StandbyModeConfig));
    PWR_MNGR_LOG(2, "Go to Standby mode...\n\r");
    st_pwr_standby.duration = sleep_conf.duration;
    st_pwr_standby.memRetentionSecIdList = sleep_conf.section_id_list;
    if (DRV_PM_EnterStandbyMode(&st_pwr_standby) != DRV_PM_OK) return PWR_MNGR_GEN_ERROR;
  } else if (sleep_conf.mode == PWR_MNGR_MODE_SHUTDOWN) {
    DRV_PM_ShutdownModeConfig st_pwr_shutdown;

    memset(&st_pwr_shutdown, 0x0, sizeof(DRV_PM_ShutdownModeConfig));
    PWR_MNGR_LOG(2, "Go to Shutdown mode...\n\r");
    st_pwr_shutdown.duration = sleep_conf.duration;
    if (DRV_PM_EnterShutdownMode(&st_pwr_shutdown) != DRV_PM_OK) return PWR_MNGR_GEN_ERROR;
  } else {
    return PWR_MNGR_GEN_ERROR;
  }

  return PWR_MNGR_OK;
}

/*-----------------------------------------------------------------------------
 * PWR_MNGR_Status pwr_mngr_init(void)
 * PURPOSE: This function would configure and intialize Power Manager.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  PWR_MNGR_Status.
 *-----------------------------------------------------------------------------
 */
PWR_MNGR_Status pwr_mngr_init(void) {
#if (configUSE_ALT_SLEEP == 1)
  alt_osal_mutex_attribute mtx_param = {0};

  if (!is_initialized) {
    pm_conf_init();
    pwr_mngr_clr_prevent_sleep_cntr();
    is_initialized = true;

    /* Create mutex. */
    if (alt_osal_create_mutex(&pwr_mngr_mtx, &mtx_param) < 0) {
      PWR_MNGR_LOG(1, "Mutex create failed.\n");
      return PWR_MNGR_GEN_ERROR;
    }
  } else {
    return PWR_MNGR_GEN_ERROR;
  }

  return PWR_MNGR_OK;
#else
  return PWR_MNGR_GEN_ERROR;
#endif
}

/*-----------------------------------------------------------------------------
 * PWR_MNGR_Status pwr_mngr_set_monitor_io(GPIO_Id gpio_id, int32_t level)
 * PURPOSE: This function would set the gpio_num and level for stop MCU go to sleep.
 * PARAMs:
 *      INPUT:
 *          gpio_id
 *          level: 1 or 0;
 *      OUTPUT: None
 * RETURN: PWR_MNGR_Status.
 *-----------------------------------------------------------------------------
 */
PWR_MNGR_Status pwr_mngr_set_monitor_io(GPIO_Id gpio_id,
                                        int32_t level) { /* Not allow duplicate entry */
  PWR_MNGR_Status ret_val = PWR_MNGR_GEN_ERROR;
  int32_t idx;

  if (!is_initialized) return (PWR_MNGR_NOT_INIT);

  if ((gpio_id < MCU_GPIO_ID_01) || (gpio_id >= MCU_GPIO_ID_MAX) || (level < 0) || (level > 1))
    return (PWR_MNGR_GEN_ERROR);

  PWR_MNGR_LOCK(&pwr_mngr_mtx);
  for (idx = 0; idx < PWR_MNGR_MON_GPIO_MAX_ENTRY; idx++) { /* replace */
    if (mon_gpio_list.entry[idx].gpio_num == gpio_id) {
      mon_gpio_list.entry[idx].pol_level = level;
      ret_val = PWR_MNGR_OK;
      PWR_MNGR_LOG(3, "Replace idx %ld port %ld set %ld\n", idx, mon_gpio_list.entry[idx].gpio_num,
                   mon_gpio_list.entry[idx].pol_level);
      break;
    }
  }

  if (ret_val != 0) { /* new */
    for (idx = 0; idx < PWR_MNGR_MON_GPIO_MAX_ENTRY; idx++) {
      if (mon_gpio_list.entry[idx].gpio_num == 255) {
        mon_gpio_list.entry[idx].gpio_num = gpio_id;
        mon_gpio_list.entry[idx].pol_level = level;
        ret_val = PWR_MNGR_OK;
        PWR_MNGR_LOG(3, "Add idx %ld port %ld set %ld\n", idx, mon_gpio_list.entry[idx].gpio_num,
                     mon_gpio_list.entry[idx].pol_level);
        break;
      }
    }
  }
  PWR_MNGR_UNLOCK(&pwr_mngr_mtx);

  return ret_val;
}

/*-----------------------------------------------------------------------------
 * PWR_MNGR_Status pwr_mngr_del_monitor_io(GPIO_Id gpio_id)
 * PURPOSE: This function would used to remove the gpio_num from mointored list.
 * PARAMs:
 *      INPUT:
 *          gpio_id
 *      OUTPUT: None
 * RETURN: PWR_MNGR_Status.
 *-----------------------------------------------------------------------------
 */
PWR_MNGR_Status pwr_mngr_del_monitor_io(GPIO_Id gpio_id) { /* Not allow duplicate entry */
  PWR_MNGR_Status ret_val = PWR_MNGR_GEN_ERROR;
  int32_t idx;

  if (!is_initialized) return (PWR_MNGR_NOT_INIT);

  if ((gpio_id < MCU_GPIO_ID_01) || (gpio_id >= MCU_GPIO_ID_MAX)) return (PWR_MNGR_GEN_ERROR);

  PWR_MNGR_LOCK(&pwr_mngr_mtx);
  for (idx = 0; idx < PWR_MNGR_MON_GPIO_MAX_ENTRY; idx++) {
    if (mon_gpio_list.entry[idx].gpio_num == gpio_id) {
      // found, so delete it, set to default gpio_id 255
      mon_gpio_list.entry[idx].gpio_num = 255;
      ret_val = PWR_MNGR_OK;
      break;
    }
  }
  PWR_MNGR_UNLOCK(&pwr_mngr_mtx);

  return ret_val;
}
