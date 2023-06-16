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
#include "ALT1255.h"
#include "DRV_IOSEL.h"
#include "DRV_COMMON.h"
#include <stddef.h>

/*This table uses MCU_PinId as index */
static IOSEL_RegisterOffset iosel_reg_offset_table[IOSEL_CFG_TB_SIZE] = {
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*Dummy element. MCU_PinId starts from 1*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, GPIO0),
    /*MCU PIN ID: ALT1255_GPIO1, Ball No.: G15,  PAD Name: GPIO0*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, GPIO1),
    /*MCU PIN ID: ALT1255_GPIO2, Ball No.: F14,  PAD Name: GPIO1*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, GPIO2),
    /*MCU PIN ID: ALT1255_GPIO3, Ball No.: E13,  PAD Name: GPIO2*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1255_GPIO4, Ball No.: N/A,  PAD Name: N/A*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1255_GPIO5, Ball No.: N/A,  PAD Name: N/A*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1255_GPIO6, Ball No.: N/A,  PAD Name: N/A*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1255_GPIO7, Ball No.: N/A,  PAD Name: N/A*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1255_GPIO8, Ball No.: N/A,  PAD Name: N/A*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1255_GPIO9, Ball No.: N/A,  PAD Name: N/A*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1255_GPIO10, Ball No.: N/A,  PAD Name: N/A*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, RFFE_SCLK),
    /*MCU PIN ID: ALT1255_GPIO11, Ball No.: F8,  PAD Name: RFFE_SCLK*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, RFFE_SDATA),
    /*MCU PIN ID: ALT1255_GPIO12, Ball No.: F10,  PAD Name: RFFE_SDATA*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, SC_RST),
    /*MCU PIN ID: ALT1255_GPIO13, Ball No.: J7,  PAD Name: SC_RST*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, SC_IO),
    /*MCU PIN ID: ALT1255_GPIO14, Ball No.: L5,  PAD Name: SC_IO*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, SC_CLK),
    /*MCU PIN ID: ALT1255_GPIO15, Ball No.: K6,  PAD Name: SC_CLK*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, SC_DET),
    /*MCU PIN ID: ALT1255_GPIO16, Ball No.: F6,  PAD Name: SC_DET*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1255_GPIO17, Ball No.: N/A,  PAD Name: N/A*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1255_GPIO18, Ball No.: N/A,  PAD Name: N/A*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1255_GPIO19, Ball No.: N/A,  PAD Name: N/A*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, EJ_TRST),
    /*MCU PIN ID: ALT1255_GPIO20, Ball No.: E11,  PAD Name: EJ_TRST*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1255_GPIO21, Ball No.: N/A,  PAD Name: N/A*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, EJ_TDO),
    /*MCU PIN ID: ALT1255_GPIO22, Ball No.: F12,  PAD Name: EJ_TDO*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, UART0_RX),
    /*MCU PIN ID: ALT1255_GPIO23, Ball No.: K12,  PAD Name: UART0_RX*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, UART0_TX),
    /*MCU PIN ID: ALT1255_GPIO24, Ball No.: J13,  PAD Name: UART0_TX*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, UART0_CTS),
    /*MCU PIN ID: ALT1255_GPIO25, Ball No.: H12,  PAD Name: UART0_CTS*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, UART0_RTS),
    /*MCU PIN ID: ALT1255_GPIO26, Ball No.: G13,  PAD Name: UART0_RTS*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, UART2_RX),
    /*MCU PIN ID: ALT1255_GPIO27, Ball No.: J11,  PAD Name: UART2_RX*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, UART2_TX),
    /*MCU PIN ID: ALT1255_GPIO28, Ball No.: K10,  PAD Name: UART2_TX*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, UART2_CTS),
    /*MCU PIN ID: ALT1255_GPIO29, Ball No.: H10,  PAD Name: UART2_CTS*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, UART2_RTS),
    /*MCU PIN ID: ALT1255_GPIO30, Ball No.: G11,  PAD Name: UART2_RTS*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1255_GPIO31, Ball No.: N/A,  PAD Name: N/A*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1255_GPIO32, Ball No.: N/A,  PAD Name: N/A*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1255_GPIO33, Ball No.: N/A,  PAD Name: N/A*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1255_GPIO34, Ball No.: N/A,  PAD Name: N/A*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1255_GPIO35, Ball No.: N/A,  PAD Name: N/A*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1255_GPIO36, Ball No.: N/A,  PAD Name: N/A*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1255_GPIO37, Ball No.: N/A,  PAD Name: N/A*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, SPIM1_MOSI),
    /*MCU PIN ID: ALT1255_GPIO38, Ball No.: L9,  PAD Name: SPIM1_MOSI*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, SPIM1_MISO),
    /*MCU PIN ID: ALT1255_GPIO39, Ball No.: H8,  PAD Name: SPIM1_MISO*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, SPIM1_EN),
    /*MCU PIN ID: ALT1255_GPIO40, Ball No.: J9,  PAD Name: SPIM1_EN*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, SPIM1_CLK),
    /*MCU PIN ID: ALT1255_GPIO41, Ball No.: K8,  PAD Name: SPIM1_CLK*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, I2C0_SDA),
    /*MCU PIN ID: ALT1255_GPIO42, Ball No.: M12,  PAD Name: I2C0_SDA*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, I2C0_SCL),
    /*MCU PIN ID: ALT1255_GPIO43, Ball No.: P12,  PAD Name: I2C0_SCL*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1255_GPIO44, Ball No.: N/A,  PAD Name: N/A*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1255_GPIO45, Ball No.: N/A,  PAD Name: N/A*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1255_GPIO46, Ball No.: N/A,  PAD Name: N/A*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1255_GPIO47, Ball No.: N/A,  PAD Name: N/A*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1255_GPIO48, Ball No.: N/A,  PAD Name: N/A*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1255_GPIO49, Ball No.: N/A,  PAD Name: N/A*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1255_GPIO50, Ball No.: N/A,  PAD Name: N/A*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1255_GPIO51, Ball No.: N/A,  PAD Name: N/A*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1255_GPIO52, Ball No.: N/A,  PAD Name: N/A*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1255_GPIO53, Ball No.: N/A,  PAD Name: N/A*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1255_GPIO54, Ball No.: N/A,  PAD Name: N/A*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1255_GPIO55, Ball No.: N/A,  PAD Name: N/A*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, FLASH1_IO0),
    /*MCU PIN ID: ALT1255_GPIO56, Ball No.: N11,  PAD Name: FLASH1_IO0*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, FLASH1_IO1),
    /*MCU PIN ID: ALT1255_GPIO57, Ball No.: L11,  PAD Name: FLASH1_IO1*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, FLASH1_IO2),
    /*MCU PIN ID: ALT1255_GPIO58, Ball No.: M10,  PAD Name: FLASH1_IO2*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, FLASH1_IO3),
    /*MCU PIN ID: ALT1255_GPIO59, Ball No.: N9,  PAD Name: FLASH1_IO3*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, CLKOUT),
    /*MCU PIN ID: ALT1255_GPIO60, Ball No.: G9,  PAD Name: CLKOUT*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, WAKEUP),
    /*MCU PIN ID: ALT1255_GPIO61, Ball No.: J3,  PAD Name: WAKEUP*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1255_GPIO62, Ball No.: N/A,  PAD Name: N/A*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1255_GPIO63, Ball No.: N/A,  PAD Name: N/A*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1255_GPIO64, Ball No.: N/A,  PAD Name: N/A*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, FLASH0_CS_N0),
    /*MCU PIN ID: ALT1255_GPIO65, Ball No.H14,  PAD Name: FLASH0_CS_N0*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, FLASH0_CS_N1),
    /*MCU PIN ID: ALT1255_GPIO66, Ball No.L13,  PAD Name: FLASH0_CS_N1*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, FLASH0_SCK),
    /*MCU PIN ID: ALT1255_GPIO67, Ball No.K14,  PAD Name: FLASH0_SCK*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1255_GPIO68, Ball No.: N/A,  PAD Name: N/A*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1255_GPIO69, Ball No.: N/A,  PAD Name: N/A*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, FLASH0_IO0),
    /*MCU PIN ID: ALT1255_GPIO70, Ball No.: N15,  PAD Name: FLASH0_IO0*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, FLASH0_IO1),
    /*MCU PIN ID: ALT1255_GPIO71, Ball No.: P14,  PAD Name: FLASH0_IO1*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, FLASH0_IO2),
    /*MCU PIN ID: ALT1255_GPIO72, Ball No.: N13,  PAD Name: FLASH0_IO2*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, FLASH0_IO3),
    /*MCU PIN ID: ALT1255_GPIO73, Ball No.: M14,  PAD Name: FLASH0_IO3*/
};

IOSEL_RegisterOffset DRV_IOSEL_GetRegisterOffset(MCU_PinId pin_id) {
  if (pin_id >= IOSEL_CFG_TB_SIZE) return IOSEL_REGISTER_OFFSET_UNDEFINED;
  return iosel_reg_offset_table[pin_id];
}

static uint8_t iosel_mux_table[IOSEL_CFG_TB_SIZE] = {4};

void DRV_IOSEL_PreSleepProcess(void) {
  MCU_PinId pin_id;
  IOSEL_RegisterOffset reg_offset;
  for (pin_id = ALT1255_GPIO1; pin_id < IOSEL_CFG_TB_SIZE; pin_id++) {
    reg_offset = iosel_reg_offset_table[pin_id];
    if (reg_offset != IOSEL_REGISTER_OFFSET_UNDEFINED) {
      iosel_mux_table[pin_id] = (uint8_t)REGISTER(IO_FUNC_SEL_BASE + reg_offset);
    }
  }
}
void DRV_IOSEL_PostSleepProcess(void) {
  MCU_PinId pin_id;
  IOSEL_RegisterOffset reg_offset;
  for (pin_id = ALT1255_GPIO1; pin_id < IOSEL_CFG_TB_SIZE; pin_id++) {
    reg_offset = iosel_reg_offset_table[pin_id];
    if (reg_offset != IOSEL_REGISTER_OFFSET_UNDEFINED) {
      REGISTER(IO_FUNC_SEL_BASE + reg_offset) = (uint32_t)iosel_mux_table[pin_id];
    }
  }
}
