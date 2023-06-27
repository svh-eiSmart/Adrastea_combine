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
/**
 * @file io_mngr.h
 */
#ifndef IOMNGR_H
#define IOMNGR_H
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "iosel.h"
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/****************************************************************************
 * Data types
 ****************************************************************************/
/**
 * @defgroup io_mngr IO Manager
 * @{
 */
/**
 * @defgroup io_mngr_data_types IO Manager Types
 * @{
 */
/**
 * @typedef eIoMngrRet
 * Definition of IO manager API return codes
 */
typedef enum {
  IOMNGR_SUCCESS,    /**< API returns with no error */
  IOMNGR_IO_PAR_ERR, /**< API returns with no io_par setting error */
  IOMNGR_FAILED      /**< API returns with generic error */
} eIoMngrRet;
/** @} io_mngr_data_types  */
/*******************************************************************************
 * API
 ******************************************************************************/
/**
 * @defgroup io_mngr_apis IO Manager APIs
 * @{
 */
/**
 * @brief Configure IO mux setting.
 *
 * @param [in] pin_id: IO pin id with type eMcuPinIds.
 * @param [in] io_func: Mux function ID with type @ref eIoMngrFunc.
 * @return @ref eIoMngrRet.
 */
eIoMngrRet io_mngr_iosel_set(eMcuPinIds pin_id, eIoMngrFunc io_func);
/**
 * @brief Get current IO mux setting.
 *
 * @param [in] pin_id: IO pin id with type eMcuPinIds.
 *
 * @return Current IO mux function ID with type @ref eIoMngrFunc.
 */
eIoMngrFunc io_mngr_iosel_get(eMcuPinIds pin_id);
/**
 * @brief Validate IO partiton ownership was configured to MCU.
 *
 * @param [in] pin_id: IO pin id with type eMcuPinIds.
 *
 * @return @ref eIoMngrRet.
 */
eIoMngrRet io_mngr_iopar_validate(eMcuPinIds pin_id);
/**
 * @brief Get the register offset to the BASE_ADDRESS_MCU_IO_FUNC_SEL.
 *
 * @param [in] pin_id: IO pin id with type eMcuPinIds.
 *
 * @return register offset.
 */
io_mngr_reg_ofs_t io_mngr_get_iosel_reg_ofs(eMcuPinIds pin_id);
/** @} io_mngr_apis */
/** @} io_mngr */
#endif
