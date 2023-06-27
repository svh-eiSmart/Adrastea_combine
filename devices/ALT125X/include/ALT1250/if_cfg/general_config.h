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
#ifndef GEN_CONFIG_H
#define GEN_CONFIG_H

typedef enum {
  ALT1250_GPIO_UNDEF = 0,
  ALT1250_GPIO1,   // L13 GPIO0
  ALT1250_GPIO2,   // M14 GPIO1
  ALT1250_GPIO3,   // K14 GPIO2
  ALT1250_GPIO4,   // J13 GPIO3
  ALT1250_GPIO5,   // H12
  ALT1250_GPIO6,   // K12
  ALT1250_GPIO7,   // N/A - DO NOT USE
  ALT1250_GPIO8,   // N/A - DO NOT USE
  ALT1250_GPIO9,   // N/A - DO NOT USE
  ALT1250_GPIO10,  // H10
  ALT1250_GPIO11,  // H6
  ALT1250_GPIO12,  // H8
  ALT1250_GPIO13,  // M8
  ALT1250_GPIO14,  // M10
  ALT1250_GPIO15,  // L9
  ALT1250_GPIO16,  // J11
  ALT1250_GPIO17,  // J9
  ALT1250_GPIO18,  // R5
  ALT1250_GPIO19,  // K4
  ALT1250_GPIO20,  // J5
  ALT1250_GPIO21,  // L5
  ALT1250_GPIO22,  // N5
  ALT1250_GPIO23,  // G11
  ALT1250_GPIO24,  // K10
  ALT1250_GPIO25,  // G9
  ALT1250_GPIO26,  // K8
  ALT1250_GPIO27,  // H14
  ALT1250_GPIO28,  // G13
  ALT1250_GPIO29,  // G15
  ALT1250_GPIO30,  // K6
  ALT1250_GPIO31,  // M4 - DO NOT USE
  ALT1250_GPIO32,  // P4 - DO NOT USE
  ALT1250_GPIO33,  // U13
  ALT1250_GPIO34,  // T12
  ALT1250_GPIO35,  // P12
  ALT1250_GPIO36,  // R13
  ALT1250_GPIO37,  // V12
  ALT1250_GPIO38,  // T8
  ALT1250_GPIO39,  // N9
  ALT1250_GPIO40,  // P8
  ALT1250_GPIO41,  // R9
  ALT1250_GPIO42,  // J7
  ALT1250_GPIO43,  // L7
  ALT1250_GPIO44,  // H2
  ALT1250_GPIO45,  // H4
  ALT1250_GPIO46,  // V10
  ALT1250_GPIO47,  // R11
  ALT1250_GPIO48,  // N11
  ALT1250_GPIO49,  // T10
  ALT1250_GPIO50,  // P10
  ALT1250_GPIO51,  // L11
  ALT1250_GPIO52,  // M12
  ALT1250_GPIO53,  // N13
  ALT1250_GPIO54,  // M6
  ALT1250_GPIO55,  // U7
  ALT1250_GPIO56,  // T6
  ALT1250_GPIO57,  // R7
  ALT1250_GPIO58,  // P6
  ALT1250_GPIO59,  // N7
  ALT1250_GPIO60,  // U9
  ALT1250_GPIO61,  // P2
  ALT1250_GPIO62,  // K2
  ALT1250_GPIO63,  // L15 - DO NOT USE
  ALT1250_GPIO64,  // J15 - DO NOT USE
  ALT1250_GPIO65,  // W13
  ALT1250_GPIO66,  // V14
  ALT1250_GPIO67,  // W15
  ALT1250_GPIO68,  // F12
  ALT1250_GPIO69,  // T14
  ALT1250_GPIO70,  // AA15
  ALT1250_GPIO71,  // Y14
  ALT1250_GPIO72,  // AA13
  ALT1250_GPIO73,  // Y12
  ALT1250_GPIO74,  // AA11
  ALT1250_GPIO75,  // Y10
  ALT1250_GPIO76,  // U11
  ALT1250_GPIO77,  // Y8
  ALT1250_GPIO78,  // W11
  ALT1250_GPIO_NUM
} eMcuPinIds;

#define MCU_PIN_ID_NUM ALT1250_GPIO_NUM
#define MCU_PIN_ID_START ALT1250_GPIO1
#define MCU_PIN_ID_UNDEF ALT1250_GPIO_UNDEF
#define MCU_PIN_IS_VIRTUAL_PIN(p) (p == ALT1250_GPIO61 || p == ALT1250_GPIO62)
#define MCU_PIN_IS_GPM_DOMAIN(p) (p >= ALT1250_GPIO1 && p <= ALT1250_GPIO62)
#define MCU_PIN_IS_PMP_DOMAIN(p) (p >= ALT1250_GPIO65 && pin_id <= ALT1250_GPIO78)
#endif /*GEN_CONFIG_H*/
