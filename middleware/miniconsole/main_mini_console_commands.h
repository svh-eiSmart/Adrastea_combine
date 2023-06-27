/****************************************************************************
 *
 *  (c) copyright 2019 Altair Semiconductor, Ltd. All rights reserved.
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

/* Declare all CLI commands in this file */

#undef MCU_PROJECT_NAME
#define MCU_PROJECT_NAME "MCU"

DECLARE_COMMAND("read", do_read, "read addr - Read address (hex format)")
DECLARE_COMMAND("write", do_write, "write addr value - Write value (hex) to address (hex)")
DECLARE_COMMAND("ps", do_ps, "ps - Print task list")
DECLARE_COMMAND("meminfo", do_mem_info, "meminfo - Reports memory information")
DECLARE_COMMAND("ver", do_ver, "ver - Show complitaion date and time")
DECLARE_COMMAND("hifc", do_hifc, "hifc [status [clr]] [timer MS] [sus] [res]")
DECLARE_COMMAND("serialinfo", do_serialinfo, "serialinfo [serial num]")
DECLARE_COMMAND("time", do_time, "time [show | set Seconds(from epoch)]]")
DECLARE_COMMAND("reset", do_reset,
                "reset [all] - Reset MCU or reset all cores if \"all\" arg is given")

/* Extra CLI commands to be declared in following file */
#include "mini_console_commands.h"
