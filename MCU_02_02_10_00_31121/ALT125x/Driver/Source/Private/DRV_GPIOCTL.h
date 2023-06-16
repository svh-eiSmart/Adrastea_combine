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
#ifndef DRV_GPIOCTL_H_
#define DRV_GPIOCTL_H_

#include DEVICE_HEADER
#include "PIN.h"

#include <stdint.h>

#define GPIO_NUM_BLOCKS (10)

#define GPIO_PINS_PER_BLOCK (8)

#define GPIO_BIT(x) (((x)-1) % GPIO_PINS_PER_BLOCK)

#define GPIO_BANK_SIZE (GPIO_NUM_BLOCKS * GPIO_PINS_PER_BLOCK)

#define GPIO_BLOCK_SIZE (GPIO_1_BASE - GPIO_0_BASE)
#define GPIO_BLOCK_BASE(x) ((GPIO_Type *)(GPIO_0_BASE + ((x)*GPIO_BLOCK_SIZE)))

#define GPIO_PIN(x) (((x) > GPIO_BANK_SIZE) ? ((x)-GPIO_BANK_SIZE) : (x))
#define PIN_TO_GPIO_BASE(x) \
  ((GPIO_Type *)(GPIO_0_BASE + (((GPIO_PIN(x) - 1) >> 3) * GPIO_BLOCK_SIZE)))

static inline void DRV_GPIOCTL_SetDirectionOut(MCU_PinId pin_id) {
  PIN_TO_GPIO_BASE(pin_id)->DIR_SET = (0x1 << GPIO_BIT(pin_id));
}

static inline void DRV_GPIOCTL_SetDirectionIn(MCU_PinId pin_id) {
  PIN_TO_GPIO_BASE(pin_id)->DIR_CLR = (0x1 << GPIO_BIT(pin_id));
}

static inline void DRV_GPIOCTL_SetDirection(MCU_PinId pin_id, uint8_t dir) {
  if (dir)
    PIN_TO_GPIO_BASE(pin_id)->DIR_SET = (0x1 << GPIO_BIT(pin_id));
  else
    PIN_TO_GPIO_BASE(pin_id)->DIR_CLR = (0x1 << GPIO_BIT(pin_id));
}

static inline uint8_t DRV_GPIOCTL_GetDirection(MCU_PinId pin_id) {
  return ((PIN_TO_GPIO_BASE(pin_id)->DIR & (0x1 << GPIO_BIT(pin_id))) != 0);
}

static inline void DRV_GPIOCTL_DisableIrq(MCU_PinId pin_id) {
  PIN_TO_GPIO_BASE(pin_id)->INT_MASK_CLR = (0x01 << GPIO_BIT(pin_id));
}

static inline void DRV_GPIOCTL_EnableIrq(MCU_PinId pin_id) {
  PIN_TO_GPIO_BASE(pin_id)->INT_MASK_SET = (0x01 << GPIO_BIT(pin_id));
}

static inline void DRV_GPIOCTL_ClearIrq(MCU_PinId pin_id) {
  PIN_TO_GPIO_BASE(pin_id)->INT_CLR = (0x01 << GPIO_BIT(pin_id));
}

static inline void DRV_GPIOCTL_SetValue(MCU_PinId pin_id, uint8_t value) {
  if (value)
    PIN_TO_GPIO_BASE(pin_id)->DATA |= (0x1 << GPIO_BIT(pin_id));
  else
    PIN_TO_GPIO_BASE(pin_id)->DATA &= ~(0x1 << GPIO_BIT(pin_id));
}

static inline uint8_t DRV_GPIOCTL_GetValue(MCU_PinId pin_id) {
  return (((PIN_TO_GPIO_BASE(pin_id)->DATA) & (0x1 << GPIO_BIT(pin_id))) != 0);
}

static inline void DRV_GPIOCTL_IrqPolarityRisingEdge(MCU_PinId pin_id) {
  PIN_TO_GPIO_BASE(pin_id)->INT_SENSE_CLR = (0x1 << GPIO_BIT(pin_id));
  PIN_TO_GPIO_BASE(pin_id)->INT_BOTH_EDGES_CLR = (0x1 << GPIO_BIT(pin_id));
  PIN_TO_GPIO_BASE(pin_id)->INT_EVENT_SET = (0x1 << GPIO_BIT(pin_id));
}

static inline void DRV_GPIOCTL_IrqPolarityFallingEdge(MCU_PinId pin_id) {
  PIN_TO_GPIO_BASE(pin_id)->INT_SENSE_CLR = (0x1 << GPIO_BIT(pin_id));
  PIN_TO_GPIO_BASE(pin_id)->INT_BOTH_EDGES_CLR = (0x1 << GPIO_BIT(pin_id));
  PIN_TO_GPIO_BASE(pin_id)->INT_EVENT_CLR = (0x1 << GPIO_BIT(pin_id));
}

static inline void DRV_GPIOCTL_IrqPolarityBothEdge(MCU_PinId pin_id) {
  PIN_TO_GPIO_BASE(pin_id)->INT_SENSE_CLR = (0x1 << GPIO_BIT(pin_id));
  PIN_TO_GPIO_BASE(pin_id)->INT_BOTH_EDGES_SET = (0x1 << GPIO_BIT(pin_id));
  PIN_TO_GPIO_BASE(pin_id)->INT_EVENT_CLR = (0x1 << GPIO_BIT(pin_id));
}

static inline void DRV_GPIOCTL_IrqPolarityHighLevel(MCU_PinId pin_id) {
  PIN_TO_GPIO_BASE(pin_id)->INT_SENSE_SET = (0x1 << GPIO_BIT(pin_id));
  PIN_TO_GPIO_BASE(pin_id)->INT_BOTH_EDGES_CLR = (0x1 << GPIO_BIT(pin_id));
  PIN_TO_GPIO_BASE(pin_id)->INT_EVENT_SET = (0x1 << GPIO_BIT(pin_id));
}

static inline void DRV_GPIOCTL_IrqPolarityLowLevel(MCU_PinId pin_id) {
  PIN_TO_GPIO_BASE(pin_id)->INT_SENSE_SET = (0x1 << GPIO_BIT(pin_id));
  PIN_TO_GPIO_BASE(pin_id)->INT_BOTH_EDGES_CLR = (0x1 << GPIO_BIT(pin_id));
  PIN_TO_GPIO_BASE(pin_id)->INT_EVENT_CLR = (0x1 << GPIO_BIT(pin_id));
}
#endif
