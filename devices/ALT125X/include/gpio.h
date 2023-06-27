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
/**
 * @file gpio.h
 */
#ifndef GPIO_ALT125X_H_
#define GPIO_ALT125X_H_
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "if_mngr.h"

/**
 * @defgroup gpio_driver GPIO Driver
 * @{
 */
/****************************************************************************
 * Data types
 ****************************************************************************/
/**
 * @defgroup gpio_data_types GPIO Types
 * @{
 */
/**
 * @typedef eGpioDir
 * Definition of GPIO direction
 */
typedef enum {
  GPIO_DIR_INPUT = 0, /**< GPIO direction input */
  GPIO_DIR_OUTPUT     /**< GPIO direction output */
} eGpioDir;
/**
 * @typedef eGpioPullMode
 * Definition of GPIO direction
 */
typedef enum {
  GPIO_PULL_NONE = 0,   /**< GPIO no pull */
  GPIO_PULL_UP,         /**< GPIO pull up */
  GPIO_PULL_DOWN,       /**< GPIO pull down */
  GPIO_PULL_DONT_CHANGE /**< Do not change GPIO pull setting. */
} eGpioPullMode;
/**
 * @typedef eGpioRet
 * Definition of GPIO API return code
 */
typedef enum {
  GPIO_RET_SUCCESS,      /**< GPIO API returns with no error */
  GPIO_RET_ERR,          /**< GPIO API returns with generic error */
  GPIO_RET_INVALID_PORT, /**< GPIO API returns with invalid port setting in inteface setting*/
  GPIO_RET_OUT_OF_RANGE, /**< GPIO API parameter out of range */
  GPIO_RET_FORBIDDEN,    /**< GPIO API returns with wrong io_par setting */
  GPIO_RET_INIT_ERR,     /**< GPIO not init error */
  GPIO_RET_NO_RESOURCE   /**< No free GPIO ID for gpio_open */
} eGpioRet;
/**
 * @typedef eGpioIrqMode
 * Definition of GPIO irq polarity setting.
 */
typedef enum {
  GPIO_IRQ_RISING_EDGE = 0, /**< GPIO interrupt trigger on rising edge */
  GPIO_IRQ_FALLING_EDGE,    /**< GPIO interrupt trigger on falling edge */
  GPIO_IRQ_BOTH_EDGES,      /**< GPIO interrupt trigger on rising and falling edge */
  GPIO_IRQ_HIGH_LEVEL,      /**< GPIO interrupt trigger on high level */
  GPIO_IRQ_LOW_LEVEL        /**< GPIO interrupt trigger on low level */
} eGpioIrqMode;
/**
 * @typedef eMcuGpioIds
 * Definition of available virtual GPIO ID.
 * The mapping between virtual to physical pin set is defined in interface_config_alt12xx.h.
 * This can be adjustable when number of gpio to be extended, however the total number is fixed to
 * 10 in interface_config_alt1250.h. For the gpio ID larger than this, application should use
 * gpio_open to allocate one phyisical pin to map to gpio dynamically.
 */
typedef enum {
  MCU_GPIO_ID_UNDEF = 0,
  MCU_GPIO_ID_01, /**< MCU GPIO 1 */
  MCU_GPIO_ID_02, /**< MCU GPIO 2 */
  MCU_GPIO_ID_03, /**< MCU GPIO 3 */
  MCU_GPIO_ID_04, /**< MCU GPIO 4 */
  MCU_GPIO_ID_05, /**< MCU GPIO 5 */
  MCU_GPIO_ID_06, /**< MCU GPIO 6 */
  MCU_GPIO_ID_07, /**< MCU GPIO 7 */
  MCU_GPIO_ID_08, /**< MCU GPIO 8 */
  MCU_GPIO_ID_09, /**< MCU GPIO 9 */
  MCU_GPIO_ID_10, /**< MCU GPIO 10 */
  MCU_GPIO_ID_11, /**< MCU GPIO 11 */
  MCU_GPIO_ID_12, /**< MCU GPIO 12 */
  MCU_GPIO_ID_13, /**< MCU GPIO 13 */
  MCU_GPIO_ID_14, /**< MCU GPIO 14 */
  MCU_GPIO_ID_15, /**< MCU GPIO 15 */
  MCU_GPIO_ID_NUM /**< enum value to indicate the boundary of MCU GPIO ID. The total GPIO ID number
                     should be MCU_GPIO_ID_NUM - 1 */
} eMcuGpioIds;
/**
 * @typedef gpio_interrupt_handler
 * Definition of gpio interrupt callback function
 * User callback function type for gpio interrupt.
 * @param[in] user_param : Parameter for this callback.
 */
typedef void (*gpio_interrupt_handler)(unsigned int user_param);
/*! @cond Doxygen_Suppress */
/**
 * @typedef gpio_cfg_t
 * Definition of gpio configuration defined in interface_config_alt1250.h.
 * This is only used to get configuration from interface manager.
 * This is not used for any API parameters.
 */
typedef struct {
  eMcuPinIds pin_set;     /**< gpio pin set*/
  eGpioDir def_dir;       /**< Default gpio direction setting*/
  eGpioPullMode def_pull; /**< Default gpio pull setting*/
  int def_val;            /**< Default gpio value setting*/
} gpio_cfg_t;
/*! @endcond */
/** @} gpio_data_types */

/*******************************************************************************
 * API
 ******************************************************************************/
/**
 * @defgroup gpio_apis GPIO APIs
 * @{
 */
/**
 * @brief Create and initialize driver local resources including an internal mapping between GPIO
 * physical ID and virtual ID as defined in interfaces_config_alt1250.h. This API needs to be called
 * before any other API is executed.
 *
 * @return @ref eGpioRet.
 */
eGpioRet gpio_init();
/**
 * @brief Set GPIO direction as input
 *
 * @param [in] gpio: ID of type @ref eMcuGpioIds to specify the gpio id to manipulate.
 *
 * @return @ref eGpioRet
 */
eGpioRet gpio_set_direction_input(eMcuGpioIds gpio);
/**
 * @brief Set GPIO direction as output and set its value.
 *
 * @param [in] gpio: ID of type @ref eMcuGpioIds to specify the gpio id to manipulate.
 * @param [in] value: Output value of this gpio pin.
 *
 * @return @ref eGpioRet.
 */
eGpioRet gpio_set_direction_output_and_value(eMcuGpioIds gpio, int value);
/**
 * @brief Set GPIO direction as output.
 *
 * @param [in] gpio: ID of type @ref eMcuGpioIds to specify the gpio id to manipulate.
 *
 * @return @ref eGpioRet.
 */
eGpioRet gpio_set_direction_output(eMcuGpioIds gpio);
/**
 * @brief Get GPIO value
 *
 * @param [in]  gpio: ID of type @ref eMcuGpioIds to specify the gpio id to manipulate.
 * @param [out] value: Current value of this gpio pin.
 *
 * @return @ref eGpioRet.
 */
eGpioRet gpio_get_value(eMcuGpioIds gpio, int *value);
/**
 * @brief Set GPIO val
 *
 * @param [in] gpio: ID of type @ref eMcuGpioIds to specify the gpio id to manipulate.
 * @param [in] value: Output value of this gpio pin.
 *
 * @return @ref eGpioRet.
 */
eGpioRet gpio_set_value(eMcuGpioIds gpio, int value);
/**
 * @brief Set GPIO pull mode.
 *
 * @param [in] gpio: ID of type @ref eMcuGpioIds to specify the gpio id to manipulate.
 * @param [in] pullmode: Pull mode of type @ref eGpioPullMode to configure.
 * Can be GPIO_PULL_NONE, GPIO_PULL_UP, GPIO_PULL_DOWN.
 *
 * @return @ref eGpioRet.
 */
eGpioRet gpio_set_pull(eMcuGpioIds gpio, eGpioPullMode pullmode);
/**
 * @brief Get GPIO direction of type @ref eGpioDir.
 *
 * @param [in]  gpio: ID of type @ref eMcuGpioIds to specify the gpio id to manipulate.
 * @param [out] dir:  Get current direction setting of type @ref eGpioDir.
 * Can be GPIO_DIR_INPUT or GPIO_DIR_OUTPUT.
 *
 * @return @ref eGpioRet
 */
eGpioRet gpio_get_direction(eMcuGpioIds gpio, eGpioDir *dir);
/**
 * @brief Get GPIO pull mode.
 *
 * @param [in]  gpio: ID of type @ref eMcuGpioIds to specify the gpio id to manipulate.
 * @param [out] pullmode: Get current pullmode setting of type @ref eGpioPullMode.
 * Can be GPIO_PULL_NONE, GPIO_PULL_UP, GPIO_PULL_DOWN.
 *
 * @return @ref eGpioRet.
 */
eGpioRet gpio_get_pull(eMcuGpioIds gpio, eGpioPullMode *pullmode);
/**
 * @brief Configure a GPIO interrupt polarity and pull mode
 *
 * @param [in] gpio: ID of type @ref eMcuGpioIds to specify the gpio id to manipulate.
 * @param [in] irq_mode: Specify interrupt polarity with type @ref eGpioIrqMode.
 * @param [in] pullmode: Specify interrupt pin pull mode with type @ref eGpioPullMode.
 *
 * @return @ref eGpioRet
 */
eGpioRet gpio_config_interrupt(eMcuGpioIds gpio, eGpioIrqMode irq_mode, eGpioPullMode pullmode);
/**
 * @brief Register an interrupt handler to a specific GPIO.
 *
 * @param [in] gpio: ID of type @ref eMcuGpioIds to specify the gpio id to manipulate.
 * @param [in] irq_handler: User callback when gpio interrupt triggered.
 * @param [in] user_param: Parameter to pass to irq user callback.
 *
 * @return @ref eGpioRet.
 */
eGpioRet gpio_register_interrupt(eMcuGpioIds gpio, gpio_interrupt_handler irq_handler,
                                 int user_param);
/**
 * @brief Enable GPIO interrupt
 *
 * @param [in] gpio: ID of type @ref eMcuGpioIds to specify the gpio id to manipulate.
 *
 * @return @ref eGpioRet.
 */
eGpioRet gpio_enable_interrupt(eMcuGpioIds gpio);
/**
 * @brief Disable GPIO interrupt
 *
 * @param [in] gpio: ID of type @ref eMcuGpioIds to specify the gpio id to manipulate.
 *
 * @return @ref eGpioRet.
 */
eGpioRet gpio_disable_interrupt(eMcuGpioIds gpio);
/**
 * @brief Clear GPIO interrupt
 *
 * @param [in] gpio: ID of type @ref eMcuGpioIds to specify the gpio id to manipulate.
 *
 * @return @ref eGpioRet.
 */
eGpioRet gpio_clear_interrupt(eMcuGpioIds gpio);
/**
 * @brief Change a pin mux setting to GPIO dynamically and query the value of this pin.
 *        This API will do
 *        1. Backup the current mux setting of this pin.
 *        2. Change the mux setting to gpio.
 *        3. Change direction of this gpio to input.
 *        4. Configure the pull mode of this pin if required.
 *        5. Get current value of this pin.
 *        6. Restore the previous stored mux setting.
 *
 * @param [in]  pin_id: ID of type eMcuPinIds. This is the physical pin ID from ALT1250_GPIO1 to
 * ALT1250_GPIO78. Please check devices/ALT125X/include/ALT1250/if_cfg/general_config.h.
 * @param [in]  pullmode: Pull mode setting to dynamically changed GPIO with type @ref
 * eGpioPullMode. If pullmode is set to GPIO_PULL_DONT_CHANGE, pullmode will remains unchanged.
 * @param [out] value: Current value of this pin.
 *
 * @return @ref eGpioRet.
 */
eGpioRet gpio_query_pin_status(eMcuPinIds pin_id, eGpioPullMode pullmode, int *value);
/**
 * @brief Configure a physical PIN to GPIO dynamically.
 *        1. Check if there is any space left inside mapping table and return error if all entries
 * is used.
 *        2. Find a free virtual GPIO ID
 *        3. Store the original mux setting of the requested pin
 *        4. Set mux setting of the requested pin to GPIO
 *        5. Set pin mapping between this gpio id and requested pin.
 *        6. Return the virtual GPIO ID.
 *
 * @param [in]  pin_id: ID of type eMcuPinIds. This is the physical pin ID from ALT1250_GPIO1 to
 * ALT1250_GPIO78. Please check devices/ALT125X/include/ALT1250/if_cfg/general_config.h.
 * @param [out] gpio_id: Dynamically allocated GPIO ID of type @ref eMcuGpioIds, and can be used as
 * parameters of other GPIO APIs.
 *
 * @return @ref eGpioRet.
 */
eGpioRet gpio_open(eMcuPinIds pin_id, eMcuGpioIds *gpio_id);
/**
 * @brief Free a GPIO and restore the previous MUX setting.
 *        1.Check if the GPIO ID was allocated.
 *        2.Restore the original mux setting of this pin.
 *        3.Clean the pin mapping of gpio table.
 *
 * @param [in] gpio: ID of type @ref eMcuGpioIds to specify the gpio id to close.
 *
 * @return @ref eGpioRet.
 */
eGpioRet gpio_close(eMcuGpioIds gpio);
/** @} gpio_apis */
/** @} gpio_driver */

/*Internal API for if_mngr*/
/*! @cond Doxygen_Suppress */
eGpioRet gpio_get_map(eMcuGpioIds gpio, eMcuPinIds *pin_id);
/*! @endcond */
#endif /* GPIO_ALT125X_H_ */
