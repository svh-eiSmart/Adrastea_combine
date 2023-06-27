/****************************************************************************
 *
 *  (c) copyright 2020 Altair Semiconductor, Ltd. All rights reserved.
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

#ifndef ALTCOM_INCLUDE_API_COAP_APICMDHDLR_CMDURCEVT_H_
#define ALTCOM_INCLUDE_API_COAP_APICMDHDLR_CMDURCEVT_H_

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "evthdl_if.h"

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: apicmdhdlr_cmdevt
 *
 * Description:
 *   This function is an API command handler for event report.
 *
 * Input Parameters:
 *  evt    Pointer to received event.
 *  evlen  Length of received event.
 *
 * Returned Value:
 *   If the API command ID matches APICMDID_REPORT_EVT,
 *   EVTHDLRC_STARTHANDLE is returned.
 *   Otherwise it returns EVTHDLRC_UNSUPPORTEDEVENT. If an internal error is
 *   detected, EVTHDLRC_INTERNALERROR is returned.
 *
 ****************************************************************************/

enum evthdlrc_e apicmdhdlr_cmdevt(FAR uint8_t *evt, uint32_t evlen);
enum evthdlrc_e apirsthdlr_cmdevt(FAR uint8_t *evt, uint32_t evlen);
enum evthdlrc_e apitermhdlr_cmdevt(FAR uint8_t *evt, uint32_t evlen);

#endif /* ALTCOM_INCLUDE_API_COAP_APICMDHDLR_CMDURCEVT_H_ */
