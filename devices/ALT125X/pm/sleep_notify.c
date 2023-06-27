/*  ---------------------------------------------------------------------------

    (c) copyright 2019 Altair Semiconductor, Ltd. All rights reserved.

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
/**
 * @file   sleep_notify.c
 * @Author Altair Semiconductor
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "portmacro.h"
#include "sleep_notify.h"

/*Mutex semaphore for protecting sleep notify global memory */
static SemaphoreHandle_t sleepNotifySem = NULL;

#define SLEEP_NOTIFY_INITIALIZED 0xafafafafL
#define SLEEP_NOTIFY_RANGE 32 /*Total number of items that can be in list*/

#define SLEEP_DBGPRINT(fmt, args...)

/*Structure of item in sleep notify list.*/
typedef struct alt_Node {
  /*Function pointer*/
  int (*pCallback)(sleep_notify_state, void*);
  /*Pointer to next item*/
  struct alt_Node* next;
  /*Index of item*/
  uint32_t item_index;
  /*Pointer to additional context pass to callback function*/
  void* ptr_context;
} sleepNode;

/*Structure of sleep notify list management*/
typedef struct alt_sleep_data {
  /*Pointer of top of list*/
  sleepNode* list_head;
  /*Initialize flag*/
  uint32_t initialized;
  /*Bit map variable of index items in sleep notify list*/
  uint32_t sleep_notify_bitmap_reg;
} sleep_notify_data;

/*sleep notify struct*/
static sleep_notify_data dev_sleep;

/****************************************************************************
 * Internal Function Prototypes
 ****************************************************************************/
/*sleep notify internal functions*/
int sleep_notify_new_item(sleepNode** newsleepNode);
int sleep_notify_remove_item(sleepNode* postsleepNode, sleepNode* presleepNode);

/*
sleep_notify_init:
1.Initialize sleep notify structure (fields of structures).
2.Create mutex semaphore for application function.
Function arguments: void
Function returns:
SLEEP_NOTIFY_SUCCESS - creation of mutex semaphore completed successfully.
SLEEP_NOTIFY_ERROR - creation of mutex semaphore failed.
*/
int sleep_notify_init(void) {
  if (dev_sleep.initialized == SLEEP_NOTIFY_INITIALIZED) {
    SLEEP_DBGPRINT("sleep notify, sleep_notify_init() - already initialized");
    /*sleep notify already initialized*/
    return SLEEP_NOTIFY_ERROR;
  }

  memset((void*)&dev_sleep, (int)0, (size_t)sizeof(dev_sleep));

  /*sleep notify create mutex semaphore*/
  sleepNotifySem = xSemaphoreCreateMutex();

  if (sleepNotifySem == NULL) {
    dev_sleep.initialized = 0;

    SLEEP_DBGPRINT("sleep notify, sleep_notify_init() - failed to create mutex semaphore");
    /*sleep notify failed to create semaphore*/
    return SLEEP_NOTIFY_ERROR;
  }

  /*sleep notify create mutex semaphore*/
  dev_sleep.initialized = SLEEP_NOTIFY_INITIALIZED;

  SLEEP_DBGPRINT("sleep notify, sleep_notify_init() - create mutex semaphore");

  return SLEEP_NOTIFY_SUCCESS;
}

/*
sleep_notify_new_item:
Allocate dynamic memory item of sleep notify callback list
Function arguments:
newsleepNode - (Out) Address of dynamic allocated memory structure (pointer).
Function returns:
SLEEP_NOTIFY_ERROR - dynamic allocation failed.
SLEEP_NOTIFY_SUCCESS - dynamic allocation success.
*/
int sleep_notify_new_item(sleepNode** newsleepNode) {
  if (newsleepNode == NULL) {
    SLEEP_DBGPRINT("sleep notify, new_item() - invalid argument");
    /*sleep notify get invalid arguments*/
    return SLEEP_NOTIFY_ERROR;
  }

  *newsleepNode = (sleepNode*)malloc(sizeof(sleepNode));

  if (*newsleepNode == NULL) {
    SLEEP_DBGPRINT("sleep notify, new_item() - dynamic allocation failed");
    /*sleep notify dynamic allocation failed*/
    return SLEEP_NOTIFY_ERROR;
  }

  memset((void*)(*newsleepNode), (int)0, (size_t)sizeof(sleepNode));

  SLEEP_DBGPRINT("sleep notify, new_item() - dynamic allocation success");
  /*sleep notify dynamic allocation success*/
  return SLEEP_NOTIFY_SUCCESS;
}

/*
sleep_notify_insert_callback_item:
Allocate dynamic memory item and insert it to sleep notify list.
Function arguments:
P_Callback - (IN) Address of function to be called back (pointer to function).
item_inx   - (OUT) Index of new item (pointer).
ptr_ctx    - (IN) Address of additional context passed to call back function (pointer).
Function returns:
SLEEP_NOTIFY_SUCCESS - sleep notify create new item and insert it into list.
SLEEP_NOTIFY_ERROR - call function in ISR,get invalid arguments,initialize failed,
memory list is full, failed to allocate dynamic memory.
*/
int sleep_notify_insert_callback_item(int (*P_Callback)(sleep_notify_state, void*),
                                      int32_t* item_inx, void* ptr_ctx) {
  sleepNode* newsleepNode = NULL;
  uint32_t inx = 0, mask = 1;
  uint8_t bit_valeu = 0;

  if (xPortIsInsideInterrupt() == pdTRUE) {
    SLEEP_DBGPRINT("sleep notify, insert_callback_item() - call function in ISR");
    /*sleep notify call function in ISR*/
    return SLEEP_NOTIFY_ERROR;
  }

  if ((P_Callback == NULL) || (item_inx == NULL)) {
    SLEEP_DBGPRINT("sleep notify, insert_callback_item() - invalid arguments");
    /*sleep notify get invalid arguments*/
    return SLEEP_NOTIFY_ERROR;
  }

  if (dev_sleep.initialized != SLEEP_NOTIFY_INITIALIZED) {
    SLEEP_DBGPRINT("sleep notify, insert_callback_item() - initialize failed");
    /*sleep notify initialize failed*/
    return SLEEP_NOTIFY_ERROR;
  }

  /*Take mutex semaphore to protect sleep notify global memory*/
  if (xSemaphoreTake(sleepNotifySem, portMAX_DELAY) == pdTRUE) {
    for (inx = 0; inx < SLEEP_NOTIFY_RANGE; inx++) {
      /*Search bit map variable for new item*/
      bit_valeu = ((dev_sleep.sleep_notify_bitmap_reg >> inx) & mask);

      /*Find available bit for new item*/
      if (bit_valeu == 0) {
        /*Set available bit of new item to one*/
        dev_sleep.sleep_notify_bitmap_reg |= (mask << inx);

        /*Return new index item to user*/
        *item_inx = (int32_t)inx;

        break;
      }
    }
    /*Not find available bit for new item*/
    if (inx >= SLEEP_NOTIFY_RANGE) {
      /*Return index item (-1) to user*/
      *item_inx = -1;

      SLEEP_DBGPRINT(
          "sleep notify, insert_callback_item() - failed insert new item (list is full)");

      xSemaphoreGive(sleepNotifySem);

      /*sleep notify memory list is full*/
      return SLEEP_NOTIFY_ERROR;
    }
    /*Dynamic memory allocation for new item*/
    if (sleep_notify_new_item(&newsleepNode) != SLEEP_NOTIFY_SUCCESS) {
      /*Failed to allocate dynamic memory*/

      /*Reset available bit of new item to one*/
      dev_sleep.sleep_notify_bitmap_reg &= ~(mask << inx);

      xSemaphoreGive(sleepNotifySem);

      return SLEEP_NOTIFY_ERROR;
    }

    if (dev_sleep.list_head == NULL) {
      /*No items in list*/
      newsleepNode->next = NULL;
    } else {
      /*Connect new item to head pointer item (top of list item)*/
      newsleepNode->next = dev_sleep.list_head;
    }

    /*Set head pointer with address of new item*/
    dev_sleep.list_head = newsleepNode;

    /*Initialize call back pointer*/
    newsleepNode->pCallback = P_Callback;

    /*Initialize index of item*/
    newsleepNode->item_index = inx;

    /*Initialize pointer to context*/
    newsleepNode->ptr_context = ptr_ctx;

    SLEEP_DBGPRINT(
        "sleep notify, insert_callback_item() - insert item: index %d Address %x Callback %x",
        (int)newsleepNode->item_index, (int)newsleepNode, (int)newsleepNode->pCallback);

    xSemaphoreGive(sleepNotifySem);

    return SLEEP_NOTIFY_SUCCESS;
  }

  SLEEP_DBGPRINT("sleep notify, insert_callback_item() - failed take semaphore");
  /*sleep notify failed take semaphore*/
  return SLEEP_NOTIFY_ERROR;
}

/*
sleep_notify_remove_callback_item:
Remove item from sleep notify list.
Function arguments:
item_inx - (IN) Index of item to need to be removed from list.
Function returns:
SLEEP_NOTIFY_SUCCESS - remove item from pm notify list.
SLEEP_NOTIFY_ERROR - call function in ISR, get invalid arguments,
initialize failed,list is empty,failed take semaphore.
*/
int sleep_notify_remove_callback_item(int32_t item_inx) {
  sleepNode *postsleepNode = NULL, *presleepNode = NULL;

  uint32_t lclItem_inx = 0, mask = 1;

  uint8_t bit_value = 0;

  if (xPortIsInsideInterrupt() == pdTRUE) {
    SLEEP_DBGPRINT("sleep notify, remove_callback_item() - call function in ISR");
    /*sleep notify call function in ISR*/
    return SLEEP_NOTIFY_ERROR;
  }

  if ((item_inx < 0) || (item_inx >= SLEEP_NOTIFY_RANGE)) {
    SLEEP_DBGPRINT(
        "sleep notify, remove_callback_item() - invalid argument (index is out of range)");
    /*sleep notify get invalid argument - index is out of range*/
    return SLEEP_NOTIFY_ERROR;
  }

  if (dev_sleep.initialized != SLEEP_NOTIFY_INITIALIZED) {
    SLEEP_DBGPRINT("sleep notify, remove_callback_item() - initialize failed");
    /*sleep notify initialize failed*/
    return SLEEP_NOTIFY_ERROR;
  }

  /*Take mutex semaphore to protect sleep notify global memory*/
  if (xSemaphoreTake(sleepNotifySem, portMAX_DELAY) == pdTRUE) {
    if (dev_sleep.list_head == NULL) {
      /*List is empty*/
      SLEEP_DBGPRINT(
          "sleep notify, remove_callback_item() - failed to remove item number %lu (list is empty)",
          item_inx);

      xSemaphoreGive(sleepNotifySem);

      return SLEEP_NOTIFY_ERROR;
    }

    lclItem_inx = (uint32_t)item_inx;

    /*Check if the bit of the item is set to one*/
    bit_value = ((dev_sleep.sleep_notify_bitmap_reg >> lclItem_inx) & mask);

    if (bit_value == 0) {
      /*Bit of the item is equal to zero*/
      SLEEP_DBGPRINT("sleep notify, remove_callback_item() - index item %lu not exist",
                     lclItem_inx);

      xSemaphoreGive(sleepNotifySem);

      return SLEEP_NOTIFY_ERROR;
    }

    /*Set pointer head of list*/
    postsleepNode = dev_sleep.list_head;
    /*Set pointer head of list*/
    presleepNode = dev_sleep.list_head;

    /*Search the list to find the item with the wanted index*/
    while (postsleepNode) {
      if (postsleepNode->item_index == lclItem_inx) {
        if (sleep_notify_remove_item(postsleepNode, presleepNode) != SLEEP_NOTIFY_SUCCESS) {
          xSemaphoreGive(sleepNotifySem);
          /*Failed to remove item from list*/
          return SLEEP_NOTIFY_ERROR;
        }

        /*Reset available bit of new item to one*/
        dev_sleep.sleep_notify_bitmap_reg &= ~(mask << lclItem_inx);

        xSemaphoreGive(sleepNotifySem);

        return SLEEP_NOTIFY_SUCCESS;
      }
      /*Move pointer to next item in list*/
      presleepNode = postsleepNode;
      /*Move pointer to next item in list*/
      postsleepNode = postsleepNode->next;
    }

    /*Reset available bit of new item to one*/
    dev_sleep.sleep_notify_bitmap_reg &= ~(mask << lclItem_inx);

    SLEEP_DBGPRINT("sleep notify, remove_callback_item() - item index %lu not exist", lclItem_inx);
    /*sleep notify list empty - Item not exist*/
    xSemaphoreGive(sleepNotifySem);

    return SLEEP_NOTIFY_ERROR;
  }

  SLEEP_DBGPRINT("sleep notify, remove_callback_item() - failed take semaphore");
  /*sleep notify failed take semaphore*/
  return SLEEP_NOTIFY_ERROR;
}

/*
sleep_notify_remove_item:
Remove item from sleep notify list.
Function arguments:
postsleepNode - (IN) faster pointer of list (pointer).
presleepNode - (IN) slower pointer of list (pointer).
Function returns:
SLEEP_NOTIFY_SUCCESS - remove item from pm notify list.
SLEEP_NOTIFY_ERROR - get invalid arguments.
*/
int sleep_notify_remove_item(sleepNode* postsleepNode, sleepNode* presleepNode) {
  if ((postsleepNode == NULL) || (presleepNode == NULL)) {
    SLEEP_DBGPRINT("sleep notify, remove_item() - invalid arguments");
    /*sleep notify get invalid arguments*/
    return SLEEP_NOTIFY_ERROR;
  }

  if (postsleepNode == dev_sleep.list_head) {
    if (postsleepNode->next == NULL) {
      dev_sleep.list_head = NULL;
    } else {
      dev_sleep.list_head = postsleepNode->next;
    }
  } else {
    if (postsleepNode->next == NULL) {
      presleepNode->next = NULL;
    } else {
      presleepNode->next = postsleepNode->next;
    }
  }

  SLEEP_DBGPRINT("sleep notify, remove_item() - remove item: index %d Address %x Callback %x",
                 (int)postsleepNode->item_index, (int)postsleepNode, (int)postsleepNode->pCallback);

  free((void*)postsleepNode);

  return SLEEP_NOTIFY_SUCCESS;
}

/*
sleep_notify:
Execute all the items callback.
Function arguments:
sleep_state - (IN) sleep notify state (suspending or resuming) (enum).
Function returns:
SLEEP_NOTIFY_SUCCESS - execute all the items callback.
SLEEP_NOTIFY_ERROR - get invalid arguments.
*/
int sleep_notify(sleep_notify_state sleep_state) {
  sleepNode* newsleepNode = NULL;

  if ((sleep_state != SUSPENDING) && (sleep_state != RESUMING)) {
    SLEEP_DBGPRINT("sleep notify, sleep_notify() - get invalid state %d", sleep_state);
    /*sleep notify get invalid arguments*/
    return SLEEP_NOTIFY_ERROR;
  }

  if (dev_sleep.list_head == NULL) {
    SLEEP_DBGPRINT("sleep notify, sleep_notify() - no callbacks (list is empty)");
    /*sleep notify list empty*/
    return SLEEP_NOTIFY_SUCCESS;
  }

  newsleepNode = dev_sleep.list_head;

  /*Pass the list and execute all callback*/
  while (newsleepNode) {
    if (newsleepNode->pCallback) {
      newsleepNode->pCallback(sleep_state, newsleepNode->ptr_context);
      SLEEP_DBGPRINT("sleep notify, sleep_notify() - call item index %d Address %x Callback %x",
                     (int)newsleepNode->item_index, (int)newsleepNode,
                     (int)newsleepNode->pCallback);
    }

    newsleepNode = newsleepNode->next;
  }

  return SLEEP_NOTIFY_SUCCESS;
}
