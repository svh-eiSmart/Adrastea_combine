/*  ---------------------------------------------------------------------------

    (c) copyright 2018 Altair Semiconductor, Ltd. All rights reserved.

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

#include "FreeRTOS.h"
#include <semphr.h>
#include <stdio.h>
#include "portmacro.h"
#include "125X_mcu.h"
#include "auxadc.h"

#define AUX_ADC_ADDR_DIFF (BASE_ADDRESS_AUX_ADC_B1 - BASE_ADDRESS_AUX_ADC_B0)
#define AUX_ADC_CFG(x) TOPREG(AUX_ADC_B0_B0_CFG + (x * AUX_ADC_ADDR_DIFF))
#define AUX_ADC_CFG_EN_SAMPLE_MSK AUX_ADC_B0_B0_CFG_B0_EN_SAMPLE_MSK
#define AUX_ADC_CFG_EN_SAMPLE_POS AUX_ADC_B0_B0_CFG_B0_EN_SAMPLE_POS
#define AUX_ADC_CFG_EN_OVERWRITE_MSK AUX_ADC_B0_B0_CFG_B0_EN_OVERWRITE_MSK
#define AUX_ADC_CFG_EN_OVERWRITE_POS AUX_ADC_B0_B0_CFG_B0_EN_OVERWRITE_POS
#define AUX_ADC_CFG_EN_AGU_MSK AUX_ADC_B0_B0_CFG_B0_EN_AGU_MSK
#define AUX_ADC_CFG_EN_AGU_POS AUX_ADC_B0_B0_CFG_B0_EN_AGU_POS
#define AUX_ADC_CFG_AGU_ACCUMULATE_MSK AUX_ADC_B0_B0_CFG_B0_AGU_ACCUMULATE_MSK
#define AUX_ADC_CFG_AGU_ACCUMULATE_POS AUX_ADC_B0_B0_CFG_B0_AGU_ACCUMULATE_POS
#define AUX_ADC_CFG_EN_GATED_MSK AUX_ADC_B0_B0_CFG_B0_EN_GATED_MSK
#define AUX_ADC_CFG_EN_GATED_POS AUX_ADC_B0_B0_CFG_B0_EN_GATED_POS
#define AUX_ADC_CFG_AVG_PROGRAM_MSK AUX_ADC_B0_B0_CFG_B0_AVG_PROGRAM_MSK
#define AUX_ADC_CFG_AVG_PROGRAM_POS AUX_ADC_B0_B0_CFG_B0_AVG_PROGRAM_POS
#define AUX_ADC_CFG_AVG_PROGRAM_EN_MSK AUX_ADC_B0_B0_CFG_B0_AVG_PROGRAM_EN_MSK
#define AUX_ADC_CFG_AVG_PROGRAM_EN_POS AUX_ADC_B0_B0_CFG_B0_AVG_PROGRAM_EN_POS
#define AUX_ADC_AVG_CNT(x) TOPREG(AUX_ADC_B0_B0_AVG_CNT + (x * AUX_ADC_ADDR_DIFF))
#define AUX_ADC_AVG_CNT_MSK AUX_ADC_B0_B0_AVG_CNT_B0_AVG_CNT_MSK
#define AUX_ADC_AVG_CNT_POS AUX_ADC_B0_B0_AVG_CNT_B0_AVG_CNT_POS
#define AUX_ADC_AVG_ACC(x) TOPREG(AUX_ADC_B0_B0_AVG_ACC + (x * AUX_ADC_ADDR_DIFF))
#define AUX_ADC_AVG_ACC_MSK AUX_ADC_B0_B0_AVG_ACC_B0_AVG_ACC_MSK
#define AUX_ADC_AVG_ACC_POS AUX_ADC_B0_B0_AVG_ACC_B0_AVG_ACC_POS
#define AUX_ADC_IRPT_CLR(x) TOPREG(AUX_ADC_B0_B0_IRPT_CLR + (x * AUX_ADC_ADDR_DIFF))
#define AUX_ADC_IRPT_CLR_AVG_DONE_MSK AUX_ADC_B0_B0_IRPT_CLR_AVG_DONE_MSK
#define AUX_ADC_IRPT_CLR_AVG_DONE_POS AUX_ADC_B0_B0_IRPT_CLR_AVG_DONE_POS
#define AUX_ADC_IRPT_EN(x) TOPREG(AUX_ADC_B0_B0_IRPT_EN + (x * AUX_ADC_ADDR_DIFF))
#define AUX_ADC_IRPT_EN_AVG_DONE_MSK AUX_ADC_B0_B0_IRPT_EN_AVG_DONE_MSK
#define AUX_ADC_IRPT_EN_AVG_DONE_POS AUX_ADC_B0_B0_IRPT_EN_AVG_DONE_POS
#define AUX_ADC_IRPT(x) TOPREG(AUX_ADC_B0_B0_IRPT + (x * AUX_ADC_ADDR_DIFF))
#define AUX_ADC_IRPT_AVG_DONE_MSK AUX_ADC_B0_B0_IRPT_AVG_DONE_MSK
#define AUX_ADC_IRPT_AVG_DONE_POS AUX_ADC_B0_B0_IRPT_AVG_DONE_POS
#define GPM_IO_CFG(x) TOPREG(GPM_IO_CFG_IO_CFG_GPIO0 + (x * 0x04))
#define MCU_IO_FUNC_SEL(x) (MCU_IO_FUNC_SEL_IO_SEL_GPIO0 + (x * 0x04))
#define GPM_SECURED_MASK_AUX_ADC_SEC(x) TOPREG(GPM_SECURED_MASK_AUX_ADC_B0_SEC + (x * 0x04))

#define AUX_ADC_SCALING_MULTIPLIER (1800)
#define AUX_ADC_SCALING_DEVIDER (4096)
#define AUX_ADC_DEFAULT_MCU_FIREWALL_BIT (0x4)
#define AUX_ADC_MS_WAIT_TIME (0x01)
#define AUX_ADC_VBAT_DIVIDER_COMPENSATOR (3)
#define AUX_ADC_VBAT_MAX_VOLTAGE (4500)
#define AUX_ADC_VBAT_HIGH_BYTES_MASK (0xFFFF0000)
#define AUX_ADC_VBAT_LOW_BYTES_MASK (0xFFFF)
#define AUX_ADC_VBAT_HIGH_BYTES_POS (16)
#define AUX_ADC_DIVIDER_PRESCALER (8)
#if defined(ALT1250)
#define CONVERT_ADC_CH_TO_GPIO(x) (x)
#elif defined(ALT1255)
#define CONVERT_ADC_CH_TO_GPIO(x) (x + 1)
#endif
#define GPIO_MUX_VALUE (4)
#define CONVERT_ADC_CH_TO_IRQ_NUM(x) (x + AUXADC_INT0)
#define LOCK() xSemaphoreTake(auxadc_data_mtx, portMAX_DELAY)
#define UNLOCK() xSemaphoreGive(auxadc_data_mtx)

typedef struct {
  int id;
  int average_count;
  const char *auxadc_interrupt_label;
  uint32_t int_counter;
  uint32_t user_param;
  auxadc_callback irq_handler_user;
} auxadc_info_t;

static SemaphoreHandle_t auxadc_data_mtx = NULL;
static uint32_t auxadc_scaling_multiplier = AUX_ADC_SCALING_MULTIPLIER;
static auxadc_info_t auxadc_info[AUX_ADC_MAX] = {
    {
        .id = 0,
        .auxadc_interrupt_label = "AUX_ADC_0",
        .int_counter = 0,
        .user_param = 0,
        .irq_handler_user = NULL,
    },
    {
        .id = 1,
        .auxadc_interrupt_label = "AUX_ADC_1",
        .int_counter = 0,
        .user_param = 0,
        .irq_handler_user = NULL,
    },
    {
        .id = 2,
        .auxadc_interrupt_label = "AUX_ADC_2",
        .int_counter = 0,
        .user_param = 0,
        .irq_handler_user = NULL,
    },
#ifdef ALT1250
    {
        .id = 3,
        .auxadc_interrupt_label = "AUX_ADC_3",
        .int_counter = 0,
        .user_param = 0,
        .irq_handler_user = NULL,
    },
#endif
};

/*****************************************************************************
 *  Function: adc_digital_2mv
 *
 *  Parameters:    ADC digital value
 *
 *  Returns:       ADC analog value in mV
 *
 *
 *****************************************************************************/
static int adc_digital_2mv(uint32_t dig_value) {
  return (dig_value * auxadc_scaling_multiplier) / AUX_ADC_SCALING_DEVIDER;
}

static void auxadc_HandleISR(int id) {
  uint32_t result;
  int err = 0;

  if (REGISTER(AUX_ADC_IRPT(id)) & AUX_ADC_IRPT_AVG_DONE_MSK) {

    result = REGISTER(AUX_ADC_AVG_ACC(id));

    auxadc_info[id].int_counter++;
    if (auxadc_info[id].irq_handler_user) {
      result = result / auxadc_info[id].average_count;
      if(result > 4095 )
      {
        err = -AUX_ADC_PARAMETERS_OUT_OF_RANGE;
        result = -1;
      }
      else
        result = adc_digital_2mv(result);

      auxadc_info[id].irq_handler_user(err, result, auxadc_info[id].user_param);
    }
    /*Disable source after conversion is over*/
    REGISTER(AUX_ADC_IRPT_EN(id)) &= ~AUX_ADC_IRPT_EN_AVG_DONE_MSK;
    REGISTER(AUX_ADC_CFG(id)) &= ~AUX_ADC_CFG_EN_SAMPLE_MSK;
  }
}

static void config_io(int gpio_num) {
  /* Modify pin to input */
  REGISTER(MCU_GPIO_0_DIR_CLR) |= 1 << gpio_num;

  REGISTER(MCU_IO_FUNC_SEL(gpio_num)) = GPIO_MUX_VALUE;

  /* Modify default pull up/down to pull disabled */
  if (REGISTER(GPM_IO_CFG(gpio_num)) & GPM_IO_CFG_IO_CFG_GPIO0_IO_CFG_PE_GPIO0_MSK) {
    REGISTER(GPM_IO_CFG(gpio_num)) &= ~GPM_IO_CFG_IO_CFG_GPIO0_IO_CFG_PE_GPIO0_MSK;
    msleep(100);  // add a delay to wait capacitor discharge.
  }
}

/***********************   interrupt handler  ***************************/

void auxdac0_handler(void) { auxadc_HandleISR(AUX_ADC_0); }

void auxdac1_handler(void) { auxadc_HandleISR(AUX_ADC_1); }

void auxdac2_handler(void) { auxadc_HandleISR(AUX_ADC_2); }

void auxdac3_handler(void) { auxadc_HandleISR(AUX_ADC_3); }

/********************** APIs *****************************/

int auxadc_init(void) {
  auxadc_data_mtx = xSemaphoreCreateMutex();
  if (auxadc_data_mtx == NULL) return -AUX_ADC_INIT_FAIL;
  return 0;
}

int auxadc_register_interrupt(auxadc_enum_t adc_channel, auxadc_callback irq_handler,
                              uint32_t user_param) {
  if (auxadc_data_mtx == NULL) return -AUX_ADC_UNINIT;

  if (adc_channel >= AUX_ADC_MAX) return -AUX_ADC_INVALID_CHANNEL_NUMBER;

  int irq_num = CONVERT_ADC_CH_TO_IRQ_NUM(adc_channel);

  if (NVIC_GetEnableIRQ(irq_num) == 1) return -AUX_ADC_INTERRUPT_REGISTERED;

  REGISTER(AUX_ADC_IRPT_CLR(adc_channel));
  REGISTER(AUX_ADC_IRPT_EN(adc_channel)) &= ~AUX_ADC_IRPT_EN_AVG_DONE_MSK;

  NVIC_SetPriority((IRQn_Type)irq_num, 7); /* set Interrupt priority */
  NVIC_EnableIRQ((IRQn_Type)irq_num);

  if (irq_handler) {
    LOCK();
    auxadc_info[adc_channel].irq_handler_user = irq_handler;
    auxadc_info[adc_channel].user_param = user_param;
    UNLOCK();
  }
  return 0;
}

int auxadc_deregister_interrupt(auxadc_enum_t adc_channel) {
  int irq_num;

  if (auxadc_data_mtx == NULL) return -AUX_ADC_UNINIT;

  if (adc_channel >= AUX_ADC_MAX) return -AUX_ADC_INVALID_CHANNEL_NUMBER;

  irq_num = CONVERT_ADC_CH_TO_IRQ_NUM(adc_channel);

  NVIC_DisableIRQ((IRQn_Type)irq_num);

  if (auxadc_info[adc_channel].irq_handler_user != NULL) {
    LOCK();
    auxadc_info[adc_channel].irq_handler_user = NULL;
    auxadc_info[adc_channel].user_param = 0;
    UNLOCK();
  }

  return 0;
}

void adc_set_scaling(uint32_t scaling_multiplier) {
  auxadc_scaling_multiplier = scaling_multiplier;
}

int adc_get_value(auxadc_enum_t adc_channel, uint32_t average_count, uint32_t *adc_value) {
  int result;
  int source_select_index = adc_channel;
  int gpio_num;
  int i;

  if (auxadc_data_mtx == NULL) return -AUX_ADC_UNINIT;

  /*-2 invalid pin/port number*/
  if (source_select_index >= AUX_ADC_MAX) {
    return -AUX_ADC_INVALID_CHANNEL_NUMBER;
  }

  if (average_count == 0 || average_count > 127) return -AUX_ADC_PARAMETERS_OUT_OF_RANGE;

  if (adc_value == NULL) return -AUX_ADC_OTHER_ERROR;

  gpio_num = CONVERT_ADC_CH_TO_GPIO(source_select_index);

  /* check whether pin is available to be accessed by MCU CPU
   * -4 - pin/port forbidden for this CPU */
  if ((REGISTER(TOPREG(PMP_SECURED_REGS_GPM_DEBUG)) &
       PMP_SECURED_REGS_GPM_DEBUG_SECURITY_ENABLE_MSK) &&
      (!(REGISTER(GPM_SECURED_MASK_AUX_ADC_SEC(source_select_index)) &
         AUX_ADC_DEFAULT_MCU_FIREWALL_BIT))) {
    return -AUX_ADC_PIN_FORBIDDEN;
  }

  config_io(gpio_num);  // config IO for measuring correct voltage.
  /* If previous conversion have not ended yet, return error */
  if ((REGISTER(AUX_ADC_CFG(source_select_index)) & AUX_ADC_CFG_EN_SAMPLE_MSK) == 0) {
    REGISTER(AUX_ADC_AVG_CNT(source_select_index)) = (average_count & AUX_ADC_AVG_CNT_MSK);
  } else
    return -AUX_ADC_RESOURSE_BUSY;

  /* Enable source */
  REGISTER(AUX_ADC_CFG(source_select_index)) |= AUX_ADC_CFG_EN_SAMPLE_MSK;

  if (auxadc_info[source_select_index].irq_handler_user == NULL) {
    for (i = 0; i < 20; i++) {
      msleep(AUX_ADC_MS_WAIT_TIME);
      if ((REGISTER(AUX_ADC_AVG_CNT(source_select_index))) == 0) break;
    }
    if ((REGISTER(AUX_ADC_AVG_CNT(source_select_index))) == 0)
      result = (REGISTER(AUX_ADC_AVG_ACC(source_select_index)) & AUX_ADC_AVG_ACC_MSK);
    else {
      REGISTER(AUX_ADC_CFG(source_select_index)) &= ~AUX_ADC_CFG_EN_SAMPLE_MSK;
      return -AUX_ADC_OTHER_ERROR;
    }
    result /= average_count;
    /* -3  parameter(s) out of range */
    if ((result) > 4095) {
      REGISTER(AUX_ADC_CFG(source_select_index)) &= ~AUX_ADC_CFG_EN_SAMPLE_MSK;
      return -AUX_ADC_PARAMETERS_OUT_OF_RANGE;
    }

    *adc_value = adc_digital_2mv(result);
    REGISTER(AUX_ADC_CFG(source_select_index)) &= ~AUX_ADC_CFG_EN_SAMPLE_MSK;
    return result;
  } else {
    auxadc_info[source_select_index].average_count = average_count;
    REGISTER(AUX_ADC_IRPT_EN(source_select_index)) |= AUX_ADC_IRPT_EN_AVG_DONE_MSK;
    return -AUX_ADC_INTERRUPT_WITH_HANDLER;
  }
}

int adc_get_bat_sns(void) {
  int devider = 1, resolution = 1, fullscale = 0;
  uint32_t result = 0, mask = AUX_ADC_VBAT_LOW_BYTES_MASK, position = 0, address, base_addr;

  /*if AGU is empty return error*/
  if ((address = REGISTER(TOPREG(AUX_ADC_B7_B7_AGU_STATUS))) == 0) {
    return -1;
  }

  /* if address is not at the start of the word i.e. reminder of division by 4 is not zero
   * address of the result need to be modified to get the result from the start of the word*/
  base_addr = REGISTER(TOPREG(AUX_ADC_SECURED_B7_AGU_BASE));
  if (address == base_addr) {
    address = base_addr + REGISTER(TOPREG(AUX_ADC_SECURED_B7_AGU_WRAP)) - 2;
  }

  if (address % 4) {
    address -= 2;
    mask = AUX_ADC_VBAT_HIGH_BYTES_MASK;
    position = AUX_ADC_VBAT_HIGH_BYTES_POS;
  }

  /* The address is decided by PMP*/
  result = REGISTER(TOPREG(address + BASE_ADDRESS_GPM_MEM));

  /*Only upper 16 bits of the result of accumulation are saved*/
  result = (result & mask) >> position;

  /* Because 16 bits saved of 19 bits it means that the result
   * placed in AGU is devided by 8 , so to get averaged result
   * it need to be devided by AVG_PROGRAM register, which filled by PMP*/
  devider = ((REGISTER(TOPREG(AUX_ADC_B7_B7_CFG)) & AUX_ADC_B7_B7_CFG_B7_AVG_PROGRAM_MSK) >>
             AUX_ADC_B7_B7_CFG_B7_AVG_PROGRAM_POS);
  devider /= AUX_ADC_DIVIDER_PRESCALER;

  /* in case of 0 division return error */
  if (devider == 0) {
    return -2;
  }

  /*resolution and fullscale variables are chosen by PMP*/
  resolution =
      1 << (6 + 2 * ((REGISTER(TOPREG(AUX_ADC_AUX_ADC_CFG)) & AUX_ADC_AUX_ADC_CFG_SEL_RES_MSK) >>
                     AUX_ADC_AUX_ADC_CFG_SEL_RES_POS));
  fullscale = AUX_ADC_SCALING_MULTIPLIER -
              (600 * ((REGISTER(TOPREG(AUX_ADC_AUX_ADC_CFG)) & AUX_ADC_AUX_ADC_CFG_SELREF_MSK) >>
                      AUX_ADC_AUX_ADC_CFG_SELREF_POS));

  /*Battery voltage have a divider of 3 which need to be compensated*/
  result = (result / devider) * AUX_ADC_VBAT_DIVIDER_COMPENSATOR;

  /*return result in mV*/
  result = (result * fullscale) / resolution;

  if (result > AUX_ADC_VBAT_MAX_VOLTAGE) {
    return -3;
  }

  return result;
}
