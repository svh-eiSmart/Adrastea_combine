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
 * @file DRV_GPTIMER.h
 */

#ifndef DRV_GPTIMER_H_
#define DRV_GPTIMER_H_

/**
 * @defgroup drv_gptimer GPTimer Low-Level Driver
 * @{
 */

/****************************************************************************
 * Data Type
 ****************************************************************************/
/**
 * @defgroup drv_gptimer_types GPTimer Data Types
 * @{
 */
/**
 * @brief Definition of GPTimer interrupt callback callback.
 */
typedef void (*DRV_GPTIMER_EventCallback)(uint32_t user_param);

/** @} drv_gptimer_types */

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/
/*! @cond Doxygen_Suppress */
#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif
/*! @endcond */
/**
 * @defgroup drv_gptimer_apis GPTimer APIs
 * @{
 */
/**
 * @brief Register an callback for GP timer interrupt.
 * When the GP timer interrupt occurs, the handler is called from interrupt
 * context.
 * @param [in] gptimer: interrupt number, 0 or 1.
 * @param [in] callback: callback function.
 * @param [in] user_param : parameter transfered to the callback function.
 * @return 0
 */
int32_t DRV_GPTIMER_RegisterInterrupt(int32_t gptimer, DRV_GPTIMER_EventCallback callback,
                                      int32_t user_param);

/**
 * @brief Read GP timer current value (counter).
 *
 * @return GP timer value.
 */
uint32_t DRV_GPTIMER_GetValue(void);

/**
 * @brief Add offset to GP timer counter.
 * @param [in] offset: offset to be added.
 * @return None.
 */
void DRV_GPTIMER_AddOffset(uint32_t offset);

/**
 * @brief Enable the GP timer hardware.
 *
 * @return None.
 */
void DRV_GPTIMER_Enable(void);

/**
 * @brief Disable the GP timer hardware.
 *
 * @return None.
 */
void DRV_GPTIMER_Disable(void);

/**
 * @brief Activate GP timer and initialize interrupt registrations DB.
 * If required, enable GP timer.
 * @param [in] value: initial value for GP timer.
 * @return None.
 */
void DRV_GPTIMER_Activate(uint32_t value);

/**
 * @brief Set GP timer interrupt with offset from current count.
 * @param [in] gptimer: interrupt number, 0 or 1.
 * @param [in] offset: at which interrupt will occur (16 bits).
 * @return Pass (0) or Fail (-1).
 */
int32_t DRV_GPTIMER_SetInterruptOffset(int32_t gptimer, uint32_t offset);

/**
 * @brief Set GP timer interrupt at absolute value.
 * @param [in] gptimer: interrupt number, 0 or 1.
 * @param [in] value: at which interrupt will occur (16 bits).
 * @return Pass (0) or Fail (-1).
 */
int32_t DRV_GPTIMER_SetInterruptValue(int32_t gptimer, uint32_t value);

/**
 * @brief Disable active interrupt of GP timer.
 * @param [in] gptimer: interrupt number, 0 or 1.
 * @return Pass (0) or Fail (-1).
 */
int32_t DRV_GPTIMER_DisableInterrupt(int32_t gptimer);

/**
 * @brief Read GP timer interrupt target value.
 * @param [in] gptimer: interrupt number, 0 or 1.
 * @return Target value.
 */
uint32_t DRV_GPTIMER_ReadTargetValue(int32_t gptimer);

/**
 * @brief Read GP timer interrupt counter.
 * @param [in] gptimer: interrupt number, 0 or 1.
 * @return Counter.
 */
uint32_t DRV_GPTIMER_ReadCurrentValue(int32_t gptimer);

/**
 * @brief Set GP timer frequency.
 * @param [in] frequency: requested frequency in Hz.
 * @return Actual frequency set, or -1 if fail.
 */
int32_t DRV_GPTIMER_SetFrequency(int32_t frequency);

/**
 * @brief Initialize GP timer interrupt registrations DB if required.
 *        This function must be called before activating GP timer.
 *        There is no effect in calling this function more than once.
 *
 * @return None.
 */
void DRV_GPTIMER_Initialize(void);

/** @} drv_gptimer_apis */
/*! @cond Doxygen_Suppress */
#undef EXTERN
#ifdef __cplusplus
}
#endif
/*! @endcond */
/** @} drv_gptimer */

#endif /* DRV_GPTIMER_H_ */
