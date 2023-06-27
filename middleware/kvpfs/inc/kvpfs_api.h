/****************************************************************************
 *  (c) copyright 2019 Altair Semiconductor, Ltd. All rights reserved.
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

#ifndef DEF_KVPFS_API_H
#define DEF_KVPFS_API_H
#include "kvpfs_err.h"
/**
 * @brief Do file system initialization. This function have to be called prior any file system
 * functions.
 *
 */
void kvpfs_init(void);

/**
 * @brief To add/delete/modify a data in file system. Provied a key which presents in file system
 * will modify the value of the key. Provide a key which not presents in file system will add a
 * data. Leave value paramter as NULL will delete the data of matching key. All modifications stays
 * in RAM until save_env() is called. Limitation:
 * 1. CRC_KEYNAME in kvpfs_config.h is a reserved word which can't be the name of the key.
 * 2. ENV_MAGIC in kvpfs_config.h is a reserved word which can't be the name of the key.
 * 3. Key and/or value can't include \x00 and \xff
 *
 * @param key [in]: The name of the key.
 * @param value [in]: the content of the value.
 * @return Return 0 on success. Return negtive for errors. Refer to kvpfs_err.h for error reasons.
 */
int32_t set_env(const char *key, const char *value);

/**
 * @brief Get data from file system.
 *
 * @param key [in]: The name of the key to be searched. Leave key as NULL will dump the whole file
 * system.
 * @param value [out]: This function allocates memory and saves the result to it. Application have
 * to free the memory after use.
 * @return Return 0 on success. Return negtive for errors. Refer to kvpfs_err.h for error reasons.
 */
int32_t get_env(const char *key, char **value);

/**
 * @brief Write data in RAM to flash.
 *
 * @return Return 0 on success. Return negtive for errors. Refer to kvpfs_err.h for error reasons.
 */
int32_t save_env(void);

/**
 * @brief Save the variables in RAM to GPM rather than flash.
 * Application calls this function prior to entering stateful PSM mode in order to reduce the amount of flash writing count.
 * @return int32_t eturn 0 on success. Return negtive for errors. Refer to kvpfs_err.h for error reasons.
 */
int32_t save_env_to_gpm(void);

/**
 * @brief Prints file system information
 *
 */
void kvpfs_fss(void);
#endif