/*  ---------------------------------------------------------------------------

    (c) copyright 2019 Altair Semiconductor, Ltd. All rights reserved.

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

#include <stdio.h>
#include <string.h>

#include "125X_mcu.h"
#include "wdt.h"
#include "mcu2pmp_msg.h"
#include "pmp_agent.h"
#ifndef UINT_MAX
#define UINT_MAX (0xFFFFFFFFUL)
#endif

extern uint32_t SystemCoreClock;

#define ALT1250_WATCHDOG_CLK_RATE (SystemCoreClock)
#define MIN_HEARTBEAT (1)
#define MAX_HEARTBEAT (UINT_MAX / (ALT1250_WATCHDOG_CLK_RATE / 2))
#define CONFIG_ALT1250_WDT_MIN 10
#define CONFIG_ALT1250_WDT_MAX 45

static unsigned long g_timeout_value = 0xffffffffU;
static wdt_timeout_cb g_timeout_cb = NULL;
static void *g_timeout_cb_param = NULL;

static unsigned long wd_ctx_reg_config = 0;
static unsigned long wd_ctx_reg_cnt_load = 0;
static unsigned long wd_ctx_reg_pmp_mask = 0;

static void send_wdt_enable_to_pmp(int enable) {
  mcu2pmp_mbox_msg_t msg;

  memset(&msg, 0x0, sizeof(mcu2pmp_mbox_msg_t));
  msg.header.msgType = MCU2PMP_MSG_WATCHDOG_ENABLE;
  msg.body.wdt_en_msg.enable = enable;

  send_msg_to_pmp(&msg);
}

void MCU_WD0_IRQHandler(void) {
  /* PMP is responsible for resetting on MCU WD second expiration
   * Notifiy user application by callback*/
  NVIC_DisableIRQ(MCU_WD0_IRQn);
  if (g_timeout_cb != NULL) g_timeout_cb(g_timeout_cb_param);

  return;
}

static inline void alt1250_wdt_unlock(void) {
  REGISTER(MCU_WD_WD0_PROTECT) = 0xa5c6;
  REGISTER(MCU_WD_WD0_PROTECT) = 0xda7e;
}

static void alt1250_wdt_ping(void) {
  unsigned long v;
  alt1250_wdt_unlock();
  REGISTER(MCU_WD_WD0_CNT_LOAD) = g_timeout_value;
  v = REGISTER(MCU_WD_WD0_CLEAR_INT);
  if (v & MCU_WD_WD0_CLEAR_INT_INT_STAT_MSK) {
    v = REGISTER(MCU_WD_WD0_CONFIG);
    v |= MCU_WD_WD0_CONFIG_INT_MASK_MSK;
    alt1250_wdt_unlock();
    REGISTER(MCU_WD_WD0_CONFIG) = v;
  }
}

static void alt1250_wdt_enable() {
  unsigned long v;

  // Enable HW module
  v = REGISTER(MCU_WD_WD0_CONFIG);
  v &= ~MCU_WD_WD0_CONFIG_RST_EN_MSK;
  v |= ((3 << MCU_WD_WD0_CONFIG_RST_EN_POS) | MCU_WD_WD0_CONFIG_INT_MASK_MSK |
        MCU_WD_WD0_CONFIG_ENABLE_MSK);
  alt1250_wdt_unlock();
  REGISTER(MCU_WD_WD0_CONFIG) = v;

  v = REGISTER(MCU_REGFILE_MCU_STATUS_MASK_PMP);
  v |= MCU_REGFILE_MCU_STATUS_MCU_WD_MSK;
  REGISTER(MCU_REGFILE_MCU_STATUS_MASK_PMP) = v;
}

static int alt1250_wdt_is_enabled() {
  unsigned long v;
  v = REGISTER(MCU_WD_WD0_CONFIG);
  return ((v & MCU_WD_WD0_CONFIG_ENABLE_MSK) != 0);
}

static void alt1250_wdt_disable() {
  unsigned long v;

  v = REGISTER(MCU_WD_WD0_CONFIG);
  v &= ~MCU_WD_WD0_CONFIG_INT_MASK_MSK;
  v &= ~MCU_WD_WD0_CONFIG_ENABLE_MSK;
  alt1250_wdt_unlock();
  REGISTER(MCU_WD_WD0_CONFIG) = v;

  v = REGISTER(MCU_REGFILE_MCU_STATUS_MASK_PMP);
  v &= ~MCU_REGFILE_MCU_STATUS_MCU_WD_MSK;
  REGISTER(MCU_REGFILE_MCU_STATUS_MASK_PMP) = v;
}

static void alt1250_wdt_set_timeout() {
  alt1250_wdt_unlock();
  REGISTER(MCU_WD_WD0_CNT_LOAD) = g_timeout_value;
}

wdt_status_e watchdog_kick(void) {
  if (!alt1250_wdt_is_enabled()) return WDT_FAILED;
  alt1250_wdt_ping();
  return WDT_SUCCESS;
}

void watchdog_set_timeout(wdt_timeout_t new_timeout) {
  if (new_timeout > MAX_HEARTBEAT) new_timeout = MAX_HEARTBEAT;

  if (new_timeout < MIN_HEARTBEAT) new_timeout = MIN_HEARTBEAT;

  g_timeout_value = ALT1250_WATCHDOG_CLK_RATE * new_timeout;
  alt1250_wdt_set_timeout();
}

void watchdog_sleep_pre_process(void) {
  wd_ctx_reg_config = REGISTER(MCU_WD_WD0_CONFIG);
  wd_ctx_reg_cnt_load = REGISTER(MCU_WD_WD0_CNT_LOAD);
  wd_ctx_reg_pmp_mask = REGISTER(MCU_REGFILE_MCU_STATUS_MASK_PMP);
  if (wd_ctx_reg_config & MCU_WD_WD0_CONFIG_ENABLE_MSK) alt1250_wdt_disable();
}

void watchdog_sleep_post_process(void) {
  alt1250_wdt_unlock();
  REGISTER(MCU_WD_WD0_CNT_LOAD) = wd_ctx_reg_cnt_load;
  alt1250_wdt_unlock();
  REGISTER(MCU_WD_WD0_CONFIG) = wd_ctx_reg_config;

  REGISTER(MCU_REGFILE_MCU_STATUS_MASK_PMP) = wd_ctx_reg_pmp_mask;
}

wdt_timeout_t watchdog_get_max_timeout(void) { return g_timeout_value / ALT1250_WATCHDOG_CLK_RATE; }

wdt_timeout_t watchdog_get_time_left(void) {
  unsigned long v;
  v = REGISTER(MCU_WD_WD0_CNT_VAL);
  return (wdt_timeout_t)(v / ALT1250_WATCHDOG_CLK_RATE);
}

wdt_status_e watchdog_stop(void) {
  if (!alt1250_wdt_is_enabled()) return WDT_FAILED;

  send_wdt_enable_to_pmp(0);
  NVIC_DisableIRQ(MCU_WD0_IRQn);
  alt1250_wdt_disable();
  return WDT_SUCCESS;
}

void watchdog_start(wdt_timeout_t timeout) {
  if (alt1250_wdt_is_enabled()) {
    watchdog_stop();
  }
  g_timeout_value = ALT1250_WATCHDOG_CLK_RATE * timeout;
  alt1250_wdt_enable();
  alt1250_wdt_set_timeout();
  NVIC_SetPriority(MCU_WD0_IRQn, 7);
  NVIC_EnableIRQ(MCU_WD0_IRQn);
  send_wdt_enable_to_pmp(1);
}

void watchdog_register_timeout_callback(wdt_timeout_cb cb, void *user_param) {
  g_timeout_cb = cb;
  g_timeout_cb_param = user_param;
}
