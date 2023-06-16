/****************************************************************************
 * modules/lte/altcom/include/evtdisp/buffpoolwrapper.h
 *
 *   Copyright 2018 Sony Semiconductor Solutions Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Sony Semiconductor Solutions Corporation nor
 *    the names of its contributors may be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef __MODULES_LTE_ALTCOM_INCLUDE_EVTDISP_BUFFPOOLWRAPPER_H
#define __MODULES_LTE_ALTCOM_INCLUDE_EVTDISP_BUFFPOOLWRAPPER_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>

#include "altcom.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define BUFFPOOL_ALLOC(reqsize) (buffpoolwrapper_alloc(reqsize))
#define BUFFPOOL_FREE(buff) (buffpoolwrapper_free(buff))
#define BUFFPOOL_SHOW_STATISTICS() (buffpoolwrapper_show())

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: buffpoolwrapper_config_memif
 *
 * Description:
 *   Configure memory management interface
 *
 * Input Parameters:
 *   mem_if     The input memory interface. NULL means ALTCOM default interface
 *
 * Returned Value:
 *   If the process succeeds, it returns 0.
 *   Otherwise errno is returned.
 *
 ****************************************************************************/

int32_t buffpoolwrapper_config_memif(mem_if_t *mem_if);

/****************************************************************************
 * Name: buffpoolwrapper_init
 *
 * Description:
 *   Initialize the bufferpool interface.
 *
 * Input Parameters:
 *   set     List of size and number for creating the buffer.
 *   setnum  Number of @set.
 *
 * Returned Value:
 *   If the process succeeds, it returns 0.
 *   Otherwise errno is returned.
 *
 ****************************************************************************/

int32_t buffpoolwrapper_init(blockset_t set[], uint8_t setnum);

/****************************************************************************
 * Name: buffpoolwrapper_fin
 *
 * Description:
 *   Finalize the bufferpool interface.
 *
 * Input Parameters:
 *   None.
 *
 * Returned Value:
 *   If the process succeeds, it returns 0.
 *   Otherwise errno is returned.
 *
 ****************************************************************************/

int32_t buffpoolwrapper_fin(void);

/****************************************************************************
 * Name: buffpoolwrapper_alloc
 *
 * Description:
 *   Allocate buffer from bufferpool interface.
 *   This function is blocking.
 *
 * Input Parameters:
 *   handle  Handle of bufferpool interface.
 *   size    Buffer size.
 *
 * Returned Value:
 *   Buffer address.
 *   If can't get available buffer, returned NULL.
 *
 ****************************************************************************/

void *buffpoolwrapper_alloc(uint32_t size);

/****************************************************************************
 * Name: buffpoolwrapper_free
 *
 * Description:
 *   Free the allocated buffer from bufferpool interface.
 *
 * Input Parameters:
 *   handle  Handle of bufferpool interface.
 *   buff    Buffer address.
 *
 * Returned Value:
 *   If the process succeeds, it returns 0.
 *   And when request buffer is NULL, returns 0.
 *   Otherwise errno is returned.
 *
 ****************************************************************************/

int32_t buffpoolwrapper_free(void *buf);

/****************************************************************************
 * Name: buffpoolwrapper_show
 *
 * Description:
 *   Show the statistics of buffpool interface.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 ****************************************************************************/

void buffpoolwrapper_show(void);

#endif
