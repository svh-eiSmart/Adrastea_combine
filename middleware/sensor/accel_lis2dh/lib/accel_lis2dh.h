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
 * @file accel_lis2dh.h
 */
#ifndef ACCEL_LIS2DH_H
#define ACCEL_LIS2DH_H

#ifdef __cplusplus
extern "C" {
#endif
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "lis2dh12_reg.h"
#include "i2c.h"
#include "gpio.h"

#ifdef ACCEL_LIS2DH_CFG_OVERLAY
#include "accel_lis2dh_projcfg.h"
#endif

// clang-format off

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/**
 * @defgroup accel_lis2dh Accelerometer LIS2DH Handler
 * @{
 */
/**
 * @defgroup lis2dh_static_config Accelerometer LIS2DH Handler Constants
 * @{
 */
#ifndef ACCEL_SENSOR_BUS
#define ACCEL_SENSOR_BUS             I2C1_BUS                           /**< I2C bus ID */
#endif

#ifndef ACCEL_SENSOR_ADDR
#define ACCEL_SENSOR_ADDR            0x18                              /**< LIS2DH I2c device ID */
#endif

#ifndef ACCEL_HANDLE_TASK_STACK_SIZE
#define ACCEL_HANDLE_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE + 256) /**< accel handler task stack size */
#endif

#ifndef ACCEL_HANDLE_TASK_PRIORITY
#define ACCEL_HANDLE_TASK_PRIORITY   (configMAX_PRIORITIES - 1)       /**< accel handler task priority */
#endif

#ifndef ACCEL_EVENT_QUEUE_SIZE
#define ACCEL_EVENT_QUEUE_SIZE       10                               /**< Size of event queue size */
#endif

#ifndef ACCEL_LIS2DH_ODR
#define ACCEL_LIS2DH_ODR             LIS2DH12_ODR_10Hz                /**< LIS2DH ODR configuration */
#endif

#ifndef ACCEL_LIS2DH_SCALE
#define ACCEL_LIS2DH_SCALE           LIS2DH12_2g                      /**< LIS2DH scale configuration */
#endif

#ifndef ACCEL_LIS2DH_OP_MODE
#define ACCEL_LIS2DH_OP_MODE         LIS2DH12_LP_8bit                 /**< LIS2DH operation mode */
#endif
/** @} lis2dh_static_config */

/**
 * @defgroup lis2dh_int_config Accelerometer LIS2DH Handler Interrupt Configuration
 * @{
 */
#define ACCEL_LIS2DH_INT_MODE_OR      0x00      /**< OR combination of interrupt events */
#define ACCEL_LIS2DH_INT_MODE_AND     0x02      /**< AND combination of interrupt events */
#define ACCEL_LIS2DH_INT_MODE_MOVE    0x01      /**< 6 direction movement recognition. An interrupt is generate when orientation move from
                                                     unknown zone to known zone. The interrupt signal stay for a duration ODR.
*/
#define ACCEL_LIS2DH_INT_MODE_POS     0x03      /**< 6 direction position recognition. An interrupt is generate when orientation is inside a
                                                     known zone. The interrupt signal stay untill orientation is inside the zone.
*/

#define ACCEL_LIS2DH_INTEVENT_Z_HIGH  0x20      /**< Enable interrupt generation on Z high event or on Direction recognition*/
#define ACCEL_LIS2DH_INTEVENT_Z_LOW   0x10      /**< Enable interrupt generation on Z low event or on Direction recognition*/
#define ACCEL_LIS2DH_INTEVENT_Y_HIGH  0x08      /**< Enable interrupt generation on Y high event or on Direction recognition*/
#define ACCEL_LIS2DH_INTEVENT_Y_LOW   0x04      /**< Enable interrupt generation on Y low event or on Direction recognition*/
#define ACCEL_LIS2DH_INTEVENT_X_HIGH  0x02      /**< Enable interrupt generation on X high event or on Direction recognition*/
#define ACCEL_LIS2DH_INTEVENT_X_LOW   0x01      /**< Enable interrupt generation on X low event or on Direction recognition*/
#define ACCEL_LIS2DH_INTEVENT_NONE    0x00      /**< No interrupt mask */
#define ACCEL_LIS2DH_INTEVENT_ALL     0x3F      /**< All interrupt mask */
/** @} lis2dh_int_config */

// clang-format on

/****************************************************************************
 * Data types
 ****************************************************************************/

/**
 * @defgroup lis2dh_data_types Accelerometer LIS2DH Handler Types
 * @{
 */
/**
 * @typedef accel_event_handler
 * Definition of callback function.
 * User callback function type for accelerometer event
 * @param[in] ev : Event mask for this notification.
 * @param[in] user_param : Parameter for this callback.
 */
typedef void (*accel_event_handler)(uint8_t ev, void *user_param);

/**
 * @brief xyz_mg_t
 * Definition of accelerator raw data type.
 * This used for accel_get_xyz_mg
 */
typedef struct {
  int16_t x; /**< Raw data of X axis */
  int16_t y; /**< Raw data of Y axis */
  int16_t z; /**< Raw data of Z axis */
} xyz_mg_t;

// clang-format off

/**
 * @brief accel_ev_cfg_t
 * Definition of parameters for accelerometer event monitor
 */
typedef struct {
  accel_event_handler ev_h;  /**< Registered user callback */
  void *user_param;          /**< Parameter to pass to user callback */
  uint8_t ev_mode;           /**< Event mode to monitor. Possible values are ACCEL_LIS2DH_INT_MODE_OR, ACCEL_LIS2DH_INT_MODE_AND
                                  ACCEL_LIS2DH_INT_MODE_MOVE, ACCEL_LIS2DH_INT_MODE_POS*/
  uint8_t ev_type;           /**< Event type to monitor. Possible values are ACCEL_LIS2DH_INTEVENT_Z_HIGH, ACCEL_LIS2DH_INTEVENT_Z_LOW, ...*/
  uint16_t ev_thr_mg;        /**< Threshold to trigger the event. This setting is in mg.*/
  uint32_t ev_dur_ms;        /**< Duration to trigger the event. If the movement fullfill the ev_mode, ev_type, ev_thr_mg
                                  longer than this setting in ms, the event will be triggered.*/
  eMcuGpioIds int_pin_set;   /**< MCU gpio to connect to the accelerometer INTx pin*/
} accel_ev_cfg_t;

/**
 * @typedef accel_ret_e
 * Definition of accel application APIs retern code
 */
typedef enum {
  ACCEL_RET_SUCCESS,        /**< API returns with no error*/
  ACCEL_RET_NOT_INIT,       /**< API returns error because initialization has not been done.*/
  ACCEL_RET_PARAM_ERR,      /**< API parameter error.*/
  ACCEL_RET_GENERIC_ERR,    /**< API generic error.*/
  ACCEL_RET_TIMEOUT,        /**< API return error because it failed to take event mutex.*/
  ACCEL_RET_NO_DATA         /**< API return error indicates there is no new data available*/
} accel_ret_e;

/**
 * @typedef accel_ev_id_e
 * Definition of event ID. The maximum interrupt source of lis2dh is 2.
 */
typedef enum {
    ACCEL_EVENT_0,
    ACCEL_EVENT_1,
    ACCEL_MAX_EVENT
} accel_ev_id_e;

/**
 * @typedef accel_fifo_mode_e
 * Definition of accelerometer FIFO mode
 */
typedef enum {
  ACCEL_FIFO_MODE_BYPASS,          /**< FIFO is not operational and for this reason it remains empty.*/
  ACCEL_FIFO_MODE_FIFO,            /**< The buffer continues filling data and it stops collecting data after FIFO is full.*/
  ACCEL_FIFO_MODE_STREAM,          /**< FIFO buffer index restarts from the beginning and older data is replaced by the current data when FIFO is full*/
  ACCEL_FIFO_MODE_STREAM_FIFO_TR0, /**< Switching modes from STREAM to FIFO dynamically performed according to the pin value of INT1*/
  ACCEL_FIFO_MODE_STREAM_FIFO_TR1  /**< Switching modes from STREAM to FIFO dynamically performed according to the pin value of INT2*/
} accel_fifo_mode_e;
/** @} lis2dh_data_types */

// clang-format on

/*******************************************************************************
 * API
 ******************************************************************************/

/**
 * @defgroup lis2dh_apis Accelerometer LIS2DH Handler APIs
 * @{
 */
/**
 * @brief This is the initialization call before all other Accelerometer API calls.
 * This function will create a task (called ACCEL TASK)to configure the accelerometer by
 * i2c and monitor accelerometer event and call the registered callback to notify user application.
 * Since Accelerometer depends on i2c and gpio driver, so application should call i2c_init and
 * gpio_init first.
 *
 * @return @ref accel_ret_e
 */
accel_ret_e accel_init(void);

/**
 * @brief This is the main API for accelerometer to monitor any event configured in the event_config
 * structure. lis2dh support two interrupt sources. So there will be two events can be configured.
 * The registered handler function is called by process context of ACCEL TASK.
 *
 * @param [in] event_id: @ref accel_ev_id_e
 * @param [in] event_config: @ref accel_ev_cfg_t
 *
 * @return @ref accel_ret_e
 */
accel_ret_e accel_start(accel_ev_id_e event_id, accel_ev_cfg_t *event_config);

/**
 * @brief The behavior of this API is same as accel_start but without enable INT pin of
 * accelerometer which means accel EV status will be masked after this API returns.
 *
 * @param [in] event_id: @ref accel_ev_id_e
 * @param [in] event_config: @ref accel_ev_cfg_t
 *
 * @return @ref accel_ret_e
 */
accel_ret_e accel_start_with_masked(accel_ev_id_e event_id, accel_ev_cfg_t *event_config);
/**
 * @brief Stop the accel event monitor.
 * This API clears interrupt configuration of accelerometer, and disable user application
 * notification of accelerometer event. After this API called, accelerometer will not raise
 * interrupt event anymore until accel_start is called.
 *
 * @param [in] event_id: @ref accel_ev_id_e.
 *
 * @return @ref accel_ret_e
 */
accel_ret_e accel_stop(accel_ev_id_e event_id);

/**
 * @brief Mask the interrupt event without clear interrupt configuration of accelerometer.
 * This API only mask the interrupt notification of user application,
 * but accelerometer will still raise interrupt signal if accel event occurred.
 *
 * @param [in] irq_mask: Specify interrupt source to be mask.
 *
 * @return @ref accel_ret_e
 */
accel_ret_e accel_irqx_mask(uint8_t irq_mask);

/**
 * @brief Unmask the interrupt event to user application.
 * This API only restores the interrupt notification to user callback but without touching the
 * interrupt configuration of accelerometer.
 *
 * @param [in] irq_mask: Specify interrupt source to be unmask.
 *
 * @return @ref accel_ret_e
 */
accel_ret_e accel_irqx_unmask(uint8_t irq_mask);

/**
 * @brief Read the accelerator raw data in mg for x, y, and z axis.
 *
 * @param [out] accel_mg: @ref xyz_mg_t.
 *
 * @return @ref accel_ret_e
 */
accel_ret_e accel_get_xyz_mg(xyz_mg_t *accel_mg);

/**
 * @brief Read the temperature data.
 *
 * @param [out] temperature_degC: Pointer to the temperature_degC to read data.
 *
 * @return @ref accel_ret_e.
 */
accel_ret_e accel_get_temp_celsius(int16_t *temperature_degC);

/**
 * @brief Query if there is accelerator raw data to read.
 *
 * @return return 1 there is data ready to read otherwise return 0.
 */
int accel_has_data(void);

/**
 * @brief Query if there is new temperature data to read.
 *
 * @return return 1 there is new temperature data ready to read otherwise return 0.
 */
int accel_temp_data_ready(void);

/**
 * @brief Dump accelerometer configuration.
 *
 * @return None.
 */
void accel_dump_config(void);

/**
 * @brief Configure accel power mode.
 *
 * @param [in] pwr_state: Set 0 configure accelerometer power off and 1 to power on.
 *
 * @return @ref accel_ret_e.
 */
accel_ret_e accel_config_power(uint8_t pwr_state);

/**
 * @brief Querry accel ODR value.
 *
 * @param [out] odr value: lis2dh12_odr_t.
 *
 * @return @ref accel_ret_e.
 */
accel_ret_e accel_get_odr(lis2dh12_odr_t *odr);

/**
 * @brief Configure accel ODR value.
 *
 * @param [in] odr value: lis2dh12_odr_t.
 *
 * @return @ref accel_ret_e.
 */
accel_ret_e accel_config_odr(lis2dh12_odr_t odr);

/**
 * @brief Reset FIFO buffer
 *
 * @return @ref accel_ret_e.
 */
accel_ret_e accel_reset_fifo_buffer();
/**
 * @brief Get current data level in FIFO
 *
 * @return FIFO data level
 */
uint8_t accel_get_fifo_data_level();

/**
 * @brief Load accel raw data stored in FIFO
 *
 * @param [out] accel_mg_buf: buffer to stored raw data inside accel FIFO with type@ref xyz_mg_t.
 *
 * @param [in] buf_size: maximum data count to be load. If should be equal to the element count of
 * accel_mg_buf
 *
 * @return Actual data loaded to accel_mg_buf after this API returns
 */
uint8_t accel_get_fifo_xyz_mg(xyz_mg_t *accel_mg_buf, uint8_t buf_size);

/**
 * @brief Query current FIFO enable status
 *
 * @param [out] enable: FIFO enable status
 *
 * @return @ref accel_ret_e.
 */
accel_ret_e accel_get_fifo_enabled(uint8_t *enable);

/**
 * @brief Query current configured FIFO mode
 *
 * @param [out] mode: FIFO mode with type @ref accel_fifo_mode_e
 *
 * @return @ref accel_ret_e.
 */
accel_ret_e accel_get_fifo_mode(accel_fifo_mode_e *mode);

/**
 * @brief Configure accel FIFO.
 *
 * @param [in] enable: 1 to enable FIFO and 0 to disable FIFO
 *
 * @param [in] mode: FIFO mode with type @ref accel_fifo_mode_e, when enable = 0 mode is
 * meaningless.
 *
 * @return @ref accel_ret_e.
 */
accel_ret_e accel_config_fifo(uint8_t enable, accel_fifo_mode_e mode);

/**
 * @brief Configure accel high pass filter enable status on output registers and FIFO
 *
 * @param [in] enable: 1 to enable 0 to disable
 *
 * @return @ref accel_ret_e.
 */
accel_ret_e accel_config_hp_filter_on_output(uint8_t enable);

/**
 * @brief Query accel high pass filter enable status on output registers and FIFO
 *
 * @param [out] Current enable status
 *
 * @return @ref accel_ret_e.
 */
accel_ret_e accel_get_hp_filter_on_output(uint8_t *enable);
/** @} lis2dh_apis */
/** @} accel_lis2dh */

#ifdef __cplusplus
}
#endif
#endif
