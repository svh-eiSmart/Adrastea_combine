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

int serialAllocCircBufUtil(void **xmitBuffer, unsigned int buf_size);
void serialFreeCircBufUtil(void *Buffer);
int serialCircBufIsEmpty(void *xmitBuffer);
int serialCircBufFree(void *xmitBuffer);
void serialCircBufInsert(void *xmit, char c);
int serialCircBufIsdata(void *xmit);
// char serialCircBufGetChar (void * xmit);
void serialCircBufGetChar(void *xmit, uint8_t *dataChar);
void serialCircGetCharAndCopy(void *xmit, volatile void *uartDataReg);
int serialCircBufGetUsage(void *xmitBufPtr);
int serialCircBufGetSize(void *xmitBufPtr);
int serialCircBufGetRoom(void *xmitBufPtr);
