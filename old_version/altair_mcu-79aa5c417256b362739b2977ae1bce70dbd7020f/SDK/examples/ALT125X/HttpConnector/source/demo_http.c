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

/* CMSIS-RTOS V2 includes. */
#include "cmsis_os2.h"

/* ALTCOM API includes */
#include "alt_osal.h"
#include "http/altcom_http.h"
#include "certmgmt/altcom_certmgmt.h"
#include "apicmd_http_urc.h"

/* Power management includes */
#include "pwr_mngr.h"
#include "DRV_PM.h"
#include "hifc_api.h"
#include "sleep_mngr.h"
#include "sleep_notify.h"

/* App includes */
#include "apitest_main.h"
#include "demo_http.h"

#define OS_TICK_PERIOD_MS (1000 / osKernelGetTickFreq())

#ifdef HTTP_SECURE
static char *Web01_Root_name = "web01.pem";
/* Certificate sample */
static char *web01 =
    "-----BEGIN CERTIFICATE-----\n"
    "MIID4jCCAsqgAwIBAgICZNgwDQYJKoZIhvcNAQELBQAwgaUxCzAJBgNVBAYTAi0t\n"
    "MRIwEAYDVQQIDAlTb21lU3RhdGUxETAPBgNVBAcMCFNvbWVDaXR5MRkwFwYDVQQK\n"
    "DBBTb21lT3JnYW5pemF0aW9uMR8wHQYDVQQLDBZTb21lT3JnYW5pemF0aW9uYWxV\n"
    "bml0MRQwEgYDVQQDDAsxNzIuMjUuMTQuMTEdMBsGCSqGSIb3DQEJARYOcm9vdEBq\n"
    "b2Vsci1jZTcwHhcNMjAxMjmfmwewMzUyWhcNMjExMjA5MTYwMzUyWjCBpTELMAkG\n"
    "A1UEBhMCLS0xEjAQBgNVBAgMCVNvbWVTdGF0ZTERMA8GA1UEBwwIU29tZUNpdHkx\n"
    "GTAXBgNVBAoMEFNvbWVPcmdhbml6YXRpb24xHzAdBgNVBAsMFlNvbWVPcmdhbml6\n"
    "YXRpb25hbFVuaXQxFDASBgNVBAMMCzE3Mi4yNS4xNC4xMR0wGwYJKoZIhvcNAQkB\n"
    "Fg5yb290QGpvZWxyLWNlNzCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB\n"
    "AMhXciuNuRNCzTMmOSaTGbeVGbDVgNl9dvztQ7PySzp3rPIp8V/qYimMGxeDoog7\n"
    "zJQ8DCsvSZdOqfc1xDSdSsostCz+SCmlwhdT8bxxrUs1VZ2nLN/r7H/8lGT6Ntlf\n"
    "h00exY/oOjQ+T7IxqLHKUEubatOePXH1WKaEKZUYgWn/oje+TzWPlCgkp6eZ9bsq\n"
    "bPq/hu7zPHFRq2V4eiv9835J9xdMubI9USLGXIf8RleKIDetJQTCkSOSo3z/09wu\n"
    "ToEuYsdN8Ur2iRwsTJJaveD/LgNSrmnYjuRU5H89RyU72modi1aIgRMXtJJz31PB\n"
    "atSNflV9CbNiY1fov/00tFsCAwEAAaMaMBgwCQYDVR0TBAIwADALBgNVHQ8EBAMC\n"
    "BeAwDQYJKoZIhvcNAQELBQADggEBAHIuWPOQt9S4duopcCJDDckYSh2ovIV0AXpL\n"
    "FRUBqqs7ZcoU6hJ74YhDuhyBlJbSJAkVFWlvZQJMEnK0VngenW5cNmPP0QJfjedH\n"
    "zoXKDcQAFpuEY32NSLMAMTABnuCVweee7hFNFrMeMaUbwGs0rIV6nJLHpnWhnn1L\n"
    "1T3YxFOaEGhPxGio7vXbr+8UEjjlrO+U/Cbb/2LAAjc/jr+6dVuc2QBMFq2fXLMf\n"
    "i7TMZKA5mKo1Yxm6lHkd4WiUAKiEfyr+4fPkiui1q7AtnNm/JIAUOzKgqeCIeE55\n"
    "/5GPfE+367cMqV5Cd2ya3MyUD9fVCBRkjOs8hoZuTQil2a7wW2k=\n"
    "-----END CERTIFICATE-----\n";
#endif

/****************************************************************************
 * Global Data
 ****************************************************************************/

Http_config_node_t http_node_config[HTTP_PROFILE_MAX];
Http_config_ip_t http_ip_config[HTTP_PROFILE_MAX];
Http_config_format_t http_format_config[HTTP_PROFILE_MAX];
Http_config_tls_t http_tls_config[HTTP_PROFILE_MAX];
Http_config_timeout_t http_timeout_config[HTTP_PROFILE_MAX];
Http_command_data_t http_send_command[HTTP_PROFILE_MAX];

void getData_cmd_cb(void *userPriv) {
  Http_cb_read_data_t *urc_data = (Http_cb_read_data_t *)userPriv;

  HTTP_DBG_PRINT("[GET DATA URC] ");
  HTTP_DBG_PRINT("profile: %d - ", urc_data->profile_id);
  HTTP_DBG_PRINT("chunk length: %d - ", urc_data->chunkLen);
  HTTP_DBG_PRINT("pending: %ld\n", urc_data->pending);

  for (int i = 0; i < urc_data->chunkLen; i++) {
    g_recv_checksum += urc_data->data[i];
    //   printf("data %d - %x g_recv_checksum %ld\n", i, urc_data->data[i], g_recv_checksum);
  }
  //  printf("getData_cmd_cb: checksum: %ld\n", g_recv_checksum);

  g_pending_data = urc_data->pending;
}

void put_sesterm_cb(void *userPriv) {
  HttpUrcIe_t *urc_data = (HttpUrcIe_t *)userPriv;
  HTTP_DBG_PRINT("[SESTERM] ");
  HTTP_DBG_PRINT("profile: %d - ", urc_data->profileId);
  HTTP_DBG_PRINT("httpEveNum: %d - ", urc_data->httpEveNum);
  HTTP_DBG_PRINT("http_status: %d - ", urc_data->http_status);
  HTTP_DBG_PRINT("RespCode:%d\n", urc_data->err_code);

  /* signal urc arrival */
  g_urc_response = (int)urc_data->err_code;
}

void put_cmd_cb(void *userPriv) {
  HttpUrcIe_t *urc_data = (HttpUrcIe_t *)userPriv;
  HTTP_DBG_PRINT("[PUT COMMAND] ");
  HTTP_DBG_PRINT("profile: %d - ", urc_data->profileId);
  HTTP_DBG_PRINT("httpEveNum: %d - ", urc_data->httpEveNum);
  HTTP_DBG_PRINT("http_status: %d - ", urc_data->http_status);
  if (urc_data->err_code == STATE_TRANSACT_OK)
    HTTP_DBG_PRINT("RespCode: OK\n");
  else
    HTTP_DBG_PRINT("RespCode:%d\n", urc_data->err_code);

  /* signal urc arrival */
  g_urc_response = (int)urc_data->err_code;
}

void get_cmd_cb(void *userPriv) {
  HttpUrcIe_t *urc_data = (HttpUrcIe_t *)userPriv;
  HTTP_DBG_PRINT("[GET COMMAND] ");
  HTTP_DBG_PRINT("profile: %d - ", urc_data->profileId);
  HTTP_DBG_PRINT("httpEveNum: %d - ", urc_data->httpEveNum);
  HTTP_DBG_PRINT("http_status: %d - ", urc_data->http_status);
  HTTP_DBG_PRINT("filesize: %d - ", urc_data->filesize);
  if (urc_data->err_code == STATE_TRANSACT_OK)
    HTTP_DBG_PRINT("RespCode: OK\n");
  else
    HTTP_DBG_PRINT("RespCode:%d\n", urc_data->err_code);

  /* signal urc arrival */
  g_pending_data = urc_data->filesize;
  g_urc_response = (int)urc_data->err_code;
}

void delete_cmd_cb(void *userPriv) {
  HttpUrcIe_t *urc_data = (HttpUrcIe_t *)userPriv;
  HTTP_DBG_PRINT("[DELETE COMMAND] ");
  HTTP_DBG_PRINT("profile: %d - ", urc_data->profileId);
  HTTP_DBG_PRINT("httpEveNum: %d - ", urc_data->httpEveNum);
  HTTP_DBG_PRINT("http_status: %d - ", urc_data->http_status);
  if (urc_data->err_code == STATE_TRANSACT_OK)
    HTTP_DBG_PRINT("RespCode: OK\n");
  else
    HTTP_DBG_PRINT("RespCode:%d\n", urc_data->err_code);

  /* signal urc arrival */
  g_urc_response = (int)urc_data->err_code;
}

void post_cmd_cb(void *userPriv) {
  HTTP_DBG_PRINT("[POST COMMAND] ");
  HttpUrcIe_t *urc_data = (HttpUrcIe_t *)userPriv;
  HTTP_DBG_PRINT("profile: %d - ", urc_data->profileId);
  HTTP_DBG_PRINT("httpEveNum: %d - ", urc_data->httpEveNum);
  HTTP_DBG_PRINT("http_status: %d - ", urc_data->http_status);
  if (urc_data->err_code == STATE_TRANSACT_OK)
    HTTP_DBG_PRINT("RespCode: OK\n");
  else
    HTTP_DBG_PRINT("RespCode:%d\n", urc_data->err_code);

  /* signal urc arrival */
  g_urc_response = (int)urc_data->err_code;
}

static Http_err_code_e generate_payload(Http_profile_id_e p, uint32_t *checksum) {
  unsigned char c = 0x1;
  if (prof_configuration[HTTP_PROFILE_ID1].payload == NULL) {
    prof_configuration[HTTP_PROFILE_ID1].payload = malloc(prof_configuration[p].total_size);
    if (prof_configuration[HTTP_PROFILE_ID1].payload == NULL) {
      HTTP_DBG_PRINT("No room for payload. Profile=%d\n", p);
      return HTTP_FAILURE;
    }

    *checksum = 0;
    for (int i = 0; i < (int)prof_configuration[HTTP_PROFILE_ID1].total_size; i++) {
      prof_configuration[HTTP_PROFILE_ID1].payload[i] = c;
      *checksum += c;
      c++;
    }
  }
  return HTTP_SUCCESS;
}

static Http_err_code_e init_demo(Http_profile_id_e p, uint32_t *checksum) {
  Http_err_code_e res = generate_payload(p, checksum);
  g_recv_checksum = 0;
  return res;
}

static Http_err_code_e post_put_command(Http_profile_id_e p, Http_cmd_method_e cmd, int size) {
  int len, timeout, off, data_left;
  Http_err_code_e ret = HTTP_SUCCESS, status;
  int chunklen = prof_configuration[p].chunklen;

  /* Headers */
  http_send_command[p].headers = NULL;
  if (strlen(prof_configuration[p].headers) > 0) {
    len = strlen(prof_configuration[p].headers);
    http_send_command[p].headers = malloc(len + 1);
    if (http_send_command[p].headers == NULL) {
      HTTP_DBG_PRINT("No room for headers. Profile=%d", p);
      return HTTP_FAILURE;
    }
    snprintf(http_send_command[p].headers, len + 1, prof_configuration[p].headers);
  }

  /* Payload allocation */
  if (chunklen > 0) {
    http_send_command[p].data_to_send = malloc(chunklen);
    if (http_send_command[p].data_to_send == NULL) {
      HTTP_DBG_PRINT("No room for chunk. Profile=%d", p);
      ret = HTTP_FAILURE;
      goto end;
    }
  } else {
    HTTP_DBG_PRINT("No Chunk length. Profile=%d", p);
    ret = HTTP_FAILURE;
    goto end;
  }

  if (size - chunklen < 0) {
    http_send_command[p].pending_data = 0;
  }
  http_send_command[p].pending_data = (size - chunklen) < 0 ? 0 : (size - chunklen);

  /* Static parameters */
  http_send_command[p].cmd = cmd;
  http_send_command[p].profile_id = p;

  http_EventRegister((void *)put_sesterm_cb, p, HTTP_CMD_SESTERM_EV);

  if (cmd == HTTP_CMD_PUT)
    http_EventRegister((void *)put_cmd_cb, p, HTTP_CMD_PUTCONF_EV);
  else
    http_EventRegister((void *)post_cmd_cb, p, HTTP_CMD_POSTCONF_EV);

  off = 0;
  data_left = size;
  g_urc_response = (int)-1;
  do {
    http_send_command[p].data_len = data_left > chunklen ? chunklen : data_left;
    memcpy(http_send_command[p].data_to_send, &prof_configuration[HTTP_PROFILE_ID1].payload[off],
           http_send_command[p].data_len);
    off += http_send_command[p].data_len;

    status = altcom_http_send_cmd(p, &http_send_command[p]);
    if (status == HTTP_FAILURE) {
      HTTP_DBG_PRINT("Error in altcom_http_send_cmd() configuration. Profile=%d", p);
      ret = HTTP_FAILURE;
      goto end;
    }
    data_left -= http_send_command[p].data_len;

    if (http_send_command[p].pending_data > 0) {
      if (http_send_command[p].pending_data >= (uint32_t)chunklen) {
        http_send_command[p].pending_data -= (uint32_t)chunklen;
      } else {
        http_send_command[p].pending_data = 0;
      }
    }
  } while (data_left > 0);

  timeout = 10;
  //  HTTP_DBG_PRINT("Waiting for URC\n");
  while (g_urc_response == (int)-1 && --timeout > 0) vTaskDelay(1000 / portTICK_PERIOD_MS);
  // HTTP_DBG_PRINT("URC arrived\n");

  /* Check for status */
  if (g_urc_response != STATE_TRANSACT_OK || timeout == 0) {
    HTTP_DBG_PRINT("Wrong URC result. rv=%d  timeout: %d\n\n", g_urc_response, timeout);
    ret = HTTP_FAILURE;
    goto end;
  }

end:
  http_free(http_send_command[p].headers);
  http_free(http_send_command[p].data_to_send);
  if (http_send_command[p].cmd == HTTP_CMD_PUT) http_EventRegister(NULL, p, HTTP_CMD_PUTCONF_EV);
  if (http_send_command[p].cmd == HTTP_CMD_POST) http_EventRegister(NULL, p, HTTP_CMD_PUTCONF_EV);
  http_EventRegister(NULL, p, HTTP_CMD_SESTERM_EV);
  return ret;
}

static Http_err_code_e readData_command(Http_profile_id_e p, uint32_t ChunkLen,
                                        uint32_t calc_check) {
  Http_err_code_e ret = HTTP_FAILURE;
  int timeout = 20;
  uint32_t last_pending = g_pending_data;

  http_EventRegister((void *)getData_cmd_cb, p, HTTP_CMD_READCONF_EV);

  while (g_pending_data > 0) {
    timeout = 20;
    if (g_pending_data < ChunkLen) ChunkLen = g_pending_data;
    ret = altcom_http_readData_cmd(HTTP_PROFILE_ID1, ChunkLen);
    if (ret == HTTP_FAILURE) {
      HTTP_DBG_PRINT("Failure with readData\n");
      return ret;
    }
    while (last_pending == g_pending_data && --timeout > 0) {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    last_pending = g_pending_data;
  }

  if (g_recv_checksum != calc_check) {
    HTTP_DBG_PRINT("Error in checksum\n");
    return HTTP_FAILURE;
  }

  return HTTP_SUCCESS;
}

static Http_err_code_e get_command(Http_profile_id_e p) {
  int len, timeout;
  Http_err_code_e ret = HTTP_SUCCESS, status;

  /* Headers */
  http_send_command[p].headers = NULL;
  if (strlen(prof_configuration[p].headers) > 0) {
    len = strlen(prof_configuration[p].headers);
    http_send_command[p].headers = malloc(len + 1);
    if (http_send_command[p].headers == NULL) {
      HTTP_DBG_PRINT("No room for headers. Profile=%d", p);
      return HTTP_FAILURE;
    }
    snprintf(http_send_command[p].headers, len + 1, prof_configuration[p].headers);
  }

  /* Static parameters */
  http_send_command[p].cmd = HTTP_CMD_GET;
  http_send_command[p].profile_id = p;
  http_send_command[p].pending_data = 0;

  http_EventRegister((void *)get_cmd_cb, p, HTTP_CMD_GETRCV_EV);
  http_EventRegister((void *)getData_cmd_cb, p, HTTP_CMD_READCONF_EV);
  http_EventRegister((void *)put_sesterm_cb, p, HTTP_CMD_SESTERM_EV);

  g_urc_response = (int)-1;
  status = altcom_http_send_cmd(p, &http_send_command[p]);
  if (status == HTTP_FAILURE) {
    HTTP_DBG_PRINT("Error in altcom_http_send_cmd() configuration. Profile=%d", p);
    ret = HTTP_FAILURE;
    goto end;
  }
  timeout = 20;
  while (g_urc_response == (int)-1 && --timeout > 0) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

end:
  g_recv_checksum = 0;
  http_free(http_send_command[p].headers);
  http_EventRegister(NULL, p, HTTP_CMD_GETRCV_EV);
  http_EventRegister(NULL, p, HTTP_CMD_READCONF_EV);
  http_EventRegister(NULL, p, HTTP_CMD_SESTERM_EV);
  return ret;
}

static Http_err_code_e delete_command(Http_profile_id_e p) {
  int len, timeout;
  Http_err_code_e ret = HTTP_SUCCESS, status;

  /* Headers */
  http_send_command[p].headers = NULL;
  if (strlen(prof_configuration[p].headers) > 0) {
    len = strlen(prof_configuration[p].headers);
    http_send_command[p].headers = malloc(len + 1);
    if (http_send_command[p].headers == NULL) {
      HTTP_DBG_PRINT("No room for headers. Profile=%d", p);
      return HTTP_FAILURE;
    }
    snprintf(http_send_command[p].headers, len + 1, prof_configuration[p].headers);
  }

  /* Static parameters */
  http_send_command[p].cmd = HTTP_CMD_DELETE;
  http_send_command[p].profile_id = p;
  http_send_command[p].pending_data = 0;

  http_EventRegister((void *)delete_cmd_cb, p, HTTP_CMD_DELCONF_EV);
  http_EventRegister((void *)put_sesterm_cb, p, HTTP_CMD_SESTERM_EV);

  g_urc_response = (int)-1;
  status = altcom_http_send_cmd(p, &http_send_command[p]);
  if (status == HTTP_FAILURE) {
    HTTP_DBG_PRINT("Error in altcom_http_send_cmd() configuration. Profile=%d", p);
    ret = HTTP_FAILURE;
    goto end;
  }
  timeout = 20;
  while (g_urc_response == (int)-1 && --timeout > 0) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  if (g_urc_response || timeout == 0) {
    HTTP_DBG_PRINT("Error in DELETE command: rv=%d  timeout: %d\n\n", g_urc_response, timeout);
    ret = HTTP_FAILURE;
    goto end;
  }

end:
  http_free(http_send_command[p].headers);
  http_EventRegister(NULL, p, HTTP_CMD_DELCONF_EV);
  http_EventRegister(NULL, p, HTTP_CMD_SESTERM_EV);
  return ret;
}

static Http_err_code_e load_configuration(Http_profile_id_e p) {
  int len;

  /* NODE configuration */

  len = strlen(prof_configuration[p].uri);
  http_node_config[p].dest_addr = malloc(len + 1);
  if (http_node_config[p].dest_addr == NULL) {
    HTTP_DBG_PRINT("No room for destination address. Profile=%d", p);
    return HTTP_FAILURE;
  }
  snprintf(http_node_config[p].dest_addr, len + 1, prof_configuration[p].uri);

  http_node_config[p].user = NULL;
  http_node_config[p].passwd = NULL;
  if (altcom_http_node_config(p, &http_node_config[p]) == HTTP_FAILURE) {
    HTTP_DBG_PRINT("Error in Node configuration. Profile=%d", p);
    return HTTP_FAILURE;
  }

  http_free(http_node_config[p].dest_addr);

  /* IP configuration */
  http_ip_config->sessionId = 0;
  http_ip_config[p].ip_type = (Http_config_ip_e)HTTP_IPTYPE_V4;
  if (altcom_http_ip_config(p, &http_ip_config[p]) == HTTP_FAILURE) {
    HTTP_DBG_PRINT("Error in IP configuration. Profile=%d", p);
    return HTTP_FAILURE;
  }

  /* FORMAT configuration */
  http_format_config[p].reqHeaderPresent = (reqHeaderPresent_e)REQ_HEADER_PRESENCE_ENABLE;
  http_format_config[p].respHeaderPresent = (respHeaderPresent_e)RESP_HEADER_PRESENCE_DISABLE;
  if (altcom_http_format_config(p, &http_format_config[p]) == HTTP_FAILURE) {
    HTTP_DBG_PRINT("Error in FORMAT configuration. Profile=%d", p);
    return HTTP_FAILURE;
  }

  /* TLS configuration */
#ifdef HTTP_SECURE_MODE
  http_tls_config[p].CipherList = NULL;
  http_tls_config[p].CipherListFilteringType = HTTP_CIPHER_NONE_LIST;
  http_tls_config[p].authentication_type = HTTP_SERVER_AUTH;
  http_tls_config[p].profile_tls = prof_configuration[p].tls_profile;
  http_tls_config[p].session_resumption = HTTP_TLS_RESUMP_SESSION_DISABLE;
  if (altcom_http_tls_config(p, &http_tls_config[p]) == HTTP_FAILURE) {
    HTTP_DBG_PRINT("Error in TLS configuration. Profile=%d", p);
    return HTTP_FAILURE;
  }
#endif

  /* TIMEOUT configuration = default */

  return HTTP_SUCCESS;
}

#ifdef HTTP_SECURE_MODE
static Http_err_code_e do_secure_impl(void) {
  if (altcom_WriteCredential(CREDTYPE_CERT, Web01_Root_name, (uint8_t *)web01, strlen(web01)) !=
      CERTMGMT_SUCCESS) {
    HTTP_DBG_PRINT("error writing certificate\n");
    return HTTP_FAILURE;
  }

  TrustedCaPath_e capath = CAPATH_USER;
  if (altcom_ConfigCredProfile(PROFCFG_ADD, HTTP_PROFILE_ID1, Web01_Root_name, &capath, NULL, NULL,
                               NULL, NULL) != CERTMGMT_SUCCESS) {
    return HTTP_FAILURE;
  }
  return HTTP_SUCCESS;
}
#endif

/*
 * flag '-p' indicates to run the demo with power management support
 */

#define ITER_NO 1

void do_Http_demo(char *s) {
  Http_profile_id_e p;
  uint32_t calc_checksum = 0;
  int iter;

  if (init_demo(HTTP_PROFILE_ID1, &calc_checksum) == HTTP_FAILURE) {
    HTTP_DBG_PRINT("Failure with init_demo. Profile=%d\n", HTTP_PROFILE_ID1);
    goto error;
  }
  HTTP_DBG_PRINT("Packet checksum: %ld\n", calc_checksum);

#ifdef HTTP_SECURE_MODE
  if (do_secure_impl() == HTTP_FAILURE) {
    HTTP_DBG_PRINT("Certificate implementation: FAIL\n");
    goto error;
  }
#endif

  for (p = HTTP_PROFILE_ID1; p < HTTP_PROFILE_ID4; p++) {
    /* Clear all profiles */
    if (altcom_http_clear_profile(p) == HTTP_FAILURE) {
      goto error;
    }
  }

  /* Load profile */
  if (load_configuration(HTTP_PROFILE_ID1) == HTTP_FAILURE) {
    goto error;
  }

  for (iter = 0; iter < ITER_NO; iter++) {
    HTTP_DBG_PRINT("Iter: %d\n", iter);
    /* Put command */
    if (post_put_command(HTTP_PROFILE_ID1, HTTP_CMD_PUT,
                         prof_configuration[HTTP_PROFILE_ID1].total_size) == HTTP_FAILURE) {
      goto error;
    }

    g_pending_data = 0;

    /* Get command */
    if (get_command(HTTP_PROFILE_ID1) == HTTP_FAILURE) {
      goto error;
    }

    if (g_pending_data == 0) {
      HTTP_DBG_PRINT("Empty file size\n");
      goto error;
    }

    if (readData_command(HTTP_PROFILE_ID1, prof_configuration[HTTP_PROFILE_ID1].chunklen,
                         calc_checksum) == HTTP_FAILURE) {
      HTTP_DBG_PRINT("Failure with readData\n");
      goto error;
    }

    /* Delete command */
    if (delete_command(HTTP_PROFILE_ID1) == HTTP_FAILURE) {
      goto error;
    }
  }

error:
  http_free(prof_configuration[HTTP_PROFILE_ID1].payload);
  // HTTP_DBG_PRINT("**** End of Demo *****\r\n");
  return;
}
