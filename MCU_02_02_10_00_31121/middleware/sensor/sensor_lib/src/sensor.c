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
#include "sensor.h"
#include "DRV_I2C.h"
#include "DRV_GPIO.h"
#include "alt_osal.h"
#include "sleep_mngr.h"
#include "pwr_mngr.h"

#ifdef SENSOR_DEBUG_MSG
#define SENSOR_DBG(...) printf(__VA_ARGS__)
#else
#define SENSOR_DBG(...)
#endif

#define SENSOR_ERR(...) printf(__VA_ARGS__)

#if (configUSE_ALT_SLEEP == 1)
#define SENSOR_SMNGR_DEVNAME "SENSOR-LIB"
unsigned int sensor_smngr_sync_id;
#endif

#define MAX_IRQ_PER_LINE 2

struct sensor_irq_entry {
  uint8_t en_msk;
  void *user_param[MAX_IRQ_PER_LINE];
  sensor_irq_handler handler[MAX_IRQ_PER_LINE];
};

struct sensor_irq_entry irq_vec_table[MCU_GPIO_ID_MAX];

static alt_osal_queue_handle sensor_ev_queue = NULL;
static alt_osal_mutex_handle irq_lock = NULL;

#define I2C_BUS_COUNT (2)
#define PLATFORM_LOCK_TIMEOUT_MS 5000
#define I2C_TRANSACTION_TIMEOUT_MS 5000
#define IRQ_LOCK_TIMEOUT_MS 5000
#define I2C_TX_BUF_LEN 32
#define I2C_EV_TO_WAIT                                                \
  (I2C_EVENT_TRANSACTION_DONE | I2C_EVENT_TRANSACTION_INCOMPLETE |    \
   I2C_EVENT_TRANSACTION_ADDRNACK | I2C_EVENT_TRANSACTION_BUS_CLEAR | \
   I2C_EVENT_TRANSACTION_DIRCHG)

struct i2c_bus_data {
  uint8_t init_done;
  Drv_I2C_Peripheral *i2c_p;
  alt_osal_mutex_handle rw_lock;
  alt_osal_eventflag_handle i2c_event;
  uint8_t *tx_buf;
  uint32_t tx_buf_len;
#if (configUSE_ALT_SLEEP == 1)
  uint32_t smngr_sync_id;
#endif
};

struct _s_i2c_handle {
  uint32_t dev_id;
  struct i2c_bus_data *i2c_bus;
};

static struct i2c_bus_data i2c_bus_table[I2C_BUS_COUNT];

static void i2c_evtcb(void *user_data, uint32_t event) {
  struct i2c_bus_data *bus_data = (struct i2c_bus_data *)user_data;

  alt_osal_set_eventflag(&bus_data->i2c_event, (alt_osal_event_bits)event);
}

static int32_t sensor_i2c_read(void *handle, uint32_t reg, uint16_t addr_len, uint8_t *read_buf,
                               uint16_t len) {
  struct _s_i2c_handle *i2c_h = (struct _s_i2c_handle *)handle;
  struct i2c_bus_data *bus_data = i2c_h->i2c_bus;
  alt_osal_event_bits i2c_ev_res = 0;
  uint32_t i;
  uint8_t *tx_buf_p = NULL;

  if (alt_osal_lock_mutex(&bus_data->rw_lock, PLATFORM_LOCK_TIMEOUT_MS) == 0) {
#if (configUSE_ALT_SLEEP == 1)
    smngr_dev_busy_set(bus_data->smngr_sync_id);
#endif
    alt_osal_clear_eventflag(&bus_data->i2c_event, (alt_osal_event_bits)I2C_EV_TO_WAIT);

    tx_buf_p = bus_data->tx_buf;
    for (i = 0; i < addr_len; i++) *tx_buf_p++ = (uint8_t)(reg >> (8 * (addr_len - i - 1)));

    if (DRV_I2C_MasterTransmit_Nonblock(bus_data->i2c_p, i2c_h->dev_id, bus_data->tx_buf, addr_len,
                                        true) != DRV_I2C_OK) {
      SENSOR_ERR("Failed to do i2c transmit\r\n");
      goto i2c_err;
    }
    if (alt_osal_wait_eventflag(&bus_data->i2c_event, (alt_osal_event_bits)I2C_EV_TO_WAIT,
                                ALT_OSAL_WMODE_TWF_ORW, true, &i2c_ev_res,
                                I2C_TRANSACTION_TIMEOUT_MS) != 0) {
      SENSOR_ERR("Failed to wait i2c transmit\r\n");
      goto i2c_err;
    }

    if ((uint32_t)i2c_ev_res != I2C_EVENT_TRANSACTION_DONE) {
      SENSOR_ERR("I2C trasaction err: %lx\r\n", i2c_ev_res);
      goto i2c_err;
    }

    alt_osal_clear_eventflag(&bus_data->i2c_event, (alt_osal_event_bits)I2C_EV_TO_WAIT);

    if (DRV_I2C_MasterReceive_Nonblock(bus_data->i2c_p, i2c_h->dev_id, read_buf, len, false) !=
        DRV_I2C_OK) {
      SENSOR_ERR("Failed to do i2c receive\r\n");
      goto i2c_err;
    }
    if (alt_osal_wait_eventflag(&bus_data->i2c_event, (alt_osal_event_bits)I2C_EV_TO_WAIT,
                                ALT_OSAL_WMODE_TWF_ORW, true, &i2c_ev_res,
                                I2C_TRANSACTION_TIMEOUT_MS) != 0) {
      SENSOR_ERR("Failed to wait i2c receive\r\n");
      goto i2c_err;
    }

    if ((uint32_t)i2c_ev_res != I2C_EVENT_TRANSACTION_DONE) {
      SENSOR_ERR("I2C trasaction err: %lx\r\n", i2c_ev_res);
      goto i2c_err;
    }

#if (configUSE_ALT_SLEEP == 1)
    smngr_dev_busy_clr(bus_data->smngr_sync_id);
#endif
    alt_osal_unlock_mutex(&bus_data->rw_lock);
  } else {
    SENSOR_ERR("Failed to obtain platform read write lock\r\n");
    return SENSOR_RET_TIMEOUT;
  }

  return SENSOR_RET_SUCCESS;

i2c_err:
#if (configUSE_ALT_SLEEP == 1)
  smngr_dev_busy_clr(bus_data->smngr_sync_id);
#endif
  alt_osal_unlock_mutex(&bus_data->rw_lock);
  return SENSOR_RET_GENERIC_ERR;
}

static int32_t sensor_i2c_write(void *handle, uint32_t reg, uint16_t addr_len, uint8_t *write_buf,
                                uint16_t len) {
  uint8_t *tx_buf_p = NULL;
  uint32_t i;
  alt_osal_event_bits i2c_ev_res = 0;
  struct _s_i2c_handle *i2c_h = (struct _s_i2c_handle *)handle;
  struct i2c_bus_data *bus_data = i2c_h->i2c_bus;

  if (alt_osal_lock_mutex(&bus_data->rw_lock, PLATFORM_LOCK_TIMEOUT_MS) == 0) {
#if (configUSE_ALT_SLEEP == 1)
    smngr_dev_busy_set(bus_data->smngr_sync_id);
#endif
    if (addr_len + len > bus_data->tx_buf_len) {
      SENSOR_ERR("tx length outof range\r\n");
      goto i2c_err;
    }
    tx_buf_p = bus_data->tx_buf;
    for (i = 0; i < addr_len; i++) *tx_buf_p++ = (uint8_t)(reg >> (8 * (addr_len - i - 1)));
    memcpy(tx_buf_p, write_buf, len);

    alt_osal_clear_eventflag(&bus_data->i2c_event, (alt_osal_event_bits)I2C_EV_TO_WAIT);

    if (DRV_I2C_MasterTransmit_Nonblock(bus_data->i2c_p, i2c_h->dev_id, bus_data->tx_buf,
                                        addr_len + len, false) != DRV_I2C_OK) {
      SENSOR_ERR("Failed to do i2c transmit\r\n");
      goto i2c_err;
    }
    if (alt_osal_wait_eventflag(&bus_data->i2c_event, (alt_osal_event_bits)I2C_EV_TO_WAIT,
                                ALT_OSAL_WMODE_TWF_ORW, true, &i2c_ev_res,
                                I2C_TRANSACTION_TIMEOUT_MS) != 0) {
      SENSOR_ERR("Failed to wait i2c transmit\r\n");
      goto i2c_err;
    }

    if ((uint32_t)i2c_ev_res != I2C_EVENT_TRANSACTION_DONE) {
      SENSOR_ERR("I2C trasaction err: %lx\r\n", i2c_ev_res);
      goto i2c_err;
    }

#if (configUSE_ALT_SLEEP == 1)
    smngr_dev_busy_clr(bus_data->smngr_sync_id);
#endif
    alt_osal_unlock_mutex(&bus_data->rw_lock);
  } else {
    SENSOR_ERR("Failed to obtain platform read write lock\r\n");
    return SENSOR_RET_TIMEOUT;
  }

  return SENSOR_RET_SUCCESS;

i2c_err:
#if (configUSE_ALT_SLEEP == 1)
  smngr_dev_busy_clr(bus_data->smngr_sync_id);
#endif
  alt_osal_unlock_mutex(&bus_data->rw_lock);
  return SENSOR_RET_GENERIC_ERR;
}

static void sensor_interrupt_handler(void *user_param) {
  int8_t indication = 0;
  GPIO_Id gpio_id = (GPIO_Id)user_param;
  indication = (int8_t)gpio_id;

  uint8_t gpio_value = 0;

  DRV_GPIO_GetValue(gpio_id, &gpio_value);

  if (gpio_value) {
    DRV_GPIO_ConfigIrq(gpio_id, GPIO_IRQ_LOW_LEVEL, GPIO_PULL_DONT_CHANGE);
    alt_osal_send_mqueue(&sensor_ev_queue, (FAR int8_t *)&indication, sizeof(indication), 0);
  } else {
    DRV_GPIO_ConfigIrq(gpio_id, GPIO_IRQ_HIGH_LEVEL, GPIO_PULL_DONT_CHANGE);
  }
}

static int32_t clear_gpio_interrupt(GPIO_Id gpio_id) {
  GPIO_Pull gpio_pull;
  DRV_GPIO_Status gpio_ret;

  DRV_GPIO_DisableIrq(gpio_id);

  if ((gpio_ret = DRV_GPIO_GetPull(gpio_id, &gpio_pull)) != DRV_GPIO_OK) {
    SENSOR_ERR("Unable to get gpio%d pull mode: %d\r\n", gpio_id, gpio_ret);
    return SENSOR_RET_GENERIC_ERR;
  }
  if ((gpio_ret = DRV_GPIO_ConfigIrq(gpio_id, GPIO_IRQ_HIGH_LEVEL, gpio_pull)) != DRV_GPIO_OK) {
    SENSOR_ERR("Failed to restore gpio interrupt to high level\r\n");
    return SENSOR_RET_GENERIC_ERR;
  }

#if (configUSE_ALT_SLEEP == 1)
  pwr_mngr_del_monitor_io(gpio_id);
#endif
  return SENSOR_RET_SUCCESS;
}

static int32_t config_gpio_interrupt(GPIO_Id gpio_id) {
  GPIO_Pull gpio_pull;
  DRV_GPIO_Status gpio_ret;

  if ((gpio_ret = DRV_GPIO_GetPull(gpio_id, &gpio_pull)) != DRV_GPIO_OK) {
    SENSOR_ERR("Unable to get gpio%d pull mode: %d\r\n", gpio_id, gpio_ret);
    return SENSOR_RET_GENERIC_ERR;
  }
  if ((gpio_ret = DRV_GPIO_ConfigIrq(gpio_id, GPIO_IRQ_HIGH_LEVEL, gpio_pull)) == DRV_GPIO_OK &&
      (gpio_ret = DRV_GPIO_RegisterIrqCallback(gpio_id, sensor_interrupt_handler,
                                               (void *)gpio_id)) == DRV_GPIO_OK &&
      (gpio_ret = DRV_GPIO_EnableIrq(gpio_id)) == DRV_GPIO_OK) {
    SENSOR_DBG("Setting gpio %d as interrupt with high level trigger\r\n", gpio_id);
  } else {
    SENSOR_ERR("Failed to set gpio %d as interrupt: %d\r\n", gpio_id, gpio_ret);
    return SENSOR_RET_GENERIC_ERR;
  }
#if (configUSE_ALT_SLEEP == 1)
  if (pwr_mngr_set_monitor_io(gpio_id, 1) != PWR_MNGR_OK) {
    SENSOR_ERR("Failed to set PWR monitored pin\r\n");
    return SENSOR_RET_GENERIC_ERR;
  }
#endif
  return SENSOR_RET_SUCCESS;
}

static inline void wait_for_sensor_isr() {
  struct sensor_irq_entry *irq_vec;
  int8_t indication;
  uint8_t i;
  GPIO_Id gpio_id;
  while (alt_osal_recv_mqueue(&sensor_ev_queue, (FAR int8_t *)&indication, sizeof(indication),
                              ALT_OSAL_TIMEO_FEVR) != sizeof(indication))
    ;
  irq_vec = irq_vec_table + indication;
  gpio_id = (GPIO_Id)indication;
  if (alt_osal_lock_mutex(&irq_lock, ALT_OSAL_TIMEO_FEVR) == 0) {
    if (irq_vec->en_msk) {
      for (i = 0; i < MAX_IRQ_PER_LINE; i++) {
        if ((irq_vec->en_msk & (1 << i)) && (irq_vec->handler[i] != NULL))
          if (irq_vec->handler[i](irq_vec->user_param[i]) == SENSOR_IRQ_HANDLED) break;
      }
    }
    /*Make sure the irq status did not changed in user callback*/
    if (irq_vec->en_msk) {
      /*Restore the the interrupt level to high and wait for next event*/
      DRV_GPIO_DisableIrq(gpio_id);
      DRV_GPIO_ConfigIrq(gpio_id, GPIO_IRQ_HIGH_LEVEL, GPIO_PULL_DONT_CHANGE);
      DRV_GPIO_EnableIrq(gpio_id);
    }
    alt_osal_unlock_mutex(&irq_lock);
  } else {
    SENSOR_ERR("Failed to obtain ev lock\r\n");
  }
}

static void sensorHdlTask(void *pvParameters) {
  for (;;) {
    wait_for_sensor_isr();
    alt_osal_task_yield();
  }
}

int32_t sensor_free_irq(uint8_t dev_irq_id, GPIO_Id irq_line) {
  struct sensor_irq_entry *irq_vec;
  uint8_t orig_en_msk;
  int32_t res = SENSOR_RET_GENERIC_ERR;

  if (irq_line >= MCU_GPIO_ID_MAX) return SENSOR_RET_PARAM_ERR;

  irq_vec = irq_vec_table + irq_line;

  if (alt_osal_lock_mutex(&irq_lock, IRQ_LOCK_TIMEOUT_MS) == 0) {
    orig_en_msk = irq_vec->en_msk;
    irq_vec->en_msk &= ~(1 << dev_irq_id);
    irq_vec->handler[dev_irq_id] = NULL;
    irq_vec->user_param[dev_irq_id] = NULL;
    if (orig_en_msk != 0 && irq_vec->en_msk == 0) clear_gpio_interrupt(irq_line);
    res = SENSOR_RET_SUCCESS;
    alt_osal_unlock_mutex(&irq_lock);
  } else {
    SENSOR_ERR("Failed to obtain irq lock\r\n");
    return SENSOR_RET_TIMEOUT;
  }
  return res;
}

int32_t sensor_take_irq_lock(uint32_t timeout_ms) {
  if (alt_osal_lock_mutex(&irq_lock, (int32_t)timeout_ms) == 0)
    return SENSOR_RET_SUCCESS;
  else
    return SENSOR_RET_TIMEOUT;
}

int32_t sensor_give_irq_lock(void) {
  if (alt_osal_unlock_mutex(&irq_lock) == 0)
    return SENSOR_RET_SUCCESS;
  else
    return SENSOR_RET_GENERIC_ERR;
}

int32_t sensor_request_irq(GPIO_Id irq_line, sensor_irq_handler irq_handler, void *user_param,
                           uint8_t *dev_irq_id) {
  struct sensor_irq_entry *irq_vec;
  uint8_t i, orig_en_msk;
  int32_t res = SENSOR_RET_GENERIC_ERR;

  if (irq_line >= MCU_GPIO_ID_MAX) return SENSOR_RET_PARAM_ERR;

  irq_vec = irq_vec_table + irq_line;

  if (alt_osal_lock_mutex(&irq_lock, IRQ_LOCK_TIMEOUT_MS) == 0) {
    orig_en_msk = irq_vec->en_msk;
    for (i = 0; i < MAX_IRQ_PER_LINE; i++) {
      if (irq_vec->en_msk & (1 << i)) continue;
      irq_vec->en_msk |= (0x01 << i);
      irq_vec->handler[i] = irq_handler;
      irq_vec->user_param[i] = user_param;
      *dev_irq_id = i;
      res = SENSOR_RET_SUCCESS;
      if (orig_en_msk == 0) config_gpio_interrupt(irq_line);
      break;
    }
    alt_osal_unlock_mutex(&irq_lock);
  } else {
    SENSOR_ERR("Failed to obtain irq lock\r\n");
    return SENSOR_RET_TIMEOUT;
  }
  return res;
}

int32_t sensor_init(void) {
  alt_osal_queue_attribute que_param = {0};
  alt_osal_mutex_attribute mutex_param = {0};
  alt_osal_task_attribute task_param = {0};
  alt_osal_task_handle task_handle = NULL;

  memset(irq_vec_table, 0x00, sizeof(struct sensor_irq_entry) * MCU_GPIO_ID_MAX);
  memset(i2c_bus_table, 0x00, sizeof(struct i2c_bus_data) * I2C_BUS_COUNT);

  que_param.numof_queue = SENSOR_EVENT_QUEUE_SIZE;
  que_param.queue_size = sizeof(int8_t);
  if (alt_osal_create_mqueue(&sensor_ev_queue, &que_param) != 0) {
    SENSOR_ERR("Failed to create ev queue\r\n");
    goto init_err;
  }

  mutex_param.attr_bits |= ALT_OSAL_MUTEX_RECURSIVE;
  if (alt_osal_create_mutex(&irq_lock, &mutex_param) != 0) {
    SENSOR_ERR("Failed to create irq lock\r\n");
    goto init_err;
  }

  task_param.function = sensorHdlTask;
  task_param.name = "SENSOR TASK";
  task_param.priority = SENSOR_HANDLE_TASK_PRIORITY;
  task_param.stack_size = SENSOR_HANDLE_TASK_STACK_SIZE;
  if (alt_osal_create_task(&task_handle, &task_param) != 0) {
    SENSOR_ERR("Failed to create SENSOR TASK\r\n");
    goto init_err;
  }

  return SENSOR_RET_SUCCESS;

init_err:
  if (sensor_ev_queue) {
    alt_osal_delete_mqueue(&sensor_ev_queue);
    sensor_ev_queue = NULL;
  }
  if (irq_lock) {
    alt_osal_delete_mutex(irq_lock);
    irq_lock = NULL;
  }
  return SENSOR_RET_GENERIC_ERR;
}

sensor_i2c_handle_t sensor_register_i2c_dev(I2C_BusId bus_id, uint32_t dev_id,
                                            sensor_i2c_id_mode_e dev_id_mode, sensor_write_fp *w_fp,
                                            sensor_read_fp *r_fp) {
  alt_osal_mutex_attribute mutex_param = {0};
  alt_osal_eventflag_attribute evflag_param = {0};
  struct i2c_bus_data *bus_data = NULL;
  struct _s_i2c_handle *i2c_h = NULL;
  Drv_I2C_EventCallback evt_cb;

#if (configUSE_ALT_SLEEP == 1)
  char smngr_dev_name[16] = {0};
#endif

  if (bus_id == I2C_BUS_0) {
    bus_data = i2c_bus_table;
#if (configUSE_ALT_SLEEP == 1)
    snprintf(smngr_dev_name, 16, "SENSOR-BUS0");
#endif
  } else if (bus_id == I2C_BUS_1) {
    bus_data = i2c_bus_table + 1;
#if (configUSE_ALT_SLEEP == 1)
    snprintf(smngr_dev_name, 16, "SENSOR-BUS1");
#endif
  } else {
    goto reg_err;
  }

  i2c_h = malloc(sizeof(struct _s_i2c_handle));
  if (!i2c_h) goto reg_err;
  memset(i2c_h, 0x00, sizeof(struct _s_i2c_handle));

  if (!bus_data->init_done) {
    bus_data->tx_buf = malloc(sizeof(uint8_t) * I2C_TX_BUF_LEN);
    bus_data->tx_buf_len = I2C_TX_BUF_LEN;
    if (alt_osal_create_mutex(&bus_data->rw_lock, &mutex_param) != 0) {
      SENSOR_ERR("Failed to create read/write lock\r\n");
      goto reg_err;
    }
    if (alt_osal_create_eventflag(&bus_data->i2c_event, &evflag_param) != 0) {
      SENSOR_ERR("Failed to create i2c bus semaphore\r\n");
      goto reg_err;
    }
#if (configUSE_ALT_SLEEP == 1)
    if (smngr_register_dev_sync(smngr_dev_name, &bus_data->smngr_sync_id) != 0) {
      SENSOR_ERR("smngr register dev sync fail\n");
      goto reg_err;
    }
#endif
    evt_cb.user_data = (void *)bus_data;
    evt_cb.event_callback = i2c_evtcb;
    if (DRV_I2C_Initialize(bus_id, &evt_cb, &bus_data->i2c_p) != DRV_I2C_OK) {
      SENSOR_ERR("Failed to init i2c_bus\r\n");
      goto reg_err;
    }
    if (DRV_I2C_PowerControl(bus_data->i2c_p, I2C_POWER_FULL) != DRV_I2C_OK) {
      SENSOR_ERR("Failed to init power on i2c_bus\r\n");
      goto reg_err;
    }
    if (DRV_I2C_SetSpeed(bus_data->i2c_p, 100) != DRV_I2C_OK) {
      SENSOR_ERR("Failed to set speed of i2c_bus\r\n");
      goto reg_err;
    }
    bus_data->init_done = 1;
  }

  if (dev_id_mode == SENSOR_I2C_ID_10BITS)
    i2c_h->dev_id = dev_id | I2C_10BIT_INDICATOR;
  else
    i2c_h->dev_id = dev_id;
  i2c_h->i2c_bus = bus_data;

  *w_fp = sensor_i2c_write;
  *r_fp = sensor_i2c_read;

  return (sensor_i2c_handle_t)i2c_h;

reg_err:

  if (i2c_h) free(i2c_h);

  if (!bus_data->init_done) {
    if (bus_data->tx_buf) {
      free(bus_data->tx_buf);
      bus_data->tx_buf = NULL;
      bus_data->tx_buf_len = 0;
    }
    if (bus_data->rw_lock) {
      alt_osal_delete_mutex(&bus_data->rw_lock);
      bus_data->rw_lock = NULL;
    }
    if (bus_data->i2c_event) {
      alt_osal_delete_eventflag(&bus_data->i2c_event);
      bus_data->i2c_event = NULL;
    }
  }
  return NULL;
}
