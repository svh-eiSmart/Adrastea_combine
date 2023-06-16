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

/****************************************************************************
 * Included Files
 ****************************************************************************/

/* Standard includes. */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "assert.h"

/* CMSIS-RTOS V2 includes. */
#include "cmsis_os2.h"

/* ALTCOM API includes */
#include "io/altcom_io.h"
#include "fs/altcom_fs.h"

/* App includes */
#include "apitest_main.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define APITEST_RESULT_OK (0)
#define APITEST_RESULT_ERROR (1)

#define APITEST_CMD_ARG (1)
#define APITEST_CMD_PARAM_1 (2)
#define APITEST_CMD_PARAM_2 (3)
#define APITEST_CMD_PARAM_3 (4)
#define APITEST_CMD_PARAM_4 (5)
#define APITEST_CMD_PARAM_5 (6)
#define APITEST_CMD_PARAM_6 (7)
#define APITEST_CMD_PARAM_7 (8)
#define APITEST_CMD_PARAM_8 (9)

#define APITEST_MAX_NUMBER_OF_PARAM (12)
#define APITEST_MAX_API_MQUEUE (16)

#define APITEST_TASK_STACKSIZE (2048)
#define APITEST_TASK_PRI (osPriorityNormal)

#define APITEST_NEWFILE_ARG ("NEW")
#define APITEST_OLDFILE_ARG ("OLD")
#define APITEST_RDONLY_ARG ("RDONLY")
#define APITEST_WRONLY_ARG ("WRONLY")
#define APITEST_RDWR_ARG ("RDWR")
#define APITEST_SEEKSET_ARG ("SET")
#define APITEST_SEEKCUR_ARG ("CUR")
#define APITEST_SEEKEND_ARG ("END")

#define APITEST_GETFUNCNAME(func) (#func)

#define LOCK() apitest_log_lock()
#define UNLOCK() apitest_log_unlock()

#define APITEST_DBG_PRINT(...) \
  do {                         \
    LOCK();                    \
    printf(__VA_ARGS__);       \
    UNLOCK();                  \
  } while (0)

#define APITEST_FREE_CMD(cmd, argc)    \
  do {                                 \
    int32_t idx;                       \
    for (idx = 0; idx < argc; idx++) { \
      free(cmd->argv[idx]);            \
      cmd->argv[idx] = NULL;           \
    }                                  \
                                       \
    free(cmd);                         \
  } while (0);
/****************************************************************************
 * Private Data Type
 ****************************************************************************/
struct apitest_command_s {
  int argc;
  char *argv[APITEST_MAX_NUMBER_OF_PARAM];
};

struct data_s {
  char *data;
  int16_t len;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static bool app_init = false;
static bool taskrunning = false;
static osMessageQueueId_t cmdq = NULL;
static osMutexId_t app_log_mtx = NULL;

/****************************************************************************
 * Public Data
 ****************************************************************************/

uint8_t *wrBuf = NULL;
uint16_t wrDataLen = 0;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void print_api_test_help() {
  APITEST_DBG_PRINT("apitest <io> <open> <NEW|OLD> <RDONLY|WRONLY|RDWR> <filename>\n");
  APITEST_DBG_PRINT("apitest <io> <close>\n");
  APITEST_DBG_PRINT("apitest <io> <read> <length>\n");
  APITEST_DBG_PRINT("apitest <io> <write>; This CLI need to collaborate with \"paste\" mode\n");
  APITEST_DBG_PRINT("apitest <io> <remove> <filename>\n");
  APITEST_DBG_PRINT("apitest <io> <lseek> <whence> <offset>\n");
  APITEST_DBG_PRINT("apitest <io> <copy> <srcfile> <destfile>\n");
  APITEST_DBG_PRINT("apitest <fs> <gtlist> <pathname> <list_size> <path_len>\n");
  APITEST_DBG_PRINT("apitest <fs> <opendir> <dirname>\n");
  APITEST_DBG_PRINT("apitest <fs> <readdir>\n");
  APITEST_DBG_PRINT("apitest <fs> <closedir>\n");
  APITEST_DBG_PRINT("apitest <fs> <mkdir> <dirname>\n");
  APITEST_DBG_PRINT("apitest <fs> <rmdir> <dirname>\n");
  APITEST_DBG_PRINT("apitest <fs> <stat> <dirname|filename>\n");
  APITEST_DBG_PRINT("apitest <fs> <ls> <dirname|filename>\n");
}

static void print_retval(bool val, char *str) {
  if (!val) {
    APITEST_DBG_PRINT("[API_NG] %s return val : \"%d\"\n", str, val);
  } else {
    APITEST_DBG_PRINT("[API_OK] %s return val : \"%d\"\n", str, val);
  }
}

static void ls_cli_impl(const char *path_name) {
#define LS_CLI_RESERVED_WIDTH (30)
  if (path_name == NULL || strlen(path_name) == 0) {
    APITEST_DBG_PRINT("Invalid path_name %p\"\n", path_name);
    return;
  }

  altcom_fs_stat_t statbuf;
  altcom_fs_err_t ret;

  ret = altcom_fs_stat(path_name, &statbuf);
  if (ret != FS_NO_ERR) {
    APITEST_DBG_PRINT("Invalid path_name %s\"\n", path_name);
    return;
  }

  if (statbuf.st_mode == FS_TYPE_DIR) {
    altcom_fs_dir_t dir;

    ret = altcom_fs_opendir(path_name, &dir);
    if (ret != FS_NO_ERR) {
      APITEST_DBG_PRINT("Invalid path_name %s\"\n", path_name);
      return;
    }

    APITEST_DBG_PRINT("\nDirectory %s:\n", path_name);

    altcom_fs_dirent_t dirent;

    ret = altcom_fs_readdir(dir, &dirent);
    if (ret != FS_NO_ERR) {
      return;
    }
    do {
      char tmpstr[ALTCOM_FS_PATH_MAXLEN];

      if (dirent.d_type == FS_TYPE_DIR) {
        snprintf(tmpstr, ALTCOM_FS_PATH_MAXLEN, "*%s*", dirent.d_name);
        APITEST_DBG_PRINT("\t%*s\n", LS_CLI_RESERVED_WIDTH, tmpstr);
      } else {
        snprintf(tmpstr, ALTCOM_FS_PATH_MAXLEN, "%s/%s", path_name, dirent.d_name);
        ret = altcom_fs_stat(tmpstr, &statbuf);
        assert(ret == FS_NO_ERR);
        assert(statbuf.st_mode == FS_TYPE_FILE);

        APITEST_DBG_PRINT("\t%*s\tsize\t%10lu\n", LS_CLI_RESERVED_WIDTH, dirent.d_name,
                          (uint32_t)statbuf.st_size);
      }
    } while ((ret = altcom_fs_readdir(dir, &dirent)) == FS_NO_ERR);
    ret = altcom_fs_closedir(dir);
    assert(ret == FS_NO_ERR);
  } else {
    APITEST_DBG_PRINT("\t%*s\tsize\t%10lu\n", LS_CLI_RESERVED_WIDTH, path_name,
                      (uint32_t)statbuf.st_size);
  }

  APITEST_DBG_PRINT("\n");
}

static ssize_t copy_file(int src_fd, int dst_fd) {
#define COPY_CHUNK_SIZE (3000)

  ssize_t res, total_len;
  static uint8_t *buf = NULL;

  if (!buf) {
    buf = malloc(COPY_CHUNK_SIZE);
    assert(buf != NULL);
  }

  res = 0;
  total_len = 0;
  while (res >= 0) {
    res = altcom_io_read(src_fd, buf, COPY_CHUNK_SIZE);
    if (res > 0) {
      res = altcom_io_write(dst_fd, buf, (size_t)res);
      if (res > 0) {
        total_len += res;
        continue;
      } else {
        APITEST_DBG_PRINT("altcom_io_write fail, res:%d, total_len: %d\n", res, total_len);
        total_len = res;
      }
    } else if (res == 0) {
      APITEST_DBG_PRINT("EOF\n");
      break;
    } else {
      APITEST_DBG_PRINT("altcom_io_read fail, total_len: %d\n", total_len);
      total_len = res;
    }
  }

  return total_len;
}

static void apitest_task(void *arg) {
  osStatus_t ret;
  struct apitest_command_s *command;
  static int fd = -1;
  static altcom_fs_dir_t dir = 0;

  taskrunning = true;
  if (NULL == cmdq) {
    APITEST_DBG_PRINT("cmdq is NULL!!\n");
    while (1)
      ;
  }

  while (1) {
    /* keep waiting until send commands */

    APITEST_DBG_PRINT("Entering blocking by osMessageQueueGet.\n");
    ret = osMessageQueueGet(cmdq, (void *)&command, 0, osWaitForever);
    if (ret != osOK) {
      APITEST_DBG_PRINT("osMessageQueueGet fail[%ld]\n", (int32_t)ret);
      continue;
    } else {
      APITEST_DBG_PRINT("osMessageQueueGet success\n");
    }

    if (command && command->argc >= 1) {
      if (!strncmp(command->argv[APITEST_CMD_ARG], "io", strlen("io"))) {
        if (!strncmp(command->argv[APITEST_CMD_PARAM_1], "open", strlen("open"))) {
          if (command->argc == 6) {
            if (fd != -1) {
              APITEST_DBG_PRINT("try to close an openned fd %d\n", fd);
              print_retval(altcom_io_close(fd) != -1, APITEST_GETFUNCNAME(altcom_io_close));
            }

            bool newfile;

            if (!strncmp(command->argv[APITEST_CMD_PARAM_2], APITEST_NEWFILE_ARG,
                         strlen(APITEST_NEWFILE_ARG))) {
              newfile = true;
            } else if (!strncmp(command->argv[APITEST_CMD_PARAM_2], APITEST_OLDFILE_ARG,
                                strlen(APITEST_OLDFILE_ARG))) {
              newfile = false;
            } else {
              APITEST_DBG_PRINT("invalid parameter %s\n", command->argv[APITEST_CMD_PARAM_2]);
              goto errout_with_param;
            }

            int flags;

            if (!strncmp(command->argv[APITEST_CMD_PARAM_3], APITEST_RDONLY_ARG,
                         strlen(APITEST_RDONLY_ARG))) {
              flags = O_RDONLY;
            } else if (!strncmp(command->argv[APITEST_CMD_PARAM_3], APITEST_WRONLY_ARG,
                                strlen(APITEST_WRONLY_ARG))) {
              flags = O_WRONLY;
            } else if (!strncmp(command->argv[APITEST_CMD_PARAM_3], APITEST_RDWR_ARG,
                                strlen(APITEST_RDWR_ARG))) {
              flags = O_RDWR;
            } else {
              APITEST_DBG_PRINT("invalid parameter operation mode %s\n",
                                command->argv[APITEST_CMD_PARAM_3]);
              goto errout_with_param;
            }

            if (newfile && (flags == O_RDONLY)) {
              APITEST_DBG_PRINT("can't read a new file\n");
              goto errout_with_param;
            }

            fd = altcom_io_open(command->argv[APITEST_CMD_PARAM_4],
                                newfile == true ? (flags | O_CREAT | O_TRUNC) : (flags),
                                S_IRWXU | S_IRWXG | S_IRWXO);
            print_retval(fd != -1, APITEST_GETFUNCNAME(altcom_io_open));
          } else {
            APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
            goto errout_with_param;
          }
        } else if (!strncmp(command->argv[APITEST_CMD_PARAM_1], "close", strlen("close"))) {
          if (fd != -1) {
            print_retval(altcom_io_close(fd) != -1, APITEST_GETFUNCNAME(altcom_io_close));
            fd = -1;
          } else {
            APITEST_DBG_PRINT("Invalid file descriptor\n");
          }
        } else if (!strncmp(command->argv[APITEST_CMD_PARAM_1], "read", strlen("read"))) {
          if (command->argc == 4) {
            static size_t count = 0;
            size_t tmp_count;
            static uint8_t *buf = NULL;

            tmp_count = strtoul(command->argv[APITEST_CMD_PARAM_2], NULL, 10);
            if (tmp_count > count) {
              if (buf) {
                free(buf);
              }

              buf = malloc(tmp_count);
              assert(buf != NULL);

              count = tmp_count;
            }

            if (fd != -1) {
              ssize_t res;

              res = altcom_io_read(fd, (void *)buf, count);
              print_retval(res != -1, APITEST_GETFUNCNAME(altcom_io_read));
              if (res != -1) {
                APITEST_DBG_PRINT("\n==read content start==\n");
                APITEST_DBG_PRINT("%.*s", res, buf);
                APITEST_DBG_PRINT("\n==read content end, length %d==\n", res);
              }
            } else {
              APITEST_DBG_PRINT("invalid file descriptor\n");
            }
          } else {
            APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
            goto errout_with_param;
          }
        } else if (!strncmp(command->argv[APITEST_CMD_PARAM_1], "write", strlen("write"))) {
          if (command->argc == 3) {
            if (fd != -1) {
              ssize_t res;

              if (wrDataLen != 0) {
                res = altcom_io_write(fd, wrBuf, wrDataLen);
                print_retval(res != -1, APITEST_GETFUNCNAME(altcom_io_write));
                if (res != -1) {
                  APITEST_DBG_PRINT("data length: %hu, written length: %d\n", wrDataLen, res);
                }

                free(wrBuf);
                wrBuf = NULL;
                wrDataLen = 0;
              } else {
                APITEST_DBG_PRINT("empty wrBuf, please write something in \"paste\" mode\n");
                goto errout_with_param;
              }
            } else {
              APITEST_DBG_PRINT("invalid file descriptor\n");
            }
          }
        } else if (!strncmp(command->argv[APITEST_CMD_PARAM_1], "remove", strlen("remove"))) {
          if (command->argc == 4) {
            print_retval(altcom_io_remove(command->argv[APITEST_CMD_PARAM_2]) != -1,
                         APITEST_GETFUNCNAME(altcom_io_remove));
          } else {
            APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
            goto errout_with_param;
          }
        } else if (!strncmp(command->argv[APITEST_CMD_PARAM_1], "lseek", strlen("lseek"))) {
          if (command->argc == 5) {
            off_t offset;
            int whence;

            if (!strncmp(command->argv[APITEST_CMD_PARAM_2], APITEST_SEEKSET_ARG,
                         strlen(APITEST_SEEKSET_ARG))) {
              whence = SEEK_SET;
            } else if (!strncmp(command->argv[APITEST_CMD_PARAM_2], APITEST_SEEKCUR_ARG,
                                strlen(APITEST_SEEKCUR_ARG))) {
              whence = SEEK_CUR;
            } else if (!strncmp(command->argv[APITEST_CMD_PARAM_2], APITEST_SEEKEND_ARG,
                                strlen(APITEST_SEEKEND_ARG))) {
              whence = SEEK_END;
            } else {
              APITEST_DBG_PRINT("invalid parameter seek mode %s\n",
                                command->argv[APITEST_CMD_PARAM_2]);
              goto errout_with_param;
            }

            offset = (off_t)strtoll(command->argv[APITEST_CMD_PARAM_3], NULL, 10);
            print_retval(altcom_io_lseek(fd, offset, whence) != -1,
                         APITEST_GETFUNCNAME(altcom_io_lseek));
          } else {
            APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
            goto errout_with_param;
          }
        } else if (!strncmp(command->argv[APITEST_CMD_PARAM_1], "copy", strlen("copy"))) {
          if (command->argc == 5) {
            int src_fd, dst_fd;

            APITEST_DBG_PRINT("Start copying file from %s to %s\n",
                              command->argv[APITEST_CMD_PARAM_2],
                              command->argv[APITEST_CMD_PARAM_3]);

            src_fd = altcom_io_open(command->argv[APITEST_CMD_PARAM_2], O_RDONLY, 0);
            dst_fd = altcom_io_open(command->argv[APITEST_CMD_PARAM_3],
                                    O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
            if (src_fd == -1 || dst_fd == -1) {
              APITEST_DBG_PRINT("file open fail, src:%d, dst:%d\n", src_fd, dst_fd);
              if (src_fd != -1) {
                if (altcom_io_close(src_fd) != 0) {
                  APITEST_DBG_PRINT("fail to close %s\n", command->argv[APITEST_CMD_PARAM_2]);
                }
              }

              if (dst_fd != -1) {
                if (altcom_io_close(dst_fd) != 0) {
                  APITEST_DBG_PRINT("fail to close %s\n", command->argv[APITEST_CMD_PARAM_3]);
                }
              }

              goto errout_with_param;
            }

            ssize_t copy_res;

            if ((copy_res = copy_file(src_fd, dst_fd)) < 0) {
              APITEST_DBG_PRINT("fail to copy file, res: %d\n", copy_res);
            } else {
              APITEST_DBG_PRINT("succeed to copy file, len: %d\n", copy_res);
            }

            if (altcom_io_close(src_fd) != 0) {
              APITEST_DBG_PRINT("fail to close %s\n", command->argv[APITEST_CMD_PARAM_2]);
            }

            if (altcom_io_close(dst_fd) != 0) {
              APITEST_DBG_PRINT("fail to close %s\n", command->argv[APITEST_CMD_PARAM_3]);
            }
          } else {
            APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
            goto errout_with_param;
          }
        } else {
          APITEST_DBG_PRINT("invalid sub-command \"%s\"\n", command->argv[APITEST_CMD_PARAM_1]);
          print_api_test_help();
          goto errout_with_param;
        }
      } else if (!strncmp(command->argv[APITEST_CMD_ARG], "fs", strlen("fs"))) {
        if (!strncmp(command->argv[APITEST_CMD_PARAM_1], "gtlist", strlen("gtlist"))) {
          if (command->argc == 6) {
            static char *list_buf = NULL;
            char *list[ALTCOM_FS_LIST_MAXSIZE] = {0};
            uint8_t list_size, path_len;
            int i;

            if (!list_buf) {
              list_buf = malloc(ALTCOM_FS_LIST_MAXSIZE * ALTCOM_FS_PATH_MAXLEN);
              assert(list_buf != NULL);
            }

            memset((void *)list_buf, 0, ALTCOM_FS_LIST_MAXSIZE * ALTCOM_FS_PATH_MAXLEN);

            list_size = (uint8_t)strtoul(command->argv[APITEST_CMD_PARAM_3], NULL, 10);
            path_len = (uint8_t)strtoul(command->argv[APITEST_CMD_PARAM_4], NULL, 10);
            for (i = 0; i < ALTCOM_FS_LIST_MAXSIZE; i++) {
              list[i] = list_buf + i * path_len;
            }

            int32_t res;

            res = altcom_fs_get_filelist(command->argv[APITEST_CMD_PARAM_2], list_size, path_len,
                                         (char **)list_buf);
            print_retval(res >= 0, APITEST_GETFUNCNAME(altcom_fs_get_filelist));
            if (res >= 0) {
              APITEST_DBG_PRINT("%lu file(s) in %s\n", res, command->argv[APITEST_CMD_PARAM_1]);
              for (i = 0; i < res; i++) {
                if (strlen(list[i]) == 0) {
                  continue;
                }

                APITEST_DBG_PRINT("==>%s\n", list[i]);
              }
            } else {
              APITEST_DBG_PRINT("get file list failed, res = %ld\n", res);
            }
          } else {
            APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
            goto errout_with_param;
          }
        } else if (!strncmp(command->argv[APITEST_CMD_PARAM_1], "opendir", strlen("opendir"))) {
          if (command->argc == 4) {
            if (dir != 0) {
              APITEST_DBG_PRINT("try to closedir an openned dir %lu\n", dir);
              print_retval(altcom_fs_closedir(dir) == FS_NO_ERR,
                           APITEST_GETFUNCNAME(altcom_fs_closedir));
            }

            altcom_fs_err_t res;

            res = altcom_fs_opendir(command->argv[APITEST_CMD_PARAM_2], &dir);
            print_retval(res == FS_NO_ERR, APITEST_GETFUNCNAME(altcom_io_open));
            if (res != FS_NO_ERR) {
              APITEST_DBG_PRINT("errcode %ld\n", (int32_t)res);
            }
          } else {
            APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
            goto errout_with_param;
          }
        } else if (!strncmp(command->argv[APITEST_CMD_PARAM_1], "closedir", strlen("closedir"))) {
          if (command->argc == 3) {
            if (dir != 0) {
              altcom_fs_err_t res;

              res = altcom_fs_closedir(dir);
              print_retval(res == FS_NO_ERR, APITEST_GETFUNCNAME(altcom_fs_closedir));
              if (res != FS_NO_ERR) {
                APITEST_DBG_PRINT("errcode %ld\n", (int32_t)res);
              }

              dir = 0;
            } else {
              APITEST_DBG_PRINT("Invalid dir handle\n");
            }
          } else {
            APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
            goto errout_with_param;
          }
        } else if (!strncmp(command->argv[APITEST_CMD_PARAM_1], "readdir", strlen("readdir"))) {
          if (command->argc == 3) {
            if (dir != 0) {
              altcom_fs_dirent_t dirent;
              altcom_fs_err_t res;

              res = altcom_fs_readdir(dir, &dirent);
              print_retval(res == FS_NO_ERR, APITEST_GETFUNCNAME(altcom_fs_readdir));
              if (res == FS_NO_ERR) {
                APITEST_DBG_PRINT("Entry name: %s\n", dirent.d_name);
                APITEST_DBG_PRINT("Entry type: %s\n",
                                  (dirent.d_type == FS_TYPE_DIR ? "dir" : "file"));
              } else {
                APITEST_DBG_PRINT("errcode %ld\n", (int32_t)res);
              }
            } else {
              APITEST_DBG_PRINT("Invalid dir handle\n");
            }
          } else {
            APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
            goto errout_with_param;
          }
        } else if (!strncmp(command->argv[APITEST_CMD_PARAM_1], "mkdir", strlen("mkdir"))) {
          if (command->argc == 4) {
            altcom_fs_err_t res;

            res = altcom_fs_mkdir(command->argv[APITEST_CMD_PARAM_2]);
            print_retval(res == FS_NO_ERR, APITEST_GETFUNCNAME(altcom_fs_mkdir));
            if (res != FS_NO_ERR) {
              APITEST_DBG_PRINT("errcode of mkdir %ld\n", (int32_t)res);
            }
          } else {
            APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
            goto errout_with_param;
          }
        } else if (!strncmp(command->argv[APITEST_CMD_PARAM_1], "rmdir", strlen("rmdir"))) {
          if (command->argc == 4) {
            altcom_fs_err_t res;

            res = altcom_fs_rmdir(command->argv[APITEST_CMD_PARAM_2]);
            print_retval(res == FS_NO_ERR, APITEST_GETFUNCNAME(altcom_fs_rmdir));
            if (res != FS_NO_ERR) {
              APITEST_DBG_PRINT("errcode of rmdir %ld\n", (int32_t)res);
            }
          } else {
            APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
            goto errout_with_param;
          }
        } else if (!strncmp(command->argv[APITEST_CMD_PARAM_1], "stat", strlen("stat"))) {
          if (command->argc == 4) {
            altcom_fs_err_t res;
            altcom_fs_stat_t statbuf;

            res = altcom_fs_stat(command->argv[APITEST_CMD_PARAM_2], &statbuf);
            print_retval(res == FS_NO_ERR, APITEST_GETFUNCNAME(altcom_fs_stat));
            if (res == FS_NO_ERR) {
              APITEST_DBG_PRINT("Target type: %s\n",
                                (statbuf.st_mode == FS_TYPE_DIR ? "dir" : "file"));
              APITEST_DBG_PRINT("Target length: %ld\n", (int32_t)statbuf.st_size);
            } else {
              APITEST_DBG_PRINT("errcode of stat %ld\n", (int32_t)res);
            }
          } else {
            APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
            goto errout_with_param;
          }
        } else if (!strncmp(command->argv[APITEST_CMD_PARAM_1], "ls", strlen("ls"))) {
          if (command->argc == 4) {
            ls_cli_impl(command->argv[APITEST_CMD_PARAM_2]);
          } else {
            APITEST_DBG_PRINT("unexpected parameter number %d\n", command->argc);
            goto errout_with_param;
          }
        } else {
          APITEST_DBG_PRINT("invalid sub-command \"%s\"\n", command->argv[APITEST_CMD_PARAM_1]);
          print_api_test_help();
          goto errout_with_param;
        }
      } else if (strncmp(command->argv[APITEST_CMD_ARG], "help", strlen("help")) == 0) {
        if (command->argc == 3) {
          print_api_test_help();
        }
      } else {
        APITEST_DBG_PRINT("invalid command \"%s\"\n", command->argv[APITEST_CMD_ARG]);
        print_api_test_help();
        goto errout_with_param;
      }
    }

  errout_with_param:
    if (command) {
      APITEST_FREE_CMD(command, command->argc);
      command = NULL;
    }
  }

  osThreadExit();
  return;
}
/****************************************************************************
 * Public Functions
 ****************************************************************************/

void apitest_log_lock(void) {
  if (app_log_mtx) {
    osMutexAcquire(app_log_mtx, osWaitForever);
  }
}

void apitest_log_unlock(void) {
  if (app_log_mtx) {
    osMutexRelease(app_log_mtx);
  }
}

int32_t apitest_init(void) {
  if (!app_init) {
    app_log_mtx = osMutexNew(NULL);
    if (!app_log_mtx) {
      APITEST_DBG_PRINT("app_log_mtx init failed\n");
      goto errout;
    }

    cmdq = osMessageQueueNew(APITEST_MAX_API_MQUEUE, sizeof(struct apitest_command_s *), NULL);
    if (!cmdq) {
      APITEST_DBG_PRINT("cmdq init failed\n");
      goto errout;
    }
  } else {
    APITEST_DBG_PRINT("App already initialized\n");
  }

  app_init = true;
  return 0;

errout:
  if (app_log_mtx) {
    osMutexDelete(app_log_mtx);
    app_log_mtx = NULL;
  }

  if (cmdq) {
    osMessageQueueDelete(cmdq);
    cmdq = NULL;
  }

  return -1;
}

int32_t apitest_main(int32_t argc, char *argv[]) {
  struct apitest_command_s *command;
  int32_t itr = 0;

  if (!app_init) {
    APITEST_DBG_PRINT("App not yet initialized\n");
    return -1;
  }

  if (2 <= argc) {
    if (APITEST_MAX_NUMBER_OF_PARAM < argc) {
      APITEST_DBG_PRINT("too many arguments\n");
      return -1;
    }

    if ((command = malloc(sizeof(struct apitest_command_s))) == NULL) {
      APITEST_DBG_PRINT("malloc fail\n");
      return -1;
    }

    memset(command, 0, sizeof(struct apitest_command_s));
    command->argc = argc;

    for (itr = 0; itr < argc; itr++) {
      if ((command->argv[itr] = malloc(strlen(argv[itr]) + 1)) == NULL) {
        APITEST_FREE_CMD(command, itr);
        return -1;
      }

      memset(command->argv[itr], '\0', (strlen(argv[itr]) + 1));
      strncpy((command->argv[itr]), argv[itr], strlen(argv[itr]));
    }

    if (!taskrunning) {
      osThreadAttr_t attr = {0};

      attr.name = "apitest";
      attr.stack_size = APITEST_TASK_STACKSIZE;
      attr.priority = APITEST_TASK_PRI;

      osThreadId_t tid = osThreadNew(apitest_task, NULL, &attr);

      if (!tid) {
        APITEST_DBG_PRINT("osThreadNew failed\n");
        APITEST_FREE_CMD(command, command->argc);
        return -1;
      }
    }

    if (osOK != osMessageQueuePut(cmdq, (const void *)&command, 0, osWaitForever)) {
      APITEST_DBG_PRINT("osMessageQueuePut to apitest_task Failed!!\n");
      APITEST_FREE_CMD(command, command->argc);
    }
  }

  return 0;
}
