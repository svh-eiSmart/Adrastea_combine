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

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "cmsis_os2.h"

#include "altcom.h"
#include "altcom_atsocket.h"
#include "altcom_certmgmt.h"
#include "buffpoolwrapper.h"
#include "pwr_mngr.h"

#define CMD_NUM_OF_PARAM_MAX 12
#define CMD_PARAM_LENGTH 128

#define MAX_DATA_SIZE 1500
#define CRLF "\r\n"

#define LOCK() atsocket_log_lock()
#define UNLOCK() atsocket_log_unlock()

#define APITEST_DBG_PRINT(...) \
  do {                         \
    LOCK();                    \
    printf(__VA_ARGS__);       \
    UNLOCK();                  \
  } while (0)

static char *GlobalSign_Root_CA_name = "GlobalSign_Root_CA.crt";
static char *GlobalSign_Root_CA =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDdTCCAl2gAwIBAgILBAAAAAABFUtaw5QwDQYJKoZIhvcNAQEFBQAwVzELMAkG\n"
    "A1UEBhMCQkUxGTAXBgNVBAoTEEdsb2JhbFNpZ24gbnYtc2ExEDAOBgNVBAsTB1Jv\n"
    "b3QgQ0ExGzAZBgNVBAMTEkdsb2JhbFNpZ24gUm9vdCBDQTAeFw05ODA5MDExMjAw\n"
    "MDBaFw0yODAxMjgxMjAwMDBaMFcxCzAJBgNVBAYTAkJFMRkwFwYDVQQKExBHbG9i\n"
    "YWxTaWduIG52LXNhMRAwDgYDVQQLEwdSb290IENBMRswGQYDVQQDExJHbG9iYWxT\n"
    "aWduIFJvb3QgQ0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDaDuaZ\n"
    "jc6j40+Kfvvxi4Mla+pIH/EqsLmVEQS98GPR4mdmzxzdzxtIK+6NiY6arymAZavp\n"
    "xy0Sy6scTHAHoT0KMM0VjU/43dSMUBUc71DuxC73/OlS8pF94G3VNTCOXkNz8kHp\n"
    "1Wrjsok6Vjk4bwY8iGlbKk3Fp1S4bInMm/k8yuX9ifUSPJJ4ltbcdG6TRGHRjcdG\n"
    "snUOhugZitVtbNV4FpWi6cgKOOvyJBNPc1STE4U6G7weNLWLBYy5d4ux2x8gkasJ\n"
    "U26Qzns3dLlwR5EiUWMWea6xrkEmCMgZK9FGqkjWZCrXgzT/LCrBbBlDSgeF59N8\n"
    "9iFo7+ryUp9/k5DPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNVHRMBAf8E\n"
    "BTADAQH/MB0GA1UdDgQWBBRge2YaRQ2XyolQL30EzTSo//z9SzANBgkqhkiG9w0B\n"
    "AQUFAAOCAQEA1nPnfE920I2/7LqivjTFKDK1fPxsnCwrvQmeU79rXqoRSLblCKOz\n"
    "yj1hTdNGCbM+w6DjY1Ub8rrvrTnhQ7k4o+YviiY776BQVvnGCv04zcQLcFGUl5gE\n"
    "38NflNUVyRRBnMRddWQVDf9VMOyGj/8N7yy5Y0b2qvzfvGn9LhJIZJrglfCm7ymP\n"
    "AbEVtQwdpf5pLGkkeB6zpxxxYu7KyJesF12KwvhHhm4qxFYxldBniYUr+WymXUad\n"
    "DKqC5JlR3XC321Y9YeRq4VzW9v493kHMB65jUr9TU/Qr6cf9tveCX4XSQRjbgbME\n"
    "HMUfpIBvFSDJ3gyICh3WZlXi/EjJKSZp4A==\n"
    "-----END CERTIFICATE-----\n";

static char *californium_cert_name = "californium_cert.pem";
static char *californium_cert =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIB/jCCAaSgAwIBAgIJAJ3+bXCmfffGMAoGCCqGSM49BAMCMFoxDjAMBgNVBAMT\n"
    "BWNmLWNhMRQwEgYDVQQLEwtDYWxpZm9ybml1bTEUMBIGA1UEChMLRWNsaXBzZSBJ\n"
    "b1QxDzANBgNVBAcTBk90dGF3YTELMAkGA1UEBhMCQ0EwHhcNMjIxMDI2MDcwMzMx\n"
    "WhcNMjMxMDI2MDcwMzMxWjBeMRIwEAYDVQQDEwljZi1jbGllbnQxFDASBgNVBAsT\n"
    "C0NhbGlmb3JuaXVtMRQwEgYDVQQKEwtFY2xpcHNlIElvVDEPMA0GA1UEBxMGT3R0\n"
    "YXdhMQswCQYDVQQGEwJDQTBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABNcwrXwo\n"
    "b76WQSIQjt398EYq6hIQrY6PtWP9I0g48vlLirtzE4v1c0rqPy/M2v4INNttLMtb\n"
    "QvanSuyornAUvoyjTzBNMB0GA1UdDgQWBBTz/KiBShT/0ALlH+PnE1WEEHAVjTAL\n"
    "BgNVHQ8EBAMCB4AwHwYDVR0jBBgwFoAUWTm+jUaqsoI0wrlDeHiybI33f+cwCgYI\n"
    "KoZIzj0EAwIDSAAwRQIgfIRjDPUDTlQD/QhhKZvabsvb6vSM26aOaJczP2MCsq0C\n"
    "IQD3UDAcYWtVuxENn2YIqBUWTIOkNMXZfMPIH4Khc/0xSA==\n"
    "-----END CERTIFICATE-----\n";

static char *californium_key_name = "californium_key.pem";
static char *californium_key =
    "-----BEGIN PRIVATE KEY-----\n"
    "MEECAQAwEwYHKoZIzj0CAQYIKoZIzj0DAQcEJzAlAgEBBCDJXqiqopBm7fDmo5Ak\n"
    "Rwv5SYDgmB2US2VvwKQQhXkISA==\n"
    "-----END PRIVATE KEY-----\n";

static char *californium_root_name = "californium_root.pem";
static char *californium_root =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIB3jCCAYSgAwIBAgIJALd+Edoht/I5MAoGCCqGSM49BAMCMFwxEDAOBgNVBAMT\n"
    "B2NmLXJvb3QxFDASBgNVBAsTC0NhbGlmb3JuaXVtMRQwEgYDVQQKEwtFY2xpcHNl\n"
    "IElvVDEPMA0GA1UEBxMGT3R0YXdhMQswCQYDVQQGEwJDQTAeFw0yMjEwMjYwNzAz\n"
    "MjRaFw0yMzEwMjYwNzAzMjRaMFwxEDAOBgNVBAMTB2NmLXJvb3QxFDASBgNVBAsT\n"
    "C0NhbGlmb3JuaXVtMRQwEgYDVQQKEwtFY2xpcHNlIElvVDEPMA0GA1UEBxMGT3R0\n"
    "YXdhMQswCQYDVQQGEwJDQTBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABH6oGChw\n"
    "1sNLAP+0FsSWWjt4Gq8gQGCfYOxfUg/R78PyQlsd7wN2fw1JzRHhKk5/E7w0PeiU\n"
    "mi7qeeA3rsZnTwKjLzAtMB0GA1UdDgQWBBRYzQRIqIulcix6HZligx+3SXSDlzAM\n"
    "BgNVHRMEBTADAQH/MAoGCCqGSM49BAMCA0gAMEUCIGpzyol3O/vrXaVD1xbWgMWy\n"
    "2qxknwDE0PIPlhFTJ+EgAiEA1ZsyEMBp6DqWYQVxr2h40P3PGKN7mNYdHQUq9OJx\n"
    "ow0=\n"
    "-----END CERTIFICATE-----\n";

#define MAX_SOCKNUM (4)
#define EVTBITS (5)

typedef enum {
  SOCKET_STATE_UNKNOWN = 0,
  SOCKET_STATE_ACTIVATED = 1,
  SOCKET_STATE_DEACTIVATED = 2,
} socket_state_t;

typedef enum {
  CONNTYPE_UNKNOWN = 0,
  CONNTYPE_OPEN = 1,
  CONNTYPE_LISTEN = 2,
} conntype_t;

typedef struct {
  socket_state_t state;
  SocketId_t sockid;
  conntype_t conntype;
  uint8_t pdn_session;
  SocketType_e socket_type;
  char hostname[256];
  uint16_t dstport;
  uint16_t srcport;
  AddrtypePrecedence_e precedence;
  bool secured;
} sockentry_t;

typedef int (*command_handler_t)(char *s);

static sockentry_t socktable[MAX_SOCKNUM];

static osEventFlagsId_t socket_ef_id = NULL;
osMutexId_t app_log_mtx = NULL;

void atsocket_log_lock(void) {
  if (app_log_mtx) {
    osMutexAcquire(app_log_mtx, osWaitForever);
  }
}

void atsocket_log_unlock(void) {
  if (app_log_mtx) {
    osMutexRelease(app_log_mtx);
  }
}

static bool param_field_alloc(char *argv[]) {
  int i;

  for (i = 0; i < CMD_NUM_OF_PARAM_MAX; i++) {
    argv[i] = malloc(CMD_PARAM_LENGTH);
    if (!argv[i]) {
      APITEST_DBG_PRINT("cmd field alloc failed.\n");
      return false;
    }
    memset(argv[i], 0, CMD_PARAM_LENGTH);
  }
  return true;
}

static void param_field_free(char *argv[]) {
  int i;

  for (i = 0; i < CMD_NUM_OF_PARAM_MAX; i++) {
    if (argv[i]) {
      free(argv[i]);
    }
  }
  return;
}

static int parse_arg(char *s, char *argv[]) {
  int argc = 1; /* At least, include "apitest" or other */
  char *tmp_s;
  int i;

  /* Parse arg */
  if (strlen(s)) {
    for (i = 1; i < CMD_NUM_OF_PARAM_MAX; i++) {
      tmp_s = strchr(s, ' ');
      if (!tmp_s) {
        tmp_s = strchr(s, '\0');
        if (!tmp_s) {
          APITEST_DBG_PRINT("Invalid arg.\n");
          break;
        }
        memcpy(argv[i], s, tmp_s - s);
        break;
      }
      memcpy(argv[i], s, tmp_s - s);
      s += tmp_s - s + 1;
    }
    argc += i;
  }

  APITEST_DBG_PRINT("Number of arg = %d\n", argc);
  for (i = 0; i < argc; i++) {
    if (argv[i]) {
      APITEST_DBG_PRINT("arg[%d] = %s\n", i, argv[i]);
    }
  }
  return argc;
}

void init_context() {
  for (int i = 0; i < MAX_SOCKNUM; i++) {
    sockentry_t *e = socktable + i;
    memset(e, 0, sizeof(*e));
    e->sockid = INVALID_SOCKET_ID;
  }
}

static sockentry_t *find_sockentry(SocketId_t sockid) {
  for (int i = 0; i < MAX_SOCKNUM; i++) {
    sockentry_t *e = socktable + i;
    if (e->sockid == sockid) {
      return e;
    }
  }

  return NULL;
}

static SocketId_t open_altcom_atsocket(uint8_t pdn_session, SocketType_e socket_type,
                                       const char *hostname, uint16_t dstport, uint16_t srcport,
                                       AddrtypePrecedence_e precedence) {
  int pos = -1;
  for (int i = 0; i < MAX_SOCKNUM; i++) {
    sockentry_t *e = socktable + i;
    if (e->sockid != INVALID_SOCKET_ID && e->conntype == CONNTYPE_OPEN) {
      if (e->pdn_session == pdn_session && e->socket_type == socket_type &&
          strcmp(e->hostname, hostname) == 0 && e->dstport == dstport && e->srcport == srcport &&
          e->precedence == precedence) {
        return e->sockid;
      }
    } else {
      if (pos < 0) {
        pos = i;
      }
    }
  }

  SocketId_t sockid = INVALID_SOCKET_ID;
  if (pos < 0) {
    return sockid;
  }

  sockid = altcom_atsocket_open(pdn_session, socket_type, hostname, dstport, srcport, precedence);
  if (sockid != INVALID_SOCKET_ID) {
    socktable[pos].sockid = sockid;
    socktable[pos].conntype = CONNTYPE_OPEN;
    socktable[pos].pdn_session = pdn_session;
    socktable[pos].socket_type = socket_type;
    strncpy(socktable[pos].hostname, hostname, 256);
    socktable[pos].hostname[255] = '\0';
    socktable[pos].dstport = dstport;
    socktable[pos].srcport = srcport;
    socktable[pos].precedence = precedence;
    socktable[pos].state = SOCKET_STATE_DEACTIVATED;
  }

  return sockid;
}

static SocketId_t listen_altcom_atsocket(uint8_t pdn_session, SocketType_e socket_type,
                                         uint16_t srcport) {
  int pos = -1;
  for (int i = 0; i < MAX_SOCKNUM; i++) {
    sockentry_t *e = socktable + i;
    if (e->sockid != INVALID_SOCKET_ID && e->conntype == CONNTYPE_LISTEN) {
      if (e->pdn_session == pdn_session && e->socket_type == socket_type && e->srcport == srcport) {
        return e->sockid;
      }
    } else {
      if (pos < 0) {
        pos = i;
      }
    }
  }

  SocketId_t sockid = INVALID_SOCKET_ID;
  if (pos < 0) {
    return sockid;
  }

  sockid = altcom_atsocket_listen(pdn_session, socket_type, srcport);
  if (sockid != INVALID_SOCKET_ID) {
    socktable[pos].sockid = sockid;
    socktable[pos].conntype = CONNTYPE_LISTEN;
    socktable[pos].pdn_session = pdn_session;
    socktable[pos].socket_type = socket_type;
    socktable[pos].srcport = srcport;
    socktable[pos].state = SOCKET_STATE_DEACTIVATED;
  }

  return sockid;
}

static SocketError_e activate_altcom_atsocket(SocketId_t sockid) {
  SocketError_e err = SOCKET_ERR;
  sockentry_t *e = find_sockentry(sockid);
  if (e) {
    // It's fine to activate an ACTIVATED socket
    err = altcom_atsocket_activate(sockid);
    if (err == SOCKET_OK) {
      e->state = SOCKET_STATE_ACTIVATED;
    }
  }

  return err;
}

static SocketError_e deactivate_altcom_atsocket(SocketId_t sockid) {
  SocketError_e err = SOCKET_ERR;
  sockentry_t *e = find_sockentry(sockid);

  if (e) {
    // It's fine to deactivate an DEACTIVATED socket
    err = altcom_atsocket_deactivate(sockid);
    if (err == SOCKET_OK) {
      e->state = SOCKET_STATE_DEACTIVATED;
    }
  }

  return err;
}

static SocketError_e delete_altcom_atsocket(SocketId_t sockid) {
  sockentry_t *e = find_sockentry(sockid);

  if (e) {
    memset(e, 0, sizeof(*e));
    e->sockid = INVALID_SOCKET_ID;
  }

  return altcom_atsocket_delete(sockid);
}

static SocketError_e secure_altcom_atsocket(SocketId_t sockid, uint8_t profileid,
                                            TlsAuthmode_e authmode,
                                            CiphersuitesFiltering_e filtering,
                                            uint16_t *ciphersuites, size_t ciphersuites_len) {
  SocketError_e err = SOCKET_ERR;
  sockentry_t *e = find_sockentry(sockid);

  if (!e) {
    return SOCKET_ERR;
  }

  if (e->state == SOCKET_STATE_DEACTIVATED) {
    err = altcom_atsocket_secure(sockid, profileid, authmode, filtering, ciphersuites,
                                 ciphersuites_len);
    if (err == SOCKET_OK) {
      e->secured = true;
    }
  }

  return e->secured ? SOCKET_OK : SOCKET_ERR;
}

static void atsocket_callback(SocketId_t sockid, SocketEvent_e event, void *user) {
  int base = EVTBITS * (sockid - 1);
  osEventFlagsSet(user, (1 << (base + event)));
}

static void setup(command_handler_t handler) {
  socket_ef_id = osEventFlagsNew(NULL);
  altcom_atsocket_set_callback(atsocket_callback, socket_ef_id);
}

static void teardown() {
  for (int i = 0; i < MAX_SOCKNUM; i++) {
    delete_altcom_atsocket(i + 1);
  }

  altcom_atsocket_set_callback(NULL, NULL);
  osEventFlagsDelete(socket_ef_id);
}

static size_t readall(osEventFlagsId_t ef_id, SocketId_t sockid, char *buf, size_t len) {
  size_t readnum = 0;

  int base = EVTBITS * (sockid - 1);

  while (readnum < len) {
    uint32_t eventflags = 0xFFFFFF;  // Event flags are limited to 24 bits
    uint32_t flags = osEventFlagsWait(ef_id, eventflags, osFlagsWaitAny, osWaitForever);

    if (flags & (1 << (base + SOCKET_EVENT_READABLE))) {
      size_t remain = 0;
      do {
        ssize_t received = altcom_atsocket_receive(sockid, buf + readnum, len - readnum, &remain);
        if (received > 0) {
          readnum += received;
        } else {
          break;
        }
      } while (remain > 0 && len - readnum > 0);

      if (remain > 0) {
        osEventFlagsSet(ef_id, (1 << (base + SOCKET_EVENT_READABLE)));
      }
    }

    if (flags & (1 << (base + SOCKET_EVENT_CLOSED))) {
      deactivate_altcom_atsocket(sockid);
      break;
    }
  }

  return readnum;
}

static void echod(osEventFlagsId_t ef_id) {
  char buf[MAX_DATA_SIZE];
  bool stopped = false;

  for (uint32_t eventflags = 0xFFFFFF;  // Event flags are limited to 24 bits
       !stopped;) {
    pwr_mngr_conf_set_mode(PWR_MNGR_MODE_STANDBY, 0);
    pwr_mngr_enable_sleep(1);

    uint32_t flags = osEventFlagsWait(ef_id, eventflags, osFlagsWaitAny, osWaitForever);

    pwr_mngr_enable_sleep(0);

    for (int i = 0; i < MAX_SOCKNUM; i++) {
      int base = EVTBITS * i;
      SocketId_t sockid = i + 1;

      if (flags & (1 << (base + SOCKET_EVENT_ACCEPTED))) {
        APITEST_DBG_PRINT("SOCKET_EVENT_ACCEPTED %d\n", sockid);
      }

      if (flags & (1 << (base + SOCKET_EVENT_READABLE))) {
        APITEST_DBG_PRINT("SOCKET_EVENT_READABLE %d\n", sockid);
        size_t remain = 0;
        do {
          ip_addr_t srcaddr;
          uint16_t srcport;
          ssize_t received =
              altcom_atsocket_receivefrom(sockid, buf, sizeof(buf), &remain, &srcaddr, &srcport);
          if (received > 0) {
            APITEST_DBG_PRINT("%d-byte data received\n", received);
            if (buf[0] == 'q') {
              stopped = true;
            }
            APITEST_DBG_PRINT("sending %d-byte data\n", received);
            altcom_atsocket_sendto(sockid, buf, received, &srcaddr, srcport);
          } else {
            break;
          }
        } while (remain > 0);

        if (remain > 0) {
          osEventFlagsSet(ef_id, (1 << (base + SOCKET_EVENT_READABLE)));
        }
      }

      if (flags & (1 << (base + SOCKET_EVENT_CLOSED))) {
        APITEST_DBG_PRINT("SOCKET_EVENT_CLOSED %d\n", sockid);
        delete_altcom_atsocket(sockid);
      }
    }
  }
}

static int whatismyip_impl(char *s) {
  // Re-activate a TLS socket will resume the previous session
  uint16_t dstport = 443;

  SocketId_t sockid =
      open_altcom_atsocket(1, SOCKET_TYPE_TCP, "ifconfig.co", dstport, 0, ADDRESS_PRECEDENCE_IPV4);
  if (sockid == INVALID_SOCKET_ID) {
    APITEST_DBG_PRINT("error opening socket\n");
    return -1;
  }

  if (activate_altcom_atsocket(sockid) != SOCKET_OK) {
    APITEST_DBG_PRINT("error activating socket\n");
    return -1;
  }

  const char *request =
      "GET /ip HTTP/1.1" CRLF "Host: ifconfig.co" CRLF "Connection: close" CRLF CRLF;

  ssize_t written = altcom_atsocket_send(sockid, request, strlen(request));
  if (written < (ssize_t)strlen(request)) {
    APITEST_DBG_PRINT("error sending data\n");
    return -1;
  }

  char buf[1024];
  size_t total = readall(socket_ef_id, sockid, buf, sizeof(buf));

  buf[total] = '\0';
  APITEST_DBG_PRINT("%s", buf);

  if (deactivate_altcom_atsocket(sockid) != SOCKET_OK) {
    APITEST_DBG_PRINT("error deactivating socket\n");
    return -1;
  }

  delete_altcom_atsocket(sockid);

  return 0;
}

static int whereami_impl(char *s) {
  if (altcom_WriteCredential(CREDTYPE_CERT, GlobalSign_Root_CA_name, (uint8_t *)GlobalSign_Root_CA,
                             strlen(GlobalSign_Root_CA)) != CERTMGMT_SUCCESS) {
    APITEST_DBG_PRINT("error writing certificate\n");
    return -1;
  }

  TrustedCaPath_e capath = CAPATH_USER;
  if (altcom_ConfigCredProfile(PROFCFG_ADD, 3, GlobalSign_Root_CA_name, &capath, NULL, NULL, NULL,
                               NULL) != CERTMGMT_SUCCESS) {
    APITEST_DBG_PRINT("error configuring profile\n");
    return -1;
  }

  uint16_t dstport = 443;

  SocketId_t sockid =
      open_altcom_atsocket(1, SOCKET_TYPE_TCP, "ifconfig.co", dstport, 0, ADDRESS_PRECEDENCE_IPV4);
  if (sockid == INVALID_SOCKET_ID) {
    APITEST_DBG_PRINT("error opening socket\n");
    return -1;
  }

  if (secure_altcom_atsocket(sockid, 3, TLS_VERIFY_REQUIRED, CIPHERSUITES_FILTERING_NONE, NULL,
                             0) != SOCKET_OK) {
    APITEST_DBG_PRINT("error securing socket\n");
    return -1;
  }

  {
    if (activate_altcom_atsocket(sockid) != SOCKET_OK) {
      APITEST_DBG_PRINT("error activating socket\n");
      return -1;
    }

    const char *request =
        "GET /city HTTP/1.1" CRLF "Host: ifconfig.co" CRLF "Connection: close" CRLF CRLF;

    ssize_t written = altcom_atsocket_send(sockid, request, strlen(request));
    if (written < (ssize_t)strlen(request)) {
      APITEST_DBG_PRINT("error sending data\n");
      return -1;
    }

    char buf[1024];
    size_t total = readall(socket_ef_id, sockid, buf, sizeof(buf));

    buf[total] = '\0';
    APITEST_DBG_PRINT("%s", buf);

    if (deactivate_altcom_atsocket(sockid) != SOCKET_OK) {
      APITEST_DBG_PRINT("error deactivating socket\n");
      return -1;
    }
  }

  return 0;
}

int do_whereami(char *s) {
  APITEST_DBG_PRINT("%s\n", __func__);
  setup(do_whereami);
  int r = whereami_impl(s);
  if (r == 0) {
    pwr_mngr_conf_set_mode(PWR_MNGR_MODE_STANDBY, 0);
    pwr_mngr_enable_sleep(1);

    osDelay(pdMS_TO_TICKS(30 * 1000));

    pwr_mngr_enable_sleep(0);

    r = whatismyip_impl(s);
  }

  teardown();
  return r;
}

int californium_impl(char *s) {
  // one-time config
  if (altcom_WriteCredential(CREDTYPE_CERT, californium_root_name, (uint8_t *)californium_root,
                             strlen(californium_root)) != CERTMGMT_SUCCESS) {
    return -1;
  }

  if (altcom_WriteCredential(CREDTYPE_CERT, californium_cert_name, (uint8_t *)californium_cert,
                             strlen(californium_cert)) != CERTMGMT_SUCCESS) {
    return -1;
  }

  if (altcom_WriteCredential(CREDTYPE_PRIVKEY, californium_key_name, (uint8_t *)californium_key,
                             strlen(californium_key)) != CERTMGMT_SUCCESS) {
    return -1;
  }

  TrustedCaPath_e capath = CAPATH_USER;
  if (altcom_ConfigCredProfile(PROFCFG_ADD, 9, californium_root_name, &capath,
                               californium_cert_name, californium_key_name, NULL,
                               NULL) != CERTMGMT_SUCCESS) {
    return -1;
  }

  SocketId_t sockid = open_altcom_atsocket(1, SOCKET_TYPE_UDP, "californium.eclipseprojects.io",
                                           5684, 0, ADDRESS_PRECEDENCE_IPV4);

  // Use TLS_VERIFY_NONE since SNI will fail the verification
  // Use CIPHERSUITES_FILTERING_WHITELIST to specify ciphersuites
  if (secure_altcom_atsocket(sockid, 9, TLS_VERIFY_NONE, CIPHERSUITES_FILTERING_WHITELIST,
                             (uint16_t[]){0xc023 /* ECDHE-ECDSA-AES128-SHA256 */},
                             1) != SOCKET_OK) {
    return -1;
  }

  if (activate_altcom_atsocket(sockid) != SOCKET_OK) {
    return -1;
  }

  char request[] = {0x46, 0x01, 0x94, 0xda, 0x2d, 0xa5, 0x4d, 0x8b, 0xc1, 0xc8,
                    0x3d, 0x0a, 0x63, 0x61, 0x6c, 0x69, 0x66, 0x6f, 0x72, 0x6e,
                    0x69, 0x75, 0x6d, 0x2e, 0x65, 0x63, 0x6c, 0x69, 0x70, 0x73,
                    0x65, 0x2e, 0x6f, 0x72, 0x67, 0x84, 0x74, 0x65, 0x73, 0x74};

  ssize_t written = altcom_atsocket_send(sockid, request, sizeof(request));

  if (written != sizeof(request)) {
    return -1;
  }

  char buf[72];
  size_t total = readall(socket_ef_id, sockid, buf, sizeof(buf));

  for (size_t i = 0; i < total; i++) {
    putchar(buf[i]);
  }

  deactivate_altcom_atsocket(sockid);
  delete_altcom_atsocket(sockid);

  return 0;
}

int do_californium(char *s) {
  APITEST_DBG_PRINT("%s\n", __func__);
  setup(do_californium);
  int r = californium_impl(s);
  teardown();
  return r;
}

int dtls_echod_impl(char *s) {
  char *pskid = "Client_identity";
  if (altcom_WriteCredential(CREDTYPE_PSKID, "test_identity.bin", (uint8_t *)pskid,
                             strlen(pskid)) != CERTMGMT_SUCCESS) {
    return -1;
  }

  char *pskkey = "secretPSK";
  if (altcom_WriteCredential(CREDTYPE_PSK, "test_key.psk", (uint8_t *)pskkey, strlen(pskkey)) !=
      CERTMGMT_SUCCESS) {
    return -1;
  }

  TrustedCaPath_e capath = CAPATH_USER;
  if (altcom_ConfigCredProfile(PROFCFG_ADD, 1, NULL, &capath, NULL, NULL, "test_identity.bin",
                               "test_key.psk") != CERTMGMT_SUCCESS) {
    return -1;
  }

  SocketId_t sockid = listen_altcom_atsocket(1, SOCKET_TYPE_UDP, 9107);
  if (sockid == INVALID_SOCKET_ID) {
    return -1;
  }

  if (secure_altcom_atsocket(sockid, 1, TLS_VERIFY_REQUIRED, CIPHERSUITES_FILTERING_NONE, NULL,
                             0) != SOCKET_OK) {
    return -1;
  }

  if (activate_altcom_atsocket(sockid) != SOCKET_OK) {
    return -1;
  }

  echod(socket_ef_id);

  deactivate_altcom_atsocket(sockid);

  delete_altcom_atsocket(sockid);

  return 0;
}

int do_dtls_echod(char *s) {
  APITEST_DBG_PRINT("%s\n", __func__);
  setup(do_dtls_echod);
  int r = dtls_echod_impl(s);
  teardown();
  return r;
}

int udp_echod_impl(char *s) {
  SocketId_t sockid = listen_altcom_atsocket(1, SOCKET_TYPE_UDP, 8007);
  if (sockid == INVALID_SOCKET_ID) {
    APITEST_DBG_PRINT("error listening socket\n");
    return -1;
  }

  if (activate_altcom_atsocket(sockid) != SOCKET_OK) {
    APITEST_DBG_PRINT("error activating socket\n");
    return -1;
  }

  echod(socket_ef_id);

  deactivate_altcom_atsocket(sockid);

  delete_altcom_atsocket(sockid);

  return 0;
}

int do_udp_echod(char *s) {
  APITEST_DBG_PRINT("%s\n", __func__);
  setup(do_udp_echod);
  int r = udp_echod_impl(s);
  teardown();
  return r;
}

int do_altcom_buffpool_statistic(char *s) {
  BUFFPOOL_SHOW_STATISTICS();
  return 0;
}

int do_altcomlog(char *s) {
  int argc;
  char *argv[CMD_NUM_OF_PARAM_MAX] = {0};

  if (false == param_field_alloc(argv)) {
    goto exit;
  }

  argc = parse_arg(s, argv);
  if (argc == 2) {
    dbglevel_e level = (dbglevel_e)strtol(argv[argc - 1], NULL, 10);

    if (level > ALTCOM_DBG_DEBUG) {
      APITEST_DBG_PRINT("Invalid log level %lu!\n", (uint32_t)level);
    } else {
      altcom_set_log_level(level);
      APITEST_DBG_PRINT("Set ALTCOM log level to %lu!\n", (uint32_t)level);
    }
  } else {
    APITEST_DBG_PRINT("Invalid argument!\n");
  }

exit:
  param_field_free(argv);

  return 0;
}
