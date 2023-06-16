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
#include "ALT1250.h"
#include "DRV_IOSEL.h"
#include "DRV_COMMON.h"
#include <stddef.h>

/*This table uses MCU_PinId as index */
static IOSEL_RegisterOffset iosel_reg_offset_table[IOSEL_CFG_TB_SIZE] = {
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*Dummy element. MCU_PinId starts from 1*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, GPIO0),
    /*MCU PIN ID: ALT1250_GPIO1, Ball No.: L13,  PAD Name: GPIO0*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, GPIO1),
    /*MCU PIN ID: ALT1250_GPIO2, Ball No.: M14,  PAD Name: GPIO1*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, GPIO2),
    /*MCU PIN ID: ALT1250_GPIO3, Ball No.: K14,  PAD Name: GPIO2*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, GPIO3),
    /*MCU PIN ID: ALT1250_GPIO4, Ball No.: J13,  PAD Name: GPIO3*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, GPIO4),
    /*MCU PIN ID: ALT1250_GPIO5, Ball No.: H12,  PAD Name: GPIO4*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, GPIO5),
    /*MCU PIN ID: ALT1250_GPIO6, Ball No.: K12,  PAD Name: GPIO5*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1250_GPIO7, Ball No.: N/A,  PAD Name: N/A*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1250_GPIO8, Ball No.: N/A,  PAD Name: N/A*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1250_GPIO9, Ball No.: N/A,  PAD Name: N/A*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, RFFE_VDDIO_OUT),
    /*MCU PIN ID: ALT1250_GPIO10, Ball No.: H10,  PAD Name: RFFE_VDDIO_OUT*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, RFFE_SCLK),
    /*MCU PIN ID: ALT1250_GPIO11, Ball No.: H6,  PAD Name: RFFE_SCLK*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, RFFE_SDATA),
    /*MCU PIN ID: ALT1250_GPIO12, Ball No.: H8,  PAD Name: RFFE_SDATA*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, SC_RST),
    /*MCU PIN ID: ALT1250_GPIO13, Ball No.: M8,  PAD Name: SC_RST*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, SC_IO),
    /*MCU PIN ID: ALT1250_GPIO14, Ball No.: M10,  PAD Name: SC_IO*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, SC_CLK),
    /*MCU PIN ID: ALT1250_GPIO15, Ball No.: L9,  PAD Name: SC_CLK*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, SC_DET),
    /*MCU PIN ID: ALT1250_GPIO16, Ball No.: J11,  PAD Name: SC_DET*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, SC_SWP),
    /*MCU PIN ID: ALT1250_GPIO17, Ball No.: J9,  PAD Name: SC_SWP*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, EJ_TCK),
    /*MCU PIN ID: ALT1250_GPIO18, Ball No.: R5,  PAD Name: EJ_TCK*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, EJ_TMS),
    /*MCU PIN ID: ALT1250_GPIO19, Ball No.: K4,  PAD Name: EJ_TMS*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, EJ_TRST),
    /*MCU PIN ID: ALT1250_GPIO20, Ball No.: J5,  PAD Name: EJ_TRST*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, EJ_TDI),
    /*MCU PIN ID: ALT1250_GPIO21, Ball No.: L5,  PAD Name: EJ_TDI*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, EJ_TDO),
    /*MCU PIN ID: ALT1250_GPIO22, Ball No.: N5,  PAD Name: EJ_TDO*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, UART0_RX),
    /*MCU PIN ID: ALT1250_GPIO23, Ball No.: G11,  PAD Name: UART0_RX*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, UART0_TX),
    /*MCU PIN ID: ALT1250_GPIO24, Ball No.: K10,  PAD Name: UART0_TX*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, UART0_CTS),
    /*MCU PIN ID: ALT1250_GPIO25, Ball No.: G9,  PAD Name: UART0_CTS*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, UART0_RTS),
    /*MCU PIN ID: ALT1250_GPIO26, Ball No.: K8,  PAD Name: UART0_RTS*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, UART2_RX),
    /*MCU PIN ID: ALT1250_GPIO27, Ball No.: H14,  PAD Name: UART2_RX*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, UART2_TX),
    /*MCU PIN ID: ALT1250_GPIO28, Ball No.: G13,  PAD Name: UART2_TX*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, UART2_CTS),
    /*MCU PIN ID: ALT1250_GPIO29, Ball No.: G15,  PAD Name: UART2_CTS*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, UART2_RTS),
    /*MCU PIN ID: ALT1250_GPIO30, Ball No.: K6,  PAD Name: UART2_RTS*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, DEBUG_SEL),
    /*MCU PIN ID: ALT1250_GPIO31, Ball No.: M4,  PAD Name: DEBUG_SEL*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1250_GPIO32, Ball No.: P4,  PAD Name: N/A*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, SPIM0_MOSI),
    /*MCU PIN ID: ALT1250_GPIO33, Ball No.: U13,  PAD Name: SPIM0_MOSI*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, SPIM0_MISO),
    /*MCU PIN ID: ALT1250_GPIO34, Ball No.: T12,  PAD Name: SPIM0_MISO*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, SPIM0_EN0),
    /*MCU PIN ID: ALT1250_GPIO35, Ball No.: P12,  PAD Name: SPIM0_EN0*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, SPIM0_EN1),
    /*MCU PIN ID: ALT1250_GPIO36, Ball No.: R13,  PAD Name: SPIM0_EN1*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, SPIM0_CLK),
    /*MCU PIN ID: ALT1250_GPIO37, Ball No.: V12,  PAD Name: SPIM0_CLK*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, SPIM1_MOSI),
    /*MCU PIN ID: ALT1250_GPIO38, Ball No.: T8,  PAD Name: SPIM1_MOSI*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, SPIM1_MISO),
    /*MCU PIN ID: ALT1250_GPIO39, Ball No.: N9,  PAD Name: SPIM1_MISO*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, SPIM1_EN),
    /*MCU PIN ID: ALT1250_GPIO40, Ball No.: P8,  PAD Name: SPIM1_EN*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, SPIM1_CLK),
    /*MCU PIN ID: ALT1250_GPIO41, Ball No.: R9,  PAD Name: SPIM1_CLK*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, I2C0_SDA),
    /*MCU PIN ID: ALT1250_GPIO42, Ball No.: J7,  PAD Name: I2C0_SDA*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, I2C0_SCL),
    /*MCU PIN ID: ALT1250_GPIO43, Ball No.: L7,  PAD Name: I2C0_SCL*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, I2C1_SDA),
    /*MCU PIN ID: ALT1250_GPIO44, Ball No.: H2,  PAD Name: I2C1_SDA*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, I2C1_SCL),
    /*MCU PIN ID: ALT1250_GPIO45, Ball No.: H4,  PAD Name: I2C1_SCL*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, PCM_CLK),
    /*MCU PIN ID: ALT1250_GPIO46, Ball No.: V10,  PAD Name: PCM_CLK*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, PCM_FS),
    /*MCU PIN ID: ALT1250_GPIO47, Ball No.: R11,  PAD Name: PCM_FS*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, PCM_IN),
    /*MCU PIN ID: ALT1250_GPIO48, Ball No.: N11,  PAD Name: PCM_IN*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, PCM_OUT),
    /*MCU PIN ID: ALT1250_GPIO49, Ball No.: T10,  PAD Name: PCM_OUT*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, PWM0),
    /*MCU PIN ID: ALT1250_GPIO50, Ball No.: P10,  PAD Name: PWM0*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, PWM1),
    /*MCU PIN ID: ALT1250_GPIO51, Ball No.: L11,  PAD Name: PWM1*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, PWM2),
    /*MCU PIN ID: ALT1250_GPIO52, Ball No.: M12,  PAD Name: PWM2*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, PWM3),
    /*MCU PIN ID: ALT1250_GPIO53, Ball No.: N13,  PAD Name: PWM3*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, FLASH1_CS_N),
    /*MCU PIN ID: ALT1250_GPIO54, Ball No.: M6,  PAD Name: FLASH1_CS_N*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, FLASH1_SCK),
    /*MCU PIN ID: ALT1250_GPIO55, Ball No.: U7,  PAD Name: FLASH1_SCK*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, FLASH1_IO0),
    /*MCU PIN ID: ALT1250_GPIO56, Ball No.: T6,  PAD Name: FLASH1_IO0*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, FLASH1_IO1),
    /*MCU PIN ID: ALT1250_GPIO57, Ball No.: R7,  PAD Name: FLASH1_IO1*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, FLASH1_IO2),
    /*MCU PIN ID: ALT1250_GPIO58, Ball No.: P6,  PAD Name: FLASH1_IO2*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, FLASH1_IO3),
    /*MCU PIN ID: ALT1250_GPIO59, Ball No.: N7,  PAD Name: FLASH1_IO3*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, CLKOUT),
    /*MCU PIN ID: ALT1250_GPIO60, Ball No.: U9,  PAD Name: CLKOUT*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, WAKEUP),
    /*MCU PIN ID: ALT1250_GPIO61, Ball No.: P2,  PAD Name: WAKEUP*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, PB),
    /*MCU PIN ID: ALT1250_GPIO62, Ball No.: K2,  PAD Name: PB*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1250_GPIO63, Ball No.: N/A,  PAD Name: N/A*/
    IOSEL_REGISTER_OFFSET_UNDEFINED,
    /*MCU PIN ID: ALT1250_GPIO64, Ball No.: N/A,  PAD Name: N/A*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, FLASH0_CS_N0),
    /*MCU PIN ID: ALT1250_GPIO65, Ball No.W13: ,  PAD Name: FLASH0_CS_N0*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, FLASH0_CS_N1),
    /*MCU PIN ID: ALT1250_GPIO66, Ball No.V14: ,  PAD Name: FLASH0_CS_N1*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, FLASH0_SCK),
    /*MCU PIN ID: ALT1250_GPIO67, Ball No.: W15,  PAD Name: FLASH0_SCK*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, FLASH0_RESETN),
    /*MCU PIN ID: ALT1250_GPIO68, Ball No.: F12,  PAD Name: FLASH0_RESETN*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, FLASH0_DQS),
    /*MCU PIN ID: ALT1250_GPIO69, Ball No.: T14,  PAD Name: FLASH0_DQS*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, FLASH0_IO0),
    /*MCU PIN ID: ALT1250_GPIO70, Ball No.: AA15,  PAD Name: FLASH0_IO0*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, FLASH0_IO1),
    /*MCU PIN ID: ALT1250_GPIO71, Ball No.: Y14,  PAD Name: FLASH0_IO1*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, FLASH0_IO2),
    /*MCU PIN ID: ALT1250_GPIO72, Ball No.: AA13,  PAD Name: FLASH0_IO2*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, FLASH0_IO3),
    /*MCU PIN ID: ALT1250_GPIO73, Ball No.: Y12,  PAD Name: FLASH0_IO3*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, FLASH0_IO4),
    /*MCU PIN ID: ALT1250_GPIO74, Ball No.: AA11,  PAD Name: FLASH0_IO4*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, FLASH0_IO5),
    /*MCU PIN ID: ALT1250_GPIO75, Ball No.: Y10,  PAD Name: FLASH0_IO5*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, FLASH0_IO6),
    /*MCU PIN ID: ALT1250_GPIO76, Ball No.: U11,  PAD Name: FLASH0_IO6*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, FLASH0_IO7),
    /*MCU PIN ID: ALT1250_GPIO77, Ball No.: Y8,  PAD Name: FLASH0_IO7*/
    (IOSEL_RegisterOffset)offsetof(IO_FUNC_SEL_Type, FLASH0_CS_N2),
    /*MCU PIN ID: ALT1250_GPIO78, Ball No.: W11,  PAD Name: FLASH0_CS_N2*/
};

IOSEL_RegisterOffset DRV_IOSEL_GetRegisterOffset(MCU_PinId pin_id) {
  if (pin_id >= IOSEL_CFG_TB_SIZE) return IOSEL_REGISTER_OFFSET_UNDEFINED;
  return iosel_reg_offset_table[pin_id];
}

static uint8_t iosel_mux_table[IOSEL_CFG_TB_SIZE] = {4};

void DRV_IOSEL_PreSleepProcess(void) {
  MCU_PinId pin_id;
  IOSEL_RegisterOffset reg_offset;
  for (pin_id = ALT1250_GPIO1; pin_id < IOSEL_CFG_TB_SIZE; pin_id++) {
    reg_offset = iosel_reg_offset_table[pin_id];
    if (reg_offset != IOSEL_REGISTER_OFFSET_UNDEFINED) {
      iosel_mux_table[pin_id] = (uint8_t)REGISTER(IO_FUNC_SEL_BASE + reg_offset);
    }
  }
}
void DRV_IOSEL_PostSleepProcess(void) {
  MCU_PinId pin_id;
  IOSEL_RegisterOffset reg_offset;
  for (pin_id = ALT1250_GPIO1; pin_id < IOSEL_CFG_TB_SIZE; pin_id++) {
    reg_offset = iosel_reg_offset_table[pin_id];
    if (reg_offset != IOSEL_REGISTER_OFFSET_UNDEFINED) {
      REGISTER(IO_FUNC_SEL_BASE + reg_offset) = (uint32_t)iosel_mux_table[pin_id];
    }
  }
}
