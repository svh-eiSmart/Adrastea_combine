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

/* Kernel includes. */
#include "DRV_PWM_DAC.h"
#define CMD_PARAM_LENGTH 20

static int32_t parse_arg(char *s, char *argv, int max_arg, int arg_len) {
  int32_t argc = 0;
  char *tmp_s = NULL;
  int32_t i;
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

static void print_pwm_err(DRV_PWM_DAC_Status pwm_ret) {
  switch (pwm_ret) {
    case DRV_PWM_DAC_ERROR_INIT:
      printf("PWM Err: Wrong init State\r\n");
      break;
    case DRV_PWM_DAC_ERROR_IF_CFG:
      printf("PWM Err: Wrong interface configuration\r\n");
      break;
    case DRV_PWM_DAC_ERROR_PARAM:
      printf("PWM Err: Wrong parameter\r\n");
      break;
    case DRV_PWM_DAC_ERROR_POWER:
      printf("PWM Err: Wrong power state\r\n");
      break;
    case DRV_PWM_DAC_ERROR_UNSUPPORTED:
      printf("PWM Err: Not support\r\n");
      break;
    default:
      break;
  }
}
#define PWM_ARGCNT 4
int do_pwm(char *s) {
  int ret_val = -1;
  PWM_DAC_Id pwm_id;
  DRV_PWM_DAC_Status pwm_ret;
  PWM_DAC_Param pwm_param;
  char argv[PWM_ARGCNT][CMD_PARAM_LENGTH];
  int32_t argc;
  uint32_t pwm_en;

  memset((char *)argv, 0, PWM_ARGCNT * CMD_PARAM_LENGTH);
  argc = parse_arg(s, (char *)argv, PWM_ARGCNT, CMD_PARAM_LENGTH);
  pwm_id = (PWM_DAC_Id)atoi(argv[0]);

  if (argc >= 2 && pwm_id < PWM_DAC_ID_MAX) {
    if (!strncmp(argv[1], "set", 20)) {
      if (argc == 4) {
        pwm_param.clk_div = (uint32_t)atoi(argv[2]);
        pwm_param.duty_cycle = (uint32_t)atoi(argv[3]);
      } else if (argc == 2) {
        if ((pwm_ret = DRV_PWM_DAC_GetDefConf(pwm_id, &pwm_param)) != DRV_PWM_DAC_OK) goto pwm_err;
      } else {
        printf("Wrong command format\r\n");
        return -1;
      }

      if ((pwm_ret = DRV_PWM_DAC_ConfigOutputChannel(pwm_id, &pwm_param)) != DRV_PWM_DAC_OK)
        goto pwm_err;
      DRV_PWM_DAC_GetChannelStatus(pwm_id, &pwm_en, &pwm_param);
      printf("PWM%d status  en: %lu, clk_div: %lu, duty_cycle: %lu\r\n", pwm_id, pwm_en,
             pwm_param.clk_div, pwm_param.duty_cycle);
      ret_val = 0;

    } else if (!strncmp(argv[1], "get", 20)) {
      if ((pwm_ret = DRV_PWM_DAC_GetChannelStatus(pwm_id, &pwm_en, &pwm_param)) != DRV_PWM_DAC_OK)
        goto pwm_err;
      printf("PWM%d status  en: %lu, clk_div: %lu, duty_cycle: %lu\r\n", pwm_id, pwm_en,
             pwm_param.clk_div, pwm_param.duty_cycle);
      ret_val = 0;
    }
  }
  return ret_val;

pwm_err:
  print_pwm_err(pwm_ret);
  return ret_val;
}
