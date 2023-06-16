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
#include "DRV_ATOMIC_COUNTER.h"

/****************************************************************************
 * Definitions
 ****************************************************************************/
#define ATOMIC_COUNTER_IRQ_LABLE "Atomic-Counter"

typedef uint32_t HWRegister;
typedef volatile HWRegister* HWRegisterPtr;

/*The offset of bit in the ctr register and the number of bits*/
#define ATOMIC_COUNTER_INT_IRQ_EN_BIT_OFFSET 0
#define ATOMIC_COUNTER_INT_IRQ_EN_BIT_NUM 1
#define ATOMIC_COUNTER_INT_MODE_BIT_OFFSET 1
#define ATOMIC_COUNTER_INT_MODE_BIT_NUM 2

#define ATOMIC_COUNTER_INC_REGISTER_OFFSET (0x0)
#define ATOMIC_COUNTER_DEC_REGISTER_OFFSET (0x4)
#define ATOMIC_COUNTER_MOD_REGISTER_OFFSET (0x8)
#define ATOMIC_COUNTER_INT_STAT_REGISTER_OFFSET (0xC)

#define ATOMIC_COUNTER_INTERRUPT_ENABLED_CTR (0x0001)
#define ATOMIC_COUNTER_INTERRUPT_ENABLE_BIT_OFFSET 0
#define ATOMIC_COUNTER_INTERRUPT_ENABLE_BIT_NUM 1

#define ATOMIC_COUNTERS_DB_SIZE 2000

#define ATOM_CNTR_DEBUG 0

#define ATOMIC_COUNTER_MAILBOX_SC_MASKS_BASE 0x0D009000UL
#define ATOMIC_COUNTER_MAILBOX_SC_MASKS_MCU (ATOMIC_COUNTER_MAILBOX_SC_MASKS_BASE + 0x0008)

/****************************************************************************
 * Private Types
 ****************************************************************************/
#define dbg_print(fmt, args...) \
  {                             \
    if (ATOM_CNTR_DEBUG) {      \
      printf(fmt, ##args);      \
    }                           \
  }
#define err_print(fmt, args...) \
  { printf(fmt, ##args); }

/*Global structure which holds AC related data*/
typedef struct drv_atom_cntr_data {
  uint32_t top_irq_register;
} DRV_ATOM_CNTR_Data;

/*Each AC is represented in the DB by this structure*/
typedef struct drv_atom_cntr_db {
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
  DRV_ATOM_CNTR_InterruptHandler irq_handler;
  /*User param to pass when there is ISR*/
  uint32_t user_param;
} DRV_ATOM_CNTR_DB;

/****************************************************************************
 * Private Data
 ****************************************************************************/
/*This will create a DB of atomic counters from the DRV_ATOMIC_COUNTER_INTERFACE.h file*/
#define ATOMIC_COUNTER(idx, domain, base, value, interrupt_mode, mask_bit) \
  {domain, base, value, interrupt_mode, mask_bit, NULL, 0},
DRV_ATOM_CNTR_DB atom_cntr_DB[ATOMIC_COUNTER_MAX_INDEX + 1] = {
#include ATOMIC_COUNT_LIST
    {0, 0, 0, 0, 0, NULL, 0}};
#undef ATOMIC_COUNTER

static DRV_ATOM_CNTR_Data atom_cntr_drv_data;

/****************************************************************************
 * Private Function Prototype
 ****************************************************************************/
/*Read the STAT register value and Clear an IRQ */
static uint32_t DRV_ATOM_CNTR_ClearIRQ(IN DRV_ATOM_CNTR_Id counter);

/****************************************************************************
 * Private Functions
 ****************************************************************************/
/*AC Read INT_STAT and clear IRQ*/
static uint32_t DRV_ATOM_CNTR_ClearIRQ(IN DRV_ATOM_CNTR_Id counter) {
  dbg_print("%s AC %d\r\n", __FUNCTION__, counter);

  volatile uint32_t temp = *(HWRegisterPtr)(atom_cntr_DB[counter].atomic_counter_base_addr +
                                            ATOMIC_COUNTER_INT_STAT_REGISTER_OFFSET);

  return temp;
}

static void MIPS_alt_insert(uint32_t* word, uint32_t value, uint32_t bitOffset,
                            uint32_t fieldSize) {
  *word &= ~((0xffffffff >> (32 - fieldSize)) << bitOffset);  // not bitwise on bitmask
  value &= (0xffffffff >> (32 - fieldSize));
  *word |= (value << bitOffset);
};
/****************************************************************************
 * Public Functions
 ****************************************************************************/
/*AC Set val*/
void DRV_ATOM_CNTR_SetCount(IN DRV_ATOM_CNTR_Id counter, IN uint8_t count) {
  dbg_print("%s AC %d,Count %d\r\n", __FUNCTION__, counter, count);
  *(HWRegisterPtr)atom_cntr_DB[counter].atomic_counter_value_addr = count;
}

/*AC get val*/
uint32_t DRV_ATOM_CNTR_GetCount(IN DRV_ATOM_CNTR_Id counter) {
  dbg_print("%s AC %d Val %ld\r\n", __FUNCTION__, counter,
            *(HWRegisterPtr)atom_cntr_DB[counter].atomic_counter_value_addr);
  return *(HWRegisterPtr)atom_cntr_DB[counter].atomic_counter_value_addr;
}

/*AC increment*/
void DRV_ATOM_CNTR_Increment(IN DRV_ATOM_CNTR_Id counter, IN uint8_t val) {
  dbg_print("%s AC %d,val %d\r\n", __FUNCTION__, counter, val);
  *(HWRegisterPtr)(atom_cntr_DB[counter].atomic_counter_base_addr +
                   ATOMIC_COUNTER_INC_REGISTER_OFFSET) = val;
}

/*AC decrement*/
void DRV_ATOM_CNTR_Decrement(IN DRV_ATOM_CNTR_Id counter, IN uint8_t val) {
  dbg_print("%s AC %d,val %d\r\n", __FUNCTION__, counter, val);
  *(HWRegisterPtr)(atom_cntr_DB[counter].atomic_counter_base_addr +
                   ATOMIC_COUNTER_DEC_REGISTER_OFFSET) = val;
}

/*AC enable IRQ*/
void DRV_ATOM_CNTR_EnableIRQ(IN DRV_ATOM_CNTR_Id counter) {
  dbg_print("%s AC %d\r\n", __FUNCTION__, counter);

  /*Read Modify write*/
  uint32_t reg = *(HWRegisterPtr)(atom_cntr_DB[counter].atomic_counter_base_addr +
                                  ATOMIC_COUNTER_MOD_REGISTER_OFFSET);
  MIPS_alt_insert(&reg, (uint32_t)1, ATOMIC_COUNTER_INT_IRQ_EN_BIT_OFFSET,
                  ATOMIC_COUNTER_INT_IRQ_EN_BIT_NUM);
  *(HWRegisterPtr)(atom_cntr_DB[counter].atomic_counter_base_addr +
                   ATOMIC_COUNTER_MOD_REGISTER_OFFSET) = reg;
}

/*AC disable IRQ*/
void DRV_ATOM_CNTR_DisableIRQ(IN DRV_ATOM_CNTR_Id counter) {
  dbg_print("%s AC %d\r\n", __FUNCTION__, counter);

  /*Read, Modify, write*/
  uint32_t reg = *(HWRegisterPtr)(atom_cntr_DB[counter].atomic_counter_base_addr +
                                  ATOMIC_COUNTER_MOD_REGISTER_OFFSET);
  MIPS_alt_insert(&reg, (uint32_t)0, ATOMIC_COUNTER_INT_IRQ_EN_BIT_OFFSET,
                  ATOMIC_COUNTER_INT_IRQ_EN_BIT_NUM);
  *(HWRegisterPtr)(atom_cntr_DB[counter].atomic_counter_base_addr +
                   ATOMIC_COUNTER_MOD_REGISTER_OFFSET) = reg;
}

/*AC set IRQ mode*/
void DRV_ATOM_CNTR_SetIrqMode(IN DRV_ATOM_CNTR_Id counter, IN uint8_t mode) {
  dbg_print("%s AC %d, mode %d\r\n", __FUNCTION__, counter, mode);

  /*Read, Modify, write*/
  uint32_t reg = *(HWRegisterPtr)(atom_cntr_DB[counter].atomic_counter_base_addr +
                                  ATOMIC_COUNTER_MOD_REGISTER_OFFSET);
  MIPS_alt_insert(&reg, (uint32_t)mode, ATOMIC_COUNTER_INT_MODE_BIT_OFFSET,
                  ATOMIC_COUNTER_INT_MODE_BIT_NUM);
  *(HWRegisterPtr)(atom_cntr_DB[counter].atomic_counter_base_addr +
                   ATOMIC_COUNTER_MOD_REGISTER_OFFSET) = reg;

  /*Now update the DB*/
  // TODO: DB should be in critical section
  atom_cntr_DB[counter].atomic_counter_intr_mode = mode;
}

// /*AC general IRQ handler*/
// static void atomic_counter_irq_interrupt_handler(void)
// {

// }

void ATMCTR_MAILBOX_IRQhandler(void) {
  /*Go through the AC counter check for TOP Domain*/
  for (int32_t i = 0; i < ATOMIC_COUNTER_MAX_INDEX; i++) {
    if (atom_cntr_DB[i].ac_domin == TOP_DOMAIN) {
      if (atom_cntr_DB[i].atomic_counter_intr_mode != ATOMIC_COUNTER_DISABLE_INTERRUPT_MODE_VALUE) {
        // DRV_ATOM_CNTR_GetCount(i);
        // TODO: we can also check mask and int_en flag
        if (DRV_ATOM_CNTR_GetCount(i) && (atom_cntr_DB[i].atomic_counter_intr_mode ==
                                          ATOMIC_COUNTER_NOT_ZERO_INTERRUPT_MODE_VALUE)) {
          if (atom_cntr_DB[i].irq_handler) atom_cntr_DB[i].irq_handler(atom_cntr_DB[i].user_param);
        } else if (DRV_ATOM_CNTR_ClearIRQ(i) !=
                   0) /*if INT_STAT returns 1 meaning there was an IRQ*/
        {
          if (atom_cntr_DB[i].irq_handler)
            atom_cntr_DB[i].irq_handler(atom_cntr_DB[i].user_param);
        }
      }
    }
  }
}

void DRV_ATOM_CNTR_RegisterIRQ(IN DRV_ATOM_CNTR_Id counter) {
  dbg_print("%s AC %d\r\n", __FUNCTION__, counter);

  // TODO: check if it is from TOP domain
  if (!atom_cntr_drv_data.top_irq_register) {
    dbg_print("%s AC %d, register INT_ATMCTR_MAILBOX_INT_MODEM IRQ\r\n ", __FUNCTION__, counter);
    // register_interrupt(INT_ATMCTR_MAILBOX_INT_MODEM, atomic_counter_irq_interrupt_handler,
    // ATOMIC_COUNTER_IRQ_LABLE);
    NVIC_SetPriority(ATMCTR_MAILBOX_IRQn, 7); /* set Interrupt priority */
    NVIC_EnableIRQ(ATMCTR_MAILBOX_IRQn);
    atom_cntr_drv_data.top_irq_register = true;
  }
}

/*Before using a AC, the user should call the init function.*/
void DRV_ATOM_CNTR_InitCounter(IN DRV_ATOM_CNTR_Id counter,
                               DRV_ATOM_CNTR_InterruptHandler irq_handler, uint32_t user_param) {
  dbg_print("%s AC %d\r\n", __FUNCTION__, counter);
  uint32_t reg;
  if ((counter == 1) ||
      (counter == 3)) { /* Notice: counter-1: mbox5; counter-3: mbox9. This is depend on the
                           ordering defined in DRV_ATOMIC_COUNTER_INTERFACE.h  */
    /*unmask the IRQ*/
    reg = *(HWRegisterPtr)(ATOMIC_COUNTER_MAILBOX_SC_MASKS_MCU);
    reg &= ~(1 << atom_cntr_DB[counter].mask_bit);
    *(HWRegisterPtr)((uint32_t)(ATOMIC_COUNTER_MAILBOX_SC_MASKS_MCU)) = reg;
  }
  if (counter < ATOMIC_COUNTER_MAX_INDEX) {
    // DRV_ATOM_CNTR_SetCount(counter, 0);
    atom_cntr_DB[counter].irq_handler = irq_handler;
    atom_cntr_DB[counter].user_param = user_param;

    if (atom_cntr_DB[counter].atomic_counter_intr_mode !=
        ATOMIC_COUNTER_DISABLE_INTERRUPT_MODE_VALUE) {
      DRV_ATOM_CNTR_SetIrqMode(counter, atom_cntr_DB[counter].atomic_counter_intr_mode);
      if (atom_cntr_DB[counter].ac_domin == TOP_DOMAIN) {
        dbg_print("%s register AC %d for TOP Domain\r\n", __FUNCTION__, counter);

        /*Register the IRQ*/
        DRV_ATOM_CNTR_RegisterIRQ(counter);
        DRV_ATOM_CNTR_DisableIRQ(counter);
      } else {
        dbg_print("%s register AC %d for None TOP Domain\r\n", __FUNCTION__, counter);
      }
    }
  } else {
    err_print("%s AC %d out of range\r\n", __FUNCTION__, counter);
  }
}

void DRV_ATOM_CNTR_Initialize() {
  DRV_ATOM_CNTR_DisableIRQ(HIFC_INTERNAL_MODEM_2_HOST);
  DRV_ATOM_CNTR_DisableIRQ(PM_INTERNAL_PMP_2_MCU);
}
