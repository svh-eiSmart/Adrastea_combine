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

#ifndef _GPTIMER_ALT125X_H_
#define _GPTIMER_ALT125X_H_

#include "FreeRTOS.h"
#include "queue.h"

typedef void (*gptimer_interrupt_handler)(unsigned int user_param);

// functions list
int gptimer_register_interrupt(int gptimer, gptimer_interrupt_handler irq_handler, int user_param);
unsigned int gptimer_get_value(void);
void gptimer_add_offset(unsigned int offset);
void gptimer_enable(void);
void gptimer_disable(void);
void gptimer_activate(unsigned int value);
int gptimer_set_interrupt_offset(int gptimer, unsigned int offset);
int gptimer_set_interrupt_value(int gptimer, unsigned int value);
int gptimer_disable_interrupt(int gptimer);
unsigned int gptimer_read_target_value(int gptimer);
unsigned int gptimer_read_current_value(int gptimer);
int gptimer_set_frequency(int frequency);
void gptimer_init(void);

#endif /* _GPTIMER_ALT125X_H_ */
