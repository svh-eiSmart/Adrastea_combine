/*
 ***************************************************************************************************
 * This file is part of WIRELESS CONNECTIVITY SDK for STM32:
 *
 *
 * THE SOFTWARE INCLUDING THE SOURCE CODE IS PROVIDED “AS IS”. YOU ACKNOWLEDGE THAT WÜRTH ELEKTRONIK
 * EISOS MAKES NO REPRESENTATIONS AND WARRANTIES OF ANY KIND RELATED TO, BUT NOT LIMITED
 * TO THE NON-INFRINGEMENT OF THIRD PARTIES’ INTELLECTUAL PROPERTY RIGHTS OR THE
 * MERCHANTABILITY OR FITNESS FOR YOUR INTENDED PURPOSE OR USAGE. WÜRTH ELEKTRONIK EISOS DOES NOT
 * WARRANT OR REPRESENT THAT ANY LICENSE, EITHER EXPRESS OR IMPLIED, IS GRANTED UNDER ANY PATENT
 * RIGHT, COPYRIGHT, MASK WORK RIGHT, OR OTHER INTELLECTUAL PROPERTY RIGHT RELATING TO ANY
 * COMBINATION, MACHINE, OR PROCESS IN WHICH THE PRODUCT IS USED. INFORMATION PUBLISHED BY
 * WÜRTH ELEKTRONIK EISOS REGARDING THIRD-PARTY PRODUCTS OR SERVICES DOES NOT CONSTITUTE A LICENSE
 * FROM WÜRTH ELEKTRONIK EISOS TO USE SUCH PRODUCTS OR SERVICES OR A WARRANTY OR ENDORSEMENT
 * THEREOF
 *
 * THIS SOURCE CODE IS PROTECTED BY A LICENSE.
 * FOR MORE INFORMATION PLEASE CAREFULLY READ THE LICENSE AGREEMENT FILE LOCATED
 * IN THE ROOT DIRECTORY OF THIS DRIVER PACKAGE.
 *
 * COPYRIGHT (c) 2023 Würth Elektronik eiSos GmbH & Co. KG
 *
 ***************************************************************************************************
 */

/**
 * @file
 * @brief This is the main header file of the WE Wireless Connectivity SDK for STM32.
 */

#ifndef GLOBAL_H_INCLUDED
#define GLOBAL_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>
//#include "global_ALT1250.h"
#include "helper.h"

// time



/**
 * @brief Driver version
 */
#define WE_WIRELESS_CONNECTIVITY_SDK_VERSION {1,7,1}

#if defined(WE_DEBUG) || defined(WE_DEBUG_INIT)
/* Redirect printf() to UART for testing/debugging purposes */
#include "debug.h"
#else
/* Ignore all occurrences of fprintf(), printf() and fflush() */
#define fprintf(...)
#define printf(...)
#define fflush(...)
#endif /* WE_DEBUG */


/**
 * @brief Priority for DMA interrupts (used for receiving data from radio module)
 */
#define WE_PRIORITY_DMA_RX 0

/**
 * @brief Priority for UART interrupts (used for communicating with radio module)
 */
#define WE_PRIORITY_UART_RX 0

/**
 * @brief Priority for (asynchronous) processing of data received from radio module.
 */
#define WE_PRIORITY_RX_DATA_PROCESSING 1

/**
 * @brief Priority for UART interface used for debugging.
 */
#define WE_PRIORITY_UART_DEBUG 2

/**
 * @brief Size of DMA receive buffer.
 */
#define WE_DMA_RX_BUFFER_SIZE 512


/**
 * @brief UART interface used for communicating with radio module.
 */
#define     __IO    volatile             /*!< Defines 'read / write' permissions */

typedef struct
{
  __IO uint32_t SR;         /*!< USART Status register,                   Address offset: 0x00 */
  __IO uint32_t DR;         /*!< USART Data register,                     Address offset: 0x04 */
  __IO uint32_t BRR;        /*!< USART Baud rate register,                Address offset: 0x08 */
  __IO uint32_t CR1;        /*!< USART Control register 1,                Address offset: 0x0C */
  __IO uint32_t CR2;        /*!< USART Control register 2,                Address offset: 0x10 */
  __IO uint32_t CR3;        /*!< USART Control register 3,                Address offset: 0x14 */
  __IO uint32_t GTPR;       /*!< USART Guard time and prescaler register, Address offset: 0x18 */
} USART_TypeDef;

extern USART_TypeDef *WE_uartWireless;

/**
 * @brief Flow control setting used for WE_uartWireless.
 */



extern WE_FlowControl_t WE_uartFlowControl;

/**
 * @brief Parity setting used for WE_uartWireless.
 */


extern WE_Parity_t WE_uartParity;

/**
 * @brief Is set to true if using DMA to receive data from radio module.
 */
extern bool WE_dmaEnabled;

/**
 * @brief Buffer used for WE_dmaWirelessRx.
 */
extern uint8_t WE_dmaRxBuffer[WE_DMA_RX_BUFFER_SIZE];

/**
 * @brief DMA used for receiving data from radio module (used only if DMA is enabled).
 */

typedef struct
{
  __IO uint32_t LISR;   /*!< DMA low interrupt status register,      Address offset: 0x00 */
  __IO uint32_t HISR;   /*!< DMA high interrupt status register,     Address offset: 0x04 */
  __IO uint32_t LIFCR;  /*!< DMA low interrupt flag clear register,  Address offset: 0x08 */
  __IO uint32_t HIFCR;  /*!< DMA high interrupt flag clear register, Address offset: 0x0C */
} DMA_TypeDef;

extern DMA_TypeDef *WE_dmaWirelessRx;

/**
 * @brief DMA stream used for receiving data from radio module (used only if DMA is enabled).
 */
extern uint32_t WE_dmaWirelessRxStream;

/**
 * @brief Last read position in DMA receive buffer (used only if DMA is enabled).
 * @see WE_CheckIfDmaDataAvailable()
 */
extern size_t WE_dmaLastReadPos;

/**
 * @brief Initializes the platform (peripherals, flash interface, Systick, system clock, interrupts etc.)
 */
extern void WE_Platform_Init(void);

/*
extern void WE_UART_Init(uint32_t baudrate,
                         WE_FlowControl_t flowControl,
                         WE_Parity_t par,
                         bool dma);


extern void WE_UART_DeInit();  */


extern void WE_UART_HandleRxByte(uint8_t receivedByte);


extern void WE_UART_Transmit(const uint8_t *data, uint16_t length);

extern void WE_Error_Handler(void);

extern void WE_SystemClock_Config(void);


extern bool WE_GetDriverVersion(uint8_t *version);



#define PERIPH_BASE           0x40000000UL /*!< Peripheral base address in the alias region                                */

//!< Peripheral memory map
#define APB1PERIPH_BASE       PERIPH_BASE
#define APB2PERIPH_BASE       (PERIPH_BASE + 0x00010000UL)
#define AHB1PERIPH_BASE       (PERIPH_BASE + 0x00020000UL)
#define AHB2PERIPH_BASE       (PERIPH_BASE + 0x10000000UL)

//!< AHB1 peripherals
#define GPIOA_BASE            (AHB1PERIPH_BASE + 0x0000UL)
#define GPIOB_BASE            (AHB1PERIPH_BASE + 0x0400UL)
#define GPIOC_BASE            (AHB1PERIPH_BASE + 0x0800UL)
#define GPIOD_BASE            (AHB1PERIPH_BASE + 0x0C00UL)
#define GPIOE_BASE            (AHB1PERIPH_BASE + 0x1000UL)
#define GPIOH_BASE            (AHB1PERIPH_BASE + 0x1C00UL)

#define GPIOA               ((GPIO_TypeDef *) GPIOA_BASE)
#define GPIOB               ((GPIO_TypeDef *) GPIOB_BASE)
#define GPIOC               ((GPIO_TypeDef *) GPIOC_BASE)
#define GPIOD               ((GPIO_TypeDef *) GPIOD_BASE)
#define GPIOE               ((GPIO_TypeDef *) GPIOE_BASE)
#define GPIOH               ((GPIO_TypeDef *) GPIOH_BASE)





// Memory mapping of Core Hardware - Defined in helper.h
//#define SCS_BASE            (0xE000E000UL)
//#define SysTick_BASE        (SCS_BASE +  0x0010UL)
//#define NVIC_BASE           (SCS_BASE +  0x0100UL)
//#define SCB_BASE            (SCS_BASE +  0x0D00UL)
//#define SCB                 ((SCB_Type       *)     SCB_BASE      )

#define SCB_ICSR_PENDSVSET_Pos             28U
#define SCB_ICSR_PENDSVSET_Msk             (1UL << SCB_ICSR_PENDSVSET_Pos)

// !< APB2 peripherals
#define TIM1_BASE             (APB2PERIPH_BASE + 0x0000UL)
#define USART1_BASE           (APB2PERIPH_BASE + 0x1000UL)

// Legacy defines
#define ADC                 ADC1_COMMON
#define SPI1                ((SPI_TypeDef *) SPI1_BASE)
#define USART1              ((USART_TypeDef *) USART1_BASE)

#define GPIO_MODE_Pos                           0U
#define GPIO_MODE                               (0x3UL << GPIO_MODE_Pos)
#define MODE_INPUT                              (0x0UL << GPIO_MODE_Pos)
#define MODE_OUTPUT                             (0x1UL << GPIO_MODE_Pos)

#define OUTPUT_TYPE_Pos                         4U
#define OUTPUT_TYPE                             (0x1UL << OUTPUT_TYPE_Pos)
#define OUTPUT_PP                               (0x0UL << OUTPUT_TYPE_Pos)
#define OUTPUT_OD                               (0x1UL << OUTPUT_TYPE_Pos)


extern bool WE_InitPins(WE_Pin_t pins[], uint8_t numPins);

/**
 * @brief Deinitialize a pin.
 *
 * @param[in] pin Pin to be deinitialized
 * @return true if request succeeded, false otherwise
 */
extern bool WE_DeinitPin(WE_Pin_t pin);

/**
 * @brief Switch pin to output high/low
 *
 * @param[in] pin Output pin to be set
 * @param[in] out Output level to be set
 * @return true if request succeeded, false otherwise
 */

/*typedef enum WE_Pin_Level_t
{
    WE_Pin_Level_Low  = (uint8_t) 0,
    WE_Pin_Level_High = (uint8_t) 1
} WE_Pin_Level_t;*/

extern bool WE_SetPin(WE_Pin_t pin, WE_Pin_Level_t out);

/**
 * @brief Read current pin level.
 *
 * @param[in] pin Pin to be read
 * @return Current level of pin
 */



extern WE_Pin_Level_t WE_GetPinLevel(WE_Pin_t pin);

/**
 * @brief Sleep function.
 *
 * @param[in] sleepForMs Delay in milliseconds
 */
extern void WE_Delay(uint32_t sleepForMs);

/**
 * @brief Sleep function.
 *
 * Note that WE_MICROSECOND_TICK needs to be defined to enable microsecond timer resolution.
 *
 * @param[in] sleepForUsec Delay in microseconds
 */
extern void WE_DelayMicroseconds(uint32_t sleepForUsec);

/**
 * @brief Returns current tick value (in milliseconds).
 *
 * @return Current tick value (in milliseconds)
 */
extern uint32_t WE_GetTick();

/**
 * @brief Returns current tick value (in microseconds).
 *
 * Note that WE_MICROSECOND_TICK needs to be defined to enable microsecond timer resolution.
 *
 * @return Current tick value (in microseconds)
 */
extern uint32_t WE_GetTickMicroseconds();

/**
 * @brief Checks if the DMA buffer contains unread data and calls OnDmaDataReceived() if so.
 */
extern void WE_CheckIfDmaDataAvailable();

/**
 * @brief Triggers asynchronous execution of WE_CheckIfDmaDataAvailable().
 *
 * Is used in ISRs to trigger reading of bytes from DMA receive buffer, which may take quite
 * some time and can thus not be done directly in ISR.
 *
 * Asynchronous execution is implemented using pendable service interrupt (PendSV).
 */
extern void WE_CheckIfDmaDataAvailableAsync();



#endif /* GLOBAL_H_INCLUDED */
