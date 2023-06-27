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
#ifndef GPIO_WAKEUP_UTILS_ALT125X_H
#define GPIO_WAKEUP_UTILS_ALT125X_H
#include "gpio_wakeup.h"
#include "sleep_notify.h"

// typedef void (*gpio_interrupt_handler)(unsigned int user_param);

typedef enum gpio_wakeup_lookup_method {
  GPIO_WAKEUP_REGISTER_LOOKUP,
  GPIO_WAKEUP_STRUCT_LOOKUP
} gpio_wakeup_lookup_method_e;

/*read number of interrupts performed*/
unsigned int get_interrupt_count(unsigned int pin_number);

/*wakeup for debug purposes, every changes need to be hardcoded*/
void full_manual_wakeup(void);

/*this function should be inserted in system.c*/
void gpio_wakeup_init(void);

/*void function for debug purposes*/
void gpio_wakeup_dummy_interrupt(unsigned int param);

/*look or wakeup machine by pin number*/
int gpio_wakeup_find_io(unsigned int pin_number);

/*read setup form registers*/
void gpio_wakeup_get_setup(gpio_wakeup_struct_t* gpio_wakeup_elem, unsigned int pin_number,
                           enum gpio_wakeup_lookup_method method);

/*print wakeup structure*/
void print_struct(gpio_wakeup_struct_t* struct_to_print);

/*  Register interrupt*/
int gpio_wakeup_register_interrupt(unsigned int gpio, gpio_interrupt_handler irq_handler,
                                   unsigned int user_param);

int gpio_wakeup_notify(sleep_notify_state sleep_state, void* ptr_ctx);

#endif
