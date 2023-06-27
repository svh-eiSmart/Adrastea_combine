/****************************************************************************
 *  (c) copyright 2019 Altair Semiconductor, Ltd. All rights reserved.
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

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>
#include "kvpfs_config.h"
#include "linkedlist.h"
#include "list_data.h"

#define LOCK()  if (xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED) \
                  assert(xSemaphoreTakeFromISR(list_data_mtx, NULL) == pdTRUE); \
                else \
                  assert(xSemaphoreTake(list_data_mtx, portMAX_DELAY) == pdTRUE);

#define UNLOCK()  if( xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED ) \
                    assert(xSemaphoreGiveFromISR(list_data_mtx, NULL) == pdTRUE); \
                  else \
                    assert(xSemaphoreGive(list_data_mtx) == pdTRUE);

static struct list_data_info ldt_info = {0};
static SemaphoreHandle_t list_data_mtx = NULL;
env_list *head = NULL;
env_list *cur = NULL;

void get_all_data_list(char *resp, int del) {
  env_list *traverse = head;
  uint32_t total_len = 0;
  uint32_t str_idx = 0;
  char delim;

  if( del == 1)
    delim = '\n';
  else
    delim = '\xff';

  if (!resp || !head) return;

  total_len = ldt_info.used_byte + 1;

  LOCK();
  traverse = head;
  while (traverse != NULL) {
    snprintf(resp + str_idx, total_len - str_idx, "%s=%s%c", traverse->key, traverse->value, delim);
    str_idx += strlen(traverse->key) + strlen(traverse->value) + 2;
    traverse = traverse->next;
  }
  UNLOCK();
}

int32_t add_data_list(const char *key, const char *value) {
  env_list *node = NULL;
  uint32_t value_len;
  LOCK();
  if (head == NULL)  // no element yet
  {
    head = cur = new_ele(key, value);
    if (!head) goto fail;
    ldt_info.used_byte += strlen(key) + strlen(value) + 2;  // includes '=' and \0
    ldt_info.num_of_ele++;
  } else {
    node = search_ele(head, key);

    if (node == NULL) {
      // add new node
      cur->next = new_ele(key, value);
      if (!cur->next) goto fail;
      cur = cur->next;
      ldt_info.used_byte += strlen(key) + strlen(value) + 2;  // includes '=' and \0
      ldt_info.num_of_ele++;
    } else {
      // modify node
      value_len = strlen(node->value);
      if (modify_ele(node, value)) goto fail;
      ldt_info.used_byte -= value_len;
      ldt_info.used_byte += strlen(value);
    }
  }
  UNLOCK();
  return 0;
fail:
  UNLOCK();
  return -1;
}

int32_t find_value_list(const char *key, const char **value) {
  env_list *node;
  int32_t ret = -1;
  *value = NULL;

  if (!head) return -1;

  LOCK();
  node = search_ele(head, key);
  if (node) {
    *value = node->value;
    ret = 0;
  }
  UNLOCK();
  return ret;
}

int32_t delete_value_list(const char *key) {
  const char *value;

  if (find_value_list(key, &value) == 0) {
    LOCK();
    ldt_info.used_byte -= (strlen(key) + strlen(value) + 2);
    ldt_info.num_of_ele--;
    head = delete_ele(head, key);
    UNLOCK();
    return 0;
  }
  return -1;
}

void init_list_data(struct list_data_info **ldt) {

  if( list_data_mtx ) return ;
  list_data_mtx = xSemaphoreCreateMutex();
  if (list_data_mtx == NULL) assert(0);
  memset(&ldt_info,'\0', sizeof(struct list_data_info ));
  *ldt = &ldt_info;
  return ;
}