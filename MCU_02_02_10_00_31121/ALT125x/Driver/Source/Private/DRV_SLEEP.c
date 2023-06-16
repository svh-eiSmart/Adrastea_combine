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
#include <stdbool.h>
#include <string.h>

/* Kernel includes. */
#include "DRV_SLEEP.h"

/****************************************************************************
 * Definitions
 ****************************************************************************/
#define DRV_SLEEP_INITIALIZED 0xafafafafL
#define DRV_SLEEP_MAX_NOTIFY_NUM (32) /*Total number of items that can be in list*/

//#define DRV_SLEEP_ENABLE_DPRINT
/* debug level: 0:critical; 1: warning; 2:information; 3: debug */
#define DRV_SLEEP_LOG_LEVEL (0)
#if defined(DRV_SLEEP_ENABLE_DPRINT)
#define DRV_SLEEP_LOG(lEVEL, fmt, args...)                 \
  {                                                        \
    if (lEVEL <= DRV_SLEEP_LOG_LEVEL) printf(fmt, ##args); \
  }
#else
#define DRV_SLEEP_LOG(lEVEL, fmt, args...) \
  {}
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/
typedef struct Drv_Sleep_Node {
  int32_t (*pCallback)(DRV_SLEEP_NotifyType, DRV_PM_PwrMode, void *);
  struct Drv_Sleep_Node *next;
  uint32_t item_index;
  void *ptr_context;
} DRV_SLEEP_Node;

typedef struct Drv_Sleep_NotifyList {
  DRV_SLEEP_Node *list_head;
  uint32_t initialized;
  uint32_t node_bitmap;
} DRV_SLEEP_NotifyList;

/****************************************************************************
 * Private Data
 ****************************************************************************/
static DRV_SLEEP_NotifyList dev_sleep;

/****************************************************************************
 * Internal Function Prototypes
 ****************************************************************************/
int32_t drv_sleep_notify_add_node(DRV_SLEEP_Node **sleep_node);
int32_t drv_sleep_notify_remove_node(DRV_SLEEP_Node *next_node, DRV_SLEEP_Node *pre_node);

/****************************************************************************
 * Private Functions
 ****************************************************************************/
int32_t drv_sleep_notify_add_node(DRV_SLEEP_Node **sleep_node) {
  if (sleep_node == NULL) {
    DRV_SLEEP_LOG(1, "drv_sleep_notify_add_node() - invalid argument");
    return DRV_SLEEP_NTFY_ERROR;
  }

  *sleep_node = (DRV_SLEEP_Node *)malloc(sizeof(DRV_SLEEP_Node));

  if (*sleep_node == NULL) {
    DRV_SLEEP_LOG(1, "drv_sleep_notify_add_node() - dynamic allocation failed");
    return DRV_SLEEP_NTFY_ERROR;
  }

  memset((void *)(*sleep_node), (int)0, (size_t)sizeof(DRV_SLEEP_Node));

  DRV_SLEEP_LOG(2, "drv_sleep_notify_add_node() - dynamic allocation success");

  return DRV_SLEEP_NTFY_SUCCESS;
}

int32_t drv_sleep_notify_remove_node(DRV_SLEEP_Node *next_node, DRV_SLEEP_Node *pre_node) {
  if ((next_node == NULL) || (pre_node == NULL)) {
    DRV_SLEEP_LOG(1, "drv_sleep_notify_remove_node() - invalid arguments");
    return DRV_SLEEP_NTFY_ERROR;
  }

  if (next_node == dev_sleep.list_head) {
    if (next_node->next == NULL) {
      dev_sleep.list_head = NULL;
    } else {
      dev_sleep.list_head = next_node->next;
    }
  } else {
    if (next_node->next == NULL) {
      pre_node->next = NULL;
    } else {
      pre_node->next = next_node->next;
    }
  }

  DRV_SLEEP_LOG(2, "drv_sleep_notify_remove_node(): index %d Address %x Callback %x",
                (int)next_node->item_index, (int)next_node, (int)next_node->pCallback);

  free((void *)next_node);

  return DRV_SLEEP_NTFY_SUCCESS;
}

/****************************************************************************
 * Public Functions (only for driver layer)
 ****************************************************************************/
int32_t DRV_SLEEP_Initialize(void) {
  if (dev_sleep.initialized == DRV_SLEEP_INITIALIZED) {
    DRV_SLEEP_LOG(1, "DRV_SLEEP_Initialize() - already initialized");
    return DRV_SLEEP_NTFY_ERROR;
  }

  memset((void *)&dev_sleep, (int)0, (size_t)sizeof(dev_sleep));

  dev_sleep.initialized = DRV_SLEEP_INITIALIZED;

  DRV_SLEEP_LOG(2, "DRV_SLEEP_Initialize() successfully");

  return DRV_SLEEP_NTFY_SUCCESS;
}

int32_t DRV_SLEEP_RegNotification(int32_t (*ptr_callback)(DRV_SLEEP_NotifyType, DRV_PM_PwrMode,
                                                          void *),
                                  int32_t *item_inx, void *ptr_context) {
  DRV_SLEEP_Node *sleep_node = NULL;
  uint32_t inx = 0, mask = 1;
  uint8_t bit_valeu = 0;

  if ((ptr_callback == NULL) || (item_inx == NULL)) {
    DRV_SLEEP_LOG(1, "DRV_SLEEP_RegNotification() - invalid arguments");
    /*sleep notify get invalid arguments*/
    return DRV_SLEEP_NTFY_ERROR;
  }

  if (dev_sleep.initialized != DRV_SLEEP_INITIALIZED) {
    DRV_SLEEP_LOG(1, "DRV_SLEEP_RegNotification() - initialize failed");
    /*sleep notify initialize failed*/
    return DRV_SLEEP_NTFY_ERROR;
  }

  for (inx = 0; inx < DRV_SLEEP_MAX_NOTIFY_NUM; inx++) {
    /*Search bit map variable for new item*/
    bit_valeu = ((dev_sleep.node_bitmap >> inx) & mask);

    /*Find available bit for new item*/
    if (bit_valeu == 0) {
      /*Set available bit of new item to one*/
      dev_sleep.node_bitmap |= (mask << inx);

      /*Return new index item to user*/
      *item_inx = (int32_t)inx;

      break;
    }
  }
  /*Not find available bit for new item*/
  if (inx >= DRV_SLEEP_MAX_NOTIFY_NUM) {
    /*Return index item (-1) to user*/
    *item_inx = -1;

    DRV_SLEEP_LOG(1, "DRV_SLEEP_RegNotification() - failed insert new item (list is full)");

    /*sleep notify memory list is full*/
    return DRV_SLEEP_NTFY_ERROR;
  }
  /*Dynamic memory allocation for new item*/
  if (drv_sleep_notify_add_node(&sleep_node) != DRV_SLEEP_NTFY_SUCCESS) {
    /*Failed to allocate dynamic memory*/

    /*Reset available bit of new item to one*/
    dev_sleep.node_bitmap &= ~(mask << inx);

    return DRV_SLEEP_NTFY_ERROR;
  }

  if (dev_sleep.list_head == NULL) {
    /*No items in list*/
    sleep_node->next = NULL;
  } else {
    /*Connect new item to head pointer item (top of list item)*/
    sleep_node->next = dev_sleep.list_head;
  }

  /*Set head pointer with address of new item*/
  dev_sleep.list_head = sleep_node;

  /*Initialize call back pointer*/
  sleep_node->pCallback = ptr_callback;

  /*Initialize index of item*/
  sleep_node->item_index = inx;

  /*Initialize pointer to context*/
  sleep_node->ptr_context = ptr_context;

  DRV_SLEEP_LOG(2, "DRV_SLEEP_RegNotification() - insert item: index %d Address %x Callback %x",
                (int)sleep_node->item_index, (int)sleep_node, (int)sleep_node->pCallback);

  return DRV_SLEEP_NTFY_SUCCESS;
}

int32_t DRV_SLEEP_UnRegNotification(int32_t item_inx) {
  DRV_SLEEP_Node *next_node = NULL, *pre_node = NULL;
  uint32_t index = 0, mask = 1;
  uint8_t bit_value = 0;

  if ((item_inx < 0) || (item_inx >= DRV_SLEEP_MAX_NOTIFY_NUM)) {
    DRV_SLEEP_LOG(1, "DRV_SLEEP_UnRegNotification() -invalid argument (index is out of range)");
    return DRV_SLEEP_NTFY_ERROR;
  }

  if (dev_sleep.initialized != DRV_SLEEP_INITIALIZED) {
    DRV_SLEEP_LOG(1, "DRV_SLEEP_UnRegNotification() -initialize failed");
    return DRV_SLEEP_NTFY_ERROR;
  }

  if (dev_sleep.list_head == NULL) { /*List is empty*/
    DRV_SLEEP_LOG(1,
                  "DRV_SLEEP_UnRegNotification() -failed to remove item number %lu (list is empty)",
                  item_inx);

    return DRV_SLEEP_NTFY_ERROR;
  }

  index = (uint32_t)item_inx;

  /*Check if the bit of the item is set to one*/
  bit_value = ((dev_sleep.node_bitmap >> index) & mask);

  if (bit_value == 0) {
    /*Bit of the item is equal to zero*/
    DRV_SLEEP_LOG(1, "DRV_SLEEP_UnRegNotification() -index item %lu not exist", index);
    return DRV_SLEEP_NTFY_ERROR;
  }

  /*Set pointer head of list*/
  next_node = dev_sleep.list_head;
  /*Set pointer head of list*/
  pre_node = dev_sleep.list_head;

  /*Search the list to find the item with the wanted index*/
  while (next_node) {
    if (next_node->item_index == index) {
      if (drv_sleep_notify_remove_node(next_node, pre_node) != DRV_SLEEP_NTFY_SUCCESS) {
        /*Failed to remove item from list*/
        return DRV_SLEEP_NTFY_ERROR;
      }

      /*Reset available bit of new item to one*/
      dev_sleep.node_bitmap &= ~(mask << index);

      return DRV_SLEEP_NTFY_SUCCESS;
    }
    pre_node = next_node;
    next_node = next_node->next;
  }

  /*Reset available bit of new item to one*/
  dev_sleep.node_bitmap &= ~(mask << index);

  DRV_SLEEP_LOG(1, "DRV_SLEEP_UnRegNotification() -item index %lu not exist", index);

  return DRV_SLEEP_NTFY_ERROR;
}

int32_t DRV_SLEEP_NOTIFY(DRV_SLEEP_NotifyType sleep_state, DRV_PM_PwrMode pwr_mode) {
  DRV_SLEEP_Node *sleep_node = NULL;

  if ((sleep_state != DRV_SLEEP_NTFY_SUSPENDING) && (sleep_state != DRV_SLEEP_NTFY_RESUMING)) {
    DRV_SLEEP_LOG(1, "DRV_SLEEP_NOTIFY() -get invalid state %d", sleep_state);
    return DRV_SLEEP_NTFY_ERROR;
  }

  if (dev_sleep.list_head == NULL) { /*sleep notify list empty*/
    DRV_SLEEP_LOG(2, "DRV_SLEEP_NOTIFY() -no callbacks (list is empty)");
    return DRV_SLEEP_NTFY_SUCCESS;
  }

  sleep_node = dev_sleep.list_head;

  while (sleep_node) {
    if (sleep_node->pCallback) {
      sleep_node->pCallback(sleep_state, pwr_mode, sleep_node->ptr_context);
      DRV_SLEEP_LOG(1, "DRV_SLEEP_NOTIFY() -call item index %d Address %x Callback %x",
                    (int)sleep_node->item_index, (int)sleep_node, (int)sleep_node->pCallback);
    }

    sleep_node = sleep_node->next;
  }

  return DRV_SLEEP_NTFY_SUCCESS;
}
