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

#ifndef MEMIF_IMPL_H
#define MEMIF_IMPL_H

#include <stdint.h>

/* CMSIS-RTOS2 includes */
#include "cmsis_os2.h"

/* Middleware includes */
#include "altcom.h"

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif
/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/
void *memif_impl_create(blockset_t set[], uint8_t setnum);
int32_t memif_impl_destroy(void *handle);
void *memif_impl_alloc(void *handle, uint32_t size);
int32_t memif_impl_free(void *handle, void *buf);
void memif_impl_show(void *handle);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* MEMIF_IMPL_H */