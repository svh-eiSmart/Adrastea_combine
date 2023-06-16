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
 * @file DRV_IOSEL.h
 */
#ifndef DRV_IOSEL_H_
#define DRV_IOSEL_H_
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "PIN.h"
#include <stdint.h>
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/*! @cond Doxygen_Suppress */
#define IOSEL_REGISTER_OFFSET_UNDEFINED (0xFFFF)
#define IOSEL_CFG_TB_SIZE ALT1250_GPIO_MAX
/*! @endcond */
/****************************************************************************
 * Data types
 ****************************************************************************/
/**
 * @defgroup iosel IO Select
 * @{
 */
/**
 * @defgroup iosel_data_types IO Select Types
 * @{
 */
/**
 * @typedef IOSEL_RegisterOffset
 * Definition of IO MUX register offset type
 */
typedef uint16_t IOSEL_RegisterOffset;
/** @} iosel_data_types  */

/*******************************************************************************
 * API
 ******************************************************************************/
#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif
/**
 * @defgroup iosel_apis IOSEL APis
 * @{
 */
/**
 * @brief Get register offset to be used to access io_func_sel register of the
 * specified pin
 *
 * @param [in] pin_id: IO pin id with type MCU_PinId.
 * @return Register offset with type @ref IOSEL_RegisterOffset
 */
IOSEL_RegisterOffset DRV_IOSEL_GetRegisterOffset(MCU_PinId pin_id);
/**
 * @brief IOSEL presleep handler to retain mux value
 */
void DRV_IOSEL_PreSleepProcess(void);
/**
 * @brief IOSEL postsleep handler to restore mux value
 */
void DRV_IOSEL_PostSleepProcess(void);

/** @} iosel_apis */
#undef EXTERN
#ifdef __cplusplus
}
#endif
/** @} iosel */
#endif /*DRV_IOSEL_H_*/
