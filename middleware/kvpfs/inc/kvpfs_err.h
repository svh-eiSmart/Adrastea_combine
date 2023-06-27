#ifndef __KVPFS_ERR_H__
#define __KVPFS_ERR_H__

#define KVPFS_OK (0)                        /*!< Success */
#define KVPFS_RESERVE_KEYWD (1000)          /*!< The key name is a reserved word */
#define KVPFS_KEY_NOT_FOUND (1001)          /*!< The key name can't find in file system */
#define KVPFS_NULL_POINTER (1002)           /*!< Pass a null pointer */
#define KVPFS_OUT_OF_MEM (1003)             /*!< Fail to allocate memory */
#define KVPFS_FLASH_SPACE_NOT_ENOUGH (1004) /*!< Can't save data to flash due to out of space */
#define KVPFS_UNINITIALIZED (1005)          /*!< Calling functions with initialization */
#endif