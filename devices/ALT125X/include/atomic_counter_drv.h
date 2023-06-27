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

#ifndef _ATOMIC_COUNTERS_DRV_H
#define _ATOMIC_COUNTERS_DRV_H

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

/*Read the STAT register value and Clear an IRQ */
uint32_t atomic_counters_clear_irq(IN atomic_counter_id counter);

#endif
