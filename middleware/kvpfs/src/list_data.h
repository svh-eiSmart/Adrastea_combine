#ifndef __LIST_DATA_H__
#define __LIST_DATA_H__
#include <FreeRTOS.h>

struct list_data_info {
  uint32_t used_byte;  // includes "key=value\0"
  uint32_t num_of_ele;
};

#define DEL_FF (0)
#define DEL_LF (1)

void get_all_data_list(char *resp, int del );
int32_t add_data_list(const char *key, const char *value);
int32_t find_value_list(const char *key, const char **value);
int32_t delete_value_list(const char *key);
void init_list_data(struct list_data_info **ldt);
#endif