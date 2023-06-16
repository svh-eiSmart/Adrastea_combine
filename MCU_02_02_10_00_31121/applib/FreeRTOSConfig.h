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
#ifndef _DEFAULT_FREERTOS_CONFIG_H_
#define _DEFAULT_FREERTOS_CONFIG_H_

#include DEVICE_HEADER

// clang-format off

/* These are the required FreeRTOS defines to implement CMSIS RTOS2 API. Do not change! */
#define INCLUDE_xSemaphoreGetMutexHolder               1
#define INCLUDE_vTaskDelay                             1
#define INCLUDE_vTaskDelayUntil                        1
#define INCLUDE_vTaskDelete                            1
#define INCLUDE_xTaskGetCurrentTaskHandle              1
#define INCLUDE_xTaskGetSchedulerState                 1
#define INCLUDE_uxTaskGetStackHighWaterMark            1
#define INCLUDE_uxTaskPriorityGet                      1
#define INCLUDE_vTaskPrioritySet                       1
#define INCLUDE_eTaskGetState                          1
#define INCLUDE_vTaskSuspend                           1
#define INCLUDE_xTimerPendFunctionCall                 1
#define configUSE_TIMERS                               1
#define configUSE_MUTEXES                              1
#define configUSE_COUNTING_SEMAPHORES                  1
#define configUSE_TASK_NOTIFICATIONS                   1
#define configUSE_TRACE_FACILITY                       1
#define configUSE_16_BIT_TICKS                         0
#define configMAX_PRIORITIES                          56
#define configUSE_PORT_OPTIMISED_TASK_SELECTION        0


/* Common FreeRTOS configuration for hooks */
#define configUSE_IDLE_HOOK                            0
#define configUSE_TICK_HOOK                            1
#define configUSE_MALLOC_FAILED_HOOK                   1
#define configCHECK_FOR_STACK_OVERFLOW                 1


#define configCPU_CLOCK_HZ                             SystemCoreClock
#define configTICK_RATE_HZ                             ((TickType_t)1000)

#define configPRIO_BITS                                __NVIC_PRIO_BITS

#define configUSE_PREEMPTION                           1
#define configUSE_NEWLIB_REENTRANT                     1
#define configUSE_RECURSIVE_MUTEXES                    1

#define xPortPendSVHandler                             PendSV_Handler
#define vPortSVCHandler                                SVC_Handler

#define configTASK_NOTIFICATION_ARRAY_ENTRIES          2

/* Interrupt nesting behaviour configuration. */
// See https://www.freertos.org/RTOS-Cortex-M3-M4.html
// See https://www.freertos.org/a00110.html

// Set the priority of interrupts used by the kernel itself to the lowest
// possible interrupt priority
#define configKERNEL_INTERRUPT_PRIORITY \
    (((1 << configPRIO_BITS) - 1) << (8 - configPRIO_BITS))

#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
    (5 << (8 - configPRIO_BITS))


#ifndef configSUPPORT_STATIC_ALLOCATION
    #define configSUPPORT_STATIC_ALLOCATION            1
#endif


#ifndef configMINIMAL_STACK_SIZE
    #define configMINIMAL_STACK_SIZE                   ((unsigned short)768)
#endif

#ifndef configQUEUE_REGISTRY_SIZE
    #define configQUEUE_REGISTRY_SIZE                  8
#endif

#ifndef configUSE_STATS_FORMATTING_FUNCTIONS
    #define configUSE_STATS_FORMATTING_FUNCTIONS       1
#endif

/* Software timer related definitions. */
#ifndef configTIMER_TASK_PRIORITY
    #define configTIMER_TASK_PRIORITY                  48
#endif

#ifndef configTIMER_QUEUE_LENGTH
    #define configTIMER_QUEUE_LENGTH                   5
#endif

#ifndef configTIMER_TASK_STACK_DEPTH
    #define configTIMER_TASK_STACK_DEPTH               (configMINIMAL_STACK_SIZE * 2)
#endif



#ifdef NDEBUG
#define configASSERT(ignore) ((void)0)
#else
__NO_RETURN void portASSERT(const char *, int, const char *, const char *);
#define configASSERT(x) ((x) ? (void)0 : portASSERT(__FILE__, __LINE__, NULL, #x))
#endif

void trace_tick_count(const uint32_t);
#define traceINCREASE_TICK_COUNT(x)                    trace_tick_count(x)


#if (configUSE_ALT_SLEEP == 1)
    #define configUSE_TICKLESS_IDLE                    1
#endif


// clang-format on

#endif /* _DEFAULT_FREERTOS_CONFIG_H_ */
