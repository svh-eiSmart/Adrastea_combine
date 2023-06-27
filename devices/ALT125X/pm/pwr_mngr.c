/****************************************************************************
 *
 *  (c) copyright 2019 Altair Semiconductor, Ltd. All rights reserved.
 *
 *  This software, in source or object form (the "Software"), is the
 *  property of Altair Semiconductor Ltd. (the "Company") and/or its
 *  licensors, which have all right, title and interest therein, You
 *  may use the Software only in  accordance with the terms of written
 *  license agreement between you and the Company (the "License").
 *  Except as expressly stated in the License, the Company grants no
 *  licenses by implication, estoppel, or otherwise. If you are not
 *  aware of or do not agree to the License terms, you may not use,
 *  copy or modify the Software. You may use the source code of the
 *  Software only for your internal purposes and may not distribute the
 *  source code of the Software, any part thereof, or any derivative work
 *  thereof, to any third party, except pursuant to the Company's prior
 *  written consent.
 *  The Software is the confidential information of the Company.
 *
 ****************************************************************************/

/* Standard includes. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "newlibPort.h"
#include "mcu2pmp_msg.h"
#include "pmp2mcu_msg.h"
#include "atomic_counter.h"
#include "gpio.h"
#include "gpio_wakeup.h"
#include "pwr_mngr.h"
#include "pwr_mngr_api.h"
#include "gpio_wakeup_utils.h"
#include "if_mngr.h"
#include "pmp_agent.h"
#include "125X_mcu.h"
#include "hifc_api.h"
#include "sleep_mngr.h"
#include "sleep_notify.h"
#include "wdt.h"

typedef struct _pwrMonGpioActiveEntry {
  int pin;
  int monPolLevel;
} pwrMonGpioActiveEntry;

typedef struct _pwrMonGpioActiveDB {
  pwrMonGpioActiveEntry entry[PWR_MON_GPIO_MAX_ENTRY];
} pwrMonGpioActiveDB;

typedef struct _gpmsection_s {
  uint32_t magic;
  uint32_t crc;  /* CRC32 over data bytes    */
  char data[16]; /* data        */
} retainSec_t;

static pwr_param_config_t gPwr_conf = {0};
static pwr_counters_t gPwr_counters = {0};
static pm_params_t pm_params = {0};

static pwrMonGpioActiveDB gMonGpioActDB = {0};
static pwr_wakeup_stat_t gPwr_stat;
static int gAllow2Sleep = 0;

static unsigned int gIntr_en_tab[64];
static unsigned int gBackup_clk_reg = 0;

/* UART inactive time */
static unsigned int uart_inactivity_time = 5; /* in s */
extern volatile unsigned int last_uart_interupt;

static uint32_t sleep_cnt;
static uint32_t enter_sleep;

#define __GPMSEC__ __attribute__((section("GPMdata")))
retainSec_t gRetainSec __GPMSEC__;

/* add a mark to retention memory for stateful sleep */
void mark_stateful_sleep(void) {
  gRetainSec.magic = 0xdeadbeef;
  gRetainSec.crc = 0x1;
  strncpy(gRetainSec.data, "StaFul-retM", sizeof(gRetainSec.data) - 1);
}

/*-----------------------------------------------------------------------------
 * int pwr_set_debug_mode(int en_debug)
 * PURPOSE: This function would switch on/off power-manager debug mode.
 * PARAMs:
 *      INPUT:  en_debug (1:enable / 0:disable)
 *      OUTPUT: None
 * RETURN:  error code. 0-success; other-fail.
 *-----------------------------------------------------------------------------
 */
int pwr_set_debug_mode(int en_debug) {
  int ret_val = 0;

  if (en_debug == 1) {
    pm_params.sleep_manager_counters_enable = 1;
  } else if (en_debug == 0) {
    pm_params.sleep_manager_counters_enable = 0;
  } else {
    ret_val = (-1);
  }

  return ret_val;
}

/*-----------------------------------------------------------------------------
 * void send_sleep_request(pwr_param_config_t *pwr_config)
 * PURPOSE: This function would send go-to-sleep to PMP.
 * PARAMs:
 *      INPUT:  pwr_config - pointer of pwr_param_config_t.
 *      OUTPUT: None
 * RETURN:  None
 *-----------------------------------------------------------------------------
 */
static void send_sleep_request(pwr_param_config_t *pwr_config) {
  mcu2pmp_mbox_msg_t sent_msg;

  memset(&sent_msg, 0x0, sizeof(mcu2pmp_mbox_msg_t));
  sent_msg.header.msgType = MCU2PMP_MSG_GOING_TO_SLEEP;

  switch (pwr_config->mode) {
    case PWR_MODE_STOP:
      sent_msg.body.goingToSleep.sleepType = MCU_STOP;
      break;
    case PWR_MODE_STANDBY:
      sent_msg.body.goingToSleep.sleepType = MCU_STANDBY;
      break;
    case PWR_MODE_SHUTDOWN:
      sent_msg.body.goingToSleep.sleepType = MCU_SHUTDOWN;
      break;
    default:
      printf(" Error: Not support sleep mode %d\n\r", pwr_config->mode);
      return;
  }
  sent_msg.body.goingToSleep.sleepDuration = pwr_config->duration;
  sent_msg.body.goingToSleep.memRetentionSecIdList =
      (gPwr_conf.memRetentionSecIdList & PWR_CON_RETENTION_SEC_MASK_ALL);

  /* Note: shall confirm the request is from IDLE task. */
  idle_task_send_msg_to_pmp(&sent_msg);
}

/*-----------------------------------------------------------------------------
 * void pm_conf_init(void)
 * PURPOSE: This function would initialize default setting of power manager.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  None
 *-----------------------------------------------------------------------------
 */
void pm_conf_init(void) {
  int idx;

  memset(&gPwr_conf, 0x0, sizeof(pwr_param_config_t));
  gPwr_conf.duration = 0;  // 0: infinite; unit:ms
  gPwr_conf.memRetentionSecIdList = (PWR_CON_RETENTION_SEC_MASK_ALL);
  gPwr_conf.mode = PWR_MODE_SLEEP;
  gPwr_conf.enableSleep = 0;  // 1: enable
  gAllow2Sleep = gPwr_conf.enableSleep;

  memset(&gMonGpioActDB, 0x0, sizeof(pwrMonGpioActiveDB));
  memset(&pm_params, 0x0, sizeof(pm_params_t));

  for (idx = 0; idx < PWR_MON_GPIO_MAX_ENTRY; idx++) gMonGpioActDB.entry[idx].pin = 255;
}

/*-----------------------------------------------------------------------------
 * void pm_sleep_enable(void)
 * PURPOSE: This function would enable MCU sleep.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  None
 *-----------------------------------------------------------------------------
 */
void pm_sleep_enable(void) {
  gPwr_conf.enableSleep = 1;
  gAllow2Sleep = 1;
}

/*-----------------------------------------------------------------------------
 * void pm_sleep_disable(void)
 * PURPOSE: This function would disable MCU sleep.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  None
 *-----------------------------------------------------------------------------
 */
void pm_sleep_disable(void) {
  gPwr_conf.enableSleep = 0;
  gAllow2Sleep = 0;
  gPwr_conf.mode = PWR_MODE_SLEEP;
}

/*-----------------------------------------------------------------------------
 * int pm_get_sleep_en(void)
 * PURPOSE: This function would get sleep setting.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  return value: 1 enable; 0 diable.
 *-----------------------------------------------------------------------------
 */
int pm_get_sleep_en(void) { return gPwr_conf.enableSleep; }

/*-----------------------------------------------------------------------------
 * int pm_set_sleep_en(int set_val)
 * PURPOSE: This function would set sleep setting.
 * PARAMs:
 *      INPUT:  set_val - 1:enable; 0:disable
 *      OUTPUT: None
 * RETURN:  error code. 0-success; other-fail
 *-----------------------------------------------------------------------------
 */
int pm_set_sleep_en(int set_val) {
  int ret_val = 0;

  if (set_val == 1)
    pm_sleep_enable();
  else if (set_val == 0)
    pm_sleep_disable();
  else
    ret_val = -1;

  return ret_val;
}

/*-----------------------------------------------------------------------------
 * int pwr_conf_param_set(power_mode_e mode, unsigned int duration)
 * PURPOSE: This function would set power mode and duration.
 * PARAMs:
 *      INPUT: powr_mode, sleep_duration (unit: ms).
 *      OUTPUT: None
 * RETURN:  error code. 0-success; other-fail
 *-----------------------------------------------------------------------------
 */
int pwr_conf_param_set(power_mode_e mode, unsigned int duration) {
  gPwr_conf.mode = mode;
  gPwr_conf.duration = duration;

  return 0;
}

/*-----------------------------------------------------------------------------
 * power_mode_e pwr_conf_param_get_mode(void)
 * PURPOSE: This function would get current power mode setting.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  power_mode_e mode.
 *-----------------------------------------------------------------------------
 */
power_mode_e pwr_conf_param_get_mode(void) { return gPwr_conf.mode; }

/*-----------------------------------------------------------------------------
 * int pwr_conf_param_set_inactivity_time(unsigned int inact_time)
 * PURPOSE: This function would set UART inactivity time.
 * PARAMs:
 *      INPUT:  unsigned int inact_time (unit: second)
 *      OUTPUT: None
 * RETURN:  error code. 0-success; other-fail
 *-----------------------------------------------------------------------------
 */
int pwr_conf_param_set_inactivity_time(unsigned int inact_time) {
  uart_inactivity_time = inact_time;
  return 0;
}

/*-----------------------------------------------------------------------------
 * void update_sleep_manager_counter(unsigned long sync_mask, unsigned long async_mask)
 * PURPOSE: This function would set UART inactivity time.
 * PARAMs:
 *      INPUT:  unsigned int inact_time (unit: second)
 *      OUTPUT: None
 * RETURN:  error code. 0-success; other-fail
 *-----------------------------------------------------------------------------
 */
void update_sleep_manager_counter(unsigned long sync_mask, unsigned long async_mask) {
  int i = 0;

  if (sync_mask > 0) {
    for (i = 1; i < SMNGR_RANGE; i++) {
      if ((sync_mask >> i) & 1L) {
        gPwr_counters.sleep_manager_devices[i]++;
      }
    }
  }

  if (async_mask > 0) {
    gPwr_counters.sleep_manager_devices[async_mask]++;
  }
}

/*-----------------------------------------------------------------------------
 * int pwr_check_allow2sleep(void)
 * PURPOSE: This function would check if meets the conditions of allow-to-sleep.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  status code.
 *-----------------------------------------------------------------------------
 */
int pwr_check_allow2sleep(void) {
  unsigned int sync_mask = 0, async_mask = 0;

  if (gAllow2Sleep == 0) {
    gPwr_counters.sleep_disable++;
    return 0;
  }

  if (pwr_check_uart_inactive_time() == 1) {
    gPwr_counters.uart_incativity_time++;
    return 0;
  }

  if (smngr_is_dev_busy(&sync_mask, &async_mask)) {
    gPwr_counters.sleep_manager++;

    /*only if sleep manager counters is enable - then look for it*/
    if (pm_params.sleep_manager_counters_enable) {
      update_sleep_manager_counter(sync_mask, async_mask);
    }
    return 0;
  }

  if (get_if_state() != IfDown) {
    gPwr_counters.hifc_busy++;
    return 0;
  }

  if (pwr_check_port_busy()) {
    gPwr_counters.mon_gpio_busy++;
    return 0;
  }

  return 1;
}

/*-----------------------------------------------------------------------------
 * void pwr_pre_sleep_process (uint32_t idle_time)
 * PURPOSE: This function would run pre-sleep process.
 * PARAMs:
 *      INPUT:  uint32_t idle_time
 *      OUTPUT: None
 * RETURN:  status code.
 *-----------------------------------------------------------------------------
 */
void pwr_pre_sleep_process(uint32_t idle_time) {
  /*Notify register callback of suspending state*/
  if (idle_time > 0) {
    sleep_notify(SUSPENDING);

    sleep_cnt++;
    enter_sleep = 1;
    watchdog_sleep_pre_process();
  }

  return;
}

/*-----------------------------------------------------------------------------
 * void pwr_post_sleep_process (uint32_t idle_time)
 * PURPOSE: This function would run post-sleep process.
 * PARAMs:
 *      INPUT:  uint32_t idle_time
 *      OUTPUT: None
 * RETURN:  status code.
 *-----------------------------------------------------------------------------
 */
void pwr_post_sleep_process(uint32_t idle_time) {
  if (enter_sleep == 1) {
    watchdog_sleep_post_process();
  }

  /*Notify register callback of resuming state*/
  sleep_notify(RESUMING);

  return;
}

/*-----------------------------------------------------------------------------
 * pm_err_code_e pwr_initWakeUpPin(int gpio_number)
 * PURPOSE: This function would initialize and configure a wakeup pin.
 * PARAMs:
 *      INPUT:  gpio_number - GPIO number.
 *      OUTPUT: None
 * RETURN:  pm_err_code_e.
 *-----------------------------------------------------------------------------
 */
pm_err_code_e pwr_initWakeUpPin(int gpio_number) {
  gpio_wakeup_struct_t wkup_struct = {0};
  pm_err_code_e ret_val = PWR_ERR_NO_ERROR;
  int res;

  wkup_struct.gpio_pin = gpio_number;
  wkup_struct.debounce_max_val = 1;
  wkup_struct.clk = GPIO_WAKEUP_RTC_CLK;
  wkup_struct.debounce_en = 1;
  wkup_struct.int_en = 1;
  wkup_struct.mode = GPIO_WAKEUP_LEVEL_POL_HIGH;
  wkup_struct.wakeup_en = 1;
  res = gpio_wakeup_add_io(&wkup_struct);
  if (res == 0) {
    printf("Pin %d have been registered.\n\r", gpio_number);
  } else {
    printf("Can't register pin %d err_code %d.\n\r", gpio_number, res);
  }

  return ret_val;
}

/*-----------------------------------------------------------------------------
 * void pwr_wakeup_gpio_init(void)
 * PURPOSE: This function would initialize wakeup_gpio.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  pm_err_code_e.
 *-----------------------------------------------------------------------------
 */
pm_err_code_e pwr_wakeup_gpio_init(void) {
  eIfMngrIf wakeup_if;
  pm_wakeup_cfg_t wakeup_cfg;
  for (wakeup_if = IF_MNGR_WAKEUP01; wakeup_if <= IF_MNGR_WAKEUP04; wakeup_if++) {
    if (if_mngr_load_defconfig(wakeup_if, &wakeup_cfg) == IF_MNGR_SUCCESS) {
      PWR_EnableWakeUpPin(wakeup_cfg.pin_set, wakeup_cfg.pin_pol);
    }
  }
  return PWR_ERR_NO_ERROR;
}

/*-----------------------------------------------------------------------------
 * void pwr_check_port_busy(void)
 * PURPOSE: This function would check if specified GPIO port is busy.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  0:not busy; 1:busy.
 *-----------------------------------------------------------------------------
 */
int pwr_check_port_busy(void) {
  int idx, gpio_val = 0;

  for (idx = 0; idx < PWR_MON_GPIO_MAX_ENTRY; idx++) {
    if ((gMonGpioActDB.entry[idx].pin >= MCU_GPIO_ID_01) &&
        (gMonGpioActDB.entry[idx].pin < MCU_GPIO_ID_NUM)) {
      if ((gpio_set_direction_input((eMcuGpioIds)gMonGpioActDB.entry[idx].pin) ==
           GPIO_RET_SUCCESS) &&
          (gpio_get_value((eMcuGpioIds)gMonGpioActDB.entry[idx].pin, &gpio_val) ==
           GPIO_RET_SUCCESS) &&
          (gpio_val == gMonGpioActDB.entry[idx].monPolLevel)) {
        /* specified gpio is busy */
        return 1;
      }
    }
  }
  return 0;
}

/*-----------------------------------------------------------------------------
 * int pwr_check_uart_inactive_time(void)
 * PURPOSE: This function would check if UART inactivity time still less than
 *          upper limitation.
 *          If yes, block this cycle to sleep.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  0: over upper limitation, 1: less than setting.
 *-----------------------------------------------------------------------------
 */
int pwr_check_uart_inactive_time(void) {
  unsigned int now = xTaskGetTickCount() / portTICK_PERIOD_MS;
  unsigned int uart_interupt_delta = 0;
  int ret_val = 0;

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
 * void pwr_refresh_uart_inactive_time(void)
 * PURPOSE: This function would refresh last UART interrupt time.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  None
 *-----------------------------------------------------------------------------
 */
void pwr_refresh_uart_inactive_time(void) {
  volatile unsigned int now = xTaskGetTickCount() / portTICK_PERIOD_MS;

  last_uart_interupt = now;
}

/*-----------------------------------------------------------------------------
 * void pwr_stat_update_last_wakeup_info(pwr_wakeup_cause_e cause, unsigned int duration_left)
 * PURPOSE: This function would update the data of last wakeup.
 * PARAMs:
 *      INPUT:  pwr_wakeup_cause_e cause - wakeup cause.
 *              unsigned int duration_left - sleep duration left.
 *      OUTPUT: None
 * RETURN:  None
 *-----------------------------------------------------------------------------
 */
void pwr_stat_update_last_wakeup_info(pwr_wakeup_cause_e cause, unsigned int duration_left) {
  gPwr_stat.last_cause = cause;
  gPwr_stat.last_dur_left = duration_left;
  gPwr_stat.counter++;

  if ((gPwr_stat.wakeup_state == PWR_WAKEUP_STATELESS) &&
      (gPwr_stat.last_cause == PWR_WAKEUP_CAUSE_NONE) && (gPwr_stat.last_dur_left == 0) &&
      (gPwr_stat.counter == 0)) {
    gPwr_stat.boot_type = PWR_DEV_COLD_BOOT;
  } else {
    gPwr_stat.boot_type = PWR_DEV_WARM_BOOT;
  }
}

/*-----------------------------------------------------------------------------
 * void pwr_stat_clr_sleep_statistics(void)
 * PURPOSE: This function would reset current sleep statistics.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  None
 *-----------------------------------------------------------------------------
 */
void pwr_stat_clr_sleep_statistics(void) { gPwr_stat.counter = 0; }

/*-----------------------------------------------------------------------------
 * int pwr_stat_get_sleep_statistics(pwr_wakeup_stat_t *pwr_stat)
 * PURPOSE: This function would get sleep statistics of MCU.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: pointer of pwr_wakeup_stat_t
 * RETURN:  error code. 0-success; other-fail
 *-----------------------------------------------------------------------------
 */
int pwr_stat_get_sleep_statistics(pwr_wakeup_stat_t *pwr_stat) {
  if (pwr_stat == NULL) return (-1);

  memcpy(pwr_stat, &gPwr_stat, sizeof(pwr_wakeup_stat_t));

  return 0;
}

/*-----------------------------------------------------------------------------
 * pwr_dev_boot_type_e pwr_stat_get_dev_boot_type(void)
 * PURPOSE: This function would get device boot type.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  enumeration value of device boot type.
 * Note:  it can't distinguish the boot is triggered by reset button.
 *-----------------------------------------------------------------------------
 */
pwr_dev_boot_type_e pwr_stat_get_dev_boot_type(void) { return gPwr_stat.boot_type; }

/*-----------------------------------------------------------------------------
 * void pwr_store_set_gatedclks_before_sleep(void)
 * PURPOSE: This function would handle gated clocks IO before sleep.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  None
 *-----------------------------------------------------------------------------
 */
void pwr_gatedclks_handle_before_sleep(void) {
  gBackup_clk_reg = REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN);
}

/*-----------------------------------------------------------------------------
 * void pwr_gatedclks_handle_after_sleep(void)
 * PURPOSE: This function would handle gated clocks IO after sleep.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  None
 *-----------------------------------------------------------------------------
 */
void pwr_gatedclks_handle_after_sleep(void) {
  REGISTER(MCU_CLK_CONTROL_MCU_CLK_REQ_EN) = gBackup_clk_reg;
}

/*-----------------------------------------------------------------------------
 * void gpm_boot_probe(void)
 * PURPOSE: This function would probe the boot type.
 *        Now this scheme is thru checking a magic string stil be kept in retention
 *        memory after sleep.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  None.
 *-----------------------------------------------------------------------------
 */
void gpm_boot_probe(void) {
  /* if (magic # is match), retention data is kept */
  if (gRetainSec.magic == 0xdeadbeef) {
    gPwr_stat.wakeup_state = (PWR_WAKEUP_STATEFUL);
  } else { /* else, retention data is not kept */
    gPwr_stat.wakeup_state = (PWR_WAKEUP_STATELESS);
  }

  /* clear magic from retention memory */
  memset(&gRetainSec, 0x0, sizeof(retainSec_t));
}

/*-----------------------------------------------------------------------------
 * int pwr_get_sleep_counters(pwr_counters_t *pwr_counters)
 * PURPOSE: This function would get sleep counters of MCU.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: pointer of pwr_counters_t
 * RETURN:  error code. 0-success; other-fail
 *-----------------------------------------------------------------------------
 */
int pwr_get_sleep_counters(pwr_counters_t *pwr_counters) {
  if (pwr_counters == NULL) return (-1);

  memcpy(pwr_counters, &gPwr_counters, sizeof(pwr_counters_t));

  return 0;
}

/*-----------------------------------------------------------------------------
 * int pwr_clr_sleep_counters(pwr_counters_t *pwr_counters)
 * PURPOSE: This function would clear sleep counters of MCU.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  error code. 0-success; other-fail
 *-----------------------------------------------------------------------------
 */
int pwr_clr_sleep_counters(void) {
  memset(&gPwr_counters, 0x0, sizeof(pwr_counters_t));
  return 0;
}
/* ----------------------------- External PM APIs ----------------------------- */
/*-----------------------------------------------------------------------------
 * pm_err_code_e PWR_EnterStopMode(pwr_stop_config_t *pwr_stop_param)
 * PURPOSE: This function would configure to Stop mode.
 * PARAMs:
 *      INPUT:  pwr_stop_config_t *pwr_stop_param
 *      OUTPUT: None
 * RETURN:  pm_err_code_e.
 *-----------------------------------------------------------------------------
 */
pm_err_code_e PWR_EnterStopMode(pwr_stop_config_t *pwr_stop_param) {
  pwr_param_config_t pwr_config;

  gPwr_stat.wakeup_state = (PWR_WAKEUP_STATEFUL);

  memset(&pwr_config, 0x0, sizeof(pwr_param_config_t));
  pwr_config.mode = PWR_MODE_STOP;
  pwr_config.duration = pwr_stop_param->duration;
  send_sleep_request(&pwr_config);

  __asm volatile("dsb" ::: "memory");
  __asm volatile("wfi");
  __asm volatile("isb");

  return PWR_ERR_NO_ERROR;
}

static void dump_active_interrupts(void) {
  unsigned int idx;

  printf("+++++ Pend INT >>");
  for (idx = 0; idx < 64; idx++) {
    if (NVIC_GetPendingIRQ((IRQn_Type)idx) == 1) {
      printf("[%d]", idx);
    }
  }
  printf("\r\n");
  return;
}
/*-----------------------------------------------------------------------------
 * pm_err_code_e PWR_EnterStandbyMode(pwr_standby_config_t *pwr_standby_param)
 * PURPOSE: This function would configure to Standby mode.
 * PARAMs:
 *      INPUT:  pwr_standby_config_t *pwr_standby_param
 *      OUTPUT: None
 * RETURN:  pm_err_code_e.
 *-----------------------------------------------------------------------------
 */
pm_err_code_e PWR_EnterStandbyMode(pwr_standby_config_t *pwr_standby_param) {
  pwr_param_config_t pwr_config;
  int i = 0;

  /* marks stateful sleep */
  mark_stateful_sleep();

  memset(&pwr_config, 0x0, sizeof(pwr_param_config_t));
  pwr_config.mode = PWR_MODE_STANDBY;
  pwr_config.duration = pwr_standby_param->duration;
  pwr_config.memRetentionSecIdList = pwr_standby_param->memRetentionSecIdList;
  send_sleep_request(&pwr_config);

  // waiting for pmp handling
  while (1) {
    __asm volatile("dsb" ::: "memory");
    __asm volatile("wfi");
    __asm volatile("isb");
    if (i >= 1) {
      /* we should never get here, print warning and return error */
      printf("Warning, fall out of standby. ICSR[%lx]\r\n", SCB->ICSR);
      dump_active_interrupts();
    }
    i++;
  }

  return PWR_ERR_GEN_ERROR;  // Unreachable, still keep it for API compatible
}

/*-----------------------------------------------------------------------------
 * pm_err_code_e PWR_EnterShutdownMode (pwr_shutdown_config_t *pwr_shutdown_param)
 * PURPOSE: This function would configure to Shutdown mode.
 * PARAMs:
 *      INPUT:  pwr_shutdown_config_t *pwr_shutdown_param
 *      OUTPUT: None
 * RETURN:  pm_err_code_e.
 *-----------------------------------------------------------------------------
 */
pm_err_code_e PWR_EnterShutdownMode(pwr_shutdown_config_t *pwr_shutdown_param) {
  pwr_param_config_t pwr_config;
  int i = 0;

  memset(&pwr_config, 0x0, sizeof(pwr_param_config_t));
  pwr_config.mode = PWR_MODE_SHUTDOWN;
  pwr_config.duration = pwr_shutdown_param->duration;
  send_sleep_request(&pwr_config);

  // waiting for pmp handling
  while (1) {
    __asm volatile("dsb" ::: "memory");
    __asm volatile("wfi");
    __asm volatile("isb");
    if (i >= 1) {
      /* we should never get here, print warning and return error */
      printf("Warning, fall out of shutdown. ICSR[%lx]\r\n", SCB->ICSR);
      dump_active_interrupts();
    }
    i++;
  }

  return PWR_ERR_GEN_ERROR;  // Unreachable, still keep it for API compatible
}

/*-----------------------------------------------------------------------------
 * pm_err_code_e PWR_EnterSleep(void)
 * PURPOSE: This function would send allow-to-sleep message to PMP.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  pm_err_code_e.
 *-----------------------------------------------------------------------------
 */
pm_err_code_e PWR_EnterSleep(void) {
  if (gPwr_conf.enableSleep == 0) return PWR_ERR_GEN_ERROR;

  if (gPwr_conf.mode == PWR_MODE_STOP) {
    pwr_stop_config_t st_pwr_stop;

    memset(&st_pwr_stop, 0x0, sizeof(pwr_stop_config_t));
    // printf("Go to Stop mode...\n\r");
    st_pwr_stop.duration = gPwr_conf.duration;
    if (PWR_EnterStopMode(&st_pwr_stop) != PWR_ERR_NO_ERROR) return PWR_ERR_GEN_ERROR;
  } else if (gPwr_conf.mode == PWR_MODE_STANDBY) {
    pwr_standby_config_t st_pwr_standby;

    memset(&st_pwr_standby, 0x0, sizeof(pwr_standby_config_t));
    // printf("Go to Standby mode...\n\r");
    st_pwr_standby.duration = gPwr_conf.duration;
    if (PWR_EnterStandbyMode(&st_pwr_standby) != PWR_ERR_NO_ERROR) return PWR_ERR_GEN_ERROR;
  } else if (gPwr_conf.mode == PWR_MODE_SHUTDOWN) {
    pwr_shutdown_config_t st_pwr_shutdown;

    memset(&st_pwr_shutdown, 0x0, sizeof(pwr_shutdown_config_t));
    // printf("Go to Shutdown mode...\n\r");
    st_pwr_shutdown.duration = gPwr_conf.duration;
    if (PWR_EnterShutdownMode(&st_pwr_shutdown) != PWR_ERR_NO_ERROR) return PWR_ERR_GEN_ERROR;
  } else {
    return PWR_ERR_GEN_ERROR;
  }

  return PWR_ERR_NO_ERROR;
}

/*-----------------------------------------------------------------------------
 * pm_err_code_e PWR_EnableWakeUpPin(unsigned int pinNum, pwr_wakeup_pin_pol_e polarity)
 * PURPOSE: This function would enable WakeUp Pin.
 * PARAMs:
 *      INPUT:  pinNum - GPIO Pin number.
 *              polarity.
 *      OUTPUT: None
 * RETURN:  pm_err_code_e.
 *-----------------------------------------------------------------------------
 */
pm_err_code_e PWR_EnableWakeUpPin(unsigned int pinNum, pwr_wakeup_pin_pol_e polarity) {
  gpio_wakeup_struct_t wkup_struct = {0}, temp_struct = {0};
  pm_err_code_e ret_val = PWR_ERR_NO_ERROR;
  int res;

  wkup_struct.gpio_pin = pinNum;
  wkup_struct.debounce_max_val = 1;
  wkup_struct.clk = GPIO_WAKEUP_RTC_CLK;
  wkup_struct.debounce_en = 1;
  wkup_struct.int_en = 0;
  wkup_struct.mode = (gpio_wakeup_polarity_mode_e)polarity;
  wkup_struct.wakeup_en = 1;

  gpio_wakeup_get_setup(&temp_struct, pinNum, GPIO_WAKEUP_REGISTER_LOOKUP);
  if ((temp_struct.gpio_pin >= 0) && (temp_struct.mode == wkup_struct.mode) &&
      (temp_struct.debounce_en == wkup_struct.debounce_en) &&
      (temp_struct.wakeup_en == wkup_struct.wakeup_en)) {
    // if set value is the same, return OK
    return PWR_ERR_NO_ERROR;
  }

  res = gpio_wakeup_add_io(&wkup_struct);
  if (res != 0) {
    ret_val = PWR_ERR_GEN_ERROR;
  }

  return ret_val;
}

/*-----------------------------------------------------------------------------
 * pm_err_code_e PWR_DisableWakeUpPin(unsigned int pinNum)
 * PURPOSE: This function would disable WakeUp Pin.
 * PARAMs:
 *      INPUT:  pinNum - GPIO Pin number.
 *      OUTPUT: None
 * RETURN:  pm_err_code_e.
 *-----------------------------------------------------------------------------
 */
pm_err_code_e PWR_DisableWakeUpPin(unsigned int pinNum) {
  if (gpio_wakeup_delete_io(pinNum) != 0) return PWR_ERR_GEN_ERROR;

  return PWR_ERR_NO_ERROR;
}

/*-----------------------------------------------------------------------------
 * pm_err_code_e PWR_EnableSRAMContentRetention(unsigned int sec_id_mask)
 * PURPOSE: This function would enable SRAM Content Retention sections.
 * PARAMs:
 *      INPUT:  sec_id_mask - section-id mask.
 *      OUTPUT: None
 * RETURN:  pm_err_code_e.
 *-----------------------------------------------------------------------------
 */
pm_err_code_e PWR_EnableSRAMContentRetention(unsigned int sec_id_mask) {
  /* check input */
#ifdef ALT1255
  if ((sec_id_mask != 0) && (sec_id_mask != PWR_CON_RETENTION_SEC_2)) {
    printf("Wrong Content Retention ID mask \n\r");
    return PWR_ERR_GEN_ERROR;
  }
#else
  if (sec_id_mask > PWR_CON_RETENTION_SEC_MASK_ALL) {
    printf("Wrong Content Retention ID mask \n\r");
    return PWR_ERR_GEN_ERROR;
  }
#endif
  gPwr_conf.memRetentionSecIdList = sec_id_mask;
  return PWR_ERR_NO_ERROR;
}

/*-----------------------------------------------------------------------------
 * pm_err_code_e PWR_Manager_Init(void)
 * PURPOSE: This function would configure and intialize Power Manager.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  pm_err_code_e.
 *-----------------------------------------------------------------------------
 */
pm_err_code_e PWR_Manager_Init(void) {
  memset(&gPwr_stat, 0, sizeof(pwr_wakeup_stat_t));
  gpm_boot_probe();
  pm_conf_init();
  smngr_init();
  sleep_notify_init();
  gpio_wakeup_init();
  pwr_stat_clr_sleep_statistics();
  pwr_clr_sleep_counters();

  if (pwr_wakeup_gpio_init() != PWR_ERR_NO_ERROR) {
    return PWR_ERR_GEN_ERROR;
  }

  return PWR_ERR_NO_ERROR;
}

/*-----------------------------------------------------------------------------
 * void pwr_mask_interrupts(power_mode_e pwr_mode)
 * PURPOSE: This function would disable interrupts before entering sleep.
 * PARAMs:
 *      INPUT:  power_mode_e mode
 *      OUTPUT: None
 * RETURN:  None.
 *-----------------------------------------------------------------------------
 */
void pwr_mask_interrupts(power_mode_e mode) {
  unsigned int cnt = 0, idx;

  memset(gIntr_en_tab, 0x0, sizeof(gIntr_en_tab));
  for (idx = 0; idx < 64; idx++) {
    if ((idx == ATMCTR_MAILBOX_IRQn) &&
        ((mode != PWR_MODE_STANDBY) && (mode != PWR_MODE_SHUTDOWN)))  // not mask pmp2mcu AC
      continue;

    if (NVIC_GetEnableIRQ((IRQn_Type)idx) == 1) {
      NVIC_DisableIRQ((IRQn_Type)idx);

      gIntr_en_tab[cnt] = idx;
      cnt++;
    }
  }
  gIntr_en_tab[cnt] = 255;

  return;
}

/*-----------------------------------------------------------------------------
 * void pwr_restore_interrupts(void)
 * PURPOSE: This function would restore interrupts disabled for entering to sleep.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  None.
 *-----------------------------------------------------------------------------
 */
void pwr_restore_interrupts(void) {
  unsigned int idx;

  for (idx = 0; idx < 64; idx++) {
    if (gIntr_en_tab[idx] >= 64) break;
    NVIC_EnableIRQ((IRQn_Type)gIntr_en_tab[idx]);
  }

  return;
}

/*-----------------------------------------------------------------------------
 * pm_err_code_e PWR_Set_Monitored_Pin(int pinNum, int level)
 * PURPOSE: This function would set the pin and level for stop MCU go to sleep.
 * PARAMs:
 *      INPUT:
 *          pinNum
 *          level: 1 or 0;
 *      OUTPUT: None
 * RETURN: pm_err_code_e.
 *-----------------------------------------------------------------------------
 */
pm_err_code_e PWR_Set_Monitored_Pin(int pinNum, int level) { /* Not allow duplicate entry */
  pm_err_code_e ret_val = PWR_ERR_GEN_ERROR;
  int idx;

  if ((pinNum < MCU_GPIO_ID_01) || (pinNum >= MCU_GPIO_ID_NUM) || (level < 0) || (level > 1))
    return (PWR_ERR_GEN_ERROR);

  for (idx = 0; idx < PWR_MON_GPIO_MAX_ENTRY; idx++) { /* replace */
    if (gMonGpioActDB.entry[idx].pin == pinNum) {
      gMonGpioActDB.entry[idx].monPolLevel = level;
      ret_val = PWR_ERR_NO_ERROR;
      // printf(" Replace idx %d port %d set %d\n\r", idx, gMonGpioActDB.entry[idx].pin,
      // gMonGpioActDB.entry[idx].monPolLevel);
      break;
    }
  }

  if (ret_val != 0) { /* new */
    for (idx = 0; idx < PWR_MON_GPIO_MAX_ENTRY; idx++) {
      if (gMonGpioActDB.entry[idx].pin == 255) {
        gMonGpioActDB.entry[idx].pin = pinNum;
        gMonGpioActDB.entry[idx].monPolLevel = level;
        ret_val = PWR_ERR_NO_ERROR;
        // printf(" Add idx %d port %d set %d\n\r", idx, gMonGpioActDB.entry[idx].pin,
        // gMonGpioActDB.entry[idx].monPolLevel);
        break;
      }
    }
  }

  return ret_val;
}

/*-----------------------------------------------------------------------------
 * pm_err_code_e PWR_Del_Monitored_Pin(int pinNum)
 * PURPOSE: This function would used to remove the pin from mointored list.
 * PARAMs:
 *      INPUT:
 *          pinNum
 *      OUTPUT: None
 * RETURN: pm_err_code_e.
 *-----------------------------------------------------------------------------
 */
pm_err_code_e PWR_Del_Monitored_Pin(int pinNum) { /* Not allow duplicate entry */
  pm_err_code_e ret_val = PWR_ERR_GEN_ERROR;
  int idx;

  if ((pinNum < MCU_GPIO_ID_01) || (pinNum >= MCU_GPIO_ID_NUM)) return (PWR_ERR_GEN_ERROR);

  for (idx = 0; idx < PWR_MON_GPIO_MAX_ENTRY; idx++) {
    if (gMonGpioActDB.entry[idx].pin == pinNum) {
      // found, so delete it, set to default pinNum 255
      // printf(" Delete idx %d port %d set %d\n\r", idx, gMonGpioActDB.entry[idx].pin,
      // gMonGpioActDB.entry[idx].monPolLevel);
      gMonGpioActDB.entry[idx].pin = 255;
      ret_val = PWR_ERR_NO_ERROR;
      break;
    }
  }

  return ret_val;
}
