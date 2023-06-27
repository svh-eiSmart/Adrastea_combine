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

#ifndef HIFC_API_H
#define HIFC_API_H

#include <stdio.h>
#include "hifc_osal_opt.h"
#include "hifc_osal.h"
#include <hifc/hifc_mi.h>

/**
 * @defgroup HIFC HIFC API
 * @{
 */

/**
 * @defgroup hifc_api_enum Enumeration
 * @{
 */

/*! @brief Definitions of HIFC request to send status */
enum hifc_request_to_send { hifc_req_fail, hifc_req_success, hifc_req_resuming };
/*! @brief Definitions of HIFC interface state */
enum hifc_if_state {
  IfDown,
  IfUp,
  IfSuspending,
  IfResuming,
  IfMdmResuming,
};

/*! @brief Definitions of HIFC running mode */
enum hifc_mode { hifc_mode_a, hifc_mode_b, hifc_mode_c, hifc_mode_off };

/*! @brief Definitions of HIFC status report field */
struct hifc_status_report {
  enum hifc_mode eMode;         /*!< current running HIFC mode  */
  enum hifc_if_state eIf_state; /*!< current HIFC interface state */
  uint32_t hres_timeout;        /*!< host resume timeout amount */
  uint32_t hsusp_timeout;       /*!< host suspend timeout amount */
  uint32_t mres_timeout;        /*!< modem resumetimeout amount */
  uint32_t hres_req;            /*!< host resume request amount */
  uint32_t hres_comp;           /*!< host resume complete amount */
  uint32_t hsus_req;            /*!< host suspend requeset amount */
  uint32_t hsus_comp;           /*!< host suspend complete amount */
  uint32_t mres_req;            /*!< modem resume requeset amount */
  uint32_t mres_comp;           /*!< modem resume complete amount */
  uint32_t inactivity_time;     /*!< Remaining inactivity time  */
  uint32_t hw_flow_ctl;         /*!< hardware flow control status */
  uint32_t uart_tx_value;       /*!< UART Tx status */
  uint32_t uart_rx_value;       /*!< UART Rx status */
  uint32_t uart_cts_value;      /*!< UART CTS status */
  uint32_t uart_rts_value;      /*!< UART RTS status */
  uint32_t h2m_value;           /*!< H2M status */
  uint32_t m2h_value;           /*!< M2H status */
};

/**
 * @brief Set output level of H2M pin
 * @param e_state [in]: Output level. See @ref gpio_state for reference
 */
typedef void (*h2m_set_callback)(enum gpio_state e_state);

/**
 * @brief Set interrupt trigger condition for M2H pin
 * @param callback [in]: The function should be called to upper layer when interrupt comes. See @ref
 * hifc_level_change_callback
 * @param wait_for [in]: The GPIO state should wait. See @ref gpio_state
 * @retval Set, set GPIO to rising edge trigger.
 * @retval clear, set GPIO to falling edge trigger.
 */
typedef void (*m2h_init_callback)(hifc_level_change_callback callback, enum gpio_state wait_for);
/**
 * @brief Disable interrupt on UART Rx pin
 */
typedef void (*m2h_isr_deinit_callback)(void);

/**
 * @brief Set output level of Uart Tx pin.
 *
 * @param e_state [in]: Output level. See @ref gpio_state for reference
 */
typedef void (*utx_set_callback)(enum gpio_state e_state);

/**
 * @brief Set interrupt trigger condition for UART Rx pin
 *
 * @param callback [in]: The function should be called to upper layer when interrupt comes. See @ref
 * hifc_level_change_callback
 * @param wait_for [in]: The GPIO state should wait. See @ref gpio_state.
 * @retval Set, set GPIO to rising edge trigger.
 * @retval Clear, set GPIO to falling edge trigger.
 */
typedef void (*urx_init_callback)(hifc_level_change_callback callback, enum gpio_state wait_for);

/**
 * @brief Disable interrupt on UART Rx pin
 */
typedef void (*urx_isr_deinit_callback)(void);

/**
 * @brief Set the functionality of Tx and Rx of UART to UART
 */
typedef void (*txrx_to_uart_callback)(void);

/**
 * @brief Set the functionality of Tx and Rx of UART to GPIO
 */
typedef void (*txrx_to_gpio_callback)(void);

/**
 * @brief To set RTS pin as UART RTS functionality.
 */
typedef void (*enableRts_callback)(void);

/**
 * @brief To set RTS pin as GPIO and output low.
 */
typedef void (*disableRts_callback)(void);

/**
 * @brief Enable the UART functionality
 *
 * @param base [in] uart controller base address
 */
typedef void (*enableTxRx_callback)(hifc_sys_lpuart_t* base);

/**
 * @brief Disable the UART functionality
 *
 * @param base [in] uart controller base address
 */
typedef void (*disableTxRx_callback)(hifc_sys_lpuart_t* base);

/**
 * @brief Check if uart tx queue is empty
 * @param base [in] uart controller base address
 * @return uint32_t the status of Tx queue
 * @retval 1 Tx queue is not empty
 * @retval 0 Tx queue is empty
 */
typedef uint32_t (*is_tx_empty_callback)(hifc_sys_lpuart_t* base);

/**
 * @brief Prototype of reset indication handler
 */
typedef void (*reset_indication_api_callback)(void);

/**
 * @brief The definition of callback functions
 *
 */
struct hifc_mi_callbacks {
  h2m_set_callback h2m_set_cb;               /*!< callback to set H2M */
  m2h_init_callback m2h_init_cb;             /*!< callback to enable M2H interupt */
  utx_set_callback utx_set_cb;               /*!< callback to set UART Tx */
  urx_init_callback urx_init_cb;             /*!< callback to enable UART Rx interrupt */
  txrx_to_uart_callback txrx_to_uart_cb;     /*!< callback to change IO to UART function */
  txrx_to_gpio_callback txrx_to_gpio_cb;     /*!< callback to change UART to GPIO */
  urx_isr_deinit_callback urx_isr_deinit_cb; /*!< callback to disable UART Rx interrupt */
  m2h_isr_deinit_callback m2h_isr_deinit_cb; /*!< callback to disable M2H interrupt */
  enableRts_callback enableRts_cb;           /*!< callback to enable UART RTS interrupt */
  disableRts_callback disableRts_cb;         /*!< callback to disable UART RTS interrupt */
  is_tx_empty_callback is_tx_empty_cb;       /*!< callback to check if tx buffer empty */
  enableTxRx_callback enableTxRx_cb;         /*!< callback to enable UART function */
  disableTxRx_callback disableTxRx_cb;       /*!< callback to disable UART Rx interrupt */
};

/** @} hifc_api_enum */

/**
 * @defgroup HIFC_APIs HIFC APIs
 * @{
 */

/*! @brief To reset the inactivity timer */
#define reset_inactivity_timer(base) generic_reset_inactivity_timer(base, false)

/**
 * @brief Initialize HIFC protocol
 * @param uart_base [in]: The uart port that is running HIFC protocol.
 * @return Return 0 on success. Otherwise negative value is returned.
 */
int32_t hifc_host_interface_init(hifc_sys_lpuart_t* uart_base);

/**
 * @brief get HIFC interface state.
 *
 * @return Current HIFC interface state. Refer to @ref hifc_if_state
 *
 */
enum hifc_if_state get_if_state(void);
/**
 * @brief Set running mode of HIFC.
 *
 * @param mode [in]: Running mode. See @ref hifc_mode
 *
 */
void set_hifc_mode(enum hifc_mode mode);

/**
 * @brief To check if the uart port is running HIFC protocol
 * @param base [in]: The uart port that is running HIFC protocol.
 * @return Return the result
 * @retval 0 the port is NOT running HIFC protocol
 * @retval 1 the port is running HIFC protocol
 **/
uint32_t isHifcPort(hifc_sys_lpuart_t* base);

/**
 * @brief Stop inactivity timer
 * @param base [in]: The uart port that is running HIFC protocol.
 *
 */
int32_t stop_inactivity_timer(hifc_sys_lpuart_t* base);

/**
 * Clear HIFC protocol timeout data.
 *
 */
void clear_hifc_status_report(void);

/**
 * @brief Get HIFC run time status report
 * @param rep [out]: HIFC run time report
 *
 */
void get_hifc_status_report(struct hifc_status_report* rep);

/**
 * @brief Change the value of UART inactivity timer.
 * @param ms [in]: UART inactivity time in ms.
 *
 */
int32_t set_inactivity_timer(uint32_t ms);

/**
 * @brief Do host suspend immediately
 *
 * @return uint32_t Return 0 on success. Otherwise negative value is returned.
 */

int32_t force_host_suspend(void);

/**
 * @brief Do host resume immediately
 *
 * @return uint32_t Return 0 on success. Otherwise negative value is returned.
 */
int32_t force_host_resume(void);

/**
 * @brief To reset the inactivity timer
 *
 * @param base [in]: The uart port that is running HIFC protocol.
 * @param do_reset [in]: Whether to reset timer immediately. 0: reset later. others: reset
 * immediately.
 * @return int32_t Return 0 on success. Otherwise negative value is returned.
 */
int32_t generic_reset_inactivity_timer(hifc_sys_lpuart_t* base, uint32_t do_reset);

/**
 * @brief UART driver should call this function to make sure the UART is ready to send data.
 *        This function does "Host resume" once HIFC interface is IfDown.
 *
 * @param base The uart port that is running HIFC protocol.
 * @return enum hifc_request_to_send If HIFC interface is neither IfUp nor IfDown, return
 hifc_req_fail. <br> If HIFC interface is IfUp, return hifc_req_success. <br> If HIFC interface is
 IfDown, return hifc_req_resuming
 *
 */
enum hifc_request_to_send request_to_send(hifc_sys_lpuart_t* base);

/**
 * @brief To init reset indication function
 *
 * @param callback The callback function that to be invokded when assertion on reset indication pin
 * @return uint32_t Return 0 on success. Otherwise negative value is returned.
 */
uint32_t hifc_rst_indication_init(reset_indication_api_callback callback);
/** @} HIFC_APIs */
/** @} HIFC */

#endif
