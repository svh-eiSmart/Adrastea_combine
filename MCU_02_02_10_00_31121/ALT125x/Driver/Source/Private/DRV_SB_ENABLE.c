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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "DRV_COMMON.h"
#include "DRV_SB_ENABLE.h"
#include "DRV_FLASH.h"
#include "sha256.h"

/****************************************************************************
 * Definitions
 ****************************************************************************/
#define OTP_WORD_SIZE 32

#define OTP_MCU_HPUK3_1_BIT_OFFSET 31744
#define OTP_MCU_HPUK3_1_SIZE_IN_BITS 256
#define OTP_MCU_HPUK3_1_LOCK_BIT (32696 + 3)  // SpareLock(3) => 32696+3

#define OTP_MCU_HPUK3_2_BIT_OFFSET 32000
#define OTP_MCU_HPUK3_2_SIZE_IN_BITS 256
#define OTP_MCU_HPUK3_2_LOCK_BIT (32696 + 3)  // SpareLock(3) => 32696+3

#define OTP_MCU_SECURE_BOOT_ENABLE_BIT_OFFSET 32256
#define OTP_MCU_SECURE_BOOT_ENABLE_SIZE_IN_BITS 2
#define OTP_MCU_SECURE_BOOT_ENABLE_LOCK_BIT (32696 + 3)  // SpareLock(3)

#define OTP_MCU_DH0_SECURE_BOOT_ENABLE_BIT_OFFSET 32258
#define OTP_MCU_DH0_SECURE_BOOT_ENABLE_SIZE_IN_BITS 2
#define OTP_MCU_DH0_SECURE_BOOT_ENABLE_LOCK_BIT (32696 + 3)  // SpareLock(3)

#define OTP_ENABLE_WRITE 0xCAFECAFE
#define OTP_DISABLE_WRITE 0xDEADBEAF

#define LOCAL_BUFFER_SIZE 120

#define SB_PUK2_SIZE (4 + 3072 / 8)  // 388 bytes
#define SB_PUBLIC_KEY_SIZE (68)      // 544 bits
#define SB_CRC32_SIZE (4)

#define BASE_ADDRESS_OTP 0x0D000800
#define BASE_ADDRESS_OTP_RAM_PMP 0x0D030000

#define OTP_OTP_SECURE_CHANNEL_OPEN_MCU (BASE_ADDRESS_OTP + 0x7C)
#define OTP_OTP_SECURE_CHANNEL_OPEN_PMP (BASE_ADDRESS_OTP + 0x80)
#define OTP_OTP_SECURE_CHANNEL_OPEN_MODEM (BASE_ADDRESS_OTP + 0x84)
#define OTP_OTP_SECURE_CHANNEL_OPEN_PMPA (BASE_ADDRESS_OTP + 0x88)
#define OTP_OTP_SECURE_CHANNEL_OPEN_PMPS (BASE_ADDRESS_OTP + 0x8C)

#define BASE_ADDRESS_MFV 0x0C110080
#define MFV_MFV (BASE_ADDRESS_MFV + 0x00)
#define BASE_ADDRESS_CHIP 0x0D000900

#define CHIP_CHIP_VER_VERSION (BASE_ADDRESS_CHIP + 0x000)
#define CHIP_CHIP_VER_VERSION_PRODUCT_NUM_MSK (0xFFFF0000)
#define CHIP_CHIP_VER_VERSION_PRODUCT_NUM_POS (16)
#define CHIP_CHIP_VER_VERSION_SW_VERSION_MSK (0x0FF00)
#define CHIP_CHIP_VER_VERSION_SW_VERSION_POS (8)
#define CHIP_CHIP_VER_VERSION_HW_VERSION_MSK (0x000FF)
#define CHIP_CHIP_VER_VERSION_HW_VERSION_POS (0)

#define LOCAL_BUFFER_SIZE 120

#ifdef OTP_DBG
#define OTP_DBG_PRINT(fmt, args...) \
  { printf("DBG " fmt, ##args); }
#else
#define OTP_DBG_PRINT(fmt, args...)
#endif
/****************************************************************************
 * Private Types
 ****************************************************************************/
enum ALT1250_STATUS {
  OTP_OK = 0,
  OTP_ACCESS_ERROR = 1,
  OTP_ALREADY_LOCKED = 2,
  OTP_IMEI_UNFILLED = 3,
  OTP_MK_UNFILLED = 4,
  OTP_ALREADY_WRITTEN = 5,
  OTP_ILLEGAL_ORDER = 6,
  OTP_ILLEGAL_VALUE = 7,
  OTP_NOK,
  OTP_MEMORY_ERROR,
  OTP_MISSED_IMEI,
  OTP_MISSED_MK,
  OTP_UNLOCKED,
  OTP_LOCKED,
  OTP_SYNTAX_ERR,
  OTP_NOT_IMPLEMENTED
};

enum ALT1250_OTP_FIELDS {
  OTP_MCU_HPUK3_1,
  OTP_MCU_HPUK3_2,
  OTP_MCU_SECURE_BOOT_ENABLE,
  OTP_MCU_DH0_SECURE_BOOT_ENABLE
};

typedef enum {
  ALT1250_CHIP_STEP_ID_UNKNOWN = 0,
  ALT1250_CHIP_STEP_ID_A0 = 1,  // 1 - 1250
  ALT1250_CHIP_STEP_ID_B0 = 2,  // 2 - 1250
  ALT1250_CHIP_STEP_ID_C0 = 3,  // 3 - 1250B
  ALT1250_CHIP_STEP_ID_D0 = 4,  // 4 - 1250B
  ALT1250_CHIP_STEP_ID_E0 = 5,  // 5 - 1250B
  ALT1250_CHIP_STEP_ID_TOTAL    // 5
} ALT1250_Chip_IDs_t;

typedef struct {
  char name[50];
  enum ALT1250_OTP_FIELDS type;
  int base_addr_bit;
  int size_bit;
  int locking_val;
} otp_field;

const otp_field otp_struct_lut[] = {
    {"MCU hush of public key 1 - puk3_1", OTP_MCU_HPUK3_1, OTP_MCU_HPUK3_1_BIT_OFFSET,
     OTP_MCU_HPUK3_1_SIZE_IN_BITS, OTP_MCU_HPUK3_1_LOCK_BIT},
    {"MCU hush of public key 2 - puk3_2", OTP_MCU_HPUK3_2, OTP_MCU_HPUK3_2_BIT_OFFSET,
     OTP_MCU_HPUK3_2_SIZE_IN_BITS, OTP_MCU_HPUK3_2_LOCK_BIT},
    {"MCU Secure boot enable", OTP_MCU_SECURE_BOOT_ENABLE, OTP_MCU_SECURE_BOOT_ENABLE_BIT_OFFSET,
     OTP_MCU_SECURE_BOOT_ENABLE_SIZE_IN_BITS, OTP_MCU_SECURE_BOOT_ENABLE_LOCK_BIT},
    {"MCU DH0 Secure boot enable", OTP_MCU_DH0_SECURE_BOOT_ENABLE,
     OTP_MCU_DH0_SECURE_BOOT_ENABLE_BIT_OFFSET, OTP_MCU_DH0_SECURE_BOOT_ENABLE_SIZE_IN_BITS,
     OTP_MCU_DH0_SECURE_BOOT_ENABLE_LOCK_BIT},
};

typedef struct {
  uint32_t magicRescue;
  uint32_t destInRam;
  uint32_t jmpAddr;
  uint32_t wordOffset0;
  uint32_t wordLength0;
  uint32_t wordOffset1;
  uint32_t wordLength1;
  uint32_t rcPatch[21];
  uint32_t patch_vesion;
} rc_patch_struct_e;

typedef struct {
  uint32_t SourceAndSize[1];
  uint32_t destInRam[1];
  uint32_t SfInitPatch[15];
  uint32_t BootRomBypassBits[1];
} sf_init_patch_struct_e;

/****************************************************************************
 * Private Data
 ****************************************************************************/
#define CONFIG_OTP_CHIP_REV 144  // first bit of otp chip revision (ASCII6 in reverse order)

static uint32_t system_chip_revision = ALT1250_CHIP_STEP_ID_UNKNOWN;
static char system_chip_revision_str[50] = "\0";

/****************************************************************************
 * Function prototypes
 ****************************************************************************/

enum ALT1250_STATUS drv_sb_enable_setotp(int index, uint32_t *buf);
enum ALT1250_STATUS drv_sb_enable_getotp(int index, uint32_t *buf);

int otpBitsRead(uint32_t bitOffset, int readBitCount, uint32_t *pDest);
int otpWordRead(uint32_t otpWordAddress, uint32_t *pDest);
int otp_burn_bit(int bit_addr);

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static void hex_dump(uint8_t *buf, int len) {
  int i;

  for (i = 0; i < len; i++) {
    if ((i % 16) == 0) printf("%s%08lx: ", i ? "\n" : "", (uint32_t)&buf[i]);
    printf("%02x ", buf[i]);
  }
  printf("\n");
}

#define OK_BUF_SIZE 40
enum AL1250_SHOW_INFO { OTP_SHOW_NORMAL, OTP_SHOW_NUMBER, OTP_SHOW_STR };

void otp_get_error(enum ALT1250_STATUS status, char *errorName, enum AL1250_SHOW_INFO show) {
  if (show == OTP_SHOW_NORMAL) {
    if (status == OTP_OK)
      strncpy(errorName, "\r\nOK\r\n", OK_BUF_SIZE);
    else
      strncpy(errorName, "\r\nERROR\r\n", OK_BUF_SIZE);
    errorName[OK_BUF_SIZE] = 0;
  } else {
    switch (status) {
      case OTP_OK:
        if (show == OTP_SHOW_STR)
          strncpy(errorName, "\r\nOK\r\n", OK_BUF_SIZE);
        else
          snprintf(errorName, OK_BUF_SIZE, "\r\n%%OTPCMD: error=%d\r\nOK\r\n", (int)status);
        break;
      case OTP_NOK:
        if (show == OTP_SHOW_STR)
          strncpy(errorName, "\r\nERROR\r\n", OK_BUF_SIZE);
        else
          snprintf(errorName, OK_BUF_SIZE, "\r\n%%OTPCMD: error=%d\r\nOK\r\n", (int)status);
        break;
      case OTP_MEMORY_ERROR:
        if (show == OTP_SHOW_STR)
          strncpy(errorName, "\r\nNO MEMORY\r\n", OK_BUF_SIZE);
        else
          snprintf(errorName, OK_BUF_SIZE, "\r\n%%OTPCMD: error=%d\r\nOK\r\n", (int)status);
        break;
      case OTP_ACCESS_ERROR:
        if (show == OTP_SHOW_STR)
          strncpy(errorName, "\r\nACCESS ERROR\r\n", OK_BUF_SIZE);
        else
          snprintf(errorName, OK_BUF_SIZE, "\r\n%%OTPCMD: error=%d\r\nOK\r\n", (int)status);
        break;
      case OTP_ALREADY_LOCKED:
        if (show == OTP_SHOW_STR)
          strncpy(errorName, "\r\nALREADY LOCKED\r\n", OK_BUF_SIZE);
        else
          snprintf(errorName, OK_BUF_SIZE, "\r\n%%OTPCMD: error=%d\r\nOK\r\n", (int)status);
        break;
      case OTP_MISSED_IMEI:
        if (show == OTP_SHOW_STR)
          strncpy(errorName, "\r\nIMEI MISSING\r\n", OK_BUF_SIZE);
        else
          snprintf(errorName, OK_BUF_SIZE, "\r\n%%OTPCMD: error=%d\r\nOK\r\n", (int)status);
        break;
      case OTP_MISSED_MK:
        if (show == OTP_SHOW_STR)
          strncpy(errorName, "\r\nMK MISSING\r\n", OK_BUF_SIZE);
        else
          snprintf(errorName, OK_BUF_SIZE, "\r\n%%OTPCMD: error=%d\r\nOK\r\n", (int)status);
        break;
      case OTP_ALREADY_WRITTEN:
        if (show == OTP_SHOW_STR)
          strncpy(errorName, "\r\n%%OTPCMD: ALREADY WRITTEN\r\n", OK_BUF_SIZE);
        else
          snprintf(errorName, OK_BUF_SIZE, "\r\nerror=%d\r\n", (int)status);
        break;
      case OTP_ILLEGAL_ORDER:
        strncpy(errorName, "\r\n%%OTPCMD: ILLEGAL ORDER\r\nOK\r\n", OK_BUF_SIZE);
        if (show == OTP_SHOW_STR)
          strncpy(errorName, "\r\nOK\r\n", OK_BUF_SIZE);
        else
          snprintf(errorName, OK_BUF_SIZE, "\r\nerror=%d\r\n", (int)status);
        break;
      case OTP_ILLEGAL_VALUE:
        if (show == OTP_SHOW_STR)
          strncpy(errorName, "\r\n%%OTPCMD: ILLEGAL VALUE\r\nOK\r\n", OK_BUF_SIZE);
        else
          snprintf(errorName, OK_BUF_SIZE, "\r\nerror=%d\r\n", (int)status);
        break;
      case OTP_UNLOCKED:
        if (show == OTP_SHOW_STR)
          strncpy(errorName, "\r\n%%OTPCMD: UNLOCKED\r\nOK\r\n", OK_BUF_SIZE);
        else
          snprintf(errorName, OK_BUF_SIZE, "\r\nerror=%d\r\n", (int)status);
        break;
      case OTP_LOCKED:
        if (show == OTP_SHOW_STR)
          strncpy(errorName, "\r\n%%OTPCMD: LOCKED\r\nOK\r\n", OK_BUF_SIZE);
        else
          snprintf(errorName, OK_BUF_SIZE, "\r\nerror=%d\r\n", (int)status);
        break;
      case OTP_SYNTAX_ERR:
        if (show == OTP_SHOW_STR)
          strncpy(errorName, "\r\n%%OTPCMD: SYNTAX ERROR\r\nOK\r\n", OK_BUF_SIZE);
        else
          snprintf(errorName, OK_BUF_SIZE, "\r\nerror=%d\r\n", (int)status);
        break;
      case OTP_NOT_IMPLEMENTED:
        if (show == OTP_SHOW_STR)
          strncpy(errorName, "\r\n%%OTPCMD: NOT IMPLEMENTED\r\nOK\r\n", OK_BUF_SIZE);
        else
          snprintf(errorName, OK_BUF_SIZE, "\r\nerror=%d\r\n", (int)status);
        break;
      default:
        if (show == OTP_SHOW_STR)
          strncpy(errorName, "\r\n%%OTPCMD: UNKNOWN ERROR\r\nOK\r\n", OK_BUF_SIZE);
        else
          snprintf(errorName, OK_BUF_SIZE, "\r\nerror=%d\r\n", (int)status);
        break;
    }
  }
  errorName[OK_BUF_SIZE] = 0;
}

static enum ALT1250_STATUS otp_sb_enable_check_error(int idx, enum ALT1250_STATUS status) {
  char error[OK_BUF_SIZE + 1] = {0};

  if (status != OTP_OK) {
    otp_get_error(status, error, OTP_SHOW_STR);
    printf("ERROR: OTP field '%s' - %s\n", otp_struct_lut[idx].name, error);
  }
  return status;
}

#define ENDIAN_SWAP_32BYTES(x) \
  ((((x)&0xff) << 24) | (((x)&0xff00) << 8) | (((x)&0xff0000) >> 8) | (((x)&0xff000000) >> 24))

#define ENDIAN_RESVERSE_32BYTES(x) \
  ((((x)&0xff000000) >> 24) | (((x)&0xff0000) >> 8) | (((x)&0xff00) << 8) | (((x)&0xff) << 24))

#define TRAN(x) ENDIAN_SWAP_32BYTES(x)

static const unsigned long crc32_tab[] = {
    TRAN(0x00000000L), TRAN(0x77073096L), TRAN(0xee0e612cL), TRAN(0x990951baL), TRAN(0x076dc419L),
    TRAN(0x706af48fL), TRAN(0xe963a535L), TRAN(0x9e6495a3L), TRAN(0x0edb8832L), TRAN(0x79dcb8a4L),
    TRAN(0xe0d5e91eL), TRAN(0x97d2d988L), TRAN(0x09b64c2bL), TRAN(0x7eb17cbdL), TRAN(0xe7b82d07L),
    TRAN(0x90bf1d91L), TRAN(0x1db71064L), TRAN(0x6ab020f2L), TRAN(0xf3b97148L), TRAN(0x84be41deL),
    TRAN(0x1adad47dL), TRAN(0x6ddde4ebL), TRAN(0xf4d4b551L), TRAN(0x83d385c7L), TRAN(0x136c9856L),
    TRAN(0x646ba8c0L), TRAN(0xfd62f97aL), TRAN(0x8a65c9ecL), TRAN(0x14015c4fL), TRAN(0x63066cd9L),
    TRAN(0xfa0f3d63L), TRAN(0x8d080df5L), TRAN(0x3b6e20c8L), TRAN(0x4c69105eL), TRAN(0xd56041e4L),
    TRAN(0xa2677172L), TRAN(0x3c03e4d1L), TRAN(0x4b04d447L), TRAN(0xd20d85fdL), TRAN(0xa50ab56bL),
    TRAN(0x35b5a8faL), TRAN(0x42b2986cL), TRAN(0xdbbbc9d6L), TRAN(0xacbcf940L), TRAN(0x32d86ce3L),
    TRAN(0x45df5c75L), TRAN(0xdcd60dcfL), TRAN(0xabd13d59L), TRAN(0x26d930acL), TRAN(0x51de003aL),
    TRAN(0xc8d75180L), TRAN(0xbfd06116L), TRAN(0x21b4f4b5L), TRAN(0x56b3c423L), TRAN(0xcfba9599L),
    TRAN(0xb8bda50fL), TRAN(0x2802b89eL), TRAN(0x5f058808L), TRAN(0xc60cd9b2L), TRAN(0xb10be924L),
    TRAN(0x2f6f7c87L), TRAN(0x58684c11L), TRAN(0xc1611dabL), TRAN(0xb6662d3dL), TRAN(0x76dc4190L),
    TRAN(0x01db7106L), TRAN(0x98d220bcL), TRAN(0xefd5102aL), TRAN(0x71b18589L), TRAN(0x06b6b51fL),
    TRAN(0x9fbfe4a5L), TRAN(0xe8b8d433L), TRAN(0x7807c9a2L), TRAN(0x0f00f934L), TRAN(0x9609a88eL),
    TRAN(0xe10e9818L), TRAN(0x7f6a0dbbL), TRAN(0x086d3d2dL), TRAN(0x91646c97L), TRAN(0xe6635c01L),
    TRAN(0x6b6b51f4L), TRAN(0x1c6c6162L), TRAN(0x856530d8L), TRAN(0xf262004eL), TRAN(0x6c0695edL),
    TRAN(0x1b01a57bL), TRAN(0x8208f4c1L), TRAN(0xf50fc457L), TRAN(0x65b0d9c6L), TRAN(0x12b7e950L),
    TRAN(0x8bbeb8eaL), TRAN(0xfcb9887cL), TRAN(0x62dd1ddfL), TRAN(0x15da2d49L), TRAN(0x8cd37cf3L),
    TRAN(0xfbd44c65L), TRAN(0x4db26158L), TRAN(0x3ab551ceL), TRAN(0xa3bc0074L), TRAN(0xd4bb30e2L),
    TRAN(0x4adfa541L), TRAN(0x3dd895d7L), TRAN(0xa4d1c46dL), TRAN(0xd3d6f4fbL), TRAN(0x4369e96aL),
    TRAN(0x346ed9fcL), TRAN(0xad678846L), TRAN(0xda60b8d0L), TRAN(0x44042d73L), TRAN(0x33031de5L),
    TRAN(0xaa0a4c5fL), TRAN(0xdd0d7cc9L), TRAN(0x5005713cL), TRAN(0x270241aaL), TRAN(0xbe0b1010L),
    TRAN(0xc90c2086L), TRAN(0x5768b525L), TRAN(0x206f85b3L), TRAN(0xb966d409L), TRAN(0xce61e49fL),
    TRAN(0x5edef90eL), TRAN(0x29d9c998L), TRAN(0xb0d09822L), TRAN(0xc7d7a8b4L), TRAN(0x59b33d17L),
    TRAN(0x2eb40d81L), TRAN(0xb7bd5c3bL), TRAN(0xc0ba6cadL), TRAN(0xedb88320L), TRAN(0x9abfb3b6L),
    TRAN(0x03b6e20cL), TRAN(0x74b1d29aL), TRAN(0xead54739L), TRAN(0x9dd277afL), TRAN(0x04db2615L),
    TRAN(0x73dc1683L), TRAN(0xe3630b12L), TRAN(0x94643b84L), TRAN(0x0d6d6a3eL), TRAN(0x7a6a5aa8L),
    TRAN(0xe40ecf0bL), TRAN(0x9309ff9dL), TRAN(0x0a00ae27L), TRAN(0x7d079eb1L), TRAN(0xf00f9344L),
    TRAN(0x8708a3d2L), TRAN(0x1e01f268L), TRAN(0x6906c2feL), TRAN(0xf762575dL), TRAN(0x806567cbL),
    TRAN(0x196c3671L), TRAN(0x6e6b06e7L), TRAN(0xfed41b76L), TRAN(0x89d32be0L), TRAN(0x10da7a5aL),
    TRAN(0x67dd4accL), TRAN(0xf9b9df6fL), TRAN(0x8ebeeff9L), TRAN(0x17b7be43L), TRAN(0x60b08ed5L),
    TRAN(0xd6d6a3e8L), TRAN(0xa1d1937eL), TRAN(0x38d8c2c4L), TRAN(0x4fdff252L), TRAN(0xd1bb67f1L),
    TRAN(0xa6bc5767L), TRAN(0x3fb506ddL), TRAN(0x48b2364bL), TRAN(0xd80d2bdaL), TRAN(0xaf0a1b4cL),
    TRAN(0x36034af6L), TRAN(0x41047a60L), TRAN(0xdf60efc3L), TRAN(0xa867df55L), TRAN(0x316e8eefL),
    TRAN(0x4669be79L), TRAN(0xcb61b38cL), TRAN(0xbc66831aL), TRAN(0x256fd2a0L), TRAN(0x5268e236L),
    TRAN(0xcc0c7795L), TRAN(0xbb0b4703L), TRAN(0x220216b9L), TRAN(0x5505262fL), TRAN(0xc5ba3bbeL),
    TRAN(0xb2bd0b28L), TRAN(0x2bb45a92L), TRAN(0x5cb36a04L), TRAN(0xc2d7ffa7L), TRAN(0xb5d0cf31L),
    TRAN(0x2cd99e8bL), TRAN(0x5bdeae1dL), TRAN(0x9b64c2b0L), TRAN(0xec63f226L), TRAN(0x756aa39cL),
    TRAN(0x026d930aL), TRAN(0x9c0906a9L), TRAN(0xeb0e363fL), TRAN(0x72076785L), TRAN(0x05005713L),
    TRAN(0x95bf4a82L), TRAN(0xe2b87a14L), TRAN(0x7bb12baeL), TRAN(0x0cb61b38L), TRAN(0x92d28e9bL),
    TRAN(0xe5d5be0dL), TRAN(0x7cdcefb7L), TRAN(0x0bdbdf21L), TRAN(0x86d3d2d4L), TRAN(0xf1d4e242L),
    TRAN(0x68ddb3f8L), TRAN(0x1fda836eL), TRAN(0x81be16cdL), TRAN(0xf6b9265bL), TRAN(0x6fb077e1L),
    TRAN(0x18b74777L), TRAN(0x88085ae6L), TRAN(0xff0f6a70L), TRAN(0x66063bcaL), TRAN(0x11010b5cL),
    TRAN(0x8f659effL), TRAN(0xf862ae69L), TRAN(0x616bffd3L), TRAN(0x166ccf45L), TRAN(0xa00ae278L),
    TRAN(0xd70dd2eeL), TRAN(0x4e048354L), TRAN(0x3903b3c2L), TRAN(0xa7672661L), TRAN(0xd06016f7L),
    TRAN(0x4969474dL), TRAN(0x3e6e77dbL), TRAN(0xaed16a4aL), TRAN(0xd9d65adcL), TRAN(0x40df0b66L),
    TRAN(0x37d83bf0L), TRAN(0xa9bcae53L), TRAN(0xdebb9ec5L), TRAN(0x47b2cf7fL), TRAN(0x30b5ffe9L),
    TRAN(0xbdbdf21cL), TRAN(0xcabac28aL), TRAN(0x53b39330L), TRAN(0x24b4a3a6L), TRAN(0xbad03605L),
    TRAN(0xcdd70693L), TRAN(0x54de5729L), TRAN(0x23d967bfL), TRAN(0xb3667a2eL), TRAN(0xc4614ab8L),
    TRAN(0x5d681b02L), TRAN(0x2a6f2b94L), TRAN(0xb40bbe37L), TRAN(0xc30c8ea1L), TRAN(0x5a05df1bL),
    TRAN(0x2d02ef8dL)};

#define CRC32_CALC_WORD_SZ (4)
uint32_t crc32_calc_by_word(const uint8_t *buf, uint32_t len) {
  uint32_t crc = 0xffffffff;
  const uint32_t *crc_tab = crc32_tab;
  const uint32_t *p = (const uint32_t *)buf;
  uint32_t i, ret_val = 0;

  /* length must be a multiple of 4 */
  if (len == 0 || len % CRC32_CALC_WORD_SZ != 0) {
    printf("Error: wrong CRC parameters!\n");
    return 0;
  }

  crc = ENDIAN_SWAP_32BYTES(crc);
  len = len / CRC32_CALC_WORD_SZ;

  while (len) {
    crc ^= *p++;
    len--;

    for (i = 0; i < CRC32_CALC_WORD_SZ; i++) {
      crc = crc_tab[((crc >> 24)) & 0xff] ^ (crc << 8);
    }
  }

  ret_val = ENDIAN_RESVERSE_32BYTES(crc);
  return (ret_val ^ 0xffffffff);
}

uint32_t crc32(const char *p, uint32_t len) { return crc32_calc_by_word((const uint8_t *)p, len); }

void calc_sha256_sw(uint8_t *start_addr, int data_len, uint8_t *out_addr) {
  mbedtls_sha256(start_addr, data_len, out_addr, 0);
}

void calc_sha256(uint8_t *start_addr, int data_len, uint8_t *out_addr) {
  calc_sha256_sw(start_addr, data_len, out_addr);
}

static int otp_write_enabled(void) { return REGISTER(OTP_OTP_SECURE_CHANNEL_OPEN_MCU); }

static int alt1250_is_otp_locked(int index) {
  uint32_t otp_read;

  OTP_DBG_PRINT("%s - %d\n", __FUNCTION__, __LINE__);

  if (otp_struct_lut[index].locking_val < 0) {
    // not locked
    return 0;
  }

  otpBitsRead(otp_struct_lut[index].locking_val, 1, &otp_read);
  return otp_read ? 1 : 0;
}

static enum ALT1250_STATUS otp_field_get(int index, uint32_t *buf) {
  enum ALT1250_STATUS status = OTP_OK;

  OTP_DBG_PRINT("%s - %d\n", __FUNCTION__, __LINE__);
  OTP_DBG_PRINT("otp_field_get: base: %d  size in bit: %d \n", otp_struct_lut[index].base_addr_bit,
                otp_struct_lut[index].size_bit);
  if (otpBitsRead(otp_struct_lut[index].base_addr_bit, otp_struct_lut[index].size_bit, buf) < 0)
    status = OTP_ACCESS_ERROR;

  return status;
}

enum ALT1250_STATUS drv_sb_enable_getotp(int index, uint32_t *buf) {
  OTP_DBG_PRINT("%s - %d - index: %u\n", __FUNCTION__, __LINE__, index);
  return otp_field_get(index, buf);
}

static enum ALT1250_STATUS otp_verify_field_content(int index) {
  uint32_t *buf;
  int size_bit, size_byte;
  int buff_bit_run = 0, word_run = 0;
  enum ALT1250_STATUS status = OTP_OK;

  OTP_DBG_PRINT("%s - %d\n", __FUNCTION__, __LINE__);

  size_bit = otp_struct_lut[index].size_bit;
  size_byte = size_bit >> 3;
  if (!size_byte) size_byte = 1;

  buf = malloc(size_byte < 4 ? 4 : size_byte + 4);
  if (!buf) return OTP_MEMORY_ERROR;

  if (otp_field_get(index, buf) == OTP_ACCESS_ERROR) {
    free(buf);
    return OTP_ACCESS_ERROR;
  }

  while (size_bit) {
    if (buff_bit_run == OTP_WORD_SIZE) {
      word_run++;
      buff_bit_run = 0;
    }
#ifndef OTP_DEBUG_SIMU
    if (buf[word_run] & (1 << buff_bit_run)) {
      status = OTP_ALREADY_WRITTEN;
      break;
    }
#endif
    buff_bit_run++;
    size_bit--;
  }
  free(buf);
  return status;
}

int otp_burn_word(int addr, uint32_t value) {
  // Memory mapped OTP burn
  uint32_t read_word;
  OTP_DBG_PRINT("addr: %0x\n", addr);
#ifndef OTP_DEBUG_SIMU
  OTP_DBG_PRINT("BURNT:: addr: %0x -> val: %0lx\n", addr << 2, value);
  REGISTER(BASE_ADDRESS_OTP_RAM_PMP + (addr << 2)) = value;
#else
  OTP_DBG_PRINT("BURN SIMU: addr: %0x -> val: %0lx\n", addr << 2, value);
#endif
  // Test that burn was done correctly
  otpWordRead(addr, &read_word);
  OTP_DBG_PRINT("read_word: %0lx\n", read_word);
#ifndef OTP_DEBUG_SIMU
  if ((read_word & value) == value) {
    // burn was successful
    return 0;
  } else {
    return -1;
  }
#else
  return 0;
#endif
}

int otp_burn_bit(int bit_addr) {
  int word_addr;
  int bit_offset;
  uint32_t word_value;

  if (bit_addr < 0) {
    // illegal
    printf("illegel bit address %d\n", bit_addr);
    return -1;
  }

  OTP_DBG_PRINT("bit_addr is : %d\n", bit_addr);

  // Calculate work addr
  word_addr = (bit_addr >> 5);

  // Calculate bit offset
  bit_offset = bit_addr & ((1 << 5) - 1);
  OTP_DBG_PRINT("bit offset: %x\n", bit_offset);

  // Calculate word mask to burn
  word_value = (1 << bit_offset);

  OTP_DBG_PRINT("word_addr: %x  word_value: %lx\n", word_addr, word_value);
  return otp_burn_word(word_addr, word_value);
}

static enum ALT1250_STATUS otp_field_write(int index, uint32_t *buf) {
  int buff_bit_run = 0, word_run = 0, retry = 0;
  enum ALT1250_STATUS status = OTP_OK;
  int size = otp_struct_lut[index].size_bit;

  OTP_DBG_PRINT("%s - %d\n", __FUNCTION__, __LINE__);
  OTP_DBG_PRINT("otp_field_burn: start of buff: %0lX\n", buf[0]);

  REGISTER(OTP_OTP_SECURE_CHANNEL_OPEN_MCU) = OTP_ENABLE_WRITE;

  while (otp_write_enabled() != 1) {
    if (retry > 1000) {
      OTP_DBG_PRINT("otp_write can be enabled!\n");
      return OTP_ACCESS_ERROR;
    }
    retry++;
  }
  OTP_DBG_PRINT("otp_write retries: %d\n", retry);

  /* Notify if OTP is locked */
  if (alt1250_is_otp_locked(index)) return OTP_LOCKED;

  /* Notify if OTP is dirty */
  if ((status = otp_verify_field_content(index)) != OTP_OK) return status;

  while (size) {
    if (buff_bit_run == OTP_WORD_SIZE) {
      word_run++;
      buff_bit_run = 0;
    }

    if (buf[word_run] & (1 << buff_bit_run)) {
      OTP_DBG_PRINT("bit: %d - burn: %0lx  running_bit: %0x status: %lx  word_run: %d(d)\n",
                    otp_struct_lut[index].base_addr_bit + (word_run * OTP_WORD_SIZE) + buff_bit_run,
                    buf[word_run], 1 << buff_bit_run, (buf[word_run] & (1 << buff_bit_run)),
                    word_run);

      otp_burn_bit(otp_struct_lut[index].base_addr_bit + (word_run * OTP_WORD_SIZE) + buff_bit_run);
    }

    buff_bit_run++;
    size--;
  }

  REGISTER(OTP_OTP_SECURE_CHANNEL_OPEN_MCU) = OTP_DISABLE_WRITE;
  return OTP_OK;
}

static enum ALT1250_STATUS otp_field_burn(int index, uint32_t *buf) {
  return otp_field_write(index, buf);
}

enum ALT1250_STATUS drv_sb_enable_setotp(int index, uint32_t *buf) {
  OTP_DBG_PRINT("%s - %d\n", __FUNCTION__, __LINE__);
  return otp_field_burn(index, buf);
}

int otpWordRead(uint32_t otpWordAddress, uint32_t *pDest) {
  *pDest = REGISTER(BASE_ADDRESS_OTP_RAM_PMP + (otpWordAddress << 2));
  return 0;
}

int otpBitsRead(uint32_t bitOffset, int readBitCount, uint32_t *pDest) {
  uint32_t WordRead;
  uint32_t otpWordAddress;
  uint32_t InternalBitOffset;
  uint32_t NumberOfBitsRead;
  int RamainingBitsToRead;
  uint32_t *pCurrentDest;

  pCurrentDest = pDest;
  RamainingBitsToRead = readBitCount;
  otpWordAddress = bitOffset >> 5;  // Change from bit address to (trunked) word address
  InternalBitOffset = bitOffset & ((1 << 5) - 1);  // Isolate 5 lower bits
  NumberOfBitsRead = 32 - InternalBitOffset;

  if (InternalBitOffset == 0) {
    do {
      if (otpWordRead(otpWordAddress, &WordRead) == -1) {
        return -1;
      }
      *(pCurrentDest) = WordRead;
      pCurrentDest++;
      otpWordAddress++;

      RamainingBitsToRead -= 32;  // Read is aligned, InternalBitOffset==0
    } while (RamainingBitsToRead > 0);
    pCurrentDest--;
  } else {
    do {
      if (otpWordRead(otpWordAddress, &WordRead) == -1) {
        return -1;
      }
      OTP_DBG_PRINT("WordRead: %lx  otpWordAddress: %lx\n", WordRead, otpWordAddress);
      if (RamainingBitsToRead <
          readBitCount) {  // Not on the first iteration - Add remaining bits to previous data read
        *(pCurrentDest) |= WordRead << NumberOfBitsRead;
        RamainingBitsToRead -= InternalBitOffset;
        if (RamainingBitsToRead > 0) {
          pCurrentDest++;
        }
      }
      if (RamainingBitsToRead > 0) {  // Need to take more bits
        *(pCurrentDest) = WordRead >> InternalBitOffset;
        RamainingBitsToRead -= NumberOfBitsRead;
      }
      otpWordAddress++;
    } while (RamainingBitsToRead > 0);
  }

  if (RamainingBitsToRead < 0) {  // Mask result
    *(pCurrentDest) &= ((1 << (readBitCount & ((1 << 5) - 1))) - 1);
  }

  return 0;
}

int get_chip_revision_string(char *chip_revision) {
  int i, j = 0;
  uint32_t c = 0;

  /* chip rev is in reverse order and ASCII6 format
   * skipping the first character since in C0 it is 'D' (i.e. "DC0_TG"), and in D0 it will be ' '
   * (space)
   */
  for (i = 5; i > 0; i--, j++) {
    otpBitsRead(CONFIG_OTP_CHIP_REV + (i - 1) * 6, 6, &c);
    if (c == 0x00) break;
    chip_revision[j] = (char)c + 32;
    if (chip_revision[j] == '_')  // change underscore sign to minus
      chip_revision[j] = '-';
  }

  chip_revision[j] = 0x00;
  return j;
}

void read_chip_revision(void) {
  uint32_t chip_ver;
  char chip_revision_otp[100];  // chip_revision field requires 40bits, in ASCII6 format, reverse
                                // order. need more due to string filling

  strcpy(system_chip_revision_str, "UNKNOWN");

  chip_ver = REGISTER(CHIP_CHIP_VER_VERSION);
#if defined(ALT1255)
  // validate chip version
  if (((chip_ver & CHIP_CHIP_VER_VERSION_PRODUCT_NUM_MSK) >>
       CHIP_CHIP_VER_VERSION_PRODUCT_NUM_POS) != 0x1255) {
#else
  // validate chip version
  if (((chip_ver & CHIP_CHIP_VER_VERSION_PRODUCT_NUM_MSK) >>
       CHIP_CHIP_VER_VERSION_PRODUCT_NUM_POS) != 0x1250) {
#endif
    snprintf(
        system_chip_revision_str, sizeof(system_chip_revision_str), "UNKNOWN %04lX, %ld. %ld",
        ((chip_ver & CHIP_CHIP_VER_VERSION_PRODUCT_NUM_MSK) >>
         CHIP_CHIP_VER_VERSION_PRODUCT_NUM_POS),
        ((chip_ver & CHIP_CHIP_VER_VERSION_SW_VERSION_MSK) >> CHIP_CHIP_VER_VERSION_SW_VERSION_POS),
        ((chip_ver & CHIP_CHIP_VER_VERSION_HW_VERSION_MSK) >>
         CHIP_CHIP_VER_VERSION_HW_VERSION_POS));

    system_chip_revision = ALT1250_CHIP_STEP_ID_UNKNOWN;
    return;
  }

  // get chip id from OTP
  int l = get_chip_revision_string(chip_revision_otp);
  // printf("chip_revision_otp =[%s]\n", chip_revision_otp);

  // read metal-fix version
  uint32_t mfv = REGISTER(MFV_MFV);  // metal-fix version register

  // check otp chip revision
  if (l != 0) {
#if defined(ALT1255)
    if (mfv == 0) {
      system_chip_revision = ALT1250_CHIP_STEP_ID_A0;

      if (!strstr(chip_revision_otp, "A0")) strcat(chip_revision_otp, "(ASSUME A0)");

    } else {
      snprintf(system_chip_revision_str, sizeof(system_chip_revision_str), "UNKNOWN (%s)",
               chip_revision_otp);
      system_chip_revision = ALT1250_CHIP_STEP_ID_UNKNOWN;  // unknown - assume A0 due to mfv=0 ?
    }
#else
    // otp chip revision burned -> verify against mfv and use it
    if (mfv == 0) {
      if (strstr(chip_revision_otp, "C0")) {
        system_chip_revision = ALT1250_CHIP_STEP_ID_C0;
      } else if (strstr(chip_revision_otp, "B0")) {
        system_chip_revision = ALT1250_CHIP_STEP_ID_C0;
      } else {
        snprintf(system_chip_revision_str, sizeof(system_chip_revision_str), "UNKNOWN (%s)",
                 chip_revision_otp);
        system_chip_revision = ALT1250_CHIP_STEP_ID_UNKNOWN;  // unknown - assume C0 due to mfv=0 ?
      }
    } else {
      /* Search for the substrings */
      if (strstr(chip_revision_otp, "E0")) {
        system_chip_revision = ALT1250_CHIP_STEP_ID_E0;
      } else if (strstr(chip_revision_otp, "D0")) {
        system_chip_revision = ALT1250_CHIP_STEP_ID_D0;
      } else {
        if (mfv == 1) {
          strcat(chip_revision_otp, "(ASSUME D0)");
          system_chip_revision = ALT1250_CHIP_STEP_ID_D0;  // Metal-fix version#1 means "D0"
        }
        if (mfv == 3) {
          strcat(chip_revision_otp, "(ASSUME E0)");
          system_chip_revision = ALT1250_CHIP_STEP_ID_E0;  // Metal-fix version#1 means "D0"
        } else {
          snprintf(system_chip_revision_str, sizeof(system_chip_revision_str),
                   "UNKNOWN (%s, mfv=%lx)", chip_revision_otp, mfv);
          system_chip_revision =
              ALT1250_CHIP_STEP_ID_UNKNOWN;  // unknown - otp burned with wrong value
        }
      }
    }
#endif
  } else {
#if defined(ALT1255)
    if (mfv == 0) {
      snprintf(chip_revision_otp, sizeof(chip_revision_otp), "ASSUME A0 (empty otp, mfv=%lx)", mfv);
      system_chip_revision = ALT1250_CHIP_STEP_ID_A0;
    } else {
      snprintf(chip_revision_otp, sizeof(chip_revision_otp), "UNKNOWN (empty otp, mfv=%lx)", mfv);
      system_chip_revision = ALT1250_CHIP_STEP_ID_UNKNOWN;
    }
#else
    // otp chip revision not burned -> pre-factory devices...
    if (mfv == 0) {  // assume B0/C0 - based on compilation only
      // step_id = alt1250_get_chip_step_id();
      strcpy(chip_revision_otp, "C0-XX");
      system_chip_revision = ALT1250_CHIP_STEP_ID_C0;
    } else if (mfv == 1) {
      // for D0 it is not allowed to have empty OTP chips
      snprintf(chip_revision_otp, sizeof(chip_revision_otp), "ASSUME D0 (empty otp, mfv=%lx)", mfv);
      system_chip_revision = ALT1250_CHIP_STEP_ID_D0;
    } else if (mfv == 3) {
      // for E0 it is not allowed to have empty OTP chips
      snprintf(chip_revision_otp, sizeof(chip_revision_otp), "ASSUME E0 (empty otp, mfv=%lx)", mfv);
      system_chip_revision = ALT1250_CHIP_STEP_ID_E0;
    } else {
      // for D0 it is not allowed to have empty OTP chips
      snprintf(chip_revision_otp, sizeof(chip_revision_otp), "UNKNOWN (empty otp, mfv=%lx)", mfv);
      system_chip_revision = ALT1250_CHIP_STEP_ID_UNKNOWN;
    }
#endif
  }

  if (system_chip_revision != ALT1250_CHIP_STEP_ID_UNKNOWN)
    snprintf(system_chip_revision_str, sizeof(system_chip_revision_str), "ALT%04lX %s (%ld)",
             ((chip_ver & CHIP_CHIP_VER_VERSION_PRODUCT_NUM_MSK) >>
              CHIP_CHIP_VER_VERSION_PRODUCT_NUM_POS),
             chip_revision_otp, system_chip_revision);
}

uint32_t get_chip_revision(void) {
  if (system_chip_revision == ALT1250_CHIP_STEP_ID_UNKNOWN) {
    read_chip_revision();
  }

  return system_chip_revision;
}

uint32_t swap_uint32(uint32_t val) {
  val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
  return (val << 16) | (val >> 16);
}

otp_sb_enable_status_e DRV_SB_Enable_MCU(uint32_t offset) {
  int puk_Idx, i;
  uint32_t pubKeyStartAddr = 0;
  uint32_t calcChecksum = 0, designatedChecksum = 0;
  enum ALT1250_STATUS status;
  uint32_t setBitVal = 0;
  uint32_t buffer[(SB_PUK2_SIZE + SB_CRC32_SIZE) / 4];
  uint32_t mbuffer[(SB_PUK2_SIZE + SB_CRC32_SIZE) / 4];
  uint32_t sha256_Out[8] = {0}, msha256_Out[8] = {0};
  uint32_t otp_data[8] = {0}, motp_data[8] = {0};
  int mcuSecureBootEnableIdx, mcuDh0SecureBootEnableIdx;

  printf("Going to burn data into OTP...\n");

  // read puk3_1 and puk3_2 from flash, verify their crc is correct and write their hash values to
  // otp (hpuk3_1 and hpuk3_2)
#if !defined(ALT1255)
  if (ALT1250_CHIP_STEP_ID_E0 == get_chip_revision())
#endif
  {
    printf("MCU_PUK3 flashPartitionOffset = 0x%08lx\n", offset);

    pubKeyStartAddr = offset;
    // make sure address within flash
    pubKeyStartAddr &= 0x00ffffff;
    pubKeyStartAddr += 0x08000000;

    // Read puk3_1
    printf("Going to read puk3_1 rsa key from flash start address=0x%08lx, length=%u\n",
           pubKeyStartAddr, sizeof(buffer));

    memcpy((ulong *)buffer, (ulong *)pubKeyStartAddr, sizeof(buffer));
    // hex_dump((uint8_t *)buffer, sizeof(buffer));

    printf("buffer[0] = 0x%08lx, buffer[4] = 0x%08lx\n", *(uint32_t *)&buffer[0],
           *(uint32_t *)&buffer[4]);
    designatedChecksum = *(uint32_t *)(buffer + SB_PUK2_SIZE / 4);

    // calculate crc32 over puk3_1
    calcChecksum = crc32((const char *)buffer, SB_PUK2_SIZE);
    printf("calcChecksum = 0x%08lx, designatedChecksum = 0x%08lx\n", calcChecksum,
           designatedChecksum);

    if (designatedChecksum != calcChecksum) {
      printf("ERROR: Checksum mismatch 0x%08lx (calculated) vs. 0x%08lx (payload). abort...\n",
             calcChecksum, designatedChecksum);
      return (OTP_SB_ENABLE_CHECKSUM_ERROR);
    }

    // calculate sha256 of puk3_1
    for (i = 0; i < ((SB_PUK2_SIZE + SB_CRC32_SIZE) / 4); i++) mbuffer[i] = swap_uint32(buffer[i]);
    calc_sha256((uint8_t *)mbuffer, SB_PUK2_SIZE, (uint8_t *)sha256_Out);

    printf("Calculated hash of puk3_1: \n");
    hex_dump((uint8_t *)sha256_Out, sizeof(sha256_Out));

    // write hash of puk3_1 to otp
    puk_Idx = OTP_MCU_HPUK3_1;

    // first, read the puk3_1 from otp to check if already equal to want we want to write now
    status = drv_sb_enable_getotp(puk_Idx, otp_data);
    if (otp_sb_enable_check_error(puk_Idx, status) != OTP_OK) {
      printf("ERROR: Cannot read otp\n");
      return (OTP_SB_READ_OTP_ERROR);
    }

    for (i = 0; i < 8; i++) motp_data[i] = swap_uint32(otp_data[i]);
    hex_dump((uint8_t *)motp_data, sizeof(motp_data));

    // compare with new buffer
    if (memcmp(motp_data, sha256_Out, sizeof(sha256_Out))) {  // not identical
      // see if all is zeros
      uint32_t sum = motp_data[0] + motp_data[1] + motp_data[2] + motp_data[3] + motp_data[4] +
                     motp_data[5] + motp_data[6] + motp_data[7];
      if (0 != sum) {
        printf(
            "ERROR: puk3_1 otp already written with other data. Cannot support MCU SB on this "
            "device anymore. Aborting... \n");
        return (OTP_SB_ENABLE_OTP_HPUK_ERROR);
      }

      // write new data to otp
      // otp_convert_stream_endianess
      for (i = 0; i < 8; i++) msha256_Out[i] = swap_uint32(sha256_Out[i]);

      status = drv_sb_enable_setotp(puk_Idx, msha256_Out);
      if (otp_sb_enable_check_error(puk_Idx, status) != OTP_OK) {
        return (OTP_SB_WRITE_OTP_ERROR);
      }
      printf("hash of puk3_1 written to otp successfully. \n");
    } else {
      printf("correct hash of puk3_1 was already written to otp in past. \n");
    }

    // Read puk3_2
    pubKeyStartAddr += (SB_PUK2_SIZE + 4);  // puk3_2 is immediately after puk3_2 crc
    printf("Going to read puk3_2 rsa key from flash start address=0x%08lx, length=%u\n",
           pubKeyStartAddr, sizeof(buffer));

    memcpy((ulong *)buffer, (ulong *)pubKeyStartAddr, sizeof(buffer));
    // hex_dump((uint8_t *)buffer, sizeof(buffer));

    designatedChecksum = *(uint32_t *)(buffer + SB_PUK2_SIZE / 4);

    // calculate crc32 over puk3_2
    calcChecksum = crc32((const char *)buffer, SB_PUK2_SIZE);
    printf("calcChecksum = 0x%08lx, designatedChecksum = 0x%08lx\n", calcChecksum,
           designatedChecksum);

    if (designatedChecksum != calcChecksum) {
      printf("ERROR: Checksum mismatch 0x%08lx (calculated) vs. 0x%08lx (payload). abort...\n",
             calcChecksum, designatedChecksum);
      return (OTP_SB_ENABLE_CHECKSUM_ERROR);
    }

    // calculate sha256 of puk3_2
    for (i = 0; i < ((SB_PUK2_SIZE + SB_CRC32_SIZE) / 4); i++) mbuffer[i] = swap_uint32(buffer[i]);
    calc_sha256((uint8_t *)mbuffer, SB_PUK2_SIZE, (uint8_t *)sha256_Out);

    printf("Calculated hash of puk3_2: \n");
    hex_dump((uint8_t *)sha256_Out, sizeof(sha256_Out));

    // write hash of puk3_2 to otp
    puk_Idx = OTP_MCU_HPUK3_2;

    // first, read the puk3_2 from otp to check if already equal to want we want to write now
    status = drv_sb_enable_getotp(puk_Idx, otp_data);
    if (otp_sb_enable_check_error(puk_Idx, status) != OTP_OK) {
      printf("ERROR: Cannot read otp\n");
      return (OTP_SB_READ_OTP_ERROR);
    }

    for (i = 0; i < 8; i++) motp_data[i] = swap_uint32(otp_data[i]);
    hex_dump((uint8_t *)motp_data, sizeof(motp_data));

    // compare with new buffer
    if (memcmp(motp_data, sha256_Out, sizeof(sha256_Out))) {  // not identical
      // see if all is zeros
      uint32_t sum = motp_data[0] + motp_data[1] + motp_data[2] + motp_data[3] + motp_data[4] +
                     motp_data[5] + motp_data[6] + motp_data[7];
      if (0 != sum) {
        printf(
            "ERROR: puk3_2 otp already written with other data. Cannot support SB on this device "
            "anymore. Aborting... \n");
        return (OTP_SB_ENABLE_OTP_HPUK_ERROR);
      }

      // write new data to otp
      // otp_convert_stream_endianess
      for (i = 0; i < 8; i++) msha256_Out[i] = swap_uint32(sha256_Out[i]);

      status = drv_sb_enable_setotp(puk_Idx, msha256_Out);
      if (otp_sb_enable_check_error(puk_Idx, status) != OTP_OK) {
        return (OTP_SB_WRITE_OTP_ERROR);
      }

      printf("hash of puk3_2 written to otp successfully. \n");
    } else {
      printf("correct hash of puk3_2 was already written to otp in past. \n");
    }

    mcuSecureBootEnableIdx = OTP_MCU_SECURE_BOOT_ENABLE;

    status = drv_sb_enable_getotp(mcuSecureBootEnableIdx, (uint32_t *)&setBitVal);
    if (otp_sb_enable_check_error(mcuSecureBootEnableIdx, status) != OTP_OK) {
      printf("ERROR: Cannot read otp\n");
      return (OTP_SB_READ_OTP_ERROR);
    }

    if (0 == setBitVal) {
      setBitVal = 1;
      status = drv_sb_enable_setotp(mcuSecureBootEnableIdx, (uint32_t *)&setBitVal);
      if (otp_sb_enable_check_error(mcuSecureBootEnableIdx, status) != OTP_OK) {
        return (OTP_SB_WRITE_OTP_ERROR);
      }
    } else {
      printf("MCU SB enable bit was already written to otp in past. \n");
    }

    mcuDh0SecureBootEnableIdx = OTP_MCU_DH0_SECURE_BOOT_ENABLE;

    status = drv_sb_enable_getotp(mcuDh0SecureBootEnableIdx, (uint32_t *)&setBitVal);
    if (otp_sb_enable_check_error(mcuDh0SecureBootEnableIdx, status) != OTP_OK) {
      printf("ERROR: Cannot read otp\n");
      return (OTP_SB_READ_OTP_ERROR);
    }

    if (0 == setBitVal) {
      setBitVal = 1;
      status = drv_sb_enable_setotp(mcuDh0SecureBootEnableIdx, (uint32_t *)&setBitVal);
      if (otp_sb_enable_check_error(mcuDh0SecureBootEnableIdx, status) != OTP_OK) {
        return (OTP_SB_WRITE_OTP_ERROR);
      }
    } else {
      printf("MCU DH0 SB enable bit was already written to otp in past. \n");
    }

    printf("SBootEnableMCU SUCCESS, SB for MCU will be applied each boot\n");
  }

  return (OTP_SB_ENABLE_OK);
}
