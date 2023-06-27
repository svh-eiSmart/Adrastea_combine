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

/* Standard includes. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "newlibPort.h"
#include "mcu2pmp_msg.h"
#include "pmp2mcu_msg.h"
#if (configUSE_ALT1250_SLEEP == 1)
#include "pwr_mngr.h"
#endif
#include "atomic_counter.h"
#include "mcu_exported.h"

mcu2pmp_mbox_msg_t mcu2PmpMbox[MCU2PMP_MAILBOX_NUM_ELEMENTS] __attribute__((section("SecMbox4")));
pmp2mcu_mbox_msg_t pmp2McuMbox[PMP2MCU_MAILBOX_NUM_ELEMENTS] __attribute__((section("SecMbox5")));

typedef struct _pmpAgentDB {
  uint32_t currMcu2PmpMboxItem;
  uint32_t currPmp2McuMboxItem;
  uint32_t lastWakeupCompleteInstant;
} pmpAgentDB_t;

static pmpAgentDB_t gPmpAgentDB = {0};
static int gPmp_agent_initialized = 0;

int mcu_send_msg_to_pmp(mcu2pmp_mbox_msg_t *out_msg) {
  mcu2pmp_mbox_msg_t *msg;
  int retVal = 0;

  msg = &mcu2PmpMbox[gPmpAgentDB.currMcu2PmpMboxItem];
  gPmpAgentDB.currMcu2PmpMboxItem += 1;
  if (gPmpAgentDB.currMcu2PmpMboxItem == MCU2PMP_MAILBOX_NUM_ELEMENTS) {
    gPmpAgentDB.currMcu2PmpMboxItem = 0;
  }

  memset(msg, 0x0, sizeof(mcu2pmp_mbox_msg_t));
  memcpy(msg, out_msg, sizeof(mcu2pmp_mbox_msg_t));
  if (msg->header.msgType == MCU2PMP_MSG_GOING_TO_SLEEP)
    msg->body.goingToSleep.wakeupCompleteInstant = gPmpAgentDB.lastWakeupCompleteInstant;

  atomic_counters_increment(PM_INTERNAL_MCU_2_PMP, 1);

  return retVal;
}
/*-----------------------------------------------------------------------------
 * int send_msg_to_pmp(mcu2pmp_mbox_msg_t *out_msg)
 * PURPOSE: This function would send the message to PMP.
 * PARAMs:
 *      INPUT:  pointer of output data.
 *      OUTPUT: None.
 * RETURN: 0:success ; others: error code.
 *-----------------------------------------------------------------------------
 */
int send_msg_to_pmp(mcu2pmp_mbox_msg_t *out_msg) {
  int retVal = 0;
  unsigned uxSavedInterruptStatus = 0;

  if (xPortIsInsideInterrupt() == pdTRUE) {
    uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
  } else {
    taskENTER_CRITICAL();
  }

  retVal = mcu_send_msg_to_pmp(out_msg);

  if (xPortIsInsideInterrupt() == pdTRUE) {
    taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
  } else {
    taskEXIT_CRITICAL();
  }

  return retVal;
}

/*-----------------------------------------------------------------------------
 * int idle_task_send_msg_to_pmp(mcu2pmp_mbox_msg_t *out_msg)
 * PURPOSE: This function would send the message to PMP.
 * PARAMs:
 *      INPUT:  pointer of output data.
 *      OUTPUT: None.
 * RETURN:  0:success ; others: error code.
 * Note: only used for sleep requests.
 *-----------------------------------------------------------------------------
 */
int idle_task_send_msg_to_pmp(mcu2pmp_mbox_msg_t *out_msg) {
  int retVal = mcu_send_msg_to_pmp(out_msg);
  return retVal;
}

/*-----------------------------------------------------------------------------
 * void pmpAgent_handle_mbox_msg(void)
 * PURPOSE: This function would handle the received message from PMP.
 * PARAMs:
 *      INPUT:  None.
 *      OUTPUT: None.
 * RETURN:  None
 *-----------------------------------------------------------------------------
 */
void pmpAgent_handle_mbox_msg(void) {
  pmp2mcu_mbox_msg_t *mboxMsg;
#if (configUSE_ALT1250_SLEEP == 1)
  pwr_wakeup_cause_e cause;
#endif

  mboxMsg = &pmp2McuMbox[gPmpAgentDB.currPmp2McuMboxItem];
  gPmpAgentDB.currPmp2McuMboxItem += 1;
  if (gPmpAgentDB.currPmp2McuMboxItem == PMP2MCU_MAILBOX_NUM_ELEMENTS) {
    gPmpAgentDB.currPmp2McuMboxItem = 0;
  }

  switch (mboxMsg->header.msgType) {
    case PMP2MCU_MSG_WAKEUP:
#if (configUSE_ALT1250_SLEEP == 1)
      cause = (pwr_wakeup_cause_e)mboxMsg->body.wakeup.cause;
      if (cause < PWR_WAKEUP_CAUSE_TIMER || cause > PWR_WAKEUP_CAUSE_MODEM) {
        cause = PWR_WAKEUP_CAUSE_UNKNOWN;
      }

      pwr_stat_update_last_wakeup_info(cause, mboxMsg->body.wakeup.duration_left);
#endif
      break;
    default:  // SYS_ASSERT(FALSE);
      break;
  }
}

/*-----------------------------------------------------------------------------
 * void pm_pmp_to_mcu_ac_irq(unsigned int user_param)
 * PURPOSE: This function would handle the interrupts from PMP.
 * PARAMs:
 *      INPUT:  user_param.
 *      OUTPUT: None
 * RETURN:  None
 *-----------------------------------------------------------------------------
 */
void pmp_to_mcu_ac_irq(unsigned int user_param) {
  /* disable IRQ */
  atomic_counters_dis_irq(PM_INTERNAL_PMP_2_MCU);

  atomic_counters_decrement(PM_INTERNAL_PMP_2_MCU, 1);

  pmpAgent_handle_mbox_msg();

  gPmpAgentDB.lastWakeupCompleteInstant++;
  REGISTER(TOPREG(GSP_REGFILE_MCU_ROM_ADDR_0X8)) = 100 + gPmpAgentDB.lastWakeupCompleteInstant;

  /* enable IRQ */
  atomic_counters_ena_irq(PM_INTERNAL_PMP_2_MCU);
}

/*-----------------------------------------------------------------------------
 * int pmpAgent_init(void)
 * PURPOSE: This function would initialize pmpMbox and related funtions.
 * PARAMs:
 *      INPUT:  None.
 *      OUTPUT: None.
 * RETURN:  0:success ; others: error code.
 *-----------------------------------------------------------------------------
 */
int pmpAgent_init(void) {
  if (gPmp_agent_initialized) return 0;

  /* init pmp agent Mbox */
  atomic_counters_init_counter(PM_INTERNAL_MCU_2_PMP, 0, 0);
  atomic_counters_init_counter(PM_INTERNAL_PMP_2_MCU, pmp_to_mcu_ac_irq, 0);
  atomic_counters_ena_irq(PM_INTERNAL_PMP_2_MCU);
  gPmp_agent_initialized = 1;

  return 0;
}
