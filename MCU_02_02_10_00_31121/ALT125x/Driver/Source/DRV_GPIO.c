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
#include <stdio.h>
#include <string.h>
#include "DRV_GPIO.h"
#include "DRV_GPIO_WAKEUP.h"
#include "DRV_CLOCK_GATING.h"
#include "DRV_IF_CFG.h"
#include "DRV_IO.h"
#include "PIN.h"
#include "DRV_GPIOCTL.h"
#if (configUSE_ALT_SLEEP == 1)
#include "DRV_PM.h"
#include "DRV_SLEEP.h"
#endif

#define GPIO_MUX_VALUE (4)

#define GPIO_INIT_CHECK         \
  if (!g_gpio_initialized) {    \
    return DRV_GPIO_ERROR_INIT; \
  }

#define GPIO_POWER_CHECK                     \
  if (g_gpio_pwr_state != GPIO_POWER_FULL) { \
    return DRV_GPIO_ERROR_POWER;             \
  }

#define GPIO_IOPAR_VALIDATE(p)                                 \
  if (DRV_IO_ValidatePartition(p) == DRV_IO_ERROR_PARTITION) { \
    return DRV_GPIO_ERROR_FORBIDDEN;                           \
  }

#define GPIO_CHECK_PIN_NUMBER(p)                          \
  if (((p) < MCU_GPIO_ID_01 || (p) >= MCU_GPIO_ID_MAX) || \
      (gpio_peripheral[p].pin_set < MCU_PIN_ID_START ||   \
       gpio_peripheral[p].pin_set >= MCU_PIN_ID_NUM)) {   \
    return DRV_GPIO_ERROR_INVALID_PORT;                   \
  }

static int g_gpio_irq_initialized = 0;
static int g_gpio_initialized = 0;
static GPIO_PowerState g_gpio_pwr_state = GPIO_POWER_OFF;

#if (configUSE_ALT_SLEEP == 1)
typedef struct {
#if (GPIO_PINS_PER_BLOCK == 8)
  uint8_t data;
  uint8_t dir;
  uint8_t int_sense;
  uint8_t int_event;
  uint8_t int_both_edges;
  uint8_t int_mask;
  uint8_t disable;
#else
  uint32_t data;
  uint32_t dir;
  uint32_t int_sense;
  uint32_t int_event;
  uint32_t int_both_edges;
  uint32_t int_mask;
  uint32_t disable;
#endif
} GPIO_Sleep_Ctx;
GPIO_Sleep_Ctx g_gpio_sleep_ctx[GPIO_NUM_BLOCKS];
int32_t g_gpio_sleep_notify_inx = 0;
#endif
typedef struct {
  MCU_PinId pin_set;
  uint32_t orig_mux;
  uint32_t irq_enabled;
  DRV_GPIO_IrqCallback irq_handler;
  void *user_param;
} GPIO_Peripheral;

static GPIO_Peripheral gpio_peripheral[MCU_GPIO_ID_MAX];
static GPIO_Id phys_pin_map[MCU_PIN_ID_NUM];

static void DRV_GPIOCTL_IrqHandler(uint8_t gpio_block) {
  int stat, t, c;
  MCU_PinId pin_id;
  GPIO_Id gpio;
  stat = GPIO_BLOCK_BASE(gpio_block)->INT_STATUS;

  if (stat != 0) {
    t = stat;
    c = 0;
    while (t != 0) {
      if (t & 0x01) {
        pin_id = (MCU_PinId)(gpio_block * GPIO_PINS_PER_BLOCK + c + 1);
        gpio = phys_pin_map[pin_id];
        if (gpio >= MCU_GPIO_ID_01 && gpio < MCU_GPIO_ID_MAX && gpio_peripheral[gpio].irq_enabled &&
            gpio_peripheral[gpio].irq_handler) {
          gpio_peripheral[gpio].irq_handler(gpio_peripheral[gpio].user_param);
          break;
        }
      }
      t >>= 1;
      c++;
    }
    GPIO_BLOCK_BASE(gpio_block)->INT_CLR = stat;  // clear all port interrupts
  }
}

void gpio_0_handler(void) { DRV_GPIOCTL_IrqHandler(0); }

void gpio_1_handler(void) { DRV_GPIOCTL_IrqHandler(1); }

void gpio_2_handler(void) { DRV_GPIOCTL_IrqHandler(2); }

void gpio_3_handler(void) { DRV_GPIOCTL_IrqHandler(3); }

void gpio_4_handler(void) { DRV_GPIOCTL_IrqHandler(4); }

void gpio_5_handler(void) { DRV_GPIOCTL_IrqHandler(5); }

void gpio_6_handler(void) { DRV_GPIOCTL_IrqHandler(6); }

void gpio_7_handler(void) { DRV_GPIOCTL_IrqHandler(7); }

void gpio_8_handler(void) { DRV_GPIOCTL_IrqHandler(8); }

void gpio_9_handler(void) { DRV_GPIOCTL_IrqHandler(9); }

static void DRV_GPIOCTL_IrqInitialize() {
  if (g_gpio_irq_initialized) return;

  size_t i;

  // disable all interrupts and clear any previous pending interrupts (specially for stateless boot)
  for (i = 0; i < GPIO_NUM_BLOCKS; i++) {
    GPIO_BLOCK_BASE(i)->INT_MASK = 0;
    GPIO_BLOCK_BASE(i)->INT_CLR = 0xFFFFFFFF;
  }
  // clear database
  for (i = MCU_GPIO_ID_01; i < MCU_GPIO_ID_MAX; i++) {
    gpio_peripheral[i].irq_enabled = 0;
    gpio_peripheral[i].irq_handler = NULL;
    gpio_peripheral[i].user_param = 0;
  }

  g_gpio_irq_initialized = 1;
}

static GPIO_Pull DRV_GPIOCTL_GetPull(MCU_PinId pin_id) {
  IO_Pull pull_value;
  GPIO_Pull gpio_pull;

  if (DRV_IO_GetPull(pin_id, &pull_value) != DRV_IO_OK) return GPIO_PULL_NONE;

  switch (pull_value) {
    case IO_PULL_NONE:
      gpio_pull = GPIO_PULL_NONE;
      break;
    case IO_PULL_UP:
      gpio_pull = GPIO_PULL_UP;
      break;
    case IO_PULL_DOWN:
      gpio_pull = GPIO_PULL_DOWN;
      break;
    default:
      gpio_pull = GPIO_PULL_NONE;
      break;
  }
  return gpio_pull;
}

static DRV_GPIO_Status DRV_GPIOCTL_SetPull(MCU_PinId pin_id, GPIO_Pull pullmode) {
  IO_Pull pull_value;

  switch (pullmode) {
    case GPIO_PULL_NONE:
      pull_value = IO_PULL_NONE;
      break;
    case GPIO_PULL_UP:
      pull_value = IO_PULL_UP;
      break;
    case GPIO_PULL_DOWN:
      pull_value = IO_PULL_DOWN;
      break;
    case GPIO_PULL_DONT_CHANGE:
      return DRV_GPIO_OK;
      break;
    default:
      return DRV_GPIO_ERROR_PARAM;
      break;
  }

  if (DRV_IO_SetPull(pin_id, pull_value) == DRV_IO_OK)
    return DRV_GPIO_OK;
  else
    return DRV_GPIO_ERROR_GENERIC;
}

static DRV_GPIO_Status DRV_GPIO_SetPinset(GPIO_Id gpio, MCU_PinId pin_id) {
  GPIO_Id i;

  if (gpio < MCU_GPIO_ID_01 || gpio >= MCU_GPIO_ID_MAX) return DRV_GPIO_ERROR_INVALID_PORT;

  if (pin_id < MCU_PIN_ID_START || pin_id >= MCU_PIN_ID_NUM) return DRV_GPIO_ERROR_PARAM;

  for (i = MCU_GPIO_ID_01; i < MCU_GPIO_ID_MAX; i++) {
    if (gpio_peripheral[i].pin_set == pin_id) return DRV_GPIO_ERROR_GENERIC;
  }

  gpio_peripheral[gpio].pin_set = pin_id;
  phys_pin_map[pin_id] = gpio;

  return DRV_GPIO_OK;
}

static DRV_GPIO_Status DRV_GPIO_ClearPinset(GPIO_Id gpio, MCU_PinId pin_id) {
  if (gpio < MCU_GPIO_ID_01 || gpio >= MCU_GPIO_ID_MAX) return DRV_GPIO_ERROR_INVALID_PORT;

  gpio_peripheral[gpio].pin_set = MCU_PIN_ID_UNDEF;
  phys_pin_map[pin_id] = MCU_GPIO_ID_UNDEF;
  return DRV_GPIO_OK;
}

#if (configUSE_ALT_SLEEP == 1)
static int32_t DRV_GPIO_SleepHandler(DRV_SLEEP_NotifyType state, DRV_PM_PwrMode pwr_mode,
                                     void *arg) {
  uint8_t i = 0, j = 0;
  uint32_t gpio_data = 0;
  if (pwr_mode == DRV_PM_MODE_STANDBY || pwr_mode == DRV_PM_MODE_SHUTDOWN) {
    if (state == DRV_SLEEP_NTFY_SUSPENDING) {
      for (i = 0; i < GPIO_NUM_BLOCKS; i++) {
#if (GPIO_PINS_PER_BLOCK == 8)
        g_gpio_sleep_ctx[i].data = (uint8_t)GPIO_BLOCK_BASE(i)->DATA;
        g_gpio_sleep_ctx[i].dir = (uint8_t)GPIO_BLOCK_BASE(i)->DIR;
        g_gpio_sleep_ctx[i].int_sense = (uint8_t)GPIO_BLOCK_BASE(i)->INT_SENSE;
        g_gpio_sleep_ctx[i].int_event = (uint8_t)GPIO_BLOCK_BASE(i)->INT_EVENT;
        g_gpio_sleep_ctx[i].int_both_edges = (uint8_t)GPIO_BLOCK_BASE(i)->INT_EVENT;
        g_gpio_sleep_ctx[i].int_mask = (uint8_t)GPIO_BLOCK_BASE(i)->INT_MASK;
        g_gpio_sleep_ctx[i].disable = (uint8_t)GPIO_BLOCK_BASE(i)->DISABLE;
#else
        g_gpio_sleep_ctx[i].data = GPIO_BLOCK_BASE(i)->DATA;
        g_gpio_sleep_ctx[i].dir = GPIO_BLOCK_BASE(i)->DIR;
        g_gpio_sleep_ctx[i].int_sense = GPIO_BLOCK_BASE(i)->INT_SENSE;
        g_gpio_sleep_ctx[i].int_event = GPIO_BLOCK_BASE(i)->INT_EVENT;
        g_gpio_sleep_ctx[i].int_both_edges = GPIO_BLOCK_BASE(i)->INT_EVENT;
        g_gpio_sleep_ctx[i].int_mask = GPIO_BLOCK_BASE(i)->INT_MASK;
        g_gpio_sleep_ctx[i].disable = GPIO_BLOCK_BASE(i)->DISABLE;
#endif
      }
    } else if (state == DRV_SLEEP_NTFY_RESUMING) {
      for (i = 0; i < GPIO_NUM_BLOCKS; i++) {
        GPIO_BLOCK_BASE(i)->DIR = (uint32_t)g_gpio_sleep_ctx[i].dir;
        gpio_data = GPIO_BLOCK_BASE(i)->DATA;
        for (j = 0; j < GPIO_PINS_PER_BLOCK; j++) {
          if ((g_gpio_sleep_ctx[i].dir >> j) & 1) {
            if ((g_gpio_sleep_ctx[i].data >> j) & 1)
              gpio_data |= (1 << j);
            else
              gpio_data &= ~(1 << j);
          }
        }
        GPIO_BLOCK_BASE(i)->DATA = gpio_data;
        /*GPIO_BLOCK_BASE(i)->DATA = (uint32_t)g_gpio_sleep_ctx[i].data;*/
        GPIO_BLOCK_BASE(i)->INT_SENSE = (uint32_t)g_gpio_sleep_ctx[i].int_sense;
        GPIO_BLOCK_BASE(i)->INT_EVENT = (uint32_t)g_gpio_sleep_ctx[i].int_event;
        GPIO_BLOCK_BASE(i)->INT_BOTH_EDGES = (uint32_t)g_gpio_sleep_ctx[i].int_both_edges;
        GPIO_BLOCK_BASE(i)->INT_MASK = (uint32_t)g_gpio_sleep_ctx[i].int_mask;
        GPIO_BLOCK_BASE(i)->DISABLE = (uint32_t)g_gpio_sleep_ctx[i].disable;
      }
    }
  }
  return 0;
}
#endif

DRV_GPIO_Status DRV_GPIO_Initialize(void) {
  Interface_Id gpio_if;
  GPIO_Param gpio_config;
  GPIO_Param gpio_orig_cfg[MCU_GPIO_ID_MAX];
  GPIO_Id gpio_id;
  MCU_PinId pin_id;

  if (g_gpio_initialized) return DRV_GPIO_OK;

  memset(gpio_peripheral, 0x0, sizeof(GPIO_Peripheral) * MCU_GPIO_ID_MAX);
  memset(phys_pin_map, 0x0, sizeof(MCU_PinId) * MCU_PIN_ID_NUM);
  memset(gpio_orig_cfg, 0x0, sizeof(GPIO_Param) * MCU_GPIO_ID_MAX);

#if (configUSE_ALT_SLEEP == 1)
  if (DRV_SLEEP_RegNotification(DRV_GPIO_SleepHandler, &g_gpio_sleep_notify_inx, NULL) != 0)
    return DRV_GPIO_ERROR_GENERIC;
#endif

  if (DRV_CLKGATING_EnableClkSource(CLK_GATING_GPIO_IF) != 0) return DRV_GPIO_ERROR_GENERIC;

  /*Reset GPIO control block*/
  RST_CONTROL->REG_SET_b.RESET_GPIO = 1;
  RST_CONTROL->REG_CLR_b.RESET_GPIO = 1;

  for (gpio_id = MCU_GPIO_ID_01; gpio_id < MCU_GPIO_ID_MAX; gpio_id++) {
    gpio_peripheral[gpio_id].pin_set = MCU_PIN_ID_UNDEF;
    gpio_orig_cfg[gpio_id].pin_set = MCU_PIN_ID_UNDEF;
  }
  memset(phys_pin_map, MCU_GPIO_ID_UNDEF, sizeof(GPIO_Id) * MCU_PIN_ID_NUM);

  /*Configure static allocated GPIO defined in interfaces_config_alt12xx.h*/
  for (gpio_if = IF_CFG_GPIO01; gpio_if <= IF_CFG_LAST_GPIO; gpio_if++) {
    if (DRV_IF_CFG_GetDefConfig(gpio_if, &gpio_config) == DRV_IF_CFG_OK) {
      gpio_id = (GPIO_Id)(MCU_GPIO_ID_01 + (gpio_if - IF_CFG_GPIO01));
      /*In case number of GPIO defined is less than the definition in DRV_IF_CFG and interface
       * header*/
      if (gpio_id >= MCU_GPIO_ID_MAX) break;

      pin_id = gpio_config.pin_set;

      if (DRV_IO_ValidatePartition(pin_id) != DRV_IO_OK) goto gpio_init_err;

      if (DRV_GPIO_SetPinset(gpio_id, pin_id) != DRV_GPIO_OK) goto gpio_init_err;

      /*Backup the previous setting for error handling*/
      DRV_IO_GetMux(pin_id, &gpio_peripheral[gpio_id].orig_mux);
      gpio_orig_cfg[gpio_id].pin_set = pin_id;
      gpio_orig_cfg[gpio_id].direction =
          DRV_GPIOCTL_GetDirection(pin_id) ? GPIO_DIR_OUTPUT : GPIO_DIR_INPUT;
      gpio_orig_cfg[gpio_id].value = DRV_GPIOCTL_GetValue(pin_id);
      gpio_orig_cfg[gpio_id].pull = DRV_GPIOCTL_GetPull(pin_id);

      if (DRV_IF_CFG_SetIO(gpio_if) != DRV_IF_CFG_OK) goto gpio_init_err;

      /*Configure the gpio settings*/
      DRV_GPIOCTL_SetDirection(pin_id, gpio_config.direction == GPIO_DIR_OUTPUT);
      DRV_GPIOCTL_SetValue(pin_id, (uint8_t)gpio_config.value);
      DRV_GPIOCTL_SetPull(pin_id, gpio_config.pull);

      DRV_IO_SetInputEnable(pin_id, 1);
    }
  }

  DRV_GPIOCTL_IrqInitialize();

  g_gpio_initialized = 1;

  return DRV_GPIO_OK;

gpio_init_err:
  /*Reset GPIO registers to value if it was previous configured.*/
  for (gpio_id = MCU_GPIO_ID_01; gpio_id < MCU_GPIO_ID_MAX; gpio_id++) {
    pin_id = gpio_orig_cfg[gpio_id].pin_set;

    if (pin_id != MCU_PIN_ID_UNDEF) {
      DRV_GPIOCTL_SetDirection(pin_id, gpio_config.direction == GPIO_DIR_OUTPUT);

      DRV_GPIOCTL_SetValue(pin_id, gpio_orig_cfg[gpio_id].value);

      DRV_GPIOCTL_SetPull(pin_id, gpio_orig_cfg[gpio_id].pull);
      /*No need to check return value here since we intends to retore the mux setting here for
       * gpio_init error handling and the orig_mux was get from iosel register before*/
      (void)DRV_IO_SetMux(pin_id, gpio_peripheral[gpio_id].orig_mux);
    }
  }

  memset(gpio_peripheral, 0x0, sizeof(GPIO_Peripheral) * MCU_GPIO_ID_MAX);
  for (gpio_id = MCU_GPIO_ID_01; gpio_id < MCU_GPIO_ID_MAX; gpio_id++)
    gpio_peripheral[gpio_id].pin_set = MCU_PIN_ID_UNDEF;

  memset(phys_pin_map, MCU_GPIO_ID_UNDEF, sizeof(GPIO_Id) * MCU_PIN_ID_NUM);
  RST_CONTROL->REG_SET_b.RESET_GPIO = 1;
  RST_CONTROL->REG_CLR_b.RESET_GPIO = 1;
  (void)DRV_CLKGATING_DisableClkSource(CLK_GATING_GPIO_IF);
  g_gpio_initialized = 0;

  return DRV_GPIO_ERROR_GENERIC;
}

DRV_GPIO_Status DRV_GPIO_PowerControl(GPIO_PowerState state) {
  IRQn_Type i;

  GPIO_INIT_CHECK;
  DRV_GPIO_Status ret = DRV_GPIO_OK;
  switch (state) {
    case GPIO_POWER_FULL:
      if (DRV_CLKGATING_EnableClkSource(CLK_GATING_GPIO_IF) != 0)
        ret = DRV_GPIO_ERROR_GENERIC;
      else {
        // Enable interrupts
        for (i = GPIO0_IRQn; i <= GPIO9_IRQn; i++) {
          NVIC_SetPriority(i, 7); /* set Interrupt priority */
          NVIC_EnableIRQ(i);
        }
        g_gpio_pwr_state = state;
      }
      break;
    case GPIO_POWER_OFF:
      for (i = GPIO0_IRQn; i <= GPIO9_IRQn; i++) NVIC_DisableIRQ(i);
      if (DRV_CLKGATING_DisableClkSource(CLK_GATING_GPIO_IF) != 0)
        ret = DRV_GPIO_ERROR_GENERIC;
      else
        g_gpio_pwr_state = state;

      break;
    default:
      ret = DRV_GPIO_ERROR_PARAM;
      break;
  }
  return ret;
}

DRV_GPIO_Status DRV_GPIO_Uninitialize(void) {
  GPIO_INIT_CHECK;
#if (configUSE_ALT_SLEEP == 1)
  if (DRV_SLEEP_UnRegNotification(g_gpio_sleep_notify_inx) != 0) return DRV_GPIO_ERROR_GENERIC;
#endif
  memset(gpio_peripheral, 0x0, sizeof(GPIO_Peripheral) * MCU_GPIO_ID_MAX);
  memset(phys_pin_map, 0x0, sizeof(MCU_PinId) * MCU_PIN_ID_NUM);
  g_gpio_initialized = 0;
  return DRV_GPIO_OK;
}

DRV_GPIO_Status DRV_GPIO_GetValue(GPIO_Id gpio, uint8_t *value) {
  MCU_PinId pin_id;

  GPIO_INIT_CHECK;
  GPIO_POWER_CHECK;
  GPIO_CHECK_PIN_NUMBER(gpio);
  pin_id = gpio_peripheral[gpio].pin_set;

  *value = DRV_GPIOCTL_GetValue(pin_id);
  return DRV_GPIO_OK;
}

DRV_GPIO_Status DRV_GPIO_SetValue(GPIO_Id gpio, uint8_t value) {
  MCU_PinId pin_id;

  GPIO_INIT_CHECK;
  GPIO_POWER_CHECK;
  GPIO_CHECK_PIN_NUMBER(gpio);
  pin_id = gpio_peripheral[gpio].pin_set;

  DRV_GPIOCTL_SetValue(pin_id, value);
  return DRV_GPIO_OK;
}

DRV_GPIO_Status DRV_GPIO_SetPull(GPIO_Id gpio, GPIO_Pull pull) {
  MCU_PinId pin_id;

  GPIO_INIT_CHECK;
  GPIO_POWER_CHECK;
  GPIO_CHECK_PIN_NUMBER(gpio);

  pin_id = gpio_peripheral[gpio].pin_set;

  return DRV_GPIOCTL_SetPull(pin_id, pull);
}

DRV_GPIO_Status DRV_GPIO_GetPull(GPIO_Id gpio, GPIO_Pull *pull) {
  MCU_PinId pin_id;

  GPIO_INIT_CHECK;
  GPIO_POWER_CHECK;
  GPIO_CHECK_PIN_NUMBER(gpio);

  pin_id = gpio_peripheral[gpio].pin_set;

  *pull = DRV_GPIOCTL_GetPull(pin_id);

  return DRV_GPIO_OK;
}

DRV_GPIO_Status DRV_GPIO_SetDirection(GPIO_Id gpio, GPIO_Direction direction) {
  MCU_PinId pin_id;

  GPIO_INIT_CHECK;
  GPIO_POWER_CHECK;
  GPIO_CHECK_PIN_NUMBER(gpio);
  pin_id = gpio_peripheral[gpio].pin_set;

  DRV_GPIOCTL_SetDirection(pin_id, direction == GPIO_DIR_OUTPUT);
  return DRV_GPIO_OK;
}
DRV_GPIO_Status DRV_GPIO_SetDirectionOutAndValue(GPIO_Id gpio, uint8_t value) {
  MCU_PinId pin_id;
  GPIO_INIT_CHECK;
  GPIO_POWER_CHECK;
  GPIO_CHECK_PIN_NUMBER(gpio);
  pin_id = gpio_peripheral[gpio].pin_set;

  DRV_GPIOCTL_SetDirection(pin_id, 1);
  DRV_GPIOCTL_SetValue(pin_id, value);
  return DRV_GPIO_OK;
}
DRV_GPIO_Status DRV_GPIO_GetDirection(GPIO_Id gpio, GPIO_Direction *direction) {
  MCU_PinId pin_id;

  GPIO_INIT_CHECK;
  GPIO_POWER_CHECK;
  GPIO_CHECK_PIN_NUMBER(gpio);
  pin_id = gpio_peripheral[gpio].pin_set;

  *direction = DRV_GPIOCTL_GetDirection(pin_id) ? GPIO_DIR_OUTPUT : GPIO_DIR_INPUT;

  return DRV_GPIO_OK;
}

DRV_GPIO_Status DRV_GPIO_ConfigIrq(GPIO_Id gpio, GPIO_IrqPolarity polarity, GPIO_Pull pull) {
  MCU_PinId pin_id;

  GPIO_INIT_CHECK;
  GPIO_POWER_CHECK;
  GPIO_CHECK_PIN_NUMBER(gpio);
  pin_id = gpio_peripheral[gpio].pin_set;
  if (DRV_GPIO_WAKEUP_FindIO(pin_id) < 0) {
    DRV_GPIOCTL_SetDirectionIn(pin_id);
    DRV_GPIOCTL_SetPull(pin_id, pull);
    switch (polarity) {
      case GPIO_IRQ_RISING_EDGE:
        DRV_GPIOCTL_IrqPolarityRisingEdge(pin_id);
        break;

      case GPIO_IRQ_FALLING_EDGE:
        DRV_GPIOCTL_IrqPolarityFallingEdge(pin_id);
        break;

      case GPIO_IRQ_BOTH_EDGES:
        DRV_GPIOCTL_IrqPolarityBothEdge(pin_id);
        break;

      case GPIO_IRQ_HIGH_LEVEL:
        DRV_GPIOCTL_IrqPolarityHighLevel(pin_id);
        break;

      case GPIO_IRQ_LOW_LEVEL:
        DRV_GPIOCTL_IrqPolarityLowLevel(pin_id);
        break;

      default:
        return DRV_GPIO_ERROR_PARAM;
        break;
    }
    DRV_GPIOCTL_ClearIrq(
        pin_id);  // clear interrupt in case it occurred during transitions states above
    return DRV_GPIO_OK;
  } else {
    switch (polarity) {
      case GPIO_IRQ_RISING_EDGE:
        DRV_GPIO_WAKEUP_SetPolarity((unsigned int)pin_id, GPIO_WAKEUP_EDGE_RISING);
        break;
      case GPIO_IRQ_FALLING_EDGE:
        DRV_GPIO_WAKEUP_SetPolarity((unsigned int)pin_id, GPIO_WAKEUP_EDGE_FALLING);
        break;
      case GPIO_IRQ_BOTH_EDGES:
        DRV_GPIO_WAKEUP_SetPolarity((unsigned int)pin_id, GPIO_WAKEUP_EDGE_RISING_FALLING);
        break;
      case GPIO_IRQ_HIGH_LEVEL:
        DRV_GPIO_WAKEUP_SetPolarity((unsigned int)pin_id, GPIO_WAKEUP_LEVEL_POL_HIGH);
        break;
      case GPIO_IRQ_LOW_LEVEL:
        DRV_GPIO_WAKEUP_SetPolarity((unsigned int)pin_id, GPIO_WAKEUP_LEVEL_POL_LOW);
        break;
      default:
        return DRV_GPIO_ERROR_PARAM;
        break;
    }
    return DRV_GPIO_OK;
  }
}

DRV_GPIO_Status DRV_GPIO_RegisterIrqCallback(GPIO_Id gpio, DRV_GPIO_IrqCallback irq_handler,
                                             void *user_param) {
  MCU_PinId pin_id;

  GPIO_INIT_CHECK;
  GPIO_POWER_CHECK;
  GPIO_CHECK_PIN_NUMBER(gpio);
  pin_id = gpio_peripheral[gpio].pin_set;
  if (DRV_GPIO_WAKEUP_FindIO(pin_id) < 0) {
    gpio_peripheral[gpio].irq_enabled = 0;
    gpio_peripheral[gpio].irq_handler = irq_handler;
    gpio_peripheral[gpio].user_param = user_param;

    return DRV_GPIO_OK;
  } else {
    DRV_GPIO_WAKEUP_RegisterInterrupt(pin_id, (DRV_GPIO_WAKEUP_IrqCallback)irq_handler, user_param);
    return DRV_GPIO_OK;
  }
}

DRV_GPIO_Status DRV_GPIO_EnableIrq(GPIO_Id gpio) {
  MCU_PinId pin_id;

  GPIO_INIT_CHECK;
  GPIO_POWER_CHECK;
  GPIO_CHECK_PIN_NUMBER(gpio);
  pin_id = gpio_peripheral[gpio].pin_set;

  if (DRV_GPIO_WAKEUP_FindIO(pin_id) < 0) {
    gpio_peripheral[gpio].irq_enabled = 1;
    DRV_GPIOCTL_EnableIrq(pin_id);
    return DRV_GPIO_OK;
  } else {
    DRV_GPIO_WAKEUP_EnableInterrupt(pin_id);
    return DRV_GPIO_OK;
  }
}

DRV_GPIO_Status DRV_GPIO_DisableIrq(GPIO_Id gpio) {
  MCU_PinId pin_id;

  GPIO_INIT_CHECK;
  GPIO_POWER_CHECK;
  GPIO_CHECK_PIN_NUMBER(gpio);
  pin_id = gpio_peripheral[gpio].pin_set;

  if (DRV_GPIO_WAKEUP_FindIO(pin_id) < 0) {
    DRV_GPIOCTL_DisableIrq(pin_id);
    gpio_peripheral[gpio].irq_enabled = 0;
  } else {
    DRV_GPIO_WAKEUP_DisableInterrupt(pin_id);
  }
  return DRV_GPIO_OK;
}

DRV_GPIO_Status DRV_GPIO_ClearIrq(GPIO_Id gpio) {
  MCU_PinId pin_id;

  GPIO_INIT_CHECK;
  GPIO_POWER_CHECK;
  GPIO_CHECK_PIN_NUMBER(gpio);
  pin_id = gpio_peripheral[gpio].pin_set;

  DRV_GPIOCTL_ClearIrq(pin_id);

  return DRV_GPIO_OK;
}

DRV_GPIO_Status DRV_GPIO_QueryPinValue(MCU_PinId pin_id, GPIO_Pull pull, uint8_t *value) {
  uint32_t orig_mux;

  GPIO_INIT_CHECK;
  GPIO_POWER_CHECK;
  GPIO_IOPAR_VALIDATE(pin_id);
  if (pin_id < MCU_PIN_ID_START || pin_id >= MCU_PIN_ID_NUM) return DRV_GPIO_ERROR_PARAM;

  if (DRV_IO_GetMux(pin_id, &orig_mux) != DRV_IO_OK) return DRV_GPIO_ERROR_GENERIC;

  if (DRV_IO_SetMux(pin_id, GPIO_MUX_VALUE) != DRV_IO_OK) return DRV_GPIO_ERROR_GENERIC;

  DRV_GPIOCTL_SetDirection(pin_id, 0);
  DRV_GPIOCTL_SetPull(pin_id, pull);
  *value = DRV_GPIOCTL_GetValue(pin_id);

  if (DRV_IO_SetMux(pin_id, orig_mux) != DRV_IO_OK) return DRV_GPIO_ERROR_GENERIC;

  return DRV_GPIO_OK;
}

DRV_GPIO_Status DRV_GPIO_Open(GPIO_Param *gpio_config, GPIO_Id *gpio) {
  DRV_GPIO_Status ret;
  MCU_PinId pin_id = gpio_config->pin_set;
  GPIO_Id free_id = MCU_GPIO_ID_UNDEF;

  GPIO_INIT_CHECK;
  GPIO_IOPAR_VALIDATE(pin_id);
  if (pin_id < MCU_PIN_ID_START || pin_id >= MCU_PIN_ID_NUM) return DRV_GPIO_ERROR_PARAM;

  for (GPIO_Id i = MCU_GPIO_ID_01; i < MCU_GPIO_ID_MAX; i++) {
    /*The requested pin_id has already been configured*/
    if (gpio_peripheral[i].pin_set == pin_id) {
      *gpio = i;
      return DRV_GPIO_OK;
    } else if (gpio_peripheral[i].pin_set == MCU_PIN_ID_UNDEF && free_id == MCU_GPIO_ID_UNDEF) {
      free_id = i;
    }
  }

  if (free_id != MCU_GPIO_ID_UNDEF) {
    (void)DRV_IO_GetMux(pin_id, &gpio_peripheral[free_id].orig_mux);
    if (DRV_IO_SetMux(pin_id, GPIO_MUX_VALUE) == DRV_IO_OK) {
      DRV_GPIO_SetPinset(free_id, pin_id);
      *gpio = free_id;
      gpio_peripheral[free_id].irq_enabled = 0;
      gpio_peripheral[free_id].irq_handler = NULL;
      gpio_peripheral[free_id].user_param = 0;
      DRV_GPIOCTL_SetDirection(pin_id, gpio_config->direction == GPIO_DIR_OUTPUT);
      DRV_GPIOCTL_SetValue(pin_id, gpio_config->value);
      DRV_GPIOCTL_SetPull(pin_id, gpio_config->pull);
      DRV_IO_SetInputEnable(pin_id, 1);
      ret = DRV_GPIO_OK;
    } else {
      ret = DRV_GPIO_ERROR_GENERIC;
    }
  } else {
    ret = DRV_GPIO_ERROR_RESOURCE;
  }
  return ret;
}

DRV_GPIO_Status DRV_GPIO_Close(GPIO_Id gpio) {
  MCU_PinId pin_id;

  GPIO_INIT_CHECK;
  GPIO_CHECK_PIN_NUMBER(gpio);
  pin_id = gpio_peripheral[gpio].pin_set;

  if (gpio_peripheral[gpio].irq_enabled) DRV_GPIO_DisableIrq(gpio);
  gpio_peripheral[gpio].irq_handler = NULL;
  gpio_peripheral[gpio].user_param = 0;
  if (DRV_IO_SetMux(pin_id, gpio_peripheral[gpio].orig_mux) == DRV_IO_OK) {
    DRV_GPIO_ClearPinset(gpio, pin_id);
    return DRV_GPIO_OK;
  } else {
    return DRV_GPIO_ERROR_GENERIC;
  }
}

DRV_GPIO_Status DRV_GPIO_GetPinset(GPIO_Id gpio, MCU_PinId *pin_id) {
  GPIO_INIT_CHECK;
  GPIO_CHECK_PIN_NUMBER(gpio);
  *pin_id = gpio_peripheral[gpio].pin_set;
  return DRV_GPIO_OK;
}
