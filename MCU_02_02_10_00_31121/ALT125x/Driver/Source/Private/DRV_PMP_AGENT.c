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
/* Standard includes. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Platform Headers */
#include DEVICE_HEADER

#include "DRV_COMMON.h"
#include "DRV_PMP_AGENT.h"
#include "DRV_ATOMIC_COUNTER.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/
typedef struct {
  uint32_t curr_mcu2pmp_item;
  uint32_t curr_pmp2mcu_item;
  uint32_t last_wakeup_complete_instant;
} DRV_PMP_AGENT_Mbox;

typedef struct {
  uint32_t wakeup_cause;
  uint32_t duration_left;
  uint32_t count;
} DRV_PMP_AGENT_WakeUpInfo;

typedef struct {
  DRV_PMP_AGENT_WakeUpInfo wake_info;
} DRV_PMP_AGENT_Data;

/****************************************************************************
 * Private Data
 ****************************************************************************/
DRV_PMP_MCU2PMP_Message mcu2pmp_mbox[MCU2PMP_MAILBOX_NUM_ELEMENTS]
    __attribute__((section("SecMbox4")));
DRV_PMP_PMP2MCU_Message pmp2mcu_mbox[PMP2MCU_MAILBOX_NUM_ELEMENTS]
    __attribute__((section("SecMbox5")));

static DRV_PMP_AGENT_Mbox message_box = {0};
static int32_t pmp_agent_initialized = 0;
static DRV_PMP_AGENT_Data boot_data = {0};
static pmp_mcu_map_shared_info pmp_shared_info[1] __attribute__((section("SecMbox11")));
/****************************************************************************
 * Private Functions
 ****************************************************************************/
int32_t DRV_PMP_AGENT_Send(DRV_PMP_MCU2PMP_Message *out_msg) {
  DRV_PMP_MCU2PMP_Message *msg;
  int32_t ret_val = 0;

  msg = &mcu2pmp_mbox[message_box.curr_mcu2pmp_item];
  message_box.curr_mcu2pmp_item += 1;
  if (message_box.curr_mcu2pmp_item == MCU2PMP_MAILBOX_NUM_ELEMENTS) {
    message_box.curr_mcu2pmp_item = 0;
  }

  memset(msg, 0x0, sizeof(DRV_PMP_MCU2PMP_Message));
  memcpy(msg, out_msg, sizeof(DRV_PMP_MCU2PMP_Message));
  if (msg->header.msgType == MCU2PMP_MSG_GOING_TO_SLEEP)
    msg->body.goingToSleep.wakeupCompleteInstant = message_box.last_wakeup_complete_instant;

  DRV_ATOM_CNTR_Increment(PM_INTERNAL_MCU_2_PMP, 1);

  return ret_val;
}

/*-----------------------------------------------------------------------------
 * void DRV_PMP_AGENT_GetWakeInfo(void)
 * PURPOSE: This function would get the data .
 * PARAMs:
 *      INPUT:  None.
 *      OUTPUT: *cause, *time_left, *count.
 * RETURN:  None
 *-----------------------------------------------------------------------------
 */
void DRV_PMP_AGENT_GetWakeInfo(uint32_t *cause, uint32_t *time_left, uint32_t *count) {
  *cause = boot_data.wake_info.wakeup_cause;
  *time_left = boot_data.wake_info.duration_left;
  *count = boot_data.wake_info.count;

  return;
}

/*-----------------------------------------------------------------------------
 * void DRV_PMP_AGENT_HandleMessage(void)
 * PURPOSE: This function would handle the received message from PMP.
 * PARAMs:
 *      INPUT:  None.
 *      OUTPUT: None.
 * RETURN:  None
 *-----------------------------------------------------------------------------
 */
void DRV_PMP_AGENT_HandleMessage(void) {
  DRV_PMP_PMP2MCU_Message *mboxMsg;

  mboxMsg = &pmp2mcu_mbox[message_box.curr_pmp2mcu_item];
  message_box.curr_pmp2mcu_item += 1;
  if (message_box.curr_pmp2mcu_item == PMP2MCU_MAILBOX_NUM_ELEMENTS) {
    message_box.curr_pmp2mcu_item = 0;
  }

  switch (mboxMsg->header.msgType) {
    case PMP2MCU_MSG_WAKEUP:
      boot_data.wake_info.wakeup_cause = mboxMsg->body.wakeup.cause;
      boot_data.wake_info.duration_left = mboxMsg->body.wakeup.duration_left;
      boot_data.wake_info.count++;

      break;
    default:  // SYS_ASSERT(FALSE);
      break;
  }
}

/*-----------------------------------------------------------------------------
 * void DRV_PMP_AGENT_HandleIRQ(uint32_t user_param)
 * PURPOSE: This function would handle the interrupts from PMP.
 * PARAMs:
 *      INPUT:  user_param.
 *      OUTPUT: None
 * RETURN:  None
 *-----------------------------------------------------------------------------
 */
void DRV_PMP_AGENT_HandleIRQ(uint32_t user_param) {
  /* disable IRQ */
  DRV_ATOM_CNTR_DisableIRQ(PM_INTERNAL_PMP_2_MCU);

  DRV_ATOM_CNTR_Decrement(PM_INTERNAL_PMP_2_MCU, 1);

  DRV_PMP_AGENT_HandleMessage();

  message_box.last_wakeup_complete_instant++;

  /* enable IRQ */
  DRV_ATOM_CNTR_EnableIRQ(PM_INTERNAL_PMP_2_MCU);
}

/*-----------------------------------------------------------------------------
 * int32_t DRV_PMP_AGENT_SendMessage(DRV_PMP_MCU2PMP_Message *out_msg)
 * PURPOSE: This function would send the message to PMP.
 * PARAMs:
 *      INPUT:  pointer of output data.
 *      OUTPUT: None.
 * RETURN: 0:success ; others: error code.
 *-----------------------------------------------------------------------------
 */
int32_t DRV_PMP_AGENT_SendMessage(DRV_PMP_MCU2PMP_Message *out_msg) {
  int32_t ret_val = 0;
  uint32_t irqmask;

  irqmask = ((__get_PRIMASK() != 0U) || (__get_BASEPRI() != 0U));
  __disable_irq();

  ret_val = DRV_PMP_AGENT_Send(out_msg);

  if (!irqmask) {
    __enable_irq();
  }

  return ret_val;
}

/*-----------------------------------------------------------------------------
 * int32_t DRV_PMP_AGENT_SendMessageByIdleTask(DRV_PMP_MCU2PMP_Message *out_msg)
 * PURPOSE: This function would send the message to PMP.
 * PARAMs:
 *      INPUT:  pointer of output data.
 *      OUTPUT: None.
 * RETURN:  0:success ; others: error code.
 * Note: only used for sleep requests.
 *-----------------------------------------------------------------------------
 */
int32_t DRV_PMP_AGENT_SendMessageByIdleTask(DRV_PMP_MCU2PMP_Message *out_msg) {
  int32_t ret_val = DRV_PMP_AGENT_Send(out_msg);
  return ret_val;
}

/*-----------------------------------------------------------------------------
 * int32_t DRV_PMP_AGENT_ReInitialize(void)
 * PURPOSE: This function would re-initialize pmpMbox and related funtions
 *          after wake from stateful standby.
 * PARAMs:
 *      INPUT:  None.
 *      OUTPUT: None.
 * RETURN:  0:success ; others: error code.
 *-----------------------------------------------------------------------------
 */
int32_t DRV_PMP_AGENT_ReInitialize(void) {
  message_box = (DRV_PMP_AGENT_Mbox){0};

  DRV_ATOM_CNTR_InitCounter(PM_INTERNAL_MCU_2_PMP, 0, 0);
  DRV_ATOM_CNTR_InitCounter(PM_INTERNAL_PMP_2_MCU, DRV_PMP_AGENT_HandleIRQ, 0);
  DRV_ATOM_CNTR_EnableIRQ(PM_INTERNAL_PMP_2_MCU);

  return 0;
}

/*-----------------------------------------------------------------------------
 * int32_t DRV_PMP_AGENT_Initialize(void)
 * PURPOSE: This function would initialize pmpMbox and related funtions.
 * PARAMs:
 *      INPUT:  None.
 *      OUTPUT: None.
 * RETURN:  0:success ; others: error code.
 *-----------------------------------------------------------------------------
 */
int32_t DRV_PMP_AGENT_Initialize(void) {
  if (pmp_agent_initialized) return 0;

  /* init pmp agent Mbox */
  DRV_ATOM_CNTR_InitCounter(PM_INTERNAL_MCU_2_PMP, 0, 0);
  DRV_ATOM_CNTR_InitCounter(PM_INTERNAL_PMP_2_MCU, DRV_PMP_AGENT_HandleIRQ, 0);
  DRV_ATOM_CNTR_EnableIRQ(PM_INTERNAL_PMP_2_MCU);
  pmp_agent_initialized = 1;

  return 0;
}

/*-----------------------------------------------------------------------------
 * int32_t DRV_PMP_AGENT_GetTemperature(void)
 * PURPOSE: This function would get the temperature.
 * PARAMs:
 *      INPUT:  None.
 *      OUTPUT: pointer of PMP shared information.
 * RETURN:  0:success ; others: error code.
 *-----------------------------------------------------------------------------
 */
int32_t DRV_PMP_AGENT_GetSharedInfo(pmp_mcu_map_shared_info *shared_info) {
  if (shared_info) {
    memcpy(shared_info, pmp_shared_info, sizeof(pmp_mcu_map_shared_info));
    return 0;
  } else
    return (-1);
}
