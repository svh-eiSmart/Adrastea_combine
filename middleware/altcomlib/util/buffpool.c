/****************************************************************************
 *
 *  (c) copyright 2020 Altair Semiconductor, Ltd. All rights reserved.
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "buffpool.h"
#include "dbg_if.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define BUFFPOOL_LOCK(handle)         \
  do {                                \
    altcom_sys_lock_mutex(&(handle)); \
  } while (0)
#define BUFFPOOL_UNLOCK(handle)         \
  do {                                  \
    altcom_sys_unlock_mutex(&(handle)); \
  } while (0)
#define BUFFPOOL_PULL_BUFFINFO(info, pullpos) \
  do {                                        \
    info = pullpos;                           \
    pullpos = (pullpos)->next;                \
  } while (0)
#define BUFFPOOL_PUSH_BUFFINFO(info, pushpos) \
  do {                                        \
    (info)->next = pushpos;                   \
    pushpos = info;                           \
  } while (0)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct buffpool_buffinfo_s {
  FAR int8_t *buffaddr;
  FAR struct buffpool_buffinfo_s *next;
};

struct buffpool_blockinfo_s {
  FAR int8_t *buffer;
  FAR int8_t *endaddr;
  uint32_t size;
  int32_t totalcnt;
  int32_t freecnt;
  int32_t usedcnt;
  int32_t maxusedcnt;
  FAR struct buffpool_buffinfo_s *freelist;
  FAR struct buffpool_buffinfo_s *alloclist;
  FAR struct buffpool_blockinfo_s *next;
};

struct buffpool_table_s {
  altcom_sys_mutex_t buffmtx;
  altcom_sys_thread_cond_t getwaitcond;
  altcom_sys_mutex_t getwaitcondmtx;
  FAR struct buffpool_blockinfo_s *startblkinfo;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void buffpool_deletebuffinfo(FAR struct buffpool_buffinfo_s *table);
static void buffpool_deleteblockinfo(FAR struct buffpool_table_s *table);
static struct buffpool_blockinfo_s *buffpool_createblockinfo(
    FAR struct buffpool_blockset_s *blkset);
static void buffpool_insertblockinfo(FAR struct buffpool_table_s *table,
                                     FAR struct buffpool_blockinfo_s *blkinfo);
static bool buffpool_getbuffer(FAR struct buffpool_table_s *table, uint32_t size,
                               FAR int8_t **buffaddr);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: buffpool_deletebuffinfo
 *
 * Description:
 *   Delete link list of buffpool_buffinfo_s.
 *
 * Input Parameters:
 *   table  Start address of linked list to delete.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void buffpool_deletebuffinfo(FAR struct buffpool_buffinfo_s *table) {
  FAR struct buffpool_buffinfo_s *target = table;
  FAR struct buffpool_buffinfo_s *next = NULL;

  while (target) {
    next = target->next;
    ALTCOM_SYS_FREE(target);
    target = next;
  }
}

/****************************************************************************
 * Name: buffpool_deleteblockinfo
 *
 * Description:
 *   Delete link list of buffpool_blockinfo_s.
 *
 * Input Parameters:
 *   table  Start address of linked list to delete.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void buffpool_deleteblockinfo(FAR struct buffpool_table_s *table) {
  FAR struct buffpool_blockinfo_s *target = table->startblkinfo;
  FAR struct buffpool_blockinfo_s *next = NULL;

  while (target) {
    next = target->next;
    buffpool_deletebuffinfo(target->freelist);
    buffpool_deletebuffinfo(target->alloclist);
    ALTCOM_SYS_FREE(target->buffer);
    ALTCOM_SYS_FREE(target);
    target = next;
  }
}

/****************************************************************************
 * Name: buffpool_createblockinfo
 *
 * Description:
 *   Create buffpool_blockinfo_s object.
 *
 * Input Parameters:
 *   blkset  Size and number of create object.
 *
 * Returned Value:
 *   Pointer of struct buffpool_blockinfo_s.
 *   If create failed, returned NULL.
 *
 * Assumptions/Limitations:
 *   The size and num elements of @blkset must not be 0.
 *
 ****************************************************************************/

static FAR struct buffpool_blockinfo_s *buffpool_createblockinfo(
    FAR struct buffpool_blockset_s *blkset) {
  FAR struct buffpool_blockinfo_s *blkinfo = NULL;
  FAR struct buffpool_buffinfo_s **targetbuffinfo = NULL;
  uint32_t num = 0;

  if (USHRT_MAX < (blkset->size * blkset->num)) {
    DBGIF_LOG2_ERROR("Unexpected value. size:%lu, num:%u\n", blkset->size, blkset->num);
    goto errout;
  }

  /* Allocate block info. */

  blkinfo =
      (FAR struct buffpool_blockinfo_s *)ALTCOM_SYS_MALLOC(sizeof(struct buffpool_blockinfo_s));
  if (!blkinfo) {
    DBGIF_LOG_ERROR("Block info allocate failed.\n");
    goto errout;
  }

  memset(blkinfo, 0, sizeof(struct buffpool_blockinfo_s));
  blkinfo->size = blkset->size;
  blkinfo->totalcnt = blkset->num;
  blkinfo->freecnt = blkset->num;

  /* Allocate main buffer. */

  blkinfo->buffer = (FAR int8_t *)ALTCOM_SYS_MALLOC(blkset->size * blkset->num);
  if (!blkinfo->buffer) {
    DBGIF_LOG2_ERROR("Buffer allocate failed. block size:%lu, num:%u\n", blkset->size, blkset->num);
    goto errout_with_blkinfofree;
  }

  blkinfo->endaddr = blkinfo->buffer + (blkset->size * blkset->num);

  /* Allocate and setting buffer info arry. */

  targetbuffinfo = &blkinfo->freelist;

  for (num = 0; num < blkset->num; num++) {
    *targetbuffinfo =
        (FAR struct buffpool_buffinfo_s *)ALTCOM_SYS_MALLOC(sizeof(struct buffpool_buffinfo_s));
    if (!(*targetbuffinfo)) {
      DBGIF_LOG2_ERROR("Buffer info allocate failed. size:%lu, num:%u\n", blkset->size,
                       blkset->num);
      goto errout_with_buffinfofree;
    }

    (*targetbuffinfo)->buffaddr = blkinfo->buffer + (blkset->size * num);
    targetbuffinfo = &(*targetbuffinfo)->next;
  }

  *targetbuffinfo = NULL;

  return blkinfo;

errout_with_buffinfofree:
  buffpool_deletebuffinfo(blkinfo->freelist);
  ALTCOM_SYS_FREE(blkinfo->buffer);
errout_with_blkinfofree:
  ALTCOM_SYS_FREE(blkinfo);
errout:
  return NULL;
}

/****************************************************************************
 * Name: buffpool_insertblockinfo
 *
 * Description:
 *   Insert new buffpool_blockinfo_s into buffpool_table_s.
 *   Inserts are done in ascending order.
 *
 * Input Parameters:
 *   table    Pointer of data table.
 *   blkinfo  Pointer of new buffpool_blockinfo_s.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void buffpool_insertblockinfo(FAR struct buffpool_table_s *table,
                                     FAR struct buffpool_blockinfo_s *blkinfo) {
  FAR struct buffpool_blockinfo_s *blkinfonow = NULL;

  if (!table->startblkinfo) {
    table->startblkinfo = blkinfo;
  } else if (blkinfo->size < table->startblkinfo->size) {
    blkinfo->next = table->startblkinfo;
    table->startblkinfo = blkinfo;
  } else {
    /* blkinfo is always greater than blkinfonow */

    blkinfonow = table->startblkinfo;
    while (1) {
      if (!blkinfonow->next) {
        blkinfonow->next = blkinfo;
        return;
      }

      if (blkinfo->size < blkinfonow->next->size) {
        blkinfo->next = blkinfonow->next;
        blkinfonow->next = blkinfo;
        return;
      }

      blkinfonow = blkinfonow->next;
    }
  }
}

/****************************************************************************
 * Name: buffpool_getbuffer
 *
 * Description:
 *   Get free buffers from the table.
 *
 * Input Parameters:
 *   table     Pointer of data table.
 *   size      Buffer size to search.
 *   buffaddr  Pointer to store if buffer is found.
 *
 * Returned Value:
 *   true is returned when processing succeeds,
 *   or when all buffers satisfying the request are in use.
 *   Otherwise false is returned.
 *
 ****************************************************************************/

static bool buffpool_getbuffer(FAR struct buffpool_table_s *table, uint32_t size,
                               FAR int8_t **buffaddr) {
  FAR struct buffpool_blockinfo_s *blkinfo = NULL;
  FAR struct buffpool_buffinfo_s *buffinfo = NULL;
  bool result = false;

  blkinfo = table->startblkinfo;
  while (blkinfo) {
    if (blkinfo->size < size) {
      blkinfo = blkinfo->next;
      continue;
    }

    result = true;
    BUFFPOOL_LOCK(table->buffmtx);

    if (blkinfo->freelist) {
      BUFFPOOL_PULL_BUFFINFO(buffinfo, blkinfo->freelist);
      BUFFPOOL_PUSH_BUFFINFO(buffinfo, blkinfo->alloclist);

      *buffaddr = buffinfo->buffaddr;
      memset(*buffaddr, 0, blkinfo->size);
      DBGIF_LOG2_DEBUG("Successful get buffer. size:%lu(%lu)\n", blkinfo->size, size);
    }

    BUFFPOOL_UNLOCK(table->buffmtx);
    if (buffinfo) {
      blkinfo->freecnt--;
      blkinfo->usedcnt++;
      DBGIF_ASSERT(blkinfo->freecnt + blkinfo->usedcnt == blkinfo->totalcnt,
                   "Total(%ld)/Free(%ld)/Used(%ld) count inconsist on allocate!\r\n",
                   blkinfo->totalcnt, blkinfo->freecnt, blkinfo->usedcnt);

      if (blkinfo->usedcnt > blkinfo->maxusedcnt) {
        blkinfo->maxusedcnt = blkinfo->usedcnt;
        DBGIF_ASSERT(blkinfo->totalcnt >= blkinfo->maxusedcnt,
                     "Total(%ld)/Max used(%ld) count inconsist!\r\n", blkinfo->totalcnt,
                     blkinfo->maxusedcnt);
      }

      return result;
    }

    blkinfo = blkinfo->next;
  }

  if (result == false) {
    DBGIF_LOG1_ERROR("There is no buffer of size to satisfy the request. reqsize:%lu\n", size);
  } else {
    DBGIF_LOG1_WARNING("All buffers that satisfy the request are in use. reqsize:%lu\n", size);
  }

  return result;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: buffpool_create
 *
 * Description:
 *   Create the bufferpool.
 *
 * Input Parameters:
 *   set     List of size and number for creating the buffer.
 *   setnum  Number of @set.
 *
 * Returned Value:
 *   If the process succeeds, it returns Object of buffpool.
 *   Otherwise NULL is returned.
 *
 ****************************************************************************/

buffpool_t buffpool_create(FAR struct buffpool_blockset_s set[], uint8_t setnum) {
  FAR struct buffpool_table_s *table = NULL;
  FAR struct buffpool_blockinfo_s *blkinfo = NULL;
  int8_t num = 0;
  altcom_sys_cremtx_s mtx_param;

  if (!set || !setnum) {
    DBGIF_LOG_ERROR("Incorrect argument.\n");
    errno = EINVAL;
    goto errout;
  }

  for (num = 0; num < setnum; num++) {
    if (set[num].size && set[num].num) {
      break;
    }
  }

  if (setnum <= num) {
    DBGIF_LOG_ERROR("Incorrect argument.\n");
    errno = EINVAL;
    goto errout;
  }

  /* Create data table. */

  table = (FAR struct buffpool_table_s *)ALTCOM_SYS_MALLOC(sizeof(struct buffpool_table_s));
  if (!table) {
    DBGIF_LOG_ERROR("Data table allocate failed.\n");
    errno = ENOMEM;
    goto errout;
  }

  memset(table, 0, sizeof(struct buffpool_table_s));

  /* Create mutex. */

  if (altcom_sys_create_mutex(&table->buffmtx, &mtx_param) < 0) {
    DBGIF_LOG_ERROR("Mutex create failed.\n");
    errno = ENOMEM;
    goto errout_with_tablefree;
  }

  /* Initialize thread condition */

  if (altcom_sys_create_thread_cond_mutex(&table->getwaitcond, &table->getwaitcondmtx) < 0) {
    DBGIF_LOG_ERROR("Initialize thread condition failed.\n");
    errno = ENOMEM;
    goto errout_with_mtxdelete;
  }

  /* Create block data. */

  for (num = 0; num < setnum; num++) {
    if (set[num].size == 0 || set[num].num == 0) {
      continue;
    }

    blkinfo = buffpool_createblockinfo(&set[num]);
    if (!blkinfo) {
      errno = ENOMEM;
      goto errout_with_blkinfodelete;
    }

    /* Insert table in ascending order. */

    buffpool_insertblockinfo(table, blkinfo);
  }

  return (buffpool_t)table;

errout_with_blkinfodelete:
  if (table->startblkinfo) {
    buffpool_deleteblockinfo(table);
  }

  altcom_sys_delete_thread_cond_mutex(&table->getwaitcond, &table->getwaitcondmtx);
errout_with_mtxdelete:
  altcom_sys_delete_mutex(&table->buffmtx);
errout_with_tablefree:
  ALTCOM_SYS_FREE(table);
errout:
  return NULL;
}

/****************************************************************************
 * Name: buffpool_delete
 *
 * Description:
 *   Delete the bufferpool.
 *
 * Input Parameters:
 *   thiz  Object of bufferpool.
 *
 * Returned Value:
 *   If the process succeeds, it returns 0.
 *   Otherwise errno is returned.
 *
 ****************************************************************************/

int32_t buffpool_delete(buffpool_t thiz) {
  FAR struct buffpool_table_s *table = NULL;

  if (!thiz) {
    DBGIF_LOG_ERROR("Incorrect argument.\n");
    return -EINVAL;
  }

  table = (FAR struct buffpool_table_s *)thiz;
  buffpool_deleteblockinfo(table);
  altcom_sys_delete_thread_cond_mutex(&table->getwaitcond, &table->getwaitcondmtx);
  altcom_sys_delete_mutex(&table->buffmtx);
  ALTCOM_SYS_FREE(table);

  return 0;
}

/****************************************************************************
 * Name: buffpool_alloc
 *
 * Description:
 *   Allocate buffer from bufferpool.
 *   This function is blocking.
 *
 * Input Parameters:
 *   thiz     Object of bufferpool.
 *   reqsize  Buffer size.
 *
 * Returned Value:
 *   Buffer address.
 *   If can't get available buffer
 *   and  if @reqsize value is under 1, returned NULL.
 *
 ****************************************************************************/

FAR void *buffpool_alloc(buffpool_t thiz, uint32_t reqsize) {
  FAR struct buffpool_table_s *table = NULL;
  FAR int8_t *result = NULL;

  if (!thiz) {
    DBGIF_LOG_ERROR("Incorrect argument.\n");
    return NULL;
  }

  if (!reqsize) {
    DBGIF_LOG_INFO("Allocation request size is 0.\n");
    return NULL;
  }

  table = (FAR struct buffpool_table_s *)thiz;
  altcom_sys_lock_mutex(&table->getwaitcondmtx);
  do {
    if (!buffpool_getbuffer(table, reqsize, &result)) {
      break;
    }

    if (result) {
      break;
    }
  } while (altcom_sys_thread_cond_wait(&table->getwaitcond, &table->getwaitcondmtx) == 0);

  altcom_sys_unlock_mutex(&table->getwaitcondmtx);

  return result;
}

/****************************************************************************
 * Name: buffpool_free
 *
 * Description:
 *   Free the allocated buffer from bufferpool.
 *
 * Input Parameters:
 *   thiz  Object of bufferpool.
 *   buff  Buffer address.
 *
 * Returned Value:
 *   If the process succeeds, it returns 0.
 *   And when request buffer is NULL to returns 0.
 *   Otherwise errno is returned.
 *
 ****************************************************************************/

int32_t buffpool_free(buffpool_t thiz, FAR void *buff) {
  FAR struct buffpool_table_s *table = NULL;
  FAR struct buffpool_blockinfo_s *blkinfo = NULL;
  FAR struct buffpool_buffinfo_s *buffinfo = NULL;
  FAR struct buffpool_buffinfo_s *tmpbuffinfo = NULL;

  if (!thiz) {
    DBGIF_LOG_ERROR("Incorrect argument.\n");
    return -EINVAL;
  }

  if (!buff) {
    DBGIF_LOG_INFO("Free request buffer is NULL.\n");
    return 0;
  }

  table = (FAR struct buffpool_table_s *)thiz;
  blkinfo = table->startblkinfo;
  while (blkinfo) {
    if ((uintptr_t)blkinfo->buffer <= (uintptr_t)buff &&
        (uintptr_t)buff < (uintptr_t)blkinfo->endaddr) {
      break;
    }

    blkinfo = blkinfo->next;
  }

  DBGIF_ASSERT(blkinfo, "The given buffer is not from the buffer pool.");

  altcom_sys_lock_mutex(&table->getwaitcondmtx);
  BUFFPOOL_LOCK(table->buffmtx);
  if (blkinfo->alloclist->buffaddr == buff) {
    BUFFPOOL_PULL_BUFFINFO(tmpbuffinfo, blkinfo->alloclist);
    BUFFPOOL_PUSH_BUFFINFO(tmpbuffinfo, blkinfo->freelist);
  } else {
    buffinfo = blkinfo->alloclist;
    while (buffinfo->next) {
      if (buffinfo->next->buffaddr == buff) {
        BUFFPOOL_PULL_BUFFINFO(tmpbuffinfo, buffinfo->next);
        BUFFPOOL_PUSH_BUFFINFO(tmpbuffinfo, blkinfo->freelist);
        break;
      }

      buffinfo = buffinfo->next;
    }

    DBGIF_ASSERT(blkinfo->freelist == tmpbuffinfo, "Given buffer is unused.");
  }

  blkinfo->freecnt++;
  blkinfo->usedcnt--;
  DBGIF_ASSERT(blkinfo->freecnt + blkinfo->usedcnt == blkinfo->totalcnt,
               "Total(%ld)/Free(%ld)/Used(%ld) count inconsist on free!\r\n", blkinfo->totalcnt,
               blkinfo->freecnt, blkinfo->usedcnt);

  BUFFPOOL_UNLOCK(table->buffmtx);
  altcom_sys_signal_thread_cond(&table->getwaitcond);
  altcom_sys_unlock_mutex(&table->getwaitcondmtx);
  return 0;
}

/****************************************************************************
 * Name: buffpool_showstatistics
 *
 * Description:
 *   Show the statistics of buffpool.
 *
 * Input Parameters:
 *   thiz  Object of bufferpool.
 *
 * Returned Value:
 *   None
 ****************************************************************************/

void buffpool_showstatistics(buffpool_t thiz) {
  if (!thiz) {
    DBGIF_LOG_ERROR("Incorrect argument.\n");
    return;
  }

  FAR struct buffpool_table_s *table;
  FAR struct buffpool_blockinfo_s *blkinfo;

  table = (FAR struct buffpool_table_s *)thiz;
  blkinfo = table->startblkinfo;
  DBGIF_LOG(DBGIF_LV_ERR, "====Buffpool Statistics====\r\n");
  BUFFPOOL_LOCK(table->buffmtx);
  while (blkinfo) {
    DBGIF_LOG(DBGIF_LV_ERR,
              "Buffpool Size(%ld)/TotalCnt(%ld)/FreeCnt(%ld)/UsedCnt(%ld)/MaxUsedCnt(%ld)\r\n",
              blkinfo->size, blkinfo->totalcnt, blkinfo->freecnt, blkinfo->usedcnt,
              blkinfo->maxusedcnt);
    blkinfo = blkinfo->next;
  }

  BUFFPOOL_UNLOCK(table->buffmtx);
}