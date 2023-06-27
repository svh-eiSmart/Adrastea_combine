/* ---------------------------------------------------------------------------

(c) copyright 2019 Altair Semiconductor, Ltd. All rights reserved.

This software, in source or object form (the "Software"), is the
property of Altair Semiconductor Ltd. (the "Company") and/or its
licensors, which have all right, title and interest therein, You
may use the Software only in accordance with the terms of written
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

//#include <alt1250b_mac_exported.h>
#include <string.h>
#include <top_exported.h>
#include "serial_flash_drv_api_1250b.h"

#define CONFIG_SFLASH_WRITE_PROTECTION_ENABLE
//#define CONFIG_SFLASH_WRITE_DETECTION_ENABLE
#define SF_USE_PRIVATE_MEM_OPER_ENABLE
#define CONFIG_SFLASH_ENABLE_VERIFY

#define SFLASH_CONTROLLER_WRITE_PAGE_SIZE (0x100) /* 256 bytes controller's page size */

#define SF_SUBSYS_REGISTER(reg) \
  REGISTER(sf_subsys_base_address + (reg - BASE_ADDRESS_SERIAL_FLASH_CTRL_MCU_SUBSYS))

#define SFLASH_HALT_REQ                                                                       \
  {                                                                                           \
    REGISTER(MCU_SERIAL_FLASH_CTRL_MANUAL_HALT) |= MCU_SERIAL_FLASH_CTRL_MANUAL_HALT_SEL_MSK; \
    while (!(REGISTER(MCU_SERIAL_FLASH_CTRL_MANUAL_STATUS) &                                  \
             MCU_SERIAL_FLASH_CTRL_MANUAL_STATUS_HALT_ACTIVE_MSK))                            \
      ;                                                                                       \
  }

#define SFLASH_HALT_CLR                                                                        \
  {                                                                                            \
    REGISTER(MCU_SERIAL_FLASH_CTRL_MANUAL_HALT) &= ~MCU_SERIAL_FLASH_CTRL_MANUAL_HALT_SEL_MSK; \
    while ((REGISTER(MCU_SERIAL_FLASH_CTRL_MANUAL_STATUS) &                                    \
            MCU_SERIAL_FLASH_CTRL_MANUAL_STATUS_HALT_ACTIVE_MSK))                              \
      ;                                                                                        \
  }

/* Wait for the current command in the pipeline to complete */
#define SFLASH_STATUS_SM_WAIT                                \
  {                                                          \
    while (((REGISTER(SERIAL_FLASH_CTRL_MANUAL_STATUS) & \
             SERIAL_FLASH_CTRL_MANUAL_STATUS_SM_MSK) >>  \
            SERIAL_FLASH_CTRL_MANUAL_STATUS_SM_POS) &    \
           0xF)                                              \
      ;                                                      \
  }

#define SF_COMMAND_RESET_ENABLE 0x8066

#define SF_COMMAND_RESET_MEMORY 0x8099

#if !defined(SF_USE_PRIVATE_MEM_OPER_ENABLE)
#define sf_memcpy memcpy
#define sf_memcmp memcmp
#endif

static u32 sf_subsys_base_address;
static volatile int sf_is_busy = 0;
u32 g_sf_do_AND_for_write;  // PRODUCTS-19415 - for Fidelix need to do AND operation before writing
                            // to have "read-modify-write"

static int validate_addr_range(ulong addr, int len) {
  if (addr >= MCU_BASE_ADDR && addr < MCU_BASE_ADDR + MCU_PART_SIZE &&
      addr + len >= MCU_BASE_ADDR && addr + len < MCU_BASE_ADDR + MCU_PART_SIZE)  // success
    return 0;
  else {
    printf("%lx,%d\n", addr, len);
    return -1;
  }
}

static void SF_FUNC_ATTR sf_delay(unsigned long delay) {
  volatile unsigned long i, j, k = 0;

  while (delay--) {
    for (i = 0; i < 10; i++)
      for (j = 0; j < 10; j++) k += 3;
  }
}

#if defined(SF_USE_PRIVATE_MEM_OPER_ENABLE)
static int SF_FUNC_ATTR sf_memcmp(const void *cs, const void *ct, unsigned long count) {
  const unsigned char *su1, *su2;
  int res = 0;
  for (su1 = cs, su2 = ct; 0 < count; ++su1, ++su2, count--)
    if ((res = *su1 - *su2) != 0) break;
  return res;
}

static void *SF_FUNC_ATTR sf_memcpy(void *dst, const void *src, unsigned long count, u32 do_AND) {
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

inline int SF_FUNC_ATTR serial_flash_is_device_busy(void) {
  /*in case sf_is_busy=1 then we need to wait for status=1 which means operation is done*/
  if (sf_is_busy) {
#if 1
    /*int s = SF_SUBSYS_REGISTER(SERIAL_FLASH_CTRL_PMP_SUBSYS_ERROR_STATUS_RC);
    if(s)
            SFERR("%s %d: ERROR in status reg = %x\n", __FUNCTION__, __LINE__, s);*/
    sf_is_busy =
        (SF_SUBSYS_REGISTER(SERIAL_FLASH_CTRL_MCU_SUBSYS_STATUS_MCU_RC) & 0x01) ? 0 : 1;

    // in case state changed from busy to not-busy then clear prefetch buffer for next operation
    /*if(!sf_is_busy) {
            //clear NET prefetch
            SF_SUBSYS_REGISTER(SERIAL_FLASH_CTRL_PMP_SUBSYS_CFG_SYSTEM) =
    0xf;//SERIAL_FLASH_CTRL_MODEM_SUBSYS_CFG_SYSTEM_CLEAR_PREFETCH_NET_PULSE_MSK;
    }*/
#else
    int i = SF_SUBSYS_REGISTER(MCU_SERIAL_FLASH_CTRL_MCU_SUBSYS_STATUS_PMP_RC);
    sf_is_busy = i ? 0 : 1;
    int s = SF_SUBSYS_REGISTER(MCU_SERIAL_FLASH_CTRL_MCU_SUBSYS_ERROR_STATUS_RC);
    printf("%s: status=%x, busy=%d, error status=%d\n", i, sf_is_busy, s);
#endif
  }

  return sf_is_busy;
}

void SF_FUNC_ATTR serial_flash_wait_device_not_busy(unsigned long timeout) {
  // printf("serial_flash_wait_device_not_busy Entered (current=%d)\n", sf_is_busy);
  while (serial_flash_is_device_busy()) {
    sf_delay(1);
    if (--timeout == 0) {
      SFERR("ERROR - flash device is busy (timeout=%lx)\n", timeout);
    }
  }

  // printf("serial_flash_wait_device_not_busy Exited: %ld\n", us_timeout);
}

#if defined(CONFIG_SFLASH_ENABLE_VERIFY)
int SF_FUNC_ATTR serial_flash_verify_buffer(unsigned char *src, unsigned long addr,
                                            unsigned long len) {
  serial_flash_wait_device_not_busy(
      SF_CMD_RESULT_TIME_OUT);  // wait for previous operation to finish
  // sf_delay(10);

  // clear prefetch buffer before verify
  SF_SUBSYS_REGISTER(SERIAL_FLASH_CTRL_MCU_SUBSYS_CFG_SYSTEM) =
      0xf;  // SERIAL_FLASH_CTRL_MODEM_SUBSYS_CFG_SYSTEM_CLEAR_PREFETCH_NET_PULSE_MSK;
#if 1
  /****** need to compare timing memcmp/sf_memcmp using other cpu ram counters****/
  int cmp = sf_memcmp(src, (void *)addr, len);
  if (cmp) {
    printf("RE-verify addr=0x%lx (%x)\n", addr, cmp);

    sf_delay(100);

    cmp = sf_memcmp(src, (void *)addr, len);
    if (cmp) {
      SFERR("%s #%d: Verify Failed - src=0x%x, addr=0x%lx, len=%ld\n", __FUNCTION__, __LINE__,
            (unsigned int)src, addr, len);
      return 1;
    }
  }

  return 0;

#else
            // printf("serial_flash_write_page 10\n");
  // int cmp = sf_memcmp(src, (void *)addr, len);
  if (1) {  // cmp) {
#if 0
			SFERR("%s #%d: Verify Failed - src=0x%x, addr=0x%lx, len=%ld\n", __FUNCTION__, __LINE__,
					(unsigned int)src, addr, len);

			//SFERR("%s #%d: Flash verify operation failed! [src=%x, dst=%x, size=%d]\n",
				//	__FUNCTION__, __LINE__, (u32)orig_from, (u32)orig_to, cmp_size);

#else  // if specific error indication required
    int i;
    for (i = 0; i < len; i++) {
      if (*(((volatile char *)src) + i) != *(((volatile char *)addr) + i)) {
#if 0
					SFERR("%s #%d: verify fail at location 0x%x [%d], src=%x, dst=%x\n",
							__FUNCTION__, __LINE__, i, i, *(((char *)src)+i), *(((char *)addr)+i));
#else
        int s = *(((char *)src) + i);
        int d = *(((char *)addr) + i);
        int t = 0;
        while (*(((volatile char *)src) + i) != *(((volatile char *)addr) + i)) {
          t++;
        }
        SFERR("%s #%d: verify fail at location 0x%x [%d], src=%x, dst=%x, t=%d\n", __FUNCTION__,
              __LINE__, i, i, s, d, t);

        // return 1;
        // break;
#endif
        return 0;  // for debug
      }
    }
#endif
  }
  return 0;
#endif
}
#endif /*CONFIG_SFLASH_ENABLE_VERIFY*/

int SF_FUNC_ATTR serial_flash_erase_sector(ulong addr, int is_64KB_sect, int wait_for_finish) {
#if 1  // USE AUTOMATIC METHOD

  u32 erase_cmd;
  u32 flash_addr = (u32)(addr & ~CONFIG_SYS_FLASH_BASE);

  if (validate_addr_range(addr, 0)) {
    printf("Address range is outside of MCU partition\n");
    return -1;
  }

  // clang-format off
  // Deter compiler from unwanted optimization on linker script symbols.
  // https://mcuoneclipse.com/2016/11/01/getting-the-memory-range-of-sections-with-gnu-linker-files/
  unsigned int *volatile p = (unsigned int *volatile) &__MCU_MTD_PARTITION_OFFSET__;  // clang-format on
  if (p == 0x00000000) {
#define ALT1250B_SF_OFFSET_STEP (0x4000)
    u32 sf_offset = REGISTER(TOPREG(SERIAL_FLASH_CTRL_SECURITY_SFR_SECURED_CONID2_OFFSET));
    sf_offset *= ALT1250B_SF_OFFSET_STEP;
    sf_offset += MCU_BASE_ADDR;
    flash_addr += sf_offset;
  }

  serial_flash_wait_device_not_busy(SF_ERASE_SECTOR_TIME_OUT);
  sf_is_busy = 1;

#if defined(CONFIG_SFLASH_WRITE_PROTECTION_ENABLE)
  // remove write protection to allow write/erase
  SF_SUBSYS_REGISTER(SERIAL_FLASH_CTRL_MCU_SUBSYS_WRITE_PROTECT) = 0;
#endif

  /*erase_cmd = ((flash_addr >> 2) << (SERIAL_FLASH_CTRL_PMP_SUBSYS_SFC_ERASE_CFG_ADDR_POS+ 0)) |
     //address is 4bytes aligned
                                  ((is_64KB_sect ? 1 : 0) <<
     SERIAL_FLASH_CTRL_PMP_SUBSYS_SFC_ERASE_CFG_OPCODE_SEL_POS) | 1;*/
  /*addr is in u32 units (div by 4), and need to shift left by 8, so in total we shift left only 6*/
  erase_cmd =
      (flash_addr << (SERIAL_FLASH_CTRL_MCU_SUBSYS_SFC_ERASE_CFG_ADDR_POS -
                      2)) |  // address is 4bytes aligned
      ((is_64KB_sect ? 1 : 0) << SERIAL_FLASH_CTRL_MCU_SUBSYS_SFC_ERASE_CFG_OPCODE_SEL_POS) |
      1;

  SFDBG(3,
        "%s #%d: addr=%08lX [flash_addr=%x], is_64KB_sect=%d, wait_for_finish=%d, erase_cmd=%x\n",
        __FUNCTION__, __LINE__, addr, flash_addr, is_64KB_sect, wait_for_finish, erase_cmd);

  SF_SUBSYS_REGISTER(SERIAL_FLASH_CTRL_MCU_SUBSYS_SFC_ERASE_CFG) = erase_cmd;

#if defined(CONFIG_SFLASH_WRITE_PROTECTION_ENABLE)

#if 1
  // reading from the register should be enough to make sure command passed the firewall since it is
  // serialized
  erase_cmd = SF_SUBSYS_REGISTER(SERIAL_FLASH_CTRL_MCU_SUBSYS_SFC_ERASE_CFG);
#else
  // wait until erase sent from internal queue to pass firewall (should be immediate, waiting
  // execution/pending)
  while (!(SF_SUBSYS_REGISTER(MCU_SERIAL_FLASH_CTRL_MCU_SUBSYS_STATUS_PMP) &
           (MCU_SERIAL_FLASH_CTRL_MCU_SUBSYS_STATUS_PMP_ERASE_EXECUTING_MSK |
            MCU_SERIAL_FLASH_CTRL_MCU_SUBSYS_STATUS_PMP_ERASE_PENDING_TO_ARB_MSK)))
    ;
#endif
  // enable back the write protection
  SF_SUBSYS_REGISTER(SERIAL_FLASH_CTRL_MCU_SUBSYS_WRITE_PROTECT) = 1;
#endif

  if (wait_for_finish) serial_flash_wait_device_not_busy(SF_ERASE_SECTOR_TIME_OUT);

  return 0;

#else  // MANUAL METHOD

  // u32 cust_cfg_orig=0;
  // u32 top=0;
  u32 dev_num = 0;
  int err = 0;
  // int cs_addr;
  // u32 cs_reg = SF_SUBSYS_REGISTER(SERIAL_FLASH_CTRL_CS_SEL);

  SFDBG(3, "%s #%d: addr=%08lX, is_64KB_sect=%d, wait_for_finish=%d\n", __FUNCTION__, __LINE__,
        addr, is_64KB_sect, wait_for_finish);
  // SFLASH_PRIORITY_HALT_REQ;
  SFLASH_HALT_REQ;

  // if in quad mode - change to Single SPI mode temporarily
  if (sf_db.devices[dev_num].use_quad_mode) {
    u32 tmp;

    cust_cfg_orig = SF_SUBSYS_REGISTER(MCU_SERIAL_FLASH_CTRL_CUSTOM_CFG);
    tmp = cust_cfg_orig;

    tmp &= ~(MCU_SERIAL_FLASH_CTRL_CUSTOM_CFG_DATA_WIDTH_MSK |
             MCU_SERIAL_FLASH_CTRL_CUSTOM_CFG_ADDR_WIDTH_MSK |
             MCU_SERIAL_FLASH_CTRL_CUSTOM_CFG_OPCODE_WIDTH_MSK |
             MCU_SERIAL_FLASH_CTRL_CUSTOM_CFG_DUMMY_CYCLE_COUNT_MSK |
             MCU_SERIAL_FLASH_CTRL_CUSTOM_CFG_DUMMY_PATTERN_SIZE_MSK);
    tmp |= (  // normal SPI mode
        (0 << MCU_SERIAL_FLASH_CTRL_CUSTOM_CFG_DATA_WIDTH_POS) |
        (0 << MCU_SERIAL_FLASH_CTRL_CUSTOM_CFG_ADDR_WIDTH_POS) |
        (0 << MCU_SERIAL_FLASH_CTRL_CUSTOM_CFG_OPCODE_WIDTH_POS) |
        (8 << MCU_SERIAL_FLASH_CTRL_CUSTOM_CFG_DUMMY_CYCLE_COUNT_POS) |
        (8 << MCU_SERIAL_FLASH_CTRL_CUSTOM_CFG_DUMMY_PATTERN_SIZE_POS));
    SF_SUBSYS_REGISTER(MCU_SERIAL_FLASH_CTRL_CUSTOM_CFG) = tmp;
  }

  // YG: FIXME - need to find n by addr - add cross-addr to sf_db, if addr>=cross-addr then second
  // device
  if (sf_db.devices[0].use_32bit_addr && ((addr & ~CONFIG_SYS_FLASH_BASE) >= _64M)) {
    u32 offset = (addr & ~CONFIG_SYS_FLASH_BASE) & 0xFC000000;

    top = 1;
    // addr -= FLASH_SIZE_16MB;//0x1000000;

    // printf("Erase above 16Mbyte - config offset (%x, %x)\n", (u32)addr, offset);
    SF_SUBSYS_REGISTER(MCU_SERIAL_FLASH_CTRL_ADDR_OFFSET) = offset;
  }

  SFLASH_SEND_WRITE_ENABLE(dev_num);
  SFLASH_STATUS_SM_WAIT

  err = serial_flash_poll_wip(SF_ERASE_SECTOR_TIME_OUT,
                              (SFLASH_STATUS_REG_WEL | SFLASH_STATUS_REG_RDY));
  if (err) printf("%s:%d: SFLASH Write Enable Operation failure\n", __func__, __LINE__);

  SF_SUBSYS_REGISTER(MCU_SERIAL_FLASH_CTRL_MANUAL_CMD_ADDR) = (addr & ~CONFIG_SYS_FLASH_BASE);

#if 0
	//SF_SUBSYS_REGISTER(SERIAL_FLASH_CTRL_CMD) = is_64KB_sect ? SF_COMMAND_ERASE_SECTOR : SF_COMMAND_4KB_ERASE_SECTOR;
	SF_SUBSYS_REGISTER(MCU_SERIAL_FLASH_CTRL_CMD) = SF_COMMAND_ERASE_SECTOR;// : SF_COMMAND_4KB_ERASE_SECTOR;
#else
  if (sf_db.devices[0].use_32bit_addr) {
    // flash size bigger than 16MB - requires 32 bit address erase command
    // printf("erase flash size BIGGER 0x1000000\n");
    SF_SUBSYS_REGISTER(MCU_SERIAL_FLASH_CTRL_MANUAL_CMD) =
        is_64KB_sect ? SF_COMMAND_ERASE_64KB_SECTOR_32BIT : SF_COMMAND_ERASE_4KB_SECTOR_32BIT;
  } else {
    // flash size up to 16MB - requires 24 bit address erase command
    // printf("erase flash size <= 0x1000000\n");
    SF_SUBSYS_REGISTER(MCU_SERIAL_FLASH_CTRL_MANUAL_CMD) =
        is_64KB_sect ? SF_COMMAND_ERASE_64KB_SECTOR : SF_COMMAND_ERASE_4KB_SECTOR;
  }
#endif

  SFLASH_STATUS_SM_WAIT

  /*if required - poll for finish*/
  if (wait_for_finish) {
    err = serial_flash_poll_wip(SF_ERASE_SECTOR_TIME_OUT, SFLASH_STATUS_REG_RDY);
    SFLASH_STATUS_SM_WAIT
    if (err) printf("%s:%d: SFLASH Erase Operation failure\n", __func__, __LINE__);
  }

  if (top) {
    // printf("Erase above 16Mbyte - restore offset\n");
    SF_SUBSYS_REGISTER(MCU_SERIAL_FLASH_CTRL_ADDR_OFFSET) = 0;
  }
  // restore quad mode on exit
  if (sf_db.devices[dev_num].use_quad_mode) {
    SF_SUBSYS_REGISTER(MCU_SERIAL_FLASH_CTRL_CUSTOM_CFG) = cust_cfg_orig;
  }

#if 0
	if(cs_addr) {
		/* Restore the CS control register */
		SF_SUBSYS_REGISTER(MCU_SERIAL_FLASH_CTRL_CS_SEL) = serial_flash_ctrl_cs_sel_reg;
	}
#endif

  // SFLASH_PRIORITY_HALT_CLR;
  SFLASH_HALT_CLR;

  return err;

#endif /*if 1 - USE AUTOMATIC METHOD*/
}

static int SF_FUNC_ATTR serial_flash_write_page(volatile ulong *from, volatile ulong *to,
                                                ulong size, int verify) {
#if defined(CONFIG_SFLASH_ENABLE_VERIFY)
  // ulong *orig_from = (ulong *)from;
  // ulong *orig_to = (ulong *)to;
  // ulong *orig_to = (ulong *)((ulong)(to) & ~(K_CALG_UNCACHED << 28)); /* Convert to cached
  // address to accelerate the verify time */ int cmp_size = size;
#endif

  SFDBG(3, "%s #%d: from=%x, to=%x, size=%ld\n", __FUNCTION__, __LINE__, (u32)from, (u32)to, size);

  serial_flash_wait_device_not_busy(SF_CMD_RESULT_TIME_OUT);
  sf_is_busy = 1;
  /* Use a dummy read to workaround the page write problem - TODO: check if still required */
  sf_memcmp((char *)(to), (char *)from, 1);
#if defined(CONFIG_SFLASH_WRITE_PROTECTION_ENABLE)
  // remove write protection to allow write/erase
  SF_SUBSYS_REGISTER(SERIAL_FLASH_CTRL_MCU_SUBSYS_WRITE_PROTECT) = 0;
#endif

  /* write 'size' to counter register:
   * hardware requires size to be in longs (32bit "words", 4 bytes) - lowest 2 bits are zero's
   * in case size is not aligned 4 bytes -> aligned the size to upper 4 bytes (write 0xff in
   * trailing) alignment is done by adding 3 and zeroing the lowest 2 bits (1,2,3,4 -> 4; 5,6,7,8 ->
   * 8; etc.) Jira PRODUCTS-15202: in case dst address is not aligned, need to add the prefix bytes
   * to the write counter, since they are written as 4 bytes
   */
  SF_SUBSYS_REGISTER(SERIAL_FLASH_CTRL_MCU_SUBSYS_WRITE_COUNT) =
      ((size + ((ulong)(to)&0x3) + 3) & ~0x3) |
      SERIAL_FLASH_CTRL_MCU_SUBSYS_WRITE_COUNT_COUNT_CLR_MSK;

  /* Test whether prefix alignment is required */
  if ((ulong)(to)&0x3) {
    ulong *aligned_to = (ulong *)((ulong)(to)&0xFFFFFFFC);
    ulong aligned_value = *aligned_to;
    ulong len = (4 - ((ulong)(to)&0x3));

    if (size < len) {
      len = size;
    }

    sf_memcpy((((char *)&aligned_value) + ((ulong)(to)&0x3)), (const void *)from, len,
              g_sf_do_AND_for_write);
    REGISTER(aligned_to) = aligned_value;
    to = ++aligned_to;
    from = (ulong *)(((char *)from) + len);
    size -= len;
  }
  /* copy all, except trailing (if not aligned to 4) */
  while (size >= 4) {
    ulong val;
    SFDBG(3, "%s #%d: size = %lu\n", __FUNCTION__, __LINE__, size);
    // if need AND - read current content and handle it
    if (g_sf_do_AND_for_write) {
      val = REGISTER(to);
      sf_memcpy(&val, (const void *)from, 4, 1);
    } else {
      // normal handling (no AND)
      if (!((ulong)(from)&0x3)) /* Aligned source address - copy by 4's */
        val = *from;
      else
        /* Unaligned source address - copy by 1's */
        sf_memcpy(&val, (const void *)from, sizeof(val), 0);
    }
    REGISTER(to) = val;

    ++from;
    ++to;
    size -= 4;
  }
  /* Test whether suffix alignment is required */
  if (size) {
    /* copy remaining - if not aligned to 4 */
    ulong val = 0xffffffff;
    // if need AND - read current content
    if (g_sf_do_AND_for_write) val = REGISTER(to);
    sf_memcpy(&val, (const void *)from, size, g_sf_do_AND_for_write);
    REGISTER(to) = val;
  }

#if defined(CONFIG_SFLASH_WRITE_PROTECTION_ENABLE)
  // wait until write sent from internal queue to pass firewall (should be immediate, waiting
  // execution/pending)
  int timeout = 10000;
  while (!(SF_SUBSYS_REGISTER(SERIAL_FLASH_CTRL_MCU_SUBSYS_STATUS_MCU) &
           (SERIAL_FLASH_CTRL_MCU_SUBSYS_STATUS_MCU_WRITE_EXECUTING_MSK |
            SERIAL_FLASH_CTRL_MCU_SUBSYS_STATUS_MCU_WRITE_PENDING_TO_ARB_MSK |
            SERIAL_FLASH_CTRL_MCU_SUBSYS_STATUS_MCU_INT_STAT_MSK)) &&
         --timeout != 0) {
    if (timeout % 1000 == 0) SFDBG(3, "%s #%d: TIMEOUT=%d \n", __FUNCTION__, __LINE__, timeout);
  }

  // enable back the write protection
  SF_SUBSYS_REGISTER(SERIAL_FLASH_CTRL_MCU_SUBSYS_WRITE_PROTECT) = 1;

  if (timeout == 0) {
    SFDBG(3, "%s #%d: TIMEOUT \n", __FUNCTION__, __LINE__);
    return -1;
  }

#endif

  return 0;
}

int serial_flash_write_buffer(unsigned char *src, ulong addr, ulong len, int verify) {
  int err = 0;
  ulong size;
  unsigned char *orig_src = src;
  ulong orig_addr = addr;
  ulong orig_len = len;

  SFDBG(3, "%s #%d: src=%x, addr=%x, cnt=%ld\n", __FUNCTION__, __LINE__, (u32)src, (u32)addr, len);

  if (validate_addr_range(addr, len)) {
    SFERR("Address range is outside of MCU partition\n");
    return -1;
  }
  serial_flash_wait_device_not_busy(SF_CMD_RESULT_TIME_OUT);

#if defined(CONFIG_SFLASH_WRITE_DETECTION_ENABLE) || defined(CONFIG_SFLASH_WRITE_PROTECTION_ENABLE)
  // check if sporadic write occurred by checking counter value
  if (SF_SUBSYS_REGISTER(SERIAL_FLASH_CTRL_MCU_SUBSYS_WRITE_COUNT) &
      SERIAL_FLASH_CTRL_MCU_SUBSYS_WRITE_COUNT_IN_COUNTER_MSK) {
    SFERR("ERROR: Serial Flash Sporadic Write Detected (count=%lu)!\n",
          (SF_SUBSYS_REGISTER(SERIAL_FLASH_CTRL_MCU_SUBSYS_WRITE_COUNT) &
           SERIAL_FLASH_CTRL_MCU_SUBSYS_WRITE_COUNT_IN_COUNTER_MSK) >>
              SERIAL_FLASH_CTRL_MCU_SUBSYS_WRITE_COUNT_IN_COUNTER_POS);

    // clear counter
    SF_SUBSYS_REGISTER(SERIAL_FLASH_CTRL_MCU_SUBSYS_WRITE_COUNT) =
        SERIAL_FLASH_CTRL_MCU_SUBSYS_WRITE_COUNT_COUNT_CLR_MSK;
  }
#endif

  /*dummy read to clean up pp error*/
  size = SF_SUBSYS_REGISTER(SERIAL_FLASH_CTRL_MCU_SUBSYS_ERROR_STATUS_RC);
  if (size) SFERR("%s #%d: ERROR status reg=%lx\n", __FUNCTION__, __LINE__, size);

  while (len && !err) {
    size = SFLASH_CONTROLLER_WRITE_PAGE_SIZE - addr % SFLASH_CONTROLLER_WRITE_PAGE_SIZE;
    if (size > len) size = len;
    SFDBG(4, "%s #%d: src=0x%x, addr=0x%lx, size=%ld\n", __FUNCTION__, __LINE__, (u32)src, addr,
          size);
    err = serial_flash_write_page((volatile ulong *)src, (volatile ulong *)addr, size, verify);
    if (err) {
      SFERR("%s #%d: Error in write src=0x%x, addr=0x%lx, size=%ld, err=%d\n", __FUNCTION__,
            __LINE__, (u32)src, addr, size, err);
      break;
    }

    src += size;
    addr += size;
    len -= size;
  }

#if defined(CONFIG_SFLASH_WRITE_DETECTION_ENABLE) || defined(CONFIG_SFLASH_WRITE_PROTECTION_ENABLE)
  // write MAX size to counter register - size is in longs (32bit "words") (wait is already done in
  // write_page)
  SF_SUBSYS_REGISTER(SERIAL_FLASH_CTRL_MCU_SUBSYS_WRITE_COUNT) =
      256 & SERIAL_FLASH_CTRL_MCU_SUBSYS_WRITE_COUNT_VAL_MSK;
#endif

#if defined(CONFIG_SFLASH_ENABLE_VERIFY)
  if (verify) {
    if (serial_flash_verify_buffer(orig_src, orig_addr, orig_len)) {
      SFERR("%s #%d: Flash verify operation failed! [src=%x, dst=%x, size=%d]\n", __FUNCTION__,
            __LINE__, (u32)orig_src, (u32)orig_addr, (u32)orig_len);
      return 1;
    }
  }
#endif /*SFLASH_CONFIG_ENABLE_VERIFY*/

  return err;
}

void SF_FUNC_ATTR serial_flash_read(volatile ulong *from, volatile ulong *to, ulong size) {
  serial_flash_wait_device_not_busy(SF_CMD_RESULT_TIME_OUT);

  // clear prefetch buffer before read (for read after write)
  SF_SUBSYS_REGISTER(SERIAL_FLASH_CTRL_MCU_SUBSYS_CFG_SYSTEM) =
      0xf;  // SERIAL_FLASH_CTRL_MODEM_SUBSYS_CFG_SYSTEM_CLEAR_PREFETCH_NET_PULSE_MSK;

  /*maybe need to check alignment of src/dst/length and do it based on alignment*/
  memcpy((void *)to, (void *)from, size);
}

void SF_FUNC_ATTR serial_flash_driver_wait_for_transaction_finish(void) {
  serial_flash_wait_device_not_busy(SF_CMD_RESULT_TIME_OUT);
}

void SF_FUNC_ATTR serial_flash_reset_command(void) {
  /* To reset the device, the RESET ENABLE command must be followed by the RESET MEMORY command */
  // SFLASH_HALT_REQ;

  REGISTER(SERIAL_FLASH_CTRL_MANUAL_CMD) = SF_COMMAND_RESET_ENABLE;
  SFLASH_STATUS_SM_WAIT

  sf_delay(100);

  REGISTER(SERIAL_FLASH_CTRL_MANUAL_CMD) = SF_COMMAND_RESET_MEMORY;
  SFLASH_STATUS_SM_WAIT
  sf_delay(100);

  // SFLASH_HALT_CLR;
}

void serial_flash_api_init(u32 subsys_base_address) {
  sf_subsys_base_address = subsys_base_address;
  g_sf_do_AND_for_write = 1;
}
