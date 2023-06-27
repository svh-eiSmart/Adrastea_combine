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

#ifndef CORE_CORE_UTILS_SERIALMNGR_LOSMUX_H_
#define CORE_CORE_UTILS_SERIALMNGR_LOSMUX_H_
#include "los_mux_types.h"
#define VIRTUAL_SERIAL_COUNT 4
#define MAX_MUX_COUNT 1

// return port handler
long losGetTimeInMilliseconds();
void *losBindToPortApi(int logicalPortNum, muxRxFp_t serialRxProcessFp, void *appCookie,
                       muxTxBuffFp_t *serialTxcharFp);
int losMuxCreateMutex(void *mutex);
void losMuxMutexTake(void *mutex);
void losMuxMutexGive(void *mutex);
int losMuxDeleteMutex(void *mutex);
int losMuxCreateEventGroup(void *eg);
losMuxEvBits_t losMuxEventGroupWait(void *eg, losMuxEvBits_t BitsToWaitFor, uint32_t timeoutMS);
void losMuxEventGroupSetBitsISR(void *eg, losMuxEvBits_t evBits);
void losMuxEventGroupSetBits(void *eg, losMuxEvBits_t evBits);
void losMuxEventGroupClearBits(void *eg, losMuxEvBits_t evBits);
int losMuxDeleteEventGroup(void *eg);
int losMuxCreateTask(losMuxTaskFunction_t taskFunction, const char *const pcName,
                     uint32_t stackSize, void *const pvParameters, uint16_t taskPrio);

#endif /* CORE_CORE_UTILS_SERIALMNGR_LOSMUX_H_ */
