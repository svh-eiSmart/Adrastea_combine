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

#ifndef TIMEX_ALT125X_H_
#define TIMEX_ALT125X_H_
#include <time.h>
#include "FreeRTOS.h"

/**
 * @brief Update the uptime(mandatory) & last wakeup time(option)
 *
 * @param [in] seconds: The past seconds to be updated.
 * @return None.
 */
void update_system_time(int seconds);

/**
 * @brief Get time in seconds
 *
 * @param [out] t: If t is non-NULL, the return value is also stored in the memory pointed to by
 * tloc.
 * @return The time as the number of seconds since the Epoch, 1970-01-01 00:00:00 +0000 (UTC).
 */
time_t time(time_t *t);

/**
 * @brief Sets the system's idea of the time and date.
 *
 * @param [out] t: The time measured in seconds since the Epoch, 1970-01-01 00:00:00 +0000 (UTC).
 * @return On success, zero is returned. On error, -1 is returned.
 */
int stime(const time_t *t);

/**
 * @brief Get uptime in seconds
 *
 * @param [out] t: The address pointer to store the uptime.
 * @return None.
 */
void uptime(time_t *t);

#endif /* TIMEX_ALT125X_H_ */
