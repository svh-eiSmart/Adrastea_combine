#ifndef __LINKEDLIST_H__
#define __LINKEDLIST_H__

typedef struct llist {
  char *key;
  char *value;
  struct llist *next;

} env_list;

env_list *new_ele(const char *key, const char *value);

/**
 * @brief search the same key on all list
 *
 * @param start
 * @param key to be found key
 * @return the list with the same key. NULL if not found.
 */
env_list *search_ele(env_list *start, const char *key);

int32_t modify_ele(env_list *list, const char *new_value);
env_list *delete_ele(env_list *start, const char *key);
#endif