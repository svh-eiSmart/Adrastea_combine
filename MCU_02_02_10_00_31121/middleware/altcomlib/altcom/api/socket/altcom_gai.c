/****************************************************************************
 * modules/lte/altcom/api/socket/altcom_gai.c
 *
 *   Copyright 2021 Sony Semiconductor Solutions Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Sony Semiconductor Solutions Corporation nor
 *    the names of its contributors may be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <string.h>
#include <stdbool.h>

#include "dbg_if.h"
#include "buffpoolwrapper.h"
#include "altcom_gai.h"
#include "apiutil.h"
#include "apicmd_gai.h"
#include "altcom_cc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define GAI_REQ_DATALEN (sizeof(struct apicmd_gai_s))
#define GAI_RES_DATALEN (sizeof(struct apicmd_gaires_s))

#define LTE_SESSION_ID_MAX (15) /**< Maximum value of session ID */

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct gai_req_s {
  uint8_t session_id;
  const char *nodename;
  const char *servname;
  const struct altcom_addrinfo *hints;
  struct altcom_addrinfo **res;
};

struct gai_async_cb_s {
  int32_t callback_id;
  altcom_getaddrinfo_ext_cb_t callback;
  void *priv;
  struct gai_async_cb_s *next;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct gai_async_cb_s *callbacklist_head = NULL;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: allocate_callbacklist
 *
 * Description:
 *   Allocate getaddrinfo callback list.
 *
 * Input Parameters:
 *   callback_id  This is ID that identifies the getaddrinfo request.
 *
 * Returned Value:
 *   If the allocation succeeds, it returns the allocated pointer.
 *   Otherwise NULL is returned.
 *
 ****************************************************************************/

static struct gai_async_cb_s *allocate_callbacklist(int32_t callback_id) {
  struct gai_async_cb_s *list;

  /* Allocate callback list */

  list = (struct gai_async_cb_s *)BUFFPOOL_ALLOC(sizeof(struct gai_async_cb_s));
  if (!list) {
    DBGIF_LOG_ERROR("Failed to allocate memory.\n");
    return NULL;
  }

  memset(list, 0, sizeof(struct gai_async_cb_s));

  /* Initialize cllback list */

  list->callback_id = callback_id;
  list->next = NULL;

  return list;
}

/****************************************************************************
 * Name: free_callbacklist
 *
 * Description:
 *   Free getaddrinfo callback list.
 *
 * Input Parameters:
 *   list  Pointer to callback list.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void free_callbacklist(struct gai_async_cb_s *list) { (void)BUFFPOOL_FREE((void *)list); }

/****************************************************************************
 * Name: add_callbacklist
 *
 * Description:
 *   Add callback list to the end of list.
 *
 * Input Parameters:
 *   list  Pointer to callback list.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void add_callbacklist(struct gai_async_cb_s *list) {
  struct gai_async_cb_s *list_ptr = callbacklist_head;

  if (callbacklist_head) {
    /* Search end of list */

    while (list_ptr->next) {
      list_ptr = list_ptr->next;
    }
    list_ptr->next = list;
  } else {
    callbacklist_head = list;
  }
}

/****************************************************************************
 * Name: delete_callbacklist
 *
 * Description:
 *   Delete list from callback list.
 *
 * Input Parameters:
 *   list  Pointer to callback list.
 *
 * Returned Value:
 *   If the process succeeds, it returns 0.
 *   Otherwise negative value is returned.
 *
 ****************************************************************************/

static int32_t delete_callbacklist(struct gai_async_cb_s *list) {
  struct gai_async_cb_s *list_ptr = callbacklist_head;
  struct gai_async_cb_s *prev_ptr = NULL;

  if (callbacklist_head) {
    while (list_ptr) {
      if (list_ptr == list) {
        /* Check begining of the list */

        if (prev_ptr) {
          prev_ptr->next = list_ptr->next;
        } else {
          callbacklist_head = list_ptr->next;
        }

        return 0;
      }
      prev_ptr = list_ptr;
      list_ptr = list_ptr->next;
    }
  }

  return -1;
}

/****************************************************************************
 * Name: search_callbacklist
 *
 * Description:
 *   Search list by getaddrinfo ID.
 *
 * Input Parameters:
 *   callback_id  Target getaddrinfo ID.
 *
 * Returned Value:
 *   If list is found, return that pointer.
 *   Otherwise NULL is returned.
 *
 ****************************************************************************/

static struct gai_async_cb_s *search_callbacklist(int32_t callback_id) {
  struct gai_async_cb_s *list_ptr = callbacklist_head;

  while (list_ptr) {
    if (list_ptr->callback_id == callback_id) {
      return list_ptr;
    }
    list_ptr = list_ptr->next;
  }

  return NULL;
}

/****************************************************************************
 * Name: setup_callback
 *
 * Description:
 *   Register callback to be used for getaddrinfo async.
 *
 * Input Parameters:
 *   callback_id  This is ID that identifies the getaddrinfo request.
 *   cb         Callback function when received getaddrinfo response.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void setup_callback(int32_t callback_id, altcom_getaddrinfo_ext_cb_t cb, void *priv) {
  struct gai_async_cb_s *list;

  /* Check if getaddrinfo ID is already in use */

  if (!search_callbacklist(callback_id)) {
    list = allocate_callbacklist(callback_id);
    if (list) {
      list->callback = cb;
      list->priv = priv;

      /* Add list to the end of list */

      add_callbacklist(list);
    }
  } else {
    DBGIF_LOG1_WARNING("This getaddrinfo id[%ld] already in use.\n", callback_id);
  }
}

/****************************************************************************
 * Name: teardown_callback
 *
 * Description:
 *   Unregister callback to be used for getaddrinfo async.
 *
 * Input Parameters:
 *   callback_id  This is ID that identifies the getaddrinfo request.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void teardown_callback(int32_t callback_id) {
  struct gai_async_cb_s *list;
  int32_t ret;

  list = search_callbacklist(callback_id);
  if (list) {
    /* Delte list from callback list */

    ret = delete_callbacklist(list);
    if (ret < 0) {
      DBGIF_LOG_ERROR("Failed to delete callbck list.\n");
    } else {
      free_callbacklist(list);
    }
  }
}

static int32_t generate_callback_id(void) {
  static int32_t callback_id = 0;

  callback_id++;

  return (callback_id & 0x7fffffff);
}

static void fill_gai_request_command(int32_t callback_id, struct apicmd_gai_s *cmd,
                                     struct gai_req_s *req) {
  uint32_t nodenamelen;
  uint32_t servnamelen;

  memset(cmd, 0, sizeof(*cmd));

  cmd->callback_id = htonl(callback_id);
  cmd->session_id = req->session_id;

  if (req->nodename) {
    nodenamelen = strnlen(req->nodename, APICMD_GETADDRINFO_NODENAME_MAX_LENGTH);
    cmd->nodenamelen = htonl(nodenamelen);
    memcpy(cmd->nodename, req->nodename, nodenamelen);
  }

  if (req->servname) {
    servnamelen = strnlen(req->servname, APICMD_GETADDRINFO_SERVNAME_MAX_LENGTH);
    memcpy(cmd->servname, req->servname, servnamelen);
    cmd->servnamelen = htonl(servnamelen);
  }

  if (req->hints) {
    cmd->hints_flag = htonl(APICMD_GETADDRINFO_HINTS_FLAG_ENABLE);
    cmd->ai_flags = htonl(req->hints->ai_flags);
    cmd->ai_family = htonl(req->hints->ai_family);
    cmd->ai_socktype = htonl(req->hints->ai_socktype);
    cmd->ai_protocol = htonl(req->hints->ai_protocol);
  } else {
    cmd->hints_flag = htonl(APICMD_GETADDRINFO_HINTS_FLAG_DISABLE);
  }
}

static int fill_result(struct apicmd_gaires_s *res, struct altcom_addrinfo **aires) {
  uint32_t i;
  int32_t cname_len;
  int32_t ai_size;
  struct altcom_addrinfo *ai = NULL;
  bool alloc_fail = false;
  struct altcom_sockaddr_storage *ss = NULL;
  char *cname = NULL;
  struct altcom_addrinfo *tmpai = NULL;

  if (ntohl(res->ai_num) > APICMD_GETADDRINFO_RES_AI_COUNT) {
    DBGIF_LOG1_ERROR("Invalid ai_num: %ld\n", ntohl(res->ai_num));
    res->ai_num = 0;
  }

  for (i = 0; i < ntohl(res->ai_num); i++) {
    cname_len = ntohl(res->ai[i].ai_cnamelen);
    if (cname_len > APICMD_GETADDRINFO_AI_CANONNAME_LENGTH) {
      DBGIF_LOG1_ERROR("Invalid cname_len: %ld\n", cname_len);
      cname_len = 0;
    }

    /* Allocate the whole area for each ai */

    ai_size = sizeof(struct altcom_addrinfo) + sizeof(struct altcom_sockaddr_storage) + cname_len;

    ai = (struct altcom_addrinfo *)BUFFPOOL_ALLOC(ai_size);
    if (!ai) {
      DBGIF_LOG_ERROR("BUFFPOOL_ALLOC()\n");
      alloc_fail = true;
    } else {
      ai->ai_flags = ntohl(res->ai[i].ai_flags);
      ai->ai_family = ntohl(res->ai[i].ai_family);
      ai->ai_socktype = ntohl(res->ai[i].ai_socktype);
      ai->ai_protocol = ntohl(res->ai[i].ai_protocol);
      ai->ai_addrlen = ntohl(res->ai[i].ai_addrlen);

      /* Fill the ai_addr area */

      ss = (struct altcom_sockaddr_storage *)((uint8_t *)ai + sizeof(struct altcom_addrinfo));
      memcpy(ss, &res->ai[i].ai_addr, sizeof(struct altcom_sockaddr_storage));
      ai->ai_addr = (struct altcom_sockaddr *)ss;

      /* Fill the ai_canonname area */

      ai->ai_canonname = NULL;
      if (cname_len != 0) {
        cname = (char *)((uint8_t *)ss + sizeof(struct altcom_sockaddr_storage));
        memcpy(cname, res->ai[i].ai_canonname, cname_len);
        ai->ai_canonname = cname;
      }
      ai->ai_next = NULL;

      if (*aires == NULL) {
        *aires = ai;
      } else {
        tmpai = *aires;
        while (tmpai->ai_next) {
          tmpai = tmpai->ai_next;
        }
        tmpai->ai_next = ai;
      }
    }
  }
  if (alloc_fail) {
    ai = *aires;
    while (ai) {
      tmpai = ai->ai_next;
      BUFFPOOL_FREE(ai);
      ai = tmpai;
    }
    *aires = NULL;
    return -1;
  }

  return 0;
}

/****************************************************************************
 * Name: gai_request
 *
 * Description:
 *   Send APICMDID_SOCK_GETADDRINFO_REQ.
 *
 ****************************************************************************/

static int32_t gai_request(struct gai_req_s *req) {
  int32_t ret;
  uint16_t reslen = 0;
  struct apicmd_gai_s *cmd = NULL;
  struct apicmd_gaires_s *res = NULL;

  /* Allocate send and response command buffer */

  if (!altcom_sock_alloc_cmdandresbuff((void **)&cmd, APICMDID_SOCK_GETADDRINFO_EXT,
                                       GAI_REQ_DATALEN, (void **)&res, GAI_RES_DATALEN)) {
    return ALTCOM_EAI_MEMORY;
  }

  /* Fill the data */

  fill_gai_request_command(0, cmd, req);

  ret =
      apicmdgw_send((uint8_t *)cmd, (uint8_t *)res, GAI_RES_DATALEN, &reslen, ALT_OSAL_TIMEO_FEVR);
  if (0 > ret) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    ret = -ret;
    goto errout_with_cmdfree;
  }

  if (reslen != GAI_RES_DATALEN) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %d\n", reslen);
    ret = ALTCOM_EFAULT;
    goto errout_with_cmdfree;
  }

  /* Check api command response */

  ret = ntohl(res->ret_code);

  if (APICMD_GETADDRINFO_RES_RET_CODE_OK != ret) {
    DBGIF_LOG1_ERROR("API command response is err :%ld.\n", ret);
    goto errout_with_cmdfree;
  }

  /* Fill result */
  ret = fill_result(res, req->res);
  if (ret < 0) {
    ret = ALTCOM_EAI_MEMORY;
  }

errout_with_cmdfree:
  altcom_sock_free_cmdandresbuff(cmd, res);
  return ret;
}

static int32_t gai_request_async(struct gai_req_s *req, altcom_getaddrinfo_ext_cb_t cb,
                                 void *priv) {
  int32_t ret;
  struct apicmd_gai_s *cmd = NULL;

  /* Allocate send command buffer only */

  if (!ALTCOM_SOCK_ALLOC_CMDBUFF(cmd, APICMDID_SOCK_GETADDRINFO_EXT, GAI_REQ_DATALEN)) {
    return ALTCOM_EAI_MEMORY;
  }

  /* Fill the data */
  int32_t callback_id = generate_callback_id();
  fill_gai_request_command(callback_id, cmd, req);

  /* Send command and block until receive a response */

  ret = apicmdgw_send((uint8_t *)cmd, NULL, 0, NULL, ALT_OSAL_TIMEO_FEVR);

  if (0 > ret) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    ret = -ret;
    goto errout_with_cmdfree;
  }

  altcom_free_cmd((uint8_t *)cmd);

  setup_callback(callback_id, cb, priv);

  return APICMD_GETADDRINFO_RES_RET_CODE_OK;

errout_with_cmdfree:
  altcom_free_cmd((uint8_t *)cmd);
  return ret;
}

/****************************************************************************
 * Name: exec_gai_callback
 *
 * Description:
 *   Execute callback that registered by altcom_getaddrinfo_ext_async().
 *
 *  Return:
 *   If the process succeeds, it returns 0.
 *   Otherwise negative value is returned.
 *
 ****************************************************************************/

static int exec_gai_callback(int32_t callback_id, int32_t status, struct altcom_addrinfo *res) {
  DBGIF_LOG1_INFO("search callback_id: %d\n", callback_id);
  struct gai_async_cb_s *list = search_callbacklist(callback_id);
  if (list) {
    /* execute callback */

    list->callback(list->priv, status, res);

    teardown_callback(callback_id);
  }

  return list ? 0 : -1;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: altcom_getaddrinfo_ext
 *
 * Description:
 *   Given node and service, which identify an Internet host and a service,
 *   altcom_getaddrinfo_ext() returns one or more addrinfo structures, each of
 *   which contains an Internet address that can be specified in a call to
 *   altcom_bind() or altcom_connect(). The altcom_getaddrinfo_ext() is reentrant
 *   and allows programs to eliminate IPv4-versus-IPv6 dependencies.
 *
 * Parameters:
 *   session_id - The numeric value of the session ID defined in the apn setting.
 *                See @ref ltesessionid for valid range
 *   nodename - Specifies either a numerical network address, or a network
 *              hostname, whose network addresses are looked up and resolved
 *   servname - Sets the port in each returned address structure
 *   hints - Points to an addrinfo structure that specifies criteria for
 *           selecting the socket address structures returned in the list
 *           pointed to by res
 *   res - Pointer to the start of the list
 *
 * Returned Value:
 *  altcom_getaddrinfo_ext() returns 0 if it succeeds, or one of the following
 *  nonzero error codes:
 *
 *     ALTCOM_EAI_NONAME
 *     ALTCOM_EAI_SERVICE
 *     ALTCOM_EAI_FAIL
 *     ALTCOM_EAI_MEMORY
 *     ALTCOM_EAI_FAMILY
 *
 ****************************************************************************/

int altcom_getaddrinfo_ext(uint8_t session_id, const char *nodename, const char *servname,
                           const struct altcom_addrinfo *hints, struct altcom_addrinfo **res) {
  int32_t result;
  struct gai_req_s req;

  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return ALTCOM_EAI_FAIL;
  }

  if (LTE_SESSION_ID_MAX < session_id) {
    DBGIF_LOG_ERROR("Invalid parameter.\n");
    return ALTCOM_EAI_FAIL;
  }

  if (res == NULL) {
    DBGIF_LOG_ERROR("Invalid parameter.\n");
    return ALTCOM_EAI_FAIL;
  }

  if (!nodename && !servname) {
    DBGIF_LOG_ERROR("Invalid parameter\n");
    return ALTCOM_EAI_NONAME;
  }

  *res = NULL;

  req.session_id = session_id;
  req.nodename = nodename;
  req.servname = servname;
  req.hints = hints;
  req.res = res;

  result = gai_request(&req);

  if (APICMD_GETADDRINFO_RES_RET_CODE_OK != result) {
    return result;
  }

  return 0;
}

/****************************************************************************
 * Name: altcom_getaddrinfo_ext_async
 *
 * Description:
 *   Given node and service, which identify an Internet host and a service,
 *   altcom_getaddrinfo_ext_async() returns one or more addrinfo structures, each of
 *   which contains an Internet address that can be specified in a call to
 *   altcom_bind() or altcom_connect(). The altcom_getaddrinfo_ext_async() is reentrant
 *   and allows programs to eliminate IPv4-versus-IPv6 dependencies.
 *
 * Parameters:
 *   session_id - The numeric value of the session ID defined in the apn setting.
 *                See @ref ltesessionid for valid range
 *   nodename - Specifies either a numerical network address, or a network
 *              hostname, whose network addresses are looked up and resolved
 *   servname - Sets the port in each returned address structure
 *   hints - Points to an addrinfo structure that specifies criteria for
 *           selecting the socket address structures returned in the list
 *           pointed to by res
 *   res - Pointer to the start of the list
 *
 * Returned Value:
 *  altcom_getaddrinfo_ext_async() returns 0 if it succeeds, or one of the following
 *  nonzero error codes:
 *
 *     ALTCOM_EAI_NONAME
 *     ALTCOM_EAI_SERVICE
 *     ALTCOM_EAI_FAIL
 *     ALTCOM_EAI_MEMORY
 *     ALTCOM_EAI_FAMILY
 *
 ****************************************************************************/

int altcom_getaddrinfo_ext_async(uint8_t session_id, const char *nodename, const char *servname,
                                 const struct altcom_addrinfo *hints,
                                 altcom_getaddrinfo_ext_cb_t callback, void *arg) {
  int32_t result;

  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return ALTCOM_EAI_FAIL;
  }

  if (LTE_SESSION_ID_MAX < session_id) {
    DBGIF_LOG_ERROR("Invalid parameter.\n");
    return ALTCOM_EAI_FAIL;
  }

  if (!nodename && !servname) {
    DBGIF_LOG_ERROR("Invalid parameter\n");
    return ALTCOM_EAI_NONAME;
  }

  if (!callback) {
    DBGIF_LOG_ERROR("Invalid parameter.\n");
    return ALTCOM_EAI_FAIL;
  }

  struct gai_req_s req;

  req.session_id = session_id;
  req.nodename = nodename;
  req.servname = servname;
  req.hints = hints;
  req.res = NULL;

  result = gai_request_async(&req, callback, arg);

  if (APICMD_GETADDRINFO_RES_RET_CODE_OK != result) {
    return result;
  }

  return 0;
}

void altcom_gai_async_job(void *arg) {
  struct apicmd_gaires_s *res = (struct apicmd_gaires_s *)arg;
  struct altcom_addrinfo *aires = NULL;
  int32_t callback_id = ntohl(res->callback_id);
  int32_t ret = ntohl(res->ret_code);

  if (APICMD_GETADDRINFO_RES_RET_CODE_OK != ret) {
    DBGIF_LOG1_ERROR("API command response is err :%ld.\n", ret);
    goto errout_with_cmdfree;
  }

  /* Fill result */
  ret = fill_result(res, &aires);
  if (ret < 0) {
    ret = ALTCOM_EAI_MEMORY;
  }

errout_with_cmdfree:

  /* In order to reduce the number of copies of the receive buffer,
   * bring a pointer to the receive buffer to the worker thread.
   * Therefore, the receive buffer needs to be released here. */

  altcom_free_cmd((uint8_t *)arg);

  ret = exec_gai_callback(callback_id, ret, aires);
  if (ret < 0) {
    DBGIF_LOG1_ERROR("exec_gai_callback() failed: %ld\n", ret);
  }
}
