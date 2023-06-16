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
 * @file DRV_SPI.h
 */

#ifndef DRV_SPI_H_
#define DRV_SPI_H_

/****************************************************************************
 * Included Files
 ****************************************************************************/
/*Stardard Header */
#include <stdint.h>
#include <stdbool.h>

/**
 * @defgroup drv_spi SPI Low-Level Driver
 * @{
 */
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/**
 * @defgroup drv_spi_constants SPI Constants
 * @{
 */
#define SPI_EVENT_TRANSFER_COMPLETE (1UL << 0) /**< Event: Transfer Complete event mask.*/
#define SPI_EVENT_DATA_LOST (1UL << 1)         /**< Event: Data Lost event mask.*/
#define SPI_EVENT_MODE_FAULT (1UL << 2)        /**< Event: Mode Fault event mask.*/

/** @} drv_spi_constants */

/****************************************************************************
 * Data Type
 ****************************************************************************/
/**
 * @defgroup drv_spi_types SPI Types
 * @{
 */

/** @brief SPI status error codes.*/
typedef enum drv_spi_status {
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
  SPI_ERROR_BUS_ID,       /**< Invalid bus id. */
  SPI_ERROR_DEFAULT_CFG,  /**< Wrong default SPI configuration. */
  SPI_ERROR_BUSY,         /**< SPI busy. */
  SPI_ERROR_MODE_FAULT,   /**< SPI mode error. */
  SPI_ERROR_NOT_INIT,     /**< SPI is uninitialized. */
  SPI_ERROR_INITIALED,    /**< SPI is initialized. */
  SPI_ERROR_INVALID_LEN,  /**< Invalid length. */
  SPI_ERROR_WRONG_PARAM,  /**< Wrong parameter. */
  SPI_ERROR_TIMEOUT,      /**< Transfer timeout. */
  SPI_ERROR_GENERIC       /**< Other errors. */
} DRV_SPI_Status;

/**
 * @brief SPI power mode.
 */
typedef enum {
  SPI_POWER_OFF,  /**< SPI power off */
  SPI_POWER_FULL, /**< SPI work with full power */
} SPI_PowerMode;

/** @brief SPI bus number.*/
typedef enum spi_bus_id {
  SPI_BUS_0 = 0, /**< SPI master (SPIM) bus 0. */
  SPI_BUS_1 = 1, /**< SPI master (SPIM) bus 1. */
  SPI_BUS_2 = 2, /**< SPI slave (SPIS) bus 0. */
  SPI_BUS_MAX    /**< total number of SPI devices. */
} SPI_Bus_Id;

/** @brief SPI master peripheral Slave Select number.*/
typedef enum spi_ss_id {
  SPIM_SS_1 = 0, /**< SPI master Slave Select 0. 2 SPI Masters support 1 CS (H/w) each. */
  SPIM_SS_2,     /**< SPI master Slave Select 1. Only available with S/W CS. */
  SPIM_SS_3      /**< SPI master Slave Select 2. Only available with S/W CS. */
} SPI_SS_Id;

/** @brief SPI clock phase. 0 = even phase exchange ; 1 = odd phase exchange.*/
typedef enum spi_clock_phase {
  SPI_CPHA_0 = 0, /**< CPHA=0. even phase exchange.*/
  SPI_CPHA_1 = 1  /**< CPHA=1. odd phase exchange.*/
} SPI_Clock_Phase;

/** @brief SPI clock polarity. 0 = idle low; 1 = idle high*/
typedef enum spi_clock_polarity {
  SPI_CPOL_0 = 0, /**< CPOL=0. Active-high(idle low) SPI clock.*/
  SPI_CPOL_1 = 1  /**< CPOL=1. Active-low(idle high) SPI clock.*/
} SPI_Clock_Polarity;

/** @brief SPI data endian. 0 = big endian. 1 = little endian ; Big endian - MsB to LsB ; Little
 * endian - LsB to MsB */
typedef enum spi_endian {
  SPI_BIG_ENDIAN,   /**< Data transfer start from most significant bit.*/
  SPI_LITTLE_ENDIAN /**< Data transfer start from least significant bit.*/
} SPI_Endian;

/** @brief Slave select (CS) active mode. */
typedef enum spi_ss_mode {
  SPI_SS_ACTIVE_LOW, /**< SPI Slave Select(SS) active low.*/
  SPI_SS_ACTIVE_HIGH /**< SPI Slave Select(SS) active high. Important note: SPI Master hardware
                        (DEFAULT) doesn't support this mode.*/
} SPI_SS_Mode;

/** @brief Bit order. */
typedef enum spi_bit_order {
  SPI_BIT_ORDER_MSB_FIRST, /**< SPI bit order MSB first.*/
  SPI_BIT_ORDER_LSB_FIRST  /**< SPI bit order LSB first.*/
} SPI_Bit_Order;

/**
 * @brief Definition of parameters for SPI configuration mode structure.
 */
typedef struct {
  SPI_Clock_Phase cpha;    /**< Clock phase.*/
  SPI_Clock_Polarity cpol; /**< Clock polarity.*/
  uint32_t data_bits;      /**< Bits per frame, minimum 1, maximum 32.*/
  SPI_Bit_Order bit_order; /**< Bit order for SPI.*/
  uint32_t bus_speed;      /**< Bus speed for SPI.*/
  SPI_Endian endian; /**< MSB or LSB data shift direction - 0 = big endian. 1 = little endian ;
                        Big endian - MsB to LsB ; Little endian - LsB to MsB.*/

  SPI_SS_Mode ss_mode; /**< Slave select (CS) mode. Set the active mode = Low/High.*/
} SPI_Param;

/**
 * @brief Definition of parameters for SPI Master configuration structure.
 */
typedef struct {
  SPI_Bus_Id bus_id; /**< The SPI bus number. */
  SPI_Param param;   /**< SPI param configuration. */
} SPI_Config;

/**
 * @brief Definition of spi interrupt callback function.
 */
typedef void (*DRV_SPI_EventCallback)(SPI_Bus_Id bus_id, uint32_t spi_events, void *user_data);

/**
 * @brief Definitions of SPI handle structure.
 */
typedef struct {
  DRV_SPI_EventCallback callback; /**< Callback. */
  void *user_data;                /**< Callback user data. */
} SPI_Handle;

/**
 * @brief Definitions of SPI transfer structure.
 */
typedef struct {
  volatile SPI_SS_Id slave_id; /**< SPI slave id. */
  const uint8_t *send_buf;     /**< Send buffer. */
  uint8_t *recv_buf;           /**< Receive buffer. */
  volatile uint16_t len;       /**< Transfer bytes. */
} SPI_Transfer;

/**
 * @brief Definitions of SPI current status.
 */
typedef struct {
  uint8_t busy;      /**< Transmitter/Receiver busy flag. */
  uint8_t data_lost; /**< Data lost: Receive overflow / Transmit underflow (cleared on start of
                        transfer operation). */
  uint8_t
      mode_fault; /**< Mode fault detected; optional (cleared on start of transfer operation). */
} SPI_Status;

/** @} drv_spi_types */

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

// functions list
/**
 * @defgroup drv_spi_apis SPI Master APIs
 * @{
 */
/**
 * @brief Initialize SPI low level driver.
 *
 * @param [in] bus_id: Bus ID of the target SPI peripheral.
 * @return On success, SPI_ERROR_NONE is returned. On failure,
 * DRV_SPI_Status is returned, see @ref DRV_SPI_Status.
 */
DRV_SPI_Status DRV_SPI_Initialize(SPI_Bus_Id bus_id);

/**
 * @brief Uninitialize SPI low level driver.
 *
 * @param bus_id: Bus ID of the target SPI peripheral.
 * @return On success, SPI_ERROR_NONE is returned. On failure,
 * DRV_SPI_Status is returned, see @ref DRV_SPI_Status.
 */
DRV_SPI_Status DRV_SPI_Uninitialize(SPI_Bus_Id bus_id);

/**
 * @brief Helper function to get SPI initialization parameters from MCU wizard.
 * Get default configuration from MCU wizard.
 *
 * @param [in] bus_id: SPI bus ID.
 * @param [out] *pConfig: Pointer to configured parameters.
 *
 * @return On success, SPI_ERROR_NONE is returned. On failure,
 * DRV_SPI_Status is returned, see @ref DRV_SPI_Status.
 */
DRV_SPI_Status DRV_SPI_GetDefConf(SPI_Bus_Id bus_id, SPI_Config *pConfig);

/**
 * @brief configure SPI and initialize SPI conmponent.
 *
 * @param [in] *pConfig: Pointer to configured parameters.
 * @param [in] ss_num: Slave Select number.
 *
 * @return On success, SPI_ERROR_NONE is returned. On failure,
 * DRV_SPI_Status is returned, see @ref DRV_SPI_Status.
 *
 * @note SPI Master mode doesn't support the configuration - SPI_SS_ACTIVE_HIGH.
 */
DRV_SPI_Status DRV_SPI_Configure(const SPI_Config *pConfig, SPI_SS_Id ss_num);

/**
 * @brief Open handle and register the callback function for data transfer.
 *
 * @param [in] bus_id: SPI bus id.
 * @param [in] *handle: Pointer to the handle.
 * @param [in] *callback: Pointer to callback function.
 * @param [in] *user_data: Pointer to user data.
 *
 * @return On success, SPI_ERROR_NONE is returned. On failure,
 * DRV_SPI_Status is returned, see @ref DRV_SPI_Status.
 */
DRV_SPI_Status DRV_SPI_OpenHandle(SPI_Bus_Id bus_id, SPI_Handle *handle,
                                  DRV_SPI_EventCallback callback, void *user_data);

/**
 * @brief Start receiving data from SPI receiver.
 *
 * @param [in] bus_id: SPI bus id.
 * @param [in] *transfer: Pointer to transmission data.
 *
 * @return On success, SPI_ERROR_NONE is returned. On failure,
 * DRV_SPI_Status is returned, see @ref DRV_SPI_Status.
 *
 * @note The API return when receiving is done.
 * In slave mode, specified receive data length MUST be multiples of 4 bytes.
 */
DRV_SPI_Status DRV_SPI_Receive(SPI_Bus_Id bus_id, SPI_Transfer *transfer);

/**
 * @brief Start sending data to SPI transmitter.
 *
 * @param [in] bus_id: SPI bus id.
 * @param [in] *transfer: Pointer to transmission data.
 *
 * @return On success, SPI_ERROR_NONE is returned. On failure,
 * DRV_SPI_Status is returned, see @ref DRV_SPI_Status.
 *
 * @note The API return when sending is done.
 * In slave mode, specified send data length MUST be multiples of 4 bytes.
 */
DRV_SPI_Status DRV_SPI_Send(SPI_Bus_Id bus_id, SPI_Transfer *transfer);

/**
 * @brief Enable or disable SPI.
 *
 * @param [in] bus_id: SPI bus id.
 * @param [in] enable: true -enable; faluse -diable.
 *
 * @return None.
 */
void DRV_SPI_Enable(SPI_Bus_Id bus_id, bool enable);

/**
 * @brief Inactive SPI.
 *
 * @param [in] bus_id: SPI bus id.
 *
 * @return None.
 */
void DRV_SPI_ModeInactive(SPI_Bus_Id bus_id);

/**
 * @brief Get or set SPI bus speed.
 *
 * @param [in] bus_id: SPI bus id.
 * @param [in] ss_num: CFG number.
 * @param [in] bus_speed: bus speed to be configured. Only be applied when opcode is 1(set).
 * @param [in] opcode: 1: set ;otherwise: get.
 *
 * @return current bus speed; return 0 once set operation is failed.
 */
uint32_t DRV_SPI_ConfigureBusSpeed(SPI_Bus_Id bus_id, const int32_t ss_num, uint32_t bus_speed,
                                   uint32_t opcode);

/**
 * @brief Get the count of data transmission.
 *
 * @param [in] bus_id: SPI bus id.
 *
 * @return data count.
 */
uint32_t DRV_SPI_GetDataCount(SPI_Bus_Id bus_id);

/**
 * @brief Get the SPI status.
 *
 * @param [in] bus_id: SPI bus id.
 * @param [out] *status: SPI status.
 *
 * @return On success, SPI_ERROR_NONE is returned. On failure,
 * DRV_SPI_Status is returned, see @ref DRV_SPI_Status.
 */
DRV_SPI_Status DRV_SPI_GetStatus(SPI_Bus_Id bus_id, SPI_Status *status);

/**
 * @brief Perform the power control operation
 *
 * @param [in] bus_id: SPI bus id.
 * @param [in] mode: The power mode, also see @ref SPI_PowerMode.
 * @return SPI_ERROR_NONE returned on success, error status returned on failure, see @ref
 * DRV_SPI_Status.
 */
DRV_SPI_Status DRV_SPI_PowerControl(SPI_Bus_Id bus_id, SPI_PowerMode mode);

/** @} drv_spi_apis */
/*! @cond Doxygen_Suppress */
#undef EXTERN
#ifdef __cplusplus
}
#endif
/*! @endcond */

/** @} drv_spi */
#endif /* DRV_SPI_H_ */
