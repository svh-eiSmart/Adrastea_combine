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

#include <hifc/hifc_mi.h>
#include "hifc_api.h"
#include "serial.h"
//#include "atomic_counter.h"

hifc_level_change_callback gM2H_LEVEL_CHANGE_CALLBACK;

void MAILBOX_IRQHandler(unsigned int user_param) {
  uint32_t val;

  val = atomic_counters_get_count(HIFC_INTERNAL_MODEM_2_HOST);
  if (gM2H_LEVEL_CHANGE_CALLBACK) {
    gM2H_LEVEL_CHANGE_CALLBACK((enum gpio_state)val);
  }
}

/* ------ funtions implementation -------*/

/* ------ h2m driver -------*/
void hifc_mi_h2m_set(enum gpio_state e_state) {
  uint32_t val = atomic_counters_get_count(HIFC_INTERNAL_HOST_2_MODEM);

  if (e_state == set) {
    if (val == 0) {
      atomic_counters_increment(HIFC_INTERNAL_HOST_2_MODEM, 1);
    }
  } else {
    if (val == 1) {
      atomic_counters_decrement(HIFC_INTERNAL_HOST_2_MODEM, 1);
    }
  }
}

/* ------ m2h driver -------*/
void hifc_mi_m2h_init(hifc_level_change_callback callback, enum gpio_state wait_for) {
  /* init gpio to be used for m2h,
   assign callbeck to be used
   by HIFC Managing Task notification  */
  gM2H_LEVEL_CHANGE_CALLBACK = callback;
  if (wait_for == clear)
    atomic_counters_set_irq_mode(HIFC_INTERNAL_MODEM_2_HOST,
                                 ATOMIC_COUNTER_DEC_INTERRUPT_MODE_VALUE);
  // PORT_SetPinInterruptConfig(PORTD, M2H_PIN, kPORT_InterruptLogicZero
  // /*kPORT_InterruptFallingEdge*/);
  else
    atomic_counters_set_irq_mode(HIFC_INTERNAL_MODEM_2_HOST,
                                 ATOMIC_COUNTER_NOT_ZERO_INTERRUPT_MODE_VALUE);
  // PORT_SetPinInterruptConfig(PORTD, M2H_PIN,
  // kPORT_InterruptLogicOne/*kPORT_InterruptRisingEdge*/);

  atomic_counters_ena_irq(HIFC_INTERNAL_MODEM_2_HOST);
}

void hifc_mi_m2h_isr_deinit(void) {
  // PORT_SetPinInterruptConfig(PORTD, M2H_PIN, kPORT_InterruptOrDMADisabled);
  atomic_counters_dis_irq(HIFC_INTERNAL_MODEM_2_HOST);
}

/* Unused function */
void hifc_mi_utx_set(enum gpio_state e_state) { /* internal MCU doen't need this functin*/
}
void hifc_mi_urx_init(hifc_level_change_callback callback,
                      enum gpio_state wait_for) { /* internal MCU doen't need this functin*/
}
void hifc_mi_urx_isr_deinit(void) { /* internal MCU doen't need this functin*/
}
void hifc_mi_txrx_to_uart(void) { /* internal MCU doen't need this functin*/
}
void hifc_mi_txrx_to_gpio(void) { /* internal MCU doen't need this functin*/
}
void hifc_mi_disableRts(void) { /* internal MCU doen't need this functin*/
}
void hifc_mi_enableRts(void) { /* internal MCU doen't need this functin*/
}
void enable_TxDataRegEmptyInterrupt(
    hifc_sys_lpuart_t *base) { /* internal MCU doen't need this functin*/
}
void hifc_mi_enableTxRx(hifc_sys_lpuart_t *base) { /* internal MCU doen't need this functin*/
}
void hifc_mi_disableTxRx(hifc_sys_lpuart_t *base) { /* internal MCU doen't need this functin*/
}
void rst_ind_isr_on(
    reset_indication_mi_callback callback) { /* internal MCU doen't need this functin*/
}
void rst_ind_isr_off(void) { /* internal MCU doen't need this functin*/
}

uint32_t is_hwflow_on(hifc_sys_lpuart_t *base) { return -1; }

uint32_t hifc_mi_is_tx_empty(hifc_sys_lpuart_t *base) { return is_serial_tx_busy(base); }

uint32_t get_pin_value(uint32_t *port, uint32_t pin) {
  if (pin == HIFC_INTERNAL_MODEM_2_HOST) {
    return atomic_counters_get_count(HIFC_INTERNAL_MODEM_2_HOST);
  } else if (pin == HIFC_INTERNAL_HOST_2_MODEM) {
    return atomic_counters_get_count(HIFC_INTERNAL_HOST_2_MODEM);
  }
  return -1;
}

void init_mi(struct hifc_mi_callbacks *callbacks) {
  callbacks->h2m_set_cb = hifc_mi_h2m_set;
  callbacks->m2h_init_cb = hifc_mi_m2h_init;
  callbacks->txrx_to_gpio_cb = hifc_mi_txrx_to_gpio;
  callbacks->txrx_to_uart_cb = hifc_mi_txrx_to_uart;
  callbacks->urx_init_cb = hifc_mi_urx_init;
  callbacks->utx_set_cb = hifc_mi_utx_set;
  callbacks->urx_isr_deinit_cb = hifc_mi_urx_isr_deinit;
  callbacks->m2h_isr_deinit_cb = hifc_mi_m2h_isr_deinit;
  callbacks->enableRts_cb = hifc_mi_enableRts;
  callbacks->disableRts_cb = hifc_mi_disableRts;
  callbacks->is_tx_empty_cb = hifc_mi_is_tx_empty;
  callbacks->enableTxRx_cb = hifc_mi_enableTxRx;
  callbacks->disableTxRx_cb = hifc_mi_disableTxRx;

  atomic_counters_init_counter(HIFC_INTERNAL_HOST_2_MODEM, 0, 0);
  atomic_counters_init_counter(HIFC_INTERNAL_MODEM_2_HOST, MAILBOX_IRQHandler, 0);
}
