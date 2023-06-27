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

#include "hifc_api.h"
//#include "fsl_lpuart.h"

static struct hifc_mi_callbacks gHifc_mi_cb;
static struct inactivity_timeout gInact_tout_anchor;
static volatile struct inactivity_timeout gInact_tout_running;
static volatile uint32_t isUartTraffic;
static reset_indication_api_callback gRST_IND_CB;
static void hifc_urx_level_change_callback(enum gpio_state e_state);
static void hifc_m2h_level_change_callback(enum gpio_state e_state);
static enum hifc_fsm hifc_host_suspend_sm(void);
static enum hifc_fsm hifc_host_resume_sm(void);
static enum hifc_fsm hifc_modem_resume_sm(void);
void hifc_protocol_timeout_isr(hifc_sys_timer_t timer);
void hifc_inactivity_timeout_isr(hifc_sys_timer_t timer);

// clang-format off
#define GPIO_RX_INTERRUPT  (0x1)
#define GPIO_M2H_INTERRUPT (0x2)
#define PROTOCOL_TIMEOUT   (1000)  // 1 sec
#define INACT_TIMEOUT      (5000)  // 5 sec
#define CMODE_RESUME_TIME  (500)   // 500ms by spec
// clang-format on

/*! @brief Definitions of HIFC state machine */
enum hifc_fsm {
  hifc_fsm_init,
  hifc_fsm_response,
  hifc_fsm_config,
  hifc_fsm_approve,
  hifc_fsm_active,
  hifc_fsm_complete,
  hifc_fsm_error,
  hifc_fsm_timeout,
};

struct hifc_priv {
  volatile enum hifc_if_state eIf_state;
  volatile enum hifc_fsm eFSM;
  enum hifc_mode eMode;
  hifc_sys_timer_t xInactivity_Timer;
  hifc_sys_timer_t protcol_timeout_timer;
  hifc_sys_timer_t cmode_resume_timer;
  hifc_sys_lpuart_t* uart_base;
  uint32_t inactivity_time;
  uint32_t hres_timeout;
  uint32_t hsusp_timeout;
  uint32_t mres_timeout;
  uint32_t hres_req;
  uint32_t hres_comp;
  uint32_t hsus_req;
  uint32_t hsus_comp;
  uint32_t mres_req;
  uint32_t mres_comp;
};

struct inactivity_timeout {
  /*inactivity time = section*period */
  uint32_t section;
  uint32_t period;
};

static struct hifc_priv gHifc_struct = {
    .eIf_state = IfDown, .eMode = hifc_mode_off, .uart_base = NULL};

/***** Private functions *****/

static void set_if_state(enum hifc_if_state new_state) { gHifc_struct.eIf_state = new_state; }

static enum hifc_fsm hifc_get_host_interface_state(void) { return gHifc_struct.eFSM; }

static void hifc_set_host_interface_state(enum hifc_fsm e_istate) { gHifc_struct.eFSM = e_istate; }

static enum hifc_mode get_hifc_mode(void) { return gHifc_struct.eMode; }

static inline void restore_inactivity_timeout(void) {
  gInact_tout_running.section = gInact_tout_anchor.section;
}

static int hifc_host_suspend(hifc_sys_lpuart_t* base) {
  if (!base) return -HIFC_EINVAL;
  if (!isHifcPort(base)) return -HIFC_EWONTDO;
  if (get_if_state() != IfUp) return -HIFC_EALRDYDONE;
  gHifc_struct.hsus_req++;
  hifc_set_host_interface_state(hifc_fsm_init);

  hifc_host_suspend_sm();
  return 0;
}

void hifc_cmode_resume_time_isr(hifc_sys_timer_t timer) {
  set_if_state(IfUp);
  enable_TxDataRegEmptyInterrupt(gHifc_struct.uart_base);
  generic_reset_inactivity_timer(gHifc_struct.uart_base, true);
}

void hifc_inactivity_timeout_isr(hifc_sys_timer_t timer) {
  if (isUartTraffic) {
    isUartTraffic = 0;
    restore_inactivity_timeout();
    hifc_osal_reset_timer_fromISR(&gHifc_struct.xInactivity_Timer);
  } else {
    if (--gInact_tout_running.section)
      hifc_osal_reset_timer_fromISR(&gHifc_struct.xInactivity_Timer);
    else {
      if (!gHifc_mi_cb.is_tx_empty_cb(gHifc_struct.uart_base))
        hifc_host_suspend(gHifc_struct.uart_base);
      else /* if tx is still busy, restart inavtivity timer again*/
        generic_reset_inactivity_timer(gHifc_struct.uart_base, true);
    }
  }
}
void hifc_protocol_timeout_isr(hifc_sys_timer_t timer) {
  hifc_set_host_interface_state(hifc_fsm_timeout);
  enum hifc_mode mode = get_hifc_mode();

  if (get_if_state() == IfSuspending) {
    /* fail to suspend.  */
    if (mode == hifc_mode_a) {
      gHifc_mi_cb.enableTxRx_cb(gHifc_struct.uart_base);
      gHifc_mi_cb.txrx_to_uart_cb();
    }
    gHifc_struct.hsusp_timeout++;
    gHifc_mi_cb.h2m_set_cb(set);

    set_if_state(IfUp);
    /* Different function has different effect.
     * hifc_osal_reset_timer_fromISR() : HIFC suspend will be invoked after xInactivity_Timer on
     * time out hifc_host_suspend() :    HIFC suspend will be invoked immediately on time out
     * */
    hifc_host_suspend(gHifc_struct.uart_base);
  } else if (get_if_state() == IfResuming) {
    /* fail to resume*/
    gHifc_struct.hres_timeout++;
    if (mode == hifc_mode_a) {
      gHifc_mi_cb.disableTxRx_cb(gHifc_struct.uart_base);
      gHifc_mi_cb.txrx_to_gpio_cb();
    }

    gHifc_mi_cb.h2m_set_cb(clear);
    gHifc_mi_cb.disableRts_cb();
    gHifc_mi_cb.m2h_init_cb(hifc_m2h_level_change_callback, set);
    set_if_state(IfDown);
  } else if (get_if_state() == IfMdmResuming) {
    /* fail to modem resume*/
    gHifc_struct.mres_timeout++;
    if (mode == hifc_mode_a) {
      gHifc_mi_cb.disableTxRx_cb(gHifc_struct.uart_base);
      gHifc_mi_cb.utx_set_cb(clear);
    }
    hifc_set_host_interface_state(hifc_fsm_config);
    gHifc_mi_cb.m2h_init_cb(hifc_m2h_level_change_callback, set);
    set_if_state(IfDown);
  }
}

static enum hifc_fsm hifc_host_resume_sm(void) {
  enum hifc_mode mode = get_hifc_mode();
  switch (hifc_get_host_interface_state()) {
    case (hifc_fsm_init):
      set_if_state(IfResuming);
      if (mode == hifc_mode_a) {
        // dh0 only
        gHifc_mi_cb.urx_init_cb(hifc_urx_level_change_callback, set);
        hifc_set_host_interface_state(hifc_fsm_config);
        gHifc_mi_cb.h2m_set_cb(set);
        hifc_osal_reset_timer_fromISR(&gHifc_struct.protcol_timeout_timer);
      } else if (mode == hifc_mode_b) {
        gHifc_mi_cb.h2m_set_cb(set);
        hifc_set_host_interface_state(hifc_fsm_approve);
        gHifc_mi_cb.enableTxRx_cb(gHifc_struct.uart_base);
        /* register m2h isr after set h2m to avoid host_resume and modem_resume happens at the same
         * time*/
        gHifc_mi_cb.m2h_init_cb(hifc_m2h_level_change_callback, set);
        hifc_osal_reset_timer_fromISR(&gHifc_struct.protcol_timeout_timer);
      } else if (mode == hifc_mode_c) {
        gHifc_mi_cb.h2m_set_cb(set);
        hifc_set_host_interface_state(hifc_fsm_complete);
      }
      break;

    case (hifc_fsm_config):
      /* called from Rx ISR */

      // stop protocol error timer
      hifc_osal_stop_timer_fromISR(&gHifc_struct.protcol_timeout_timer);
      hifc_set_host_interface_state(hifc_fsm_approve);
      gHifc_mi_cb.m2h_init_cb(hifc_m2h_level_change_callback, set);

      gHifc_mi_cb.enableTxRx_cb(gHifc_struct.uart_base);
      gHifc_mi_cb.txrx_to_uart_cb();
      hifc_osal_reset_timer_fromISR(&gHifc_struct.protcol_timeout_timer);

      break;

    case (hifc_fsm_approve):
      /* called from m2h ISR */
      hifc_osal_stop_timer_fromISR(&gHifc_struct.protcol_timeout_timer);
      set_if_state(IfUp);
      hifc_set_host_interface_state(hifc_fsm_complete);
      break;

    case (hifc_fsm_complete):
      break;

    default:
      hifc_set_host_interface_state(hifc_fsm_error);
  }

  if (hifc_get_host_interface_state() == hifc_fsm_complete) {
    gHifc_struct.hres_comp++;
    if (mode == hifc_mode_a) gHifc_mi_cb.enableRts_cb();

    if (mode == hifc_mode_a || mode == hifc_mode_b) {
      // start inactivity timer
      enable_TxDataRegEmptyInterrupt(gHifc_struct.uart_base);
      generic_reset_inactivity_timer(gHifc_struct.uart_base, true);
    } else if (mode == hifc_mode_c) {
      // wait 500 ms then set interface to UP
      hifc_osal_reset_timer_fromISR(&gHifc_struct.cmode_resume_timer);
    }
  }

  return hifc_get_host_interface_state();
}

static enum hifc_fsm hifc_host_suspend_sm(void) {
  enum hifc_mode mode = get_hifc_mode();

  switch (hifc_get_host_interface_state()) {
    case (hifc_fsm_init):
      set_if_state(IfSuspending);

      if (mode == hifc_mode_a || mode == hifc_mode_b) {
        hifc_set_host_interface_state(hifc_fsm_config);

        gHifc_mi_cb.m2h_init_cb(hifc_m2h_level_change_callback, clear);
        hifc_osal_reset_timer_fromISR(
            &gHifc_struct.protcol_timeout_timer);  // start protocol err timer

        gHifc_mi_cb.disableTxRx_cb(gHifc_struct.uart_base);
        gHifc_mi_cb.h2m_set_cb(clear);
      } else if (mode == hifc_mode_c) {
        hifc_set_host_interface_state(hifc_fsm_complete);
        gHifc_mi_cb.h2m_set_cb(clear);
      }
      break;

    case (hifc_fsm_config):
      /* called from m2h ISR */
      // stop protocol err timer
      hifc_osal_stop_timer_fromISR(&gHifc_struct.protcol_timeout_timer);

      if (mode == hifc_mode_a) {
        hifc_set_host_interface_state(hifc_fsm_approve);
        gHifc_mi_cb.urx_init_cb(hifc_urx_level_change_callback, clear);
        // start protocol err timer
        hifc_osal_reset_timer_fromISR(&gHifc_struct.protcol_timeout_timer);
        // reconfig uart to gpio and set Tx to low
        gHifc_mi_cb.txrx_to_gpio_cb();

      } else if (mode == hifc_mode_b) {
        hifc_set_host_interface_state(hifc_fsm_complete);
      }

      break;

    case (hifc_fsm_approve):
      // called from Rx ISR
      // stop protocol err timer
      hifc_osal_stop_timer_fromISR(&gHifc_struct.protcol_timeout_timer);
      if (mode == hifc_mode_a) {
        hifc_set_host_interface_state(hifc_fsm_complete);
      }

      break;
    case (hifc_fsm_complete):
      break;
    default:
      hifc_set_host_interface_state(hifc_fsm_error);
  }

  if (hifc_get_host_interface_state() == hifc_fsm_complete) {
    gHifc_struct.hsus_comp++;
    if (mode == hifc_mode_a) gHifc_mi_cb.disableRts_cb();

    if (mode == hifc_mode_a || mode == hifc_mode_b) {
      hifc_set_host_interface_state(hifc_fsm_init);
      hifc_modem_resume_sm();
    }
    set_if_state(IfDown);
  }

  return hifc_get_host_interface_state();
}

static enum hifc_fsm hifc_modem_resume_sm(void) {
  switch (hifc_get_host_interface_state()) {
    case (hifc_fsm_init):
      hifc_set_host_interface_state(hifc_fsm_config);
      gHifc_mi_cb.m2h_init_cb(hifc_m2h_level_change_callback, set);
      break;

    case (hifc_fsm_config):
      /* called from m2h ISR */
      set_if_state(IfMdmResuming);
      gHifc_struct.mres_req++;
      if (get_hifc_mode() == hifc_mode_a) {
        gHifc_mi_cb.urx_init_cb(hifc_urx_level_change_callback, set);
        // start protocol err timer
        hifc_osal_reset_timer_fromISR(&gHifc_struct.protcol_timeout_timer);
        hifc_set_host_interface_state(hifc_fsm_approve);
        gHifc_mi_cb.utx_set_cb(set);  // hifc_mi_utx_set(set);
      } else if (get_hifc_mode() == hifc_mode_b) {
        gHifc_mi_cb.enableTxRx_cb(gHifc_struct.uart_base);
        hifc_set_host_interface_state(hifc_fsm_complete);
        gHifc_mi_cb.h2m_set_cb(set);  // hifc_mi_h2m_set(set);
      }

      break;

    case (hifc_fsm_approve):
      /* called from Rx ISR */
      if (get_hifc_mode() == hifc_mode_a) {
        // stop protocol err timer
        hifc_osal_stop_timer_fromISR(&gHifc_struct.protcol_timeout_timer);
        gHifc_mi_cb.txrx_to_uart_cb();  // hifc_mi_txrx_to_uart();

        gHifc_mi_cb.enableTxRx_cb(gHifc_struct.uart_base);
        hifc_set_host_interface_state(hifc_fsm_complete);
        gHifc_mi_cb.h2m_set_cb(set);  // hifc_mi_h2m_set(set);
      }
      break;
    case (hifc_fsm_complete):
      break;

    default:
      hifc_set_host_interface_state(hifc_fsm_error);
  }

  if (hifc_get_host_interface_state() == hifc_fsm_complete) {
    gHifc_struct.mres_comp++;
    if (get_hifc_mode() == hifc_mode_a) {
      // hifc_mi_enableRts();
      gHifc_mi_cb.enableRts_cb();
    }
    set_if_state(IfUp);
    // start inactivity timer
    generic_reset_inactivity_timer(gHifc_struct.uart_base, true);
  }

  return hifc_get_host_interface_state();
}

static int32_t hifc_host_resume(hifc_sys_lpuart_t* base) {
  if (!base) return -HIFC_EINVAL;
  if (!isHifcPort(base)) return -HIFC_EWONTDO;
  if (get_if_state() != IfDown) return -HIFC_EALRDYDONE;
  gHifc_struct.hres_req++;
  /*m2h interrupt is on during IfDown. Disable it since host resume is taking place.*/
  gHifc_mi_cb.m2h_isr_deinit_cb();
  hifc_set_host_interface_state(hifc_fsm_init);

  hifc_host_resume_sm();

  return 0;
}

static void hifc_urx_level_change_callback(enum gpio_state pin_level) {
  gHifc_mi_cb.urx_isr_deinit_cb();

  if (get_if_state() == IfSuspending && pin_level == clear) {
    /* host suspend */
    hifc_host_suspend_sm();
  } else if (get_if_state() == IfResuming && pin_level == set) {
    /* host resume */
    hifc_host_resume_sm();
  } else if (get_if_state() == IfMdmResuming && pin_level == set) {
    /* modem resume */
    hifc_modem_resume_sm();
  }
}

static void hifc_m2h_level_change_callback(enum gpio_state pin_level) {
  gHifc_mi_cb.m2h_isr_deinit_cb();

  if (get_if_state() == IfSuspending && pin_level == clear) {
    /* host suspend */
    hifc_host_suspend_sm();
  } else if (get_if_state() == IfResuming && pin_level == set) {
    /* host resume */
    hifc_host_resume_sm();
  } else if (get_if_state() == IfDown && pin_level == set) {
    /* modem resume */
    hifc_modem_resume_sm();
  }
}

static uint32_t get_reset_timer_period(uint32_t inact_time) {
#define PERIOD_PER_SEC (50)

  if (inact_time > PERIOD_PER_SEC) {
    gInact_tout_anchor.section = inact_time / PERIOD_PER_SEC;
    gInact_tout_anchor.period = PERIOD_PER_SEC;
  } else {
    gInact_tout_anchor.section = 1;
    gInact_tout_anchor.period = inact_time;
  }
  return 0;
}

/******  Public functions ******/
int32_t hifc_host_interface_init(hifc_sys_lpuart_t* uart_base) {
  configASSERT(uart_base);

  if (!gHifc_struct.uart_base) {
    init_mi(&gHifc_mi_cb);  // this is platform dependent function
    gHifc_mi_cb.h2m_set_cb(clear);
    set_if_state(IfDown);
    gHifc_struct.eMode = hifc_mode_off;
    gHifc_struct.uart_base = uart_base;
    gHifc_struct.hres_timeout = 0;
    gHifc_struct.hsusp_timeout = 0;
    gHifc_struct.mres_timeout = 0;
    gHifc_struct.inactivity_time = INACT_TIMEOUT;  // default value is 5 sec.
    isUartTraffic = 0;
    gHifc_mi_cb.disableTxRx_cb(gHifc_struct.uart_base);
    gHifc_mi_cb.txrx_to_gpio_cb();
    gHifc_mi_cb.disableRts_cb();
    get_reset_timer_period(gHifc_struct.inactivity_time);

    if (hifc_osal_create_timer(&gHifc_struct.protcol_timeout_timer, PROTOCOL_TIMEOUT, false,
                               hifc_protocol_timeout_isr)) {
      return -HIFC_ENOMEM;
    }
    if (hifc_osal_create_timer(&gHifc_struct.xInactivity_Timer, gInact_tout_anchor.period, false,
                               hifc_inactivity_timeout_isr)) {
      return -HIFC_ENOMEM;
    }
    if (hifc_osal_create_timer(&gHifc_struct.cmode_resume_timer, CMODE_RESUME_TIME, false,
                               hifc_cmode_resume_time_isr)) {
      return -HIFC_ENOMEM;
    }
  }
  return 0;
}

void rst_ind_callback(void) {
  if (gRST_IND_CB) gRST_IND_CB();
}

uint32_t hifc_rst_indication_init(reset_indication_api_callback callback) {
  if (!callback) return -1;

  gRST_IND_CB = callback;
  rst_ind_isr_on(rst_ind_callback);

  return 0;
}

enum hifc_if_state get_if_state(void) { return gHifc_struct.eIf_state; }

void set_hifc_mode(enum hifc_mode mode) {
  if (mode > hifc_mode_c) return;
  // only allows changing mode once.
  if (gHifc_struct.eMode != hifc_mode_off) return;

  gHifc_struct.eMode = mode;

  if (mode != hifc_mode_a) {
    // keeps RTS outputs low
    gHifc_mi_cb.enableTxRx_cb(gHifc_struct.uart_base);
    gHifc_mi_cb.txrx_to_uart_cb();
  }

  if (mode == hifc_mode_a || mode == hifc_mode_b) {
    hifc_set_host_interface_state(hifc_fsm_init);
    hifc_modem_resume_sm();
  }
}

uint32_t isHifcPort(hifc_sys_lpuart_t* base) {
  if (!base) return 0;
  if (get_hifc_mode() == hifc_mode_off) return 0;

  return base == gHifc_struct.uart_base;
}

int32_t stop_inactivity_timer(hifc_sys_lpuart_t* base) {
  if (!base) return -HIFC_EINVAL;
  if (get_hifc_mode() == hifc_mode_off) return -HIFC_EPERM;

  if (get_if_state() == IfUp && gHifc_struct.uart_base == base) {
    hifc_osal_stop_timer_fromISR(&gHifc_struct.xInactivity_Timer);
  }
  return 0;
}

void clear_hifc_status_report(void) {
  gHifc_struct.hres_timeout = 0;
  gHifc_struct.hsusp_timeout = 0;
  gHifc_struct.mres_timeout = 0;
  gHifc_struct.hres_req = 0;
  gHifc_struct.hres_comp = 0;
  gHifc_struct.hsus_req = 0;
  gHifc_struct.hsus_comp = 0;
  gHifc_struct.mres_req = 0;
  gHifc_struct.mres_comp = 0;
}

void get_hifc_status_report(struct hifc_status_report* rep) {
  if (!rep) return;

  rep->eMode = gHifc_struct.eMode;
  rep->eIf_state = gHifc_struct.eIf_state;
  rep->hres_timeout = gHifc_struct.hres_timeout;
  rep->hsusp_timeout = gHifc_struct.hsusp_timeout;
  rep->mres_timeout = gHifc_struct.mres_timeout;
  rep->hres_req = gHifc_struct.hres_req;
  rep->hres_comp = gHifc_struct.hres_comp;
  rep->hsus_req = gHifc_struct.hsus_req;
  rep->hsus_comp = gHifc_struct.hsus_comp;
  rep->mres_req = gHifc_struct.mres_req;
  rep->mres_comp = gHifc_struct.mres_comp;
  rep->inactivity_time = gHifc_struct.inactivity_time;
  rep->hw_flow_ctl = is_hwflow_on(gHifc_struct.uart_base);
  rep->uart_tx_value = get_pin_value(LPUART_TX_PORT, LPUART_TX_PIN);
  rep->uart_rx_value = get_pin_value(LPUART_RX_PORT, LPUART_RX_PIN);
  rep->uart_rts_value = get_pin_value(LPUART_RTS_PORT, LPUART_RTS_PIN);
  rep->uart_cts_value = get_pin_value(LPUART_CTS_PORT, LPUART_CTS_PIN);
  rep->h2m_value = get_pin_value(H2M_PORT, H2M_PIN);
  rep->m2h_value = get_pin_value(M2H_PORT, M2H_PIN);
}

int32_t set_inactivity_timer(uint32_t ms) {
  if (get_hifc_mode() == hifc_mode_off) return -HIFC_EPERM;

  gHifc_struct.inactivity_time = ms < 50 ? 50 : ms;

  if (get_if_state() == IfDown) {
    get_reset_timer_period(gHifc_struct.inactivity_time);
    if (hifc_osal_change_timer_period_fromISR(&gHifc_struct.xInactivity_Timer,
                                              gInact_tout_anchor.period))
      return -HIFC_EBUSY;
    if (hifc_osal_stop_timer_fromISR(&gHifc_struct.xInactivity_Timer)) return -HIFC_EBUSY;
  } else if (get_if_state() == IfUp) {
    get_reset_timer_period(gHifc_struct.inactivity_time);
    if (hifc_osal_change_timer_period_fromISR(&gHifc_struct.xInactivity_Timer,
                                              gInact_tout_anchor.period))
      return -HIFC_EBUSY;
    if (generic_reset_inactivity_timer(gHifc_struct.uart_base, true)) return -HIFC_EBUSY;
  } else /* return busy if HIFC is suspending or resuming */
    return -HIFC_EBUSY;

  return 0;
}

int32_t force_host_suspend(void) {
  uint32_t ret = 0;

  if (get_hifc_mode() == hifc_mode_off) return -HIFC_EPERM;

  if (get_if_state() == IfUp) {
    ret = hifc_osal_stop_timer_fromISR(&gHifc_struct.xInactivity_Timer);
    if (ret) goto error;
    ret = hifc_host_suspend(gHifc_struct.uart_base);
    if (ret) goto error;
    return ret;
  } else if (get_if_state() == IfDown)
    return ret;
  else
    return -HIFC_EBUSY;
error:
  return ret;
}

int32_t force_host_resume(void) {
  if (get_hifc_mode() == hifc_mode_off) return -HIFC_EPERM;

  if (get_if_state() == IfDown)
    return hifc_host_resume(gHifc_struct.uart_base);
  else if (get_if_state() == IfUp)
    return 0;
  else
    return -HIFC_EBUSY;
}

int32_t generic_reset_inactivity_timer(hifc_sys_lpuart_t* base, uint32_t do_reset) {
  if (!base) return -HIFC_EINVAL;
  if (get_hifc_mode() == hifc_mode_off) return -HIFC_EPERM;

  if (get_if_state() == IfUp && gHifc_struct.uart_base == base) {
    if (do_reset) {
      restore_inactivity_timeout();
      hifc_osal_reset_timer_fromISR(&gHifc_struct.xInactivity_Timer);
    } else
      isUartTraffic = 1;
  }
  return 0;
}

enum hifc_request_to_send request_to_send(hifc_sys_lpuart_t* base) {
  uint32_t is_hifc_port = isHifcPort(base);
  enum hifc_if_state state;
  enum hifc_request_to_send ret = hifc_req_success;

  if (is_hifc_port) {
    if (get_hifc_mode() == hifc_mode_off) return hifc_req_fail;

    // disable interrupt to avoid inactivity timeout
    hifc_osal_enter_critical();

    state = get_if_state();

    if (state == IfUp) {
      // to avoid triggering HIFC suspend
      reset_inactivity_timer(base);
      ret = hifc_req_success;
    } else if (state == IfDown) {
      hifc_host_resume(base);
      ret = hifc_req_resuming;
    } else  // HIFC is either suspending or resuming
      ret = hifc_req_fail;

    hifc_osal_exit_critical();
  }
  return ret;
}
