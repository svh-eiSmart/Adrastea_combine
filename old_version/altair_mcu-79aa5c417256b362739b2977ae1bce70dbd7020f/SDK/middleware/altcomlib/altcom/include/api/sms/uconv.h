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

#ifndef __MODULES_INCLUDE_UTILS_UCONV_H
#define __MODULES_INCLUDE_UTILS_UCONV_H

/**
 * @defgroup uconv unicode converter API
 * @brief Unicode converter library
 *
 * @{
 * @file  uconv.h
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/** @name Functions */
/** @{ */

/**
 * Convert a string from UTF-8 format to UCS2.
 *
 * @param [in] src_size: Byte length of string encoded in UTF-8.
 *
 * @param [in] src: String pointer encoded in UTF-8.
 *
 * @param [in] dst_size: Maximum Byte length of UCS2 string buffer.
 *
 * @param [out] dst: Pointer stores the character string
 *                   converted to UCS2 format.
 *
 * @return On success, it returns the number of bytes
 *         in the converted string buffer.
 *         Otherwise negative value is returned according to <errno.h>.
 */

int uconv_utf8_to_ucs2(int src_size, uint8_t *src,
                       int dst_size, uint16_t *dst);

/**
 * Convert a string from UCS2 format to UTF-8.
 *
 * @param [in] src_size: Byte length of string encoded in UCS2.
 *
 * @param [in]  src: String pointer encoded in UCS2.
 *
 * @param [in] dst_size: Maximum Byte length of UTF-8 string buffer.
 *
 * @param [out] dst: Pointer stores the character string
 *                   converted to UTF-8 format.
 *
 * @return On success, it returns the number of bytes
 *         in the converted string buffer.
 *         Otherwise negative value is returned according to <errno.h>.
 *
 */

int uconv_ucs2_to_utf8(int src_size, uint16_t *src,
                       int dst_size, uint8_t *dst);

/** @} */

/** @} */

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __MODULES_INCLUDE_UTILS_UCONV_H */
