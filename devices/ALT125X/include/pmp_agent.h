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
 * @file pmp_agent.h
 */

#ifndef _PMP_AGENT_DRV_H
#define _PMP_AGENT_DRV_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

/*-----------------------------------------------------------------------------
 * int send_msg_to_pmp(mcu2pmp_mbox_msg_t *out_msg)
 * PURPOSE: This function would send the message to PMP.
 * PARAMs:
 *      INPUT:  pointer of output data.
 *      OUTPUT: None.
 * RETURN:  0:success ; others: error code.
 *-----------------------------------------------------------------------------
 */
int send_msg_to_pmp(mcu2pmp_mbox_msg_t *out_msg);
/*-----------------------------------------------------------------------------
 * int idle_task_send_msg_to_pmp(mcu2pmp_mbox_msg_t *out_msg)
 * PURPOSE: This function would send the message to PMP.
 * PARAMs:
 *      INPUT:  pointer of output data.
 *      OUTPUT: None.
 * RETURN:  0:success ; others: error code.
 * Note: only used for sleep requests.
 *-----------------------------------------------------------------------------
 */
int idle_task_send_msg_to_pmp(mcu2pmp_mbox_msg_t *out_msg);
/*-----------------------------------------------------------------------------
 * int pmpAgent_init(void)
 * PURPOSE: This function would initialize pmpMbox and related funtions.
 * PARAMs:
 *      INPUT:  None.
 *      OUTPUT: None.
 * RETURN:  0:success ; others: error code.
 *-----------------------------------------------------------------------------
 */
int pmpAgent_init(void);
#endif
