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

#ifndef __MODULES_INCLUDE_IO_ALTCOM_IO_H
#define __MODULES_INCLUDE_IO_ALTCOM_IO_H

/**
 * @file  altcom_io.h
 */

/**
 * @defgroup io I/O Connector APIs
 * @{
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/**
 * @defgroup io_constants I/O Constants
 * @{
 */

/** @} io_constants */

/****************************************************************************
 * Public Types
 ****************************************************************************/
/**
 * @defgroup io_types I/O Types
 * @{
 */

/** @} io_types */

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/**
 * @defgroup io_funcs I/O APIs
 * @{
 */

/**
 * @brief Open a file on modem.
 *
 * @attention In case unexpected reset occurred in LTE modem,@n
 *            or run re-initialize/re-start purposely,@n
 *            file descriptor becomes invalid at that time.@n
 *            Please re-open to obtain file descriptor.
 *            (There is no need to close original file descriptor)@n
 *            To detect reset in LTE modem,
 *            please use @ref lte_set_report_restart function.@n
 *
 * @param [in] pathname: Path name to open.
 *                       Maxinum @a pathname length is 255 bytes.
 * @param [in] flags: The file control flags and file access modes.@n
 *                    Available @a flags are as follows.
 *                    - O_RDONLY
 *                    - O_WRONLY
 *                    - O_RDWR
 *                    - O_APPEND
 *                    - O_CREAT
 *                    - O_TRUNC
 *                    - O_EXCL
 *                    .
 *                    Please set following @a flags combination
 *                    when LTE modem is ALT1250.
 *                    - Opening an existing file : O_RDWR@n
 *                    - Opening a new file : O_RDWR  |
 *                                           O_CREAT |
 *                                           O_TRUNC@n
 *                    .
 *                    Operation with other combination is not guaranteed.
 *
 * @param [in] mode: Access permission if a new file will be created.@n
 *                   Available @a mode are as follows.
 *                   - S_IRWXU
 *                   - S_IRUSR
 *                   - S_IWUSR
 *                   - S_IXUSR
 *                   - S_IRWXG
 *                   - S_IRGRP
 *                   - S_IWGRP
 *                   - S_IXGRP
 *                   - S_IRWXO
 *                   - S_IROTH
 *                   - S_IWOTH
 *                   - S_IXOTH
 *                   .
 *                   @a mode is ignored when any of the following applies:
 *                   - Opening existing files
 *                   - LTE modem is ALT1250
 *
 * @return On success, file descriptor is returned.
 *         It is a non-negative integer value.
 *         On failure, -1 is returned.
 */

int altcom_io_open(const char *pathname, int flags, mode_t mode);

/**
 * @brief Close a file descriptor.
 *
 * @param [in] fd: File descriptor.
 *
 * @return On success, 0 is returned.
 *         On failure, -1 is returned.
 */

int altcom_io_close(int fd);

/**
 * @brief Write data to file on modem.
 *
 * @param [in] fd: File descriptor.
 * @param [in] buf: Data to write.
 * @param [in] count: Length of data.
 *
 * @return On success, the number of bytes written is returned.
 *         On failure, -1 is returned.
 */

ssize_t altcom_io_write(int fd, const void *buf, size_t count);

/**
 * @brief Read data from file on modem.
 *
 * @param [in] fd: File descriptor.
 * @param [out] buf: Buffer to read.
 * @param [in] count: Read length.
 *
 * @return On success, the number of bytes read is returned.
 *         On failure, -1 is returned.
 */

ssize_t altcom_io_read(int fd, void *buf, size_t count);

/**
 * @brief Remove a file on modem.
 *
 * @param [in] pathname: Path name to remove.
 *                       Maxinum @a pathname length is 255 bytes.
 *
 * @return On success, 0 is returned.
 *         On failure, -1 is returned.
 */

int altcom_io_remove(const char *pathname);

/**
 * @brief Set the file write/read offset.
 *
 * @param [in] fd: File descriptor.
 * @param [in] offset: The number of offset bytes.
 * @param [in] whence: Reference point of offset.@n
 *                     Available @a whence are as follows.
 *                     - SEEK_SET
 *                     - SEEK_CUR
 *                     - SEEK_END
 *                     .
 *
 * @return On success, The offset from the beginning of the file is returned.
 *         On failure, -1 is returned.
 */

int altcom_io_lseek(int fd, off_t offset, int whence);

/** @} io_funcs */

#undef EXTERN
#ifdef __cplusplus
}
#endif

/** @} io */

#endif /* __MODULES_INCLUDE_IO_ALTCOM_IO_H */
