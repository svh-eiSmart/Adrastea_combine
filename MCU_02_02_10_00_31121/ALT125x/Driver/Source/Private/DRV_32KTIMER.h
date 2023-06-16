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
 * @file DRV_32KTIMER.h
 */
#ifndef DRV_32KTIMER_H_
#define DRV_32KTIMER_H_

/**
 * @defgroup drv_32ktimer_driver GDMA Driver
 * @{
 */

/****************************************************************************
 * Public Data Types
 ****************************************************************************/
/**
 * @defgroup drv_32ktimer_types 32KTimer Types
 * @{
 */

/**
 * @brief Definition of 32KTIMER callback function prototype.
 */
typedef void (*DRV_32KTIMER_InterruptHandler)(int32_t user_param);

/** @} drv_32ktimer_types */

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/
#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif
/**
 * @defgroup drv_32ktimer_apis 32Ktimer APIs
 * @{
 */

/**
 * @brief Register an interrupt handler for shadow 32k timer interrupt.
 *        When the shadow 32k timer interrupt occurs, the handler is called
 *        from interrupt context.
 *
 * @param [in] irq_handler: interrupt handler function.
 * @param [in] user_param: parameter transfered to the interrupt handler function.
 * @return OK (0).
 */
int32_t DRV_32KTIMER_RegisterInterrupt(DRV_32KTIMER_InterruptHandler irq_handler,
                                       int32_t user_param);

/**
 * @brief Read shadow 32k timer current value (counter).
 *
 * @param [in] None.
 * @return Shadow 32k timer value.
 */
uint32_t DRV_32KTIMER_GetValue(void);

/**
 * @brief Add offset to shadow 32k timer counter.
 *
 * @param [in] offset: offset to be added.
 * @return None.
 */
void DRV_32KTIMER_AddOffset(uint32_t offset);

/**
 * @brief Enable the shadow 32k timer HW.
 *
 * @param [in] None.
 * @return None.
 */
void DRV_32KTIMER_Enable(void);

/**
 * @brief Disable the shadow 32k timer HW.
 *
 * @param [in] None.
 * @return None.
 */
void DRV_32KTIMER_Disable(void);

/**
 * @brief Activate shadow 32k timer and initialize interrupt registrations DB
 *        if required, enable timer.
 *
 * @param [in] value: Initial value for timer.
 * @return None.
 */
void DRV_32KTIMER_Activate(uint32_t value);

/**
 * @brief Set shadow 32k timer interrupt with offset from current count.
 *
 * @param [in] offset: At which interrupt will occur (16 bits).
 * @return OK (0) or Fail (-1).
 */
int32_t DRV_32KTIMER_SetInterruptOffset(uint32_t offset);

/**
 * @brief Set shadow 32k timer interrupt at absolute value.
 *
 * @param [in] value: At which interrupt will occur (16 bits).
 * @return OK (0).
 */
int32_t DRV_32KTIMER_SetInterruptValue(uint32_t value);

/**
 * @brief Disable active interrupt of shadow 32k timer.
 *
 * @param [in] None.
 * @return OK (0).
 */
int32_t DRV_32KTIMER_DisableInterrupt(void);

/**
 * @brief Read timer interrupt target.
 *
 * @param [in] None.
 * @return Target value.
 */
uint32_t DRV_32KTIMER_ReadTargetValue(void);

/**
 * @brief Read GP timer interrupt counter.
 *
 * @param [in] None.
 * @return Current value.
 */
uint32_t DRV_32KTIMER_ReadCurrentValue(void);

/**
 * @brief Initialize timer interrupt registrations DB if required.
 *        This function must be called before activating timer.
 *        There is no effect in calling this function more than once.
 *
 * @param [in] None.
 * @return None.
 */
void DRV_32KTIMER_Initialize(void);
/** @} drv_32ktimer_apis */
#undef EXTERN
#ifdef __cplusplus
}
#endif
/** @} drv_32ktimer_driver */

#endif /* DRV_32KTIMER_H_ */
