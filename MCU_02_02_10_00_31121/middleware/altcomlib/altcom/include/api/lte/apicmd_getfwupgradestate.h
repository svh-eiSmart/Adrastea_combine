/****************************************************************************
 * modules/lte/altcom/include/api/lte/apicmd_getfwupgradestate.h
 *
 *   Copyright 2018 Sony Semiconductor Solutions Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Sony Semiconductor Solutions Corporation nor
 *    the names of its contributors may be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef __MODULES_LTE_ALTCOM_INCLUDE_API_LTE_APICMD_GET_FW_UPGRADE_STATE_H
#define __MODULES_LTE_ALTCOM_INCLUDE_API_LTE_APICMD_GET_FW_UPGRADE_STATE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "apicmd.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define APICMD_GET_FW_UPGRADE_STATE_RES_OK (0)
#define APICMD_GET_FW_UPGRADE_STATE_RES_ERR (1)

#define APICMD_GET_FW_UPGRADE_STATE_RES_INITIAL_VALUE (0)
#define APICMD_GET_FW_UPGRADE_STATE_RES_SUCCESS (1)
#define APICMD_GET_FW_UPGRADE_STATE_RES_NOT_ENOUGH_FLASH_MEMORY_DURING_DOWNLOAD (2)
#define APICMD_GET_FW_UPGRADE_STATE_RES_OUT_OF_RAM_DURING_DOWNLOAD (3)
#define APICMD_GET_FW_UPGRADE_STATE_RES_CONNECTION_LOST_DURING_DOWNLOAD (4)
#define APICMD_GET_FW_UPGRADE_STATE_RES_INTEGRITY_CHECK_FAILURE (5)
#define APICMD_GET_FW_UPGRADE_STATE_RES_UNSUPPORTED_PACKAGE_TYPE (6)
#define APICMD_GET_FW_UPGRADE_STATE_RES_INVALID_URI (7)
#define APICMD_GET_FW_UPGRADE_STATE_RES_IMAGE_UPDATE_FAILED (8)
#define APICMD_GET_FW_UPGRADE_STATE_RES_UNSUPPORTED_PROTOCOL (9)

#define APICMD_GET_FW_UPGRADE_STATE_IDLE (0)
#define APICMD_GET_FW_UPGRADE_STATE_WAITING_FOR_DOWNLOAD_CONFIRMATION (1)
#define APICMD_GET_FW_UPGRADE_STATE_DOWNLOAD_PRECONDITION (2)
#define APICMD_GET_FW_UPGRADE_STATE_DURING_DOWNLOAD (3)
#define APICMD_GET_FW_UPGRADE_STATE_DOWNLOAD_COMPLETED (4)
#define APICMD_GET_FW_UPGRADE_STATE_WAITING_FOR_UPDATE_CONFIRMATION (5)
#define APICMD_GET_FW_UPGRADE_STATE_UPDATE_PRECONDITION (6)
#define APICMD_GET_FW_UPGRADE_STATE_WAITING_FOR_REBOOT (7)
#define APICMD_GET_FW_UPGRADE_STATE_WAITING_FOR_HOST_RESULT (8)
#define APICMD_GET_FW_UPGRADE_STATE_FOTA_FINISHED (9)

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* This structure describes the data structure of the API command */

/* APICMDID_GET_FW_UPGRADE_STATE
 * no data
 */

begin_packed_struct struct apicmd_cmddat_getfwupgradestateres_s {
  uint8_t result;
  uint8_t state;
  uint32_t image_downloaded_size;
  uint32_t image_total_size;
  uint8_t fota_result;
} end_packed_struct;

#endif /* __MODULES_LTE_ALTCOM_INCLUDE_API_LTE_APICMD_GET_FW_UPGRADE_STATE_H */
