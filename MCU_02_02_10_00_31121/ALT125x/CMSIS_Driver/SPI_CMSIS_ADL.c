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

/*****************************************************************************
 * Includes
 *****************************************************************************/
#include <string.h>

/* CMSIS-Driver Headers */
#include "Driver_SPI.h"

/* Low-Level Driver Headers */
#include "DRV_SPI.h"

/*****************************************************************************
 * Pre-processor Definitions
 *****************************************************************************/
#define ARM_SPI_DRV_VERSION ARM_DRIVER_VERSION_MAJOR_MINOR(1, 0) /* driver version */

#define DECLARE_SPI_ADL_INSTANCE_FUNC_WRAPPER(n, inst)                                          \
  static ARM_DRIVER_VERSION SPI_ADL_##n##_GetVersion(void) { return SPI_ADL_GetVersion(inst); } \
  static ARM_SPI_CAPABILITIES SPI_ADL_##n##_GetCapabilities(void) {                             \
    return SPI_ADL_GetCapabilities(inst);                                                       \
  }                                                                                             \
  static int32_t SPI_ADL_##n##_Initialize(ARM_SPI_SignalEvent_t cb_event) {                     \
    return SPI_ADL_Initialize(inst, cb_event);                                                  \
  }                                                                                             \
  static int32_t SPI_ADL_##n##_Uninitialize(void) { return SPI_ADL_Uninitialize(inst); }        \
  static int32_t SPI_ADL_##n##_PowerControl(ARM_POWER_STATE state) {                            \
    return SPI_ADL_PowerControl(inst, state);                                                   \
  }                                                                                             \
  static int32_t SPI_ADL_##n##_Send(const void *data, uint32_t num) {                           \
    return SPI_ADL_Send(inst, data, num);                                                       \
  }                                                                                             \
  static int32_t SPI_ADL_##n##_Receive(void *data, uint32_t num) {                              \
    return SPI_ADL_Receive(inst, data, num);                                                    \
  }                                                                                             \
  static int32_t SPI_ADL_##n##_Transfer(const void *data_out, void *data_in, uint32_t num) {    \
    return SPI_ADL_Transfer(inst, data_out, data_in, num);                                      \
  }                                                                                             \
  static uint32_t SPI_ADL_##n##_GetDataCount(void) { return SPI_ADL_GetDataCount(inst); }       \
  static int32_t SPI_ADL_##n##_Control(uint32_t control, uint32_t arg) {                        \
    return SPI_ADL_Control(inst, control, arg);                                                 \
  }                                                                                             \
  static ARM_SPI_STATUS SPI_ADL_##n##_GetStatus(void) { return SPI_ADL_GetStatus(inst); }

#define DECLARE_SPI_ADL_INSTANCE_FUNC(n) DECLARE_SPI_ADL_INSTANCE_FUNC_WRAPPER(n, SPI_BUS_##n)

#define DECLARE_SPI_ADL_HOOK_WRAPPER(n)                                                \
  {                                                                                    \
    SPI_ADL_##n##_GetVersion, SPI_ADL_##n##_GetCapabilities, SPI_ADL_##n##_Initialize, \
        SPI_ADL_##n##_Uninitialize, SPI_ADL_##n##_PowerControl, SPI_ADL_##n##_Send,    \
        SPI_ADL_##n##_Receive, SPI_ADL_##n##_Transfer, SPI_ADL_##n##_GetDataCount,     \
        SPI_ADL_##n##_Control, SPI_ADL_##n##_GetStatus                                 \
  }

#define DECLARE_SPI_ADL_HOOK(n) DECLARE_SPI_ADL_HOOK_WRAPPER(n)

/* Current driver status flag definition */
#define SPI_INITIALIZED (1 << 0)  // SPI initialized
#define SPI_POWERED (1 << 1)      // SPI powered on
#define SPI_CONFIGURED (1 << 2)   // SPI configured
#define SPI_DATA_LOST (1 << 3)    // SPI data lost occurred
#define SPI_MODE_FAULT (1 << 4)   // SPI mode fault occurred
/****************************************************************************
 * Private Data Types
 ****************************************************************************/
/* SPI Control Information */
typedef struct {
  ARM_SPI_SignalEvent_t cb_event;  ///< Event callback
  ARM_SPI_STATUS status;           ///< Status flags
  uint8_t state;                   ///< Current state
  uint32_t mode;
} SPI_Ctrl_Info;

/* SPI Resource Information */
typedef struct {
  SPI_Ctrl_Info ctrl_info;
} SPI_Resource;

/*****************************************************************************
 * Private Data
 *****************************************************************************/
/* Driver Version */
static const ARM_DRIVER_VERSION DriverVersion = {ARM_SPI_API_VERSION, ARM_SPI_DRV_VERSION};

/* Driver Capabilities */
static const ARM_SPI_CAPABILITIES DriverCapabilities = {
    0, /* Reserved (must be zero) */
    0, /* TI Synchronous Serial Interface */
    0, /* Microwire Interface */
    0, /* Signal Mode Fault event: \ref ARM_SPI_EVENT_MODE_FAULT */
    0  /* Reserved (must be zero) */
};

static SPI_Resource spi0_resource = {0};
#ifdef ALT1250
static SPI_Resource spi1_resource = {0};
#endif
static SPI_Resource spi2_resource = {0};

static SPI_Handle spi_event_handle;

/*****************************************************************************
 * Private function prototypes
 *****************************************************************************/
/* interrupts declaration */
/* Prototypes- SPI callback */
void SPI_ADL_EventCallback(SPI_Bus_Id bus_id, uint32_t SPI_events, void *user_data);

/*****************************************************************************
 * Private Functions
 *****************************************************************************/
static SPI_Resource *SPI_ADL_GetResource(SPI_Bus_Id bus_id) {
  SPI_Resource *spi = NULL;

  switch (bus_id) {
    case SPI_BUS_0:
      spi = &spi0_resource;
      break;

#ifdef ALT1250
    case SPI_BUS_1:
      spi = &spi1_resource;
      break;
#endif
    case SPI_BUS_2:
      spi = &spi2_resource;
      break;
    default:
      spi = NULL;
      break;
  }

  return spi;
}

void SPI_ADL_EventCallback(SPI_Bus_Id bus_id, uint32_t SPI_events, void *user_data) {
  uint32_t event;
  SPI_Resource *spi = SPI_ADL_GetResource(bus_id);

  event = 0;
  /* handle ISR if needed */
  switch (SPI_events) {
    case SPI_EVENT_TRANSFER_COMPLETE:
      event |= ARM_SPI_EVENT_TRANSFER_COMPLETE;
      break;
    case SPI_EVENT_DATA_LOST:
      event |= ARM_SPI_EVENT_DATA_LOST;
      break;
    case SPI_EVENT_MODE_FAULT:
      event |= ARM_SPI_EVENT_MODE_FAULT;
      break;
    default:
      event |= ARM_SPI_EVENT_TRANSFER_COMPLETE;
      break;
  }

  if (event && (spi != NULL) && (spi->ctrl_info.cb_event != NULL)) {
    spi->ctrl_info.cb_event(event);
  }
}
/****************************************************************************
 * Driver Interface Functions
 ****************************************************************************/
static ARM_DRIVER_VERSION SPI_ADL_GetVersion(SPI_Bus_Id bus_id) { return DriverVersion; }

static ARM_SPI_CAPABILITIES SPI_ADL_GetCapabilities(SPI_Bus_Id bus_id) {
  return DriverCapabilities;
}

static int32_t SPI_ADL_Initialize(SPI_Bus_Id bus_id, ARM_SPI_SignalEvent_t cb_event) {
  DRV_SPI_Status ret_val;
  SPI_Resource *spi;
  spi = SPI_ADL_GetResource(bus_id);

  if (!spi) {
    printf("Unsupported bus ID %d\n", (int)bus_id);
    return ARM_DRIVER_ERROR_PARAMETER;
  }

  if (spi->ctrl_info.state & SPI_INITIALIZED) {
    printf("SPI: already initialized!\r\n");
    return ARM_DRIVER_ERROR;
  }

  ret_val = DRV_SPI_Initialize(bus_id);
  if (ret_val != SPI_ERROR_NONE) {
    printf("\n\r Failed to initialize SPI. error-%d", ret_val);
    return ARM_DRIVER_ERROR;
  }

  /* register callback */
  if (bus_id == SPI_BUS_0)
    DRV_SPI_OpenHandle(bus_id, &spi_event_handle, SPI_ADL_EventCallback, NULL);
  else if (bus_id == SPI_BUS_1)
    DRV_SPI_OpenHandle(bus_id, &spi_event_handle, SPI_ADL_EventCallback, NULL);
  else if (bus_id == SPI_BUS_2)
    DRV_SPI_OpenHandle(bus_id, &spi_event_handle, SPI_ADL_EventCallback, NULL);
  else
    return ARM_DRIVER_ERROR;

  /* Hook callback */
  spi->ctrl_info.cb_event = cb_event;

  spi->ctrl_info.state = SPI_INITIALIZED;  // SPI is initialized

  return ARM_DRIVER_OK;
}

static int32_t SPI_ADL_Uninitialize(SPI_Bus_Id bus_id) {
  SPI_Resource *spi;
  spi = SPI_ADL_GetResource(bus_id);
  if (!spi) {
    printf("Unsupported bus ID %d\n", (int)bus_id);
    return ARM_DRIVER_ERROR_PARAMETER;
  }

  spi->ctrl_info.state = 0U;
  return ARM_DRIVER_OK;
}

static int32_t SPI_ADL_PowerControl(SPI_Bus_Id bus_id, ARM_POWER_STATE state) {
  SPI_Resource *spi;
  spi = SPI_ADL_GetResource(bus_id);
  if (!spi) {
    printf("Unsupported bus ID %d\n", (int)bus_id);
    return ARM_DRIVER_ERROR_PARAMETER;
  }

  switch (state) {
    case ARM_POWER_OFF:
      spi->ctrl_info.state &= ~SPI_POWERED;  // SPI is not powered
      break;
    case ARM_POWER_FULL:
      if ((spi->ctrl_info.state & SPI_INITIALIZED) == 0U) {
        return ARM_DRIVER_ERROR;
      }
      if ((spi->ctrl_info.state & SPI_POWERED) != 0U) {
        return ARM_DRIVER_OK;
      }
      spi->ctrl_info.state |= SPI_POWERED;  // SPI is powered
      break;
    case ARM_POWER_LOW:
    default:
      return ARM_DRIVER_ERROR_UNSUPPORTED;
  }

  return ARM_DRIVER_OK;
}

static int32_t SPI_ADL_Send(SPI_Bus_Id bus_id, const void *data, uint32_t num) {
  SPI_Transfer transfer_data;
  SPI_Resource *spi;
  int ret_val = 0;

  spi = SPI_ADL_GetResource(bus_id);
  if (!spi) {
    printf("Unsupported bus ID %d\n", (int)bus_id);
    return ARM_DRIVER_ERROR_PARAMETER;
  }

  if ((data == NULL) || (num == 0U)) {
    return ARM_DRIVER_ERROR_PARAMETER;
  }

  if (!(spi->ctrl_info.state & SPI_CONFIGURED)) {
    return ARM_DRIVER_ERROR;
  }

  if (spi->ctrl_info.status.busy) {
    return ARM_DRIVER_ERROR_BUSY;
  }

  spi->ctrl_info.status.busy = 1U;

  memset(&transfer_data, 0, sizeof(SPI_Transfer));
  transfer_data.slave_id = SPIM_SS_1;
  transfer_data.send_buf = data;
  transfer_data.recv_buf = NULL;
  transfer_data.len = num;

  if ((ret_val = DRV_SPI_Send(bus_id, &transfer_data)) <= 0) {
    printf("\n\r Failed to Send data. error:%d!!", ret_val);
    spi->ctrl_info.status.busy = 0;
    return ARM_DRIVER_ERROR;
  }
  spi->ctrl_info.status.busy = 0;

  return ARM_DRIVER_OK;
}

static int32_t SPI_ADL_Receive(SPI_Bus_Id bus_id, void *data, uint32_t num) {
  SPI_Transfer transfer_data;
  SPI_Resource *spi;
  int ret_val = 0;

  spi = SPI_ADL_GetResource(bus_id);
  if (!spi) {
    printf("Unsupported bus ID %d\n", (int)bus_id);
    return ARM_DRIVER_ERROR_PARAMETER;
  }

  if ((data == NULL) || (num == 0U)) {
    return ARM_DRIVER_ERROR_PARAMETER;
  }
  if (!(spi->ctrl_info.state & SPI_CONFIGURED)) {
    return ARM_DRIVER_ERROR;
  }
  if (spi->ctrl_info.status.busy) {
    return ARM_DRIVER_ERROR_BUSY;
  }
  spi->ctrl_info.status.busy = 1U;

  memset(&transfer_data, 0, sizeof(SPI_Transfer));
  transfer_data.slave_id = SPIM_SS_1;
  transfer_data.send_buf = NULL;
  transfer_data.recv_buf = data;
  transfer_data.len = num;

  if ((ret_val = DRV_SPI_Receive(bus_id, &transfer_data)) < 0) {
    printf("\n\r Failed to Transfer data. error:%d!!", ret_val);
    spi->ctrl_info.status.busy = 0;
    return ARM_DRIVER_ERROR;
  }

  spi->ctrl_info.status.busy = 0;
  return ARM_DRIVER_OK;
}

static int32_t SPI_ADL_Transfer(SPI_Bus_Id bus_id, const void *data_out, void *data_in,
                                uint32_t num) {
  return ARM_DRIVER_ERROR_UNSUPPORTED;
}

static uint32_t SPI_ADL_GetDataCount(SPI_Bus_Id bus_id) {
  uint32_t cnt;
  SPI_Resource *spi = SPI_ADL_GetResource(bus_id);
  if (!spi) {
    printf("Unsupported bus ID %d\n", (int)bus_id);
    return ARM_DRIVER_ERROR_PARAMETER;
  }

  if (!(spi->ctrl_info.state & SPI_CONFIGURED)) {
    return 0U;
  }

  cnt = DRV_SPI_GetDataCount(bus_id);

  return cnt;
}

static int32_t SPI_ADL_Control(SPI_Bus_Id bus_id, uint32_t control, uint32_t arg) {
  SPI_Resource *spi = SPI_ADL_GetResource(bus_id);
  if (!spi) {
    printf("Unsupported bus ID %d\n", (int)bus_id);
    return ARM_DRIVER_ERROR_PARAMETER;
  }

  if (!(spi->ctrl_info.state & SPI_POWERED)) {
    return ARM_DRIVER_ERROR;
  }

  // not allow to configure when device is busy
  if (spi->ctrl_info.status.busy) {
    return ARM_DRIVER_ERROR_BUSY;
  }

  switch (control & ARM_SPI_CONTROL_Msk) {
    default:
      return ARM_DRIVER_ERROR_UNSUPPORTED;

    case ARM_SPI_MODE_INACTIVE:  // SPI Inactive
      DRV_SPI_ModeInactive(bus_id);

      spi->ctrl_info.mode &= ~ARM_SPI_CONTROL_Msk;
      spi->ctrl_info.mode |= ARM_SPI_MODE_INACTIVE;

      spi->ctrl_info.state &= ~SPI_CONFIGURED;
      return ARM_DRIVER_OK;

    case ARM_SPI_MODE_MASTER:  // SPI Master (Output on MOSI, Input on MISO); arg = Bus Speed in bps
      spi->ctrl_info.state |= SPI_CONFIGURED;
      return ARM_DRIVER_OK;

    case ARM_SPI_MODE_SLAVE:  // SPI Slave  (Output on MISO, Input on MOSI)
      return ARM_DRIVER_ERROR_UNSUPPORTED;

    case ARM_SPI_SET_BUS_SPEED:  // Set Bus Speed in bps; arg = value
      if (DRV_SPI_ConfigureBusSpeed(bus_id, 1, arg, 1) != 0)
        return ARM_DRIVER_OK;
      else
        return ARM_DRIVER_ERROR;

    case ARM_SPI_GET_BUS_SPEED:  // Get Bus Speed in bps
      return DRV_SPI_ConfigureBusSpeed(bus_id, 1, 0, 0);

    case ARM_SPI_SET_DEFAULT_TX_VALUE:  // Set default Transmit value; arg = value
      return ARM_DRIVER_ERROR_UNSUPPORTED;

    case ARM_SPI_CONTROL_SS:  // Control Slave Select; arg = 0:inactive, 1:active
      if (((spi->ctrl_info.mode & ARM_SPI_CONTROL_Msk) != ARM_SPI_MODE_MASTER) ||
          ((spi->ctrl_info.mode & ARM_SPI_SS_MASTER_MODE_Msk) != ARM_SPI_SS_MASTER_SW)) {
        return ARM_DRIVER_ERROR;
      }
      /*      if (ssp->pin.ssel == NULL) {
              return ARM_DRIVER_ERROR;
            }*/
      if (arg == ARM_SPI_SS_INACTIVE) {
        // Implement it till new GPIO APIs ready
      } else {
        // Implement it till new GPIO APIs ready
      }
      return ARM_DRIVER_OK;

    case ARM_SPI_ABORT_TRANSFER:  // Abort current data transfer
      return ARM_DRIVER_ERROR_UNSUPPORTED;
  }
  return ARM_DRIVER_OK;
}

static ARM_SPI_STATUS SPI_ADL_GetStatus(SPI_Bus_Id bus_id) {
  ARM_SPI_STATUS status = {0};
  SPI_Status cur_status = {0};

  if (DRV_SPI_GetStatus(bus_id, &cur_status) == SPI_ERROR_NONE) {
    status.busy = cur_status.busy;
    status.data_lost = cur_status.data_lost;
    status.mode_fault = cur_status.mode_fault;
  }
  return (status);
}

/*****************************************************************************
 * SPI Interface Hook unctions
 *****************************************************************************/

#ifdef ALT1250
/* Master SPI0, SPI1; Slave SPI2 */
DECLARE_SPI_ADL_INSTANCE_FUNC(0)
ARM_DRIVER_SPI Driver_SPI0 = DECLARE_SPI_ADL_HOOK(0);
DECLARE_SPI_ADL_INSTANCE_FUNC(1)
ARM_DRIVER_SPI Driver_SPI1 = DECLARE_SPI_ADL_HOOK(1);
DECLARE_SPI_ADL_INSTANCE_FUNC(2)
ARM_DRIVER_SPI Driver_SPI2 = DECLARE_SPI_ADL_HOOK(2);
#elif defined(ALT1255)
/* Master SPI0; Slave SPI2 */
DECLARE_SPI_ADL_INSTANCE_FUNC(0)
ARM_DRIVER_SPI Driver_SPI0 = DECLARE_SPI_ADL_HOOK(0);
DECLARE_SPI_ADL_INSTANCE_FUNC(2)
ARM_DRIVER_SPI Driver_SPI2 = DECLARE_SPI_ADL_HOOK(2);
#else
#error No target platform specified!!
#endif
