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

#ifndef DRV_SB_ENABLE_H_
#define DRV_SB_ENABLE_H_

typedef enum {
  OTP_SB_ENABLE_OK = 0,
  OTP_SB_ENABLE_CHECKSUM_ERROR = -1,
  OTP_SB_ENABLE_OTP_ERROR = -3,
  OTP_SB_ENABLE_RC_PATCH_ERROR = -4,
  OTP_SB_ENABLE_OTP_HPUK_ERROR = -6,
  OTP_SB_READ_OTP_ERROR = -7,
  OTP_SB_WRITE_OTP_ERROR = -8,
  OTP_SB_ENABLE_CMD_NOT_APPLICABLE = -9
} otp_sb_enable_status_e;

/*****************************************************************************
 *  Function: DRV_SB_Enable_MCU
 *  Parameters:	uint32_t offset - PUK3 flash Partition Offset
 *  Returns:    otp_sb_enable_status_e
 *  Description: enable mcu secure boot
 *****************************************************************************/
otp_sb_enable_status_e DRV_SB_Enable_MCU(uint32_t offset);

#endif /* DRV_SB_ENABLE_H_ */
