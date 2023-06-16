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
#include "DRV_IF_CFG.h"
#include "DRV_IO.h"
#include "PIN_MUX.h"

#if IF_CFG_ANY_UART_IS_DEFINED
#include "DRV_UART.h"
#endif

#if IF_CFG_ANY_SPIM_IS_DEFINED
#include "DRV_SPI.h"
#endif

#if IF_CFG_ANY_SPIS_IS_DEFINED
#include "DRV_SPI.h"
#endif

#if IF_CFG_ANY_GPIO_IS_DEFINED
#include "DRV_GPIO.h"
#endif

#if IF_CFG_ANY_I2C_IS_DEFINED
#include "DRV_I2C.h"
#endif

#if IF_CFG_ANY_WAKEUP_IS_DEFINED
#include "DRV_PM.h"
#endif

DRV_IF_CFG_Status DRV_IF_CFG_GetDefConfig(Interface_Id intf, void *config) {
#if IF_CFG_ANY_UART_IS_DEFINED
  DRV_UART_Config *serial_config;
#endif
#if (IF_CFG_ANY_SPIM_IS_DEFINED || IF_CFG_ANY_SPIS_IS_DEFINED)
  SPI_Config *spi_config;
#endif
#if IF_CFG_ANY_GPIO_IS_DEFINED
  GPIO_Param *gpio_config;
#endif
#if IF_CFG_ANY_WAKEUP_IS_DEFINED
  DRV_PM_WakeupConfig *wakeup_config;
#endif
  DRV_IF_CFG_Status ret = DRV_IF_CFG_ERROR_UNDEFINED;

  switch (intf) {
#if IF_CFG_UART_IS_DEFINED(UARTF0)
    case IF_CFG_UARTF0:
      serial_config = (DRV_UART_Config *)config;
      serial_config->BaudRate = UARTF0_BAUDRATE_DEFAULT;
      serial_config->WordLength = UARTF0_WORDLENGTH_DEFAULT;
      serial_config->StopBits = UARTF0_STOPBITS_DEFAULT;
      serial_config->Parity = UARTF0_PARITY_DEFAULT;
      serial_config->HwFlowCtl = UARTF0_HWFLOWCTRL_DEFAULT;
      ret = DRV_IF_CFG_OK;
      break;
#endif
#if IF_CFG_SPIM_IS_DEFINED(SPIM0)
    case IF_CFG_SPIM0:
      spi_config = (SPI_Config *)config;
      spi_config->bus_id = SPI_BUS_0;
      spi_config->param.cpha = SPIM0_CPHA_DEFAULT;
      spi_config->param.cpol = SPIM0_CPOL_DEFAULT;
      spi_config->param.endian = SPIM0_ENDIAN_DEFAULT;
      spi_config->param.bit_order = SPIM0_BITORDER_DEFAULT;
      spi_config->param.bus_speed = SPIM0_BUSSPEED_DEFAULT;
      spi_config->param.data_bits = SPIM0_DATABITS_DEFAULT;
      spi_config->param.ss_mode = SPIM0_SSMODE_DEFAULT;
      ret = DRV_IF_CFG_OK;
      break;
#endif
#if IF_CFG_SPIS_IS_DEFINED(SPIS0)
    case IF_CFG_SPIS0:
      spi_config = (SPI_Config *)config;
      spi_config->bus_id = SPI_BUS_2;
      spi_config->param.cpha = SPIS0_CPHA_DEFAULT;
      spi_config->param.cpol = SPIS0_CPOL_DEFAULT;
      spi_config->param.endian = SPIS0_ENDIAN_DEFAULT;
      spi_config->param.ss_mode = SPIS0_SSMODE_DEFAULT;
      ret = DRV_IF_CFG_OK;
      break;
#endif
#if IF_CFG_I2C_IS_DEFINED(I2C0)
    case IF_CFG_I2C0:
      break;
#endif
#if IF_CFG_GPIO_IS_DEFINED(01)
    case IF_CFG_GPIO01:
      gpio_config = (GPIO_Param *)config;
      gpio_config->pin_set = GPIO_ID_01_PIN_SET;
      gpio_config->direction = GPIO_ID_01_DIR_DEFAULT;
      gpio_config->pull = GPIO_ID_01_PU_DEFAULT;
      gpio_config->value = GPIO_ID_01_VAL_DEFAULT;
      ret = DRV_IF_CFG_OK;
      break;
#endif
#if IF_CFG_GPIO_IS_DEFINED(02)
    case IF_CFG_GPIO02:
      gpio_config = (GPIO_Param *)config;
      gpio_config->pin_set = GPIO_ID_02_PIN_SET;
      gpio_config->direction = GPIO_ID_02_DIR_DEFAULT;
      gpio_config->pull = GPIO_ID_02_PU_DEFAULT;
      gpio_config->value = GPIO_ID_02_VAL_DEFAULT;
      ret = DRV_IF_CFG_OK;
      break;
#endif
#if IF_CFG_GPIO_IS_DEFINED(03)
    case IF_CFG_GPIO03:
      gpio_config = (GPIO_Param *)config;
      gpio_config->pin_set = GPIO_ID_03_PIN_SET;
      gpio_config->direction = GPIO_ID_03_DIR_DEFAULT;
      gpio_config->pull = GPIO_ID_03_PU_DEFAULT;
      gpio_config->value = GPIO_ID_03_VAL_DEFAULT;
      ret = DRV_IF_CFG_OK;
      break;
#endif
#if IF_CFG_GPIO_IS_DEFINED(04)
    case IF_CFG_GPIO04:
      gpio_config = (GPIO_Param *)config;
      gpio_config->pin_set = GPIO_ID_04_PIN_SET;
      gpio_config->direction = GPIO_ID_04_DIR_DEFAULT;
      gpio_config->pull = GPIO_ID_04_PU_DEFAULT;
      gpio_config->value = GPIO_ID_04_VAL_DEFAULT;
      ret = DRV_IF_CFG_OK;
      break;
#endif
#if IF_CFG_GPIO_IS_DEFINED(05)
    case IF_CFG_GPIO05:
      gpio_config = (GPIO_Param *)config;
      gpio_config->pin_set = GPIO_ID_05_PIN_SET;
      gpio_config->direction = GPIO_ID_05_DIR_DEFAULT;
      gpio_config->pull = GPIO_ID_05_PU_DEFAULT;
      gpio_config->value = GPIO_ID_05_VAL_DEFAULT;
      ret = DRV_IF_CFG_OK;
      break;
#endif
#if IF_CFG_GPIO_IS_DEFINED(06)
    case IF_CFG_GPIO06:
      gpio_config = (GPIO_Param *)config;
      gpio_config->pin_set = GPIO_ID_06_PIN_SET;
      gpio_config->direction = GPIO_ID_06_DIR_DEFAULT;
      gpio_config->pull = GPIO_ID_06_PU_DEFAULT;
      gpio_config->value = GPIO_ID_06_VAL_DEFAULT;
      ret = DRV_IF_CFG_OK;
      break;
#endif
#if IF_CFG_GPIO_IS_DEFINED(07)
    case IF_CFG_GPIO07:
      gpio_config = (GPIO_Param *)config;
      gpio_config->pin_set = GPIO_ID_07_PIN_SET;
      gpio_config->direction = GPIO_ID_07_DIR_DEFAULT;
      gpio_config->pull = GPIO_ID_07_PU_DEFAULT;
      gpio_config->value = GPIO_ID_07_VAL_DEFAULT;
      ret = DRV_IF_CFG_OK;
      break;
#endif
#if IF_CFG_GPIO_IS_DEFINED(08)
    case IF_CFG_GPIO08:
      gpio_config = (GPIO_Param *)config;
      gpio_config->pin_set = GPIO_ID_08_PIN_SET;
      gpio_config->direction = GPIO_ID_08_DIR_DEFAULT;
      gpio_config->pull = GPIO_ID_08_PU_DEFAULT;
      gpio_config->value = GPIO_ID_08_VAL_DEFAULT;
      ret = DRV_IF_CFG_OK;
      break;
#endif
#if IF_CFG_GPIO_IS_DEFINED(09)
    case IF_CFG_GPIO09:
      gpio_config = (GPIO_Param *)config;
      gpio_config->pin_set = GPIO_ID_09_PIN_SET;
      gpio_config->direction = GPIO_ID_09_DIR_DEFAULT;
      gpio_config->pull = GPIO_ID_09_PU_DEFAULT;
      gpio_config->value = GPIO_ID_09_VAL_DEFAULT;
      ret = DRV_IF_CFG_OK;
      break;
#endif
#if IF_CFG_GPIO_IS_DEFINED(10)
    case IF_CFG_GPIO10:
      gpio_config = (GPIO_Param *)config;
      gpio_config->pin_set = GPIO_ID_10_PIN_SET;
      gpio_config->direction = GPIO_ID_10_DIR_DEFAULT;
      gpio_config->pull = GPIO_ID_10_PU_DEFAULT;
      gpio_config->value = GPIO_ID_10_VAL_DEFAULT;
      ret = DRV_IF_CFG_OK;
      break;
#endif
#if IF_CFG_GPIO_IS_DEFINED(11)
    case IF_CFG_GPIO11:
      gpio_config = (GPIO_Param *)config;
      gpio_config->pin_set = GPIO_ID_11_PIN_SET;
      gpio_config->direction = GPIO_ID_11_DIR_DEFAULT;
      gpio_config->pull = GPIO_ID_11_PU_DEFAULT;
      gpio_config->value = GPIO_ID_11_VAL_DEFAULT;
      ret = DRV_IF_CFG_OK;
      break;
#endif
#if IF_CFG_GPIO_IS_DEFINED(12)
    case IF_CFG_GPIO12:
      gpio_config = (GPIO_Param *)config;
      gpio_config->pin_set = GPIO_ID_12_PIN_SET;
      gpio_config->direction = GPIO_ID_12_DIR_DEFAULT;
      gpio_config->pull = GPIO_ID_12_PU_DEFAULT;
      gpio_config->value = GPIO_ID_12_VAL_DEFAULT;
      ret = DRV_IF_CFG_OK;
      break;
#endif
#if IF_CFG_GPIO_IS_DEFINED(13)
    case IF_CFG_GPIO13:
      gpio_config = (GPIO_Param *)config;
      gpio_config->pin_set = GPIO_ID_13_PIN_SET;
      gpio_config->direction = GPIO_ID_13_DIR_DEFAULT;
      gpio_config->pull = GPIO_ID_13_PU_DEFAULT;
      gpio_config->value = GPIO_ID_13_VAL_DEFAULT;
      ret = DRV_IF_CFG_OK;
      break;
#endif
#if IF_CFG_GPIO_IS_DEFINED(14)
    case IF_CFG_GPIO14:
      gpio_config = (GPIO_Param *)config;
      gpio_config->pin_set = GPIO_ID_14_PIN_SET;
      gpio_config->direction = GPIO_ID_14_DIR_DEFAULT;
      gpio_config->pull = GPIO_ID_14_PU_DEFAULT;
      gpio_config->value = GPIO_ID_14_VAL_DEFAULT;
      ret = DRV_IF_CFG_OK;
      break;
#endif
#if IF_CFG_GPIO_IS_DEFINED(15)
    case IF_CFG_GPIO15:
      gpio_config = (GPIO_Param *)config;
      gpio_config->pin_set = GPIO_ID_15_PIN_SET;
      gpio_config->direction = GPIO_ID_15_DIR_DEFAULT;
      gpio_config->pull = GPIO_ID_15_PU_DEFAULT;
      gpio_config->value = GPIO_ID_15_VAL_DEFAULT;
      ret = DRV_IF_CFG_OK;
      break;
#endif
#if IF_CFG_WAKEUP_IS_DEFINED(01)
    case IF_CFG_WAKEUP01:
      wakeup_config = (DRV_PM_WakeupConfig *)config;
      wakeup_config->pin_set = WAKEUP_IO_ID_01_PIN_SET;
      wakeup_config->pin_pol = WAKEUP_IO_ID_01_PIN_POL;
      ret = DRV_IF_CFG_OK;
      break;
#endif
#if IF_CFG_WAKEUP_IS_DEFINED(02)
    case IF_CFG_WAKEUP02:
      wakeup_config = (DRV_PM_WakeupConfig *)config;
      wakeup_config->pin_set = WAKEUP_IO_ID_02_PIN_SET;
      wakeup_config->pin_pol = WAKEUP_IO_ID_02_PIN_POL;
      ret = DRV_IF_CFG_OK;
      break;
#endif
#if IF_CFG_WAKEUP_IS_DEFINED(03)
    case IF_CFG_WAKEUP03:
      wakeup_config = (DRV_PM_WakeupConfig *)config;
      wakeup_config->pin_set = WAKEUP_IO_ID_03_PIN_SET;
      wakeup_config->pin_pol = WAKEUP_IO_ID_03_PIN_POL;
      ret = DRV_IF_CFG_OK;
      break;
#endif
#if IF_CFG_WAKEUP_IS_DEFINED(04)
    case IF_CFG_WAKEUP04:
      wakeup_config = (DRV_PM_WakeupConfig *)config;
      wakeup_config->pin_set = WAKEUP_IO_ID_04_PIN_SET;
      wakeup_config->pin_pol = WAKEUP_IO_ID_04_PIN_POL;
      ret = DRV_IF_CFG_OK;
      break;
#endif
    default:
      ret = DRV_IF_CFG_ERROR_CONFIG;
      break;
  }
  return ret;
}

#define GEN_MUX_DEF(PINSET, FUNC) MUX_##PINSET##_##FUNC
#define PIN_MUX_VALUE(PINSET, FUNC) GEN_MUX_DEF(PINSET, FUNC)

DRV_IF_CFG_Status DRV_IF_CFG_SetIO(Interface_Id intf) {
  DRV_IF_CFG_Status ret = DRV_IF_CFG_ERROR_UNDEFINED;
  DRV_IO_Status io_ret = DRV_IO_OK;
  switch (intf) {
#if IF_CFG_UART_IS_DEFINED(UARTF0)
    case IF_CFG_UARTF0:
      if ((io_ret = DRV_IO_SetMux(UARTF0_RX_PIN_SET,
                                  PIN_MUX_VALUE(UARTF0_RX_PIN_SET, UART0_RXD))) != DRV_IO_OK)
        goto out;
      if ((io_ret = DRV_IO_SetMux(UARTF0_TX_PIN_SET,
                                  PIN_MUX_VALUE(UARTF0_TX_PIN_SET, UART0_TXD))) != DRV_IO_OK)
        goto out;

      /*  MCU-2358.
          In order to detect break signal, set Rx pin to pull low
      */
      if ((io_ret = DRV_IO_SetPull(UARTF0_RX_PIN_SET, IO_PULL_DOWN)) != DRV_IO_OK) goto out;
#if IF_CFG_UART_RTS_CTS_IS_DEFINED(UARTF0)
      if ((io_ret = DRV_IO_SetMux(UARTF0_RTS_PIN_SET,
                                  PIN_MUX_VALUE(UARTF0_RTS_PIN_SET, UART0_RTS_N))) != DRV_IO_OK)
        goto out;
      if ((io_ret = DRV_IO_SetMux(UARTF0_CTS_PIN_SET,
                                  PIN_MUX_VALUE(UARTF0_CTS_PIN_SET, UART0_CTS_N))) != DRV_IO_OK)
        goto out;
#endif
      ret = DRV_IF_CFG_OK;
      break;
#endif

#if IF_CFG_SPIM_IS_DEFINED(SPIM0)
    case IF_CFG_SPIM0:
      if ((io_ret = DRV_IO_SetMux(SPIM0_CLK_PIN_SET,
                                  PIN_MUX_VALUE(SPIM0_CLK_PIN_SET, SPI0_SCK_OUT))) != DRV_IO_OK)
        goto out;
#if (SPIM0_EN_PIN_SET != ALT1255_PIN_UNDEFINED)
      if ((io_ret = DRV_IO_SetMux(SPIM0_EN_PIN_SET,
                                  PIN_MUX_VALUE(SPIM0_EN_PIN_SET, SPI0_CS0_OUT))) != DRV_IO_OK)
        goto out;
#endif
      if ((io_ret = DRV_IO_SetMux(SPIM0_MISO_PIN_SET,
                                  PIN_MUX_VALUE(SPIM0_MISO_PIN_SET, SPI0_CD_OUT))) != DRV_IO_OK)
        goto out;
      if ((io_ret = DRV_IO_SetMux(SPIM0_MOSI_PIN_SET,
                                  PIN_MUX_VALUE(SPIM0_MOSI_PIN_SET, SPI0_MOSI_OUT))) != DRV_IO_OK)
        goto out;
      ret = DRV_IF_CFG_OK;
      break;
#endif
#if IF_CFG_SPIS_IS_DEFINED(SPIS0)
    case IF_CFG_SPIS0:
      if ((io_ret = DRV_IO_SetMux(SPIS0_CLK_PIN_SET,
                                  PIN_MUX_VALUE(SPIS0_CLK_PIN_SET, SPI_SLAVE_CLK))) != DRV_IO_OK)
        goto out;
      if ((io_ret = DRV_IO_SetMux(SPIS0_MRDY_PIN_SET,
                                  PIN_MUX_VALUE(SPIS0_MRDY_PIN_SET, SPI_SLAVE_MRDY))) != DRV_IO_OK)
        goto out;
#if (SPIS0_SRDY_PIN_SET != ALT1255_PIN_UNDEFINED)
      if ((io_ret = DRV_IO_SetMux(SPIS0_SRDY_PIN_SET,
                                  PIN_MUX_VALUE(SPIS0_SRDY_PIN_SET, SPI_SLAVE_SRDY))) != DRV_IO_OK)
        goto out;
#endif
      if ((io_ret = DRV_IO_SetMux(SPIS0_MISO_PIN_SET,
                                  PIN_MUX_VALUE(SPIS0_MISO_PIN_SET, SPI_SLAVE_MISO))) != DRV_IO_OK)
        goto out;
      if ((io_ret = DRV_IO_SetMux(SPIS0_MOSI_PIN_SET,
                                  PIN_MUX_VALUE(SPIS0_MOSI_PIN_SET, SPI_SLAVE_MOSI))) != DRV_IO_OK)
        goto out;
      ret = DRV_IF_CFG_OK;
      break;
#endif
#if IF_CFG_I2C_IS_DEFINED(I2C0)
    case IF_CFG_I2C0:
      if ((io_ret = DRV_IO_SetMux(I2C0_SDA_PIN_SET,
                                  PIN_MUX_VALUE(I2C0_SDA_PIN_SET, I2C0_SDA_OUT))) != DRV_IO_OK)
        goto out;
      if ((io_ret = DRV_IO_SetMux(I2C0_SCL_PIN_SET,
                                  PIN_MUX_VALUE(I2C0_SCL_PIN_SET, I2C0_SCL_OUT))) != DRV_IO_OK)
        goto out;
      ret = DRV_IF_CFG_OK;
      break;
#endif
#if IF_CFG_GPIO_IS_DEFINED(01)
    case IF_CFG_GPIO01:
      if ((io_ret = DRV_IO_SetMux(GPIO_ID_01_PIN_SET, PIN_MUX_VALUE(GPIO_ID_01_PIN_SET, GPIO))) !=
          DRV_IO_OK)
        goto out;
      ret = DRV_IF_CFG_OK;
      break;
#endif

#if IF_CFG_GPIO_IS_DEFINED(02)
    case IF_CFG_GPIO02:
      if ((io_ret = DRV_IO_SetMux(GPIO_ID_02_PIN_SET, PIN_MUX_VALUE(GPIO_ID_02_PIN_SET, GPIO))) !=
          DRV_IO_OK)
        goto out;
      ret = DRV_IF_CFG_OK;
      break;
#endif

#if IF_CFG_GPIO_IS_DEFINED(03)
    case IF_CFG_GPIO03:
      if ((io_ret = DRV_IO_SetMux(GPIO_ID_03_PIN_SET, PIN_MUX_VALUE(GPIO_ID_03_PIN_SET, GPIO))) !=
          DRV_IO_OK)
        goto out;
      ret = DRV_IF_CFG_OK;
      break;
#endif

#if IF_CFG_GPIO_IS_DEFINED(04)
    case IF_CFG_GPIO04:
      if ((io_ret = DRV_IO_SetMux(GPIO_ID_04_PIN_SET, PIN_MUX_VALUE(GPIO_ID_04_PIN_SET, GPIO))) !=
          DRV_IO_OK)
        goto out;
      ret = DRV_IF_CFG_OK;
      break;
#endif

#if IF_CFG_GPIO_IS_DEFINED(05)
    case IF_CFG_GPIO05:
      if ((io_ret = DRV_IO_SetMux(GPIO_ID_05_PIN_SET, PIN_MUX_VALUE(GPIO_ID_05_PIN_SET, GPIO))) !=
          DRV_IO_OK)
        goto out;
      ret = DRV_IF_CFG_OK;
      break;
#endif

#if IF_CFG_GPIO_IS_DEFINED(06)
    case IF_CFG_GPIO06:
      if ((io_ret = DRV_IO_SetMux(GPIO_ID_06_PIN_SET, PIN_MUX_VALUE(GPIO_ID_06_PIN_SET, GPIO))) !=
          DRV_IO_OK)
        goto out;
      ret = DRV_IF_CFG_OK;
      break;
#endif

#if IF_CFG_GPIO_IS_DEFINED(07)
    case IF_CFG_GPIO07:
      if ((io_ret = DRV_IO_SetMux(GPIO_ID_07_PIN_SET, PIN_MUX_VALUE(GPIO_ID_07_PIN_SET, GPIO))) !=
          DRV_IO_OK)
        goto out;
      ret = DRV_IF_CFG_OK;
      break;
#endif

#if IF_CFG_GPIO_IS_DEFINED(08)
    case IF_CFG_GPIO08:
      if ((io_ret = DRV_IO_SetMux(GPIO_ID_08_PIN_SET, PIN_MUX_VALUE(GPIO_ID_08_PIN_SET, GPIO))) !=
          DRV_IO_OK)
        goto out;
      ret = DRV_IF_CFG_OK;
      break;
#endif

#if IF_CFG_GPIO_IS_DEFINED(09)
    case IF_CFG_GPIO09:
      if ((io_ret = DRV_IO_SetMux(GPIO_ID_09_PIN_SET, PIN_MUX_VALUE(GPIO_ID_09_PIN_SET, GPIO))) !=
          DRV_IO_OK)
        goto out;
      ret = DRV_IF_CFG_OK;
      break;
#endif

#if IF_CFG_GPIO_IS_DEFINED(10)
    case IF_CFG_GPIO10:
      if ((io_ret = DRV_IO_SetMux(GPIO_ID_10_PIN_SET, PIN_MUX_VALUE(GPIO_ID_10_PIN_SET, GPIO))) !=
          DRV_IO_OK)
        goto out;
      ret = DRV_IF_CFG_OK;
      break;
#endif

#if IF_CFG_GPIO_IS_DEFINED(11)
    case IF_CFG_GPIO11:
      if ((io_ret = DRV_IO_SetMux(GPIO_ID_11_PIN_SET, PIN_MUX_VALUE(GPIO_ID_11_PIN_SET, GPIO)) != DRV_IO_OK)
        goto out;
      ret = DRV_IF_CFG_OK;
      break;
#endif

#if IF_CFG_GPIO_IS_DEFINED(12)
    case IF_CFG_GPIO12:
      if ((io_ret = DRV_IO_SetMux(GPIO_ID_12_PIN_SET, PIN_MUX_VALUE(GPIO_ID_12_PIN_SET, GPIO))) != DRV_IO_OK)
        goto out;
      ret = DRV_IF_CFG_OK;
      break;
#endif

#if IF_CFG_GPIO_IS_DEFINED(13)
    case IF_CFG_GPIO13:
      if ((io_ret = DRV_IO_SetMux(GPIO_ID_13_PIN_SET, PIN_MUX_VALUE(GPIO_ID_13_PIN_SET, GPIO))) != DRV_IO_OK)
        goto out;
      ret = DRV_IF_CFG_OK;
      break;
#endif

#if IF_CFG_GPIO_IS_DEFINED(14)
    case IF_CFG_GPIO14:
      if ((io_ret = DRV_IO_SetMux(GPIO_ID_14_PIN_SET, PIN_MUX_VALUE(GPIO_ID_14_PIN_SET, GPIO))) != DRV_IO_OK)
        goto out;
      ret = DRV_IF_CFG_OK;
      break;
#endif

#if IF_CFG_GPIO_IS_DEFINED(15)
    case IF_CFG_GPIO15:
      if ((io_ret = DRV_IO_SetMux(GPIO_ID_15_PIN_SET, PIN_MUX_VALUE(GPIO_ID_15_PIN_SET, GPIO))) != DRV_IO_OK)
        goto out;
      ret = DRV_IF_CFG_OK;
      break;
#endif

#if IF_CFG_LED_IS_DEFINED(LED0)
    case IF_CFG_LED0:
      if ((io_ret = DRV_IO_SetMux(LED0_PIN_SET, PIN_MUX_VALUE(LED0_PIN_SET, LED_CTRL_LED_0))) !=
          DRV_IO_OK)
        goto out;
      ret = DRV_IF_CFG_OK;
      break;
#endif

#if IF_CFG_LED_IS_DEFINED(LED1)
    case IF_CFG_LED1:
      if ((io_ret = DRV_IO_SetMux(LED1_PIN_SET, PIN_MUX_VALUE(LED1_PIN_SET, LED_CTRL_LED_1))) !=
          DRV_IO_OK)
        goto out;
      ret = DRV_IF_CFG_OK;
      break;
#endif

#if IF_CFG_LED_IS_DEFINED(LED2)
    case IF_CFG_LED2:
      if ((io_ret = DRV_IO_SetMux(LED2_PIN_SET, PIN_MUX_VALUE(LED2_PIN_SET, LED_CTRL_LED_2))) !=
          DRV_IO_OK)
        goto out;
      ret = DRV_IF_CFG_OK;
      break;
#endif

#if IF_CFG_PWM_IS_DEFINED(PWM0)
    case IF_CFG_PWM0:
      if ((io_ret = DRV_IO_SetMux(PWM0_PIN_SET, PIN_MUX_VALUE(PWM0_PIN_SET, PWM0))) != DRV_IO_OK)
        goto out;
      ret = DRV_IF_CFG_OK;
      break;
#endif

#if IF_CFG_PWM_IS_DEFINED(PWM1)
    case IF_CFG_PWM1:
      if ((io_ret = DRV_IO_SetMux(PWM1_PIN_SET, PIN_MUX_VALUE(PWM1_PIN_SET, PWM1))) != DRV_IO_OK)
        goto out;
      ret = DRV_IF_CFG_OK;
      break;
#endif
    default:
      ret = DRV_IF_CFG_ERROR_UNDEFINED;
      break;
  }
out:
  if (io_ret == DRV_IO_ERROR_PARTITION)
    ret = DRV_IF_CFG_ERROR_IOPAR;
  else if (io_ret == DRV_IO_ERROR_PARAM)
    ret = DRV_IF_CFG_ERROR_CONFIG;
  else if (io_ret == DRV_IO_ERROR_UNSUPPORTED)
    ret = DRV_IF_CFG_ERROR_UNSUPPORTED;

  return ret;
}
