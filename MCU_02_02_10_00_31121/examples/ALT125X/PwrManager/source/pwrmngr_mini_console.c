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

/* CMSIS-RTOS V2 includes. */
#include "cmsis_os2.h"

#include "assert.h"

/* Kernel includes. */
#include "newlibPort.h"
#include "DRV_ATOMIC_COUNTER.h"
#include "DRV_GPIO.h"
#include "DRV_IO.h"
#include "PIN.h"
#include "serial.h"
#include "pwr_mngr.h"
#include "DRV_PM.h"
#include "DRV_WATCHDOG.h"
#include "mini_console.h"
#include "kvpfs_api.h"
#include "hibernate.h"

#define PWR_KVP_BUFF_SIZE (64)

serial_handle sHandle_i = NULL;

void set_and_save2flash(char *kvp_key, char *val, int32_t save_to) {
  int32_t ret;
  char *ret_val = NULL;

  if (get_env(kvp_key, &ret_val) == 0) {
    if (!strcmp(ret_val, val)) {
      free(ret_val);
      return;  // already exist!
    }
  }
  free(ret_val);

  ret = set_env(kvp_key, val);
  if (ret != 0) {
    printf("fail to set key %s to %s.err:%ld\r\n", kvp_key, val, ret);
  } else {
    if (save_to == 0)  // to gpm
      ret = save_env_to_gpm();
    else  // to flash
      ret = save_env();
    if (ret != 0) {
      printf("fail to save to %ld (0:gpm ;otherwise:flash)- ret %ld \r\n", save_to, ret);
    }
  }
}

int32_t do_gpioWKUP2(char *s) {
  uint32_t pin_set = 0;
  char command[20], mode[20];
  int32_t argc = 0, ret_val = 0;
  DRV_PM_Status err_code;
  DRV_PM_WAKEUP_PinPolarity mode_val;
  char *tok, *strgp;

  strgp = s;
  tok = strsep(&strgp, " ");
  if (!tok) {
    return (-1);
  }
  pin_set = (uint32_t)strtoul(tok, NULL, 10);
  argc++;

  tok = strsep(&strgp, " ");
  if (!tok) {
    return (-1);
  }
  strncpy(command, tok, sizeof(command) - 1);
  argc++;

  tok = strsep(&strgp, " ");
  if (tok) {
    strncpy(mode, tok, sizeof(command) - 1);
    argc++;
  }

  if (argc == 2 || argc == 3) {
    if (!strcmp(command, "set")) {
      if (argc == 3) {
        if (!strcmp("h", mode)) {
          mode_val = DRV_PM_WAKEUP_LEVEL_POL_HIGH;
        } else if (!strcmp("l", mode)) {
          mode_val = DRV_PM_WAKEUP_LEVEL_POL_LOW;
        } else if (!strcmp("r", mode)) {
          mode_val = DRV_PM_WAKEUP_EDGE_RISING;
        } else if (!strcmp("f", mode)) {
          mode_val = DRV_PM_WAKEUP_EDGE_FALLING;
        } else if (!strcmp("rf", mode)) {
          mode_val = DRV_PM_WAKEUP_EDGE_RISING_FALLING;
        } else {
          printf("Wrong mode [%s]\r\n", mode);
          ret_val = -1;
        }

        if (ret_val != (-1)) {
          err_code = DRV_PM_EnableWakeUpPin(pin_set, mode_val);
          if (err_code == DRV_PM_OK) {
            printf("Pin %lu is registered.\r\n", pin_set);
          } else {
            printf("Can't register pin %lu err_code %d \r\n", pin_set, err_code);
          }
        }
      } else {
        printf("Incorrect amount of arguments\n");
        ret_val = -1;
      }
    } else if (!strcmp(command, "del")) {
      err_code = DRV_PM_DisableWakeUpPin(pin_set);
      if (err_code == DRV_PM_OK) {
        printf("Pin %lu is un-registered.\r\n", pin_set);
      } else {
        printf("Can't un-register pin %lu err_code %d \r\n", pin_set, err_code);
      }
    } else {
      ret_val = -1;
    }
  }

  return ret_val;
}

static void prvAtCmdTask(void *pvParameters) {
  char c;

  sHandle_i = serial_open(ACTIVE_UARTI0);
  assert(sHandle_i);

  while (1) {
    serial_read(sHandle_i, &c, 1);
    console_write(&c, 1);
  }
}

int32_t do_map(char *s) {
  char c;

  osThreadAttr_t attr = {0};

  attr.name = "Console";
  attr.stack_size = 512;
  attr.priority = configMAX_PRIORITIES - 1;

  osThreadId_t tid = osThreadNew(prvAtCmdTask, NULL, &attr);

  if (!tid) {
    printf("osThreadNew prvAtCmdTask failed\n");
    return -1;
  }

  printf("Open MAP CLI (Press Ctrl+D to exit)\r\n");
  do {
    console_read(&c, 1);
  
    if (c != 4) {
      serial_write(sHandle_i, &c, 1);
    }
  } while (c != 4);

  serial_close(sHandle_i);
  printf("MAP CLI Closed.\r\n");
  sHandle_i = NULL;

  osThreadTerminate(tid);

  return 0;
}

int32_t do_pwrMode(char *s) {
  PWR_MNGR_Configuation conf = {0};
  char command[20], buff[PWR_KVP_BUFF_SIZE + 1] = {0};
  int32_t argc = 0, ret_val = 0;
  uint32_t input_time = 0, duration = 0;
  char *tok, *strgp;

  strgp = s;
  tok = strsep(&strgp, " ");
  if (!tok) {
    return (-1);
  }
  strncpy(command, tok, sizeof(command) - 1);
  argc++;

  tok = strsep(&strgp, " ");
  if (tok) {
    input_time = (uint32_t)strtoul(tok, NULL, 10);
    argc++;
  }

  if (argc == 1) {
    duration = PWR_MNGR_DFT_SLEEP_DRUATION;
  } else {
    duration = input_time;
  }

  if (!strcmp(command, "stop")) {
    if (pwr_mngr_conf_set_mode(PWR_MNGR_MODE_STOP, duration) == PWR_MNGR_OK) {
      printf("Configured to Stop mode, duration %lu ms\r\n", duration);
    }
    else
      printf("Failed.\r\n");
  } else if (!strcmp(command, "standby")) {
    if (pwr_mngr_conf_set_mode(PWR_MNGR_MODE_STANDBY, duration) == PWR_MNGR_OK) {
      printf("Configured to Standby mode, duration %lu ms\r\n", duration);
    }
    else
      printf("Failed.\r\n");
  } else if (!strcmp(command, "shutdown")) {
    if (pwr_mngr_conf_set_mode(PWR_MNGR_MODE_SHUTDOWN, duration) == PWR_MNGR_OK) {
      printf("Configured to Shutdown mode, duration %lu ms\r\n", duration);
      if((duration != 0) && (duration < PWR_MNGR_SHUTDOWN_THRESHOLD))
        printf("Notice: MCU will not enter shutdown because duration %lu have to large than the threshold %u!\r\n", duration, PWR_MNGR_SHUTDOWN_THRESHOLD+1000);
    }
    else
      printf("Failed.\r\n");
  } else if (!strcmp(command, "show")) {
    pwr_mngr_conf_get_settings(&conf);
    switch (conf.mode) {
      case PWR_MNGR_MODE_SLEEP:
        printf("Current mode: SLEEP\r\n");
        break;
      case PWR_MNGR_MODE_STOP:
        printf("Current mode: STOP\r\n");
        break;
      case PWR_MNGR_MODE_STANDBY:
        printf("Current mode: STANDBY\r\n");
        break;
      case PWR_MNGR_MODE_SHUTDOWN:
        printf("Current mode: SHUTDOWN\r\n");
        break;
      default:
        printf("Unknown mode type[%d]\r\n", conf.mode);
        break;
    }
    return 0;
  } else {
    printf("Not support command %s.\r\n", command);
    ret_val = -1;
  }

  if (ret_val == 0) {  // set & save to flash
    set_and_save2flash("PWR_CONF_SLP_MODE", command, 1);

    memset(buff, 0, PWR_KVP_BUFF_SIZE);
    snprintf(buff, PWR_KVP_BUFF_SIZE, "%lu", duration);
    set_and_save2flash("PWR_CONF_SLP_DUR", buff, 1);
  }

  return ret_val;
}

/* Important Note: This test command is for OS-less test.
 * In FreeRTOS, power management should be developped in IDLE task. Or it will not stop
 * in ARM WFI  */
int32_t do_oslessStop(char *s) {
  DRV_PM_StopModeConfig stop_config;
  uint32_t duration = 0;
  int32_t argc = 0, ret_val = 0;
  char *tok, *strgp;

  // confirm OS sleep mode is off
  if (pwr_mngr_conf_get_sleep() == 1) {
    printf("Error: OS ware sleep is running!\r\n");
    return (-1);
  }

  // CLI params handling
  strgp = s;
  tok = strsep(&strgp, " ");
  if (!tok) {
    return (-1);
  }
  duration = (uint32_t)strtoul(tok, NULL, 10);
  argc++;

  memset(&stop_config, 0x0, sizeof(stop_config));
  stop_config.duration = duration;

  printf("going to sleep\r\n");
  // (1) disable interrupts
  __asm volatile("cpsid i" ::: "memory");

  DRV_PM_MaskInterrupts(DRV_PM_MODE_STOP);

  // (2) optional - add system specific pre sleep procedure here

  // (3) send sleep command to PMP
  DRV_PM_EnterStopMode(&stop_config);

  // (4) optional - add system specific post sleep procedure here

  // (5) enable interrupts
  DRV_PM_RestoreInterrupts(DRV_PM_MODE_STOP);

  __asm volatile("cpsie i" ::: "memory");

  printf("waking up from sleep\r\n");
  return ret_val;
}

/* Important Note: This test command is for OS-less test.
 * In FreeRTOS, power management should be developped in IDLE task. Or it will not stop
 * in ARM WFI  */
int32_t do_oslessStandby(char *s) {
  DRV_PM_StandbyModeConfig standby_config;
  uint32_t duration = 0;
  int32_t argc = 0, ret_val = 0;
  char *tok, *strgp;

  // confirm OS sleep mode is off
  if (pwr_mngr_conf_get_sleep() == 1) {
    printf("Error: OS ware sleep is running!\r\n");
    return (-1);
  }

  // CLI params handling
  strgp = s;
  tok = strsep(&strgp, " ");
  if (!tok) {
    return (-1);
  }
  duration = (uint32_t)strtoul(tok, NULL, 10);
  argc++;

  memset(&standby_config, 0x0, sizeof(standby_config));
  standby_config.duration = duration;
  standby_config.memRetentionSecIdList = PWR_CON_RETENTION_SEC_MASK_ALL;

  printf("going to Standby\r\n");
  // (1) disable interrupts
  __asm volatile("cpsid i" ::: "memory");

  DRV_PM_MaskInterrupts(DRV_PM_MODE_STANDBY);

  // (2) optional - add system specific pre sleep procedure here

  // (3) send sleep command to PMP
  DRV_PM_EnterStandbyMode(&standby_config);

  DRV_PM_RestoreInterrupts(DRV_PM_MODE_STANDBY);

  __asm volatile("cpsie i" ::: "memory");

  return ret_val;
}

/* Important Note: This test command is for OS-less test.
 * In FreeRTOS, power management should be developped in IDLE task. Or it will not stop
 * in ARM WFI  */
int32_t do_oslessShutdown(char *s) {
  DRV_PM_ShutdownModeConfig shutdown_config;
  uint32_t duration = 0;
  int32_t argc = 0, ret_val = 0;
  char *tok, *strgp;

  // confirm OS sleep mode is off
  if (pwr_mngr_conf_get_sleep() == 1) {
    printf("Error: OS ware sleep is running!\r\n");
    return (-1);
  }

  // CLI params handling
  strgp = s;
  tok = strsep(&strgp, " ");
  if (!tok) {
    return (-1);
  }
  duration = (uint32_t)strtoul(tok, NULL, 10);
  argc++;

  memset(&shutdown_config, 0x0, sizeof(shutdown_config));
  shutdown_config.duration = duration;

  printf("going to Shutdown\r\n");
  // (1) disable interrupts
  __asm volatile("cpsid i" ::: "memory");
  DRV_PM_MaskInterrupts(DRV_PM_MODE_SHUTDOWN);

  // (2) optional - add system specific pre sleep procedure here

  // (3) send sleep command to PMP
  DRV_PM_EnterShutdownMode(&shutdown_config);

  DRV_PM_RestoreInterrupts(DRV_PM_MODE_SHUTDOWN);
  __asm volatile("cpsie i" ::: "memory");

  return ret_val;
}

int32_t do_configureSleep(char *s) {
  char command[20];
  int32_t argc = 0, ret_val = 0;
  char *tok, *strgp;

  strgp = s;
  tok = strsep(&strgp, " ");
  if (!tok) {
    return (-1);
  }
  strncpy(command, tok, sizeof(command) - 1);
  argc++;

  if ((strcmp(command, "enable") == 0)) {
    if (pwr_mngr_conf_get_sleep() == 1) return (0);

    if (pwr_mngr_enable_sleep(1) != PWR_MNGR_OK) {
      printf("Failed to enable\r\n");
      return (0);
    }

    printf("Sleep - Enabled\r\n");
  } else if ((strcmp(command, "disable") == 0)) {
    if (pwr_mngr_conf_get_sleep() == 0) return (0);

    if (pwr_mngr_enable_sleep(0) != PWR_MNGR_OK) {
      printf("Failed to disable\r\n");
      return (0);
    }
    printf("Sleep - Disabled\r\n");
  } else if ((strcmp(command, "show") == 0)) {
    printf("Sleep: %s\r\n", (pwr_mngr_conf_get_sleep() == 1) ? "Enable" : "Disable");
    return (0);
  } else {
    return (-1);
  }

  set_and_save2flash("PWR_CONF_SLP_EN", command, 1);

  return ret_val;
}

int32_t do_RetentMemTest(char *s) {
  char command[20] = {0}, key_str[32] = {0}, input_str[32] = {0};
  char *output_str = NULL;
  int32_t argc = 0, ret_val = 0, ret;
  char *tok, *strgp;

  strgp = s;
  tok = strsep(&strgp, " ");
  if (!tok) {
    return (-1);
  }
  strncpy(command, tok, sizeof(command) - 1);
  argc++;

  tok = strsep(&strgp, " ");
  if (!tok) {
    return (-1);
  }
  strncpy(key_str, tok, sizeof(key_str) - 1);
  argc++;

  tok = strsep(&strgp, " ");
  if (tok) {
    strncpy(input_str, tok, sizeof(input_str) - 1);
    argc++;
  }

  if (argc == 2) {  // read
    if (!strcmp(command, "get")) {
      if (!strcmp(key_str, "DUMPALL")) {
        ret = get_env(NULL, &output_str);
        if (output_str) printf("=== Dump all ===\r\n%s\r\n", output_str);
      } else {
        ret = get_env(key_str, &output_str);
        if (output_str) printf("Read retention memory: %s = %s\r\n", key_str, output_str);
      }

      if (ret != 0) {
        if (ret == (-KVPFS_KEY_NOT_FOUND))
          printf("key [%s] is not found.\r\n", key_str);
        else
          printf("unknown error. ret=%ld\r\n", ret);
      }

      free(output_str);
    } else {
      ret_val = (-1);
    }
  } else if (argc == 3) {
    if (!strcmp(command, "set")) {
      if (0 != (ret = set_env(key_str, input_str))) {
        printf("failed to set %s, ret = %ld\r\n", key_str, ret);
      } else {
        printf("Write retention memory: %s => %s.\r\n", key_str, input_str);
        ret = save_env_to_gpm();
        if (ret != 0) {
          printf("failed to save to retention memory, ret = %ld\r\n", ret);
        }
      }
    } else {
      ret_val = (-1);
    }
  }

  return ret_val;
}

void print_gpio_err(DRV_GPIO_Status gpio_ret) {
  switch (gpio_ret) {
    case DRV_GPIO_ERROR_GENERIC:
      printf("GPIO generic error\r\n");
      break;
    case DRV_GPIO_ERROR_INVALID_PORT:
      printf("GPIO invalid port\r\n");
      break;
    case DRV_GPIO_ERROR_PARAM:
      printf("GPIO port out of range\r\n");
      break;
    case DRV_GPIO_ERROR_FORBIDDEN:
      printf("GPIO port not controlled by MCU\r\n");
      break;
    case DRV_GPIO_ERROR_INIT:
      printf("GPIO driver not initialized or failed to initialize\r\n");
      break;
    case DRV_GPIO_ERROR_RESOURCE:
      printf("No free GPIO virtual ID\r\n");
      break;
    case DRV_GPIO_ERROR_POWER:
      printf("Wrong power state\r\n");
      break;
    default:
      printf("GPIO unknown err code\r\n");
      break;
  }
}

static int32_t lsgpio(void) {
  GPIO_Id gpio_id;
  GPIO_Pull gpio_pull;
  GPIO_Direction gpio_dir;
  DRV_GPIO_Status gpio_ret;
  MCU_PinId pin_id;
  printf("---------------------\r\n");
  for (gpio_id = MCU_GPIO_ID_01; gpio_id < MCU_GPIO_ID_MAX; gpio_id++) {
    gpio_ret = DRV_GPIO_GetPinset(gpio_id, &pin_id);
    if (gpio_ret == DRV_GPIO_OK && pin_id != MCU_PIN_ID_UNDEF) {
      printf("MCU_GPIO %d\r\n", gpio_id);
      printf("PIN set: %d\r\n", pin_id);
      if ((gpio_ret = DRV_GPIO_GetDirection(gpio_id, &gpio_dir)) != DRV_GPIO_OK) {
        printf("Unable to get gpio%d direction\r\n", gpio_id);
        print_gpio_err(gpio_ret);
        return -1;
      }
      if ((gpio_ret = DRV_GPIO_GetPull(gpio_id, &gpio_pull)) != DRV_GPIO_OK) {
        printf("Unable to get gpio%d pull mode\r\n", gpio_id);
        print_gpio_err(gpio_ret);
        return -1;
      }
      printf("State: direction=%d (", gpio_dir);
      if (gpio_dir == GPIO_DIR_INPUT) {
        printf("in");
      } else {
        printf("out");
      }
      printf("), pull=%d (", gpio_pull);
      switch (gpio_pull) {
        case GPIO_PULL_NONE:
          printf("none");
          break;
        case GPIO_PULL_UP:
          printf("up");
          break;
        case GPIO_PULL_DOWN:
          printf("down");
          break;
        case GPIO_PULL_DONT_CHANGE:
        default:
          break;
      }
      printf(")\r\n");
      printf("---------------------\r\n");
    } else if (gpio_ret == DRV_GPIO_ERROR_INIT) {
      print_gpio_err(gpio_ret);
      return -1;
    }
  }
  return 0;
}

int32_t do_configMonPin(char *s) {
  uint32_t pin_set = 0, level = 0;
  char command[20];
  int32_t argc = 0, ret_val = 0, res;
  PWR_MNGR_Status temp;
  char *tok, *strgp;

  strgp = s;
  tok = strsep(&strgp, " ");
  if (!tok) {
    return (-1);
  }
  pin_set = (uint32_t)strtoul(tok, NULL, 10);
  argc++;

  tok = strsep(&strgp, " ");
  if (!tok) {
    return (-1);
  }
  strncpy(command, tok, sizeof(command) - 1);
  argc++;

  tok = strsep(&strgp, " ");
  if (tok) {
    level = (uint32_t)strtoul(tok, NULL, 10);
    argc++;
  }

  if ((pin_set < MCU_GPIO_ID_01) || (pin_set >= MCU_GPIO_ID_MAX)) {
    printf("GPIO ID range is %d ~ %d\r\n", MCU_GPIO_ID_01, (MCU_GPIO_ID_MAX - 1));
    lsgpio();
    return (0);
  }

  if (argc == 3) {
    if (!strcmp(command, "set")) {
      temp = pwr_mngr_set_monitor_io(pin_set, level);
      if (temp == PWR_MNGR_OK) {
        printf("Pin %lu is registered.\r\n", pin_set);
      } else {
        printf("Can't register pin %lu err_code %d \r\n", pin_set, temp);
      }
    } else {
      ret_val = -1;
    }
  } else if (argc == 2) {
    if (!strcmp(command, "del")) {
      res = pwr_mngr_del_monitor_io(pin_set);
      if (res == 0) {
        printf("Pin %lu is un-registered.\r\n", pin_set);
      } else {
        printf("Can't un-register pin %lu err_code %ld \r\n", pin_set, res);
      }
    } else {
      ret_val = -1;
    }
  }

  return ret_val;
}

int32_t do_sleepStatistics(char *s) {
  DRV_PM_Statistics statistics;
  PWR_MNGR_PwrCounters counters;
  char command[20], boot_type_str[20] = {0}, cause_str[20] = {0};
  int32_t argc = 0, ret_val = 0, res, i;
  char *tok, *strgp;

  strgp = s;
  tok = strsep(&strgp, " ");
  if (!tok) {
    return (-1);
  }
  strncpy(command, tok, sizeof(command) - 1);
  argc++;

  if (!strcmp(command, "show")) {
    printf("================== Sleep Statistics ==================\r\n");
    res = DRV_PM_GetStatistics(&statistics);
    if (res == 0) {
      switch (statistics.boot_type) {
        case DRV_PM_DEV_COLD_BOOT:
          strncpy(boot_type_str, "Cold", sizeof(boot_type_str) - 1);
          break;
        case DRV_PM_DEV_WARM_BOOT:
          strncpy(boot_type_str, "Warm", sizeof(boot_type_str) - 1);
          break;
        default:
          strncpy(boot_type_str, "Unknown", sizeof(boot_type_str) - 1);
          break;
      }
      switch (statistics.last_cause) {
        case DRV_PM_WAKEUP_CAUSE_NONE:
          strncpy(cause_str, "N/A", sizeof(cause_str) - 1);
          break;
        case DRV_PM_WAKEUP_CAUSE_TIMEOUT:
          strncpy(cause_str, "Timeout", sizeof(cause_str) - 1);
          break;
        case DRV_PM_WAKEUP_CAUSE_IO_ISR:
          strncpy(cause_str, "IO interrupt", sizeof(cause_str) - 1);
          break;
        case DRV_PM_WAKEUP_CAUSE_MODEM:
          strncpy(cause_str, "Modem", sizeof(cause_str) - 1);
          break;
        default:
          strncpy(cause_str, "Unknown", sizeof(cause_str) - 1);
          break;
      }

      printf(" ------------------ Wakup information ------------------\r\n");
      printf(" Last boot type           : %s (%d) \r\n", boot_type_str, statistics.boot_type);
      printf(" Last wakeUp cause        : %s (%d) \r\n", cause_str, statistics.last_cause);
      printf(" Last sleep duration left : %lu ms\r\n", statistics.last_dur_left);
      printf(" WakeUp counter           : %ld \r\n", statistics.counter);
    } else {
      printf("Error: Failed to get sleep statistics!\r\n");
    }

    printf(" ------------------- Prevents(counters) ----------------\r\n");
    res = pwr_mngr_get_prevent_sleep_cntr(&counters);
    if (res == 0) {
      printf(" Power manager disable               ---> %lu\r\n", counters.sleep_disable);
      printf(" CLI inactivity time                 ---> %lu\r\n", counters.uart_incativity_time);
      printf(" HiFC channel busy                   ---> %lu\r\n", counters.hifc_busy);
      printf(" Monitored GPIO busy                 ---> %lu\r\n", counters.mon_gpio_busy);
      printf(" Sleep manager total                 ---> %lu\r\n", counters.sleep_manager);
      for (i = 0; i < SMNGR_RANGE; i++) {
        if (smngr_get_devname(i)) {
          printf("   |---- (%2ld) %-20s ---> %ld\r\n", i, smngr_get_devname(i),
                 counters.sleep_manager_devices[i]);
        }
      }
      pwr_mngr_clr_prevent_sleep_cntr();
      printf(" ---------------------- IO park list -------------------\r\n");
      DRV_IO_DumpSleepList();
#if (PWR_MNGR_EN_HIBERNATE_SHUTDOWN == 1)
      printf(" ------------------ Hibernation Debug ------------------\n");
      hibernate_dbg_dump();
#endif
    } else {
      printf("Error: Failed to get sleep counters!\r\n");
    }
  } else if (!strcmp(command, "reset")) {
    DRV_PM_ClearStatistics();
    printf("Sleep statistics is reset.\r\n");
  } else {
    ret_val = -1;
  }

  return ret_val;
}

static void wdt_timeout_handler(void *user_param) {
  DRV_WATCHDOG_Time wd_time;
  DRV_WATCHDOG_GetRemainingTime(&wd_time);

  /* Note: The additional 10s delay is a reserved time for the Modem log printout when WD reset take place. */
  printf(
      "Watchdog timeout!!! Prevent go to sleep.\r\nSystem will be reset on next timeout (ETA "
      "%lds) + additional delay (10s).\r\n",
      wd_time);

  if (pwr_mngr_conf_get_sleep() == 1) {
    pwr_mngr_enable_sleep(0);
  }
}

static void wd_kicker_task(void *pvParameters) {
  uint32_t kick_period = 0;
  DRV_WATCHDOG_Status ret;
  DRV_WATCHDOG_Time wd_time;
  uint32_t tick_rate_ms = 1000 / osKernelGetTickFreq();
  int32_t sl;
  while (1) {
    sl = osKernelLock();
    ret = DRV_WATCHDOG_Kick();
    osKernelRestoreLock(sl);
    if (ret != DRV_WATCHDOG_OK) vTaskDelete(NULL);

    ret = DRV_WATCHDOG_GetTimeout(&wd_time);
    if (ret != DRV_WATCHDOG_OK) vTaskDelete(NULL);

    kick_period = (wd_time * 1000 / tick_rate_ms) / 2;
    osDelay(kick_period);
  }
  osThreadExit();
}

int32_t do_wd(char *s) {
  char command[20] = {0}, par2[20] = {0}, par3[20] = {0};
  int32_t argc = 0;
  int32_t ret_val = 0;
  int32_t sl;
  static osThreadId_t wd_task_h = NULL;
  osThreadAttr_t wd_task_attr = {0};
  DRV_WATCHDOG_Status wdt_ret;
  DRV_WATCHDOG_Config wdt_config;
  DRV_WATCHDOG_Time wdt_time;
  char *tok, *strgp;

  strgp = s;
  tok = strsep(&strgp, " ");
  if (!tok) {
    return (-1);
  }
  strncpy(command, tok, sizeof(command) - 1);
  argc++;

  tok = strsep(&strgp, " ");
  if (tok) {
    strncpy(par2, tok, sizeof(par2) - 1);
    argc++;
  }
  tok = strsep(&strgp, " ");
  if (tok) {
    strncpy(par3, tok, sizeof(par3) - 1);
    argc++;
  }

  switch (argc) {
    case 1:
      if (!strncmp("enable", command, 20)) {
        if (wd_task_h != NULL) {
          printf("Watchdog is already enabled\r\n");
          ret_val = 1;
        } else {
          wdt_config.timeout = CONFIG_WATCHDOG_DEFAULT_TIMEOUT;
          wdt_config.timeout_cb = wdt_timeout_handler;
          wdt_config.timeout_cb_param = NULL;
          if ((wdt_ret = DRV_WATCHDOG_Initialize(&wdt_config)) != DRV_WATCHDOG_OK) {
            printf("Failed to initialize watchdog\r\n");
            return 1;
          }
          sl = osKernelLock();
          wdt_ret = DRV_WATCHDOG_Start();
          osKernelRestoreLock(sl);
          if (wdt_ret != DRV_WATCHDOG_OK) {
            printf("Failed to start watchdog\r\n");
            return 1;
          }
          wd_task_attr.name = "WD TASK";
          wd_task_attr.stack_size = 512;
          wd_task_attr.priority = osPriorityLow;

          wd_task_h = osThreadNew(wd_kicker_task, NULL, &wd_task_attr);
          if (wd_task_h == NULL)
            printf("Failed to create kicker task\r\n");
          else
            printf("Watchdog enabled!\r\n");
        }
      } else if (!strncmp("disable", command, 20)) {
        sl = osKernelLock();
        wdt_ret = DRV_WATCHDOG_Stop();
        osKernelRestoreLock(sl);
        wdt_ret = DRV_WATCHDOG_Uninitialize();
        if (wdt_ret == DRV_WATCHDOG_OK) {
          printf("Disable watchdog\r\n");
          if (wd_task_h) {
            if (osThreadTerminate(wd_task_h) != osOK) {
              printf("Failed to terminate kicker task\r\n");
              ret_val = 1;
            } else {
              printf("Watchdog disabled\r\n");
              wd_task_h = NULL;
            }
          }
        } else {
          printf("Failed to disable watchdog\r\n");
          ret_val = 1;
        }
      } else if (!strncmp("testbark", command, 20)) {
        if (wd_task_h) {
          if (osThreadTerminate(wd_task_h) != osOK) {
            printf("Failed to terminate kicker task before change timeout\r\n");
            ret_val = 1;
          }
          wd_task_h = NULL;
        }
      } else if (!strncmp("timeleft", command, 20)) {
        wdt_ret = DRV_WATCHDOG_GetRemainingTime(&wdt_time);
        if (wdt_ret == DRV_WATCHDOG_OK) {
          printf("Watchdog time left: %ld\r\n", wdt_time);
        } else {
          printf("Failed to get remaining time: %d\r\n", wdt_ret);
        }
      } else {
        printf("Wrong command format\r\n");
        ret_val = 1;
      }
      break;
    case 2:
      if (!strncmp("timeout", command, 20)) {
        wdt_time = (DRV_WATCHDOG_Time)atoi(par2);
        sl = osKernelLock();
        wdt_ret = DRV_WATCHDOG_SetTimeout(wdt_time);
        osKernelRestoreLock(sl);
        if (wd_task_h && wdt_ret == DRV_WATCHDOG_OK) {
          if (osThreadTerminate(wd_task_h) != osOK) {
            printf("Failed to terminate kicker task before change timeout\r\n");
            ret_val = 1;
          } else {
            wd_task_attr.name = "WD TASK";
            wd_task_attr.stack_size = 512;
            wd_task_attr.priority = osPriorityLow;

            wd_task_h = osThreadNew(wd_kicker_task, NULL, &wd_task_attr);
            if (wd_task_h == NULL) {
              printf("Failed to create kicker task\r\n");
              ret_val = 1;
            }
          }
        } else {
          printf("Failed to config new timeout\r\n");
          ret_val = 1;
        }
      } else {
        printf("Wrong command format\r\n");
        ret_val = 1;
      }
      break;
    default:
      printf("Wrong command format\r\n");
      ret_val = 1;
      break;
  }

  return ret_val;
}

int32_t do_setInactivityTime(char *s) {
  char buff[PWR_KVP_BUFF_SIZE + 1] = {0};
  uint32_t inact_time_val = 0;
  int32_t argc = 0, ret_val = 0;
  char *tok, *strgp;

  strgp = s;
  tok = strsep(&strgp, " ");
  if (!tok) {
    return (-1);
  }
  inact_time_val = (uint32_t)strtoul(tok, NULL, 10);
  argc++;

  if (pwr_mngr_conf_set_cli_inact_time(inact_time_val) != PWR_MNGR_OK) {
    printf("Failed to set MCU CLI inactivity timeout to %lu\r\n", inact_time_val);
    return (-1);
  }

  printf("Configured the MCU CLI inactivity timeout to %lu sec.\r\n", inact_time_val);

  if (ret_val == 0) {
    memset(buff, 0, PWR_KVP_BUFF_SIZE);
    snprintf(buff, PWR_KVP_BUFF_SIZE, "%lu", inact_time_val);
    set_and_save2flash("PWR_CONF_INACTT", buff, 1);
  }

  return ret_val;
}

int32_t do_setMAPmode(char *set_val) {
  int32_t ret_val = 0;

  if (set_val == NULL) {
    return (-1);
  }

  if (!strcmp(set_val, "on") || !strcmp(set_val, "off")) {
    set_and_save2flash("PWR_CONF_MAPON", set_val, 1);
  } else {
    return (-1);
  }

  printf("Configured the MAP channel switch to %s\r\n", set_val);

  return ret_val;
}

// List all free blocks
static void DoListFree(void) {
  struct hibernate_free_block *freeList = NULL, *cur_node;
  size_t bytes = 0, contexts = 0;

  printf("  Start Address     Size\r\n");
  printf("---------------   ------\r\n");
  //           0x00000000   123456

  taskENTER_CRITICAL();
  hibernate_get_free_blocks(&freeList);

  for (cur_node = freeList; cur_node; cur_node = cur_node->next) {
    printf("     0x%08x   %6x\r\n", (size_t)cur_node, cur_node->size);

    contexts++;
    bytes += cur_node->size;
  }

  hibernate_restore_stacks();

  taskEXIT_CRITICAL();

  printf("\r\n");
  printf("%x bytes free in %x contexts.\r\n", bytes, contexts);
}

// Zero all free blocks
static void DoZeroFree(void) {
  struct hibernate_free_block *freeList = NULL;

  taskENTER_CRITICAL();

  hibernate_get_free_blocks(&freeList);
  hibernate_zero_free_blocks(freeList);

  hibernate_restore_stacks();

  taskEXIT_CRITICAL();
}

int32_t do_memRetenDbg(char *set_val) {
  if (!strcmp(set_val, "-l")) {
    DoListFree();
  } else if (!strcmp(set_val, "-z")) {
    DoZeroFree();
  } else {
    return -1;
  }

  return 0;
}

int32_t do_ioPark(char *s) {
  uint32_t pin_set = 0;
  char command[20];
  int32_t argc = 0, ret_val = 0;
  DRV_IO_Status err_code;
  char *tok, *strgp;

  strgp = s;
  tok = strsep(&strgp, " ");
  if (!tok) {
    return (-1);
  }
  pin_set = (uint32_t)strtoul(tok, NULL, 10);
  argc++;

  tok = strsep(&strgp, " ");
  if (!tok) {
    return (-1);
  }
  strncpy(command, tok, sizeof(command) - 1);
  argc++;

  if (argc == 2) {
    if (!strcmp(command, "add")) {
      err_code = DRV_IO_AddSleepPin(pin_set);
      if (err_code == DRV_IO_OK) {
        printf("Pin %lu is added.\r\n", pin_set);
      } else {
        printf("Can't add pin %lu err_code %d\r\n", pin_set, err_code);
      }
    } else if (!strcmp(command, "del")) {
      err_code = DRV_IO_DelSleepPin(pin_set, 1);
      if (err_code == DRV_IO_OK) {
        printf("Pin %lu is removed.\r\n", pin_set);
      } else {
        printf("Can't remove pin %lu err_code %d\r\n", pin_set, err_code);
      }
    } else if (!strcmp(command, "clr")) {
      err_code = DRV_IO_DelSleepPin(pin_set, 0);
      if (err_code == DRV_IO_OK) {
        printf("Pin %lu is cleared.\r\n", pin_set);
      } else {
        printf("Can't clear pin %lu err_code %d\r\n", pin_set, err_code);
      }
    } else {
      printf("Wrong command parameter!\r\n");
      return 0;
    }
  }
  return ret_val;
}
