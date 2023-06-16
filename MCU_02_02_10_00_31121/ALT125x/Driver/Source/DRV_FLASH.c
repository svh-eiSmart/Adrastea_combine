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
#include DEVICE_HEADER
#include <string.h>
#include "DRV_COMMON.h"
#include "DRV_FLASH.h"

#define CONFIG_SFLASH_WRITE_PROTECTION_ENABLE
#define SF_USE_PRIVATE_MEM_OPER_ENABLE
#define CONFIG_SFLASH_ENABLE_VERIFY

#define SFLASH_CONTROLLER_WRITE_PAGE_SIZE (0x100) /* 256 bytes controller's page size */
#define SF_CMD_RESULT_TIME_OUT (5000000)
#define SF_ERASE_SECTOR_TIME_OUT (5000000)

#if !defined(SF_USE_PRIVATE_MEM_OPER_ENABLE)
#define sf_memcpy memcpy
#define sf_memcmp memcmp
#endif

#if !defined(CONFIG_SYS_FLASH_BASE)
#define CONFIG_SYS_FLASH_BASE 0xb8000000 /*Memory-mapped FLASH base address*/
#endif

#define CHECK_PARAM(par)          \
  if (!par) {                     \
    return FLASH_ERROR_PARAMETER; \
  }
static volatile int sf_is_busy = 0;
static uint32_t g_sf_do_AND_for_write = 1;  // PRODUCTS-19415 - for Fidelix need to do AND operation
                                            // before writing to have "read-modify-write"
static int validate_addr_range(void *addr, size_t len) {
  uint32_t local_addr = (uint32_t)addr;

  if (local_addr >= MCU_BASE_ADDR && local_addr <= MCU_BASE_ADDR + MCU_PART_SIZE &&
      local_addr + len >= MCU_BASE_ADDR &&
      local_addr + len <= MCU_BASE_ADDR + MCU_PART_SIZE)  // success
  {
    return 0;
  } else {
    return -1;
  }
}

static void sf_delay(size_t delay) {
  volatile unsigned long i, j, k = 0;

  while (delay--) {
    for (i = 0; i < 10; i++)
      for (j = 0; j < 10; j++) k += 3;
  }
}

#if defined(SF_USE_PRIVATE_MEM_OPER_ENABLE)
static int sf_memcmp(const void *cs, const void *ct, unsigned long count) {
  const unsigned char *su1, *su2;
  int res = 0;
  for (su1 = cs, su2 = ct; 0 < count; ++su1, ++su2, count--)
    if ((res = *su1 - *su2) != 0) break;
  return res;
}

static void *sf_memcpy(void *dst, const void *src, unsigned long count, uint32_t do_AND) {
  unsigned char *su1;
  const unsigned char *su2;
  if (!do_AND) {
    // normal copy
    for (su1 = dst, su2 = src; 0 < count; ++su1, ++su2, count--) *su1 = *su2;
  } else {
    // do AND operation before actual copy
    for (su1 = dst, su2 = src; 0 < count; ++su1, ++su2, count--) {
      *su1 = *su1 & *su2;  // do AND with current content
    }
  }

  return dst;
}
#endif /*SF_USE_PRIVATE_MEM_OPER_ENABLE*/

static inline int serial_flash_is_device_busy(void) {
  /*in case sf_is_busy=1 then we need to wait for status=1 which means operation is done*/
  if (sf_is_busy) {
    sf_is_busy = SERIAL_FLASH_CTRL_MCU_SUBSYS->STATUS_MCU_RC_b.INT_STAT ? 0 : 1;
  }

  return sf_is_busy;
}

static void serial_flash_wait_device_not_busy(size_t timeout) {
  while (serial_flash_is_device_busy()) {
    sf_delay(1);
    timeout--;
    if (timeout == 0) {
      break;
    }
  }
}

#if defined(CONFIG_SFLASH_ENABLE_VERIFY)
static int serial_flash_verify_buffer(void *src, void *dst, size_t len) {
  serial_flash_wait_device_not_busy(
      SF_CMD_RESULT_TIME_OUT);  // wait for previous operation to finish

  // clear prefetch buffer before verify
  SERIAL_FLASH_CTRL_MCU_SUBSYS->CFG_SYSTEM = 0xf;

  // need to compare timing memcmp/sf_memcmp using other cpu ram counters
  int cmp = sf_memcmp(src, dst, len);
  if (cmp) {
    sf_delay(100);

    cmp = sf_memcmp(src, dst, len);
    if (cmp) {
      return FLASH_ERROR_VERIFY;
    }
  }

  return FLASH_ERROR_NONE;
}
#endif /*CONFIG_SFLASH_ENABLE_VERIFY*/

static Flash_Err_Code serial_flash_write_page(void *src, void *dst, size_t size, int verify) {
  uint32_t local_src = (uint32_t)src;
  uint32_t local_dst = (uint32_t)dst;

  serial_flash_wait_device_not_busy(SF_CMD_RESULT_TIME_OUT);
  sf_is_busy = 1;
  /* Use a dummy read to workaround the page write problem - TODO: check if still required */
  sf_memcmp((void *)local_dst, (void *)local_src, 1);

#if defined(CONFIG_SFLASH_WRITE_PROTECTION_ENABLE)
  // remove write protection to allow write/erase
  SERIAL_FLASH_CTRL_MCU_SUBSYS->WRITE_PROTECT = 0;
#endif
  /* write 'size' to counter register:
   * hardware requires size to be in longs (32bit "words", 4 bytes) - lowest 2 bits are zero's
   * in case size is not aligned 4 bytes -> aligned the size to upper 4 bytes (write 0xff in
   * trailing) alignment is done by adding 3 and zeroing the lowest 2 bits (1,2,3,4 -> 4; 5,6,7,8 ->
   * 8; etc.) Jira PRODUCTS-15202: in case dst address is not aligned, need to add the prefix bytes
   * to the write counter, since they are written as 4 bytes
   */
  SERIAL_FLASH_CTRL_MCU_SUBSYS->WRITE_COUNT =
      ((size + (local_dst & 0x3) + 3) & ~0x3) |
      SERIAL_FLASH_CTRL_MCU_SUBSYS_WRITE_COUNT_COUNT_CLR_Msk;

  /* Test whether prefix alignment is required */
  if (local_dst & 0x3) {
    uint32_t *aligned_to = (uint32_t *)(local_dst & 0xFFFFFFFC);
    uint32_t aligned_value = *aligned_to;
    uint32_t len = (4 - (local_dst & 0x3));
    if (size < len) {
      len = size;
    }
    sf_memcpy(((char *)&aligned_value + (local_dst & 0x3)), (void *)local_src, len,
              g_sf_do_AND_for_write);
    REGISTER(aligned_to) = aligned_value;
    local_dst = (uint32_t)++aligned_to;
    local_src = (local_src + len);
    size -= len;
  }
  // printf("size:%ld dst:%lx, src:%lx\n",size,local_dst,local_src);
  /* copy all, except trailing (if not aligned to 4) */
  while (size >= 4) {
    uint32_t val;
    // if need AND - read current content and handle it
    if (g_sf_do_AND_for_write) {
      val = REGISTER(local_dst);
      sf_memcpy(&val, (const void *)local_src, 4, 1);
    } else {
      // normal handling (no AND)
      if (!(local_src & 0x3)) /* Aligned source address - copy by 4's */
      {
        val = *(uint32_t *)local_src;
      } else
        /* Unaligned source address - copy by 1's */
        sf_memcpy(&val, (const void *)local_src, sizeof(val), 0);
    }
    REGISTER(local_dst) = val;

    local_src += 4;
    local_dst += 4;
    size -= 4;
  }
  // printf("size:%ld dst:%lx, src:%lx\n",size,local_dst,local_src);
  /* Test whether suffix alignment is required */
  if (size) {
    /* copy remaining - if not aligned to 4 */
    uint32_t val = 0xffffffff;
    // if need AND - read current content
    if (g_sf_do_AND_for_write) {
      val = REGISTER(local_dst);
    }
    sf_memcpy(&val, (const void *)local_src, size, g_sf_do_AND_for_write);
    REGISTER(local_dst) = val;
  }

#if defined(CONFIG_SFLASH_WRITE_PROTECTION_ENABLE)
  // wait until write sent from internal queue to pass firewall (should be immediate, waiting
  // execution/pending)
  int timeout = 10000;
  while (!(SERIAL_FLASH_CTRL_MCU_SUBSYS->STATUS_MCU_b.WRITE_EXECUTING |
           SERIAL_FLASH_CTRL_MCU_SUBSYS->STATUS_MCU_b.WRITE_PENDING_TO_ARB |
           SERIAL_FLASH_CTRL_MCU_SUBSYS->STATUS_MCU_b.INT_STAT) &&
         --timeout != 0) {
  }
  // enable back the write protection
  SERIAL_FLASH_CTRL_MCU_SUBSYS->WRITE_PROTECT = 1;

  if (timeout == 0) {
    return FLASH_ERROR_TIMEOUT;
  }
#endif

  return FLASH_ERROR_NONE;
}

//search block if it is already erased (not written since last erase)
static int serial_flash_do_blank_check(void *addr, int is_64KB_sect)
{
  int ret = 1; //assume is blank
  uint32_t size = is_64KB_sect ? (64 * 1024u) : (4 * 1024u);
  uint32_t flash_addr = (uint32_t)addr;

  //mask address to make sure it starts on 4k/64 aligned address - so check will start at start of sector
  flash_addr &= ~(size -1);

  uint32_t end_addr = flash_addr + size;

  while(flash_addr != end_addr)
  {
    if(*((uint32_t *)flash_addr) != 0xFFFFFFFF)
    {
      ret = 0;
      break;
    }
    flash_addr += 4;
  }

  return ret;
}

Flash_Err_Code DRV_FLASH_Erase_Sector(void *addr, int is_64KB_sect, int wait_for_finish) {
  uint32_t erase_cmd;
  uint32_t flash_addr;

  CHECK_PARAM(addr)

  flash_addr = ((uint32_t)addr & ~CONFIG_SYS_FLASH_BASE);

  if (validate_addr_range(addr, 0)) {
    return FLASH_ERROR_ADDRESS_RANGE;
  }

  if (serial_flash_do_blank_check(addr, is_64KB_sect)) {
    return FLASH_ERROR_NONE;
  }

  // clang-format off
  // Deter compiler from unwanted optimization on linker script symbols.
  // https://mcuoneclipse.com/2016/11/01/getting-the-memory-range-of-sections-with-gnu-linker-files/
  unsigned int *volatile p = (unsigned int *volatile) &__MCU_MTD_PARTITION_OFFSET__;
  // clang-format on
  if (p == 0x00000000) {
#define ALT1250B_SF_OFFSET_STEP (0x4000)
    uint32_t sf_offset = SERIAL_FLASH_CTRL_SECURITY->SECURED_CONID2_OFFSET;
    sf_offset *= ALT1250B_SF_OFFSET_STEP;
    sf_offset += MCU_BASE_ADDR;
    flash_addr += sf_offset;
  }

  serial_flash_wait_device_not_busy(SF_ERASE_SECTOR_TIME_OUT);
  sf_is_busy = 1;

#if defined(CONFIG_SFLASH_WRITE_PROTECTION_ENABLE)
  // remove write protection to allow write/erase
  SERIAL_FLASH_CTRL_MCU_SUBSYS->WRITE_PROTECT = 0;
#endif

  /*addr is in u32 units (div by 4), and need to shift left by 8, so in total we shift left only 6*/
  erase_cmd =
      (flash_addr << (SERIAL_FLASH_CTRL_MCU_SUBSYS_SFC_ERASE_CFG_ADDR_Pos -
                      2)) |  // address is 4bytes aligned
      ((is_64KB_sect ? 1 : 0) << SERIAL_FLASH_CTRL_MCU_SUBSYS_SFC_ERASE_CFG_OPCODE_SEL_Pos) |
      1;

  SERIAL_FLASH_CTRL_MCU_SUBSYS->SFC_ERASE_CFG = erase_cmd;

#if defined(CONFIG_SFLASH_WRITE_PROTECTION_ENABLE)

  // reading from the register should be enough to make sure command passed the firewall since it is
  // serialized
  erase_cmd = SERIAL_FLASH_CTRL_MCU_SUBSYS->SFC_ERASE_CFG;

  // enable back the write protection
  SERIAL_FLASH_CTRL_MCU_SUBSYS->WRITE_PROTECT = 1;
#endif

  if (wait_for_finish) {
    serial_flash_wait_device_not_busy(SF_ERASE_SECTOR_TIME_OUT);
  }

  return FLASH_ERROR_NONE;
}

Flash_Err_Code DRV_FLASH_Write(void *src, void *dst, size_t len, int do_verify) {
  Flash_Err_Code ret = FLASH_ERROR_NONE;
  size_t size;
  uint32_t local_dst = (uint32_t)dst;
  uint32_t local_src = (uint32_t)src;
  size_t local_len = len;

  CHECK_PARAM(src)
  CHECK_PARAM(dst)

  if (validate_addr_range(dst, len)) {
    return FLASH_ERROR_ADDRESS_RANGE;
  }
  serial_flash_wait_device_not_busy(SF_CMD_RESULT_TIME_OUT);

#if defined(CONFIG_SFLASH_WRITE_DETECTION_ENABLE) || defined(CONFIG_SFLASH_WRITE_PROTECTION_ENABLE)
  // check if sporadic write occurred by checking counter value
  if (SERIAL_FLASH_CTRL_MCU_SUBSYS->WRITE_COUNT_b.IN_COUNTER) {
    // clear counter
    SERIAL_FLASH_CTRL_MCU_SUBSYS->WRITE_COUNT_b.COUNT_CLR = 1;
  }
#endif

  /*dummy read to clean up pp error*/
  SERIAL_FLASH_CTRL_MCU_SUBSYS->ERROR_STATUS_RC;

  while (local_len) {
    size = SFLASH_CONTROLLER_WRITE_PAGE_SIZE - local_dst % SFLASH_CONTROLLER_WRITE_PAGE_SIZE;
    if (size > local_len) {
      size = local_len;
    }
    ret = serial_flash_write_page((void *)local_src, (void *)local_dst, size, do_verify);
    if (ret) {
      break;
    }

    local_src += size;
    local_dst += size;
    local_len -= size;
  }

#if defined(CONFIG_SFLASH_WRITE_DETECTION_ENABLE) || defined(CONFIG_SFLASH_WRITE_PROTECTION_ENABLE)
  // write MAX size to counter register - size is in longs (32bit "words") (wait is already done in
  // write_page)
  SERIAL_FLASH_CTRL_MCU_SUBSYS->WRITE_COUNT =
      256 & SERIAL_FLASH_CTRL_MCU_SUBSYS_WRITE_COUNT_VAL_Msk;
#endif

#if defined(CONFIG_SFLASH_ENABLE_VERIFY)
  if (do_verify) {
    if (serial_flash_verify_buffer(src, dst, len)) {
      return FLASH_ERROR_VERIFY;
    }
  }
#endif /*SFLASH_CONFIG_ENABLE_VERIFY*/

  return ret;
}

Flash_Err_Code DRV_FLASH_Read(void *src, void *dst, size_t size) {
  CHECK_PARAM(src)
  CHECK_PARAM(dst)

  if (validate_addr_range(src, size)) {
    return FLASH_ERROR_ADDRESS_RANGE;
  }

  // do not use origin value (SF_CMD_RESULT_TIME_OUT) for hibernation
  serial_flash_wait_device_not_busy(500);

  // clear prefetch buffer before read (for read after write)
  SERIAL_FLASH_CTRL_MCU_SUBSYS->CFG_SYSTEM = 0x3;

  memcpy(dst, src, size);
  return FLASH_ERROR_NONE;
}

Flash_Err_Code DRV_FLASH_Initialize(void) { return FLASH_ERROR_NONE; }

Flash_Err_Code DRV_FLASH_Uninitialize(void) { return FLASH_ERROR_NONE; }
