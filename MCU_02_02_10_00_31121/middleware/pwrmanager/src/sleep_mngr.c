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

/* Kernel includes. */
#include "sleep_mngr.h"
#include "alt_osal.h"

/****************************************************************************
 * Definitions
 ****************************************************************************/
#define SMNGR_INITIALIZED 0xafafafafL

/* debug level: 0:critical; 1: warning; 2:information; 3: debug */
#define SMNGR_LOG_LEVEL 0
#if defined(SMNGR_ENABLE_DEBUG_PRINTS)
#define SMNGR_LOG(lEVEL, fmt, args...)                 \
  {                                                    \
    if (lEVEL <= SMNGR_LOG_LEVEL) printf(fmt, ##args); \
  }
#else
#define SMNGR_LOG(lEVEL, fmt, args...) \
  {}
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/
struct smngr_element {
  void *hw;
  int32_t (*callback)(void *data);
  uint32_t smngr_id;
};

struct Node {
  struct smngr_element *data;
  struct Node *next;
  struct Node *prev;
};

struct smngr_data {
  uint32_t smngr_dev_busy_mask;
  uint32_t smngr_dev_bitmap_registration;
  struct smngr_element element;
  struct Node *list_head;  // global variable - pointer to head node.
  uint32_t initialized;
  TimerHandle_t xTimer;
  int32_t timer_activation;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/
static struct smngr_data dev_smngr;
static char *smngr_devname[SMNGR_RANGE];

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/
static struct Node *GetNewNode(struct smngr_element *x);
static void InsertAtTail(struct smngr_element *x);
static void deleteNode(struct Node *del);
static void delElementByID(uint32_t id);

/****************************************************************************
 * Private Functions
 ****************************************************************************/
// Creates a new Node and returns pointer to it.
static struct Node *GetNewNode(struct smngr_element *x) {
  struct Node *newNode = (struct Node *)malloc(sizeof(struct Node));
  newNode->data = x;
  newNode->prev = NULL;
  newNode->next = NULL;
  return newNode;
}

// Inserts a Node at tail of Doubly linked list
static void InsertAtTail(struct smngr_element *x) {
  struct Node *temp = dev_smngr.list_head;
  struct Node *newNode = GetNewNode(x);
  if (dev_smngr.list_head == NULL) {
    dev_smngr.list_head = newNode;
    return;
  }
  while (temp->next != NULL) temp = temp->next;  // Go To last Node
  temp->next = newNode;
  newNode->prev = temp;
}

/* Function to delete a node in a Doubly Linked List.
   head_ref --> pointer to head node pointer.
   del  -->  pointer to node to be deleted. */
static void deleteNode(struct Node *del) {
  /* base case */
  if (dev_smngr.list_head == NULL || del == NULL) {
    SMNGR_LOG(0, "Sleep manager - deleteNode NULL pointers\n");
    return;
  }

  /* If node to be deleted is head node */
  if (dev_smngr.list_head == del) dev_smngr.list_head = del->next;

  /* Change next only if node to be deleted is NOT the last node */
  if (del->next != NULL) del->next->prev = del->prev;

  /* Change prev only if node to be deleted is NOT the first node */
  if (del->prev != NULL) del->prev->next = del->next;

  /* free element */
  free(del->data);
  /* Finally, free the memory occupied by del*/
  free(del);
  return;
}

static void delElementByID(uint32_t id) {
  struct Node *temp = dev_smngr.list_head;

  while (temp != NULL) {
    //  printf("%d ",temp->data);
    if (temp->data->smngr_id == id) {
      deleteNode(temp);
      return;
    }
    temp = temp->next;
  }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
/*
 * Clean main structure
 * init linked list
 */
int32_t smngr_init(void) {
  if (dev_smngr.initialized == SMNGR_INITIALIZED) {
    return (-1);
  }

  SMNGR_LOG(2, "Sleep manager initialized");

  memset(&dev_smngr, 0, sizeof(dev_smngr));
  memset(smngr_devname, 0, sizeof(smngr_devname));

  dev_smngr.initialized = SMNGR_INITIALIZED;

  return (0);
}

/*
 * Sleep manager registration
 * Valid lut bitmaps are [1..31]. Value of 0 is considered as error
 * Parameters:
 * Return 0 upon success otherwise error code
 */
int32_t smngr_register_dev_async(char *dev_name, int32_t (*callback)(void *), void *hw,
                                 uint32_t *id) {
  uint32_t uxSavedInterruptStatus = 0;
  int32_t bit = 0, i = 0;
  int32_t ret = 0;

  if (dev_smngr.initialized != SMNGR_INITIALIZED) {
    SMNGR_LOG(1, "Sleep manager not initialized");
    return -1;
  }

  uxSavedInterruptStatus = alt_osal_enter_critical();

  *id = 0;
  for (i = 1; i < SMNGR_RANGE; i++) {
    bit = (dev_smngr.smngr_dev_bitmap_registration >> i) & 1L;
    if (bit == 0) {
      struct smngr_element *new_element =
          (struct smngr_element *)malloc(sizeof(struct smngr_element));

      if (!new_element)  // Error: no memory
      {
        SMNGR_LOG(1, "Sleep manager dev registration failure - no memory");
        ret = -1;
        *id = 0;
        alt_osal_exit_critical(uxSavedInterruptStatus);
        return ret;
      }

      dev_smngr.smngr_dev_bitmap_registration |= (1L << i);
      smngr_devname[i] = strdup(dev_name);
      new_element->hw = hw;
      new_element->callback = (int32_t(*)(void *))callback;
      new_element->smngr_id = i;
      InsertAtTail(new_element);
      *id = i;

      SMNGR_LOG(2, "Sleep manager registration Dev id %ld", i);

      break;
    }
  }

  if (i >= SMNGR_RANGE) {
    ret = -1;  // Error: no memory
    *id = 0;
  }

  if (ret < 0) {
    SMNGR_LOG(1, "Sleep manager dev registration failure");
  }

  alt_osal_exit_critical(uxSavedInterruptStatus);
  return ret;
}

/* Register a dev get new id */
int32_t smngr_register_dev_sync(char *dev_name, uint32_t *id) {
  uint32_t uxSavedInterruptStatus = 0;
  int32_t bit = 0, i = 0, ret = 0;

  if (dev_smngr.initialized != SMNGR_INITIALIZED) {
    SMNGR_LOG(1, "Sleep manager not initialized");
    return -1;
  }

  uxSavedInterruptStatus = alt_osal_enter_critical();

  for (i = 1; i < SMNGR_RANGE; i++) {
    bit = (dev_smngr.smngr_dev_bitmap_registration >> i) & 1L;
    if (bit == 0) {
      dev_smngr.smngr_dev_bitmap_registration |= (1L << i);
      smngr_devname[i] = strdup(dev_name);
      *id = i;
      dev_smngr.element.smngr_id = i;
      SMNGR_LOG(3, "Sleep manager registration Dev id: %ld", i);
      break;
    }
  }

  if (i >= SMNGR_RANGE) {
    ret = -1;
    *id = 0;
  }

  alt_osal_exit_critical(uxSavedInterruptStatus);

  if (ret < 0) {
    SMNGR_LOG(1, "Sleep manager dev registration failure Dev");
  }

  return ret;
}

/* Unregister a dev by its id */
int32_t smngr_unregister_dev_sync(uint32_t id) {
  uint32_t uxSavedInterruptStatus = 0;

  if (dev_smngr.initialized != SMNGR_INITIALIZED) {
    SMNGR_LOG(1, "Sleep manager not initialized");
    return -1;
  }

  if (id < 1 || id >= SMNGR_RANGE) {
    SMNGR_LOG(1, "Sleep manager - smngr_unregister_dev_sync received illegal id = %ld", id);
    return -1;
  }

  uxSavedInterruptStatus = alt_osal_enter_critical();

  dev_smngr.smngr_dev_bitmap_registration &= ~(1L << id);
  dev_smngr.smngr_dev_busy_mask &= ~(1L << id);
  dev_smngr.element.smngr_id = 0;
  if (smngr_devname[id]) free(smngr_devname[id]);

  alt_osal_exit_critical(uxSavedInterruptStatus);

  return 0;
}

/* Unregister a dev by its id */
int32_t smngr_unregister_dev_async(uint32_t id) {
  uint32_t uxSavedInterruptStatus = 0;

  if (dev_smngr.initialized != SMNGR_INITIALIZED) {
    SMNGR_LOG(1, "Sleep manager not initialized");
    return -1;
  }

  if (id < 1 || id >= SMNGR_RANGE) {
    SMNGR_LOG(1, "Sleep manager - received illegal id = %ld", id);
    return -1;
  }

  uxSavedInterruptStatus = alt_osal_enter_critical();
  dev_smngr.smngr_dev_bitmap_registration &= ~(1L << id);
  delElementByID(id);
  alt_osal_exit_critical(uxSavedInterruptStatus);

  return 0;
}

/* A device should call this function when it has work to do and this will avoid
 * the system to go to sleep or to hibernate
 */
int32_t smngr_dev_busy_set(uint32_t id) {
  uint32_t uxSavedInterruptStatus = 0;

  if (id < 1 || id >= SMNGR_RANGE) {
    SMNGR_LOG(1, "Sleep manager - received illegal id = %ld", id);
    return -1;
  }

  // because this can be called indirectly from multiple tasks or ISRs the action needs to be atomic
  uxSavedInterruptStatus = alt_osal_enter_critical();

  dev_smngr.smngr_dev_busy_mask |= 1L << id;

  alt_osal_exit_critical(uxSavedInterruptStatus);

  return 0;
}

/* A device should call this function when it has nothing to do and from the device
 * perspective the system can to go to sleep or to hibernate
 */
int32_t smngr_dev_busy_clr(uint32_t id) {
  uint32_t uxSavedInterruptStatus = 0;

  if (id < 1 || id >= SMNGR_RANGE) {
    SMNGR_LOG(1, "Sleep manager - received illegal id = %ld", id);
    return -1;
  }

  uxSavedInterruptStatus = alt_osal_enter_critical();

  dev_smngr.smngr_dev_busy_mask &= ~(1L << id);

  alt_osal_exit_critical(uxSavedInterruptStatus);

  return 0;
}

/* Get the device ID registration name*/
char *smngr_get_devname(uint32_t id) {
  // dont need to return name for unregistered driver
  if ((dev_smngr.smngr_dev_bitmap_registration & (1L << id)) == 0) return NULL;

  return smngr_devname[id];
}

/* To be called from IDLE task to check the SMNGR status*/
bool smngr_is_dev_busy(uint32_t *sync_mask, uint32_t *async_mask) {
  int32_t busy = 0;

  *sync_mask = dev_smngr.smngr_dev_busy_mask;
  /* Sync */
  if (dev_smngr.smngr_dev_busy_mask) {
    return 1;
  }

  /* Async */
  struct Node *temp = dev_smngr.list_head;
  *async_mask = 0;
  while (temp != NULL) {
    if (temp->data->callback) busy = temp->data->callback(temp->data->hw);

    if (busy) {
      *async_mask = temp->data->smngr_id;
      return 1;
    }

    temp = temp->next;
  }

  return busy;
}
