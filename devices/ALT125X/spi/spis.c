/*  ---------------------------------------------------------------------------

        (c) copyright 2017 Altair Semiconductor, Ltd. All rights reserved.

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

#include "FreeRTOS.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "portmacro.h"
#include "125X_mcu.h"
#include "semphr.h"
#include "spi.h"
#include "event_groups.h"
#include "task.h"
#include "if_mngr.h"
#include "serialMngrLogPort.h"
#include "serial_container.h"

/* SPI_Mode */
#define SPI_SLAVE_REV_DATA (0x0100)
#define SPI_SLAVE_MISO_IDLE_HIGH (0x0200)

/* SPI_PROPERTY */
#define SPI_SLAVE_SCK_IS_ACTIVE (1 << 0)    /* SCK input schmitt active */
#define SPI_SLAVE_MOSI_IS_ACTIVE (1 << 1)   /* MOSI input schmitt active */
#define SPI_SLAVE_MISO_DRIVE_LARGE (1 << 2) /* MISO drive strength large */

#define SPI_SLAVE_DEFAULT_PROPERTY (SPI_SLAVE_SCK_IS_ACTIVE | SPI_SLAVE_MOSI_IS_ACTIVE)

#define SPI_SLAVE_TX_QUEUELENGTH (1)
#define SPI_SLAVE_RX_QUEUELENGTH (1)  //(2)

#define SPI_SLAVE_MESSAGE_STATUS_INVALID_MSK (0x8000)

#ifdef CONFIG_SPI_MAX_PAYLOAD_SIZE
#define SPI_SLAVE_MAX_BUFSIZE (CONFIG_SPI_MAX_PAYLOAD_SIZE)
#else
#define SPI_SLAVE_MAX_BUFSIZE (2064)
#endif

/* Spi Slave Inner events. */
#define SPI_SLAVE_WRITE_EVENT (1 << 0)
#define SPI_SLAVE_READ_EVENT (1 << 1)
#define SPI_SLAVE_RESEND_EVENT (1 << 2)
#define SPI_SLAVE_CLOSE_EVENT (1 << 3)
#define SPI_SLAVE_RESET_EVENT (1 << 4)

#define SPI_SLAVE_NON_TIMEOUT (portMAX_DELAY)
#define SPI_SLAVE_RX_GETVALIDMESSAGE_DELAY (1 / portTICK_RATE_MS)
#define SPI_SLAVE_WAITH2DDOWN_DELAY (1 / portTICK_RATE_MS)
#define SPI_SLAVE_RESEND_INTERVAL (1 / portTICK_RATE_MS)
#define SPI_SLAVE_CLOSE_INTERVAL (10 / portTICK_RATE_MS)
#define SPI_SLAVE_WAITM2SDOWN_DELAY (10 / portTICK_RATE_MS)
#define SPI_SLAVE_WRITE_QUEUESEND_TIMEOUT (SPI_SLAVE_NON_TIMEOUT)
#define SPI_SLAVE_WRITE_QUEUERECEIVE_TIMEOUT (SPI_SLAVE_NON_TIMEOUT)
#define SPI_SLAVE_READ_QUEUESEND_TIMEOUT (SPI_SLAVE_NON_TIMEOUT)
#define SPI_SLAVE_READ_QUEUERECEIVE_TIMEOUT \
  (SPI_SLAVE_NON_TIMEOUT) /* If this value is portMAX_DELAY, it will be in blocking mode. */
#define SPI_SLAVE_TRANSFER_END_CHECK_DELAY (1 / portTICK_RATE_MS) /* 1 millisecond */
#define SPI_SLAVE_TRANSFER_END_CHECK_CNT (1000)                   /* 1seconds */

#define SPI_SLAVE_DEV_MAX (1)

static int devCount = 0;
static volatile int sleepReqOkPkt = 0;
static int initDone = 0;
static SemaphoreHandle_t transactionSemaphore = NULL; /* Interrupt at the end of transaction */

#define SPI_SLAVE_LOCK_TIMEOUT(handle, tmOut) xSemaphoreTake(handle, tmOut)
#define SPI_SLAVE_LOCK(handle) SPI_SLAVE_LOCK_TIMEOUT(handle, portMAX_DELAY)
#define SPI_SLAVE_TRYLOCK(handle) SPI_SLAVE_LOCK_TIMEOUT(handle, 0)
#define SPI_SLAVE_UNLOCK(handle) \
  do {                           \
    xSemaphoreGive(handle);      \
  } while (0)
#define SPI_SLAVE_CONDWAIT(handle, tmOut) \
  do {                                    \
    xSemaphoreTake(handle, tmOut);        \
  } while (0)
#define SPI_SLAVE_CONDSIGNAL(handle)             \
  do {                                           \
    if (pdPASS == xSemaphoreGive(handle)) break; \
  } while (1)
#define SPI_SLAVE_CONDSIGNAL_FROM_ISR(handle, TaskWoken)      \
  do {                                                        \
    if (xSemaphoreGiveFromISR(handle, TaskWoken) != pdTRUE) { \
      ;                                                       \
    }                                                         \
  } while (0)

//#define CONFIG_SPIS_DEBUG
#ifdef CONFIG_SPIS_DEBUG
#define SPISDPRT(severity, fmt, ...)                                                         \
  do {                                                                                       \
    if (severity <= 3)                                                                       \
      fprintf(stderr, "\n\r[%s:%d(%d)]: " fmt, __FILE__, __LINE__, severity, ##__VA_ARGS__); \
  } while (0)
#else
#define SPISDPRT(severity, args...)
#endif

#define SPI_SLAVE_EVENT_TASK_STACK_SIZE configMINIMAL_STACK_SIZE

#ifndef CONFIG_SPI_SLAVE_RX_CALLBACK_TASK_STACK_SIZE
#define SPI_SLAVE_RX_CALLBACK_TASK_STACK_SIZE configMINIMAL_STACK_SIZE
#else
#define SPI_SLAVE_RX_CALLBACK_TASK_STACK_SIZE \
  ((((CONFIG_SPI_SLAVE_RX_CALLBACK_TASK_STACK_SIZE) + (4) - 1u) & ~((4) - 1u)) / 4)
#endif

/* When selected Spi_Debug optiuon in menuconfig task stack size. */
#define SPI_SLAVE_DEBUG_TASK_STACK_SIZE configMEDIUM_STACK_SIZE

// move it to header if necessary
typedef enum _spi_ioctl_id_t {
  IOCTAL_SPI_RECV_CB_FUNC,
  IOCTAL_SPI_WRITE_MODE_CNTRL,
  IOCTAL_SPI_READ_ABORT
} spi_ioctl_id_t;

typedef enum {
  SPIS_LOG_CRITICAL,
  SPIS_LOG_ERR,
  SPIS_LOG_WARN,
  SFPS_LOG_NOR,
  SPIS_LOG_INF,
  SPIS_LOG_DBG,
  SPIS_LOG_VERBOSE,
  SPIS_LOG_SEVERITY_NUM
} SPIS_LOG_SEVERITY_t;

struct stSpiSlaveMessage {
  uint16_t status;
  uint16_t buffsize;
  uint16_t len;
  uint16_t recv_len;
  char *body;
};

struct stSpiSlaveData {
  int devId;
  EventGroupHandle_t eventHandle;
  TaskHandle_t eventTaskHandle;
  TaskHandle_t readTaskHandle;
  QueueHandle_t txMessageQueue;
  QueueHandle_t rxMessageQueue;
  struct stSpiSlaveMessage *txDataBuffer[SPI_SLAVE_TX_QUEUELENGTH];
  struct stSpiSlaveMessage *rxDataBuffer[SPI_SLAVE_RX_QUEUELENGTH];
  struct stSpiSlaveMessage *dummyBuffer;
  SemaphoreHandle_t txDataBufferMutex;
  SemaphoreHandle_t rxDataBufferMutex;
  SemaphoreHandle_t mngTableMutex;
  uint8_t *readBuffer;
  serialRxProcessFp_t rxProcess; /* Rx callback handle */
  void *rxProcessCookie;         /* Rx callback ProcessCookie */
  void *sleepProcess;            /* Sleep packet rcv callback handle */
  enum hwSer_tx_mode txBlockingMode;
  SemaphoreHandle_t writeMutex;          /* Blocking mode write mutex. */
  SemaphoreHandle_t readMutex;           /* read mutex. */
  SemaphoreHandle_t writeBlockSemaphore; /* Use blocking mode. */
};

static struct stSpiSlaveData *devData[SPI_SLAVE_DEV_MAX];
static struct stSpiSlaveMessage *abortMsg = NULL; /* Receive abort packet */

#define ROUND_UP(x, n) (((x) + (n)-1u) & ~((n)-1u))

static inline uint16_t max(uint16_t a, uint16_t b) { return a > b ? a : b; }

static int SpiSlaveTransferOneMessage(struct stSpiSlaveData *dev, struct stSpiSlaveMessage *outMsg,
                                      int resetInd);

void spis_assert(const char *pcMessage);

/* interrupts declaration */
void spis0_interrupt_handler(void);

void spis_assert(const char *pcMessage) {
  (void)pcMessage;
  printf("Assert!!message - %s", pcMessage);
  configASSERT(0);
  //	for (;;);
}

#define SPI_SLAVE_ASSERT(expression)                        \
  do {                                                      \
    if (0 == (expression)) spis_assert("spi_slave assert"); \
  } while (0)

static int spis_write(void *handle, const uint8_t *buf, size_t bufSize, size_t recvSize);
static int spis_read(void *handle, uint8_t *buf, size_t bufSize);
/*****************************************************************************
 *  Function: SpiSlaveGetValidTxMessage
 *  Parameters: dev- Spi slave manage table pointer.
 *  Returns: struct stSpiSlaveMessage pointer.
 *  Description: Get an available Tx message buffer.
 *****************************************************************************/
static struct stSpiSlaveMessage *SpiSlaveGetValidTxMessage(struct stSpiSlaveData *dev) {
  int num = 0;
  struct stSpiSlaveMessage *ret = NULL;

  SPI_SLAVE_LOCK(dev->txDataBufferMutex);
  for (num = 0; num < SPI_SLAVE_TX_QUEUELENGTH; num++) {
    if (!(dev->txDataBuffer[num]->status & SPI_SLAVE_MESSAGE_STATUS_INVALID_MSK)) {
      dev->txDataBuffer[num]->status |= SPI_SLAVE_MESSAGE_STATUS_INVALID_MSK;

      ret = dev->txDataBuffer[num];
      break;
    }
  }
  SPI_SLAVE_UNLOCK(dev->txDataBufferMutex);
  if (ret) {
    SPISDPRT(SPIS_LOG_DBG, "Debug: Get valid Tx message buffer success.\n");
  } else {
    SPISDPRT(SPIS_LOG_WARN, "Warning: Get valid Tx message buffer failed.\n");
  }
  return ret;
}

/*****************************************************************************
 *  Function: SpiSlaveGetValidRxMessage
 *  Parameters: dev- Spi slave manage table pointer.
 *  Returns: struct stSpiSlaveMessage pointer.
 *  Description: Wait until available Rx message buffer can be acquired.
 *****************************************************************************/
static struct stSpiSlaveMessage *SpiSlaveGetValidRxMessage(struct stSpiSlaveData *dev) {
  int num = 0;
  struct stSpiSlaveMessage *ret = NULL;

  SPI_SLAVE_LOCK(dev->rxDataBufferMutex);
  for (num = 0; num < SPI_SLAVE_RX_QUEUELENGTH; num++) {
    if (!(dev->rxDataBuffer[num]->status & SPI_SLAVE_MESSAGE_STATUS_INVALID_MSK)) {
      dev->rxDataBuffer[num]->status |= SPI_SLAVE_MESSAGE_STATUS_INVALID_MSK;

      ret = dev->rxDataBuffer[num];
      break;
    }
  }
  SPI_SLAVE_UNLOCK(dev->rxDataBufferMutex);
  if (ret) {
    SPISDPRT(SPIS_LOG_DBG, "Get valid Rx message buffer success.\n");
  } else {
    SPISDPRT(SPIS_LOG_WARN, "Get valid Rx message buffer failed.\n");
  }
  return ret;
}

/*****************************************************************************
 *  Function: SpiSlaveGetValidTxMessageCount
 *  Parameters: dev- Spi slave manage table pointer.
 *  Returns: Data count.
 *  Description: Wait until the Tx message buffer is available.
 *****************************************************************************/
static int SpiSlaveGetValidTxMessageCount(struct stSpiSlaveData *dev) {
  int num = 0;
  int count = 0;

  SPI_SLAVE_LOCK(dev->txDataBufferMutex);
  for (num = 0; num < SPI_SLAVE_TX_QUEUELENGTH; num++) {
    if (!(dev->txDataBuffer[num]->status & SPI_SLAVE_MESSAGE_STATUS_INVALID_MSK)) count++;
  }
  SPI_SLAVE_UNLOCK(dev->txDataBufferMutex);
  return count;
}

/*****************************************************************************
 *  Function: SpiSlaveReleaseTxMessage
 *  Parameters: dev- Spi slave manage table pointer.
 *              message- Target Tx message pointer.
 *  Returns: none
 *  Description: Reverts the state of the used Tx message data to valid.
 *****************************************************************************/
static void SpiSlaveReleaseTxMessage(struct stSpiSlaveData *dev,
                                     struct stSpiSlaveMessage *message) {
  SPI_SLAVE_LOCK(dev->txDataBufferMutex);

  message->status &= ~SPI_SLAVE_MESSAGE_STATUS_INVALID_MSK;
  SPI_SLAVE_UNLOCK(dev->txDataBufferMutex);
  SPISDPRT(SPIS_LOG_DBG, "Release Tx message buffer.\n");
}

/*****************************************************************************
 *  Function: SpiSlaveReleaseRxMessage
 *  Parameters: dev- Spi slave manage table pointer.
 *			   message- Target Rx message pointer.
 *  Returns: none
 *  Description: Reverts the state of the used Rx message data to valid.
 *****************************************************************************/
static void SpiSlaveReleaseRxMessage(struct stSpiSlaveData *dev,
                                     struct stSpiSlaveMessage *message) {
  SPI_SLAVE_LOCK(dev->rxDataBufferMutex);

  message->status &= ~SPI_SLAVE_MESSAGE_STATUS_INVALID_MSK;
  SPI_SLAVE_UNLOCK(dev->rxDataBufferMutex);
  SPISDPRT(SPIS_LOG_DBG, "Release Rx message buffer.\n");
}

/*****************************************************************************
 *  Function: SpiSlaveCreateMessage
 *  Parameters: data size.
 *  Returns:	struct stSpiSlaveMessage pointer.
 *  Description:	Create and set data for struct stSpiSlaveMessage.
 *****************************************************************************/
static struct stSpiSlaveMessage *SpiSlaveCreateMessage(uint16_t size) {
  struct stSpiSlaveMessage *message =
      (struct stSpiSlaveMessage *)pvPortMalloc(sizeof(struct stSpiSlaveMessage));
  if (!message) {
    SPISDPRT(SPIS_LOG_ERR, "Can't allocate memory for stSpiSlaveMessage\n");
    goto Exit;
  }

  message->buffsize = size;  // ROUND_UP(size, DCACHE_LINESIZE);
  message->body = (char *)pvPortMalloc(message->buffsize);
  if (!message->body) {
    SPISDPRT(SPIS_LOG_ERR, "Can't allocate memory for stSpiSlaveMessage->body\n");
    vPortFree(message);
    message = NULL;
    goto Exit;
  }

  message->status = 0;
  message->len = 0;
  message->recv_len = 0;

Exit:
  return message;
}

/*****************************************************************************
 *  Function: SpiSlaveDeleteMessage
 *  Parameters: message- stSpiSlaveMessage pointer.
 *  Returns: none
 *  Description: Release the struct stSpiSlaveMessage.
 *****************************************************************************/
static void SpiSlaveDeleteMessage(struct stSpiSlaveMessage *message) {
  vPortFree(message->body);
  message->body = NULL;
  vPortFree(message);
  message = NULL;
}

/*****************************************************************************
 *  Function: spis0_interrupt_handler
 *  Parameters: none
 *  Returns: none
 *  Description: Interrupt processing at the end of transaction.
 *****************************************************************************/
void spis0_interrupt_handler(void) {
  volatile uint32_t slaveIntStatusReg, slaveIntMaskReg;
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  slaveIntStatusReg = REGISTER(MCU_SPI_SLAVE_RD_CLR_INT);

  /* Disable the corresponding interrupt source(s) */
  slaveIntMaskReg = REGISTER(MCU_SPI_SLAVE_MASK_INT);
  slaveIntMaskReg |= slaveIntStatusReg;
  REGISTER(MCU_SPI_SLAVE_MASK_INT) = slaveIntMaskReg;

  if (slaveIntStatusReg) {
    /* Test whether slave request line should be de-asserted by SPI Slave driver or by host
     * interface driver */
    SPI_SLAVE_CONDSIGNAL_FROM_ISR(transactionSemaphore, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}

/*****************************************************************************
 *  Function: spis_readTask
 *  Parameters: spidev_slave Pointer
 *  Returns: none
 *  Description: Wait Rx Queun entry.
 *****************************************************************************/
static void spis_readTask(void *dev_slave) {
  struct stSpiSlaveData *dev = (struct stSpiSlaveData *)dev_slave;
  int pos = 0, bufSize = 0;

  SPISDPRT(SPIS_LOG_DBG, "Start read task.\n");

  while (1) {
    bufSize = spis_read(dev, dev->readBuffer, SPI_SLAVE_MAX_BUFSIZE);
    if (bufSize > 0) {
      if (dev->rxProcess != NULL) {
        SPI_SLAVE_LOCK(dev->mngTableMutex);
        for (pos = 0; pos < bufSize; pos++) {
          dev->rxProcess(dev->readBuffer[pos], dev->rxProcessCookie);
        }
        SPI_SLAVE_UNLOCK(dev->mngTableMutex);
      } else {
        /* CallBack Function is Null, Revocation Message. */
        SPISDPRT(SPIS_LOG_DBG, "Rcv callback is null. Revocation Message.\n");
      }
    } else {
      SPISDPRT(SPIS_LOG_CRITICAL, "spis_read() bufSize[%d] should not a negative value.\n",
               bufSize);
      SPI_SLAVE_ASSERT(bufSize);
    }
  }
}

/*****************************************************************************
 *  Function: SpiSlaveControllerInit
 *  Parameters: none
 *  Returns: 1:Pass / ERRCode:Failed
 *  Description:
 *  Remarks: Befoure Transaction call this.
 *****************************************************************************/
static void SpiSlaveControllerInit(struct stSpiSlaveData *dev) {
  uint32_t interrptMask = 0;

  /* Unmask and enable complete interrupt */
  interrptMask = REGISTER(MCU_SPI_SLAVE_MASK_INT);

  /* Mask all interrupts */
  interrptMask |= (MCU_SPI_SLAVE_MASK_INT_MASK_MULTI_PACKET_COMPLETE_INT_MSK |
                   MCU_SPI_SLAVE_MASK_INT_MASK_MISO_PACKET_CNT_INT_MSK |
                   MCU_SPI_SLAVE_MASK_INT_MASK_MOSI_PACKET_CNT_INT_MSK |
                   MCU_SPI_SLAVE_MASK_INT_MASK_MASTER_READY_INT_MSK);
  REGISTER(MCU_SPI_SLAVE_MASK_INT) = interrptMask;
  REGISTER(MCU_SPI_SLAVE_INT_EN) = 0;
}

/*****************************************************************************
 *  Function: SpiSlaveEventGroupTask
 *  Parameters: dev Pointer.
 *  Returns: none
 *  Description: Event wait Context.
 *****************************************************************************/
static void SpiSlaveEventGroupTask(void *devSlave) {
  EventBits_t bits = 0;
  int ret = -1;
  int resetInd = 0;
  struct stSpiSlaveData *dev = (struct stSpiSlaveData *)devSlave;
  struct stSpiSlaveMessage *message = NULL;

  SPISDPRT(SPIS_LOG_DBG, "Event wait task start.\n");

  while (1) {
    resetInd = 0;

    bits = xEventGroupWaitBits(
        dev->eventHandle,
        (SPI_SLAVE_WRITE_EVENT | SPI_SLAVE_RESEND_EVENT | SPI_SLAVE_CLOSE_EVENT |
         SPI_SLAVE_RESET_EVENT), /* wait Master Req & SPI_WRITE event*/
        pdFALSE,                 /* bits not clear */
        pdFALSE,                 /* Don't wait for both bits, either bit will do */
        SPI_SLAVE_NON_TIMEOUT);  /* Wait time */

    if ((bits & SPI_SLAVE_CLOSE_EVENT) != 0) {
      SPISDPRT(SPIS_LOG_DBG, "SPI_SLAVE_CLOSE_EVENT start.\n");
      xEventGroupClearBits(dev->eventHandle, SPI_SLAVE_CLOSE_EVENT);
      /* Prepare for Close */
      if (message) SpiSlaveReleaseTxMessage(dev, message);

      break;
    }

    if ((bits & SPI_SLAVE_RESET_EVENT) != 0) {
      SPISDPRT(SFPS_LOG_NOR, "SPI_SLAVE_RESET_EVENT start.\n");
      xEventGroupClearBits(dev->eventHandle, SPI_SLAVE_RESET_EVENT);
      /* TBD: Notification reset for Spi Master. */
      resetInd = 1;
      // message = &resetMessage;
    } else if ((bits & SPI_SLAVE_WRITE_EVENT) != 0) {
      SPISDPRT(SFPS_LOG_NOR, "SPI_SLAVE_WRITE_EVENT start.\n");
      xEventGroupClearBits(dev->eventHandle, SPI_SLAVE_WRITE_EVENT);
      xQueueReceive(dev->txMessageQueue, &message, SPI_SLAVE_WRITE_QUEUERECEIVE_TIMEOUT);
      if (!message) {
        SPISDPRT(SPIS_LOG_ERR, "Rcv Write Event, But non message. Cancel transaction.\n");
        continue;
      }
    } else {
      /* nop */
    }

    if ((bits & SPI_SLAVE_RESEND_EVENT) != 0) {
      SPISDPRT(SFPS_LOG_NOR, "SPI_SLAVE_RESEND_EVENT start.\n");
      xEventGroupClearBits(dev->eventHandle, SPI_SLAVE_RESEND_EVENT);
      /* Resend message, So not QueueReceive. */
      if (!message) {
        SPISDPRT(SPIS_LOG_ERR, "Rcv Resend Event, But non message. Cancel transaction\n");
        continue;
      }
    }

    ret = SpiSlaveTransferOneMessage(dev, message, resetInd);

    /* Choose end proccess */
    if ((bits & SPI_SLAVE_RESET_EVENT) != 0) {
      message = NULL;
      SPISDPRT(SPIS_LOG_DBG, "SPI_SLAVE_RESET_EVENT end.\n");
    } else if ((bits & (SPI_SLAVE_RESEND_EVENT | SPI_SLAVE_WRITE_EVENT)) != 0) {
      if (ret < 0) {
        /* Transaction failed. Kick Resend event */
        SPISDPRT(SPIS_LOG_WARN, "Transaction failed. Kick Resend event.\n");
        xEventGroupSetBits(dev->eventHandle, SPI_SLAVE_RESEND_EVENT);

        vTaskDelay(SPI_SLAVE_RESEND_INTERVAL);
      } else {
        SpiSlaveReleaseTxMessage(dev, message);
        message = NULL;

        if (dev->txBlockingMode == BLOCKING_MODE) {
          /* Release Blocking */
          SPI_SLAVE_CONDSIGNAL(dev->writeBlockSemaphore);
          SPISDPRT(SPIS_LOG_DBG, "Blocking mode now, Release Tx semaphore.\n");
        } else {
          if (SpiSlaveGetValidTxMessageCount(dev) < SPI_SLAVE_TX_QUEUELENGTH) {
            /* Tx Queue not empty. Next message write. */
            SPISDPRT(SPIS_LOG_DBG, "Tx Queue not empty. Next message write.\n");
            xEventGroupSetBits(dev->eventHandle, SPI_SLAVE_WRITE_EVENT);
          }
        }

        SPISDPRT(SPIS_LOG_DBG, "Transaction success.\n");
        SPISDPRT(SPIS_LOG_INF, "SPI_SLAVE_RESEND_EVENT | SPI_SLAVE_WRITE_EVENT end.\n");
      }
    } else {
      /* nop */
      SPISDPRT(SPIS_LOG_INF, "SPI_SLAVE_CLOSE_EVENT end.\n");
    }
  }

  /* Delete itself. */
  vTaskDelete(NULL);
}

SPIS_ERR_CODE spis_get_def_config(spis_config_t *pConfig) {
  memset(pConfig, 0x0, sizeof(spis_config_t));

  if (if_mngr_load_defconfig(IF_MNGR_SPIS0, pConfig) != IF_MNGR_SUCCESS) return SPIS_ERR_GENERIC;

  return SPIS_ERR_NONE;
}

/*****************************************************************************
 *  Function: spis_restore_state
 *  Parameters: [*slave_data]:spidev_slave_data
 *  Returns: 0: OK; error codes
 *  Description: slave state restore
 *****************************************************************************/
static int spis_restore_state(uint32_t mode) {
  spis_config_t mConfig;
  uint32_t RegVal;
  int ret_val = 0;

  if ((ret_val = spis_get_def_config(&mConfig)) != SPIS_ERR_NONE) {
    printf("SPI slave: error to load default config from MCU wizard. error:%d!\r\n", ret_val);
    return (-1);
  }

  RegVal = REGISTER(MCU_SPI_SLAVE_CFG);

  /* Set parameters */
  RegVal &= ~MCU_SPI_SLAVE_CFG_CPHA_MSK;
  if (mConfig.param.cpha) RegVal |= MCU_SPI_SLAVE_CFG_CPHA_MSK;

  RegVal &= ~MCU_SPI_SLAVE_CFG_CPOL_MSK;
  if (mConfig.param.cpol) RegVal |= MCU_SPI_SLAVE_CFG_CPOL_MSK;

  /* endian */
  RegVal &= ~MCU_SPI_SLAVE_CFG_ENDIAN_SWITCH_MSK;
  if (mConfig.param.endian) RegVal |= MCU_SPI_SLAVE_CFG_ENDIAN_SWITCH_MSK;

  RegVal &= ~MCU_SPI_SLAVE_CFG_SS_ACTIVE_HIGH_MSK;
  if (mConfig.param.ssMode) RegVal |= MCU_SPI_SLAVE_CFG_SS_ACTIVE_HIGH_MSK;

  /* Set parameters */
  RegVal &= ~MCU_SPI_SLAVE_CFG_REVERSAL_MSK;
  if (mode & SPI_SLAVE_REV_DATA) RegVal |= MCU_SPI_SLAVE_CFG_REVERSAL_MSK;

  RegVal &= ~MCU_SPI_SLAVE_CFG_MISO_IDLE_VALUE_MSK;
  if (mode & SPI_SLAVE_MISO_IDLE_HIGH) RegVal |= MCU_SPI_SLAVE_CFG_MISO_IDLE_VALUE_MSK;

  RegVal &= ~MCU_SPI_SLAVE_CFG_WORD_SIZE_MSK;
  /*cfg |= (host->cur_chip->word_size << SPI_SLAVE_CFG_WORD_SIZE_POS);*/
  RegVal |= (8 << MCU_SPI_SLAVE_CFG_WORD_SIZE_POS);
  RegVal |= MCU_SPI_SLAVE_CFG_INFINITE_MSK;

  /* It's up to host to choose clock frequency */

  REGISTER(MCU_SPI_SLAVE_CFG) = (RegVal);

  return 0;
}

/*****************************************************************************
 *  Function: spis_init
 *  Parameters: none
 *  Returns: 0- pass / error code
 *  Description:	 Initialize Spi Driver
 *****************************************************************************/
int spis_init(void) {
  // int ret = 0;

  if (initDone) {
    SPISDPRT(SPIS_LOG_ERR, "Spi slave initialized.\n");
    return -SPIS_ERR_INVAL;
  }

  /* Configure IO mux */
  if (if_mngr_config_io(IF_MNGR_SPIS0) != IF_MNGR_SUCCESS) {
    SPISDPRT(SPIS_LOG_ERR, "Failed to configure IO mux.\n");
    return -SPIS_ERR_INVAL;
  }

  transactionSemaphore = xSemaphoreCreateBinary();
  if (!transactionSemaphore) {
    SPISDPRT(SPIS_LOG_ERR, "TransactionSemaphore create failed.\n");
    SPI_SLAVE_ASSERT(transactionSemaphore);
  }

  /*
   * Create receive abort message *
   * To reduce memory, abortMsg is not use stSpiSlaveMessage() function.
   * Only struct size allocation.
   */
  abortMsg = (struct stSpiSlaveMessage *)pvPortMalloc(sizeof(struct stSpiSlaveMessage));
  if (!abortMsg) {
    SPISDPRT(SPIS_LOG_ERR, "Receiv abort message create failed.\n");
    vSemaphoreDelete(transactionSemaphore);
    transactionSemaphore = NULL;
    return -SPIS_ERR_NOMEM;
  }
  abortMsg->len = 0;

  NVIC_SetPriority(SPIS0_IRQn, 7);
  NVIC_EnableIRQ(SPIS0_IRQn);

  initDone = 1;

  SPISDPRT(SPIS_LOG_DBG, "SPI_INIT successfully.\n");

  return 0;
}

/*****************************************************************************
 *  Function: spis_open
 *  Parameters: none
 *  Returns: manage table pointer or NULL.
 *  Description:
 *****************************************************************************/
void *spis_open(void) {
  int ret = 0;
  struct stSpiSlaveData *SpiSlave = NULL;
  uint32_t mode = SPI_SLAVE_DEFAULT_PROPERTY; /* SPI configuration mode */
  int cnt = 0;

  /* check initialize */
  if (!initDone) {
    SPISDPRT(SPIS_LOG_ERR, "Spi slave not initialize.\n");
    return NULL;
  }

  /* check open */
  if (devCount) {
    SPISDPRT(SPIS_LOG_ERR, "Spi slave opened.\n");
    return NULL;
  }

  SpiSlave = (struct stSpiSlaveData *)pvPortMalloc(sizeof(struct stSpiSlaveData));
  if (!SpiSlave) {
    SPISDPRT(SPIS_LOG_ERR, "Can't allocate memory for stSpiSlaveData.\n");
    return NULL;
  }

  memset(SpiSlave, 0, sizeof(*SpiSlave));

  if (spis_restore_state(mode) != 0) {
    SPISDPRT(SPIS_LOG_ERR, "Failed to load SPI configuration.\n");
    goto outFree;
  }

  /* Initialize the SPI slave controller */
  SpiSlaveControllerInit(SpiSlave);

  SpiSlave->rxMessageQueue =
      xQueueCreate(SPI_SLAVE_RX_QUEUELENGTH, sizeof(struct stSpiSlaveMessage *));
  if (SpiSlave->rxMessageQueue == NULL) {
    SPISDPRT(SPIS_LOG_ERR, "Rx Message Queue create failed.\n");
    goto outFree;
  }

  SpiSlave->txMessageQueue =
      xQueueCreate(SPI_SLAVE_TX_QUEUELENGTH, sizeof(struct stSpiSlaveMessage *));
  if (SpiSlave->txMessageQueue == NULL) {
    SPISDPRT(SPIS_LOG_ERR, "Tx Message Queue create failed.\n");
    goto outRxQueueDelete;
  }

  SpiSlave->rxDataBufferMutex = xSemaphoreCreateMutex();
  if (!SpiSlave->rxDataBufferMutex) {
    SPISDPRT(SPIS_LOG_ERR, "Rx buffer mutex create failed.\n");
    goto outTxQueueDelete;
  }

  for (cnt = 0; cnt < SPI_SLAVE_RX_QUEUELENGTH; cnt++) {
    SpiSlave->rxDataBuffer[cnt] = SpiSlaveCreateMessage(SPI_SLAVE_MAX_BUFSIZE);
    if (!SpiSlave->rxDataBuffer[cnt]) {
      SPISDPRT(SPIS_LOG_ERR, "Rx buffer create failed. cnt = %d\n", cnt);
      goto outRxBufferDelete;
    }
  }

  SpiSlave->txDataBufferMutex = xSemaphoreCreateMutex();
  if (!SpiSlave->txDataBufferMutex) {
    SPISDPRT(SPIS_LOG_ERR, "Tx buffer mutex create failed.\n");
    goto outRxBufferDelete;
  }

  for (cnt = 0; cnt < SPI_SLAVE_TX_QUEUELENGTH; cnt++) {
    SpiSlave->txDataBuffer[cnt] = SpiSlaveCreateMessage(SPI_SLAVE_MAX_BUFSIZE);
    if (!SpiSlave->txDataBuffer[cnt]) {
      SPISDPRT(SPIS_LOG_ERR, "Tx buffer create failed. cnt = %d\n", cnt);
      goto outTxBufferDelete;
    }
  }

  /* Create dummy message buffer */
  SpiSlave->dummyBuffer = SpiSlaveCreateMessage(SPI_SLAVE_MAX_BUFSIZE);
  if (!SpiSlave->dummyBuffer) {
    SPISDPRT(SPIS_LOG_ERR, "dummy buffer create failed.\n");
    goto outTxBufferDelete;
  }

  SpiSlave->eventHandle = xEventGroupCreate();
  if (!SpiSlave->eventHandle) {
    SPISDPRT(SPIS_LOG_ERR, "EventHandleGroup create failed.\n");
    goto outDummyBufferDelete;
  }

  SpiSlave->mngTableMutex = xSemaphoreCreateMutex();
  if (!SpiSlave->mngTableMutex) {
    SPISDPRT(SPIS_LOG_ERR, "Rx callback mutex create failed.\n");
    goto outEventGroupDelete;
  }

  SpiSlave->writeBlockSemaphore = xSemaphoreCreateBinary();
  if (!SpiSlave->writeBlockSemaphore) {
    SPISDPRT(SPIS_LOG_ERR, "Tx semaphore create failed.\n");
    goto outRxMutexDelete;
  }

  SpiSlave->writeMutex = xSemaphoreCreateMutex();
  if (!SpiSlave->writeMutex) {
    SPISDPRT(SPIS_LOG_ERR, "write mutex create failed.\n");
    goto outTxSemaphoreDelete;
  }

  SpiSlave->readMutex = xSemaphoreCreateMutex();
  if (!SpiSlave->readMutex) {
    SPISDPRT(SPIS_LOG_ERR, "read mutex create failed.\n");
    goto outWriteMutexDelete;
  }

  devData[devCount] = SpiSlave;
  SpiSlave->devId = ++devCount;

  /* Event task */
  ret = xTaskCreate(
      (TaskFunction_t)SpiSlaveEventGroupTask, /* Task Function */
      "spi-slave-event_task",                 /* Task Name */
#ifdef CONFIG_SPI_DEBUG
      SPI_SLAVE_DEBUG_TASK_STACK_SIZE, /* Stack Depth (is not bytes. this size is valiable count. so
                                          16bit 100valiables = 200bytes allocation) */
#else
      SPI_SLAVE_EVENT_TASK_STACK_SIZE, /* Stack Depth (is not bytes. this size is valiable count. so
                                          16bit 100valiables = 200bytes allocation) */
#endif
      (void *)SpiSlave,            /* Params  */
      (tskIDLE_PRIORITY + 20),     /* Priority */
      &SpiSlave->eventTaskHandle); /* TaskHandle */
  if (!ret) {
    SPISDPRT(SPIS_LOG_ERR, "Wait event task create failed.\n");
    goto outReadMutexDelete;
  }

  /* Set default Tx mode */
  SpiSlave->txBlockingMode = BLOCKING_MODE;

  SPISDPRT(SFPS_LOG_NOR, "SPI_OPEN successfully\n");

  return SpiSlave;

outReadMutexDelete:
  --devCount;
  devData[devCount] = NULL;
  vSemaphoreDelete(SpiSlave->readMutex);
outWriteMutexDelete:
  vSemaphoreDelete(SpiSlave->writeMutex);
outTxSemaphoreDelete:
  vSemaphoreDelete(SpiSlave->writeBlockSemaphore);
outRxMutexDelete:
  vSemaphoreDelete(SpiSlave->mngTableMutex);
outEventGroupDelete:
  vEventGroupDelete(SpiSlave->eventHandle);
outTxBufferDelete:
  for (cnt = 0; cnt < SPI_SLAVE_TX_QUEUELENGTH; cnt++) {
    if (SpiSlave->txDataBuffer[cnt]) {
      SpiSlaveDeleteMessage(SpiSlave->txDataBuffer[cnt]);
    }
  }
  vSemaphoreDelete(SpiSlave->txDataBufferMutex);
outRxBufferDelete:
  for (cnt = 0; cnt < SPI_SLAVE_RX_QUEUELENGTH; cnt++) {
    if (SpiSlave->rxDataBuffer[cnt]) {
      SpiSlaveDeleteMessage(SpiSlave->rxDataBuffer[cnt]);
    }
  }
  vSemaphoreDelete(SpiSlave->rxDataBufferMutex);
outDummyBufferDelete:
  SpiSlaveDeleteMessage(SpiSlave->dummyBuffer);
outTxQueueDelete:
  vQueueDelete(SpiSlave->txMessageQueue);
outRxQueueDelete:
  vQueueDelete(SpiSlave->rxMessageQueue);
outFree:
  vPortFree(SpiSlave);
  SpiSlave = NULL;

  return NULL;
}

/*****************************************************************************
 *  Function: spis_close
 *  Parameters: handle - manage table pointer.
 *  Returns: 1- pass / 0- fail
 *  Description:
 *****************************************************************************/
int spis_close(void *handle) {
  struct stSpiSlaveData *dev = (struct stSpiSlaveData *)handle;
  int cnt = 0;

  if (!devCount) {
    /* Not Open */
    SPISDPRT(SPIS_LOG_ERR, "Spi slave not open.\n");
    return -SPIS_ERR_PERM;
  }

  if (!handle || devData[(dev->devId - 1)] != handle) {
    SPISDPRT(SPIS_LOG_ERR, "Parameter is invalid.\n");
    return -SPIS_ERR_INVAL;
  }

  /* Kick Close event. */
  xEventGroupSetBits(dev->eventHandle, SPI_SLAVE_CLOSE_EVENT);
  while (0 != (xEventGroupGetBits(dev->eventHandle) & SPI_SLAVE_CLOSE_EVENT)) {
    vTaskDelay(SPI_SLAVE_CLOSE_INTERVAL);
    SPISDPRT(SPIS_LOG_WARN, "Event group task close wait.\n");
  }
  /* Set Transaction end event Mask to Disable. */
  REGISTER(MCU_SPI_SLAVE_MOSI_PAC_AT_CNT_CTRL) =
      ~(MCU_SPI_SLAVE_MOSI_PAC_AT_CNT_CTRL_BD_CNT_INT_EN_MSK |
        0x3 << MCU_SPI_SLAVE_MOSI_PAC_AT_CNT_CTRL_BD_CNT_INT_MODE_POS |
        MCU_SPI_SLAVE_MOSI_PAC_AT_CNT_CTRL_BD_CNT_HW_EN_MSK |
        MCU_SPI_SLAVE_MOSI_PAC_AT_CNT_CTRL_BD_CNT_THRS_EN_MSK |
        (0x00 << MCU_SPI_SLAVE_MOSI_PAC_AT_CNT_CTRL_BD_CNT_THRS_VALUE_POS));

  /* Delete Resource */
  /* Tasks */
  if (dev->readTaskHandle) {
    vTaskDelete(dev->readTaskHandle);
    dev->readTaskHandle = NULL;
    vPortFree(dev->readBuffer);
  }

  /* Semaphore */
  vSemaphoreDelete(dev->readMutex);
  vSemaphoreDelete(dev->writeMutex);
  vSemaphoreDelete(dev->writeBlockSemaphore);
  /* Manage mutex */
  vSemaphoreDelete(dev->mngTableMutex);
  /* EventHandle */
  vEventGroupDelete(dev->eventHandle);
  /* Tx Buffer */
  for (cnt = 0; cnt < SPI_SLAVE_TX_QUEUELENGTH; cnt++) {
    if (dev->txDataBuffer[cnt]) {
      SpiSlaveDeleteMessage(dev->txDataBuffer[cnt]);
    }
  }
  vSemaphoreDelete(dev->txDataBufferMutex);
  /* Rx Buffer */
  for (cnt = 0; cnt < SPI_SLAVE_RX_QUEUELENGTH; cnt++) {
    if (dev->rxDataBuffer[cnt]) {
      SpiSlaveDeleteMessage(dev->rxDataBuffer[cnt]);
    }
  }
  vSemaphoreDelete(dev->rxDataBufferMutex);
  /* Dummy Buffer */
  SpiSlaveDeleteMessage(dev->dummyBuffer);

  /* Queues */
  xQueueReset(dev->txMessageQueue);
  vQueueDelete(dev->txMessageQueue);
  xQueueReset(dev->rxMessageQueue);
  vQueueDelete(dev->rxMessageQueue);

  --devCount;
  devData[(dev->devId - 1)] = NULL;
  vPortFree(dev);
  dev = NULL;
  SPISDPRT(SPIS_LOG_DBG, "SPI_CLOSE successfully\n");

  return 0;
}

/*****************************************************************************
 *  Function:		SpiSlaveTransferOneMessage
 *  Parameters:		(i)dev				Data table of spi slave.
 *					(i)outMsg			Message of slave to master.
 *If there is no message, please set @outMsg to NULL. (i)resetInd			Reset packet
 *status. Send reset packet is 1, otherwise 0.
 *  Returns:			Over 0:				Success. Message length in slave to
 *master.
 *					0:					Success. However if
 *@outMsg is NULL, Illegal/bad/unknown packet header is being received from the master. Under 0:
 *Fail. Description: The transaction between master and slave is executed.
 *****************************************************************************/
static int SpiSlaveTransferOneMessage(struct stSpiSlaveData *dev, struct stSpiSlaveMessage *outMsg,
                                      int resetInd) {
  uint16_t rxBuffSize = 0, rxDataSize = 0;
  uint16_t txDataSize = 0;
  struct stSpiSlaveMessage *in_msg = NULL;
  uint32_t ocpmBuffActive = 0;
  int totalBufSize;
  uint32_t slaveIntMaskReg;
  volatile uint32_t spiSlaveOcpmBufFifoStatus, spiSlaveOcpmOcpLenStatus;
  int ret = 0, i = 0;
  struct stSpiSlaveMessage *message = outMsg;

  if (outMsg != NULL) {
    txDataSize = outMsg->len;

    rxBuffSize = outMsg->recv_len;
    rxDataSize = outMsg->recv_len;
  }

  /* Flush & Reset RD/WR buffers right away before the transaction is started */
  SpiSlaveControllerInit(dev);
  REGISTER(MCU_SPI_SLAVE_OCPM_INIT) = 0xFFFFFFFF;

  /* Test for illegal/bad packet header transaction */
  if (rxDataSize == 0 && !message) {
    SpiSlaveControllerInit(dev);
    SPISDPRT(
        SPIS_LOG_ERR,
        "Illegal/bad/unknown packet header transaction from master. rxDataSize=%d, rxBuffSize=%d\n",
        rxDataSize, rxBuffSize);

    ret = rxDataSize;
    goto Exit;
  }
#if 0
  if (SPI_SLAVE_MAX_BUFSIZE < rxBuffSize || rxBuffSize < rxDataSize) {
    SpiSlaveControllerInit(dev);
    SPISDPRT(
        SPIS_LOG_ERR,
        "Illegal/bad/unknown packet header transaction from master. rxDataSize=%d, rxBuffSize=%d\n",
        rxDataSize, rxBuffSize);

    if (message) {
      rxBuffSize = 0;
      rxDataSize = 0;
    } else {
      ret = -1;
      goto Exit;
    }
  }
  SPISDPRT(SPIS_LOG_DBG,
           "1st stage Tx/Rx Packet Header Transaction has been successfully completed\n");
#endif
  /* Flush & Reset RD/WR buffers right away before the transaction is started */
  SpiSlaveControllerInit(dev);
  REGISTER(MCU_SPI_SLAVE_OCPM_INIT) = 0xFFFFFFFF;

  /* Test for master to slave transaction request */
  if (rxDataSize) {
    /* Allocate memory for RX SPI data transactions */
    in_msg = SpiSlaveGetValidRxMessage(dev);

    if (in_msg) in_msg->len = rxDataSize;
  }

  /* Calculate the total transaction size (simultaneous transaction should have the same size) */
  totalBufSize = (max(rxBuffSize, ROUND_UP(txDataSize, 4)));
  // totalCacheLineBufSize = totalBufSize;//ROUND_UP(totalBufSize, DCACHE_LINESIZE);

  /* Test for Slave to Master transaction */
  if (message && message->body && (message->len != 0)) {
    totalBufSize = message->len;

    SPISDPRT(SPIS_LOG_DBG, "Preparing Tx SPI Buffer (S2M), addr=%p, total/actual size=%d/%d\n",
             (uint32_t *)message->body, totalBufSize, message->len);

    /* Set DMA address of Tx SPI buffer */
    REGISTER(MCU_SPI_SLAVE_OCPM_RD_BD_ADDR) = (uint32_t)message->body;
    /* Set size of Tx SPI DMA buffer */
    REGISTER(MCU_SPI_SLAVE_OCPM_RD_BD_SIZE) = totalBufSize;

    ocpmBuffActive |= MCU_SPI_SLAVE_OCPM_BUFF_ACTIVE_RD_BUFF_ACTIVE_MSK;

    /* Set the whole message as 1 packet (i.e. set the MISO Atomic Packet Counter to 1 */
    REGISTER(MCU_SPI_SLAVE_MISO_PAC_AT_CNT) = 1;

    /* Generate interrupt at the end of the Tx buffer transmission */
    REGISTER(MCU_SPI_SLAVE_MISO_PAC_AT_CNT_CTRL) =
        (MCU_SPI_SLAVE_MISO_PAC_AT_CNT_CTRL_BD_CNT_INT_EN_MSK |
         0x3 << MCU_SPI_SLAVE_MISO_PAC_AT_CNT_CTRL_BD_CNT_INT_MODE_POS |
         MCU_SPI_SLAVE_MISO_PAC_AT_CNT_CTRL_BD_CNT_HW_EN_MSK |
         MCU_SPI_SLAVE_MISO_PAC_AT_CNT_CTRL_BD_CNT_THRS_EN_MSK |
         (0x00 << MCU_SPI_SLAVE_MISO_PAC_AT_CNT_CTRL_BD_CNT_THRS_VALUE_POS));

    /* Clear pending interrupts */
    REGISTER(MCU_SPI_SLAVE_RD_CLR_INT);

    /* Test for half-duplex transaction */
    /*  For full-duplex transaction the interrupt will be generated only by the longest operation,
     * i.e. RX transaction */
    if (!in_msg) {
      /* Enable the MISO Packet Counter Interrupt */
      slaveIntMaskReg = REGISTER(MCU_SPI_SLAVE_MASK_INT);
      slaveIntMaskReg &= ~MCU_SPI_SLAVE_MASK_INT_MASK_MISO_PACKET_CNT_INT_MSK;
      REGISTER(MCU_SPI_SLAVE_MASK_INT) = slaveIntMaskReg;
    }
  } else {
    /* Clear the MISO Atomic Packet Counter */
    REGISTER(MCU_SPI_SLAVE_MISO_PAC_AT_CNT) = 0;
  }

  /* Test for Master to Slave transaction */
  if (in_msg) {
    totalBufSize = (max(rxBuffSize, ROUND_UP(txDataSize, 4)));

    SPISDPRT(SPIS_LOG_DBG, "Preparing Rx SPI Buffer (M2S), addr=%p, tota/actuall size=%d/%d\n",
             (uint32_t *)in_msg->body, totalBufSize, in_msg->len);

    /* Set DMA address of Rx SPI buffer */
    REGISTER(MCU_SPI_SLAVE_OCPM_WR_BD_ADDR) = (uint32_t)in_msg->body;
    /* Set size of Rx SPI DMA buffer */
    REGISTER(MCU_SPI_SLAVE_OCPM_WR_BD_SIZE) = totalBufSize;

    ocpmBuffActive |= MCU_SPI_SLAVE_OCPM_BUFF_ACTIVE_WR_BUFF_ACTIVE_MSK;

    /* Set the whole message as 1 packet (i.e. set the MOSI Atomic Packet Counter to 1 */
    REGISTER(MCU_SPI_SLAVE_MOSI_PAC_AT_CNT) = 1;
    /* Generate interrupt at the end of the Rx buffer reception */
    REGISTER(MCU_SPI_SLAVE_MOSI_PAC_AT_CNT_CTRL) =
        (MCU_SPI_SLAVE_MOSI_PAC_AT_CNT_CTRL_BD_CNT_INT_EN_MSK |
         0x3 << MCU_SPI_SLAVE_MOSI_PAC_AT_CNT_CTRL_BD_CNT_INT_MODE_POS |
         MCU_SPI_SLAVE_MOSI_PAC_AT_CNT_CTRL_BD_CNT_HW_EN_MSK |
         MCU_SPI_SLAVE_MOSI_PAC_AT_CNT_CTRL_BD_CNT_THRS_EN_MSK |
         (0x00 << MCU_SPI_SLAVE_MOSI_PAC_AT_CNT_CTRL_BD_CNT_THRS_VALUE_POS));

    /* Clear pending interrupts */
    REGISTER(MCU_SPI_SLAVE_RD_CLR_INT);

    /* Enable the MOSI Packet Counter Interrupt */
    slaveIntMaskReg = REGISTER(MCU_SPI_SLAVE_MASK_INT);
    slaveIntMaskReg &= ~MCU_SPI_SLAVE_MASK_INT_MASK_MOSI_PACKET_CNT_INT_MSK;
    REGISTER(MCU_SPI_SLAVE_MASK_INT) = slaveIntMaskReg;

  } else
    /* Clear the MOSI Atomic Packet Counter */
    REGISTER(MCU_SPI_SLAVE_MOSI_PAC_AT_CNT) = 0;

  /* Set the actual packet size */
  REGISTER(MCU_SPI_SLAVE_CFG_PACKET) = ((totalBufSize << MCU_SPI_SLAVE_CFG_PACKET_PACKET_SIZE_POS) |
                                        (1 << MCU_SPI_SLAVE_CFG_PACKET_MULTI_PACKET_SIZE_POS));

  REGISTER(MCU_SPI_SLAVE_OCPM_BUFF_ACTIVE) = ocpmBuffActive;

  SPISDPRT(SPIS_LOG_DBG,
           "Notify the Master that Slave is ready for 2nd transaction. M2S=%d, S2M=%d, size=%d\n",
           (in_msg ? 1 : 0), (message ? 1 : 0), totalBufSize);

  SPI_SLAVE_CONDWAIT(transactionSemaphore, SPI_SLAVE_NON_TIMEOUT);

  spiSlaveOcpmBufFifoStatus = REGISTER(MCU_SPI_SLAVE_OCPM_BUF_FIFO_STATUS);
  spiSlaveOcpmOcpLenStatus = REGISTER(MCU_SPI_SLAVE_OCPM_OCP_LEN_STATUS);

  /* Wait until completion of the whole transaction */
  while ((spiSlaveOcpmBufFifoStatus &
          (MCU_SPI_SLAVE_OCPM_BUF_FIFO_STATUS_OCPM_MOSI_FIFO_STATUS_MSK |
           MCU_SPI_SLAVE_OCPM_BUF_FIFO_STATUS_OCPM_MISO_FIFO_STATUS_MSK)) ||
         spiSlaveOcpmOcpLenStatus) {
    spiSlaveOcpmBufFifoStatus = REGISTER(MCU_SPI_SLAVE_OCPM_BUF_FIFO_STATUS);
    spiSlaveOcpmOcpLenStatus = REGISTER(MCU_SPI_SLAVE_OCPM_OCP_LEN_STATUS);
  }

  /* delay 1000 * 1ms */
  for (i = 0; i < SPI_SLAVE_TRANSFER_END_CHECK_CNT; ++i) {
    if (0 == (REGISTER(MCU_SPI_SLAVE_MISO_PAC_AT_CNT)) &&
        (0 == REGISTER(MCU_SPI_SLAVE_MOSI_PAC_AT_CNT))) {
      break;
    }
    SPISDPRT(SPIS_LOG_DBG, "MISO_PAC_AT_CNT=%lu, MOSI_PAC_AT_CNT=%lu\n",
             (uint32_t)REGISTER(MCU_SPI_SLAVE_MISO_PAC_AT_CNT),
             (uint32_t)REGISTER(MCU_SPI_SLAVE_MOSI_PAC_AT_CNT));
    vTaskDelay(SPI_SLAVE_TRANSFER_END_CHECK_DELAY);
  }

  if (SPI_SLAVE_TRANSFER_END_CHECK_CNT <= i) {
    SPISDPRT(SPIS_LOG_DBG, "MISO_PAC_AT_CNT=%lu, MOSI_PAC_AT_CNT=%lu\n",
             (uint32_t)REGISTER(MCU_SPI_SLAVE_MISO_PAC_AT_CNT),
             (uint32_t)REGISTER(MCU_SPI_SLAVE_MOSI_PAC_AT_CNT));
    SPISDPRT(SPIS_LOG_ERR, "2nd stage Payload Transaction has been aborted! M2S=%d, S2M=%d\n",
             (in_msg ? 1 : 0), (message ? 1 : 0));

    ret = -1;
  }

  /* Deactivate the OCMP buffer mechanism */
  REGISTER(MCU_SPI_SLAVE_OCPM_BUFF_ACTIVE) = 0x0;

  /* Test for available incoming SPI message */
  if (!ret && in_msg) {
    /* Add the incoming SPI message to the receive message queue (in a FIFO manner) */
    if (xQueueSendToBack(dev->rxMessageQueue, &in_msg, SPI_SLAVE_READ_QUEUESEND_TIMEOUT) !=
        pdPASS) {
      SPISDPRT(SPIS_LOG_ERR, "Rx queue add error. Release receive data. \n");
      SpiSlaveReleaseRxMessage(dev, in_msg);
      ret = -1;
    }
  } else {
    /* nop */
  }

  if (ret) {
    SpiSlaveControllerInit(dev);
    SPISDPRT(SPIS_LOG_DBG, "Rx/Tx/total size=%d/%d/%d\n", rxDataSize, txDataSize, totalBufSize);
  } else {
    SPISDPRT(SPIS_LOG_INF, "Transaction success, Rx/Tx/total size=%d/%d/%d\n", rxDataSize,
             txDataSize, totalBufSize);
    SPISDPRT(SPIS_LOG_DBG, "2nd stage Payload Transaction has been successfully completed.\n");
  }

  if ((ret == 0) && message) {
    ret = message->len;
  } else {
    /* nop */
  }

Exit:
  return ret;
}

/*****************************************************************************
 *  Function: spis_writeQueue
 *  Parameters:
 *					dev - manage table pointer.
 *					data - Send Data
 *					wait - Wait time(ms). If @wait is less than 0, this function
 *is in blocking mode. Returns: 1- pass / 0- fail Description:
 *****************************************************************************/
static int spis_writeQueue(struct stSpiSlaveData *dev, struct stSpiSlaveMessage *data) {
  int ret = 0;

  SPISDPRT(SPIS_LOG_DBG, "Write Queue start.\n");

  /* Insert tx_queue */
  if (xQueueSendToBack(dev->txMessageQueue, &data, SPI_SLAVE_WRITE_QUEUESEND_TIMEOUT) == pdPASS) {
    SPISDPRT(SPIS_LOG_DBG, "Write Queue Push Success.\n");

    /* Unblock the tasks waiting for the event */
    xEventGroupSetBits(dev->eventHandle, SPI_SLAVE_WRITE_EVENT);
    ret = 1;
  } else {
    SPISDPRT(SPIS_LOG_ERR, "Tx queue send failed.\n");
  }

  return ret;
}

/*****************************************************************************
 *  Function: spis_write
 *  Parameters:
 *			handle - spi slave manage table pointer.
 *			buf - Send data.
 *			bufSize - Send data length.
 *  Returns: write fail or Write message length.
 *  Description:
 *****************************************************************************/
static int spis_write(void *handle, const uint8_t *buf, size_t bufSize, size_t recvSize) {
  struct stSpiSlaveMessage *message;
  struct stSpiSlaveData *dev = NULL;
  size_t ret = 0;

  if (!devCount) {
    SPISDPRT(SPIS_LOG_ERR, "SPI slave is not open.\n");
    return -SPIS_ERR_PERM;
  }

  dev = (struct stSpiSlaveData *)handle;
  if (!dev) {
    SPISDPRT(SPIS_LOG_ERR, "stSpiSlaveData is NULL.\n");
    return -SPIS_ERR_INVAL;
  }

  if (recvSize > SPI_SLAVE_MAX_BUFSIZE) {
    SPISDPRT(SPIS_LOG_ERR, "Receive buff size is invalid. recvSize=%d\n", recvSize);
    return -SPIS_ERR_INVAL;
  }

  if (!buf || SPI_SLAVE_MAX_BUFSIZE < bufSize) {
    SPISDPRT(SPIS_LOG_ERR, "Write data is invalid. bufAddr=%p, bufSize=%d\n", (uint32_t *)buf,
             bufSize);
    return -SPIS_ERR_INVAL;
  }

  SPI_SLAVE_LOCK(dev->writeMutex);

  message = SpiSlaveGetValidTxMessage(dev);
  if (message == NULL) {
    SPISDPRT(SPIS_LOG_ERR, "Get valid Tx message buffer failed.\n");
    taskYIELD();
    SPI_SLAVE_UNLOCK(dev->writeMutex);
    return -SPIS_ERR_NOSPC;
  }
  message->len = bufSize;
  message->recv_len = recvSize;

  memcpy(message->body, buf, message->len);

  ret = spis_writeQueue(dev, message);
  if (ret) {
    if (dev->txBlockingMode == BLOCKING_MODE) {
      SPI_SLAVE_CONDWAIT(dev->writeBlockSemaphore, SPI_SLAVE_NON_TIMEOUT);
    }

    ret = bufSize;
  } else {
    SpiSlaveReleaseTxMessage(dev, message);
    SPI_SLAVE_ASSERT(ret);
  }

  SPI_SLAVE_UNLOCK(dev->writeMutex);

  return ret;
}

/*****************************************************************************
 *  Function:		spis_read
 *  Parameters:		(i)handle		spi slave manage table pointer.
 *					(o)buf			Buffer for storing received data.
 *					(i)bufSize		@buf size.
 *  Returns:			Received data lengh.
 *  Description:		Reading spi buffer.
 *****************************************************************************/
static int spis_read(void *handle, uint8_t *buf, size_t bufSize) {
  struct stSpiSlaveData *dev = (struct stSpiSlaveData *)handle;
  struct stSpiSlaveMessage *message = NULL;
  size_t len = 0;

  if (!handle || !buf || !bufSize) {
    SPISDPRT(SPIS_LOG_ERR, "Read data is invalid. handle=%p, bufAddr=%p, bufSize=%d\n",
             (uint32_t *)handle, (uint32_t *)buf, bufSize);
    return -SPIS_ERR_INVAL;
  }

  if (dev->readTaskHandle && dev->readBuffer != buf) {
    SPISDPRT(SPIS_LOG_ERR, "A read task exists. Therefore it can not be read from the outside.\n");
    return -SPIS_ERR_INVAL;
  }

  SPI_SLAVE_LOCK(dev->readMutex);

  if (xQueueReceive(dev->rxMessageQueue, &message, SPI_SLAVE_READ_QUEUERECEIVE_TIMEOUT) != pdTRUE) {
    SPISDPRT(SPIS_LOG_ERR, "Failed to xQueueReceive().\n");
    SPI_SLAVE_UNLOCK(dev->readMutex);
    return -SPIS_ERR_GENERIC;
  }

  if (message) {
    if (message->len) {
      len = message->len > bufSize ? bufSize : message->len;
      memcpy(buf, message->body, len);
      SpiSlaveReleaseRxMessage(dev, message);
    } else {
      /* When message length 0 = receive "Read Abort" message. */
      SPISDPRT(SPIS_LOG_DBG, "Receive Read abort msg.\n");
      len = -SPIS_STATUS_ERR_CONNABORTED;
    }
  }

  SPI_SLAVE_UNLOCK(dev->readMutex);

  return len;
}

/*****************************************************************************
 *  Function: spis_readAbort
 *  Parameters:
 *	      (i)dev - pointer of SpiSlave Manage Table.
 *  Returns:	0- pass / errno- fail
 *  Description:
 *****************************************************************************/
static int spis_readAbort(struct stSpiSlaveData *dev) {
  int ret = 0;
  BaseType_t rslt = pdFAIL;

  /* When not reading. */
  if (!dev || !dev->rxMessageQueue) {
    SPISDPRT(SPIS_LOG_ERR, "Spi slave not open.\n");
    return -SPIS_ERR_INVAL;
  }

  /* When RxBuffer full. */
  if (!abortMsg) {
    SPISDPRT(SPIS_LOG_ERR, "Abort message is null.\n");
    return -SPIS_ERR_NOSPC;
  }

  rslt = SPI_SLAVE_TRYLOCK(dev->readMutex);
  if (pdPASS != rslt) {
    if (!xQueueSendToFront(dev->rxMessageQueue, &abortMsg, SPI_SLAVE_NON_TIMEOUT)) {
      SPISDPRT(SPIS_LOG_ERR, "RxQueue send fail for Read abort.\n");
      SPI_SLAVE_ASSERT(0);
    }
  } else {
    SPI_SLAVE_UNLOCK(dev->readMutex);
  }

  return ret;
}

/*****************************************************************************
 *  Function: SpiSlaveIoctl
 *  Parameters:
 *    handle - Pointer of SpiSlave Manage Table.
 *    ioctlId - Ioctl Process Id.
 *    cookie - App Cookie.
 *  Returns:	0- pass / errno- fail
 *  Description:
 *****************************************************************************/
int SpiSlaveIoctl(void *handle, spi_ioctl_id_t ioctlId, void *param, void *cookie) {
  struct stSpiSlaveData *dev = (struct stSpiSlaveData *)handle;
  int ret = 0;

  if (!devCount) {
    SPISDPRT(SPIS_LOG_ERR, "Spi slave is not open.\n");
    return -SPIS_ERR_PERM;
  }

  if (dev == NULL) {
    SPISDPRT(SPIS_LOG_ERR, "Spi slave managetable is NULL.\n");
    return -SPIS_ERR_INVAL;
  }

  switch (ioctlId) {
    case IOCTAL_SPI_RECV_CB_FUNC:
      if (!dev->readTaskHandle) {
        dev->readBuffer = (uint8_t *)pvPortMalloc(SPI_SLAVE_MAX_BUFSIZE);
        if (!dev->readBuffer) {
          SPISDPRT(SPIS_LOG_ERR, "Can't allocate memory for readBuffer.\n");
          break;
        }

        ret = xTaskCreate((TaskFunction_t)spis_readTask, /* Task Function */
                          "spi-slave-read_task",         /* Task Name */
#ifdef CONFIG_SPI_DEBUG
                          SPI_SLAVE_DEBUG_TASK_STACK_SIZE, /* Stack Depth (is not bytes. this size
                                                              is valiable count. so 16bit
                                                              100valiables = 200bytes allocation) */
#else
                          SPI_SLAVE_RX_CALLBACK_TASK_STACK_SIZE, /* Stack Depth (is not bytes. this
                                                                    size is valiable count. so 16bit
                                                                    100valiables = 200bytes
                                                                    allocation) */
#endif
                          (void *)dev,           /* Params  */
                          3,                     /* Prioprity */
                          &dev->readTaskHandle); /* TaskHandle */
        if (!ret) {
          SPISDPRT(SPIS_LOG_ERR, "Read task create failed.\n");
          vPortFree(dev->readBuffer);
          SPI_SLAVE_ASSERT(ret);
        }
      }

      SPI_SLAVE_LOCK(dev->mngTableMutex);
      /* Set CallBack function to Manage table. */
      SPISDPRT(SPIS_LOG_DBG, "Set rcv call back.\n");
      dev->rxProcess = (serialRxProcessFp_t)param;
      dev->rxProcessCookie = cookie;
      ret = 0;

      SPI_SLAVE_UNLOCK(dev->mngTableMutex);
      break;
    case IOCTAL_SPI_WRITE_MODE_CNTRL:
      /* Change Write blocking mode. */
      SPI_SLAVE_LOCK(dev->writeMutex);
      SPISDPRT(SPIS_LOG_DBG, "Change blocking mode.\n");
      dev->txBlockingMode = *((enum hwSer_tx_mode *)param);
      SPI_SLAVE_UNLOCK(dev->writeMutex);
      break;
    case IOCTAL_SPI_READ_ABORT:
      ret = spis_readAbort(dev);
      break;
    default:
      SPISDPRT(SPIS_LOG_ERR, "unknown Ioctl id.\n");
      break;
  }

  return ret;
}

int spis_send(void *handle, const uint8_t *buf, size_t bufSize) {
  if ((bufSize == 0) || (bufSize % 4 != 0)) {
    printf("SPI data size %d is not multiple of word(4 bytes)!\n", bufSize);
    return (-SPIS_ERR_INVAL);
  }

  return (int32_t)spis_write(handle, buf, bufSize, 0);
}

int spis_receive(void *handle, uint8_t *buf, size_t bufSize) {
  int ret_val;

  if ((bufSize == 0) || (bufSize % 4 != 0)) {
    printf("SPI data size %d is not multiple of word(4 bytes)!\n", bufSize);
    return (-SPIS_ERR_INVAL);
  }

  ret_val = (int32_t)spis_write(handle, buf, 0, bufSize);
  if (ret_val < 0) {
    printf("SPI Slave triggr failed!\n");
    return ret_val;
  }

  ret_val = spis_read(handle, buf, bufSize);
  if (ret_val <= 0) {
    if (-SPIS_STATUS_ERR_CONNABORTED == ret_val) {
      printf("spis_read abort\n");
      return ret_val;
    } else {
      printf("spis_read [errno=%d]\n", ret_val);
      return ret_val;
    }
  }
  return ret_val;
}
