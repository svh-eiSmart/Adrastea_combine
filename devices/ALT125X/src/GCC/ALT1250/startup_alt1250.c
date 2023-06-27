/******************************************************************************
 * @file     startup_ARMCM4.s
 * @brief    CMSIS Core Device Startup File for
 *           ARMCM4 Device Series
 * @version  V5.00
 * @date     10. January 2018
 ******************************************************************************/
/*
 * Copyright (c) 2009-2018 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>
#include <stdio.h>
#include "bt.h"
#include "125X_mcu.h"

/*----------------------------------------------------------------------------
  Linker generated Symbols
 *----------------------------------------------------------------------------*/
extern uint32_t __etext;
extern uint32_t __data_start__;
extern uint32_t __data_end__;
extern uint32_t __copy_table_start__;
extern uint32_t __copy_table_end__;
extern uint32_t __zero_table_start__;
extern uint32_t __zero_table_end__;
extern uint32_t __bss_start__;
extern uint32_t __bss_end__;
extern uint32_t __StackTop;

/*----------------------------------------------------------------------------
  Exception / Interrupt Handler Function Prototype
 *----------------------------------------------------------------------------*/
typedef void (*pFunc)(void);

/*----------------------------------------------------------------------------
  External References
 *----------------------------------------------------------------------------*/
#ifndef __START
extern void _start(void) __attribute__((noreturn)); /* PreeMain (C library entry point) */
#else
extern int __START(void) __attribute__((noreturn)); /* main entry point */
#endif

#ifndef __NO_SYSTEM_INIT
extern void SystemInit(void); /* CMSIS System Initialization      */
#endif

/*----------------------------------------------------------------------------
  Internal References
 *----------------------------------------------------------------------------*/
void Default_Handler(void); /* Default empty handler */
void Reset_Handler(void);   /* Reset Handler */

/*----------------------------------------------------------------------------
  User Initial Stack & Heap
 *----------------------------------------------------------------------------*/
#ifndef __STACK_SIZE
#define __STACK_SIZE 0x00000400
#endif
static uint8_t stack[__STACK_SIZE] __attribute__((aligned(8), used, section(".stack")));

#ifndef __HEAP_SIZE
#define __HEAP_SIZE 0x00000C00
#endif
#if __HEAP_SIZE > 0
static uint8_t heap[__HEAP_SIZE] __attribute__((aligned(8), used, section(".heap")));
#endif

// clang-format off
/*----------------------------------------------------------------------------
  Exception / Interrupt Handler
 *----------------------------------------------------------------------------*/
/* Cortex-M4 Processor Exceptions */
void NMI_Handler                 (void) __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler           (void) __attribute__((naked, aligned));

void MemManage_Handler           (void) __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler            (void) __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler          (void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler                 (void) __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler            (void) __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler              (void) __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler             (void) __attribute__((weak, alias("Default_Handler")));

/* ARMCM4 Specific Interrupts */
void WDT_IRQHandler              (void) __attribute__((weak, alias("Default_Handler")));
void RTC_IRQHandler              (void) __attribute__((weak, alias("Default_Handler")));
void TIM0_IRQHandler             (void) __attribute__((weak, alias("Default_Handler")));
void TIM2_IRQHandler             (void) __attribute__((weak, alias("Default_Handler")));
void MCIA_IRQHandler             (void) __attribute__((weak, alias("Default_Handler")));
void MCIB_IRQHandler             (void) __attribute__((weak, alias("Default_Handler")));
void UART0_IRQHandler            (void) __attribute__((weak, alias("Default_Handler")));
void UART1_IRQHandler            (void) __attribute__((weak, alias("Default_Handler")));
void UART2_IRQHandler            (void) __attribute__((weak, alias("Default_Handler")));
void UART4_IRQHandler            (void) __attribute__((weak, alias("Default_Handler")));
void AACI_IRQHandler             (void) __attribute__((weak, alias("Default_Handler")));
void CLCD_IRQHandler             (void) __attribute__((weak, alias("Default_Handler")));
void ENET_IRQHandler             (void) __attribute__((weak, alias("Default_Handler")));
void USBDC_IRQHandler            (void) __attribute__((weak, alias("Default_Handler")));
void USBHC_IRQHandler            (void) __attribute__((weak, alias("Default_Handler")));
void CHLCD_IRQHandler            (void) __attribute__((weak, alias("Default_Handler")));
void FLEXRAY_IRQHandler          (void) __attribute__((weak, alias("Default_Handler")));
void CAN_IRQHandler              (void) __attribute__((weak, alias("Default_Handler")));
void LIN_IRQHandler              (void) __attribute__((weak, alias("Default_Handler")));
void i2c0_interrupt_handler      (void) __attribute__((weak, alias("Default_Handler")));
void i2c1_interrupt_handler      (void) __attribute__((weak, alias("Default_Handler")));
void CPU_CLCD_IRQHandler         (void) __attribute__((weak, alias("Default_Handler")));
void UART3_IRQHandler            (void) __attribute__((weak, alias("Default_Handler")));
void spis0_interrupt_handler     (void) __attribute__((weak, alias("Default_Handler")));
void spim0_interrupt_handler     (void) __attribute__((weak, alias("Default_Handler")));
void spim1_interrupt_handler     (void) __attribute__((weak, alias("Default_Handler")));
void auxdac0_handler             (void) __attribute__((weak, alias("Default_Handler")));
void auxdac1_handler             (void) __attribute__((weak, alias("Default_Handler")));
void auxdac2_handler             (void) __attribute__((weak, alias("Default_Handler")));
void auxdac3_handler             (void) __attribute__((weak, alias("Default_Handler")));
void auxdac4_handler             (void) __attribute__((weak, alias("Default_Handler")));
void auxdac5_handler             (void) __attribute__((weak, alias("Default_Handler")));
void auxdac6_handler             (void) __attribute__((weak, alias("Default_Handler")));
void auxdac7_handler             (void) __attribute__((weak, alias("Default_Handler")));
void gdma_channel_0_handler      (void) __attribute__((weak, alias("Default_Handler")));
void gdma_channel_1_handler      (void) __attribute__((weak, alias("Default_Handler")));
void gdma_channel_2_handler      (void) __attribute__((weak, alias("Default_Handler")));
void gdma_channel_3_handler      (void) __attribute__((weak, alias("Default_Handler")));
void gdma_channel_4_handler      (void) __attribute__((weak, alias("Default_Handler")));
void gdma_channel_5_handler      (void) __attribute__((weak, alias("Default_Handler")));
void gdma_channel_6_handler      (void) __attribute__((weak, alias("Default_Handler")));
void gdma_channel_7_handler      (void) __attribute__((weak, alias("Default_Handler")));
void hw_uart_interrupt_handler0  (void) __attribute__((weak, alias("Default_Handler")));
void hw_uart_interrupt_handler1  (void) __attribute__((weak, alias("Default_Handler")));
void hw_uart_interrupt_handleri0 (void) __attribute__((weak, alias("Default_Handler")));
void UARTF_ERR_IRQHandler        (void) __attribute__((weak, alias("Default_Handler")));
void UARTI_IRQHandler            (void) __attribute__((weak, alias("Default_Handler")));
void gpio_0_handler              (void) __attribute__((weak, alias("Default_Handler")));
void gpio_1_handler              (void) __attribute__((weak, alias("Default_Handler")));
void gpio_2_handler              (void) __attribute__((weak, alias("Default_Handler")));
void gpio_3_handler              (void) __attribute__((weak, alias("Default_Handler")));
void gpio_4_handler              (void) __attribute__((weak, alias("Default_Handler")));
void gpio_5_handler              (void) __attribute__((weak, alias("Default_Handler")));
void gpio_6_handler              (void) __attribute__((weak, alias("Default_Handler")));
void gpio_7_handler              (void) __attribute__((weak, alias("Default_Handler")));
void gpio_8_handler              (void) __attribute__((weak, alias("Default_Handler")));
void gpio_9_handler              (void) __attribute__((weak, alias("Default_Handler")));
void GP_Timer_0_IRQHandler       (void) __attribute__((weak, alias("Default_Handler")));
void GP_Timer_1_IRQHandler       (void) __attribute__((weak, alias("Default_Handler")));
void shadow32ktimer_IRQhandler   (void) __attribute__((weak, alias("Default_Handler")));
void MCU_WD0_IRQHandler          (void) __attribute__((weak, alias("Default_Handler")));
void gpio_wakeup_IRQHandler      (void) __attribute__((weak, alias("Default_Handler")));
void ATMCTR_MAILBOX_IRQhandler   (void) __attribute__((weak, alias("Default_Handler")));
// clang-format on

/*----------------------------------------------------------------------------
  Exception / Interrupt Vector table
 *----------------------------------------------------------------------------*/
const pFunc __Vectors[] __attribute__((section(".vectors"))) = {
    /* Cortex-M4 Exceptions Handler */
    (pFunc)((uint32_t)&__StackTop), /*      Initial Stack Pointer     */
    Reset_Handler,                  /*      Reset Handler             */
    NMI_Handler,                    /*      NMI Handler               */
    HardFault_Handler,              /*      Hard Fault Handler        */
    MemManage_Handler,              /*      MPU Fault Handler         */
    BusFault_Handler,               /*      Bus Fault Handler         */
    UsageFault_Handler,             /*      Usage Fault Handler       */
    0,                              /*      Reserved                  */
    0,                              /*      Reserved                  */
    0,                              /*      Reserved                  */
    0,                              /*      Reserved                  */
    SVC_Handler,                    /*      SVCall Handler            */
    DebugMon_Handler,               /*      Debug Monitor Handler     */
    0,                              /*      Reserved                  */
    PendSV_Handler,                 /*      PendSV Handler            */
    SysTick_Handler,                /*      SysTick Handler           */

    /* External interrupts */
    Default_Handler,             /*  0: Reserved                   */
    Default_Handler,             /*  1: Reserved                   */
    auxdac0_handler,             /* 2: Aux ADC int 0               */
    auxdac1_handler,             /* 3: Aux ADC int 1               */
    auxdac2_handler,             /* 4: Aux ADC int 2               */
    auxdac3_handler,             /* 5: Aux ADC int 3               */
    auxdac4_handler,             /* 6: Aux ADC int 4               */
    auxdac5_handler,             /* 7: Aux ADC int 5               */
    auxdac6_handler,             /* 8: Aux ADC int 6               */
    auxdac7_handler,             /* 9: Aux ADC int 7               */
    gdma_channel_0_handler,      /* 10: GDMA channel 0 done        */
    gdma_channel_1_handler,      /* 11: GDMA channel 0 done        */
    gdma_channel_2_handler,      /* 12: GDMA channel 0 done        */
    gdma_channel_3_handler,      /* 13: GDMA channel 0 done        */
    gdma_channel_4_handler,      /* 14: GDMA channel 0 done        */
    gdma_channel_5_handler,      /* 15: GDMA channel 0 done        */
    gdma_channel_6_handler,      /* 16: GDMA channel 0 done        */
    gdma_channel_7_handler,      /* 17: GDMA channel 0 done        */
    Default_Handler,             /* 18: Reserved                   */
    Default_Handler,             /* 19: Reserved                   */
    Default_Handler,             /* 20: Reserved                   */
    Default_Handler,             /* 21: Reserved                   */
    hw_uart_interrupt_handler0,  /* 22: MCU_SS_UARTF0_INT          */
    UARTF_ERR_IRQHandler,        /* 23: MCU_SS_UARTF0_ERR_INT      */
    hw_uart_interrupt_handler1,  /* 24: MCU_SS_UARTF1_INT          */
    UARTF_ERR_IRQHandler,        /* 25: MCU_SS_UARTF0_ERR_INT      */
    spis0_interrupt_handler,     /* 26: MCU_SS_SPI_SLAVE_INT       */
    Default_Handler,             /* 27: Reserved                   */
    Default_Handler,             /* 28: Reserved                   */
    i2c0_interrupt_handler,      /* 29: MCU_SS_I2C0_INTERRUPT      */
    Default_Handler,             /* 30: Reserved                   */
    i2c1_interrupt_handler,      /* 31: MCU_SS_I2C1_INTERRUPT      */
    Default_Handler,             /* 32: Reserved                   */
    spim0_interrupt_handler,     /* 33: MCU_SS_SPI_MASTER0_INTERRUPT */
    Default_Handler,             /* 34: Reserved                   */
    spim1_interrupt_handler,     /* 35: MCU_SS_SPI_MASTER1_INTERRUPT */
    hw_uart_interrupt_handleri0, /* 36: MCU_SS_UARTI0_INTR         */
    UARTI_IRQHandler,            /* 37: MCU_SS_UARTI1_INTR         */
    UARTI_IRQHandler,            /* 38: MCU_SS_UARTI2_INTR         */
    Default_Handler,             /* 39: Reserved                   */
    Default_Handler,             /* 40: Reserved                   */
    gpio_0_handler,              /* 41: MCU_SS_GPIO0_INT           */
    gpio_1_handler,              /* 42: MCU_SS_GPIO1_INT           */
    gpio_2_handler,              /* 43: MCU_SS_GPIO2_INT           */
    gpio_3_handler,              /* 44: MCU_SS_GPIO3_INT           */
    gpio_4_handler,              /* 45: MCU_SS_GPIO4_INT           */
    gpio_5_handler,              /* 46: MCU_SS_GPIO5_INT           */
    gpio_6_handler,              /* 47: MCU_SS_GPIO6_INT           */
    gpio_7_handler,              /* 48: MCU_SS_GPIO7_INT           */
    gpio_8_handler,              /* 49: MCU_SS_GPIO8_INT           */
    gpio_9_handler,              /* 50: MCU_SS_GPIO9_INT           */
    GP_Timer_0_IRQHandler,       /* 51: MCU GP Timer 0             */
    GP_Timer_1_IRQHandler,       /* 52: MCU GP Timer 1             */
    shadow32ktimer_IRQhandler,   /* 53: MCU Shadow 32K Timer       */
    Default_Handler,             /* 54: Reserved                   */
    MCU_WD0_IRQHandler,          /* 55: Reserved                   */
    gpio_wakeup_IRQHandler,      /* 56: GPM_IO_WAKEUP_INT_MCU      */
    ATMCTR_MAILBOX_IRQhandler,   /* 57: Reserved                   */
    Default_Handler,             /* 58: Reserved                   */
    Default_Handler,             /* 59: Reserved                   */
    Default_Handler,             /* 60: Reserved                   */
    Default_Handler,             /* 61: Reserved                   */
    Default_Handler,             /* 62: Reserved                   */
    Default_Handler,             /* 63: Reserved                   */
};

/*----------------------------------------------------------------------------
  Reset Handler called on controller reset
 *----------------------------------------------------------------------------*/
void Reset_Handler(void) __attribute__((__section__(".uncached")));
void Reset_Handler(void) {
  uint32_t *pSrc, *pDest;
  uint32_t *pTable __attribute__((unused));

  /*  Firstly it copies data from read only memory to RAM. There are two schemes
   *  to copy. One can copy more than one sections. Another can only copy
   *  one section.  The former scheme needs more instructions and read-only
   *  data to implement than the latter.
   *  Macro __STARTUP_COPY_MULTIPLE is used to choose between two schemes.  */

#ifdef __STARTUP_COPY_MULTIPLE
  /*  Multiple sections scheme.
   *
   *  Between symbol address __copy_table_start__ and __copy_table_end__,
   *  there are array of triplets, each of which specify:
   *    offset 0: LMA of start of a section to copy from
   *    offset 4: VMA of start of a section to copy to
   *    offset 8: size of the section to copy. Must be multiply of 4
   *
   *  All addresses must be aligned to 4 bytes boundary.
   */
  pTable = &__copy_table_start__;

  for (; pTable < &__copy_table_end__; pTable = pTable + 3) {
    pSrc = (uint32_t *)*(pTable + 0);
    pDest = (uint32_t *)*(pTable + 1);
    for (; pDest < (uint32_t *)(*(pTable + 1) + *(pTable + 2));) {
      *pDest++ = *pSrc++;
    }
  }
#else

  /*  Single section scheme.
   *
   *  The ranges of copy from/to are specified by following symbols
   *    __etext: LMA of start of the section to copy from. Usually end of text
   *    __data_start__: VMA of start of the section to copy to
   *    __data_end__: VMA of end of the section to copy to
   *
   *  All addresses must be aligned to 4 bytes boundary.
   */
  pSrc = &__etext;
  pDest = &__data_start__;

  for (; pDest < &__data_end__;) {
    *pDest++ = *pSrc++;
  }
#endif /*__STARTUP_COPY_MULTIPLE */

/*  This part of work usually is done in C library startup code. Otherwise,
 *  define this macro to enable it in this startup.
 *
 *  There are two schemes too. One can clear multiple BSS sections. Another
 *  can only clear one section. The former is more size expensive than the
 *  latter.
 *
 *  Define macro __STARTUP_CLEAR_BSS_MULTIPLE to choose the former.
 *  Otherwise efine macro __STARTUP_CLEAR_BSS to choose the later.
 */
#ifdef __STARTUP_CLEAR_BSS_MULTIPLE
  /*  Multiple sections scheme.
   *
   *  Between symbol address __copy_table_start__ and __copy_table_end__,
   *  there are array of tuples specifying:
   *    offset 0: Start of a BSS section
   *    offset 4: Size of this BSS section. Must be multiply of 4
   */
  pTable = &__zero_table_start__;

  for (; pTable < &__zero_table_end__; pTable = pTable + 2) {
    pDest = (uint32_t *)*(pTable + 0);
    for (; pDest < (uint32_t *)(*(pTable + 0) + *(pTable + 1));) {
      *pDest++ = 0;
    }
  }
#elif defined(__STARTUP_CLEAR_BSS)
  /*  Single BSS section scheme.
   *
   *  The BSS section is specified by following symbols
   *    __bss_start__: start of the BSS section.
   *    __bss_end__: end of the BSS section.
   *
   *  Both addresses must be aligned to 4 bytes boundary.
   */
  pDest = &__bss_start__;

  for (; pDest < &__bss_end__;) {
    *pDest++ = 0UL;
  }
#endif /* __STARTUP_CLEAR_BSS_MULTIPLE || __STARTUP_CLEAR_BSS */

#ifndef __NO_SYSTEM_INIT
  SystemInit();
#endif

#ifndef __START
#define __START _start
#endif
  __START();
}

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
  for (;;)
    ;
}

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
  printf("Default Handler for Exceptions / Interruptswas hit !!!!!!!!!!!!!!!!!\r\n");
  dump_backtrace();
  machine_reset(MACHINE_RESET_CAUSE_EXCEPTION);
  while (1)
    ;
}
