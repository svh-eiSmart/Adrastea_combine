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
#ifndef DRV_GPIO_WAKEUP_H_
#define DRV_GPIO_WAKEUP_H_

/****************************************************************************
 * Data Type
 ****************************************************************************/
typedef void (*DRV_GPIO_WAKEUP_IrqCallback)(void *user_param);

typedef enum {
  DRV_GPIO_WAKEUP_REGISTER_LOOKUP,
  DRV_GPIO_WAKEUP_STRUCT_LOOKUP
} DRV_GPIO_WAKEUP_LookupMethod;

/*  Clock types*/
typedef enum {
  GPIO_WAKEUP_RTC_CLK,   /*RTC ~32KHz*/
  GPIO_WAKEUP_URCLK_CLK, /*URCLK ~4MHz*/
  GPIO_WAKEUP_ASYNC_CLK  /*async*/
} GPIO_WAKEUP_Clock;

/*  Polarity types*/
typedef enum {
  GPIO_WAKEUP_EDGE_RISING,
  GPIO_WAKEUP_EDGE_FALLING,
  GPIO_WAKEUP_EDGE_RISING_FALLING,
  GPIO_WAKEUP_LEVEL_POL_HIGH,
  GPIO_WAKEUP_LEVEL_POL_LOW
} GPIO_WAKEUP_Polarity;

typedef struct {
  int32_t gpio_pin;
  GPIO_WAKEUP_Clock clk;
  GPIO_WAKEUP_Polarity mode;
  uint32_t debounce_en;
  uint32_t debounce_max_val;
  uint32_t wakeup_en;
  uint32_t int_en;
  DRV_GPIO_WAKEUP_IrqCallback irq_handler;
  void *user_param;
  uint32_t int_counter;
} DRV_GPIO_WAKEUP_Configuration;
/****************************************************************************
 * Function Prototypes
 ****************************************************************************/
#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif
/* Select polarity mode */
int32_t DRV_GPIO_WAKEUP_SetPolarity(uint32_t pin_number, GPIO_WAKEUP_Polarity mode);

/* Register the IO as wakeup with given parameters */
int32_t DRV_GPIO_WAKEUP_AddIO(DRV_GPIO_WAKEUP_Configuration *params);

/* Delete wakeup */
int32_t DRV_GPIO_WAKEUP_DeleteIO(uint32_t pin_number);

/* Disable wakeup for the specified IO */
int32_t DRV_GPIO_WAKEUP_Disable(uint32_t pin_number);

/* Enable wakeup for the specified IO*/
int32_t DRV_GPIO_WAKEUP_Enable(uint32_t pin_number);

/* Enable interrupt for the specified IO */
int32_t DRV_GPIO_WAKEUP_EnableInterrupt(uint32_t pin_number);

/* Disable interrupt for specified IO at active state,
 * but continue to function as wakeup source in sleep
 * state*/
int32_t DRV_GPIO_WAKEUP_DisableInterrupt(uint32_t pin_number);

/*this function should be inserted in system.c*/
void DRV_GPIO_WAKEUP_Initialize(void);

/*look or wakeup machine by pin number*/
int32_t DRV_GPIO_WAKEUP_FindIO(uint32_t pin_number);

/*  Register interrupt*/
int32_t DRV_GPIO_WAKEUP_RegisterInterrupt(uint32_t gpio, DRV_GPIO_WAKEUP_IrqCallback irq_handler,
                                          void *user_param);

/*read setup form registers*/
void DRV_GPIO_WAKEUP_GetSetup(DRV_GPIO_WAKEUP_Configuration *gpio_wakeup_elem, uint32_t pin_number,
                              DRV_GPIO_WAKEUP_LookupMethod method);

void DRV_GPIO_WAKEUP_ClearInterrupts(void);

uint8_t DRV_GPIO_WAKEUP_ValidateUsingPin(uint32_t pin_number);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* DRV_GPIO_WAKEUP_H_ */
