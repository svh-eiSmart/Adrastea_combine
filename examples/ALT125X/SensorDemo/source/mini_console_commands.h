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

/* Declare extra CLI commands in this file */
#undef MCU_PROJECT_NAME
#define MCU_PROJECT_NAME "SensorIntegration"

DECLARE_COMMAND("map", do_map, "map - pipeline to MAP via internal UART")
DECLARE_COMMAND("map2", do_map2, "map2 - pipeline to MAP via internal UART")
DECLARE_COMMAND("i2c mode", do_i2c_speed, "i2c mode - i2c mode(speed) of i2c. Standard(1), Fast(2), Fast Plus(3), High speed (4) and ultra high speed (5)")
DECLARE_COMMAND("tids", do_tids, "tids [id | temp]")
DECLARE_COMMAND("itds", do_itds, "itds [id | temp | accel]")
DECLARE_COMMAND("pads", do_pads, "pads [id | temp | pressure]")
DECLARE_COMMAND("hids", do_hids, "hids [id | temp]")
DECLARE_COMMAND("test", do_test, "test spi and i2c in parallel")

