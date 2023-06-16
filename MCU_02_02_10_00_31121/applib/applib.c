/*  ---------------------------------------------------------------------------

    (c) copyright 2021 Altair Semiconductor, Ltd. All rights reserved.

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
#include DEVICE_HEADER

#include "DRV_CLOCK_GATING.h"
#include "DRV_ATOMIC_COUNTER.h"
#include "DRV_PMP_AGENT.h"
#if (configUSE_ALT_SLEEP == 1)
#include "DRV_PM.h"
#endif

#include "cmsis_os2.h"

#include "assert.h"

void sysclk_init();

static void common_platform_initialize() {
  DRV_CLKGATING_Initialize();

  DRV_ATOM_CNTR_Initialize();

  DRV_PMP_AGENT_Initialize();

#if (configUSE_ALT_SLEEP == 1)
  DRV_PM_Initialize();
#endif
}

int main();

__NO_RETURN void _start() {
  common_platform_initialize();

  // Initialize CMSIS-RTOS
  osStatus_t status = osKernelInitialize();
  assert(status == osOK);

  osKernelState_t state = osKernelGetState();
  assert(state == osKernelReady);

  int err = main();
  assert(err == 0);

  // Begin thread switching.
  // This will not return in case of success.
  status = osKernelStart();

  assert(0);
  for (;;) {}
}
