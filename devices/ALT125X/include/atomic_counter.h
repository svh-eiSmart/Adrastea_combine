/*  ---------------------------------------------------------------------------

    (c) copyright 2017 Altair Semiconductor, Ltd. All rights reserved.

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

#ifndef _ATOMIC_COUNTERS_H
#define _ATOMIC_COUNTERS_H

#include <stdint.h>
#include <stdbool.h>
//#include "cpu_alt1250.h"
//#include "irq_alt1250.h"
//#include "interrupt_index_enum.h"
#include "125X_mcu.h"

#define IN  // TODO: move to general place

typedef struct ac_drv_data {
  int top_irq_register;
} ac_drv;

/*ISR prototype*/
typedef void (*atomic_counter_interrupt_handler)(unsigned int user_param);

/*The different IRQ modes for AC*/
#define ATOMIC_COUNTER_NOT_ZERO_INTERRUPT_MODE_VALUE 0
#define ATOMIC_COUNTER_INC_INTERRUPT_MODE_VALUE 1
#define ATOMIC_COUNTER_DEC_INTERRUPT_MODE_VALUE 2
#define ATOMIC_COUNTER_DEC_OR_INC_INTERRUPT_MODE_VALUE 3
#define ATOMIC_COUNTER_DISABLE_INTERRUPT_MODE_VALUE 4

enum ac_domain { TOP_DOMAIN, MAC_DOMAIN };

/*Note that the bits in mask register are in revers order 0 is 15 14 is 1 etc...*/
enum top_modem_maks_bit {
  ATOMIC_COUNTER_MAILBOX15,
  ATOMIC_COUNTER_MAILBOX14,
  ATOMIC_COUNTER_MAILBOX13,
  ATOMIC_COUNTER_MAILBOX12,
  ATOMIC_COUNTER_MAILBOX11,
  ATOMIC_COUNTER_MAILBOX10,
  ATOMIC_COUNTER_MAILBOX9,
  ATOMIC_COUNTER_MAILBOX8,
  ATOMIC_COUNTER_MAILBOX7,
  ATOMIC_COUNTER_MAILBOX6,
  ATOMIC_COUNTER_MAILBOX5,
  ATOMIC_COUNTER_MAILBOX4,
  ATOMIC_COUNTER_MAILBOX3,
  ATOMIC_COUNTER_MAILBOX2,
  ATOMIC_COUNTER_MAILBOX1,
  ATOMIC_COUNTER_MAILBOX0
};

/*List of supported atomic counters*/
#define ATOMIC_COUNT_LIST "atomic_counter_interface.h"

/*This will create the names and number of atomic counter from the atomic_counters_interface.h
 * file*/
#define ATOMIC_COUNTER(enumName, domin, a, b, c, d) enumName,
typedef enum {
#include ATOMIC_COUNT_LIST
  ATOMIC_COUNTER_MAX_INDEX
} atomic_counter_id;
#undef ATOMIC_COUNTER

/*Call this first to initialize the atomic counter from the interface table*/
void atomic_counters_init_counter(IN atomic_counter_id counter,
                                  atomic_counter_interrupt_handler irq_handler,
                                  uint32_t user_param);
/*Increment AC by val*/
void atomic_counters_increment(IN atomic_counter_id counter, IN uint8_t val);
/*Decrement AC by val*/
void atomic_counters_decrement(IN atomic_counter_id counter, IN uint8_t val);
/*Set the AC mode to one of the supported modes*/
void atomic_counters_set_irq_mode(IN atomic_counter_id counter, IN uint8_t mode);
/*Enable AC IRQ*/
void atomic_counters_ena_irq(IN atomic_counter_id counter);
/*Disable AC IRQ*/
void atomic_counters_dis_irq(IN atomic_counter_id counter);
/*Set the value of the AC*/
void atomic_counters_set_count(IN atomic_counter_id counter, IN uint8_t count);
/*Get the AC corrent value*/
uint32_t atomic_counters_get_count(IN atomic_counter_id counter);
/*Restore the atomic counter register to default value*/
void atomic_counter_init();
#endif
