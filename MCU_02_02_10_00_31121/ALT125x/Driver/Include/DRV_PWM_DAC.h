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
 * @file DRV_PWM_DAC.h
 */
#ifndef DRV_PWM_DAC_H_
#define DRV_PWM_DAC_H_
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdint.h>
/**
 * @defgroup pwm_driver PWM-DAC Driver
 * @{
 */
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/****************************************************************************
 * Data types
 ****************************************************************************/
/**
 * @defgroup pwm_data_types PWM-DAC Types
 * @{
 */
/**
 * @struct PWM_DAC_Param
 * Definition of pwm configuration defined in interfaces_config_alt1250.h.
 */
typedef struct {
  uint32_t clk_div;    /**< CLK divider setting. Dividing input clock to reduce power. The block can
                          work at any from input clk to 1/16 of input clk allowing SW to control the on
                          and off duration and power consumption. Available value is from 1 to 16.*/
  uint32_t duty_cycle; /**< Duty cycle setting. Duty cycle setting of this PWM channe. 0: Disable
                        * PWM and output 0. 1024: Disable PWM and output 1. 1 ~ 1023: duty cycle in
                        * the rate of 1024. */
} PWM_DAC_Param;
/**
 * @typedef PWM_DAC_Id
 * Definition of PWM channel ID
 */
typedef enum {
  PWM_DAC_ID_0 = 0, /**< PWM-DAC channel 0*/
  PWM_DAC_ID_1,     /**< PWM-DAC channel 1*/
  PWM_DAC_ID_2,     /**< PWM-DAC channel 2*/
  PWM_DAC_ID_3,     /**< PWM-DAC channel 3*/
  PWM_DAC_ID_MAX
} PWM_DAC_Id;
/**
 * @typedef DRV_PWM_DAC_Status
 * Definition of DRV_PWM_DAC API return code
 */
typedef enum {
  DRV_PWM_DAC_OK,                /**< API returns with no error*/
  DRV_PWM_DAC_ERROR_INIT,        /**< API returns with wrong init state*/
  DRV_PWM_DAC_ERROR_IF_CFG,      /**< API returns with inteface configuration error*/
  DRV_PWM_DAC_ERROR_PARAM,       /**< API returns with wrong parameter*/
  DRV_PWM_DAC_ERROR_POWER,       /**< API returns with wrong power state*/
  DRV_PWM_DAC_ERROR_UNSUPPORTED, /**< API returns with not support*/
  DRV_PWM_DAC_ERROR_PM           /**< API returns with power manager related error*/
} DRV_PWM_DAC_Status;

/**
 * @typedef PWM_DAC_PowerState
 * Contains Power State modes
 */
typedef enum {
  PWM_POWER_FULL, /**< Set-up peripheral for full operationnal mode. Can be called multiple times.
                     If the peripheral is already in this mode the function performs no operation
                     and returns with DRV_PWM_DAC_OK. */
  PWM_POWER_LOW,  /**< May use power saving. Returns DRV_PWM_DAC_ERROR_UNSUPPORTED when not
                     implemented.*/
  PWM_POWER_OFF   /**< terminates any pending operation and disables peripheral and related
                     interrupts. This is the state after device reset. */
} PWM_DAC_PowerState;
/** @} pwm_data_types */

/*******************************************************************************
 * API
 ******************************************************************************/
/*! @cond Doxygen_Suppress */
#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif
/*! @endcond */
/**
 * @defgroup pwm_apis PWM-DAC APIs
 * @{
 */
/**
 * @brief Initialize PWM_DAC interface.
 *
 * @return @ref DRV_PWM_DAC_Status
 */
DRV_PWM_DAC_Status DRV_PWM_DAC_Initialize(void);

/**
 * @brief Uninitialize PWM_DAC interface.
 *
 * @return @ref DRV_PWM_DAC_Status
 */
DRV_PWM_DAC_Status DRV_PWM_DAC_Uninitialized(void);

/**
 * @brief Control PWM_DAC interface power state.
 *
 * @param [in] state: Power state with type @ref PWM_DAC_PowerState.
 *
 * @return @ref DRV_PWM_DAC_Status
 */
DRV_PWM_DAC_Status DRV_PWM_DAC_PowerControl(PWM_DAC_PowerState state);

/**
 * @brief Helper function to get PWM DAC parameters from MCU wizard.
 * Get default configuration from MCU wizard.
 *
 * @param [in] pwm_id: PWM channel ID with type @ref PWM_DAC_Id.
 *
 * @param [out] param: Pointer to configured parameters.
 *
 * @return @ref DRV_PWM_DAC_Status
 */
DRV_PWM_DAC_Status DRV_PWM_DAC_GetDefConf(PWM_DAC_Id pwm_id, PWM_DAC_Param *param);

/**
 * @brief Configure duty cycle in run time.
 *
 * @param [in] pwm_id: PWM channel ID with type @ref PWM_DAC_Id
 *
 * @param [in] param: Parameter with type @ref PWM_DAC_Param of this PWM channel.
 *
 * @return @ref DRV_PWM_DAC_Status
 */
DRV_PWM_DAC_Status DRV_PWM_DAC_ConfigOutputChannel(PWM_DAC_Id pwm_id, PWM_DAC_Param *param);

/**
 * @brief Query current configuration of each pwm channel.
 *
 * @param [in] pwm_id: PWM channel ID with type @ref PWM_DAC_Id
 *
 * @param [out] enable: PWM channel enable status
 *
 * @param [out] status: PWM channel status with type @ref PWM_DAC_Param
 *
 * @return @ref DRV_PWM_DAC_Status
 */
DRV_PWM_DAC_Status DRV_PWM_DAC_GetChannelStatus(PWM_DAC_Id pwm_id, uint32_t *enable,
                                                PWM_DAC_Param *status);
/** @} pwm_apis */
/*! @cond Doxygen_Suppress */
#undef EXTERN
#ifdef __cplusplus
}
#endif
/*! @endcond */
/** @} pwm_driver */
#endif /*DRV_PWM_DAC_H_*/
