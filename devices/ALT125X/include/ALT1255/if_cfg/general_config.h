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
  ALT1255_GPIO_UNDEF = 0,
  ALT1255_GPIO1,   // G15
  ALT1255_GPIO2,   // F14
  ALT1255_GPIO3,   // E13
  ALT1255_GPIO4,   // N/A - DO NOT USE
  ALT1255_GPIO5,   // N/A - DO NOT USE
  ALT1255_GPIO6,   // N/A - DO NOT USE
  ALT1255_GPIO7,   // N/A - DO NOT USE
  ALT1255_GPIO8,   // N/A - DO NOT USE
  ALT1255_GPIO9,   // N/A - DO NOT USE
  ALT1255_GPIO10,  // N/A - DO NOT USE
  ALT1255_GPIO11,  // F8
  ALT1255_GPIO12,  // F10
  ALT1255_GPIO13,  // J7
  ALT1255_GPIO14,  // L5
  ALT1255_GPIO15,  // K6
  ALT1255_GPIO16,  // F6
  ALT1255_GPIO17,  // N/A - DO NOT USE
  ALT1255_GPIO18,  // N/A - DO NOT USE
  ALT1255_GPIO19,  // N/A - DO NOT USE
  ALT1255_GPIO20,  // E11
  ALT1255_GPIO21,  // N/A - DO NOT USE
  ALT1255_GPIO22,  // F12
  ALT1255_GPIO23,  // K12
  ALT1255_GPIO24,  // J13
  ALT1255_GPIO25,  // H12
  ALT1255_GPIO26,  // G13
  ALT1255_GPIO27,  // J11
  ALT1255_GPIO28,  // K10
  ALT1255_GPIO29,  // H10
  ALT1255_GPIO30,  // G11
  ALT1255_GPIO31,  // N/A - DO NOT USE
  ALT1255_GPIO32,  // N/A - DO NOT USE
  ALT1255_GPIO33,  // N/A - DO NOT USE
  ALT1255_GPIO34,  // N/A - DO NOT USE
  ALT1255_GPIO35,  // N/A - DO NOT USE
  ALT1255_GPIO36,  // N/A - DO NOT USE
  ALT1255_GPIO37,  // N/A - DO NOT USE
  ALT1255_GPIO38,  // L9
  ALT1255_GPIO39,  // H8
  ALT1255_GPIO40,  // J9
  ALT1255_GPIO41,  // K8
  ALT1255_GPIO42,  // M12
  ALT1255_GPIO43,  // P12
  ALT1255_GPIO44,  // N/A - DO NOT USE
  ALT1255_GPIO45,  // N/A - DO NOT USE
  ALT1255_GPIO46,  // N/A - DO NOT USE
  ALT1255_GPIO47,  // N/A - DO NOT USE
  ALT1255_GPIO48,  // N/A - DO NOT USE
  ALT1255_GPIO49,  // N/A - DO NOT USE
  ALT1255_GPIO50,  // N/A - DO NOT USE
  ALT1255_GPIO51,  // N/A - DO NOT USE
  ALT1255_GPIO52,  // N/A - DO NOT USE
  ALT1255_GPIO53,  // N/A - DO NOT USE
  ALT1255_GPIO54,  // N/A - DO NOT USE
  ALT1255_GPIO55,  // N/A - DO NOT USE
  ALT1255_GPIO56,  // N11
  ALT1255_GPIO57,  // L11
  ALT1255_GPIO58,  // M10
  ALT1255_GPIO59,  // N9
  ALT1255_GPIO60,  // G9
  ALT1255_GPIO61,  // J3
  ALT1255_GPIO62,  // N/A - DO NOT USE
  ALT1255_GPIO63,  // N/A - DO NOT USE
  ALT1255_GPIO64,  // N/A - DO NOT USE
  ALT1255_GPIO65,  // H14
  ALT1255_GPIO66,  // L13
  ALT1255_GPIO67,  // K14
  ALT1255_GPIO68,  // N/A - DO NOT USE
  ALT1255_GPIO69,  // N/A - DO NOT USE
  ALT1255_GPIO70,  // N15
  ALT1255_GPIO71,  // P14
  ALT1255_GPIO72,  // N13
  ALT1255_GPIO73,  // M14
  ALT1255_GPIO_NUM
} eMcuPinIds;

#define MCU_PIN_ID_NUM ALT1255_GPIO_NUM
#define MCU_PIN_ID_START ALT1255_GPIO1
#define MCU_PIN_ID_UNDEF ALT1255_GPIO_UNDEF
#define MCU_PIN_IS_VIRTUAL_PIN(p) (p == ALT1255_GPIO61)
#define MCU_PIN_IS_GPM_DOMAIN(p) (p >= ALT1255_GPIO1 && p <= ALT1255_GPIO61)
#define MCU_PIN_IS_PMP_DOMAIN(p) (p >= ALT1255_GPIO65 && pin_id <= ALT1255_GPIO73)
#endif /*GEN_CONFIG_H*/
