/*  ---------------------------------------------------------------------------

        (c) copyright 2018 Altair Semiconductor, Ltd. All rights reserved.

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

#ifndef _SHADOW32KTIMER_ALT125X_H_
#define _SHADOW32KTIMER_ALT125X_H_

#include "FreeRTOS.h"
#include "queue.h"

typedef void (*shadow32ktimer_interrupt_handler)(unsigned int user_param);

// functions list
int shadow32ktimer_register_interrupt(shadow32ktimer_interrupt_handler irq_handler, int user_param);
unsigned int shadow32ktimer_get_value(void);
void shadow32ktimer_add_offset(unsigned int offset);
void shadow32ktimer_enable(void);
void shadow32ktimer_disable(void);
void shadow32ktimer_activate(unsigned int value);
int shadow32ktimer_set_interrupt_offset(unsigned int offset);
int shadow32ktimer_set_interrupt_value(unsigned int value);
int shadow32ktimer_disable_interrupt(void);
unsigned int shadow32ktimer_read_target_value(void);
unsigned int shadow32ktimer_read_current_value(void);
void shadow32ktimer_init(void);

#endif /* _SHADOW32KTIMER_ALT125X_H_ */
