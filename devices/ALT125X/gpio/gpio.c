/*  ---------------------------------------------------------------------------

        (c) copyright 2017 Altair Semiconductor, Ltd. All rights reserved.

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

#include <stdio.h>
#include <string.h>
#include "125X_mcu.h"
#include "gpio.h"
#include "gpio_wakeup_utils.h"
#include "mcu_exported.h"
#include "clk_gating.h"

#define ALT125X_GPIO_INTERRUPTS_ENABLED

/*
 * The GPIO module in the alt38xx Systems-on-Chip is a
 * device, managing 8 pins and alternate functions.
 */
#define CONFIG_ALTxxxx_IO_BASE BASE_ADDRESS_MCU_IO_FUNC_SEL
#define GPIO_NUM_BLOCKS 10

#define CONFIG_ALT125X_GPIO_BASE MCU_GPIO_0_DATA

#define GPIO_PINS_PER_BLOCK 8

#define GPIO_BIT(x) (((x)-1) % (GPIO_PINS_PER_BLOCK))

#define GPIO_BANK_SIZE (GPIO_NUM_BLOCKS * GPIO_PINS_PER_BLOCK)
#define GPIO_BLOCK_BASE(x) (((x) > GPIO_BANK_SIZE) ? ((x)-GPIO_BANK_SIZE) : (x))

#define GPIO_BLOCK_SIZE (MCU_GPIO_1_DATA - MCU_GPIO_0_DATA)

/*Macros are ONE based GPIO pin number (first pin is 1)*/
#define GPIO_DAT_PORT(x) \
  ((((GPIO_BLOCK_BASE(x) - 1) >> 3) * GPIO_BLOCK_SIZE) + CONFIG_ALT125X_GPIO_BASE + 0x0)
#define GPIO_DIR_PORT(x) \
  ((((GPIO_BLOCK_BASE(x) - 1) >> 3) * GPIO_BLOCK_SIZE) + CONFIG_ALT125X_GPIO_BASE + 0xC)

// INT_SENSE: 0=edge, 1=level
#define GPIO_INT_SENSE_PORT(x) \
  ((((GPIO_BLOCK_BASE(x) - 1) >> 3) * GPIO_BLOCK_SIZE) + CONFIG_ALT125X_GPIO_BASE + 0x18)
// INT_BOTH_EDGES: only valid on 'edge'. 0=single edge, 1=both edges
#define GPIO_INT_BOTH_EDGES_PORT(x) \
  ((((GPIO_BLOCK_BASE(x) - 1) >> 3) * GPIO_BLOCK_SIZE) + CONFIG_ALT125X_GPIO_BASE + 0x24)
// INT_EVENT: 0=low level/falling edge, 1=high level/rising edge
#define GPIO_INT_EVENT_PORT(x) \
  ((((GPIO_BLOCK_BASE(x) - 1) >> 3) * GPIO_BLOCK_SIZE) + CONFIG_ALT125X_GPIO_BASE + 0x30)
// INT_MASK: 0=interrupt disabled, 1=interrupt enable
#define GPIO_INT_MASK_PORT(x) \
  ((((GPIO_BLOCK_BASE(x) - 1) >> 3) * GPIO_BLOCK_SIZE) + CONFIG_ALT125X_GPIO_BASE + 0x3C)
// INT_MASK_SET: enable interrupt
#define GPIO_INT_MASK_SET_PORT(x) \
  ((((GPIO_BLOCK_BASE(x) - 1) >> 3) * GPIO_BLOCK_SIZE) + MCU_GPIO_0_INT_MASK_SET)
// INT_MASK_CLR: disable interrupt
#define GPIO_INT_MASK_CLR_PORT(x) \
  ((((GPIO_BLOCK_BASE(x) - 1) >> 3) * GPIO_BLOCK_SIZE) + MCU_GPIO_0_INT_MASK_CLR)
// INT_CLR: clear gpio interrupt source
#define GPIO_INT_CLR_PORT(x) \
  ((((GPIO_BLOCK_BASE(x) - 1) >> 3) * GPIO_BLOCK_SIZE) + CONFIG_ALT125X_GPIO_BASE + 0x48)

#define GPIO_INIT_CHECK       \
  if (!g_gpio_initialized) {  \
    return GPIO_RET_INIT_ERR; \
  }

#define GPIO_IOPAR_VALIDATE(p)                          \
  if (io_mngr_iopar_validate(p) == IOMNGR_IO_PAR_ERR) { \
    return GPIO_RET_FORBIDDEN;                          \
  }

#define GPIO_CHECK_PIN_NUMBER(p)                           \
  if (((p) < MCU_GPIO_ID_01 || (p) >= MCU_GPIO_ID_NUM) ||  \
      (mcu_gpio_table[p].phys_pin_id < MCU_PIN_ID_START || \
       mcu_gpio_table[p].phys_pin_id >= MCU_PIN_ID_NUM)) { \
    return GPIO_RET_INVALID_PORT;                          \
  }

#if defined(ALT125X_GPIO_INTERRUPTS_ENABLED)
static int g_gpio_irq_initialized = 0;
static void gpio_interrupts_init();
#endif

static int g_gpio_initialized = 0;

typedef struct _mcuGpioCfg {
  eMcuPinIds phys_pin_id;
  eIoMngrFunc orig_mux;
#if defined(ALT125X_GPIO_INTERRUPTS_ENABLED)
  uint32_t irq_enabled;
  gpio_interrupt_handler irq_handler;
  int user_param;
#endif
} mcuGpioCfg_t;

static mcuGpioCfg_t mcu_gpio_table[MCU_GPIO_ID_NUM];
static eMcuGpioIds phys_pin_map[MCU_PIN_ID_NUM];

static void gpioctl_set_pull(eMcuPinIds pin_id, eGpioPullMode pullmode) {
  io_mngr_reg_ofs_t iosel_reg_ofs;
  unsigned int reg = 0;

  iosel_reg_ofs = io_mngr_get_iosel_reg_ofs(pin_id);

  if (MCU_PIN_IS_VIRTUAL_PIN(pin_id)) {
    return;
  }

  if (MCU_PIN_IS_GPM_DOMAIN(pin_id)) {
    reg = REGISTER(TOPREG(BASE_ADDRESS_GPM_IO_CFG) + iosel_reg_ofs);
  }

  if (MCU_PIN_IS_PMP_DOMAIN(pin_id)) {
    reg = REGISTER(TOPREG(BASE_ADDRESS_PMP_IO_CFG) + iosel_reg_ofs);
  }

  switch (pullmode) {
    case GPIO_PULL_NONE:
      reg &= ~GPM_IO_CFG_IO_CFG_GPIO0_IO_CFG_PE_GPIO0_MSK;
      break;

    case GPIO_PULL_UP:
      reg |= (GPM_IO_CFG_IO_CFG_GPIO0_IO_CFG_PE_GPIO0_MSK |
              GPM_IO_CFG_IO_CFG_GPIO0_IO_CFG_PS_GPIO0_MSK);
      break;

    case GPIO_PULL_DOWN:
      reg |= GPM_IO_CFG_IO_CFG_GPIO0_IO_CFG_PE_GPIO0_MSK;
      reg &= ~GPM_IO_CFG_IO_CFG_GPIO0_IO_CFG_PS_GPIO0_MSK;
      break;
    case GPIO_PULL_DONT_CHANGE:
    default:
      break;
  }

  /*GPIO0 ... PB*/
  if (MCU_PIN_IS_GPM_DOMAIN(pin_id)) {
    REGISTER(TOPREG(BASE_ADDRESS_GPM_IO_CFG) + iosel_reg_ofs) = reg;
  }

  /*FLASH0_CS_N0 ... FLASH0_CS_N2*/
  if (MCU_PIN_IS_PMP_DOMAIN(pin_id)) {
    REGISTER(TOPREG(BASE_ADDRESS_PMP_IO_CFG) + iosel_reg_ofs) = reg;
  }
}

static void gpioctl_set_direction(eMcuPinIds pin_id, eGpioDir dir) {
  if (dir == GPIO_DIR_INPUT)
    REGISTER(GPIO_DIR_PORT(pin_id)) = REGISTER(GPIO_DIR_PORT(pin_id)) & ~(0x1 << GPIO_BIT(pin_id));
  else
    REGISTER(GPIO_DIR_PORT(pin_id)) = REGISTER(GPIO_DIR_PORT(pin_id)) | (0x1 << GPIO_BIT(pin_id));
}

static int gpioctl_get_value(eMcuPinIds pin_id) {
  return ((REGISTER(GPIO_DAT_PORT(pin_id)) & (0x1 << GPIO_BIT(pin_id))) != 0);
}

static eGpioRet gpioctl_disable_interrupt(eMcuPinIds pin_id) {
  REGISTER(GPIO_INT_MASK_CLR_PORT(pin_id)) = (0x01 << GPIO_BIT(pin_id));
  return GPIO_RET_SUCCESS;
}

static void gpioctl_clear_interrupt(eMcuPinIds pin_id) {
  REGISTER(GPIO_INT_CLR_PORT(pin_id)) = (0x01 << GPIO_BIT(pin_id));  // 1=clear interrupt
}

static void gpioctl_enable_interrupt(eMcuPinIds pin_id) {
  REGISTER(GPIO_INT_MASK_SET_PORT(pin_id)) = (0x01 << GPIO_BIT(pin_id));  // 1=enable interrupt
}

static void gpioctl_config_interrupt(eMcuPinIds pin_id, eGpioIrqMode irq_mode,
                                     eGpioPullMode pullmode) {
  gpioctl_set_direction(pin_id, GPIO_DIR_INPUT);  // one based
  gpioctl_set_pull(pin_id, pullmode);

  switch (irq_mode) {
    case GPIO_IRQ_RISING_EDGE:
      REGISTER(GPIO_INT_SENSE_PORT(pin_id)) =
          REGISTER(GPIO_INT_SENSE_PORT(pin_id)) & ~(0x1 << GPIO_BIT(pin_id));  // edge
      REGISTER(GPIO_INT_BOTH_EDGES_PORT(pin_id)) =
          REGISTER(GPIO_INT_BOTH_EDGES_PORT(pin_id)) & ~(0x1 << GPIO_BIT(pin_id));  // single edge
      REGISTER(GPIO_INT_EVENT_PORT(pin_id)) =
          REGISTER(GPIO_INT_EVENT_PORT(pin_id)) | (0x1 << GPIO_BIT(pin_id));  // rising
      break;

    case GPIO_IRQ_FALLING_EDGE:
      REGISTER(GPIO_INT_SENSE_PORT(pin_id)) =
          REGISTER(GPIO_INT_SENSE_PORT(pin_id)) & ~(0x1 << GPIO_BIT(pin_id));  // edge
      REGISTER(GPIO_INT_BOTH_EDGES_PORT(pin_id)) =
          REGISTER(GPIO_INT_BOTH_EDGES_PORT(pin_id)) & ~(0x1 << GPIO_BIT(pin_id));  // single edge
      REGISTER(GPIO_INT_EVENT_PORT(pin_id)) =
          REGISTER(GPIO_INT_EVENT_PORT(pin_id)) & ~(0x1 << GPIO_BIT(pin_id));  // falling
      break;

    case GPIO_IRQ_BOTH_EDGES:
      REGISTER(GPIO_INT_SENSE_PORT(pin_id)) =
          REGISTER(GPIO_INT_SENSE_PORT(pin_id)) & ~(0x1 << GPIO_BIT(pin_id));  // edge
      REGISTER(GPIO_INT_BOTH_EDGES_PORT(pin_id)) =
          REGISTER(GPIO_INT_BOTH_EDGES_PORT(pin_id)) | (0x1 << GPIO_BIT(pin_id));  // both edges
      REGISTER(GPIO_INT_EVENT_PORT(pin_id)) =
          REGISTER(GPIO_INT_EVENT_PORT(pin_id)) &
          ~(0x1 << GPIO_BIT(pin_id));  // just set as zero (should be ignored)
      break;

    case GPIO_IRQ_HIGH_LEVEL:
      REGISTER(GPIO_INT_SENSE_PORT(pin_id)) =
          REGISTER(GPIO_INT_SENSE_PORT(pin_id)) | (0x1 << GPIO_BIT(pin_id));  // level
      REGISTER(GPIO_INT_BOTH_EDGES_PORT(pin_id)) =
          REGISTER(GPIO_INT_BOTH_EDGES_PORT(pin_id)) &
          ~(0x1 << GPIO_BIT(pin_id));  // just set as zero (should be ignored)
      REGISTER(GPIO_INT_EVENT_PORT(pin_id)) =
          REGISTER(GPIO_INT_EVENT_PORT(pin_id)) | (0x1 << GPIO_BIT(pin_id));  // high level
      break;

    case GPIO_IRQ_LOW_LEVEL:
      REGISTER(GPIO_INT_SENSE_PORT(pin_id)) =
          REGISTER(GPIO_INT_SENSE_PORT(pin_id)) | (0x1 << GPIO_BIT(pin_id));  // level
      REGISTER(GPIO_INT_BOTH_EDGES_PORT(pin_id)) =
          REGISTER(GPIO_INT_BOTH_EDGES_PORT(pin_id)) &
          ~(0x1 << GPIO_BIT(pin_id));  // just set as zero (should be ignored)
      REGISTER(GPIO_INT_EVENT_PORT(pin_id)) =
          REGISTER(GPIO_INT_EVENT_PORT(pin_id)) & ~(0x1 << GPIO_BIT(pin_id));  // low level
      break;

    default:
      printf("Error - invalid gpio irq mode (%d, %d)\n\r", pin_id, irq_mode);
      break;
  }

  gpioctl_clear_interrupt(
      pin_id);  // clear interrupt in case it occurred during transitions states above
}

static eGpioPullMode gpioctl_get_pull(eMcuPinIds pin_id) {
  unsigned int reg = 0;
  io_mngr_reg_ofs_t iosel_reg_ofs;

  iosel_reg_ofs = io_mngr_get_iosel_reg_ofs(pin_id);
  /*GPIO61 & GPIO62 has no pull*/
  if (MCU_PIN_IS_VIRTUAL_PIN(pin_id)) {
    return GPIO_PULL_NONE;
  }
  /*GPIO0 ... PB*/
  if (MCU_PIN_IS_GPM_DOMAIN(pin_id)) {
    reg = REGISTER(TOPREG(BASE_ADDRESS_GPM_IO_CFG) + iosel_reg_ofs);
  }

  /*FLASH0_CS_N0 ... FLASH0_CS_N2*/
  if (MCU_PIN_IS_PMP_DOMAIN(pin_id)) {
    reg = REGISTER(TOPREG(BASE_ADDRESS_PMP_IO_CFG) + iosel_reg_ofs);
  }

  if (reg & GPM_IO_CFG_IO_CFG_GPIO0_IO_CFG_PE_GPIO0_MSK) {
    if (reg & GPM_IO_CFG_IO_CFG_GPIO0_IO_CFG_PS_GPIO0_MSK) {
      return GPIO_PULL_UP;
    } else {
      return GPIO_PULL_DOWN;
    }
  } else {
    return GPIO_PULL_NONE;
  }
}

static void gpioctl_set_value(eMcuPinIds pin_id, int value) {
  if (value) {
    REGISTER(GPIO_DAT_PORT(pin_id)) |= (0x1 << GPIO_BIT(pin_id));
  } else {
    REGISTER(GPIO_DAT_PORT(pin_id)) &= ~(0x1 << GPIO_BIT(pin_id));
  }
}

static eGpioDir gpioctl_get_direction(eMcuPinIds pin_id) {
  eGpioDir dir;
  if (REGISTER(GPIO_DIR_PORT(pin_id)) & (0x1 << GPIO_BIT(pin_id))) {
    dir = GPIO_DIR_OUTPUT;
  } else {
    dir = GPIO_DIR_INPUT;
  }
  return dir;
}

static eGpioRet gpio_set_map(eMcuGpioIds gpio, eMcuPinIds pin_id) {
  eMcuGpioIds i;

  if (gpio < MCU_GPIO_ID_01 || gpio >= MCU_GPIO_ID_NUM) return GPIO_RET_INVALID_PORT;

  if (pin_id < MCU_PIN_ID_START || pin_id >= MCU_PIN_ID_NUM) return GPIO_RET_OUT_OF_RANGE;

  for (i = MCU_GPIO_ID_01; i < MCU_GPIO_ID_NUM; i++) {
    if (mcu_gpio_table[i].phys_pin_id == pin_id) return GPIO_RET_ERR;
  }

  mcu_gpio_table[gpio].phys_pin_id = pin_id;
  phys_pin_map[pin_id] = gpio;

  return GPIO_RET_SUCCESS;
}

static eGpioRet gpio_clear_map(eMcuGpioIds gpio, eMcuPinIds pin_id) {
  if (gpio < MCU_GPIO_ID_01 || gpio >= MCU_GPIO_ID_NUM) return GPIO_RET_INVALID_PORT;

  mcu_gpio_table[gpio].phys_pin_id = MCU_PIN_ID_UNDEF;
  phys_pin_map[pin_id] = MCU_GPIO_ID_UNDEF;
  return GPIO_RET_SUCCESS;
}

eGpioRet gpio_init() {
  eIfMngrIf gpio_if;
  gpio_cfg_t gpio_config;
  gpio_cfg_t gpio_orig_cfg[MCU_GPIO_ID_NUM];
  eMcuGpioIds gpio_id;
  eMcuPinIds pin_id;

  if (g_gpio_initialized) return GPIO_RET_SUCCESS;

  memset(mcu_gpio_table, 0x0, sizeof(mcuGpioCfg_t) * MCU_GPIO_ID_NUM);
  memset(gpio_orig_cfg, 0x0, sizeof(gpio_cfg_t) * MCU_GPIO_ID_NUM);

  for (gpio_id = MCU_GPIO_ID_01; gpio_id < MCU_GPIO_ID_NUM; gpio_id++) {
    mcu_gpio_table[gpio_id].phys_pin_id = MCU_PIN_ID_UNDEF;
    gpio_orig_cfg[gpio_id].pin_set = MCU_PIN_ID_UNDEF;
  }
  memset(phys_pin_map, MCU_GPIO_ID_UNDEF, sizeof(eMcuGpioIds) * MCU_PIN_ID_NUM);

  if (clk_gating_source_enable(CLK_GATING_GPIO_IF) != 0) {
    g_gpio_initialized = 0;
    return GPIO_RET_ERR;
  }

  /*Configure static allocated GPIO defined in interface_config_alt1250.h*/
  for (gpio_if = IF_MNGR_GPIO01; gpio_if <= IF_MNGR_LAST_GPIO; gpio_if++) {
    if (if_mngr_load_defconfig(gpio_if, &gpio_config) == IF_MNGR_SUCCESS) {
      gpio_id = (eMcuGpioIds)(MCU_GPIO_ID_01 + (gpio_if - IF_MNGR_GPIO01));
      /*In case number of GPIO defined is less than the definition which is 10 in interface
       * manager.*/
      if (gpio_id >= MCU_GPIO_ID_NUM) break;

      pin_id = gpio_config.pin_set;

      if (io_mngr_iopar_validate(pin_id) != IOMNGR_SUCCESS)
    	  goto gpio_init_err;

      if (gpio_set_map(gpio_id, pin_id) != GPIO_RET_SUCCESS)
    	  goto gpio_init_err;

      /*Backup the previous setting for error handling*/
      mcu_gpio_table[gpio_id].orig_mux = io_mngr_iosel_get(pin_id);
      gpio_orig_cfg[gpio_id].pin_set = pin_id;
      gpio_orig_cfg[gpio_id].def_dir = gpioctl_get_direction(pin_id);
      gpio_orig_cfg[gpio_id].def_val = gpioctl_get_value(pin_id);
      gpio_orig_cfg[gpio_id].def_pull = gpioctl_get_pull(pin_id);

      if (if_mngr_config_io(gpio_if) != IF_MNGR_SUCCESS)
    	  goto gpio_init_err;

      /*Configure the gpio settings*/
      gpioctl_set_direction(pin_id, gpio_config.def_dir);
      if (gpio_config.def_dir == GPIO_DIR_OUTPUT) gpioctl_set_value(pin_id, gpio_config.def_val);
      gpioctl_set_pull(pin_id, gpio_config.def_pull);
    }
  }

#if defined(ALT125X_GPIO_INTERRUPTS_ENABLED)
  gpio_interrupts_init();
#endif

  g_gpio_initialized = 1;

  return GPIO_RET_SUCCESS;

gpio_init_err:
  /*Reset GPIO registers to value if it was previous configured.*/
  for (gpio_id = MCU_GPIO_ID_01; gpio_id < MCU_GPIO_ID_NUM; gpio_id++) {
    pin_id = gpio_orig_cfg[gpio_id].pin_set;

    if (pin_id != MCU_PIN_ID_UNDEF) {
      gpioctl_set_direction(pin_id, gpio_orig_cfg[gpio_id].def_dir);

      gpioctl_set_value(pin_id, gpio_orig_cfg[gpio_id].def_val);

      gpioctl_set_pull(pin_id, gpio_orig_cfg[gpio_id].def_pull);
      /*No need to check return value here since we intends to retore the mux setting here for
       * gpio_init error handling and the orig_mux was get from iosel register before*/
      (void)io_mngr_iosel_set(pin_id, mcu_gpio_table[gpio_id].orig_mux);
    }
  }

  memset(mcu_gpio_table, 0x0, sizeof(mcuGpioCfg_t) * MCU_GPIO_ID_NUM);
  for (gpio_id = MCU_GPIO_ID_01; gpio_id < MCU_GPIO_ID_NUM; gpio_id++)
    mcu_gpio_table[gpio_id].phys_pin_id = MCU_PIN_ID_UNDEF;

  memset(phys_pin_map, MCU_GPIO_ID_UNDEF, sizeof(eMcuGpioIds) * MCU_PIN_ID_NUM);

  g_gpio_initialized = 0;

  return GPIO_RET_ERR;
}

eGpioRet gpio_get_map(eMcuGpioIds gpio, eMcuPinIds *pin_id) {
  GPIO_INIT_CHECK;
  GPIO_CHECK_PIN_NUMBER(gpio);
  *pin_id = mcu_gpio_table[gpio].phys_pin_id;
  return GPIO_RET_SUCCESS;
}

eGpioRet gpio_get_pull(eMcuGpioIds gpio, eGpioPullMode *pull) {
  eMcuPinIds pin_id;

  GPIO_INIT_CHECK;
  GPIO_CHECK_PIN_NUMBER(gpio);  // first pin is 1

  pin_id = mcu_gpio_table[gpio].phys_pin_id;
  GPIO_IOPAR_VALIDATE(pin_id);

  *pull = gpioctl_get_pull(pin_id);

  return GPIO_RET_SUCCESS;
}

eGpioRet gpio_set_pull(eMcuGpioIds gpio, eGpioPullMode pullmode) {
  eMcuPinIds pin_id;

  GPIO_INIT_CHECK;
  GPIO_CHECK_PIN_NUMBER(gpio);

  pin_id = mcu_gpio_table[gpio].phys_pin_id;
  GPIO_IOPAR_VALIDATE(pin_id);

  gpioctl_set_pull(pin_id, pullmode);

  return GPIO_RET_SUCCESS;
}

eGpioRet gpio_get_value(eMcuGpioIds gpio, int *value) {
  eMcuPinIds pin_id;

  GPIO_INIT_CHECK;
  GPIO_CHECK_PIN_NUMBER(gpio);  // first pin is 1
  pin_id = mcu_gpio_table[gpio].phys_pin_id;

  if (io_mngr_iosel_get(pin_id) == IO_FUNC_GPIO) {
    *value = gpioctl_get_value(pin_id);
    return GPIO_RET_SUCCESS;
  } else {
    return GPIO_RET_ERR;
  }
}

eGpioRet gpio_set_value(eMcuGpioIds gpio, int value) {
  eMcuPinIds pin_id;

  GPIO_INIT_CHECK;
  GPIO_CHECK_PIN_NUMBER(gpio);  // first pin is 1
  pin_id = mcu_gpio_table[gpio].phys_pin_id;

  if (io_mngr_iosel_get(pin_id) == IO_FUNC_GPIO) {
    gpioctl_set_value(pin_id, value);
    return GPIO_RET_SUCCESS;
  }

  return GPIO_RET_ERR;
}

eGpioRet gpio_get_direction(eMcuGpioIds gpio, eGpioDir *dir) {
  //	int reg=0;
  eMcuPinIds pin_id;

  GPIO_INIT_CHECK;
  GPIO_CHECK_PIN_NUMBER(gpio);  // first pin is 1
  pin_id = mcu_gpio_table[gpio].phys_pin_id;

  // check if output
  *dir = gpioctl_get_direction(pin_id);

  return GPIO_RET_SUCCESS;
}

eGpioRet gpio_set_direction_input(eMcuGpioIds gpio) {
  eMcuPinIds pin_id;

  GPIO_INIT_CHECK;
  GPIO_CHECK_PIN_NUMBER(gpio);  // first pin is 1
  pin_id = mcu_gpio_table[gpio].phys_pin_id;

  if (io_mngr_iosel_get(pin_id) == IO_FUNC_GPIO) {
    gpioctl_set_direction(pin_id, GPIO_DIR_INPUT);
    return GPIO_RET_SUCCESS;
  }
  return GPIO_RET_ERR;
}

eGpioRet gpio_set_direction_output(eMcuGpioIds gpio) {
  eMcuPinIds pin_id;

  GPIO_INIT_CHECK;
  GPIO_CHECK_PIN_NUMBER(gpio);  // first pin is 1
  pin_id = mcu_gpio_table[gpio].phys_pin_id;

  if (io_mngr_iosel_get(pin_id) == IO_FUNC_GPIO) {
    gpioctl_set_direction(pin_id, GPIO_DIR_OUTPUT);
    return GPIO_RET_SUCCESS;
  }
  return GPIO_RET_ERR;
}

eGpioRet gpio_set_direction_output_and_value(eMcuGpioIds gpio, int value) {
  if (gpio_set_direction_output(gpio) == GPIO_RET_ERR) {
    return GPIO_RET_ERR;
  }
  return gpio_set_value(gpio, value);
}

#if defined(ALT125X_GPIO_INTERRUPTS_ENABLED)

static void gpio_irq_interrupt_handler(int gpio_block);

void gpio_0_handler(void) { gpio_irq_interrupt_handler(0); }

void gpio_1_handler(void) { gpio_irq_interrupt_handler(1); }

void gpio_2_handler(void) { gpio_irq_interrupt_handler(2); }

void gpio_3_handler(void) { gpio_irq_interrupt_handler(3); }

void gpio_4_handler(void) { gpio_irq_interrupt_handler(4); }

void gpio_5_handler(void) { gpio_irq_interrupt_handler(5); }

void gpio_6_handler(void) { gpio_irq_interrupt_handler(6); }

void gpio_7_handler(void) { gpio_irq_interrupt_handler(7); }

void gpio_8_handler(void) { gpio_irq_interrupt_handler(8); }

void gpio_9_handler(void) { gpio_irq_interrupt_handler(9); }

static void gpio_irq_interrupt_handler(int gpio_block) {
  int stat, t, c;
  eMcuPinIds pin_id;
  eMcuGpioIds gpio;

  //  printf("GPIO IRQ %d happened!\n\r",gpio_block);

  stat = REGISTER(MCU_GPIO_0_INT_STATUS + gpio_block * GPIO_BLOCK_SIZE);

  if (stat != 0)  // Should ALWAYS be TRUE
  {
    t = stat;
    c = 0;
    while (t != 0) {
      if (t & 0x01) {
        pin_id = (eMcuPinIds)(gpio_block * GPIO_PINS_PER_BLOCK + c + 1);  // one based
        gpio = phys_pin_map[pin_id];
        //      printf("GPIO int pin=%d\n\r", gpio);//one based display
        if (gpio >= MCU_GPIO_ID_01 && gpio < MCU_GPIO_ID_NUM && mcu_gpio_table[gpio].irq_enabled &&
            mcu_gpio_table[gpio].irq_handler) {
          mcu_gpio_table[gpio].irq_handler(mcu_gpio_table[gpio].user_param);
          break;
        }
      }
      t >>= 1;
      c++;
    }
    REGISTER(MCU_GPIO_0_INT_CLR + gpio_block * GPIO_BLOCK_SIZE) = stat;  // clear all port
                                                                         // interrupts
  } else {
    printf("Error - Ileagal GPIO interrupt\n\r");
  }
}

static void gpio_interrupts_init() {
  if (g_gpio_irq_initialized) return;

  size_t i;

  // disable all interrupts and clear any previous pending interrupts (specially for stateless boot)
  for (i = 0; i < GPIO_NUM_BLOCKS; i++) {
    REGISTER(MCU_GPIO_0_INT_MASK + i * GPIO_BLOCK_SIZE) = 0;
    REGISTER(MCU_GPIO_0_INT_CLR + i * GPIO_BLOCK_SIZE) = 0xffffffff;
  }
  // clear database
  for (i = MCU_GPIO_ID_01; i < MCU_GPIO_ID_NUM; i++) {
    mcu_gpio_table[i].irq_enabled = 0;
    mcu_gpio_table[i].irq_handler = NULL;
    mcu_gpio_table[i].user_param = 0;
  }

  // Enable interrupts
  for (i = GPIO0_IRQn; i <= GPIO9_IRQn; i++) {
    NVIC_SetPriority((IRQn_Type)i, 7); /* set Interrupt priority */
    NVIC_EnableIRQ((IRQn_Type)i);
  }

  g_gpio_irq_initialized = 1;
}

eGpioRet gpio_disable_interrupt(eMcuGpioIds gpio) {
  eMcuPinIds pin_id;

  GPIO_INIT_CHECK;
  GPIO_CHECK_PIN_NUMBER(gpio);  // first pin is 1
  pin_id = mcu_gpio_table[gpio].phys_pin_id;

  if (gpio_wakeup_find_io(pin_id) < 0) {
    if (gpioctl_disable_interrupt(pin_id) != GPIO_RET_SUCCESS) return GPIO_RET_ERR;

    mcu_gpio_table[gpio].irq_enabled = 0;
  } else {
    gpio_wakeup_interrupt_disable(pin_id);
  }
  return GPIO_RET_SUCCESS;
}

eGpioRet gpio_enable_interrupt(eMcuGpioIds gpio) {
  eMcuPinIds pin_id;

  GPIO_INIT_CHECK;
  GPIO_CHECK_PIN_NUMBER(gpio);  // first pin is 1
  pin_id = mcu_gpio_table[gpio].phys_pin_id;

  if (gpio_wakeup_find_io(pin_id) < 0) {
    mcu_gpio_table[gpio].irq_enabled = 1;
    gpioctl_enable_interrupt(pin_id);

    return GPIO_RET_SUCCESS;
  } else {
    gpio_wakeup_interrupt_enable(pin_id);
    return GPIO_RET_SUCCESS;
  }
}

eGpioRet gpio_clear_interrupt(eMcuGpioIds gpio) {
  eMcuPinIds pin_id;

  GPIO_INIT_CHECK;
  GPIO_CHECK_PIN_NUMBER(gpio);  // first pin is 1
  pin_id = mcu_gpio_table[gpio].phys_pin_id;

  gpioctl_clear_interrupt(pin_id);

  return GPIO_RET_SUCCESS;
}

eGpioRet gpio_register_interrupt(eMcuGpioIds gpio, gpio_interrupt_handler irq_handler,
                                 int user_param) {
  eMcuPinIds pin_id;

  GPIO_INIT_CHECK;
  GPIO_CHECK_PIN_NUMBER(gpio);  // first pin is 1
  pin_id = mcu_gpio_table[gpio].phys_pin_id;
  if (gpio_wakeup_find_io(pin_id) < 0) {
    mcu_gpio_table[gpio].irq_enabled = 0;
    mcu_gpio_table[gpio].irq_handler = irq_handler;
    mcu_gpio_table[gpio].user_param = user_param;

    return GPIO_RET_SUCCESS;
  } else {
    gpio_wakeup_register_interrupt(pin_id, irq_handler, user_param);
    return GPIO_RET_SUCCESS;
  }
}

eGpioRet gpio_config_interrupt(eMcuGpioIds gpio, eGpioIrqMode irq_mode, eGpioPullMode pullmode) {
  eMcuPinIds pin_id;

  GPIO_INIT_CHECK;
  GPIO_CHECK_PIN_NUMBER(gpio);  // first pin is 1
  pin_id = mcu_gpio_table[gpio].phys_pin_id;
  if (gpio_wakeup_find_io(pin_id) < 0) {
    gpioctl_config_interrupt(pin_id, irq_mode, pullmode);
    return GPIO_RET_SUCCESS;
  } else {
    switch (irq_mode) {
      case GPIO_IRQ_RISING_EDGE:
        gpio_wakeup_polarity_set((unsigned int)pin_id, GPIO_WAKEUP_EDGE_RISING);
        break;
      case GPIO_IRQ_FALLING_EDGE:
        gpio_wakeup_polarity_set((unsigned int)pin_id, GPIO_WAKEUP_EDGE_FALLING);
        break;
      case GPIO_IRQ_BOTH_EDGES:
        gpio_wakeup_polarity_set((unsigned int)pin_id, GPIO_WAKEUP_EDGE_RISING_FALLING);
        break;
      case GPIO_IRQ_HIGH_LEVEL:
        gpio_wakeup_polarity_set((unsigned int)pin_id, GPIO_WAKEUP_LEVEL_POL_HIGH);
        break;
      case GPIO_IRQ_LOW_LEVEL:
        gpio_wakeup_polarity_set((unsigned int)pin_id, GPIO_WAKEUP_LEVEL_POL_LOW);
        break;
      default:
        printf("Error - invalid gpio irq mode (%d, %d)\n\r", pin_id, irq_mode);
        return GPIO_RET_ERR;
        break;
    }
    return GPIO_RET_SUCCESS;
  }
}
#endif /*ALT125X_GPIO_INTERRUPTS_ENABLED*/

eGpioRet gpio_query_pin_status(eMcuPinIds pin_id, eGpioPullMode pullmode, int *value) {
  eIoMngrFunc orig_func;

  GPIO_INIT_CHECK;
  if (pin_id < MCU_PIN_ID_START || pin_id >= MCU_PIN_ID_NUM) return GPIO_RET_OUT_OF_RANGE;

  if (io_mngr_iopar_validate(pin_id) != IOMNGR_SUCCESS) return GPIO_RET_INVALID_PORT;

  orig_func = io_mngr_iosel_get(pin_id);

  if (io_mngr_iosel_set(pin_id, IO_FUNC_GPIO) != IOMNGR_SUCCESS) return GPIO_RET_ERR;

  gpioctl_set_direction(pin_id, GPIO_DIR_INPUT);
  gpioctl_set_pull(pin_id, pullmode);
  *value = gpioctl_get_value(pin_id);

  if (io_mngr_iosel_set(pin_id, orig_func) != IOMNGR_SUCCESS) return GPIO_RET_ERR;

  return GPIO_RET_SUCCESS;
}

eGpioRet gpio_open(eMcuPinIds pin_id, eMcuGpioIds *gpio_id) {
  eGpioRet ret;
  eMcuGpioIds free_id = MCU_GPIO_ID_UNDEF;

  GPIO_INIT_CHECK;
  if (pin_id < MCU_PIN_ID_START || pin_id >= MCU_PIN_ID_NUM) return GPIO_RET_OUT_OF_RANGE;

  if (io_mngr_iopar_validate(pin_id) != IOMNGR_SUCCESS) return GPIO_RET_INVALID_PORT;

  for (eMcuGpioIds i = MCU_GPIO_ID_01; i < MCU_GPIO_ID_NUM; i++) {
    /*The requested pin_id has already been configured*/
    if (mcu_gpio_table[i].phys_pin_id == pin_id) {
      *gpio_id = i;
      return GPIO_RET_SUCCESS;
    } else if (mcu_gpio_table[i].phys_pin_id == MCU_PIN_ID_UNDEF && free_id == MCU_GPIO_ID_UNDEF) {
      free_id = i;
    }
  }

  if (free_id != MCU_GPIO_ID_UNDEF) {
    mcu_gpio_table[free_id].orig_mux = io_mngr_iosel_get(pin_id);
    if (io_mngr_iosel_set(pin_id, IO_FUNC_GPIO) == IOMNGR_SUCCESS) {
      gpio_set_map(free_id, pin_id);
      *gpio_id = free_id;
#if defined(ALT125X_GPIO_INTERRUPTS_ENABLED)
      mcu_gpio_table[free_id].irq_enabled = 0;
      mcu_gpio_table[free_id].irq_handler = NULL;
      mcu_gpio_table[free_id].user_param = 0;
#endif
      ret = GPIO_RET_SUCCESS;
    } else {
      ret = GPIO_RET_ERR;
    }
  } else {
    ret = GPIO_RET_NO_RESOURCE;
  }
  return ret;
}

eGpioRet gpio_close(eMcuGpioIds gpio) {
  eMcuPinIds pin_id;

  GPIO_INIT_CHECK;
  GPIO_CHECK_PIN_NUMBER(gpio);
  pin_id = mcu_gpio_table[gpio].phys_pin_id;

#if defined(ALT125X_GPIO_INTERRUPTS_ENABLED)
  if (mcu_gpio_table[gpio].irq_enabled) gpio_disable_interrupt(gpio);
  mcu_gpio_table[gpio].irq_handler = NULL;
  mcu_gpio_table[gpio].user_param = 0;
#endif
  if (io_mngr_iosel_set(pin_id, mcu_gpio_table[gpio].orig_mux) == IOMNGR_SUCCESS) {
    gpio_clear_map(gpio, pin_id);
    return GPIO_RET_SUCCESS;
  } else {
    return GPIO_RET_ERR;
  }
}
