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

#ifndef HIFC_OSAL_OPT_H_
#define HIFC_OSAL_OPT_H_

#ifndef HIFC_EPERM
#define HIFC_EPERM 1 /* Operation not permitted */
#endif

#ifndef HIFC_EIO
#define HIFC_EIO 5 /* I/O error */
#endif

#ifndef HIFC_ENOMEM
#define HIFC_ENOMEM 12 /* Out of memory */
#endif

#ifndef HIFC_EBUSY
#define HIFC_EBUSY 16 /* Device or resource busy */
#endif

#ifndef HIFC_EINVAL
#define HIFC_EINVAL 22 /* Invalid argument */
#endif

#ifndef HIFC_ENOSPC
#define HIFC_ENOSPC 28 /* No space left on device */
#endif

#ifndef HIFC_ETIME
#define HIFC_ETIME 62 /* Timer expired */
#endif

#ifndef HIFC_ECONNABORTED
#define HIFC_ECONNABORTED 113
#endif

#ifndef HIFC_ETIMEDOUT
#define HIFC_ETIMEDOUT 116
#endif

#ifndef HIFC_EPROTO
#define HIFC_EPROTO 71
#endif

#ifndef HIFC_EALRDYDONE
#define HIFC_EALRDYDONE 125 /* Already done */
#endif

#ifndef HIFC_EWONTDO
#define HIFC_EWONTDO 126 /* Won't do */
#endif

#endif /* OSAL_OPENRTOS_OSAL_OPT_H_ */
