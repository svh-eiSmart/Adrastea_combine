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
 * @file DRV_OTP.h
 */

#ifndef DRV_OTP_H_
#define DRV_OTP_H_

/*Stardard Header */
#include <stdbool.h>
#include <stdint.h>

/**
 * @defgroup drv_otp OTP Driver
 * @{
 */

/****************************************************************************
 * Data Type
 ****************************************************************************/
/**
 * @defgroup drv_otp_types OTP Types
 * @{
 */

/**
 * @brief Enumeration of MCU OTP field id list.
 */
typedef enum {
  MCU_OEM_PARAM_1, /**< OEM Parameter 1. The field size is 128 Bytes */
  MCU_OEM_PARAM_2, /**< OEM Parameter 2. The field size is 64 Bytes */
  OTP_NULL_LAST    /**< Not a real field. used by the checking of end of enumeration. */
} DRV_OTP_FIELDS;

/**
 * @brief Enumeration of OTP return status.
 */
typedef enum {
  OTP_OK = 0,          /**< Operation succeeded */
  OTP_ACCESS_ERROR,    /**< Access error */
  OTP_ALREADY_LOCKED,  /**< OTP is already locked */
  OTP_ALREADY_WRITTEN, /**< OTP is already written */
  OTP_ILLEGAL_ORDER,   /**< Illegal order */
  OTP_ILLEGAL_VALUE,   /**< Illegal value */
  OTP_NOK,             /**< Operation is not succeeded */
  OTP_MEMORY_ERROR,    /**< Memory error */
  OTP_LOCKED,          /**< OTP field is locked */
  OTP_NOT_IMPLEMENTED  /**< OTP function is not implemented */
} DRV_OTP_STATUS;

/** @} drv_otp_types */

/****************************************************************************
 * Public Function Prototypes
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
 * @defgroup drv_otp_apis OTP APIs
 * @{
 */

/**
 * @brief Performs OTP write action.
 *
 * @param [in] index: index of OTP fields. see @ref DRV_OTP_FIELDS.
 * @param [in] buf: pointer of write data buffer.
 * @param [in] size: size in byte to be written.
 * @return DRV_OTP_STATUS returned on success, error status returned on failure, see @ref
 * DRV_OTP_STATUS.
 * @note This function will write to specified OTP field and lock it. That is, it can't be overwrote
 * after writing.
 */
DRV_OTP_STATUS DRV_OTP_Set(DRV_OTP_FIELDS index, uint32_t *buf, uint32_t size);

/**
 * @brief Performs OTP read action.
 *
 * @param [in] index: index of OTP fields. see @ref DRV_OTP_FIELDS.
 * @param [out] buf: pointer of read data buffer.
 * @param [in] size: size in byte to be read.
 * @return DRV_OTP_STATUS returned on success, error status returned on failure, see @ref
 * DRV_OTP_STATUS.
 */
DRV_OTP_STATUS DRV_OTP_Get(DRV_OTP_FIELDS index, uint32_t *buf, uint32_t size);

/** @} drv_otp_apis */
/*! @cond Doxygen_Suppress */
#undef EXTERN
#ifdef __cplusplus
}
#endif
/*! @endcond */

/** @} drv_otp */

#endif /* DRV_OTP_H_ */
