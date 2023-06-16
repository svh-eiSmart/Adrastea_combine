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
/* Standard includes. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include DEVICE_HEADER

#include "DRV_COMMON.h"
#include "DRV_GPIO_WAKEUP.h"
#include "DRV_GPIO.h"
#if (configUSE_ALT_SLEEP == 1)
#include "DRV_PM.h"
#include "DRV_SLEEP.h"
#endif

/****************************************************************************
 * Definitions
 ****************************************************************************/
#define IO_WKUP_REG(regAddress) (*(volatile uint32_t*)((unsigned char*)regAddress))

/* Registers */
#define BASE_ADDRESS_IO_WKUP_CTRL (IO_WKUP_CTRL_BASE)
#define IO_WKUP_CTRL_CTRL0(x) (BASE_ADDRESS_IO_WKUP_CTRL + 0x000 + (x * 0x20))
#define IO_WKUP_CTRL_CTRL1(x) (BASE_ADDRESS_IO_WKUP_CTRL + 0x004 + (x * 0x20))
#define IO_WKUP_CTRL_WAKEUP_EN(x) (BASE_ADDRESS_IO_WKUP_CTRL + 0x008 + (x * 0x20))
#define IO_WKUP_CTRL_INT_EN(x) (BASE_ADDRESS_IO_WKUP_CTRL + 0x00C + (x * 0x20))
#define IO_WKUP_CTRL_INT(x) (BASE_ADDRESS_IO_WKUP_CTRL + 0x010 + (x * 0x20))

#define GPIO_WAKEUP_MODEM_SOURCE_SIZE 8  // quantity of wakeups available for MCU
#define GPIO_WAKEUP_DEBOUNCE_QUANTITY 4
#define GPIO_WAKEUP_CTRL_SIZE IO_WKUP_CTRL_IO_WAKEUP_1_CTRL0 - IO_WKUP_CTRL_IO_WAKEUP_0_CTRL0
#define GPIO_WAKEUP_MODE_EDGE_FALLING 1
#define GPIO_WAKEUP_MODE_EDGE_TOGGLE 2
#define GPIO_WAKEUP_FOURTH_BIT_FOR_MASKING 16
#define GPIO_WAKEUP_SOURCES_WITH_DEBOUNCE 4
#define GPIO_WAKEUP_SOURCE_WITHOUT_DEBOUNCE 6
#define GPIO_WAKEUP_MAX_GPIO_ID 63
#define GPIO_WAKEUP_FORBIDDEN_WAKEUP_PIN \
  255  // wakeup pin can't be larger than 57
       // so forbidden value defined as 255
#define GPIO_WAKEUP_OFFSET_IN_WAKEUP_VECTOR 4

/****************************************************************************
 * Private Data
 ****************************************************************************/
/* Lookup tables */
static uint8_t gpio_to_wakeup_lookup[] = {
    255, 0,  1,  2,  3,  4,  5,  255, 255, 255, 6,  7,   8,  9,  10, 11, 12, 13, 14, 15, 16,
    17,  18, 19, 20, 21, 22, 23, 24,  25,  26,  27, 255, 28, 29, 30, 31, 32, 33, 34, 35, 36,
    37,  38, 39, 40, 41, 42, 43, 44,  45,  46,  47, 48,  49, 50, 51, 52, 53, 54, 55, 56, 57};

static uint8_t wakeup_to_gpio_lookup[] = {
    1,  2,  3,  4,  5,  6,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
    24, 25, 26, 27, 28, 29, 30, 31, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44,
    45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62};

static uint8_t wakeup_info_lookup[GPIO_WAKEUP_MAX_GPIO_ID] = {254, 254};
static DRV_GPIO_WAKEUP_Configuration gpio_wakeup_info[GPIO_WAKEUP_MODEM_SOURCE_SIZE];
static uint32_t gpio_wakeup_irq_configured = 0;
static int32_t volatile reg_temp = 0;

#define WKUP_PROCESSORS_PMG_WAKEUP_MASK_MCU0 (0x0C110300UL + 0x30UL)
#define URCLK_CFG_URCLK_CFG (0x0C111FA0UL + 0x00UL)
#define URCLK_CFG_URCLK_CFG_RING_OSC_UR_EN_MSK (0x1UL)

static bool is_initialized = false;
/****************************************************************************
 * Function Prototype
 ****************************************************************************/
/* Select GPIO wakeup clock */
int32_t DRV_GPIO_WAKEUP_SelectClock(uint32_t pin_number, GPIO_WAKEUP_Clock clk_id);
/* Enable debounce */
int32_t DRV_GPIO_WAKEUP_EnableDebounce(uint32_t pin_number);
/* Disable debounce */
int32_t DRV_GPIO_WAKEUP_DisableDebounce(uint32_t pin_number);
/* Set debounce count value */
int32_t DRV_GPIO_WAKEUP_SetDebounceValue(uint32_t pin_number, uint32_t val);
/*read number of interrupts performed*/
uint32_t DRV_GPIO_WAKEUP_GetInterruptCount(uint32_t pin_number);
/*void function for debug purposes*/
void DRV_GPIO_WAKEUP_DummyInterruptHandler(void* param);

/****************************************************************************
 * Functions
 ****************************************************************************/
/*checks whether there is available port*/
static int32_t DRV_GPIO_WAKEUP_CheckAvailability(int32_t debounce_required) {
  int32_t i;
  /* Wakeup sources 2,3,6,7 are
   * allocated to MCU (only 4 first wakeup machines
   * have a debounce option, so if there is no need in
   * debounce we should use machines 6-7)*/

  if (debounce_required == 1) {
    for (i = 2; i < 4; i++) /* 2,3 */
      if (gpio_wakeup_info[i].gpio_pin == -1) return i;
  }

  for (i = 6; i < GPIO_WAKEUP_MODEM_SOURCE_SIZE; i++) /* 6,7 */
    if (gpio_wakeup_info[i].gpio_pin == -1) return i;

  return -1;
}

/* Busy wait for approximate delay us */
static void DRV_GPIO_WAKEUP_Delayus(uint32_t freq, int32_t d) {
  uint32_t p;

  p = (d >> 1) * (freq >> 19);
  while (p--)
    ;
}

/* Fill the gpio_wakeup_info structure .Used after the wakeup */
int32_t DRV_GPIO_WAKEUP_Restore(void) {
  int32_t i, warm_wakeup = -1;
  uint32_t pin_number;

  /* Initialize wakeup_info_lookup tab */
  if (wakeup_info_lookup[0] == 254 && wakeup_info_lookup[1] == 254)
    memset(wakeup_info_lookup, GPIO_WAKEUP_FORBIDDEN_WAKEUP_PIN, sizeof(wakeup_info_lookup));

  /* Inspect all the sources allocated for MCU processor (2,3,6,7)
   * and fill the structure according to the registers */
  for (i = 0; i < GPIO_WAKEUP_MODEM_SOURCE_SIZE; i++) {
    if ((i != 2) && (i != 3) && (i != 6) && (i != 7))  // 4th source is reserved for PMP processor
      continue;                                        // 2,3,6,7 are reserved for MCU

    if (IO_WKUP_REG(IO_WKUP_CTRL_WAKEUP_EN(i))) {
      warm_wakeup = 0;
      pin_number =
          (IO_WKUP_REG(IO_WKUP_CTRL_CTRL1(i)) & IO_WKUP_CTRL_WAKEUP_0_CTRL1_WAKEUP_IO_SEL_Msk) >>
          IO_WKUP_CTRL_WAKEUP_0_CTRL1_WAKEUP_IO_SEL_Pos;

      /* fill the temporary structure from the registers */
      DRV_GPIO_WAKEUP_GetSetup(gpio_wakeup_info + i, pin_number, DRV_GPIO_WAKEUP_REGISTER_LOOKUP);

      if (pin_number >= (sizeof(wakeup_to_gpio_lookup) / sizeof(wakeup_to_gpio_lookup[0])))
        continue;
      pin_number = wakeup_to_gpio_lookup[pin_number];
      gpio_wakeup_info[i].gpio_pin = pin_number;

      wakeup_info_lookup[pin_number] = i;
      /* even if interrupt is disabled it still need to be handled
       * in wakeup machine*/
      gpio_wakeup_info[i].irq_handler =
          (DRV_GPIO_WAKEUP_IrqCallback)DRV_GPIO_WAKEUP_DummyInterruptHandler;
      gpio_wakeup_info[i].user_param = 0;

      /*another level of protection, in case there is enabled  wakeup
       * machine, but no valid pin assigned to it*/
    } else {
      gpio_wakeup_info[i].gpio_pin = -1;
    }
  }
  return warm_wakeup;
}

inline int32_t DRV_GPIO_WAKEUP_FindIO(uint32_t pin_number) {
  if (wakeup_info_lookup[0] == 254 && wakeup_info_lookup[1] == 254)
    memset(wakeup_info_lookup, GPIO_WAKEUP_FORBIDDEN_WAKEUP_PIN, sizeof(wakeup_info_lookup));

  if (pin_number >= GPIO_WAKEUP_MAX_GPIO_ID) return -1;
  return ((wakeup_info_lookup[pin_number] == GPIO_WAKEUP_FORBIDDEN_WAKEUP_PIN)
              ? -1
              : wakeup_info_lookup[pin_number]);
}

/*Returns structure of wakeup source by pin_number.Wakeup source data can be retrieved
 * from the structure or directly from registers. gpio_wakeup_lookup_method enumerator
 * should be used for each case */
void DRV_GPIO_WAKEUP_GetSetup(DRV_GPIO_WAKEUP_Configuration* gpio_wakeup_elem, uint32_t pin_number,
                              DRV_GPIO_WAKEUP_LookupMethod method) {
  int32_t id_in_structure;
  uint32_t wakeup_id;

  if (pin_number >= (sizeof(gpio_to_wakeup_lookup) / sizeof(gpio_to_wakeup_lookup[0]))) return;
  wakeup_id = gpio_to_wakeup_lookup[pin_number];
  if (wakeup_id == GPIO_WAKEUP_FORBIDDEN_WAKEUP_PIN) return;
  if (method == DRV_GPIO_WAKEUP_STRUCT_LOOKUP) {
    id_in_structure = DRV_GPIO_WAKEUP_FindIO(pin_number);
    if (id_in_structure < 0) {
      gpio_wakeup_elem->gpio_pin = -1;
      return;
    }
  } else if (method == DRV_GPIO_WAKEUP_REGISTER_LOOKUP) {
    for (id_in_structure = 0; id_in_structure < GPIO_WAKEUP_MODEM_SOURCE_SIZE; id_in_structure++) {
      if ((id_in_structure == 0) || (id_in_structure == 1) || (id_in_structure == 4)) continue;
      if ((((IO_WKUP_REG(IO_WKUP_CTRL_CTRL1(id_in_structure)) &
             IO_WKUP_CTRL_WAKEUP_0_CTRL1_WAKEUP_IO_SEL_Msk) >>
            IO_WKUP_CTRL_WAKEUP_0_CTRL1_WAKEUP_IO_SEL_Pos) == wakeup_id)) {
        break;
      }
    }
    if (id_in_structure == GPIO_WAKEUP_MODEM_SOURCE_SIZE) {
      gpio_wakeup_elem->gpio_pin = -1;
      return;
    }
  } else {
    gpio_wakeup_elem->gpio_pin = -1;
    return;
  }
  if (IO_WKUP_REG(IO_WKUP_CTRL_CTRL0(id_in_structure)) &
      IO_WKUP_CTRL_WAKEUP_0_CTRL0_ASYNC_WAKEUP_Msk)
    gpio_wakeup_elem->clk = GPIO_WAKEUP_ASYNC_CLK;
  else if (IO_WKUP_REG(IO_WKUP_CTRL_CTRL0(id_in_structure)) &
           IO_WKUP_CTRL_WAKEUP_0_CTRL0_CLK_SELECT_Msk)
    gpio_wakeup_elem->clk = GPIO_WAKEUP_URCLK_CLK;
  else
    gpio_wakeup_elem->clk = GPIO_WAKEUP_RTC_CLK;

  if (IO_WKUP_REG(IO_WKUP_CTRL_CTRL0(id_in_structure)) &
      IO_WKUP_CTRL_WAKEUP_0_CTRL0_DEBOUNCE_EN_Msk)
    gpio_wakeup_elem->debounce_en = 1;
  else
    gpio_wakeup_elem->debounce_en = 0;

  if (IO_WKUP_REG(IO_WKUP_CTRL_WAKEUP_EN(id_in_structure)) & IO_WKUP_CTRL_WAKEUP_0_EN_WAKEUP_EN_Msk)
    gpio_wakeup_elem->wakeup_en = 1;
  else
    gpio_wakeup_elem->wakeup_en = 0;

  if (REGISTER(WKUP_PROCESSORS_PMG_WAKEUP_MASK_MCU0) &
      (1 << (id_in_structure + GPIO_WAKEUP_OFFSET_IN_WAKEUP_VECTOR)))
    gpio_wakeup_elem->int_en = 0;
  else
    gpio_wakeup_elem->int_en = 1;

  gpio_wakeup_elem->gpio_pin = (IO_WKUP_REG(IO_WKUP_CTRL_CTRL1(id_in_structure)) &
                                IO_WKUP_CTRL_WAKEUP_0_CTRL1_WAKEUP_IO_SEL_Msk) >>
                               IO_WKUP_CTRL_WAKEUP_0_CTRL1_WAKEUP_IO_SEL_Pos;
  gpio_wakeup_elem->debounce_max_val = (IO_WKUP_REG(IO_WKUP_CTRL_CTRL1(id_in_structure)) &
                                        IO_WKUP_CTRL_WAKEUP_0_CTRL1_DEBOUNCE_MAX_VAL_Msk);

  /* Polarity/Edge understanding */
  /* Check whether edge or high/low functionality configured */
  if (((IO_WKUP_REG(IO_WKUP_CTRL_CTRL0(id_in_structure)) &
        IO_WKUP_CTRL_WAKEUP_0_CTRL0_EDGE_EN_Msk) == 0)) {
    if (((IO_WKUP_REG(IO_WKUP_CTRL_CTRL0(id_in_structure)) &
          IO_WKUP_CTRL_WAKEUP_0_CTRL0_POL_CHANGE_Msk) >>
         IO_WKUP_CTRL_WAKEUP_0_CTRL0_POL_CHANGE_Pos) == 0) {  // 0 - polarity high, 1 - polarity low
      gpio_wakeup_elem->mode = GPIO_WAKEUP_LEVEL_POL_HIGH;
    } else {
      gpio_wakeup_elem->mode = GPIO_WAKEUP_LEVEL_POL_LOW;
    }
  } else {
    switch ((IO_WKUP_REG(IO_WKUP_CTRL_CTRL0(id_in_structure)) &
             IO_WKUP_CTRL_WAKEUP_0_CTRL0_EDGE_KIND_Msk) >>
            IO_WKUP_CTRL_WAKEUP_0_CTRL0_EDGE_KIND_Pos) {
      case 0: /*rising*/
        gpio_wakeup_elem->mode = GPIO_WAKEUP_EDGE_RISING;
        break;
      case 1: /*falling*/
        gpio_wakeup_elem->mode = GPIO_WAKEUP_EDGE_FALLING;
        break;
      case 2: /*toggle*/
        gpio_wakeup_elem->mode = GPIO_WAKEUP_EDGE_RISING_FALLING;
        break;
    }
  }
}

uint32_t DRV_GPIO_WAKEUP_GetInterruptCount(uint32_t pin_number) {
  int32_t id_in_structure;

  id_in_structure = DRV_GPIO_WAKEUP_FindIO(pin_number);
  if (id_in_structure < 0)
    return -1;
  else
    return gpio_wakeup_info[id_in_structure].int_counter;
}

/*Disables wakeup for specified IO in wakeup machine*/
int32_t DRV_GPIO_WAKEUP_Disable(uint32_t pin_number) {
  int32_t id_in_structure;

  id_in_structure = DRV_GPIO_WAKEUP_FindIO(pin_number);
  if (id_in_structure < 0) return -1;

  IO_WKUP_REG(IO_WKUP_CTRL_WAKEUP_EN(id_in_structure)) &= ~IO_WKUP_CTRL_WAKEUP_0_EN_WAKEUP_EN_Msk;
  reg_temp = IO_WKUP_REG(IO_WKUP_CTRL_WAKEUP_EN(id_in_structure));  // make sure its written

  gpio_wakeup_info[id_in_structure].wakeup_en = 0;
  return 0;
}

/*Enables wakeup for specified IO in wakeup machine*/
int32_t DRV_GPIO_WAKEUP_Enable(uint32_t pin_number) {
  int32_t id_in_structure;

  id_in_structure = DRV_GPIO_WAKEUP_FindIO(pin_number);
  if (id_in_structure < 0) return -1;

  IO_WKUP_REG(IO_WKUP_CTRL_WAKEUP_EN(id_in_structure)) |= IO_WKUP_CTRL_WAKEUP_0_EN_WAKEUP_EN_Msk;
  reg_temp = IO_WKUP_REG(IO_WKUP_CTRL_WAKEUP_EN(id_in_structure));  // make sure its written

  gpio_wakeup_info[id_in_structure].wakeup_en = 1;

  return 0;
}

/*Disables wakeup for specified IO in wakeup machine,
 *  removes member from auxiliary array.*/
int32_t DRV_GPIO_WAKEUP_DeleteIO(uint32_t pin_number) {
  int32_t id_in_structure;

  id_in_structure = DRV_GPIO_WAKEUP_FindIO(pin_number);
  if (id_in_structure < 0) return -1;

  DRV_GPIO_WAKEUP_Disable(pin_number);
  IO_WKUP_REG(IO_WKUP_CTRL_CTRL1(id_in_structure)) |=
      ((0 << IO_WKUP_CTRL_WAKEUP_0_CTRL1_WAKEUP_IO_SEL_Pos) &
       IO_WKUP_CTRL_WAKEUP_0_CTRL1_WAKEUP_IO_SEL_Msk);
  gpio_wakeup_info[id_in_structure].wakeup_en = 0;
  gpio_wakeup_info[id_in_structure].gpio_pin = -1;
  wakeup_info_lookup[pin_number] = GPIO_WAKEUP_FORBIDDEN_WAKEUP_PIN;

  return 0;
}

/*Select polarity mode for wakeup (HIGH – mode=0, LOW – mode=1,
 *  RISING - mode=2, FALLING - mode=3, RISING_FALLING - mode=4)*/
int32_t DRV_GPIO_WAKEUP_SetPolarity(uint32_t pin_number, GPIO_WAKEUP_Polarity mode) {
  int32_t id_in_structure;
  uint32_t temp_reg;

  id_in_structure = DRV_GPIO_WAKEUP_FindIO(pin_number);
  if (id_in_structure < 0) return -1;

  DRV_GPIO_WAKEUP_Disable(pin_number);

  temp_reg = IO_WKUP_REG(IO_WKUP_CTRL_CTRL0(id_in_structure));

  /*wakeup mode selection*/
  switch (mode) {
    case GPIO_WAKEUP_LEVEL_POL_HIGH: /*polarity for level wakeup 0 = high*/
      temp_reg &= ~IO_WKUP_CTRL_WAKEUP_0_CTRL0_EDGE_EN_Msk;
      temp_reg &= ~IO_WKUP_CTRL_WAKEUP_0_CTRL0_POL_CHANGE_Msk;
      break;

    case GPIO_WAKEUP_LEVEL_POL_LOW: /*polarity for level wakeup 1 = low*/
      temp_reg &= ~IO_WKUP_CTRL_WAKEUP_0_CTRL0_EDGE_EN_Msk;
      temp_reg |= IO_WKUP_CTRL_WAKEUP_0_CTRL0_POL_CHANGE_Msk;
      break;

    case GPIO_WAKEUP_EDGE_RISING: /*0 in edge_kind register*/
      temp_reg |= IO_WKUP_CTRL_WAKEUP_0_CTRL0_EDGE_EN_Msk;
      temp_reg &= ~IO_WKUP_CTRL_WAKEUP_0_CTRL0_EDGE_KIND_Msk;
      break;

    case GPIO_WAKEUP_EDGE_FALLING: /*1 in edge_kind register*/
      temp_reg |= IO_WKUP_CTRL_WAKEUP_0_CTRL0_EDGE_EN_Msk;
      temp_reg &= ~IO_WKUP_CTRL_WAKEUP_0_CTRL0_EDGE_KIND_Msk;
      temp_reg |= (GPIO_WAKEUP_MODE_EDGE_FALLING << IO_WKUP_CTRL_WAKEUP_0_CTRL0_EDGE_KIND_Pos);
      break;

    case GPIO_WAKEUP_EDGE_RISING_FALLING: /*2 in edge_kind register*/
      temp_reg |= IO_WKUP_CTRL_WAKEUP_0_CTRL0_EDGE_EN_Msk;
      temp_reg &= ~IO_WKUP_CTRL_WAKEUP_0_CTRL0_EDGE_KIND_Msk;
      temp_reg |= (GPIO_WAKEUP_MODE_EDGE_TOGGLE << IO_WKUP_CTRL_WAKEUP_0_CTRL0_EDGE_KIND_Pos);
      break;

    default:
      return -3; /*incorrect wakeup mode */
  }

  IO_WKUP_REG(IO_WKUP_CTRL_CTRL0(id_in_structure)) = temp_reg;
  gpio_wakeup_info[id_in_structure].mode = mode;

  DRV_GPIO_WAKEUP_Enable(pin_number);

  return 0;
}

/*Select clock (RTC – clk_id=0, URCLK – clk_id=1, ASYNC – clk_id=2)*/
int32_t DRV_GPIO_WAKEUP_SelectClock(uint32_t pin_number, GPIO_WAKEUP_Clock clk_id) {
  int32_t id_in_structure;
  uint32_t temp_reg, temp_reg_urclk;

  id_in_structure = DRV_GPIO_WAKEUP_FindIO(pin_number);
  if (id_in_structure < 0) return -1;

  DRV_GPIO_WAKEUP_Disable(pin_number);
  temp_reg = IO_WKUP_REG(IO_WKUP_CTRL_CTRL0(id_in_structure));

  /*Clock select*/
  switch (clk_id) {
    case GPIO_WAKEUP_ASYNC_CLK: /*polarity for level wakeup 0 = high*/
      temp_reg |= IO_WKUP_CTRL_WAKEUP_0_CTRL0_ASYNC_WAKEUP_Msk;
      break;

    case GPIO_WAKEUP_RTC_CLK: /*polarity for level wakeup 0 = high*/
      temp_reg &= ~IO_WKUP_CTRL_WAKEUP_0_CTRL0_ASYNC_WAKEUP_Msk;
      temp_reg &= ~IO_WKUP_CTRL_WAKEUP_0_CTRL0_CLK_SELECT_Msk;
      break;

    case GPIO_WAKEUP_URCLK_CLK: /*polarity for level wakeup 0 = high*/
      if ((REGISTER(URCLK_CFG_URCLK_CFG) & URCLK_CFG_URCLK_CFG_RING_OSC_UR_EN_MSK) == 0) {
        temp_reg_urclk = REGISTER(URCLK_CFG_URCLK_CFG);
        temp_reg_urclk |= URCLK_CFG_URCLK_CFG_RING_OSC_UR_EN_MSK;
        REGISTER(URCLK_CFG_URCLK_CFG) = temp_reg_urclk;
      }

      temp_reg &= ~IO_WKUP_CTRL_WAKEUP_0_CTRL0_ASYNC_WAKEUP_Msk;
      temp_reg |= IO_WKUP_CTRL_WAKEUP_0_CTRL0_CLK_SELECT_Msk;
      break;

    default:
      return -3; /*incorrect wakeup mode */
  }

  IO_WKUP_REG(IO_WKUP_CTRL_CTRL0(id_in_structure)) = temp_reg;
  gpio_wakeup_info[id_in_structure].clk = clk_id;

  DRV_GPIO_WAKEUP_Enable(pin_number);

  return 0;
}

/*Enables debounce for specified IO, returns zero in
 * case of success and -1 in case this IO is not registered as wakeup.*/
int32_t DRV_GPIO_WAKEUP_EnableDebounce(uint32_t pin_number) {
  int32_t id_in_structure;
  uint32_t temp_reg;

  id_in_structure = DRV_GPIO_WAKEUP_FindIO(pin_number);

  /* check valid range of with-debounce */
  if (id_in_structure < 2 || id_in_structure > 3) return -1;

  /* In case that specific io is still not registered as wakeup
   * no need to disable it, because it is not enabled yet.
   * Enabling will be done in the end of the add_io function.
   * But if user wishes to modify some parameters after
   * registration, wakeup machine should be disabled and enabled
   * after modification*/
  DRV_GPIO_WAKEUP_Disable(pin_number);

  temp_reg = IO_WKUP_REG(IO_WKUP_CTRL_CTRL0(id_in_structure));
  temp_reg |= IO_WKUP_CTRL_WAKEUP_0_CTRL0_DEBOUNCE_EN_Msk;
  IO_WKUP_REG(IO_WKUP_CTRL_CTRL0(id_in_structure)) = temp_reg;

  gpio_wakeup_info[id_in_structure].debounce_en = 1;

  DRV_GPIO_WAKEUP_Enable(pin_number);

  return 0;
}

/* Disables debounce for specified IO, returns zero in
 * case of success and -1 in case this IO is not registered as wakeup.*/
int32_t DRV_GPIO_WAKEUP_DisableDebounce(uint32_t pin_number) {
  int32_t id_in_structure;
  uint32_t temp_reg;

  id_in_structure = DRV_GPIO_WAKEUP_FindIO(pin_number);

  /* check valid range of with-debounce */
  if (id_in_structure < 2 || id_in_structure > 3) return -1;

  DRV_GPIO_WAKEUP_Disable(pin_number);
  temp_reg = IO_WKUP_REG(IO_WKUP_CTRL_CTRL0(id_in_structure));
  temp_reg &= ~IO_WKUP_CTRL_WAKEUP_0_CTRL0_DEBOUNCE_EN_Msk;

  IO_WKUP_REG(IO_WKUP_CTRL_CTRL0(id_in_structure)) = temp_reg;
  gpio_wakeup_info[id_in_structure].debounce_en = 0;

  DRV_GPIO_WAKEUP_Enable(pin_number);

  return 0;
}

/*Sets number of ckocks that signal should stay before change*/
int32_t DRV_GPIO_WAKEUP_SetDebounceValue(uint32_t pin_number, uint32_t val) {
  int32_t id_in_structure;
  uint32_t temp_reg;

  id_in_structure = DRV_GPIO_WAKEUP_FindIO(pin_number);

  /* check valid range of with-debounce */
  if (id_in_structure < 2 || id_in_structure > 3) return -1;

  DRV_GPIO_WAKEUP_Disable(pin_number);
  temp_reg = IO_WKUP_REG(IO_WKUP_CTRL_CTRL1(id_in_structure));
  temp_reg &= ~IO_WKUP_CTRL_WAKEUP_0_CTRL1_DEBOUNCE_MAX_VAL_Msk;
  temp_reg |= (val << IO_WKUP_CTRL_WAKEUP_0_CTRL1_DEBOUNCE_MAX_VAL_Pos) &
              IO_WKUP_CTRL_WAKEUP_0_CTRL1_DEBOUNCE_MAX_VAL_Msk;
  IO_WKUP_REG(IO_WKUP_CTRL_CTRL1(id_in_structure)) = temp_reg;
  gpio_wakeup_info[id_in_structure].debounce_max_val = val;

  DRV_GPIO_WAKEUP_Enable(pin_number);

  return 0;
}

/*Enable interrupt for specified IO, returns zero in
 *  case of success and -1 in case this IO is not registered as wakeup.*/
int32_t DRV_GPIO_WAKEUP_EnableInterrupt(uint32_t pin_number) {
  int32_t id_in_structure;

  id_in_structure = DRV_GPIO_WAKEUP_FindIO(pin_number);
  if (id_in_structure < 0) return -1;

  if (gpio_wakeup_info[id_in_structure].int_en) {
    return -2;
  }

  gpio_wakeup_info[id_in_structure].int_en = 1;

  /* if gpio configured as level interrupt - then clear its interrupt pending bit before enabling
   * the interrupt procedure of interrupt clearing has to include following steps:
   *      - disable interrupt
   *      - disable wakeup
   *      - enable wakeup
   *      - enable interrupt
   * ,because of hardware bug */
  if ((IO_WKUP_REG(IO_WKUP_CTRL_CTRL0(id_in_structure)) &
       IO_WKUP_CTRL_WAKEUP_0_CTRL0_EDGE_EN_Msk) == 0) {
    /*Disable intrrupt*/
    IO_WKUP_REG(IO_WKUP_CTRL_INT_EN(id_in_structure)) &= ~IO_WKUP_CTRL_WAKEUP_0_INT_EN_INT_EN_Msk;
    /*Disable wakeup machine*/
    IO_WKUP_REG(IO_WKUP_CTRL_WAKEUP_EN(id_in_structure)) &= ~IO_WKUP_CTRL_WAKEUP_0_EN_WAKEUP_EN_Msk;
    /*Enable wakeup machine*/
    IO_WKUP_REG(IO_WKUP_CTRL_WAKEUP_EN(id_in_structure)) |= IO_WKUP_CTRL_WAKEUP_0_EN_WAKEUP_EN_Msk;
    /*Enable interrupt*/
    IO_WKUP_REG(IO_WKUP_CTRL_INT_EN(id_in_structure)) |= IO_WKUP_CTRL_WAKEUP_0_INT_EN_INT_EN_Msk;
  }

  /* Unmask specific wakeup source for interrupts*/
  REGISTER(WKUP_PROCESSORS_PMG_WAKEUP_MASK_MCU0) &=
      ~(1 << (id_in_structure + GPIO_WAKEUP_OFFSET_IN_WAKEUP_VECTOR));

  return 0;
}

/*Disables interrupt for specified IO, returns zero
 * in case of success and -1 in case this IO is not registered as wakeup.
 * -2 in case disable interrupt did not ended */
int32_t DRV_GPIO_WAKEUP_DisableInterrupt(uint32_t pin_number) {
  int32_t id_in_structure;
  int32_t count = 5, ret_val = 0;
  uint32_t irqmask;

  id_in_structure = DRV_GPIO_WAKEUP_FindIO(pin_number);
  if (id_in_structure < 0) return -1;

  // if already disabled - return
  if (!gpio_wakeup_info[id_in_structure].int_en) {
    reg_temp = REGISTER(WKUP_PROCESSORS_PMG_WAKEUP_MASK_MCU0);
    if ((reg_temp & (1 << (id_in_structure + GPIO_WAKEUP_OFFSET_IN_WAKEUP_VECTOR))) == 1) {
      return 0;
    }
  }

  // limit retry times
  while (--count) {
    if (__get_IPSR() != 0U) {
      /* is inside IRQ */
      /* Mask specific wakeup source for interrupts*/
      REGISTER(WKUP_PROCESSORS_PMG_WAKEUP_MASK_MCU0) |=
          (1 << (id_in_structure + GPIO_WAKEUP_OFFSET_IN_WAKEUP_VECTOR));

      reg_temp = REGISTER(
          WKUP_PROCESSORS_PMG_WAKEUP_MASK_MCU0);  // read mask register - make sure its written
      gpio_wakeup_info[id_in_structure].int_en = 0;

      return 0;
    }

    irqmask = ((__get_PRIMASK() != 0U) || (__get_BASEPRI() != 0U));
    __disable_irq();

    /* Mask specific wakeup source for interrupts*/
    REGISTER(WKUP_PROCESSORS_PMG_WAKEUP_MASK_MCU0) |=
        (1 << (id_in_structure + GPIO_WAKEUP_OFFSET_IN_WAKEUP_VECTOR));

    reg_temp = REGISTER(
        WKUP_PROCESSORS_PMG_WAKEUP_MASK_MCU0);  // read mask register - make sure its written

    // check if interrupt arrived due to the race condition
    if ((IO_WKUP_REG(IO_WKUP_CTRL_INT(id_in_structure)) &
         IO_WKUP_CTRL_WAKEUP_0_INT_INT_GPMCLK_Msk)) {
      // enable back interrupt (and try again)
      REGISTER(WKUP_PROCESSORS_PMG_WAKEUP_MASK_MCU0) &=
          ~(1 << (id_in_structure + GPIO_WAKEUP_OFFSET_IN_WAKEUP_VECTOR));

      if (!irqmask) {
        __enable_irq();
      }

      reg_temp = REGISTER(WKUP_PROCESSORS_PMG_WAKEUP_MASK_MCU0);  // dummy read - give some time
    } else {
      // normal case - continue normally
      gpio_wakeup_info[id_in_structure].int_en = 0;

      if (!irqmask) {
        __enable_irq();
      }

      return 0;
    }
  }
  if (count == 0) {
    /* ERROR: disable interrupt did not ended!" */
    ret_val = (-2);
  }

  gpio_wakeup_info[id_in_structure].int_en = 0;

  return ret_val;
}

/*  Writes the IO as wakeup and puts values in registers,
 *  returns zero in case of success and error code otherwise.
 *  Also adds new member in auxiliary array.*/
int32_t DRV_GPIO_WAKEUP_AddIO(DRV_GPIO_WAKEUP_Configuration* params) {
  int32_t free_pin_id;

  if (DRV_GPIO_WAKEUP_FindIO(params->gpio_pin) >= 0) return (-1);

  /* if all wakeups occupied */
  free_pin_id = DRV_GPIO_WAKEUP_CheckAvailability(params->debounce_en);
  if (free_pin_id < 0) return (-3);

  /* if got free_pin_id >= GPIO_WAKEUP_SOURCES_WITH_DEBOUNCE, set debounce_en to disable */
  if (free_pin_id >= GPIO_WAKEUP_SOURCES_WITH_DEBOUNCE) params->debounce_en = 0;

  uint32_t wakeup_id;
  if (params->gpio_pin >= (int)(sizeof(gpio_to_wakeup_lookup) / sizeof(gpio_to_wakeup_lookup[0])))
    return (-4);
  wakeup_id = gpio_to_wakeup_lookup[params->gpio_pin];
  if (wakeup_id == GPIO_WAKEUP_FORBIDDEN_WAKEUP_PIN) return (-5);

  wakeup_info_lookup[params->gpio_pin] = free_pin_id;

  gpio_wakeup_info[free_pin_id].gpio_pin = params->gpio_pin;

  /* pin select */
  IO_WKUP_REG(IO_WKUP_CTRL_CTRL1(free_pin_id)) |=
      ((wakeup_id << IO_WKUP_CTRL_WAKEUP_0_CTRL1_WAKEUP_IO_SEL_Pos) &
       IO_WKUP_CTRL_WAKEUP_0_CTRL1_WAKEUP_IO_SEL_Msk);

  /* debounce */
  if (params->debounce_en > 0) {
    DRV_GPIO_WAKEUP_EnableDebounce(params->gpio_pin);
    DRV_GPIO_WAKEUP_SetDebounceValue(params->gpio_pin, params->debounce_max_val);
  } else {
    DRV_GPIO_WAKEUP_DisableDebounce(params->gpio_pin);
  }

  /* wakeup mode selection */
  DRV_GPIO_WAKEUP_SetPolarity(params->gpio_pin, params->mode);

  /* Interrupt enable */
  if (params->int_en)
    DRV_GPIO_WAKEUP_EnableInterrupt(params->gpio_pin);
  else
    DRV_GPIO_WAKEUP_DisableInterrupt(params->gpio_pin);

  /* Clock select */
  DRV_GPIO_WAKEUP_SelectClock(params->gpio_pin, params->clk);

  /* Wakeup enable */
  if (params->wakeup_en > 0) {
    DRV_GPIO_WAKEUP_Enable(params->gpio_pin);
  } else {
    DRV_GPIO_WAKEUP_Disable(params->gpio_pin);
  }

  return 0;
}

/* Because of bug in hardware ,disabling interrupt can't be done
 * by IO_WKUP_CTRL_INT_EN register. Interrupt need to be masked
 * to MCU processor. In this case if there is interrupt latched,
 * it need to be cleared before going to sleep.
 * */
void DRV_GPIO_WAKEUP_ClearInterrupts(void) {
  uint32_t i;

  for (i = 0; i < GPIO_WAKEUP_MODEM_SOURCE_SIZE; i++) {
    if ((i != 2) && (i != 3) && (i != 6) && (i != 7)) continue;  // 2,3,6,7 are reserved for MCU

    if (((REGISTER(WKUP_PROCESSORS_PMG_WAKEUP_MASK_MCU0) &
          (1 << (i + GPIO_WAKEUP_OFFSET_IN_WAKEUP_VECTOR))) != 0) &&
        IO_WKUP_REG(IO_WKUP_CTRL_WAKEUP_EN(i))) {
      /*Disable intrrupt*/
      IO_WKUP_REG(IO_WKUP_CTRL_INT_EN(i)) &= ~IO_WKUP_CTRL_WAKEUP_0_INT_EN_INT_EN_Msk;
      /*Disable wakeup machine*/
      IO_WKUP_REG(IO_WKUP_CTRL_WAKEUP_EN(i)) &= ~IO_WKUP_CTRL_WAKEUP_0_EN_WAKEUP_EN_Msk;
      DRV_GPIO_WAKEUP_Delayus(REF_CLK, 40);
      /*Enable wakeup machine*/
      IO_WKUP_REG(IO_WKUP_CTRL_WAKEUP_EN(i)) |= IO_WKUP_CTRL_WAKEUP_0_EN_WAKEUP_EN_Msk;
      /*Enable interrupt*/
      IO_WKUP_REG(IO_WKUP_CTRL_INT_EN(i)) |= IO_WKUP_CTRL_WAKEUP_0_INT_EN_INT_EN_Msk;
    }
  }
}

/* this function is registered in sleep-manager clear of
 * masked interrupts should be done before going to
 * sleep and should not be done on the wakeup*/
#if (configUSE_ALT_SLEEP == 1)
int32_t DRV_GPIO_WAKEUP_HandleSleepNotify(DRV_SLEEP_NotifyType sleep_state, DRV_PM_PwrMode pwr_mode,
                                          void* ptr_ctx) {
  if (sleep_state == DRV_SLEEP_NTFY_SUSPENDING) {
    /* These seetings may be stored before/restored after sleep */

    // DRV_GPIO_WAKEUP_ClearInterrupts();
  } else if (sleep_state == DRV_SLEEP_NTFY_RESUMING) {
    /* These seetings may be stored before/restored after sleep */
  }

  return 0;
}
#endif

void DRV_GPIO_WAKEUP_Initialize(void) {
  int32_t i;

  if (!is_initialized) {
    for (i = 0; i < GPIO_WAKEUP_MODEM_SOURCE_SIZE; i++) {
      if ((i != 2) && (i != 3) && (i != 6) && (i != 7))  // 2,3,6,7 are reserved for MCU
        continue;
      else
        gpio_wakeup_info[i].gpio_pin = -1;
    }

    DRV_GPIO_WAKEUP_Restore();
#if (configUSE_ALT_SLEEP == 1)
    int32_t temp_for_sleep_notify, ret_val;

    ret_val =
        DRV_SLEEP_RegNotification(&DRV_GPIO_WAKEUP_HandleSleepNotify, &temp_for_sleep_notify, NULL);
    if (ret_val != 0) { /* failed to register sleep notify callback */
      ;
    }
#endif
    NVIC_SetPriority(GPM_IO_WAKEUP_IRQn, 7); /* set Interrupt priority */
    NVIC_EnableIRQ(GPM_IO_WAKEUP_IRQn);
    is_initialized = true;
  }
}

/* Because of hardware fault, interrupt need to be cleared by
 * disabling both interrupt_enable and wakeup_enable */
void gpio_wakeup_IRQHandler(void) {
  uint32_t i;

  /* Inspect all the sources allocated for NET processor (0-3,5-7)
   * and fill the structure according to the registers*/
  for (i = 0; i < GPIO_WAKEUP_MODEM_SOURCE_SIZE; i++) {
    if ((i != 2) && (i != 3) && (i != 6) && (i != 7)) continue;
    /* check all conditions - wakeup enable , interrupt enable and raised interrupt flag
     * Flow that takes place below is the right way to deal with interrupt
     * Description in of the flow is in the function's description*/
    if ((IO_WKUP_REG(IO_WKUP_CTRL_WAKEUP_EN(i)) &&
         (IO_WKUP_REG(IO_WKUP_CTRL_INT(i)) & IO_WKUP_CTRL_WAKEUP_0_INT_INT_GPMCLK_Msk)) &&
        ((REGISTER(WKUP_PROCESSORS_PMG_WAKEUP_MASK_MCU0) &
          (1 << (i + GPIO_WAKEUP_OFFSET_IN_WAKEUP_VECTOR))) == 0)) {
      /*Disable intrrupt*/
      IO_WKUP_REG(IO_WKUP_CTRL_INT_EN(i)) &= ~IO_WKUP_CTRL_WAKEUP_0_INT_EN_INT_EN_Msk;

      /*Disable wakeup machine*/
      IO_WKUP_REG(IO_WKUP_CTRL_WAKEUP_EN(i)) &= ~IO_WKUP_CTRL_WAKEUP_0_EN_WAKEUP_EN_Msk;

      /* delay of 3 ticks of 32K pmp timer */
      DRV_GPIO_WAKEUP_Delayus(REF_CLK, 40);

      /*Enable wakeup machine*/
      IO_WKUP_REG(IO_WKUP_CTRL_WAKEUP_EN(i)) |= IO_WKUP_CTRL_WAKEUP_0_EN_WAKEUP_EN_Msk;

      /*Run user interrupt routine*/
      if (gpio_wakeup_info[i].irq_handler && gpio_wakeup_info[i].int_en) {
        gpio_wakeup_info[i].irq_handler(gpio_wakeup_info[i].user_param);
      } else {
        DRV_GPIO_WAKEUP_DisableInterrupt(gpio_wakeup_info[i].gpio_pin);
      }

      gpio_wakeup_info[i].int_counter++;

      /*Enable interrupt*/
      IO_WKUP_REG(IO_WKUP_CTRL_INT_EN(i)) |= IO_WKUP_CTRL_WAKEUP_0_INT_EN_INT_EN_Msk;
    }
  }
}

/*before registering interrupt all handlers should be assigned*/
int32_t DRV_GPIO_WAKEUP_RegisterInterrupt(uint32_t gpio, DRV_GPIO_WAKEUP_IrqCallback irq_handler,
                                          void* user_param) {
  if (!gpio_wakeup_irq_configured) {
    int32_t i;
    for (i = 0; i < GPIO_WAKEUP_MODEM_SOURCE_SIZE; i++) {
      gpio_wakeup_info[i].int_en = 0;
      gpio_wakeup_info[i].irq_handler = NULL;
      gpio_wakeup_info[i].user_param = 0;
    }
    gpio_wakeup_irq_configured = 1;
  }
  int32_t id_in_structure;

  id_in_structure = DRV_GPIO_WAKEUP_FindIO(gpio);
  if (id_in_structure < 0)
    return -1;
  else {
    gpio_wakeup_info[id_in_structure].int_counter = 0;
    gpio_wakeup_info[id_in_structure].irq_handler = irq_handler;
    gpio_wakeup_info[id_in_structure].user_param = user_param;
  }
  return 0;
}

void DRV_GPIO_WAKEUP_DummyInterruptHandler(void* param) { return; }

uint8_t DRV_GPIO_WAKEUP_ValidateUsingPin(uint32_t pin_number) {
  uint32_t i;

  for (i = 2; i < 4; i++) /* 2,3 */
    if (gpio_wakeup_info[i].gpio_pin == (int32_t)pin_number) return 1;

  for (i = 6; i < GPIO_WAKEUP_MODEM_SOURCE_SIZE; i++) /* 6,7 */
    if (gpio_wakeup_info[i].gpio_pin == (int32_t)pin_number) return 1;

  return 0;
}