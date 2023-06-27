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
#ifndef MCU_2_PMP_MSG_H_
#define MCU_2_PMP_MSG_H_

#include "mcu_pmp_api.h"

typedef enum _mcu2pmp_msg_type {
  MCU2PMP_MSG_TYPE_ENUM_BEGIN = 0,
  MCU2PMP_MSG_GOING_TO_SLEEP,
  MCU2PMP_MSG_WATCHDOG_ENABLE,
  MCU2PMP_MSG_RESET_REQUEST,
  MCU2PMP_MSG_TYPE_ENUM_END,
  MCU2PMP_MSG_TYPE_DUMMY_FORCE_WIDTH = 0xffffffff  // force the enum to 4 bytes
} mcu2pmp_msg_type_e;

typedef struct _mcu2pmp_go2sleep_msg {
  mcu_power_save_e sleepType;
  uint32_t sleepDuration;          // in milliseconds, 0 means infinite sleep
  uint32_t wakeupCompleteInstant;  // PMP_SHADOW_32K_Timer timestamp of last wakeup
  uint32_t memRetentionSecIdList;  // Section ID List for Memory Retention
} mcu2pmp_go2sleep_msg_t;

typedef struct _mcu2pmp_wdt_msg {
  uint32_t enable;
} mcu2pmp_wdt_en_msg_t;

typedef struct _mcu2pmp_reset_msg {
  mcu_reset_cause_e reset_cause;
} mcu2pmp_reset_msg_t;

typedef union _mcu2pmp_mbox_msgbody {
  mcu2pmp_go2sleep_msg_t goingToSleep;
  mcu2pmp_wdt_en_msg_t wdt_en_msg;
  mcu2pmp_reset_msg_t reset_request;
} mcu2pmp_mbox_msgbody_t;

typedef struct _mcu2pmp_mbox_msg_hdr {
  mcu2pmp_msg_type_e msgType;
} mcu2pmp_mbox_msg_hdr_t;

typedef struct _mcu2pmp_mbox_msg {
  mcu2pmp_mbox_msg_hdr_t header;
  mcu2pmp_mbox_msgbody_t body;
} mcu2pmp_mbox_msg_t;

#define MCU2PMP_MAILBOX_NUM_ELEMENTS (SEC_MBOX_MEM_LENGTH / sizeof(mcu2pmp_mbox_msg_t))

#endif  // MCU_2_PMP_MSG_H_
