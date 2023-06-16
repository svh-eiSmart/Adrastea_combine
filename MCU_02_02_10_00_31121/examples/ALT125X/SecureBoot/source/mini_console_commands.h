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
#define MCU_PROJECT_NAME "Secure Boot"

DECLARE_COMMAND("secureboot", do_secureboot, "secureboot <puk3_offset>- enable MCU secure boot. <puk3_offset> MCU PUK3 address offset on flash.")
DECLARE_COMMAND("otp", do_otp, "otp [w | r] <field_id> <size_in_bytes>- write(w) test data to or read(r) from the specified OTP field. <field_id> OTP field ID. <size_in_bytes> read/write data size.")