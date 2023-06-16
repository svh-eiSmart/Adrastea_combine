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
 * @file DRV_IO.h
 */
#ifndef DRV_IO_H_
#define DRV_IO_H_
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "PIN.h"
#include <stdint.h>
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/****************************************************************************
 * Data types
 ****************************************************************************/
/**
 * @defgroup drv_io IO Management Driver
 * @{
 */
/**
 * @defgroup drvio_data_types IO Management Types
 * @{
 */
/**
 * @typedef IO_Pull
 * Definition of IO internal pull value
 */
typedef enum {
  IO_PULL_NONE, /**< IO no pull */
  IO_PULL_UP,   /**< IO pull up*/
  IO_PULL_DOWN  /**< IO pull down*/
} IO_Pull;
/**
 * @typedef IO_DriveStrength
 * Definition of IO driving strength value
 */
typedef enum {
  IO_DRIVE_STRENGTH_2MA, /**< IO driving strength 2mA */
  IO_DRIVE_STRENGTH_4MA, /**< IO driving strength 4mA */
  IO_DRIVE_STRENGTH_8MA, /**< IO driving strength 8mA */
  IO_DRIVE_STRENGTH_12MA /**< IO driving strength 12mA */
} IO_DriveStrength;
/**
 * @typedef IO_SlewRate
 * Definition of IO slew rate settings.
 */
typedef enum {
  IO_SLEW_RATE_FAST, /**< IO fast slew rate */
  IO_SLEW_RATE_SLOW  /**< IO slow slew rate */
} IO_SlewRate;
/**
 * @typedef DRV_IO_Status
 * Definition of DRV_IO retrun codes
 */
typedef enum {
  DRV_IO_OK,               /**< API returns with no error */
  DRV_IO_ERROR_PARTITION,  /**< API returns with io_par setting error */
  DRV_IO_ERROR_PARAM,      /**< API returns with parameter error */
  DRV_IO_ERROR_UNSUPPORTED /**< API returns with not support error */
} DRV_IO_Status;
/** @} drv_io_data_types  */
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
 * @defgroup drvio_apis IO Management APIs
 * @{
 */
/**
 * @brief Configure IO pull value.
 *
 * @param [in] pin_id: IO pin id with type MCU_PinId.
 * @param [in] pull_mode: Specify with type IO_Pull
 * @return @ref DRV_IO_Status.
 */
DRV_IO_Status DRV_IO_SetPull(MCU_PinId pin_id, IO_Pull pull_mode);
/**
 * @brief Get current IO pull value.
 *
 * @param [in] pin_id: IO pin id with type MCU_PinId.
 * @param [out] pull_mode: Current pull mode setting.
 * @return @ref DRV_IO_Status.
 */
DRV_IO_Status DRV_IO_GetPull(MCU_PinId pin_id, IO_Pull *pull_mode);
/**
 * @brief Configure IO driving strength
 *
 * @param [in] pin_id: IO pin id with type MCU_PinId.
 * @param [in] ds: Specify driving strength with type IO_DrivingStrength
 * @return @ref DRV_IO_Status.
 */
DRV_IO_Status DRV_IO_SetDriveStrength(MCU_PinId pin_id, IO_DriveStrength ds);
/**
 * @brief Get current IO driving strength
 *
 * @param [in] pin_id: IO pin id with type MCU_PinId.
 * @param [out] ds: Current driving strength setting.
 * @return @ref DRV_IO_Status.
 */
DRV_IO_Status DRV_IO_GetDriveStrength(MCU_PinId pin_id, IO_DriveStrength *ds);
/**
 * @brief Configure IO slew rate.
 *
 * @param [in] pin_id: IO pin id with type MCU_PinId.
 * @param [in] slew_rate: Specify with type IO_SlewRate
 * @return @ref DRV_IO_Status.
 */
DRV_IO_Status DRV_IO_SetSlewRate(MCU_PinId pin_id, IO_SlewRate slew_rate);
/**
 * @brief Get current IO slew rate.
 *
 * @param [in] pin_id: IO pin id with type MCU_PinId.
 * @param [out] slew_rate: Specify with type IO_SlewRate
 * @return @ref DRV_IO_Status.
 */
DRV_IO_Status DRV_IO_GetSlewRate(MCU_PinId pin_id, IO_SlewRate *slew_rate);
/**
 * @brief Get current mux value in IO_FUNC_SEL register.
 *
 * @param [in] pin_id: IO pin id with type MCU_PinId.
 * @param [out] mux_value: Current mux value in IO_FUNC_SEL register.
 * @return @ref DRV_IO_Status.
 */
DRV_IO_Status DRV_IO_GetMux(MCU_PinId pin_id, uint32_t *mux_value);
/**
 * @brief Configure IO mux value in IO_FUNC_SEL register.
 *
 * @param [in] pin_id: IO pin id with type MCU_PinId.
 * @param [in] mux_value: Value to be set to IO_FUNC_SEL register
 * @return @ref DRV_IO_Status.
 */
DRV_IO_Status DRV_IO_SetMux(MCU_PinId pin_id, uint32_t mux_value);
/**
 * @brief Validate IO partiton ownership was configured to MCU.
 *
 * @param [in] pin_id: IO pin id with type MCU_PinId.
 *
 * @return @ref DRV_IO_Status.
 */
DRV_IO_Status DRV_IO_ValidatePartition(MCU_PinId pin_id);
/**
 * @brief Enable/Disable input.
 *
 * @param [in] pin_id: IO pin id with type MCU_PinId.
 * @param [in] enable: 0:disable, 1: enable.
 *
 * @return @ref DRV_IO_Status.
 */
DRV_IO_Status DRV_IO_SetInputEnable(MCU_PinId pin_id, int32_t enable);
/**
 * @brief Add new IO pin to the sleep-parking list.
 *
 * @param [in] pin_id: IO pin id with type MCU_PinId.
 *
 * @return @ref DRV_IO_Status.
 *
 * @note IO 61 and 62 are not real IO so they can't be configured.
 */
DRV_IO_Status DRV_IO_AddSleepPin(MCU_PinId pin_id);
/**
 * @brief Remove the IO pin from the sleep-parking list.
 *
 * @param [in] pin_id: IO pin id with type MCU_PinId.
 *
 * @param [in] action: 0: set to default; 1: force to remove the from sleep list.
 *
 * @return @ref DRV_IO_Status.
 *
 * @note IO 61 and 62 are not real IO so they can't be configured.
 */
DRV_IO_Status DRV_IO_DelSleepPin(MCU_PinId pin_id, uint8_t action);
/**
 * @brief Pre-sleep process for IO.
 * @return 0 - OK; otherwise: error.
 */
int32_t DRV_IO_PreSleepProcess(void);
/**
 * @brief Post-sleep process for IO.
 * @return 0 - OK; otherwise: error.
 */
int32_t DRV_IO_PostSleepProcess(void);
/**
 * @brief Dump IO sleep table.
 */
void DRV_IO_DumpSleepList(void);
/**
 * @brief Initialize IO sleep functions.
 * @return 0 - OK; otherwise: error.
 */
int32_t DRV_IO_SLEEP_Initialize(void);
/** @} drv_io_apis */
/*! @cond Doxygen_Suppress */
#undef EXTERN
#ifdef __cplusplus
}
#endif
/*! @endcond */
/** @} drv_io */
#endif /*DRV_IO_H_*/
