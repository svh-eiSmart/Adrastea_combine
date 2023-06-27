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

#include "if_mngr.h"
#include "mcu_exported.h"

#if IF_MNGR_ANY_UART_IS_DEFINED
#include "serial_container.h"
#endif

#if IF_MNGR_ANY_SPIM_IS_DEFINED
#include "spi.h"
#endif

#if IF_MNGR_ANY_SPIS_IS_DEFINED
#include "spi.h"
#endif

#if IF_MNGR_ANY_GPIO_IS_DEFINED
#include "gpio.h"
#endif

#if IF_MNGR_ANY_I2C_IS_DEFINED
#include "i2c.h"
#endif

#if IF_MNGR_ANY_WAKEUP_IS_DEFINED
#include "pwr_mngr.h"
#endif

#if IF_MNGR_ANY_PWM_IS_DEFINED
#include "pwm_dac.h"
#endif

eIfMngrRet if_mngr_load_defconfig(eIfMngrIf intf, void *config) {
#if IF_MNGR_ANY_UART_IS_DEFINED
  sHuart *serial_config;
#endif
#if IF_MNGR_ANY_SPIM_IS_DEFINED
  spi_config_t *spi_config;
#endif
#if IF_MNGR_ANY_SPIS_IS_DEFINED
  spis_config_t *spis_config;
#endif
#if IF_MNGR_ANY_PWM_IS_DEFINED
  pwm_cfg_t *pwm_config;
#endif
#if IF_MNGR_ANY_GPIO_IS_DEFINED
  gpio_cfg_t *gpio_config;
#endif
#if IF_MNGR_ANY_WAKEUP_IS_DEFINED
  pm_wakeup_cfg_t *wakeup_config;
#endif
  eIfMngrRet ret = IF_MNGR_FAILED;

  switch (intf) {
#if IF_MNGR_UART_IS_DEFINED(UARTF0)
    case IF_MNGR_UARTF0:
      serial_config = (sHuart *)config;
      serial_config->Instance = ACTIVE_UARTF0;
      serial_config->Init.BaudRate = UARTF0_BAUDRATE_DEFAULT;
      serial_config->Init.WordLength = UARTF0_WORDLENGTH_DEFAULT;
      serial_config->Init.StopBits = UARTF0_STOPBITS_DEFAULT;
      serial_config->Init.Parity = UARTF0_PARITY_DEFAULT;
      serial_config->Init.HwFlowCtl = UARTF0_HWFLOWCTRL_DEFAULT;
      ret = IF_MNGR_SUCCESS;
      break;
#endif

#if IF_MNGR_UART_IS_DEFINED(UARTF1)
    case IF_MNGR_UARTF1:
      serial_config = (sHuart *)config;
      serial_config->Instance = ACTIVE_UARTF1;
      serial_config->Init.BaudRate = UARTF1_BAUDRATE_DEFAULT;
      serial_config->Init.WordLength = UARTF1_WORDLENGTH_DEFAULT;
      serial_config->Init.StopBits = UARTF1_STOPBITS_DEFAULT;
      serial_config->Init.Parity = UARTF1_PARITY_DEFAULT;
      serial_config->Init.HwFlowCtl = UARTF1_HWFLOWCTRL_DEFAULT;
      ret = IF_MNGR_SUCCESS;
      break;
#endif

#if IF_MNGR_SPIM_IS_DEFINED(SPIM0)
    case IF_MNGR_SPIM0:
      spi_config = (spi_config_t *)config;
      spi_config->spiMode = SPI_MODE_MASTER;
      spi_config->spiBusId = SPIM0_BUS;
      spi_config->param.cpha = SPIM0_CPHA_DEFAULT;
      spi_config->param.cpol = SPIM0_CPOL_DEFAULT;
      spi_config->param.endian = SPIM0_ENDIAN_DEFAULT;
      spi_config->param.bitOrder = SPIM0_BITORDER_DEFAULT;
      spi_config->param.busSpeed = SPIM0_BUSSPEED_DEFAULT;
      spi_config->param.dataBits = SPIM0_DATABITS_DEFAULT;
      spi_config->param.ssMode = SPIM0_SSMODE_DEFAULT;
      ret = IF_MNGR_SUCCESS;
      break;
#endif
#if IF_MNGR_SPIM_IS_DEFINED(SPIM1)
    case IF_MNGR_SPIM1:
      spi_config = (spi_config_t *)config;
      spi_config->spiMode = SPI_MODE_MASTER;
      spi_config->spiBusId = SPIM1_BUS;
      spi_config->param.cpha = SPIM1_CPHA_DEFAULT;
      spi_config->param.cpol = SPIM1_CPOL_DEFAULT;
      spi_config->param.endian = SPIM1_ENDIAN_DEFAULT;
      spi_config->param.bitOrder = SPIM1_BITORDER_DEFAULT;
      spi_config->param.busSpeed = SPIM1_BUSSPEED_DEFAULT;
      spi_config->param.dataBits = SPIM1_DATABITS_DEFAULT;
      spi_config->param.ssMode = SPIM1_SSMODE_DEFAULT;
      ret = IF_MNGR_SUCCESS;
      break;
#endif
#if IF_MNGR_SPIS_IS_DEFINED(SPIS0)
    case IF_MNGR_SPIS0:
      spis_config = (spis_config_t *)config;
      spis_config->spisBusId = SPIS0_BUS;
      spis_config->param.cpha = SPIS0_CPHA_DEFAULT;
      spis_config->param.cpol = SPIS0_CPOL_DEFAULT;
      spis_config->param.endian = SPIS0_ENDIAN_DEFAULT;
      spis_config->param.ssMode = SPIS0_SSMODE_DEFAULT;
      ret = IF_MNGR_SUCCESS;
      break;
#endif
#if IF_MNGR_I2C_IS_DEFINED(I2C0)
    case IF_MNGR_I2C0:
      break;
#endif
#if IF_MNGR_I2C_IS_DEFINED(I2C1)
    case IF_MNGR_I2C1:
      break;
#endif
#if IF_MNGR_PWM_IS_DEFINED(PWM0)
    case IF_MNGR_PWM0:
      pwm_config = (pwm_cfg_t *)config;
      pwm_config->def_clk_div = PWM0_CLK_DIV_DEFAULT;
      pwm_config->def_duty_cycle = PWM0_DUTY_CYCLE_DEFAULT;
      ret = IF_MNGR_SUCCESS;
      break;
#endif
#if IF_MNGR_PWM_IS_DEFINED(PWM1)
    case IF_MNGR_PWM1:
      pwm_config = (pwm_cfg_t *)config;
      pwm_config->def_clk_div = PWM1_CLK_DIV_DEFAULT;
      pwm_config->def_duty_cycle = PWM1_DUTY_CYCLE_DEFAULT;
      ret = IF_MNGR_SUCCESS;
      break;
#endif
#if IF_MNGR_PWM_IS_DEFINED(PWM2)
    case IF_MNGR_PWM2:
      pwm_config = (pwm_cfg_t *)config;
      pwm_config->def_clk_div = PWM2_CLK_DIV_DEFAULT;
      pwm_config->def_duty_cycle = PWM2_DUTY_CYCLE_DEFAULT;
      ret = IF_MNGR_SUCCESS;
      break;
#endif
#if IF_MNGR_PWM_IS_DEFINED(PWM3)
    case IF_MNGR_PWM3:
      pwm_config = (pwm_cfg_t *)config;
      pwm_config->def_clk_div = PWM3_CLK_DIV_DEFAULT;
      pwm_config->def_duty_cycle = PWM3_DUTY_CYCLE_DEFAULT;
      ret = IF_MNGR_SUCCESS;
      break;
#endif
#if IF_MNGR_GPIO_IS_DEFINED(01)
    case IF_MNGR_GPIO01:
      gpio_config = (gpio_cfg_t *)config;
      gpio_config->pin_set = GPIO_ID_01_PIN_SET;
      gpio_config->def_dir = GPIO_ID_01_DIR_DEFAULT;
      gpio_config->def_pull = GPIO_ID_01_PU_DEFAULT;
      gpio_config->def_val = GPIO_ID_01_VAL_DEFAULT;
      ret = IF_MNGR_SUCCESS;
      break;
#endif
#if IF_MNGR_GPIO_IS_DEFINED(02)
    case IF_MNGR_GPIO02:
      gpio_config = (gpio_cfg_t *)config;
      gpio_config->pin_set = GPIO_ID_02_PIN_SET;
      gpio_config->def_dir = GPIO_ID_02_DIR_DEFAULT;
      gpio_config->def_pull = GPIO_ID_02_PU_DEFAULT;
      gpio_config->def_val = GPIO_ID_02_VAL_DEFAULT;
      ret = IF_MNGR_SUCCESS;
      break;
#endif
#if IF_MNGR_GPIO_IS_DEFINED(03)
    case IF_MNGR_GPIO03:
      gpio_config = (gpio_cfg_t *)config;
      gpio_config->pin_set = GPIO_ID_03_PIN_SET;
      gpio_config->def_dir = GPIO_ID_03_DIR_DEFAULT;
      gpio_config->def_pull = GPIO_ID_03_PU_DEFAULT;
      gpio_config->def_val = GPIO_ID_03_VAL_DEFAULT;
      ret = IF_MNGR_SUCCESS;
      break;
#endif
#if IF_MNGR_GPIO_IS_DEFINED(04)
    case IF_MNGR_GPIO04:
      gpio_config = (gpio_cfg_t *)config;
      gpio_config->pin_set = GPIO_ID_04_PIN_SET;
      gpio_config->def_dir = GPIO_ID_04_DIR_DEFAULT;
      gpio_config->def_pull = GPIO_ID_04_PU_DEFAULT;
      gpio_config->def_val = GPIO_ID_04_VAL_DEFAULT;
      ret = IF_MNGR_SUCCESS;
      break;
#endif
#if IF_MNGR_GPIO_IS_DEFINED(05)
    case IF_MNGR_GPIO05:
      gpio_config = (gpio_cfg_t *)config;
      gpio_config->pin_set = GPIO_ID_05_PIN_SET;
      gpio_config->def_dir = GPIO_ID_05_DIR_DEFAULT;
      gpio_config->def_pull = GPIO_ID_05_PU_DEFAULT;
      gpio_config->def_val = GPIO_ID_05_VAL_DEFAULT;
      ret = IF_MNGR_SUCCESS;
      break;
#endif
#if IF_MNGR_GPIO_IS_DEFINED(06)
    case IF_MNGR_GPIO06:
      gpio_config = (gpio_cfg_t *)config;
      gpio_config->pin_set = GPIO_ID_06_PIN_SET;
      gpio_config->def_dir = GPIO_ID_06_DIR_DEFAULT;
      gpio_config->def_pull = GPIO_ID_06_PU_DEFAULT;
      gpio_config->def_val = GPIO_ID_06_VAL_DEFAULT;
      ret = IF_MNGR_SUCCESS;
      break;
#endif
#if IF_MNGR_GPIO_IS_DEFINED(07)
    case IF_MNGR_GPIO07:
      gpio_config = (gpio_cfg_t *)config;
      gpio_config->pin_set = GPIO_ID_07_PIN_SET;
      gpio_config->def_dir = GPIO_ID_07_DIR_DEFAULT;
      gpio_config->def_pull = GPIO_ID_07_PU_DEFAULT;
      gpio_config->def_val = GPIO_ID_07_VAL_DEFAULT;
      ret = IF_MNGR_SUCCESS;
      break;
#endif
#if IF_MNGR_GPIO_IS_DEFINED(08)
    case IF_MNGR_GPIO08:
      gpio_config = (gpio_cfg_t *)config;
      gpio_config->pin_set = GPIO_ID_08_PIN_SET;
      gpio_config->def_dir = GPIO_ID_08_DIR_DEFAULT;
      gpio_config->def_pull = GPIO_ID_08_PU_DEFAULT;
      gpio_config->def_val = GPIO_ID_08_VAL_DEFAULT;
      ret = IF_MNGR_SUCCESS;
      break;
#endif
#if IF_MNGR_GPIO_IS_DEFINED(09)
    case IF_MNGR_GPIO09:
      gpio_config = (gpio_cfg_t *)config;
      gpio_config->pin_set = GPIO_ID_09_PIN_SET;
      gpio_config->def_dir = GPIO_ID_09_DIR_DEFAULT;
      gpio_config->def_pull = GPIO_ID_09_PU_DEFAULT;
      gpio_config->def_val = GPIO_ID_09_VAL_DEFAULT;
      ret = IF_MNGR_SUCCESS;
      break;
#endif
#if IF_MNGR_GPIO_IS_DEFINED(10)
    case IF_MNGR_GPIO10:
      gpio_config = (gpio_cfg_t *)config;
      gpio_config->pin_set = GPIO_ID_10_PIN_SET;
      gpio_config->def_dir = GPIO_ID_10_DIR_DEFAULT;
      gpio_config->def_pull = GPIO_ID_10_PU_DEFAULT;
      gpio_config->def_val = GPIO_ID_10_VAL_DEFAULT;
      ret = IF_MNGR_SUCCESS;
      break;
#endif
#if IF_MNGR_GPIO_IS_DEFINED(11)
    case IF_MNGR_GPIO11:
      gpio_config = (gpio_cfg_t *)config;
      gpio_config->pin_set = GPIO_ID_11_PIN_SET;
      gpio_config->def_dir = GPIO_ID_11_DIR_DEFAULT;
      gpio_config->def_pull = GPIO_ID_11_PU_DEFAULT;
      gpio_config->def_val = GPIO_ID_11_VAL_DEFAULT;
      ret = IF_MNGR_SUCCESS;
      break;
#endif
#if IF_MNGR_GPIO_IS_DEFINED(12)
    case IF_MNGR_GPIO12:
      gpio_config = (gpio_cfg_t *)config;
      gpio_config->pin_set = GPIO_ID_12_PIN_SET;
      gpio_config->def_dir = GPIO_ID_12_DIR_DEFAULT;
      gpio_config->def_pull = GPIO_ID_12_PU_DEFAULT;
      gpio_config->def_val = GPIO_ID_12_VAL_DEFAULT;
      ret = IF_MNGR_SUCCESS;
      break;
#endif
#if IF_MNGR_GPIO_IS_DEFINED(13)
    case IF_MNGR_GPIO13:
      gpio_config = (gpio_cfg_t *)config;
      gpio_config->pin_set = GPIO_ID_13_PIN_SET;
      gpio_config->def_dir = GPIO_ID_13_DIR_DEFAULT;
      gpio_config->def_pull = GPIO_ID_13_PU_DEFAULT;
      gpio_config->def_val = GPIO_ID_13_VAL_DEFAULT;
      ret = IF_MNGR_SUCCESS;
      break;
#endif
#if IF_MNGR_GPIO_IS_DEFINED(14)
    case IF_MNGR_GPIO14:
      gpio_config = (gpio_cfg_t *)config;
      gpio_config->pin_set = GPIO_ID_14_PIN_SET;
      gpio_config->def_dir = GPIO_ID_14_DIR_DEFAULT;
      gpio_config->def_pull = GPIO_ID_14_PU_DEFAULT;
      gpio_config->def_val = GPIO_ID_14_VAL_DEFAULT;
      ret = IF_MNGR_SUCCESS;
      break;
#endif
#if IF_MNGR_GPIO_IS_DEFINED(15)
    case IF_MNGR_GPIO15:
      gpio_config = (gpio_cfg_t *)config;
      gpio_config->pin_set = GPIO_ID_15_PIN_SET;
      gpio_config->def_dir = GPIO_ID_15_DIR_DEFAULT;
      gpio_config->def_pull = GPIO_ID_15_PU_DEFAULT;
      gpio_config->def_val = GPIO_ID_15_VAL_DEFAULT;
      ret = IF_MNGR_SUCCESS;
      break;
#endif
#if IF_MNGR_WAKEUP_IS_DEFINED(01)
    case IF_MNGR_WAKEUP01:
      wakeup_config = (pm_wakeup_cfg_t *)config;
      wakeup_config->pin_set = WAKEUP_IO_ID_01_PIN_SET;
      wakeup_config->pin_pol = WAKEUP_IO_ID_01_PIN_POL;
      ret = IF_MNGR_SUCCESS;
      break;
#endif
#if IF_MNGR_WAKEUP_IS_DEFINED(02)
    case IF_MNGR_WAKEUP02:
      wakeup_config = (pm_wakeup_cfg_t *)config;
      wakeup_config->pin_set = WAKEUP_IO_ID_02_PIN_SET;
      wakeup_config->pin_pol = WAKEUP_IO_ID_02_PIN_POL;
      ret = IF_MNGR_SUCCESS;
      break;
#endif
#if IF_MNGR_WAKEUP_IS_DEFINED(03)
    case IF_MNGR_WAKEUP03:
      wakeup_config = (pm_wakeup_cfg_t *)config;
      wakeup_config->pin_set = WAKEUP_IO_ID_03_PIN_SET;
      wakeup_config->pin_pol = WAKEUP_IO_ID_03_PIN_POL;
      ret = IF_MNGR_SUCCESS;
      break;
#endif
#if IF_MNGR_WAKEUP_IS_DEFINED(04)
    case IF_MNGR_WAKEUP04:
      wakeup_config = (pm_wakeup_cfg_t *)config;
      wakeup_config->pin_set = WAKEUP_IO_ID_04_PIN_SET;
      *wakeup_config->pin_pol = WAKEUP_IO_ID_04_PIN_POL;
      ret = IF_MNGR_SUCCESS;
      break;
#endif
    default:
      break;
  }
  return ret;
}

eIfMngrRet if_mngr_config_io(eIfMngrIf intf) {
  eIfMngrRet ret = IF_MNGR_FAILED;
  switch (intf) {
#if IF_MNGR_UART_IS_DEFINED(UARTF0)
    case IF_MNGR_UARTF0:
      if (io_mngr_iosel_set(UARTF0_RX_PIN_SET, IO_FUNC_UART0_RXD) != IOMNGR_SUCCESS) goto out;
      if (io_mngr_iosel_set(UARTF0_TX_PIN_SET, IO_FUNC_UART0_TXD) != IOMNGR_SUCCESS) goto out;
      if (IF_MNGR_UART_RTS_CTS_IS_DEFINED(UARTF0)) {
        if (io_mngr_iosel_set(UARTF0_RTS_PIN_SET, IO_FUNC_UART0_RTS_N) != IOMNGR_SUCCESS) goto out;
        if (io_mngr_iosel_set(UARTF0_CTS_PIN_SET, IO_FUNC_UART0_CTS_N) != IOMNGR_SUCCESS) goto out;
      }
      ret = IF_MNGR_SUCCESS;
      break;
#endif
#if IF_MNGR_UART_IS_DEFINED(UARTF1)
    case IF_MNGR_UARTF1:
      if (io_mngr_iosel_set(UARTF1_RX_PIN_SET, IO_FUNC_UART1_RXD) != IOMNGR_SUCCESS) goto out;
      if (io_mngr_iosel_set(UARTF1_TX_PIN_SET, IO_FUNC_UART1_TXD) != IOMNGR_SUCCESS) goto out;
      if (IF_MNGR_UART_RTS_CTS_IS_DEFINED(UARTF1)) {
        if (io_mngr_iosel_set(UARTF1_RTS_PIN_SET, IO_FUNC_UART1_RTS_N) != IOMNGR_SUCCESS) goto out;
        if (io_mngr_iosel_set(UARTF1_CTS_PIN_SET, IO_FUNC_UART1_CTS_N) != IOMNGR_SUCCESS) goto out;
      }
      ret = IF_MNGR_SUCCESS;
      break;
#endif
#if IF_MNGR_SPIM_IS_DEFINED(SPIM0)
    case IF_MNGR_SPIM0:
      if (io_mngr_iosel_set(SPIM0_CLK_PIN_SET, IO_FUNC_SPI0_SCK_OUT) != IOMNGR_SUCCESS) goto out;
      if (SPIM0_EN_PIN_SET != ALT1250_PIN_UNDEFINED) {
        if (io_mngr_iosel_set(SPIM0_EN_PIN_SET, IO_FUNC_SPI0_CS0_OUT) != IOMNGR_SUCCESS) goto out;
      }
      if (io_mngr_iosel_set(SPIM0_MISO_PIN_SET, IO_FUNC_SPI0_CD_OUT) != IOMNGR_SUCCESS) goto out;
      if (io_mngr_iosel_set(SPIM0_MOSI_PIN_SET, IO_FUNC_SPI0_MOSI_OUT) != IOMNGR_SUCCESS) goto out;
      ret = IF_MNGR_SUCCESS;
      break;
#endif

#if IF_MNGR_SPIM_IS_DEFINED(SPIM1)
    case IF_MNGR_SPIM1:
      if (io_mngr_iosel_set(SPIM1_CLK_PIN_SET, IO_FUNC_SPI1_SCK_OUT) != IOMNGR_SUCCESS) goto out;
      if (SPIM1_EN_PIN_SET != ALT1250_PIN_UNDEFINED) {
        if (io_mngr_iosel_set(SPIM1_EN_PIN_SET, IO_FUNC_SPI1_CS0_OUT) != IOMNGR_SUCCESS) goto out;
      }
      if (io_mngr_iosel_set(SPIM1_MISO_PIN_SET, IO_FUNC_SPI1_CD_OUT) != IOMNGR_SUCCESS) goto out;
      if (io_mngr_iosel_set(SPIM1_MOSI_PIN_SET, IO_FUNC_SPI1_MOSI_OUT) != IOMNGR_SUCCESS) goto out;
      ret = IF_MNGR_SUCCESS;
      break;
#endif

#if IF_MNGR_SPIS_IS_DEFINED(SPIS0)
    case IF_MNGR_SPIS0:
      if (io_mngr_iosel_set(SPIS0_CLK_PIN_SET, IO_FUNC_SPI_SLAVE_CLK) != IOMNGR_SUCCESS) goto out;
      if (io_mngr_iosel_set(SPIS0_MRDY_PIN_SET, IO_FUNC_SPI_SLAVE_MRDY) != IOMNGR_SUCCESS) goto out;
      if (SPIS0_SRDY_PIN_SET != ALT1250_PIN_UNDEFINED) {
        if (io_mngr_iosel_set(SPIS0_SRDY_PIN_SET, IO_FUNC_SPI_SLAVE_SRDY) != IOMNGR_SUCCESS)
          goto out;
      }
      if (io_mngr_iosel_set(SPIS0_MISO_PIN_SET, IO_FUNC_SPI_SLAVE_MISO) != IOMNGR_SUCCESS) goto out;
      if (io_mngr_iosel_set(SPIS0_MOSI_PIN_SET, IO_FUNC_SPI_SLAVE_MOSI) != IOMNGR_SUCCESS) goto out;
      ret = IF_MNGR_SUCCESS;
      break;
#endif

#if IF_MNGR_I2C_IS_DEFINED(I2C0)
    case IF_MNGR_I2C0:
      if (io_mngr_iosel_set(I2C0_SDA_PIN_SET, IO_FUNC_I2C0_SDA_OUT) != IOMNGR_SUCCESS) goto out;
      if (io_mngr_iosel_set(I2C0_SCL_PIN_SET, IO_FUNC_I2C0_SCL_OUT) != IOMNGR_SUCCESS) goto out;
      ret = IF_MNGR_SUCCESS;
      break;
#endif

#if IF_MNGR_I2C_IS_DEFINED(I2C1)
    case IF_MNGR_I2C1:
      if (io_mngr_iosel_set(I2C1_SDA_PIN_SET, IO_FUNC_I2C1_SDA_OUT) != IOMNGR_SUCCESS) goto out;
      if (io_mngr_iosel_set(I2C1_SCL_PIN_SET, IO_FUNC_I2C1_SCL_OUT) != IOMNGR_SUCCESS) goto out;
      ret = IF_MNGR_SUCCESS;
      break;
#endif

#if IF_MNGR_GPIO_IS_DEFINED(01)
    case IF_MNGR_GPIO01:
      if (io_mngr_iosel_set(GPIO_ID_01_PIN_SET, IO_FUNC_GPIO) != IOMNGR_SUCCESS) goto out;
      ret = IF_MNGR_SUCCESS;
      break;
#endif

#if IF_MNGR_GPIO_IS_DEFINED(02)
    case IF_MNGR_GPIO02:
      if (io_mngr_iosel_set(GPIO_ID_02_PIN_SET, IO_FUNC_GPIO) != IOMNGR_SUCCESS) goto out;
      ret = IF_MNGR_SUCCESS;
      break;
#endif

#if IF_MNGR_GPIO_IS_DEFINED(03)
    case IF_MNGR_GPIO03:
      if (io_mngr_iosel_set(GPIO_ID_03_PIN_SET, IO_FUNC_GPIO) != IOMNGR_SUCCESS) goto out;
      ret = IF_MNGR_SUCCESS;
      break;
#endif

#if IF_MNGR_GPIO_IS_DEFINED(04)
    case IF_MNGR_GPIO04:
      if (io_mngr_iosel_set(GPIO_ID_04_PIN_SET, IO_FUNC_GPIO) != IOMNGR_SUCCESS) goto out;
      ret = IF_MNGR_SUCCESS;
      break;
#endif

#if IF_MNGR_GPIO_IS_DEFINED(05)
    case IF_MNGR_GPIO05:
      if (io_mngr_iosel_set(GPIO_ID_05_PIN_SET, IO_FUNC_GPIO) != IOMNGR_SUCCESS) goto out;
      ret = IF_MNGR_SUCCESS;
      break;
#endif

#if IF_MNGR_GPIO_IS_DEFINED(06)
    case IF_MNGR_GPIO06:
      if (io_mngr_iosel_set(GPIO_ID_06_PIN_SET, IO_FUNC_GPIO) != IOMNGR_SUCCESS) goto out;
      ret = IF_MNGR_SUCCESS;
      break;
#endif

#if IF_MNGR_GPIO_IS_DEFINED(07)
    case IF_MNGR_GPIO07:
      if (io_mngr_iosel_set(GPIO_ID_07_PIN_SET, IO_FUNC_GPIO) != IOMNGR_SUCCESS) goto out;
      ret = IF_MNGR_SUCCESS;
      break;
#endif

#if IF_MNGR_GPIO_IS_DEFINED(08)
    case IF_MNGR_GPIO08:
      if (io_mngr_iosel_set(GPIO_ID_08_PIN_SET, IO_FUNC_GPIO) != IOMNGR_SUCCESS) goto out;
      ret = IF_MNGR_SUCCESS;
      break;
#endif

#if IF_MNGR_GPIO_IS_DEFINED(09)
    case IF_MNGR_GPIO09:
      if (io_mngr_iosel_set(GPIO_ID_09_PIN_SET, IO_FUNC_GPIO) != IOMNGR_SUCCESS) goto out;
      ret = IF_MNGR_SUCCESS;
      break;
#endif

#if IF_MNGR_GPIO_IS_DEFINED(10)
    case IF_MNGR_GPIO10:
      if (io_mngr_iosel_set(GPIO_ID_10_PIN_SET, IO_FUNC_GPIO) != IOMNGR_SUCCESS) goto out;
      ret = IF_MNGR_SUCCESS;
      break;
#endif

#if IF_MNGR_GPIO_IS_DEFINED(11)
    case IF_MNGR_GPIO11:
      if (io_mngr_iosel_set(GPIO_ID_11_PIN_SET, IO_FUNC_GPIO) != IOMNGR_SUCCESS) goto out;
      ret = IF_MNGR_SUCCESS;
      break;
#endif

#if IF_MNGR_GPIO_IS_DEFINED(12)
    case IF_MNGR_GPIO12:
      if (io_mngr_iosel_set(GPIO_ID_12_PIN_SET, IO_FUNC_GPIO) != IOMNGR_SUCCESS) goto out;
      ret = IF_MNGR_SUCCESS;
      break;
#endif

#if IF_MNGR_GPIO_IS_DEFINED(13)
    case IF_MNGR_GPIO13:
      if (io_mngr_iosel_set(GPIO_ID_13_PIN_SET, IO_FUNC_GPIO) != IOMNGR_SUCCESS) goto out;
      ret = IF_MNGR_SUCCESS;
      break;
#endif

#if IF_MNGR_GPIO_IS_DEFINED(14)
    case IF_MNGR_GPIO14:
      if (io_mngr_iosel_set(GPIO_ID_14_PIN_SET, IO_FUNC_GPIO) != IOMNGR_SUCCESS) goto out;
      ret = IF_MNGR_SUCCESS;
      break;
#endif

#if IF_MNGR_GPIO_IS_DEFINED(15)
    case IF_MNGR_GPIO15:
      if (io_mngr_iosel_set(GPIO_ID_15_PIN_SET, IO_FUNC_GPIO) != IOMNGR_SUCCESS) goto out;
      ret = IF_MNGR_SUCCESS;
      break;
#endif

#if IF_MNGR_LED_IS_DEFINED(LED0)
    case IF_MNGR_LED0:
      if (io_mngr_iosel_set(LED0_PIN_SET, IO_FUNC_LED_CTRL_LED_0) != IOMNGR_SUCCESS) goto out;
      ret = IF_MNGR_SUCCESS;
      break;
#endif

#if IF_MNGR_LED_IS_DEFINED(LED1)
    case IF_MNGR_LED1:
      if (io_mngr_iosel_set(LED1_PIN_SET, IO_FUNC_LED_CTRL_LED_1) != IOMNGR_SUCCESS) goto out;
      ret = IF_MNGR_SUCCESS;
      break;
#endif

#if IF_MNGR_LED_IS_DEFINED(LED2)
    case IF_MNGR_LED2:
      if (io_mngr_iosel_set(LED2_PIN_SET, IO_FUNC_LED_CTRL_LED_2) != IOMNGR_SUCCESS) goto out;
      ret = IF_MNGR_SUCCESS;
      break;
#endif

#if IF_MNGR_LED_IS_DEFINED(LED3)
    case IF_MNGR_LED3:
      if (io_mngr_iosel_set(LED3_PIN_SET, IO_FUNC_LED_CTRL_LED_3) != IOMNGR_SUCCESS) goto out;
      ret = IF_MNGR_SUCCESS;
      break;
#endif

#if IF_MNGR_LED_IS_DEFINED(LED4)
    case IF_MNGR_LED4:
      if (io_mngr_iosel_set(LED4_PIN_SET, IO_FUNC_LED_CTRL_LED_4) != IOMNGR_SUCCESS) goto out;
      ret = IF_MNGR_SUCCESS;
      break;
#endif

#if IF_MNGR_LED_IS_DEFINED(LED5)
    case IF_MNGR_LED5:
      if (io_mngr_iosel_set(LED5_PIN_SET, IO_FUNC_LED_CTRL_LED_5) != IOMNGR_SUCCESS) goto out;
      ret = IF_MNGR_SUCCESS;
      break;
#endif

#if IF_MNGR_PWM_IS_DEFINED(PWM0)
    case IF_MNGR_PWM0:
      if (io_mngr_iosel_set(PWM0_PIN_SET, IO_FUNC_PWM0) != IOMNGR_SUCCESS) goto out;
      ret = IF_MNGR_SUCCESS;
      break;
#endif

#if IF_MNGR_PWM_IS_DEFINED(PWM1)
    case IF_MNGR_PWM1:
      if (io_mngr_iosel_set(PWM1_PIN_SET, IO_FUNC_PWM1) != IOMNGR_SUCCESS) goto out;
      ret = IF_MNGR_SUCCESS;
      break;
#endif

#if IF_MNGR_PWM_IS_DEFINED(PWM2)
    case IF_MNGR_PWM2:
      if (io_mngr_iosel_set(PWM2_PIN_SET, IO_FUNC_PWM2) != IOMNGR_SUCCESS) goto out;
      ret = IF_MNGR_SUCCESS;
      break;
#endif

#if IF_MNGR_PWM_IS_DEFINED(PWM3)
    case IF_MNGR_PWM3:
      if (io_mngr_iosel_set(PWM3_PIN_SET, IO_FUNC_PWM3) != IOMNGR_SUCCESS) goto out;
      ret = IF_MNGR_SUCCESS;
      break;
#endif
    default:
      break;
  }
out:
  return ret;
}
