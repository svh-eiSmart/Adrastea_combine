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

#include "dbg_if.h"
#include "apicmd.h"
#include "buffpoolwrapper.h"
#include "apicmdgw.h"
#include "apicmd_errind.h"
#include "apiutil.h"
#include "altcom.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Size of Main task stack size. */

#define APICMDGW_MAIN_TASK_STACK_SIZE (4096)

/* Length of main task name. */

#define APICMDGW_MAX_MAIN_TASK_NAME_LEN (16)

#define APICMDGW_CHKSUM_LENGTH (12)

#define APICMDGW_APICMDHDR_LEN (sizeof(struct apicmd_cmdhdr_s))
#define APICMDGW_APICMDPAYLOAD_SIZE_MAX (3092)
#define APICMDGW_BUFF_SIZE_MAX (APICMDGW_APICMDPAYLOAD_SIZE_MAX + APICMDGW_APICMDHDR_LEN)

#define APICMDGW_HDR_ERR_VER (-1)
#define APICMDGW_HDR_ERR_CHKSUM (-2)
#define APICMDGW_HDR_DATAERR_CHKSUM (-3)

#define APICMDGW_GET_SEQID (++g_seqid_counter)
#define APICMDGW_GET_CMDID(hdr_ptr) (ntohs(((FAR struct apicmd_cmdhdr_s *)hdr_ptr)->cmdid))
#define APICMDGW_GET_DATA_PTR(hdr_ptr) (((FAR uint8_t *)(hdr_ptr) + APICMDGW_APICMDHDR_LEN))
#define APICMDGW_GET_HDR_PTR(data_ptr) (((FAR uint8_t *)(data_ptr)-APICMDGW_APICMDHDR_LEN))
#define APICMDGW_GET_TRANSID(hdr_ptr) (ntohs(((FAR struct apicmd_cmdhdr_s *)hdr_ptr)->transid))
#define APICMDGW_GET_DATA_LEN(hdr_ptr) (ntohs(((FAR struct apicmd_cmdhdr_s *)hdr_ptr)->dtlen))

#define APICMDGW_GET_RESCMDID(cmdid) (cmdid | 0x01 << 15)

#define ECHO_TIMEOUT 1000
#define ECHO_RETRYCNT 5
#define APICMDGW_ECHOTASK_STACK_SIZE (2048)
#define ECHO_CHAR 0xA5

#define MAX_MODULECNT 255

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct apicmdgw_blockinf_s {
  FAR uint8_t *recvbuff;
  FAR uint16_t *recvlen;
  uint16_t cmdid;
  uint16_t transid;
  uint16_t bufflen;
  altcom_sys_thread_cond_t waitcond;
  altcom_sys_mutex_t waitcondmtx;
  int32_t result;
  uint32_t isBlkReady;
  FAR struct apicmdgw_blockinf_s *next;
};

enum apicmdgw_state_e {
  APICMDGW_INIT = 0,
  APICMDGW_ECHO_TESTING = 1,
  APICMDGW_ECHO_COMPLETED = 2,
  APICMDGW_FIN = 3,
};

begin_packed_struct struct apicmd_echo_s { uint8_t dummy; } end_packed_struct;
begin_packed_struct struct apicmd_echo_ext_s {
  uint8_t dummy;
  uint8_t moduleInfoCnt;
  struct apicmd_moduleinfo_s moduleInfo[MAX_MODULECNT];
} end_packed_struct;
begin_packed_struct struct apicmd_echores_s { uint8_t dummy; } end_packed_struct;
begin_packed_struct struct apicmd_echores_ext_s {
  uint8_t dummy;
  uint8_t moduleInfoCnt;
  struct apicmd_moduleinfo_s moduleInfo[MAX_MODULECNT];
} end_packed_struct;

struct replay_evt_info_s {
  int8_t *buffer;
  int8_t *rdptr;
  size_t buflen;
  struct replay_evt_info_s *next;
};
/****************************************************************************
 * Private Data
 ****************************************************************************/

static bool g_isinit = false;
static bool g_isTaskRun = false;
static FAR struct apicmdgw_blockinf_s *g_blkinfotbl = NULL;
static altcom_sys_mutex_t g_blkinfotbl_mtx;
static altcom_sys_task_t g_rcvtask;
static uint8_t g_seqid_counter = 0;
static altcom_sys_thread_cond_t g_delwaitcond;
static altcom_sys_mutex_t g_delwaitcondmtx;
static FAR struct hal_if_s *g_hal_if = NULL;
static FAR struct evtdisp_s *g_evtdisp = NULL;
static altcom_sys_cremtx_s g_mtxparam;
static altcom_sys_thread_cond_t g_stateCond = 0;
static altcom_sys_mutex_t g_stateCondMtx = NULL;
static enum apicmdgw_state_e g_apicmdgwState = APICMDGW_FIN;
static altcom_sys_task_t g_echotask;
static altcom_sys_sem_t g_checkpostpone_sem;

/****************************************************************************
 * Public Data
 ****************************************************************************/
extern struct apicmd_moduleinfo_s g_moduleInfo[];
extern const uint8_t g_moduleInfoCnt;

#define AVAIL_MODULEINFO_LEN (g_moduleInfoCnt * sizeof(struct apicmd_moduleinfo_s))

/****************************************************************************
 * Inline functions
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: apicmdgw_createtransid
 *
 * Description:
 *   Create transaction ID.
 *
 * Input Parameters:
 *   None.
 *
 * Returned Value:
 *   Non-zero value is returned.
 *
 ****************************************************************************/

static uint16_t apicmdgw_createtransid(void) {
  static uint16_t transid = 0;

  transid++;
  if (!transid) {
    transid++;
  }

  return transid;
}

/****************************************************************************
 * Name: apicmdgw_createchksum
 *
 * Description:
 *   Create api command checksum.
 *
 * Input Parameters:
 *   startPtr  Start pointer.
 *   chkLen    checksum calculation length
 *
 * Returned Value:
 *   Returns checksum value.
 *
 ****************************************************************************/

static uint16_t apicmdgw_createchksum(FAR uint8_t *startPtr, size_t chkLen) {
  uint32_t ret = 0x00;
  uint32_t calctmp = 0x00;
  uint32_t i;

  /* Data accumulating */
  for (i = 0; i < chkLen; i++) {
    calctmp += startPtr[i];
  }

  /* Prepare result */
  ret = ~((calctmp & 0xFFFF) + (calctmp >> 16));
  DBGIF_LOG1_DEBUG("create check sum. chksum = %04x.\n", (uint16_t)ret);

  return (uint16_t)ret;
}

/****************************************************************************
 * Name: apicmdgw_checkheader
 *
 * Description:
 *   Check api command header.
 *
 * Input Parameters:
 *   evt       Api command header pointer.
 *
 * Returned Value:
 *   If the process succeeds, it returns 0.
 *   Otherwise APICMDGW_HDR_ERR value is returned.
 *
 ****************************************************************************/

static int32_t apicmdgw_checkheader(FAR uint8_t *evt) {
  FAR struct apicmd_cmdhdr_s *hdr = NULL;
  uint16_t chksum = 0;

  hdr = (FAR struct apicmd_cmdhdr_s *)evt;
  if (hdr->ver != APICMD_VER) {
    DBGIF_LOG2_ERROR("version mismatch [sender:0x%x, receiver:0x%x]\n", hdr->ver, APICMD_VER);
    return APICMDGW_HDR_ERR_VER;
  }

  if (APICMDGW_APICMDPAYLOAD_SIZE_MAX < APICMDGW_GET_DATA_LEN(hdr)) {
    DBGIF_LOG1_ERROR("Data length error. [data len:%lu]\n", APICMDGW_GET_DATA_LEN(hdr));
    return APICMDGW_HDR_ERR_VER;
  }

  chksum = apicmdgw_createchksum((FAR uint8_t *)hdr, APICMDGW_CHKSUM_LENGTH);

  if (chksum != ntohs(hdr->chksum)) {
    DBGIF_LOG2_ERROR("checksum error [header:0x%04x, calculation:0x%04x]\n",
                     (unsigned int)ntohs(hdr->chksum), (unsigned int)chksum);
    return APICMDGW_HDR_ERR_CHKSUM;
  }

  DBGIF_LOG2_INFO("Receive header[cmd_id:0x%04x, data len:0x%04x]\n",
                  (unsigned int)ntohs(hdr->cmdid), (unsigned int)ntohs(hdr->dtlen));

  return 0;
}

/****************************************************************************
 * Name: apicmdgw_checkdata
 *
 * Description:
 *   Check data checksum in api command header.
 *
 * Input Parameters:
 *   evt       Api command header pointer.
 *
 * Returned Value:
 *   If the process succeeds, it returns 0.
 *   Otherwise APICMDGW_HDR_DATAERR_CHKSUM value is returned.
 *
 ****************************************************************************/

static int32_t apicmdgw_checkdata(FAR uint8_t *evt) {
  FAR struct apicmd_cmdhdr_s *hdr = (FAR struct apicmd_cmdhdr_s *)evt;
  uint16_t dtchksum = 0;

  dtchksum = apicmdgw_createchksum(APICMDGW_GET_DATA_PTR(evt), ntohs(hdr->dtlen));
  if (dtchksum != ntohs(hdr->dtchksum)) {
    DBGIF_LOG2_ERROR("data checksum error [header:0x%04x, calculation:0x%04x]\n",
                     (unsigned int)ntohs(hdr->dtchksum), (unsigned int)dtchksum);
    return APICMDGW_HDR_DATAERR_CHKSUM;
  }

  return 0;
}

/****************************************************************************
 * Name: apicmdgw_errind
 *
 * Description:
 *  handling process error.
 *
 * Input Parameters:
 *  evt  handling process function pointer.
 *  evlen  length of event.
 *
 * Returned Value:
 *  function pointer handling result.
 *
 ****************************************************************************/

void apicmdgw_errind(FAR struct apicmd_cmdhdr_s *evthdr) {
  FAR struct apicmd_cmddat_errind_s *errind = NULL;
  int32_t ret;

  errind = (FAR struct apicmd_cmddat_errind_s *)apicmdgw_cmd_allocbuff(
      APICMDID_ERRIND, sizeof(struct apicmd_cmddat_errind_s));
  DBGIF_ASSERT(NULL != errind, "apicmdgw_cmd_allocbuff()\n");

  errind->ver = evthdr->ver;
  errind->seqid = evthdr->seqid;
  errind->cmdid = htons(evthdr->cmdid);
  errind->transid = htons(evthdr->transid);
  errind->dtlen = htons(evthdr->dtlen);
  errind->chksum = htons(evthdr->chksum);
  errind->dtchksum = htons(evthdr->dtchksum);

  ret = APICMDGW_SEND_ONLY((uint8_t *)errind);
  DBGIF_ASSERT(0 <= ret, "APICMDGW_SEND_ONLY() failed\n");

  if (ret < 0) {
    DBGIF_LOG1_ERROR("APICMDGW_SEND_ONLY failed, ret = %lu\n", ret);
  }

  ret = apicmdgw_freebuff((uint8_t *)errind);
  DBGIF_ASSERT(0 == ret, "apicmdgw_freebuff()\n");

  if (ret < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_freebuff failed, ret = %lu\n", ret);
  }
}

/****************************************************************************
 * Name: apicmdgw_errhandle
 *
 * Description:
 *  handling process error.
 *
 * Input Parameters:
 *  evt  handling process function pointer.
 *  evlen  length of event.
 *
 * Returned Value:
 *  function pointer handling result.
 *
 ****************************************************************************/

void apicmdgw_errhandle(FAR struct apicmd_cmdhdr_s *evthdr) {
  DBGIF_LOG_DEBUG("dispatch error\n");
  DBGIF_LOG1_DEBUG("version:0x%x\n", evthdr->ver);
  DBGIF_LOG1_DEBUG("sequence ID:0x%x\n", evthdr->seqid);
  DBGIF_LOG1_DEBUG("command ID:0x%x\n", (unsigned int)ntohs(evthdr->cmdid));
  DBGIF_LOG1_DEBUG("transaction ID:0x%x\n", (unsigned int)ntohs(evthdr->transid));
  DBGIF_LOG1_DEBUG("data length:0x%x\n", (unsigned int)ntohs(evthdr->dtlen));
  DBGIF_LOG1_DEBUG("check sum:0x%x\n", (unsigned int)ntohs(evthdr->chksum));
}

/****************************************************************************
 * Name: apicmdgw_addtable
 *
 * Description:
 *   Add wait table fot waittablelist.
 *
 * Input Parameters:
 *   tbl    waittable.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void apicmdgw_addtable(FAR struct apicmdgw_blockinf_s *tbl) {
  altcom_sys_lock_mutex(&g_blkinfotbl_mtx);

  if (!g_blkinfotbl) {
    g_blkinfotbl = tbl;
  } else {
    tbl->next = g_blkinfotbl;
    g_blkinfotbl = tbl;
  }

  altcom_sys_unlock_mutex(&g_blkinfotbl_mtx);
}

/****************************************************************************
 * Name: apicmdgw_remtable
 *
 * Description:
 *   Remove wait table fot waittablelist.
 *
 * Input Parameters:
 *   tbl    waittable.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void apicmdgw_remtable(FAR struct apicmdgw_blockinf_s *tbl) {
  FAR struct apicmdgw_blockinf_s *tmptbl;

  DBGIF_ASSERT(g_blkinfotbl, "table list is null.\n");

  altcom_sys_lock_mutex(&g_blkinfotbl_mtx);

  tmptbl = g_blkinfotbl;
  if (tmptbl == tbl) {
    g_blkinfotbl = g_blkinfotbl->next;
  } else {
    while (tmptbl->next) {
      if (tmptbl->next == tbl) {
        tmptbl->next = tmptbl->next->next;
        tmptbl = tbl;
        break;
      }

      tmptbl = tmptbl->next;
    }
  }

  DBGIF_ASSERT(tmptbl == tbl, "Can not find a table from the table list.");
  altcom_sys_delete_thread_cond_mutex(&tmptbl->waitcond, &tmptbl->waitcondmtx);
  BUFFPOOL_FREE(tmptbl);

  altcom_sys_unlock_mutex(&g_blkinfotbl_mtx);
}

/****************************************************************************
 * Name: apicmdgw_writetable
 *
 * Description:
 *   Get wait table for waittablelist and write data.
 *
 * Input Parameters:
 *   transid    Transaction id.
 *   cmdid      Api command id.
 *   data       Write data.
 *   datalen    @data length.
 *
 * Returned Value:
 *   If get wait table for wait table list success, return true.
 *   Otherwise false is returned.
 *
 ****************************************************************************/

static bool apicmdgw_writetable(uint16_t cmdid, uint16_t transid, FAR uint8_t *data,
                                uint16_t datalen) {
  bool result = false;
  FAR struct apicmdgw_blockinf_s *tbl = NULL;

  altcom_sys_lock_mutex(&g_blkinfotbl_mtx);

  tbl = g_blkinfotbl;
  while (tbl) {
    if (tbl->transid == transid && tbl->cmdid == cmdid) {
      result = true;
      break;
    }

    tbl = tbl->next;
  }

  if (tbl) {
    altcom_sys_lock_mutex(&tbl->waitcondmtx);
    if (datalen <= tbl->bufflen) {
      tbl->result = 0;
      memcpy(tbl->recvbuff, data, datalen);
      *(tbl->recvlen) = datalen;
      tbl->isBlkReady = 1;
    } else {
      tbl->result = -ENOSPC;
      DBGIF_LOG2_ERROR("Unexpected length. datalen: %d, bufflen: %d\n", datalen, tbl->bufflen);
    }

    altcom_sys_signal_thread_cond(&tbl->waitcond);
    altcom_sys_unlock_mutex(&tbl->waitcondmtx);
  }

  altcom_sys_unlock_mutex(&g_blkinfotbl_mtx);

  return result;
}

/****************************************************************************
 * Name: apicmdgw_checktable
 *
 * Description:
 *   Check and wait for block ready.
 *
 * Input Parameters:
 *   tbl    waittable.
 *   timeout_ms  Response wait timeout value (msec).
 *               When use ALTCOM_SYS_TIMEO_FEVR to waiting non timeout.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static int32_t apicmdgw_checktable(FAR struct apicmdgw_blockinf_s *tbl, int32_t timeout_ms) {
  int32_t ret;

  altcom_sys_lock_mutex(&tbl->waitcondmtx);
  do {
    if (tbl->isBlkReady) {
      ret = 0;
      break;
    }
  } while (0 ==
           (ret = altcom_sys_thread_cond_timedwait(&tbl->waitcond, &tbl->waitcondmtx, timeout_ms)));

  altcom_sys_unlock_mutex(&tbl->waitcondmtx);
  return ret;
}

/****************************************************************************
 * Name: apicmdgw_relcondwaitall
 *
 * Description:
 *   Release all waiting tasks.
 *
 * Input Parameters:
 *   None.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void apicmdgw_relcondwaitall(void) {
  FAR struct apicmdgw_blockinf_s *tmptbl;

  altcom_sys_lock_mutex(&g_blkinfotbl_mtx);

  tmptbl = g_blkinfotbl;
  while (tmptbl) {
    altcom_sys_lock_mutex(&tmptbl->waitcondmtx);
    tmptbl->isBlkReady = 1;
    altcom_sys_signal_thread_cond(&tmptbl->waitcond);
    altcom_sys_unlock_mutex(&tmptbl->waitcondmtx);

    tmptbl = tmptbl->next;
  }

  altcom_sys_unlock_mutex(&g_blkinfotbl_mtx);
}

static void apicmdgw_freereplaylist(struct replay_evt_info_s *replay_list) {
  struct replay_evt_info_s *tmplist;

  while (replay_list) {
    tmplist = replay_list;
    replay_list = replay_list->next;
    BUFFPOOL_FREE(tmplist->buffer);
    BUFFPOOL_FREE(tmplist);
  }
}

static int32_t apicmdgw_replaypostponedevt(struct replay_evt_info_s **replay_list, uint8_t *rcvbuf,
                                           size_t buflen) {
  struct replay_evt_info_s *tmplist;

  if (!replay_list || !(*replay_list) || !rcvbuf || (buflen == 0)) {
    DBGIF_LOG3_ERROR("Invalid parameter, replay_list: %p, rcvbuf: %p, buflen: %lu", replay_list,
                     rcvbuf, (uint32_t)buflen);
    if (replay_list) {
      DBGIF_LOG1_ERROR("Invalid parameter, *replay_list: %p", *replay_list);
    }

    return -EINVAL;
  }

  tmplist = *replay_list;
  if (buflen > tmplist->buflen ||
      ((tmplist->rdptr + buflen) > (tmplist->buffer + tmplist->buflen))) {
    DBGIF_LOG3_ERROR("Corrupted event, buffer: %p, rdptr: %p, buflen: %lu", tmplist->buffer,
                     tmplist->rdptr, (uint32_t)tmplist->buflen);
    DBGIF_LOG1_ERROR("Request buflen: %lu", (uint32_t)buflen);
    *replay_list = tmplist->next;
    BUFFPOOL_FREE(tmplist->buffer);
    BUFFPOOL_FREE(tmplist);
    return -EINVAL;
  }

  memcpy((void *)rcvbuf, (void *)tmplist->rdptr, buflen);
  tmplist->rdptr += buflen;
  if (tmplist->rdptr == tmplist->buffer + tmplist->buflen) {
    *replay_list = tmplist->next;
    BUFFPOOL_FREE(tmplist->buffer);
    BUFFPOOL_FREE(tmplist);
  }

  return buflen;
}

/****************************************************************************
 * Name: apicmdgw_checkpostponeevt
 *
 * Description:
 *   Check and append(at tail) the event to be postponed into an given event replay list.
 *
 * Input Parameters:
 *   postpone_list: The checking list of event to be postponed before application ready.
 *   replay_list: The replaying list to be append if the event id in postponed_list appears in
 *revbuf. rcvbuf: The buffer to be checked. buflen: the length of rcvbuf.
 *
 * Returned Value:
 *   A new replaying list of events to be postponed after application ready.
 *
 ****************************************************************************/
static struct replay_evt_info_s *apicmdgw_checkpostponeevt(enum apiCmdId *postponable_list,
                                                           struct replay_evt_info_s *replay_list,
                                                           void *rcvbuf, size_t buflen) {
  int i;
  enum apiCmdId event_id;
  struct replay_evt_info_s *newevt, *tmplist;

  if (!postponable_list || !rcvbuf || buflen == 0) {
    DBGIF_LOG3_ERROR("Invalid parameter, postpone_list: %p, rcvbuf: %p, buflen: %lu",
                     postponable_list, rcvbuf, (uint32_t)buflen);
    return NULL;
  }

  event_id = (enum apiCmdId)APICMDGW_GET_CMDID(rcvbuf);
  for (i = 0; postponable_list[i] != APICMDID_ERRIND; i++) {
    if (event_id == postponable_list[i]) {
      newevt = BUFFPOOL_ALLOC(sizeof(struct replay_evt_info_s));
      DBGIF_ASSERT(newevt != NULL, "BUFFPOOL_ALLOC error\n");

      newevt->buffer = rcvbuf;
      newevt->rdptr = rcvbuf;
      newevt->buflen = buflen;
      newevt->next = NULL;
      if (!replay_list) {
        return newevt;
      } else {
        /* find and append to tail */
        tmplist = replay_list;
        while (tmplist->next != NULL) {
          tmplist = tmplist->next;
        }

        tmplist->next = newevt;
        return replay_list;
      }
    }
  }

  return NULL;
}

/****************************************************************************
 * Name: apicmdgw_recvtask
 *
 * Description:
 *   Main process of receiving API command gateway
 *
 * Input Parameters:
 *   arg     Option parameter.
 *
 * Returned Value:
 *   none.
 *
 ****************************************************************************/

static void apicmdgw_recvtask(void *arg) {
#define APICMDGW_RECV_MAGICNUMBER_LEN (1)
#define APICMDGW_RECV_STATUS_IDLE (0)
#define APICMDGW_RECV_STATUS_MAGICNUMBER (1)
#define APICMDGW_RECV_STATUS_REPLAY_MAGICNUMBER (2)
#define APICMDGW_RECV_STATUS_HEADER (3)
#define APICMDGW_RECV_STATUS_REPLAY_HEADER (4)
#define APICMDGW_RECV_STATUS_DATA (5)
#define APICMDGW_RECV_STATUS_REPLAY_DATA (6)
#define APICMDGW_MAGICNUMBER_LENGTH (4)
#define APICMDGW_HEADER_REMAIN_LNGTH (12)

  int32_t ret;
  uint32_t magicnum = 0;
  uint8_t recvsts;
  FAR uint8_t *rcvbuff;
  FAR uint8_t *evtbuff;
  FAR uint8_t *rcvptr;
  uint16_t rcvlen;
  uint16_t totallen = 0;
  uint16_t i = 0;
  uint16_t datalen = 0;
  enum apiCmdId *postponable_evt_list = (enum apiCmdId *)arg;
  struct replay_evt_info_s *replay_evt_list;
  int is_postpone_evt = false;

#define APICMDGW_RECV_STATUS_INIT()                                          \
  {                                                                          \
    recvsts = APICMDGW_RECV_STATUS_IDLE;                                     \
    rcvptr = rcvbuff;                                                        \
    rcvlen = APICMDGW_RECV_MAGICNUMBER_LEN;                                  \
    totallen = 0;                                                            \
    DBGIF_LOG1_DEBUG("Next APICMDGW_RECV_STATUS: %lu\n", (uint32_t)recvsts); \
  }

  rcvbuff = (uint8_t *)g_hal_if->allocbuff(g_hal_if, APICMDGW_BUFF_SIZE_MAX);
  DBGIF_ASSERT(rcvbuff, "g_hal_atunsolevt->allocbuff()\n");

  APICMDGW_RECV_STATUS_INIT();
  g_isTaskRun = true;
  is_postpone_evt = postponable_evt_list ? true : false;
  replay_evt_list = NULL;
  while (true) {
    if (rcvlen) {
      if (APICMDGW_BUFF_SIZE_MAX < totallen + rcvlen) {
        memset(rcvbuff, 0, APICMDGW_BUFF_SIZE_MAX);
        APICMDGW_RECV_STATUS_INIT();
      }

      if (is_postpone_evt && recvsts == APICMDGW_RECV_STATUS_IDLE) {
        if (altcom_sys_wait_semaphore(g_checkpostpone_sem, ALTCOM_SYS_TIMEO_NOWAIT) == 0) {
          is_postpone_evt = false;
          altcom_sys_post_semaphore(g_checkpostpone_sem);
          DBGIF_LOG_DEBUG("Start to process postponed events\n");
        }
      }

      if (is_postpone_evt || !replay_evt_list) {
        ret = g_hal_if->recv(g_hal_if, rcvptr, rcvlen);
        if (recvsts == APICMDGW_RECV_STATUS_IDLE && ret > 0) {
          recvsts = APICMDGW_RECV_STATUS_MAGICNUMBER;
          DBGIF_LOG1_DEBUG("Next APICMDGW_RECV_STATUS: %lu\n", (uint32_t)recvsts);
        }
      } else {
        ret = apicmdgw_replaypostponedevt(&replay_evt_list, rcvptr, (size_t)rcvlen);
        if (recvsts == APICMDGW_RECV_STATUS_IDLE && ret > 0) {
          recvsts = APICMDGW_RECV_STATUS_REPLAY_MAGICNUMBER;
          DBGIF_LOG1_DEBUG("Next APICMDGW_RECV_STATUS: %lu\n", (uint32_t)recvsts);
        }
      }
    } else {
      ret = 0;
    }

    if (0 > ret) {
      if (-ECONNABORTED == ret) {
        DBGIF_LOG_NORMAL("recv() abort and terminate\n");
        altcom_sys_lock_mutex(&g_delwaitcondmtx);
        break;
      } else if (-EAGAIN == ret) {
        DBGIF_LOG_NORMAL("recv() abort and again\n");
        continue;
      } else {
        DBGIF_LOG1_ERROR("recv() [errno=%ld]\n", ret);
        APICMDGW_RECV_STATUS_INIT();
        continue;
      }
    } else {
      totallen += ret;
      switch (recvsts) {
        case APICMDGW_RECV_STATUS_MAGICNUMBER:
        case APICMDGW_RECV_STATUS_REPLAY_MAGICNUMBER: {
          if (APICMDGW_MAGICNUMBER_LENGTH <= totallen) {
            for (i = 0; i <= totallen - APICMDGW_MAGICNUMBER_LENGTH; i++) {
              memcpy(&magicnum, &rcvbuff[i], APICMDGW_MAGICNUMBER_LENGTH);
              if (APICMD_MAGICNUMBER == ntohl(magicnum)) {
                memmove(rcvbuff, &rcvbuff[i], APICMDGW_MAGICNUMBER_LENGTH);
                memset(rcvbuff + APICMDGW_MAGICNUMBER_LENGTH, 0, &rcvbuff[i] - rcvbuff);
                totallen = APICMDGW_MAGICNUMBER_LENGTH;
                recvsts = (recvsts == APICMDGW_RECV_STATUS_MAGICNUMBER)
                              ? APICMDGW_RECV_STATUS_HEADER
                              : APICMDGW_RECV_STATUS_REPLAY_HEADER;
                rcvptr = rcvbuff + APICMDGW_MAGICNUMBER_LENGTH;
                rcvlen = APICMDGW_HEADER_REMAIN_LNGTH;
                DBGIF_LOG1_DEBUG("Next APICMDGW_RECV_STATUS: %lu\n", (uint32_t)recvsts);
                break;
              }
            }
          }
          if (APICMDGW_RECV_STATUS_HEADER != recvsts &&
              APICMDGW_RECV_STATUS_REPLAY_HEADER != recvsts) {
            rcvptr++;
            rcvlen = APICMDGW_RECV_MAGICNUMBER_LEN;
          }
        } break;

        case APICMDGW_RECV_STATUS_HEADER:
        case APICMDGW_RECV_STATUS_REPLAY_HEADER: {
          if (totallen == APICMDGW_APICMDHDR_LEN) {
            if (0 == apicmdgw_checkheader(rcvbuff)) {
              datalen = APICMDGW_GET_DATA_LEN(rcvbuff);
              recvsts = (recvsts == APICMDGW_RECV_STATUS_HEADER) ? APICMDGW_RECV_STATUS_DATA
                                                                 : APICMDGW_RECV_STATUS_REPLAY_DATA;
              rcvptr = rcvbuff + APICMDGW_APICMDHDR_LEN;
              rcvlen = datalen;
              DBGIF_LOG1_DEBUG("Next APICMDGW_RECV_STATUS: %lu\n", (uint32_t)recvsts);
            } else {
              DBGIF_LOG_ERROR("Header CHKSUM NOK\n");
              apicmdgw_errind((FAR struct apicmd_cmdhdr_s *)rcvbuff);
              APICMDGW_RECV_STATUS_INIT();
            }
          } else {
            rcvptr += ret;
            rcvlen = APICMDGW_APICMDHDR_LEN - totallen;
          }
        } break;

        case APICMDGW_RECV_STATUS_DATA:
        case APICMDGW_RECV_STATUS_REPLAY_DATA: {
          if (totallen == APICMDGW_APICMDHDR_LEN + datalen) {
            if (0 != apicmdgw_checkdata(rcvbuff)) {
              DBGIF_LOG_ERROR("Data CHKSUM NOK\n");
              apicmdgw_errind((FAR struct apicmd_cmdhdr_s *)rcvbuff);
              APICMDGW_RECV_STATUS_INIT();
              break;
            }

            if (!apicmdgw_writetable(APICMDGW_GET_CMDID(rcvbuff), APICMDGW_GET_TRANSID(rcvbuff),
                                     APICMDGW_GET_DATA_PTR(rcvbuff), datalen)) {
              evtbuff = (uint8_t *)g_hal_if->allocbuff(g_hal_if, totallen);
              DBGIF_ASSERT(evtbuff, "BUFFPOOL_ALLOC() error.\n");
              memcpy(evtbuff, rcvbuff, totallen);
              struct replay_evt_info_s *tmplist = NULL;
              if (is_postpone_evt) {
                tmplist = apicmdgw_checkpostponeevt(postponable_evt_list, replay_evt_list, evtbuff,
                                                    totallen);
              }

              if (!tmplist) {
                ret = g_evtdisp->dispatch(g_evtdisp, APICMDGW_GET_DATA_PTR(evtbuff),
                                          APICMDGW_GET_DATA_LEN(evtbuff));
                if (0 > ret) {
                  apicmdgw_errhandle((FAR struct apicmd_cmdhdr_s *)evtbuff);
                  g_hal_if->freebuff(g_hal_if, evtbuff);
                  DBGIF_LOG1_DEBUG("dispatch() [errno=%ld]\n", ret);
                }
              } else {
                replay_evt_list = tmplist;
              }
            }

            APICMDGW_RECV_STATUS_INIT();
          } else {
            rcvptr += ret;
            rcvlen = APICMDGW_APICMDHDR_LEN + datalen - totallen;
          }
        } break;

        default:
          DBGIF_LOG1_ERROR("recvsts = %d\n", recvsts);
          APICMDGW_RECV_STATUS_INIT();
          continue;
      }
    }
  }

  if (rcvbuff) {
    g_hal_if->freebuff(g_hal_if, (void *)rcvbuff);
  }

  apicmdgw_freereplaylist(replay_evt_list);

  g_isTaskRun = false;
  ret = altcom_sys_signal_thread_cond(&g_delwaitcond);
  DBGIF_ASSERT(0 == ret, "altcom_sys_signal_thread_cond().\n");

  altcom_sys_unlock_mutex(&g_delwaitcondmtx);

  ret = altcom_sys_delete_task(ALTCOM_SYS_OWN_TASK);
  DBGIF_ASSERT(0 == ret, "altcom_sys_delete_task()\n");
}

static int32_t __apicmdgw_send(FAR uint8_t *cmd, FAR uint8_t *respbuff, uint16_t bufflen,
                               FAR uint16_t *resplen, int32_t timeout_ms) {
  int32_t ret;
  uint32_t sendlen;
  FAR struct apicmd_cmdhdr_s *hdr_ptr;
  FAR struct apicmdgw_blockinf_s *blocktbl = NULL;

  if (!g_isinit) {
    DBGIF_LOG_ERROR("apicmd gw in not initialized.\n");
    return -EPERM;
  }

  if (!cmd || (respbuff && !resplen)) {
    DBGIF_LOG_ERROR("Invalid argument.\n");
    return -EINVAL;
  }

  hdr_ptr = (FAR struct apicmd_cmdhdr_s *)APICMDGW_GET_HDR_PTR(cmd);
  hdr_ptr->dtchksum = apicmdgw_createchksum((FAR uint8_t *)cmd, ntohs(hdr_ptr->dtlen));
  hdr_ptr->dtchksum = htons(hdr_ptr->dtchksum);
  if (respbuff) {
    blocktbl = (FAR struct apicmdgw_blockinf_s *)BUFFPOOL_ALLOC(sizeof(struct apicmdgw_blockinf_s));
    if (!blocktbl) {
      DBGIF_LOG_ERROR("BUFFPOOL_ALLOC() failed.\n");
      return -ENOSPC;
    }

    /* Set wait table. */

    blocktbl->transid = APICMDGW_GET_TRANSID(hdr_ptr);
    blocktbl->recvbuff = respbuff;
    blocktbl->bufflen = bufflen;
    blocktbl->cmdid = APICMDGW_GET_RESCMDID(APICMDGW_GET_CMDID(hdr_ptr));
    blocktbl->recvlen = resplen;
    blocktbl->isBlkReady = 0;
    ret = altcom_sys_create_thread_cond_mutex(&blocktbl->waitcond, &blocktbl->waitcondmtx);
    if (0 > ret) {
      BUFFPOOL_FREE(blocktbl);
      return ret;
    }

    apicmdgw_addtable(blocktbl);
  }

  sendlen = ntohs(hdr_ptr->dtlen) + APICMDGW_APICMDHDR_LEN;
  g_hal_if->lock(g_hal_if);
  ret = g_hal_if->send(g_hal_if, (FAR uint8_t *)hdr_ptr, sendlen);
  g_hal_if->unlock(g_hal_if);

  if (0 > ret) {
    DBGIF_LOG_ERROR("hal_if->send() failed.\n");
    if (respbuff) {
      apicmdgw_remtable(blocktbl);
    }

    return ret;
  }

  if (respbuff) {
    ret = apicmdgw_checktable(blocktbl, timeout_ms);
    if (0 > ret) {
      ret = -ETIMEDOUT;
    } else {
      if (0 > blocktbl->result) {
        ret = blocktbl->result;
      }

      if (!g_isinit) {
        ret = -ECONNABORTED;
      }
    }

    apicmdgw_remtable(blocktbl);
  }

  if (0 <= ret) {
    ret = ntohs(hdr_ptr->dtlen);
  }

  return ret;
}

int32_t apicmdgw_echo(void) {
  int32_t ret;
  uint16_t resLen = 0;
  struct apicmd_echo_ext_s *cmd = NULL;
  struct apicmd_echores_ext_s *res = NULL;
  uint16_t sendLen;
  uint16_t resBufLen;

  /* Check if the library is initialized */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return -EINVAL;
  }

  sendLen = 1 + AVAIL_MODULEINFO_LEN + sizeof(struct apicmd_echo_s);
  resBufLen = sizeof(struct apicmd_echores_ext_s);
  if (altcom_generic_alloc_cmdandresbuff((void **)&cmd, APICMDID_ECHO, sendLen, (void **)&res,
                                         resBufLen)) {
    cmd->dummy = ECHO_CHAR;
    cmd->moduleInfoCnt = g_moduleInfoCnt;
    if (0 != g_moduleInfoCnt) {
      memcpy((void *)cmd->moduleInfo, (void *)g_moduleInfo, AVAIL_MODULEINFO_LEN);
    }

    /* Send API command to modem */
    ret = __apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, resBufLen, &resLen, ECHO_TIMEOUT);
  } else {
    DBGIF_LOG_ERROR("Failed to allocate command buffer.\n");
    return -ENOMEM;
  }

  if (0 > ret) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    goto errout_with_cmdfree;
  }

  if (sizeof(struct apicmd_echores_s) > resLen) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %hu\n", resLen);
    ret = -EINVAL;
    goto errout_with_cmdfree;
  }

  if (0 > (ret = (cmd->dummy != res->dummy) ? -1 : 0)) {
    DBGIF_LOG2_ERROR("Incorrect dummy character, cmd:0x%X, res:0x%X\n", (unsigned int)cmd->dummy,
                     (unsigned int)res->dummy);
    ret = -EINVAL;
    goto errout_with_cmdfree;
  }

  /* Here we start to check and compare with MAP's module version information */
  if (sizeof(struct apicmd_echores_s) < resLen) {
    uint8_t i, j;
    uint8_t mapModuleInfoCnt;
    struct apicmd_moduleinfo_s *mapModuleInfo;

    mapModuleInfoCnt = res->moduleInfoCnt;
    mapModuleInfo = res->moduleInfo;
    for (i = 0; i < g_moduleInfoCnt; i++) {
      for (j = 0; j < mapModuleInfoCnt; j++) {
        if (g_moduleInfo[i].moduleId == mapModuleInfo[j].moduleId) {
          if (g_moduleInfo[i].majorVer != mapModuleInfo[j].majorVer ||
              g_moduleInfo[i].minorVer != mapModuleInfo[j].minorVer) {
            DBGIF_LOG1_ERROR("Module 0x%02X version mismatched!\n",
                             (unsigned int)(mapModuleInfo[j].moduleId & 0xFF));
            DBGIF_LOG2_ERROR("MCU: V%u.%u\n", (unsigned int)g_moduleInfo[i].majorVer,
                             (unsigned int)g_moduleInfo[i].minorVer);
            DBGIF_LOG2_ERROR("MAP: V%u.%u\n", (unsigned int)mapModuleInfo[j].majorVer,
                             (unsigned int)mapModuleInfo[j].minorVer);
          } else {
            DBGIF_LOG3_DEBUG("Module 0x%02X version matched! V%u.%u\n",
                             (unsigned int)(g_moduleInfo[i].moduleId & 0xFF),
                             (unsigned int)g_moduleInfo[i].majorVer,
                             (unsigned int)g_moduleInfo[i].minorVer);
          }

          break;
        }
      }

      if (j == mapModuleInfoCnt) {
        DBGIF_LOG1_ERROR("Module 0x%02X not supported(or not enabled) !\n",
                         (unsigned int)(g_moduleInfo[i].moduleId & 0xFF));
      }
    }
  } else {
    DBGIF_LOG_WARNING("No module version information from MAP\n");
  }

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return ret;
}

static void apicmdgw_echotask(void *arg) {
  uint32_t retryCnt = 0;
  int32_t ret;

  while (ECHO_RETRYCNT > retryCnt++) {
    DBGIF_LOG1_DEBUG("Try echo %lu\n", retryCnt);
    ret = apicmdgw_echo();
    if (0 == ret) {
      DBGIF_LOG1_DEBUG("Got echo response at %lu time.\n", retryCnt);
      altcom_sys_lock_mutex(&g_stateCondMtx);
      g_apicmdgwState = APICMDGW_ECHO_COMPLETED;
      altcom_sys_broadcast_thread_cond(&g_stateCond);
      altcom_sys_unlock_mutex(&g_stateCondMtx);
      break;
    }

    altcom_sys_sleep_task(1000);
  }

  if (ECHO_RETRYCNT <= retryCnt) {
    DBGIF_LOG1_DEBUG("Echo retry count exceeded, %lu.\n", retryCnt);
    altcom_sys_lock_mutex(&g_stateCondMtx);
    g_apicmdgwState = APICMDGW_FIN;
    altcom_sys_broadcast_thread_cond(&g_stateCond);
    altcom_sys_unlock_mutex(&g_stateCondMtx);
  }

  DBGIF_LOG1_DEBUG("%s leave.\n", __FUNCTION__);
  altcom_sys_delete_task(ALTCOM_SYS_OWN_TASK);
}

int32_t apicmdgw_recvagain(void) {
  int32_t ret;

  ret = g_hal_if->abortrecv(g_hal_if, HAL_ABORT_AGAIN);
  DBGIF_ASSERT(0 <= ret, "abortrecv()\n");

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: apicmdgw_init
 *
 * Description:
 *   Initialize api command gateway instance.
 *
 * Input Parameters:
 *   halif    hal_if_s pointer.
 *
 * Returned Value:
 *   If the process succeeds, it returns 0.
 *   Otherwise errno is returned.
 *
 ****************************************************************************/

int32_t apicmdgw_init(FAR struct apicmdgw_set_s *set) {
  int32_t ret;
  altcom_sys_cretask_s taskset;
  char thname[] = "apicmdgw_main";
  altcom_sys_cresem_s sem_param = {.initial_count = 0, .max_count = 1};

  if (!set || !set->halif || !set->dispatcher) {
    DBGIF_LOG_ERROR("null parameter.\n");
    return -EINVAL;
  }

  if (g_isinit) {
    DBGIF_LOG_ERROR("apicmdgw is initialized.\n");
    return -EPERM;
  }

  g_hal_if = set->halif;
  g_evtdisp = set->dispatcher;

  ret = altcom_sys_create_thread_cond_mutex(&g_delwaitcond, &g_delwaitcondmtx);
  DBGIF_ASSERT(0 == ret, "altcom_sys_create_thread_cond_mutex().\n");

  if (ret != 0) {
    goto delwaitconderr;
  }

  if (0 == g_stateCond && NULL == g_stateCondMtx) {
    ret = altcom_sys_create_thread_cond_mutex(&g_stateCond, &g_stateCondMtx);
    DBGIF_ASSERT(0 == ret, "altcom_sys_create_thread_cond_mutex().\n");

    if (ret != 0) {
      goto stateconderr;
    }
  }

  altcom_sys_lock_mutex(&g_stateCondMtx);
  if (set->bypass_echo == 0) {
    g_apicmdgwState = APICMDGW_INIT;
  } else {
    g_apicmdgwState = APICMDGW_ECHO_COMPLETED;
  }

  altcom_sys_unlock_mutex(&g_stateCondMtx);

  ret = altcom_sys_create_semaphore(&g_checkpostpone_sem, &sem_param);
  DBGIF_ASSERT(0 == ret, "g_checkpostpone_sem create failed.\n");

  if (ret != 0) {
    goto checkpostponedsemerr;
  }

  taskset.function = apicmdgw_recvtask;
  taskset.name = (FAR int8_t *)thname;
  taskset.priority = ALTCOM_SYS_TASK_PRIO_HIGH;
  taskset.stack_size = APICMDGW_MAIN_TASK_STACK_SIZE;
  taskset.arg = set->postponable_evt_list;

  ret = altcom_sys_create_mutex(&g_blkinfotbl_mtx, &g_mtxparam);
  DBGIF_ASSERT(0 == ret, "altcom_sys_create_mutex().\n");

  if (ret != 0) {
    goto blkintotblmtxderr;
  }

  ret = altcom_sys_create_task(&g_rcvtask, &taskset);
  DBGIF_ASSERT(0 == ret, "altcom_sys_create_task().\n");

  if (ret != 0) {
    goto rcvtaskerr;
  }

  g_isinit = true;
  return ret;

rcvtaskerr:
  altcom_sys_delete_mutex(&g_blkinfotbl_mtx);

blkintotblmtxderr:
  altcom_sys_delete_semaphore(&g_checkpostpone_sem);

checkpostponedsemerr:
  altcom_sys_delete_thread_cond_mutex(&g_stateCond, &g_stateCondMtx);

stateconderr:
  altcom_sys_delete_thread_cond_mutex(&g_delwaitcond, &g_delwaitcondmtx);

delwaitconderr:
  return ret;
}

/****************************************************************************
 * Name: apicmdgw_fin
 *
 * Description:
 *   Finalize api command gateway instance.
 *
 * Input Parameters:
 *   None.
 *
 * Returned Value:
 *   If the process succeeds, it returns 0.
 *   Otherwise errno is returned.
 *
 ****************************************************************************/

int32_t apicmdgw_fin(void) {
  int32_t ret;

  if (!g_isinit) {
    DBGIF_LOG_ERROR("apicmdgw is finalized.\n");
    return -EPERM;
  }

  g_isinit = false;

  ret = g_hal_if->abortrecv(g_hal_if, HAL_ABORT_TERMINATE);
  DBGIF_ASSERT(0 <= ret, "abortrecv()\n");

  altcom_sys_lock_mutex(&g_stateCondMtx);
  g_apicmdgwState = APICMDGW_FIN;
  altcom_sys_broadcast_thread_cond(&g_stateCond);
  altcom_sys_unlock_mutex(&g_stateCondMtx);

  altcom_sys_lock_mutex(&g_delwaitcondmtx);
  do {
    if (!g_isTaskRun) {
      break;
    }
  } while (0 == (ret = altcom_sys_thread_cond_wait(&g_delwaitcond, &g_delwaitcondmtx)));

  altcom_sys_unlock_mutex(&g_delwaitcondmtx);
  altcom_sys_delete_thread_cond_mutex(&g_delwaitcond, &g_delwaitcondmtx);
  apicmdgw_relcondwaitall();

  ret = altcom_sys_delete_semaphore(&g_checkpostpone_sem);
  DBGIF_ASSERT(0 == ret, "altcom_sys_delete_semaphore().\n");

  ret = altcom_sys_delete_mutex(&g_blkinfotbl_mtx);
  DBGIF_ASSERT(0 == ret, "altcom_sys_delete_mutex().\n");

  g_hal_if = NULL;
  g_evtdisp = NULL;

  return ret;
}

/****************************************************************************
 * Name: apicmdgw_send
 *
 * Description:
 *   Send api command.
 *   And wait to response for parameter of timeout_ms value when
 *   parameter of respbuff set valid buffer.
 *   Non wait to response when parameter of respbuff set NULL.
 *
 * Input Parameters:
 *   cmd         Send command payload pointer.
 *   respbuff    Response buffer.
 *   bufflen     @respbuff length.
 *   resplen     Response length.
 *   timeout_ms  Response wait timeout value (msec).
 *               When use ALTCOM_SYS_TIMEO_FEVR to waiting non timeout.
 *
 * Returned Value:
 *   On success, the length of the sent command in bytes is returned.
 *   On failure, negative value is returned.
 *
 ****************************************************************************/

int32_t apicmdgw_send(FAR uint8_t *cmd, FAR uint8_t *respbuff, uint16_t bufflen,
                      FAR uint16_t *resplen, int32_t timeout_ms) {
  int32_t ret;
  altcom_sys_cretask_s taskset;
  char thname[configMAX_TASK_NAME_LEN] = "echotask";

  altcom_sys_lock_mutex(&g_stateCondMtx);
  switch (g_apicmdgwState) {
    case APICMDGW_INIT:
      g_apicmdgwState = APICMDGW_ECHO_TESTING;
      taskset.function = apicmdgw_echotask;
      taskset.arg = NULL;
      taskset.name = (FAR int8_t *)thname;
      taskset.priority = ALTCOM_SYS_TASK_PRIO_HIGH;
      taskset.stack_size = APICMDGW_ECHOTASK_STACK_SIZE;
      ret = altcom_sys_create_task(&g_echotask, &taskset);
      DBGIF_ASSERT(0 == ret, "altcom_sys_create_task().\n");

      /* Here we recursive call apicmdgw_send to make the target cmd wait until echo replied */
      altcom_sys_unlock_mutex(&g_stateCondMtx);
      return apicmdgw_send(cmd, respbuff, bufflen, resplen, timeout_ms);
      break;

    case APICMDGW_ECHO_TESTING:
      ret = altcom_sys_thread_cond_timedwait(&g_stateCond, &g_stateCondMtx, timeout_ms);
      if (0 > ret) {
        ret = -ETIMEDOUT;
      } else {
        /* Here we recursive call apicmdgw_send to send the target cmd */
        altcom_sys_unlock_mutex(&g_stateCondMtx);
        return apicmdgw_send(cmd, respbuff, bufflen, resplen, timeout_ms);
      }

      break;

    case APICMDGW_ECHO_COMPLETED:
      altcom_sys_unlock_mutex(&g_stateCondMtx);
      return __apicmdgw_send(cmd, respbuff, bufflen, resplen, timeout_ms);

    case APICMDGW_FIN:
      DBGIF_LOG_ERROR("APICMDGW already finished\n");
      ret = -ECONNABORTED;
      break;

    default:
      DBGIF_LOG1_ERROR("Incorrect APICMDGW state: %lu\n", (uint32_t)g_apicmdgwState);
      ret = -EINVAL;
      DBGIF_ASSERT(false, "We are assert here!");
      break;
  }

  altcom_sys_unlock_mutex(&g_stateCondMtx);
  return ret;
}

/****************************************************************************
 * Name: apicmdgw_cmd_allocbuff
 *
 * Description:
 *   Allocate buffer for API command to be sent. The length to be allocated
 *   is the sum of the data length and header length.
 *   And this function is make api command header in allocated buffer.
 *
 * Input Parameters:
 *   cmdid    Api command id.
 *   len      Length of data field.
 *
 * Returned Value:
 *   If succeeds allocate buffer, start address of the data field
 *   is returned. Otherwise NULL is returned.
 *
 ****************************************************************************/

FAR uint8_t *apicmdgw_cmd_allocbuff(uint16_t cmdid, uint16_t len) {
  FAR struct apicmd_cmdhdr_s *buff = NULL;

  if (!g_isinit) {
    DBGIF_LOG_ERROR("apicmd gw in not initialized.\n");
    return NULL;
  }

  if (APICMDGW_APICMDPAYLOAD_SIZE_MAX < len) {
    DBGIF_LOG1_ERROR("Over max API command data size. len:%d\n", len);
    return NULL;
  }

  buff = (FAR struct apicmd_cmdhdr_s *)g_hal_if->allocbuff(g_hal_if, len + APICMDGW_APICMDHDR_LEN);
  if (!buff) {
    DBGIF_LOG_ERROR("hal_if->allocbuff failed.\n");
    return NULL;
  }

  /* Make header. */
  g_hal_if->lock(g_hal_if);
  buff->magic = htonl(APICMD_MAGICNUMBER);
  buff->ver = APICMD_VER;
  buff->seqid = APICMDGW_GET_SEQID;
  buff->cmdid = htons(cmdid);
  buff->transid = apicmdgw_createtransid();
  buff->transid = htons(buff->transid);
  buff->dtlen = htons(len);
  buff->chksum = apicmdgw_createchksum((FAR uint8_t *)buff, APICMDGW_CHKSUM_LENGTH);
  buff->chksum = htons(buff->chksum);
  buff->dtchksum = 0;
  g_hal_if->unlock(g_hal_if);

  return APICMDGW_GET_DATA_PTR(buff);
}

/****************************************************************************
 * Name: apicmdgw_reply_allocbuff
 *
 * Description:
 *   Allocate buffer for API command to be sent. The length to be allocated
 *   is the sum of the data length and header length.
 *   And this function is make api resopnse command header in allocated buffer.
 *
 * Input Parameters:
 *   cmd      Replyning to api command payload pointer.
 *   len      Length of data field.
 *
 * Returned Value:
 *   If succeeds allocate buffer, start address of the data field
 *   is returned. Otherwise NULL is returned.
 *
 ****************************************************************************/

FAR uint8_t *apicmdgw_reply_allocbuff(FAR const uint8_t *cmd, uint16_t len) {
  FAR struct apicmd_cmdhdr_s *buff = NULL;
  FAR struct apicmd_cmdhdr_s *evthdr = NULL;

  if (!g_isinit) {
    DBGIF_LOG_ERROR("apicmd gw in not initialized.\n");
    return NULL;
  }

  if (!cmd) {
    DBGIF_LOG_ERROR("Invalid argument.\n");
    return NULL;
  }

  if (APICMDGW_APICMDPAYLOAD_SIZE_MAX < len) {
    DBGIF_LOG1_ERROR("Over max API command data size. len:%d\n", len);
    return NULL;
  }

  buff = (FAR struct apicmd_cmdhdr_s *)g_hal_if->allocbuff(g_hal_if, len + APICMDGW_APICMDHDR_LEN);
  if (!buff) {
    DBGIF_LOG_ERROR("hal_if->allocbuff failed.\n");
    return NULL;
  }

  /* Make reply header. */

  evthdr = (FAR struct apicmd_cmdhdr_s *)APICMDGW_GET_HDR_PTR(cmd);
  buff->magic = htonl(APICMD_MAGICNUMBER);
  buff->ver = APICMD_VER;
  buff->seqid = APICMDGW_GET_SEQID;
  buff->cmdid = htons(APICMDGW_GET_RESCMDID(APICMDGW_GET_CMDID(evthdr)));
  buff->transid = evthdr->transid;
  buff->dtlen = htons(len);
  buff->chksum = apicmdgw_createchksum((FAR uint8_t *)buff, APICMDGW_CHKSUM_LENGTH);
  buff->chksum = htons(buff->chksum);
  buff->dtchksum = 0;

  return APICMDGW_GET_DATA_PTR(buff);
}

/****************************************************************************
 * Name: apicmdgw_freebuff
 *
 * Description:
 *   Free allocated buffer.
 *
 * Input Parameters:
 *   buff  Pointer to data field.
 *
 * Returned Value:
 *   If the process succeeds, it returns 0.
 *   Otherwise errno in errno.h is returned.
 *
 ****************************************************************************/

int32_t apicmdgw_freebuff(FAR uint8_t *buff) {
  if (!g_isinit) {
    DBGIF_LOG_ERROR("apicmd gw in not initialized.\n");
    return -EPERM;
  }

  if (!buff) {
    DBGIF_LOG_INFO("freebuff target buffer is null.\n");
    return 0;
  }

  return g_hal_if->freebuff(g_hal_if, (FAR void *)APICMDGW_GET_HDR_PTR(buff));
}

/****************************************************************************
 * Name: apicmdgw_cmdid_compare
 *
 * Description:
 *   Compare to receive event command id and waiting command id.
 *
 * Input Parameters:
 *   cmd      Receive command payload pointer.
 *   cmdid    Waiting command id.
 *
 * Returned Value:
 *   If the process succeeds, it returns true.
 *   Otherwise false is returned.
 *
 ****************************************************************************/

bool apicmdgw_cmdid_compare(FAR uint8_t *cmd, uint16_t cmdid) {
  uint16_t rcvid;

  if (!cmd) {
    return false;
  }

  rcvid = APICMDGW_GET_CMDID(APICMDGW_GET_HDR_PTR(cmd));
  if (rcvid == cmdid) {
    return true;
  }

  return false;
}

/****************************************************************************
 * Name: apicmdgw_replay_postponed_event
 *
 * Description:
 *   Start to replay the postponed events in apicmdgw_recvtask
 ****************************************************************************/
void apicmdgw_replay_postponed_event(void) { altcom_sys_post_semaphore(g_checkpostpone_sem); }