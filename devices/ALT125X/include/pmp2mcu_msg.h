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
#ifndef PMP_2_MCU_MSG_H_
#define PMP_2_MCU_MSG_H_

#include "mcu_pmp_api.h"

typedef enum _pmp2mcu_msg_type {
  PMP2MCU_MSG_TYPE_ENUM_BEGIN = 0,
  PMP2MCU_MSG_WAKEUP,
  PMP2MCU_MSG_TYPE_ENUM_END,
  PMP2MCU_MSG_TYPE_DUMMY_FORCE_WIDTH = 0xffffffff  // force the enum to 4 bytes
} pmp2mcu_msg_type_e;

typedef struct _pmp2mcu_wakeup_msg {
  pmp2mcu_wakeup_reason_e cause;
  uint32_t duration_left;  // in milliseconds, non zero when waking-up before requested sleep
                           // duration elapsed
} pmp2mcu_wakeup_msg_t;

typedef union _pmp2mcu_mbox_msg_body {
  pmp2mcu_wakeup_msg_t wakeup;
} pmp2mcu_mbox_msg_body_t;

typedef struct _pmp2mcu_mbox_msg_hdr {
  pmp2mcu_msg_type_e msgType;
} pmp2mcu_mbox_msg_hdr_t;

typedef struct _pmp2mcu_mbox_msg {
  pmp2mcu_mbox_msg_hdr_t header;
  pmp2mcu_mbox_msg_body_t body;
} pmp2mcu_mbox_msg_t;

#define PMP2MCU_MAILBOX_NUM_ELEMENTS (128 / sizeof(pmp2mcu_mbox_msg_t))

#endif  // PMP_2_MCU_MSG_H_
