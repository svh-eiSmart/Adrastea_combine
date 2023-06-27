/*  ---------------------------------------------------------------------------

    (c) copyright 2018 Altair Semiconductor, Ltd. All rights reserved.

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

#ifndef CORE_INCLUDE_SERIALMNGR_LOSMUXTYPESOS_H_
#define CORE_INCLUDE_SERIALMNGR_LOSMUXTYPESOS_H_

#include "FreeRTOSConfig.h"
#include <FreeRTOS.h>
#include <event_groups.h>
#include <semphr.h>
#include <stream_buffer.h>

// define OS dependent types and functions for the use of MUX
#define MUX_0_ID 0
#define LOS_TIMEOUT_FOREVER portMAX_DELAY
#define losMuxMalloc(size) pvPortMalloc(size)
#define losMuxFree(ptr) vPortFree(ptr)

// typedef for OS dependent
typedef void (*muxRxFp_t)(const char charRecv, void *recvCookie);
typedef int (*muxTxBuffFp_t)(void *portHandle, const char *txCharBuf, unsigned short txCharBufLen);

typedef SemaphoreHandle_t losMuxSemaphoreHandler_t;
typedef EventBits_t losMuxEvBits_t;
typedef EventGroupHandle_t losMuxEventGroupHandle_t;
typedef BaseType_t losMuxBool_t;
typedef TaskFunction_t losMuxTaskFunction_t;
#endif /* CORE_INCLUDE_SERIALMNGR_LOSMUXTYPESOS_H_ */
