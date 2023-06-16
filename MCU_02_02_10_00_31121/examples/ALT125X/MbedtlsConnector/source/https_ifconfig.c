/*  ---------------------------------------------------------------------------

    (c) copyright 2022 Altair Semiconductor, Ltd. All rights reserved.

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

#include <string.h>

#include "mbedtls/config.h"
#include "mbedtls/platform.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/certs.h"

#include "certificates.h"

#define CRLF "\r\n"

static int my_verify(void *data, mbedtls_x509_crt *crt, int depth, uint32_t *flags) {
  char buf[1024];
  ((void)data);

  printf("\nVerify requested for (Depth %d):\n", depth);
  memset(buf, 0, sizeof(buf));
  mbedtls_x509_crt_info(buf, sizeof(buf) - 1, "", crt);
  printf("%s", buf);

  if ((*flags) == 0)
    printf("  This certificate has no flags\n");
  else {
    memset(buf, 0, sizeof(buf));
    mbedtls_x509_crt_verify_info(buf, sizeof(buf) - 1, "  ! ", *flags);
    printf("%s\n", buf);
  }

  return (0);
}

static int send_http_request(mbedtls_ssl_context *ssl, const char *path) {
  char buf[128] = {0};
  int ret, len;
  int written = 0, frags = 0;

  len =
      snprintf(buf, sizeof(buf),
               "GET %s HTTP/1.1" CRLF "Host: ifconfig.co" CRLF "Connection: close" CRLF CRLF, path);

  do {
    while ((ret = mbedtls_ssl_write(ssl, (unsigned char *)buf + written, len - written)) < 0) {
      if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE &&
          ret != MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS) {
        printf(" failed\n  ! mbedtls_ssl_write returned -0x%x\n\n", -ret);
        return ret;
      }
    }

    frags++;
    written += ret;
  } while (written < len);

  printf(" %d bytes written in %d fragments\n\n%s\n", written, frags, buf);

  return written;
}

static int print_http_response(mbedtls_ssl_context *ssl) {
  char buf[64] = {0};
  int ret, len = sizeof(buf) - 1;
  int readnum = 0;

  do {
    ret = mbedtls_ssl_read(ssl, (unsigned char *)buf + readnum, len - readnum);

    if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
      continue;
    }

    if (ret <= 0) {
      switch (ret) {
        case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
          printf(" connection was closed gracefully\n");
          goto exit;

        case 0:
        case MBEDTLS_ERR_NET_CONN_RESET:
          printf(" connection was reset by peer\n");
          goto exit;

        default:
          printf(" mbedtls_ssl_read returned -0x%x\n", -ret);
          goto exit;
      }
    }

    readnum += ret;

    if (len == readnum) {
      buf[readnum] = '\0';
      printf("%s", buf);
      len = sizeof(buf) - 1;
      readnum = 0;
    }
  } while (mbedtls_ssl_get_bytes_avail(ssl) > 0);

exit:
  if (readnum > 0) {
    buf[readnum] = '\0';
    printf("%s", buf);
  }

  return ret;
}

int do_whereami(char *s) {
  int ret = 0;
  mbedtls_net_context sockfd;
  char crtinfo[256];
  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context ctr_drbg;
  mbedtls_ssl_context ssl;
  mbedtls_ssl_config conf;
  mbedtls_ssl_session saved_session;
  uint32_t flags;
  mbedtls_x509_crt cacert;

  mbedtls_net_init(&sockfd);
  mbedtls_ssl_init(&ssl);
  mbedtls_ssl_config_init(&conf);
  mbedtls_ssl_session_init(&saved_session);
  mbedtls_ctr_drbg_init(&ctr_drbg);
  mbedtls_x509_crt_init(&cacert);

  printf("\n  . Seeding the random number generator ...");

  mbedtls_entropy_init(&entropy);
  if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0)) != 0) {
    printf(" failed\n  ! mbedtls_ctr_drbg_seed returned -0x%x\n\n", -ret);
    goto exit;
  }

  printf(" ok\n");

  printf("  . Loading the CA root certificate ...");

  ret = mbedtls_x509_crt_parse_file(&cacert, GlobalSign_Root_CA_name);
  if (ret < 0) {
    ret = mbedtls_x509_crt_parse(&cacert, (unsigned char *)GlobalSign_Root_CA,
                                 strlen(GlobalSign_Root_CA) + 1 /* including the NULL byte */);
    if (ret < 0) {
      printf(" failed\n  !  mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
      goto exit;
    }
  }

  printf(" ok\n");

  printf("  . Connecting to ifconfig.co:443 ...");

  if ((ret = mbedtls_net_connect(&sockfd, "ifconfig.co", "443", MBEDTLS_NET_PROTO_TCP)) != 0) {
    printf(" failed\n  ! mbedtls_net_connect returned -0x%x\n\n", -ret);
    goto exit;
  }

  ret = mbedtls_net_set_block(&sockfd);
  if (ret != 0) {
    printf(" failed\n  ! net_set_(non)block() returned -0x%x\n\n", -ret);
    goto exit;
  }

  printf(" ok\n");

  printf("  . Setting up the SSL/TLS structure ...");
  if ((ret = mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM,
                                         MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
    printf(" failed\n  ! mbedtls_ssl_config_defaults returned -0x%x\n\n", -ret);
    goto exit;
  }

  mbedtls_ssl_conf_verify(&conf, my_verify, NULL);
  mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_REQUIRED);

#if defined(MBEDTLS_SSL_MAX_FRAGMENT_LENGTH)
  if ((ret = mbedtls_ssl_conf_max_frag_len(&conf, MBEDTLS_SSL_MAX_FRAG_LEN_4096)) != 0) {
    printf(" failed\n  ! mbedtls_ssl_conf_max_frag_len returned -0x%x (skipped)\n\n", -ret);
  }
#endif

  mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);

  mbedtls_ssl_conf_read_timeout(&conf, 0);

  // 0xC02F: TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256
  mbedtls_ssl_conf_ciphersuites(&conf, (int[]){0xC02F, 0x00});

  mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);

#if defined(MBEDTLS_SSL_FALLBACK_SCSV)
  mbedtls_ssl_conf_fallback(&conf, MBEDTLS_SSL_IS_NOT_FALLBACK);
#endif

  if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0) {
    printf(" failed\n  ! mbedtls_ssl_setup returned -0x%x\n\n", -ret);
    goto exit;
  }

  if ((ret = mbedtls_ssl_set_hostname(&ssl, "ifconfig.co")) != 0) {
    printf(" failed\n  ! mbedtls_ssl_set_hostname returned %d\n\n", ret);
    goto exit;
  }

  mbedtls_ssl_set_bio(&ssl, &sockfd.fd, mbedtls_net_send, mbedtls_net_recv,
                      mbedtls_net_recv_timeout);

  printf(" ok\n");

  printf("  . Performing the SSL/TLS handshake ...");

  while ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
    if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE &&
        ret != MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS) {
      printf(" failed\n  ! mbedtls_ssl_handshake returned -0x%x\n\n", -ret);
      if (ret == MBEDTLS_ERR_X509_CERT_VERIFY_FAILED)
        printf("    Unable to verify the server's certificate.\n");
      goto exit;
    }
    printf("    *\n");
  }

  printf(" ok\n    [ Protocol is %s ]\n    [ Ciphersuite is %s ]\n", mbedtls_ssl_get_version(&ssl),
         mbedtls_ssl_get_ciphersuite(&ssl));

  if ((ret = mbedtls_ssl_get_record_expansion(&ssl)) >= 0)
    printf("    [ Record expansion is %d ]\n", ret);
  else
    printf("    [ Record expansion is unknown (compression) ]\n");

  printf("  . Saving session for reuse ...");

  if ((ret = mbedtls_ssl_get_session(&ssl, &saved_session)) != 0) {
    printf(" failed\n  ! mbedtls_ssl_get_session returned -0x%x\n\n", -ret);
    goto exit;
  }

  printf(" ok\n");

  printf("  . Verifying peer X.509 certificate ...");

  if ((flags = mbedtls_ssl_get_verify_result(&ssl)) != 0) {
    char vrfy_buf[512];

    printf(" failed\n");

    mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "  ! ", flags);

    printf("%s\n", vrfy_buf);
  } else
    printf(" ok\n");

  if (mbedtls_ssl_get_peer_cert(&ssl) != NULL) {
    printf("  . Peer certificate information ...\n");
    mbedtls_x509_crt_info((char *)crtinfo, sizeof(crtinfo) - 1, "      ",
                          mbedtls_ssl_get_peer_cert(&ssl));
    printf("%s\n", crtinfo);
  }

  printf("  > Write the GET request:");

  send_http_request(&ssl, "/city");

  printf("  < Read the HTTP response\n");

  print_http_response(&ssl);

  printf("  . Closing the connection ...");

  /* No error checking, the connection might be closed already */
  do ret = mbedtls_ssl_close_notify(&ssl);
  while (ret == MBEDTLS_ERR_SSL_WANT_WRITE);
  ret = 0;

  printf(" done\n");

  mbedtls_net_free(&sockfd);

  printf("  . Reconnecting with saved session ...");

  if ((ret = mbedtls_ssl_session_reset(&ssl)) != 0) {
    printf(" failed\n  ! mbedtls_ssl_session_reset returned -0x%x\n\n", -ret);
    goto exit;
  }

  if ((ret = mbedtls_ssl_set_session(&ssl, &saved_session)) != 0) {
    printf(" failed\n  ! mbedtls_ssl_conf_session returned %d\n\n", ret);
    goto exit;
  }

  if ((ret = mbedtls_net_connect(&sockfd, "ifconfig.co", "443", MBEDTLS_NET_PROTO_TCP)) != 0) {
    printf(" failed\n  ! mbedtls_net_connect returned -0x%x\n\n", -ret);
    goto exit;
  }

  ret = mbedtls_net_set_block(&sockfd);
  if (ret != 0) {
    printf(" failed\n  ! net_set_(non)block() returned -0x%x\n\n", -ret);
    goto exit;
  }

  while ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
    if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE &&
        ret != MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS) {
      printf(" failed\n  ! mbedtls_ssl_handshake returned -0x%x\n\n", -ret);
      goto exit;
    }
  }

  printf(" ok\n");

  printf("  > Write the GET request:");

  send_http_request(&ssl, "/ip");

  printf("  < Read the HTTP response\n");

  print_http_response(&ssl);

  printf("  . Closing the connection ...");

  /* No error checking, the connection might be closed already */
  do {
    ret = mbedtls_ssl_close_notify(&ssl);
  } while (ret == MBEDTLS_ERR_SSL_WANT_WRITE);

  printf(" done\n");

exit:
  mbedtls_net_free(&sockfd);
  mbedtls_x509_crt_free(&cacert);
  mbedtls_ssl_session_free(&saved_session);
  mbedtls_ssl_free(&ssl);
  mbedtls_ssl_config_free(&conf);
  mbedtls_ctr_drbg_free(&ctr_drbg);
  mbedtls_entropy_free(&entropy);

  return (ret);
}
