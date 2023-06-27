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
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include "pwr_mngr.h"
#include "kvpfs_config.h"
#include "checksum.h"
#include "flash_mirror.h"
#include "list_data.h"
#include "kvpfs_err.h"

typedef enum {
  flash_space_enough = 0,
  flash_need_compact = 1,
  flash_space_not_enough = 2,
} flash_space_status_t;

struct KVPFS {
  uint8_t initialized;
  struct flash_mirror_info *flm_info;
  struct list_data_info *ldt_info;
} kvpfs_info __attribute__((section("GPMdata")));

static SemaphoreHandle_t kvpfs_mtx = NULL;

#define LOCK()  if (xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED) \
                  assert(xSemaphoreTakeFromISR(kvpfs_mtx, NULL) == pdTRUE); \
                else \
                  assert(xSemaphoreTake(kvpfs_mtx, portMAX_DELAY) == pdTRUE);

#define UNLOCK() if( xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED ) \
                  assert(xSemaphoreGiveFromISR(kvpfs_mtx, NULL) == pdTRUE); \
                 else \
                  assert(xSemaphoreGive(kvpfs_mtx) == pdTRUE);

void kvpfs_fss(void) {
  printf("primary addr.: %p, backup addr.: %p\n",kvpfs_info.flm_info->env_primary, kvpfs_info.flm_info->env_backup);
  printf("flash mirror\n\t");
  printf("env size:%ld, num zero:%ld, num data:%ld, used:%ld \n", kvpfs_info.flm_info->env_size,
         kvpfs_info.flm_info->num_of_zero, kvpfs_info.flm_info->num_of_data,
         kvpfs_info.flm_info->used_byte);
  printf("list data\n\t");
  printf("used: %ld, num data:%ld\n", kvpfs_info.ldt_info->used_byte,
         kvpfs_info.ldt_info->num_of_ele);
  // print_content();
}
static flash_space_status_t check_flash_space(void) {
  uint32_t expected_flash_size =
      kvpfs_info.flm_info->used_byte + kvpfs_info.ldt_info->used_byte + CRC_LEN;

  if (expected_flash_size < kvpfs_info.flm_info->env_size)
    return flash_space_enough;  // OK
  else if (expected_flash_size - kvpfs_info.flm_info->num_of_zero <= kvpfs_info.flm_info->env_size)
    return flash_need_compact;  // space is enough if compact flash
  else
    return flash_space_not_enough;  // space is not enough even if compact flash
}

static int32_t compact_flash(void) {
  char *pch = NULL;
  char *savep = NULL;
  char *mirror_data = NULL;
  uint32_t total_data_len;

  mirror_data = pvPortMalloc(kvpfs_info.flm_info->used_byte - kvpfs_info.flm_info->num_of_zero + 1);

  if (mirror_data == NULL) return -1;

  memset(mirror_data + kvpfs_info.flm_info->used_byte - kvpfs_info.flm_info->num_of_zero, '\0', 1);

  total_data_len = get_all_data_mirror(mirror_data, DEL_FF);

  if( kvpfs_info.flm_info->used_byte - kvpfs_info.flm_info->num_of_zero  != total_data_len )
  {
    printf("Total len = %lu\n",total_data_len);
    kvpfs_fss();
    print_content();
    assert(0);
  }
  clear_flash_mirror();

  pch = strtok_r(mirror_data, "\xff", &savep);
  while (pch != NULL) {
    if (strncmp(pch, CRC_KEYNAME, strlen(CRC_KEYNAME))) {
      if (append_data_mirror(pch)) {
        printf("%s\n", "ERROR!! flash is full");
        break;
      }
    }
    pch = strtok_r(NULL, "\xff", &savep);
  }

  kvpfs_info.flm_info->do_erase = 1;

  vPortFree(mirror_data);
  return 0;
}
static int32_t save_list_to_flash(void) {
  char *pch = NULL;
  char *savep = NULL;
  char *list_data = NULL;
  char *eqp = NULL;  // equal pointer

  if (kvpfs_info.ldt_info->num_of_ele == 0) return KVPFS_OK;

  list_data = pvPortMalloc(kvpfs_info.ldt_info->used_byte + 1);

  if (list_data == NULL) return -KVPFS_OUT_OF_MEM;

  memset(list_data + kvpfs_info.ldt_info->used_byte, '\0', 1);
  get_all_data_list(list_data, DEL_FF);

  pch = strtok_r(list_data, "\xff", &savep);
  while (pch != NULL) {
    if (append_data_mirror(pch)) {
      assert(0);  // had checked flash space. this suppose not to happen
    }

    eqp = strchr(pch, '=');
    if (eqp) {
      *eqp = '\0';
      delete_value_list(pch);
    }
    pch = strtok_r(NULL, "\xff", &savep);
  }

  vPortFree(list_data);
  return KVPFS_OK;
}

static int32_t flush_list_to_mirror()
{
  flash_space_status_t flash_space;

  flash_space = check_flash_space();

  if (flash_space == flash_need_compact) {
    if (compact_flash()) {
      return -KVPFS_OUT_OF_MEM;
    }
  } else if (flash_space == flash_space_not_enough) {
    return -KVPFS_FLASH_SPACE_NOT_ENOUGH;
  }

  return save_list_to_flash() ;

}
int32_t save_env_to_gpm(void)
{
  int32_t ret;
  if (kvpfs_info.initialized == 0)
    return -KVPFS_UNINITIALIZED;

  LOCK();
  ret = flush_list_to_mirror();
  UNLOCK();
  return ret;
}

int32_t save_env(void) {
  int32_t ret = KVPFS_OK;

  if (kvpfs_info.initialized == 0)
    return -KVPFS_UNINITIALIZED;
  LOCK();
  ret = flush_list_to_mirror();
  if( ret )
    goto fail;
  write_to_flash();
fail:
  UNLOCK();
  return ret;
}

int32_t get_env(const char *key, char **value) {
  if (kvpfs_info.initialized == 0) return -KVPFS_UNINITIALIZED;

  uint32_t chars_in_mirror = 0;
  uint32_t str_size = 0;
  int32_t ret;
  char *env_data = NULL;
  const char *ret_value = NULL;
  *value = NULL;

  LOCK();
  if (!key) {
    // list all data
    chars_in_mirror = kvpfs_info.flm_info->used_byte - kvpfs_info.flm_info->num_of_zero;
    str_size = chars_in_mirror + kvpfs_info.ldt_info->used_byte + 1;  // preserve for \0
    env_data = *value = pvPortMalloc(str_size);
    if (env_data == NULL) return -KVPFS_OUT_OF_MEM;

    get_all_data_mirror(env_data, DEL_LF);

    env_data += chars_in_mirror;
    get_all_data_list(env_data, DEL_LF);
    env_data += kvpfs_info.ldt_info->used_byte;
    memset(env_data, '\0', 1);
    ret = KVPFS_OK;
    goto exit;
  } else {
    // search flash mirror
    if (find_value_mirror(key, &ret_value) != NULL) {
      // strdup will fail on IAR build because malloc used different memory space than pvPortMalloc
      str_size = strlen(ret_value) + 1;
      *value = pvPortMalloc( str_size );
      memcpy(*value, ret_value, str_size);
      ret = KVPFS_OK;
      goto exit;
    }

    // search list
    if (find_value_list(key, &ret_value) == 0) {
      str_size = strlen(ret_value) + 1;
      *value = pvPortMalloc( str_size );
      memcpy(*value, ret_value, str_size);
      ret = KVPFS_OK;
      goto exit;
    }

    ret = -KVPFS_KEY_NOT_FOUND;
  }

exit:
  UNLOCK();
  return ret;
}

int32_t set_env(const char *key, const char *value) {
  int32_t ret = 0;

  if (kvpfs_info.initialized == 0) return -KVPFS_UNINITIALIZED;

  if (!key) return -KVPFS_NULL_POINTER;
  if (strcmp(key, CRC_KEYNAME) == 0)
    return -KVPFS_RESERVE_KEYWD;  // CRC_KEYNAME is a reserved word

  LOCK();
  if (value) {
    // add new env data
    delete_value_mirror(key);
    if (add_data_list(key, value)) ret = -KVPFS_OUT_OF_MEM;
  } else {
    //  delete env data
    if (delete_value_mirror(key)) {
      if (delete_value_list(key)) ret = -KVPFS_KEY_NOT_FOUND;
    }
  }
  UNLOCK();
  return ret;
}

static void kvpfs_stateful_init(void)
{
  /**
   * Called when returning from stateful boot.
   * Data keeps at GPM so that no need to load flash content again
   */
  if( kvpfs_mtx ) return;
  kvpfs_mtx = xSemaphoreCreateMutex();
  if (kvpfs_mtx == NULL) assert(0);

  LOCK();
  init_flash_mirror_from_stateful();
  init_list_data(&kvpfs_info.ldt_info);
  UNLOCK();
}

static void kvpfs_full_init(void)
{
  if( kvpfs_mtx ) return;
  kvpfs_mtx = xSemaphoreCreateMutex();
  if (kvpfs_mtx == NULL) assert(0);

  init_flash_mirror(&kvpfs_info.flm_info);
  init_list_data(&kvpfs_info.ldt_info);

  load_flash_content();
  kvpfs_info.initialized = 1;
}

void kvpfs_init(void) {

  int32_t res;
  pwr_wakeup_stat_t stat;

  res = pwr_stat_get_sleep_statistics(&stat);

  if ((res != 0) || (stat.wakeup_state != PWR_WAKEUP_STATEFUL))
    kvpfs_full_init();
  else
    kvpfs_stateful_init();
}

