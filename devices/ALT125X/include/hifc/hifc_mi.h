/*-------------------------------------------------------------------------
* (c) copyright 2019 � Altair Semiconductor, Ltd.
* All rights reserved.

This Sample Code (the �Code�), provided to you in source or binary form,
is the property of Altair Semiconductor Ltd. (the �Company�) and/or its
licensors, which have all right, title and interest therein. You may use
the Code only in accordance with the terms of written commercial agreement
between you and the Company (the �Agreement�). Except as expressly stated
in the Agreement, the Company grants no licenses by implication, estoppel
or otherwise. If you are not aware of or do not agree to the Agreement
terms, you may not use, copy or modify the Code. You may use the Code,
whether in its source form or binary form, only for your internal purposes
and may not distribute the Code, any part thereof, or any derivative work
thereof, to any third party, except pursuant to the Company�s prior written
consent, at its sole discretion.
The Code is the confidential and proprietary information of the Company.


THIS CODE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS �AS IS�
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE, ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE), ARISING IN ANY WAY OUT OF THE USE OF THIS
CODE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
---------------------------------------------------------------------------
*    file: hifc_mi.h
* version: 70
*    date: 17_02_2019 14_05
*   brief: drivers interfaces
*-------------------------------------------------------------------------*/

/**
 * @file hifc_mi.h
 * @brief HAL layer which handles IOs and interrupts.
 */

#ifndef HIFC_MI_H
#define HIFC_MI_H
#include "serial_container.h"
#include "atomic_counter.h"

/**
 * @defgroup HIFC_MI HIFC machine interface
 * @{
 */
#define LPUART_RX_PORT NULL
#define LPUART_RX_PIN 8U
/*! @brief  Uart Tx pin */
#define LPUART_TX_PORT NULL
#define LPUART_TX_PIN 9U

#define LPUART_RTS_PORT NULL
#define LPUART_RTS_PIN 10U

#define LPUART_CTS_PORT NULL
#define LPUART_CTS_PIN 11U

/*! @brief  M2H pin */
#define M2H_PORT NULL
#define M2H_PIN HIFC_INTERNAL_MODEM_2_HOST
/*! @brief  H2M pin */
#define H2M_PORT NULL
#define H2M_PIN HIFC_INTERNAL_HOST_2_MODEM

/**
 * @defgroup hifc_mi_enum Enumeration
 *
 * @{
 */

/*! @brief  Definitions of GPIO output level */
enum gpio_state { clear, set };
/** @} hifc_mi_enum */

/**
 * @defgroup hifc_mi_const Constant
 * @{
 */

typedef serial_handle hifc_sys_lpuart_t;

/** @}hifc_mi_const */

/**
 * @defgroup hifc_mi_cbs Callbacks
 * @{
 */
struct hifc_mi_callbacks;

/*! @brief  Prototype of callback function on IO changed */
typedef void (*hifc_level_change_callback)(enum gpio_state e_state);
typedef void (*reset_indication_mi_callback)(void);
/**@} hifc_mi_cbs */

/**
 * @defgroup mi_function HAL APIs
 * HAL functions that should be customized
 * @{
 */

/**
 * Check is hardware flow control is enabled
 *
 * @param base [in]: The UART port that is going to enable.
 * @return Return 1 on enabled. Return 0 on disabled
 * @retval 1 hardware flow control is enabled
 * @retval 0 hardware flow control is disabled
 */
uint32_t is_hwflow_on(hifc_sys_lpuart_t *base);

/**
 * Query the pin level
 *
 * @param port [in]: The base address of GPIO port.
 * @param pin [in]: The pin number
 * @return Return the pin level
 * @retval 1 the pin is high
 * @retval 0 the pin is low
 */
uint32_t get_pin_value(uint32_t *port, uint32_t pin);

/**
 * Enable TxDataRegEmptyInterrupt flag so that UART IP will generate an interrupt while Tx ia
 * available.
 *
 * @param base [in]: The UART port that is going to enable.
 */
void enable_TxDataRegEmptyInterrupt(hifc_sys_lpuart_t *base);

/**
 * @brief Turn on interrupt of reset indication pin
 *
 * @param callback [in]: the function to be invoked when interrupt arrives
 */
void rst_ind_isr_on(reset_indication_mi_callback callback);

/**
 * @brief Turn off the interrupt of reset indication pin
 *
 */
void rst_ind_isr_off(void);

/**
 * @brief Init platform dependent callback functions
 *
 * @param callbacks [out]: paremter to get callback functions
 */
void init_mi(struct hifc_mi_callbacks *callbacks);
/** @} di_function*/
/** @} HIFC_MI*/
#endif
