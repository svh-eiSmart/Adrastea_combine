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

/* Standard Headers */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/* CMSIS-Driver Headers */
#include DEVICE_HEADER

#include "DRV_SPI.h"
#include "DRV_IF_CFG.h"
#include "PIN.h"

#if (configUSE_ALT_SLEEP == 1)
#include "pwr_mngr.h"
#endif
#include "DRV_PM.h"
#include "DRV_SLEEP.h"

/****************************************************************************
 * Definitions
 ****************************************************************************/
//#define _MCU_SPI_DEBUG

#ifdef _MCU_SPI_DEBUG
#define SPIDPRT(format, args...) printf("\r\n[%s:%d] " format, __FILE__, __LINE__, ##args)
#else
#define SPIDPRT(args...)
#endif

#define DIV_ROUND_UP(n, d) (((n) + (d)-1) / (d))
#define ROUND_UP(x, n) (((x) + (n)-1u) & ~((n)-1u))

/* Note: the value 500000~5000000 was tested. */
#define MIN_SPI_BUS_SPEED (500000)
#define MAX_SPI_BUS_SPEED (30000000)

#define SPI_FIFO_SIZE_BYTES (0x80 * 4)

#define SPI_INITIALIZED (1 << 0)  // SPI initialized

/* transfer checking count */
#define SPIS_BUSYWAIT_CHECK_CNT (100000)

/* SPI Slave private mode */
#define SPI_SLAVE_REV_DATA (0x0100)
#define SPI_SLAVE_MISO_IDLE_HIGH (0x0200)

/* SPI Slave private property  */
#define SPI_SLAVE_SCK_IS_ACTIVE (1 << 0)    /* SCK input schmitt active */
#define SPI_SLAVE_MOSI_IS_ACTIVE (1 << 1)   /* MOSI input schmitt active */
#define SPI_SLAVE_MISO_DRIVE_LARGE (1 << 2) /* MISO drive strength large */

#define SPI_SLAVE_DEFAULT_PROPERTY (SPI_SLAVE_SCK_IS_ACTIVE | SPI_SLAVE_MOSI_IS_ACTIVE)

/****************************************************************************
 * Private Types
 ****************************************************************************/
typedef struct spi_ctrl {
  uint8_t state;
  SPI_Status status;
} SPI_Ctrl;

typedef enum spi_bus_mode {
  SPI_MODE_MASTER, /**< SPI master.*/
  SPI_MODE_SLAVE   /**< SPI slave.*/
} SPI_BusMode;

typedef struct spi_transfer_info {
  uint32_t num;     // Total number of transfers
  uint32_t rx_cnt;  // Number of data received
  uint32_t tx_cnt;  // Number of data sent
} SPI_Transfer_Info;

typedef struct drv_spi_peripheral {
  union {
    SPI_MASTER_Type *spim_reg; /* Peripheral Register file */
    SPI_SLAVE_Type *spis_reg;
  } spi_reg;
  SPI_BusMode bus_mode;
  volatile int32_t dev_num;
  IRQn_Type irq_num;
  IRQn_Type err_irq_num;
  const char *interrupt_label;
  SPI_Ctrl ctrl;
  SPI_Transfer_Info xfer;
  SPI_Handle *evt_handle; /* Event callback */
  bool is_init;
} DRV_SPI_Peripheral;

typedef struct spi_isrcnt {
  unsigned long recv_int_done;
  unsigned long recv_int_rdata;
  unsigned long recv_int_wdata;
  unsigned long recv_int_rdata_overflow;
  unsigned long recv_int_rdata_underflow;
  unsigned long recv_int_error;
} SPI_IsrCnt;

/****************************************************************************
 * Private Data
 ****************************************************************************/
static DRV_SPI_Peripheral SpimPeripheral0 = {{.spim_reg = SPI_MASTER0},
                                             SPI_MODE_MASTER,
                                             0,
                                             SPIM0_IRQn,
                                             SPIM0_ERR_IRQn,
                                             "spim0Drv",
                                             {0},
                                             {0},
                                             NULL,
                                             false};

#ifdef ALT1250
static DRV_SPI_Peripheral SpimPeripheral1 = {{.spim_reg = SPI_MASTER1},
                                             SPI_MODE_MASTER,
                                             0,
                                             SPIM1_IRQn,
                                             SPIM1_ERR_IRQn,
                                             "spim1Drv",
                                             {0},
                                             {0},
                                             NULL,
                                             false};
#endif
static DRV_SPI_Peripheral SpisPeripheral0 = {{.spis_reg = SPI_SLAVE},
                                             SPI_MODE_SLAVE,
                                             0,
                                             SPIS0_IRQn,
                                             SPIS0_ERR_IRQn,
                                             "spis0Drv",
                                             {0},
                                             {0},
                                             NULL,
                                             false};
static bool gSpiLastTransfer = false;
static SPI_Config gSpiConfig[SPI_BUS_MAX] = {0};
static SPI_IsrCnt gSpimCounters[SPI_BUS_MAX] = {0};

#if (configUSE_ALT_SLEEP == 1)
static uint32_t SPI_REG_CFG[SPI_BUS_MAX][6] = {0};
static uint8_t spi_registered_sleep_notify = 0;
#endif
/****************************************************************************
 * Function Prototype
 ****************************************************************************/
void spim0_interrupt_handler(void);
#ifdef ALT1250
void spim1_interrupt_handler(void);
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/
size_t min_t(size_t a, size_t b) {
  if (a < b)
    return a;
  else
    return b;
}

static void DRV_SPIM_ResetCounters(SPI_Bus_Id bus_id) {
  gSpimCounters[bus_id].recv_int_done = 0;
  gSpimCounters[bus_id].recv_int_rdata = 0;
  gSpimCounters[bus_id].recv_int_wdata = 0;
  gSpimCounters[bus_id].recv_int_rdata_overflow = 0;
  gSpimCounters[bus_id].recv_int_rdata_underflow = 0;
  gSpimCounters[bus_id].recv_int_error = 0;
}

static DRV_SPI_Peripheral *DRV_SPI_GetPeripheral(SPI_Bus_Id bus_id) {
  switch (bus_id) {
    case SPI_BUS_0:
      return &SpimPeripheral0;
#ifdef ALT1250
    case SPI_BUS_1:
      return &SpimPeripheral1;
#endif
    case SPI_BUS_2:
      return &SpisPeripheral0;
    default:
      printf("\r\nCriticial Error: spi bus_id %d is not valid!", bus_id);
      return NULL;
  }
}

#ifdef _MCU_SPI_DEBUG
static void DRV_SPI_DumpCounters(SPI_Bus_Id bus_id) {
  printf("\r\n====== Counters =======");
  printf("\r\n DONE: %lu", gSpimCounters[bus_id].recv_int_done);
  printf("\r\n RDATA: %lu", gSpimCounters[bus_id].recv_int_rdata);
  printf("\r\n WDATA: %lu", gSpimCounters[bus_id].recv_int_wdata);
  printf("\r\n RDATA Overflow: %lu", gSpimCounters[bus_id].recv_int_rdata_overflow);
  printf("\r\n RDATA Underflow: %lu", gSpimCounters[bus_id].recv_int_rdata_underflow);
  printf("\r\n Error: %lu", gSpimCounters[bus_id].recv_int_error);
  printf("\r\n=======================\n");
}

static void DRV_SPI_DumpState(SPI_Bus_Id bus_id) {
  uint32_t reg_val;

  if (bus_id == SPI_BUS_0 || bus_id == SPI_BUS_1) {
    DRV_SPI_Peripheral *spi = DRV_SPI_GetPeripheral(bus_id);

    printf("\r\n Bus Id: %.8d", bus_id);
    reg_val = spi->spi_reg.spim_reg->CMD_FIFO_STATUS;
    printf("\r\n CMD_FIFO_STATUS: %.8lx", reg_val);

    reg_val = spi->spi_reg.spim_reg->WDATA_FIFO_STATUS;
    printf("\r\n WDATA_FIFO_STATUS: %.8lx", reg_val);

    reg_val = spi->spi_reg.spim_reg->RDATA_FIFO_STATUS;
    printf("\r\n RDATA_FIFO_STATUS: %.8lx", reg_val);

    reg_val = spi->spi_reg.spim_reg->LAST_EXE_CMD;
    printf("\r\n LAST_EXE_CMD: %.8lx", reg_val);

    reg_val = spi->spi_reg.spim_reg->INTERRUPT_CFG1;
    printf("\r\n INTERRUPT_CFG1: %.8lx", reg_val);

    reg_val = spi->spi_reg.spim_reg->INTERRUPT_CFG2;
    printf("\r\n INTERRUPT_CFG2: %.8lx", reg_val);

    reg_val = spi->spi_reg.spim_reg->INTERRUPT_STATUS;
    printf("\r\n INTERRUPT_STATUS: %.8lx", reg_val);

    reg_val = spi->spi_reg.spim_reg->CURRENT_STATE;
    printf("\r\n CURRENT_STATE: %.8lx", reg_val);

    reg_val = spi->spi_reg.spim_reg->CS1_CFG;
    printf("\r\n CS1_CFG: %.8lx", reg_val);

    reg_val = spi->spi_reg.spim_reg->CS2_CFG;
    printf("\r\n CS2_CFG: %.8lx", reg_val);

    reg_val = spi->spi_reg.spim_reg->CS3_CFG;
    printf("\r\n CS3_CFG: %.8lx", reg_val);
  }
}
#endif

static void DRV_SPI_FlushFifo(DRV_SPI_Peripheral *spi, uint32_t mask) {
  if (spi->bus_mode == SPI_MODE_MASTER) {
    spi->spi_reg.spim_reg->FLUSH_CFG |= mask;
  }
}

static void DRV_SPI_EnableInterrupts(DRV_SPI_Peripheral *spi, uint32_t mask) {
  if (spi->bus_mode == SPI_MODE_MASTER) {
    spi->spi_reg.spim_reg->INTERRUPT_CFG2 &= ~mask;
  }
}

static void DRV_SPI_DisableInterrupts(DRV_SPI_Peripheral *spi, uint32_t mask) {
  if (spi->bus_mode == SPI_MODE_MASTER) {
    spi->spi_reg.spim_reg->INTERRUPT_CFG2 |= mask;
  }
}

static uint32_t DRV_SPIM_SetCfgReg(DRV_SPI_Peripheral *spi, const int32_t ss_num, uint32_t set_val,
                                   uint8_t mode) {
  if (mode == 1) {  // set
    if (ss_num == 1)
      spi->spi_reg.spim_reg->CS2_CFG = set_val;
    else if (ss_num == 2)
      spi->spi_reg.spim_reg->CS3_CFG = set_val;
    else
      spi->spi_reg.spim_reg->CS1_CFG = set_val;
  }

  if (ss_num == 1)
    return spi->spi_reg.spim_reg->CS2_CFG;
  else if (ss_num == 2)
    return spi->spi_reg.spim_reg->CS3_CFG;
  else
    return spi->spi_reg.spim_reg->CS1_CFG;
}

static void DRV_SPI_RestoreState(SPI_Bus_Id bus_id) {
  DRV_SPIM_ResetCounters(bus_id);
  gSpiLastTransfer = false;
  return;
}

static void DRV_SPIM_SetParams(const int32_t ss_num, const SPI_Config *masterConfig) {
  uint32_t reg_val, div;
  DRV_SPI_Peripheral *spi = DRV_SPI_GetPeripheral(masterConfig->bus_id);

  reg_val = DRV_SPIM_SetCfgReg(spi, ss_num, 0, 0);
  SPIDPRT("\r\n MCU_SPI_MASTER1_CS1_CFG [%lx]", reg_val);

  /* infinite mode */
  reg_val |= SPI_MASTER0_CS1_CFG_INFINITE_CS1_Msk;

  /* Set parameters */
  reg_val &= ~SPI_MASTER0_CS1_CFG_CPHA_CS1_Msk;
  if (masterConfig->param.cpha) reg_val |= SPI_MASTER0_CS1_CFG_CPHA_CS1_Msk;

  reg_val &= ~SPI_MASTER0_CS1_CFG_CPOL_CS1_Msk;
  if (masterConfig->param.cpol) reg_val |= SPI_MASTER0_CS1_CFG_CPOL_CS1_Msk;

  /* endian */
  reg_val &= ~SPI_MASTER0_CS1_CFG_ENDIAN_CS1_Msk;
  if (masterConfig->param.endian) reg_val |= SPI_MASTER0_CS1_CFG_ENDIAN_CS1_Msk;

  /* data bits */
  reg_val &= ~SPI_MASTER0_CS1_CFG_WS_CS1_Msk;
  reg_val |= (masterConfig->param.data_bits << SPI_MASTER0_CS1_CFG_WS_CS1_Pos);

  /* bit order */
  reg_val &= ~SPI_MASTER0_CS1_CFG_REVERSAL_CS1_Msk;
  if (masterConfig->param.bit_order == SPI_BIT_ORDER_MSB_FIRST)
    reg_val |= (0 << SPI_MASTER0_CS1_CFG_REVERSAL_CS1_Pos);
  else
    reg_val |= (1 << SPI_MASTER0_CS1_CFG_REVERSAL_CS1_Pos);

  /* data speed */
  reg_val &= ~SPI_MASTER0_CS1_CFG_COUNT_DIVIDE_CS1_Msk;
  div = DIV_ROUND_UP(SystemCoreClock, masterConfig->param.bus_speed);
  SPIDPRT("\r\n SystemCoreClock [%ld] speed[%lu] div[%lu]", SystemCoreClock,
          masterConfig->param.bus_speed, div);

  if (div == 2) div++;
  if (div > 255) div = 255;

  reg_val |= (div << SPI_MASTER0_CS1_CFG_COUNT_DIVIDE_CS1_Pos);

  SPIDPRT("\r\n ss_num [%ld] MCU_SPI_MASTERX_CSX_CFG [%lx]", ss_num, reg_val);

  DRV_SPIM_SetCfgReg(spi, ss_num, reg_val, 1);
#ifdef _MCU_SPI_DEBUG
  DRV_SPI_DumpState(masterConfig->bus_id);
#endif
}

static void DRV_SPIM_IrqHandler(SPI_Bus_Id bus_id) {
  DRV_SPI_Peripheral *spi = DRV_SPI_GetPeripheral(bus_id);
  uint32_t reg_val, event_mask = 0;

  reg_val = spi->spi_reg.spim_reg->INTERRUPT_STATUS_RC;
  SPIDPRT("\r\n got bus_id %d interrupt: %ld last %d", bus_id, reg_val, gSpiLastTransfer);

  if (reg_val & SPI_MASTER0_INTERRUPT_STATUS_RC_DONE_C_INT_Msk) {
    spi->xfer.tx_cnt = spi->xfer.num;
    gSpimCounters[bus_id].recv_int_done++;
    if ((spi->evt_handle != 0) && (gSpiLastTransfer == true)) {
      event_mask = SPI_EVENT_TRANSFER_COMPLETE;
      gSpiLastTransfer = false;
      if (spi->evt_handle->callback != 0)
        spi->evt_handle->callback(bus_id, event_mask, spi->evt_handle->user_data);
    }
  }

  if (reg_val & SPI_MASTER0_INTERRUPT_STATUS_RC_RDATA_STATUS_C_INT_Msk) {
    spi->xfer.rx_cnt = spi->xfer.num;
    gSpimCounters[bus_id].recv_int_rdata++;
    spi->spi_reg.spim_reg->INTERRUPT_CFG2_b.M_RDATA = 1;
  }

  if (reg_val & SPI_MASTER0_INTERRUPT_STATUS_RC_WDATA_STATUS_C_INT_Msk) {
    gSpimCounters[bus_id].recv_int_wdata++;
    spi->spi_reg.spim_reg->INTERRUPT_CFG2_b.M_WDATA = 1;
  }

  if (reg_val & SPI_MASTER0_INTERRUPT_STATUS_RC_RDATA_OVER_C_INT_Msk) {
    if (spi->evt_handle != 0 && spi->evt_handle->callback != 0) {
      event_mask = SPI_EVENT_DATA_LOST;
      spi->evt_handle->callback(bus_id, event_mask, spi->evt_handle->user_data);
    }
    gSpimCounters[bus_id].recv_int_rdata_overflow++;
    spi->spi_reg.spim_reg->INTERRUPT_CFG2_b.M_OVERFLOW_RDATA = 1;
  }

  if (reg_val & SPI_MASTER0_INTERRUPT_STATUS_RC_WDATA_UNDER_C_INT_Msk) {
    if (spi->evt_handle != 0 && spi->evt_handle->callback != 0) {
      event_mask = SPI_EVENT_DATA_LOST;
      spi->evt_handle->callback(bus_id, event_mask, spi->evt_handle->user_data);
    }
    gSpimCounters[bus_id].recv_int_rdata_underflow++;
    spi->spi_reg.spim_reg->INTERRUPT_CFG2_b.M_UNDERFLOW_WDATA = 1;
  }

  if ((reg_val & SPI_MASTER0_INTERRUPT_STATUS_RC_ERR_WDATA_OVER_CMD_EMPTY_Msk) |
      (reg_val & SPI_MASTER0_INTERRUPT_STATUS_RC_ERR_RDATA_UNDR_CMD_EMPTY_Msk) |
      (reg_val & SPI_MASTER0_INTERRUPT_STATUS_RC_ERR_CMD_OVER_WDATA_EMPTY_Msk) |
      (reg_val & SPI_MASTER0_INTERRUPT_STATUS_RC_ERR_CMD_OVER_RDATA_FULL_Msk) |
      (reg_val & SPI_MASTER0_INTERRUPT_STATUS_RC_ERR_SW_WRITE_RDATA_Msk) |
      (reg_val & SPI_MASTER0_INTERRUPT_STATUS_RC_ERR_SW_READ_WDATA_Msk)) {
    if (spi->evt_handle != 0 && spi->evt_handle->callback != 0) {
      event_mask = SPI_EVENT_DATA_LOST;
      spi->evt_handle->callback(bus_id, event_mask, spi->evt_handle->user_data);
    }
    gSpimCounters[bus_id].recv_int_rdata_underflow++;
    spi->ctrl.status.data_lost = 1U;
    spi->spi_reg.spim_reg->INTERRUPT_CFG2_b.M_UNDERFLOW_WDATA = 1;
  }

  // Clear interrupt cause
  reg_val = spi->spi_reg.spim_reg->INTERRUPT_STATUS_RC;

  spi->ctrl.status.busy = 0;
}

void DRV_SPIS_IsrHandler(SPI_Bus_Id bus_id) {
  DRV_SPI_Peripheral *spi = DRV_SPI_GetPeripheral(bus_id);
  volatile uint32_t slaveIntStatusReg, slaveIntMaskReg;
  uint32_t event_mask = 0;

  slaveIntStatusReg = spi->spi_reg.spis_reg->RD_CLR_INT;

  SPIDPRT("\r\n got bus_id %d interrupt: %ld", bus_id, slaveIntStatusReg);

  /* Disable the corresponding interrupt source(s) */
  slaveIntMaskReg = spi->spi_reg.spis_reg->MASK_INT;
  SPIDPRT("\r\n ####### Recv interrupt[%lu]#######", slaveIntMaskReg);
  slaveIntMaskReg |= slaveIntStatusReg;
  spi->spi_reg.spis_reg->MASK_INT = slaveIntMaskReg;

  spi->xfer.rx_cnt = spi->xfer.num;

  // handle user callback
  event_mask = SPI_EVENT_TRANSFER_COMPLETE;
  if (spi->evt_handle != 0 && spi->evt_handle->callback != 0) {
    spi->evt_handle->callback(bus_id, event_mask, spi->evt_handle->user_data);
  }
  spi->ctrl.status.busy = 0;
}

void spim0_interrupt_handler(void) { DRV_SPIM_IrqHandler(SPI_BUS_0); }
void spim0_err_irq_handler(void) { DRV_SPIM_IrqHandler(SPI_BUS_0); }
#ifdef ALT1250
void spim1_interrupt_handler(void) { DRV_SPIM_IrqHandler(SPI_BUS_1); }
void spim1_err_irq_handler(void) { DRV_SPIM_IrqHandler(SPI_BUS_1); }
#endif
void spis0_interrupt_handler(void) { DRV_SPIS_IsrHandler(SPI_BUS_2); }

static int32_t DRV_SPIM_BusInit(const SPI_Config *masterConfig) {
  DRV_SPI_Peripheral *spi = DRV_SPI_GetPeripheral(masterConfig->bus_id);

  NVIC_SetPriority(spi->irq_num, 7);
  NVIC_EnableIRQ(spi->irq_num);

  NVIC_SetPriority(spi->err_irq_num, 7);
  NVIC_EnableIRQ(spi->err_irq_num);

  spi->ctrl.status.busy = 0;
  spi->ctrl.status.data_lost = 0;
  spi->ctrl.status.mode_fault = 0;

  // Starts without valid dev
  spi->dev_num = 0;

  // Reset SPI
  DRV_SPI_FlushFifo(spi, SPI_MASTER0_FLUSH_CFG_FLUSH_RDATA_Msk);

  spi->ctrl.state = SPI_INITIALIZED;
  return 0;
}

static int32_t DRV_SPI_ConfigureIO(SPI_Bus_Id bus_id) {
  int32_t ret_val = 0;

  switch (bus_id) {
    case SPI_BUS_0:
      if (DRV_IF_CFG_SetIO(IF_CFG_SPIM0) != DRV_IF_CFG_OK) ret_val = (-1);
      break;
#ifdef ALT1250
    case SPI_BUS_1:
      if (DRV_IF_CFG_SetIO(IF_CFG_SPIM1) != DRV_IF_CFG_OK) ret_val = (-1);
      break;
#endif
    case SPI_BUS_2:
      if (DRV_IF_CFG_SetIO(IF_CFG_SPIS0) != DRV_IF_CFG_OK) ret_val = (-1);
      break;
    default:
      ret_val = (-1);
      break;
  }
  return ret_val;
}

DRV_SPI_Status DRV_SPI_GetDefConf(SPI_Bus_Id bus_id, SPI_Config *pConfig) {
  memset(pConfig, 0x0, sizeof(SPI_Config));
  if (bus_id == SPI_BUS_0) {
    if (DRV_IF_CFG_GetDefConfig(IF_CFG_SPIM0, pConfig) != DRV_IF_CFG_OK) return SPI_ERROR_GENERIC;
  }
#ifdef ALT1250
  else if (bus_id == SPI_BUS_1) {
    if (DRV_IF_CFG_GetDefConfig(IF_CFG_SPIM1, pConfig) != DRV_IF_CFG_OK) return SPI_ERROR_GENERIC;
  }
#endif
  else if (bus_id == SPI_BUS_2) {
    if (DRV_IF_CFG_GetDefConfig(IF_CFG_SPIS0, pConfig) != DRV_IF_CFG_OK) return SPI_ERROR_GENERIC;
  } else {
    return SPI_ERROR_GENERIC;
  }

  return SPI_ERROR_NONE;
}

static DRV_SPI_Status DRV_SPI_CheckParam(const SPI_Config *pConfig, SPI_SS_Id ss_num) {
  /* specific check */
  if (pConfig->bus_id == SPI_BUS_0) {
    if (ss_num > SPIM_SS_3) return SPI_ERROR_SS_NUM;
  }
#ifdef ALT1250
  else if (pConfig->bus_id == SPI_BUS_1) {
    if (ss_num > SPIM_SS_3) return SPI_ERROR_SS_NUM;
  }
#endif

  /* Data bits */
  if ((pConfig->param.bus_speed < MIN_SPI_BUS_SPEED) ||
      (pConfig->param.bus_speed > MAX_SPI_BUS_SPEED))
    return SPI_ERROR_BUS_SPEED;

  /* frame format */
  if ((pConfig->param.cpha != SPI_CPHA_0) && (pConfig->param.cpha != SPI_CPHA_1))
    return SPI_ERROR_FRAME_FORMAT;
  if ((pConfig->param.cpol != SPI_CPOL_0) && (pConfig->param.cpol != SPI_CPOL_1))
    return SPI_ERROR_FRAME_FORMAT;

  /* data bits (1..32) */
  if ((pConfig->param.data_bits == 0) || (pConfig->param.data_bits > 32))
    return SPI_ERROR_DATA_BITS;

  /* bit order */
  if ((pConfig->param.bit_order != SPI_BIT_ORDER_MSB_FIRST) &&
      (pConfig->param.bit_order != SPI_BIT_ORDER_LSB_FIRST))
    return SPI_ERROR_BIT_ORDER;

  /* endian */
  if ((pConfig->param.endian != SPI_BIG_ENDIAN) && (pConfig->param.endian != SPI_LITTLE_ENDIAN))
    return SPI_ERROR_ENDIAN;

  return SPI_ERROR_NONE;
}

/****************************************************************************
 * External Functions
 ****************************************************************************/

DRV_SPI_Status DRV_SPI_Configure(const SPI_Config *pConfig, SPI_SS_Id ss_num) {
  DRV_SPI_Peripheral *spi = DRV_SPI_GetPeripheral(pConfig->bus_id);
  DRV_SPI_Status ret_val;

  ret_val = DRV_SPI_CheckParam(pConfig, ss_num);
  if (ret_val != SPI_ERROR_NONE) {
    return ret_val;
  }

  if (DRV_SPIM_BusInit(pConfig) != SPI_ERROR_NONE) {
    return SPI_ERROR_GENERIC;
  }

  if (DRV_SPI_ConfigureIO(pConfig->bus_id) != 0) {
    return SPI_ERROR_GENERIC;
  }

  if (spi->bus_mode == SPI_MODE_MASTER) DRV_SPIM_SetParams(ss_num, pConfig);

  DRV_SPI_RestoreState(pConfig->bus_id);

  spi->spi_reg.spim_reg->SPI_ENABLE_b.SPI_EN = 1;

  memset(&gSpiConfig[pConfig->bus_id], 0, sizeof(SPI_Config));
  memcpy(&gSpiConfig[pConfig->bus_id], pConfig, sizeof(SPI_Config));

  return SPI_ERROR_NONE;
}

DRV_SPI_Status DRV_SPI_OpenHandle(SPI_Bus_Id bus_id, SPI_Handle *handle,
                                  DRV_SPI_EventCallback callback, void *user_data) {
  DRV_SPI_Peripheral *spi = DRV_SPI_GetPeripheral(bus_id);
  if (handle == NULL) return SPI_ERROR_WRONG_PARAM;

  memset(handle, 0, sizeof(*handle));
  spi->evt_handle = handle;
  handle->callback = callback;
  handle->user_data = user_data;

  return SPI_ERROR_NONE;
}

static DRV_SPI_Status DRV_SPIS_RestoreState(SPI_Bus_Id bus_id, uint32_t mode) {
  DRV_SPI_Peripheral *spi = DRV_SPI_GetPeripheral(bus_id);
  SPI_Config mConfig;
  uint32_t RegVal;
  DRV_SPI_Status ret_val;

  if ((ret_val = DRV_SPI_GetDefConf(bus_id, &mConfig)) != SPI_ERROR_NONE) {
    return SPI_ERROR_DEFAULT_CFG;
  }

  RegVal = spi->spi_reg.spis_reg->CFG;

  /* Set parameters */
  RegVal &= ~SPI_SLAVE_CFG_CPHA_Msk;
  if (mConfig.param.cpha) RegVal |= SPI_SLAVE_CFG_CPHA_Msk;

  RegVal &= ~SPI_SLAVE_CFG_CPOL_Msk;
  if (mConfig.param.cpol) RegVal |= SPI_SLAVE_CFG_CPOL_Msk;

  /* endian */
  RegVal &= ~SPI_SLAVE_CFG_ENDIAN_SWITCH_Msk;
  if (mConfig.param.endian) RegVal |= SPI_SLAVE_CFG_ENDIAN_SWITCH_Msk;

  RegVal &= ~SPI_SLAVE_CFG_SS_ACTIVE_HIGH_Msk;
  if (mConfig.param.ss_mode) RegVal |= SPI_SLAVE_CFG_SS_ACTIVE_HIGH_Msk;

  /* Set parameters */
  RegVal &= ~SPI_SLAVE_CFG_REVERSAL_Msk;
  if (mode & SPI_SLAVE_REV_DATA) RegVal |= SPI_SLAVE_CFG_REVERSAL_Msk;

  RegVal &= ~SPI_SLAVE_CFG_MISO_IDLE_VALUE_Msk;
  if (mode & SPI_SLAVE_MISO_IDLE_HIGH) RegVal |= SPI_SLAVE_CFG_MISO_IDLE_VALUE_Msk;

  RegVal &= ~SPI_SLAVE_CFG_WORD_SIZE_Msk;
  /*cfg |= (host->cur_chip->word_size << SPI_SLAVE_CFG_WORD_SIZE_POS);*/
  RegVal |= (8 << SPI_SLAVE_CFG_WORD_SIZE_Pos);
  RegVal |= SPI_SLAVE_CFG_INFINITE_Msk;

  /* It's up to host to choose clock frequency */
  spi->spi_reg.spis_reg->CFG = (RegVal);

  return SPI_ERROR_NONE;
}

static void DRV_SPIS_ResetController(SPI_Bus_Id bus_id) {
  DRV_SPI_Peripheral *spi = DRV_SPI_GetPeripheral(bus_id);
  uint32_t interrptMask = 0;

  /* Unmask and enable complete interrupt */
  interrptMask = spi->spi_reg.spis_reg->MASK_INT;

  /* Mask all interrupts */
  interrptMask |= (SPI_SLAVE_MASK_INT_MASK_MULTI_PACKET_COMPLETE_INT_Msk |
                   SPI_SLAVE_MASK_INT_MASK_MISO_PACKET_CNT_INT_Msk |
                   SPI_SLAVE_MASK_INT_MASK_MOSI_PACKET_CNT_INT_Msk |
                   SPI_SLAVE_MASK_INT_MASK_MASTER_READY_INT_Msk);

  spi->spi_reg.spis_reg->MASK_INT = interrptMask;
  spi->spi_reg.spis_reg->INT_EN = 0;
}

static DRV_SPI_Status DRV_SPI_SlaveReceive(SPI_Bus_Id bus_id, SPI_Transfer *transfer) {
  DRV_SPI_Peripheral *spi = DRV_SPI_GetPeripheral(bus_id);
  uint16_t rxDataSize = 0;
  uint32_t ocpmBuffActive = 0;
  int32_t totalBufSize;
  uint32_t slaveIntMaskReg;
  volatile uint32_t spiSlaveOcpmBufFifoStatus, spiSlaveOcpmOcpLenStatus;
  int32_t ret = 0, i = 0;

  if (spi->ctrl.status.busy == 1) {
    return SPI_ERROR_BUSY;
  }
  if (!(spi->is_init)) {
    return SPI_ERROR_NOT_INIT;
  }

  if (transfer == NULL) {
    /* Clear the MOSI Atomic Packet Counter */
    spi->spi_reg.spis_reg->MOSI_PAC_AT_CNT = 0;

    return SPI_ERROR_WRONG_PARAM;
  } else {
    rxDataSize = transfer->len;
  }

  spi->xfer.num = transfer->len;
  spi->xfer.rx_cnt = 0;
  spi->xfer.tx_cnt = 0;

  totalBufSize = ROUND_UP(rxDataSize, 4);

  SPIDPRT("\r\n Preparing Rx SPI Buffer (M2S), tota/actuall size=%ld/%d", totalBufSize,
          transfer->len);
  // got_mosi_int = false;
  /* Flush & Reset RD/WR buffers right away before the transaction is started */
  DRV_SPIS_ResetController(bus_id);
  spi->spi_reg.spis_reg->OCPM_INIT = 0xFFFFFFFF;

  spi->ctrl.status.busy = 1U;
  spi->ctrl.status.data_lost = 0U;
  spi->ctrl.status.mode_fault = 0U;

  /* Set DMA address of Rx SPI buffer */
  spi->spi_reg.spis_reg->OCPM_WR_BD_ADDR = (uint32_t)transfer->recv_buf;

  /* Set size of Rx SPI DMA buffer */
  spi->spi_reg.spis_reg->OCPM_WR_BD_SIZE = totalBufSize;

  ocpmBuffActive |= SPI_SLAVE_OCPM_BUFF_ACTIVE_WR_BUFF_ACTIVE_Msk;

  /* Set the whole message as 1 packet (i.e. set the MOSI Atomic Packet Counter to 1 */
  spi->spi_reg.spis_reg->MOSI_PAC_AT_CNT = 1;

  /* Generate interrupt at the end of the Rx buffer reception */
  spi->spi_reg.spis_reg->MOSI_PAC_AT_CNT_CTRL =
      (SPI_SLAVE_MOSI_PAC_AT_CNT_CTRL_BD_CNT_INT_EN_Msk |
       0x3 << SPI_SLAVE_MOSI_PAC_AT_CNT_CTRL_BD_CNT_INT_MODE_Pos |
       SPI_SLAVE_MOSI_PAC_AT_CNT_CTRL_BD_CNT_HW_EN_Msk |
       SPI_SLAVE_MOSI_PAC_AT_CNT_CTRL_BD_CNT_THRS_EN_Msk |
       (0x00 << SPI_SLAVE_MOSI_PAC_AT_CNT_CTRL_BD_CNT_THRS_VALUE_Pos));

  /* Clear pending interrupts */
  spi->spi_reg.spis_reg->RD_CLR_INT;

  /* Enable the MOSI Packet Counter Interrupt */
  slaveIntMaskReg = spi->spi_reg.spis_reg->MASK_INT;
  slaveIntMaskReg &= ~SPI_SLAVE_MASK_INT_MASK_MOSI_PACKET_CNT_INT_Msk;
  spi->spi_reg.spis_reg->MASK_INT = slaveIntMaskReg;

  /* Set the actual packet size */
  spi->spi_reg.spis_reg->CFG_PACKET = ((totalBufSize << SPI_SLAVE_CFG_PACKET_PACKET_SIZE_Pos) |
                                       (1 << SPI_SLAVE_CFG_PACKET_MULTI_PACKET_SIZE_Pos));

  spi->spi_reg.spis_reg->OCPM_BUFF_ACTIVE = ocpmBuffActive;

  while (spi->ctrl.status.busy == 1) {
    ;
  }

  spiSlaveOcpmBufFifoStatus = spi->spi_reg.spis_reg->OCPM_BUF_FIFO_STATUS;
  spiSlaveOcpmOcpLenStatus = spi->spi_reg.spis_reg->OCPM_OCP_LEN_STATUS;

  /* Wait until completion of the whole transaction */
  while ((spiSlaveOcpmBufFifoStatus & (SPI_SLAVE_OCPM_BUF_FIFO_STATUS_OCPM_MOSI_FIFO_STATUS_Msk |
                                       SPI_SLAVE_OCPM_BUF_FIFO_STATUS_OCPM_MISO_FIFO_STATUS_Msk)) ||
         spiSlaveOcpmOcpLenStatus) {
    spiSlaveOcpmBufFifoStatus = spi->spi_reg.spis_reg->OCPM_BUF_FIFO_STATUS;
    spiSlaveOcpmOcpLenStatus = spi->spi_reg.spis_reg->OCPM_OCP_LEN_STATUS;
  }

  /* busy waiting */
  for (i = 0; i < SPIS_BUSYWAIT_CHECK_CNT; ++i) {
    if (0 == spi->spi_reg.spis_reg->MOSI_PAC_AT_CNT) {
      break;
    }
    SPIDPRT("\r\n MOSI_PAC_AT_CNT=%lu", (uint32_t)spi->spi_reg.spis_reg->MOSI_PAC_AT_CNT);
  }

  if (SPIS_BUSYWAIT_CHECK_CNT <= i) { /* timeout */
    SPIDPRT("\r\n MOSI_PAC_AT_CNT=%lu", (uint32_t)spi->spi_reg.spis_reg->MOSI_PAC_AT_CNT);
    ret = -1;
  }

  /* Deactivate the OCMP buffer mechanism */
  spi->spi_reg.spis_reg->OCPM_BUFF_ACTIVE = 0x0;

  if (ret) {
    DRV_SPIS_ResetController(bus_id);
    SPIDPRT("\r\n Rx/total size=%d/%ld", rxDataSize, totalBufSize);
    return SPI_ERROR_GENERIC;
  } else {
    SPIDPRT("\r\n Transaction success, Rx/total size=%d/%ld", rxDataSize, totalBufSize);
    SPIDPRT("\r\n 2nd stage Payload Transaction has been successfully completed.");
  }

  return SPI_ERROR_NONE;
}

static DRV_SPI_Status DRV_SPI_SlaveSend(SPI_Bus_Id bus_id, SPI_Transfer *transfer) {
  DRV_SPI_Peripheral *spi = DRV_SPI_GetPeripheral(bus_id);
  uint16_t txDataSize = 0;
  uint32_t ocpmBuffActive = 0, slaveIntMaskReg;
  int32_t totalBufSize, ret = 0, i = 0;
  volatile uint32_t spiSlaveOcpmBufFifoStatus, spiSlaveOcpmOcpLenStatus;

  if (spi->ctrl.status.busy == 1) {
    return SPI_ERROR_BUSY;
  }
  if (!(spi->is_init)) {
    return SPI_ERROR_NOT_INIT;
  }

  /* Test for illegal/bad packet header transaction */
  if (transfer == NULL || transfer->send_buf == NULL || (transfer->len == 0)) {
    spi->spi_reg.spis_reg->MISO_PAC_AT_CNT = 0;
    return SPI_ERROR_WRONG_PARAM;
  } else {
    txDataSize = transfer->len;
  }

  spi->xfer.num = transfer->len;
  spi->xfer.rx_cnt = 0;
  spi->xfer.tx_cnt = 0;

  /* Flush & Reset RD/WR buffers right away before the transaction is started */
  DRV_SPIS_ResetController(bus_id);
  spi->spi_reg.spis_reg->OCPM_INIT = 0xFFFFFFFF;

  /* Calculate the total transaction size (simultaneous transaction should have the same size) */
  totalBufSize = ROUND_UP(txDataSize, 4);

  spi->ctrl.status.busy = 1U;
  spi->ctrl.status.data_lost = 0U;
  spi->ctrl.status.mode_fault = 0U;

  SPIDPRT("\r\n Preparing Tx SPI Buffer (S2M), total/actual size=%ld/%d\n", totalBufSize,
          transfer->len);

  /* Set DMA address of Tx SPI buffer */
  spi->spi_reg.spis_reg->OCPM_RD_BD_ADDR = (uint32_t)transfer->send_buf;

  /* Set size of Tx SPI DMA buffer */
  spi->spi_reg.spis_reg->OCPM_RD_BD_SIZE = totalBufSize;

  ocpmBuffActive |= SPI_SLAVE_OCPM_BUFF_ACTIVE_RD_BUFF_ACTIVE_Msk;

  /* Set the whole message as 1 packet (i.e. set the MISO Atomic Packet Counter to 1 */
  spi->spi_reg.spis_reg->MISO_PAC_AT_CNT = 1;

  /* Generate interrupt at the end of the Tx buffer transmission */
  spi->spi_reg.spis_reg->MISO_PAC_AT_CNT_CTRL =
      (SPI_SLAVE_MISO_PAC_AT_CNT_CTRL_BD_CNT_INT_EN_Msk |
       0x3 << SPI_SLAVE_MISO_PAC_AT_CNT_CTRL_BD_CNT_INT_MODE_Pos |
       SPI_SLAVE_MISO_PAC_AT_CNT_CTRL_BD_CNT_HW_EN_Msk |
       SPI_SLAVE_MISO_PAC_AT_CNT_CTRL_BD_CNT_THRS_EN_Msk |
       (0x00 << SPI_SLAVE_MISO_PAC_AT_CNT_CTRL_BD_CNT_THRS_VALUE_Pos));

  /* Clear pending interrupts */
  spi->spi_reg.spis_reg->RD_CLR_INT;

  /* Enable the MISO Packet Counter Interrupt */
  slaveIntMaskReg = spi->spi_reg.spis_reg->MASK_INT;
  slaveIntMaskReg &= ~SPI_SLAVE_MASK_INT_MASK_MISO_PACKET_CNT_INT_Msk;
  spi->spi_reg.spis_reg->MASK_INT = slaveIntMaskReg;

  /* Set the actual packet size */
  spi->spi_reg.spis_reg->CFG_PACKET = ((totalBufSize << SPI_SLAVE_CFG_PACKET_PACKET_SIZE_Pos) |
                                       (1 << SPI_SLAVE_CFG_PACKET_MULTI_PACKET_SIZE_Pos));

  spi->spi_reg.spis_reg->OCPM_BUFF_ACTIVE = ocpmBuffActive;

  SPIDPRT("\r\n Notify the Master that Slave is ready for 2nd transaction. S2M=%d, size=%ld",
          (transfer ? 1 : 0), totalBufSize);

  spiSlaveOcpmBufFifoStatus = spi->spi_reg.spis_reg->OCPM_BUF_FIFO_STATUS;
  spiSlaveOcpmOcpLenStatus = spi->spi_reg.spis_reg->OCPM_OCP_LEN_STATUS;

  /* Wait until completion of the whole transaction */
  while ((spiSlaveOcpmBufFifoStatus & (SPI_SLAVE_OCPM_BUF_FIFO_STATUS_OCPM_MOSI_FIFO_STATUS_Msk |
                                       SPI_SLAVE_OCPM_BUF_FIFO_STATUS_OCPM_MISO_FIFO_STATUS_Msk)) ||
         spiSlaveOcpmOcpLenStatus) {
    spiSlaveOcpmBufFifoStatus = spi->spi_reg.spis_reg->OCPM_BUF_FIFO_STATUS;
    spiSlaveOcpmOcpLenStatus = spi->spi_reg.spis_reg->OCPM_OCP_LEN_STATUS;
  }

  /* busy waiting */
  for (i = 0; i < SPIS_BUSYWAIT_CHECK_CNT; ++i) {
    if (0 == spi->spi_reg.spis_reg->MISO_PAC_AT_CNT) {
      break;
    }
    SPIDPRT("\r\n MISO_PAC_AT_CNT=%lu", (uint32_t)spi->spi_reg.spis_reg->MISO_PAC_AT_CNT);
  }

  if (SPIS_BUSYWAIT_CHECK_CNT <= i) { /* timeout */
    SPIDPRT("\r\n MISO_PAC_AT_CNT=%lu", (uint32_t)spi->spi_reg.spis_reg->MISO_PAC_AT_CNT);
    ret = -1;
  }

  /* Deactivate the OCMP buffer mechanism */
  spi->spi_reg.spis_reg->OCPM_BUFF_ACTIVE = 0x0;

  if (ret) {
    DRV_SPIS_ResetController(bus_id);
    SPIDPRT("\r\n Tx/total size=%d/%ld", txDataSize, totalBufSize);
    return SPI_ERROR_GENERIC;
  } else {
    SPIDPRT("\r\n Transaction success, Tx/total size=%d/%ld\n", txDataSize, totalBufSize);
    SPIDPRT("\r\n Payload Transaction has been successfully completed.\n");
  }

  return SPI_ERROR_NONE;
}

static DRV_SPI_Status DRV_SPI_MasterReceive(SPI_Bus_Id bus_id, SPI_Transfer *transfer) {
  DRV_SPI_Peripheral *spi = DRV_SPI_GetPeripheral(bus_id);
  uint32_t reg_val, val, min_word = 4, ss_option = 0;
  uint16_t len = 0;
  uint8_t *pRecvData = NULL;
  size_t chunk_len;

  if (transfer == NULL) {
    return SPI_ERROR_WRONG_PARAM;
  }

  pRecvData = transfer->recv_buf;
  len = transfer->len;

  spi->xfer.num = transfer->len;
  spi->xfer.rx_cnt = 0;
  spi->xfer.tx_cnt = 0;

  if (pRecvData == NULL) {
    return SPI_ERROR_WRONG_PARAM;
  }
  if (spi->ctrl.status.busy == 1) {
    return SPI_ERROR_BUSY;
  }
  if (!(spi->is_init)) {
    return SPI_ERROR_NOT_INIT;
  }
  if (gSpiConfig[bus_id].param.bus_speed == 0) {
    return SPI_ERROR_WRONG_PARAM;
  }

  if (spi->dev_num >= 0) {
    /* check CMD FIFO is empty */
    if (spi->spi_reg.spim_reg->CMD_FIFO_STATUS & SPI_MASTER0_CMD_FIFO_STATUS_CMD_STATUS_Msk) {
      SPIDPRT("CMD FIFO isn't empty yet!\n");
      return SPI_ERROR_BUSY;
    }

    spi->ctrl.status.busy = 1U;
    spi->ctrl.status.data_lost = 0U;
    spi->ctrl.status.mode_fault = 0U;

    /* Flush Read FIFO */
    DRV_SPI_FlushFifo(spi, SPI_MASTER0_FLUSH_CFG_FLUSH_RDATA_Msk);

    /* Enable HW flow control */
    reg_val = spi->spi_reg.spim_reg->INTERRUPT_CFG1;
    reg_val |= SPI_MASTER0_INTERRUPT_CFG1_HW_FLOW_CTL_Msk;
    reg_val &= ~SPI_MASTER0_INTERRUPT_CFG1_RDATA_S_INT_Msk;
    reg_val |= (0x0 << SPI_MASTER0_INTERRUPT_CFG1_RDATA_S_INT_Pos);
    spi->spi_reg.spim_reg->INTERRUPT_CFG1 = reg_val;

    /* Enable(Unmask) done interrupt */
    // DRV_SPI_EnableInterrupts(spi, SPI_MASTER0_INTERRUPT_CFG2_M_DONE_Msk);

    /* Enable(Unmask) Receive overflow interrupts */
    DRV_SPI_EnableInterrupts(spi, SPI_MASTER0_INTERRUPT_CFG2_M_OVERFLOW_RDATA_Msk);

    /* Issue read command */
    ss_option = transfer->slave_id + 1;
    spi->spi_reg.spim_reg->CMD_FIFO =
        ((len / (gSpiConfig[bus_id].param.data_bits / 8)) << SPI_MASTER0_CMD_FIFO_LEN_Pos) |
        SPI_MASTER0_CMD_FIFO_READ_EN_Msk | SPI_MASTER0_CMD_FIFO_EN_INT_DONE_Msk |
        (ss_option << SPI_MASTER0_CMD_FIFO_SLAVE_SELECT_Pos);

    spi->ctrl.status.busy = 1;
    while (len) {
      chunk_len = min_t(SPI_FIFO_SIZE_BYTES, len);
      len -= chunk_len;
      if (len <= chunk_len) {
        gSpiLastTransfer = true;
        DRV_SPI_EnableInterrupts(spi, SPI_MASTER0_INTERRUPT_CFG2_M_DONE_Msk);
      }

      SPIDPRT("\r\n reading %d from FIFO.", chunk_len);
      while (chunk_len) {
        val = spi->spi_reg.spim_reg->RDATA_FIFO;
        SPIDPRT("\r\n Recv [%lx]chunk_len[%d]", val, chunk_len);

        val = val >> ((min_word - (min_t(min_word, chunk_len))) * 8);
        SPIDPRT("[%lx]", val);

        memcpy(pRecvData, &val, min_t(4, chunk_len));
        pRecvData += min_t(4, chunk_len);
        chunk_len -= min_t(4, chunk_len);
      }
    }
  } else {
    return SPI_ERROR_GENERIC;
  }

  return SPI_ERROR_NONE;
}

static DRV_SPI_Status DRV_SPI_MasterSend(SPI_Bus_Id bus_id, SPI_Transfer *transfer) {
  DRV_SPI_Peripheral *spi = DRV_SPI_GetPeripheral(bus_id);
  uint32_t reg_val = 0, val, ss_option = 0;
  uint16_t len = 0;
  const uint8_t *pSendData = NULL;
  uint8_t endian = 0;
  int32_t n, left_to_send, chunk;

  if (transfer == NULL) {
    return SPI_ERROR_WRONG_PARAM;
  }
  if (transfer->send_buf == NULL) {
    return SPI_ERROR_WRONG_PARAM;
  }
  if (spi->ctrl.status.busy == 1) {
    return SPI_ERROR_BUSY;
  }
  if (!(spi->is_init)) {
    return SPI_ERROR_NOT_INIT;
  }
  if (gSpiConfig[bus_id].param.bus_speed == 0) {
    return SPI_ERROR_WRONG_PARAM;
  }

  pSendData = transfer->send_buf;
  len = transfer->len;

  spi->xfer.num = transfer->len;
  spi->xfer.rx_cnt = 0;
  spi->xfer.tx_cnt = 0;

  spi->ctrl.status.busy = 1U;
  spi->ctrl.status.data_lost = 0U;
  spi->ctrl.status.mode_fault = 0U;

  if (spi->spi_reg.spim_reg->CS1_CFG_b.ENDIAN_CS1)
    endian = 1; /* little endian */
  else
    endian = 0; /* big endian */

  if (spi->dev_num >= 0) {
    /* check FIFO is empty */
    if (spi->spi_reg.spim_reg->CMD_FIFO_STATUS & SPI_MASTER0_CMD_FIFO_STATUS_CMD_STATUS_Msk) {
      SPIDPRT("CMD FIFO isn't empty yet!\n");
      return SPI_ERROR_BUSY;
    }

    /* flush FIFO */
    DRV_SPI_FlushFifo(spi, SPI_MASTER0_FLUSH_CFG_FLUSH_WDATA_Msk);

    /* Enable HW flow control */
    reg_val = spi->spi_reg.spim_reg->INTERRUPT_CFG1;
    reg_val |= SPI_MASTER0_INTERRUPT_CFG1_HW_FLOW_CTL_Msk;
    reg_val &= ~SPI_MASTER0_INTERRUPT_CFG1_WDATA_S_INT_Msk;
    reg_val |= (0x0 << SPI_MASTER0_INTERRUPT_CFG1_WDATA_S_INT_Pos);
    spi->spi_reg.spim_reg->INTERRUPT_CFG1 = reg_val;

    /* Unmask done interrupt */
    DRV_SPI_EnableInterrupts(spi, SPI_MASTER0_INTERRUPT_CFG2_M_DONE_Msk);

    /* Unmask send underflow interrupt */
    // DRV_SPI_EnableInterrupts(spi, SPI_MASTER0_INTERRUPT_CFG2_M_UNDERFLOW_WDATA_Msk);

    /* adjust for hw issue */
    uint16_t adjust_len = len, total_sent = 0;
    bool need_adjust = false, _cmd_is_sent = false;
    if (len % 4 == 3) {
      adjust_len = len - 1;
      need_adjust = true;
    }
    left_to_send = adjust_len;

    /* Print out transmit buffer */
    while (total_sent < len) {
      total_sent += left_to_send;
      while (left_to_send > 0) {
        if (left_to_send > SPI_FIFO_SIZE_BYTES) {
          left_to_send = left_to_send - SPI_FIFO_SIZE_BYTES;
          chunk = SPI_FIFO_SIZE_BYTES;
        } else {
          chunk = left_to_send;
          left_to_send = 0;
          gSpiLastTransfer = true;
          DRV_SPI_EnableInterrupts(spi, SPI_MASTER0_INTERRUPT_CFG2_M_DONE_Msk);
        }

        n = chunk;
        int32_t min_word = 4;
        while (n > 0) {
          val = 0;

          /* adjust for hw issue */
          if ((need_adjust == true) && (min_t(min_word, n) == 2)) {
            if (endian == 0) { /* big endian */
              memcpy(&val, pSendData, 3);
              val = val << 8;
            } else {
              memcpy(&val, pSendData, 2);
              val = val << 16;
            }
          } else if ((need_adjust == true) && (min_t(min_word, n) == 1)) {
            if (endian == 0) { /* big endian */
              memcpy(&val, pSendData - 2, 1);
              val = val << 24;
            } else {
              memcpy(&val, pSendData, 1);
              val = val << 24;
            }
          } else {
            memcpy(&val, pSendData, min_t(min_word, n));
            val = val << ((min_word - (min_t(min_word, n))) * 8);
          }

          SPIDPRT("[%lx]", val);
          spi->spi_reg.spim_reg->WDATA_FIFO = val;
          pSendData += min_t(min_word, n);
          n -= min_t(min_word, n);
        }

        SPIDPRT("\r\n wrote %ld characters", chunk);

        // issue write command
        if (_cmd_is_sent == false) {
          _cmd_is_sent = true;
          ss_option = transfer->slave_id + 1;
          SPIDPRT("\r\n SPI Write options [%lx]",
                  (ss_option << SPI_MASTER0_CMD_FIFO_SLAVE_SELECT_Pos) |
                      SPI_MASTER0_CMD_FIFO_EN_INT_DONE_Msk |
                      ((len / (gSpiConfig[bus_id].param.data_bits / 8))
                       << SPI_MASTER0_CMD_FIFO_LEN_Pos));

          spi->spi_reg.spim_reg->CMD_FIFO =
              (ss_option << SPI_MASTER0_CMD_FIFO_SLAVE_SELECT_Pos) |
              SPI_MASTER0_CMD_FIFO_EN_INT_DONE_Msk |
              ((len / (gSpiConfig[bus_id].param.data_bits / 8)) << SPI_MASTER0_CMD_FIFO_LEN_Pos);
          spi->ctrl.status.busy = 1;
        }
      }
      if (adjust_len != len) left_to_send = 1;
    }
  } else {
    return SPI_ERROR_GENERIC;
  }
  return SPI_ERROR_NONE;
}

DRV_SPI_Status DRV_SPI_Receive(SPI_Bus_Id bus_id, SPI_Transfer *transfer) {
  DRV_SPI_Peripheral *spi = DRV_SPI_GetPeripheral(bus_id);

  if (spi->bus_mode == SPI_MODE_MASTER)
    return DRV_SPI_MasterReceive(bus_id, transfer);
  else
    return DRV_SPI_SlaveReceive(bus_id, transfer);
}

DRV_SPI_Status DRV_SPI_Send(SPI_Bus_Id bus_id, SPI_Transfer *transfer) {
  DRV_SPI_Peripheral *spi = DRV_SPI_GetPeripheral(bus_id);

  if (spi->bus_mode == SPI_MODE_MASTER)
    return DRV_SPI_MasterSend(bus_id, transfer);
  else
    return DRV_SPI_SlaveSend(bus_id, transfer);
}

void DRV_SPI_Enable(SPI_Bus_Id bus_id, bool enable) {
  DRV_SPI_Peripheral *spi = DRV_SPI_GetPeripheral(bus_id);

  if (spi->bus_mode == SPI_MODE_MASTER) {
    if (enable)
      spi->spi_reg.spim_reg->SPI_ENABLE_b.SPI_EN = 1;
    else
      spi->spi_reg.spim_reg->SPI_ENABLE_b.SPI_EN = 0;
  }
}

void DRV_SPI_ModeInactive(SPI_Bus_Id bus_id) {
  DRV_SPI_Peripheral *spi = DRV_SPI_GetPeripheral(bus_id);

  if (spi->bus_mode == SPI_MODE_MASTER) {
    // Disable SPI
    spi->spi_reg.spim_reg->SPI_ENABLE_b.SPI_EN = 0;
  }
  // Disable interrupts
  DRV_SPI_DisableInterrupts(spi, 0xFFFFFFFF);
}

uint32_t DRV_SPI_ConfigureBusSpeed(SPI_Bus_Id bus_id, const int32_t ss_num, uint32_t bus_speed,
                                   uint32_t opcode) {
  uint32_t reg_val, div;
  DRV_SPI_Peripheral *spi = DRV_SPI_GetPeripheral(bus_id);

  if (opcode == 1) {  // Set
    if ((bus_speed < MIN_SPI_BUS_SPEED) || (bus_speed > MAX_SPI_BUS_SPEED)) return 0;

    reg_val = DRV_SPIM_SetCfgReg(spi, ss_num, 0, 0);
    SPIDPRT("\r\n MCU_SPI_MASTER1_CS1_CFG [%lx]", reg_val);

    reg_val &= ~SPI_MASTER0_CS1_CFG_COUNT_DIVIDE_CS1_Msk;
    div = DIV_ROUND_UP(SystemCoreClock, bus_speed);
    SPIDPRT("\r\n SystemCoreClock [%ld] speed[%lu] div[%lu]", SystemCoreClock, bus_speed, div);

    if (div == 2) div++;
    if (div > 255) div = 255;

    reg_val |= (div << SPI_MASTER0_CS1_CFG_COUNT_DIVIDE_CS1_Pos);
    DRV_SPIM_SetCfgReg(spi, ss_num, reg_val, 1);
    gSpiConfig[bus_id].param.bus_speed = bus_speed;
  }

  return gSpiConfig[bus_id].param.bus_speed;
}

uint32_t DRV_SPI_GetDataCount(SPI_Bus_Id bus_id) {
  DRV_SPI_Peripheral *spi = DRV_SPI_GetPeripheral(bus_id);
  if (spi == NULL) {
    return (0);
  }
  if (!spi->is_init) {
    return (0);
  }

  return (spi->xfer.rx_cnt + spi->xfer.tx_cnt);
}

DRV_SPI_Status DRV_SPI_GetStatus(SPI_Bus_Id bus_id, SPI_Status *status) {
  DRV_SPI_Peripheral *spi = DRV_SPI_GetPeripheral(bus_id);
  if (spi == NULL) return SPI_ERROR_BUS_ID;

  status->busy = spi->ctrl.status.busy;
  status->data_lost = spi->ctrl.status.data_lost;
  status->mode_fault = spi->ctrl.status.mode_fault;

  return SPI_ERROR_NONE;
}

static DRV_SPI_Status DRV_SPIM_RestoreState(SPI_Bus_Id bus_id) {
  SPI_Config mConfig;
  DRV_SPI_Status ret_val;

  if ((ret_val = DRV_SPI_GetDefConf(bus_id, &mConfig)) != SPI_ERROR_NONE) {
    return ret_val;
  }

  if ((ret_val = DRV_SPI_Configure(&mConfig, SPIM_SS_1)) != SPI_ERROR_NONE) {
    return ret_val;
  }

  return SPI_ERROR_NONE;
}

#if (configUSE_ALT_SLEEP == 1)
int32_t DRV_SPI_HandleSleepNotify(DRV_SLEEP_NotifyType sleep_state, DRV_PM_PwrMode pwr_mode,
                                  void *ptr_ctx) {
  int id;
  if (pwr_mode == DRV_PM_MODE_STANDBY || pwr_mode == DRV_PM_MODE_SHUTDOWN) {
    if (sleep_state == DRV_SLEEP_NTFY_SUSPENDING) {
      for (id = 0; id < SPI_BUS_MAX; id++) {
        DRV_SPI_Peripheral *spi = DRV_SPI_GetPeripheral(id);
        if (spi->is_init == true) {
          if (spi->bus_mode == SPI_MODE_MASTER) {
            SPI_REG_CFG[id][0] = spi->spi_reg.spim_reg->INTERRUPT_CFG1;
            SPI_REG_CFG[id][1] = spi->spi_reg.spim_reg->INTERRUPT_CFG2;
            SPI_REG_CFG[id][2] = spi->spi_reg.spim_reg->CS1_CFG;
            SPI_REG_CFG[id][3] = spi->spi_reg.spim_reg->CS2_CFG;
            SPI_REG_CFG[id][4] = spi->spi_reg.spim_reg->CS3_CFG;
            SPI_REG_CFG[id][5] = spi->spi_reg.spim_reg->SPI_ENABLE_b.SPI_EN;
          } else {
            SPI_REG_CFG[id][0] = spi->spi_reg.spis_reg->CFG;
            SPI_REG_CFG[id][1] = spi->spi_reg.spis_reg->MASK_INT;
          }
        }
      }
    } else if (sleep_state == DRV_SLEEP_NTFY_RESUMING) {
      for (id = 0; id < SPI_BUS_MAX; id++) {
        DRV_SPI_Peripheral *spi = DRV_SPI_GetPeripheral(id);
        if (spi->is_init == true) {
          // Reset SPI
          // DRV_SPI_FlushFifo(spi, SPI_MASTER0_FLUSH_CFG_FLUSH_RDATA_Msk);
          if (spi->bus_mode == SPI_MODE_MASTER) {
            spi->spi_reg.spim_reg->INTERRUPT_CFG1 = SPI_REG_CFG[id][0];
            spi->spi_reg.spim_reg->INTERRUPT_CFG2 = SPI_REG_CFG[id][1];
            spi->spi_reg.spim_reg->CS1_CFG = SPI_REG_CFG[id][2];
            spi->spi_reg.spim_reg->CS2_CFG = SPI_REG_CFG[id][3];
            spi->spi_reg.spim_reg->CS3_CFG = SPI_REG_CFG[id][4];
            spi->spi_reg.spim_reg->SPI_ENABLE_b.SPI_EN = SPI_REG_CFG[id][5];
          } else {
            spi->spi_reg.spis_reg->CFG = SPI_REG_CFG[id][0];
            spi->spi_reg.spis_reg->MASK_INT = SPI_REG_CFG[id][0];
          }
        }
      }
    }
  }
  return 0;
}
#endif

DRV_SPI_Status DRV_SPI_Initialize(SPI_Bus_Id bus_id) {
  DRV_SPI_Peripheral *spi = DRV_SPI_GetPeripheral(bus_id);
  DRV_SPI_Status ret_val;

  if (!spi) {
    return SPI_ERROR_BUS_ID;
  }
  if (spi->is_init) {
    return SPI_ERROR_INITIALED;
  }

  /* Configure IO mux */
  if (DRV_SPI_ConfigureIO(bus_id) != 0) {
    return SPI_ERROR_GENERIC;
  }

  if (spi->bus_mode == SPI_MODE_SLAVE) {
    ret_val = DRV_SPIS_RestoreState(bus_id, SPI_SLAVE_DEFAULT_PROPERTY);
    if (ret_val != SPI_ERROR_NONE) {
      return ret_val;
    }

    DRV_SPIS_ResetController(bus_id);
  } else {
    ret_val = DRV_SPIM_RestoreState(bus_id);
    if (ret_val != SPI_ERROR_NONE) {
      return ret_val;
    }
  }

#if (configUSE_ALT_SLEEP == 1)
  if (!spi_registered_sleep_notify) {
    int32_t temp_for_sleep_notify, ret_val;

    ret_val = DRV_SLEEP_RegNotification(&DRV_SPI_HandleSleepNotify, &temp_for_sleep_notify, NULL);
    if (ret_val != 0) { /* failed to register sleep notify callback */
      ;
    } else {
      spi_registered_sleep_notify = 1;
    }
  }
#endif

  /* Configure system interrupt */
  NVIC_SetPriority(spi->irq_num, 7U);     /* set Interrupt priority */
  NVIC_SetPriority(spi->err_irq_num, 7U); /* set Interrupt priority */
  NVIC_EnableIRQ(spi->irq_num);
  NVIC_EnableIRQ(spi->err_irq_num);

  spi->is_init = true;
  return SPI_ERROR_NONE;
}

DRV_SPI_Status DRV_SPI_Uninitialize(SPI_Bus_Id bus_id) {
  DRV_SPI_Peripheral *spi = DRV_SPI_GetPeripheral(bus_id);

  if (!spi) {
    return SPI_ERROR_BUS_ID;
  }
  if (!spi->is_init) {
    return SPI_ERROR_NOT_INIT;
  }

  NVIC_DisableIRQ(spi->irq_num);
  NVIC_DisableIRQ(spi->err_irq_num);

  if (spi->bus_mode == SPI_MODE_SLAVE) {
    DRV_SPIS_ResetController(bus_id);
  }

  spi->is_init = false;

  return SPI_ERROR_NONE;
}

DRV_SPI_Status DRV_SPI_PowerControl(SPI_Bus_Id bus_id, SPI_PowerMode mode) {
  DRV_SPI_Peripheral *spi = DRV_SPI_GetPeripheral(bus_id);

  if (!spi) {
    return SPI_ERROR_BUS_ID;
  }

  if (!spi->is_init) {
    return SPI_ERROR_NOT_INIT;
  }

  switch (mode) {
    case SPI_POWER_OFF:
      NVIC_DisableIRQ(spi->irq_num);
      NVIC_DisableIRQ(spi->err_irq_num);
      break;

    case SPI_POWER_FULL:
      NVIC_ClearPendingIRQ(spi->irq_num);
      NVIC_ClearPendingIRQ(spi->err_irq_num);
      NVIC_EnableIRQ(spi->irq_num);
      NVIC_EnableIRQ(spi->err_irq_num);
      break;

    default:
      return SPI_ERROR_WRONG_PARAM;
  }

  return SPI_ERROR_NONE;
}

DRV_SPI_Status DRV_SPI_AbortTransfer(SPI_Bus_Id bus_id) {
  DRV_SPI_Peripheral *spi = DRV_SPI_GetPeripheral(bus_id);

  // check powered if needed

  if (!spi->is_init) {
    return SPI_ERROR_NOT_INIT;
  }

  // Disable SPI
  // Disable interrupts

  if (spi->ctrl.status.busy == 1) {
    // disable TX
    // disable RX
  }

  // clear transfer data
  memset(&spi->xfer, 0, sizeof(SPI_Transfer_Info));
  spi->ctrl.status.busy = 0;

  // Enable interrupts

  return SPI_ERROR_NONE;
}
