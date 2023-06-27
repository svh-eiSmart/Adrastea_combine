/****************************************************************************
 *
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

#if !defined(__SERIAL_FLASH_DRV_API_1250B_H__)
#define __SERIAL_FLASH_DRV_API_1250B_H__

#include <stdio.h>
//#include <string.h>
//#include "osal/osal.h"
//#include <sys/types.h>
//#include <stdlib.h>
#include <stdint.h>
#include "125X_mcu.h"

typedef unsigned int u32;

#if !defined(CONFIG_SYS_FLASH_BASE)
#define CONFIG_SYS_FLASH_BASE 0xb8000000 /*Memory-mapped FLASH base address*/
#endif

#if 1
#define SF_FUNC_ATTR
#else
#define SF_FUNC_ATTR __ram_text
#endif

extern char __MCU_MTD_PARTITION_BASE__;
extern char __MCU_MTD_PARTITION_OFFSET__;
extern char __MCU_MTD_PARTITION_SIZE__;
#define MCU_BASE_ADDR \
  ((unsigned int)&__MCU_MTD_PARTITION_BASE__ + (unsigned int)&__MCU_MTD_PARTITION_OFFSET__)
#define MCU_PART_SIZE ((unsigned int)&__MCU_MTD_PARTITION_SIZE__)

#if !defined(CONFIG_ALTAIR_PALLADIUM)
#define SF_BUSY_WAIT_TIME_OUT (5000000)
#define SF_CMD_RESULT_TIME_OUT (5000000)
#define SF_PAGE_WRITE_TIME_OUT (5000000)
#define SF_ERASE_SECTOR_TIME_OUT (5000000)
#else
#define SF_BUSY_WAIT_TIME_OUT (50000)
#define SF_CMD_RESULT_TIME_OUT (50000)
#define SF_PAGE_WRITE_TIME_OUT (50000)
#define SF_ERASE_SECTOR_TIME_OUT (5000000)
#endif /*CONFIG_ALTAIR_PALLADIUM*/

#define SFLASH_4K_ERASE_SECTOR_SIZE (1UL << 12) /* 4KB erase size */
#define SFLASH_4K_MASK (~(SFLASH_4K_ERASE_SECTOR_SIZE - 1))
#define SFLASH_64K_ERASE_SECTOR_SIZE (1UL << 16) /* 64KB erase size */

#define SFLASH_CONFIG_ENABLE_DEBUG_PRINTS
#define SFLASH_CONFIG_DEBUG_LEVEL 0
//#define SFLASH_CONFIG_DISABLE_ERROR_PRINTS

#if defined(SFLASH_CONFIG_ENABLE_DEBUG_PRINTS)
#define SFDBG(lEVEL, fmt, args...)                               \
  {                                                              \
    if (lEVEL <= SFLASH_CONFIG_DEBUG_LEVEL) printf(fmt, ##args); \
  }
#else
#define SFDBG(lEVEL, fmt, args...) \
  {}
#endif

#if !defined(SFLASH_CONFIG_DISABLE_ERROR_PRINTS)
#define SFERR(fmt, args...) printf(fmt, ##args)
#else
#define SFERR(fmt, args...) \
  {}
#endif

void serial_flash_api_init(u32 subsys_base_address);  // BASE_ADDRESS_SERIAL_FLASH_CTRL_xyz_SUBSYS
void serial_flash_reset_command(void);
int serial_flash_is_device_busy(void);

void serial_flash_read(volatile ulong *from, volatile ulong *to, ulong size);
int serial_flash_write_buffer(unsigned char *src, ulong addr, ulong len, int verify);
int serial_flash_erase_sector(ulong addr, int is_64KB_sect, int wait_for_finish);
int serial_flash_verify_buffer(unsigned char *src, ulong addr, ulong len);
void serial_flash_driver_wait_for_transaction_finish(void);

#endif /*__SERIAL_FLASH_DRV_API_1250B_H__*/
