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

#ifndef HTTP_EXAMPLE_H_
#define HTTP_EXAMPLE_H_

#include "apitest_main.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void do_Http_PwrMngmt_demo(char *s);

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Coap configuration
   Parameters may be modified here */

/* How many rounds should the application be run */
#define START_ITERATION 5

/* Standby */
#define STDBY 1000
/* HIFC timeout tuning */
#define HIFC_TIMEOUT 200

/* URC timeout */
#define TIMEOUT 100

#define DEFAULT NULL
#define GPM_DEFAULT 0

/* Logger utilities */
#define HTTP_LOCK() apitest_log_lock()
#define HTTP_UNLOCK() apitest_log_unlock()

#define HTTP_DBG_PRINT(...) \
  do {                      \
    HTTP_LOCK();            \
    printf(__VA_ARGS__);    \
    HTTP_UNLOCK();          \
  } while (0)

#define http_free(p) \
  do {               \
    free(p);         \
    p = NULL;        \
  } while (0)

/****************************************************************************
 * Private Data
 ****************************************************************************/

typedef struct {
  char uri[100];
  char headers[100];
  uint32_t chunklen;
  uint32_t total_size;
  unsigned char *payload;
  uint32_t tls_profile;
} http_config_t;

#undef HTTP_SECURE_MODE

#define dont_care 0
#define no_header ""

http_config_t prof_configuration[] = {
    /* Profile 0 Invalid */
    {},
    /* Profile #1 */
    {
#ifdef HTTP_SECURE_MODE
        .uri = "https://172.25.14.1/test_profile1",
#else
        .uri = "http://172.25.14.1/test_profile1",
#endif
        .headers = no_header,
        .chunklen = 1400,
        .total_size = 8000,
        .tls_profile = 1,
        .payload = NULL}};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Global Data
 ****************************************************************************/
int g_urc_response = -1;
uint32_t g_recv_checksum = 0;
uint32_t g_pending_data;
extern SemaphoreHandle_t app_log_mtx;
;

#endif /* HTTP_EXAMPLE_H_ */
