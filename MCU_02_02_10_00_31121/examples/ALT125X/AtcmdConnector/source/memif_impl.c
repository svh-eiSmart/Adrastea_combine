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

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

/* CMSIS-RTOS2 includes */
#include "cmsis_os2.h"

/* Middleware includes */
#include "altcom.h"

/* Implementation includes */
#include "memif_impl.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Define size of the byte array required to create count of blocks of given size */
#define MEMPOOL_ARR_SIZE(bl_size, bl_count) (((((bl_size) + (4 - 1)) / 4) * 4) * (bl_count))

/****************************************************************************
 * Private Data Type
 ****************************************************************************/

struct buffpool_blockinfo_s {
  osMemoryPoolId_t pool_id;
  uint8_t *buffer;
  uint8_t *endaddr;
  uint32_t size;
  uint32_t max_used_cnt;
  struct buffpool_blockinfo_s *next;
};

struct buffpool_table_s {
  osSemaphoreId_t wait_sem;
  osMutexId_t buff_mtx;
  struct buffpool_blockinfo_s *blk_info;
};

/****************************************************************************
 * Private Function
 ****************************************************************************/

static bool memif_impl_getbuffer(struct buffpool_table_s *table, uint32_t size,
                                 uint8_t **buffaddr) {
  struct buffpool_blockinfo_s *blk_info;
  bool result = false;

  blk_info = table->blk_info;
  while (blk_info) {
    if (blk_info->size < size) {
      blk_info = blk_info->next;
      continue;
    }

    result = true;
    osMutexAcquire(table->buff_mtx, osWaitForever);
    if (osMemoryPoolGetSpace(blk_info->pool_id)) {
      if ((*buffaddr = (uint8_t *)osMemoryPoolAlloc(blk_info->pool_id, osWaitForever)) != NULL) {
        memset((void *)*buffaddr, 0, (size_t)blk_info->size);
      }

      uint32_t used_cnt;

      used_cnt = osMemoryPoolGetCount(blk_info->pool_id);
      if (used_cnt > blk_info->max_used_cnt) {
        blk_info->max_used_cnt = used_cnt;
      }

      osMutexRelease(table->buff_mtx);
      return result;
    }

    osMutexRelease(table->buff_mtx);
    blk_info = blk_info->next;
  }

  if (result == false) {
    printf("[%s:%d]No matched size of buffer. reqsize:%lu\n", __FUNCTION__, __LINE__, size);
  } else {
    printf("[%s:%d]All buffers  in use. reqsize:%lu\n", __FUNCTION__, __LINE__, size);
  }

  return result;
}

struct buffpool_blockinfo_s *memif_impl_createblockinfo(uint32_t blk_size, uint16_t blk_num) {
  struct buffpool_blockinfo_s *blk_info;

  if (UINT16_MAX < blk_size * blk_num) {
    printf("[%s:%d]Unexpected value. size:%lu, num:%hu\n", __FUNCTION__, __LINE__, blk_size,
           blk_num);
  }

  blk_info = malloc(sizeof(struct buffpool_blockinfo_s));
  if (!blk_info) {
    printf("[%s:%d]Block info malloc failed\n", __FUNCTION__, __LINE__);
    goto errout;
  }

  memset((void *)blk_info, 0, sizeof(struct buffpool_blockinfo_s));

  uint32_t aligned_size;

  aligned_size = MEMPOOL_ARR_SIZE(blk_size, blk_num);
  blk_info->size = blk_size;
  blk_info->buffer = malloc((size_t)aligned_size);
  if (!blk_info->buffer) {
    printf("[%s:%d]Block buffer malloc failed\n", __FUNCTION__, __LINE__);
    goto errout;
  }

  blk_info->endaddr = blk_info->buffer + aligned_size;

  osMemoryPoolAttr_t attr;

  memset((void *)&attr, 0, sizeof(osMemoryPoolAttr_t));
  attr.mp_mem = blk_info->buffer;
  attr.mp_size = aligned_size;
  blk_info->pool_id = osMemoryPoolNew(blk_num, blk_size, &attr);
  if (!blk_info->pool_id) {
    printf("[%s:%d]osMemoryPoolNew failed\n", __FUNCTION__, __LINE__);
    goto errout;
  }

  return blk_info;

errout:
  if (blk_info) {
    if (blk_info->buffer) {
      free(blk_info->buffer);
    }

    free(blk_info);
  }

  return NULL;
}

static void memif_impl_insertblockinfo(struct buffpool_table_s *table,
                                       struct buffpool_blockinfo_s *blk_info) {
  struct buffpool_blockinfo_s *blk_info_now = NULL;

  if (!table->blk_info) {
    table->blk_info = blk_info;
  } else if (blk_info->size < table->blk_info->size) {
    blk_info->next = table->blk_info;
    table->blk_info = blk_info;
  } else {
    blk_info_now = table->blk_info;
    while (1) {
      if (!blk_info_now->next) {
        blk_info_now->next = blk_info;
        return;
      }

      if (blk_info->size < blk_info_now->next->size) {
        blk_info->next = blk_info_now->next;
        blk_info_now->next = blk_info;
        return;
      }

      blk_info_now = blk_info_now->next;
    }
  }
}

/****************************************************************************
 * Public Function
 ****************************************************************************/

void *memif_impl_create(blockset_t set[], uint8_t setnum) {
  struct buffpool_blockinfo_s *blk_info = NULL;
  struct buffpool_table_s *table = NULL;

  if (!set || !setnum) {
    printf("[%s:%d]Invalid parameter, set: 0x%p, setnum: %lu\n", __FUNCTION__, __LINE__, set,
           (uint32_t)setnum);
    goto errout;
  }

  table = malloc(sizeof(struct buffpool_table_s));
  if (!table) {
    printf("[%s:%d]Block info table malloc failed\n", __FUNCTION__, __LINE__);
    goto errout;
  }

  memset((void *)table, 0, sizeof(struct buffpool_table_s));
  table->wait_sem = osSemaphoreNew(1, 0, NULL);
  if (!table->wait_sem) {
    printf("[%s:%d]wait_sem new failed\n", __FUNCTION__, __LINE__);
    goto errout;
  }

  table->buff_mtx = osMutexNew(NULL);
  if (!table->buff_mtx) {
    printf("[%s:%d]buff_mtx new failed\n", __FUNCTION__, __LINE__);
    goto errout;
  }

  uint8_t i;
  for (i = 0; i < setnum; i++) {
    blk_info = memif_impl_createblockinfo(set[i].blkSize, set[i].blkNum);
    if (!blk_info) {
      printf("[%s:%d]memif_impl_createblockinfo failed\n", __FUNCTION__, __LINE__);
      goto errout;
    }

    memif_impl_insertblockinfo(table, blk_info);
  }

  return table;

errout:
  if (table) {
    if (table->wait_sem) {
      osSemaphoreDelete(table->wait_sem);
    }

    if (table->buff_mtx) {
      osMutexDelete(table->buff_mtx);
    }

    while (table->blk_info) {
      blk_info = table->blk_info;
      table->blk_info = table->blk_info->next;
      osMemoryPoolDelete(blk_info->pool_id);
      free(blk_info->buffer);
      free(blk_info);
    }

    free(table);
  }

  return NULL;
}

int32_t memif_impl_destroy(void *handle) {
  if (!handle) {
    printf("[%s:%d]NULL handle\n", __FUNCTION__, __LINE__);
    return -1;
  }

  struct buffpool_table_s *table;
  struct buffpool_blockinfo_s *blk_info, *tmp;

  table = (struct buffpool_table_s *)handle;
  blk_info = table->blk_info;
  while (blk_info) {
    tmp = blk_info;
    blk_info = blk_info->next;
    osMemoryPoolDelete(tmp->pool_id);
    free(tmp->buffer);
    free(tmp);
  }

  if (table->wait_sem) {
    osSemaphoreDelete(table->wait_sem);
  }

  if (table->buff_mtx) {
    osMutexDelete(table->buff_mtx);
  }

  free(table);
  return 0;
}

void *memif_impl_alloc(void *handle, uint32_t size) {
  if (!handle || !size) {
    printf("[%s:%d]Invalid handle %p or size %lu\n", __FUNCTION__, __LINE__, handle, size);
    return NULL;
  }

  struct buffpool_table_s *table = (struct buffpool_table_s *)handle;
  uint8_t *result = NULL;

  do {
    if (!memif_impl_getbuffer(table, size, &result)) {
      break;
    }

    if (result) {
      break;
    }
  } while (osSemaphoreAcquire(table->wait_sem, osWaitForever) == osOK);

  return result;
}

int32_t memif_impl_free(void *handle, void *buf) {
  if (!handle || !buf) {
    printf("[%s:%d]NULL handle: 0x%p or buf: 0x%p\n", __FUNCTION__, __LINE__, handle, buf);
    return -1;
  }

  struct buffpool_table_s *table;
  struct buffpool_blockinfo_s *blk_info;

  table = (struct buffpool_table_s *)handle;
  blk_info = table->blk_info;
  while (blk_info) {
    if (blk_info->buffer <= (uint8_t *)buf && (uint8_t *)buf < blk_info->endaddr) {
      break;
    }

    blk_info = blk_info->next;
  }

  if (!blk_info) {
    printf("[%s:%d]Invalid buffer.\n", __FUNCTION__, __LINE__);
    return -1;
  }

  osMutexAcquire(table->buff_mtx, osWaitForever);

  osStatus_t status;

  status = osMemoryPoolFree(blk_info->pool_id, buf);
  if (status != osOK) {
    printf("[%s:%d]osMemoryPoolFree failed, status = %ld.\n", __FUNCTION__, __LINE__,
           (int32_t)status);
    osMutexRelease(table->buff_mtx);
    return -1;
  }

  osMutexRelease(table->buff_mtx);
  osSemaphoreRelease(table->wait_sem);
  return 0;
}

void memif_impl_show(void *handle) {
  if (!handle) {
    printf("[%s:%d]NULL handle\n", __FUNCTION__, __LINE__);
    return;
  }

  struct buffpool_table_s *table;
  struct buffpool_blockinfo_s *blk_info;

  table = (struct buffpool_table_s *)handle;
  blk_info = table->blk_info;
  printf("====Buffpool Statistics====\n");
  osMutexAcquire(table->buff_mtx, osWaitForever);
  while (blk_info) {
    printf("Buffpool Size(%ld)/TotalCnt(%ld)/FreeCnt(%ld)/UsedCnt(%ld)/MaxUsedCnt(%ld)\r\n",
           blk_info->size, osMemoryPoolGetCapacity(blk_info->pool_id),
           osMemoryPoolGetSpace(blk_info->pool_id), osMemoryPoolGetCount(blk_info->pool_id),
           blk_info->max_used_cnt);
    blk_info = blk_info->next;
  }

  osMutexRelease(table->buff_mtx);
}
