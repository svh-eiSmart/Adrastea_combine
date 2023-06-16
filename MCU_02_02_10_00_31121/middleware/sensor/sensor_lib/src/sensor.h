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
 * @file sensor.h
 */
#ifndef SENSOR_H_
#define SENSOR_H_
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdint.h>
#include "DRV_GPIO.h"
#include "DRV_I2C.h"
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/**
 * @defgroup sensor_lib Sensor IO/Interrupt Library
 * @{
 */
#define SENSOR_RET_SUCCESS (0)      /**< API returns with no error */
#define SENSOR_RET_TIMEOUT (-1)     /**< API returns with timeout error */
#define SENSOR_RET_NOT_INIT (-2)    /**< API returns with not init error*/
#define SENSOR_RET_GENERIC_ERR (-3) /**< API returns with generic error*/
#define SENSOR_RET_PARAM_ERR (-4)   /**< API returns with parameter error*/
#define SENSOR_RET_UNSUPPORTED (-5) /**< API returns with not support error*/

#ifndef SENSOR_HANDLE_TASK_STACK_SIZE
#define SENSOR_HANDLE_TASK_STACK_SIZE (4096) /**< Stack size of sensor task*/
#endif
#ifndef SENSOR_HANDLE_TASK_PRIORITY
#define SENSOR_HANDLE_TASK_PRIORITY (ALT_OSAL_TASK_PRIO_HIGH) /**< Priority of sensor task */
#endif
#ifndef SENSOR_EVENT_QUEUE_SIZE
#define SENSOR_EVENT_QUEUE_SIZE 10 /**< Queue size configuration for IRQ notification*/
#endif
/****************************************************************************
 * Data types
 ****************************************************************************/
/**
 * @typedef sensor_i2c_handle_t
 * Handler for sensor i2c operations
 */
typedef void *sensor_i2c_handle_t;

/**
 * @typedef sensor_irq_ret_e
 * Return code definition of sensor_irq_handler
 */
typedef enum { SENSOR_IRQ_HANDLED, SENSOR_IRQ_NONE } sensor_irq_ret_e;
/**
 * @typedef sensor_irq_handler
 * Callback function defintion for sensor_request_irq
 * @param[in] user_param : Parameter for this callback.
 */
typedef sensor_irq_ret_e (*sensor_irq_handler)(void *user_param);
/**
 * @typedef sensor_write_fp
 * Sensor write operation function pointer
 * @param[in] handle: Handle for write operation
 * @param[in] reg: Register address for write.
 * @param[in] addr_len: Register address length
 * @param[in] write_buf: Write buffer to store data to be written to sensor.
 * @param[in] len: Length of writeBuf.
 */
typedef int32_t (*sensor_write_fp)(void *handle, uint32_t reg, uint16_t addr_len,
                                   uint8_t *write_buf, uint16_t len);
/**
 * @typedef sensor_read_fp
 * Sensor read operation function pointer
 * @param[in] handle: Handle for write operation
 * @param[in] reg: Register address to read
 * @param[in] addr_len: Register address length
 * @param[in] read_buf: Buffer to store data read from sensor
 * @param[in] len: Length of readBuf
 */
typedef int32_t (*sensor_read_fp)(void *handle, uint32_t reg, uint16_t addr_len, uint8_t *read_buf,
                                  uint16_t len);

/**
 * @typedef sensor_i2c_id_mode_e
 * Definition of sensor slave address mode
 */
typedef enum {
  SENSOR_I2C_ID_7BITS, /**< Sensor slave address with 7bits mode. */
  SENSOR_I2C_ID_10BITS /**< Sensor slave address with 10bits mode. */
} sensor_i2c_id_mode_e;

/*******************************************************************************
 * API
 ******************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief This is the initialization call before all other sensor APIs
 *
 * @return sensor return codes
 */
int32_t sensor_init(void);
/**
 * @brief Take common IRQ lock to protect interrupt configuration and sensor_irq_handler
 *
 * @param[in] timeout_ms: Maximum block time to wait for the lock in ms
 *
 * @return sensor return codes
 */
int32_t sensor_take_irq_lock(uint32_t timeout_ms);
/**
 * @brief Release common IRQ lock.
 *
 * @return sensor return codes
 */
int32_t sensor_give_irq_lock(void);
/**
 * @brief Unregister IRQ handler of a spcified line.
 *
 * @param[in] dev_irq_id: Device IRQ identifier received from sensor_request_irq.
 *
 * @param[in] irq_line: Connected GPIO ID.
 *
 * @return sensor return codes
 */
int32_t sensor_free_irq(uint8_t dev_irq_id, GPIO_Id irq_line);
/**
 * @brief Register IRQ handler of a spcified line.
 *
 * @param[in] irq_line: Connected GPIO ID.
 *
 * @param[in] irq_handler: IRQ handler function to received notification of an interrupt.
 * This callback will be trigger on sensor task context.
 *
 * @param[in] user_param: User parameter for irq_handler.
 *
 * @param[out] dev_irq_id: Registered device IRQ ID can be used to sensor_free_irq.
 *
 * @return sensor return codes
 */
int32_t sensor_request_irq(GPIO_Id irq_line, sensor_irq_handler irq_handler, void *user_param,
                           uint8_t *dev_irq_id);

/**
 * @brief Sensor driver register I2C device
 *
 * @param[in] bus_id: I2C bus ID. Could be I2C0_BUS or I2C1_BUS depends on HW layout.
 *
 * @param[in] dev_id: Sensor I2C address.
 *
 * @param[in] dev_id_mode: Slave address mode.
 *
 * @param[out] w_fp: Write function pointer for I2C write operation.
 *
 * @param[out] r_fp: Read function pointer for I2C read operation
 *
 * @return handle with type @ref sensor_i2c_handle_t for I2C read/write operations.
 */
sensor_i2c_handle_t sensor_register_i2c_dev(I2C_BusId bus_id, uint32_t dev_id,
                                            sensor_i2c_id_mode_e dev_id_mode, sensor_write_fp *w_fp,
                                            sensor_read_fp *r_fp);
/** @} sensor_lib */
#ifdef __cplusplus
}
#endif
#endif
