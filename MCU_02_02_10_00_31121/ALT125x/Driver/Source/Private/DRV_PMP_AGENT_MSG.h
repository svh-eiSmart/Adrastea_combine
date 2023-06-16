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
#ifndef DRV_PMP_AGENT_MSG_H_
#define DRV_PMP_AGENT_MSG_H_

#include <stdint.h>

#define SEC_MBOX_MEM_LENGTH (128)

typedef enum {
  MCU_STOP = 0,
  MCU_STANDBY,
  MCU_SHUTDOWN,
  MCU_DUMMY_FORCE_WIDTH = 0xffffffff  // force the enum to 4 bytes
} DRV_PMP_MSG_PowerMode;

typedef enum {
  MCU_RESET_CAUSE_UNKNOWN = 0,
  MCU_RESET_CAUSE_USER_TRIGGER,
  MCU_RESET_CAUSE_SW_FAILURE_ASSERT,
  MCU_RESET_CAUSE_SW_FAILURE_EXCEPTION,
  MCU_RESET_CAUSE_DUMMY_FORCE_WIDTH = 0xffffffff  // force the enum to 4 bytes
} DRV_PMP_MSG_ResetCause;

typedef enum {
  PMP2MCU_WAKEUP_REASON_NONE = 0,
  PMP2MCU_WAKEUP_REASON_TIMER,
  PMP2MCU_WAKEUP_REASON_IO_ISR,
  PMP2MCU_WAKEUP_REASON_MAP,
  PMP2MCU_WAKEUP_REASON_FORCE_WIDTH = 0xffffffff  // force the enum to 4 bytes
} DRV_PMP_MSG_WakeupReason;

/* -------------------------------- Message format - MCU to PMP -------------------------------- */

typedef enum {
  MCU2PMP_MSG_TYPE_ENUM_BEGIN = 0,
  MCU2PMP_MSG_GOING_TO_SLEEP,
  MCU2PMP_MSG_WATCHDOG_ENABLE,
  MCU2PMP_MSG_RESET_REQUEST,
  MCU2PMP_MSG_TYPE_ENUM_END,
  MCU2PMP_MSG_TYPE_DUMMY_FORCE_WIDTH = 0xffffffff  // force the enum to 4 bytes
} DRV_PMP_MCU2PMP_MsgType;

typedef struct {
  DRV_PMP_MSG_PowerMode sleepType;
  uint32_t sleepDuration;          // in milliseconds, 0 means infinite sleep
  uint32_t wakeupCompleteInstant;  // PMP_SHADOW_32K_Timer timestamp of last wakeup
  uint32_t memRetentionSecIdList;  // Section ID List for Memory Retention
} DRV_PMP_MCU2PMP_ToSleepMsg;

typedef struct {
  uint32_t enable;
} DRV_PMP_MCU2PMP_EnableWdtMsg;

typedef struct {
  DRV_PMP_MSG_ResetCause reset_cause;
} DRV_PMP_MCU2PMP_ResetMsg;

typedef union {
  DRV_PMP_MCU2PMP_ToSleepMsg goingToSleep;
  DRV_PMP_MCU2PMP_EnableWdtMsg wdt_en_msg;
  DRV_PMP_MCU2PMP_ResetMsg reset_request;
} DRV_PMP_MCU2PMP_MsgBody;

typedef struct {
  DRV_PMP_MCU2PMP_MsgType msgType;
} DRV_PMP_MCU2PMP_MsgHeader;

typedef struct {
  DRV_PMP_MCU2PMP_MsgHeader header;
  DRV_PMP_MCU2PMP_MsgBody body;
} DRV_PMP_MCU2PMP_Message;

#define MCU2PMP_MAILBOX_NUM_ELEMENTS (SEC_MBOX_MEM_LENGTH / sizeof(DRV_PMP_MCU2PMP_Message))

/* -------------------------------- Message format - PMP to MCU -------------------------------- */
typedef enum {
  PMP2MCU_MSG_TYPE_ENUM_BEGIN = 0,
  PMP2MCU_MSG_WAKEUP,
  PMP2MCU_MSG_TYPE_ENUM_END,
  PMP2MCU_MSG_TYPE_DUMMY_FORCE_WIDTH = 0xffffffff  // force the enum to 4 bytes
} DRV_PMP_PMP2MCU_MsgType;

typedef struct {
  DRV_PMP_MSG_WakeupReason cause;
  uint32_t duration_left;  // in milliseconds, non zero when waking-up before requested sleep
                           // duration elapsed
} DRV_PMP_PMP2MCU_WakeupMsg;

typedef union {
  DRV_PMP_PMP2MCU_WakeupMsg wakeup;
} DRV_PMP_PMP2MCU_MsgBody;

typedef struct {
  DRV_PMP_PMP2MCU_MsgType msgType;
} DRV_PMP_PMP2MCU_MsgHeader;

typedef struct {
  DRV_PMP_PMP2MCU_MsgHeader header;
  DRV_PMP_PMP2MCU_MsgBody body;
} DRV_PMP_PMP2MCU_Message;

#define PMP2MCU_MAILBOX_NUM_ELEMENTS (SEC_MBOX_MEM_LENGTH / sizeof(DRV_PMP_PMP2MCU_Message))

#endif /* DRV_PMP_AGENT_MSG_H_ */
