/**************************************************************************//**
 * @file     system_ARMCM4.c
 * @brief    CMSIS Device System Source File for
 *           ARMCM4 Device
 * @version  V1.0.1
 * @date     15. November 2019
 ******************************************************************************/
/*
 * Copyright (c) 2009-2019 Arm Limited. All rights reserved.
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

/*----------------------------------------------------------------------------
  Define clocks
 *----------------------------------------------------------------------------*/
#if defined(ALT1250)
  #define RC_CLK     (120000000UL)
#elif defined(ALT1255)
  #define RC_CLK     (100000000UL)
#else
  #error device not specified!
#endif

#define RC_DIV (1U)

#if (RC_DIV != 1U) && (RC_DIV != 3U) && (RC_DIV != 7U)
#error Invalid divide value to RC clock
#endif

#define SYSTEM_CLOCK (RC_CLK / (RC_DIV + 1))

/*----------------------------------------------------------------------------
  Exception / Interrupt Vector table
 *----------------------------------------------------------------------------*/
extern const VECTOR_TABLE_Type __VECTOR_TABLE[96];


/*----------------------------------------------------------------------------
  System Core Clock Variable
 *----------------------------------------------------------------------------*/
uint32_t SystemCoreClock = SYSTEM_CLOCK;  /* System Core Clock Frequency */


/*----------------------------------------------------------------------------
  System Core Clock update function
 *----------------------------------------------------------------------------*/
void SystemCoreClockUpdate (void) {
}

/*----------------------------------------------------------------------------
  System initialization function
 *----------------------------------------------------------------------------*/
void SystemInit (void)
{
#if defined (__VTOR_PRESENT) && (__VTOR_PRESENT == 1U)
  SCB->VTOR = (uint32_t) &(__VECTOR_TABLE[0]);
#endif

#if defined (__FPU_USED) && (__FPU_USED == 1U)
  SCB->CPACR |= ((3U << 10U*2U) |           /* enable CP10 Full Access */
                 (3U << 11U*2U)  );         /* enable CP11 Full Access */
#endif

#ifdef UNALIGNED_SUPPORT_DISABLE
  SCB->CCR |= SCB_CCR_UNALIGN_TRP_Msk;
#endif

  if (!CLK_CONTROL->CTRL_b.GF_RST) { // move back to boot clk
    CLK_CONTROL->CTRL_b.GF_SEL = 0; // boot clk
    CLK_CONTROL->CTRL_b.GF_RST = 1;
  }

  // uart clk derived from mcu clk
  CLK_CONTROL->CTRL_b.MUX_UART0_EXT_CLK_SEL = 0;
#if defined(ALT1250)
  CLK_CONTROL->CTRL_b.MUX_UART1_EXT_CLK_SEL = 0;
#endif

  CLK_CONTROL->CTRL_b.DIV = RC_DIV;

  CLK_CONTROL->CTRL_b.SEL1 = 0; // select rc clk

  // switch in glitch free manner from boot clock to fast clock
  CLK_CONTROL->CTRL_b.GF_RST = 0;

  CLK_CONTROL->CTRL_b.GF_SEL = 1; // rc clk divided
}
