/****************************************************************************
 *
 *   Copyright 2020 Sony Semiconductor Solutions Corporation
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <errno.h>
#include <string.h>

#include "apiutil.h"
#include "buffpoolwrapper.h"

#include "altcom_fs.h"
#include "apicmd_fs.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ALTCOM_FS_GETFILELIST_DATA_LEN (sizeof(struct apicmd_cmddat_fs_getfilelist_s))

#define ALTCOM_FS_GETFILELIST_RES_DATA_LEN(list_size, path_len) \
  (sizeof(struct apicmd_cmddat_fs_getfilelistres_s) + (list_size * path_len))

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: altcom_fs_get_filelist
 *
 * Description:
 *  Get a list of file path in folder.
 *
 * Returned Value:
 * On success, number of files is returned.
 * On failure, a negative value is returned according to <errno.h>.
 *
 ****************************************************************************/

int32_t altcom_fs_get_filelist(const char *folder_path, uint8_t list_size, uint8_t path_len,
                               char **list) {
  FAR struct apicmd_cmddat_fs_getfilelist_s *cmdbuff = NULL;
  FAR struct apicmd_cmddat_fs_getfilelistres_s *resbuff = NULL;
  int32_t ret = 0;
  uint16_t reslen = 0;

  /* Check if the library is initialized */

  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return -1;
  }

  if (!folder_path) {
    DBGIF_LOG_ERROR("Folder path is NULL.\n");
    return -EINVAL;
  }
  if (0 == strlen(folder_path) || ALTCOM_FS_PATH_MAXLEN <= strlen(folder_path)) {
    DBGIF_LOG1_ERROR("Invalid folder path length [%d].\n", strlen(folder_path));

    return -EINVAL;
  }

  if (!list) {
    DBGIF_LOG_ERROR("List array is NULL.\n");
    return -EINVAL;
  }

  if (0 == list_size || ALTCOM_FS_LIST_MAXSIZE < list_size) {
    DBGIF_LOG1_ERROR("Invalid list size [%d].\n", list_size);
    return -EINVAL;
  }

  if (0 == path_len) {
    DBGIF_LOG1_ERROR("Invalid file path length [%d].\n", path_len);
    return -EINVAL;
  }

  /* Allocate API command buffer to send */

  cmdbuff = (struct apicmd_cmddat_fs_getfilelist_s *)apicmdgw_cmd_allocbuff(
      APICMDID_FS_GETFILELIST, ALTCOM_FS_GETFILELIST_DATA_LEN);
  if (!cmdbuff) {
    DBGIF_LOG_ERROR("Failed to allocate command buffer.\n");
    return -ENOMEM;
  }

  /* Set command parameter. */

  cmdbuff->list_size = list_size;
  cmdbuff->path_len = path_len;
  strncpy(cmdbuff->path, folder_path, strlen(folder_path));

  /* Allocate API response buffer.
   * Allocate only the number of arrays required by user.
   */

  resbuff = (FAR struct apicmd_cmddat_fs_getfilelistres_s *)BUFFPOOL_ALLOC(
      ALTCOM_FS_GETFILELIST_RES_DATA_LEN(list_size, path_len));
  if (!resbuff) {
    DBGIF_LOG_ERROR("Failed to allocate command buffer.\n");
    altcom_free_cmd((FAR uint8_t *)cmdbuff);
    return -ENOMEM;
  }

  /* Send API command to modem */

  ret = apicmdgw_send((FAR uint8_t *)cmdbuff, (FAR uint8_t *)resbuff,
                      ALTCOM_FS_GETFILELIST_RES_DATA_LEN(list_size, path_len), &reslen,
                      ALT_OSAL_TIMEO_FEVR);

  if (0 > ret) {
    DBGIF_LOG1_ERROR("Failed to apicmdgw_send [%d].\n", ret);
  } else if (0 == reslen) {
    DBGIF_LOG_ERROR("Modem does not support this API.\n");
    ret = -ENOTSUP;
  } else {
    ret = ntohl(resbuff->result);

    if (0 > ret) {
      DBGIF_LOG1_ERROR("Modem returned error [%d].\n", ret);
    } else {
      DBGIF_LOG1_INFO("Number of obtained files [%d].\n", resbuff->list_size);

      if (path_len != resbuff->path_len || list_size < resbuff->list_size) {
        DBGIF_LOG2_ERROR("Bad array size sent from modem [%d][%d].\n", resbuff->list_size,
                         resbuff->path_len);
        ret = -ENOBUFS;
      } else if ((list_size * path_len) <
                 (reslen - sizeof(struct apicmd_cmddat_fs_getfilelistres_s))) {
        DBGIF_LOG2_ERROR("Exceeded array size [%d]. expect [%d].\n",
                         (reslen - sizeof(struct apicmd_cmddat_fs_getfilelistres_s)),
                         (list_size * path_len));

        ret = -ENOBUFS;
      } else {
        memcpy(list, resbuff->listdat, (reslen - sizeof(struct apicmd_cmddat_fs_getfilelistres_s)));
      }
    }
  }

  altcom_free_cmd((FAR uint8_t *)cmdbuff);
  (void)BUFFPOOL_FREE(resbuff);

  return ret;
}
