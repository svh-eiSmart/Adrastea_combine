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
#include <string.h>
#include <stdlib.h>

#include "accel_lis2dh.h"
#include "pwr_mngr.h"
#include "DRV_PM.h"
#include "sleep_mngr.h"

extern accel_ev_cfg_t accel_ev_mv_start_config;

#define CMD_PARAM_LENGTH 20

static int parse_arg(char *s, char *argv, int max_arg, int arg_len) {
  int argc = 0;
  char *tmp_s = NULL;
  int i;
  /* Parse arg */
  while (*s == ' ') s++;

  if (strlen(s)) {
    for (i = 0; i < max_arg; i++) {
      tmp_s = strchr(s, ' ');
      if (!tmp_s) {
        tmp_s = strchr(s, '\0');
        if (!tmp_s) goto arg_parse_err;

        if (tmp_s - s < arg_len) {
          memcpy(argv + (i * arg_len), s, tmp_s - s);
          argc++;
        } else {
          goto arg_parse_err;
        }
        break;
      }
      if (tmp_s - s < arg_len) {
        memcpy(argv + (i * arg_len), s, tmp_s - s);
        argc++;
      } else {
        goto arg_parse_err;
      }
      while (*tmp_s == ' ') tmp_s++;
      if (*tmp_s != '\0')
        s = tmp_s;
      else
        break;
    }
  }
  return argc;

arg_parse_err:
  printf("Invalid arg.\r\n");
  return 0;
}

int do_accel(char *s) {
  xyz_mg_t accel_raw;

  if (!strcmp(s, "dump")) {
    accel_dump_config();
  } else if (!strcmp(s, "start")) {
    if (accel_start(ACCEL_EVENT_0, &accel_ev_mv_start_config) != ACCEL_RET_SUCCESS)
      printf("Failed to start accel event\r\n");
    else
      printf("accel event started\r\n");
  } else if (!strcmp(s, "stop")) {
    if (accel_stop(ACCEL_EVENT_0) != ACCEL_RET_SUCCESS)
      printf("Failed to stop accel event\r\n");
    else
      printf("accel event stopped\r\n");
  } else if (!strcmp(s, "mask")) {
    if (accel_irqx_mask(0x1) != ACCEL_RET_SUCCESS)
      printf("Failed to mask accel event\r\n");
    else
      printf("accel event masked\r\n");
  } else if (!strcmp(s, "unmask")) {
    if (accel_irqx_unmask(0x1) != ACCEL_RET_SUCCESS)
      printf("Failed to unmask accel event\r\n");
    else
      printf("accel event unmasked\r\n");
  } else if (!strcmp(s, "xyz")) {
    pwr_mngr_enable_sleep(0);
    while (1) {
      if (accel_has_data() && (accel_get_xyz_mg(&accel_raw) == ACCEL_RET_SUCCESS))
        printf("x: %d    y: %d    z: %d\r\n", accel_raw.x, accel_raw.y, accel_raw.z);
    }
  }
  return 0;
}

int do_temp(char *s) {
  int16_t temp;
  if (accel_get_temp_celsius(&temp) == ACCEL_RET_SUCCESS)
    printf("Temperature: %d\r\n", temp);
  else
    printf("Failed to get temperature\r\n");
  return 0;
}

int do_sleep(char *s) {
  if (!strcmp(s, "enable")) {
    pwr_mngr_conf_set_mode(PWR_MNGR_MODE_SHUTDOWN, 0);
    pwr_mngr_enable_sleep(1);
    return 0;
  } else if (!strcmp(s, "disable")) {
    pwr_mngr_enable_sleep(0);
    return 0;
  } else {
    return -1;
  }
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

int do_lsgpio(char *s) {
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

int do_gpio(char *s) {
  char *command = NULL;
  int handled = 0;
  GPIO_Id gpio_num;
  int input_num;
  uint8_t gpio_val = 0;
  int argc;
  int ret_val = 0;
  GPIO_Direction gpio_dir;
  GPIO_Pull gpio_pull;
  DRV_GPIO_Status gpio_ret;
  char argv[2][CMD_PARAM_LENGTH];

  memset((char *)argv, 0, 2 * CMD_PARAM_LENGTH);
  argc = parse_arg(s, (char *)argv, 2, CMD_PARAM_LENGTH);

  input_num = atoi(argv[0]);
  command = argv[1];

  if (argc == 2 && input_num >= (int)MCU_GPIO_ID_01 && input_num < (int)MCU_GPIO_ID_MAX) {
    gpio_num = (GPIO_Id)input_num;
    gpio_dir = GPIO_DIR_OUTPUT;
    if (!strcmp(command, "set")) {
      handled = 1;
      gpio_val = 1;
    } else if (!strcmp(command, "clr")) {
      handled = 1;
      gpio_val = 0;
    } else if (!strcmp(command, "get")) {
      handled = 1;
      gpio_dir = GPIO_DIR_INPUT;
    } else if (!strcmp(command, "toggle")) {
      if ((gpio_ret = DRV_GPIO_GetValue(gpio_num, &gpio_val)) == DRV_GPIO_OK) {
        handled = 1;
        gpio_val = 1 - gpio_val;
      } else {
        ret_val = -1;
        goto gpio_err;
      }
    } else if (!strcmp(command, "state")) {
      if ((gpio_ret = DRV_GPIO_GetDirection(gpio_num, &gpio_dir)) != DRV_GPIO_OK) {
        printf("Unable to get gpio%d direction\r\n", gpio_num);
        ret_val = -1;
        goto gpio_err;
      }
      if ((gpio_ret = DRV_GPIO_GetPull(gpio_num, &gpio_pull)) != DRV_GPIO_OK) {
        printf("Unable to get gpio%d pull mode\r\n", gpio_num);
        ret_val = -1;
        goto gpio_err;
      }

      printf("gpio %d state: direction=%d (", gpio_num, gpio_dir);
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
    } else if (!strcmp(command, "pullnone")) {
      printf("gpio pull none %d\n\r", gpio_num);
      if ((gpio_ret = DRV_GPIO_SetPull(gpio_num, GPIO_PULL_NONE)) != DRV_GPIO_OK) {
        ret_val = -1;
        goto gpio_err;
      }
    } else if (!strcmp(command, "pullup")) {
      printf("gpio pull up %d\n\r", gpio_num);
      if ((gpio_ret = DRV_GPIO_SetPull(gpio_num, GPIO_PULL_UP)) != DRV_GPIO_OK) {
        ret_val = -1;
        goto gpio_err;
      }
    } else if (!strcmp(command, "pulldown")) {
      printf("gpio pull down %d\n\r", gpio_num);
      if ((gpio_ret = DRV_GPIO_SetPull(gpio_num, GPIO_PULL_DOWN)) != DRV_GPIO_OK) {
        ret_val = -1;
        goto gpio_err;
      }
    } else {
      printf("Unknown gpio command\r\n");
      ret_val = -1;
    }
  } else {
    ret_val = -1;
  }

  if (handled) {
    if (gpio_dir == GPIO_DIR_INPUT) {
      if ((gpio_ret = DRV_GPIO_SetDirection(gpio_num, GPIO_DIR_INPUT)) != DRV_GPIO_OK) {
        printf("Failed: ");
        ret_val = -1;
        goto gpio_err;
      } else {
        DRV_GPIO_GetValue(gpio_num, &gpio_val);
        printf("gpio %d input %d\n\r", gpio_num, gpio_val);
      }
    } else {
      if ((gpio_ret = DRV_GPIO_SetDirectionOutAndValue(gpio_num, gpio_val)) != DRV_GPIO_OK) {
        printf("Failed: gpio %d output %d\n\r", gpio_num, gpio_val);
        ret_val = -1;
        goto gpio_err;
      } else {
        printf("gpio %d output %d\n\r", gpio_num, gpio_val);
      }
    }
  }
  return ret_val;

gpio_err:
  print_gpio_err(gpio_ret);
  return ret_val;
}
