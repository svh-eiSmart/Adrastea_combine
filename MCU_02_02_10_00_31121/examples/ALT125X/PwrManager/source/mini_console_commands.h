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

/* Declare extra CLI commands in this file */
#undef MCU_PROJECT_NAME
#define MCU_PROJECT_NAME "PowerManager"
DECLARE_COMMAND("map", do_map, "map - Pipeline to MAP via internal UART")
DECLARE_COMMAND("pwrMode", do_pwrMode,
                "pwrMode - Configure MCU sleep mode and duration (units: MSec). Usage:[stop | "
                "standby | shutdown] {<DURATION_VAL>}")
DECLARE_COMMAND("sleepSet", do_configureSleep,
                "sleepSet - Enable or disable go to sleep. Usage:[enable | disable]")
DECLARE_COMMAND(
    "gpioWkup", do_gpioWKUP2,
    "gpioWkup - Configure gpio wakeup source pin. Usage:<PHYSICAL_GPIO_ID> [set [ h | l "
    "| r | f | rf] | del]")
DECLARE_COMMAND(
    "gpioMon", do_configMonPin,
    "gpioMon - Configure gpio monitored pin. Usage:<LOGICAL_GPIO_ID> [set <VALUE> | del]")
DECLARE_COMMAND("retMemTest", do_RetentMemTest,
                "retMemTest - Read/Write a key-value pair to/from Retention Memory. Usage:[get "
                "[<KEY> | DUMPALL]| set <KEY> <VALUE>]")
DECLARE_COMMAND("sleepStat", do_sleepStatistics,
                "sleepStat - Print out or reset current statistics. Usage:[show | reset]")
DECLARE_COMMAND("wd", do_wd,
                "wd - Watchdog control command. Usage:[enable | disable | testbark | timeleft | "
                "timeout <NEW_TIMEOUT_VAL>] ")
DECLARE_COMMAND("inActTime", do_setInactivityTime,
                "inActTime - Configure the value of MCU CLI inactivity timeout in units of Sec. "
                "Usage:<NEW_TIMEOUT_VAL> ")
DECLARE_COMMAND("mapMode", do_setMAPmode,
                "mapMode - Switch to MAP mode when MCU wakeup from warm boot. Usage:[on | off] ")
DECLARE_COMMAND(
    "ioPark", do_ioPark,
    "ioPark - Configure IO Park pin. Usage:<PHYSICAL_GPIO_ID> [add | del | clr]")
//DECLARE_COMMAND("memRetenDbg", do_memRetenDbg,
//		        "memRetenDbg - Debug retention memory logic.  Usage:[-l | -z]")

/* Low level CLI for OS less mode and only For reference.
 * In freeRTOS platform, it should be implemented in IDLE task. */
// DECLARE_COMMAND("oslessStop", do_oslessStop, "oslessStop - Low level api for os less device")
// DECLARE_COMMAND("oslessStandby", do_oslessStandby, "oslessStandby - Low level api for os less
// device") DECLARE_COMMAND("oslessShutdown", do_oslessShutdown, "oslessShutdown - Low level api for
// os less device")
