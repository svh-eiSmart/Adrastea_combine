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
#define MCU_PROJECT_NAME "CertmgmtConnector"

DECLARE_COMMAND("apitest", do_apitest, "apitest - Command for CertmgmtConnector APIs test")
DECLARE_COMMAND("paste", do_certpaste,
                "paste - enter into certificate pasting mode to send to apitest queue")
DECLARE_COMMAND("altcomlog", do_altcomlog,
                "altcomlog - Change ALTCOM log level 0: Disable, 1~5 for each level")
DECLARE_COMMAND("altcombuff", do_altcom_buffpool_statistic,
                "altcombuff - Show ALTCOM buffpool statistics")
