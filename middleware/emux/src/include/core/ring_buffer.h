/*  ---------------------------------------------------------------------------

        (c) copyright 2017 Altair Semiconductor, Ltd. All rights reserved.

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
#ifndef _EMUX_RING_BUFFER_H_
#define _EMUX_RING_BUFFER_H_

int allocCircBufUtil(void **buffer, unsigned char *buf, unsigned int buf_size);
void ClearCirBuff(void *xmit, uint32_t in_bytes);
void *getCircBufPtr(void *xmit);
void freeCircBufUtil(void **Buffer);
int circBufIsEmpty(void *Buffer);
int circBufFree(void *Buffer);
void circBufInsert(void *xmit, char c);
int circBufIsdata(void *xmit);
void circBufGetChar(void *xmit, uint8_t *dataChar);
int circBufGetUsage(void *xmitBufPtr);
int circBufGetSize(void *BuffPtr);
void debugPrintCircBuf(void *xmitBufPtr);
#endif
