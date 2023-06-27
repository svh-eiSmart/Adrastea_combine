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

#ifndef CORE_CORE_UTILS_SERIALMNGR_MUXUTIL_H_
#define CORE_CORE_UTILS_SERIALMNGR_MUXUTIL_H_

#include "los_mux_types.h"

#if defined(__cplusplus)
extern "C" {
#endif /* _cplusplus */
#define ESC_SEQ_PART_ONE 0xA3, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA
#define ESC_SEQ_PART_TWO 0xF8, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F
#define ESC_SEQ_DELAY (100)
#define ESC_SEQ_DELAY_LOWER_THRESH (75)
#define ESC_SEQ_DELAY_UPPER_THRESH (125)
int bindToMuxVirtualPort(int muxID, int virtualPortID, muxRxFp_t serialRxProcessFp, void *appCookie,
                         muxTxBuffFp_t *serialTxcharFp);
int unbindFromMuxVirtualPort(int muxID, int virtualSerID);
int createMux(int logicalPortID, int muxID, int numberOfVirtualPorts);

#if defined(__cplusplus)
}
#endif

#endif /* CORE_CORE_UTILS_SERIALMNGR_MUXUTIL_H_ */
