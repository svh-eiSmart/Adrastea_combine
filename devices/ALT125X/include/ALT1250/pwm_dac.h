/*  ---------------------------------------------------------------------------

    (c) copyright 2020 Altair Semiconductor, Ltd. All rights reserved.

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
 * @file pwm_dac.h
 */
#ifndef PWM_DAC_H
#define PWM_DAC_H
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "if_mngr.h"
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/*! @cond Doxygen_Suppress */
#ifndef UINT_MAX
#define UINT_MAX (0xFFFFFFFFUL)
#endif
/*! @endcond */
/**
 * @defgroup pwm_driver PWM-DAC Driver
 * @{
 */
/**
 * @defgroup pwm_static_config PWM-DAC Constants
 * @{
 */
#define PWM_DUTY_CYCLE_DEFCONFIG \
  UINT_MAX /**< This is used to set pwm duty cycle defined in interface_config_alt1250.h */
/** @} pwm_static_config  */
/****************************************************************************
 * Data types
 ****************************************************************************/
/**
 * @defgroup pwm_data_types PWM-DAC Types
 * @{
 */
/*! @cond Doxygen_Suppress */
/**
 * @typedef pwm_cfg_t
 * Definition of pwm configuration defined in interface_config_alt1250.h.
 * This is only used to get configuration from interface manager.
 * This is not used for any API parameters.
 */
typedef struct {
  unsigned int def_clk_div;    /**< Default CLK divider setting*/
  unsigned int def_duty_cycle; /**< Default duty cycle setting*/
} pwm_cfg_t;
/*! @endcond */
/**
 * @typedef eMcuPwmIds
 * Definition of PWM channel ID
 */
typedef enum {
  PWM_ID_0 = 0, /**< PWM-DAC channel 0*/
  PWM_ID_1,     /**< PWM-DAC channel 1*/
  PWM_ID_2,     /**< PWM-DAC channel 2*/
  PWM_ID_3,     /**< PWM-DAC channel 3*/
  PWM_NUM
} eMcuPwmIds;
/**
 * @typedef ePwmRet
 * Definition of PWM API return code
 */
typedef enum {
  PWM_RET_IFCFG_ERR, /**< API returns with inteface configuration error*/
  PWM_RET_ID_ERR,    /**< API returns with wrong ID*/
  PWM_RET_PARAM_ERR, /**< API returns with wrong parameter*/
  PWM_RET_SUCCESS    /**< API returns with no error*/
} ePwmRet;

/** @} pwm_data_types */

/*******************************************************************************
 * API
 ******************************************************************************/
/**
 * @defgroup pwm_apis PWM-DAC APIs
 * @{
 */
/**
 * @brief Configure duty cycle in run time.
 *
 * @param [in] pwm_id: PWM to control with type @ref eMcuPwmIds.
 * @param [in] duty_cycle: Duty cycle setting of this PWM channe.
 *                   - 0: Disable PWM and output 0
 *                   - 1024: Disable PWM and output 1
 *                   - 1 ~ 1023: duty cycle in the rate of 1024
 *                   - PWM_DUTY_CYCLE_DEFCONFIG: Default setting defined in
 *                   interfaces_config_alt1250.h
 * @return @ref ePwmRet
 */
ePwmRet pwm_set(eMcuPwmIds pwm_id, unsigned int duty_cycle);
/**
 * @brief Query the status of each PWM
 *
 * @param [in]  pwm_id: PWM to control with type @ref eMcuPwmIds.
 * @param [out] pwm_en: PWM enable status.
 * @param [out] clk_div: PWM clk divider setting.
 * @param [out] duty_cycle: PWM duty cycle setting.
 * @return @ref ePwmRet.
 */
ePwmRet pwm_get(eMcuPwmIds pwm_id, unsigned int *pwm_en, unsigned int *clk_div,
                unsigned int *duty_cycle);
/** @} pwm_apis */
/** @} pwm_driver */
#endif /*PWM_DAC_H*/
