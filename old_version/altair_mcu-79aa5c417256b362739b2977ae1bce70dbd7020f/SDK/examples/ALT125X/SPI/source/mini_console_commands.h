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
#define MCU_PROJECT_NAME "SPI"

DECLARE_COMMAND(
    "spim_roundtrip", do_spim_transfer_test,
    "spim_roundtrip - A round trip test. Send 64 or specified bytes test data to the SPI slave, "
    "wait and expect to receive the same data from the SPI slave. Usage:[<SIZE>]")
DECLARE_COMMAND("spim_send", do_spim_send_test,
                "spim_send - send 64 or specified bytes data to SPI slave. Usage:[<SIZE>]")
DECLARE_COMMAND("spim_receive", do_spim_receive_test,
                "spim_receive - receive 64 or specified bytes data from SPI slave. Usage:[<SIZE>]")
DECLARE_COMMAND("spis_roundtrip", do_spis_transfer_test,
                "spis_roundtrip - A round trip test. Wait and receive 64 or specified bytes test "
                "data from the SPI master and then send the same data back. Usage:[<SIZE>]")
DECLARE_COMMAND("spis_send", do_spis_send_test,
                "spis_send - send 64 or specified bytes data to SPI master. Usage:[<SIZE>]")
DECLARE_COMMAND("spis_receive", do_spis_receive_test,
                "spis_receive - receive 64 or specified bytes data from SPI master. Usage:[<SIZE>]")
