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
#ifndef _ATOMIC_COUNTERS_H
#define _ATOMIC_COUNTERS_H

#include <stdint.h>
#include <stdbool.h>

#define IN  // TODO: move to general place

/*ISR prototype*/
typedef void (*DRV_ATOM_CNTR_InterruptHandler)(uint32_t user_param);

/*The different IRQ modes for AC*/
#define ATOMIC_COUNTER_NOT_ZERO_INTERRUPT_MODE_VALUE 0
#define ATOMIC_COUNTER_INC_INTERRUPT_MODE_VALUE 1
#define ATOMIC_COUNTER_DEC_INTERRUPT_MODE_VALUE 2
#define ATOMIC_COUNTER_DEC_OR_INC_INTERRUPT_MODE_VALUE 3
#define ATOMIC_COUNTER_DISABLE_INTERRUPT_MODE_VALUE 4

enum ac_domain { TOP_DOMAIN, MAC_DOMAIN };

/*Note that the bits in mask register are in revers order 0 is 15 14 is 1 etc...*/
enum top_modem_maks_bit {
  ATOMIC_COUNTER_MAILBOX15_BIT,
  ATOMIC_COUNTER_MAILBOX14_BIT,
  ATOMIC_COUNTER_MAILBOX13_BIT,
  ATOMIC_COUNTER_MAILBOX12_BIT,
  ATOMIC_COUNTER_MAILBOX11_BIT,
  ATOMIC_COUNTER_MAILBOX10_BIT,
  ATOMIC_COUNTER_MAILBOX9_BIT,
  ATOMIC_COUNTER_MAILBOX8_BIT,
  ATOMIC_COUNTER_MAILBOX7_BIT,
  ATOMIC_COUNTER_MAILBOX6_BIT,
  ATOMIC_COUNTER_MAILBOX5_BIT,
  ATOMIC_COUNTER_MAILBOX4_BIT,
  ATOMIC_COUNTER_MAILBOX3_BIT,
  ATOMIC_COUNTER_MAILBOX2_BIT,
  ATOMIC_COUNTER_MAILBOX1_BIT,
  ATOMIC_COUNTER_MAILBOX0_BIT
};

/*List of supported atomic counters*/
#define ATOMIC_COUNT_LIST "DRV_ATOMIC_COUNTER_INTERFACE.h"

/*This will create the names and number of atomic counter from the atomic_counters_interface.h
 * file*/
#define ATOMIC_COUNTER(enumName, domin, a, b, c, d) enumName,
typedef enum {
#include ATOMIC_COUNT_LIST
  ATOMIC_COUNTER_MAX_INDEX
} DRV_ATOM_CNTR_Id;
#undef ATOMIC_COUNTER

/*Call this first to initialize the atomic counter from the interface table*/
void DRV_ATOM_CNTR_InitCounter(IN DRV_ATOM_CNTR_Id counter,
                               DRV_ATOM_CNTR_InterruptHandler irq_handler, uint32_t user_param);
/*Increment AC by val*/
void DRV_ATOM_CNTR_Increment(IN DRV_ATOM_CNTR_Id counter, IN uint8_t val);

/*Decrement AC by val*/
void DRV_ATOM_CNTR_Decrement(IN DRV_ATOM_CNTR_Id counter, IN uint8_t val);

/*Set the AC mode to one of the supported modes*/
void DRV_ATOM_CNTR_SetIrqMode(IN DRV_ATOM_CNTR_Id counter, IN uint8_t mode);

/*Enable AC IRQ*/
void DRV_ATOM_CNTR_EnableIRQ(IN DRV_ATOM_CNTR_Id counter);

/*Disable AC IRQ*/
void DRV_ATOM_CNTR_DisableIRQ(IN DRV_ATOM_CNTR_Id counter);

/*Set the value of the AC*/
void DRV_ATOM_CNTR_SetCount(IN DRV_ATOM_CNTR_Id counter, IN uint8_t count);

/*Get the AC corrent value*/
uint32_t DRV_ATOM_CNTR_GetCount(IN DRV_ATOM_CNTR_Id counter);

/*Restore the atomic counter register to default value*/
void DRV_ATOM_CNTR_Initialize();
#endif
