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
#ifndef MCU_PMP_API_H_
#define MCU_PMP_API_H_

#define SEC_MBOX_MEM_LENGTH (128)

typedef enum _mcu_power_save {
  MCU_STOP = 0,
  MCU_STANDBY,
  MCU_SHUTDOWN,
  MCU_DUMMY_FORCE_WIDTH = 0xffffffff  // force the enum to 4 bytes
} mcu_power_save_e;

typedef enum _mcu_reset_cause {
  MCU_RESET_CAUSE_UNKNOWN = 0,
  MCU_RESET_CAUSE_USER_TRIGGER,
  MCU_RESET_CAUSE_SW_FAILURE_ASSERT,
  MCU_RESET_CAUSE_SW_FAILURE_EXCEPTION,
  MCU_RESET_CAUSE_DUMMY_FORCE_WIDTH = 0xffffffff  // force the enum to 4 bytes
} mcu_reset_cause_e;

typedef enum _pmp2mcu_wakeup_reason {
  PMP_2_MCU_WAKEUP_REASON_NONE = 0,
  PMP_2_MCU_WAKEUP_REASON_TIMER,
  PMP_2_MCU_WAKEUP_REASON_IO_ISR,
  PMP_2_MCU_WAKEUP_REASON_MAP,
  PMP_2_MCU_WAKEUP_REASON_FORCE_WIDTH = 0xffffffff  // force the enum to 4 bytes
} pmp2mcu_wakeup_reason_e;

#endif  // MCU_PMP_API_H_
