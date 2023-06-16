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
#include "cmsis_os2.h"
#include "pwr_mngr.h"
#include "accel_lis2dh.h"
#include "sensor.h"
#include "DRV_I2C.h"
#include "mini_console.h"
#include "DRV_PM.h"

#define PUSHWAKE_SMNGR_DEVNAME "DemoApp-PUSHWAKE"
uint32_t pushwake_sync_id;

static void on_accel_ev(uint8_t ev, void *user_param);

enum { EV_MOVE_START, EV_MOVE_END };

accel_ev_cfg_t accel_ev_mv_start_config = {.ev_h = on_accel_ev,
                                           .user_param = (void *)EV_MOVE_START,
                                           .ev_mode = ACCEL_LIS2DH_INT_MODE_OR,
                                           .ev_type = ACCEL_LIS2DH_INTEVENT_X_HIGH |
                                                      ACCEL_LIS2DH_INTEVENT_Y_HIGH |
                                                      ACCEL_LIS2DH_INTEVENT_Z_HIGH,
                                           .ev_thr_mg = 200,
                                           .ev_dur_ms = 0,
                                           .int_pin_set = MCU_GPIO_ID_01};

accel_ev_cfg_t accel_ev_mv_end_config = {.ev_h = on_accel_ev,
                                         .user_param = (void *)EV_MOVE_END,
                                         .ev_mode = ACCEL_LIS2DH_INT_MODE_AND,
                                         .ev_type = ACCEL_LIS2DH_INTEVENT_X_LOW |
                                                    ACCEL_LIS2DH_INTEVENT_Y_LOW |
                                                    ACCEL_LIS2DH_INTEVENT_Z_LOW,
                                         .ev_thr_mg = 200,
                                         .ev_dur_ms = 300,
                                         .int_pin_set = MCU_GPIO_ID_01};

uint8_t accel_dump_en = 0;

static void accel_dump_task(void *pvParameters) {
  xyz_mg_t accel_raw;

  while (accel_dump_en) {
    if (accel_has_data() && (accel_get_xyz_mg(&accel_raw) == ACCEL_RET_SUCCESS) && accel_dump_en)
      printf("x: %d    y: %d    z: %d\r\n", accel_raw.x, accel_raw.y, accel_raw.z);
  }
  osThreadExit();
}

static void on_accel_ev(uint8_t ev, void *user_param) {
  osThreadAttr_t attr = {0};
  int move_action = (int)user_param;
  int16_t temp;

  if (move_action == EV_MOVE_START) {
    printf("Move detected!!!\r\n");
    smngr_dev_busy_set(pushwake_sync_id);
    /*Change to monitor movement end event*/
    if (accel_start(ACCEL_EVENT_0, &accel_ev_mv_end_config) != ACCEL_RET_SUCCESS)
      printf("Failed to start monitor move end event\r\n");
    else {
      accel_dump_en = 1;

      attr.name = "accelDumpTask";
      attr.stack_size = 512;
      attr.priority = osPriorityLow;

      if (osThreadNew(accel_dump_task, NULL, &attr) == NULL)
        printf("Failed to create accel dump task\r\n");
    }

  } else if (move_action == EV_MOVE_END) {
    accel_dump_en = 0;
    printf("Move end\r\n");
    /*Change to monitor movement start event*/
    if (accel_start(ACCEL_EVENT_0, &accel_ev_mv_start_config) != ACCEL_RET_SUCCESS)
      printf("Faile to start monitor move start event\r\n");
    else {
      if (accel_get_temp_celsius(&temp) == ACCEL_RET_SUCCESS)
        printf("Temperature: %d\r\n", temp);
      else
        printf("Failed to get temperature\r\n");
      smngr_dev_busy_clr(pushwake_sync_id);
    }
  }
}

static void pushwake_init_task(void *arg) {
  if (sensor_init() != SENSOR_RET_SUCCESS) {
    printf("Failed to init sensor lib\r\n");
    while (1)
      ;
  }
  if (accel_init() != ACCEL_RET_SUCCESS) {
    printf("Failed to init accelerometer\r\n");
    while (1)
      ;
  }
  if (accel_start(ACCEL_EVENT_0, &accel_ev_mv_start_config) != ACCEL_RET_SUCCESS) {
    printf("Failed to start accelerometer motion event\r\n");
    while (1)
      ;
  }
  osThreadExit();
}

void pushwake_app_init(void) {
  osThreadAttr_t attr = {0};
  attr.name = "init task";
  attr.stack_size = 2048;
  attr.priority = osPriorityHigh;
  osThreadId_t tid = osThreadNew(pushwake_init_task, NULL, &attr);
  if (!tid) {
    printf("osThreadNew failed\n");
    for (;;)
      ;
  }
}

/*-----------------------------------------------------------*/
int main(void) {
  DRV_PM_Initialize();

  DRV_GPIO_Initialize();
  DRV_GPIO_PowerControl(GPIO_POWER_FULL);

  mini_console_ext(0);

  pushwake_app_init();

  if (smngr_register_dev_sync(PUSHWAKE_SMNGR_DEVNAME, &pushwake_sync_id) == 0) {
#if defined(ALT1250)
    pwr_mngr_conf_set_mode(PWR_MNGR_MODE_SHUTDOWN, 0);
#elif defined(ALT1255) /*TODO: Should remove this after shutdown mode is ready*/
    pwr_mngr_conf_set_mode(PWR_MNGR_MODE_STANDBY, 0);
#endif
    pwr_mngr_enable_sleep(1);
  }

  return 0;
}
