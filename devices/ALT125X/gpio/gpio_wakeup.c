/*  ---------------------------------------------------------------------------

    (c) copyright 2019 Altair Semiconductor, Ltd. All rights reserved.

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

#include "FreeRTOS.h"
#include "task.h"
#include "portmacro.h"
#include "125X_mcu.h"
#include "gpio_wakeup_utils.h"
#include "gpio.h"
#include <string.h>

#define BASE_ADDRESS_IO_WKUP_CTRL 0xBC111E00
#define BASE_ADDRESS_WKUP_PROCESSORS 0xBC110300

#define IO_WKUP_CTRL_CTRL0(x) (BASE_ADDRESS_IO_WKUP_CTRL + 0x000 + (x * 0x20))
#define IO_WKUP_CTRL_IO_WAKEUP_CTRL0_CLK_SELECT_MSK (0x00040)
#define IO_WKUP_CTRL_IO_WAKEUP_CTRL0_CLK_SELECT_POS (6)
#define IO_WKUP_CTRL_IO_WAKEUP_CTRL0_DEBOUNCE_EN_MSK (0x00020)
#define IO_WKUP_CTRL_IO_WAKEUP_CTRL0_DEBOUNCE_EN_POS (5)
#define IO_WKUP_CTRL_IO_WAKEUP_CTRL0_POL_CHANGE_MSK (0x00010)
#define IO_WKUP_CTRL_IO_WAKEUP_CTRL0_POL_CHANGE_POS (4)
#define IO_WKUP_CTRL_IO_WAKEUP_CTRL0_ASYNC_WAKEUP_MSK (0x00008)
#define IO_WKUP_CTRL_IO_WAKEUP_CTRL0_ASYNC_WAKEUP_POS (3)
#define IO_WKUP_CTRL_IO_WAKEUP_CTRL0_EDGE_KIND_MSK (0x00006)
#define IO_WKUP_CTRL_IO_WAKEUP_CTRL0_EDGE_KIND_POS (1)
#define IO_WKUP_CTRL_IO_WAKEUP_CTRL0_EDGE_EN_MSK (0x00001)
#define IO_WKUP_CTRL_IO_WAKEUP_CTRL0_EDGE_EN_POS (0)
#define IO_WKUP_CTRL_CTRL1(x) (BASE_ADDRESS_IO_WKUP_CTRL + 0x004 + (x * 0x20))
#define IO_WKUP_CTRL_IO_WAKEUP_CTRL1_WAKEUP_IO_SEL_MSK (0x0FC00)
#define IO_WKUP_CTRL_IO_WAKEUP_CTRL1_WAKEUP_IO_SEL_POS (10)
#define IO_WKUP_CTRL_IO_WAKEUP_CTRL1_DEBOUNCE_MAX_VAL_MSK (0x003FF)
#define IO_WKUP_CTRL_IO_WAKEUP_CTRL1_DEBOUNCE_MAX_VAL_POS (0)
#define IO_WKUP_CTRL_WAKEUP_EN(x) (BASE_ADDRESS_IO_WKUP_CTRL + 0x008 + (x * 0x20))
#define IO_WKUP_CTRL_IO_WAKEUP_EN_WAKEUP_EN_MSK (0x00001)
#define IO_WKUP_CTRL_IO_WAKEUP_EN_WAKEUP_EN_POS (0)
#define IO_WKUP_CTRL_INT_EN(x) (BASE_ADDRESS_IO_WKUP_CTRL + 0x00C + (x * 0x20))
#define IO_WKUP_CTRL_IO_WAKEUP_INT_EN_INT_EN_MSK (0x00001)
#define IO_WKUP_CTRL_IO_WAKEUP_INT_EN_INT_EN_POS (0)
#define IO_WKUP_CTRL_INT(x) (BASE_ADDRESS_IO_WKUP_CTRL + 0x010 + (x * 0x20))
#define IO_WKUP_CTRL_IO_WAKEUP_INT_INT_ASYNC_MSK (0x00002)
#define IO_WKUP_CTRL_IO_WAKEUP_INT_INT_ASYNC_POS (1)
#define IO_WKUP_CTRL_IO_WAKEUP_INT_INT_GPMCLK_MSK (0x00001)
#define IO_WKUP_CTRL_IO_WAKEUP_INT_INT_GPMCLK_POS (0)
#define IO_WKUP_CTRL_INT_RCLR(x) (BASE_ADDRESS_IO_WKUP_CTRL + 0x014 + (x * 0x20))
#define IO_WKUP_CTRL_IO_WAKEUP_INT_RCLR_INT_GPMCLK_RCLK_MSK (0x00001)
#define IO_WKUP_CTRL_IO_WAKEUP_INT_RCLR_INT_GPMCLK_RCLK_POS (0)
//#define WKUP_PROCESSORS_PMG_WAKEUP_MASK_MCU0 (BASE_ADDRESS_WKUP_PROCESSORS + 0x30)

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

static int volatile g_temp = 0;

static uint8_t gpio_to_wakeup_lookup[] = {
    255, 0,  1,  2,  3,  4,  5,  255, 255, 255, 6,  7,   8,  9,  10, 11, 12, 13, 14, 15, 16,
    17,  18, 19, 20, 21, 22, 23, 24,  25,  26,  27, 255, 28, 29, 30, 31, 32, 33, 34, 35, 36,
    37,  38, 39, 40, 41, 42, 43, 44,  45,  46,  47, 48,  49, 50, 51, 52, 53, 54, 55, 56, 57};

static uint8_t wakeup_to_gpio_lookup[] = {
    1,  2,  3,  4,  5,  6,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
    24, 25, 26, 27, 28, 29, 30, 31, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44,
    45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62};

#if 1
/* for compile pass with IAR. But we still need a way to initial array for gpio interrupt checking.
 * So we give this array a magic number set 255,255 first. And this array will be init to right
 * value in gpio_wakeup_init() or gpio_wakeup_find_io() */
static uint8_t wakeup_info_lookup[GPIO_WAKEUP_MAX_GPIO_ID] = {254, 254};
#else
/* compile error with IAR */
static uint8_t wakeup_info_lookup[GPIO_WAKEUP_MAX_GPIO_ID] = {[0 ...(GPIO_WAKEUP_MAX_GPIO_ID - 1)] =
                                                                  GPIO_WAKEUP_FORBIDDEN_WAKEUP_PIN};
#endif

static gpio_wakeup_struct_t gpio_wakeup_info[GPIO_WAKEUP_MODEM_SOURCE_SIZE];
static unsigned int gpio_wakeup_irq_configured = 0;

inline int gpio_wakeup_find_io(unsigned int pin_number) {
  if (wakeup_info_lookup[0] == 254 && wakeup_info_lookup[1] == 254)
    memset(wakeup_info_lookup, GPIO_WAKEUP_FORBIDDEN_WAKEUP_PIN, sizeof(wakeup_info_lookup));

  if (pin_number >= GPIO_WAKEUP_MAX_GPIO_ID) return -1;
  return ((wakeup_info_lookup[pin_number] == GPIO_WAKEUP_FORBIDDEN_WAKEUP_PIN)
              ? -1
              : wakeup_info_lookup[pin_number]);
}

/*checks whether there is available port*/
static int gpio_wakeup_check_availability(int debounce_required) {
  int i;
  /* According to PRODUCTS-18944, only wakeup sources 2,3,6,7
   * allocated to MCU (only 4 first wakeup machines
   * have a debounce option, so if there is no need in
   * debounce we should use machines 6-7)*/

  if (debounce_required == 0) /* 6,7 */
    for (i = 6; i < GPIO_WAKEUP_MODEM_SOURCE_SIZE; i++)
      if (gpio_wakeup_info[i].gpio_pin == -1) return i;

  for (i = 2; i < 4; i++) /* 2,3 */
    if (gpio_wakeup_info[i].gpio_pin == -1) return i;

  return -1;
}

/*Returns structure of wakeup source by pin_number.Wakeup source data can be retrieved
 * from the structure or directly from registers. gpio_wakeup_lookup_method enumerator
 * should be used for each case */
void gpio_wakeup_get_setup(gpio_wakeup_struct_t* gpio_wakeup_elem, unsigned int pin_number,
                           gpio_wakeup_lookup_method_e method) {
  int id_in_structure;
  unsigned int wakeup_id;

  if (pin_number >= (sizeof(gpio_to_wakeup_lookup) / sizeof(gpio_to_wakeup_lookup[0]))) return;
  wakeup_id = gpio_to_wakeup_lookup[pin_number];
  if (wakeup_id == GPIO_WAKEUP_FORBIDDEN_WAKEUP_PIN) return;
  if (method == GPIO_WAKEUP_STRUCT_LOOKUP) {
    id_in_structure = gpio_wakeup_find_io(pin_number);
    if (id_in_structure < 0) {
      gpio_wakeup_elem->gpio_pin = -1;
      return;
    }
  } else if (method == GPIO_WAKEUP_REGISTER_LOOKUP) {
    for (id_in_structure = 0; id_in_structure < GPIO_WAKEUP_MODEM_SOURCE_SIZE; id_in_structure++) {
      if ((id_in_structure == 0) || (id_in_structure == 1) || (id_in_structure == 4)) continue;
      if ((((REGISTER(TOPREG(IO_WKUP_CTRL_CTRL1(id_in_structure))) &
             IO_WKUP_CTRL_IO_WAKEUP_CTRL1_WAKEUP_IO_SEL_MSK) >>
            IO_WKUP_CTRL_IO_WAKEUP_CTRL1_WAKEUP_IO_SEL_POS) == wakeup_id)) {
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
  if (REGISTER(TOPREG(IO_WKUP_CTRL_CTRL0(id_in_structure))) &
      IO_WKUP_CTRL_IO_WAKEUP_CTRL0_ASYNC_WAKEUP_MSK)
    gpio_wakeup_elem->clk = GPIO_WAKEUP_ASYNC_CLK;
  else if (REGISTER(TOPREG(IO_WKUP_CTRL_CTRL0(id_in_structure))) &
           IO_WKUP_CTRL_IO_WAKEUP_CTRL0_CLK_SELECT_MSK)
    gpio_wakeup_elem->clk = GPIO_WAKEUP_URCLK_CLK;
  else
    gpio_wakeup_elem->clk = GPIO_WAKEUP_RTC_CLK;

  if (REGISTER(TOPREG(IO_WKUP_CTRL_CTRL0(id_in_structure))) &
      IO_WKUP_CTRL_IO_WAKEUP_CTRL0_DEBOUNCE_EN_MSK)
    gpio_wakeup_elem->debounce_en = 1;
  else
    gpio_wakeup_elem->debounce_en = 0;

  if (REGISTER(TOPREG(IO_WKUP_CTRL_WAKEUP_EN(id_in_structure))) &
      IO_WKUP_CTRL_IO_WAKEUP_EN_WAKEUP_EN_MSK)
    gpio_wakeup_elem->wakeup_en = 1;
  else
    gpio_wakeup_elem->wakeup_en = 0;

  if (REGISTER(TOPREG(WKUP_PROCESSORS_PMG_WAKEUP_MASK_MCU0)) &
      (1 << (id_in_structure + GPIO_WAKEUP_OFFSET_IN_WAKEUP_VECTOR)))
    gpio_wakeup_elem->int_en = 0;
  else
    gpio_wakeup_elem->int_en = 1;

  gpio_wakeup_elem->gpio_pin = (REGISTER(TOPREG(IO_WKUP_CTRL_CTRL1(id_in_structure))) &
                                IO_WKUP_CTRL_IO_WAKEUP_CTRL1_WAKEUP_IO_SEL_MSK) >>
                               IO_WKUP_CTRL_IO_WAKEUP_CTRL1_WAKEUP_IO_SEL_POS;
  gpio_wakeup_elem->debounce_max_val = (REGISTER(TOPREG(IO_WKUP_CTRL_CTRL1(id_in_structure))) &
                                        IO_WKUP_CTRL_IO_WAKEUP_CTRL1_DEBOUNCE_MAX_VAL_MSK);

  /* Polarity/Edge understanding */
  /* Check whether edge or high/low functionality configured */
  if (((REGISTER(TOPREG(IO_WKUP_CTRL_CTRL0(id_in_structure))) &
        IO_WKUP_CTRL_IO_WAKEUP_CTRL0_EDGE_EN_MSK) == 0)) {
    if (((REGISTER(TOPREG(IO_WKUP_CTRL_CTRL0(id_in_structure))) &
          IO_WKUP_CTRL_IO_WAKEUP_CTRL0_POL_CHANGE_MSK) >>
         IO_WKUP_CTRL_IO_WAKEUP_CTRL0_POL_CHANGE_POS) ==
        0) {  // 0 - polarity high, 1 - polarity low
      gpio_wakeup_elem->mode = GPIO_WAKEUP_LEVEL_POL_HIGH;
    } else {
      gpio_wakeup_elem->mode = GPIO_WAKEUP_LEVEL_POL_LOW;
    }
  } else {
    switch ((REGISTER(TOPREG(IO_WKUP_CTRL_CTRL0(id_in_structure))) &
             IO_WKUP_CTRL_IO_WAKEUP_CTRL0_EDGE_KIND_MSK) >>
            IO_WKUP_CTRL_IO_WAKEUP_CTRL0_EDGE_KIND_POS) {
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

/*prints structure, name and value*/
void print_struct(gpio_wakeup_struct_t* struct_to_print) {
  printf(" pin # %d \n", struct_to_print->gpio_pin);
  printf(" clk %u \n", struct_to_print->clk);
  printf(" debounce_en %u \n", struct_to_print->debounce_en);
  printf(" debounce_max_val %u \n", struct_to_print->debounce_max_val);
  printf(" int_en %u \n", struct_to_print->int_en);
  printf(" mode %u \n", struct_to_print->mode);
  printf(" wakeup_en %u \n", struct_to_print->wakeup_en);
}

unsigned int get_interrupt_count(unsigned int pin_number) {
  int id_in_structure;

  id_in_structure = gpio_wakeup_find_io(pin_number);
  if (id_in_structure < 0)
    return -1;
  else
    return gpio_wakeup_info[id_in_structure].int_counter;
}

/*Disables wakeup for specified IO in wakeup machine*/
int gpio_wakeup_disable(unsigned int pin_number) {
  int id_in_structure;

  id_in_structure = gpio_wakeup_find_io(pin_number);
  if (id_in_structure < 0) return -1;

  REGISTER(TOPREG(IO_WKUP_CTRL_WAKEUP_EN(id_in_structure))) &=
      ~IO_WKUP_CTRL_IO_WAKEUP_EN_WAKEUP_EN_MSK;
  g_temp = REGISTER(TOPREG(IO_WKUP_CTRL_WAKEUP_EN(id_in_structure)));  // make sure its written

  gpio_wakeup_info[id_in_structure].wakeup_en = 0;
  return 0;
}

/*Enables wakeup for specified IO in wakeup machine*/
int gpio_wakeup_enable(unsigned int pin_number) {
  int id_in_structure;

  id_in_structure = gpio_wakeup_find_io(pin_number);
  if (id_in_structure < 0) return -1;

  REGISTER(TOPREG(IO_WKUP_CTRL_WAKEUP_EN(id_in_structure))) |=
      IO_WKUP_CTRL_IO_WAKEUP_EN_WAKEUP_EN_MSK;
  g_temp = REGISTER(TOPREG(IO_WKUP_CTRL_WAKEUP_EN(id_in_structure)));  // make sure its written

  gpio_wakeup_info[id_in_structure].wakeup_en = 1;

  return 0;
}

/*Disables wakeup for specified IO in wakeup machine,
 *  removes member from auxiliary array.*/
int gpio_wakeup_delete_io(unsigned int pin_number) {
  int id_in_structure;

  id_in_structure = gpio_wakeup_find_io(pin_number);
  if (id_in_structure < 0) return -1;

  gpio_wakeup_disable(pin_number);
  REGISTER(TOPREG(IO_WKUP_CTRL_CTRL1(id_in_structure))) |=
      ((0 << IO_WKUP_CTRL_IO_WAKEUP_CTRL1_WAKEUP_IO_SEL_POS) &
       IO_WKUP_CTRL_IO_WAKEUP_CTRL1_WAKEUP_IO_SEL_MSK);
  gpio_wakeup_info[id_in_structure].wakeup_en = 0;
  gpio_wakeup_info[id_in_structure].gpio_pin = -1;
  wakeup_info_lookup[pin_number] = GPIO_WAKEUP_FORBIDDEN_WAKEUP_PIN;

  return 0;
}

/*Select polarity mode for wakeup (HIGH – mode=0, LOW – mode=1,
 *  RISING - mode=2, FALLING - mode=3, RISING_FALLING - mode=4)*/
int gpio_wakeup_polarity_set(unsigned int pin_number, gpio_wakeup_polarity_mode_e mode) {
  int id_in_structure;
  uint32_t temp_reg;

  id_in_structure = gpio_wakeup_find_io(pin_number);
  if (id_in_structure < 0) return -1;

  gpio_wakeup_disable(pin_number);

  temp_reg = REGISTER(TOPREG(IO_WKUP_CTRL_CTRL0(id_in_structure)));

  /*wakeup mode selection*/
  switch (mode) {
    case GPIO_WAKEUP_LEVEL_POL_HIGH: /*polarity for level wakeup 0 = high*/
      temp_reg &= ~IO_WKUP_CTRL_IO_WAKEUP_CTRL0_EDGE_EN_MSK;
      temp_reg &= ~IO_WKUP_CTRL_IO_WAKEUP_CTRL0_POL_CHANGE_MSK;
      break;

    case GPIO_WAKEUP_LEVEL_POL_LOW: /*polarity for level wakeup 1 = low*/
      temp_reg &= ~IO_WKUP_CTRL_IO_WAKEUP_CTRL0_EDGE_EN_MSK;
      temp_reg |= IO_WKUP_CTRL_IO_WAKEUP_CTRL0_POL_CHANGE_MSK;
      break;

    case GPIO_WAKEUP_EDGE_RISING: /*0 in edge_kind register*/
      temp_reg |= IO_WKUP_CTRL_IO_WAKEUP_CTRL0_EDGE_EN_MSK;
      temp_reg &= ~IO_WKUP_CTRL_IO_WAKEUP_CTRL0_EDGE_KIND_MSK;
      break;

    case GPIO_WAKEUP_EDGE_FALLING: /*1 in edge_kind register*/
      temp_reg |= IO_WKUP_CTRL_IO_WAKEUP_CTRL0_EDGE_EN_MSK;
      temp_reg &= ~IO_WKUP_CTRL_IO_WAKEUP_CTRL0_EDGE_KIND_MSK;
      temp_reg |= (GPIO_WAKEUP_MODE_EDGE_FALLING << IO_WKUP_CTRL_IO_WAKEUP_CTRL0_EDGE_KIND_POS);
      break;

    case GPIO_WAKEUP_EDGE_RISING_FALLING: /*2 in edge_kind register*/
      temp_reg |= IO_WKUP_CTRL_IO_WAKEUP_CTRL0_EDGE_EN_MSK;
      temp_reg &= ~IO_WKUP_CTRL_IO_WAKEUP_CTRL0_EDGE_KIND_MSK;
      temp_reg |= (GPIO_WAKEUP_MODE_EDGE_TOGGLE << IO_WKUP_CTRL_IO_WAKEUP_CTRL0_EDGE_KIND_POS);
      break;

    default:
      return -3; /*incorrect wakeup mode */
  }

  REGISTER(TOPREG(IO_WKUP_CTRL_CTRL0(id_in_structure))) = temp_reg;
  gpio_wakeup_info[id_in_structure].mode = mode;

  gpio_wakeup_enable(pin_number);

  return 0;
}

/*Select clock (RTC – clk_id=0, URCLK – clk_id=1, ASYNC – clk_id=2)*/
int gpio_wakeup_clk_select(unsigned int pin_number, gpio_wakeup_clk_e clk_id) {
  int id_in_structure;
  uint32_t temp_reg, temp_reg_urclk;

  id_in_structure = gpio_wakeup_find_io(pin_number);
  if (id_in_structure < 0) return -1;

  gpio_wakeup_disable(pin_number);
  temp_reg = REGISTER(TOPREG(IO_WKUP_CTRL_CTRL0(id_in_structure)));

  /*Clock select*/
  switch (clk_id) {
    case GPIO_WAKEUP_ASYNC_CLK: /*polarity for level wakeup 0 = high*/
      temp_reg |= IO_WKUP_CTRL_IO_WAKEUP_CTRL0_ASYNC_WAKEUP_MSK;
      break;

    case GPIO_WAKEUP_RTC_CLK: /*polarity for level wakeup 0 = high*/
      temp_reg &= ~IO_WKUP_CTRL_IO_WAKEUP_CTRL0_ASYNC_WAKEUP_MSK;
      temp_reg &= ~IO_WKUP_CTRL_IO_WAKEUP_CTRL0_CLK_SELECT_MSK;
      break;

    case GPIO_WAKEUP_URCLK_CLK: /*polarity for level wakeup 0 = high*/
      if ((REGISTER(TOPREG(URCLK_CFG_URCLK_CFG)) & URCLK_CFG_URCLK_CFG_RING_OSC_UR_EN_MSK) == 0) {
        temp_reg_urclk = REGISTER(TOPREG(URCLK_CFG_URCLK_CFG));
        temp_reg_urclk |= URCLK_CFG_URCLK_CFG_RING_OSC_UR_EN_MSK;
        REGISTER(TOPREG(URCLK_CFG_URCLK_CFG)) = temp_reg_urclk;
      }
      temp_reg &= ~IO_WKUP_CTRL_IO_WAKEUP_CTRL0_ASYNC_WAKEUP_MSK;
      temp_reg |= IO_WKUP_CTRL_IO_WAKEUP_CTRL0_CLK_SELECT_MSK;
      break;

    default:
      return -3; /*incorrect wakeup mode */
  }

  REGISTER(TOPREG(IO_WKUP_CTRL_CTRL0(id_in_structure))) = temp_reg;
  gpio_wakeup_info[id_in_structure].clk = clk_id;

  gpio_wakeup_enable(pin_number);

  return 0;
}

/*Enables debounce for specified IO, returns zero in
 * case of success and -1 in case this IO is not registered as wakeup.*/
int gpio_wakeup_debounce_enable(unsigned int pin_number) {
  int id_in_structure;
  uint32_t temp_reg;

  id_in_structure = gpio_wakeup_find_io(pin_number);

  /* check valid range of with-debounce */
  if (id_in_structure < 2 || id_in_structure > 3) return -1;

  /* In case that specific io is still not registered as wakeup
   * no need to disable it, because it is not enabled yet.
   * Enabling will be done in the end of the add_io function.
   * But if user wishes to modify some parameters after
   * registration, wakeup machine should be disabled and enabled
   * after modification*/
  gpio_wakeup_disable(pin_number);

  temp_reg = REGISTER(TOPREG(IO_WKUP_CTRL_CTRL0(id_in_structure)));
  temp_reg |= IO_WKUP_CTRL_IO_WAKEUP_CTRL0_DEBOUNCE_EN_MSK;
  REGISTER(TOPREG(IO_WKUP_CTRL_CTRL0(id_in_structure))) = temp_reg;

  gpio_wakeup_info[id_in_structure].debounce_en = 1;

  gpio_wakeup_enable(pin_number);

  return 0;
}

/* Disables debounce for specified IO, returns zero in
 * case of success and -1 in case this IO is not registered as wakeup.*/
int gpio_wakeup_debounce_disable(unsigned int pin_number) {
  int id_in_structure;
  uint32_t temp_reg;

  id_in_structure = gpio_wakeup_find_io(pin_number);

  /* check valid range of with-debounce */
  if (id_in_structure < 2 || id_in_structure > 3) return -1;

  gpio_wakeup_disable(pin_number);
  temp_reg = REGISTER(TOPREG(IO_WKUP_CTRL_CTRL0(id_in_structure)));
  temp_reg &= ~IO_WKUP_CTRL_IO_WAKEUP_CTRL0_DEBOUNCE_EN_MSK;

  REGISTER(TOPREG(IO_WKUP_CTRL_CTRL0(id_in_structure))) = temp_reg;
  gpio_wakeup_info[id_in_structure].debounce_en = 0;

  gpio_wakeup_enable(pin_number);

  return 0;
}

/*Sets number of ckocks that signal should stay before change*/
int gpio_wakeup_debounce_set_val(unsigned int pin_number, unsigned int val) {
  int id_in_structure;
  uint32_t temp_reg;

  id_in_structure = gpio_wakeup_find_io(pin_number);

  /* check valid range of with-debounce */
  if (id_in_structure < 2 || id_in_structure > 3) return -1;

  gpio_wakeup_disable(pin_number);
  temp_reg = REGISTER(TOPREG(IO_WKUP_CTRL_CTRL1(id_in_structure)));
  temp_reg &= ~IO_WKUP_CTRL_IO_WAKEUP_CTRL1_DEBOUNCE_MAX_VAL_MSK;
  temp_reg |= (val << IO_WKUP_CTRL_IO_WAKEUP_CTRL1_DEBOUNCE_MAX_VAL_POS) &
              IO_WKUP_CTRL_IO_WAKEUP_CTRL1_DEBOUNCE_MAX_VAL_MSK;
  REGISTER(TOPREG(IO_WKUP_CTRL_CTRL1(id_in_structure))) = temp_reg;
  gpio_wakeup_info[id_in_structure].debounce_max_val = val;

  gpio_wakeup_enable(pin_number);

  return 0;
}

/*Enable interrupt for specified IO, returns zero in
 *  case of success and -1 in case this IO is not registered as wakeup.*/
int gpio_wakeup_interrupt_enable(unsigned int pin_number) {
  int id_in_structure;

  id_in_structure = gpio_wakeup_find_io(pin_number);
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
  if ((REGISTER(TOPREG(IO_WKUP_CTRL_CTRL0(id_in_structure))) &
       IO_WKUP_CTRL_IO_WAKEUP_CTRL0_EDGE_EN_MSK) == 0) {
    /*Disable intrrupt*/
    REGISTER(TOPREG(IO_WKUP_CTRL_INT_EN(id_in_structure))) &=
        ~IO_WKUP_CTRL_IO_WAKEUP_INT_EN_INT_EN_MSK;
    /*Disable wakeup machine*/
    REGISTER(TOPREG(IO_WKUP_CTRL_WAKEUP_EN(id_in_structure))) &=
        ~IO_WKUP_CTRL_IO_WAKEUP_EN_WAKEUP_EN_MSK;
    /*Enable wakeup machine*/
    REGISTER(TOPREG(IO_WKUP_CTRL_WAKEUP_EN(id_in_structure))) |=
        IO_WKUP_CTRL_IO_WAKEUP_EN_WAKEUP_EN_MSK;
    /*Enable interrupt*/
    REGISTER(TOPREG(IO_WKUP_CTRL_INT_EN(id_in_structure))) |=
        IO_WKUP_CTRL_IO_WAKEUP_INT_EN_INT_EN_MSK;
  }

  /* Unmask specific wakeup source for interrupts*/
  REGISTER(TOPREG(WKUP_PROCESSORS_PMG_WAKEUP_MASK_MCU0)) &=
      ~(1 << (id_in_structure + GPIO_WAKEUP_OFFSET_IN_WAKEUP_VECTOR));

  return 0;
}

/*Disables interrupt for specified IO, returns zero
 * in case of success and -1 in case this IO is not registered as wakeup.*/
/* Fix for Jira PRODUCTS-16831:
 * Problem: sometimes gpio interrupt received as unhandled and are not recognized
 * Cause  : gpio input interrupt is triggered from external source while (or close to) a
 * "gpio_disable_interrupt" was called for that source (to clear the mask register). The execution
 * of disabling the interrupt source is handled in lower-level bus (slow bus) which takes few 100MHz
 * clocks to happen. In parallel the interrupt handler called and checks for the interrupt source,
 * no source is identified since the mask register above impacts the actual output of the interrupt.
 * (it would be better of the mask would not affect pending interrupts, so it would mask the input
 * of the gpio instead of the output as it is now) Fix    : when calling to disable gpio source
 * interrupt do it as follows: do it in loop of 5 disable global interrupt (critical section) mask
 * specific source interrupt check specific source interrupt if (interrupt status is pending) -> it
 * means we need to execute the interrupt enable specific interrupt source enable global interrupts
 * (interrupt handler would be called here else enable global interrupts
 */

int gpio_wakeup_interrupt_disable(unsigned int pin_number) {
  int id_in_structure;
  int count = 5;

  id_in_structure = gpio_wakeup_find_io(pin_number);
  if (id_in_structure < 0) return -1;

  // if already disabled - return
  if (!gpio_wakeup_info[id_in_structure].int_en) {
    g_temp = REGISTER(TOPREG(WKUP_PROCESSORS_PMG_WAKEUP_MASK_MCU0));
    if ((g_temp & (1 << (id_in_structure + GPIO_WAKEUP_OFFSET_IN_WAKEUP_VECTOR))) == 1) {
      return 0;
    }
  }

  // limit retry times
  while (--count) {
    if (xPortIsInsideInterrupt() == pdTRUE) {
      /* Mask specific wakeup source for interrupts*/
      REGISTER(TOPREG(WKUP_PROCESSORS_PMG_WAKEUP_MASK_MCU0)) |=
          (1 << (id_in_structure + GPIO_WAKEUP_OFFSET_IN_WAKEUP_VECTOR));
      g_temp = REGISTER(TOPREG(
          WKUP_PROCESSORS_PMG_WAKEUP_MASK_MCU0));  // read mask register - make sure its written

      gpio_wakeup_info[id_in_structure].int_en = 0;

      return 0;
    }
    taskENTER_CRITICAL();

    /* Mask specific wakeup source for interrupts*/
    REGISTER(TOPREG(WKUP_PROCESSORS_PMG_WAKEUP_MASK_MCU0)) |=
        (1 << (id_in_structure + GPIO_WAKEUP_OFFSET_IN_WAKEUP_VECTOR));
    g_temp = REGISTER(TOPREG(
        WKUP_PROCESSORS_PMG_WAKEUP_MASK_MCU0));  // read mask register - make sure its written

    // check if interrupt arrived due to the race condition
    if ((REGISTER(TOPREG(IO_WKUP_CTRL_INT(id_in_structure))) &
         IO_WKUP_CTRL_IO_WAKEUP_INT_INT_GPMCLK_MSK)) {
      // enable back interrupt (and try again)
      REGISTER(TOPREG(WKUP_PROCESSORS_PMG_WAKEUP_MASK_MCU0)) &=
          ~(1 << (id_in_structure + GPIO_WAKEUP_OFFSET_IN_WAKEUP_VECTOR));

      taskEXIT_CRITICAL();

      g_temp =
          REGISTER(TOPREG(WKUP_PROCESSORS_PMG_WAKEUP_MASK_MCU0));  // dummy read - give some time
    } else {
      // normal case - continue normally
      gpio_wakeup_info[id_in_structure].int_en = 0;

      taskEXIT_CRITICAL();

      return 0;
    }
  }
  if (count == 0) {
    printf("ERROR: gpio %d disable interrupt did not ended!\n", pin_number);
  }

  gpio_wakeup_info[id_in_structure].int_en = 0;

  return 0;
}

/*  Writes the IO as wakeup and puts values in registers,
 *  returns zero in case of success and error code otherwise.
 *  Also adds new member in auxiliary array.*/
int gpio_wakeup_add_io(gpio_wakeup_struct_t* params) {
  int free_pin_id;

  if (gpio_wakeup_find_io(params->gpio_pin) >= 0) return -1;

  /* if all wakeups occupied */
  free_pin_id = gpio_wakeup_check_availability(params->debounce_en);
  if (free_pin_id < 0) return -3;

  unsigned int wakeup_id;
  if (params->gpio_pin >= (int)(sizeof(gpio_to_wakeup_lookup) / sizeof(gpio_to_wakeup_lookup[0])))
    return -4;
  wakeup_id = gpio_to_wakeup_lookup[params->gpio_pin];
  if (wakeup_id == GPIO_WAKEUP_FORBIDDEN_WAKEUP_PIN) return -5;

  wakeup_info_lookup[params->gpio_pin] = free_pin_id;

  gpio_wakeup_info[free_pin_id].gpio_pin = params->gpio_pin;

  /* pin select */
  REGISTER(TOPREG(IO_WKUP_CTRL_CTRL1(free_pin_id))) |=
      ((wakeup_id << IO_WKUP_CTRL_IO_WAKEUP_CTRL1_WAKEUP_IO_SEL_POS) &
       IO_WKUP_CTRL_IO_WAKEUP_CTRL1_WAKEUP_IO_SEL_MSK);

  /* debounce */
  if (params->debounce_en > 0) {
    gpio_wakeup_debounce_enable(params->gpio_pin);
    gpio_wakeup_debounce_set_val(params->gpio_pin, params->debounce_max_val);
  } else
    gpio_wakeup_debounce_disable(params->gpio_pin);

  /* wakeup mode selection */
  gpio_wakeup_polarity_set(params->gpio_pin, params->mode);

  /* Interrupt enable */
  if (params->int_en)
    gpio_wakeup_interrupt_enable(params->gpio_pin);
  else
    gpio_wakeup_interrupt_disable(params->gpio_pin);

  /* Clock select */
  gpio_wakeup_clk_select(params->gpio_pin, params->clk);

  /* Wakeup enable */
  if (params->wakeup_en > 0) {
    gpio_wakeup_enable(params->gpio_pin);
  } else {
    gpio_wakeup_disable(params->gpio_pin);
  }

  return 0;
}

/* Fill the gpio_wakeup_info structure .Used after the wakeup */
int gpio_wakeup_restore(void) {
  int i, warm_wakeup = -1;
  unsigned int pin_number;

  /* Initialize wakeup_info_lookup tab */
  if (wakeup_info_lookup[0] == 254 && wakeup_info_lookup[1] == 254)
    memset(wakeup_info_lookup, GPIO_WAKEUP_FORBIDDEN_WAKEUP_PIN, sizeof(wakeup_info_lookup));

  /* Inspect all the sources allocated for MCU processor (2,3,6,7)
   * and fill the structure according to the registers*/
  for (i = 0; i < GPIO_WAKEUP_MODEM_SOURCE_SIZE; i++) {
    if ((i != 2) && (i != 3) && (i != 6) && (i != 7))  // 4th source is reserved for PMP processor
      continue;                                        // 2,3,6,7 are reserved for MCU

    if (REGISTER(TOPREG(IO_WKUP_CTRL_WAKEUP_EN(i)))) {
      warm_wakeup = 0;
      pin_number = (REGISTER(TOPREG(IO_WKUP_CTRL_CTRL1(i))) &
                    IO_WKUP_CTRL_IO_WAKEUP_CTRL1_WAKEUP_IO_SEL_MSK) >>
                   IO_WKUP_CTRL_IO_WAKEUP_CTRL1_WAKEUP_IO_SEL_POS;

      /* fill the temporary structure from the registers */
      gpio_wakeup_get_setup(gpio_wakeup_info + i, pin_number, GPIO_WAKEUP_REGISTER_LOOKUP);

      if (pin_number >= (sizeof(wakeup_to_gpio_lookup) / sizeof(wakeup_to_gpio_lookup[0])))
        continue;
      pin_number = wakeup_to_gpio_lookup[pin_number];
      gpio_wakeup_info[i].gpio_pin = pin_number;

      wakeup_info_lookup[pin_number] = i;
      /* even if interrupt is disabled it still need to be handled
       * in wakeup machine*/
      gpio_wakeup_info[i].irq_handler = (gpio_interrupt_handler)gpio_wakeup_dummy_interrupt;
      gpio_wakeup_info[i].user_param = 0;

      /*another level of protection, in case there is enabled  wakeup
       * machine, but no valid pin assigned to it*/
    } else {
      gpio_wakeup_info[i].gpio_pin = -1;
    }
  }
  return warm_wakeup;
}

/* Because of bug in hardware ,disabling interrupt can't be done
 * by IO_WKUP_CTRL_INT_EN register. Interrupt need to be masked
 * to MCU processor. In this case if there is interrupt latched,
 * it need to be cleared before going to sleep.
 * */
void gpio_wakeup_clr_interrupts_before_sleep(void) {
  unsigned int i;

  for (i = 0; i < GPIO_WAKEUP_MODEM_SOURCE_SIZE; i++) {
    if ((i != 2) && (i != 3) && (i != 6) && (i != 7)) continue;  // 2,3,6,7 are reserved for MCU
    if (((REGISTER(TOPREG(WKUP_PROCESSORS_PMG_WAKEUP_MASK_MCU0)) &
          (1 << (i + GPIO_WAKEUP_OFFSET_IN_WAKEUP_VECTOR))) != 0) &&
        REGISTER(TOPREG(IO_WKUP_CTRL_WAKEUP_EN(i)))) {
      /*Disable intrrupt*/
      REGISTER(TOPREG(IO_WKUP_CTRL_INT_EN(i))) &= ~IO_WKUP_CTRL_IO_WAKEUP_INT_EN_INT_EN_MSK;
      /*Disable wakeup machine*/
      REGISTER(TOPREG(IO_WKUP_CTRL_WAKEUP_EN(i))) &= ~IO_WKUP_CTRL_IO_WAKEUP_EN_WAKEUP_EN_MSK;
      delayus(26000000, 40);
      /*Enable wakeup machine*/
      REGISTER(TOPREG(IO_WKUP_CTRL_WAKEUP_EN(i))) |= IO_WKUP_CTRL_IO_WAKEUP_EN_WAKEUP_EN_MSK;
      /*Enable interrupt*/
      REGISTER(TOPREG(IO_WKUP_CTRL_INT_EN(i))) |= IO_WKUP_CTRL_IO_WAKEUP_INT_EN_INT_EN_MSK;
    }
  }
}

void gpio_wakeup_init(void) {
  int i;
  int32_t temp_for_sleep_notify;

  for (i = 0; i < GPIO_WAKEUP_MODEM_SOURCE_SIZE; i++) {
    if ((i != 2) && (i != 3) && (i != 6) && (i != 7))  // 2,3,6,7 are reserved for MCU
      continue;
    else
      gpio_wakeup_info[i].gpio_pin = -1;
  }

  gpio_wakeup_restore();
  sleep_notify_insert_callback_item(&gpio_wakeup_notify, &temp_for_sleep_notify, NULL);

  NVIC_SetPriority(WAKEUP_IRQn, 7); /* set Interrupt priority */
  NVIC_EnableIRQ(WAKEUP_IRQn);
}

/* Because of hardware fault, interrupt need to be cleared by
 * disabling both interrupt_enable and wakeup_enable */
void gpio_wakeup_IRQHandler(void) {
  unsigned int i;

  /* Inspect all the sources allocated for NET processor (0-3,5-7)
   * and fill the structure according to the registers*/
  for (i = 0; i < GPIO_WAKEUP_MODEM_SOURCE_SIZE; i++) {
    if ((i != 2) && (i != 3) && (i != 6) && (i != 7)) continue;
    /* check all conditions - wakeup enable , interrupt enable and raised interrupt flag
     * Flow that takes place below is the right way to deal with interrupt
     * Description in of the flow is in the function's description*/
    if ((REGISTER(TOPREG(IO_WKUP_CTRL_WAKEUP_EN(i))) /*&& REGISTER(TOPREG(IO_WKUP_CTRL_INT_EN(i))*/
         && (REGISTER(TOPREG(IO_WKUP_CTRL_INT(i))) & IO_WKUP_CTRL_IO_WAKEUP_INT_INT_GPMCLK_MSK)) &&
        ((REGISTER(TOPREG(WKUP_PROCESSORS_PMG_WAKEUP_MASK_MCU0)) &
          (1 << (i + GPIO_WAKEUP_OFFSET_IN_WAKEUP_VECTOR))) == 0)) {
      /*Disable intrrupt*/
      REGISTER(TOPREG(IO_WKUP_CTRL_INT_EN(i))) &= ~IO_WKUP_CTRL_IO_WAKEUP_INT_EN_INT_EN_MSK;

      /*Disable wakeup machine*/
      REGISTER(TOPREG(IO_WKUP_CTRL_WAKEUP_EN(i))) &= ~IO_WKUP_CTRL_IO_WAKEUP_EN_WAKEUP_EN_MSK;

      /* delay of 3 ticks of 32K pmp timer */
      delayus(26000000, 40);

      /*Enable wakeup machine*/
      REGISTER(TOPREG(IO_WKUP_CTRL_WAKEUP_EN(i))) |= IO_WKUP_CTRL_IO_WAKEUP_EN_WAKEUP_EN_MSK;

      /*Run user interrupt routine*/
      if (gpio_wakeup_info[i].irq_handler && gpio_wakeup_info[i].int_en) {
        gpio_wakeup_info[i].irq_handler(gpio_wakeup_info[i].user_param);
      } else {
        gpio_wakeup_interrupt_disable(gpio_wakeup_info[i].gpio_pin);
      }

      gpio_wakeup_info[i].int_counter++;

      /*Enable interrupt*/
      REGISTER(TOPREG(IO_WKUP_CTRL_INT_EN(i))) |= IO_WKUP_CTRL_IO_WAKEUP_INT_EN_INT_EN_MSK;
    }
  }
}

/* this function is registered in sleep-manager clear of
 * masked interrupts should be done before going to
 * sleep and should not be done on the wakeup*/
int gpio_wakeup_notify(sleep_notify_state sleep_state, void* ptr_ctx) {
  if (sleep_state == SUSPENDING) {
    /* These seetings may be stored before/restored after sleep */

    gpio_wakeup_clr_interrupts_before_sleep();
  } else if (sleep_state == RESUMING) {
    /* These seetings may be stored before/restored after sleep */
  }

  return SLEEP_NOTIFY_SUCCESS;
}

/*before registering interrupt all handlers should be assigned*/
int gpio_wakeup_register_interrupt(unsigned int gpio, gpio_interrupt_handler irq_handler,
                                   unsigned int user_param) {
  if (!gpio_wakeup_irq_configured) {
    int i;
    for (i = 0; i < GPIO_WAKEUP_MODEM_SOURCE_SIZE; i++) {
      gpio_wakeup_info[i].int_en = 0;
      gpio_wakeup_info[i].irq_handler = NULL;
      gpio_wakeup_info[i].user_param = 0;
    }
    gpio_wakeup_irq_configured = 1;
  }
  int id_in_structure;

  id_in_structure = gpio_wakeup_find_io(gpio);
  if (id_in_structure < 0)
    return -1;
  else {
    gpio_wakeup_info[id_in_structure].int_counter = 0;
    gpio_wakeup_info[id_in_structure].irq_handler = irq_handler;
    gpio_wakeup_info[id_in_structure].user_param = user_param;
  }
  return 0;
}

void gpio_wakeup_dummy_interrupt(unsigned int param) { return; }
