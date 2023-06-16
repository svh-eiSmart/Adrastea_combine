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
 * @file DRV_PMP_AGENT.h
 */

#ifndef DRV_PMP_AGENT_H_
#define DRV_PMP_AGENT_H_

#include "DRV_PMP_AGENT_MSG.h"
/****************************************************************************
 * Function Prototypes
 ****************************************************************************/
#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

/*-----------------------------------------------------------------------------
 * int32_t DRV_PMP_AGENT_SendMessage(DRV_PMP_MCU2PMP_Message *out_msg)
 * PURPOSE: This function would send the message to PMP.
 * PARAMs:
 *      INPUT:  pointer of output data.
 *      OUTPUT: None.
 * RETURN:  0:success ; others: error code.
 *-----------------------------------------------------------------------------
 */
int32_t DRV_PMP_AGENT_SendMessage(DRV_PMP_MCU2PMP_Message *out_msg);
/*-----------------------------------------------------------------------------
 * int32_t DRV_PMP_AGENT_SendMessageByIdleTask(DRV_PMP_MCU2PMP_Message *out_msg)
 * PURPOSE: This function would send the message to PMP.
 * PARAMs:
 *      INPUT:  pointer of output data.
 *      OUTPUT: None.
 * RETURN:  0:success ; others: error code.
 * Note: only used for sleep requests.
 *-----------------------------------------------------------------------------
 */
int32_t DRV_PMP_AGENT_SendMessageByIdleTask(DRV_PMP_MCU2PMP_Message *out_msg);
/*-----------------------------------------------------------------------------
 * int32_t DRV_PMP_AGENT_Initialize(void)
 * PURPOSE: This function would initialize pmpMbox and related funtions.
 * PARAMs:
 *      INPUT:  None.
 *      OUTPUT: None.
 * RETURN:  0:success ; others: error code.
 *-----------------------------------------------------------------------------
 */
int32_t DRV_PMP_AGENT_Initialize(void);

/*-----------------------------------------------------------------------------
 * void DRV_PMP_AGENT_GetWakeInfo(void)
 * PURPOSE: This function would get wake up data .
 * PARAMs:
 *      INPUT:  None.
 *      OUTPUT: *cause, *time_left, *count.
 * RETURN:  None
 *-----------------------------------------------------------------------------
 */
void DRV_PMP_AGENT_GetWakeInfo(uint32_t *cause, uint32_t *time_left, uint32_t *count);

/*-----------------------------------------------------------------------------
 * int32_t DRV_PMP_AGENT_GetTemperature(void)
 * PURPOSE: This function would get the temperature.
 * PARAMs:
 *      INPUT:  None.
 * RETURN:  Temperature.
 *-----------------------------------------------------------------------------
 */
int32_t DRV_PMP_AGENT_GetTemperature(void);
#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* DRV_PMP_AGENT_H_ */
