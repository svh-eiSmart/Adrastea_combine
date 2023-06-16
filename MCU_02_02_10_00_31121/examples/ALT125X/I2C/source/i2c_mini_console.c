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

/* Standard includes. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* CMSIS-RTOS V2 includes. */
#include "cmsis_os2.h"

/* DRV includes */
#include "DRV_I2C.h"

#ifdef ALT1250
#define MAX_I2C_DEVICES_QUANT (2)
#elif defined(ALT1255)
#define MAX_I2C_DEVICES_QUANT (1)
#else
#error No target platform specified!!
#endif

#define NONBLOCK_TX_FUNC_NAME "DRV_I2C_MasterTransmit_Nonblock"
#define NONBLOCK_RX_FUNC_NAME "DRV_I2C_MasterReceive_Nonblock"
#define BLOCK_TX_FUNC_NAME "DRV_I2C_MasterTransmit_Block"
#define BLOCK_RX_FUNC_NAME "DRV_I2C_MasterReceive_Block"

typedef DRV_I2C_Status (*i2cReceive)(Drv_I2C_Peripheral* i2c, uint32_t addr, const uint8_t* data,
                                     uint32_t num, bool xfer_pending);
typedef DRV_I2C_Status (*i2cTransmit)(Drv_I2C_Peripheral* i2c, uint32_t addr, const uint8_t* data,
                                      uint32_t num, bool xfer_pending);
typedef enum {
  OPMODE_NONBLOCK,
  OPMODE_BLOCK,
  OPMODE_MAX,
} I2C_OpMode;

osSemaphoreId_t i2c_sem = NULL;
bool i2c_err = false;
uint32_t i2c_event[MAX_I2C_DEVICES_QUANT] = {0};

void print_i2c_usage(void) {
  printf("Usage:\n\r");
  printf("  i2c bus [0|1] - Sets or print device bus number\r\n");
  printf("  i2c init - Initialize I2C driver\r\n");
  printf("  i2c pwrctrl <0|1> - Initialize I2C driver\r\n");
  printf("  i2c speed <speed> - Set I2C bus speed\r\n");
  printf("  i2c mode <0:Nonblock|1:Block> - Set I2C access mode\r\n");
  printf("  i2c <probe|probe10> [addrlen] - Probe I2C devices on bus\r\n");
  printf(
      "  i2c <read|read10> <device_id> <address> <addrlen> [num of bytes] - Read from I2C "
      "device\r\n");
  printf(
      "  i2c <write|write10> <device_id> <address> <addrlen> <value> [count] - Write to I2C "
      "device\r\n");
}

static void i2c_evtcb(void* user_data, uint32_t event) {
  if (event & (I2C_EVENT_TRANSACTION_INCOMPLETE | I2C_EVENT_TRANSACTION_ADDRNACK)) {
    i2c_err = true;
  }

  i2c_event[(uint32_t)user_data] = event;
  osSemaphoreRelease(i2c_sem);
}

int do_i2c(char* s) {
  char command[20] = {0}, par2[20] = {0}, par3[20] = {0}, par4[20] = {0}, par5[20] = {0},
       par6[20] = {0};
  int argc;
  int ret_val = 0;
  uint16_t device_id;
  uint32_t address, num, n;
  uint8_t data[64] = {0}, value;
  char* param;
  static I2C_BusId i2c_bus_id = I2C_BUS_0;
  uint8_t addrLen;
  uint16_t dev_num;
  static Drv_I2C_Peripheral* i2c[MAX_I2C_DEVICES_QUANT] = {0};
  static i2cReceive local_receive = DRV_I2C_MasterReceive_Nonblock;
  static char* local_receive_str = NONBLOCK_RX_FUNC_NAME;
  static i2cTransmit local_transmit = DRV_I2C_MasterTransmit_Nonblock;
  static char* local_transmit_str = NONBLOCK_TX_FUNC_NAME;
  static I2C_OpMode opmode = OPMODE_NONBLOCK;

  if (!i2c_sem) {
    i2c_sem = osSemaphoreNew(1, 0, NULL);
    if (!i2c_sem) {
      printf("osSemaphoreNew fail\n\r");
      return -1;
    }
  }

  argc = sscanf(s, "%s %s %s %s %s %s", command, par2, par3, par4, par5, par6) + 1;
  printf("argc is :", argc);

  if (argc <= 1) {
    print_i2c_usage();
    ret_val = -1;
  } else {
    if (argc > 1) {
      if (strcmp("bus", command) == 0) {
        if (argc == 2) {
          printf("Current device is %d\n\r", i2c_bus_id);
        } else if (argc == 3) {
          uint32_t device_number = strtoul(par2, NULL, 10);

          if (device_number < MAX_I2C_DEVICES_QUANT) {
            i2c_bus_id = (I2C_BusId)device_number;
            printf("Set I2C device %d\n\r", i2c_bus_id);
          } else {
            i2c_bus_id = I2C_BUS_0;
            printf("Cannot set I2C device to %ld, setting to default(0)\n\r",
                   (uint32_t)device_number);
          }
        } else {
          printf("i2c bus [Bus] - Sets or print device bus number\n\r");
        }
      } else if (strcmp("mode", command) == 0) {
        if (argc == 2) {
          printf("Current opmode is %ld\n\r", (uint32_t)opmode);
        } else if (argc == 3) {
          uint32_t mode = strtoul(par2, NULL, 10);

          if (mode < OPMODE_MAX) {
            opmode = (I2C_OpMode)mode;
            local_receive = opmode == OPMODE_NONBLOCK ? DRV_I2C_MasterReceive_Nonblock
                                                      : DRV_I2C_MasterReceive_Block;
            local_receive_str =
                opmode == OPMODE_NONBLOCK ? NONBLOCK_RX_FUNC_NAME : BLOCK_RX_FUNC_NAME;
            local_transmit = opmode == OPMODE_NONBLOCK ? DRV_I2C_MasterTransmit_Nonblock
                                                       : DRV_I2C_MasterTransmit_Block;
            local_transmit_str =
                opmode == OPMODE_NONBLOCK ? NONBLOCK_TX_FUNC_NAME : BLOCK_TX_FUNC_NAME;
            printf("Set I2C opmode %ld\n\r", (uint32_t)opmode);
          } else {
            opmode = OPMODE_NONBLOCK;
            local_receive = DRV_I2C_MasterReceive_Nonblock;
            local_receive_str = NONBLOCK_RX_FUNC_NAME;
            local_transmit = DRV_I2C_MasterTransmit_Nonblock;
            local_transmit_str = NONBLOCK_TX_FUNC_NAME;
            printf("Cannot set opmode to %ld, setting to default(0)\n\r", mode);
          }
        } else {
          printf("i2c mode [opmode] - Sets or print operation mode\n\r");
        }
      } else if (0 == strcmp("probe", command) || 0 == strcmp("probe10", command)) {
        uint32_t devid_mode;

        devid_mode = (0 == strcmp("probe10", command)) ? I2C_10BIT_INDICATOR : 0;
        dev_num = (I2C_10BIT_INDICATOR != devid_mode) ? ((1 << 7) - 1) : ((1 << 10) - 1);
        addrLen = 1;

        if (argc > 2) {
          param = par2;
          addrLen = (uint8_t)strtoul(param, NULL, 10);
          if (addrLen > 3) {
            addrLen = 3;
          }
        }

        printf("Valid %s devices IDs:", (I2C_10BIT_INDICATOR == devid_mode) ? "10-Bits" : "7-Bits");
        for (device_id = 0; device_id <= dev_num; device_id++) {
          DRV_I2C_Status ret;
          uint8_t reg[3] = {0};
          uint32_t dev_id = (uint32_t)(device_id | devid_mode);

          i2c_err = false;
          ret = local_transmit(i2c[i2c_bus_id], dev_id, reg, (uint32_t)addrLen, true);
          if (ret == DRV_I2C_OK) {
            if (opmode == OPMODE_NONBLOCK
                    ? (i2c_sem && (osSemaphoreAcquire(i2c_sem, osWaitForever) == osOK) && !i2c_err)
                    : true) {
              ret = local_receive(i2c[i2c_bus_id], dev_id, data, 1, false);
              if (ret == DRV_I2C_OK) {
                if (opmode == OPMODE_NONBLOCK
                        ? (i2c_sem && (osSemaphoreAcquire(i2c_sem, osWaitForever) == osOK) &&
                           !i2c_err)
                        : true) {
                  printf(" 0x%02X", (unsigned int)device_id);
                }
              }
            }
          }
        }

        printf("\n\r");
      } else if (0 == strcmp("read", command) || 0 == strcmp("read10", command)) {
        if (argc > 4) {
          address = 0;
          addrLen = 1;
          num = 1;
          param = par2;
          if ((param[0] == '0') && (param[1] == 'x')) {
            device_id = (uint16_t)strtoul(&param[0], NULL, 16);
          } else {
            device_id = (uint16_t)strtoul(param, NULL, 10);
          }

          param = par3;
          if ((param[0] == '0') && (param[1] == 'x')) {
            address = (uint32_t)strtoul(&param[2], NULL, 16);
          } else {
            address = (uint32_t)strtoul(param, NULL, 10);
          }

          param = par4;
          addrLen = (uint8_t)strtoul(param, NULL, 10);
          if (addrLen > 3) {
            addrLen = 3;
          }

          if (argc > 5) {
            param = par5;
            if ((param[0] == '0') && (param[1] == 'x')) {
              num = strtoul(&param[2], NULL, 16);
            } else {
              num = strtoul(param, NULL, 10);
            }

            if (num > 64) {
              num = 64;
            }
          }

          uint32_t devid_mode;

          devid_mode = (0 == strcmp("read10", command)) ? I2C_10BIT_INDICATOR : 0;

          DRV_I2C_Status ret;
          uint8_t reg[3];
          uint32_t dev_id = (uint32_t)(device_id | devid_mode);

          uint32_t i;
          for (i = 0; i < (uint32_t)addrLen; i++) {
            reg[i] = (uint8_t)(address >> (8 * (addrLen - i - 1)));
          }

          i2c_err = false;
          ret = local_transmit(i2c[i2c_bus_id], dev_id, reg, (uint32_t)addrLen, true);
          printf("%s(), ret = %ld\n\r", local_transmit_str, (int32_t)ret);
          if (ret == DRV_I2C_OK) {
            if (opmode == OPMODE_NONBLOCK
                    ? (i2c_sem && (osSemaphoreAcquire(i2c_sem, osWaitForever) == osOK))
                    : true) {
              if (opmode == OPMODE_NONBLOCK) {
                if (i2c_err) {
                  printf("i2c_drv TX error, event = %lu\n\r", i2c_event[i2c_bus_id]);
                  return ret_val;
                } else {
                  printf("i2c_drv TX done, cnt = %ld\n\r", DRV_I2C_GetDataCount(i2c[i2c_bus_id]));
                }
              } else {
                printf("i2c_drv TX done, cnt = %ld\n\r", DRV_I2C_GetDataCount(i2c[i2c_bus_id]));
              }
              ret = local_receive(i2c[i2c_bus_id], dev_id, data, num, false);
              printf("%s(), ret = %ld\n\r", local_receive_str, (int32_t)ret);
              if (ret == DRV_I2C_OK) {
                if (opmode == OPMODE_NONBLOCK
                        ? (i2c_sem && (osSemaphoreAcquire(i2c_sem, osWaitForever) == osOK))
                        : true) {
                  if (opmode == OPMODE_NONBLOCK) {
                    if (i2c_err) {
                      printf("i2c_drv RX error, event = %lu\n\r", i2c_event[i2c_bus_id]);
                      return ret_val;
                    } else {
                      printf("i2c_drv RX done, cnt = %ld\n\r",
                             DRV_I2C_GetDataCount(i2c[i2c_bus_id]));
                    }
                  } else {
                    printf("i2c_drv RX done, cnt = %ld\n\r", DRV_I2C_GetDataCount(i2c[i2c_bus_id]));
                  }
                  for (n = 0; n < num; n++) {
                    if ((n & 0xf) == 0) {
                      printf("%04lx:", address + n);
                    }

                    printf(" %02x", data[n]);
                    if ((n & 0xf) == 0xf) {
                      printf("\n\r");
                    }
                  }

                  if ((n & 0xf) != 0) {
                    printf("\n\r");
                  }
                }
              }
            }
          }
        } else {
          printf(
              "i2c read/read10 device_id address addrlen [num of objects] - read from I2C "
              "device\n\r");
        }
      } else if (0 == strcmp("write", command) || 0 == strcmp("write10", command)) {
        if (argc > 5) {
          num = 1;
          param = par2;
          if ((param[0] == '0') && (param[1] == 'x')) {
            device_id = (uint16_t)strtoul(&param[2], NULL, 16);
          } else {
            device_id = (uint16_t)strtoul(param, NULL, 10);
          }

          param = par3;
          if ((param[0] == '0') && (param[1] == 'x')) {
            address = strtoul(&param[2], NULL, 16);
          } else {
            address = strtoul(param, NULL, 10);
          }

          param = par4;
          addrLen = (uint8_t)strtoul(param, NULL, 10);
          if (addrLen > 3) {
            addrLen = 3;
          }

          param = par5;
          if ((param[0] == '0') && (param[1] == 'x')) {
            value = (uint8_t)strtoul(&param[2], NULL, 16);
          } else {
            value = (uint8_t)strtoul(param, NULL, 10);
          }

          if (argc > 6) {
            param = par6;
            if ((param[0] == '0') && (param[1] == 'x')) {
              num = strtoul(&param[2], NULL, 16);
            } else {
              num = strtoul(param, NULL, 10);
            }

            if (num > 64) {
              num = 64;
            }
          }

          for (n = 0; n < num; n++) {
            data[n] = value;
          }

          uint32_t devid_mode;

          devid_mode = (0 == strcmp("write10", command)) ? I2C_10BIT_INDICATOR : 0;

          DRV_I2C_Status ret;
          uint8_t reg_data[128];
          uint32_t dev_id = (uint32_t)(device_id | devid_mode);

          uint32_t i;
          for (i = 0; i < (uint32_t)addrLen; i++) {
            reg_data[i] = (uint8_t)(address >> (8 * (addrLen - i - 1)));
          }

          for (i = 0; i < num; i++) {
            reg_data[addrLen + i] = data[i];
          }

          i2c_err = false;
          ret = local_transmit(i2c[i2c_bus_id], dev_id, reg_data, (uint32_t)addrLen + num, false);
          printf("%s(), ret = %ld\n\r", local_transmit_str, (int32_t)ret);
          if (ret == DRV_I2C_OK) {
            if (opmode == OPMODE_NONBLOCK
                    ? (i2c_sem && (osSemaphoreAcquire(i2c_sem, osWaitForever) == osOK))
                    : true) {
              if (opmode == OPMODE_NONBLOCK) {
                if (i2c_err) {
                  printf("i2c_drv TX error, event = %lu\n\r", i2c_event[i2c_bus_id]);
                } else {
                  printf("i2c_drv TX done, cnt = %ld\n\r", DRV_I2C_GetDataCount(i2c[i2c_bus_id]));
                }
              } else {
                printf("i2c_drv TX done, cnt = %ld\n\r", DRV_I2C_GetDataCount(i2c[i2c_bus_id]));
              }
            }
          }
        } else {
          printf(
              "i2c write/write10 device_id address addrlen value [count] - write to I2C "
              "device\n\r");
        }
      } else if (0 == strcmp("init", command)) {
        int32_t ret;
        Drv_I2C_EventCallback evt_cb;

        evt_cb.user_data = (void*)i2c_bus_id;
        evt_cb.event_callback = i2c_evtcb;
        ret = DRV_I2C_Initialize(i2c_bus_id, &evt_cb, &i2c[i2c_bus_id]);
        printf("DRV_I2C_Initialize(), ret = %ld\n\r", ret);
      } else if (0 == strcmp("uninit", command)) {
        int32_t ret;
        ret = DRV_I2C_Uninitialize(i2c_bus_id);
        printf("DRV_I2C_Uninitialize(), ret = %ld\n\r", ret);
      } else if (0 == strcmp("gtcnt", command)) {
        int32_t cnt;

        cnt = DRV_I2C_GetDataCount(i2c[i2c_bus_id]);
        printf("DRV_I2C_GetDataCount(), cnt = %ld\n\r", cnt);
      } else if (0 == strcmp("pwrctrl", command)) {
        I2C_PowerMode pwrmode = (uint32_t)strtol(par2, NULL, 10);
        if (pwrmode <= I2C_POWER_FULL) {
          int32_t ret;

          ret = DRV_I2C_PowerControl(i2c[i2c_bus_id], pwrmode);
          printf("DRV_I2C_PowerControl(), ret = %ld\n\r", ret);
        } else {
          printf("Incorrect power mode %ld\n\r", (uint32_t)pwrmode);
          ret_val = -1;
        }
      } else if (0 == strcmp("speed", command)) {
        if (argc == 3) {
          int32_t ret;
          uint32_t speed = (uint32_t)strtoul(par2, NULL, 10);

          ret = DRV_I2C_SetSpeed(i2c[i2c_bus_id], speed);
          printf("DRV_I2C_SetSpeed(), ret = %ld\n\r", ret);
        } else {
          printf("i2c <ctrl> <ctrlvalue> <arg> - set control value and argument\n\r");
          ret_val = -1;
        }
      } else {
        print_i2c_usage();
        ret_val = -1;
      }
    }
  }

  return ret_val;
}
