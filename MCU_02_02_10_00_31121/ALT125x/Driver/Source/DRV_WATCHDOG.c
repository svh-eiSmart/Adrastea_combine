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
#include DEVICE_HEADER
#include "DRV_WATCHDOG.h"
#include "DRV_PMP_AGENT.h"
#if (CONFIG_WATCHDOG_SUPPORT_PM == 1)
#include "DRV_PM.h"
#include "DRV_SLEEP.h"
#endif

#include <string.h>

#ifndef UINT_MAX
#define UINT_MAX (0xFFFFFFFFUL)
#endif

extern uint32_t SystemCoreClock;

#define WATCHDOG_CLK_RATE (SystemCoreClock)
#define MIN_HEARTBEAT (1)
#define MAX_HEARTBEAT (UINT_MAX / (WATCHDOG_CLK_RATE / 2))

struct DRV_WATCHDOG_Peripheral {
  WD_Type *reg_base;
  DRV_WATCHDOG_TimeoutCallback timeout_cb;
  void *timeout_cb_param;
  uint32_t cnt_load;
  uint32_t init_done;
#if (CONFIG_WATCHDOG_SUPPORT_PM == 1)
  uint32_t wd_ctx_reg_config;
  uint32_t wd_ctx_reg_cnt_load;
  uint32_t wd_ctx_reg_pmp_mask;
  int32_t sleep_notify_inx;
#endif
};
static struct DRV_WATCHDOG_Peripheral wdt_peripheral = {.reg_base = WD,
                                                        .timeout_cb = NULL,
                                                        .timeout_cb_param = 0,
                                                        .cnt_load = UINT_MAX,
#if (CONFIG_WATCHDOG_SUPPORT_PM == 1)
                                                        .wd_ctx_reg_config = 0,
                                                        .wd_ctx_reg_cnt_load = 0,
                                                        .wd_ctx_reg_pmp_mask = 0,
                                                        .sleep_notify_inx = 0,
#endif
                                                        .init_done = 0};

static void DRV_WATCHDOG_SendEnableToPmp(uint32_t enable) {
  DRV_PMP_MCU2PMP_Message msg;

  memset(&msg, 0x0, sizeof(DRV_PMP_MCU2PMP_Message));
  msg.header.msgType = MCU2PMP_MSG_WATCHDOG_ENABLE;
  msg.body.wdt_en_msg.enable = enable;

  DRV_PMP_AGENT_SendMessage(&msg);
}

void MCU_WD0_IRQHandler(void) {
  /* PMP is responsible for resetting on MCU WD second expiration
   * Notifiy user application by callback*/
  NVIC_DisableIRQ(WD_IRQn);
  if (wdt_peripheral.timeout_cb != NULL) wdt_peripheral.timeout_cb(wdt_peripheral.timeout_cb_param);

  return;
}

static inline void DRV_WATCHDOG_Unlock(void) {
  wdt_peripheral.reg_base->PROTECT = 0xa5c6;
  wdt_peripheral.reg_base->PROTECT = 0xda7e;
}

static void DRV_WATCHDOG_Ping(void) {
  DRV_WATCHDOG_Unlock();
  wdt_peripheral.reg_base->CNT_LOAD = wdt_peripheral.cnt_load;
  if (wdt_peripheral.reg_base->CLEAR_INT_b.INT_STAT) {
    DRV_WATCHDOG_Unlock();
    wdt_peripheral.reg_base->CONFIG_b.INT_MASK = 1;
  }
}

static void DRV_WATCHDOG_Enable() {
  // Enable HW module
  DRV_WATCHDOG_Unlock();
  wdt_peripheral.reg_base->CONFIG_b.RST_EN = 3;
  DRV_WATCHDOG_Unlock();
  wdt_peripheral.reg_base->CONFIG_b.INT_MASK = 1;
  DRV_WATCHDOG_Unlock();
  wdt_peripheral.reg_base->CONFIG_b.ENABLE = 1;

  REGFILE->MCU_STATUS_MASK_PMP |= REGFILE_MCU_STATUS_MCU_WD_Msk;
}

static uint32_t DRV_WATCHDOG_IsEnabled() { return (wdt_peripheral.reg_base->CONFIG_b.ENABLE != 0); }

static void DRV_WATCHDOG_Disable() {
  DRV_WATCHDOG_Unlock();
  wdt_peripheral.reg_base->CONFIG_b.INT_MASK = 0;
  DRV_WATCHDOG_Unlock();
  wdt_peripheral.reg_base->CONFIG_b.ENABLE = 0;

  REGFILE->MCU_STATUS_MASK_PMP &= ~REGFILE_MCU_STATUS_MCU_WD_Msk;
}

static void DRV_WATCHDOG_SetCntLoad() {
  DRV_WATCHDOG_Unlock();
  wdt_peripheral.reg_base->CNT_LOAD = wdt_peripheral.cnt_load;
}

#if (CONFIG_WATCHDOG_SUPPORT_PM == 1)
static int32_t DRV_WATCHDOG_SleepHandler(DRV_SLEEP_NotifyType state, DRV_PM_PwrMode pwr_mode,
                                         void *arg) {
  if (state == DRV_SLEEP_NTFY_SUSPENDING) {
    wdt_peripheral.wd_ctx_reg_config = wdt_peripheral.reg_base->CONFIG;
    wdt_peripheral.wd_ctx_reg_cnt_load = wdt_peripheral.reg_base->CNT_LOAD;
    wdt_peripheral.wd_ctx_reg_pmp_mask = REGFILE->MCU_STATUS_MASK_PMP;
    if (wdt_peripheral.wd_ctx_reg_config & WD_CONFIG_ENABLE_Msk) DRV_WATCHDOG_Disable();
  } else if (state == DRV_SLEEP_NTFY_RESUMING) {
    DRV_WATCHDOG_Unlock();
    wdt_peripheral.reg_base->CNT_LOAD = wdt_peripheral.wd_ctx_reg_cnt_load;
    DRV_WATCHDOG_Unlock();
    wdt_peripheral.reg_base->CONFIG = wdt_peripheral.wd_ctx_reg_config;
    REGFILE->MCU_STATUS_MASK_PMP = wdt_peripheral.wd_ctx_reg_pmp_mask;
  }
  return 0;
}
#endif

DRV_WATCHDOG_Status DRV_WATCHDOG_Initialize(DRV_WATCHDOG_Config *config) {
  if (wdt_peripheral.init_done) return DRV_WATCHDOG_OK;

#if (CONFIG_WATCHDOG_SUPPORT_PM == 1)
  if (DRV_SLEEP_RegNotification(DRV_WATCHDOG_SleepHandler, &wdt_peripheral.sleep_notify_inx,
                                NULL) != 0)
    return DRV_WATCHDOG_ERROR_PM;
#endif
  if (config->timeout < MIN_HEARTBEAT || config->timeout > MAX_HEARTBEAT)
    return DRV_WATCHDOG_ERROR_PARAM;
  wdt_peripheral.timeout_cb = config->timeout_cb;
  wdt_peripheral.timeout_cb_param = config->timeout_cb_param;
  wdt_peripheral.cnt_load = config->timeout * WATCHDOG_CLK_RATE;
  wdt_peripheral.init_done = 1;

  NVIC_SetPriority(WD_IRQn, 7);
  NVIC_EnableIRQ(WD_IRQn);

  return DRV_WATCHDOG_OK;
}

DRV_WATCHDOG_Status DRV_WATCHDOG_Uninitialize(void) {
  if (!wdt_peripheral.init_done) return DRV_WATCHDOG_OK;

#if (CONFIG_WATCHDOG_SUPPORT_PM == 1)
  if (DRV_SLEEP_UnRegNotification(wdt_peripheral.sleep_notify_inx) != 0)
    return DRV_WATCHDOG_ERROR_PM;
#endif

  if (DRV_WATCHDOG_IsEnabled()) DRV_WATCHDOG_Disable();

  NVIC_DisableIRQ(WD_IRQn);

  wdt_peripheral.timeout_cb = NULL;
  wdt_peripheral.timeout_cb_param = 0;
  wdt_peripheral.init_done = 0;

  return DRV_WATCHDOG_OK;
}

DRV_WATCHDOG_Status DRV_WATCHDOG_Start(void) {
  if (!wdt_peripheral.init_done || DRV_WATCHDOG_IsEnabled()) return DRV_WATCHDOG_ERROR_STATE;
  DRV_WATCHDOG_Enable();
  DRV_WATCHDOG_SetCntLoad();
  DRV_WATCHDOG_SendEnableToPmp(1);

  return DRV_WATCHDOG_OK;
}

DRV_WATCHDOG_Status DRV_WATCHDOG_Stop(void) {
  if (!wdt_peripheral.init_done || !DRV_WATCHDOG_IsEnabled()) return DRV_WATCHDOG_ERROR_STATE;

  DRV_WATCHDOG_SendEnableToPmp(0);
  DRV_WATCHDOG_Disable();
  return DRV_WATCHDOG_OK;
}

DRV_WATCHDOG_Status DRV_WATCHDOG_Kick(void) {
  if (!wdt_peripheral.init_done || !DRV_WATCHDOG_IsEnabled()) return DRV_WATCHDOG_ERROR_STATE;

  DRV_WATCHDOG_Ping();
  return DRV_WATCHDOG_OK;
}

DRV_WATCHDOG_Status DRV_WATCHDOG_SetTimeout(DRV_WATCHDOG_Time timeout) {
  if (!wdt_peripheral.init_done || !DRV_WATCHDOG_IsEnabled()) return DRV_WATCHDOG_ERROR_STATE;

  if (timeout > MAX_HEARTBEAT || timeout < MIN_HEARTBEAT) return DRV_WATCHDOG_ERROR_PARAM;

  wdt_peripheral.cnt_load = WATCHDOG_CLK_RATE * timeout;
  DRV_WATCHDOG_SetCntLoad();
  return DRV_WATCHDOG_OK;
}

DRV_WATCHDOG_Status DRV_WATCHDOG_GetTimeout(DRV_WATCHDOG_Time *timeout) {
  if (!wdt_peripheral.init_done) return DRV_WATCHDOG_ERROR_STATE;

  *timeout = wdt_peripheral.cnt_load / WATCHDOG_CLK_RATE;
  return DRV_WATCHDOG_OK;
}

DRV_WATCHDOG_Status DRV_WATCHDOG_GetRemainingTime(DRV_WATCHDOG_Time *timeleft) {
  if (!wdt_peripheral.init_done || !DRV_WATCHDOG_IsEnabled()) return DRV_WATCHDOG_ERROR_STATE;

  *timeleft = wdt_peripheral.reg_base->CNT_VAL / WATCHDOG_CLK_RATE;
  return DRV_WATCHDOG_OK;
}
