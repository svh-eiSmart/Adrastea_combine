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

#ifndef COAP_EXAMPLE_H_
#define COAP_EXAMPLE_H_

#include "apitest_main.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void do_Coap_PwrMngmt_demo(char *s);

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Coap configuration
   Parameters may be modified here */

/* How many rounds should the application be run */
#define START_ITERATION 2

/* HIFC timeout tuning */
#define HIFC_TIMEOUT 200

/* URC timeout */
#define TIMEOUT 10000

#define DEFAULT NULL
#define GPM_DEFAULT 0

/* Profile in use */
#define COAP_PROFILE_1 1

/* IP configuration */
#define URL "coaps://coap.me:5683"
#define IP_TYPE COAP_IPTYPE_V4V6
#define SESSION_ID DEFAULT

/* Coap Security parameters */
#define COAP_DTLS_PROFILE_1 10
#define COAP_DTLS_MODE COAP_NONE_AUTH
#define COAP_AUTH_MODE COAP_UNSEC_MODE
#define COAP_RESUMPTION COAP_DTLS_RESUMP_SESSION_DISABLE
#define CIPHERLIST_LEN 64
#define CYPHERTYPE COAP_CIPHER_WHITE_LIST
#define CIPHERLIST "C02C;C030;009F"

/* Coap URI and protocol */
#define URI_MASK COAP_URI_ALL
#define COAP_PROTOCOL 0 /* Default - 45 Seconds */

/* Coap Command parameters */
#define URI DEFAULT
#define CONFIRMABLE COAP_CMD_CONFIRMED
#define TOKEN DEFAULT
#define CONTENT DEFAULT
#define DATA_LEN 1024
#define BLK_SIZE DATA_LEN
#define PKT_1 1
#define PKT_2 2
#define PKT_3 3
#define BLK_NO_1 0
#define BLK_NO_2 1
#define BLK_NO_3 2

/* Coap Options parameters  */
#define OPTION_NB 1
#define OPTION_SIZE 28
#define OPTION_TYPE 11
#define OPTION_VALUE "large-update"

#define CMD_NUM_OF_PARAM_MAX 12
#define CMD_PARAM_LENGTH 128

/* Logger utilities */
#define COAP_LOCK() apitest_log_lock()
#define COAP_UNLOCK() apitest_log_unlock()

#define COAP_DBG_PRINT(...) \
  do {                      \
    COAP_LOCK();            \
    printf(__VA_ARGS__);    \
    COAP_UNLOCK();          \
  } while (0)

/* States machine enum */
enum SM {
  coap_config = 0,
  put_packet1,
  put_packet2,
  put_packet3,
  get_packet1,
  get_packet2,
  get_packet3
};

/* Coap configuration data for GPM saving */
typedef struct {
  char dest_addr[64];
  int sessionId;
  Coap_ip_type ip_type;
  unsigned char dtls_profile;
  Coap_dtls_mode_e dtls_mode;
  Coap_dtls_auth_mode_e auth_mode;
  Coap_dtls_session_resumption_e session_resumption;
  Coap_cypher_filtering_type_e CipherListFilteringType;
  char CipherList[CIPHERLIST_LEN];
  Coap_option_uri_e uriMask;
} CoapGpmCfgData_t;

#endif /* COAP_EXAMPLE_H_ */
