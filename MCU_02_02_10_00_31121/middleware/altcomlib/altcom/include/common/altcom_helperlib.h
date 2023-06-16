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

#ifndef __MODULES_ALTCOM_INCLUDE_COMMON_ALTCOM_HELPERLIB_H
#define __MODULES_ALTCOM_INCLUDE_COMMON_ALTCOM_HELPERLIB_H

/**
 * @file altcom_helperlib.h
 */

/**
 * @defgroup altcom_helperlib ALTCOM Helper Library APIs
 * @{
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/**
 * @defgroup herlperlib_constants ALTCOM Helper Library Constant
 * @{
 */

/** @} herlperlib_constant */

/****************************************************************************
 * Public Types
 ****************************************************************************/

/**
 * @defgroup herlperlib_types ALTCOM Helper Library Types
 * @{
 */

/** @} herlperlib_types */

/*! @cond Doxygen_Suppress */
#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif
/*! @endcond */

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/**
 * @defgroup helperlib_func ALTCOM Helper Library APIs
 * @{
 */

/**
 * @brief To convert a dotted IPv6 string into colon separated format.
 *
 * @param [in] src: Source string in dotted format.
 * @param [out] dst: Destination string in colon separated format.
 * @return 0 returned on succeeded , and -1 returned on failure.
 */

int altcom_helperlib_conv_dot2colon_v6(const char *src, char *dst);

/** @} helperlib_func */

#undef EXTERN
#ifdef __cplusplus
}
#endif
/** @} altcom_helperlib */

#endif /* __MODULES_ALTCOM_INCLUDE_COMMON_ALTCOM_HELPERLIB_H */
