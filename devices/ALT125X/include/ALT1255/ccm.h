/*  ---------------------------------------------------------------------------

    (c) copyright 2020 Altair Semiconductor, Ltd. All rights reserved.

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
 * @file ccm.h
 */
#ifndef _CCM_H_
#define _CCM_H_
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
// CCM (Capture Compare Module) Status Error Codes
#define CCM_DRIVER_OK (0)                 /**< Operation succeeded.*/
#define CCM_DRIVER_ERROR (-1)             /**< Unspecified error. */
#define CCM_DRIVER_ERROR_BUSY (-2)        /**< Driver is busy. */
#define CCM_DRIVER_ERROR_TIMEOUT (-3)     /**< Timeout occurred. */
#define CCM_DRIVER_ERROR_UNSUPPORTED (-4) /**< Operation not supported.*/
#define CCM_DRIVER_ERROR_PARAMETER (-5)   /**< Parameter error. */
#define CCM_DRIVER_ERROR_SPECIFIC (-6)    /**< Start of driver specific errors. */

#define CCM_ID_0_TIMER_EVT (0x1)   /**< CCM Channel 0 timer INT event*/
#define CCM_ID_0_COMPARE_EVT (0x2) /**< CCM Channel 0 compare INT event*/
#define CCM_ID_1_TIMER_EVT (0x4)   /**< CCM Channel 1 timer INT event*/
#define CCM_ID_1_COMPARE_EVT (0x8) /**< CCM Channel 1 compare INT event*/

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
 * @typedef CCM_SignalEvent_t
 * SignalEvent: is an optional callback function that is registered with the Initialize function.
 * This callback function is initiated from interrupt service routines and indicates hardware events
 * or the completion of a data block transfer operation.
 * @param[in] event. Event bit mask
 */
typedef void (*CCM_SignalEvent_t)(uint32_t event);

/**
 * @struct CCM_CAPABILITIES
 * @brief The data fields of this structure encode the capabilities implemented by this driver.
 */
typedef struct {
  uint8_t OneThresholdPWM; /**< Nb of One threshold PWM feature*/
  uint8_t TwoThresholdPWM; /**< Nb of Two threshold PWM feature*/
  uint8_t ClkOut;          /**< Nb of Generate equal duty cycle clock out feature*/
  uint8_t OneBitDac;       /**< Nb of Generate narrow pulse feature*/
  uint8_t Led;             /**< Nb of Led output feature*/
  uint8_t Cascade; /**< While we want to combine few slots together in cascade so they drive each
                        other with any mathematical operation (or/and/xor) */
  uint8_t CaptureAlarm;    /**< Nb of Input pin from external driver feature*/
  uint8_t GracefulForce;   /**< ???? */
  uint8_t ShadowRegisters; /**< ???? */
} CCM_CAPABILITIES;

/**
 * @struct CCM_CHANNEL_STATUS
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
} CCM_CHANNEL_STATUS;
/**
 * @enum CCM_POWER_STATE
 * @brief Contains Power State modes
 */
typedef enum {
  POWER_FULL, /**< Set-up peripheral for full operationnal mode. Can be called multiple times. If
                 the peripheral is already in this mode the function performs no operation and
                 returns with CCM_DRIVER_OK. */
  POWER_LOW,  /**< May use power saving. Returns CCM_DRIVER_ERROR_UNSUPPORTED when not implemented.
               */
  POWER_OFF   /**< terminates any pending operation and disables peripheral and related interrupts.
                 This is the state after device reset. */
} CCM_POWER_STATE;

/**
 * @enum CCM_ID
 * @brief Definition of CCM channel ID
 */
typedef enum {
  CCM_ID_0 = 0, /**< Output Channel 0*/
  CCM_ID_1,     /**< Output channel 1*/
  CCM_NUM       /**< Total number of channel */
} CCM_ID;

/** @} ccm_data_types */

/*******************************************************************************
 * API
 ******************************************************************************/
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
uint32_t CCM_GetVersion(void);

/**
 * @brief Can be called at any time to obtain capabilities of the driver interface.
 * @return Returns information about capabilities in this driver implementation.
 * @note
 */
const CCM_CAPABILITIES *CCM_GetCapabilities(void);

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
int32_t CCM_Initialize(CCM_SignalEvent_t cb_event);

/**
 * @brief De-initialize CCM Interface.
 * @return Returns Status Error Codes.
 * @note De-initializes the resources of CCM interface
 */
int32_t CCM_Uninitialize(void);

/**
 * @brief Control CCM Interface Power.
 * @param [in] state: Power state
 * @return Returns Status Error Codes.
 * @note The function CCM_PowerControl operates the power modes of the CCM interface.
 */
int32_t CCM_PowerControl(CCM_POWER_STATE state);

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
int32_t CCM_Control(uint32_t control, uint32_t arg);

/**
 * @brief Configure duty cycle in run time.
 * @param [in] CCM_id: Output channel to control with enum CCM_ID (max number is returned
 * by CCM_GetCapabilities()).
 * @param [in] param1:
 * CCM_MODE_PWM_ONE_THRESHOLD: The total period of the PWM output waveform (value in ns)
 * CCM_MODE_1BIT_DAC: Max_timer – the maximum value of the timer
 * CCM_MODE_CLK_OUT: Max_timer – the maximum value of the timer
 * @param [in] param2:
 * CCM_MODE_PWM_ONE_THRESHOLD: The active time of the PWM output (value in ns)
 * CCM_MODE_1BIT_DAC: Step – how many to add each clk to the timer counter
 * CCM_MODE_CLK_OUT: Step – how many to add each clk. The output frequency therefore is freq =
 * in_clk * (step/max).
 * @param [in] param3:
 * CM_MODE_PWM_ONE_THRESHOLD: PWM polarity (0 or 1)
 * CCM_MODE_1BIT_DAC: N/A
 * CCM_MODE_CLK_OUT: N/A
 * @return Returns Status Error Codes.
 */
int32_t CCM_ConfigOutputChannel(CCM_ID CCM_id, uint32_t param1, uint32_t param2, uint32_t param3);

/**
 * @brief This function enables/disables CCM Output channels
 * @param[in] channelMask: channelMask is a combination of enabled channels. Each bit
 * corresponds to a channel. Bit 0 is channel 0, bit 1 is channel 1...
 * @return None
 */
void CCM_EnableDisableOutputChannels(uint32_t channelMask);

/**
 * @brief This function queries the current status of a CCM channel.
 * @param [in] CCM_id: Output channel to control with enum CCM_ID (max number is returned
 * by CCM_GetCapabilities()).
 * @param [out] status: Queried status of the specified CCM channel.
 * @return Returns Status Error Codes.
 */
int32_t CCM_GetChannelStatus(CCM_ID CCM_id, CCM_CHANNEL_STATUS *status);
/** @} ccm_apis */
/** @} ccm_driver */
#endif /*_CCM_H_*/
