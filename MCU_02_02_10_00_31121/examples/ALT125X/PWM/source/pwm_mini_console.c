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
#include "cmsis_os2.h"

#include "DRV_CCM.h"
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

void print_pwm_ret_info(DRV_CCM_Status pwm_ret) {
  switch (pwm_ret) {
    case DRV_CCM_OK:
      printf("Operation succeeded\r\n");
      break;
    case DRV_CCM_ERROR_INIT:
      printf("Device is in wrong init state\r\n");
      break;
    case DRV_CCM_ERROR_IF_CFG:
      printf("Wrong interface configuration\r\n");
      break;
    case DRV_CCM_ERROR_POWER:
      printf("Device is in wrong power state\r\n");
      break;
    case DRV_CCM_ERROR_UNSUPPORTED:
      printf("Operation not supported\r\n");
      break;
    case DRV_CCM_ERROR_PARAM:
      printf("Parameter error\r\n");
      break;
    default:
      printf("Unknown error\r\n");
      break;
  }
}
static void cc_pwm_event(uint32_t evt) {
  static uint32_t last_ccm_0_timer_evt = 0;
  static uint32_t last_ccm_0_compare_evt = 0;
  static uint32_t last_ccm_1_timer_evt = 0;
  static uint32_t last_ccm_1_compare_evt = 0;
  static uint32_t tick_rate_ms = 0;

  if (tick_rate_ms == 0) tick_rate_ms = 1000 / osKernelGetTickFreq();

  uint32_t current_tick = osKernelGetTickCount();

  if (evt & CCM_ID_0_TIMER_EVT) {
    if (last_ccm_0_timer_evt != 0)
      printf("pwm0 prd: %ld ms, duty: %ld ms\r\n",
             (current_tick - last_ccm_0_timer_evt) * tick_rate_ms,
             (last_ccm_0_compare_evt - last_ccm_0_timer_evt) * tick_rate_ms);

    last_ccm_0_timer_evt = current_tick;
  } else if (evt & CCM_ID_0_COMPARE_EVT) {
    last_ccm_0_compare_evt = current_tick;
  } else if (evt & CCM_ID_1_TIMER_EVT) {
    if (last_ccm_1_timer_evt != 0)
      printf("pwm1 prd: %ld ms, duty: %ld ms\r\n",
             (current_tick - last_ccm_1_timer_evt) * tick_rate_ms,
             (last_ccm_1_compare_evt - last_ccm_1_timer_evt) * tick_rate_ms);

    last_ccm_1_timer_evt = current_tick;
  } else if (evt & CCM_ID_1_COMPARE_EVT) {
    last_ccm_1_compare_evt = current_tick;
  }
}

static void print_ccm_channel_status(CCM_Id cid) {
  CCM_ChannelStatus status;
  int32_t ret;
  printf("-----------ccm ch%d status----------------\r\n", cid);
  if ((ret = DRV_CCM_GetChannelStatus(cid, &status)) != DRV_CCM_OK) {
    printf("Unable to get current CCM channel status\r\n");
    print_pwm_ret_info(ret);
  } else {
    printf("CCM en: %ld\r\n", status.enable);
    switch (status.mode) {
      case CCM_MODE_PWM_ONE_THRESHOLD:
        printf("mode: PWM one threshold\r\n");
        printf("period: %ldns\r\nduty: %ldns\r\npolarity: %ld\r\n", status.param1, status.param2,
               status.param3);
        break;
      case CCM_MODE_1BIT_DAC:
        printf("mode: PWM 1bit DAC\r\n");
        printf("max_timer: %ld\r\nstep: %ld\r\n", status.param1, status.param2);
        break;
      case CCM_MODE_CLK_OUT:
        printf("mode: clock out\r\n");
        printf("max_timer: %ld\r\nstep: %ld\r\n", status.param1, status.param2);
        break;
      default:
        printf("Unknown mode\r\n");
        break;
    }
  }
  printf("------------------------------------------\r\n");
}

static void config_cc_pwm_event(int enable) {
  DRV_CCM_PowerControl(CCM_POWER_OFF);
  DRV_CCM_Uninitialize();
  if (enable)
    DRV_CCM_Initialize(cc_pwm_event);
  else
    DRV_CCM_Initialize(NULL);
  DRV_CCM_PowerControl(CCM_POWER_FULL);
}

static int32_t config_enable_ccm(CCM_Id ccm_id, uint32_t mode, uint32_t param1, uint32_t param2,
                                 uint32_t param3) {
  CCM_ChannelStatus ccm_ch[CCM_ID_MAX];
  CCM_Id i;
  int32_t ret;
  uint32_t cur_en_msk = 0;
  for (i = CCM_ID_0; i < CCM_ID_MAX; i++) {
    if ((ret = DRV_CCM_GetChannelStatus(i, &ccm_ch[i])) != DRV_CCM_OK) {
      printf("Unable to get current CCM channel status\r\n");
      print_pwm_ret_info(ret);
      return -1;
    }
    if (ccm_ch[i].enable) cur_en_msk |= (1 << i);
  }
  if ((ret = DRV_CCM_Control(mode, (uint32_t)ccm_id)) != DRV_CCM_OK) {
    printf("Failed to configure CCM mode\r\n");
    print_pwm_ret_info(ret);
    return -1;
  }
  if ((ret = DRV_CCM_ConfigOutputChannel(ccm_id, param1, param2, param3)) != DRV_CCM_OK) {
    printf("Failed to config CCM output channel\r\n");
    print_pwm_ret_info(ret);
    return -1;
  }
  cur_en_msk |= (1 << ccm_id);
  DRV_CCM_EnableDisableOutputChannels(cur_en_msk);
  return 0;
}

int do_ccmevt(char *s) {
  int32_t event_en = atoi(s);
  CCM_Id cid;

  config_cc_pwm_event(event_en);

  for (cid = CCM_ID_0; cid < CCM_ID_MAX; cid++) {
    DRV_CCM_Control(CCM_MODE_PWM_ONE_THRESHOLD, cid);
    print_ccm_channel_status(cid);
  }
  printf("%s CCM interrupt event callback\r\n", (event_en) ? "Enable" : "Disable");
  return 0;
}

int do_ccmget(char *s) {
  CCM_Id pwm_id;
  pwm_id = (CCM_Id)atoi(s);
  printf("Print CCM channel %d status\r\n", pwm_id);
  print_ccm_channel_status(pwm_id);
  return 0;
}

#define PWMDAC_ARGCNT 3
int do_pwmdac(char *s) {
  CCM_Id pwm_id;
  char argv[PWMDAC_ARGCNT][CMD_PARAM_LENGTH];
  int32_t argc;
  uint32_t param1 = 0;
  uint32_t param2 = 0;

  memset((char *)argv, 0, PWMDAC_ARGCNT * CMD_PARAM_LENGTH);
  argc = parse_arg(s, (char *)argv, PWMDAC_ARGCNT, CMD_PARAM_LENGTH);
  if (argc != PWMDAC_ARGCNT) return -1;

  pwm_id = (CCM_Id)atoi(argv[0]);
  param1 = atoi(argv[1]);
  param2 = atoi(argv[2]);

  if (config_enable_ccm(pwm_id, CCM_MODE_1BIT_DAC, param1, param2, 0) == 0) {
    printf("Configure and enable ccm channel %d to PWM 1bit DAC mode\r\n", pwm_id);
    return 0;
  }
  return -1;
}

#define CLKOUT_ARGCNT 3
int do_clkout(char *s) {
  CCM_Id pwm_id;
  char argv[CLKOUT_ARGCNT][CMD_PARAM_LENGTH];
  int32_t argc;
  uint32_t param1 = 0;
  uint32_t param2 = 0;

  memset((char *)argv, 0, CLKOUT_ARGCNT * CMD_PARAM_LENGTH);
  argc = parse_arg(s, (char *)argv, CLKOUT_ARGCNT, CMD_PARAM_LENGTH);
  if (argc != CLKOUT_ARGCNT) return -1;

  pwm_id = (CCM_Id)atoi(argv[0]);
  param1 = atoi(argv[1]);
  param2 = atoi(argv[2]);

  if (config_enable_ccm(pwm_id, CCM_MODE_CLK_OUT, param1, param2, 0) == 0) {
    printf("Configure and enable ccm channel %d to clock out mode\r\n", pwm_id);
    return 0;
  }
  return -1;
}

#define PWM_ARGCNT 4
int do_pwm(char *s) {
  CCM_Id pwm_id;
  char argv[PWM_ARGCNT][CMD_PARAM_LENGTH];
  int32_t argc;
  uint32_t param1 = 0;
  uint32_t param2 = 0;
  uint32_t param3 = 0;

  memset((char *)argv, 0, PWM_ARGCNT * CMD_PARAM_LENGTH);
  argc = parse_arg(s, (char *)argv, PWM_ARGCNT, CMD_PARAM_LENGTH);
  if (argc != PWM_ARGCNT) return -1;

  pwm_id = (CCM_Id)atoi(argv[0]);
  param1 = atoi(argv[1]);
  param2 = atoi(argv[2]);
  param3 = atoi(argv[3]);

  if (config_enable_ccm(pwm_id, CCM_MODE_PWM_ONE_THRESHOLD, param1, param2, param3) == 0) {
    printf("Configure and enable ccm channel %d to PWM one threshold mode\r\n", pwm_id);
    return 0;
  }
  return -1;
}
