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

#ifndef __MODULES_ALTCOM_INCLUDE_API_FS_APICMD_FS_H
#define __MODULES_ALTCOM_INCLUDE_API_FS_APICMD_FS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "altcom_fs.h"
#include "apicmd.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* APICMDID_FS_GETFILELIST */

begin_packed_struct struct apicmd_cmddat_fs_getfilelist_s {
  uint8_t list_size;
  uint8_t path_len;
  char path[ALTCOM_FS_PATH_MAXLEN];
} end_packed_struct;

/* APICMDID_FS_GETFILELIST_RES */

begin_packed_struct struct apicmd_cmddat_fs_getfilelistres_s {
  int32_t result;
  uint8_t list_size;
  uint8_t path_len;
  char listdat[];
} end_packed_struct;

begin_packed_struct struct apicmd_cmddat_fs_opendir_s {
  char dir_name[ALTCOM_FS_PATH_MAXLEN];
} end_packed_struct;

begin_packed_struct struct apicmd_cmddat_fs_opendirres_s {
  int32_t result;
  int32_t errcode;
  int32_t dir_handle;
} end_packed_struct;

begin_packed_struct struct apicmd_cmddat_fs_readdir_s { uint32_t dir_handle; } end_packed_struct;

begin_packed_struct struct apicmd_fs_dirent_s {
  uint8_t ent_type;
  char ent_name[ALTCOM_FS_PATH_MAXLEN];
} end_packed_struct;

begin_packed_struct struct apicmd_cmddat_fs_readdirres_s {
  int32_t result;
  int32_t errcode;
  struct apicmd_fs_dirent_s dirent;
} end_packed_struct;

begin_packed_struct struct apicmd_cmddat_fs_closedir_s { uint32_t dir_handle; } end_packed_struct;

begin_packed_struct struct apicmd_cmddat_fs_closedirres_s {
  int32_t result;
  int32_t errcode;
} end_packed_struct;

begin_packed_struct struct apicmd_cmddat_fs_mkdir_s {
  char dir_name[ALTCOM_FS_PATH_MAXLEN];
} end_packed_struct;

begin_packed_struct struct apicmd_cmddat_fs_mkdirres_s {
  int32_t result;
  int32_t errcode;
} end_packed_struct;

begin_packed_struct struct apicmd_cmddat_fs_rmdir_s {
  char dir_name[ALTCOM_FS_PATH_MAXLEN];
} end_packed_struct;

begin_packed_struct struct apicmd_cmddat_fs_rmdirres_s {
  int32_t result;
  int32_t errcode;
} end_packed_struct;

begin_packed_struct struct apicmd_cmddat_fs_stat_s {
  char dir_name[ALTCOM_FS_PATH_MAXLEN];
} end_packed_struct;

begin_packed_struct struct apicmd_fs_stat_s {
  uint8_t ent_type;
  uint32_t ent_len;
} end_packed_struct;

begin_packed_struct struct apicmd_cmddat_fs_statres_s {
  int32_t result;
  int32_t errcode;
  struct apicmd_fs_stat_s stat;
} end_packed_struct;

#endif /* __MODULES_ALTCOM_INCLUDE_API_FS_APICMD_FS_H */
