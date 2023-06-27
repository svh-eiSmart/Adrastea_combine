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

#ifndef __FLASH_MIRROR_H__
#define __FLASH_MIRROR_H__

#include "125X_mcu.h"

typedef int (*flash_write_fp)(uint8_t *, ulong, ulong, int);
typedef int (*flash_erase_fp)(ulong, int, int);

struct flash_mirror_info {
  uint32_t env_size;
  uint32_t used_byte;
  uint32_t num_of_data;  // how many data stored
  uint32_t num_of_zero;  // how many zero
  int8_t *env_primary;
  int8_t *env_backup;
  uint8_t do_erase;
  char *mirror;  // flash_mirror
  flash_write_fp flash_write;
  flash_erase_fp flash_erase;
};
#define DEL_FF (0)
#define DEL_LF (1)

typedef enum { PRIMARY = 0, BACKUP } env_bank;

void init_flash_mirror(struct flash_mirror_info **mirror_info);
void init_flash_mirror_from_stateful(void);
int32_t load_flash_content();
char *find_value_mirror(const char *search_key, const char **value);
int32_t delete_value_mirror(const char *searched_key);
void print_content();
uint32_t get_all_data_mirror(char *resp, int del);
void clear_flash_mirror(void);
int32_t erase_flash(env_bank erase_bank);
int32_t append_data_mirror(char *str);
int32_t write_to_flash(void);

#endif
