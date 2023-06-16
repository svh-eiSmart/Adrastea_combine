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
#include <stdio.h>
#include DEVICE_HEADER
#include "DRV_COMMON.h"
#include "DRV_AUXADC.h"
#include "DRV_GPIO.h"
#include "DRV_GPIOCTL.h"
#include "DRV_CLOCK_GATING.h"
#include "DRV_PMP_AGENT.h"
#include "DRV_IO.h"
#include "alt_osal_opt.h"

#define AUX_ADC_SCALING_MULTIPLIER (1800)
#define AUX_ADC_SCALING_DEVIDER (4096)
#define AUX_ADC_DEFAULT_MCU_FIREWALL_BIT (0x4)
#define CONVERT_ADC_CH_TO_GPIO(x) (x + 1)

typedef struct {
  AUX_ADC_Type *base;
  IRQn_Type irq_number;
  uint8_t average_count;
  auxadc_callback callback;
  void *callback_param;
} auxadc_info_t;

static uint32_t auxadc_scaling_multiplier = AUX_ADC_SCALING_MULTIPLIER;
static auxadc_info_t auxadc_info[] = {
    {
        .base = AUX_ADC_B0,
        .irq_number = AUXADC_INT0_IRQn,
        .callback = NULL,
    },
    {
        .base = AUX_ADC_B1,
        .irq_number = AUXADC_INT1_IRQn,
        .callback = NULL,
    },
    {
        .base = AUX_ADC_B2,
        .irq_number = AUXADC_INT2_IRQn,
        .callback = NULL,
    },
#ifdef ALT1250
    {
        .base = AUX_ADC_B3,
        .irq_number = AUXADC_INT3_IRQn,
        .callback = NULL,
    },
#endif
};

static uint32_t adc_digital_2mv(uint32_t dig_value) {
  return (dig_value * auxadc_scaling_multiplier) / AUX_ADC_SCALING_DEVIDER;
}

static void DRV_AUXADC_IrqHandler(int id) {
  uint32_t result;
  Auxadc_Err_Code err = AUXADC_ERROR_NONE;
  auxadc_info_t *adc_info = &auxadc_info[id];
  AUX_ADC_Type *base = adc_info->base;

  if (base->IRPT_b.AVG_DONE) {
    if (adc_info->callback) {
      result = base->AVG_ACC / adc_info->average_count;
      if (result > 4095) {
        err = AUXADC_ERROR_OUT_OF_RANGE;
      } else {
        result = adc_digital_2mv(result);
      }
      adc_info->callback(err, result, adc_info->callback_param);
    }
    /*Disable source after conversion is over*/
    base->IRPT_CLR_b.AVG_DONE = 1;
    base->IRPT_EN_b.AVG_DONE = 0;
    base->CFG_b.EN_SAMPLE = 0;
  }
}

static int32_t config_io(int gpio_num) {
  MCU_PinId pinid = (MCU_PinId)(gpio_num);

  DRV_CLKGATING_EnableClkSource(CLK_GATING_GPIO_IF);
  if (DRV_IO_SetMux(pinid, 4) != DRV_IO_OK) {
    goto err;
  }
  if (DRV_IO_SetPull(pinid, IO_PULL_NONE) != DRV_IO_OK) {
    goto err;
  }
  DRV_GPIOCTL_SetDirectionIn(pinid);

  return 0;

err:
  return -1;
}

/***********************   interrupt handler  ***************************/

void auxdac0_handler(void) { DRV_AUXADC_IrqHandler(AUX_ADC_0); }

void auxdac1_handler(void) { DRV_AUXADC_IrqHandler(AUX_ADC_1); }

void auxdac2_handler(void) { DRV_AUXADC_IrqHandler(AUX_ADC_2); }
#ifdef ALT1250
void auxdac3_handler(void) { DRV_AUXADC_IrqHandler(AUX_ADC_3); }
#endif
/********************** APIs *****************************/

Auxadc_Err_Code DRV_AUXADC_Initialize(void) { return AUXADC_ERROR_NONE; }

Auxadc_Err_Code DRV_AUXADC_Uninitialize(void) { return AUXADC_ERROR_NONE; }

Auxadc_Err_Code DRV_AUXAUDC_Register_Interrupt(auxadc_enum_t adc_channel, auxadc_callback callback,
                                               void *callback_param) {
  int irq_num;
  auxadc_info_t *adc_info;

  if (adc_channel >= AUX_ADC_MAX) {
    return AUXADC_ERROR_CHANNEL_NUMBER;
  }
  adc_info = &auxadc_info[adc_channel];

  irq_num = adc_info->irq_number;
  if (adc_info->callback) {
    return AUXADC_ERROR_CB_REGISTERD;
  }

  adc_info->base->IRPT_EN_b.AVG_DONE = 0;
  adc_info->callback = callback;
  adc_info->callback_param = callback_param;

  NVIC_SetPriority((IRQn_Type)irq_num, 7); /* set Interrupt priority */
  NVIC_EnableIRQ((IRQn_Type)irq_num);

  return AUXADC_ERROR_NONE;
}

Auxadc_Err_Code DRV_AUXADC_Deregister_Interrupt(auxadc_enum_t adc_channel) {
  int irq_num;
  auxadc_info_t *adc_info;

  if (adc_channel >= AUX_ADC_MAX) {
    return AUXADC_ERROR_CHANNEL_NUMBER;
  }
  adc_info = &auxadc_info[adc_channel];

  irq_num = adc_info->irq_number;

  NVIC_DisableIRQ((IRQn_Type)irq_num);

  adc_info->callback = NULL;
  adc_info->callback_param = NULL;

  return AUXADC_ERROR_NONE;
}

Auxadc_Err_Code DRV_AUXADC_Set_Scale(uint32_t scaling_multiplier) {
  auxadc_scaling_multiplier = scaling_multiplier;
  return AUXADC_ERROR_NONE;
}

Auxadc_Err_Code DRV_AUXADC_Get_Value(auxadc_enum_t adc_channel, uint8_t average_count,
                                     uint32_t *adc_value) {
  uint32_t result;
  uint32_t gpio_num;
  auxadc_info_t *adc_info;
  AUX_ADC_Type *base;
  Auxadc_Err_Code ret;

  if (adc_channel >= AUX_ADC_MAX) {
    return AUXADC_ERROR_CHANNEL_NUMBER;
  }

  if (adc_value == NULL) {
    return AUXADC_ERROR_PARAMETER;
  }

  if (average_count > 127 || average_count == 0) {
    return AUXADC_ERROR_PARAMETER;
  }

  adc_info = &auxadc_info[adc_channel];
  base = adc_info->base;
  gpio_num = CONVERT_ADC_CH_TO_GPIO(adc_channel);

  /* If previous conversion have not ended yet, return error */
  if (base->CFG_b.EN_SAMPLE != 0) {
    ret = AUXADC_ERROR_RESOURCE_BUSY;
    goto error_exit;
  }

  // config IO to input floating for measuring correct voltage.
  if (config_io(gpio_num)) {
    ret = AUXADC_ERROR_IO_CONFIG;
    goto error_exit;
  }
  /* Enable source */
  base->AVG_CNT = average_count & AUX_ADC_B0_AVG_CNT_AVG_CNT_Msk;
  base->CFG_b.EN_SAMPLE = 1;

  if (adc_info->callback == NULL) {
    /* polling mode */
    while (base->AVG_CNT != 0)
      ;

    result = base->AVG_ACC / average_count;

    if ((result) > 4095) {
      ret = AUXADC_ERROR_OUT_OF_RANGE;
      goto exit;
    }

    *adc_value = adc_digital_2mv(result);
    ret = AUXADC_ERROR_NONE;
    goto exit;

  } else {
    /* interrupt mode */
    adc_info->average_count = average_count;
    base->IRPT_EN_b.AVG_DONE = 1;
    return AUXADC_ERROR_WAIT_INTERRUPT;
  }

exit:
  base->CFG_b.EN_SAMPLE = 0;
error_exit:
  return ret;
}

int DRV_AUXADC_Get_Bat_Sns(void) {
  pmp_mcu_map_shared_info shared_info;
  uint16_t res;
  uint16_t v_ref;
  uint32_t D;
  int result = (-1);

  if (!DRV_PMP_AGENT_GetSharedInfo(&shared_info)) {
    /*get data provided by PMP in mailbox shared memory*/
    res = shared_info.AuxadcSharedInfo.resolution;
    v_ref = shared_info.AuxadcSharedInfo.selref;

    /*perform computations as described in mini-SRS of auxadc*/
    D = shared_info.AuxadcSharedInfo.vbat_meas_result / shared_info.AuxadcSharedInfo.num_of_samples;
    result = (D * v_ref * 3) / res;
  }
  return result;
}

int32_t DRV_AUXADC_Get_Temperature(void) {
  pmp_mcu_map_shared_info shared_info;
  int result = (-255);

  if (!DRV_PMP_AGENT_GetSharedInfo(&shared_info)) {
    result = shared_info.AuxadcSharedInfo.temp_meas_result;
  }

  return result;
}
