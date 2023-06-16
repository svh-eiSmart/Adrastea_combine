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
#include "reset.h"

#include <string.h>

#include "alt_osal.h"

#include "DRV_PMP_AGENT.h"

void machine_reset(eMachineResetCause reset_cause) {
  DRV_PMP_MCU2PMP_Message msg;

  memset(&msg, 0x0, sizeof(DRV_PMP_MCU2PMP_Message));
  msg.header.msgType = MCU2PMP_MSG_RESET_REQUEST;
  switch (reset_cause) {
    case MACHINE_RESET_CAUSE_USER:
      msg.body.reset_request.reset_cause = MCU_RESET_CAUSE_USER_TRIGGER;
      break;
    case MACHINE_RESET_CAUSE_EXCEPTION:
      msg.body.reset_request.reset_cause = MCU_RESET_CAUSE_SW_FAILURE_EXCEPTION;
      break;
    case MACHINE_RESET_CAUSE_ASSERT:
      msg.body.reset_request.reset_cause = MCU_RESET_CAUSE_SW_FAILURE_ASSERT;
      break;
    default:
      msg.body.reset_request.reset_cause = MCU_RESET_CAUSE_UNKNOWN;
      break;
  }
  /*Make sure pmp message will be sent atomically*/
  if (!alt_osal_is_inside_interrupt()) alt_osal_enter_critical();
  DRV_PMP_AGENT_SendMessage(&msg);
  while (1)
    ;
}

