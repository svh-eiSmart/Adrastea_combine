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
/**
 * @file DRV_GPIO.h
 */
#ifndef DRV_GPIO_H_
#define DRV_GPIO_H_
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "PIN.h"
#include <stdint.h>
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
 * @typedef GPIO_Direction
 * Definition of GPIO direction
 */
typedef enum {
  GPIO_DIR_INPUT = 0, /**< GPIO direction input */
  GPIO_DIR_OUTPUT     /**< GPIO direction output */
} GPIO_Direction;
/**
 * @typedef GPIO_Pull
 * Definition of GPIO direction
 */
typedef enum {
  GPIO_PULL_NONE = 0,   /**< GPIO no pull */
  GPIO_PULL_UP,         /**< GPIO pull up */
  GPIO_PULL_DOWN,       /**< GPIO pull down */
  GPIO_PULL_DONT_CHANGE /**< Do not change GPIO pull setting. */
} GPIO_Pull;
/**
 * @typedef DRV_GPIO_Status
 * Definition of GPIO API return code
 */
typedef enum {
  DRV_GPIO_OK,                 /**< GPIO API returns with no error */
  DRV_GPIO_ERROR_GENERIC,      /**< GPIO API returns with generic error */
  DRV_GPIO_ERROR_INVALID_PORT, /**< GPIO API returns with invalid port setting in inteface setting*/
  DRV_GPIO_ERROR_PARAM,        /**< GPIO API parameter out of range */
  DRV_GPIO_ERROR_FORBIDDEN,    /**< GPIO API returns with wrong io_par setting */
  DRV_GPIO_ERROR_INIT,         /**< GPIO not init error */
  DRV_GPIO_ERROR_RESOURCE,     /**< No free GPIO ID for DRV_GPIO_Open */
  DRV_GPIO_ERROR_POWER         /**< Wrong power state */
} DRV_GPIO_Status;
/**
 * @typedef GPIO_IrqPolarity
 * Definition of GPIO irq polarity setting.
 */
typedef enum {
  GPIO_IRQ_RISING_EDGE = 0, /**< GPIO interrupt trigger on rising edge */
  GPIO_IRQ_FALLING_EDGE,    /**< GPIO interrupt trigger on falling edge */
  GPIO_IRQ_BOTH_EDGES,      /**< GPIO interrupt trigger on rising and falling edge */
  GPIO_IRQ_HIGH_LEVEL,      /**< GPIO interrupt trigger on high level */
  GPIO_IRQ_LOW_LEVEL        /**< GPIO interrupt trigger on low level */
} GPIO_IrqPolarity;
/**
 * @typedef GPIO_Id
 * Definition of available virtual GPIO ID.
 * The mapping between virtual to physical pin set is defined in interfaces_config_alt12xx.h.
 * This can be adjustable when number of gpio to be extended, however the total number is fixed to
 * 15 in interfaces_config_alt12xx.h. For the gpio ID larger than this, application should use
 * DRV_GPIO_Open to allocate one phyisical pin to map to GPIO_Id dynamically.
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
  MCU_GPIO_ID_MAX /**< enum value to indicate the boundary of MCU GPIO ID. The total GPIO ID number
                     should be MCU_GPIO_ID_MAX - 1 */
} GPIO_Id;
/**
 * @typedef DRV_GPIO_IrqCallback
 * Definition of gpio interrupt callback function
 * User callback function type for gpio interrupt.
 * @param[in] user_param : Parameter for this callback.
 */
typedef void (*DRV_GPIO_IrqCallback)(void *user_param);
/**
 * @struct GPIO_Param
 * Definition of gpio configuration defined in interfaces_config_alt12xx.h.
 * This is used to get configuration from DRV_IF_CFG.
 * This is also for configuration to DRV_GPIO_Open.
 */
typedef struct {
  MCU_PinId pin_set;        /**< GPIO pin set*/
  GPIO_Direction direction; /**< GPIO direction setting*/
  GPIO_Pull pull;           /**< GPIO pull setting*/
  uint8_t value;            /**< GPIO value setting*/
} GPIO_Param;
/**
 * @enum GPIO_PowerState
 * @brief Contains Power State modes
 */
typedef enum {
  GPIO_POWER_FULL, /**< Set-up peripheral for full operationnal mode. Can be called multiple times.
                      If the peripheral is already in this mode the function performs no operation
                      and returns with DRV_GPIO_OK. */
  GPIO_POWER_OFF   /**< terminates any pending operation and disables peripheral and related
                      interrupts. This is the state after device reset. */
} GPIO_PowerState;

/** @} gpio_data_types */

/*******************************************************************************
 * API
 ******************************************************************************/
/*! @cond Doxygen_Suppress */
#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif
/*! @endcond */
/**
 * @defgroup gpio_apis GPIO APIs
 * @{
 */
/**
 * @brief Create and initialize driver local resources including an internal mapping between
 * physical pin ID and virtual GPIO ID as defined in interface header. This API needs to be called
 * before any other API is executed.
 *
 * @return @ref DRV_GPIO_Status.
 */
DRV_GPIO_Status DRV_GPIO_Initialize(void);
/**
 * @brief Uninitialize driver local resources including an internal mapping between physical pin ID
 * and virtual GPIO ID as defined in interfaces header
 *
 * @return @ref DRV_GPIO_Status.
 */
DRV_GPIO_Status DRV_GPIO_Uninitialize(void);
/**
 * @brief Control GPIO interface power state.
 *
 * @param [in] state: Power state with type @ref GPIO_PowerState.
 *
 * @return @ref DRV_GPIO_Status
 */
DRV_GPIO_Status DRV_GPIO_PowerControl(GPIO_PowerState state);
/**
 * @brief Get GPIO value
 *
 * @param [in]  gpio: ID of type @ref GPIO_Id to specify the gpio id to manipulate.
 * @param [out] value: Current value of this gpio pin.
 *
 * @return @ref DRV_GPIO_Status.
 */
DRV_GPIO_Status DRV_GPIO_GetValue(GPIO_Id gpio, uint8_t *value);
/**
 * @brief Set GPIO value
 *
 * @param [in] gpio: ID of type @ref GPIO_Id to specify the gpio to manipulate.
 * @param [in] value: Output value of this gpio pin.
 *
 * @return @ref DRV_GPIO_Status.
 */
DRV_GPIO_Status DRV_GPIO_SetValue(GPIO_Id gpio, uint8_t value);
/**
 * @brief Set GPIO pull mode.
 *
 * @param [in] gpio: ID of type @ref GPIO_Id to specify the gpio to manipulate.
 * @param [in] pull: Pull mode of type @ref GPIO_Pull to configure.
 * Can be GPIO_PULL_NONE, GPIO_PULL_UP, GPIO_PULL_DOWN.
 *
 * @return @ref DRV_GPIO_Status.
 */
DRV_GPIO_Status DRV_GPIO_SetPull(GPIO_Id gpio, GPIO_Pull pull);
/**
 * @brief Get GPIO pull mode.
 *
 * @param [in]  gpio: ID of type @ref GPIO_Id to specify the gpio to manipulate.
 * @param [out] pull: Get current pullmode setting of type @ref GPIO_Pull.
 * Can be GPIO_PULL_NONE, GPIO_PULL_UP, GPIO_PULL_DOWN.
 *
 * @return @ref DRV_GPIO_Status.
 */
DRV_GPIO_Status DRV_GPIO_GetPull(GPIO_Id gpio, GPIO_Pull *pull);
/**
 * @brief Set GPIO direction
 *
 * @param [in] gpio: ID of type @ref GPIO_Id to specify the gpio to manipulate.
 * @param [in] direction: gpio direction with type @ref GPIO_Direction
 *
 * @return @ref DRV_GPIO_Status
 */
DRV_GPIO_Status DRV_GPIO_SetDirection(GPIO_Id gpio, GPIO_Direction direction);
/**
 * @brief Set GPIO direction as output and set its value.
 *
 * @param [in] gpio: ID of type @ref GPIO_Id to specify the gpio to manipulate.
 * @param [in] value: Output value of this gpio pin.
 *
 * @return @ref DRV_GPIO_Status.
 */
DRV_GPIO_Status DRV_GPIO_SetDirectionOutAndValue(GPIO_Id gpio, uint8_t value);
/**
 * @brief Get GPIO direction of type @ref GPIO_Direction.
 *
 * @param [in]  gpio: ID of type @ref GPIO_Id to specify the gpio to manipulate.
 * @param [out] direction:  Get current direction setting of type @ref GPIO_Direction.
 * Can be GPIO_DIR_INPUT or GPIO_DIR_OUTPUT.
 *
 * @return @ref DRV_GPIO_Status
 */
DRV_GPIO_Status DRV_GPIO_GetDirection(GPIO_Id gpio, GPIO_Direction *direction);
/**
 * @brief Configure a GPIO interrupt polarity and pull mode
 *
 * @param [in] gpio: ID of type @ref GPIO_Id to specify the gpio to manipulate.
 * @param [in] polarity: Specify interrupt polarity with type @ref GPIO_IrqPolarity.
 * @param [in] pull: Specify interrupt pin pull mode with type @ref GPIO_Pull.
 *
 * @return @ref DRV_GPIO_Status
 */
DRV_GPIO_Status DRV_GPIO_ConfigIrq(GPIO_Id gpio, GPIO_IrqPolarity polarity, GPIO_Pull pull);
/**
 * @brief Register an interrupt handler to a specific GPIO.
 *
 * @param [in] gpio: ID of type @ref GPIO_Id to specify the gpio to manipulate.
 * @param [in] irq_handler: User callback when gpio interrupt triggered.
 * @param [in] user_param: Parameter to pass to irq user callback.
 *
 * @return @ref DRV_GPIO_Status.
 */
DRV_GPIO_Status DRV_GPIO_RegisterIrqCallback(GPIO_Id gpio, DRV_GPIO_IrqCallback irq_handler,
                                             void *user_param);
/**
 * @brief Enable GPIO interrupt
 *
 * @param [in] gpio: ID of type @ref GPIO_Id to specify the gpio to manipulate.
 *
 * @return @ref DRV_GPIO_Status.
 */
DRV_GPIO_Status DRV_GPIO_EnableIrq(GPIO_Id gpio);
/**
 * @brief Disable GPIO interrupt
 *
 * @param [in] gpio: ID of type @ref GPIO_Id to specify the gpio to manipulate.
 *
 * @return @ref DRV_GPIO_Status.
 */
DRV_GPIO_Status DRV_GPIO_DisableIrq(GPIO_Id gpio);
/**
 * @brief Clear GPIO interrupt
 *
 * @param [in] gpio: ID of type @ref GPIO_Id to specify the gpio to manipulate.
 *
 * @return @ref DRV_GPIO_Status.
 */
DRV_GPIO_Status DRV_GPIO_ClearIrq(GPIO_Id gpio);
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
 * @param [in]  pin_id: ID of type MCU_PinId. This is the physical pin defined in pin.h.
 * @param [in]  pull: Pull mode setting to dynamically changed GPIO with type @ref
 * GPIO_Pull. If pullmode is set to GPIO_PULL_DONT_CHANGE, pullmode will remains unchanged.
 * @param [out] value: Current value of this pin.
 *
 * @return @ref DRV_GPIO_Status.
 */
DRV_GPIO_Status DRV_GPIO_QueryPinValue(MCU_PinId pin_id, GPIO_Pull pull, uint8_t *value);
/**
 * @brief Configure a physical pin to GPIO dynamically.
 *        1. Check if there is any space left inside mapping table and return error if all entries
 * is used.
 *        2. Find a free virtual GPIO ID
 *        3. Store the original mux setting of the requested pin
 *        4. Set mux setting of the requested pin to GPIO
 *        5. Set pin mapping between this gpio id and requested pin.
 *        6. Return the virtual GPIO ID.
 *
 * @param [in]  gpio_config: GPIO configuration @ref GPIO_Param with the pin_set defined in pin.h
 * @param [out] gpio: Dynamically allocated GPIO ID of type @ref GPIO_Id, and can be used as
 * parameters of other GPIO APIs.
 *
 * @return @ref DRV_GPIO_Status.
 */
DRV_GPIO_Status DRV_GPIO_Open(GPIO_Param *gpio_config, GPIO_Id *gpio);
/**
 * @brief Free a GPIO and restore the previous MUX setting.
 *        1.Check if the GPIO ID was allocated.
 *        2.Restore the original mux setting of this pin.
 *        3.Clean the pin mapping of gpio table.
 *
 * @param [in] gpio: ID of type @ref GPIO_Id to specify the gpio id to close.
 *
 * @return @ref DRV_GPIO_Status.
 */
DRV_GPIO_Status DRV_GPIO_Close(GPIO_Id gpio);
/**
 * @brief Query current pinset of a virtual gpio ID. Mapping between virtual GPIO ID and physical
 * pin ID.
 *
 * @param [in] gpio: gpio ID with type @ref GPIO_Id to query.
 * @param [out] pin_id: Current pin set of the specified gpio ID.
 *
 * @return @ref DRV_GPIO_Status.
 */
DRV_GPIO_Status DRV_GPIO_GetPinset(GPIO_Id gpio, MCU_PinId *pin_id);
/** @} gpio_apis */
/*! @cond Doxygen_Suppress */
#undef EXTERN
#ifdef __cplusplus
}
#endif
/*! @endcond */
/** @} gpio_driver */

#endif /* DRV_GPIO_H_ */
