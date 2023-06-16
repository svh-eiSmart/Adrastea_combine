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
/* Standard includes. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* CMSIS-RTOS V2 includes. */
#include "cmsis_os2.h"

/* DRV includes */
#include "DRV_SPI.h"

/* Notice: if you are using SW CS, please check gpio is initialized. */
#include "DRV_GPIO.h"

#define MAX_TRANSFER_SIZE (4096)
#define DEFAULT_TEST_SIZE (64)
#define SPI_TEST_MAX_WAIT_TIMES (10000)

static uint8_t transferRxData[MAX_TRANSFER_SIZE] = {0};
static uint8_t transferTxData[MAX_TRANSFER_SIZE] = {0};
static uint32_t event_count[SPI_BUS_MAX][3] = {0};
static bool is_transfer_complete[SPI_BUS_MAX] = {0};

#ifdef ALT1255
#define SPI_TEST_SPIM_BUS SPI_BUS_0
#else
#define SPI_TEST_SPIM_BUS SPI_BUS_1
#endif
static SPI_Handle spim_handle, spis_handle;
static int loopCount = 1;

#define SPI_TEST_BLOCK_API

/* refer to GPIO setting in the ALT125X/SPI/include/if_cfg/interfaces_config_alt125x.h
  when you used GPIO for SPI SS signal, please make sure below configuration.
  #define SPIMx_EN_PIN_SET	ALT1250_PIN_UNDEFINED
*/
static GPIO_Id SPI_TEST_SS_GPIO_ID = MCU_GPIO_ID_UNDEF;

/* Prototypes- SPI user app callback */
void SPI_SignalEvent(SPI_Bus_Id bus_id, uint32_t SPI_events, void *user_data);

void SPI_SignalEvent(SPI_Bus_Id bus_id, uint32_t SPI_events, void *user_data) {
  /* handle ISR if needed */
  switch (SPI_events) {
    case SPI_EVENT_TRANSFER_COMPLETE:
      event_count[bus_id][0]++;
      // is_transfer_complete[bus_id] = true;
      break;
    case SPI_EVENT_DATA_LOST:
      event_count[bus_id][1]++;
      break;
    default:
      event_count[bus_id][2]++;
      break;
  }
  is_transfer_complete[bus_id] = true;
}

static void reset_evt_count(SPI_Bus_Id bus_id) {
  for (int i = 0; i < 3; i++) event_count[bus_id][i] = 0;
}

static void dump_evt_counter(SPI_Bus_Id bus_id) {
  printf("\r\n====== EVT Counters =======");
  printf("\r\n Transfer Complete: %lu", event_count[bus_id][0]);
  printf("\r\n Data Lost: %lu", event_count[bus_id][1]);
  printf("\r\n Others: %lu", event_count[bus_id][2]);
  printf("\r\n===========================\r\n\n");
}

int spitest_set_ss_active(int val) {  // 0:de-active; otherwise, active
  if (SPI_TEST_SS_GPIO_ID == MCU_GPIO_ID_UNDEF) return (-1);

  if (DRV_GPIO_SetDirectionOutAndValue(SPI_TEST_SS_GPIO_ID, val) != DRV_GPIO_OK) {
    printf("\r\n Failed to set SPI SS (GPIO id-%d) %d", SPI_TEST_SS_GPIO_ID, val);
    return (-2);
  }

  return 0;
}

int do_spim_transfer_test(char *s) {
  SPI_Transfer transfer_data;
  DRV_SPI_Status ret_val = 0;
  uint32_t test_len = 0, idx, argc = 0;
  char *tok, *strgp;

  strgp = s;
  tok = strsep(&strgp, " ");
  if (!tok) {
    test_len = DEFAULT_TEST_SIZE;
  } else {
    test_len = (uint32_t)strtoul(tok, NULL, 10);
    argc++;
  }

  if (test_len == 0 || test_len > MAX_TRANSFER_SIZE) {
    printf("\r\nTest length should be less or equal to %u\r\n", MAX_TRANSFER_SIZE);
    return (0);
  }

  printf("************************************************\r\n");
  printf(" SPI master transfer example\r\n");
  printf("************************************************\r\n");

  /* SPI initialize */
  ret_val = DRV_SPI_Initialize(SPI_TEST_SPIM_BUS);
  if (ret_val != SPI_ERROR_NONE && ret_val != SPI_ERROR_INITIALED) {
    printf("\r\n Failed to initialize SPI Master. error-%d", ret_val);
    return (0);
  }

  /* Create handle and register callback function and event */
  if ((ret_val = DRV_SPI_OpenHandle(SPI_TEST_SPIM_BUS, &spim_handle, SPI_SignalEvent, NULL)) !=
      SPI_ERROR_NONE) {
    printf("\r\n Failed to Open handle. error:%d!!", ret_val);
  }

  /* Set up the transfer data */
  for (idx = 0; idx < test_len; idx++) {
    transferTxData[idx] = (idx + loopCount) % 256;
    transferRxData[idx] = 0;
  }
  loopCount++;

  /* Print out transmit buffer */
  printf("\r\n Sent:\r\n");
  for (idx = 0U; idx < test_len; idx++) {
    if ((idx & 0x0f) == 0) printf("\r\n");

    printf(" %02X", transferTxData[idx]);
  }
  printf("\r\n");

  /* Prepare transfer data and call transfer API. */
  spitest_set_ss_active(0);
  is_transfer_complete[SPI_TEST_SPIM_BUS] = false;
  memset(&transfer_data, 0, sizeof(SPI_Transfer));
  transfer_data.slave_id = SPIM_SS_1;
  transfer_data.send_buf = transferTxData;
  transfer_data.len = test_len;

  if ((ret_val = DRV_SPI_Send(SPI_TEST_SPIM_BUS, &transfer_data)) != SPI_ERROR_NONE) {
    printf("\r\n Failed to Send data. error:%d!!", ret_val);
    spitest_set_ss_active(1);
    return 0;
  }

  uint32_t sleep_cnt = 0;
  while (!is_transfer_complete[SPI_TEST_SPIM_BUS]) {
    osDelay(1);
    if (sleep_cnt > SPI_TEST_MAX_WAIT_TIMES) {
      printf("Error: Send timeout!");
      return 0;
    }
    sleep_cnt++;
  }

  spitest_set_ss_active(1);

  osDelay(200);
  printf("\r\n Receiving data... \r\n");
  spitest_set_ss_active(0);

  is_transfer_complete[SPI_TEST_SPIM_BUS] = false;
  memset(transferRxData, 0, sizeof(transferRxData));
  memset(&transfer_data, 0, sizeof(SPI_Transfer));
  transfer_data.slave_id = SPIM_SS_1;
  transfer_data.recv_buf = transferRxData;
  transfer_data.len = test_len;

  if ((ret_val = DRV_SPI_Receive(SPI_TEST_SPIM_BUS, &transfer_data)) != SPI_ERROR_NONE) {
    printf("\r\n Failed to Receive data. error:%d!!", ret_val);
    spitest_set_ss_active(1);
    return 0;
  }

  sleep_cnt = 0;
  while (!is_transfer_complete[SPI_TEST_SPIM_BUS]) {
    osDelay(1);
    if (sleep_cnt > SPI_TEST_MAX_WAIT_TIMES) {
      printf("Error: Receive timeout!");
      return 0;
    }
    sleep_cnt++;
  }

  printf("\r\n Received:\r\n");
  for (idx = 0U; idx < test_len; idx++) {
    if ((idx & 0x0f) == 0) printf("\r\n");

    printf(" %02X", transferRxData[idx]);
  }

  if (memcmp(transferTxData, transferRxData, test_len))
    printf("\r\n\n Warn: Transmitted & Received are not match.\r\n");
  else
    printf("\r\n\n Pass: Transmitted & Received are match.\r\n");

  return 0;
}

int do_spim_send_test(char *s) {
  SPI_Transfer transfer_data;
  DRV_SPI_Status ret_val;
  uint32_t test_len = 0, idx, argc = 0, sleep_cnt = 0;
  char *tok, *strgp;

  strgp = s;
  tok = strsep(&strgp, " ");
  if (!tok) {
    test_len = DEFAULT_TEST_SIZE;
  } else {
    test_len = (uint32_t)strtoul(tok, NULL, 10);
    argc++;
  }

  if (test_len == 0 || test_len > MAX_TRANSFER_SIZE) {
    printf("\r\n test length should be less or equal to %u", MAX_TRANSFER_SIZE);
    return (0);
  }

  printf("************************************************\r\n");
  printf(" SPI master send example\r\n");
  printf("************************************************\r\n");
  reset_evt_count(SPI_TEST_SPIM_BUS);

  /* SPI initialize */
  ret_val = DRV_SPI_Initialize(SPI_TEST_SPIM_BUS);
  if (ret_val != SPI_ERROR_NONE && ret_val != SPI_ERROR_INITIALED) {
    printf("\r\n Failed to initialize SPI Master. error-%d", ret_val);
    return (0);
  }

  /* Create handle and register callback function and event */
  if ((ret_val = DRV_SPI_OpenHandle(SPI_TEST_SPIM_BUS, &spim_handle, SPI_SignalEvent, NULL)) !=
      SPI_ERROR_NONE) {
    printf("\r\n Failed to Open handle. error:%d!!", ret_val);
  }

  /* Set up the transfer data */
  for (idx = 0; idx < test_len; idx++) {
    transferTxData[idx] = (idx + loopCount) % 256;
    transferRxData[idx] = 0;
  }
  loopCount++;

  /* Print out transmit buffer */
  printf("\r\n Sent:\r\n");
  for (idx = 0U; idx < test_len; idx++) {
    if ((idx & 0x0f) == 0) printf("\r\n");

    printf(" %02X", transferTxData[idx]);
  }
  printf("\r\n\n");

  /* Prepare transfer data and call transfer API. */
  spitest_set_ss_active(0);
  is_transfer_complete[SPI_TEST_SPIM_BUS] = false;
  memset(&transfer_data, 0, sizeof(SPI_Transfer));
  transfer_data.slave_id = SPIM_SS_1;
  transfer_data.send_buf = transferTxData;
  transfer_data.recv_buf = NULL;
  transfer_data.len = test_len;

  if ((ret_val = DRV_SPI_Send(SPI_TEST_SPIM_BUS, &transfer_data)) != SPI_ERROR_NONE) {
    printf("\r\n Failed to Send data. error:%d!!", ret_val);
    spitest_set_ss_active(1);
    return (0);
  }

  while (!is_transfer_complete[SPI_TEST_SPIM_BUS]) {
    osDelay(1);
    if (sleep_cnt > SPI_TEST_MAX_WAIT_TIMES) {
      printf("Error: Send timeout!");
      return 0;
    }
    sleep_cnt++;
  }

  spitest_set_ss_active(1);

  dump_evt_counter(SPI_TEST_SPIM_BUS);
  return 0;
}

/*
 * This command is used for spi_receive demo.
 */
int do_spim_receive_test(char *s) {
  SPI_Transfer transfer_data;
  DRV_SPI_Status ret_val;
  uint32_t test_len, idx, argc = 0, sleep_cnt = 0;
  char *tok, *strgp;

  /* input parameter handling */
  strgp = s;
  tok = strsep(&strgp, " ");
  if (!tok) {
    test_len = DEFAULT_TEST_SIZE;
  } else {
    test_len = (uint32_t)strtoul(tok, NULL, 10);
    argc++;
  }

  if (test_len == 0 || test_len > MAX_TRANSFER_SIZE) {
    printf("\r\n test length should be less or equal to %u", MAX_TRANSFER_SIZE);
    return (0);
  }

  printf("************************************************\r\n");
  printf(" SPI master receive example\r\n");
  printf("************************************************\r\n");
  reset_evt_count(SPI_TEST_SPIM_BUS);

  /* SPI initialize */
  ret_val = DRV_SPI_Initialize(SPI_TEST_SPIM_BUS);
  if (ret_val != SPI_ERROR_NONE && ret_val != SPI_ERROR_INITIALED) {
    printf("\r\n Failed to initialize SPI Master. error-%d", ret_val);
    return (0);
  }

  /* Create handle and register callback function and event */
  if ((ret_val = DRV_SPI_OpenHandle(SPI_TEST_SPIM_BUS, &spim_handle, SPI_SignalEvent, NULL)) !=
      SPI_ERROR_NONE) {
    printf("\r\n Failed to Open handle. error:%d!!", ret_val);
  }

  spitest_set_ss_active(0);

  is_transfer_complete[SPI_TEST_SPIM_BUS] = false;
  memset(transferRxData, 0, sizeof(transferRxData));
  memset(&transfer_data, 0, sizeof(SPI_Transfer));
  transfer_data.slave_id = SPIM_SS_1;
  transfer_data.send_buf = NULL;
  transfer_data.recv_buf = transferRxData;
  transfer_data.len = test_len;

  if ((ret_val = DRV_SPI_Receive(SPI_TEST_SPIM_BUS, &transfer_data)) != SPI_ERROR_NONE) {
    printf("\r\n Failed to Receive data. error:%d!!", ret_val);
    spitest_set_ss_active(1);
    return (0);
  }

  while (!is_transfer_complete[SPI_TEST_SPIM_BUS]) {
    osDelay(1);
    if (sleep_cnt > SPI_TEST_MAX_WAIT_TIMES) {
      printf("\r\n Error: Receive timeout!");
      return 0;
    }
    sleep_cnt++;
  }
  spitest_set_ss_active(1);

  printf("\r\n Received:\r\n");
  for (idx = 0U; idx < test_len; idx++) {
    if ((idx & 0x0f) == 0) printf("\r\n");

    printf(" %02X", transferRxData[idx]);
  }
  printf("\r\n\n");

  dump_evt_counter(SPI_TEST_SPIM_BUS);
  return 0;
}

/** ------------------------------- SPIS examples ------------------------------- **/
static int spis_initialized = 0;

int do_spis_transfer_test(char *s) {
  SPI_Transfer transfer_data;
  int ret_val = 0, argc = 0;
  uint32_t test_len = 0, idx;
  char *tok, *strgp;

  strgp = s;
  tok = strsep(&strgp, " ");
  if (!tok) {
    test_len = DEFAULT_TEST_SIZE;
  } else {
    test_len = (uint32_t)strtoul(tok, NULL, 10);
    argc++;
  }

  if (test_len == 0 || test_len > MAX_TRANSFER_SIZE) {
    printf("\r\n Test length should be less or equal to %u", MAX_TRANSFER_SIZE);
    return (0);
  }

  printf("************************************************\r\n");
  printf(" SPI slave round trip example\r\n");
  printf("************************************************\r\n");
  if (!spis_initialized) {
    if (DRV_SPI_Initialize(SPI_BUS_2) != SPI_ERROR_NONE)
      printf("\r\n SPI slave driver init failed.");

    spis_initialized = 1;
  }

  is_transfer_complete[SPI_BUS_2] = false;
  memset(transferRxData, 0, sizeof(transferRxData));
  memset(&transfer_data, 0, sizeof(SPI_Transfer));
  transfer_data.send_buf = NULL;
  transfer_data.recv_buf = transferRxData;
  transfer_data.len = test_len;

  if ((ret_val = DRV_SPI_Receive(SPI_BUS_2, &transfer_data)) != SPI_ERROR_NONE) {
    printf("\r\n Failed to receive data. error:%d!!", ret_val);
    return ret_val;
  }

  is_transfer_complete[SPI_BUS_2] = false;
  memset(&transfer_data, 0, sizeof(SPI_Transfer));
  transfer_data.send_buf = transferRxData;
  transfer_data.recv_buf = NULL;
  transfer_data.len = test_len;
  if ((ret_val = DRV_SPI_Send(SPI_BUS_2, &transfer_data)) != SPI_ERROR_NONE) {
    printf("\r\n Failed to send data. error:%d!!", ret_val);
    spitest_set_ss_active(1);
    return ret_val;
  }

  osDelay(1000);

  printf("\r\n Received and sent:\r\n");
  for (idx = 0U; idx < test_len; idx++) {
    if ((idx & 0x0f) == 0) printf("\r\n");

    printf(" %02X", transferRxData[idx]);
  }
  printf("\r\n\n");

  return 0;
}

int do_spis_send_test(char *s) {
  SPI_Transfer transfer_data;
  int ret_val = 0, argc = 0;
  uint32_t test_len = 0, idx;
  char *tok, *strgp;

  strgp = s;
  tok = strsep(&strgp, " ");
  if (!tok) {
    test_len = DEFAULT_TEST_SIZE;
  } else {
    test_len = (uint32_t)strtoul(tok, NULL, 10);
    argc++;
  }

  if (test_len == 0 || test_len > MAX_TRANSFER_SIZE) {
    printf("\r\n Test length should be less or equal to %u\r\n", MAX_TRANSFER_SIZE);
    return (0);
  }

  printf("************************************************\r\n");
  printf(" SPI slave send example\r\n");
  printf("************************************************\r\n");
  reset_evt_count(SPI_BUS_2);
  if (!spis_initialized) {
    if (DRV_SPI_Initialize(SPI_BUS_2) != SPI_ERROR_NONE)
      printf("\r\n SPI slave driver init failed.");

    spis_initialized = 1;
  }

  /* Create handle and register callback function and event */
  if ((ret_val = DRV_SPI_OpenHandle(SPI_BUS_2, &spis_handle, SPI_SignalEvent, NULL)) !=
      SPI_ERROR_NONE) {
    printf("\r\n Failed to Open handle. error:%d!!", ret_val);
  }

  /* Set up the transfer data */
  for (idx = 0; idx < test_len; idx++) {
    transferTxData[idx] = (idx + loopCount) % 256;
    transferRxData[idx] = 0;
  }
  loopCount++;

  is_transfer_complete[SPI_BUS_2] = false;
  memset(&transfer_data, 0, sizeof(SPI_Transfer));
  transfer_data.send_buf = transferTxData;
  transfer_data.recv_buf = NULL;
  transfer_data.len = test_len;

  if ((ret_val = DRV_SPI_Send(SPI_BUS_2, &transfer_data)) != SPI_ERROR_NONE) {
    printf("\r\n Failed to send data. error:%d!!", ret_val);
    return ret_val;
  }

  osDelay(1000);

  printf("\r\n Sent:\r\n");
  for (idx = 0U; idx < test_len; idx++) {
    if ((idx & 0x0f) == 0) printf("\r\n");

    printf(" %02X", transferTxData[idx]);
  }
  printf("\r\n\n");

  dump_evt_counter(SPI_BUS_2);
  return 0;
}

int do_spis_receive_test(char *s) {
  SPI_Transfer transfer_data;
  int ret_val = 0, argc = 0;
  uint32_t test_len = 0, idx;
  char *tok, *strgp;

  strgp = s;
  tok = strsep(&strgp, " ");
  if (!tok) {
    test_len = DEFAULT_TEST_SIZE;
  } else {
    test_len = (uint32_t)strtoul(tok, NULL, 10);
    argc++;
  }

  if (test_len == 0 || test_len > MAX_TRANSFER_SIZE) {
    printf("\r\n Test length should be less or equal to %u\r\n", MAX_TRANSFER_SIZE);
    return (0);
  }

  printf("************************************************\r\n");
  printf(" SPI slave receive example\r\n");
  printf("************************************************\r\n");
  reset_evt_count(SPI_BUS_2);
  if (!spis_initialized) {
    if (DRV_SPI_Initialize(SPI_BUS_2) != SPI_ERROR_NONE)
      printf("\r\n SPI slave driver init failed.");

    spis_initialized = 1;
  }

  /* Create handle and register callback function and event */
  if ((ret_val = DRV_SPI_OpenHandle(SPI_BUS_2, &spis_handle, SPI_SignalEvent, NULL)) !=
      SPI_ERROR_NONE) {
    printf("\r\n Failed to Open handle. error:%d!!", ret_val);
  }

  is_transfer_complete[SPI_BUS_2] = false;
  memset(transferRxData, 0, sizeof(transferRxData));
  memset(&transfer_data, 0, sizeof(SPI_Transfer));
  transfer_data.send_buf = NULL;
  transfer_data.recv_buf = transferRxData;
  transfer_data.len = test_len;

  if ((ret_val = DRV_SPI_Receive(SPI_BUS_2, &transfer_data)) != SPI_ERROR_NONE) {
    printf("\r\n Failed to Receive data. error:%d!!", ret_val);
    return ret_val;
  }

  osDelay(1000);

  printf("\r\n Received: %lu bytes\r\n", DRV_SPI_GetDataCount(SPI_BUS_2));
  for (idx = 0U; idx < test_len; idx++) {
    if ((idx & 0x0f) == 0) printf("\r\n");

    printf(" %02X", transferRxData[idx]);
  }
  printf("\r\n\n");

  dump_evt_counter(SPI_BUS_2);
  return 0;
}
