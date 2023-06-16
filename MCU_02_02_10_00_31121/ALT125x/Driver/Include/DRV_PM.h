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
 * @file DRV_PM.h
 */

#ifndef DRV_PM_H_
#define DRV_PM_H_

/**
 * @defgroup drv_pm PM (Power Management) Driver
 * @{
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "DRV_GPIO.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/**
 * @defgroup pm_constant PM Constants
 * @{
 */
/* Retention Section Id mask.*/
#define PWR_CON_RETENTION_SEC_1 (0x1) /**< Retention Section 1 (0xBC0E0000 – 0xBC0EFFFF).*/
#define PWR_CON_RETENTION_SEC_2 (0x2) /**< Retention Section 2 (0xBC0F0000 – 0xBC0FFFFF).*/
#ifdef ALT1255
#define PWR_CON_RETENTION_SEC_MASK_ALL                                                \
  PWR_CON_RETENTION_SEC_2 /**< Retention Section ID mask = (PWR_CON_RETENTION_SEC_1 | \
                             PWR_CON_RETENTION_SEC_2...) */
#else
#define PWR_CON_RETENTION_SEC_MASK_ALL \
  (0x3) /**< Retention Section ID mask = (PWR_CON_RETENTION_SEC_1 | PWR_CON_RETENTION_SEC_2...) */
#endif
/** @} pm_constant */

/****************************************************************************
 * Public Types
 ****************************************************************************/
/**
 * @defgroup pm_types PM Types
 * @{
 */
/**
 * @brief Definition of the error code of PM APIs.
 */
typedef enum {
  DRV_PM_OK,       /**< 0: No error. */
  DRV_PM_GEN_ERROR /**< 1: Other errors. */
} DRV_PM_Status;

/** @brief Power Modes. Please notice that only STOP/STANDBY/SHUTDOWN can be configured by
 * application. */
typedef enum {
  DRV_PM_MODE_RUN,     /**< \internal 0: Active mode. Internally use only.*/
  DRV_PM_MODE_SLEEP,   /**< \internal 1: OS sleep mode. Internally use only. */
  DRV_PM_MODE_STOP,    /**< 2: Stop mode. */
  DRV_PM_MODE_STANDBY, /**< 3: Standby mode. */
  DRV_PM_MODE_SHUTDOWN /**< 4: Shutdown mode. */
} DRV_PM_PwrMode;

/** @brief Power Wakeup PIN Mode/Polarity.*/
typedef enum {
  DRV_PM_WAKEUP_EDGE_RISING,         /**< 0: Edge Rising.*/
  DRV_PM_WAKEUP_EDGE_FALLING,        /**< 1: Edge Falling.*/
  DRV_PM_WAKEUP_EDGE_RISING_FALLING, /**< 2: Edge Rising or Falling.*/
  DRV_PM_WAKEUP_LEVEL_POL_HIGH,      /**< 3: Level Polarity High.*/
  DRV_PM_WAKEUP_LEVEL_POL_LOW        /**< 4: Level Polarity Low.*/
} DRV_PM_WAKEUP_PinPolarity;

/**
 * @brief Definition of parameters for Stop mode configuration structure.
 */
typedef struct {
  uint32_t duration; /**< Duration of stop mode. Default value is 0(infinite). unit: millisecond.*/
} DRV_PM_StopModeConfig;

/**
 * @brief Definition of parameters for Standby mode configuration structure.
 */
typedef struct {
  uint32_t
      duration; /**< Duration of standby mode. Default value is 0(infinite). unit: millisecond.*/
  uint32_t memRetentionSecIdList; /**< Section ID List for Memory Retention.*/
} DRV_PM_StandbyModeConfig;

/**
 * @brief Definition of parameters for Shutdown mode configuration structure.
 */
typedef struct {
  uint32_t
      duration; /**< Duration of shutdown mode. Default value is 0(infinite). unit: millisecond.*/
} DRV_PM_ShutdownModeConfig;

/**
 * @brief Definition of the cause of the last MCU wake up.
 */
typedef enum {
  DRV_PM_WAKEUP_CAUSE_NONE = 0, /**< 0: N/A */
  DRV_PM_WAKEUP_CAUSE_TIMEOUT,  /**< 1: Sleep duration timeout.*/
  DRV_PM_WAKEUP_CAUSE_IO_ISR,   /**< 2: IO ISR.*/
  DRV_PM_WAKEUP_CAUSE_MODEM,    /**< 3: Modem.*/
  DRV_PM_WAKEUP_CAUSE_UNKNOWN =
      0xffffffff /**< 0xFFFFFFFF: Unknown. */  // force the enum to 4 bytes
} DRV_PM_WakeUpCause;

/**
 * @brief Definition of device boot type.
 */
typedef enum {
  DRV_PM_DEV_COLD_BOOT = 0, /**< 0: Device cold boot. */
  DRV_PM_DEV_WARM_BOOT      /**< 1: Device warm boot. */
} DRV_PM_BootType;

/**
 * @brief Definition of MCU wake up state. only available with specific retention memory.
 */
typedef enum {
  DRV_PM_WAKEUP_STATELESS = 0, /**< 0: Data in specific retention memory area is lost. */
  DRV_PM_WAKEUP_STATEFUL       /**< 1: Data in specific retention memory area is kept. */
} DRV_PM_SpecMemRetentionState;

/**
 * @brief Definition of the information of the last MCU wake up and related statistics.
 */
typedef struct {
  DRV_PM_BootType boot_type;                 /**< device boot type */
  DRV_PM_SpecMemRetentionState wakeup_state; /**< wake up state */
  DRV_PM_WakeUpCause last_cause;             /**< wakeup cause */
  uint32_t last_dur_left; /**< left sleep duration. in milliseconds, non zero when waking-up before
                             requested sleep duration elapsed */
  uint32_t
      counter; /**< how many times MCU wake up from STOP mode. Now it only applies to STOP mode. */
} DRV_PM_Statistics;

/**
 * @brief Definition of the configuration for MCU wake up.
 */
typedef struct {
  MCU_PinId pin_set;                 /**< GPIO pin set*/
  DRV_PM_WAKEUP_PinPolarity pin_pol; /**< Wakeup pin mode/polarity */
} DRV_PM_WakeupConfig;

/** @} pm_types */

/****************************************************************************
 * Public APIs
 ****************************************************************************/
/*! @cond Doxygen_Suppress */
#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif
/*! @endcond */

/**
 * @defgroup pm_funcs PM Core APIs
 * @{
 */

/**
 * @brief Intialize Power Management module.
 *
 * @return DRV_PM_Status is returned.
 */
DRV_PM_Status DRV_PM_Initialize(void);

/**
 * @brief Driver layer process before entering sleep.
 *
 * @param [in] mode: sleep mode.
 *
 * @return DRV_PM_Status is returned.
 */
DRV_PM_Status DRV_PM_PreSleepProcess(DRV_PM_PwrMode mode);

/**
 * @brief Driver process after leaving sleep.
 *
 * @param [in] mode: sleep mode.
 *
 * @return DRV_PM_Status is returned.
 */
DRV_PM_Status DRV_PM_PostSleepProcess(DRV_PM_PwrMode mode);

/**
 * @brief Configure Power mode to Stop mode.
 *
 * @param [in] *pwr_stop_param: pointer of configured parameters.
 *
 * @return DRV_PM_Status is returned.
 */
DRV_PM_Status DRV_PM_EnterStopMode(DRV_PM_StopModeConfig *pwr_stop_param);

/**
 * @brief Configure Power mode to Standby mode.
 *
 * @param [in] *pwr_standby_param: pointer of configured parameters.
 *
 * @return DRV_PM_Status is returned.
 */
DRV_PM_Status DRV_PM_EnterStandbyMode(DRV_PM_StandbyModeConfig *pwr_standby_param);

/**
 * @brief Configure Power mode to Shutdown mode.
 *
 * @param [in] *pwr_shutdown_param: pointer of configured parameters.
 *
 * @return DRV_PM_Status is returned.
 */
DRV_PM_Status DRV_PM_EnterShutdownMode(DRV_PM_ShutdownModeConfig *pwr_shutdown_param);

/**
 * @brief Configure/Enable Wakeup PIN.
 *
 * @param [in] pinNum: GPIO PIN number.
 * @param [in] polarity: see enumeration DRV_PM_WAKEUP_PinPolarity
 *
 * @return DRV_PM_Status is returned.
 */
DRV_PM_Status DRV_PM_EnableWakeUpPin(uint32_t pinNum, DRV_PM_WAKEUP_PinPolarity polarity);

/**
 * @brief Disable Wakeup pin.
 *
 * @param [in] pinNum: GPIO pin number.
 *
 * @return DRV_PM_Status is returned.
 */
DRV_PM_Status DRV_PM_DisableWakeUpPin(uint32_t pinNum);

/**
 * @brief Get sleep statistics of MCU.
 *
 * @param [out] *pwr_stat: pointer of DRV_PM_Statistics.
 *
 * @return error code. 0-success; other-fail
 */
int32_t DRV_PM_GetStatistics(DRV_PM_Statistics *pwr_stat);

/**
 * @brief Clear sleep statistics of MCU.
 */
void DRV_PM_ClearStatistics(void);

/**
 * @brief This function would get device boot type.
 *
 * @return DRV_PM_BootType: enumeration value of device boot type.
 * @note May not recognize the boot is triggered by reset button.
 */
DRV_PM_BootType DRV_PM_GetDevBootType(void);

/**
 * @brief Mask interrupts except for atomic-counter interrupt used for wake up.
 * It is used for disabled interrupts before entering sleep.
 *
 * @param [in] mode: sleep mode.
 *
 * @return DRV_PM_Status is returned.
 */
void DRV_PM_MaskInterrupts(DRV_PM_PwrMode mode);

/**
 * @brief Restore those interrupts were disabled for entering to sleep.
 * It is used for restore interrupts after sleep.
 *
 * @return None.
 */
void DRV_PM_RestoreInterrupts(DRV_PM_PwrMode mode);

/**
 * @brief This function would handle gated clocks IO before entering or after leaving sleep.
 *
 * @param enter_sleep: bool enter_sleep(true); leave_sleep(false).
 *
 * @return None.
 */
void DRV_PM_HandleClockGating(bool enter_sleep);

/**
 * @brief Get whether boot was stateful early in the boot
 *     process, to allow deciding if the code should restore state
 *     from GPM
 *
 * @return DRV_PM_WAKEUP_STATEFUL if state is kept in GPM, otherwise
 *      return DRV_PM_WAKEUP_STATELESS.
 */
DRV_PM_SpecMemRetentionState DRV_PM_EarlyBootProbe(void);

/**
 *  @brief This function restores register system state after waking
 *  from standby mode.
 *
 *  @return Does not return.
 */
__attribute__((noreturn)) void DRV_PM_Resume(void);

/**
 * @brief This function would probe device boot type (from PMP).
 *
 * @return DRV_PM_BootType: enumeration value of device boot type.
 * @note This API is only working with new RK/CP releases. Please refer to MCU release note.
 */
DRV_PM_BootType DRV_PM_EarlyBootTypeProbe(void);

/** @} pm_funcs */
/*! @cond Doxygen_Suppress */
#undef EXTERN
#ifdef __cplusplus
}
#endif
/*! @endcond */
/** @} drv_pm */
#endif /* DRV_PM_H_ */
