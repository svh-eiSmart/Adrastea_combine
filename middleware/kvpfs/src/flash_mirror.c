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
#include <semphr.h>
#include <task.h>
// #include "serial_flash_drv_api_1250b.h"
#include "flash_mirror.h"
#include "kvpfs_config.h"
#include "checksum.h"

#define ENV_MAGIC MAGIC_KEYNAME"="MAGIC_VALUE

static char flash_mirror[ENV_SIZE] __attribute__((section("GPMdata")));
static struct flash_mirror_info flm_info __attribute__((section("GPMdata")));
static SemaphoreHandle_t mirror_data_mtx = NULL;

#define LOCK() if (xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED) \
                  assert(xSemaphoreTakeFromISR(mirror_data_mtx, NULL) == pdTRUE); \
                else \
                  assert(xSemaphoreTake(mirror_data_mtx, portMAX_DELAY) == pdTRUE);
#define UNLOCK() if( xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED ) \
                  assert(xSemaphoreGiveFromISR(mirror_data_mtx, NULL) == pdTRUE); \
                else \
                  assert(xSemaphoreGive(mirror_data_mtx) == pdTRUE);


const char default_env_data[] = {
     "Alt=s\0"
    "def=def1\0"
    "def2=def2" };

void print_content() {
  int i, j;

  LOCK();
  for (i = 0; i < ENV_SIZE / 16; i++) {
    for (j = 0; j < 16; j++) {
      if (flash_mirror[i * 16 + j] == (char)0xFF)
        printf("%.2x ", 0xFF);
      else if (flash_mirror[i * 16 + j] == 0)
        printf("%.2x ", 0);
      else
        printf("%2c ", flash_mirror[i * 16 + j]);
    }
    printf("\n");
  }
  UNLOCK();
}

static uint32_t get_key_len(char *str) {
  char *pch;
  uint32_t key_len = 0;

  pch = strchr(str, '=');
  if (pch == NULL) return -1;  // no '=' in the string

  key_len = (uint32_t)(pch - str);
  // printf("str=%p, pch=%p keylen=%u\n",str, pch, key_len);
  return key_len;
}

static uint32_t validate_magic() { return strncmp(flash_mirror, ENV_MAGIC, ENV_MIGIC_LEN); }

static int32_t load_flash_mirror_info(void) {
  char *str = flash_mirror;
  uint32_t zero_cnt = 0;
  uint32_t data_cnt = 0;

  while ((uint32_t)(str - flash_mirror) < flm_info.env_size) {
    if (*str == '\0')
      zero_cnt++;
    else if (*str == (char)0xFF)
      break;
    else
    {
      str += strlen(str);
      data_cnt++;
      continue;
    }
    str++;
  }

  flm_info.num_of_data = data_cnt;
  flm_info.num_of_zero = zero_cnt - data_cnt;
  flm_info.used_byte = str - flash_mirror;

  return 0;
}

static int32_t validate_env(env_bank select_bank) {
  uint32_t ret = 0;
  uint16_t crc_calc = 0;
  uint16_t crc_env = 0;
  char *pos = NULL;
  const char *value = NULL;

  if (select_bank == BACKUP)
    memcpy(flash_mirror, flm_info.env_backup, ENV_SIZE);
  else
    memcpy(flash_mirror, flm_info.env_primary, ENV_SIZE);

  if (validate_magic()) {
    printf("Wrong Magic!! \n");
    return -1;
  }

  pos = find_value_mirror(CRC_KEYNAME, &value);

  if (pos) {
    crc_calc = crc_16((unsigned char *)flash_mirror, pos - flash_mirror);
    crc_env = strtoul(value, NULL, 16);
    printf("crc_calc %x crc_env %x\n", crc_calc, crc_env);
    if (crc_calc != crc_env) ret = -1;  // crc mismatch
  } else
    ret = -1;  // can't find "crc" keyword
  return ret;
}

static inline int32_t has_room_to_write(uint32_t request_len) {
  if (flm_info.used_byte + request_len + CRC_LEN > flm_info.env_size) return -1;
  return 0;
}

int32_t append_data_mirror(char *str) {
  char *free_pos = flash_mirror + flm_info.used_byte;
  uint32_t write_len = strlen(str) + 1;

  if (strncmp(str, CRC_KEYNAME, strlen(CRC_KEYNAME))) {
    // check if room is enough if not writing CRC
    if (has_room_to_write(write_len)) {
      return -1;
    }
  }
  LOCK();
  memcpy(free_pos, str, write_len);
  flm_info.used_byte += write_len;
  flm_info.num_of_data++;
  UNLOCK();
  return 0;
}

int32_t delete_value_mirror(const char *searched_key) {
  char *pos = 0;
  const char *value;

  if ((pos = find_value_mirror(searched_key, &value)) != NULL) {
    LOCK();
    flm_info.num_of_data--;
    flm_info.num_of_zero += strlen(pos) + 1;  // plus original \0
    memset(pos, '\0', strlen(pos));
    UNLOCK();
    return 0;
  }
  return -1;
}

/**
 * @brief return the pointer of occurence
 *
 * @param search_key
 * @param value
 * @return start position of the env data
 */
char *find_value_mirror(const char *search_key, const char **value) {
  uint16_t str_len = 0;
  char *str = flash_mirror;
  uint32_t key_len = 0;
  char *ret = NULL;

  LOCK();
  while ((uint32_t)(str - flash_mirror) < flm_info.env_size) {
    // printf("str pos %d\n", str - flash_mirror);
    if (*str == '\0')
      str++;
    else if (*str != (char)0xFF) {
      str_len = strlen(str);  // str: key=value
      key_len = get_key_len(str);
      // printf("strlen=%u, keylen=%lu\n", str_len, key_len);
      if (key_len == strlen(search_key) && strncmp(str, search_key, key_len) == 0) {
        // found key
        *value = str + key_len + 1;
        //   printf("value = %s(%p)\n", *value, *value);
        ret = str;
        break;
      } else
        str += str_len + 1;  // points to the first byte of next key
    } else
      break;
  }
  UNLOCK();
  return ret;
}

void clear_flash_mirror(void) {
  LOCK();
  memset(flash_mirror, 0xff, flm_info.env_size);
  flm_info.num_of_data = 0;
  flm_info.num_of_zero = 0;
  flm_info.used_byte = 0;
  UNLOCK();
}

uint32_t get_all_data_mirror(char *resp, int del) {
  char *str = flash_mirror;
  uint32_t resp_index = 0;
  uint32_t str_len;
  char delim;

  if( del == 1)
    delim = '\n';
  else
    delim = '\xff';

  if (resp == NULL) return 0;

  LOCK();

  str = flash_mirror;
  while ((uint32_t)(str - flash_mirror) < flm_info.env_size) {
    // printf("str:%s(%d) \n",str, strlen(str));
    if (*str == '\0')
      str++;
    else if (*str != (char)0xFF) {
      // printf("%s %s\n", str);
      str_len = strlen(str);
      strncpy(resp + resp_index, str, str_len);
      resp_index += str_len;
      resp[resp_index++] = delim;
      str += str_len + 1;
    } else
      break;
  }

  UNLOCK();
  return resp_index;
}

int32_t load_default_setting() {
  char *mirror = flash_mirror;
  uint16_t crc_calc;
  uint32_t req_len = sizeof(default_env_data) + ENV_MIGIC_LEN + CRC_LEN;
  uint32_t index = 0;

  memset(mirror, 0xff, flm_info.env_size);
  memcpy(mirror, ENV_MAGIC, ENV_MIGIC_LEN);
  index += ENV_MIGIC_LEN;

  if (req_len <= flm_info.env_size) {
    memcpy(mirror + ENV_MIGIC_LEN, default_env_data, sizeof(default_env_data));
    index += sizeof(default_env_data);
  }

  crc_calc = crc_16((unsigned char *)mirror, index);

  snprintf(mirror + index, flm_info.env_size - index, "%s=%x", CRC_KEYNAME, crc_calc);
  return 0;
}

int32_t load_flash_content(void) {
  int32_t backup_ret, primary_ret;
  backup_ret = validate_env(BACKUP);
  primary_ret = validate_env(PRIMARY);


  if (primary_ret != 0 && backup_ret == 0) {
    //  primary broken. restore backup to primary
    printf("Primary env is broken. Restore from backup\n");
    memcpy(flm_info.mirror, flm_info.env_backup, flm_info.env_size);
    erase_flash(PRIMARY);
    flm_info.flash_write((uint8_t *)flm_info.mirror, (ulong)flm_info.env_primary,
                         flm_info.env_size - 1, 1);
  } else if (primary_ret == 0 && backup_ret != 0) {
    //  backup broken. restore primary to backup
    printf("Backup env is broken. Restore from primary\n");

    erase_flash(BACKUP);
    flm_info.flash_write((uint8_t *)flm_info.mirror, (ulong)flm_info.env_backup,
                         flm_info.env_size - 1, 1);
  } else if (primary_ret != 0 && backup_ret != 0) {
    //  both bank broken. restore default or force use it.
    printf("Both env are broken. Restore to default\n");
    erase_flash(PRIMARY);
    erase_flash(BACKUP);
    load_default_setting();
  }
  load_flash_mirror_info();

  return 0;
}
int32_t erase_flash(env_bank erase_bank) {
  int8_t *bank;
  uint32_t env_size = flm_info.env_size;
  if (erase_bank > BACKUP) return -1;

  bank = erase_bank == BACKUP ? flm_info.env_backup : flm_info.env_primary;

  do {
    flm_info.flash_erase((uint32_t)bank, 0, 1);
    env_size = env_size > 4096 ? env_size - 4096 : 0;
    bank += 4096;
  } while (env_size > 0);

  return 0;
}

int32_t write_to_flash(void) {
  uint16_t crc_calc;
  char crc[CRC_LEN];

  delete_value_mirror(CRC_KEYNAME);

  crc_calc = crc_16((unsigned char *)flash_mirror, flm_info.used_byte);

  snprintf(crc, CRC_LEN, "%s=%x", CRC_KEYNAME, crc_calc);

  append_data_mirror(crc);

  if( flm_info.do_erase )
    erase_flash(PRIMARY);
  flm_info.flash_write((uint8_t *)flm_info.mirror, (ulong)flm_info.env_primary,
                       flm_info.env_size - 1, 1);

  if( flm_info.do_erase )
    erase_flash(BACKUP);
  flm_info.flash_write((uint8_t *)flm_info.mirror, (ulong)flm_info.env_backup,
                       flm_info.env_size - 1, 1);

  flm_info.do_erase = 0;
  return 0;
}

static void determin_kvpfs_secter(struct flash_mirror_info *mirro_info)
{
  int8_t *backup_addr;
  mirro_info->env_primary = (int8_t *)(KVPFS_BASE_ADDR & SFLASH_4K_MASK); //align primary bank to start of 4k

  backup_addr = mirro_info->env_primary + ENV_SIZE;
  backup_addr = (int8_t *)((int32_t)backup_addr & SFLASH_4K_MASK); // align backup to start of 4K
  if( backup_addr < mirro_info->env_primary + ENV_SIZE)
    backup_addr += SFLASH_4K_ERASE_SECTOR_SIZE; // if backup is within the range of primary, round it to next 4K

  mirro_info->env_backup = backup_addr;

}

void init_flash_mirror(struct flash_mirror_info **mirror_info) {
  if( mirror_data_mtx ) return;
  mirror_data_mtx = xSemaphoreCreateMutex();
  if (mirror_data_mtx == NULL) assert(0);

  serial_flash_api_init(BASE_ADDRESS_SERIAL_FLASH_CTRL_MCU_SUBSYS);
  memset(&flm_info,'\0', sizeof(struct flash_mirror_info ));
  flm_info.mirror = flash_mirror;
  flm_info.env_size = ENV_SIZE;
  determin_kvpfs_secter(&flm_info);
  flm_info.flash_write = serial_flash_write_buffer;
  flm_info.flash_erase = serial_flash_erase_sector;
  *mirror_info = &flm_info;
}

void init_flash_mirror_from_stateful(void)
{
  if( mirror_data_mtx ) return;
  mirror_data_mtx = xSemaphoreCreateMutex();
  if (mirror_data_mtx == NULL) assert(0);
  serial_flash_api_init(BASE_ADDRESS_SERIAL_FLASH_CTRL_MCU_SUBSYS);
}
