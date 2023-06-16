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

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <string.h>
#include <stdbool.h>

#include "alt_osal.h"
#include "dbg_if.h"
#include "aws/altcom_aws.h"
#include "util/apiutil.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/
extern bool gAwsInit;
extern MQTTSession_t *gAwsSession;
extern awsMsgCbFunc_t gAwsCallback;
extern awsConnEvtCbFunc_t gAwsEvtCallback;
extern void *gAwsUserPriv;

/****************************************************************************
 * External Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

AWSError_e altcom_awsUninitialize(void) {
  /* Check init */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return AWS_FAILURE;
  }

  if (!gAwsInit) {
    return AWS_FAILURE;
  }

  if (gAwsSession) {
    altcom_mqttDisconnect(gAwsSession);
    vTaskDelay(1000);
    altcom_mqttSessionDelete(gAwsSession);
    gAwsSession = NULL;
    gAwsCallback = NULL;
    gAwsEvtCallback = NULL;
    gAwsUserPriv = NULL;
  }

  return AWS_SUCCESS;
}
