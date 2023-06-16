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
 * @file pwr_mngr.h
 */

#ifndef PWR_MNGR_H_
#define PWR_MNGR_H_
/**
 * @defgroup powermanager(Power Manager) Middleware
 * @{
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "sleep_mngr.h"
#include "DRV_GPIO.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/**
 * @defgroup pwrmngr_constant Power Manager Constants
 * @{
 */
#define PWR_MNGR_DFT_SLEEP_DRUATION (60000) /**< The default sleep duration */
#define PWR_MNGR_MON_GPIO_MAX_ENTRY (4) /**< The maximum monitored GPIOs */

#define PWR_MNGR_EN_HIBERNATE_SHUTDOWN  1  /* Enable the shutdown mode with hibernation. 0: disable; 1: enable */
#define PWR_MNGR_SHUTDOWN_THRESHOLD (50*1000) /**< The threshold of sleep duration to enter shutdown sleep. \
When the calculated sleep duration over the defined threshold value, it can send shutdown request; \
otherwise, it sends standby request. This design is for SFlash wear-out protection.  */

/** @} pwrmngr_constant */

/****************************************************************************
 * Public Types
 ****************************************************************************/
/**
 * @defgroup pwrmngr_types Power Manager Types
 * @{
 */

/**
 * @brief Definition of the error code of Power Manager APIs.
 */
typedef enum {
  PWR_MNGR_OK,        /**< 0: No error. */
  PWR_MNGR_GEN_ERROR, /**< 1: Other errors. */
  PWR_MNGR_NOT_INIT   /**< 2: Not initialized. */
} PWR_MNGR_Status;

/** @brief Power Modes. Please notice that only STOP/STANDBY/SHUTDOWN can be configured by
 * application. */
typedef enum {
  PWR_MNGR_MODE_RUN,     /**< \internal 0: Active mode. Internally use only.*/
  PWR_MNGR_MODE_SLEEP,   /**< \internal 1: OS sleep mode. Internally use only. */
  PWR_MNGR_MODE_STOP,    /**< 2: Stop mode. */
  PWR_MNGR_MODE_STANDBY, /**< 3: Standby mode. */
  PWR_MNGR_MODE_SHUTDOWN /**< 4: Shutdown mode. */
} PWR_MNGR_PwrMode;

/**
 * @brief Definition of the counters to record how many times RTOS was prevented to go to low power
 * mode.
 */
typedef struct {
  unsigned long sleep_manager;                /**< sleep manager is busy */
  unsigned long sleep_disable;                /**< sleep is disabled */
  unsigned long hifc_busy;                    /**< internal HiFC is busy */
  unsigned long uart_incativity_time;         /**< CLI inactivity time is not reach */
  unsigned long mon_gpio_busy;                /**< Configured GPIO state is busy */
  int32_t sleep_manager_devices[SMNGR_RANGE]; /**< sleep manager clients (id). */
} PWR_MNGR_PwrCounters;

/** @brief Definition of power manager configuration. */
typedef struct {
  uint32_t enable;          /**< sleep enable/disable.*/
  PWR_MNGR_PwrMode mode;    /**< power mode.*/
  uint32_t duration;        /**< sleep duration */
  uint32_t threshold;       /**< the duration threshold to enter shutdown */
  uint32_t section_id_list; /**< section ID list for Memory Retention */
} PWR_MNGR_Configuation;

/** @} pwrmngr_types */

/****************************************************************************
 * Public APIs
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
 * @defgroup pwrmngr_apis PM Core APIs
 * @{
 */
/**
 * @brief Intialize Power Manager module.
 *
 * @return PWR_MNGR_Status is returned.
 */
PWR_MNGR_Status pwr_mngr_init(void);

/**
 * @brief Configure (Add/Modify) the monitored GPIO for stop MCU going to sleep.
 *
 * @param [in] gpio_id: GPIO ID.
 * @param [in] level: 1 or 0.
 *
 * @return PWR_MNGR_Status is returned.
 */
PWR_MNGR_Status pwr_mngr_set_monitor_io(GPIO_Id gpio_id, int32_t level);

/**
 * @brief Removed the GPIO from monitored list.
 *
 * @param [in] gpio_id: GPIO ID.
 *
 * @return PWR_MNGR_Status is returned.
 */
PWR_MNGR_Status pwr_mngr_del_monitor_io(GPIO_Id gpio_id);

/**
 * @brief Get the enabled setting of power manager.
 *
 * @return value: 1 enable; 0 disable.
 */
int32_t pwr_mngr_conf_get_sleep(void);

/**
 * @brief Enabled/disabled the power manager.
 *
 * @param [in] set_val: 1:enable; 0:disable.
 *
 * @return PWR_MNGR_Status is returned.
 */
PWR_MNGR_Status pwr_mngr_enable_sleep(int32_t set_val);

/**
 * @brief Configure the sleep mode and duration. New setting will take effect in
 * next sleep cycle.
 *
 * @param [in] mode: power mode(PWR_MNGR_MODE_STOP, PWR_MNGR_MODE_STANDBY and
 * PWR_MNGR_MODE_SHUTDOWN)
 * @param [in] duration: sleep duration in unit of millisecond.
 *
 * @return PWR_MNGR_Status is returned.
 */
PWR_MNGR_Status pwr_mngr_conf_set_mode(PWR_MNGR_PwrMode mode, uint32_t duration);

/**
 * @brief Get current sleep settings (mode, duration,...).
 *
 * @param [out] *pwr_conf: pointer of PWR_MNGR_Configuation.
 *
 * @return PWR_MNGR_Status is returned.
 */
PWR_MNGR_Status pwr_mngr_conf_get_settings(PWR_MNGR_Configuation *pwr_conf);

/**
 * @brief Get MCU sleep counters of MCU.
 *
 * @param [in] *pwr_counters: pointer of PWR_MNGR_PwrCounters.
 *
 * @return error code. 0-success; other-fail.
 */
int32_t pwr_mngr_get_prevent_sleep_cntr(PWR_MNGR_PwrCounters *pwr_counters);

/**
 * @brief Clear MCU sleep counters of MCU.
 *
 * @return error code. 0-success; other-fail.
 */
int32_t pwr_mngr_clr_prevent_sleep_cntr(void);

/**
 * @brief Configure MCU CLI inactivity time.
 *
 * @param [in] inact_time: inactivity time in unit of second.
 *
 * @return PWR_MNGR_Status is returned.
 */
PWR_MNGR_Status pwr_mngr_conf_set_cli_inact_time(uint32_t inact_time);

/**
 * @brief Configure SRAM Content Retention sections.
 * Retention Section ID mask = (PWR_CON_RETENTION_SEC_1 | PWR_CON_RETENTION_SEC_2...).
 *
 * @param [in] sec_id_mask: section id mask.
 *
 * @return PWR_MNGR_Status is returned.
 * 
 * @note Notice: Improper use of this API may cause data loss and 
 * failure of stateful standby resume. 
 */
PWR_MNGR_Status pwr_mngr_enable_retention_sec(uint32_t sec_id_mask);

/**
 * @brief Check if meets the conditions of allow-to-sleep.
 *
 * @return status number.
 */
int32_t pwr_mngr_check_allow2sleep(void);

/**
 * @brief Run pre-sleep process.
 *
 * @param [in] idle_time: expected idle time.
 * @param [in] pwr_mode: power mode.
 *
 * @return None.
 */
void pwr_mngr_pre_sleep_process(uint32_t idle_time, PWR_MNGR_PwrMode pwr_mode);

/**
 * @brief Run post-sleep process.
 *
 * @param [in] idle_time: expected idle time.
 * @param [in] pwr_mode: power mode.
 *
 * @return None.
 */
void pwr_mngr_post_sleep_process(uint32_t idle_time, PWR_MNGR_PwrMode pwr_mode);

/**
 * @brief Send allow-to-sleep request.
 *
 * @return PWR_MNGR_Status.
 */
PWR_MNGR_Status pwr_mngr_enter_sleep(void);

/**
 * @brief Switch on/off power-manager debug mode.
 *
 * @param [in] en_debug: 1:enable / 0:disable.
 *
 * @return error code. 0-success; other-fail.
 */
int32_t pwr_mngr_set_debug_mode(int32_t en_debug);

/**
 * @brief Refresh the last UART interrupt time.
 *
 * @return None.
 */
void pwr_mngr_refresh_uart_inactive_time(void);

/** @} pwrmngr_apis */
/*! @cond Doxygen_Suppress */
#undef EXTERN
#ifdef __cplusplus
}
#endif
/*! @endcond */
/** @} powermanager */

#endif /* PWR_MNGR_H_ */
