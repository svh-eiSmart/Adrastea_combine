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

/* Declare extra CLI commands in this file */

#undef MCU_PROJECT_NAME
#define MCU_PROJECT_NAME "SocketConnector"

DECLARE_COMMAND("nctest", do_nctest, "nctest netcat <IP> <PORT>- Command for netcat test")
DECLARE_COMMAND("iperf", do_nctest,
                "iperf <tcp|udp> <client ip port|server(6) port> - Command for iperf test")
DECLARE_COMMAND("ncmsg", do_ncmsg, "ncmsg - send netcat data to nctest netcat")
DECLARE_COMMAND("apitest", do_nctest, "apitest - Command for SocketConnector APIs test")
DECLARE_COMMAND("altcomlog", do_altcomlog,
                "altcomlog - Change ALTCOM log level 0: Disable, 1~5 for each level")
DECLARE_COMMAND("altcombuff", do_altcom_buffpool_statistic,
                "altcombuff - Show ALTCOM buffpool statistics")