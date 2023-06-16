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
 * @file DRV_I2C.h
 */

#ifndef DRV_I2C_H_
#define DRV_I2C_H_

/*Stardard Header */
#include <stdbool.h>
#include <stdint.h>

/**
 * @defgroup drv_i2c I2C Low-Level Driver
 * @{
 */

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/**
 * @defgroup drv_i2c_constants I2C Constants
 * @{
 */

/* I2C Operation Speed */
#define I2C_BUS_SPEED_STANDARD (100)   /**< Standard Speed (100kHz) */
#define I2C_BUS_SPEED_FAST (400)       /**< Fast Speed     (400kHz) */
#define I2C_BUS_SPEED_FAST_PLUS (1000) /**< Fast+ Speed    (  1MHz) */
#define I2C_BUS_SPEED_HIGH (3400)      /**< High Speed     (3.4MHz) */

#define I2C_10BIT_INDICATOR \
  (0x0400UL) /**< An bitwise indicator to represent a 10-bit slave address */

/* I2C Transaction Event */
#define I2C_EVENT_TRANSACTION_DONE (1UL << 0)       /**< Transactiond done */
#define I2C_EVENT_TRANSACTION_INCOMPLETE (1UL << 1) /**< Transaction incomplete */
#define I2C_EVENT_TRANSACTION_ADDRNACK (1UL << 2)   /**< Transaction address NACK */
#define I2C_EVENT_TRANSACTION_BUS_CLEAR (1UL << 3)  /**< Bus cleared */
#define I2C_EVENT_TRANSACTION_DIRCHG (1UL << 4)     /**< Direction changed during transaction */

/** @} drv_i2c_constants */

/****************************************************************************
 * Data Type
 ****************************************************************************/
/**
 * @defgroup drv_i2c_types I2C Types
 * @{
 */

/**
 * @brief Enumeration of I2C return status.
 */
typedef enum {
  DRV_I2C_OK = 0x0U,         /**< Operation succeeded */
  DRV_I2C_ERROR_UNKNOWN,     /**< Unknown/Unspecified error */
  DRV_I2C_ERROR_BUSY,        /**< Operation busy */
  DRV_I2C_ERROR_TIMEOUT,     /**< Operation timeout */
  DRV_I2C_ERROR_UNSUPPORTED, /**< Operation not supported */
  DRV_I2C_ERROR_PARAMETER,   /**< Parameter error */
  DRV_I2C_ERROR_SPECIFIC,    /**< Specific mapping */
  DRV_I2C_ERROR_UNINIT,      /**< Driver not yet initialized */
  DRV_I2C_ERROR_INIT,        /**< Driver already initialized */
  DRV_I2C_ERROR_START,       /**< START sequence error */
  DRV_I2C_ERROR_STOP,        /**< STOP sequence error */
  DRV_I2C_ERROR_HNACK,       /**< Slave high address NACK */
  DRV_I2C_ERROR_LNACK,       /**< Slave low address NACK */
  DRV_I2C_ERROR_TX,          /**< Transmit error */
  DRV_I2C_ERROR_RX,          /**< Receive error */
  DRV_I2C_ERROR_RXFIFO,      /**< RX FIFO error */
  DRV_I2C_ERROR_PWRCFG,      /**< Power configuration error */
  DRV_I2C_ERROR_PWRMNGR      /**< Power management operation error */
} DRV_I2C_Status;

/**
 * @brief Enumeration of I2C bus.
 */
typedef enum {
  I2C_BUS_0, /**< I2C bus 0 */
  I2C_BUS_1, /**< I2C bus 1 */
} I2C_BusId;

/**
 * @brief Enumeration of I2C power mode.
 */
typedef enum {
  I2C_POWER_OFF,  /**< I2C power off */
  I2C_POWER_FULL, /**< I2C work with full power */
} I2C_PowerMode;

/**
 * @brief Definition of I2C peripheral structure.
 */
/* Opaqued peripheral definition */
struct Drv_I2C_Peripheral;
typedef struct Drv_I2C_Peripheral Drv_I2C_Peripheral; /**< Structure of peripheral handle */

/**
 * @brief Definition of I2C event notification callback.
 */
typedef struct {
  void *user_data;                                       /**< User data on notification callback */
  void (*event_callback)(void *user_data, uint32_t evt); /**< Event notification callback */
} Drv_I2C_EventCallback;

/** @} drv_i2c_types */

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/
/*! @cond Doxygen_Suppress */
#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif
/*! @endcond */

/**
 * @defgroup drv_i2c_apis I2C APIs
 * @{
 */

/**
 * @brief Initialize I2C low level driver.
 *
 * @param [in] bus_id: Bus ID of the target I2C peripheral.
 * @param [in] evt_cb: A structure pointer to specify the event callback. see @ref
 * Drv_I2C_EventCallback.
 * @param [out] peripheral: A pointer to obtain the peripheral handle.
 * @return DRV_I2C_OK returned on success, error status returned on failure, see @ref
 * DRV_I2C_Status.
 */
DRV_I2C_Status DRV_I2C_Initialize(I2C_BusId bus_id, Drv_I2C_EventCallback *evt_cb,
                                  Drv_I2C_Peripheral **peripheral);

/**
 * @brief Uninitialize I2C low level driver.
 *
 * @param bus_id: Bus ID of the target I2C peripheral.
 * @return DRV_I2C_OK returned on success, error status returned on failure, see @ref
 * DRV_I2C_Status.
 */
DRV_I2C_Status DRV_I2C_Uninitialize(I2C_BusId bus_id);

/**
 * @brief Set I2C clock speed of SCL.
 *
 * @param [in] i2c: Handle of target peripheral.
 * @param [in] speed_khz: Target speed of peripheral.
 * @return DRV_I2C_OK returned on success, error status returned on failure, see @ref
 * DRV_I2C_Status.
 */
DRV_I2C_Status DRV_I2C_SetSpeed(Drv_I2C_Peripheral *i2c, uint32_t speed_khz);

/**
 * @brief Start the non-blocking data transmission as a master. On succeeded, the event
 * I2C_EVENT_TRANSACTION_DONE will sent, otherwise, I2C_EVENT_TRANSACTION_INCOMPLETE/
 * I2C_EVENT_TRANSACTION_ADDRNACK will sent.
 *
 * @param [in] i2c: Handle of target peripheral.
 * @param [in] addr: The address of target slave device.
 * @param [in] data: The data buffer to transmit to slave.
 * @param [in] num: The number of the data bytes to transmit.
 * @param [in] xfer_pending: Transfer operation is pending - Stop condition will not be generated.
 * @return DRV_I2C_OK returned on success, error status returned on failure, see @ref
 * DRV_I2C_Status.
 */
DRV_I2C_Status DRV_I2C_MasterTransmit_Nonblock(Drv_I2C_Peripheral *i2c, uint32_t addr,
                                               const uint8_t *data, uint32_t num,
                                               bool xfer_pending);
/**
 * @brief Start the non-blocking data reception as a master. . On succeeded, the event
 * I2C_EVENT_TRANSACTION_DONE will sent, otherwise, I2C_EVENT_TRANSACTION_INCOMPLETE/
 * I2C_EVENT_TRANSACTION_ADDRNACK will sent.
 *
 * @param [in] i2c: Handle of target peripheral.
 * @param [in] addr: The address of target slave device.
 * @param [in] data: The data buffer to receive from slave.
 * @param [in] num: The number of the data bytes to receive.
 * @param xfer_pending
 * @return DRV_I2C_OK returned on success, error status returned on failure, see @ref
 * DRV_I2C_Status.
 */
DRV_I2C_Status DRV_I2C_MasterReceive_Nonblock(Drv_I2C_Peripheral *i2c, uint32_t addr,
                                              const uint8_t *data, uint32_t num, bool xfer_pending);

/**
 * @brief Start the blocking data transmission as a master.
 *
 * @param [in] i2c: Handle of target peripheral.
 * @param [in] addr: The address of target slave device.
 * @param [in] data: The data buffer to transmit to slave.
 * @param [in] num: The number of the data bytes to transmit.
 * @param [in] xfer_pending: Transfer operation is pending - Stop condition will not be generated.
 * @return DRV_I2C_OK returned on success, error status returned on failure, see @ref
 * DRV_I2C_Status.
 */
DRV_I2C_Status DRV_I2C_MasterTransmit_Block(Drv_I2C_Peripheral *i2c, uint32_t addr,
                                            const uint8_t *data, uint32_t num, bool xfer_pending);
/**
 * @brief Start the blocking data reception as a master.
 *
 * @param [in] i2c: Handle of target peripheral.
 * @param [in] addr: The address of target slave device.
 * @param [in] data: The data buffer to receive from slave.
 * @param [in] num: The number of the data bytes to receive.
 * @param xfer_pending
 * @return DRV_I2C_OK returned on success, error status returned on failure, see @ref
 * DRV_I2C_Status.
 */
DRV_I2C_Status DRV_I2C_MasterReceive_Block(Drv_I2C_Peripheral *i2c, uint32_t addr,
                                           const uint8_t *data, uint32_t num, bool xfer_pending);

/**
 * @brief Perform the power control operation
 *
 * @param [in] i2c: Handle of target peripheral.
 * @param [in] mode: The power mode, also see @ref I2C_PowerMode.
 * @return DRV_I2C_OK returned on success, error status returned on failure, see @ref
 * DRV_I2C_Status.
 */
DRV_I2C_Status DRV_I2C_PowerControl(Drv_I2C_Peripheral *i2c, I2C_PowerMode mode);

/**
 * @brief Get Transferred/Received data count of the lastest transaction.
 *
 * @param [in] i2c: Handle of target peripheral.
 * @return Number of data bytes transferred/receive, negative error status returned on failure, see
 * @ref drv_i2c_constants..
 */
int32_t DRV_I2C_GetDataCount(Drv_I2C_Peripheral *i2c);

/**
 * @brief Clear the I2C bus by a forcing STOP condition. After bus cleared, the event
 * I2C_EVENT_TRANSACTION_BUS_CLEAR will sent if the event callback hooked on
 * DRV_I2C_Initialize() called.
 *
 * @param [in] i2c: Handle of target peripheral.
 * @return DRV_I2C_OK returned on success, error status returned on failure, see @ref
 * DRV_I2C_Status.
 */
DRV_I2C_Status DRV_I2C_ClearBus(Drv_I2C_Peripheral *i2c);

/**
 * @brief Attempt to abort an ongoing I2C transaction
 *
 * @param [in] i2c: Handle of target peripheral.
 * @return DRV_I2C_OK returned on success, error status returned on failure, see @ref
 * DRV_I2C_Status.
 */
DRV_I2C_Status DRV_I2C_AbortTransfer(Drv_I2C_Peripheral *i2c);

/** @} drv_i2c_apis */
/*! @cond Doxygen_Suppress */
#undef EXTERN
#ifdef __cplusplus
}
#endif
/*! @endcond */

/** @} drv_i2c */

#endif /* DRV_I2C_H_ */
