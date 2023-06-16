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
#ifndef APITEST_MAIN_H
#define APITEST_MAIN_H

#include <stdint.h>
#include <stdbool.h>
#include "http/altcom_http.h"
#include "cmsis_os2.h"

/* HTTP data structures (demo)*/

extern Http_config_node_t http_node_config[HTTP_PROFILE_MAX];
extern Http_config_ip_t http_ip_config[HTTP_PROFILE_MAX];
extern Http_config_format_t http_format_config[HTTP_PROFILE_MAX];
extern Http_config_tls_t http_tls_config[HTTP_PROFILE_MAX];
extern Http_config_timeout_t http_timeout_config[HTTP_PROFILE_MAX];
extern Http_command_data_t http_send_command[HTTP_PROFILE_MAX];

/* HTTP URC callbacks (demo)*/

extern void getData_cmd_cb(void *userPriv);
extern void put_sesterm_cb(void *userPriv);
extern void put_cmd_cb(void *userPriv);
extern void get_cmd_cb(void *userPriv);
extern void delete_cmd_cb(void *userPriv);
extern void post_cmd_cb(void *userPriv);

extern uint8_t *credBuf;
extern uint16_t credDataLen;

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

void apitest_log_lock(void);
void apitest_log_unlock(void);
int32_t apitest_init(void);
int32_t apitest_main(int32_t argc, char *argv[]);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* APITEST_MAIN_H */
