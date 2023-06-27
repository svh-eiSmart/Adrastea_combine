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
 * @file spi.h
 */

#ifndef SPI_ALT125X_H_
#define SPI_ALT125X_H_

/**
 * @defgroup alt125x_spi SPI Driver
 * @details ALT1250X SPI drivers (Master and Slave)
 * @{
 */

/**
 * @defgroup spim_drv SPI Master Driver
 * @details None
 * @{
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "FreeRTOS.h"
#include <stdbool.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/**
 * @defgroup spim_constants SPI Master Constants
 * @{
 */
#ifdef ALT1255
#define MAX_SPIM_DEVICES_QUANT (1) /**< Maximum 1255 SPI Master devices */
#define MAX_SPIM0_SS_QUANT (3)     /**< Maximum slave quantity for SPI Master 0 */
#else
#define MAX_SPIM_DEVICES_QUANT (2) /**< Maximum 1250 SPI Master devices */
#define MAX_SPIM0_SS_QUANT (3)     /**< Maximum slave quantity for SPI Master 0 */
#define MAX_SPIM1_SS_QUANT (3)     /**< Maximum slave quantity for SPI Master 1 */
#endif

#define SPI_EVENT_TRANSFER_COMPLETE (0x1) /**< Event: Transfer Complete event mask.*/
#define SPI_EVENT_DATA_LOST (0x2)         /**< Event: Data Lost event mask.*/
#define SPI_EVENT_MODE_FAULT (0x4)        /**< Event: Mode Fault event mask.*/

/** @} spim_constants */

/**
 * @defgroup spim_types SPI Master Types
 * @{
 */

/** @brief SPI error codes.*/
typedef enum SPI_ERROR_CODE {
  SPI_ERROR_NONE,         /**< No error. */
  SPI_ERROR_MODE,         /**< Invalid mode. */
  SPI_ERROR_FRAME_FORMAT, /**< Invalid frame format. */
  SPI_ERROR_DATA_BITS,    /**< Invalid data bits. */
  SPI_ERROR_BIT_ORDER,    /**< Invalid bit order. */
  SPI_ERROR_BUS_SPEED,    /**< Invalid bus speed. */
  SPI_ERROR_SS_MODE,      /**< Invalid SS mode. */
  SPI_ERROR_SS_NUM,       /**< Invalid Slave number. */
  SPI_ERROR_ENDIAN,       /**< Invalid endian. */
  SPI_ERROR_SS_PIN,       /**< Invalid SS pin. */
  SPI_ERROR_GENERIC       /**< Other errors. */
} SPI_ERROR_CODE;

/** @brief SPI status error codes.*/
typedef enum SPI_STATUS_ERR_CODE {
  SPI_STATUS_ERR_NONE,        /**< No error. */
  SPI_STATUS_ERR_BUSY,        /**< SPI busy. */
  SPI_STATUS_ERR_DATA_LOST,   /**< SPI data lost. */
  SPI_STATUS_ERR_MODE_FAULT,  /**< SPI mode error. */
  SPI_STATUS_ERR_NOT_INIT,    /**< SPI configuration is not set. */
  SPI_STATUS_ERR_INVALID_LEN, /**< Invalid length. */
  SPI_STATUS_ERR_WRONG_PARAM, /**< Wrong parameter. */
  SPI_STATUS_ERR_TIMEOUT,     /**< Transfer timeout. */
  SPI_STATUS_ERR_GENERIC      /**< Other errors. */
} SPI_STATUS_ERROR_CODE;

/** @brief SPI master bus number.*/
typedef enum spi_bus_t {
  SPIM0_BUS = 0, /**< SPI master (SPIM) bus 0. */
  SPIM1_BUS = 1  /**< SPI master (SPIM) bus 1. */
} spi_bus_t;

/** @brief SPI master peripheral Slave Select number.*/
typedef enum spi_ss_t {
  SPIM_SS_1 = 0, /**< SPI master Slave Select 0. 2 SPI Masters support 1 CS (H/w) each. */
  SPIM_SS_2,     /**< SPI master Slave Select 1. Only available with S/W CS. */
  SPIM_SS_3      /**< SPI master Slave Select 2. Only available with S/W CS. */
} spi_ss_t;

/** @brief SPI clock phase. 0 = even phase exchange ; 1 = odd phase exchange.*/
typedef enum _spi_clock_phase_t {
  SPI_CPHA_0 = 0, /**< CPHA=0. even phase exchange.*/
  SPI_CPHA_1 = 1  /**< CPHA=1. odd phase exchange.*/
} spi_clock_phase_t;

/** @brief SPI clock polarity. 0 = idle low; 1 = idle high*/
typedef enum _spi_clock_polarity_t {
  SPI_CPOL_0 = 0, /**< CPOL=0. Active-high(idle low) SPI clock.*/
  SPI_CPOL_1 = 1  /**< CPOL=1. Active-low(idle high) SPI clock.*/
} spi_clock_polarity_t;

/** @brief SPI data endian. 0 = big endian. 1 = little endian ; Big endian - MsB to LsB ; Little
 * endian - LsB to MsB */
typedef enum _spi_endian_t {
  SPI_BIG_ENDIAN,   /**< Data transfer start from most significant bit.*/
  SPI_LITTLE_ENDIAN /**< Data transfer start from least significant bit.*/
} spi_endian_t;

/** @brief SPI mode. SPI Master, SPI Slave */
typedef enum _spi_mode_t {
  SPI_MODE_MASTER, /**< SPI master.*/
  SPI_MODE_SLAVE   /**< SPI slave.*/
} spi_mode_t;

/** @brief Slave select (CS) active mode. */
typedef enum _spi_ss_mode_t {
  SPI_SS_ACTIVE_LOW, /**< SPI Slave Select(SS) active low.*/
  SPI_SS_ACTIVE_HIGH /**< SPI Slave Select(SS) active high. Important note: SPI Master hardware
                        (DEFAULT) doesn't support this mode.*/
} spi_ss_mode_t;

/** @brief Bit order. */
typedef enum _spi_bit_order_t {
  SPI_BIT_ORDER_MSB_FIRST, /**< SPI bit order MSB first.*/
  SPI_BIT_ORDER_LSB_FIRST  /**< SPI bit order LSB first.*/
} spi_bit_order_t;

/** @brief Slave select (CS) active mode. */
typedef enum _spi_ss_signal_manage_t {
  SPI_SS_SIGNAL_BY_DEFAULT, /**< SPI Slave Select(SS) signal is managed by hardware.*/
  SPI_SS_SIGNAL_BY_DRV,     /**< SPI Slave Select(SS) signal is managed by SPI driver. */
  SPI_SS_SIGNAL_BY_APP      /**< SPI Slave Select(SS) signal is managed by application. */
} spi_ss_signal_manage_t;

/**
 * @brief spi_param_config_t
 * Definition of parameters for SPI configuration mode structure.
 */
typedef struct _spi_param_config_t {
  spi_clock_phase_t cpha;    /**< Clock phase.*/
  spi_clock_polarity_t cpol; /**< Clock polarity.*/
  uint32_t dataBits;         /**< Bits per frame, minimum 1, maximum 32.*/
  spi_bit_order_t bitOrder;  /**< Bit order for SPI.*/
  uint32_t busSpeed;         /**< Bus speed for SPI.*/
  spi_endian_t endian; /**< MSB or LSB data shift direction - 0 = big endian. 1 = little endian ;
                        Big endian - MsB to LsB ; Little endian - LsB to MsB.*/

  spi_ss_mode_t ssMode; /**< Slave select (CS) mode. Set the active mode = Low/High.*/
  spi_ss_signal_manage_t ssSignalMgmt; /**< Slave select (CS) signal management. */
  uint16_t gpioId; /**< gpio_pin id for CS. This parameter is available when ssSignalMgmt is
                       configured to SPI_SS_SIGNAL_BY_DRV and SPI_SS_SIGNAL_BY_APP.*/
} spi_param_config_t;

/**
 * @brief spi_config_t
 * Definition of parameters for SPI Master configuration structure.
 */
typedef struct _spi_config_t {
  spi_mode_t spiMode;       /**< The SPI mode. */
  spi_bus_t spiBusId;       /**< The SPI bus number. */
  spi_param_config_t param; /**< SPI param configuration. */
} spi_config_t;

/**
 * @typedef spi_event_callback
 * Definition of spi interrupt callback function
 * User callback function type for spi interrupt.
 * @param [out] spi_bus_id: spi bus id.
 * @param [out] SPI_events: Event indication bitmask.
 * @param [in] *userData : Parameter for this callback.
 */
typedef void (*spi_event_callback)(spi_bus_t spi_bus_id, unsigned int SPI_events, void *userData);

/**
 * @brief spim_handle_t
 * Definitions of SPI handle structure.
 */
typedef struct _spim_handle_t {
  spi_event_callback callback; /**< Callback. */
  void *userData;              /**< Callback user data. */
} spim_handle_t;

/**
 * @brief spi_transfer_t
 * Definitions of SPI transfer structure.
 */
typedef struct _spi_transfer_t {
  volatile spi_ss_t slaveId; /**< SPI slave id. */
  uint8_t *sendData;         /**< Send buffer. */
  uint8_t *recvData;         /**< Receive buffer. */
  volatile uint16_t len;     /**< Transfer bytes. */
} spi_transfer_t;

/** @} spim_types */

// functions list
/**
 * @defgroup spim_funcs SPI Master APIs
 * @{
 */
/**
 * @brief Helper function to get SPI initialization parameters from MCU wizard.
 * Get default configuration from MCU wizard.
 *
 * @param [in] bus_id: SPIM bus ID.
 * @param [out] *pConfig: Pointer to configured parameters.
 *
 * @return On success, SPI_ERROR_NONE is returned. On failure,
 * SPI_ERROR_CODES is returned.
 */
SPI_ERROR_CODE spi_get_def_config(spi_bus_t bus_id, spi_config_t *pConfig);

/**
 * @brief configure SPI and initialize SPI conmponent.
 *
 * @param [in] *pConfig: Pointer to configured parameters.
 * @param [in] ss_num: Slave Select number.
 *
 * @return On success, transferred data count is returned. On failure,
 * SPI_ERROR_CODES is returned.
 *
 * @note SPI Master mode doesn't support the configuration - SPI_SS_ACTIVE_HIGH.
 */
SPI_ERROR_CODE spi_configure(const spi_config_t *pConfig, spi_ss_t ss_num);

/**
 * @brief Open handle and register the callback function for data transfer.
 *
 * @param [in] bus_id: SPI Master bus id.
 * @param [in] *handle: Pointer to the handle.
 * @param [in] *callback: Pointer to callback function.
 * @param [in] *user_data: Pointer to user data.
 *
 * @return On success, return 0; Otherwise, return a error (negative) value.
 */
int spi_open_handle(spi_bus_t bus_id, spim_handle_t *handle, spi_event_callback callback,
                    void *user_data);

/**
 * @brief Start receiving data from SPI receiver.
 *
 * @param [in] spi_bus_id: SPI Master bus id.
 * @param [in] *transfer: Pointer to transmission data.
 *
 * @return On success, transferred data count is returned. On failure,
 * SPI_ERROR_CODES(negative value) is returned.
 *
 * @note The API return when receiving is done.
 */
int spi_receive(spi_bus_t spi_bus_id, spi_transfer_t *transfer);

/**
 * @brief Start receiving data from SPI receiver.
 *
 * @param [in] spi_bus_id: SPI Master bus id.
 * @param [in] *transfer: Pointer to transmission data.
 *
 * @return On success, transferred data count is returned. On failure,
 * SPI_ERROR_CODES(negative value) is returned.
 *
 * @note This API is nonblocking, so user application has to register a handle/callback for the
 * subsequent processing.
 */
int spi_receive_nonblock(spi_bus_t spi_bus_id, spi_transfer_t *transfer);

/**
 * @brief Start sending data to SPI transmitter.
 *
 * @param [in] spi_bus_id: SPI Master bus id.
 * @param [in] *transfer: Pointer to transmission data.
 *
 * @return On success, transferred data count is returned. On failure,
 * SPI_ERROR_CODES(negative value) is returned.
 *
 * @note The API return when sending is done.
 */
int spi_send(spi_bus_t spi_bus_id, spi_transfer_t *transfer);

/**
 * @brief Start sending data to SPI transmitter.
 *
 * @param [in] spi_bus_id: SPI Master bus id.
 * @param [in] *transfer: Pointer to transmission data.
 *
 * @return On success, transferred data count is returned. On failure,
 * SPI_ERROR_CODES(negative value) is returned.
 *
 * @note This API is nonblocking, so user application has to register a handle/callback for the
 * subsequent processing.
 */
int spi_send_nonblock(spi_bus_t spi_bus_id, spi_transfer_t *transfer);

/**
 * @brief Start sending/receiving data to/from SPI transmitter/receiver.
 *
 * @param [in] spi_bus_id: SPI Master bus id.
 * @param [in] *transfer: Pointer to transmission data.
 *
 * @return On success, transferred data count is returned. On failure,
 * SPI_ERROR_CODES(negative value) is returned.
 * @note spi_transfer() will return error when configured to SPI_SS_SIGNAL_BY_APP.
 * @note The API return when transfering is done.
 */
int spi_transfer(spi_bus_t spi_bus_id, spi_transfer_t *transfer);

/** @} spim_funcs */
/** @} spim_drv */

/**
 * @defgroup spis_drv SPI Slave Driver
 * @details None
 * @{
 */

/**
 * @defgroup spis_constants SPI Slave Constants
 * @{
 */
#define MAX_SPIS_DEVICES_QUANT (1)         /**< Maximum SPI Slave devices */
#define CONFIG_SPI_MAX_PAYLOAD_SIZE (4096) /**< Maximum SPI Slave payload size */

/** @} spis_constants */

/**
 * @defgroup spis_types SPI Slave Types
 * @{
 */

/** @brief SPI slave error codes.*/
typedef enum SPIS_ERR_CODE {
  SPIS_ERR_NONE,   /**< No error. */
  SPIS_ERR_PERM,   /**< Operation not permitted */
  SPIS_ERR_NOMEM,  /**< Out of memory. */
  SPIS_ERR_INVAL,  /**< Invalid argument. */
  SPIS_ERR_NOSPC,  /**< No space left on device. */
  SPIS_ERR_GENERIC /**< Other errors. */
} SPIS_ERR_CODE;

/** @brief SPI slave status error codes.*/
typedef enum SPIS_STATUS_ERR_CODE {
  SPIS_STATUS_ERR_NONE,        /**< No error. */
  SPIS_STATUS_ERR_CONNABORTED, /**< Software caused connection abort. */
  SPIS_STATUS_ERR_GENERIC      /**< Other errors. */
} SPIS_STATUS_ERROR_CODE;

/** @brief SPI slave bus number.*/
typedef enum spis_bus_t {
  SPIS0_BUS = 0 /**< SPI slave (SPIS) bus 0. */
} spis_bus_t;

/**
 * @brief spi_param_config_t
 * Definition of parameters for SPI configuration mode structure.
 */
typedef struct _spis_param_config_t {
  spi_clock_phase_t cpha;    /**< Clock phase.*/
  spi_clock_polarity_t cpol; /**< Clock polarity.*/
  spi_endian_t endian;  /**< MSB or LSB data shift direction - 0 = big endian. 1 = little endian ;
                         Big endian - MsB to LsB ; Little endian - LsB to MsB.*/
  spi_ss_mode_t ssMode; /**< Slave select (CS) mode. Set the active mode = Low/High.*/
} spis_param_config_t;

/**
 * @brief spi_config_t
 * Definition of parameters for SPI Slave configuration structure.
 */
typedef struct _spis_config_t {
  spis_bus_t spisBusId;      /**< The SPIS bus number. */
  spis_param_config_t param; /**< SPI param configuration. */
} spis_config_t;

/** @} spis_types */

// functions list
/**
 * @defgroup spis_funcs SPI Slave APIs
 * @{
 */
/**
 * @brief Helper function to get SPIS initialization parameters from MCU wizard.
 * Get default configuration from MCU wizard.
 *
 * @param [out] *pConfig: Pointer to configured parameters.
 *
 * @return On success, SPIS_ERROR_NONE is returned. On failure,
 * SPIS_ERR_CODES is returned.
 */
SPIS_ERR_CODE spis_get_def_config(spis_config_t *pConfig);

/**
 * @brief Initialize SPI Slave.
 *
 * @return On success, value 0. On failure,
 * error codes(negative value) is returned.
 */
int spis_init(void);

/**
 * @brief Open SPI Slave.
 *
 * @return manage table pointer or NULL.
 */
void *spis_open(void);

/**
 * @brief Close SPI Slave.
 *
 * @param [in] handle - manage table pointer.
 *
 * @return On success, value 0. On failure,
 * error codes(negative value) is returned.
 */
int spis_close(void *handle);

/**
 * @brief Start sending data to SPI transmitter.
 *
 * @param [in] *handle: spi slave manage table pointer.
 * @param [in] *buf: data to be sent.
 * @param [in] bufSize: Send data length.
 *
 * @return On success, transferred data count is returned. On failure,
 * error codes(negative value) is returned.
 *
 * @note The API return when sending is done. Send data length must be multiples of 4 bytes.
 */
int spis_send(void *handle, const uint8_t *buf, size_t bufSize);

/**
 * @brief Start receiving data from SPI receiver.
 *
 * @param [in] *handle: spi slave manage table pointer.
 * @param [in] *buf: data to be received.
 * @param [in] bufSize: Receive data length.
 *
 * @return On success, received data count is returned. On failure,
 * error codes(negative value) is returned.
 *
 * @note The API return when receiving is done. receive data length must be multiples of 4 bytes.
 */
int spis_receive(void *handle, uint8_t *buf, size_t bufSize);

/** @} spis_funcs */
/** @} spis_drv */
/** @} alt125x_spi */

#endif /* SPI_ALT125X_H_ */
