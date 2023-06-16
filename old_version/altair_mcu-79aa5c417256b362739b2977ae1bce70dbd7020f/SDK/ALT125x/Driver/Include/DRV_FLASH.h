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
/**
* @file DRV_FLASH.h
*/

#if !defined(__DRV_FLASH_H__)
#define __DRV_FLASH_H__

#include <stdio.h>

/**
 * @defgroup flash_driver FLASH Low-Level Driver
 * @{
 */


/**
 * @defgroup drv_flash_constants FLASH Constants
 * @{
 */
extern char __MCU_MTD_PARTITION_BASE__;   /*!< Start address of the serial flash */
extern char __MCU_MTD_PARTITION_OFFSET__; /*!< Offset of the MCU partition from flash base*/
extern char __MCU_MTD_PARTITION_SIZE__;   /*!< Size of the MCU partition */

#define MCU_BASE_ADDR \
  ((unsigned int)&__MCU_MTD_PARTITION_BASE__ + (unsigned int)&__MCU_MTD_PARTITION_OFFSET__) /*!< Start address of the MCU partition */
#define MCU_PART_SIZE ((unsigned int)&__MCU_MTD_PARTITION_SIZE__) /*!< Size of the MCU partition */

#define SFLASH_4K_ERASE_SECTOR_SIZE (1UL << 12)                 /*!< Flash sector size (minimum erase size) */
#define SFLASH_4K_MASK (~(SFLASH_4K_ERASE_SECTOR_SIZE - 1))     /*!< Flash sector size mask */
/** @} drv_flash_constants */

/**
 * @defgroup drv_flash_types FLASH Types
 * @{
 */
/**
 * @brief Flash driver error code
 *
 */
typedef enum flash_err_code {
  FLASH_ERROR_NONE,          /*!< No errors*/
  FLASH_ERROR_TIMEOUT,       /*!< Flash command timeout */
  FLASH_ERROR_VERIFY,        /*!< Fail on flash content verification */
  FLASH_ERROR_ADDRESS_RANGE, /*!< Requested address outside of MCU range */
  FLASH_ERROR_PARAMETER,     /*!< Wrong parameters */
  FLASH_ERROR_OTHER_ERROR,   /*!< Unclassified error */
} Flash_Err_Code;
/** @} drv_flash_types */

#if defined(__cplusplus)
extern "C" {
#endif /* _cplusplus */

/**
 * @brief Initialize flash driver
 *
 * @return Return the error code. Refer to @ref Flash_Err_Code.
 */
Flash_Err_Code DRV_FLASH_Initialize(void);

/**
 * @brief Uninitialize flash driver
 *
 * @return Return the error code. Refer to @ref Flash_Err_Code.
 */
Flash_Err_Code DRV_FLASH_Uninitialize(void);

/**
 * @brief Read data from flash
 *
 * @param src Address to be read from flash
 * @param dst RAM space to store the data
 * @param size Data length to be read
 * @return Return the error code. Refer to @ref Flash_Err_Code.
 */
Flash_Err_Code DRV_FLASH_Read(void *src, void *dst, size_t size);

/**
 * @brief Write data to flash. Flash can only be changed from 1 to 0. This function doesn't check if
 * the flash is erased or not. User should take care of flash status.
 *
 * @param src RAM space that hold the data
 * @param dst Address to be written to flash
 * @param len Data length to be written
 * @param do_verify Examine if the src equals to dst after writing to flash. 0 do not verify. Others
 * do verify.
 * @return Return the error code. Refer to @ref Flash_Err_Code.
 */
Flash_Err_Code DRV_FLASH_Write(void *src, void *dst, size_t len, int do_verify);

/**
 * @brief Erase a sector in order to resore all element to 0xFF. A sector size is either 4 KB or 64
 * KB.
 *
 * @param addr Target sector that contains the flash address
 * @param is_64KB_sect 0 to erase 4KB. Others erase 64 KB
 * @param wait_for_finish 0 returns when erase command send to controller. Others return until
 * controller is done.
 * @return Return the error code. Refer to @ref Flash_Err_Code.
 */
Flash_Err_Code DRV_FLASH_Erase_Sector(void *addr, int is_64KB_sect, int wait_for_finish);

#if defined(__cplusplus)
}
#endif
/** @} flash_driver */
#endif /*__DRV_FLASH_H__*/
