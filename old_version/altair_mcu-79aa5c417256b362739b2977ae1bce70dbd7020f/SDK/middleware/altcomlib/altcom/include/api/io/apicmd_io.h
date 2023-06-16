/****************************************************************************
 *
 *   Copyright 2019 Sony Semiconductor Solutions Corporation
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

#ifndef __MODULES_ALTCOM_INCLUDE_API_IO_APICMD_IO_H
#define __MODULES_ALTCOM_INCLUDE_API_IO_APICMD_IO_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "apicmd.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define APICMD_IO_O_RDONLY (0x0001)
#define APICMD_IO_O_WRONLY (0x0002)
#define APICMD_IO_O_RDWR   (0x0004)
#define APICMD_IO_O_APPEND (0x0008)
#define APICMD_IO_O_CREAT  (0x0010)
#define APICMD_IO_O_TRUNC  (0x0020)
#define APICMD_IO_O_EXCL   (0x0040)

#define APICMD_IO_S_IRUSR (0000400)
#define APICMD_IO_S_IWUSR (0000200)
#define APICMD_IO_S_IXUSR (0000100)
#define APICMD_IO_S_IRWXU (APICMD_IO_S_IRUSR | \
                           APICMD_IO_S_IWUSR | \
                           APICMD_IO_S_IXUSR)

#define APICMD_IO_S_IRGRP (0000040)
#define APICMD_IO_S_IWGRP (0000020)
#define APICMD_IO_S_IXGRP (0000010)
#define APICMD_IO_S_IRWXG (APICMD_IO_S_IRGRP | \
                           APICMD_IO_S_IWGRP | \
                           APICMD_IO_S_IXGRP)

#define APICMD_IO_S_IROTH (0000004)
#define APICMD_IO_S_IWOTH (0000002)
#define APICMD_IO_S_IXOTH (0000001)
#define APICMD_IO_S_IRWXO (APICMD_IO_S_IROTH | \
                           APICMD_IO_S_IWOTH | \
                           APICMD_IO_S_IXOTH)

#define APICMD_IO_LSEEK_SET (0)
#define APICMD_IO_LSEEK_CUR (1)
#define APICMD_IO_LSEEK_END (2)

#define APICMD_IO_PATHNAME_LENGTH   (256)

/* 4096 is the most efficient value for reading / writing files. */

#define APICMD_IO_WRITE_DATA_LENGTH (4096)
#define APICMD_IO_READ_DATA_LENGTH  (4096)

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* This structure discribes the data structure of the API command */

begin_packed_struct struct apicmd_io_open_s
{
  int8_t pathname[APICMD_IO_PATHNAME_LENGTH];
  int32_t flags;
  int32_t mode;
} end_packed_struct;

begin_packed_struct struct apicmd_io_openres_s
{
  int32_t ret_code;
  int32_t err_code;
} end_packed_struct;

begin_packed_struct struct apicmd_io_close_s
{
  int32_t fd;
} end_packed_struct;

begin_packed_struct struct apicmd_io_closeres_s
{
  int32_t ret_code;
  int32_t err_code;
} end_packed_struct;

begin_packed_struct struct apicmd_io_write_s
{
  int32_t fd;
  int32_t datalen;
  int8_t writedata[APICMD_IO_WRITE_DATA_LENGTH];
} end_packed_struct;

begin_packed_struct struct apicmd_io_writeres_s
{
  int32_t ret_code;
  int32_t err_code;
} end_packed_struct;

begin_packed_struct struct apicmd_io_read_s
{
  int32_t fd;
  int32_t readlen;
} end_packed_struct;

begin_packed_struct struct apicmd_io_readres_s
{
  int32_t ret_code;
  int32_t err_code;
  int8_t readdata[APICMD_IO_READ_DATA_LENGTH];
} end_packed_struct;

begin_packed_struct struct apicmd_io_remove_s
{
  int8_t pathname[APICMD_IO_PATHNAME_LENGTH];
} end_packed_struct;

begin_packed_struct struct apicmd_io_removeres_s
{
  int32_t ret_code;
  int32_t err_code;
} end_packed_struct;

begin_packed_struct struct apicmd_io_lseek_s
{
  int32_t fd;
  int32_t offset;
  int32_t whence;
} end_packed_struct;

begin_packed_struct struct apicmd_io_lseekres_s
{
  int32_t ret_code;
  int32_t err_code;
} end_packed_struct;

#endif /* __MODULES_ALTCOM_INCLUDE_API_IO_APICMD_IO_H */
