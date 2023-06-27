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

#ifndef __ALTCOM_INCLUDE_GW_HAL_UART_NXP_H
#define __ALTCOM_INCLUDE_GW_HAL_UART_NXP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <errno.h>
#include "hal_if.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: hal_uart_nxp_create
 *
 * Description:
 *   Create an object of HAL uart_nxp and get the instance.
 *
 * Input Parameters:
 *   None.
 *
 * Returned Value:
 *   struct hal_if_s pointer(i.e. instance of HAL internal UART).
 *   If can't create instance, returned NULL.
 *
 ****************************************************************************/

struct hal_if_s *hal_uart_nxp_create(void);

/****************************************************************************
 * Name: hal_uart_nxp_delete
 *
 * Description:
 *   Delete instance of HAL uart_nxp.
 *
 * Input Parameters:
 *   None.
 *
 * Returned Value:
 *   If the process succeeds, it returns 0.
 *   Otherwise errno is returned.
 *
 ****************************************************************************/

int32_t hal_uart_nxp_delete(struct hal_if_s *thiz);

#endif /* __ALTCOM_INCLUDE_GW_HAL_UART_NXP_H */
