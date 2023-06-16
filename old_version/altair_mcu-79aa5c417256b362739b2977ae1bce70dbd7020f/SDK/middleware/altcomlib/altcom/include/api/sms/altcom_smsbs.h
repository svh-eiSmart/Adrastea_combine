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

#ifndef __MODULES_LTE_ALTCOM_INCLUDE_API_SMS_ALTCOM_SMSBS_H
#define __MODULES_LTE_ALTCOM_INCLUDE_API_SMS_ALTCOM_SMSBS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "buffpoolwrapper.h"
#include "apiutil.h"

#include "uconv.h"
#include "altcom_sms.h"
#include "apicmd_sms.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ALTCOM_SMSBS_STATUS_INITIALIZED (0)
#define ALTCOM_SMSBS_STATUS_UNINITIALIZED (1)

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: apicmdhdlr_sms_report_recv
 *
 * Description:
 *   This function is an API command handler for sms report received .
 *
 * Input Parameters:
 *  evt    Pointer to received event.
 *  evlen  Length of received event.
 *
 * Returned Value:
 *   If the API command ID matches APICMDID_SMS_REPORT_RECV,
 *   EVTHDLRC_STARTHANDLE is returned.
 *   Otherwise it returns EVTHDLRC_UNSUPPORTEDEVENT. If an internal error is
 *   detected, EVTHDLRC_INTERNALERROR is returned.
 *
 ****************************************************************************/

enum evthdlrc_e apicmdhdlr_sms_report_recv(FAR uint8_t *evt, uint32_t evlen);

void altcom_smsbs_set_status(int status, bool storage_use);

void altcom_smsbs_get_status(int *status, bool *storage_use);

int altcom_smsbs_check_initialize_status(void);


void altcom_sms_set_time(struct altcom_apicmd_sms_time_s *input, struct altcom_sms_time_s *output);

int altcom_sms_set_addr(struct altcom_apicmd_sms_addr_s *input, struct altcom_sms_addr_s *output);

int altcom_sms_set_apicmd_addr(struct altcom_sms_addr_s *input,
                               struct altcom_apicmd_sms_addr_s *output);

int32_t report_recv_status_chg_cb(int32_t new_stat, int32_t old_stat);

#endif /* __MODULES_LTE_ALTCOM_INCLUDE_API_SMS_ALTCOM_SMSBS_H */
