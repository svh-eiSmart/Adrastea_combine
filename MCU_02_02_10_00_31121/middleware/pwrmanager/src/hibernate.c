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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "alt_osal.h"
#include "fastlz.h"
#include "hibernate.h"
#if (PWR_MNGR_EN_HIBERNATE_SHUTDOWN == 1)
#include "DRV_COMMON.h"
#include "DRV_FLASH.h"
#include "DRV_PM.h"
#endif
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* debug level: 0:critical; 1: warning; 2:information; 3: debug */
#define HBN_LOG_LEVEL 3
#if defined(HBN_ENABLE_DEBUG_PRINTS)
#define HBN_LOG(lEVEL, fmt, args...)                 \
  do {                                               \
    if (lEVEL <= HBN_LOG_LEVEL) printf(fmt, ##args); \
  } while (0)
#else
#define HBN_LOG(lEVEL, fmt, args...)
#endif

/* Size of decompressed blocks used for compression.  Must be a power of 2. */
#define WORKING_BLOCK_SIZE 4096

/* Maximum size of a compressed block.  This can be larger than a decompressed block in worst case
where there's 0 compression and everything is stored as literals */
#define MAX_COMPRESSED_BLOCK_SIZE \
  (WORKING_BLOCK_SIZE + (WORKING_BLOCK_SIZE / 32) + (sizeof(size_t) * 2))

#define MAX_TASKS 16

#if (PWR_MNGR_EN_HIBERNATE_SHUTDOWN == 1)
#define MCU_PARTITION_OFFSET MCU_BASE_ADDR
#define MCU_PARTITION_SIZE MCU_PART_SIZE
#define MCU_PART_IMAGE_SZ_OFFSET (MCU_BASE_ADDR + 0x4)
#endif
/****************************************************************************
 * External Symbols
 ****************************************************************************/

/* Linker provided memory regions */
extern char __gpm_compress_start__[], __gpm_compress_end__[], __data_start__[], __bss_end__[],
    __HeapBase0[], __HeapLimit0[], __HeapBase[], __HeapLimit[];

extern void get_unused_heap_mem(struct hibernate_free_block **freeListPtr);

#if (PWR_MNGR_EN_HIBERNATE_SHUTDOWN == 1)
extern char __bss_gpm_start__[];
#endif
/****************************************************************************
 * Private Types
 ****************************************************************************/
/* Memory regions to compress */

static const struct {
  char *start;
  char *end;
} MemRegions[] = {
    /* DO NOT CHANGE THE ORDER OF REGION, IN-PLACE COMPRESSION OF GPM */
    {.start = __HeapBase, .end = __HeapLimit},     /* GPM RESERVED heap region */
    {.start = __data_start__, .end = __bss_end__}, /* .data/.bss region */
    {.start = __HeapBase0, .end = __HeapLimit0},   /* SRAM heap region */
};

/* End of compressed memory after compression is complete. */
UNCOMPRESSED void *EndOfCompressedMem;

#if (PWR_MNGR_EN_HIBERNATE_SHUTDOWN == 1)
UNCOMPRESSED uint32_t retain_flash_offset;

typedef struct {
  uint32_t magic_num;            
  uint32_t size;              
  uint32_t reserved1;
  uint32_t reserved2;
} retain_flash_hdr;
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/**
 * Get a list of all current tasks and stack sizes.
 */
static void GetFreeStack(struct hibernate_free_block **freeListPtr) {
  alt_osal_task_status taskStatus[MAX_TASKS];
  uint32_t i, activeTasks;

  activeTasks = alt_osal_get_tasks_status(taskStatus, MAX_TASKS);
  HBN_LOG(2, "Active task number: %lu\n", activeTasks);

  for (i = 0; i < activeTasks; ++i) {
    size_t freeStackSz;
    StackType_t *topOfStack;
    struct hibernate_free_block *freeBlockPtr;

    if (taskStatus[i].state == ALT_OSAL_TASK_RUNNING) {
      /* Current task is using its own stack; skip. */
      continue;
    }

    /* Top of stack is always the first element of the TCB. */
    topOfStack = *(alt_osal_stack_type **)(taskStatus[i].handle);

    freeStackSz = (uint8_t *)topOfStack - (uint8_t *)taskStatus[i].stack_base;

    if (freeStackSz < sizeof(struct hibernate_free_block)) {
      /* Not enough free stack to store a free stack record.  Concerning... */
      HBN_LOG(0, "Free stack size insufficient: %zu\n", freeStackSz);
      continue;
    }

    freeBlockPtr = (struct hibernate_free_block *)taskStatus[i].stack_base;
    freeBlockPtr->start = (void *)freeBlockPtr;
    freeBlockPtr->next = *freeListPtr;
    freeBlockPtr->size = freeStackSz;
    freeBlockPtr->type = BLOCK_TCB;

    *freeListPtr = freeBlockPtr;
  }
}

/**
 * Erase the free part of the stack which was used to store the free block ptr.
 */
static void RestoreFreeStack(void) {
  alt_osal_task_status taskStatus[MAX_TASKS];
  uint32_t i, activeTasks;

  activeTasks = alt_osal_get_tasks_status(taskStatus, MAX_TASKS);

  for (i = 0; i < activeTasks; ++i) {
    size_t freeStackSz;
    alt_osal_stack_type *topOfStack;

    if (taskStatus[i].state == ALT_OSAL_TASK_RUNNING) {
      /* Current task is using its own stack; skip. */
      continue;
    }

    /* Top of stack is always the first element of the TCB. */
    topOfStack = *(alt_osal_stack_type **)(taskStatus[i].handle);

    freeStackSz = (uint8_t *)topOfStack - (uint8_t *)taskStatus[i].stack_base;

    memset(taskStatus[i].stack_base, 0xa5, freeStackSz);
  }
}

/**
 * Get the list of all free memory across the system.
 *
 * This memory does not need to be preserved across hibernation.
 */
static void GetFreeList(struct hibernate_free_block **freeListPtr) {
  /* Initialize the list to empty */
  *freeListPtr = NULL;

  GetFreeStack(freeListPtr);
  get_unused_heap_mem(freeListPtr);
}

/**
 * Zero all unused memory, so it compresses easier.
 *
 * Some prototyping was done to avoid passing unused memory to the compressor at all, but this was
 * found to be slower in general.
 */
static void ZeroFreeMem(struct hibernate_free_block *freeBlockList) {
  struct hibernate_free_block *nextFreeBlock;

  while (1) {
    if (!freeBlockList) {
      break;
    }

    /* Store the next free block */
    nextFreeBlock = freeBlockList->next;

    if (freeBlockList->type == BLOCK_TCB) {
      /* Set 0xA5 TCB block(unused task stack) */
      memset(freeBlockList->start, 0xA5, freeBlockList->size);
    } else {
      /* And zero the contents of this block */
      memset(freeBlockList->start, 0, freeBlockList->size);
    }

    /* And move on to the next block */
    freeBlockList = nextFreeBlock;
  }
}

/**
 * Decompress a (potentially) partially compressed memory space back into its original location.
 *
 * As memory is compressed into the beginning of GPM, it needs to be decompressed starting at the
 * end of compressed GPM memory
 */
static int DecompressPartial(int region,    /* Last region which was compressed */
                             char *dstAddr, /* End of the region to decompress into */
                             char *srcAddr  /* End of the compressed region to decompress */
) {
  do {
    while (dstAddr > MemRegions[region].start) {
      size_t srcSize, dstSize;

      /* First read the size of the compressed region */
      srcAddr -= sizeof(size_t);
      memcpy(&dstSize, srcAddr, sizeof(size_t));
      srcAddr -= sizeof(size_t);
      memcpy(&srcSize, srcAddr, sizeof(size_t));

      dstAddr -= dstSize;
      srcAddr -= srcSize;

      /* Should always be able to reconstruct original memory.  If we can't, only
      choice is to reset, as memory currently resembles a toddler's art project */
      if (fastlz_decompress_internal(srcAddr, srcSize, dstAddr, dstSize) == 0) {
        return -1;
      }
    }

    /* Front of the region -- go to the next (previous?) region */
    if (region--) {
      dstAddr = MemRegions[region].end;
    } else {
      /* Done */
      break;
    }
  } while (1);
  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
#if (PWR_MNGR_EN_HIBERNATE_SHUTDOWN == 1)
int32_t hibernate_to_flash(void) {
  int32_t ret = 0;
  uint32_t start = retain_flash_offset, *dstFlashAddr;
  size_t retainedSize = 0;
  char *dstGpmAddr = __bss_gpm_start__;
  char *dstAddr = EndOfCompressedMem;
  Flash_Err_Code ret_val;

  if(start == 0) {
    printf("\n\r wrong mcu flash offset");
    return (-1);
  }

  dstFlashAddr = (uint32_t *)start;
  retainedSize = (size_t)(dstAddr - dstGpmAddr);

  //write header
  retain_flash_hdr retain_hdr;
  memset(&retain_hdr, 0x0, sizeof(retain_flash_hdr));
  retain_hdr.magic_num = 0xdeadbeef;
  retain_hdr.size = retainedSize;

  ret_val = DRV_FLASH_Write(&retain_hdr, dstFlashAddr, sizeof(retain_flash_hdr), 0);
  if(ret_val != FLASH_ERROR_NONE) {// not verify
    printf("\n\r failed to write flash - %d", ret_val);
    return (ret_val);  
  }

  //write data
  dstFlashAddr = (uint32_t *)(start+sizeof(retain_flash_hdr));
  ret_val = DRV_FLASH_Write(dstGpmAddr, dstFlashAddr, retainedSize, 0);
  if(ret_val != FLASH_ERROR_NONE) {// not verify
    printf("\n\r failed to write flash - %d", ret_val);
    return (ret_val);  
  }

  int i = 20000;
  while (i >= 1) {
    i--;
  }

  return ret;
}

int32_t hibernate_from_flash(void) {
  uint32_t start = retain_flash_offset, *dstFlashAddr;
  size_t retainedSize = 0;
  char *dstGpmAddr = __bss_gpm_start__;

  //read the header
  dstFlashAddr = (uint32_t *)start;
  if (dstFlashAddr[0] != 0xdeadbeef) return (-2);

  retainedSize = (size_t)dstFlashAddr[1];

  dstFlashAddr = (uint32_t *)(start+sizeof(retain_flash_hdr));

  if (DRV_FLASH_Read(dstFlashAddr, dstGpmAddr, retainedSize) != FLASH_ERROR_NONE) {
    printf("\n\rFailed to Read flash!!");
    return (-3);
  }

  return 0;
}

uint32_t hibernate_get_flash_offset(void) {
  return retain_flash_offset;
}

void hibernate_dbg_dump(void) {
#if (PWR_MNGR_EN_HIBERNATE_SHUTDOWN == 1)
  printf("Partition offset: %lx\r\n", hibernate_get_flash_offset());
#endif
}

int32_t hibernate_prepare_flash_space(size_t retained_size) {
  uint32_t *dstFlashAddr, *probeAddr, asked_sz = retained_size, flash_offset_64KB, flash_offset_4KB, dbg_offset = 0; 
  uint32_t mcu_image_sz_offset = MCU_PART_IMAGE_SZ_OFFSET, *mcu_image_sz = 0, available_sz = 0;

  mcu_image_sz = (uint32_t *)mcu_image_sz_offset;
  retain_flash_offset = 0;

  // check remaining flash size is enough for rentention
  available_sz = MCU_PARTITION_SIZE - *mcu_image_sz - retained_size - 0x1000;
  if(available_sz < retained_size) {
    HBN_LOG(1, "flash size is not enough");
    return (-1);  
  }

  if(*mcu_image_sz == 0) 
    return (-2);

  flash_offset_64KB = ((*mcu_image_sz/65536) + 1)*65536 + MCU_PARTITION_OFFSET; //64KB alignment for 64KB erase
  flash_offset_4KB = ((*mcu_image_sz/4096) + 1)*4096 + MCU_PARTITION_OFFSET; //4KB alignment for 4KB erase

  if(available_sz >= 0x20000) {
    retain_flash_offset = flash_offset_64KB;
    dstFlashAddr = (uint32_t *)retain_flash_offset;
    available_sz -= ((*mcu_image_sz/65536) + 1)*65536 - *mcu_image_sz;

    //erase previous header if necessary
    probeAddr = (uint32_t *)flash_offset_4KB;
    if(*probeAddr == 0xdeadbeef) { DRV_FLASH_Erase_Sector(probeAddr, 0, 1); }
  } else {
    retain_flash_offset = flash_offset_4KB; 
    dstFlashAddr = (uint32_t *)retain_flash_offset;
    available_sz -= ((*mcu_image_sz/4096) + 1)*4096 - *mcu_image_sz;

    //erase previous header if necessary
    probeAddr = (uint32_t *)flash_offset_64KB;
    if(*probeAddr == 0xdeadbeef) { DRV_FLASH_Erase_Sector(probeAddr, 0, 1); }    
  }

  //printf("\n\r offset[%lx] size[%lx] available_sz[%lx] retend_offset[%lx]", mcu_image_sz_offset, *mcu_image_sz, available_sz, retain_flash_offset);
  dbg_offset = retain_flash_offset;

  while(asked_sz > 0) {
    if (available_sz >= (64*1024)) {
      DRV_FLASH_Erase_Sector(dstFlashAddr, 1, 1);
      //printf("\n\r 64K: erased offset[%lx] size[%lx]", dbg_offset, asked_sz);
      asked_sz = asked_sz > (64*1024) ? asked_sz - (64*1024) : 0;
      dstFlashAddr += (64*1024);

      dbg_offset += (64*1024); 
    } else if (available_sz >= (4*1024)) {
      DRV_FLASH_Erase_Sector(dstFlashAddr, 0, 1);
      //printf("\n\r 4K: erased offset[%lx] size[%lx]", dbg_offset, asked_sz);
      asked_sz = asked_sz > (4*1024) ? asked_sz - (4*1024) : 0;
      dstFlashAddr += (4*1024);

      dbg_offset += (4*1024); 
    } else {
      return (-2);
    }
  } 

  return 0;
}

void probe_flash_offset(void) {
  uint32_t *probeAddr, flash_offset_64KB, flash_offset_4KB; 
  uint32_t mcu_image_sz_offset = MCU_PART_IMAGE_SZ_OFFSET, *mcu_image_sz = 0;
  mcu_image_sz = (uint32_t *)mcu_image_sz_offset;
  retain_flash_offset = 0;

  if(*mcu_image_sz == 0)
    return;

  flash_offset_64KB = ((*mcu_image_sz/65536) + 1)*65536 + MCU_PARTITION_OFFSET; //64KB alignment for 64KB erase
  flash_offset_4KB = ((*mcu_image_sz/4096) + 1)*4096 + MCU_PARTITION_OFFSET; //4KB alignment for 4KB erase

  probeAddr = (uint32_t *)flash_offset_4KB;
  if(*probeAddr == 0xdeadbeef) { 
    retain_flash_offset = flash_offset_4KB; 
    return;
  } 

  probeAddr = (uint32_t *)flash_offset_64KB;
  if(*probeAddr == 0xdeadbeef) { 
    retain_flash_offset = flash_offset_64KB; 
    return;
  } 
}

#endif

/**
 * Perform any actions needed to prepare for hibernation.
 *
 * This function must be followed by hibernate_to_gpm() after resuming from hibernation.
 *
 * @return error code if hibernation failed
 */
int32_t hibernate_to_gpm(PWR_MNGR_PwrMode pwr_mode) {
  struct hibernate_free_block *freeList;
  size_t region, srcSize, dstSize, accSrcLen = 0, accDstLen = 0;
  char *dstAddr = __gpm_compress_start__, *srcAddr;
#if (PWR_MNGR_EN_HIBERNATE_SHUTDOWN == 1)
  size_t retainedSize = 0;
  char *dstGpmAddr = __bss_gpm_start__;
#endif
  /* Get all unused memory and zero it so it compresses well.
   * Other methods were examined, such adding special code to
   * handle free memory without zeroing it; however this was
   * found to have minimal effect on compression times. */
  GetFreeList(&freeList);
  ZeroFreeMem(freeList);

  for (region = 0; region < sizeof(MemRegions) / sizeof(MemRegions[0]); ++region) {
    HBN_LOG(3, "Region%zu, start:%p, end:%p, len:%zu\n", region, MemRegions[region].start,
            MemRegions[region].end, (size_t)(MemRegions[region].end - MemRegions[region].start));
    for (srcAddr = MemRegions[region].start; srcAddr < MemRegions[region].end;
         srcAddr += WORKING_BLOCK_SIZE) {
      srcSize = MemRegions[region].end - srcAddr;
      if (srcSize > WORKING_BLOCK_SIZE) {
        srcSize = WORKING_BLOCK_SIZE;
      }

      /* Here we check if reamin compression buffer large enough for next source block */
      if ((size_t)(__gpm_compress_end__ - dstAddr) < MAX_COMPRESSED_BLOCK_SIZE) {
        HBN_LOG(1, "Remain compressing buffer insufficient, srcSize:%zu, remain: %zu\n", srcSize,
                (size_t)(__gpm_compress_end__ - dstAddr));
        DecompressPartial(region, srcAddr - WORKING_BLOCK_SIZE, dstAddr);
        return (-1);
      }

      dstSize = fastlz_compress_level(1, srcAddr, srcSize, dstAddr);

      HBN_LOG(3, "Compress src:%p, srcSize:%zu, dst:%p, dstSize:%zu, ratio: %f\n", srcAddr, srcSize,
              dstAddr, dstSize, (float)srcSize / dstSize);
      dstAddr += dstSize;
      accSrcLen += srcSize;
      accDstLen += dstSize;

      /* Then place the compressed/original size of each block at the end of the block.
      Memcpy to avoid alignment issues */
      memcpy(dstAddr, &dstSize, sizeof(size_t));
      dstAddr += sizeof(size_t);
      memcpy(dstAddr, &srcSize, sizeof(size_t));
      dstAddr += sizeof(size_t);
    }
  }

  HBN_LOG(3, "accSrcLen:%zu, accDstLen:%zu, ratio: %f, remain: %zu\n", accSrcLen, accDstLen,
          (float)accSrcLen / accDstLen, (size_t)(__gpm_compress_end__ - dstAddr));
  EndOfCompressedMem = dstAddr;

  //hexDump("hibernate_to_gpm", dstGpmAddr, retainedSize+4);
#if (PWR_MNGR_EN_HIBERNATE_SHUTDOWN == 1)
  if(pwr_mode == PWR_MNGR_MODE_SHUTDOWN) {
    retainedSize = (size_t)(dstAddr - dstGpmAddr);
    if((hibernate_prepare_flash_space(retainedSize) != 0) || (retain_flash_offset == 0))
      return (-2);
  }
#endif
  return 0;
}

/**
 * Perform any actions needed to prepare for normal execution.
 *
 * This function must always be called after resuming from hibernation started
 * by hibernate_from_gpm().
 */
int32_t hibernate_from_gpm(void) {
  static const size_t lastRegion = sizeof(MemRegions) / sizeof(MemRegions[0]) - 1;

  return DecompressPartial(lastRegion, MemRegions[lastRegion].end, EndOfCompressedMem);
}

/****************************************************************************
 * Debug Functions
 ****************************************************************************/

/**
 * DEBUG API - Get the list of free blocks in the system
 */
void hibernate_get_free_blocks(struct hibernate_free_block **freeListPtr) {
  GetFreeList(freeListPtr);
}

/**
 * DEBUG API - Zero all free blocks in the system
 */
void hibernate_zero_free_blocks(struct hibernate_free_block *freeListPtr) {
  ZeroFreeMem(freeListPtr);
}

/**
 * DEBUG API - Restore guard blocks on stacks after zeroing
 * unused memory
 */
void hibernate_restore_stacks(void) { RestoreFreeStack(); }
