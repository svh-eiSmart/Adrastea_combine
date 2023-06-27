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
/**
 * @file i2c.h
 */

#ifndef I2C_ALT125X_H_
#define I2C_ALT125X_H_

/**
 * @defgroup i2c_driver I2C Driver
 * @{
 */

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/**
 * @defgroup i2c_constants I2C Constants
 * @{
 */
#define MAX_I2C_DEVICES_QUANT (2) /**< Maximum I2C devices */

/** @} i2c_constants */

/****************************************************************************
 * Public Data Types
 ****************************************************************************/
/**
 * @defgroup i2c_types I2C Types
 * @{
 */

/**
 * @brief Definition of I2C bus.
 */
typedef enum {
  I2C0_BUS, /**< I2C bus 0 */
  I2C1_BUS, /**< I2C bus 1 */
} i2c_bus_e;

/**
 * @brief Definition of I2C data address length.
 */
typedef enum {
  I2C_ADDRLEN_1BYTE = 1,  /**< 1 byte address length */
  I2C_ADDRLEN_2BYTES = 2, /**< 2 bytes address length */
  I2C_ADDRLEN_3BYTES = 3, /**< 3 bytes address length */
} i2c_addrlen_e;

/**
 * @brief Definition of I2C device ID mode.
 */
typedef enum {
  I2C_DEVID_MODE_7BITS,  /**< 7 bits device ID mode */
  I2C_DEVID_MODE_10BITS, /**< 10 bits device ID mode */
} i2c_devid_mode_e;

typedef enum I2C_MODE{
	I2C_MODE_STANDARD = 1,
	I2C_MODE_FAST,
	I2C_MODE_FAST_PLUS,
	I2C_MODE_HIGH_SPEED,
	I2C_MODE_UTRA_FAST,
} I2C_MODE;

/** @} i2c_types */

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
 * @defgroup i2c_apis I2C APIs
 * @{
 */

/**
 * @brief I2C driver initialization function
 *
 * @param [in] i2c_bus_id: Target bus to be initializaed.
 *
 * @return 0: Success; -1: Fail.
 */
int i2c_init(i2c_bus_e i2c_bus_id);


int i2c_select_speed(i2c_bus_e bus, I2C_MODE i2c_mode);

/**
 * @brief Reads from I2C bus using advanced mode
 *
 * @param [in] device: I2C device ID
 * @param [in] mode: I2C device ID mode(7/10-bits)
 * @param [in] address: I2C register address to start reading
 * @param [in] addrLen: I2C register address length(1/2/3 Bytes)
 * @param [in] len: Number of bytes to read
 * @param [out] pDest: Pointer to destination array (its size must be bigger than len)
 * @param [in] i2c_bus_id: Target bus ID to read.
 *
 * @return 0: Success; -1: Fail.
 */
int i2c_read_advanced_mode(uint16_t device, i2c_devid_mode_e mode, uint32_t address,
                           i2c_addrlen_e addrLen, uint32_t len, uint8_t* pDest,
                           i2c_bus_e i2c_bus_id);

/**
 * @brief Writes to I2C bus using advanced mode
 *
 * @param [in] device: I2C device ID
 * @param [in] mode: I2C device ID mode(7/10-bits)
 * @param [in] address: I2C register address to start reading
 * @param [in] addrLen: I2C register address length(1/2/3 Bytes)
 * @param [in] len: Number of bytes to write
 * @param [in] pSrc: Pointer to data array (its size must be bigger than len)
 * @param [in] i2c_bus_id: Target bus ID to write.
 *
 * @return 0: Success; -1: Fail.
 */
int i2c_write_advanced_mode(uint16_t device, i2c_devid_mode_e mode, uint32_t address,
                            i2c_addrlen_e addrLen, uint32_t len, const uint8_t* pSrc,
                            i2c_bus_e i2c_bus_id);

/**
 * @brief A high level wrapper to read from I2C bus using i2c_basic_read
 *
 * @param [in] device: I2C device ID
 * @param [in] mode: I2C device ID mode(7/10-bits)
 * @param [in] address: I2C register address to start reading
 * @param [in] addrLen: I2C register address length(1/2/3 Bytes)
 * @param [in] len: Number of bytes to read
 * @param [out] pDest: Pointer to destination array (its size must be bigger than len)
 * @param [in] i2c_bus_id: Bus ID - 0 or 1
 *
 * @return 0: Success; -1: Fail.
 */
int i2c_read_basic_mode(uint16_t device, i2c_devid_mode_e mode, uint32_t address,
                        i2c_addrlen_e addrLen, uint32_t len, uint8_t* pDest, i2c_bus_e i2c_bus_id);

/**
 * @brief A high level wrapper to write to I2C bus using i2c_basic_write
 *
 * @param [in] device: I2C device ID
 * @param [in] mode: I2C device ID mode(7/10-bits)
 * @param [in] address: I2C register address to start reading
 * @param [in] addrLen: I2C register address length(1/2/3 Bytes)
 * @param [in] len: Number of bytes to write
 * @param [in] pSrc: Pointer to source data array (its size must be bigger than len)
 * @param [in] i2c_bus_id: Bus ID - 0 or 1
 *
 * @return 0: Success; -1: Fail.
 */
int i2c_write_basic_mode(uint16_t device, i2c_devid_mode_e mode, uint32_t address,
                         i2c_addrlen_e addrLen, uint32_t len, const uint8_t* pSrc,
                         i2c_bus_e i2c_bus_id);

/**
 * @brief Write to I2C bus using low level FIFO command
 *
 * @param [in] i2c_bus_id: Bus ID - 0 or 1
 * @param [in] device: Device ID
 * @param [in] data: Pointer to data array to write
 * @param [in] length: Length to write
 * @param [in] resetfifo: Reset I2C FIFO - 0 or 1
 * @param [in] start: Start I2C - 0 or 1
 * @param [in] restart: Restart I2C - 0 or 1
 * @param [in] stop: Stop I2C after finish, 0 or 1
 *
 * @return 0: Success; -1: Fail.
 */
int i2c_basic_write(i2c_bus_e i2c_bus_id, uint16_t device, const uint8_t* data, uint32_t length,
                    int resetfifo, int start, int restart, int stop);

/**
 * @brief Read from I2C bus using low level FIFO command
 *
 * @param [in] i2c_bus_id: Bus ID - 0 or 1
 * @param [in] device: Device ID
 * @param [out] data: Pointer to data array for reading
 * @param [in] length: Length to read
 * @param [in] resetfifo: Reset I2C FIFO - 0 or 1
 * @param [in] start: Restart I2C - 0 or 1
 * @param [in] restart: Start I2C - 0 or 1
 * @param [in] stop: Stop I2C after finish, 0 or 1
 *
 * @return 0: Success; -1: Fail.
 */
int i2c_basic_read(i2c_bus_e i2c_bus_id, uint16_t device, uint8_t* data, uint32_t length,
                   int resetfifo, int start, int restart, int stop);

/** @} i2c_apis */

#undef EXTERN
#ifdef __cplusplus
}
#endif

/** @} i2c_driver */

#endif /* I2C_ALT125X_H_ */
