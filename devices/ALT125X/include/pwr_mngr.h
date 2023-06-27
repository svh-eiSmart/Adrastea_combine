/****************************************************************************
 *
 *  (c) copyright 2019 Altair Semiconductor, Ltd. All rights reserved.
 *
 *  This software, in source or object form (the "Software"), is the
 *  property of Altair Semiconductor Ltd. (the "Company") and/or its
 *  licensors, which have all right, title and interest therein, You
 *  may use the Software only in  accordance with the terms of written
 *  license agreement between you and the Company (the "License").
 *  Except as expressly stated in the License, the Company grants no
 *  licenses by implication, estoppel, or otherwise. If you are not
 *  aware of or do not agree to the License terms, you may not use,
 *  copy or modify the Software. You may use the source code of the
 *  Software only for your internal purposes and may not distribute the
 *  source code of the Software, any part thereof, or any derivative work
 *  thereof, to any third party, except pursuant to the Company's prior
 *  written consent.
 *  The Software is the confidential information of the Company.
 *
 ****************************************************************************/
/**
 * @file pwr_mngr.h
 */

#ifndef _POWER_MANAGER_DRV_H
#define _POWER_MANAGER_DRV_H

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "mcu_pmp_api.h"
#include "sleep_mngr.h"
#include "pwr_mngr_api.h"

#define PWR_MON_GPIO_MAX_ENTRY (4)
#define PWR_DEF_MAX_DRUATION (60000)
/* ------------------- internal structures and prototypes ------------------- */

typedef struct _pwr_param_config {
  unsigned int enableSleep;
  power_mode_e mode; /* power mode.*/
  unsigned int duration;
  unsigned int memRetentionSecIdList; /* section ID list for Memory Retention */
} pwr_param_config_t;

typedef struct _pm_params {
  int sleep_manager_counters_enable;
} pm_params_t;

typedef struct _pm_wakeup_cfg {
  eMcuPinIds pin_set;
  pwr_wakeup_pin_pol_e pin_pol;
} pm_wakeup_cfg_t;
/*-----------------------------------------------------------------------------
 * int pwr_check_allow2sleep(void)
 * PURPOSE: This function would check if meets the conditions of allow-to-sleep.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  status code.
 *-----------------------------------------------------------------------------
 */
int pwr_check_allow2sleep(void);

/*-----------------------------------------------------------------------------
 * void pwr_pre_sleep_process (uint32_t idle_time)
 * PURPOSE: This function would run pre-sleep process.
 * PARAMs:
 *      INPUT:  uint32_t idle_time
 *      OUTPUT: None
 * RETURN:  status code.
 *-----------------------------------------------------------------------------
 */
void pwr_pre_sleep_process(uint32_t idle_time);

/*-----------------------------------------------------------------------------
 * void pwr_post_sleep_process (uint32_t idle_time)
 * PURPOSE: This function would run post-sleep process.
 * PARAMs:
 *      INPUT:  uint32_t idle_time
 *      OUTPUT: None
 * RETURN:  status code.
 *-----------------------------------------------------------------------------
 */
void pwr_post_sleep_process(uint32_t idle_time);

/*-----------------------------------------------------------------------------
 * void pwr_wakeup_gpio_init(void)
 * PURPOSE: This function would initialize wakeup_gpio.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  pm_err_code_e.
 *-----------------------------------------------------------------------------
 */
pm_err_code_e pwr_wakeup_gpio_init(void);

/*-----------------------------------------------------------------------------
 * pm_err_code_e PWR_EnterSleep(void)
 * PURPOSE: This function would send allow-to-sleep message to PMP.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  pm_err_code_e.
 *-----------------------------------------------------------------------------
 */
pm_err_code_e PWR_EnterSleep(void);

/*-----------------------------------------------------------------------------
 * void pwr_check_port_busy(void)
 * PURPOSE: This function would check if specified GPIO port is busy.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  0:not busy; 1:busy.
 *-----------------------------------------------------------------------------
 */
int pwr_check_port_busy(void);

/*-----------------------------------------------------------------------------
 * int pwr_check_uart_inactive_time(void)
 * PURPOSE: This function would check if UART inactivity time still less than
 *          upper limitation.
 *          If yes, block this cycle to sleep.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  0: over upper limitation, 1: less than setting.
 *-----------------------------------------------------------------------------
 */
int pwr_check_uart_inactive_time(void);

/*-----------------------------------------------------------------------------
 * void pwr_refresh_uart_inactive_time(void)
 * PURPOSE: This function would refresh last UART interrupt time.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  None
 *-----------------------------------------------------------------------------
 */
void pwr_refresh_uart_inactive_time(void);

/*-----------------------------------------------------------------------------
 * void pwr_stat_update_last_wakeup_info(pwr_wakeup_cause_e cause, unsigned int duration_left)
 * PURPOSE: This function would update the data of last wakeup.
 * PARAMs:
 *      INPUT:  pwr_wakeup_cause_e cause - wakeup cause.
 *              unsigned int duration_left - sleep duration left.
 *      OUTPUT: None
 * RETURN:  None
 *-----------------------------------------------------------------------------
 */
void pwr_stat_update_last_wakeup_info(pwr_wakeup_cause_e cause, unsigned int duration_left);

/*-----------------------------------------------------------------------------
 * void pwr_store_set_gatedclks_before_sleep(void)
 * PURPOSE: This function would handle gated clocks IO before sleep.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  None
 *-----------------------------------------------------------------------------
 */
void pwr_gatedclks_handle_before_sleep(void);

/*-----------------------------------------------------------------------------
 * void pwr_gatedclks_handle_after_sleep(void)
 * PURPOSE: This function would handle gated clocks IO after sleep.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  None
 *-----------------------------------------------------------------------------
 */
void pwr_gatedclks_handle_after_sleep(void);

/*-----------------------------------------------------------------------------
 * pwr_dev_boot_type_e pwr_stat_get_dev_boot_type(void)
 * PURPOSE: This function would get device boot type.
 * PARAMs:
 *      INPUT:  None
 *      OUTPUT: None
 * RETURN:  enumeration value of device boot type.
 * Note:  it can't distinguish the boot is triggered by reset button.
 *-----------------------------------------------------------------------------
 */
pwr_dev_boot_type_e pwr_stat_get_dev_boot_type(void);

/*-----------------------------------------------------------------------------
 * int pwr_set_debug_mode(int en_debug)
 * PURPOSE: This function would switch on/off power-manager debug mode.
 * PARAMs:
 *      INPUT:  en_debug (1:enable / 0:disable)
 *      OUTPUT: None
 * RETURN:  error code. 0-success; other-fail.
 *-----------------------------------------------------------------------------
 */
int pwr_set_debug_mode(int en_debug);

/*-----------------------------------------------------------------------------
 * PURPOSE: Mask interrupts except for atomic-counter interrupt used for wake up.
 * It is used for disabled interrupts before entering sleep.
 *-----------------------------------------------------------------------------
 */
void pwr_mask_interrupts(power_mode_e mode);

/*-----------------------------------------------------------------------------
 * PURPOSE: Restore those interrupts were disabled for entering to sleep.
 * It is used for disabled interrupts before entering sleep.
 *-----------------------------------------------------------------------------
 */
void pwr_restore_interrupts(void);

#endif /* _POWER_MANAGER_DRV_H */
