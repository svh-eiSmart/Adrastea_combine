/* ---------------------------------------------------------------------------

(c) copyright 2019 Altair Semiconductor, Ltd. All rights reserved.

This software, in source or object form (the "Software"), is the
property of Altair Semiconductor Ltd. (the "Company") and/or its
licensors, which have all right, title and interest therein, You
may use the Software only in accordance with the terms of written
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

#include "FreeRTOS.h"
#include <stdio.h>
#include <string.h>
#include "portmacro.h"
#include "125X_mcu.h"
#include "semphr.h"
#include "spi.h"
#include "task.h"
#include "if_mngr.h"
#include "gpio.h"

//#define _MCU_SPI_DEBUG

#ifdef _MCU_SPI_DEBUG
#define SPIDPRT(format, args...) printf("\n\r[%s:%d] " format, __FILE__, __LINE__, ##args)
#else
#define SPIDPRT(args...)
#endif

#define DIV_ROUND_UP(n, d) (((n) + (d)-1) / (d))

/* Note: the value 500000~5000000 was tested. */
#define MIN_SPI_BUS_SPEED (500000)
#define MAX_SPI_BUS_SPEED (30000000)

#define SPI_FIFO_SIZE_BYTES (0x80 * 4)

#define SPI_MAX_WAIT_TIMES (10000)

typedef struct spim_struct_params_t {
  volatile int spi_Dev;
  volatile int Prevdemux_option;
  volatile SemaphoreHandle_t spiIsrSemaphone;
  volatile SemaphoreHandle_t spi_Mutex;
  uint32_t spi_ctrl_interrupt_status_rc;
  uint32_t spi_base_offset;
  IRQn_Type spi_irqn_number;
  const char *spi_driver_interrupt_label;
} spim_struct_params_t;

// Bus frequency in KHz
#define SPI_BUS_FREQUENCY_KHZ (100)

// Bus period in usec
#define SPI_BUS_PERIOD_USEC (1000 / SPI_BUS_FREQUENCY_KHZ)

#define LONG_TIME 0xffff

#define SPIM1_BASE_OFFSET (BASE_ADDRESS_MCU_SPI_MASTER1 - BASE_ADDRESS_MCU_SPI_MASTER0)
#define SPIM0_BASE_OFFSET (0)

/* interrupts declaration */
void spim0_interrupt_handler(void);
#ifdef ALT1250
void spim1_interrupt_handler(void);
#endif

/* global variable */
static spim_struct_params_t spim0_params = {
    0,          0,         NULL, NULL, MCU_SPI_MASTER0_INTERRUPT_STATUS_RC, SPIM0_BASE_OFFSET,
    SPIM0_IRQn, "spim0Drv"};

#ifdef BASE_ADDRESS_MCU_SPI_MASTER1
static spim_struct_params_t spim1_params = {
    0,          0,         NULL, NULL, MCU_SPI_MASTER1_INTERRUPT_STATUS_RC, SPIM1_BASE_OFFSET,
    SPIM1_IRQn, "spim1Drv"};
#endif

/* global structures and variables */
#ifdef ALT1255
static bool gSpiBusInited[MAX_SPIM_DEVICES_QUANT] = {false};
static bool device_is_busy[MAX_SPIM_DEVICES_QUANT] = {false};
#else
static bool gSpiBusInited[MAX_SPIM_DEVICES_QUANT] = {false, false};
static bool device_is_busy[MAX_SPIM_DEVICES_QUANT] = {false, false};
#endif
static bool gSpiLastTransfer = false;
static spi_config_t gSpiConfig[MAX_SPIM_DEVICES_QUANT];
static void *gSpimHandle[MAX_SPIM_DEVICES_QUANT];

typedef struct _spim_counters {
  unsigned long recv_int_done;
  unsigned long recv_int_rdata;
  unsigned long recv_int_wdata;
  unsigned long recv_int_rdata_overflow;
  unsigned long recv_int_rdata_underflow;
} spim_counters_t;
static spim_counters_t gSpimCounters[MAX_SPIM_DEVICES_QUANT];

size_t min_t(size_t a, size_t b) {
  if (a < b)
    return a;
  else
    return b;
}

static void spim_reset_counters(spi_bus_t bus) {
  gSpimCounters[bus].recv_int_done = 0;
  gSpimCounters[bus].recv_int_rdata = 0;
  gSpimCounters[bus].recv_int_wdata = 0;
  gSpimCounters[bus].recv_int_rdata_overflow = 0;
  gSpimCounters[bus].recv_int_rdata_underflow = 0;
}

static void spim_dump_counters(spi_bus_t bus) {
#ifdef _MCU_SPI_DEBUG
  printf("\n\r====== Counters =======");
  printf("\n\r DONE: %lu", gSpimCounters[bus].recv_int_done);
  printf("\n\r RDATA: %lu", gSpimCounters[bus].recv_int_rdata);
  printf("\n\r WDATA: %lu", gSpimCounters[bus].recv_int_wdata);
  printf("\n\r RDATA Overflow: %lu", gSpimCounters[bus].recv_int_rdata_overflow);
  printf("\n\r RDATA Underflow: %lu", gSpimCounters[bus].recv_int_rdata_underflow);
  printf("\n\r=======================\n");
#endif
}

SPI_STATUS_ERROR_CODE spi_set_ss_active(spi_bus_t bus_id,
                                        bool active) {  // 0:de-active; 1: active
  if (gSpiConfig[bus_id].param.ssSignalMgmt != SPI_SS_SIGNAL_BY_DRV) {
    return SPI_STATUS_ERR_NONE;
  }
  if (gSpiConfig[bus_id].param.gpioId == 0) {
    return SPI_STATUS_ERR_NONE;
  }

  spi_ss_mode_t set_mode = gSpiConfig[bus_id].param.ssMode;  // active
  eGpioRet ret_val;

  if (!active) {  // deactive
    if (gSpiConfig[bus_id].param.ssMode == SPI_SS_ACTIVE_LOW) {
      set_mode = SPI_SS_ACTIVE_HIGH;
    } else {
      set_mode = SPI_SS_ACTIVE_LOW;
    }
  }

  ret_val =
      gpio_set_direction_output_and_value((eMcuGpioIds)gSpiConfig[bus_id].param.gpioId, set_mode);
  if (ret_val != GPIO_RET_SUCCESS) {
    printf("\n\r Warning: failed to set SPI SS (GPIO id-%d) %s error:%d!",
           gSpiConfig[bus_id].param.gpioId, active ? "Active" : "Deactive", ret_val);
    return SPI_STATUS_ERR_GENERIC;
  }

  return SPI_STATUS_ERR_NONE;
}

/*****************************************************************************
 * static void spim_restore_state(void)
 * PURPOSE: This function would restore SPI setting.
 * Parameters:
 *       Input: bus_id.
 *       Output: None.
 * Returns: Pointer to structure containing parameters for each spi..
 * MODIFICTION HISTORY:
 *****************************************************************************/
static void spim_restore_state(spi_bus_t bus_id) {
  spim_reset_counters(bus_id);
  gSpiLastTransfer = false;
  return;
}

/*****************************************************************************
 * static spim_struct_params_t *spim_bus_select(spi_bus_t spi_bus_id)
 * PURPOSE: This function would select spi device parameters by its number.
 * Parameters:
 *       Input: spi_bus_id - SPI bus id.
 *       Output: None.
 * Returns: Pointer to structure containing parameters for each spi..
 * MODIFICTION HISTORY:
 *****************************************************************************/
static spim_struct_params_t *spim_bus_select(spi_bus_t spi_bus_id) {
  switch (spi_bus_id) {
    case 0:
      return &spim0_params;
#ifdef BASE_ADDRESS_MCU_SPI_MASTER1
    case 1:
      return &spim1_params;
#endif
    default:
      printf("\n\rCriticial Error, spi bus_id %d is not valid!", spi_bus_id);
      break;
  }
  return NULL;
}

/*****************************************************************************
 * static void mcu_spi_dump_state(spi_bus_t spi_bus_id)
 * PURPOSE: This function would dump SPI configuration & state.
 * Parameters:
 *       Input: spi_bus_id: SPI bus id.
 *       Output: None.
 * Returns: None.
 * MODIFICTION HISTORY:
 *****************************************************************************/
static void mcu_spi_dump_state(spi_bus_t spi_bus_id) {
  unsigned int RegVal;

  spim_struct_params_t *spim_bus_params = spim_bus_select(spi_bus_id);

  RegVal = REGISTER(MCU_SPI_MASTER0_CMD_FIFO_STATUS + spim_bus_params->spi_base_offset);
  printf("\n\rCMD_FIFO_STATUS: %.8x\n", RegVal);

  RegVal = REGISTER(MCU_SPI_MASTER0_WDATA_FIFO_STATUS + spim_bus_params->spi_base_offset);
  printf("\n\rWDATA_FIFO_STATUS: %.8x\n", RegVal);

  RegVal = REGISTER(MCU_SPI_MASTER0_RDATA_FIFO_STATUS + spim_bus_params->spi_base_offset);
  printf("\n\rRDATA_FIFO_STATUS: %.8x\n", RegVal);

  RegVal = REGISTER(MCU_SPI_MASTER0_LAST_EXE_CMD + spim_bus_params->spi_base_offset);
  printf("\n\rLAST_EXE_CMD: %.8x\n", RegVal);

  RegVal = REGISTER(MCU_SPI_MASTER0_INTERRUPT_CFG1 + spim_bus_params->spi_base_offset);
  printf("\n\rINTERRUPT_CFG1: %.8x\n", RegVal);

  RegVal = REGISTER(MCU_SPI_MASTER0_INTERRUPT_CFG2 + spim_bus_params->spi_base_offset);
  printf("\n\rINTERRUPT_CFG2: %.8x\n", RegVal);

  RegVal = REGISTER(MCU_SPI_MASTER0_INTERRUPT_STATUS + spim_bus_params->spi_base_offset);
  printf("\n\rINTERRUPT_STATUS: %.8x\n", RegVal);

  RegVal = REGISTER(MCU_SPI_MASTER0_CURRENT_STATE + spim_bus_params->spi_base_offset);
  printf("\n\rCURRENT_STATE: %.8x\n", RegVal);

  RegVal = REGISTER(MCU_SPI_MASTER0_CS1_CFG + spim_bus_params->spi_base_offset);
  printf("\n\rCS1_CFG: %.8x\n", RegVal);

  RegVal = REGISTER(MCU_SPI_MASTER0_CS2_CFG + spim_bus_params->spi_base_offset);
  printf("\n\rCS2_CFG: %.8x\n", RegVal);

  RegVal = REGISTER(MCU_SPI_MASTER0_CS3_CFG + spim_bus_params->spi_base_offset);
  printf("\n\rCS3_CFG: %.8x\n", RegVal);
}

/*****************************************************************************
 * static void spim_config_params(onst int ss_num, const spi_config_t *masterConfig)
 * PURPOSE: This function would handle SPI configuration.
 * Parameters:
 *       Input: ss_num: SS number.
 *       Input: *masterConfig: pointer of SPI master configuration.
 *       Output: None.
 * Returns: None.
 * MODIFICTION HISTORY:
 *****************************************************************************/
static void spim_config_params(const int ss_num, const spi_config_t *masterConfig) {
  unsigned int RegVal, div, ss_cfg_offset;

  spim_struct_params_t *spim_bus_params = spim_bus_select(masterConfig->spiBusId);
  ss_cfg_offset = spim_bus_params->spi_base_offset + (ss_num * 0x4);

  RegVal = REGISTER(MCU_SPI_MASTER0_CS1_CFG + ss_cfg_offset);
  SPIDPRT("\n\r MCU_SPI_MASTER1_CS1_CFG [%x]offset[%lx]", RegVal,
          MCU_SPI_MASTER0_CS1_CFG + spim_bus_params->spi_base_offset);

  /* We're using infinite mode */
  RegVal |= MCU_SPI_MASTER0_CS1_CFG_INFINITE_CS1_MSK;

  /* Set parameters */
  RegVal &= ~MCU_SPI_MASTER0_CS1_CFG_CPHA_CS1_MSK;
  if (masterConfig->param.cpha) RegVal |= MCU_SPI_MASTER0_CS1_CFG_CPHA_CS1_MSK;

  RegVal &= ~MCU_SPI_MASTER0_CS1_CFG_CPOL_CS1_MSK;
  if (masterConfig->param.cpol) RegVal |= MCU_SPI_MASTER0_CS1_CFG_CPOL_CS1_MSK;

  /* endian */
  RegVal &= ~MCU_SPI_MASTER0_CS1_CFG_ENDIAN_CS1_MSK;
  if (masterConfig->param.endian) RegVal |= MCU_SPI_MASTER0_CS1_CFG_ENDIAN_CS1_MSK;

  /* data bits */
  RegVal &= ~MCU_SPI_MASTER0_CS1_CFG_WS_CS1_MSK;
  RegVal |= (masterConfig->param.dataBits << MCU_SPI_MASTER0_CS1_CFG_WS_CS1_POS);

  /* bit order */
  RegVal &= ~MCU_SPI_MASTER0_CS1_CFG_REVERSAL_CS1_MSK;
  if (masterConfig->param.bitOrder == SPI_BIT_ORDER_MSB_FIRST)
    RegVal |= (0 << MCU_SPI_MASTER0_CS1_CFG_REVERSAL_CS1_POS);
  else
    RegVal |= (1 << MCU_SPI_MASTER0_CS1_CFG_REVERSAL_CS1_POS);

  /* data speed */
  RegVal &= ~MCU_SPI_MASTER0_CS1_CFG_COUNT_DIVIDE_CS1_MSK;
  div = DIV_ROUND_UP(SystemCoreClock, masterConfig->param.busSpeed);
  SPIDPRT("\n\r SystemCoreClock [%ld] speed[%lu] div[%d]", SystemCoreClock,
          masterConfig->param.busSpeed, div);

  if (div == 2) div++;
  if (div > 255) div = 255;

  RegVal |= (div << MCU_SPI_MASTER0_CS1_CFG_COUNT_DIVIDE_CS1_POS);

  SPIDPRT("\n\r ss_num [%d] MCU_SPI_MASTERX_CSX_CFG offset[%x] [%x]\r\n", ss_num,
          MCU_SPI_MASTER0_CS1_CFG + ss_cfg_offset, RegVal);
  REGISTER(MCU_SPI_MASTER0_CS1_CFG + ss_cfg_offset) = RegVal;
}

/*****************************************************************************
 * static void spim_handle_isr(spi_bus_t spi_bus_id)
 * PURPOSE: This function would handle SPI driver interrupts.
 * Parameters:
 *       Input: spi_bus_id: SPI bus id.
 *       Output: None.
 * Returns: None.
 * MODIFICTION HISTORY:
 *****************************************************************************/
static void spim_handle_isr(spi_bus_t spi_bus_id) {
  spim_struct_params_t *spim_bus_params = spim_bus_select(spi_bus_id);
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  unsigned int RegVal, RegVal2;
  unsigned int eventMask = 0;
  spim_handle_t *handle = NULL;

  handle = gSpimHandle[spi_bus_id];

  RegVal = REGISTER(MCU_SPI_MASTER0_INTERRUPT_STATUS_RC + spim_bus_params->spi_base_offset);

  if (RegVal & MCU_SPI_MASTER0_INTERRUPT_STATUS_RC_DONE_C_INT_MSK) {
    /* DONE */
    gSpimCounters[spi_bus_id].recv_int_done++;
    if ((handle->callback != 0) && (gSpiLastTransfer == true)) {
      eventMask = SPI_EVENT_TRANSFER_COMPLETE;
      gSpiLastTransfer = false;
      handle->callback(spi_bus_id, eventMask, handle->userData);
    }
  }

  if (RegVal & MCU_SPI_MASTER0_INTERRUPT_STATUS_RC_RDATA_STATUS_C_INT_MSK) {
    /* RDATA */
    gSpimCounters[spi_bus_id].recv_int_rdata++;
    RegVal2 = REGISTER(MCU_SPI_MASTER0_INTERRUPT_CFG2 + spim_bus_params->spi_base_offset);
    RegVal2 |= MCU_SPI_MASTER0_INTERRUPT_CFG2_M_RDATA_MSK;
    REGISTER(MCU_SPI_MASTER0_INTERRUPT_CFG2 + spim_bus_params->spi_base_offset) = RegVal2;
  }

  if (RegVal & MCU_SPI_MASTER0_INTERRUPT_STATUS_RC_WDATA_STATUS_C_INT_MSK) {
    /* WDATA */
    gSpimCounters[spi_bus_id].recv_int_wdata++;
    RegVal2 = REGISTER(MCU_SPI_MASTER0_INTERRUPT_CFG2 + spim_bus_params->spi_base_offset);
    RegVal2 |= MCU_SPI_MASTER0_INTERRUPT_CFG2_M_WDATA_MSK;
    REGISTER(MCU_SPI_MASTER0_INTERRUPT_CFG2 + spim_bus_params->spi_base_offset) = RegVal2;
  }

  if (RegVal &
      MCU_SPI_MASTER0_INTERRUPT_STATUS_RC_RDATA_OVER_C_INT_MSK) {  // update status - Data lost
    if (handle->callback != 0) {
      eventMask = SPI_EVENT_DATA_LOST;
      handle->callback(spi_bus_id, eventMask, handle->userData);
    }
    gSpimCounters[spi_bus_id].recv_int_rdata_overflow++;
    RegVal2 = REGISTER(MCU_SPI_MASTER0_INTERRUPT_CFG2 + spim_bus_params->spi_base_offset);
    RegVal2 |= MCU_SPI_MASTER0_INTERRUPT_CFG2_M_OVERFLOW_RDATA_MSK;
    REGISTER(MCU_SPI_MASTER0_INTERRUPT_CFG2 + spim_bus_params->spi_base_offset) = RegVal2;
  }

  if (RegVal &
      MCU_SPI_MASTER0_INTERRUPT_STATUS_RC_WDATA_UNDER_C_INT_MSK) {  // update status - Data lost
    if (handle->callback != 0) {
      eventMask = SPI_EVENT_DATA_LOST;
      handle->callback(spi_bus_id, eventMask, handle->userData);
    }
    gSpimCounters[spi_bus_id].recv_int_rdata_underflow++;
    RegVal2 = REGISTER(MCU_SPI_MASTER0_INTERRUPT_CFG2 + spim_bus_params->spi_base_offset);
    RegVal2 |= MCU_SPI_MASTER0_INTERRUPT_CFG2_M_UNDERFLOW_WDATA_MSK;
    REGISTER(MCU_SPI_MASTER0_INTERRUPT_CFG2 + spim_bus_params->spi_base_offset) = RegVal2;
  }

  // Clear interrupt cause
  REGISTER(spim_bus_params->spi_ctrl_interrupt_status_rc);

  if (xSemaphoreGiveFromISR(spim_bus_params->spiIsrSemaphone, &xHigherPriorityTaskWoken) !=
      pdTRUE) {
    ; /* for the checking */
  }

  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

  device_is_busy[spi_bus_id] = false;
}

/*****************************************************************************
 * spiX_interrupt_handler_call
 * PURPOSE: This function would calls general handler with specific spi parameter.
 * Parameters: None.
 * Returns: None.
 * MODIFICTION HISTORY:
 *****************************************************************************/
void spim0_interrupt_handler(void) { spim_handle_isr(SPIM0_BUS); }
#ifdef ALT1250
void spim1_interrupt_handler(void) { spim_handle_isr(SPIM1_BUS); }
#endif
/*****************************************************************************
 * static int spim_bus_init(const spi_config_t *masterConfig)
 * PURPOSE: This function would initialize SPI configuration.
 * Parameters:
 *        Init: SPI configuration.
 * Returns:    Pass(0)/Fail(-1)
 * MODIFICTION HISTORY:
 *****************************************************************************/
static int spim_bus_init(const spi_config_t *masterConfig) {
  unsigned int RegVal;

  if (gSpiBusInited[masterConfig->spiBusId] == true) {
    return 0;
  }

  spim_struct_params_t *spim_bus_params = spim_bus_select(masterConfig->spiBusId);
  // SPIDPRT("spi_bus_params->spi_irqn_number [%d]", spi_bus_params->spi_irqn_number);
  NVIC_SetPriority(spim_bus_params->spi_irqn_number, 7); /* set Interrupt priority */
  NVIC_EnableIRQ(spim_bus_params->spi_irqn_number);

  // Create Semaphore
  spim_bus_params->spiIsrSemaphone = xSemaphoreCreateBinary();

  // Give Mutex
  spim_bus_params->spi_Mutex = xSemaphoreCreateMutex();
  xSemaphoreGive(spim_bus_params->spi_Mutex);

  // Starts without valid dev
  spim_bus_params->spi_Dev = 0;

  // Reset SPI HW
  RegVal = REGISTER(MCU_SPI_MASTER0_FLUSH_CFG + spim_bus_params->spi_base_offset);
  RegVal |= MCU_SPI_MASTER0_FLUSH_CFG_FLUSH_RDATA_MSK;
  REGISTER(MCU_SPI_MASTER0_FLUSH_CFG + spim_bus_params->spi_base_offset) = RegVal;

  gSpiBusInited[masterConfig->spiBusId] = true;
  return 0;
}

/*-----------------------------------------------------------------------------
 * int spim_configure_pins(spi_bus_t spi_bus_id)
 * PURPOSE: This function would configure SPI pins.
 * PARAMs:
 *      INPUT:  spi_bus_t spim_bus_id: SPI bus id.
 *      OUTPUT: None
 * RETURN:  0 - Successful.   Otherwise - Failed.
 * MODIFICTION HISTORY:
 *-----------------------------------------------------------------------------
 */
int spim_configure_pins(spi_bus_t spim_bus_id) {
  unsigned int retVal = 0;

  switch (spim_bus_id) {
    case SPIM0_BUS:
      if (if_mngr_config_io(IF_MNGR_SPIM0) != IF_MNGR_SUCCESS) retVal = (-1);
      break;
#ifdef BASE_ADDRESS_MCU_SPI_MASTER1
    case SPIM1_BUS:
      if (if_mngr_config_io(IF_MNGR_SPIM1) != IF_MNGR_SUCCESS) retVal = (-1);
      break;
#endif
    default:
      retVal = (-1);
      break;
  }
  return retVal;
}

/*-----------------------------------------------------------------------------
 * SPI_ERROR_CODE spi_get_def_config(spi_bus_t bus_id, spi_config_t *pConfig)
 * PURPOSE: This function would load spim configurations from MCU wizard.
 * PARAMs:
 *      INPUT:  SPIM bus ID.
 *      OUTPUT: SPI configuration.
 * RETURN:  0 - Successful.   Otherwise - SPI_Error_Codes.
 *
 * MODIFICTION HISTORY:
 *-----------------------------------------------------------------------------
 */
SPI_ERROR_CODE spi_get_def_config(spi_bus_t bus_id, spi_config_t *pConfig) {
  memset(pConfig, 0x0, sizeof(spi_config_t));
  if (bus_id == SPIM0_BUS) {
    if (if_mngr_load_defconfig(IF_MNGR_SPIM0, pConfig) != IF_MNGR_SUCCESS) return SPI_ERROR_GENERIC;
  }
#ifdef BASE_ADDRESS_MCU_SPI_MASTER1
  else if (bus_id == SPIM1_BUS) {
    if (if_mngr_load_defconfig(IF_MNGR_SPIM1, pConfig) != IF_MNGR_SUCCESS) return SPI_ERROR_GENERIC;
  }
#endif
  else {
    return SPI_ERROR_GENERIC;
  }

  return SPI_ERROR_NONE;
}

int spi_check_cs_pin(eMcuGpioIds gpio_id) {
  eGpioRet gpio_ret;
  eMcuPinIds pin_id;
  eGpioDir gpio_dir;

  if ((gpio_id < MCU_GPIO_ID_01) || (gpio_id >= MCU_GPIO_ID_NUM)) return (-1);

  gpio_ret = gpio_get_map(gpio_id, &pin_id);

  if (gpio_ret == GPIO_RET_SUCCESS && pin_id != 0) {
    if ((gpio_ret = gpio_get_direction(gpio_id, &gpio_dir)) != GPIO_RET_SUCCESS) {
      printf("Unable to get gpio%d direction\r\n", gpio_id);
      return (-1);
    }

    if (gpio_dir != GPIO_DIR_OUTPUT) {
      printf("Error: GPIO ID %d(PIN %d) direction is not configured to OUTPUT!", gpio_id, pin_id);
      return (-1);
    }
  } else {
    printf("SS GPIO ID %d is not valid. error: %d\r\n", gpio_id, gpio_ret);
    return (-1);
  }

  return 0;
}

/*-----------------------------------------------------------------------------
 * SPI_ERROR_CODE spi_configure(const spi_config_t *pConfig)
 * PURPOSE: This function would initialize SPI configuration.
 * PARAMs:
 *      INPUT:  SPI configuration.
 *      INPUT:  Slave Select number.
 *      OUTPUT: None
 * RETURN:  0 - Successful.   Otherwise - SPI_Error_Codes.
 *
 * MODIFICTION HISTORY:
 *-----------------------------------------------------------------------------
 */
SPI_ERROR_CODE spi_configure(const spi_config_t *pConfig, spi_ss_t ss_num) {
  unsigned int RegVal;
  spim_struct_params_t *spim_bus_params = spim_bus_select(pConfig->spiBusId);

  /* specific check */
  if (pConfig->spiBusId == SPIM0_BUS) {
    if (ss_num > SPIM_SS_3) return SPI_ERROR_SS_NUM;
  }
#ifdef BASE_ADDRESS_MCU_SPI_MASTER1
  else if (pConfig->spiBusId == SPIM1_BUS) {
    if (ss_num > SPIM_SS_3) return SPI_ERROR_SS_NUM;
  }
#endif
  else {
    return SPI_ERROR_GENERIC;
  }

  /* Data bits */
  if ((pConfig->param.busSpeed < MIN_SPI_BUS_SPEED) ||
      (pConfig->param.busSpeed > MAX_SPI_BUS_SPEED))
    return SPI_ERROR_BUS_SPEED;

  /* mode */
  if (pConfig->spiMode != SPI_MODE_MASTER) return SPI_ERROR_MODE;

  /* frame format */
  if ((pConfig->param.cpha != SPI_CPHA_0) && (pConfig->param.cpha != SPI_CPHA_1))
    return SPI_ERROR_FRAME_FORMAT;
  if ((pConfig->param.cpol != SPI_CPOL_0) && (pConfig->param.cpol != SPI_CPOL_1))
    return SPI_ERROR_FRAME_FORMAT;

  /* Data bits (1..32) */
  if ((pConfig->param.dataBits == 0) || (pConfig->param.dataBits > 32)) return SPI_ERROR_DATA_BITS;

  /* bitOrder */
  if ((pConfig->param.bitOrder != SPI_BIT_ORDER_MSB_FIRST) &&
      (pConfig->param.bitOrder != SPI_BIT_ORDER_LSB_FIRST))
    return SPI_ERROR_BIT_ORDER;

  /* ss mode */
  if ((pConfig->param.ssMode != SPI_SS_ACTIVE_LOW) &&
      (pConfig->param.ssSignalMgmt == SPI_SS_SIGNAL_BY_DEFAULT))
    return SPI_ERROR_SS_MODE;

  /* endian */
  if ((pConfig->param.endian != SPI_BIG_ENDIAN) && (pConfig->param.endian != SPI_LITTLE_ENDIAN))
    return SPI_ERROR_ENDIAN;

  if (spim_bus_init(pConfig) != SPI_ERROR_NONE) {
    return SPI_ERROR_GENERIC;
  }

  if (spim_configure_pins(pConfig->spiBusId) != 0) {
    return SPI_ERROR_GENERIC;
  }

  spim_config_params(ss_num, pConfig);

  if (pConfig->param.ssSignalMgmt == SPI_SS_SIGNAL_BY_DRV) {
    if (spi_check_cs_pin((eMcuGpioIds)pConfig->param.gpioId) != 0) return SPI_ERROR_SS_PIN;
  }

  spim_restore_state(pConfig->spiBusId);

  /* enable SPIM */
  RegVal = REGISTER(MCU_SPI_MASTER0_SPI_ENABLE + spim_bus_params->spi_base_offset);
  RegVal |= MCU_SPI_MASTER0_SPI_ENABLE_SPI_EN_MSK;
  REGISTER(MCU_SPI_MASTER0_SPI_ENABLE + spim_bus_params->spi_base_offset) = RegVal;

  memset(&gSpiConfig[pConfig->spiBusId], 0x0, sizeof(spi_config_t));
  memcpy(&gSpiConfig[pConfig->spiBusId], pConfig, sizeof(spi_config_t));

  return SPI_ERROR_NONE;
}

int spi_open_handle(spi_bus_t bus_id, spim_handle_t *handle, spi_event_callback callback,
                    void *userData) {
  if (handle == NULL) return (-1);

  memset(handle, 0, sizeof(*handle));
  gSpimHandle[bus_id] = handle;
  handle->callback = callback;
  handle->userData = userData;

  return 0;
}

int bus_config_check(spi_bus_t bus_id, spi_ss_t slave_id) {
  int ret_val = 0;
  /* check bus */
  if (gSpiBusInited[bus_id] != true) {
    SPIDPRT("SPIM%d! bus is not initialized.\n", bus_id);
    return (-1);
  }

  if (gSpiConfig[bus_id].param.busSpeed == 0) {
    SPIDPRT("SPIM%d! parameters seems not configured.\n", bus_id);
    return (-2);
  }

  /* check SS config if needed */
  if (slave_id) {
    ;
  }

  return ret_val;
}

int spi_receive_hdl(spi_bus_t spi_bus_id, spi_transfer_t *transfer, bool blocking) {
  int retVal;
  uint32_t RegVal, val, TimeOutUs, min_word = 4, ss_option = 0;
  uint16_t len = 0;
  uint8_t *pRecvData = NULL;
  size_t chunk_len;

  if (transfer == NULL) {
    return (-SPI_STATUS_ERR_WRONG_PARAM);
  }

  spim_reset_counters(spi_bus_id);

  pRecvData = transfer->recvData;
  retVal = len = transfer->len;

  spim_struct_params_t *spim_bus_params = spim_bus_select(spi_bus_id);

  if (pRecvData == NULL) {
    SPIDPRT("Invalid parameter!\n");
    return (-SPI_STATUS_ERR_INVALID_LEN);
  }

  /* bus init and configuration checking */
  if (bus_config_check(spi_bus_id, transfer->slaveId) < 0) return (-SPI_STATUS_ERR_NOT_INIT);

  if (spim_bus_params->spi_Dev >= 0) {
    // check CMD FIFO is empty
    RegVal = REGISTER(MCU_SPI_MASTER0_CMD_FIFO_STATUS + spim_bus_params->spi_base_offset);
    if (RegVal & MCU_SPI_MASTER0_CMD_FIFO_STATUS_CMD_STATUS_MSK) {
      SPIDPRT("CMD FIFO isn't empty yet!\n");
      return (-SPI_STATUS_ERR_BUSY);
    }

    if (xSemaphoreTake(spim_bus_params->spi_Mutex, LONG_TIME) == pdTRUE) {
      /* Flush Read FIFO */
      RegVal = REGISTER(MCU_SPI_MASTER0_FLUSH_CFG + spim_bus_params->spi_base_offset);
      RegVal |= MCU_SPI_MASTER0_FLUSH_CFG_FLUSH_RDATA_MSK;
      REGISTER(MCU_SPI_MASTER0_FLUSH_CFG + spim_bus_params->spi_base_offset) = RegVal;

      /* Enable HW flow control */
      RegVal = REGISTER(MCU_SPI_MASTER0_INTERRUPT_CFG1 + spim_bus_params->spi_base_offset);
      RegVal |= MCU_SPI_MASTER0_INTERRUPT_CFG1_HW_FLOW_CTL_MSK;
      RegVal &= ~MCU_SPI_MASTER0_INTERRUPT_CFG1_RDATA_S_INT_MSK;
      RegVal |= (0x0 << MCU_SPI_MASTER0_INTERRUPT_CFG1_RDATA_S_INT_POS);
      REGISTER(MCU_SPI_MASTER0_INTERRUPT_CFG1 + spim_bus_params->spi_base_offset) = RegVal;

      /* Unmask done interrupt */
      RegVal = REGISTER(MCU_SPI_MASTER0_INTERRUPT_CFG2 + spim_bus_params->spi_base_offset);
      RegVal &= ~MCU_SPI_MASTER0_INTERRUPT_CFG2_M_DONE_MSK;
      REGISTER(MCU_SPI_MASTER0_INTERRUPT_CFG2 + spim_bus_params->spi_base_offset) = RegVal;

      /* Unmask Receive overflow interrupt */
      RegVal = REGISTER(MCU_SPI_MASTER0_INTERRUPT_CFG2 + spim_bus_params->spi_base_offset);
      RegVal &= ~MCU_SPI_MASTER0_INTERRUPT_CFG2_M_OVERFLOW_RDATA_MSK;
      REGISTER(MCU_SPI_MASTER0_INTERRUPT_CFG2 + spim_bus_params->spi_base_offset) = RegVal;

      spi_set_ss_active(spi_bus_id, 1);

      /* Issue read command */
      ss_option = transfer->slaveId + 1;

      REGISTER(MCU_SPI_MASTER0_CMD_FIFO + spim_bus_params->spi_base_offset) =
          ((len / (gSpiConfig[spi_bus_id].param.dataBits / 8))
           << MCU_SPI_MASTER0_CMD_FIFO_LEN_POS) |
          MCU_SPI_MASTER0_CMD_FIFO_READ_EN_MSK | MCU_SPI_MASTER0_CMD_FIFO_EN_INT_DONE_MSK |
          (ss_option << MCU_SPI_MASTER0_CMD_FIFO_SLAVE_SELECT_POS);
      device_is_busy[spi_bus_id] = true;
      while (len) {
        chunk_len = min_t(SPI_FIFO_SIZE_BYTES, len);
        len -= chunk_len;
        if (len <= chunk_len) gSpiLastTransfer = true;

        val = REGISTER(MCU_SPI_MASTER0_INTERRUPT_CFG2 + spim_bus_params->spi_base_offset);
        val &= ~MCU_SPI_MASTER0_INTERRUPT_CFG2_M_RDATA_MSK;
        REGISTER(MCU_SPI_MASTER0_INTERRUPT_CFG2 + spim_bus_params->spi_base_offset) = val;
        SPIDPRT("Waiting for RDATA int\n");

        TimeOutUs =
            (SPI_BUS_PERIOD_USEC * 40) + (len * SPI_BUS_PERIOD_USEC * 9);  // Value is in usec
        if (xSemaphoreTake(spim_bus_params->spiIsrSemaphone,
                           (TickType_t)(1 + (TimeOutUs / 1000))) == pdFALSE) {
          SPIDPRT("\nfailed to spi_read 1!");

          // Reset SPI
          RegVal = REGISTER(MCU_SPI_MASTER0_FLUSH_CFG + spim_bus_params->spi_base_offset);
          RegVal |= MCU_SPI_MASTER0_FLUSH_CFG_FLUSH_WDATA_MSK;
          REGISTER(MCU_SPI_MASTER0_FLUSH_CFG + spim_bus_params->spi_base_offset) = RegVal;

          xSemaphoreGive(spim_bus_params->spi_Mutex);
          spi_set_ss_active(spi_bus_id, 0);
          return (-SPI_STATUS_ERR_DATA_LOST);
        }

        SPIDPRT("reading %d from FIFO \n", chunk_len);
        while (chunk_len) {
          val = REGISTER(MCU_SPI_MASTER0_RDATA_FIFO + spim_bus_params->spi_base_offset);
          SPIDPRT("\n\rRecv [%lx]chunk_len[%d]", val, chunk_len);

          val = val >> ((min_word - (min_t(min_word, chunk_len))) * 8);
          SPIDPRT("[%lx]", val);

          memcpy(pRecvData, &val, min_t(4, chunk_len));
          pRecvData += min_t(4, chunk_len);
          chunk_len -= min_t(4, chunk_len);
        }
      }

      if ((gSpiConfig[spi_bus_id].param.ssSignalMgmt == SPI_SS_SIGNAL_BY_DRV) || blocking == true) {
        int sleep_cnt = 0;
        while (device_is_busy[spi_bus_id]) {
          vTaskDelay(1 / portTICK_PERIOD_MS);
          if (sleep_cnt > SPI_MAX_WAIT_TIMES) {
            SPIDPRT("Error: Send timeout!");
            retVal = (-SPI_STATUS_ERR_TIMEOUT);
            break;
          }
          sleep_cnt++;
        }
        spi_set_ss_active(spi_bus_id, 0);
      }

      xSemaphoreGive(spim_bus_params->spi_Mutex);
    } else {
      retVal = (-SPI_STATUS_ERR_GENERIC);
    }
  } else {
    retVal = (-SPI_STATUS_ERR_GENERIC);
  }

  spim_dump_counters(spi_bus_id);

   // mcu_spi_dump_state(spi_bus_id);
  return retVal;
}

int spi_receive(spi_bus_t spi_bus_id, spi_transfer_t *transfer) {
  return spi_receive_hdl(spi_bus_id, transfer, true);
}

int spi_receive_nonblock(spi_bus_t spi_bus_id, spi_transfer_t *transfer) {
  return spi_receive_hdl(spi_bus_id, transfer, false);
}

int spi_send_hdl(spi_bus_t spi_bus_id, spi_transfer_t *transfer, bool blocking) {
  uint32_t RegVal = 0, val, ss_cfg_offset, ss_option = 0, TimeOutUs;
  uint16_t len = 0;
  uint8_t *pSendData = NULL, endian = 0;
  int n, retVal, left_to_send, chunk;

  if (transfer == NULL) {
    return (-SPI_STATUS_ERR_WRONG_PARAM);
  }

  if (transfer->sendData == NULL) {
    SPIDPRT("Invalid parameter!\n");
    return (-SPI_STATUS_ERR_INVALID_LEN);
  }

  if (bus_config_check(spi_bus_id, transfer->slaveId) < 0) return (-SPI_STATUS_ERR_NOT_INIT);

  pSendData = transfer->sendData;
  retVal = len = transfer->len;

  spim_struct_params_t *spim_bus_params = spim_bus_select(spi_bus_id);

  ss_cfg_offset = spim_bus_params->spi_base_offset + (transfer->slaveId * 0x4);
  RegVal = REGISTER(MCU_SPI_MASTER0_CS1_CFG + ss_cfg_offset);
  RegVal &= MCU_SPI_MASTER0_CS1_CFG_ENDIAN_CS1_MSK;

  if (RegVal)
    endian = 1; /* little endian */
  else
    endian = 0; /* big endian */

  if (spim_bus_params->spi_Dev >= 0) {
    /* check FIFO is empty */
    RegVal = REGISTER(MCU_SPI_MASTER0_CMD_FIFO_STATUS + spim_bus_params->spi_base_offset);
    if (RegVal & MCU_SPI_MASTER0_CMD_FIFO_STATUS_CMD_STATUS_MSK) {
      SPIDPRT("CMD FIFO isn't empty yet!\n");
      return (-SPI_STATUS_ERR_BUSY);
    }

    /* flush FIFO */
    RegVal = REGISTER(MCU_SPI_MASTER0_FLUSH_CFG + spim_bus_params->spi_base_offset);
    RegVal |= MCU_SPI_MASTER0_FLUSH_CFG_FLUSH_WDATA_MSK;
    REGISTER(MCU_SPI_MASTER0_FLUSH_CFG + spim_bus_params->spi_base_offset) = RegVal;

    if (xSemaphoreTake(spim_bus_params->spi_Mutex, LONG_TIME) == pdTRUE) {
      /* Enable HW flow control */
      RegVal = REGISTER(MCU_SPI_MASTER0_INTERRUPT_CFG1 + spim_bus_params->spi_base_offset);
      RegVal |= MCU_SPI_MASTER0_INTERRUPT_CFG1_HW_FLOW_CTL_MSK;
      RegVal &= ~MCU_SPI_MASTER0_INTERRUPT_CFG1_WDATA_S_INT_MSK;
      RegVal |= (0x0 << MCU_SPI_MASTER0_INTERRUPT_CFG1_WDATA_S_INT_POS);
      REGISTER(MCU_SPI_MASTER0_INTERRUPT_CFG1 + spim_bus_params->spi_base_offset) = RegVal;

      /* Unmask done interrupt */
      RegVal = REGISTER(MCU_SPI_MASTER0_INTERRUPT_CFG2 + spim_bus_params->spi_base_offset);
      RegVal &= ~MCU_SPI_MASTER0_INTERRUPT_CFG2_M_DONE_MSK;
      REGISTER(MCU_SPI_MASTER0_INTERRUPT_CFG2 + spim_bus_params->spi_base_offset) = RegVal;

      /* Unmask send underflow interrupt */
      RegVal = REGISTER(MCU_SPI_MASTER0_INTERRUPT_CFG2 + spim_bus_params->spi_base_offset);
      RegVal &= ~MCU_SPI_MASTER0_INTERRUPT_CFG2_M_UNDERFLOW_WDATA_MSK;
      REGISTER(MCU_SPI_MASTER0_INTERRUPT_CFG2 + spim_bus_params->spi_base_offset) = RegVal;

      /* adjust for hw issue */
      uint16_t adjust_len = len, total_sent = 0;
      bool need_adjust = false, _cmd_is_sent = false;
      if (len % 4 == 3) {
        adjust_len = len - 1;
        need_adjust = true;
      }
      left_to_send = adjust_len;

      spi_set_ss_active(spi_bus_id, 1);

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
          }

          n = chunk;
          int min_word = 4;
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
            REGISTER(MCU_SPI_MASTER0_WDATA_FIFO + spim_bus_params->spi_base_offset) = val;
            pSendData += min_t(min_word, n);
            n -= min_t(min_word, n);
          }

          SPIDPRT("\n\r wrote %d characters", chunk);

          // issue write command
          if (_cmd_is_sent == false) {
            _cmd_is_sent = true;
            ss_option = transfer->slaveId + 1;
            SPIDPRT("\n\r SPI Write options [%lx]",
                    (ss_option << MCU_SPI_MASTER0_CMD_FIFO_SLAVE_SELECT_POS) |
                        MCU_SPI_MASTER0_CMD_FIFO_EN_INT_DONE_MSK |
                        ((len / (gSpiConfig[spi_bus_id].param.dataBits / 8))
                         << MCU_SPI_MASTER0_CMD_FIFO_LEN_POS));

            REGISTER(MCU_SPI_MASTER0_CMD_FIFO + spim_bus_params->spi_base_offset) =
                (ss_option << MCU_SPI_MASTER0_CMD_FIFO_SLAVE_SELECT_POS) |
                MCU_SPI_MASTER0_CMD_FIFO_EN_INT_DONE_MSK |
                ((len / (gSpiConfig[spi_bus_id].param.dataBits / 8))
                 << MCU_SPI_MASTER0_CMD_FIFO_LEN_POS);
            device_is_busy[spi_bus_id] = true;
          }
        }
        if (adjust_len != len) left_to_send = 1;
      }

      TimeOutUs = (SPI_BUS_PERIOD_USEC * 40) + (len * SPI_BUS_PERIOD_USEC * 9);  // Value is in usec
      if (xSemaphoreTake(spim_bus_params->spiIsrSemaphone, (TickType_t)(1 + (TimeOutUs / 1000))) ==
          pdFALSE) {
        // Reset SPI
        RegVal = REGISTER(MCU_SPI_MASTER0_FLUSH_CFG + spim_bus_params->spi_base_offset);
        RegVal |= MCU_SPI_MASTER0_FLUSH_CFG_FLUSH_WDATA_MSK;
        REGISTER(MCU_SPI_MASTER0_FLUSH_CFG + spim_bus_params->spi_base_offset) = RegVal;

        SPIDPRT("\nfailed to spi_write!");
        xSemaphoreGive(spim_bus_params->spi_Mutex);
        spi_set_ss_active(spi_bus_id, 0);
        return (-SPI_STATUS_ERR_DATA_LOST);
      }

      if ((gSpiConfig[spi_bus_id].param.ssSignalMgmt == SPI_SS_SIGNAL_BY_DRV) || blocking == true) {
        int sleep_cnt = 0;
        while (device_is_busy[spi_bus_id]) {
          vTaskDelay(1 / portTICK_PERIOD_MS);
          if (sleep_cnt > SPI_MAX_WAIT_TIMES) {
            SPIDPRT("Error: Send timeout!");
            retVal = (-SPI_STATUS_ERR_TIMEOUT);
            break;
          }
          sleep_cnt++;
        }
        spi_set_ss_active(spi_bus_id, 0);
      }

      xSemaphoreGive(spim_bus_params->spi_Mutex);
    } else {
      retVal = (-SPI_STATUS_ERR_GENERIC);
    }
  } else {
    retVal = (-SPI_STATUS_ERR_GENERIC);
  }

  return retVal;
}

int spi_send(spi_bus_t spi_bus_id, spi_transfer_t *transfer) {
  return spi_send_hdl(spi_bus_id, transfer, true);
}

int spi_send_nonblock(spi_bus_t spi_bus_id, spi_transfer_t *transfer) {
  return spi_send_hdl(spi_bus_id, transfer, false);
}

int spi_transfer(spi_bus_t spi_bus_id, spi_transfer_t *transfer) {
  int ret_val = 0;

  if (transfer == NULL) {
    return (-SPI_STATUS_ERR_WRONG_PARAM);
  }

  if (gSpiConfig[spi_bus_id].param.ssSignalMgmt == SPI_SS_SIGNAL_BY_APP) {
    /* transfer can't support SPI_SS_SIGNAL is managed BY_APP*/
    return (-SPI_STATUS_ERR_GENERIC);
  }

  spim_reset_counters(spi_bus_id);

  ret_val = spi_send(spi_bus_id, transfer);
  if (ret_val <= 0) {
    SPIDPRT("\n\r Failed to Send data. error:%d!!", ret_val);
    //mcu_spi_dump_state(spi_bus_id);

    return (-1);
  }

  SPIDPRT("\n\r%d bytes are sent", ret_val);
  spim_dump_counters(spi_bus_id);

  /* Delay to wait slave is ready. Tune this value per project. */
  vTaskDelay(500 / portTICK_PERIOD_MS);

  spim_reset_counters(spi_bus_id);

  ret_val = spi_receive(spi_bus_id, transfer);
  if (ret_val <= 0) {
    SPIDPRT("\n\r Failed to Receive data. error:%d!!", ret_val);
    return (-2);
  }

  SPIDPRT("\n\r%d bytes are received", ret_val);
  spim_dump_counters(spi_bus_id);

  return 0;
}
