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

#ifndef _1250B_MCU_H
#define _1250B_MCU_H
#if !defined(__IASMARM__)
#include <stdint.h>
#endif
#include "mcu_exported.h"
#include "top_exported.h"
#if defined(__ICCARM__)
#include "portmacro.h"
#elif defined(__GNUC__)
typedef long BaseType_t;
#endif

/* Old compatibility names for C types.  */
typedef unsigned long int ulong;
typedef unsigned short int ushort;
typedef unsigned int uint;

// clang-format off
typedef enum
{
  /******* Cortex-M4 Processor Exceptions Numbers ********************************************/
  NonMaskableInt_IRQn         = -14,    /*!< 2 Cortex-M4 Non Maskable Interrupt              */
  HardFault_IRQn              = -13,    /*!< 3 Cortex-M4 Hard Fault Interrupt                */
  MemoryManagement_IRQn       = -12,    /*!< 4 Cortex-M4 Memory Management Interrupt         */
  BusFault_IRQn               = -11,    /*!< 5 Cortex-M4 Bus Fault Interrupt                 */
  UsageFault_IRQn             = -10,    /*!< 6 Cortex-M4 Usage Fault Interrupt               */
  SVCall_IRQn                 = -5,     /*!< 11 Cortex-M4 SV Call Interrupt                  */
  DebugMonitor_IRQn           = -4,     /*!< 12 Cortex-M4 Debug Monitor Interrupt            */
  PendSV_IRQn                 = -2,     /*!< 14 Cortex-M4 Pend SV Interrupt                  */
  SysTick_IRQn                = -1,     /*!< 15 Cortex-M4 System Tick Interrupt              */
  /******* 1250B specific Interrupt Numbers **************************************************/
  Default_IRQn                =  0,     /*!<                                                 */
  AUXADC_INT0                 =  2,     /*!<  2 AUXADC_INT0 Interrupt                        */
  AUXADC_INT1                 =  3,     /*!<  3 AUXADC_INT1 Interrupt                        */
  AUXADC_INT2                 =  4,     /*!<  4 AUXADC_INT2 Interrupt                        */
  AUXADC_INT3                 =  5,     /*!<  5 AUXADC_INT3 Interrupt                        */
  AUXADC_INT4                 =  6,     /*!<  6 AUXADC_INT4 Interrupt                        */
  AUXADC_INT5                 =  7,     /*!<  7 AUXADC_INT5 Interrupt                        */
  AUXADC_INT6                 =  8,     /*!<  8 AUXADC_INT6 Interrupt                        */
  AUXADC_INT7                 =  9,     /*!<  9 AUXADC_INT7 Interrupt                        */
  GDMA_CH0_IRQn               = 10,     /*!< 10 GDMA channel 0 Interrupt                     */
  GDMA_CH1_IRQn               = 11,     /*!< 11 GDMA channel 1 Interrupt                     */
  GDMA_CH2_IRQn               = 12,     /*!< 12 GDMA channel 2 Interrupt                     */
  GDMA_CH3_IRQn               = 13,     /*!< 13 GDMA channel 3 Interrupt                     */
  GDMA_CH4_IRQn               = 14,     /*!< 14 GDMA channel 4 Interrupt                     */
  GDMA_CH5_IRQn               = 15,     /*!< 15 GDMA channel 5 Interrupt                     */
  GDMA_CH6_IRQn               = 16,     /*!< 16 GDMA channel 6 Interrupt                     */
  GDMA_CH7_IRQn               = 17,     /*!< 17 GDMA channel 7 Interrupt                     */
  UARTF0_IRQn                 = 22,     /*!< 22 UARTF0 Interrupt                             */
  UARTF0_ERR_IRQn             = 23,     /*!< 23 UARTF0_ERR Interrupt                         */
  UARTF1_IRQn                 = 24,     /*!< 24 UARTF1 Interrupt                             */
  UARTF1_ERR_IRQn             = 25,     /*!< 25 UARTF1_ERR Interrupt                         */
  SPIS0_IRQn                  = 26,     /*!< 26 SPIS0 Interrupt                              */
  I2C0_IRQn                   = 29,     /*!< 29 I2C0 Interrupt                               */
  I2C1_IRQn                   = 31,     /*!< 31 I2C1 Interrupt                               */
  SPIM0_IRQn                  = 33,     /*!< 33 SPIM0 Interrupt                              */
  SPIM1_IRQn                  = 35,     /*!< 35 SPIM1 Interrupt                              */
  UARTI0_IRQn                 = 36,     /*!< 36 UARTI0 Interrupt                             */
  UARTI1_IRQn                 = 37,     /*!< 37 UARTI1 Interrupt                             */
  UARTI2_IRQn                 = 38,     /*!< 38 UARTI2 Interrupt                             */
  GPIO0_IRQn                  = 41,     /*!< 41 GPIO0 Interrupt                              */
  GPIO1_IRQn                  = 42,     /*!< 42 GPIO1 Interrupt                              */
  GPIO2_IRQn                  = 43,     /*!< 43 GPIO2 Interrupt                              */
  GPIO3_IRQn                  = 44,     /*!< 44 GPIO3 Interrupt                              */
  GPIO4_IRQn                  = 45,     /*!< 45 GPIO4 Interrupt                              */
  GPIO5_IRQn                  = 46,     /*!< 46 GPIO5 Interrupt                              */
  GPIO6_IRQn                  = 47,     /*!< 47 GPIO6 Interrupt                              */
  GPIO7_IRQn                  = 48,     /*!< 48 GPIO7 Interrupt                              */
  GPIO8_IRQn                  = 49,     /*!< 49 GPIO8 Interrupt                              */
  GPIO9_IRQn                  = 50,     /*!< 50 GPIO9 Interrupt                              */
  GP_Timer_0_IRQn             = 51,     /*!< 51 GP Timer 0 Interrupt                         */
  GP_Timer_1_IRQn             = 52,     /*!< 52 GP Timer 1 Interrupt                         */
  Shadow32K_Timer_IRQn        = 53,     /*!< 53 Shadow 32K Timer Interrupt                   */
  MCU_WD0_IRQn                = 55,     /*!< 53 Shadow 32K Timer Interrupt                   */
  WAKEUP_IRQn                 = 56,     /*!< 56 Wake Up Interrupt                            */
  ATMCTR_MAILBOX_IRQn         = 57      /*!< 57 Atomic counter Interrupt                     */
} IRQn_Type;
// clang-format on

typedef enum {
  MACHINE_RESET_CAUSE_USER,
  MACHINE_RESET_CAUSE_EXCEPTION,
  MACHINE_RESET_CAUSE_ASSERT,
  MACHINE_RESET_CAUSE_UNKNOWN
} eMachineResetCause;

// clang-format off
/**
  * @brief Configuration of the Cortex-M4 Processor and Core Peripherals
   */
#define __CM4_REV                 0x0001  /*!< Cortex-M4 revision r0p1                       */
#define __MPU_PRESENT             1U      /*!< provides an MPU                               */
#define __VTOR_PRESENT            1U      /*!< VTOR present                                  */
#define __NVIC_PRIO_BITS          3U      /*!< 3 Bits for the Priority Levels                */
#define __Vendor_SysTickConfig    0U      /*!< Set to 1 if different SysTick Config is used  */
#define __FPU_PRESENT             0U      /*!< FPU present                                   */
// clang-format on

#define REF_CLK_26000000 26000000UL

#include "core_cm4.h" /* Cortex-M4 processor and core peripherals */

//#define TARGET_PORT 1

extern volatile uint32_t Ticks; /* counts timeTicks */
extern volatile uint32_t GP_Timer_cnt;

void Activate_GP_Timer_0(void);
void delayus(uint32_t freq, int32_t d);
void msleep(uint32_t milliseconds);
void machine_reset(eMachineResetCause reset_cause);

//#define MCU_UARTF_OFFSET 0
//#define MCU_UARTI_OFFSET (BASE_ADDRESS_MCU_UARTI0-BASE_ADDRESS_MCU_UARTF0)
#define MCU_UARTF_GROUP 0
#define MCU_UARTI_GROUP 1

void serial_setbrg(void);
void serial_putc_f(const char c);
void serial_putc_i(const char c);
int serial_tstc_f(void);
int serial_tstc_i(void);
int serial_getc_f(void);
BaseType_t serial_try_2getc_f(int *p_c);
void serial_read_chars_i(void);
int serial_getc_i(void);
void serial_puts_f(const char *s);
void serial_puts_i(const char *s);
int serial_tx_busy_f(void);
int serial_tx_busy_i(void);
char *serial_gets(char *dst, int max, char *names[]);

#define print_boot_msg(...)

#endif
