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

#ifndef __MODULES_INCLUDE_FS_ALTCOM_FS_H
#define __MODULES_INCLUDE_FS_ALTCOM_FS_H

/**
 * @file  altcom_fs.h
 */

/**
 * @defgroup fs Filesystem Connector APIs
 * @{
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <sys/types.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/**
 * @defgroup fs_constants Filesystem Constants
 * @{
 */

#define ALTCOM_FS_LIST_MAXSIZE (10) /**< Maximum number of files */
#define ALTCOM_FS_PATH_MAXLEN (255) /**< Maximum length of file path including terminator '\0' */

/** @} io_constants */

/****************************************************************************
 * Public Types
 ****************************************************************************/
/**
 * @defgroup fs_types Filesystem Types
 * @{
 */

/**
 * @brief Definition of an directory handle.
 */

typedef int32_t altcom_fs_dir_t;

/**
 * @brief Enumeration of an file type.
 */

typedef enum {
  FS_TYPE_DIR = 0,  /**< Entry is a directory */
  FS_TYPE_FILE = 1, /**< Entry is a regular file */
  FS_TYPE_MAX       /**< Maximum value */
} altcom_fs_type_t;

/**
 * @brief Definition of an directory entry.
 */

typedef struct {
  altcom_fs_type_t d_type;            /**< Entry type, please also see @ref altcom_fs_type_t */
  char d_name[ALTCOM_FS_PATH_MAXLEN]; /**< Entry name, excludes its absolute path */
} altcom_fs_dirent_t;

/**
 * @brief Definition of an file status structure.
 */

typedef struct {
  altcom_fs_type_t st_mode; /**< Entry type, please also see @ref altcom_fs_type_t */
  off_t st_size;            /**< Entry length */
} altcom_fs_stat_t;

/**
 * @brief Enumeration of error code for directory operation.
 */

typedef enum {
  FS_NO_ERR = 0,             /**< Operation with no error */
  FS_ERR_INVALIDDIR = 3,     /**< Invalid directory */
  FS_ERR_INVALIDNAME = 4,    /**< Invalid directory name */
  FS_ERR_NOTFOUND = 5,       /**< Target entry not found */
  FS_ERR_DUPLICATED = 6,     /**< Target entry already exist */
  FS_ERR_NOMOREENTRY = 7,    /**< No more entry in the directory */
  FS_ERR_ACCESSDENIED = 13,  /**< Access denied to the target */
  FS_ERR_NOTEMPTY = 14,      /**< Target directory is not empty */
  FS_ERR_INVALIDPARAM = 901, /**< Invalid parameter */
  FS_ERR_REQERR = 902,       /**< API request error */
  FS_ERR_NOINIT = 903,       /**< Infrastructure is not initialized */
  FS_ERR_NOMEM = 904,        /**< Memory allocation issue */
  FS_ERR_MAX = 999           /**< Maximum value */
} altcom_fs_err_t;
/** @} fs_types */

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
 * @defgroup fs_funcs Filesystem APIs
 * @{
 */

/**
 * @brief Get a list of file path in folder.
 *
 * @attention Attention to the following when this API calling.
 *            - The order of the obtained file paths is modem-dependent
 *            and is not guaranteed.
 *            - If there are more files than @ref ALTCOM_FS_LIST_MAXSIZE,
 *            the subsequent file path can't be obtained unless files
 *            are deleted.
 *
 * @param [in] folder_path: The full path of the folder to
 *                          get the file list. Shall contain terminator '\0'.
 *                          See @ref ALTCOM_FS_PATH_MAXLEN for maximum length
 *                          that can be set.
 *
 * @param [in] list_size: The maximum number of elements of the list to get.
 *                        See @ref ALTCOM_FS_LIST_MAXSIZE for maximum size
 *                        that can be set.
 *
 * @param [in] path_len: The maximum length of the file path to get
 *                       including the terminator '\0'.
 *                       See @ref ALTCOM_FS_PATH_MAXLEN for maximum length
 *                       that can be set.
 *
 * @param [out] list: Pointer to the obtained file path list.
 *                    File path string contains '\0'.
 *
 * @return On success, number of files is returned.
 *         On failure, a negative value is returned according to <errno.h>.
 */

int32_t altcom_fs_get_filelist(const char *folder_path, uint8_t list_size, uint8_t path_len,
                               char **list);

/**
 * @brief Open a directory and obtain the directory handle for further access.
 *
 * @param [in] dir_name: The target directory to open. Shall contain terminator '\0'.
 *                       See @ref ALTCOM_FS_PATH_MAXLEN for maximum length that can be set.
 *
 * @param [in] dir: The dir handle openned from the target directory.
 *
 * @return On success, FS_NO_ERR is returned.
 *         On failure, other is returned, please also see @ref altcom_fs_err_t.
 */

altcom_fs_err_t altcom_fs_opendir(const char *dir_name, altcom_fs_dir_t *dir);

/**
 * @brief Read an entry information from an openned directory handle.
 *
 * @param [in] dir: The target directory handle to read
 *
 * @param [out] dirent: The entry found by specified handle @a dir.
 *
 * @return On success, FS_NO_ERR is returned.
 *         On failure, other is returned, please also see @ref altcom_fs_err_t.
 */
altcom_fs_err_t altcom_fs_readdir(altcom_fs_dir_t dir, altcom_fs_dirent_t *dirent);

/**
 * @brief Close an openned directory handle.
 *
 * @param [in] dir: The target directory handle to close
 *
 * @return On success, FS_NO_ERR is returned.
 *         On failure, other is returned, please also see @ref altcom_fs_err_t.
 */

altcom_fs_err_t altcom_fs_closedir(altcom_fs_dir_t dir);

/**
 * @brief Create a new directory.
 *
 * @param [in] dir_name: The directory name to create. Shall contain terminator '\0'.
 *                       See @ref ALTCOM_FS_PATH_MAXLEN for maximum length that can be set.
 *
 * @return On success, FS_NO_ERR is returned.
 *         On failure, other is returned, please also see @ref altcom_fs_err_t.
 */

altcom_fs_err_t altcom_fs_mkdir(const char *dir_name);

/**
 * @brief Remove an exist directory.
 *
 * @param [in] dir_name: The directory name to remove. Shall contain terminator '\0'.
 *                       See @ref ALTCOM_FS_PATH_MAXLEN for maximum length that can be set.
 *
 * @return On success, FS_NO_ERR is returned.
 *         On failure, other is returned, please also see @ref altcom_fs_err_t.
 */

altcom_fs_err_t altcom_fs_rmdir(const char *dir_name);

/**
 * @brief Get information of file/directory
 *
 * @param [in] path_name: The name of a file/directory. Shall contain terminator '\0'.
 *                       See @ref ALTCOM_FS_PATH_MAXLEN for maximum length that can be set.
 *
 * @param [out] stat: The output status of specified @a dir.
 *
 * @return On success, FS_NO_ERR is returned.
 *         On failure, other is returned, please also see @ref altcom_fs_err_t.
 */

altcom_fs_err_t altcom_fs_stat(const char *path_name, altcom_fs_stat_t *stat);

/** @} fs_funcs */

#undef EXTERN
#ifdef __cplusplus
}
#endif

/** @} fs */

#endif /* __MODULES_INCLUDE_FS_ALTCOM_FS_H */
