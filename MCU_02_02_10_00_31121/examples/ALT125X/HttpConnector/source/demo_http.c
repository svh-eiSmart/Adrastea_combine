/****************************************************************************
 *
 *  (c) copyright 2021 Altair Semiconductor, Ltd. All rights reserved.
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

/* Standard includes. */
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* CMSIS-RTOS V2 includes. */
#include "cmsis_os2.h"

/* ALTCOM API includes */
#include "http/altcom_http.h"
#include "certmgmt/altcom_certmgmt.h"

/* Power management includes */
#include "sleep_notify.h"

/* App includes */
#include "apitest_main.h"
#include "demo_http.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define OS_TICK_PERIOD_MS (1000 / osKernelGetTickFreq())
#define MAX_URC_WAIT_MS 2000
#define ITER_NO 2
#define HTTPDEMO_CMD_PARAM_1 (1)
#define HTTPDEMO_CMD_PARAM_2 (2)
#define CMD_NUM_OF_PARAM_MAX 12

#define DEMO_HEADER "Content-Type: application/octet-stream;aaa: 123;bbb: 456;ccc: 789"

#define HTTP_SLEEP_TIME_MS 15000
#define HTTP_HEXDUMP_LEN 256

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
 * Exernal Symbol
 ****************************************************************************/
extern bool param_field_alloc(char *argv[]);
extern void param_field_free(char *argv[]);
extern int parse_arg(char *s, char *argv[]);

/****************************************************************************
 * Private Type
 ****************************************************************************/

enum demo_http_secure_mode_t {
  DEMO_HTTPS_MUTUAL,
  DEMO_HTTPS_CLIENTONLY,
  DEMO_HTTPS_SERVERONLY,
  DEMO_HTTPS_NOAUTH,
  DEMO_HTTP_NOSECURE,
  DEMO_HTTP_MAX,
};

/****************************************************************************
 * Private Data
 ****************************************************************************/
static char *root_cert = "ca.crt";
static char *client_cert = "client.crt";
static char *client_key = "client.key";
static char *default_user = "iotuser";
static char *default_password = "iotuser123";
static uint8_t *read_buf = NULL;
static bool is_demo = false;
static osMessageQueueId_t urc_que = NULL;
static struct {
  char uri[100];
  char uris[100];
  char headers[100];
  uint32_t chunklen;
  uint32_t total_size;
  unsigned char *payload;
} prof_configuration = {.uri = "http://192.168.10.131:8080/upload/crudtest",
                        .uris = "https://192.168.10.131:8090/upload/crudtest",
                        .headers = DEMO_HEADER,
                        .chunklen = 1400,
                        .total_size = 8000,
                        .payload = NULL};

/****************************************************************************
 * Private Function
 ****************************************************************************/
static Http_err_code_e generate_payload(void) {
  unsigned char c = 0x1;
  if (prof_configuration.payload == NULL) {
    prof_configuration.payload = malloc(prof_configuration.total_size);
    if (prof_configuration.payload == NULL) {
      HTTP_DBG_PRINT("No room for payload.\r\n");
      return HTTP_FAILURE;
    }

    for (int i = 0; i < (int)prof_configuration.total_size; i++) {
      prof_configuration.payload[i] = c;
      c++;
    }
  }

  return HTTP_SUCCESS;
}

static void unregister_callbacks(Http_profile_id_e p) {
  altcom_http_event_register(p, HTTP_CMD_PUTCONF_EV, NULL, NULL);
  altcom_http_event_register(p, HTTP_CMD_POSTCONF_EV, NULL, NULL);
  altcom_http_event_register(p, HTTP_CMD_DELCONF_EV, NULL, NULL);
  altcom_http_event_register(p, HTTP_CMD_GETRCV_EV, NULL, NULL);
  altcom_http_event_register(p, HTTP_CMD_SESTERM_EV, NULL, NULL);
  altcom_http_event_register(p, HTTP_CMD_PRSPRCV_EV, NULL, NULL);
}

static Http_err_code_e init_demo(void) {
  /* Prepare read buffer */
  if (read_buf == NULL) {
    read_buf = malloc(HTTP_MAX_DATA_LENGTH);
    if (read_buf == NULL) {
      HTTP_DBG_PRINT("read buffer malloc failed\r\n");
      return HTTP_FAILURE;
    }
  }

  /* Prepare urc queue */
  if (urc_que == NULL) {
    urc_que = osMessageQueueNew(5, sizeof(void *), NULL);
    if (urc_que == NULL) {
      HTTP_DBG_PRINT("create urc_que failed\r\n");
    }
  }

  /* Generate test payload */
  if (generate_payload() == HTTP_FAILURE) {
    return HTTP_FAILURE;
  }

  is_demo = true;
  return HTTP_SUCCESS;
}

static void deinit_demo(void) {
  is_demo = false;

  /* Free elements & delete queue */
  uint32_t i, remain = osMessageQueueGetCount(urc_que);
  void *urc;
  osStatus_t status;

  for (i = 0; i < remain; i++) {
    status = osMessageQueueGet(urc_que, &urc, 0, osWaitForever);
    if (status != osOK) {
      HTTP_DBG_PRINT("osMessageQueueGet failed, i:%lu, status: %ld\r\n", i, (int32_t)status);
      break;
    }

    free(urc);
  }

  status = osMessageQueueDelete(urc_que);
  if (status != osOK) {
    HTTP_DBG_PRINT("osMessageQueueDelete failed, status: %ld\r\n", (int32_t)status);
  }

  urc_que = NULL;

  /* Free read buffer */
  http_free(read_buf);

  /* Free generated payload */
  http_free(prof_configuration.payload);

  /* Unregister all callbacks */
  Http_profile_id_e id;

  for (id = HTTP_PROFILE_ID1; id < HTTP_PROFILE_MAX; id++) {
    unregister_callbacks(id);
  }
}

static Http_err_code_e wait_resp(Http_profile_id_e p, bool read_resp) {
  uint32_t urc_cnt = 0;
  Http_err_code_e ret = HTTP_SUCCESS;

  do {
    osStatus_t status;
    Http_event_t *evt;

    /* Wait for URC */
    status = osMessageQueueGet(urc_que, &evt, 0, MAX_URC_WAIT_MS * OS_TICK_PERIOD_MS);
    if (status == osOK) {
      if (evt->err_code != STATE_TRANSACT_OK) {
        HTTP_DBG_PRINT("Request failed, errcode %ld, http_status %lu\r\n", (int32_t)evt->err_code,
                       (uint32_t)evt->http_status);
        ret = HTTP_FAILURE;
        free(evt);
        break;
      }

      if (!read_resp) {
        free(evt);
        break;
      }

      uint16_t chunk_len, read_len, avail_len;
      uint32_t pend_len;

      avail_len = evt->avail_size;
      while (avail_len > 0) {
        chunk_len = avail_len > HTTP_MAX_DATA_LENGTH ? HTTP_MAX_DATA_LENGTH : (uint16_t)avail_len;
        ret = altcom_http_read_data(p, chunk_len, read_buf, &read_len, &pend_len);
        if (ret == HTTP_SUCCESS) {
          HTTP_DBG_PRINT("chunk_len: %hu, read_len: %hu, pend_len: %lu\r\n", chunk_len, read_len,
                         pend_len);
          assert(read_len <= chunk_len);

          hexdump(read_buf, read_len);
        } else {
          HTTP_DBG_PRINT("altcom_http_read_data failed\r\n");
          break;
        }

        avail_len = (uint16_t)pend_len;
      }

      free(evt);
      if (ret != HTTP_SUCCESS) {
        break;
      }

      urc_cnt++;
      HTTP_DBG_PRINT("urc_cnt: %lu\r\n", urc_cnt);
    } else if ((status == osErrorTimeout)) {
      if (urc_cnt == 0) {
        HTTP_DBG_PRINT("Response timeout\r\n");
        ret = HTTP_FAILURE;
        break;
      }

      if (read_resp && (urc_cnt != 0)) {
        HTTP_DBG_PRINT("No more data to read\r\n");
      }

      break;
    } else {
      HTTP_DBG_PRINT("Fail to read queue, status = %ld\r\n", (int32_t)status);
      assert(false);
    }
  } while (true);

  return ret;
}

static Http_err_code_e post_put_command(Http_profile_id_e p, Http_cmd_method_e cmd, uint32_t size) {
  size_t len, off;
  Http_err_code_e ret = HTTP_SUCCESS, status;
  uint32_t chunklen = prof_configuration.chunklen;
  Http_command_data_t http_send_command;

  /* Headers */
  memset(&http_send_command, 0, sizeof(Http_command_data_t));
  if (strlen(prof_configuration.headers) > 0) {
    len = strlen(prof_configuration.headers);
    http_send_command.headers = malloc(len + 1);
    if (http_send_command.headers == NULL) {
      HTTP_DBG_PRINT("No room for headers. Profile=%d\r\n", p);
      return HTTP_FAILURE;
    }

    snprintf(http_send_command.headers, len + 1, prof_configuration.headers);
  }

  /* Payload allocation */
  if (chunklen > 0) {
    http_send_command.data_to_send = malloc(chunklen);
    if (http_send_command.data_to_send == NULL) {
      HTTP_DBG_PRINT("No room for chunk. Profile=%d\r\n", p);
      ret = HTTP_FAILURE;
      goto end;
    }
  } else {
    HTTP_DBG_PRINT("No Chunk length. Profile=%d\r\n", p);
    ret = HTTP_FAILURE;
    goto end;
  }

  /* Static parameters */
  http_send_command.cmd = cmd;
  http_send_command.profile_id = p;

  altcom_http_event_register(p, HTTP_CMD_SESTERM_EV, (void *)http_event_cb, (void *)0xDEADBEEF);
  altcom_http_event_register(p, HTTP_CMD_PRSPRCV_EV, (void *)http_event_cb, (void *)0xDEADBEEF);

  if (cmd == HTTP_CMD_PUT) {
    altcom_http_event_register(p, HTTP_CMD_PUTCONF_EV, (void *)http_event_cb, (void *)0xDEADBEEF);
  } else {
    altcom_http_event_register(p, HTTP_CMD_POSTCONF_EV, (void *)http_event_cb, (void *)0xDEADBEEF);
  }

  HTTP_DBG_PRINT("Sending %s request\r\n", cmd == HTTP_CMD_POST ? "POST" : "PUT");
  off = 0;
  http_send_command.pending_data = size;
  do {
    http_send_command.data_len =
        http_send_command.pending_data > chunklen ? chunklen : http_send_command.pending_data;
    http_send_command.pending_data -= http_send_command.data_len;
    memcpy(http_send_command.data_to_send, &prof_configuration.payload[off],
           http_send_command.data_len);
    off += http_send_command.data_len;
    status = altcom_http_send_cmd(p, &http_send_command);
    if (status == HTTP_FAILURE) {
      HTTP_DBG_PRINT("Error in altcom_http_send_cmd() configuration. Profile=%d\r\n", p);
      ret = HTTP_FAILURE;
      goto end;
    }

    HTTP_DBG_PRINT("data_len: %hu, pending_data: %lu\r\n", http_send_command.data_len,
                   http_send_command.pending_data);
  } while (http_send_command.pending_data > 0);

  HTTP_DBG_PRINT("Waiting for %s/PRSPRCV URC\r\n", cmd == HTTP_CMD_PUT ? "PUTCONF" : "POSTCONF");

  ret = wait_resp(p, true);
  HTTP_DBG_PRINT("wait_resp ret: %lu\r\n", (uint32_t)ret);

end:
  http_free(http_send_command.headers);
  http_free(http_send_command.data_to_send);
  if (http_send_command.cmd == HTTP_CMD_PUT) {
    altcom_http_event_register(p, HTTP_CMD_PUTCONF_EV, NULL, NULL);
    altcom_http_event_register(p, HTTP_CMD_PRSPRCV_EV, NULL, NULL);
  }

  if (http_send_command.cmd == HTTP_CMD_POST) {
    altcom_http_event_register(p, HTTP_CMD_PUTCONF_EV, NULL, NULL);
    altcom_http_event_register(p, HTTP_CMD_PRSPRCV_EV, NULL, NULL);
  }

  altcom_http_event_register(p, HTTP_CMD_SESTERM_EV, NULL, NULL);
  return ret;
}

static Http_err_code_e get_command(Http_profile_id_e p) {
  int len;
  Http_err_code_e ret = HTTP_SUCCESS;
  Http_command_data_t http_send_command;
  /* Headers */
  memset(&http_send_command, 0, sizeof(Http_command_data_t));
  if (strlen(prof_configuration.headers) > 0) {
    len = strlen(prof_configuration.headers);
    http_send_command.headers = malloc(len + 1);
    if (http_send_command.headers == NULL) {
      HTTP_DBG_PRINT("No room for headers. Profile=%d\r\n", p);
      return HTTP_FAILURE;
    }
    snprintf(http_send_command.headers, len + 1, prof_configuration.headers);
  }

  /* Static parameters */
  http_send_command.cmd = HTTP_CMD_GET;
  http_send_command.profile_id = p;
  http_send_command.pending_data = 0;

  altcom_http_event_register(p, HTTP_CMD_GETRCV_EV, (void *)http_event_cb, (void *)0xDEADBEEF);
  altcom_http_event_register(p, HTTP_CMD_SESTERM_EV, (void *)http_event_cb, (void *)0xDEADBEEF);

  HTTP_DBG_PRINT("Sending GET request\r\n");
  ret = altcom_http_send_cmd(p, &http_send_command);
  if (ret == HTTP_FAILURE) {
    HTTP_DBG_PRINT("Error in altcom_http_send_cmd() configuration. Profile=%d\r\n", p);
    goto end;
  }

  HTTP_DBG_PRINT("Waiting for GETRCV URC\r\n");

  ret = wait_resp(p, true);
  HTTP_DBG_PRINT("wait_resp ret: %lu\r\n", (uint32_t)ret);

end:
  http_free(http_send_command.headers);
  altcom_http_event_register(p, HTTP_CMD_GETRCV_EV, NULL, NULL);
  altcom_http_event_register(p, HTTP_CMD_SESTERM_EV, NULL, NULL);
  return ret;
}

static Http_err_code_e delete_command(Http_profile_id_e p) {
  int len;
  Http_err_code_e ret = HTTP_SUCCESS, status;
  Http_command_data_t http_send_command;
  /* Headers */
  memset(&http_send_command, 0, sizeof(Http_command_data_t));
  if (strlen(prof_configuration.headers) > 0) {
    len = strlen(prof_configuration.headers);
    http_send_command.headers = malloc(len + 1);
    if (http_send_command.headers == NULL) {
      HTTP_DBG_PRINT("No room for headers. Profile=%d\r\n", p);
      return HTTP_FAILURE;
    }
    snprintf(http_send_command.headers, len + 1, prof_configuration.headers);
  }

  /* Static parameters */
  http_send_command.cmd = HTTP_CMD_DELETE;
  http_send_command.profile_id = p;
  http_send_command.pending_data = 0;

  altcom_http_event_register(p, HTTP_CMD_DELCONF_EV, (void *)http_event_cb, (void *)0xDEADBEEF);
  altcom_http_event_register(p, HTTP_CMD_SESTERM_EV, (void *)http_event_cb, (void *)0xDEADBEEF);

  HTTP_DBG_PRINT("Sending DELETE request\r\n");
  status = altcom_http_send_cmd(p, &http_send_command);
  if (status == HTTP_FAILURE) {
    HTTP_DBG_PRINT("Error in altcom_http_send_cmd() configuration. Profile=%d\r\n", p);
    ret = HTTP_FAILURE;
    goto end;
  }

  HTTP_DBG_PRINT("Waiting for DELCONF URC\r\n");

  ret = wait_resp(p, false);
  HTTP_DBG_PRINT("wait_resp ret: %lu\r\n", (uint32_t)ret);

end:
  http_free(http_send_command.headers);
  altcom_http_event_register(p, HTTP_CMD_DELCONF_EV, NULL, NULL);
  altcom_http_event_register(p, HTTP_CMD_SESTERM_EV, NULL, NULL);
  return ret;
}

static Http_err_code_e load_configuration(Http_profile_id_e p,
                                          enum demo_http_secure_mode_t secure_mode) {
  int len;
  char *uri;
  Http_config_node_t http_node_config;

  /* NODE configuration */
  memset(&http_node_config, 0, sizeof(Http_config_node_t));
  uri = (secure_mode != DEMO_HTTP_NOSECURE) ? prof_configuration.uris : prof_configuration.uri;
  len = strlen(uri);
  http_node_config.dest_addr = malloc(len + 1);
  if (http_node_config.dest_addr == NULL) {
    HTTP_DBG_PRINT("No room for destination address. Profile=%d\r\n", p);
    return HTTP_FAILURE;
  }

  snprintf(http_node_config.dest_addr, len + 1, uri);
  http_node_config.user = default_user;
  http_node_config.passwd = default_password;
  http_node_config.encode_format = HTTP_ENC_B64;
  if (altcom_http_node_config(p, &http_node_config) == HTTP_FAILURE) {
    HTTP_DBG_PRINT("Error in Node configuration. Profile=%d\r\n", p);
    return HTTP_FAILURE;
  }

  http_free(http_node_config.dest_addr);

  Http_config_ip_t http_ip_config;

  /* IP configuration */
  memset(&http_ip_config, 0, sizeof(Http_config_ip_t));
  http_ip_config.sessionId = 0;
  http_ip_config.ip_type = (Http_config_ip_e)HTTP_IPTYPE_V4;
  if (altcom_http_ip_config(p, &http_ip_config) == HTTP_FAILURE) {
    HTTP_DBG_PRINT("Error in IP configuration. Profile=%d\r\n", p);
    return HTTP_FAILURE;
  }

  Http_config_format_t http_format_config;

  /* FORMAT configuration */
  memset(&http_format_config, 0, sizeof(Http_config_format_t));
  http_format_config.reqHeaderPresent = (Http_req_header_present_e)HTTP_REQ_HEADER_PRESENCE_DISABLE;
  http_format_config.respHeaderPresent =
      (Http_resp_header_present_e)HTTP_RESP_HEADER_PRESENCE_ENABLE;
  if (altcom_http_format_config(p, &http_format_config) == HTTP_FAILURE) {
    HTTP_DBG_PRINT("Error in FORMAT configuration. Profile=%d\r\n", p);
    return HTTP_FAILURE;
  }

  Http_config_tls_t http_tls_config;

  /* TLS configuration */
  memset(&http_tls_config, 0, sizeof(Http_config_tls_t));
  if (secure_mode != DEMO_HTTP_NOSECURE) {
    http_tls_config.CipherList = NULL;
    http_tls_config.CipherListFilteringType = HTTP_CIPHER_NONE_LIST;
    http_tls_config.authentication_type = (Http_config_tls_e)secure_mode;
    http_tls_config.profile_tls = (uint8_t)secure_mode + 1;
    http_tls_config.session_resumption = HTTP_TLS_RESUMP_SESSION_DISABLE;
    if (altcom_http_tls_config(p, &http_tls_config) == HTTP_FAILURE) {
      HTTP_DBG_PRINT("Error in TLS configuration. Profile=%d\r\n", p);
      return HTTP_FAILURE;
    }
  }

  /* TIMEOUT configuration = default */

  return HTTP_SUCCESS;
}

static Http_err_code_e do_secure_impl(enum demo_http_secure_mode_t secure_mode) {
  TrustedCaPath_e capath;
  char *root, *crt, *key;
  uint8_t profile_id;

  switch (secure_mode) {
    case DEMO_HTTPS_MUTUAL:
      root = root_cert;
      capath = CAPATH_USER;
      crt = client_cert;
      key = client_key;
      break;

    case DEMO_HTTPS_CLIENTONLY:
      root = NULL;
      capath = CAPATH_UNDEF;
      crt = client_cert;
      key = client_key;
      break;

    case DEMO_HTTPS_SERVERONLY:
      root = root_cert;
      capath = CAPATH_USER;
      crt = NULL;
      key = NULL;
      break;

    case DEMO_HTTPS_NOAUTH:
      root = NULL;
      capath = CAPATH_UNDEF;
      crt = NULL;
      key = NULL;
      break;

    default:
      return HTTP_FAILURE;
  }

  profile_id = (uint8_t)secure_mode + 1;
  if (altcom_ConfigCredProfile(PROFCFG_ADD, profile_id, root, &capath, crt, key, NULL, NULL) !=
      CERTMGMT_SUCCESS) {
    return HTTP_FAILURE;
  }

  return HTTP_SUCCESS;
}

/****************************************************************************
 * Public Function
 ****************************************************************************/

void hexdump(uint8_t *buf, uint32_t len) {
  HTTP_DBG_PRINT("Dump %lu bytes\r\n", len);

  uint32_t i;

  for (i = 0; i < len; i++) {
    if ((i & 0xf) == 0) {
      HTTP_DBG_PRINT("%04lx:", (uint32_t)i);
    }

    HTTP_DBG_PRINT(" %02x", buf[i]);
    if ((i & 0xf) == 0xf) {
      uint32_t j;

      HTTP_DBG_PRINT(" --> |");
      for (j = i - 0xf; j <= i; j++) {
        if (buf[j] >= 0x20 && buf[j] < 0x7f) {
          HTTP_DBG_PRINT("%c", buf[j]);
        } else {
          HTTP_DBG_PRINT(".");
        }
      }

      HTTP_DBG_PRINT("|\r\n");
    }
  }

  if ((i & 0xf) != 0) {
    uint32_t j;

    HTTP_DBG_PRINT(" --> |");
    for (j = i - (i & 0xf); j < i; j++) {
      if (buf[j] >= 0x20 && buf[j] < 0x7f) {
        HTTP_DBG_PRINT("%c", buf[j]);
      } else {
        HTTP_DBG_PRINT(".");
      }
    }

    HTTP_DBG_PRINT("|\r\n");
  }
}

void http_event_cb(void *event, void *userPriv) {
  Http_event_t *http_evt = (Http_event_t *)event;
  char *evt_str[HTTP_CMD_MAX_EV] = {"PUTCONF", "POSTCONF", "DELCONF",
                                    "GETRCV",  "SESTERM",  "PRSPRCV"};
  bool forward_urc = false;

  HTTP_DBG_PRINT("[%s(%lu) URC] ", evt_str[http_evt->event_type], (uint32_t)http_evt->event_type);
  HTTP_DBG_PRINT("profile: %d - ", http_evt->profileId);
  HTTP_DBG_PRINT("event_type: %d - ", http_evt->event_type);
  HTTP_DBG_PRINT("http_status: %d - ", http_evt->http_status);
  HTTP_DBG_PRINT("RespCode:%d\r\n", http_evt->err_code);
  HTTP_DBG_PRINT("UserPriv:%p\r\n", userPriv);
  if (http_evt->event_type != HTTP_CMD_SESTERM_EV) {
    HTTP_DBG_PRINT("file_size: %lu, avail_size: %lu\r\n", http_evt->file_size,
                   http_evt->avail_size);
    forward_urc = true;
  }

  if (is_demo && forward_urc) {
    Http_event_t *evt;

    evt = malloc(sizeof(Http_event_t));
    if (evt == NULL) {
      HTTP_DBG_PRINT("urc event malloc failed\r\n");
      return;
    }

    *evt = *http_evt;

    osStatus_t status;

    status = osMessageQueuePut(urc_que, (const void *)&evt, 0, osWaitForever);
    if (status != osOK) {
      HTTP_DBG_PRINT("osMessageQueuePut failed,status: %ld\r\n", (int32_t)status);
      free(evt);
    }
  }
}

void do_Http_demo(char *s) {
  int iter;
  int argc;
  char *argv[CMD_NUM_OF_PARAM_MAX] = {0};
  bool permanent = false;
  enum demo_http_secure_mode_t secure_mode = DEMO_HTTP_NOSECURE;

  if (false == param_field_alloc(argv)) {
    HTTP_DBG_PRINT("param_field_alloc fail\r\n");
    return;
  }

  /* Parsing arguments */
  argc = parse_arg(s, argv);
  if (argc >= 2) {
    if (!strncmp(argv[HTTPDEMO_CMD_PARAM_1], "-p", 2)) {
      permanent = true;
      HTTP_DBG_PRINT("Run permanently\r\n");
    }
  }

  if (argc >= 3) {
    secure_mode = (enum demo_http_secure_mode_t)strtoul(argv[HTTPDEMO_CMD_PARAM_2], NULL, 10);
    if (secure_mode >= DEMO_HTTP_MAX) {
      secure_mode = DEMO_HTTP_NOSECURE;
    }

    if (secure_mode < DEMO_HTTP_NOSECURE) {
      HTTP_DBG_PRINT("Enable TLS(%lu)\r\n", (uint32_t)secure_mode);
    }
  }

  param_field_free(argv);

  /* Configure certificate profile */
  if ((secure_mode < DEMO_HTTP_NOSECURE) && (do_secure_impl(secure_mode) == HTTP_FAILURE)) {
    HTTP_DBG_PRINT("Certificate configure failed\r\n");
    return;
  }

  /* Initialize demo parameters */
  if (init_demo() == HTTP_FAILURE) {
    HTTP_DBG_PRINT("init_demo failed\r\n");
    goto error;
  }

  uint32_t round;
  static Http_profile_id_e p;

  for (p = HTTP_PROFILE_ID1; p <= HTTP_PROFILE_ID5; p++) {
    /* Clear all profiles */
    if (altcom_http_clear_profile(p) == HTTP_FAILURE) {
      goto error;
    }
  }

  round = 0;
  p = HTTP_PROFILE_ID1;
  do {
    if (p >= HTTP_PROFILE_MAX) {
      p = HTTP_PROFILE_ID1;
    }

    round++;
    HTTP_DBG_PRINT("Round %lu, profile_id %lu\r\n", round, (uint32_t)p);

    /* Load HTTP profile */
    if (load_configuration(p, secure_mode) == HTTP_FAILURE) {
      goto error;
    }

    /* Unregister all callbacks */
    unregister_callbacks(p);

    for (iter = 0; iter < ITER_NO; iter++) {
      if (iter != 0 && permanent) {
        HTTP_DBG_PRINT("Wait to sleep\r\n");
        osDelay(HTTP_SLEEP_TIME_MS / OS_TICK_PERIOD_MS);
      }

      HTTP_DBG_PRINT("Iter: %d\r\n", iter);
      /* Post/Put command */
      if (post_put_command(p, (iter % 2 == 0 ? HTTP_CMD_POST : HTTP_CMD_PUT),
                           prof_configuration.total_size) == HTTP_FAILURE) {
        goto error;
      }

      if (permanent) {
        HTTP_DBG_PRINT("Wait to sleep\r\n");
        osDelay(HTTP_SLEEP_TIME_MS / OS_TICK_PERIOD_MS);
      }

      /* Get command */
      if (get_command(p) == HTTP_FAILURE) {
        goto error;
      }

      /* Delete command */
      if (delete_command(p) == HTTP_FAILURE) {
        goto error;
      }
    }

    /* Clear profile */
    if (altcom_http_clear_profile(p) == HTTP_FAILURE) {
      goto error;
    }

    p++;
  } while (permanent);

error:
  deinit_demo();
  HTTP_DBG_PRINT("**** End of Demo *****\r\n");
  return;
}
