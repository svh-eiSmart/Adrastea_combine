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

#include "FreeRTOS.h"
#include "task.h"

#include "reset.h"
#include "backtrace.h"
#include "assert.h"

#pragma GCC push_options
#pragma GCC optimize("O0")
void prvGetRegistersFromStack(uint32_t *pulFaultStackAddress) {
  /* These are volatile to try and prevent the compiler/linker optimising them
  away as the variables never actually get used.  If the debugger won't show the
  values of the variables, make them global my moving their declaration outside
  of this function. */
  volatile uint32_t cfsr = SCB->CFSR;
  volatile uint32_t hfsr = SCB->HFSR;
  volatile uint32_t mmfar = SCB->MMFAR;
  volatile uint32_t bfar = SCB->BFAR;

  volatile uint32_t r0;
  volatile uint32_t r1;
  volatile uint32_t r2;
  volatile uint32_t r3;
  volatile uint32_t r12;
  volatile uint32_t lr;  /* Link register. */
  volatile uint32_t pc;  /* Program counter. */
  volatile uint32_t psr; /* Program status register. */

  r0 = pulFaultStackAddress[0];
  r1 = pulFaultStackAddress[1];
  r2 = pulFaultStackAddress[2];
  r3 = pulFaultStackAddress[3];

  r12 = pulFaultStackAddress[4];
  lr = pulFaultStackAddress[5];
  pc = pulFaultStackAddress[6];
  psr = pulFaultStackAddress[7];

  printf("\r\n####@@@@\r\n");
  printf("HardFault handler was hit !!!\r\n");
  printf("SCB->CFSR   0x%08lx\r\n", cfsr);
  printf("SCB->HFSR   0x%08lx\r\n", hfsr);
  printf("SCB->MMFAR  0x%08lx\r\n", mmfar);
  printf("SCB->BFAR   0x%08lx\r\n", bfar);
  printf("\r\nRegisters:\r\n");
  printf("SP          0x%08lx\r\n", (uint32_t)&pulFaultStackAddress[8]);
  printf("R0          0x%08lx\r\n", r0);
  printf("R1          0x%08lx\r\n", r1);
  printf("R2          0x%08lx\r\n", r2);
  printf("R3          0x%08lx\r\n", r3);
  printf("R12         0x%08lx\r\n", r12);
  printf("LR          0x%08lx\r\n", lr);
  printf("PC          0x%08lx\r\n", pc);
  printf("PSR         0x%08lx\r\n", psr);
  printf("\r\nStack dump:\r\n");

  unsigned char *memPrint = (unsigned char *)&pulFaultStackAddress[8];
  int i;

#define STACK_DUMP_SIZE (2048)
  for (i = 0; i < STACK_DUMP_SIZE; i++) {
    printf("%02x", memPrint[i]);
    if ((i + 1) % 32 == 0) {
      printf("\n");
    }
  }

  printf("\r\n######@@\r\n");
  printf("Perform system reset request\r\n");
  machine_reset(MACHINE_RESET_CAUSE_EXCEPTION);
  /* When the following line is hit, the variables contain the register values. */
  for (;;) {
  }
}
#pragma GCC pop_options

void HardFault_Handler(void) {
  __asm volatile(
      " tst lr, #4                                                \n"
      " ite eq                                                    \n"
      " mrseq r0, msp                                             \n"
      " mrsne r0, psp                                             \n"
      " ldr r1, [r0, #24]                                         \n"
      " ldr r2, handler2_address_const                            \n"
      " bx r2                                                     \n"
      " handler2_address_const: .word prvGetRegistersFromStack    \n");
}

/*----------------------------------------------------------------------------
  Default Handler for Exceptions / Interrupts
 *----------------------------------------------------------------------------*/
void Default_Handler(void) {
  printf("Default Handler for Exceptions / Interrupts was hit !!!!!!!!!!!!!!!!!\r\n");
  dump_backtrace();
  machine_reset(MACHINE_RESET_CAUSE_EXCEPTION);
  for (;;) {
  }
}

__NO_RETURN void portASSERT(const char *file, int line, const char *func, const char *failedexpr) {
  __disable_irq();
  for (;;) {
  }
}

void vApplicationMallocFailedHook(void) { assert(0); }

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) { assert(0); }
