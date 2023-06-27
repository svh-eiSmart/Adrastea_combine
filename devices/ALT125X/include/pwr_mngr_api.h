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
 * @file pwr_mngr_api.h
 */

#ifndef _POWER_MANAGER_API_H
#define _POWER_MANAGER_API_H
/**
 * @defgroup powermanagement PM (Power Management) Driver
 * @{
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "mcu_pmp_api.h"
#include "sleep_mngr.h"
#include "gpio.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/**
 * @defgroup pm_constant PM Constants
 * @{
 */
#define WAKEUP_GPIO_NUM (MCU_GPIO_ID_01)
/**< ALT1250 MCU Demo kit WakeUp PIN number - GPI61. It should be adjusted per hardware. */

/* Retention Section Id mask.*/
#define PWR_CON_RETENTION_SEC_1 (0x1) /**< Retention Section 1.*/
#define PWR_CON_RETENTION_SEC_2 (0x2) /**< Retention Section 2.*/
#ifdef ALT1255
#define PWR_CON_RETENTION_SEC_MASK_ALL                                                \
  PWR_CON_RETENTION_SEC_2 /**< Retention Section ID mask = (PWR_CON_RETENTION_SEC_1 | \
                             PWR_CON_RETENTION_SEC_2...) */
#else
#define PWR_CON_RETENTION_SEC_MASK_ALL \
  (0x3) /**< Retention Section ID mask = (PWR_CON_RETENTION_SEC_1 | PWR_CON_RETENTION_SEC_2...) */
#endif
/** @} pm_constant */

/****************************************************************************
 * Public Types
 ****************************************************************************/
/**
 * @defgroup pm_types PM Types
 * @{
 */
/**
 * @brief Definition of the error code of PM APIs.
 */
typedef enum _pm_err_code {
  PWR_ERR_NO_ERROR, /**< 0: No error. */
  PWR_ERR_GEN_ERROR /**< 1: Other errors. */
} pm_err_code_e;

/** @brief Power Modes. Please notice that only STOP/STANDBY/SHUTDOWN can be configured by
 * application. */
typedef enum _power_mode {
  PWR_MODE_RUN,      /**< \internal 0: Active mode. Internally use only.*/
  PWR_MODE_SLEEP,    /**< \internal 1: OS sleep mode. Internally use only. */
  PWR_MODE_STOP,     /**< 2: Stop mode. */
  PWR_MODE_STANDBY,  /**< 3: Standby mode. */
  PWR_MODE_SHUTDOWN, /**< 4: Shutdown mode. */
} power_mode_e;

/** @brief Power Wakeup PIN Mode/Polarity.*/
typedef enum _pwr_wakeup_pin_pol {
  PWR_WAKEUP_EDGE_RISING,         /**< 0: Edge Rising.*/
  PWR_WAKEUP_EDGE_FALLING,        /**< 1: Edge Falling.*/
  PWR_WAKEUP_EDGE_RISING_FALLING, /**< 2: Edge Rising or Falling.*/
  PWR_WAKEUP_LEVEL_POL_HIGH,      /**< 3: Level Polarity High.*/
  PWR_WAKEUP_LEVEL_POL_LOW,       /**< 4: Level Polarity Low.*/
} pwr_wakeup_pin_pol_e;

/**
 * @brief pwr_stop_config_t
 * Definition of parameters for Power configuration structure.
 */
typedef struct _pwr_stop_config {
  unsigned int
      duration; /**< Duration of stop mode. Default value is 0(infinite). unit: millisecond.*/
} pwr_stop_config_t;

/**
 * @brief pwr_standby_config_t
 * Definition of parameters for Power configuration structure.
 */
typedef struct _pwr_standby_config {
  unsigned int
      duration; /**< Duration of standby mode. Default value is 0(infinite). unit: millisecond.*/
  unsigned int memRetentionSecIdList; /**< Section ID List for Memory Retention.*/
} pwr_standby_config_t;

/**
 * @brief pwr_shutdown_config_t
 * Definition of parameters for Power configuration structure.
 */
typedef struct _pwr_shutdown_config {
  unsigned int
      duration; /**< Duration of shutdown mode. Default value is 0(infinite). unit: millisecond.*/
} pwr_shutdown_config_t;

/**
 * @brief pwr_wakeup_cause_e
 * Definition of the cause of last MCU wake up.
 */
typedef enum _pwr_wakeup_cause {
  PWR_WAKEUP_CAUSE_NONE = 0, /**< 0: N/A */
  PWR_WAKEUP_CAUSE_TIMER,    /**< 1: Sleep duration timeout.*/
  PWR_WAKEUP_CAUSE_IO_ISR,   /**< 2: IO ISR.*/
  PWR_WAKEUP_CAUSE_MODEM,    /**< 3: Modem.*/
  PWR_WAKEUP_CAUSE_UNKNOWN = 0xffffffff /**< 0xFFFFFFFF: Unknown. */  // force the enum to 4 bytes
} pwr_wakeup_cause_e;

/**
 * @brief pwr_dev_boot_type_e
 * Definition of device boot type.
 */
typedef enum _pwr_dev_boot_type {
  PWR_DEV_COLD_BOOT = 0, /**< 0: Device cold boot. */
  PWR_DEV_WARM_BOOT      /**< 1: Device warm boot. */
} pwr_dev_boot_type_e;

/**
 * @brief pwr_mcu_wakeup_state_e
 * Definition of MCU wake up state. only available with specific retention memory(GPMdata).
 */
typedef enum _pwr_mcu_wakeup_state {
  PWR_WAKEUP_STATELESS = 0, /**< 0: Data in specific retention memory area is lost. */
  PWR_WAKEUP_STATEFUL       /**< 1: Data in specific retention memory area is kept. */
} pwr_mcu_wakeup_state_e;

/**
 * @brief pwr_wakeup_stat_t
 * Definition of the information of last MCU wake up.
 */
typedef struct _pwr_wakeup_stat {
  pwr_dev_boot_type_e boot_type;       /**< device boot type */
  pwr_mcu_wakeup_state_e wakeup_state; /**< wake up state */
  pwr_wakeup_cause_e last_cause;       /**< wakeup cause */
  uint32_t last_dur_left; /**< left sleep duration. in milliseconds, non zero when waking-up before
                             requested sleep duration elapsed */
  uint32_t
      counter; /**< how many times MCU wake up from STOP mode. Now it only applies to STOP mode. */
} pwr_wakeup_stat_t;

/**
 * @brief pwr_counters_t
 * Definition of the counters to record how many times RTOS was prevented to go to low power mode.
 */
typedef struct _pwr_counters {
  unsigned long sleep_manager;            /**< sleep manager is busy */
  unsigned long sleep_disable;            /**< sleep is disabled */
  unsigned long hifc_busy;                /**< internal HiFC is busy */
  unsigned long uart_incativity_time;     /**< CLI incativity time is not reach */
  unsigned long mon_gpio_busy;            /**< Configurd GPIO state is busy */
  int sleep_manager_devices[SMNGR_RANGE]; /**< sleep manager clients (id). */
} pwr_counters_t;

/** @} pm_types */

/* ----------------------------- External PM APIs ----------------------------- */
/**
 * @defgroup pm_funcs PM Core APIs
 * @{
 */

/**
 * @brief Intialize Power Manager module.
 *
 * @return pm_err_code_e is returned.
 */
pm_err_code_e PWR_Manager_Init(void);

/**
 * @brief Configure Power mode to Stop mode.
 *
 * @param [in] *pwr_stop_param: pointer of configured parameters.
 *
 * @return pm_err_code_e is returned.
 */
pm_err_code_e PWR_EnterStopMode(pwr_stop_config_t *pwr_stop_param);

/**
 * @brief Configure Power mode to Standby mode.
 *
 * @param [in] *pwr_standby_param: pointer of configured parameters.
 *
 * @return pm_err_code_e is returned.
 */
pm_err_code_e PWR_EnterStandbyMode(pwr_standby_config_t *pwr_standby_param);

/**
 * @brief Configure Power mode to Shutdown mode.
 *
 * @param [in] *pwr_shutdown_param: pointer of configured parameters.
 *
 * @return pm_err_code_e is returned.
 */
pm_err_code_e PWR_EnterShutdownMode(pwr_shutdown_config_t *pwr_shutdown_param);

/**
 * @brief Configure/Enable Wakeup PIN.
 *
 * @param [in] pinNum: GPIO PIN number.
 * @param [in] polarity: see enumeration pwr_wakeup_pin_pol_e
 *
 * @return pm_err_code_e is returned.
 */
pm_err_code_e PWR_EnableWakeUpPin(unsigned int pinNum, pwr_wakeup_pin_pol_e polarity);

/**
 * @brief Disable Wakeup pin.
 *
 * @param [in] pinNum: GPIO pin number.
 *
 * @return pm_err_code_e is returned.
 */
pm_err_code_e PWR_DisableWakeUpPin(unsigned int pinNum);

/**
 * @brief Configure SRAM Content Retention sections.
 * Retention Section ID mask = (PWR_CON_RETENTION_SEC_1 | PWR_CON_RETENTION_SEC_2...).
 *
 * @param [in] sec_id_mask: section id mask.
 *
 * @return pm_err_code_e is returned.
 */
pm_err_code_e PWR_EnableSRAMContentRetention(unsigned int sec_id_mask);

/**
 * @brief Configure (Add/Modify) the monitored PIN for stop MCU going to sleep.
 *
 * @param [in] pinNum: GPIO pin number.
 * @param [in] level: 1 or 0.
 *
 * @return pm_err_code_e is returned.
 */
pm_err_code_e PWR_Set_Monitored_Pin(int pinNum, int level);

/**
 * @brief Removed the PIN from monitored list.
 *
 * @param [in] pinNum: GPIO pin number.
 *
 * @return pm_err_code_e is returned.
 */
pm_err_code_e PWR_Del_Monitored_Pin(int pinNum);

/**
 * @brief Get the state of power manager.
 *
 * @return value: 1 enable; 0 diable.
 */
int pm_get_sleep_en(void);

/**
 * @brief Enabled/disabled the power manager.
 *
 * @param [in] set_val: 1:enable; 0:disable.
 *
 * @return error code. 0-success; other-fail.
 */
int pm_set_sleep_en(int set_val);

/**
 * @brief Configure the sleep mode and duration. New setting will take effect in
 * next sleep cycle.
 *
 * @param [in] mode: power mode(PWR_MODE_STOP, PWR_MODE_STANDBY and PWR_MODE_SHUTDOWN)
 * @param [in] duration: sleep duration in unit of mili-second.
 *
 * @return error code. 0-success; other-fail.
 */
int pwr_conf_param_set(power_mode_e mode, unsigned int duration);

/**
 * @brief Get current sleep mode.
 *
 * @return power_mode_e is returned.
 */
power_mode_e pwr_conf_param_get_mode(void);

/**
 * @brief Get sleep statistics of MCU.
 *
 * @param [out] *pwr_stat: pointer of pwr_wakeup_stat_t.
 *
 * @return error code. 0-success; other-fail
 */
int pwr_stat_get_sleep_statistics(pwr_wakeup_stat_t *pwr_stat);

/**
 * @brief Clear sleep statistics of MCU.
 */
void pwr_stat_clr_sleep_statistics(void);

/**
 * @brief Get MCU sleep counters of MCU.
 *
 * @param [in] *pwr_counters: pointer of pwr_counters_t.
 *
 * @return error code. 0-success; other-fail.
 */
int pwr_get_sleep_counters(pwr_counters_t *pwr_counters);

/**
 * @brief Clear MCU sleep counters of MCU.
 *
 * @return error code. 0-success; other-fail.
 */
int pwr_clr_sleep_counters(void);

/**
 * @brief Configure MCU CLI inactivity time.
 *
 * @param [in] inact_time: inactivity time in unit of second.
 *
 * @return error code. 0-success; other-fail.
 */
int pwr_conf_param_set_inactivity_time(unsigned int inact_time);

/** @} pm_funcs */

/** @} powermanagement */

#endif /* _POWER_MANAGER_API_H */
