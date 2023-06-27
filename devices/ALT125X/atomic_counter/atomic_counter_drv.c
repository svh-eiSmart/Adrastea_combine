/****************************************************************************
 *
 *  (c) copyright 2019 Altair Semiconductor, Ltd. All rights reserved.
 *
 *  This software, in source or object form (the "Software"), is the
 *  property of Altair Semiconductor Ltd. (the "Company") and/or its
 *  licensors, which have all right, title and interest therein, You
 *  may use the Software only in  accordance with the terms of written
 *  license agreement between you and the Company (the "License").
 *  Except as expressly stated in the License, the Company grants no
 *  licenses by implication, estoppel, or otherwise. If you are not
 *  aware of or do not agree to the License terms, you may not use,
 *  copy or modify the Software. You may use the source code of the
 *  Software only for your internal purposes and may not distribute the
 *  source code of the Software, any part thereof, or any derivative work
 *  thereof, to any third party, except pursuant to the Company's prior
 *  written consent.
 *  The Software is the confidential information of the Company.
 *
 ****************************************************************************/

#include <stdio.h>

#include "FreeRTOS.h"
#include "atomic_counter.h"
#include "atomic_counter_drv.h"
#include "125X_mcu.h"

#define ATOMIC_COUNTERS_DB_SIZE 2000

#define ATM_DEBUG 0
#define dbg_print(fmt, args...) \
  {                             \
    if (ATM_DEBUG) {            \
      printf(fmt, ##args);      \
    }                           \
  }
#define err_print(fmt, args...) \
  { printf(fmt, ##args); }

/*Global structure which holds AC related data*/
ac_drv g_ac_drv;

/*Each AC is represented in the DB by this structure*/
typedef struct {
  uint8_t ac_domin;
  /*The register base address*/
  uint32_t atomic_counter_base_addr;
  /*The register value address*/
  uint32_t atomic_counter_value_addr;
  /*IRQ mode*/
  uint8_t atomic_counter_intr_mode;
  /*Bit number in the mask register for TOP domain*/
  uint8_t mask_bit;
  /*User ISR*/
  atomic_counter_interrupt_handler irq_handler;
  /*User param to pass when there is ISR*/
  uint32_t user_param;
} atomicCounterDB_t;

/*This will create a DB of atomic counters from the atomic_counter_interface.h file*/
#define ATOMIC_COUNTER(idx, domain, base, value, interrupt_mode, mask_bit) \
  {domain, base, value, interrupt_mode, mask_bit, NULL, 0},
atomicCounterDB_t atomicCounterDB[ATOMIC_COUNTER_MAX_INDEX + 1] = {
#include ATOMIC_COUNT_LIST
    {0, 0, 0, 0, 0, NULL, 0}};
#undef ATOMIC_COUNTER

void MIPS_alt_insert(uint32_t* word, uint32_t value, uint32_t bitOffset, uint32_t fieldSize) {
  *word &= ~((0xffffffff >> (32 - fieldSize)) << bitOffset);  // not bitwise on bitmask
  value &= (0xffffffff >> (32 - fieldSize));
  *word |= (value << bitOffset);
};

/*AC Set val*/
void atomic_counters_set_count(IN atomic_counter_id counter, IN uint8_t count) {
  dbg_print("%s AC %d,Count %d\r\n", __FUNCTION__, counter, count);
  *(HWRegisterPtr)atomicCounterDB[counter].atomic_counter_value_addr = count;
}

/*AC get val*/
uint32_t atomic_counters_get_count(IN atomic_counter_id counter) {
  dbg_print("%s AC %d Val %ld\r\n", __FUNCTION__, counter,
            *(HWRegisterPtr)atomicCounterDB[counter].atomic_counter_value_addr);
  return *(HWRegisterPtr)atomicCounterDB[counter].atomic_counter_value_addr;
}

/*AC increment*/
void atomic_counters_increment(IN atomic_counter_id counter, IN uint8_t val) {
  dbg_print("%s AC %d,val %d\r\n", __FUNCTION__, counter, val);
  *(HWRegisterPtr)(atomicCounterDB[counter].atomic_counter_base_addr +
                   ATOMIC_COUNTER_INC_REGISTER_OFFSET) = val;
}

/*AC decrement*/
void atomic_counters_decrement(IN atomic_counter_id counter, IN uint8_t val) {
  dbg_print("%s AC %d,val %d\r\n", __FUNCTION__, counter, val);
  *(HWRegisterPtr)(atomicCounterDB[counter].atomic_counter_base_addr +
                   ATOMIC_COUNTER_DEC_REGISTER_OFFSET) = val;
}

/*AC enable IRQ*/
void atomic_counters_ena_irq(IN atomic_counter_id counter) {
  dbg_print("%s AC %d\r\n", __FUNCTION__, counter);

  /*Read Modify write*/
  uint32_t reg = *(HWRegisterPtr)(atomicCounterDB[counter].atomic_counter_base_addr +
                                  ATOMIC_COUNTER_MOD_REGISTER_OFFSET);
  MIPS_alt_insert(&reg, (uint32_t)1, ATOMIC_COUNTER_INT_IRQ_EN_BIT_OFFSET,
                  ATOMIC_COUNTER_INT_IRQ_EN_BIT_NUM);
  *(HWRegisterPtr)(atomicCounterDB[counter].atomic_counter_base_addr +
                   ATOMIC_COUNTER_MOD_REGISTER_OFFSET) = reg;
}

/*AC disable IRQ*/
void atomic_counters_dis_irq(IN atomic_counter_id counter) {
  dbg_print("%s AC %d\r\n", __FUNCTION__, counter);

  /*Read, Modify, write*/
  uint32_t reg = *(HWRegisterPtr)(atomicCounterDB[counter].atomic_counter_base_addr +
                                  ATOMIC_COUNTER_MOD_REGISTER_OFFSET);
  MIPS_alt_insert(&reg, (uint32_t)0, ATOMIC_COUNTER_INT_IRQ_EN_BIT_OFFSET,
                  ATOMIC_COUNTER_INT_IRQ_EN_BIT_NUM);
  *(HWRegisterPtr)(atomicCounterDB[counter].atomic_counter_base_addr +
                   ATOMIC_COUNTER_MOD_REGISTER_OFFSET) = reg;
}

/*AC set IRQ mode*/
void atomic_counters_set_irq_mode(IN atomic_counter_id counter, IN uint8_t mode) {
  dbg_print("%s AC %d, mode %d\r\n", __FUNCTION__, counter, mode);

  /*Read, Modify, write*/
  uint32_t reg = *(HWRegisterPtr)(atomicCounterDB[counter].atomic_counter_base_addr +
                                  ATOMIC_COUNTER_MOD_REGISTER_OFFSET);
  MIPS_alt_insert(&reg, (uint32_t)mode, ATOMIC_COUNTER_INT_MODE_BIT_OFFSET,
                  ATOMIC_COUNTER_INT_MODE_BIT_NUM);
  *(HWRegisterPtr)(atomicCounterDB[counter].atomic_counter_base_addr +
                   ATOMIC_COUNTER_MOD_REGISTER_OFFSET) = reg;

  /*Now update the DB*/
  // TODO: DB should be in critical section
  atomicCounterDB[counter].atomic_counter_intr_mode = mode;
}

/*AC Read INT_STAT and clear IRQ*/
uint32_t atomic_counters_clear_irq(IN atomic_counter_id counter) {
  dbg_print("%s AC %d\r\n", __FUNCTION__, counter);

  volatile uint32_t temp = *(HWRegisterPtr)(atomicCounterDB[counter].atomic_counter_base_addr +
                                            ATOMIC_COUNTER_INT_STAT_REGISTER_OFFSET);

  return temp;
}

// /*AC general IRQ handler*/
// static void atomic_counter_irq_interrupt_handler(void)
// {

// }

void ATMCTR_MAILBOX_IRQhandler(void) {
  /*Go through the AC counter check for TOP Domain*/
  for (int i = 0; i < ATOMIC_COUNTER_MAX_INDEX; i++) {
    if (atomicCounterDB[i].ac_domin == TOP_DOMAIN) {
      if (atomicCounterDB[i].atomic_counter_intr_mode !=
          ATOMIC_COUNTER_DISABLE_INTERRUPT_MODE_VALUE) {
        // atomic_counters_get_count(i);
        // TODO: we can also check mask and int_en flag
        if (atomic_counters_get_count(i) && (atomicCounterDB[i].atomic_counter_intr_mode ==
                                             ATOMIC_COUNTER_NOT_ZERO_INTERRUPT_MODE_VALUE)) {
          if (atomicCounterDB[i].irq_handler)
            atomicCounterDB[i].irq_handler(atomicCounterDB[i].user_param);
        } else if (atomic_counters_clear_irq(i) !=
                   0) /*if INT_STAT returns 1 meaning there was an IRQ*/
        {
          if (atomicCounterDB[i].irq_handler)
            atomicCounterDB[i].irq_handler(atomicCounterDB[HIFC_INTERNAL_HOST_2_MODEM].user_param);
        }
      }
    }
  }
}

void atomic_counter_register_IRQ(IN atomic_counter_id counter) {
  dbg_print("%s AC %d\r\n", __FUNCTION__, counter);

  // TODO: check if it is from TOP domain
  if (!g_ac_drv.top_irq_register) {
    dbg_print("%s AC %d, register INT_ATMCTR_MAILBOX_INT_MODEM IRQ\r\n ", __FUNCTION__, counter);
    // register_interrupt(INT_ATMCTR_MAILBOX_INT_MODEM, atomic_counter_irq_interrupt_handler,
    // ATOMIC_COUNTER_IRQ_LABLE);
    NVIC_SetPriority(ATMCTR_MAILBOX_IRQn, 7); /* set Interrupt priority */
    NVIC_EnableIRQ(ATMCTR_MAILBOX_IRQn);
    g_ac_drv.top_irq_register = true;
  }
}

/*Before using a AC, the user should call the init function.*/
void atomic_counters_init_counter(IN atomic_counter_id counter,
                                  atomic_counter_interrupt_handler irq_handler,
                                  uint32_t user_param) {
#define BASE_ADDRESS_ATOMIC_COUNTER_MAILBOX_SC_MCU_MASKS (0x0D009000 + 0x0008)
  dbg_print("%s AC %d\r\n", __FUNCTION__, counter);
  uint32_t reg;
  if ((counter == 1) ||
      (counter == 3)) { /* Notice: counter-1: mbox5; counter-3: mbox9. This is depend on the
                           ordering defined in atomic_counter_interface.h  */
    /*unmask the IRQ*/
    reg = *(HWRegisterPtr)(BASE_ADDRESS_ATOMIC_COUNTER_MAILBOX_SC_MCU_MASKS);
    reg &= ~(1 << atomicCounterDB[counter].mask_bit);
    *(HWRegisterPtr)((uint32_t)(BASE_ADDRESS_ATOMIC_COUNTER_MAILBOX_SC_MCU_MASKS)) = reg;
  }
  if (counter < ATOMIC_COUNTER_MAX_INDEX) {
    // atomic_counters_set_count(counter, 0);
    atomicCounterDB[counter].irq_handler = irq_handler;
    atomicCounterDB[counter].user_param = user_param;

    if (atomicCounterDB[counter].atomic_counter_intr_mode !=
        ATOMIC_COUNTER_DISABLE_INTERRUPT_MODE_VALUE) {
      atomic_counters_set_irq_mode(counter, atomicCounterDB[counter].atomic_counter_intr_mode);
      if (atomicCounterDB[counter].ac_domin == TOP_DOMAIN) {
        dbg_print("%s register AC %d for TOP Domain\r\n", __FUNCTION__, counter);

        /*Register the IRQ*/
        atomic_counter_register_IRQ(counter);
        atomic_counters_dis_irq(counter);
      } else {
        dbg_print("%s register AC %d for None TOP Domain\r\n", __FUNCTION__, counter);
      }
    }
  } else {
    err_print("%s AC %d out of range\r\n", __FUNCTION__, counter);
  }
}

void atomic_counter_init() {
  atomic_counters_dis_irq(HIFC_INTERNAL_MODEM_2_HOST);
  atomic_counters_dis_irq(PM_INTERNAL_PMP_2_MCU);
}
