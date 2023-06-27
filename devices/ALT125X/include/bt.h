/*  ---------------------------------------------------------------------------

        (c) copyright 2019 Altair Semiconductor, Ltd. All rights reserved.

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
#ifndef _BT_H_
#define _BT_H_

#if defined(__GNUC__)

#include "125X_mcu.h"
#include <stdio.h>
#include <string.h>

#define BACKTRACE_SIZE 25

void dump_backtrace();

#endif /*__GNUC__*/

#if defined(__ICCARM__)

#define dump_backtrace()

#endif /*__ICCARM__*/

#endif /*_BT_H_*/
