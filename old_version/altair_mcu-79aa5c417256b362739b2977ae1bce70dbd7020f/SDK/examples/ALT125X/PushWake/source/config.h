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
#ifndef _CONFIG_H_
#define _CONFIG_H_

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular
 * application requirements.
 *----------------------------------------------------------*/

#define configUSE_ALT_SLEEP 1
/*Accelerometer lis2dh configuration*/
#if defined(ALT1250)
#define ACCEL_SENSOR_BUS I2C_BUS_1
#define ACCEL_SENSOR_ADDR 0x18
#elif defined(ALT1255)
#define ACCEL_SENSOR_BUS I2C_BUS_0
#define ACCEL_SENSOR_ADDR 0x19
#endif
#define ACCEL_LIS2DH_ODR LIS2DH12_ODR_10Hz
#define ACCEL_LIS2DH_SCALE LIS2DH12_2g
#define ACCEL_LIS2DH_OP_MODE LIS2DH12_LP_8bit
/*Sensor lib configuration*/
#define SENSOR_HANDLE_TASK_STACK_SIZE (4096)
#define SENSOR_HANDLE_TASK_PRIORITY (ALT_OSAL_TASK_PRIO_HIGH)
#define SENSOR_EVENT_QUEUE_SIZE 10
#endif /* _CONFIG_H_ */
