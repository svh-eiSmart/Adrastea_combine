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

#include "altcom_fs.h"
#include "apicmd_fs.h"
#include "buffpoolwrapper.h"
#include "apiutil.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ALTCOM_FS_STAT_DATA_LEN (sizeof(struct apicmd_cmddat_fs_stat_s))
#define ALTCOM_FS_STAT_RES_DATA_LEN (sizeof(struct apicmd_cmddat_fs_statres_s))

/****************************************************************************
 * Public Functions
 ****************************************************************************/

altcom_fs_err_t altcom_fs_stat(const char *path_name, altcom_fs_stat_t *statbuf) {
  int32_t ret = 0;
  FAR struct apicmd_cmddat_fs_stat_s *cmdbuff = NULL;
  FAR struct apicmd_cmddat_fs_statres_s *resbuff = NULL;
  uint16_t reslen = 0;

  /* Check if the library is initialized */

  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return FS_ERR_NOINIT;
  }

  /* Check parameter */

  if (!path_name || strlen(path_name) == 0) {
    DBGIF_LOG_ERROR("Invalid path_name\n");
    return FS_ERR_INVALIDPARAM;
  }

  if (!statbuf) {
    DBGIF_LOG_ERROR("Invalid statbuf\n");
    return FS_ERR_INVALIDPARAM;
  }

  /* Allocate API command buffer to send */

  if (altcom_generic_alloc_cmdandresbuff((FAR void **)&cmdbuff, APICMDID_FS_STAT,
                                         ALTCOM_FS_STAT_DATA_LEN, (FAR void **)&resbuff,
                                         ALTCOM_FS_STAT_RES_DATA_LEN)) {
    snprintf(cmdbuff->dir_name, ALTCOM_FS_PATH_MAXLEN, "%s", path_name);
  } else {
    DBGIF_LOG_ERROR("Failed to allocate command buffer.\n");
    return FS_ERR_NOMEM;
  }

  /* Send API command to modem */

  ret = apicmdgw_send((FAR uint8_t *)cmdbuff, (FAR uint8_t *)resbuff, ALTCOM_FS_STAT_RES_DATA_LEN,
                      &reslen, ALT_OSAL_TIMEO_FEVR);

  if (0 > ret) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %d\n", ret);
    ret = FS_ERR_NOMEM;
  } else if (ALTCOM_FS_STAT_RES_DATA_LEN != reslen) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %d\n", reslen);
    ret = FS_ERR_NOMEM;
  } else {
    ret = ntohl(resbuff->result);
    DBGIF_LOG2_INFO("%s[%d]\n", __FUNCTION__, ret);
    if (ret == 0) {
      statbuf->st_mode = (altcom_fs_type_t)resbuff->stat.ent_type;
      DBGIF_ASSERT(statbuf->st_mode < FS_TYPE_MAX, "Invalid st_mode %lu",
                   (uint32_t)statbuf->st_mode);
      statbuf->st_size = (off_t)ntohl(resbuff->stat.ent_len);
    } else {
      DBGIF_LOG2_ERROR("%s ret %ld\n", __FUNCTION__, ret);
      ret = ntohl(resbuff->errcode);
      DBGIF_LOG2_ERROR("%s errcode %ld\n", __FUNCTION__, ret);
    }
  }

  altcom_generic_free_cmdandresbuff(cmdbuff, resbuff);
  return (altcom_fs_err_t)ret;
}
