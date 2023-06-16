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

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"
#include "DRV_AUXADC.h"

static void auxadc_interrupt_handler(Auxadc_Err_Code error, uint32_t digital_value,
                                     void *user_param) {
  printf("auxadc_interrupt_handler: value %ld\n", digital_value);
}

int do_auxadc_get(char *s) {
  uint32_t adc_channel;
  uint32_t adc_value = 0;
  int argc;
  int ret_val = -1;
  int api_ret;

  argc = sscanf(s, "%ld", &adc_channel);

  if (adc_channel >= AUX_ADC_MAX)
    printf("invalid pin/port number\n");
  else if (argc == 1) {
#ifdef ALT1255
    if (adc_channel != 2) {
      printf("Only channel 2 is available on 1255 platform\n");
      return ret_val;
    }
#endif
    api_ret = DRV_AUXADC_Get_Value(adc_channel, 64, &adc_value);

    if (api_ret) {
      if (api_ret == AUXADC_ERROR_OTHER_ERROR)
        printf("error (other)\n");
      else if (api_ret == AUXADC_ERROR_CHANNEL_NUMBER)
        printf("invalid channel number\n");
      else if (api_ret == AUXADC_ERROR_OUT_OF_RANGE)
        printf("parameter(s) out of range\n");
      else if (api_ret == AUXADC_ERROR_RESOURCE_BUSY)
        printf("previous conversion have not ended yet\n");
      else if (api_ret == AUXADC_ERROR_WAIT_INTERRUPT) {
        ret_val = 0;
        printf("Handled by interrupt\n");
      } else {
        printf("Unknown error %d\n", api_ret);
      }
    } else {
      ret_val = 0;
      printf("auxadc_get value for ADC channel %ld is voltage %ld mV\n", adc_channel, adc_value);
    }
  }

  return ret_val;
}

int do_auxadc_int_reg(char *s) {
  uint32_t adc_channel;
  int argc;
  int ret_val = -1;

  argc = sscanf(s, "%ld", &adc_channel);

  if (adc_channel >= AUX_ADC_MAX)
    printf("invalid pin/port number\n");
  else if (argc == 1) {
#ifdef ALT1255
    if (adc_channel != 2) {
      printf("Only channel 2 is available on 1255 platform\n");
      return ret_val;
    }
#endif
    DRV_AUXAUDC_Register_Interrupt(adc_channel, auxadc_interrupt_handler, (void *)adc_channel);
    printf("port # %ld have been registered\n", adc_channel);
    ret_val = 0;
  } else
    printf("incorrect amount of arguments\n");

  return ret_val;
}

int do_auxadc_int_dereg(char *s) {
  uint32_t adc_channel;
  int argc;
  int ret_val = -1;

  argc = sscanf(s, "%ld", &adc_channel);

  if (adc_channel >= AUX_ADC_MAX)
    printf("invalid pin/port number\n");
  else if (argc == 1) {
#ifdef ALT1255
    if (adc_channel != 2) {
      printf("Only channel 2 is available on 1255 platform\n");
      return ret_val;
    }
#endif
    DRV_AUXADC_Deregister_Interrupt(adc_channel);
    printf("port # %ld have been deregistered\n", adc_channel);
    ret_val = 0;
  } else
    printf("incorrect amount of arguments\n");

  return ret_val;
}

int do_auxadc_set_scale(char *s) {
  int argc;
  unsigned int scale;

  argc = sscanf(s, "%d", &scale);
  if (argc == 1) {
    DRV_AUXADC_Set_Scale(scale);
    printf("Set ADC scale to %d\n", scale);
    return 0;
  }
  return -1;
}

int do_auxadc_vbat_get(char *s) {
  int result = DRV_AUXADC_Get_Bat_Sns();
  printf("Battery Sensor %d mV\n", result);
  return 0;
}

int do_auxadc_temperature_get(char *s) {
  int32_t result = DRV_AUXADC_Get_Temperature();
  printf("Temperature: %ld\n", result);
  return 0;
}
