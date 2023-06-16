/******************************************************************************
 * @file     startup_ARMCM4.c
 * @brief    CMSIS-Core(M) Device Startup File for a Cortex-M4 Device
 * @version  V2.0.3
 * @date     31. March 2020
 ******************************************************************************/
/*
 * Copyright (c) 2009-2020 Arm Limited. All rights reserved.
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

#include DEVICE_HEADER
#if (configUSE_ALT_SLEEP == 1)
#include "DRV_PM.h"
#include "hibernate.h"
#endif

/*----------------------------------------------------------------------------
  External References
 *----------------------------------------------------------------------------*/
extern uint32_t __INITIAL_SP;

extern __NO_RETURN void __PROGRAM_START(void);

/*----------------------------------------------------------------------------
  Internal References
 *----------------------------------------------------------------------------*/
__NO_RETURN void Reset_Handler(void) __attribute__((__section__(".uncached")));
void Default_Handler(void) __attribute__((weak));

/*----------------------------------------------------------------------------
  Exception / Interrupt Handler
 *----------------------------------------------------------------------------*/
/* Cortex-M4 Processor Exceptions */
void NMI_Handler(void) __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void) __attribute__((weak));

void MemManage_Handler(void) __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void) __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void) __attribute__((weak, alias("Default_Handler")));

/* ALT125x Specific Interrupts */
void WDT_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void RTC_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void MCIA_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void MCIB_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void UART0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void UART1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void UART2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void UART4_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void AACI_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void CLCD_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void ENET_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void USBDC_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void USBHC_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void CHLCD_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void FLEXRAY_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void CAN_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void LIN_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void i2c0_interrupt_err_handler(void) __attribute__((weak, alias("Default_Handler")));
void i2c0_interrupt_handler(void) __attribute__((weak, alias("Default_Handler")));
void i2c1_interrupt_err_handler(void) __attribute__((weak, alias("Default_Handler")));
void i2c1_interrupt_handler(void) __attribute__((weak, alias("Default_Handler")));
void CPU_CLCD_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void UART3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void spis0_interrupt_handler(void) __attribute__((weak, alias("Default_Handler")));
void spim0_interrupt_handler(void) __attribute__((weak, alias("Default_Handler")));
void spim1_interrupt_handler(void) __attribute__((weak, alias("Default_Handler")));
void auxdac0_handler(void) __attribute__((weak, alias("Default_Handler")));
void auxdac1_handler(void) __attribute__((weak, alias("Default_Handler")));
void auxdac2_handler(void) __attribute__((weak, alias("Default_Handler")));
void auxdac3_handler(void) __attribute__((weak, alias("Default_Handler")));
void auxdac4_handler(void) __attribute__((weak, alias("Default_Handler")));
void auxdac5_handler(void) __attribute__((weak, alias("Default_Handler")));
void auxdac6_handler(void) __attribute__((weak, alias("Default_Handler")));
void auxdac7_handler(void) __attribute__((weak, alias("Default_Handler")));
void gdma_channel_0_handler(void) __attribute__((weak, alias("Default_Handler")));
void gdma_channel_1_handler(void) __attribute__((weak, alias("Default_Handler")));
void gdma_channel_2_handler(void) __attribute__((weak, alias("Default_Handler")));
void gdma_channel_3_handler(void) __attribute__((weak, alias("Default_Handler")));
void gdma_channel_4_handler(void) __attribute__((weak, alias("Default_Handler")));
void gdma_channel_5_handler(void) __attribute__((weak, alias("Default_Handler")));
void gdma_channel_6_handler(void) __attribute__((weak, alias("Default_Handler")));
void gdma_channel_7_handler(void) __attribute__((weak, alias("Default_Handler")));
void hw_uart_interrupt_handler0(void) __attribute__((weak, alias("Default_Handler")));
void hw_uart_interrupt_handler1(void) __attribute__((weak, alias("Default_Handler")));
void hw_uart_interrupt_handleri0(void) __attribute__((weak, alias("Default_Handler")));
void UARTF_ERR_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void UARTI_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void gpio_0_handler(void) __attribute__((weak, alias("Default_Handler")));
void gpio_1_handler(void) __attribute__((weak, alias("Default_Handler")));
void gpio_2_handler(void) __attribute__((weak, alias("Default_Handler")));
void gpio_3_handler(void) __attribute__((weak, alias("Default_Handler")));
void gpio_4_handler(void) __attribute__((weak, alias("Default_Handler")));
void gpio_5_handler(void) __attribute__((weak, alias("Default_Handler")));
void gpio_6_handler(void) __attribute__((weak, alias("Default_Handler")));
void gpio_7_handler(void) __attribute__((weak, alias("Default_Handler")));
void gpio_8_handler(void) __attribute__((weak, alias("Default_Handler")));
void gpio_9_handler(void) __attribute__((weak, alias("Default_Handler")));
void GP_Timer_0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void GP_Timer_1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void shadow32ktimer_IRQhandler(void) __attribute__((weak, alias("Default_Handler")));
void MCU_WD0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void gpio_wakeup_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void ATMCTR_MAILBOX_IRQhandler(void) __attribute__((weak, alias("Default_Handler")));
void CC_PWM0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void CC_PWM1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));

/*----------------------------------------------------------------------------
  Exception / Interrupt Vector table
 *----------------------------------------------------------------------------*/

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

extern const VECTOR_TABLE_Type __VECTOR_TABLE[96];
const VECTOR_TABLE_Type __VECTOR_TABLE[96] __VECTOR_TABLE_ATTRIBUTE = {
    (VECTOR_TABLE_Type)(&__INITIAL_SP), /*     Initial Stack Pointer */
    Reset_Handler,                      /*     Reset Handler         */
    NMI_Handler,                        /* -14 NMI Handler           */
    HardFault_Handler,                  /* -13 Hard Fault Handler    */
    MemManage_Handler,                  /* -12 MPU Fault Handler     */
    BusFault_Handler,                   /* -11 Bus Fault Handler     */
    UsageFault_Handler,                 /* -10 Usage Fault Handler   */
    0,                                  /*     Reserved              */
    0,                                  /*     Reserved              */
    0,                                  /*     Reserved              */
    0,                                  /*     Reserved              */
    SVC_Handler,                        /*  -5 SVCall Handler        */
    DebugMon_Handler,                   /*  -4 Debug Monitor Handler */
    0,                                  /*     Reserved              */
    PendSV_Handler,                     /*  -2 PendSV Handler        */
    SysTick_Handler,                    /*  -1 SysTick Handler       */

    /* External interrupts */
    Default_Handler,             /*  0: Reserved              */
    Default_Handler,             /*  1: Reserved              */
    auxdac0_handler,             /*  2: Aux ADC int 0         */
    auxdac1_handler,             /*  3: Aux ADC int 1         */
    auxdac2_handler,             /*  4: Aux ADC int 2         */
    auxdac3_handler,             /*  5: Aux ADC int 3         */
    auxdac4_handler,             /*  6: Aux ADC int 4         */
    auxdac5_handler,             /*  7: Aux ADC int 5         */
    auxdac6_handler,             /*  8: Aux ADC int 6         */
    auxdac7_handler,             /*  9: Aux ADC int 7         */
    gdma_channel_0_handler,      /* 10: GDMA channel 0 done   */
    gdma_channel_1_handler,      /* 11: GDMA channel 0 done   */
    gdma_channel_2_handler,      /* 12: GDMA channel 0 done   */
    gdma_channel_3_handler,      /* 13: GDMA channel 0 done   */
    gdma_channel_4_handler,      /* 14: GDMA channel 0 done   */
    gdma_channel_5_handler,      /* 15: GDMA channel 0 done   */
    gdma_channel_6_handler,      /* 16: GDMA channel 0 done   */
    gdma_channel_7_handler,      /* 17: GDMA channel 0 done   */
    CC_PWM0_IRQHandler,          /* 18: CC_PWM0_INT           */
    Default_Handler,             /* 19: Reserved              */
    Default_Handler,             /* 20: Reserved              */
    Default_Handler,             /* 21: Reserved              */
    hw_uart_interrupt_handler0,  /* 22: UARTF0_INT            */
    UARTF_ERR_IRQHandler,        /* 23: UARTF0_ERR_INT        */
    hw_uart_interrupt_handler1,  /* 24: UARTF1_INT            */
    UARTF_ERR_IRQHandler,        /* 25: UARTF0_ERR_INT        */
    spis0_interrupt_handler,     /* 26: SPI_SLAVE_INT         */
    Default_Handler,             /* 27: Reserved              */
    i2c0_interrupt_err_handler,  /* 28: I2C0_ERROR_INT        */
    i2c0_interrupt_handler,      /* 29: I2C0_INTERRUPT        */
    i2c1_interrupt_err_handler,  /* 30: I2C1_ERROR_INT        */
    i2c1_interrupt_handler,      /* 31: I2C1_INTERRUPT        */
    Default_Handler,             /* 32: Reserved              */
    spim0_interrupt_handler,     /* 33: SPI_MASTER0_INTERRUPT */
    Default_Handler,             /* 34: Reserved              */
    spim1_interrupt_handler,     /* 35: SPI_MASTER1_INTERRUPT */
    hw_uart_interrupt_handleri0, /* 36: UARTI0_INTR           */
    UARTI_IRQHandler,            /* 37: UARTI1_INTR           */
    UARTI_IRQHandler,            /* 38: UARTI2_INTR           */
    Default_Handler,             /* 39: Reserved              */
    Default_Handler,             /* 40: Reserved              */
    gpio_0_handler,              /* 41: GPIO0_INT             */
    gpio_1_handler,              /* 42: GPIO1_INT             */
    gpio_2_handler,              /* 43: GPIO2_INT             */
    gpio_3_handler,              /* 44: GPIO3_INT             */
    gpio_4_handler,              /* 45: GPIO4_INT             */
    gpio_5_handler,              /* 46: GPIO5_INT             */
    gpio_6_handler,              /* 47: GPIO6_INT             */
    gpio_7_handler,              /* 48: GPIO7_INT             */
    gpio_8_handler,              /* 49: GPIO8_INT             */
    gpio_9_handler,              /* 50: GPIO9_INT             */
    GP_Timer_0_IRQHandler,       /* 51: GP Timer 0            */
    GP_Timer_1_IRQHandler,       /* 52: GP Timer 1            */
    shadow32ktimer_IRQhandler,   /* 53: Shadow 32K Timer      */
    Default_Handler,             /* 54: Reserved              */
    MCU_WD0_IRQHandler,          /* 55: Reserved              */
    gpio_wakeup_IRQHandler,      /* 56: GPM_IO_WAKEUP_INT_MCU */
    ATMCTR_MAILBOX_IRQhandler,   /* 57: Reserved              */
    CC_PWM1_IRQHandler,          /* 58: CC_PWM1_INT           */
    Default_Handler,             /* 59: Reserved              */
    Default_Handler,             /* 60: Reserved              */
    Default_Handler,             /* 61: Reserved              */
    Default_Handler,             /* 62: Reserved              */
    Default_Handler,             /* 63: Reserved              */
};

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

/*----------------------------------------------------------------------------
  Reset Handler called on controller reset
 *----------------------------------------------------------------------------*/
#if (configUSE_ALT_SLEEP == 1)
__NO_RETURN void Reset_Handler(void) {
  // invalidate cache lines
  REGFILE->ICACHE_CTRL = 1;
  REGFILE->ICACHE_CTRL = 0;

  /* DEBUG TIP: to debug early startup using JTAG, uncomment the line below.
   * this will force wait on startup, then break in JTAG and advance manually
   * past this point. */
  //__asm volatile("wfi");

  SystemInit(); /* CMSIS System Initialization */
  if (DRV_PM_EarlyBootProbe() == DRV_PM_WAKEUP_STATEFUL) {
    if (hibernate_from_gpm() != 0) {
      __PROGRAM_START();
    }

    DRV_PM_Resume();
  } 
#if (PWR_MNGR_EN_HIBERNATE_SHUTDOWN == 1)
  else if (DRV_PM_EarlyBootTypeProbe() == DRV_PM_DEV_WARM_BOOT) {
    if (hibernate_from_flash() != 0) {
      __PROGRAM_START();
    }

    if (hibernate_from_gpm() != 0) {
      __PROGRAM_START();
    }

    DRV_PM_Resume();
  } 
#endif
  else {
    __PROGRAM_START(); /* Enter PreMain (C library entry point) */
  }
}
#else
__NO_RETURN void Reset_Handler(void) {
  // invalidate cache lines
  REGFILE->ICACHE_CTRL = 1;
  REGFILE->ICACHE_CTRL = 0;

  SystemInit();      /* CMSIS System Initialization */
  __PROGRAM_START(); /* Enter PreMain (C library entry point) */
}
#endif

#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
#endif

/*----------------------------------------------------------------------------
  Hard Fault Handler
 *----------------------------------------------------------------------------*/
void HardFault_Handler(void) {
  while (1)
    ;
}

/*----------------------------------------------------------------------------
  Default Handler for Exceptions / Interrupts
 *----------------------------------------------------------------------------*/
void Default_Handler(void) {
  while (1)
    ;
}

#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
#pragma clang diagnostic pop
#endif
