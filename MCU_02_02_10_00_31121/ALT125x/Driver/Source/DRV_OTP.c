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
/* Standard Headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "DRV_COMMON.h"
#include "DRV_OTP.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define BASE_ADDRESS_OTP 0x0D000800
#define BASE_ADDRESS_OTP_RAM_PMP 0x0D030000

#define OTP_OTP_SECURE_CHANNEL_OPEN_MCU (BASE_ADDRESS_OTP + 0x7C)

#define OTP_MCU_CUSTOM1_BIT_OFFSET 18720
#define OTP_MCU_CUSTOM1_SIZE_IN_BITS 1024
#define OTP_MCU_CUSTOM1_LOCK_BIT (20475 + 2)

#define OTP_MCU_CUSTOM2_BIT_OFFSET 19744
#define OTP_MCU_CUSTOM2_SIZE_IN_BITS 512
#define OTP_MCU_CUSTOM2_LOCK_BIT (20475 + 3)

#define OTP_WORD_SIZE 32

#define OTP_ENABLE_WRITE 0xCAFECAFE
#define OTP_DISABLE_WRITE 0xDEADBEAF

#ifdef OTP_DBG
#define OTP_DBG_PRINT(fmt, args...) \
  { printf("DBG " fmt, ##args); }
#else
#define OTP_DBG_PRINT(fmt, args...)
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/
typedef struct {
  char name[50];
  DRV_OTP_FIELDS type;
  uint32_t base_addr_bit;
  uint32_t size_bit;
  int32_t locking_val;
} drv_otp_field;

const drv_otp_field drv_otp_list[] = {
    {"\"MCU_CUSTOM1\"", MCU_OEM_PARAM_1, OTP_MCU_CUSTOM1_BIT_OFFSET, OTP_MCU_CUSTOM1_SIZE_IN_BITS,
     OTP_MCU_CUSTOM1_LOCK_BIT},
    {"\"MCU_CUSTOM2\"", MCU_OEM_PARAM_2, OTP_MCU_CUSTOM2_BIT_OFFSET, OTP_MCU_CUSTOM2_SIZE_IN_BITS,
     OTP_MCU_CUSTOM2_LOCK_BIT}};

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static int drv_otp_WordRead(uint32_t otpWordAddress, uint32_t *pDest) {
  *pDest = REGISTER(BASE_ADDRESS_OTP_RAM_PMP + (otpWordAddress << 2));
  return 0;
}

static int drv_otp_BitsRead(uint32_t bitOffset, int readBitCount, uint32_t *pDest) {
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
      if (drv_otp_WordRead(otpWordAddress, &WordRead) == -1) {
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
      if (drv_otp_WordRead(otpWordAddress, &WordRead) == -1) {
        return -1;
      }
      OTP_DBG_PRINT("WordRead: %x  otpWordAddress: %x\n", WordRead, otpWordAddress);
      if (RamainingBitsToRead < readBitCount) {
        // Not on the first iteration - Add remaining bits to previous data read
        *(pCurrentDest) |= WordRead << NumberOfBitsRead;
        RamainingBitsToRead -= InternalBitOffset;
        if (RamainingBitsToRead > 0) {
          pCurrentDest++;
        }
      }
      if (RamainingBitsToRead > 0) {
        // Need to take more bits
        *(pCurrentDest) = WordRead >> InternalBitOffset;
        RamainingBitsToRead -= NumberOfBitsRead;
      }
      otpWordAddress++;
    } while (RamainingBitsToRead > 0);
  }
  if (RamainingBitsToRead < 0) {
    // Mask result
    *(pCurrentDest) &= ((1 << (readBitCount & ((1 << 5) - 1))) - 1);
  }
  return 0;
}

static int drv_otp_burn_word(int addr, uint32_t value) {
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
  drv_otp_WordRead(addr, &read_word);
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

static int drv_otp_burn_bit(int bit_addr) {
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

  OTP_DBG_PRINT("word_addr: %x  word_value: %x\n", word_addr, word_value);
  return drv_otp_burn_word(word_addr, word_value);
}

static int drv_otp_is_otp_locked(int index) {
  uint32_t otp_read;

  OTP_DBG_PRINT("%s - %d\n", __FUNCTION__, __LINE__);

  if (drv_otp_list[index].locking_val < 0) {
    // not locked
    return 0;
  }

  drv_otp_BitsRead(drv_otp_list[index].locking_val, 1, &otp_read);
  return otp_read ? 1 : 0;
}

static DRV_OTP_STATUS drv_otp_lock_otp(int index) {
  /* Always word aligned */
  DRV_OTP_STATUS status = OTP_NOK;

  OTP_DBG_PRINT("%s - %d\n", __FUNCTION__, __LINE__);

  if (drv_otp_is_otp_locked(index)) {
    return OTP_ALREADY_LOCKED;
  }

  if (!drv_otp_burn_bit(drv_otp_list[index].locking_val) && drv_otp_is_otp_locked(index)) {
    status = OTP_OK;
  }

  return status;
}

static DRV_OTP_STATUS drv_otp_field_get(int index, uint32_t *buf, uint32_t in_size_bit) {
  DRV_OTP_STATUS status = OTP_OK;

  OTP_DBG_PRINT("%s - %d\n", __FUNCTION__, __LINE__);
  OTP_DBG_PRINT("drv_otp_field_get: base: %d  size in bit: %d \n",
                drv_otp_list[index].base_addr_bit, drv_otp_list[index].size_bit);

  if (drv_otp_list[index].size_bit < in_size_bit) {
    return OTP_ILLEGAL_VALUE;
  }

  if (drv_otp_BitsRead(drv_otp_list[index].base_addr_bit, in_size_bit, buf) < 0) {
    status = OTP_ACCESS_ERROR;
  }

  return status;
}

static DRV_OTP_STATUS drv_otp_verify_field_content(int index) {
  uint32_t *buf;
  int size_bit, size_byte;
  int buff_bit_run = 0, word_run = 0;
  DRV_OTP_STATUS status = OTP_OK;

  OTP_DBG_PRINT("%s - %d\n", __FUNCTION__, __LINE__);

  size_bit = drv_otp_list[index].size_bit;
  size_byte = size_bit >> 3;
  if (!size_byte) {
    size_byte = 1;
  }

  buf = malloc(size_byte < 4 ? 4 : size_byte + 4);
  if (!buf) {
    return OTP_MEMORY_ERROR;
  }

  if (drv_otp_field_get(index, buf, drv_otp_list[index].size_bit) == OTP_ACCESS_ERROR) {
    free(buf);
    return OTP_ACCESS_ERROR;
  }

  while (size_bit) {
    if (buff_bit_run == OTP_WORD_SIZE) {
      word_run++;
      buff_bit_run = 0;
    }

    if (buf[word_run] & (1 << buff_bit_run)) {
      status = OTP_ALREADY_WRITTEN;
      break;
    }

    buff_bit_run++;
    size_bit--;
  }

  free(buf);
  return status;
}

static int drv_otp_write_enabled(void) { return REGISTER(OTP_OTP_SECURE_CHANNEL_OPEN_MCU); }

static DRV_OTP_STATUS drv_otp_field_write(int index, uint32_t *buf, uint32_t in_size_bit) {
  int buff_bit_run = 0, word_run = 0, retry = 0;
  DRV_OTP_STATUS status = OTP_OK;
  int size = in_size_bit;  // drv_otp_list[index].size_bit;

  OTP_DBG_PRINT("%s - %d\n", __FUNCTION__, __LINE__);
  OTP_DBG_PRINT("otp_field_burn: start of buff: %0X\n", buf[0]);

  REGISTER(OTP_OTP_SECURE_CHANNEL_OPEN_MCU) = OTP_ENABLE_WRITE;

  while (!drv_otp_write_enabled()) {
    if (retry > 1000) {
      OTP_DBG_PRINT("otp_write can be enabled!\n");
      return OTP_ACCESS_ERROR;
    }
    retry++;
  }
  OTP_DBG_PRINT("otp_write retries: %d\n", retry);

  /* Notify if OTP is locked */
  if (drv_otp_is_otp_locked(index)) {
    return OTP_LOCKED;
  }

  /* Notify if OTP is dirty */
  if ((status = drv_otp_verify_field_content(index)) != OTP_OK) {
    return status;
  }

  while (size) {
    if (buff_bit_run == OTP_WORD_SIZE) {
      word_run++;
      buff_bit_run = 0;
    }

    /*
     * OTP bit = 0 at factory setting.
     * Check if burning is required
     */

    if (buf[word_run] & (1 << buff_bit_run)) {
      OTP_DBG_PRINT("bit: %d - burn: %0x  running_bit: %0x status: %x  word_run: %d(d)\n",
                    drv_otp_list[index].base_addr_bit + (word_run * OTP_WORD_SIZE) + buff_bit_run,
                    buf[word_run], 1 << buff_bit_run, (buf[word_run] & (1 << buff_bit_run)),
                    word_run);

      drv_otp_burn_bit(drv_otp_list[index].base_addr_bit + (word_run * OTP_WORD_SIZE) +
                       buff_bit_run);
    }

    buff_bit_run++;
    size--;
  }

  /* locked it */
  drv_otp_lock_otp(index);

  REGISTER(OTP_OTP_SECURE_CHANNEL_OPEN_MCU) = OTP_DISABLE_WRITE;

  return OTP_OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

DRV_OTP_STATUS DRV_OTP_Set(DRV_OTP_FIELDS index, uint32_t *buf, uint32_t size) {
  OTP_DBG_PRINT("%s - %d - index: %u\n", __FUNCTION__, __LINE__, index);
  if (drv_otp_list[index].type >= OTP_NULL_LAST) {
    return OTP_ILLEGAL_VALUE;
  }

  if ((!size) || (drv_otp_list[index].size_bit < (size * 8))) {
    return OTP_ILLEGAL_VALUE;
  }

  return drv_otp_field_write(index, buf, size * 8);
}

DRV_OTP_STATUS DRV_OTP_Get(DRV_OTP_FIELDS index, uint32_t *buf, uint32_t size) {
  OTP_DBG_PRINT("%s - %d - index: %u\n", __FUNCTION__, __LINE__, index);
  if (drv_otp_list[index].type >= OTP_NULL_LAST) {
    return OTP_ILLEGAL_VALUE;
  }

  if ((!size) || (drv_otp_list[index].size_bit < (size * 8))) {
    return OTP_ILLEGAL_VALUE;
  }

  return drv_otp_field_get(index, buf, size * 8);
}
