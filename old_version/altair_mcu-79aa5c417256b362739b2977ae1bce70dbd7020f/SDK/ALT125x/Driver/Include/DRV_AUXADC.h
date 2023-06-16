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
/**
 * @file DRV_AUXADC.h
 */

#ifndef AUXADC_ALT125X_H_
#define AUXADC_ALT125X_H_
#include "stdio.h"
/**
 * @defgroup auxadc AUXADC Driver
 * @{
 */

/**
 * @defgroup   AUXADC Types
 * @{
 */

/*! AUXADC error code*/
typedef enum auxadc_err_code {
  AUXADC_ERROR_NONE,           /*!< No error */
  AUXADC_ERROR_WAIT_INTERRUPT, /*!< Will notify ADC result via callback function. This is not an
                                  error */
  AUXADC_ERROR_CHANNEL_NUMBER, /*!< Invalid channel number */
  AUXADC_ERROR_RESOURCE_BUSY,  /*!< AUXADC resource is not available */
  AUXADC_ERROR_PARAMETER,      /*!< Wrong parameter */
  AUXADC_ERROR_IO_CONFIG,      /*!< Failure on GPIO config */
  AUXADC_ERROR_TIMEOUT,        /*!< Convertion timeout */
  AUXADC_ERROR_OUT_OF_RANGE,   /*!< Convertion result out of range */
  AUXADC_ERROR_CB_REGISTERD,   /*!< Callback function have beed registerd */
  AUXADC_ERROR_OTHER_ERROR,    /*!< Unclassfied error */
} Auxadc_Err_Code;

/** @brief AUXADC available channels*/
typedef enum {
  AUX_ADC_0,
  AUX_ADC_1,
  AUX_ADC_2,
#ifdef ALT1250
  AUX_ADC_3,
#endif
  AUX_ADC_MAX
} auxadc_enum_t;

/** @brief prototype of callback function */
typedef void (*auxadc_callback)(Auxadc_Err_Code error, uint32_t digital_value,
                                void *callback_param);
/** @} ADC_types */

#if defined(__cplusplus)
extern "C" {
#endif /* _cplusplus */

/**
 * @defgroup auxadc_api AUXADC APIs
 * @{
 */

/**
 * @brief Initialize ADC driver.
 *
 * @return Return the error code. Refer to @ref Auxadc_Err_Code.
 */
Auxadc_Err_Code DRV_AUXADC_Initialize(void);

/**
 * @brief Uninitialize ADC driver
 *
 * @return Return the error code. Refer to @ref Auxadc_Err_Code.
 */
Auxadc_Err_Code DRV_AUXADC_Uninitialize(void);
/**
 * @brief Reads value on requested ADC channel. This API will turn on GPIO clock and set
 * corresponding pin to INPUT with pull-none.
 *
 * @param [in] adc_channel: Choose which ADC channel to be read. Refer to @ref auxadc_enum_t
 * @param [in] average_count: sample times on adc channel and takes the average. valid value from 1
 * to 127
 * @param [out] adc_value: returned ADC value in mV
 *
 * @return Return the error code. Refer to @ref Auxadc_Err_Code.
 */

Auxadc_Err_Code DRV_AUXADC_Get_Value(auxadc_enum_t adc_channel, uint8_t average_count,
                                     uint32_t *adc_value);

/**
 * @brief Register an ADC interrupt on a specific ADC channel.
 *
 * @param [in] adc_channel: Choose which ADC channel to be registered. Refer to @ref auxadc_enum_t
 * @param [in] callback: callback function to notify upper layer when ADC convertion is done.
 * @param [in] callback_param: parameter which passed to callback function
 *
 * @return Return the error code. Refer to @ref Auxadc_Err_Code.
 */
Auxadc_Err_Code DRV_AUXAUDC_Register_Interrupt(auxadc_enum_t adc_channel, auxadc_callback callback,
                                               void *callback_param);
/**
 * @brief Deregister an ADC interrupt on a specific ADC channel
 *
 * @param [in] adc_channel: Choose which ADC channel to be deregistered. Refer to @ref auxadc_enum_t
 *
 * @return Return the error code. Refer to @ref Auxadc_Err_Code.
 */
Auxadc_Err_Code DRV_AUXADC_Deregister_Interrupt(auxadc_enum_t adc_channel);

/**
 * @brief Set ADC channel returned scale. Default is 1800 mV
 *
 * @param [in] scaling_multiplier: Prefered ADC scale in mV
 * @return Return the error code. Refer to @ref Auxadc_Err_Code.
 */
Auxadc_Err_Code DRV_AUXADC_Set_Scale(uint32_t scaling_multiplier);

/**
 * @brief returns value reading from battery sensor(8th channel of aux_adc, AUX_ADC_B7)
 *
 * @return if positive value, represents battery level in mV.<br>
 *         if negetive value, represents error code as following<br>
 *            -1: AGU empty<br>
 *            -2: divided by 0 during converting<br>
 *            -3: larger than maximun value AUX_ADC_VBAT_MAX_VOLTAGE
 */
int DRV_AUXADC_Get_Bat_Sns(void);

/**
 * @brief returns temperature value
 *
 * @return Return temperature.
 */
int32_t DRV_AUXADC_Get_Temperature(void);

#if defined(__cplusplus)
}
#endif /* _cplusplus */
/** @} auxadc_api */
/** @} auxadc */

#endif /* AUXADC_ALT125X_H_ */
