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

// Initialize atomic counters using:
// ATOMIC_COUNTER(Name,Increment register, Value Register,Interrupt mode,mask bit number (from CR))

/* A list of Atomic Counters*/
ATOMIC_COUNTER(PM_INTERNAL_MCU_2_PMP, TOP_DOMAIN, (TOPREG(BASE_ADDRESS_ATOMIC_COUNTER_MAILBOX4)),
               (ATOMIC_COUNTER_MAILBOX4_VAL & 0x0FFFFFFF),
               ATOMIC_COUNTER_DISABLE_INTERRUPT_MODE_VALUE, ATOMIC_COUNTER_MAILBOX4)
ATOMIC_COUNTER(PM_INTERNAL_PMP_2_MCU, TOP_DOMAIN, (TOPREG(BASE_ADDRESS_ATOMIC_COUNTER_MAILBOX5)),
               (ATOMIC_COUNTER_MAILBOX5_VAL & 0x0FFFFFFF),
               ATOMIC_COUNTER_NOT_ZERO_INTERRUPT_MODE_VALUE, ATOMIC_COUNTER_MAILBOX5)
ATOMIC_COUNTER(HIFC_INTERNAL_HOST_2_MODEM, TOP_DOMAIN,
               (TOPREG(BASE_ADDRESS_ATOMIC_COUNTER_MAILBOX8)),
               (ATOMIC_COUNTER_MAILBOX8_VAL & 0x0FFFFFFF),
               ATOMIC_COUNTER_DISABLE_INTERRUPT_MODE_VALUE, ATOMIC_COUNTER_MAILBOX8)
ATOMIC_COUNTER(HIFC_INTERNAL_MODEM_2_HOST, TOP_DOMAIN,
               (TOPREG(BASE_ADDRESS_ATOMIC_COUNTER_MAILBOX9)),
               (ATOMIC_COUNTER_MAILBOX9_VAL & 0x0FFFFFFF), ATOMIC_COUNTER_INC_INTERRUPT_MODE_VALUE,
               ATOMIC_COUNTER_MAILBOX9)
