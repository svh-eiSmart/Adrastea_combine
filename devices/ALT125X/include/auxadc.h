/*  ---------------------------------------------------------------------------

        (c) copyright 2018 Altair Semiconductor, Ltd. All rights reserved.

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

/**
 * @file auxadc.h
 */

#ifndef AUXADC_ALT125X_H_
#define AUXADC_ALT125X_H_

#include "FreeRTOS.h"

/**
 * @defgroup auxadc AUXADC Driver
 * @{
 */

/**
 * @defgroup ADC_const AUXADC Constants
 * @{
 */

// clang-format off

#define AUX_ADC_OTHER_ERROR             7001 /*!< Unclassfied error */
#define AUX_ADC_INVALID_CHANNEL_NUMBER  7002 /*!< Invalid channel number*/
#define AUX_ADC_PARAMETERS_OUT_OF_RANGE 7003 /*!< Parameter out of range*/
#define AUX_ADC_PIN_FORBIDDEN           7004 /*!< Pin/port is forbidden for this CPU*/
#define AUX_ADC_DIFFERENT_FUNCTIONALITY 7005 /*!< Pin assigned for different functionality*/
#define AUX_ADC_RESOURSE_BUSY           7006 /*!< AUXADC is not available*/
#define AUX_ADC_INTERRUPT_WITH_HANDLER  7008 /*!< Will notify ADC result via callback function. This is not an error*/
#define AUX_ADC_INTERRUPT_REGISTERED    7009 /*!< ISR has been registered*/
#define AUX_ADC_INIT_FAIL               7010 /*!< AUXADC initialization failure*/
#define AUX_ADC_UNINIT                  7011 /*!< AUXADC hasn't been initialized*/
// clang-format on
/** @} ADC_const */

/**
 * @defgroup ADC_types AUXADC Types
 * @{
 */
/** @brief AUXADC available channels*/
typedef enum { AUX_ADC_0 = 0, AUX_ADC_1, AUX_ADC_2, AUX_ADC_3, AUX_ADC_MAX } auxadc_enum_t;

/** @brief prototype of callback function */
typedef void (*auxadc_callback)(int error, uint32_t digital_value, uint32_t user_param);
/** @} ADC_types */

/**
 * @defgroup auxadc_api AUXADC APIs
 * @{
 */
/**
 * @brief Init aux adc driver.This function must be called before calling any auxadc functions.
 *
 * @return 0 on success.<br>
 *         -AUX_ADC_INIT_FAIL on failure.
 */
int auxadc_init(void);

/**
 * @brief Reads value on requested ADC channel. ADC channel 0 to 3 corresponds to GPIO 1 to 4
 *
 * @param [in] adc_channel: Choose which ADC channel to be read. Valid number is from 0 to 3 which
 * represents adc channel from 0 to 3
 * @param [in] average_count: sample times on adc channel and takes the average. valid value from 1
 * to 127
 * @param [out] value: returned ADC value in mV
 *
 * @return If positive, represents raw ADC value in mV.<br>
 *         If -AUX_ADC_INTERRUPT_WITH_HANDLER, represents value will report by interrupt.<br>
 *         Other negetive value, represents error
 */

int adc_get_value(auxadc_enum_t adc_channel, uint32_t average_count, uint32_t *value);

/**
 * @brief Register an ADC interrupt on a specific ADC channel.
 *
 * @param [in] adc_channel: valid number is from 0 to 3 which represents ADC channel from 0 to 3
 * @param [in] irq_handler: callback function once ADC finished
 * @param [in] user_param: parameter which passed to callback function
 *
 * @return 0 on success.<br>
 *         -AUX_ADC_UNINIT on auxadc not initialized<br>
 *         -AUX_ADC_INVALID_CHANNEL_NUMBER on wrong channel number<br>
 *         -AUX_ADC_INTERRUPT_REGISTERED on interrupt has been registerd.
 */
int auxadc_register_interrupt(auxadc_enum_t adc_channel, auxadc_callback irq_handler,
                              uint32_t user_param);

/**
 * @brief Deregister an ADC interrupt on a specific ADC channel
 *
 * @param [in] adc_channel: valid number is from 0 to 3 which represents ADC channel from 0 to 3
 *
 * @return 0 on success.<br>
 *         -AUX_ADC_UNINIT on auxadc not initialized.<br>
 *         -AUX_ADC_INVALID_CHANNEL_NUMBER on wrong channel number
 */
int auxadc_deregister_interrupt(auxadc_enum_t adc_channel);

/**
 * @brief Set ADC channel returned scale. Default is 1800 mV
 *
 * @param [in] scaling_multiplier: Prefered ADC scale in mV
 */
void adc_set_scaling(uint32_t scaling_multiplier);

/**
 * @brief returns value reading from battery sensor(8th channel of aux_adc, AUX_ADC_B7)
 *
 * @return if positive value, represents battery level in mV.<br>
 *         if negetive value, represents error code as following<br>
 *            -1: AGU empty<br>
 *            -2: divided by 0 during converting<br>
 *            -3: larger than maximun value AUX_ADC_VBAT_MAX_VOLTAGE
 */
int adc_get_bat_sns(void);
/** @} auxadc_api */
/** @} auxadc */

#endif /* AUXADC_ALT125X_H_ */
