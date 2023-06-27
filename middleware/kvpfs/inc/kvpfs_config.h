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

#ifndef __DEF_KVPFS_H__
#define __DEF_KVPFS_H__

#include "serial_flash_drv_api_1250b.h"

#define ENV_SIZE (8192)
#define CRC_KEYNAME "CRC"
#define MAGIC_KEYNAME "mag"
#define MAGIC_VALUE "a1b2c3d4"


#define KVPFS_BASE_ADDR (MCU_BASE_ADDR + MCU_PART_SIZE - (2 * ENV_SIZE))
#define CRC_LEN (sizeof(CRC_KEYNAME) + 1 + 4)  // +1(=) +4(16 byte crc)
#define ENV_MIGIC_LEN sizeof(ENV_MAGIC)

#endif