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

#include <stdio.h>
#include <string.h>
#include "accel_lis2dh.h"
#include "gpio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

#if (configUSE_ALT1250_SLEEP == 1)
#include "sleep_mngr.h"
#include "pwr_mngr.h"
#endif

#define ACCEL_INT_LEVEL_TRIGGER
#define ACCEL_INIT_RETRY 5

#ifdef ACCEL_DEBUG_MSG
#define ACCEL_DBG(...) printf(__VA_ARGS__)
#else
#define ACCEL_DBG(...)
#endif

#define ACCEL_ERR(...) printf(__VA_ARGS__)

#define ACCEL_ASSERT(...) \
  do {                    \
    printf(__VA_ARGS__);  \
    configASSERT(0)       \
  } while (0)

static stmdev_ctx_t lis2dh_ctx;

typedef enum {
  EV_STOPPED = 0,
  EV_PENDING_START,
  EV_PENDING_MASKED_START,
  EV_PENDING_STOP,
  EV_STARTED,
  EV_MASKED
} ev_status_e;

typedef struct {
  ev_status_e ev_status;
  accel_ev_cfg_t ev_cfg;
} accel_ev_table_t;

static accel_ev_table_t accel_ev_tbl[ACCEL_MAX_EVENT];

#if (configUSE_ALT1250_SLEEP == 1)
#define ACCEL_SMNGR_DEVNAME "Accelerometer-LIS2DH"
unsigned int accel_smngr_sync_id;
#endif
/*This flag is set after accel_init is done*/
static int g_accel_basic_init_done = 0;
/*This is flag is set after ACCEL_TASK finished accelerometer configuration*/
static int g_accel_init_done = 0;

static lis2dh12_fs_t g_accel_fs = ACCEL_LIS2DH_SCALE;
static lis2dh12_op_md_t g_accel_op_md = ACCEL_LIS2DH_OP_MODE;
static lis2dh12_odr_t g_accel_odr = ACCEL_LIS2DH_ODR;

static xQueueHandle accel_ev_queue = NULL;

static SemaphoreHandle_t ev_lock = NULL;
#define EV_LOCK_DELAY_MS 5000
static const TickType_t ev_lock_delay = EV_LOCK_DELAY_MS / portTICK_PERIOD_MS;

static SemaphoreHandle_t platform_rw_lock = NULL;
#define PLATFORM_LOCK_DELAY_MS 5000
static const TickType_t platform_lock_delay = PLATFORM_LOCK_DELAY_MS / portTICK_PERIOD_MS;

typedef union {
  int16_t i16bit[3];
  uint8_t u8bit[6];
} axis3bit16_t;

typedef union {
  int16_t i16bit;
  uint8_t u8bit[2];
} axis1bit16_t;

static int32_t alt_i2c_read(void *handle, uint8_t reg, uint8_t *readBuf, uint16_t len) {
  int res = 0;

  if (len > 1) reg |= 0x80;

  if (xSemaphoreTake(platform_rw_lock, platform_lock_delay) == pdTRUE) {
#if (configUSE_ALT1250_SLEEP == 1)
    smngr_dev_busy_set(accel_smngr_sync_id);
#endif
    res = i2c_read_advanced_mode(ACCEL_SENSOR_ADDR, I2C_DEVID_MODE_7BITS, reg, I2C_ADDRLEN_1BYTE,
                                 len, readBuf, ACCEL_SENSOR_BUS);
#if (configUSE_ALT1250_SLEEP == 1)
    smngr_dev_busy_clr(accel_smngr_sync_id);
#endif
    xSemaphoreGive(platform_rw_lock);
  } else {
    ACCEL_ERR("Failed to obtain platform read write lock\r\n");
    res = -1;
  }
  return res;
}

static int32_t alt_i2c_write(void *handle, uint8_t reg, uint8_t *writeBuf, uint16_t len) {
  int res = 0;

  if (len > 1) reg |= 0x80;

  if (xSemaphoreTake(platform_rw_lock, platform_lock_delay) == pdTRUE) {
#if (configUSE_ALT1250_SLEEP == 1)
    smngr_dev_busy_set(accel_smngr_sync_id);
#endif
    res = i2c_write_advanced_mode(ACCEL_SENSOR_ADDR, I2C_DEVID_MODE_7BITS, reg, I2C_ADDRLEN_1BYTE,
                                  len, writeBuf, ACCEL_SENSOR_BUS);
#if (configUSE_ALT1250_SLEEP == 1)
    smngr_dev_busy_clr(accel_smngr_sync_id);
#endif
    xSemaphoreGive(platform_rw_lock);
  } else {
    ACCEL_ERR("Failed to obtain platform read write lock\r\n");
    res = -1;
  }
  return res;
}

static void accel_interrupt_handler(unsigned int user_param) {
  uint8_t indication = 0;
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  accel_ev_id_e ev_id = (accel_ev_id_e)user_param;

  indication = (uint8_t)ev_id;

#ifdef ACCEL_INT_LEVEL_TRIGGER
  int gpio_value = 0;
  eMcuGpioIds gpio_id = accel_ev_tbl[ev_id].ev_cfg.int_pin_set;

  gpio_get_value(gpio_id, &gpio_value);

  if (gpio_value) {
    gpio_config_interrupt(gpio_id, GPIO_IRQ_LOW_LEVEL, GPIO_PULL_DONT_CHANGE);
    xQueueSendToBackFromISR(accel_ev_queue, &indication, &xHigherPriorityTaskWoken);
  } else {
    gpio_config_interrupt(gpio_id, GPIO_IRQ_HIGH_LEVEL, GPIO_PULL_DONT_CHANGE);
  }
#else
  xQueueSendToBackFromISR(accel_ev_queue, &indication, &xHigherPriorityTaskWoken);
#endif
  if (xHigherPriorityTaskWoken) {
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}

static accel_ret_e accel_intx_clear_sync(accel_ev_id_e ev_id) {
  eMcuGpioIds gpio_id = accel_ev_tbl[ev_id].ev_cfg.int_pin_set;
  eGpioRet gpio_ret;
  uint16_t poll_count = 2048;
  uint8_t int_src = 0;
  int res = 0, gpio_value;

  do {
    gpio_ret = gpio_get_value(gpio_id, &gpio_value);
    if (ev_id == ACCEL_EVENT_0)
      res = lis2dh12_read_reg(&lis2dh_ctx, LIS2DH12_INT1_SRC, &int_src, 1);
    else if (ev_id == ACCEL_EVENT_1)
      res = lis2dh12_read_reg(&lis2dh_ctx, LIS2DH12_INT2_SRC, &int_src, 1);
    else
      int_src = 0;
    poll_count--;
  } while ((gpio_value == 1 || int_src != 0) && gpio_ret == GPIO_RET_SUCCESS && res == 0 &&
           poll_count != 0);

  if (poll_count == 0 && (gpio_value == 1 || int_src != 0)) {
    ACCEL_ERR("Failed to clear accel int%d gpio_value: %d int_src: %x\r\n", ev_id + 1, gpio_value,
              int_src);
    return ACCEL_RET_GENERIC_ERR;
  }
  return ACCEL_RET_SUCCESS;
}

static accel_ret_e clear_gpio_interrupt(accel_ev_id_e ev_id) {
  eMcuGpioIds gpio_id = accel_ev_tbl[ev_id].ev_cfg.int_pin_set;

  gpio_disable_interrupt(gpio_id);

#ifdef ACCEL_INT_LEVEL_TRIGGER
  eGpioPullMode gpio_pull;
  eGpioRet gpio_ret;

  if ((gpio_ret = gpio_get_pull(gpio_id, &gpio_pull)) != GPIO_RET_SUCCESS) {
    ACCEL_ERR("Unable to get gpio%d pull mode: %d\r\n", gpio_id, gpio_ret);
    return ACCEL_RET_GENERIC_ERR;
  }
  if ((gpio_ret = gpio_config_interrupt(gpio_id, GPIO_IRQ_HIGH_LEVEL, gpio_pull)) !=
      GPIO_RET_SUCCESS) {
    ACCEL_ERR("Failed to restore gpio interrupt to high level\r\n");
    return ACCEL_RET_GENERIC_ERR;
  }
#endif

#if (configUSE_ALT1250_SLEEP == 1)
  PWR_Del_Monitored_Pin(gpio_id);
#endif
  return ACCEL_RET_SUCCESS;
}

static accel_ret_e config_gpio_interrupt(accel_ev_id_e ev_id) {
  eGpioPullMode gpio_pull;
  eGpioRet gpio_ret;
  eMcuGpioIds gpio_id = accel_ev_tbl[ev_id].ev_cfg.int_pin_set;

  if ((gpio_ret = gpio_get_pull(gpio_id, &gpio_pull)) != GPIO_RET_SUCCESS) {
    ACCEL_ERR("Unable to get gpio%d pull mode: %d\r\n", gpio_id, gpio_ret);
    return ACCEL_RET_GENERIC_ERR;
  }

#ifndef ACCEL_INT_LEVEL_TRIGGER
  int gpio_value;
  uint8_t indication = 0;
  indication = (uint8_t)ev_id;

  if ((gpio_ret = gpio_config_interrupt(gpio_id, GPIO_IRQ_RISING_EDGE, gpio_pull)) ==
          GPIO_RET_SUCCESS &&
      (gpio_ret = gpio_register_interrupt(gpio_id, accel_interrupt_handler, ev_id)) ==
          GPIO_RET_SUCCESS &&
      (gpio_ret = gpio_enable_interrupt(gpio_id)) == GPIO_RET_SUCCESS) {
    ACCEL_DBG("Setting gpio %d as interrupt with rising edge trigger\r\n", gpio_id);
  } else {
    ACCEL_ERR("Failed to set gpio %d as interrupt: %d\r\n", gpio_id, gpio_ret);
    return ACCEL_RET_GENERIC_ERR;
  }

  if (((gpio_ret = gpio_get_value(gpio_id, &gpio_value)) == GPIO_RET_SUCCESS) &&
      (gpio_value == 1)) {
    xQueueSendToBack(accel_ev_queue, &indication, 0);
  }

#else
  if ((gpio_ret = gpio_config_interrupt(gpio_id, GPIO_IRQ_HIGH_LEVEL, gpio_pull)) ==
          GPIO_RET_SUCCESS &&
      (gpio_ret = gpio_register_interrupt(gpio_id, accel_interrupt_handler, ev_id)) ==
          GPIO_RET_SUCCESS &&
      (gpio_ret = gpio_enable_interrupt(gpio_id)) == GPIO_RET_SUCCESS) {
    ACCEL_DBG("Setting gpio %d as interrupt with high level trigger\r\n", gpio_id);
  } else {
    ACCEL_ERR("Failed to set gpio %d as interrupt: %d\r\n", gpio_id, gpio_ret);
    return ACCEL_RET_GENERIC_ERR;
  }

#endif

#if (configUSE_ALT1250_SLEEP == 1)
  if (PWR_Set_Monitored_Pin(gpio_id, 1) != PWR_ERR_NO_ERROR) {
    ACCEL_ERR("Failed to set PWR monitored pin\r\n");
    return ACCEL_RET_GENERIC_ERR;
  }
#endif
  return ACCEL_RET_SUCCESS;
}

static uint32_t convert_int_duration_ms(uint32_t dur, lis2dh12_odr_t target_odr) {
  uint32_t duration;

  switch (target_odr) {
    case LIS2DH12_ODR_1Hz:
      duration = (uint32_t)(dur / 1000);
      break;
    case LIS2DH12_ODR_10Hz:
      duration = (uint32_t)(dur / 100);
      break;
    case LIS2DH12_ODR_25Hz:
      duration = (uint32_t)(dur / 40);
      break;
    case LIS2DH12_ODR_50Hz:
      duration = (uint32_t)(dur / 20);
      break;
    case LIS2DH12_ODR_100Hz:
      duration = (uint32_t)(dur / 10);
      break;
    case LIS2DH12_ODR_200Hz:
      duration = (uint32_t)(dur / 5);
      break;
    case LIS2DH12_ODR_400Hz:
      duration = (uint32_t)((dur * 2) / 5);
      break;
    case LIS2DH12_ODR_1kHz620_LP:
      duration = (uint32_t)((dur * 100) / 617);
      break;
    case LIS2DH12_ODR_5kHz376_LP_1kHz344_NM_HP:
      duration = (uint32_t)((dur * 5376) / 1000);
      break;
    default:
      ACCEL_ERR("Unable to determine the duration of interrupt, exiting\n");
      return 0xFF;
      break;
  }
  return duration;
}

static uint32_t calculate_new_duration(uint8_t orig_dur, lis2dh12_odr_t target_odr) {
  uint32_t duration_ms;

  switch (g_accel_odr) {
    case LIS2DH12_ODR_1Hz:
      duration_ms = (uint32_t)(orig_dur * 1000);
      break;
    case LIS2DH12_ODR_10Hz:
      duration_ms = (uint32_t)(orig_dur * 100);
      break;
    case LIS2DH12_ODR_25Hz:
      duration_ms = (uint32_t)(orig_dur * 40);
      break;
    case LIS2DH12_ODR_50Hz:
      duration_ms = (uint32_t)(orig_dur * 20);
      break;
    case LIS2DH12_ODR_100Hz:
      duration_ms = (uint32_t)(orig_dur * 10);
      break;
    case LIS2DH12_ODR_200Hz:
      duration_ms = (uint32_t)(orig_dur * 5);
      break;
    case LIS2DH12_ODR_400Hz:
      duration_ms = (uint32_t)((orig_dur * 5) / 2);
      break;
    case LIS2DH12_ODR_1kHz620_LP:
      duration_ms = (uint32_t)((orig_dur * 617) / 100);
      break;
    case LIS2DH12_ODR_5kHz376_LP_1kHz344_NM_HP:
      duration_ms = (uint32_t)((orig_dur * 1000) / 5376);
      break;
    default:
      ACCEL_ERR("Unable to determine the duration of interrupt, exiting\n");
      return 0xFF;
      break;
  }
  return convert_int_duration_ms(duration_ms, target_odr);
}

static uint8_t convert_int_threshold_mg(uint16_t thr) {
  uint8_t threshold;

  switch (g_accel_fs) {
    case LIS2DH12_2g:
      threshold = (uint8_t)(thr / 16);
      break;
    case LIS2DH12_4g:
      threshold = (uint8_t)(thr / 32);
      break;
    case LIS2DH12_8g:
      threshold = (uint8_t)(thr / 62);
      break;
    case LIS2DH12_16g:
      threshold = (uint8_t)(thr / 186);
      break;
    default:
      ACCEL_ERR("Unable to determine the threshold of interrupt, exiting\n");
      return 0xFF;
      break;
  }
  return threshold;
}

static void accel_clear_int2() {
  lis2dh12_int2_cfg_t m_lis2dh12_int_cfg;
  int res = 0;
  uint8_t int_src = 0;
  uint16_t poll_count = 2048;
  memset(&m_lis2dh12_int_cfg, 0x0, sizeof(m_lis2dh12_int_cfg));
  lis2dh12_int2_gen_conf_set(&lis2dh_ctx, &m_lis2dh12_int_cfg);

  lis2dh12_int2_gen_threshold_set(&lis2dh_ctx, 0x0);

  lis2dh12_int2_gen_duration_set(&lis2dh_ctx, 0x0);

  lis2dh12_ctrl_reg6_t reg6;
  lis2dh12_pin_int2_config_get(&lis2dh_ctx, &reg6);
  reg6.i2_ia2 = 0;
  lis2dh12_pin_int2_config_set(&lis2dh_ctx, &reg6);

  lis2dh12_int2_pin_notification_mode_set(&lis2dh_ctx, LIS2DH12_INT2_PULSED);
  do {
    res = lis2dh12_read_reg(&lis2dh_ctx, LIS2DH12_INT2_SRC, &int_src, 1);
    poll_count--;
  } while (((res != 0) || (int_src & 0x40)) && poll_count != 0);
  if (int_src & 0x40) ACCEL_ERR("Failed to clear int2: %x\r\n", int_src);
}
#define ACCEL_LIS2DH_INT_MODE_SHIFT 6
#define ACCEL_LIS2DH_INTEVNET_AOI 0x80
#define ACCEL_LIS2DH_INTEVNET_6D 0x40

static accel_ret_e accel_config_int2(uint8_t int_pin_en) {
  accel_ev_cfg_t *ev_cfg = &accel_ev_tbl[1].ev_cfg;

  uint32_t duration = convert_int_duration_ms(ev_cfg->ev_dur_ms, g_accel_odr);
  uint16_t threshold = convert_int_threshold_mg(ev_cfg->ev_thr_mg);

  lis2dh12_int2_cfg_t m_lis2dh12_int_cfg;
  memset(&m_lis2dh12_int_cfg, 0x0, sizeof(m_lis2dh12_int_cfg));

  if (duration < 0x80) {
    lis2dh12_int2_gen_duration_set(&lis2dh_ctx, (uint8_t)duration);
  } else {
    ACCEL_ERR("Invalid duration value\n");
    return ACCEL_RET_PARAM_ERR;
  }

  if (threshold < 0x80) {
    lis2dh12_int2_gen_threshold_set(&lis2dh_ctx, threshold);
    ACCEL_DBG("Threshold for interrupt2: %u\n", threshold);
  } else {
    ACCEL_ERR("Invalid threshold value\n");
    return ACCEL_RET_PARAM_ERR;
  }

  if ((ev_cfg->ev_mode << ACCEL_LIS2DH_INT_MODE_SHIFT) & ACCEL_LIS2DH_INTEVNET_AOI)
    m_lis2dh12_int_cfg.aoi = 1;
  if ((ev_cfg->ev_mode << ACCEL_LIS2DH_INT_MODE_SHIFT) & ACCEL_LIS2DH_INTEVNET_6D)
    m_lis2dh12_int_cfg._6d = 1;
  if (ev_cfg->ev_type & ACCEL_LIS2DH_INTEVENT_Z_HIGH) m_lis2dh12_int_cfg.zhie = 1;
  if (ev_cfg->ev_type & ACCEL_LIS2DH_INTEVENT_Z_LOW) m_lis2dh12_int_cfg.zlie = 1;
  if (ev_cfg->ev_type & ACCEL_LIS2DH_INTEVENT_Y_HIGH) m_lis2dh12_int_cfg.yhie = 1;
  if (ev_cfg->ev_type & ACCEL_LIS2DH_INTEVENT_Y_LOW) m_lis2dh12_int_cfg.ylie = 1;
  if (ev_cfg->ev_type & ACCEL_LIS2DH_INTEVENT_X_HIGH) m_lis2dh12_int_cfg.xhie = 1;
  if (ev_cfg->ev_type & ACCEL_LIS2DH_INTEVENT_X_LOW) m_lis2dh12_int_cfg.xlie = 1;

  lis2dh12_int2_gen_conf_set(&lis2dh_ctx, &m_lis2dh12_int_cfg);

  lis2dh12_ctrl_reg6_t reg6;
  lis2dh12_pin_int2_config_get(&lis2dh_ctx, &reg6);
  reg6.i2_ia2 = int_pin_en;
  reg6.int_polarity = 0;
  lis2dh12_pin_int2_config_set(&lis2dh_ctx, &reg6);

  lis2dh12_int2_pin_notification_mode_set(&lis2dh_ctx, LIS2DH12_INT2_LATCHED);

  lis2dh12_int2_pin_detect_4d_set(&lis2dh_ctx, 0);
  return ACCEL_RET_SUCCESS;
}

static void accel_clear_int1() {
  int res = 0;
  uint8_t int_src = 0;
  uint16_t poll_count = 2048;
  lis2dh12_int1_cfg_t m_lis2dh12_int_cfg;
  memset(&m_lis2dh12_int_cfg, 0x0, sizeof(m_lis2dh12_int_cfg));
  lis2dh12_int1_gen_conf_set(&lis2dh_ctx, &m_lis2dh12_int_cfg);

  lis2dh12_int1_gen_threshold_set(&lis2dh_ctx, 0x0);

  lis2dh12_int1_gen_duration_set(&lis2dh_ctx, 0x0);

  lis2dh12_ctrl_reg3_t reg3;
  lis2dh12_pin_int1_config_get(&lis2dh_ctx, &reg3);
  reg3.i1_ia1 = 0;
  lis2dh12_pin_int1_config_set(&lis2dh_ctx, &reg3);

  lis2dh12_int1_pin_notification_mode_set(&lis2dh_ctx, LIS2DH12_INT1_PULSED);
  do {
    res = lis2dh12_read_reg(&lis2dh_ctx, LIS2DH12_INT1_SRC, &int_src, 1);
    poll_count--;
  } while (((res != 0) || (int_src & 0x40)) && poll_count != 0);
  if (int_src & 0x40) ACCEL_ERR("Failed to clear int1: %x\r\n", int_src);
}

static accel_ret_e accel_config_int1(uint8_t int_pin_en) {
  accel_ev_cfg_t *ev_cfg = &accel_ev_tbl[0].ev_cfg;

  uint32_t duration = convert_int_duration_ms(ev_cfg->ev_dur_ms, g_accel_odr);
  uint16_t threshold = convert_int_threshold_mg(ev_cfg->ev_thr_mg);

  lis2dh12_int1_cfg_t m_lis2dh12_int_cfg;
  memset(&m_lis2dh12_int_cfg, 0x0, sizeof(m_lis2dh12_int_cfg));

  if (duration < 0x80) {
    lis2dh12_int1_gen_duration_set(&lis2dh_ctx, (uint8_t)duration);
  } else {
    ACCEL_ERR("Invalid duration value\n");
    return ACCEL_RET_PARAM_ERR;
  }

  if (threshold < 0x80) {
    lis2dh12_int1_gen_threshold_set(&lis2dh_ctx, threshold);
    ACCEL_DBG("Threshold for interrupt1: %u\n", threshold);
  } else {
    ACCEL_ERR("Invalid threshold value\n");
    return ACCEL_RET_PARAM_ERR;
  }

  if ((ev_cfg->ev_mode << ACCEL_LIS2DH_INT_MODE_SHIFT) & ACCEL_LIS2DH_INTEVNET_AOI)
    m_lis2dh12_int_cfg.aoi = 1;
  if ((ev_cfg->ev_mode << ACCEL_LIS2DH_INT_MODE_SHIFT) & ACCEL_LIS2DH_INTEVNET_6D)
    m_lis2dh12_int_cfg._6d = 1;
  if (ev_cfg->ev_type & ACCEL_LIS2DH_INTEVENT_Z_HIGH) m_lis2dh12_int_cfg.zhie = 1;
  if (ev_cfg->ev_type & ACCEL_LIS2DH_INTEVENT_Z_LOW) m_lis2dh12_int_cfg.zlie = 1;
  if (ev_cfg->ev_type & ACCEL_LIS2DH_INTEVENT_Y_HIGH) m_lis2dh12_int_cfg.yhie = 1;
  if (ev_cfg->ev_type & ACCEL_LIS2DH_INTEVENT_Y_LOW) m_lis2dh12_int_cfg.ylie = 1;
  if (ev_cfg->ev_type & ACCEL_LIS2DH_INTEVENT_X_HIGH) m_lis2dh12_int_cfg.xhie = 1;
  if (ev_cfg->ev_type & ACCEL_LIS2DH_INTEVENT_X_LOW) m_lis2dh12_int_cfg.xlie = 1;

  lis2dh12_int1_gen_conf_set(&lis2dh_ctx, &m_lis2dh12_int_cfg);

  lis2dh12_ctrl_reg3_t reg3;
  lis2dh12_pin_int1_config_get(&lis2dh_ctx, &reg3);
  reg3.i1_ia1 = int_pin_en;
  lis2dh12_pin_int1_config_set(&lis2dh_ctx, &reg3);

  lis2dh12_ctrl_reg6_t reg6;
  lis2dh12_pin_int2_config_get(&lis2dh_ctx, &reg6);
  reg6.int_polarity = 0;
  lis2dh12_pin_int2_config_set(&lis2dh_ctx, &reg6);

  lis2dh12_int1_pin_notification_mode_set(&lis2dh_ctx, LIS2DH12_INT1_LATCHED);

  lis2dh12_int1_pin_detect_4d_set(&lis2dh_ctx, 0);
  return ACCEL_RET_SUCCESS;
}

static accel_ret_e lis2dh_global_init(void) {
  uint8_t id = 0;
  lis2dh12_reg_t dummy_reg;
  axis1bit16_t dummy_temp;

  lis2dh12_fs_t c_fs;
  lis2dh12_op_md_t c_op_md;
  lis2dh12_odr_t c_odr;

  lis2dh12_device_id_get(&lis2dh_ctx, &id);

  if (id == 0x33) {
    /*According to DS: It is recommended to read register REFERENCE (26h) for a complete reset of
     * the filtering block*/
    if (lis2dh12_filter_reference_get(&lis2dh_ctx, &dummy_reg.byte) != 0)
      return ACCEL_RET_GENERIC_ERR;

    ACCEL_DBG("Dummy read reference reg: %x\r\n", dummy_reg.byte);

    if (lis2dh12_block_data_update_set(&lis2dh_ctx, PROPERTY_ENABLE) != 0)
      return ACCEL_RET_GENERIC_ERR;

    if (lis2dh12_data_rate_set(&lis2dh_ctx, g_accel_odr) != 0) return ACCEL_RET_GENERIC_ERR;

    if (lis2dh12_full_scale_set(&lis2dh_ctx, g_accel_fs) != 0) return ACCEL_RET_GENERIC_ERR;

    if (lis2dh12_temperature_meas_set(&lis2dh_ctx, LIS2DH12_TEMP_ENABLE) != 0)
      return ACCEL_RET_GENERIC_ERR;

    if (lis2dh12_operating_mode_set(&lis2dh_ctx, g_accel_op_md) != 0) return ACCEL_RET_GENERIC_ERR;

    if (lis2dh12_high_pass_int_conf_set(&lis2dh_ctx, LIS2DH12_ON_INT1_INT2_GEN) != 0)
      return ACCEL_RET_GENERIC_ERR;

    if (lis2dh12_operating_mode_get(&lis2dh_ctx, &c_op_md) != 0) return ACCEL_RET_GENERIC_ERR;

    if (lis2dh12_full_scale_get(&lis2dh_ctx, &c_fs) != 0) return ACCEL_RET_GENERIC_ERR;

    if (lis2dh12_data_rate_get(&lis2dh_ctx, &c_odr) != 0) return ACCEL_RET_GENERIC_ERR;

    /*Dummy read OUT_TEMP_L OUT_TEMP_H to make TDA in STATUS_AUX register working*/
    memset(dummy_temp.u8bit, 0x00, sizeof(int16_t));
    if (lis2dh12_temperature_raw_get(&lis2dh_ctx, dummy_temp.u8bit) != 0)
      return ACCEL_RET_GENERIC_ERR;

    if (g_accel_op_md != c_op_md || g_accel_fs != c_fs || g_accel_odr != c_odr) {
      ACCEL_ERR("LIS2DH init check failed op mode :%X, scale: %X, odr: %X\r\n", g_accel_op_md,
                g_accel_fs, g_accel_odr);
      return ACCEL_RET_GENERIC_ERR;
    }

    accel_clear_int1();
    accel_clear_int2();
  } else {
    ACCEL_ERR("Failed to probe LIS2DH (read ID: %x)\r\n", id);
    return ACCEL_RET_GENERIC_ERR;
  }

  ACCEL_DBG("LIS2DH global init done op mode :%X, scale: %X, odr: %X\r\n", g_accel_op_md,
            g_accel_fs, g_accel_odr);
  return ACCEL_RET_SUCCESS;
}

#if (configUSE_ALT1250_SLEEP == 1)
static accel_ret_e lis2dh_light_init() {
  lis2dh12_odr_t c_odr;
  lis2dh12_reg_t dummy_reg;

  g_accel_op_md = ACCEL_LIS2DH_OP_MODE;
  g_accel_fs = ACCEL_LIS2DH_SCALE;

  if (lis2dh12_data_rate_get(&lis2dh_ctx, &c_odr) != 0) return ACCEL_RET_GENERIC_ERR;
  /*In case accel_config_power was called and set to power down*/
  if (c_odr == LIS2DH12_POWER_DOWN) {
    /*According to DS: It is recommended to read register REFERENCE (26h) for a complete reset of
     * the filtering block*/
    if (lis2dh12_filter_reference_get(&lis2dh_ctx, &dummy_reg.byte) != 0)
      return ACCEL_RET_GENERIC_ERR;

    ACCEL_DBG("ODR configured to power down. Configure it to %x\r\n", g_accel_odr);
    if (lis2dh12_data_rate_set(&lis2dh_ctx, g_accel_odr) != 0) return ACCEL_RET_GENERIC_ERR;
  } else {
    g_accel_odr = c_odr;
  }
  ACCEL_DBG("g_accel_odr: %x\r\n", g_accel_odr);
  return ACCEL_RET_SUCCESS;
}
#endif

static inline void wait_for_accel_isr(void) {
  uint8_t indication;
  accel_ev_id_e ev_id;
  accel_ev_cfg_t *ev_cfg;
  uint8_t int_src = 0;
  uint8_t int_src_reg = 0;
  int32_t res = 0;
#ifdef ACCEL_INT_LEVEL_TRIGGER
  eMcuGpioIds gpio_id;
  int gpio_value;
#endif

  /* Wait until something arrives in the queue - this task will block indefinitely */
  while (xQueueReceive(accel_ev_queue, &indication, portMAX_DELAY) != pdPASS)
    ;
  ev_id = (accel_ev_id_e)indication;

  if (xSemaphoreTakeRecursive(ev_lock, portMAX_DELAY) == pdTRUE) {
    if ((ev_id < ACCEL_MAX_EVENT) && (accel_ev_tbl[ev_id].ev_status == EV_STARTED)) {
      ev_cfg = &accel_ev_tbl[ev_id].ev_cfg;

#ifdef ACCEL_INT_LEVEL_TRIGGER
      gpio_id = accel_ev_tbl[ev_id].ev_cfg.int_pin_set;
#endif

      if (ev_id == ACCEL_EVENT_0)
        int_src_reg = LIS2DH12_INT1_SRC;
      else
        int_src_reg = LIS2DH12_INT2_SRC;

      do {
        res = lis2dh12_read_reg(&lis2dh_ctx, int_src_reg, &int_src, 1);
      } while (res != 0);

      if ((ev_cfg->ev_h) && (int_src & 0x40)) {
        ACCEL_DBG("Got accel event and start to call user callback (int_src: %x)\r\n", int_src);
        ev_cfg->ev_h(int_src, ev_cfg->user_param);
        ACCEL_DBG("Exit from accel user callback.\r\n");
#ifdef ACCEL_INT_LEVEL_TRIGGER
        do {
          res = lis2dh12_read_reg(&lis2dh_ctx, int_src_reg, &int_src, 1);
        } while ((res != 0) || (int_src & 0x40));
#endif
      } else {
#ifdef ACCEL_INT_LEVEL_TRIGGER
        gpio_get_value(gpio_id, &gpio_value);
        if (!(int_src & 0x40) && gpio_value)
          ACCEL_ERR(
              "We got accel event not generated by accelerometer.(int_src: %x, gpio_value: %d)\r\n",
              int_src, gpio_value);
#else
        if (!(int_src & 0x40))
          ACCEL_ERR("We got accel event not generated by accelerometer.(int_src: %x)\r\n", int_src);
#endif
      }
#ifdef ACCEL_INT_LEVEL_TRIGGER
      /*Make sure the ev_status did not changed in user callback*/
      if (accel_ev_tbl[ev_id].ev_status == EV_STARTED) {
        /*Restore the the interrupt level to high and wait for next event*/
        gpio_disable_interrupt(gpio_id);
        gpio_config_interrupt(gpio_id, GPIO_IRQ_HIGH_LEVEL, GPIO_PULL_DONT_CHANGE);
        gpio_enable_interrupt(gpio_id);
      }
#endif
    }
    xSemaphoreGiveRecursive(ev_lock);
  } else {
    ACCEL_ERR("[wait_for_accel_isr]Failed to obtain ev lock\r\n");
  }
}

static void accelHdlTask(void *pvParameters) {
#ifdef ACCEL_INIT_IN_ACCEL_TASK
  uint8_t accel_init_retry = ACCEL_INIT_RETRY;
  accel_ret_e accel_init_ret;
#endif
  if (xSemaphoreTakeRecursive(ev_lock, ev_lock_delay) == pdTRUE) {
#ifdef ACCEL_INIT_IN_ACCEL_TASK
#if (configUSE_ALT1250_SLEEP == 1)
    pwr_dev_boot_type_e boot_type;
    if (smngr_register_dev_sync(ACCEL_SMNGR_DEVNAME, &accel_smngr_sync_id) != 0)
      ACCEL_ASSERT("smngr register dev sync fail\n");

    boot_type = pwr_stat_get_dev_boot_type();

    do {
      if (boot_type == PWR_DEV_COLD_BOOT)
        accel_init_ret = lis2dh_global_init();
      else
        accel_init_ret = lis2dh_light_init();

      if (accel_init_ret != ACCEL_RET_SUCCESS && accel_init_retry > 0) {
        accel_init_retry--;
        ACCEL_ERR("Failed to do accel init. Retry count: %d\r\n", accel_init_retry);
      }
    } while (accel_init_ret != ACCEL_RET_SUCCESS && accel_init_retry > 0);

    if (accel_init_ret != ACCEL_RET_SUCCESS && accel_init_retry == 0) {
      ACCEL_ASSERT("Assert! Failed to init LIS2DH%s:%d\r\n", __FILE__, __LINE__);
    } else if (accel_init_ret == ACCEL_RET_SUCCESS && accel_init_retry != ACCEL_INIT_RETRY) {
      ACCEL_ERR("Accel init retry success\r\n");
    }
#else
    do {
      accel_init_ret = lis2dh_global_init();
      if (accel_init_ret != ACCEL_RET_SUCCESS && accel_init_retry > 0) {
        accel_init_retry--;
        ACCEL_ERR("Failed to do accel init. Retry count: %d\r\n", accel_init_retry);
      }
    } while (accel_init_ret != ACCEL_RET_SUCCESS && accel_init_retry > 0);

    if (accel_init_ret != ACCEL_RET_SUCCESS && accel_init_retry == 0) {
      ACCEL_ASSERT("Assert! Failed to init LIS2DH%s:%d\r\n", __FILE__, __LINE__);
    } else if (accel_init_ret == ACCEL_RET_SUCCESS && accel_init_retry != ACCEL_INIT_RETRY) {
      ACCEL_ERR("Accel init retry success\r\n");
    }
#endif /*configUSE_ALT1250_SLEEP == 1*/

    g_accel_init_done = 1;
#endif /*ACCEL_INIT_IN_ACCEL_TASK*/
    if (accel_ev_tbl[ACCEL_EVENT_0].ev_status == EV_PENDING_START) {
      accel_config_int1(1);
      config_gpio_interrupt(ACCEL_EVENT_0);
      accel_ev_tbl[ACCEL_EVENT_0].ev_status = EV_STARTED;
    } else if (accel_ev_tbl[ACCEL_EVENT_0].ev_status == EV_PENDING_MASKED_START) {
      accel_config_int1(0);
      accel_ev_tbl[ACCEL_EVENT_0].ev_status = EV_MASKED;
    } else if (accel_ev_tbl[ACCEL_EVENT_0].ev_status == EV_PENDING_STOP) {
      accel_clear_int1();
      accel_ev_tbl[ACCEL_EVENT_0].ev_status = EV_STOPPED;
    }
    if (accel_ev_tbl[ACCEL_EVENT_1].ev_status == EV_PENDING_START) {
      accel_config_int2(1);
      config_gpio_interrupt(ACCEL_EVENT_1);
      accel_ev_tbl[ACCEL_EVENT_1].ev_status = EV_STARTED;
    } else if (accel_ev_tbl[ACCEL_EVENT_1].ev_status == EV_PENDING_MASKED_START) {
      accel_config_int2(0);
      accel_ev_tbl[ACCEL_EVENT_1].ev_status = EV_MASKED;
    } else if (accel_ev_tbl[ACCEL_EVENT_1].ev_status == EV_PENDING_STOP) {
      accel_clear_int2();
      accel_ev_tbl[ACCEL_EVENT_1].ev_status = EV_STOPPED;
    }
    xSemaphoreGiveRecursive(ev_lock);
  } else {
    ACCEL_ASSERT("Failed to obtain ev lock\r\n");
  }

  for (;;) {
    wait_for_accel_isr();
    /*Makes sure other task with same priority has the chance to change the EV state
     if interrupt strom comes from accel*/
    taskYIELD();
  }
}

accel_ret_e accel_stop(accel_ev_id_e event_id) {
  accel_ret_e ret;

  if (event_id >= ACCEL_MAX_EVENT) return ACCEL_RET_PARAM_ERR;
  /* We allow accel_stop is called before ACCEL_TASK complete the lis2dh configraton
   * But we do not allow accel_stop is called before accel_init is called*/
  if (!g_accel_basic_init_done) return ACCEL_RET_NOT_INIT;

  if (xSemaphoreTakeRecursive(ev_lock, ev_lock_delay) == pdTRUE) {
    if (!g_accel_init_done) {
      /*ACCEL_TASK has not complete with the initialization, set EV_PENDING_STOP and return*/
      accel_ev_tbl[event_id].ev_status = EV_PENDING_STOP;
      xSemaphoreGiveRecursive(ev_lock);
      return ACCEL_RET_SUCCESS;
    }
    if (accel_ev_tbl[event_id].ev_status == EV_STOPPED) {
      if (event_id == ACCEL_EVENT_0)
        accel_clear_int1();
      else
        accel_clear_int2();
      xSemaphoreGiveRecursive(ev_lock);
      return ACCEL_RET_SUCCESS;
    }
    if (accel_ev_tbl[event_id].ev_status == EV_STARTED) {
      if ((ret = clear_gpio_interrupt(event_id)) != ACCEL_RET_SUCCESS) {
        xSemaphoreGiveRecursive(ev_lock);
        return ret;
      }
    }

    if (event_id == ACCEL_EVENT_0)
      accel_clear_int1();
    else
      accel_clear_int2();

    if ((ret = accel_intx_clear_sync(event_id)) != ACCEL_RET_SUCCESS) {
      ACCEL_ERR("Failed to clear accel interrupt in accel_stop\r\n");
#if 0 /*Accel int is connectd to other input source(RTS). Log this instead of return error*/
      xSemaphoreGiveRecursive(ev_lock);
      return ret;
#endif
    }

    accel_ev_tbl[event_id].ev_status = EV_STOPPED;

    xSemaphoreGiveRecursive(ev_lock);
    return ACCEL_RET_SUCCESS;
  } else {
    ACCEL_ERR("[accel_stop]Failed to obtain ev lock\r\n");
    return ACCEL_RET_TIMEOUT;
  }
}

accel_ret_e accel_start_with_masked(accel_ev_id_e event_id, accel_ev_cfg_t *event_config) {
  accel_ret_e ret;

  if (event_id >= ACCEL_MAX_EVENT) return ACCEL_RET_PARAM_ERR;
  /* We allow accel_start is called before ACCEL_TASK complete the lis2dh configraton
   * But we do not allow accel_start is called before accel_init is called*/
  if (!g_accel_basic_init_done) return ACCEL_RET_NOT_INIT;

  if (xSemaphoreTakeRecursive(ev_lock, ev_lock_delay) == pdTRUE) {
    if (!g_accel_init_done) {
      memcpy(&accel_ev_tbl[event_id].ev_cfg, event_config, sizeof(accel_ev_cfg_t));
      accel_ev_tbl[event_id].ev_status = EV_PENDING_MASKED_START;
      xSemaphoreGiveRecursive(ev_lock);
      return ACCEL_RET_SUCCESS;
    }

    if (g_accel_odr == LIS2DH12_POWER_DOWN) {
      ACCEL_ERR("accel start with wrong power state\r\n");
      xSemaphoreGiveRecursive(ev_lock);
      return ACCEL_RET_GENERIC_ERR;
    }

    if (accel_ev_tbl[event_id].ev_status == EV_STARTED) {
      if ((ret = accel_stop(event_id)) != ACCEL_RET_SUCCESS) {
        xSemaphoreGiveRecursive(ev_lock);
        return ret;
      }
    }

    memcpy(&accel_ev_tbl[event_id].ev_cfg, event_config, sizeof(accel_ev_cfg_t));

    if (event_id == ACCEL_EVENT_0) {
      if ((ret = accel_config_int1(0)) != ACCEL_RET_SUCCESS) {
        accel_clear_int1();
        xSemaphoreGiveRecursive(ev_lock);
        return ret;
      }
    } else if (event_id == ACCEL_EVENT_1) {
      if ((ret = accel_config_int2(0)) != ACCEL_RET_SUCCESS) {
        accel_clear_int2();
        xSemaphoreGiveRecursive(ev_lock);
        return ret;
      }
    }

    accel_ev_tbl[event_id].ev_status = EV_MASKED;

    xSemaphoreGiveRecursive(ev_lock);
    return ACCEL_RET_SUCCESS;

  } else {
    ACCEL_ERR("[accel_start]Failed to obtain ev lock\r\n");
    return ACCEL_RET_TIMEOUT;
  }
}

accel_ret_e accel_start(accel_ev_id_e event_id, accel_ev_cfg_t *event_config) {
  accel_ret_e ret;

  if (event_id >= ACCEL_MAX_EVENT) return ACCEL_RET_PARAM_ERR;
  /* We allow accel_start is called before ACCEL_TASK complete the lis2dh configraton
   * But we do not allow accel_start is called before accel_init is called*/
  if (!g_accel_basic_init_done) return ACCEL_RET_NOT_INIT;

  if (xSemaphoreTakeRecursive(ev_lock, ev_lock_delay) == pdTRUE) {
    if (!g_accel_init_done) {
      memcpy(&accel_ev_tbl[event_id].ev_cfg, event_config, sizeof(accel_ev_cfg_t));
      accel_ev_tbl[event_id].ev_status = EV_PENDING_START;
      xSemaphoreGiveRecursive(ev_lock);
      return ACCEL_RET_SUCCESS;
    }

    if (g_accel_odr == LIS2DH12_POWER_DOWN) {
      ACCEL_ERR("accel start with wrong power state\r\n");
      xSemaphoreGiveRecursive(ev_lock);
      return ACCEL_RET_GENERIC_ERR;
    }

    if (accel_ev_tbl[event_id].ev_status == EV_STARTED) {
      if ((ret = accel_stop(event_id)) != ACCEL_RET_SUCCESS) {
        xSemaphoreGiveRecursive(ev_lock);
        return ret;
      }
    }

    memcpy(&accel_ev_tbl[event_id].ev_cfg, event_config, sizeof(accel_ev_cfg_t));

    if (event_id == ACCEL_EVENT_0) {
      if ((ret = accel_config_int1(1)) != ACCEL_RET_SUCCESS) {
        accel_clear_int1();
        xSemaphoreGiveRecursive(ev_lock);
        return ret;
      }
    } else if (event_id == ACCEL_EVENT_1) {
      if ((ret = accel_config_int2(1)) != ACCEL_RET_SUCCESS) {
        accel_clear_int2();
        xSemaphoreGiveRecursive(ev_lock);
        return ret;
      }
    }

    if ((ret = config_gpio_interrupt(event_id)) != ACCEL_RET_SUCCESS) {
      if (event_id == ACCEL_EVENT_0)
        accel_clear_int1();
      else if (event_id == ACCEL_EVENT_1)
        accel_clear_int2();
      xSemaphoreGiveRecursive(ev_lock);
      return ret;
    }

    accel_ev_tbl[event_id].ev_status = EV_STARTED;

    xSemaphoreGiveRecursive(ev_lock);
    return ACCEL_RET_SUCCESS;

  } else {
    ACCEL_ERR("[accel_start]Failed to obtain ev lock\r\n");
    return ACCEL_RET_TIMEOUT;
  }
}

accel_ret_e accel_init(void) {
#ifndef ACCEL_INIT_IN_ACCEL_TASK
  uint8_t accel_init_retry = ACCEL_INIT_RETRY;
  accel_ret_e accel_init_ret;
  pwr_dev_boot_type_e boot_type;
#endif

  if (g_accel_basic_init_done) return ACCEL_RET_SUCCESS;

  lis2dh_ctx.write_reg = alt_i2c_write;
  lis2dh_ctx.read_reg = alt_i2c_read;

  ev_lock = xSemaphoreCreateRecursiveMutex();
  if (ev_lock == NULL) {
    ACCEL_ERR("Failed to create ev lock\r\n");
    goto init_err;
  }

  platform_rw_lock = xSemaphoreCreateMutex();
  if (platform_rw_lock == NULL) {
    ACCEL_ERR("Failed to create read/write lock\r\n");
    goto init_err;
  }

  accel_ev_queue = xQueueCreate(ACCEL_EVENT_QUEUE_SIZE, sizeof(char));
  if (accel_ev_queue == NULL) {
    ACCEL_ERR("Failed to create ev queue\r\n");
    goto init_err;
  }

  memset(accel_ev_tbl, 0x0, sizeof(accel_ev_table_t) * ACCEL_MAX_EVENT);

  g_accel_basic_init_done = 1;
/*Do lis2dh initialization directly here if allowed to*/
#ifndef ACCEL_INIT_IN_ACCEL_TASK
  if (xSemaphoreTakeRecursive(ev_lock, ev_lock_delay) == pdTRUE) {
    if (smngr_register_dev_sync(ACCEL_SMNGR_DEVNAME, &accel_smngr_sync_id) != 0)
      ACCEL_ASSERT("smngr register dev sync fail\n");

    boot_type = pwr_stat_get_dev_boot_type();

    do {
      if (boot_type == PWR_DEV_COLD_BOOT)
        accel_init_ret = lis2dh_global_init();
      else
        accel_init_ret = lis2dh_light_init();

      if (accel_init_ret != ACCEL_RET_SUCCESS && accel_init_retry > 0) {
        accel_init_retry--;
        ACCEL_ERR("Failed to do accel init. Retry count: %d\r\n", accel_init_retry);
      }
    } while (accel_init_ret != ACCEL_RET_SUCCESS && accel_init_retry > 0);

    if (accel_init_ret != ACCEL_RET_SUCCESS && accel_init_retry == 0) {
      ACCEL_ASSERT("Assert! Failed to init LIS2DH%s:%d\r\n", __FILE__, __LINE__);
    } else if (accel_init_ret == ACCEL_RET_SUCCESS && accel_init_retry != ACCEL_INIT_RETRY) {
      ACCEL_ERR("Accel init retry success\r\n");
    }
    g_accel_init_done = 1;
    xSemaphoreGiveRecursive(ev_lock);
  } else {
    ACCEL_ASSERT("Failed to obtain ev lock\r\n");
  }
#endif

  if (xTaskCreate(accelHdlTask, "ACCEL TASK", ACCEL_HANDLE_TASK_STACK_SIZE, NULL,
                  ACCEL_HANDLE_TASK_PRIORITY, NULL) != pdPASS) {
    ACCEL_ERR("Failed to create ACCEL TASK\r\n");
    goto init_err;
  }

  return ACCEL_RET_SUCCESS;

init_err:
  g_accel_basic_init_done = 0;
  if (ev_lock) {
    vSemaphoreDelete(ev_lock);
    ev_lock = NULL;
  }
  if (platform_rw_lock) {
    vSemaphoreDelete(platform_rw_lock);
    platform_rw_lock = NULL;
  }
  if (accel_ev_queue) {
    vQueueDelete(accel_ev_queue);
    accel_ev_queue = NULL;
  }
  return ACCEL_RET_GENERIC_ERR;
}

int accel_has_data(void) {
  lis2dh12_reg_t reg;

  if (!g_accel_init_done) return 0;

  lis2dh12_xl_data_ready_get(&lis2dh_ctx, &reg.byte);
  return (reg.byte != 0);
}

int accel_temp_data_ready(void) {
  lis2dh12_reg_t reg;

  if (!g_accel_init_done) return 0;

  lis2dh12_temp_data_ready_get(&lis2dh_ctx, &reg.byte);
  return (reg.byte != 0);
}

accel_ret_e accel_irqx_mask(uint8_t irq_mask) {
  lis2dh12_ctrl_reg3_t reg3;
  lis2dh12_ctrl_reg6_t reg6;

  /* We allow this is called before ACCEL_TASK complete the lis2dh configraton
   * But we do not allow accel_start is called before accel_init is called*/
  if (!g_accel_basic_init_done) return ACCEL_RET_NOT_INIT;

  if (xSemaphoreTakeRecursive(ev_lock, ev_lock_delay) == pdTRUE) {
    if (irq_mask & 0x1) {
      lis2dh12_pin_int1_config_get(&lis2dh_ctx, &reg3);
      reg3.i1_ia1 = 0;
      lis2dh12_pin_int1_config_set(&lis2dh_ctx, &reg3);
      if (accel_ev_tbl[ACCEL_EVENT_0].ev_status == EV_STARTED) {
        clear_gpio_interrupt(ACCEL_EVENT_0);
        accel_ev_tbl[ACCEL_EVENT_0].ev_status = EV_MASKED;
      } else if (accel_ev_tbl[ACCEL_EVENT_0].ev_status == EV_PENDING_START) {
        accel_ev_tbl[ACCEL_EVENT_0].ev_status = EV_PENDING_MASKED_START;
      }
    }
    if (irq_mask & 0x2) {
      lis2dh12_pin_int2_config_get(&lis2dh_ctx, &reg6);
      reg6.i2_ia2 = 0;
      lis2dh12_pin_int2_config_set(&lis2dh_ctx, &reg6);
      if ((accel_ev_tbl[ACCEL_EVENT_1].ev_status == EV_STARTED)) {
        clear_gpio_interrupt(ACCEL_EVENT_1);
        accel_ev_tbl[ACCEL_EVENT_1].ev_status = EV_MASKED;
      } else if (accel_ev_tbl[ACCEL_EVENT_1].ev_status == EV_PENDING_START) {
        accel_ev_tbl[ACCEL_EVENT_1].ev_status = EV_PENDING_MASKED_START;
      }
    }
    xSemaphoreGiveRecursive(ev_lock);
  } else {
    ACCEL_ERR("[accel_irqx_mask]Failed to obtain ev lock\r\n");
    return ACCEL_RET_TIMEOUT;
  }
  return ACCEL_RET_SUCCESS;
}

accel_ret_e accel_irqx_unmask(uint8_t irq_mask) {
  lis2dh12_ctrl_reg3_t reg3;
  lis2dh12_ctrl_reg6_t reg6;

  /* We allow this is called before ACCEL_TASK complete the lis2dh configraton
   * But we do not allow accel_start is called before accel_init is called*/
  if (!g_accel_basic_init_done) return ACCEL_RET_NOT_INIT;

  if (xSemaphoreTakeRecursive(ev_lock, ev_lock_delay) == pdTRUE) {
    if (irq_mask & 0x1) {
      lis2dh12_pin_int1_config_get(&lis2dh_ctx, &reg3);
      reg3.i1_ia1 = 1;
      lis2dh12_pin_int1_config_set(&lis2dh_ctx, &reg3);
      if (accel_ev_tbl[ACCEL_EVENT_0].ev_status == EV_MASKED) {
        config_gpio_interrupt(ACCEL_EVENT_0);
        accel_ev_tbl[ACCEL_EVENT_0].ev_status = EV_STARTED;
      } else if (accel_ev_tbl[ACCEL_EVENT_0].ev_status == EV_PENDING_MASKED_START) {
        accel_ev_tbl[ACCEL_EVENT_0].ev_status = EV_PENDING_START;
      }
    }
    if (irq_mask & 0x2) {
      lis2dh12_pin_int2_config_get(&lis2dh_ctx, &reg6);
      reg6.i2_ia2 = 1;
      lis2dh12_pin_int2_config_set(&lis2dh_ctx, &reg6);
      if (accel_ev_tbl[ACCEL_EVENT_1].ev_status == EV_MASKED) {
        config_gpio_interrupt(ACCEL_EVENT_1);
        accel_ev_tbl[ACCEL_EVENT_1].ev_status = EV_STARTED;
      } else if (accel_ev_tbl[ACCEL_EVENT_1].ev_status == EV_PENDING_MASKED_START) {
        accel_ev_tbl[ACCEL_EVENT_1].ev_status = EV_PENDING_START;
      }
    }

    xSemaphoreGiveRecursive(ev_lock);
  } else {
    ACCEL_ERR("[accel_irqx_unmask]Failed to obtain ev lock\r\n");
    return ACCEL_RET_TIMEOUT;
  }
  return ACCEL_RET_SUCCESS;
}

accel_ret_e accel_get_temp_celsius(int16_t *temperature_degC) {
  uint8_t shift;
  if (!g_accel_init_done) {
    return ACCEL_RET_NOT_INIT;
  }
  axis1bit16_t data_raw_temperature;
  memset(data_raw_temperature.u8bit, 0x00, sizeof(int16_t));
  shift = 8;
  if (lis2dh12_temperature_raw_get(&lis2dh_ctx, data_raw_temperature.u8bit) != 0)
    return ACCEL_RET_GENERIC_ERR;
  *temperature_degC = (data_raw_temperature.i16bit >> shift) + 25;
  return ACCEL_RET_SUCCESS;
}

accel_ret_e accel_get_xyz_mg(xyz_mg_t *accel_mg) {
  uint8_t shift;
  uint8_t raw_unit;
  axis3bit16_t data_raw_acceleration;

  if (!g_accel_init_done) return ACCEL_RET_NOT_INIT;

  memset(data_raw_acceleration.u8bit, 0x00, 3 * sizeof(int16_t));

  if (lis2dh12_acceleration_raw_get(&lis2dh_ctx, data_raw_acceleration.u8bit) != 0)
    return ACCEL_RET_GENERIC_ERR;

  switch (g_accel_op_md) {
    case LIS2DH12_HR_12bit:
      shift = 4;
      raw_unit = 1;
      break;
    case LIS2DH12_NM_10bit:
      shift = 6;
      raw_unit = 4;
      break;
    case LIS2DH12_LP_8bit:
      shift = 8;
      raw_unit = 16;
      break;
    default:
      shift = 8;
      raw_unit = 16;
      break;
  }
  switch (g_accel_fs) {
    case LIS2DH12_2g:
      raw_unit *= 1;
      break;
    case LIS2DH12_4g:
      raw_unit *= 2;
      break;
    case LIS2DH12_8g:
      raw_unit *= 4;
      break;
    case LIS2DH12_16g:
      raw_unit *= 12;
      break;
    default:
      raw_unit *= 1;
      break;
  }
  accel_mg->x = (data_raw_acceleration.i16bit[0] >> shift) * raw_unit;
  accel_mg->y = (data_raw_acceleration.i16bit[1] >> shift) * raw_unit;
  accel_mg->z = (data_raw_acceleration.i16bit[2] >> shift) * raw_unit;

  return ACCEL_RET_SUCCESS;
}

accel_ret_e accel_config_power(uint8_t pwr_state) {
  if (pwr_state)
    return accel_config_odr(ACCEL_LIS2DH_ODR);
  else {
    /*Clear all event before power down*/
    accel_stop(ACCEL_EVENT_0);
    accel_stop(ACCEL_EVENT_1);
    return accel_config_odr(LIS2DH12_POWER_DOWN);
  }
}

accel_ret_e accel_get_odr(lis2dh12_odr_t *odr) {
  if (!g_accel_init_done) return ACCEL_RET_NOT_INIT;

  if (lis2dh12_data_rate_get(&lis2dh_ctx, odr) != 0) return ACCEL_RET_GENERIC_ERR;

  return ACCEL_RET_SUCCESS;
}

accel_ret_e accel_config_odr(lis2dh12_odr_t odr) {
  accel_ret_e ret = ACCEL_RET_SUCCESS;
  uint8_t odr_can_change = 1;
  uint8_t orig_dur_raw = 0;
  uint32_t new_dur_raw = 0;

  if (!g_accel_basic_init_done) return ACCEL_RET_NOT_INIT;

  if (xSemaphoreTakeRecursive(ev_lock, ev_lock_delay) == pdTRUE) {
    if (!g_accel_init_done) {
      /*Set g_accel_odr directly and let ACCEL_TASK to configure the ODR value.*/
      g_accel_odr = odr;
      xSemaphoreGiveRecursive(ev_lock);
      return ACCEL_RET_SUCCESS;
    }

    /*Check if any accel event was started and recalculate int duration if needed.*/
    lis2dh12_int1_gen_duration_get(&lis2dh_ctx, &orig_dur_raw);
    if (orig_dur_raw != 0) {
      new_dur_raw = calculate_new_duration(orig_dur_raw, odr);
      if (new_dur_raw < 0x80) {
        lis2dh12_int1_gen_duration_set(&lis2dh_ctx, (uint8_t)new_dur_raw);
      } else {
        ACCEL_ERR("Invalid duration value\n");
        odr_can_change = 0;
      }
    }
    lis2dh12_int2_gen_duration_get(&lis2dh_ctx, &orig_dur_raw);
    if (orig_dur_raw != 0) {
      new_dur_raw = calculate_new_duration(orig_dur_raw, odr);
      if (new_dur_raw < 0x80) {
        lis2dh12_int2_gen_duration_set(&lis2dh_ctx, (uint8_t)new_dur_raw);
      } else {
        ACCEL_ERR("Invalid duration value\n");
        odr_can_change = 0;
      }
    }
    if (odr_can_change) {
      if (lis2dh12_data_rate_set(&lis2dh_ctx, odr) != 0) {
        ACCEL_ERR("Failed to set accel odr\r\n");
        ret = ACCEL_RET_GENERIC_ERR;
      } else {
        g_accel_odr = odr;
        ACCEL_DBG("g_accel_odr: %x\r\n", g_accel_odr);
      }
    } else {
      ACCEL_ERR("Failed to configure accel odr because of invalid int duration\r\n");
      ret = ACCEL_RET_GENERIC_ERR;
    }
    xSemaphoreGiveRecursive(ev_lock);
  } else {
    ACCEL_ERR("Failed to obtain ev lock\r\n");
    return ACCEL_RET_TIMEOUT;
  }

  return ret;
}

accel_ret_e accel_get_hp_filter_on_output(uint8_t *enable) {
  if (!g_accel_init_done) return ACCEL_RET_NOT_INIT;

  if (lis2dh12_high_pass_on_outputs_get(&lis2dh_ctx, enable) != 0) return ACCEL_RET_GENERIC_ERR;

  return ACCEL_RET_SUCCESS;
}

accel_ret_e accel_config_hp_filter_on_output(uint8_t enable) {
  if (!g_accel_init_done) return ACCEL_RET_NOT_INIT;

  if (lis2dh12_high_pass_on_outputs_set(&lis2dh_ctx, enable) != 0) return ACCEL_RET_GENERIC_ERR;

  return ACCEL_RET_SUCCESS;
}

accel_ret_e accel_reset_fifo_buffer() {
  lis2dh12_fm_t fm;

  if (lis2dh12_fifo_mode_get(&lis2dh_ctx, &fm) != 0) {
    ACCEL_ERR("Failed to backup FIFO mode\r\n");
    return ACCEL_RET_GENERIC_ERR;
  }

  if (lis2dh12_fifo_mode_set(&lis2dh_ctx, LIS2DH12_BYPASS_MODE) != 0) {
    ACCEL_ERR("Failed to reset fifo mode\r\n");
    return ACCEL_RET_GENERIC_ERR;
  }

  if (lis2dh12_fifo_mode_set(&lis2dh_ctx, fm) != 0) {
    ACCEL_ERR("Failed to restore fifo mode\r\n");
    return ACCEL_RET_GENERIC_ERR;
  }

  return ACCEL_RET_SUCCESS;
}

uint8_t accel_get_fifo_data_level() {
  uint8_t fifo_cnt = 0;

  if (!g_accel_init_done) return 0;

  if ((!g_accel_init_done) || (lis2dh12_fifo_data_level_get(&lis2dh_ctx, &fifo_cnt) != 0)) return 0;

  return fifo_cnt;
}

uint8_t accel_get_fifo_xyz_mg(xyz_mg_t *accel_mg_buf, uint8_t buf_size) {
  uint8_t fifo_cnt = 0;
  uint8_t fifo_empty = 0;
  uint8_t shift;
  uint8_t raw_unit;
  axis3bit16_t data_raw_acceleration;

  if (!g_accel_init_done) return 0;

  do {
    if ((lis2dh12_fifo_empty_flag_get(&lis2dh_ctx, &fifo_empty) != 0) || (fifo_empty)) break;

    memset(data_raw_acceleration.u8bit, 0x00, 3 * sizeof(int16_t));

    if (lis2dh12_acceleration_raw_get(&lis2dh_ctx, data_raw_acceleration.u8bit) != 0) break;

    switch (g_accel_op_md) {
      case LIS2DH12_HR_12bit:
        shift = 4;
        raw_unit = 1;
        break;
      case LIS2DH12_NM_10bit:
        shift = 6;
        raw_unit = 4;
        break;
      case LIS2DH12_LP_8bit:
        shift = 8;
        raw_unit = 16;
        break;
      default:
        shift = 8;
        raw_unit = 16;
        break;
    }
    switch (g_accel_fs) {
      case LIS2DH12_2g:
        raw_unit *= 1;
        break;
      case LIS2DH12_4g:
        raw_unit *= 2;
        break;
      case LIS2DH12_8g:
        raw_unit *= 4;
        break;
      case LIS2DH12_16g:
        raw_unit *= 12;
        break;
      default:
        raw_unit *= 1;
        break;
    }
    accel_mg_buf[fifo_cnt].x = (data_raw_acceleration.i16bit[0] >> shift) * raw_unit;
    accel_mg_buf[fifo_cnt].y = (data_raw_acceleration.i16bit[1] >> shift) * raw_unit;
    accel_mg_buf[fifo_cnt].z = (data_raw_acceleration.i16bit[2] >> shift) * raw_unit;
    fifo_cnt++;
    buf_size--;
  } while (buf_size);

  return fifo_cnt;
}

accel_ret_e accel_get_fifo_enabled(uint8_t *enable) {
  if (!g_accel_init_done) return ACCEL_RET_NOT_INIT;

  if (lis2dh12_fifo_get(&lis2dh_ctx, enable) != 0) {
    ACCEL_ERR("Failed to get FIFO EN\r\n");
    return ACCEL_RET_GENERIC_ERR;
  }
  return ACCEL_RET_SUCCESS;
}

accel_ret_e accel_get_fifo_mode(accel_fifo_mode_e *mode) {
  lis2dh12_fm_t fm;
  lis2dh12_tr_t tr;
  if (!g_accel_init_done) return ACCEL_RET_NOT_INIT;

  if (lis2dh12_fifo_mode_get(&lis2dh_ctx, &fm) != 0) {
    ACCEL_ERR("Failed to get FIFO mode\r\n");
    return ACCEL_RET_GENERIC_ERR;
  }
  switch (fm) {
    case LIS2DH12_BYPASS_MODE:
      *mode = ACCEL_FIFO_MODE_BYPASS;
      break;
    case LIS2DH12_FIFO_MODE:
      *mode = ACCEL_FIFO_MODE_FIFO;
      break;
    case LIS2DH12_DYNAMIC_STREAM_MODE:
      *mode = ACCEL_FIFO_MODE_STREAM;
      break;
    case LIS2DH12_STREAM_TO_FIFO_MODE:
      if (lis2dh12_fifo_trigger_event_get(&lis2dh_ctx, &tr) != 0) {
        ACCEL_ERR("Failed to get TR\r\n");
        return ACCEL_RET_GENERIC_ERR;
      }
      if (LIS2DH12_INT2_GEN)
        *mode = ACCEL_FIFO_MODE_STREAM_FIFO_TR1;
      else
        *mode = ACCEL_FIFO_MODE_STREAM_FIFO_TR0;
      break;
    default:
      *mode = LIS2DH12_BYPASS_MODE;
      break;
  }
  return ACCEL_RET_SUCCESS;
}

accel_ret_e accel_config_fifo(uint8_t enable, accel_fifo_mode_e mode) {
  if (!g_accel_init_done) return ACCEL_RET_NOT_INIT;

  if (enable) {
    switch (mode) {
      case ACCEL_FIFO_MODE_BYPASS:
        if (lis2dh12_fifo_mode_set(&lis2dh_ctx, LIS2DH12_BYPASS_MODE) != 0) {
          ACCEL_ERR("Failed to set fifo mode\r\n");
          return ACCEL_RET_GENERIC_ERR;
        }
        break;
      case ACCEL_FIFO_MODE_FIFO:
        if (lis2dh12_fifo_mode_set(&lis2dh_ctx, LIS2DH12_FIFO_MODE) != 0) {
          ACCEL_ERR("Failed to set fifo mode\r\n");
          return ACCEL_RET_GENERIC_ERR;
        }
        break;
      case ACCEL_FIFO_MODE_STREAM:
        if (lis2dh12_fifo_mode_set(&lis2dh_ctx, LIS2DH12_DYNAMIC_STREAM_MODE) != 0) {
          ACCEL_ERR("Failed to set fifo mode\r\n");
          return ACCEL_RET_GENERIC_ERR;
        }
        break;
      case ACCEL_FIFO_MODE_STREAM_FIFO_TR0:
        if (lis2dh12_fifo_mode_set(&lis2dh_ctx, LIS2DH12_STREAM_TO_FIFO_MODE) != 0) {
          ACCEL_ERR("Failed to set fifo mode\r\n");
          return ACCEL_RET_GENERIC_ERR;
        }
        if (lis2dh12_fifo_trigger_event_set(&lis2dh_ctx, LIS2DH12_INT1_GEN) != 0) {
          ACCEL_ERR("Failed to set fifo tr\r\n");
          return ACCEL_RET_GENERIC_ERR;
        }
        break;
      case ACCEL_FIFO_MODE_STREAM_FIFO_TR1:
        if (lis2dh12_fifo_mode_set(&lis2dh_ctx, LIS2DH12_STREAM_TO_FIFO_MODE) != 0) {
          ACCEL_ERR("Failed to set fifo mode\r\n");
          return ACCEL_RET_GENERIC_ERR;
        }
        if (lis2dh12_fifo_trigger_event_set(&lis2dh_ctx, LIS2DH12_INT2_GEN) != 0) {
          ACCEL_ERR("Failed to set fifo tr\r\n");
          return ACCEL_RET_GENERIC_ERR;
        }
        break;
      default:
        ACCEL_ERR("Unknown FIFO mode\r\n");
        return ACCEL_RET_PARAM_ERR;
        break;
    }
    if (lis2dh12_fifo_set(&lis2dh_ctx, 1) != 0) {
      ACCEL_ERR("Failed to enable fifo\r\n");
      return ACCEL_RET_GENERIC_ERR;
    }
  } else {
    if (lis2dh12_fifo_mode_set(&lis2dh_ctx, LIS2DH12_BYPASS_MODE) != 0) {
      ACCEL_ERR("Failed to restore fifo mode\r\n");
      return ACCEL_RET_GENERIC_ERR;
    }
    if (lis2dh12_fifo_set(&lis2dh_ctx, 0) != 0) {
      ACCEL_ERR("Failed to disable fifo\r\n");
      return ACCEL_RET_GENERIC_ERR;
    }
  }
  return ACCEL_RET_SUCCESS;
}

void accel_dump_config(void) {
  uint8_t reg_value = 0;

  if (!g_accel_init_done) {
    ACCEL_ERR("accel not init\r\n");
    return;
  }

  ACCEL_ERR("---LIS2DH Config---\r\n");
  lis2dh12_read_reg(&lis2dh_ctx, LIS2DH12_CTRL_REG1, &reg_value, 1);
  ACCEL_ERR("CTRL_REG1 : 0x%02X\r\n", reg_value);
  lis2dh12_read_reg(&lis2dh_ctx, LIS2DH12_CTRL_REG2, &reg_value, 1);
  ACCEL_ERR("CTRL_REG2 : 0x%02X\r\n", reg_value);
  lis2dh12_read_reg(&lis2dh_ctx, LIS2DH12_CTRL_REG3, &reg_value, 1);
  ACCEL_ERR("CTRL_REG3 : 0x%02X\r\n", reg_value);
  lis2dh12_read_reg(&lis2dh_ctx, LIS2DH12_CTRL_REG4, &reg_value, 1);
  ACCEL_ERR("CTRL_REG4 : 0x%02X\r\n", reg_value);
  lis2dh12_read_reg(&lis2dh_ctx, LIS2DH12_CTRL_REG5, &reg_value, 1);
  ACCEL_ERR("CTRL_REG5 : 0x%02X\r\n", reg_value);
  lis2dh12_read_reg(&lis2dh_ctx, LIS2DH12_CTRL_REG6, &reg_value, 1);
  ACCEL_ERR("CTRL_REG6 : 0x%02X\r\n", reg_value);
  lis2dh12_read_reg(&lis2dh_ctx, LIS2DH12_STATUS_REG, &reg_value, 1);
  ACCEL_ERR("STATUS_REG : 0x%02X\r\n", reg_value);
  lis2dh12_read_reg(&lis2dh_ctx, LIS2DH12_FIFO_CTRL_REG, &reg_value, 1);
  ACCEL_ERR("FIFO_CTRL_REG : 0x%02X\r\n", reg_value);
  lis2dh12_read_reg(&lis2dh_ctx, LIS2DH12_FIFO_SRC_REG, &reg_value, 1);
  ACCEL_ERR("FIFO_SRC_REG : 0x%02X\r\n", reg_value);
  lis2dh12_read_reg(&lis2dh_ctx, LIS2DH12_INT1_CFG, &reg_value, 1);
  ACCEL_ERR("INT1_CFG : 0x%02X\r\n", reg_value);
  lis2dh12_read_reg(&lis2dh_ctx, LIS2DH12_INT1_DURATION, &reg_value, 1);
  ACCEL_ERR("INT1_DUR : 0x%02X\r\n", reg_value);
  lis2dh12_read_reg(&lis2dh_ctx, LIS2DH12_INT1_SRC, &reg_value, 1);
  ACCEL_ERR("INT1_SRC : 0x%02X\r\n", reg_value);
  lis2dh12_read_reg(&lis2dh_ctx, LIS2DH12_INT1_THS, &reg_value, 1);
  ACCEL_ERR("INT1_THS : 0x%02X\r\n", reg_value);
  lis2dh12_read_reg(&lis2dh_ctx, LIS2DH12_INT2_CFG, &reg_value, 1);
  ACCEL_ERR("INT2_CFG : 0x%02X\r\n", reg_value);
  lis2dh12_read_reg(&lis2dh_ctx, LIS2DH12_INT2_DURATION, &reg_value, 1);
  ACCEL_ERR("INT2_DUR : 0x%02X\r\n", reg_value);
  lis2dh12_read_reg(&lis2dh_ctx, LIS2DH12_INT2_SRC, &reg_value, 1);
  ACCEL_ERR("INT2_SRC : 0x%02X\r\n", reg_value);
  lis2dh12_read_reg(&lis2dh_ctx, LIS2DH12_INT2_THS, &reg_value, 1);
  ACCEL_ERR("INT2_THS : 0x%02X\r\n", reg_value);
  lis2dh12_read_reg(&lis2dh_ctx, LIS2DH12_INT2_CFG, &reg_value, 1);
}
