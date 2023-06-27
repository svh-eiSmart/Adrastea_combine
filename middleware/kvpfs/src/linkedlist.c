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

#include <FreeRTOS.h>
#include <stdlib.h>
#include <string.h>
#include "linkedlist.h"

env_list *new_ele(const char *key, const char *value) {
  if (!key || !value) return NULL;

  env_list *cur_list = pvPortMalloc(sizeof(env_list));
  if (cur_list == NULL) return NULL;

  uint32_t key_len, value_len;
  key_len = strlen(key) + 1;
  value_len = strlen(value) + 1;
  cur_list->next = NULL;
  cur_list->key = pvPortMalloc(key_len);
  cur_list->value = pvPortMalloc(value_len);
  if (!cur_list->key || !cur_list->value) {
    vPortFree(cur_list->next);
    vPortFree(cur_list->value);
    vPortFree(cur_list);
    return NULL;
  }

  memset(cur_list->key, '\0', key_len);
  memset(cur_list->value, '\0', value_len);
  strncpy(cur_list->key, key, key_len - 1);
  strncpy(cur_list->value, value, value_len - 1);

  return cur_list;
}

int32_t modify_ele(env_list *list, const char *new_value) {
  if (!new_value || !list) return -1;
  uint32_t new_value_len = strlen(new_value) + 1;
  uint32_t old_value_len = strlen(list->value) + 1;
  uint32_t active_value_len = old_value_len;

  if (old_value_len < new_value_len) {
    vPortFree(list->value);
    if ((list->value = (char *)pvPortMalloc(new_value_len)) == NULL)
      return -1;  // fail to allocate mem
    active_value_len = new_value_len;
  }
  memset(list->value, '\0', active_value_len);
  strncpy(list->value, new_value, active_value_len);

  return 0;
}

env_list *search_ele(env_list *start, const char *key) {
  env_list *cur = start;
  while (cur != NULL) {
    if (strcmp(cur->key, key) == 0) return cur;
    cur = cur->next;
  }
  return NULL;
}
void free_ele(env_list *node) {
  vPortFree(node->key);
  node->key = NULL;
  vPortFree(node->value);
  node->value = NULL;
  vPortFree(node);
  node = NULL;
}

env_list *delete_ele(env_list *start, const char *key) {
  env_list *cur;
  env_list *next;
  env_list *head;

  if (!start) return NULL;
  cur = head = start;
  next = cur->next;

  if (strcmp(cur->key, key) == 0) {
    free_ele(cur);
    head = next;
    return head;
  }

  while (next != NULL) {
    if (strcmp(next->key, key) == 0) {
      cur->next = next->next;
      free_ele(next);
      break;
    } else {
      cur = next;
      next = next->next;
    }
  }

  return head;
}