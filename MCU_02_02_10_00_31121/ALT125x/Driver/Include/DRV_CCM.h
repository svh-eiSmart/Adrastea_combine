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
 * @file DRV_CCM.h
 */
#ifndef DRV_CCM_H_
#define DRV_CCM_H_
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdint.h>
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/**
 * @defgroup ccm_driver CCM Driver
 * @{
 */
/**
 * @defgroup ccm_constants CCM Constants
 * @{
 */
/*CCM event bitmask definition for DRV_CCM_EventCallback*/
#define CCM_ID_0_TIMER_EVT (0x01UL)   /**< CCM Channel 0 timer INT event*/
#define CCM_ID_0_COMPARE_EVT (0x02UL) /**< CCM Channel 0 compare INT event*/
#define CCM_ID_1_TIMER_EVT (0x04UL)   /**< CCM Channel 1 timer INT event*/
#define CCM_ID_1_COMPARE_EVT (0x08UL) /**< CCM Channel 1 compare INT event*/

/*CCM mode definition*/
#define CCM_MODE_PWM_ONE_THRESHOLD (0x01UL)
/**< One comparison point and wrap around PWM mode
 * Generate pulse sequence with programmable frequency, duty cycle and phase.
 * As long as the internal timer value is bigger than the threshold value, the duration
 * of the PWM output signal is low (or high) */
#define CCM_MODE_1BIT_DAC (0x02UL)
/**< 1bit-dac is a special case in PWM while the output pulse width should be minimum
 * (one clock) while the frequency could be change so in amount of 1024 clocks you will
 * receive N required pulses each of them one cycle width.*/
#define CCM_MODE_CLK_OUT (0x03UL)
/**< clkout is a special case in PWM while the output duty cycle should try to be 50% */

/** @} ccm_constants  */

/**
 * @defgroup ccm_data_type CCM Types
 * @{
 */
/**
 * @enum DRV_CCM_Status
 * @brief CCM (Capture Compare Module) Status Error Codes
 */
typedef enum {
  DRV_CCM_OK,                /**< Operation succeeded.*/
  DRV_CCM_ERROR_INIT,        /**< Device is in wrong init state */
  DRV_CCM_ERROR_IF_CFG,      /**< Wrong configuration in interface header */
  DRV_CCM_ERROR_POWER,       /**< Device is in wrong power state */
  DRV_CCM_ERROR_UNSUPPORTED, /**< Operation not supported.*/
  DRV_CCM_ERROR_PARAM,       /**< Parameter error. */
  DRV_CCM_ERROR_GENERIC      /**< Unspecified error. */
} DRV_CCM_Status;
/**
 * @typedef DRV_CCM_EventCallback
 * DRV_CCM_EventCallback: is an optional callback function that is registered with the Initialize
 * function. This callback function is initiated from interrupt service routines and indicates
 * hardware events or the completion of a data block transfer operation.
 * @param[in] event. Event bit mask
 */
typedef void (*DRV_CCM_EventCallback)(uint32_t event);

/**
 * @struct CCM_Capabilities
 * @brief The data fields of this structure encode the capabilities implemented by this driver.
 */
typedef struct {
  uint8_t one_threshold_pwm; /**< Nb of One threshold PWM feature*/
  uint8_t two_threshold_pwm; /**< Nb of Two threshold PWM feature*/
  uint8_t clkout;            /**< Nb of Generate equal duty cycle clock out feature*/
  uint8_t one_bit_dac;       /**< Nb of Generate narrow pulse feature*/
  uint8_t led;               /**< Nb of Led output feature*/
  uint8_t cascade; /**< While we want to combine few slots together in cascade so they drive each
                        other with any mathematical operation (or/and/xor) */
  uint8_t capture_alarm;    /**< Nb of Input pin from external driver feature*/
  uint8_t graceful_force;   /**< ???? */
  uint8_t shadow_registers; /**< ???? */
} CCM_Capabilities;

/**
 * @struct CCM_ChannelStatus
 * @brief The is used to query the current status of a ccm channel.
 */
typedef struct {
  uint32_t enable; /**< CCM channel is enabled */
  uint32_t mode;   /**< CCM channel working mode */
  uint32_t param1; /**< Current config of param1 of a CCM channel which applied by
                      CCM_ConfigOutputChannel */
  uint32_t param2; /**< Current config of param2 of a CCM channel which applied by
                      CCM_ConfigOutputChannel */
  uint32_t param3; /**< Current config of param3 of a CCM channel which applied by
                      CCM_ConfigOutputChannel and it is only meaningful when CCM channel is working
                      on CCM_MODE_PWM_ONE_THRESHOLD */
} CCM_ChannelStatus;
/**
 * @enum CCM_PowerState
 * @brief Contains Power State modes
 */
typedef enum {
  CCM_POWER_FULL, /**< Set-up peripheral for full operationnal mode. Can be called multiple times.
                     If the peripheral is already in this mode the function performs no operation
                     and returns with DRV_CCM_OK. */
  CCM_POWER_LOW,  /**< May use power saving. Returns DRV_CCM_ERROR_UNSUPPORTED when not implemented.
                   */
  CCM_POWER_OFF   /**< terminates any pending operation and disables peripheral and related
                     interrupts. This is the state after device reset. */
} CCM_PowerState;

/**
 * @enum CCM_Id
 * @brief Definition of CCM channel ID
 */
typedef enum {
  CCM_ID_0 = 0, /**< Output Channel 0*/
  CCM_ID_1,     /**< Output channel 1*/
  CCM_ID_MAX    /**< Total number of channel */
} CCM_Id;

/** @} ccm_data_types */

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
 * @defgroup ccm_apis CCM APIs
 * @{
 */
/**
 * @brief Can be called at any time to obtain version information of the driver interface.
 * @return Returns version information of the driver implementation.
 * @note The version is encoded as 32-bit unsigned value (uint32_t) with:
 *  	API version (uint16_t) which is the version of the Driver specification used to implement
 *this driver. high-byte: major version = 1 (for this release)
 *			low-byte: minor version = 0	(for this release)
 *		Driver version (uint16_t) which is the source code version of the actual driver
 *implementation. high-byte: major version low-byte: minor version
 */
uint32_t DRV_CCM_GetVersion(void);

/**
 * @brief Can be called at any time to obtain capabilities of the driver interface.
 * @return Returns information about capabilities in this driver implementation.
 * @note
 */
const CCM_Capabilities *DRV_CCM_GetCapabilities(void);

/**
 * @brief Initialize CCM Interface.
 * @param [in] cb_event: Pointer to CCM_SignalEvent
 * @return Returns Status Error Codes.
 * @note The function performs the following operations:
 *	Initializes the resources needed for the CCM interface.
 *	Registers the CCM_SignalEvent callback function.
 *	The parameter cb_event is a pointer to the CCM_SignalEvent callback function; use a NULL
 *pointer when no callback signals are required.
 */
DRV_CCM_Status DRV_CCM_Initialize(DRV_CCM_EventCallback cb_event);

/**
 * @brief De-initialize CCM Interface.
 * @return Returns Status Error Codes.
 * @note De-initializes the resources of CCM interface
 */
DRV_CCM_Status DRV_CCM_Uninitialize(void);

/**
 * @brief Control CCM Interface Power.
 * @param [in] state: Power state
 * @return Returns Status Error Codes.
 * @note The function CCM_PowerControl operates the power modes of the CCM interface.
 */
DRV_CCM_Status DRV_CCM_PowerControl(CCM_PowerState state);

/**
 * @brief Control CCM Interface.
 * @param [in] control: control Operation
 * @param [in] arg: Argument of operation

 * @return Returns Status Error Codes.
 * @note The function CCM_Control controls Capture Compare Module interface settings and execute
 various operations.
 * control 	= CCM_MODE_PWM_ONE_THRESHOLD to set CCM in simple PWM One Threshold mode (arg
 parameter to be used to select CCM channel)
 * 			= CM_MODE_1BIT_DAC to set CCM in 1 bit DAC mode (arg parameter to be used to
 select CCM channel)
 * 			= CCM_MODE_CLK_OUT to set CCM in Clock Out mode (arg parameter to be used to
 select CCM channel)
*/
DRV_CCM_Status DRV_CCM_Control(uint32_t control, uint32_t arg);

/**
 * @brief Configure duty cycle in run time.
 * @param [in] ccm_id: Output channel to control with enum CCM_ID (max number is returned
 * by CCM_GetCapabilities()).
 * @param [in] param1:
 * CCM_MODE_PWM_ONE_THRESHOLD: The total period of the PWM output waveform (value in ns)
 * CCM_MODE_1BIT_DAC: Max_timer - the maximum value of the timer
 * CCM_MODE_CLK_OUT: Max_timer - the maximum value of the timer
 * @param [in] param2:
 * CCM_MODE_PWM_ONE_THRESHOLD: The active time of the PWM output (value in ns)
 * CCM_MODE_1BIT_DAC: Step - how many to add each clk to the timer counter
 * CCM_MODE_CLK_OUT: Step - how many to add each clk. The output frequency therefore is freq =
 * in_clk * (step/max).
 * @param [in] param3:
 * CM_MODE_PWM_ONE_THRESHOLD: PWM polarity (0 or 1)
 * CCM_MODE_1BIT_DAC: N/A
 * CCM_MODE_CLK_OUT: N/A
 * @return Returns Status Error Codes.
 */
DRV_CCM_Status DRV_CCM_ConfigOutputChannel(CCM_Id ccm_id, uint32_t param1, uint32_t param2,
                                           uint32_t param3);

/**
 * @brief This function enables/disables CCM Output channels
 * @param[in] channel_mask: channel_mask is a combination of enabled channels. Each bit
 * corresponds to a channel. Bit 0 is channel 0, bit 1 is channel 1...
 * @return None
 */
void DRV_CCM_EnableDisableOutputChannels(uint32_t channel_mask);

/**
 * @brief This function queries the current status of a CCM channel.
 * @param [in] ccm_id: Output channel to control with enum CCM_ID (max number is returned
 * by CCM_GetCapabilities()).
 * @param [out] status: Queried status of the specified CCM channel.
 * @return Returns Status Error Codes.
 */
DRV_CCM_Status DRV_CCM_GetChannelStatus(CCM_Id ccm_id, CCM_ChannelStatus *status);
/** @} ccm_apis */
/*! @cond Doxygen_Suppress */
#undef EXTERN
#ifdef __cplusplus
}
#endif
/*! @endcond */
/** @} ccm_driver */
#endif /*DRV_CCM_H_*/
