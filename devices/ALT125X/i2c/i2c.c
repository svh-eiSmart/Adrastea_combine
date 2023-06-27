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
/****************************************************************************
 * Included Files
 ****************************************************************************/
/* Standard Headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* FreeRTOS Headers */
#include "FreeRTOS.h"
#include "portmacro.h"
#include "semphr.h"
#include "task.h"

/* Platform Headers */
#include "125X_mcu.h"
#include "i2c.h"
#include "if_mngr.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Bus frequency in KHz */
#define I2C_BUS_FREQUENCY_KHZ (400)
/* Bus period in usec */
#define I2C_BUS_PERIOD_USEC ((float)1000 / I2C_BUS_FREQUENCY_KHZ)

#define LONG_TIME (0xffff)

#define I2C1_BASE_OFFSET (BASE_ADDRESS_MCU_I2C1_CTRL - BASE_ADDRESS_MCU_I2C0_CTRL)

#define I2C0_BASE_OFFSET (0)

/* Basic mode macros*/
#define ALT_I2C_10BITS_PREFIX (0x7800) /* S 1 1 1 1 0 A9 A8 ACK A7 A6 A5 A4 A3 A2 A1 A0 ACK ... */
#define ALT_I2C_10BITS_MASK (0x03FF)
#define ALT_I2C_7BITS_MASK (0x007F)
/* fifo cmd */
#define ALT_I2C_START_SEQ (1 << 4) | (1 << 6)
#define ALT_I2C_RESTART_SEQ (1 << 4) | (1 << 5) | (1 << 6)
#define ALT_I2C_STOP_SEQ (1 << 3) | (1 << 6)
#define ALT_I2C_TX_SEQ (1 << 2) | (1 << 6)
#define ALT_I2C_RX_SEQ (1 << 1) | (1 << 6)
#define ALT_I2C_NACK_SEQ (1 << 0)
#define ALT_I2C_CMD_FIFO_INT (1 << 6)
#define ALT_I2C_CMD_FIFO_WRITE (0 << 7)
#define ALT_I2C_CMD_FIFO_READ (1 << 7)

/* fifo cmd */
#define ALT_I2C_ADDRESS_POS (8)

#define I2C_RDATA_FIFO_MAX_SIZE (64)

/****************************************************************************
 * Private Types
 ****************************************************************************/
typedef struct {
  volatile SemaphoreHandle_t i2cIsrSemaphone;
  volatile SemaphoreHandle_t i2c_Mutex;
  uint32_t i2c_ctrl_interrupt_status_rc;
  uint32_t i2c_base_offset;
  IRQn_Type i2c_irqn_number;
  const char* i2c_driver_interrupt_label;
} i2c_struct_params_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/
static i2c_struct_params_t i2c0_params = {
    NULL, NULL, MCU_I2C0_CTRL_INTERRUPT_STATUS_RC, I2C0_BASE_OFFSET, I2C0_IRQn, "i2c0Drv"};
#ifdef BASE_ADDRESS_MCU_I2C1_CTRL
static i2c_struct_params_t i2c1_params = {
    NULL, NULL, MCU_I2C1_CTRL_INTERRUPT_STATUS_RC, I2C1_BASE_OFFSET, I2C1_IRQn, "i2c1Drv"};
#endif

uint16_t i2c_freq_khz = I2C_BUS_FREQUENCY_KHZ;
float i2c_period_usec = I2C_BUS_PERIOD_USEC;

/****************************************************************************
 * Private Functions
 ****************************************************************************/
/**
 * @brief Get bus parameters of specified bus ID.
 *
 * @param [in] i2c_bus_id: Specified bus ID.
 *
 * @return On success, a non-NULL i2c parameter sturcture pointer retuend, otherwise, NULL returned.
 */
static i2c_struct_params_t* i2c_bus_select(i2c_bus_e i2c_bus_id) {
  i2c_struct_params_t* param = NULL;

  switch (i2c_bus_id) {
    case I2C0_BUS:
      param = &i2c0_params;
      break;

#ifdef ALT1250
    case I2C1_BUS:
      param = &i2c1_params;
      break;

#else
    default:
      param = NULL;
      break;
#endif
  }

  return param;
}

/**
 * @brief I2C driver handler function called by interrupt service routine.
 *
 * @param [in] i2c_bus_id: Source bus of interrupt.
 */
static void i2c_Driver_HandleISR(i2c_bus_e i2c_bus_id) {
  i2c_struct_params_t* i2c_bus_params = i2c_bus_select(i2c_bus_id);
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  configASSERT(i2c_bus_params);

  /* Clear interrupt cause */
  REGISTER(i2c_bus_params->i2c_ctrl_interrupt_status_rc);
  if (xSemaphoreGiveFromISR(i2c_bus_params->i2cIsrSemaphone, &xHigherPriorityTaskWoken) != pdPASS) {
    ; /* For Coverity checking only */
  }

  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/**
 * @brief I2C bus 0 interrupt service routine
 *
 */
void i2c0_interrupt_handler(void) { i2c_Driver_HandleISR(I2C0_BUS); }

#ifdef ALT1250
/**
 * @brief I2C bus 1 interrupt service routine
 *
 */
void i2c1_interrupt_handler(void) { i2c_Driver_HandleISR(I2C1_BUS); }
#endif

static void alt_i2c_ctrl_init(i2c_bus_e i2c_bus_id) {
  uint32_t old_val;
  uint32_t val = 0;
  i2c_struct_params_t* i2c_bus_params = i2c_bus_select(i2c_bus_id);

  if (!i2c_bus_params) {
    return;
  }

  old_val = REGISTER(MCU_I2C0_CTRL_MAIN_CTRL_CFG + i2c_bus_params->i2c_base_offset);
  val = old_val | MCU_I2C0_CTRL_MAIN_CTRL_CFG_BASIC_I2C_MSK |
        MCU_I2C0_CTRL_MAIN_CTRL_CFG_ENDIAN_SWAP_EN_MSK;

  val &= ~MCU_I2C0_CTRL_MAIN_CTRL_CFG_WP_MSK;

  REGISTER(MCU_I2C0_CTRL_MAIN_CTRL_CFG + i2c_bus_params->i2c_base_offset) = val;
}

/**
 * @brief Low level command FIFO write used by the implementation of basic operation
 *
 * @param [in] i2c_bus_params: The parameters of target bus.
 * @param [in] fifo_cmd: The command to be written in into FIFO
 * @param [in] wait_for_done: Set to 1 if caller wants to wait for command done.
 *
 * @return 0: Success; -1: Fail.
 */
static int i2c_cmd_fifo_write(i2c_struct_params_t* i2c_bus_params, uint32_t fifo_cmd,
                              int wait_for_done) {
  unsigned int CurrentTime;
  unsigned int TimeOutUs;
  unsigned int val;

  CurrentTime = xTaskGetTickCount() / portTICK_PERIOD_MS;
  while (((REGISTER(MCU_I2C0_CTRL_CMD_FIFO_STATUS + i2c_bus_params->i2c_base_offset) &
           MCU_I2C0_CTRL_CMD_FIFO_STATUS_CMD_STATUS_MSK) >>
          MCU_I2C0_CTRL_CMD_FIFO_STATUS_CMD_STATUS_POS) > 15) {
    vTaskDelay(1);
    if ((xTaskGetTickCount() / portTICK_PERIOD_MS - CurrentTime) > 3000) {
      return -1;
    }
  }

  val = wait_for_done ? fifo_cmd | ALT_I2C_CMD_FIFO_INT : fifo_cmd & ~ALT_I2C_CMD_FIFO_INT;
  REGISTER(MCU_I2C0_CTRL_CMD_FIFO + i2c_bus_params->i2c_base_offset) = val;
  TimeOutUs = 5000;
  if (wait_for_done) {
    if (xSemaphoreTake(i2c_bus_params->i2cIsrSemaphone, (TickType_t)(1 + (TimeOutUs / 1000))) ==
        pdFALSE) {
      /* Reset I2C HW if failed */
      REGISTER(MCU_I2C0_CTRL_FLUSH_CFG + i2c_bus_params->i2c_base_offset) =
          MCU_I2C0_CTRL_FLUSH_CFG_SM_RESET_MSK;
      return -1;
    }
  }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
int i2c_init(i2c_bus_e i2c_bus_id) {
  int count_divide;
  eIfMngrIf i2c_intf;
  i2c_struct_params_t* i2c_bus_params = i2c_bus_select(i2c_bus_id);

  if (!i2c_bus_params) {
    printf("Unsupported bus ID %d\n", (int)i2c_bus_id);
    return -1;
  }

  /* Configure IO mux */
#ifdef ALT1250
  i2c_intf = (i2c_bus_id == I2C0_BUS) ? IF_MNGR_I2C0 : IF_MNGR_I2C1;
#else
  i2c_intf = (i2c_bus_id == I2C0_BUS) ? IF_MNGR_I2C0 : IF_MNGR_LAST_IF;
#endif
  if (if_mngr_config_io(i2c_intf) != IF_MNGR_SUCCESS) {
    printf("if_mngr_config_io error on %d\n", (int)i2c_intf);
    return -1;
  }

  NVIC_SetPriority(i2c_bus_params->i2c_irqn_number, 7); /* set Interrupt priority */
  NVIC_EnableIRQ(i2c_bus_params->i2c_irqn_number);

  /* Create Semaphore */
  i2c_bus_params->i2cIsrSemaphone = xSemaphoreCreateBinary();
  i2c_bus_params->i2c_Mutex = xSemaphoreCreateMutex();

  count_divide = (int)((SystemCoreClock >> 21) * i2c_period_usec);

  /* Set timing */
  REGISTER(MCU_I2C0_CTRL_COUNTER1_CFG + i2c_bus_params->i2c_base_offset) =
      (count_divide << MCU_I2C0_CTRL_COUNTER1_CFG_COUNT_DIVIDE_POS) |
      ((count_divide * 10) << MCU_I2C0_CTRL_COUNTER1_CFG_COUNT_RECOVERY_POS);
  REGISTER(MCU_I2C0_CTRL_COUNTER2_CFG + i2c_bus_params->i2c_base_offset) =
      (2 << MCU_I2C0_CTRL_COUNTER2_CFG_COUNT_T2R_POS) |
      (2 << MCU_I2C0_CTRL_COUNTER2_CFG_COUNT_R2T_POS);
  REGISTER(MCU_I2C0_CTRL_COUNTER3_CFG + i2c_bus_params->i2c_base_offset) =
      ((count_divide >> 1) << MCU_I2C0_CTRL_COUNTER3_CFG_COUNT_START_HOLD_POS) |
      ((count_divide >> 1) << MCU_I2C0_CTRL_COUNTER3_CFG_COUNT_START_SETUP_POS);
  REGISTER(MCU_I2C0_CTRL_COUNTER4_CFG + i2c_bus_params->i2c_base_offset) =
      ((count_divide >> 1) << MCU_I2C0_CTRL_COUNTER4_CFG_COUNT_STOP_SETUP_POS) |
      (0 << MCU_I2C0_CTRL_COUNTER4_CFG_DUTY_CYCLE_POS);

  /* Enable interrupts */
  REGISTER(MCU_I2C0_CTRL_INTERRUPT_CFG + i2c_bus_params->i2c_base_offset) =
      (REGISTER(MCU_I2C0_CTRL_INTERRUPT_CFG + i2c_bus_params->i2c_base_offset) &
       ~MCU_I2C0_CTRL_INTERRUPT_CFG_M_DONE_MSK);

  /* Reset I2C HW */
  REGISTER(MCU_I2C0_CTRL_FLUSH_CFG + i2c_bus_params->i2c_base_offset) =
      MCU_I2C0_CTRL_FLUSH_CFG_SM_RESET_MSK;

  /* Force to make a stop sequence */
  alt_i2c_ctrl_init(i2c_bus_id);
  return i2c_cmd_fifo_write(i2c_bus_params, ALT_I2C_STOP_SEQ, 0);
}

int i2c_select_speed(i2c_bus_e bus, I2C_MODE i2c_mode){

	switch(i2c_mode)
	{
		case I2C_MODE_STANDARD:
		{
			i2c_freq_khz = 100;
			break;
		}
		case I2C_MODE_FAST:
		{
			i2c_freq_khz = 400;
			break;
		}
		case I2C_MODE_FAST_PLUS:
		{
			i2c_freq_khz = 1000;
			break;
		}
		case I2C_MODE_HIGH_SPEED:
		{
			i2c_freq_khz = 3400;
			break;
		}
		case I2C_MODE_UTRA_FAST:
		{
			i2c_freq_khz = 5000;
			break;
		}
		default:
		{
			return -1;
		}
	}

	i2c_period_usec = (float)1000/i2c_freq_khz;
	return i2c_init(bus);
}

int i2c_read_advanced_mode(uint16_t device, i2c_devid_mode_e mode, uint32_t address,
                           i2c_addrlen_e addrLen, uint32_t len, uint8_t* pDest,
                           i2c_bus_e i2c_bus_id) {
  uint32_t RegVal;
  uint32_t TimeOutUs; /* Timeout in microsec */
  i2c_struct_params_t* i2c_bus_params = i2c_bus_select(i2c_bus_id);
  int retVal = 0;

  if (!i2c_bus_params) {
    printf("Unsupported bus ID %d\n", (int)i2c_bus_id);
    return -1;
  }

  if (xSemaphoreTake(i2c_bus_params->i2c_Mutex, LONG_TIME) == pdTRUE) {
    /* Set main ctrl config */
    RegVal = REGISTER(MCU_I2C0_CTRL_MAIN_CTRL_CFG + i2c_bus_params->i2c_base_offset);
    RegVal = RegVal & ~MCU_I2C0_CTRL_MAIN_CTRL_CFG_BASIC_I2C_MSK; /* No manual mode */
    RegVal = RegVal & ~MCU_I2C0_CTRL_MAIN_CTRL_CFG_WP_MSK;        /* No write protect */
    RegVal =
        RegVal & ~MCU_I2C0_CTRL_MAIN_CTRL_CFG_DRIVE_SDA_HIGH_EN_MSK; /* Do not drive SDA high */
    RegVal =
        RegVal & ~MCU_I2C0_CTRL_MAIN_CTRL_CFG_DRIVE_SCL_HIGH_EN_MSK; /* Do not drive SCL high */
    RegVal =
        RegVal |
        MCU_I2C0_CTRL_MAIN_CTRL_CFG_SCL_STRETCH_EN_MSK; /* Enables clock stretch by slave device */
    RegVal =
        RegVal & ~MCU_I2C0_CTRL_MAIN_CTRL_CFG_ENABLE_MULTI_MASTER_MSK; /* Disable multi master */
    RegVal = RegVal | MCU_I2C0_CTRL_MAIN_CTRL_CFG_ENDIAN_SWAP_EN_MSK;  /* Endian swap enable */
    RegVal = (RegVal & ~MCU_I2C0_CTRL_MAIN_CTRL_CFG_TEN_BIT_DEVICE_SEL_MSK) |
             ((I2C_DEVID_MODE_7BITS == mode ? 0 : 1)
              << MCU_I2C0_CTRL_MAIN_CTRL_CFG_TEN_BIT_DEVICE_SEL_POS); /* Set 7 or 10 bits */
    RegVal = (RegVal & ~MCU_I2C0_CTRL_MAIN_CTRL_CFG_ADD_LEN_MSK) |
             (addrLen << MCU_I2C0_CTRL_MAIN_CTRL_CFG_ADD_LEN_POS); /* address length 1~3 */
    RegVal = (RegVal & ~MCU_I2C0_CTRL_MAIN_CTRL_CFG_DEVICE_SEL_MSK) |
             (device << MCU_I2C0_CTRL_MAIN_CTRL_CFG_DEVICE_SEL_POS); /* Set device ID */
    REGISTER(MCU_I2C0_CTRL_MAIN_CTRL_CFG + i2c_bus_params->i2c_base_offset) = RegVal;

    /* Reset the FIFOs */
    REGISTER(MCU_I2C0_CTRL_FLUSH_CFG + i2c_bus_params->i2c_base_offset) =
        (MCU_I2C0_CTRL_FLUSH_CFG_FLUSH_CMD_MSK | MCU_I2C0_CTRL_FLUSH_CFG_FLUSH_WDATA_MSK |
         MCU_I2C0_CTRL_FLUSH_CFG_FLUSH_RDATA_MSK);

    /* Issue read command */
    REGISTER(MCU_I2C0_CTRL_CMD_FIFO + i2c_bus_params->i2c_base_offset) =
        (address << MCU_I2C0_CTRL_CMD_FIFO_ADDRESS_POS) | MCU_I2C0_CTRL_CMD_FIFO_RW_MSK |
        MCU_I2C0_CTRL_CMD_FIFO_EN_INT_DONE_MSK | ((len - 1) << MCU_I2C0_CTRL_CMD_FIFO_COMMAND_POS);

    /* Wait till read is done, here we set (deviceId(1~2) + address(1~3) + n data) x 300% as it
    timeout */
    TimeOutUs = (unsigned int)(((len + (I2C_DEVID_MODE_7BITS == mode ? 1 : 2) + addrLen) *
                                i2c_period_usec * 9)) *
                3;
    if (xSemaphoreTake(i2c_bus_params->i2cIsrSemaphone, (TickType_t)(2 + (TimeOutUs / 1000))) ==
        pdFALSE) {
      /* Reset I2C HW if failed */
      REGISTER(MCU_I2C0_CTRL_FLUSH_CFG + i2c_bus_params->i2c_base_offset) =
          MCU_I2C0_CTRL_FLUSH_CFG_SM_RESET_MSK | MCU_I2C0_CTRL_FLUSH_CFG_FLUSH_CMD_MSK |
          MCU_I2C0_CTRL_FLUSH_CFG_FLUSH_WDATA_MSK | MCU_I2C0_CTRL_FLUSH_CFG_FLUSH_RDATA_MSK;

      retVal = -1;
    } else {
      if ((uint32_t)len ==
          ((REGISTER(MCU_I2C0_CTRL_RDATA_FIFO_STATUS + i2c_bus_params->i2c_base_offset) &
            MCU_I2C0_CTRL_RDATA_FIFO_STATUS_RDATA_STATUS_MSK) >>
           MCU_I2C0_CTRL_RDATA_FIFO_STATUS_RDATA_STATUS_POS)) {
        /* Read from RFIFO if read exactly len words */
        while (len > 0) {
          if (len >= 4) {
            *(unsigned int*)pDest =
                REGISTER(MCU_I2C0_CTRL_RDATA_FIFO + i2c_bus_params->i2c_base_offset);
            len = len - 4;
            pDest = pDest + 4;
          } else {
            uint32_t rData = REGISTER(MCU_I2C0_CTRL_RDATA_FIFO + i2c_bus_params->i2c_base_offset);
            uint8_t* curByte = (uint8_t*)&rData;
            while (len > 0) {
              *pDest++ = *curByte++;
              len--;
            }
          }
        }
      } else {
        /* Treat as error */
        /* Reset I2C HW if failed */
        REGISTER(MCU_I2C0_CTRL_FLUSH_CFG + i2c_bus_params->i2c_base_offset) =
            MCU_I2C0_CTRL_FLUSH_CFG_SM_RESET_MSK | MCU_I2C0_CTRL_FLUSH_CFG_FLUSH_CMD_MSK |
            MCU_I2C0_CTRL_FLUSH_CFG_FLUSH_WDATA_MSK | MCU_I2C0_CTRL_FLUSH_CFG_FLUSH_RDATA_MSK;
        retVal = -1;
      }
    }
    /* Release Mutex */
    xSemaphoreGive(i2c_bus_params->i2c_Mutex);
  } else {
    retVal = -1;
  }

  return retVal;
}

int i2c_write_advanced_mode(uint16_t device, i2c_devid_mode_e mode, uint32_t address,
                            i2c_addrlen_e addrLen, uint32_t len, const uint8_t* pSrc,
                            i2c_bus_e i2c_bus_id) {
  uint32_t RegVal;
  int n;
  uint32_t TimeOutUs; /* Timeout in microsec */
  i2c_struct_params_t* i2c_bus_params = i2c_bus_select(i2c_bus_id);
  int retVal = 0;

  if (!i2c_bus_params) {
    printf("Unsupported bus ID %d\n", (int)i2c_bus_id);
    return -1;
  }

  if (xSemaphoreTake(i2c_bus_params->i2c_Mutex, LONG_TIME) == pdTRUE) {
    /* Set main ctrl config */
    RegVal = REGISTER(MCU_I2C0_CTRL_MAIN_CTRL_CFG + i2c_bus_params->i2c_base_offset);
    RegVal = RegVal & ~MCU_I2C0_CTRL_MAIN_CTRL_CFG_BASIC_I2C_MSK; /* No manual mode */
    RegVal =
        RegVal |
        MCU_I2C0_CTRL_MAIN_CTRL_CFG_SCL_STRETCH_EN_MSK; /* Enables clock stretch by slave device */
    RegVal = (RegVal & ~MCU_I2C0_CTRL_MAIN_CTRL_CFG_TEN_BIT_DEVICE_SEL_MSK) |
             ((I2C_DEVID_MODE_7BITS == mode ? 0 : 1)
              << MCU_I2C0_CTRL_MAIN_CTRL_CFG_TEN_BIT_DEVICE_SEL_POS); /* Set 7 or 10 bits */
    RegVal = (RegVal & ~MCU_I2C0_CTRL_MAIN_CTRL_CFG_ADD_LEN_MSK) |
             (addrLen << MCU_I2C0_CTRL_MAIN_CTRL_CFG_ADD_LEN_POS); /* address length 1~3 */
    RegVal = (RegVal & ~MCU_I2C0_CTRL_MAIN_CTRL_CFG_DEVICE_SEL_MSK) |
             (device << MCU_I2C0_CTRL_MAIN_CTRL_CFG_DEVICE_SEL_POS); /* Set device ID */
    REGISTER(MCU_I2C0_CTRL_MAIN_CTRL_CFG + i2c_bus_params->i2c_base_offset) = RegVal;

    /* Reset the FIFOs */
    REGISTER(MCU_I2C0_CTRL_FLUSH_CFG + i2c_bus_params->i2c_base_offset) =
        (MCU_I2C0_CTRL_FLUSH_CFG_FLUSH_CMD_MSK | MCU_I2C0_CTRL_FLUSH_CFG_FLUSH_WDATA_MSK |
         MCU_I2C0_CTRL_FLUSH_CFG_FLUSH_RDATA_MSK);

    /* Write to the WFIFO */
    n = len;
    while (n > 0) {
      (*(volatile uint8_t*)(MCU_I2C0_CTRL_WDATA_FIFO + i2c_bus_params->i2c_base_offset)) = *pSrc;
      n = n - 1;
      pSrc = pSrc + 1;
    }

    /* Issue write command */
    REGISTER(MCU_I2C0_CTRL_CMD_FIFO + i2c_bus_params->i2c_base_offset) =
        (address << MCU_I2C0_CTRL_CMD_FIFO_ADDRESS_POS) | MCU_I2C0_CTRL_CMD_FIFO_EN_INT_DONE_MSK |
        ((len - 1) << MCU_I2C0_CTRL_CMD_FIFO_COMMAND_POS);

    /* Wait till write is done, here we set (deviceId(1~2) + address(1~3) + n data) x 300% as it
    timeout */
    TimeOutUs = (unsigned int)(((len + (I2C_DEVID_MODE_7BITS == mode ? 1 : 2) + addrLen) *
                                i2c_period_usec * 9)) *
                3;
    if (xSemaphoreTake(i2c_bus_params->i2cIsrSemaphone, (TickType_t)(2 + (TimeOutUs / 1000))) ==
        pdFALSE) {
      /* Reset I2C HW if failed */
      REGISTER(MCU_I2C0_CTRL_FLUSH_CFG + i2c_bus_params->i2c_base_offset) =
          MCU_I2C0_CTRL_FLUSH_CFG_SM_RESET_MSK;
      retVal = -1;
    }

    /* Release Mutex */
    xSemaphoreGive(i2c_bus_params->i2c_Mutex);
  } else {
    retVal = -1;
  }

  return retVal;
}

int i2c_basic_write(i2c_bus_e i2c_bus_id, uint16_t device, const uint8_t* data, uint32_t length,
                    int resetfifo, int start, int restart, int stop) {
  uint32_t i;
  int retVal;
  i2c_struct_params_t* i2c_bus_params = i2c_bus_select(i2c_bus_id);
  retVal = 0;

  if (!i2c_bus_params) {
    printf("Unsupported bus ID %d\n", (int)i2c_bus_id);
    return -1;
  }

  alt_i2c_ctrl_init(i2c_bus_id);

  if (resetfifo == 1) {
    /* Reset the FIFOs */
    REGISTER(i2c_bus_params->i2c_base_offset + MCU_I2C0_CTRL_FLUSH_CFG) =
        (MCU_I2C0_CTRL_FLUSH_CFG_FLUSH_CMD_MSK | MCU_I2C0_CTRL_FLUSH_CFG_FLUSH_WDATA_MSK |
         MCU_I2C0_CTRL_FLUSH_CFG_FLUSH_RDATA_MSK);
  }

  if (start == 1) {
    retVal = i2c_cmd_fifo_write(i2c_bus_params, ALT_I2C_START_SEQ, 0);
    if (retVal != 0) {
      printf("i2c_cmd_fifo_write START_SEQ error\n");
      return retVal;
    }
  }

  if (restart == 1) {
    retVal = i2c_cmd_fifo_write(i2c_bus_params, ALT_I2C_RESTART_SEQ, 0);
    if (retVal != 0) {
      printf("i2c_cmd_fifo_write RESTART_SEQ error\n");
      return retVal;
    }
  }

  /* Initiate I2C write command */
  if (device & ALT_I2C_10BITS_PREFIX) {
    uint8_t devId;

    /* 10Bits mode */
    /* Send 1 1 1 1 0 A9 A8 W */
    devId = (device >> 8) & 0x7F;
    retVal = i2c_cmd_fifo_write(
        i2c_bus_params, ALT_I2C_TX_SEQ | ((devId << 1) << (MCU_I2C0_CTRL_CMD_FIFO_ADDRESS_POS)), 0);
    if (retVal != 0) {
      printf("i2c_cmd_fifo_write device(high) error\n");
      return retVal;
    }

    /* Send A7 A6 A5 A4 A3 A2 A1 A0 */
    devId = device & 0xFF;
    retVal = i2c_cmd_fifo_write(i2c_bus_params,
                                ALT_I2C_TX_SEQ | (devId << MCU_I2C0_CTRL_CMD_FIFO_ADDRESS_POS), 0);
    if (retVal != 0) {
      printf("i2c_cmd_fifo_write device(low) error\n");
      return retVal;
    }
  } else {
    /* 7Bits mode */
    /* Send A6 A5 A4 A3 A2 A1 A0 W*/
    retVal = i2c_cmd_fifo_write(
        i2c_bus_params, ALT_I2C_TX_SEQ | ((device << 1) << (MCU_I2C0_CTRL_CMD_FIFO_ADDRESS_POS)),
        0);
    if (retVal != 0) {
      printf("i2c_cmd_fifo_write device error\n");
      return retVal;
    }
  }

  for (i = 0; retVal == 0 && i < length; ++i) {
    /* Send the I2C data */
    retVal = i2c_cmd_fifo_write(
        i2c_bus_params, ALT_I2C_TX_SEQ | (data[i] << MCU_I2C0_CTRL_CMD_FIFO_ADDRESS_POS), 0);
    if (retVal != 0) {
      printf("i2c_cmd_fifo_write TX_SEQ error\n");
      return retVal;
    }
  }

  if (stop == 1) {
    retVal = i2c_cmd_fifo_write(i2c_bus_params, ALT_I2C_STOP_SEQ, 1);
    if (retVal != 0) {
      printf("i2c_cmd_fifo_write STOP_SEQ error\n");
      return retVal;
    }
  }

  return retVal;
}

int i2c_basic_read(i2c_bus_e i2c_bus_id, uint16_t device, uint8_t* data, uint32_t length,
                   int resetfifo, int start, int restart, int stop) {
  unsigned int RegVal, rx_msg_len;
  i2c_struct_params_t* i2c_bus_params = i2c_bus_select(i2c_bus_id);
  int retVal = 0;

  if (!i2c_bus_params) {
    printf("Unsupported bus ID %d\n", (int)i2c_bus_id);
    return -1;
  }

  alt_i2c_ctrl_init(i2c_bus_id);
  if (resetfifo == 1) {
    /* Reset the FIFOs */
    REGISTER(i2c_bus_params->i2c_base_offset + MCU_I2C0_CTRL_FLUSH_CFG) =
        (MCU_I2C0_CTRL_FLUSH_CFG_FLUSH_CMD_MSK | MCU_I2C0_CTRL_FLUSH_CFG_FLUSH_WDATA_MSK |
         MCU_I2C0_CTRL_FLUSH_CFG_FLUSH_RDATA_MSK);
  }

  if (start == 1) {
    retVal = i2c_cmd_fifo_write(i2c_bus_params, ALT_I2C_START_SEQ, 0);
    if (retVal != 0) {
      printf("i2c_cmd_fifo_write START_SEQ error\n");
      return retVal;
    }
  }

  if (restart == 1) {
    retVal = i2c_cmd_fifo_write(i2c_bus_params, ALT_I2C_RESTART_SEQ, 0);
    if (retVal != 0) {
      printf("i2c_cmd_fifo_write RESTART_SEQ error\n");
      return retVal;
    }
  }

  /* Initiate I2C read command */
  if (device & ALT_I2C_10BITS_PREFIX) {
    uint8_t devId;

    /* 10Bits mode */
    /* Send 1 1 1 1 0 A9 A8 W */
    devId = (device >> 8) & 0x7F;
    retVal = i2c_cmd_fifo_write(
        i2c_bus_params,
        ALT_I2C_TX_SEQ | (((devId << 1) | 0x1) << MCU_I2C0_CTRL_CMD_FIFO_ADDRESS_POS), 0);
    if (retVal != 0) {
      printf("i2c_cmd_fifo_write device(high) error\n");
      return retVal;
    }

    /* Send A7 A6 A5 A4 A3 A2 A1 A0 */
    devId = device & 0xFF;
    retVal = i2c_cmd_fifo_write(i2c_bus_params,
                                ALT_I2C_TX_SEQ | (devId << MCU_I2C0_CTRL_CMD_FIFO_ADDRESS_POS), 0);
    if (retVal != 0) {
      printf("i2c_cmd_fifo_write device(low) error\n");
      return retVal;
    }
  } else {
    /* 7Bits mode */
    /* Send A6 A5 A4 A3 A2 A1 A0 R*/
    retVal = i2c_cmd_fifo_write(
        i2c_bus_params,
        ALT_I2C_TX_SEQ | ((((device << 1) | 0x1) << MCU_I2C0_CTRL_CMD_FIFO_ADDRESS_POS)), 0);
    if (retVal != 0) {
      printf("i2c_cmd_fifo_write device error\n");
      return retVal;
    }
  }

  rx_msg_len = 0;
  do {
    int max_rx_msg_len, i;

    max_rx_msg_len = ((length - rx_msg_len) / I2C_RDATA_FIFO_MAX_SIZE)
                         ? I2C_RDATA_FIFO_MAX_SIZE
                         : ((length - rx_msg_len) % I2C_RDATA_FIFO_MAX_SIZE);
    for (i = 0; retVal == 0 && i < max_rx_msg_len; ++i) {
      /* Receive the I2C data */
      if (i == max_rx_msg_len - 1) {
        /* Wait for all data to be received before reading it from FIFO */
        retVal = i2c_cmd_fifo_write(
            i2c_bus_params,
            ALT_I2C_RX_SEQ | /* Last byte read should not be acknowledged, i.e. NACK */
                (rx_msg_len + max_rx_msg_len == length ? ALT_I2C_NACK_SEQ : 0),
            /* We should wait for completion of the transaction only for
             * non last subpacket */
            (max_rx_msg_len == I2C_RDATA_FIFO_MAX_SIZE &&
             ((length - rx_msg_len) != I2C_RDATA_FIFO_MAX_SIZE))
                ? 1
                : 0);
      } else {
        retVal = i2c_cmd_fifo_write(i2c_bus_params, ALT_I2C_RX_SEQ, 0);
      }
    }

    if (retVal != 0 || rx_msg_len + max_rx_msg_len == length) {
      unsigned int tmpRegVal = 0;

      if (retVal != 0) {
        printf("i2c_cmd_fifo_write RX_SEQ error\n");
        tmpRegVal = retVal;
      }

      /* Send STOP_SEQ immediately after an error or the length condition reach */
      if (stop == 1) {
        retVal = i2c_cmd_fifo_write(i2c_bus_params, ALT_I2C_STOP_SEQ, 1);
        if (retVal != 0) {
          printf("i2c_cmd_fifo_write STOP_SEQ error\n");
          return retVal;
        }
      }

      if (tmpRegVal != 0) {
        return retVal;
      }
    }

    RegVal = REGISTER(i2c_bus_params->i2c_base_offset + MCU_I2C0_CTRL_RDATA_FIFO_STATUS);
    RegVal = (RegVal & MCU_I2C0_CTRL_RDATA_FIFO_STATUS_RDATA_STATUS_MSK) >>
             MCU_I2C0_CTRL_RDATA_FIFO_STATUS_RDATA_STATUS_POS;
    while (RegVal) {
      uint32_t rxfifo_data;

      rxfifo_data = REGISTER(i2c_bus_params->i2c_base_offset + MCU_I2C0_CTRL_RDATA_FIFO);

      memcpy(&data[rx_msg_len], ((uint8_t*)&rxfifo_data), (RegVal / 4) ? 4 : (RegVal % 4));
      rx_msg_len += ((RegVal / 4) ? 4 : (RegVal % 4));

      RegVal = REGISTER(i2c_bus_params->i2c_base_offset + MCU_I2C0_CTRL_RDATA_FIFO_STATUS);
      RegVal = (RegVal & MCU_I2C0_CTRL_RDATA_FIFO_STATUS_RDATA_STATUS_MSK) >>
               MCU_I2C0_CTRL_RDATA_FIFO_STATUS_RDATA_STATUS_POS;
    }
  } while (rx_msg_len < length);

  return retVal;
}

int i2c_read_basic_mode(uint16_t device, i2c_devid_mode_e mode, uint32_t address,
                        i2c_addrlen_e addrLen, uint32_t len, uint8_t* pDest, i2c_bus_e i2c_bus_id) {
  int retVal = 0;
  uint8_t addr[3];
  uint16_t devId;

  if (I2C_DEVID_MODE_10BITS == mode) {
    devId = (device & ALT_I2C_10BITS_MASK) | ALT_I2C_10BITS_PREFIX;
  } else if (I2C_DEVID_MODE_7BITS == mode) {
    devId = device & ALT_I2C_7BITS_MASK;
  } else {
    printf("Incorrect bit mode %lu\n\r", (uint32_t)mode);
    return -1;
  }

  switch (addrLen) {
    case I2C_ADDRLEN_1BYTE:
      addr[0] = address & 0xFF;
      break;

    case I2C_ADDRLEN_2BYTES:
      addr[0] = (address >> 8) & 0xFF;
      addr[1] = address & 0xFF;
      break;

    case I2C_ADDRLEN_3BYTES:
      addr[0] = (address >> 16) & 0xFF;
      addr[1] = (address >> 8) & 0xFF;
      addr[2] = address & 0xFF;
      break;

    default:
      printf("Incorrect addrLen %lu\n\r", (uint32_t)addrLen);
      return -1;
  }

  retVal = i2c_basic_write(i2c_bus_id, devId, addr, /*len*/ (uint32_t)addrLen, /*resetfifo*/ 1,
                           /*start*/ 1,
                           /*restart*/ 0, /*stop*/ 0);
  if (retVal == 0) {
    retVal = i2c_basic_read(i2c_bus_id, devId, pDest, /*len*/ len, /*resetfifo*/ 0, /*start*/ 0,
                            /*restart*/ 1,
                            /*stop*/ 1);
  }

  return retVal;
}

int i2c_write_basic_mode(uint16_t device, i2c_devid_mode_e mode, uint32_t address,
                         i2c_addrlen_e addrLen, uint32_t len, const uint8_t* pSrc,
                         i2c_bus_e i2c_bus_id) {
  int retVal = 0;
  uint8_t* data;
  uint16_t devId;

  if (I2C_DEVID_MODE_10BITS == mode) {
    devId = (device & ALT_I2C_10BITS_MASK) | ALT_I2C_10BITS_PREFIX;
  } else if (I2C_DEVID_MODE_7BITS == mode) {
    devId = device & ALT_I2C_7BITS_MASK;
  } else {
    printf("Incorrect bit mode %lu\n\r", (uint32_t)mode);
    return -1;
  }

  data = (uint8_t*)malloc((size_t)len + addrLen);
  if (!data) {
    printf("I2C malloc failed\n\r");
    return -1;
  }

  switch (addrLen) {
    case I2C_ADDRLEN_1BYTE:
      data[0] = address & 0xFF;
      break;

    case I2C_ADDRLEN_2BYTES:
      data[0] = (address >> 8) & 0xFF;
      data[1] = address & 0xFF;
      break;

    case I2C_ADDRLEN_3BYTES:
      data[0] = (address >> 16) & 0xFF;
      data[1] = (address >> 8) & 0xFF;
      data[2] = address & 0xFF;
      break;

    default:
      printf("Incorrect addrLen %lu\n\r", (uint32_t)addrLen);
      free(data);
      return -1;
  }

  memcpy((void*)&data[addrLen], (void*)pSrc, (size_t)len);
  retVal = i2c_basic_write(i2c_bus_id, devId, data, /*len*/ len + addrLen, /*resetfifo*/ 1,
                           /*start*/ 1, /*restart*/ 0,
                           /*stop*/ 1);
  free(data);
  return retVal;
}
