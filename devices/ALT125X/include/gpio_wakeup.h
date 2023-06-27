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

#ifndef ALT12XX_MCU_GPIO_WAKEUP_ALT125X_H_
#define ALT12XX_MCU_GPIO_WAKEUP_ALT125X_H_

#include "FreeRTOS.h"
#include <stdio.h>
#include "portmacro.h"
#include "gpio.h"

/*  Clock types*/
typedef enum _gpio_wakeup_clk {
  GPIO_WAKEUP_RTC_CLK,   /*RTC ~32KHz*/
  GPIO_WAKEUP_URCLK_CLK, /*URCLK ~4MHz*/
  GPIO_WAKEUP_ASYNC_CLK  /*async*/
} gpio_wakeup_clk_e;

/*  Polarity types*/
typedef enum _gpio_wakeup_polarity_mode_enum {
  GPIO_WAKEUP_EDGE_RISING,
  GPIO_WAKEUP_EDGE_FALLING,
  GPIO_WAKEUP_EDGE_RISING_FALLING,
  GPIO_WAKEUP_LEVEL_POL_HIGH,
  GPIO_WAKEUP_LEVEL_POL_LOW
} gpio_wakeup_polarity_mode_e;

typedef struct {
  int gpio_pin;
  gpio_wakeup_clk_e clk;
  gpio_wakeup_polarity_mode_e mode;
  unsigned int debounce_en;
  unsigned int debounce_max_val;
  unsigned int wakeup_en;
  unsigned int int_en;
  gpio_interrupt_handler irq_handler;
  unsigned int user_param;
  unsigned int int_counter;
} gpio_wakeup_struct_t;

/***************USER API's****************/

/* Register the IO as wakeup with given parameters */
int gpio_wakeup_add_io(gpio_wakeup_struct_t* params);

/* Delete wakeup */
int gpio_wakeup_delete_io(unsigned int pin_number);

/* Disable wakeup for the specified IO */
int gpio_wakeup_disable(unsigned int pin_number);

/* Enable wakeup for the specified IO*/
int gpio_wakeup_enable(unsigned int pin_number);

/* Select polarity mode */
int gpio_wakeup_polarity_set(unsigned int pin_number, gpio_wakeup_polarity_mode_e mode);

/* Select GPIO wakeup clock */
int gpio_wakeup_clk_select(unsigned int pin_number, gpio_wakeup_clk_e clk_id);

/* Enable debounce */
int gpio_wakeup_debounce_enable(unsigned int pin_number);

/* Disable debounce */
int gpio_wakeup_debounce_disable(unsigned int pin_number);

/* Set debounce count value */
int gpio_wakeup_debounce_set_val(unsigned int pin_number, unsigned int val);

/* Enable interrupt for the specified IO */
int gpio_wakeup_interrupt_enable(unsigned int pin_number);

/* Disable interrupt for specified IO at active state,
 * but continue to function as wakeup source in sleep
 * state*/
int gpio_wakeup_interrupt_disable(unsigned int pin_number);

#endif /* ALT12XX_MCU_GPIO_WAKEUP_ALT125X_H_ */
