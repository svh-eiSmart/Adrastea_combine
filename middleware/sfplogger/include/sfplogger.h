/*  ---------------------------------------------------------------------------

        (c) copyright 2017 Altair Semiconductor, Ltd. All rights reserved.

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

#ifndef SFP_LOGGER_H
#define SFP_LOGGER_H
#include "FreeRTOS.h"  //Needed for definition of CONFIG_DEBUG_TIMER_SERVICE

// clang-format off
#define SFPLOGGER_MAX_NUM_DBS          2
#define SFPLOG_ENTRY_SIZE              20
#define SFPLOG_EXTENDED_ENTRY_SIZE     24
#define SFPLOG_STRING_DATA_MAX_SIZE    80
#define SFPLOG_STRING_INPUT_MAX_SIZE   256
#define SFPLOG_STRING_ENTRYHEADER_SIZE 6
#define SFPLOG_COMPLEX_STRING_ENTRYHEADER_SIZE 11
#define SFPLOG_COMPLEX_STRING_ENTRYHEADER_SIZE_PLUS_NULL_TERM 12
#define SFPLOG_EMPTY_FILLER_MIN_SIZE   2
#define SFPLOG_LINEBUFF_LEN            384  //Complex string entry is 256 bytes and dictionary extracted string of 128
#define SFPLOG_DICTLINE_LEN            256
#define SFPLOG_FILENAME_LINEBUFF_LEN   64
#define SFPLOG_INIT_DB_SIZE            500
#define SFPLOG_DB_SIZE	               1500
#define SFPLOG_RESIZE_MINIMAL_DB_SIZE  256

#define SFPLOG_COMMERCIAL_DEFAULT_SIZE 3500
#define SFPLOG_COMMERCIAL_AT_SIZE      7000
#define SFPLOG_COMMERCIAL_ECM_SIZE     4000

#define SFPLOG_MAIN_MEM_DB_SIZE        5000
#define SFP_DICTIONARY_LOCATION        "c:\\dictionary\\dictionary.txt"
#define SFP_LOCATOR_LOCATION           "c:\\dictionary\\locator.bin"
#define SFP_ALTER_LOCATOR_LOCATION     "b:\\locator.bin"
#define SFP_ALTER_DICTIONARY_LOCATION  "b:\\dictionary.txt"
#define SFP_CONF_FILE_LOC              "d:\\config\\sfplogger"


#define SFP_AT_DB_SIZE_KEY             "at_db_size"
#define SFP_DATA_DB_SIZE_KEY           "data_db_size"
#define SFP_INTERNAL_DATABASE          "sfplogger.internal_db.enable"
#define SFP_DB_COMMERCIAL              "admin.services.sfp_commercial"
#define SFP_STREAM_ENABLE              "sfplogger.stream.enable"
#define SFP_STREAM_TASK_PRIORITY       "sfplogger.stream.taskprio"
#define SFP_HIBERNATE_CLEAR_ENABLE     "sfplogger.hib_clr.enable"
#define SFP_TRUE                       "y"
#define SFP_FALSE                      "n"
#define SFPLOGGER_CONF_FILE_NAME       "sfplogger"
#define SFPLOGGER_CONF_OPTION_NAME     "sfplogger.mod"
#define SFPLOGGER_CONF_SIZE_PARAM      "size"
#define SFPLOGGER_CONF_PRINT_PARAM     "print"
#define SFPLOGGER_CONF_SEVERITY_PARAM  "severity"
#define SFPLOGGER_CONF_PARAM_SIZE      64
#define SFPLOGGER_CONF_VAL_SIZE        10

#define SFP_MSB_SFP_TYPE_MASK          0x80
#define SFP_MSB_SFP_TYPE               0x00
#define SFP_MSB_EXTENDED_SFP_TYPE_MASK 0xC0
#define SFP_MSB_EXTENDED_SFP_TYPE_ZERO 0xBF
#define SFP_MSB_EXTENDED_SFP_TYPE      0x80
#define SFP_MSB_STRING_MASK            0xE0
#define SFP_MSB_STRING_TYPE            0xC0
#define SFP_MSB_COMPLEX_STRING_MASK    0xF0
#define SFP_MSB_COMPLEX_STRING_TYPE    0xF0

#define SFP_MSB_EMPTY_FILLER_TYPE_MASK 0xF0
#define SFP_MSB_EMPTY_FILLER_TYPE      0xE0

#define SFP_MODULE_NAME_LEN            12
#define SFP_DBID_ILLEGAL               255

#define SFPLOGGER_MASK_SLEEP_ALT_READ  3
#define SFPLOGGER_MARK_SLEEP_FLAG      2
#define SFPLOGGER_MARK_ALTER_READ      1
// clang-format on

typedef struct {
  char *sfpdbp;
  unsigned int next_entry_delta;  // delta pointer from sfpdbp to the next free entry in the buffer
  unsigned int buffer_size;       // in Bytes
  char lock;
} spflogdbentry_t;

typedef struct {
  char dbid;
  char severity;
  char modulename[SFP_MODULE_NAME_LEN];
  char printtostd;
} spflogmoduleentry_t;

typedef enum {
  SFPLOG_TRACE = 0,
  SFPLOG_DBG_PRINT,
  SFPLOG_APP_ASSERT,
  SFPLOG_ASSERT_ISR,
  SFPLOG_POWER_MNG,
  SFPLOG_DBG_ERROR,
  SFPLOG_ALTCOM,
  SFPLOG_MAX_MODULE_ID
} SFP_log_module_id_t;

typedef enum {
  SFPLOG_CRITICAL,
  SFPLOG_ERROR,
  SFPLOG_WARNING,
  SFPLOG_NORMAL,
  SFPLOG_INFO,
  SFPLOG_DEBUG,
  SFPLOG_VERBOSE,
  SFPLOG_SEVERITY_NUM
} SFP_SEVERITY_t;

int sfp_log(int param1, int param2, int param3, unsigned int severitymoduleid,
            unsigned int line_num, void *pfilename);
int sfp_log_complex_str(const char *storestr, unsigned int severitymoduleid, unsigned int line_num,
                        void *pfilename);
int sfp_log_str(SFP_log_module_id_t moduleid, char severity, char *logstring);
int sfplogger_set_module_print_state(SFP_log_module_id_t moduleid, char printstate,
                                      int write_conf_to_file);
char sfplogger_get_module_print_state(SFP_log_module_id_t moduleid);
int sfplogger_set_module_severity(SFP_log_module_id_t moduleid, char severity, int savetofile);
char sfplogger_get_module_severity(SFP_log_module_id_t moduleid);
void sfplogger_print_buffer(char *pcWriteBuffer, size_t xWriteBufferLen, void *flush_func,
                            void *flush_arg, void *psbufferid);
void sfplogger_print_buffer_locked(char *pcWriteBuffer, size_t xWriteBufferLen, void *flush_func,
                                   void *flush_arg, void *psbufferid);
void sfplogger_dump_hex(char *pcWriteBuffer, size_t xWriteBufferLen, void *flush_func,
                        void *flush_arg);
void sfplogger_enable_stream(char enable);
void sfplogger_set_stream_task_priority(char priority);
void sfplogger_enable_hibernate_log_clear(char enable);
void sfplogger_enable_commercial_mode(char enable);
void sfplogger_enable_internal_log(char enable);
void sfplogger_print_registered_modules(char *pcWriteBuffer, size_t xWriteBufferLen,
                                        void *flush_func, void *flush_arg);
void sfplogger_clear_buffers(char *buffer_to_clear, char *pcWriteBuffer, size_t xWriteBufferLen,
                             void *flush_func, void *flush_arg);
int sfplogger_create_db(int size);
int sfplogger_resize_db(int moduleid, unsigned int size, int write_conf_to_file);
int sfplogger_save_buffer_in_db(unsigned int dbid, char *binaryBuff, unsigned int size);
int sfplogger_register_moduletodb(SFP_log_module_id_t moduleid, char dbid, char *modulename);
int sfp_log_string(const char *format, ...);
int sfp_log_formatted(SFP_log_module_id_t moduleid, char severity, const char *format, ...);
int sfp_log_init();
int sfp_log_init_after_fs();
char *sfpdb_get_next_entry(spflogdbentry_t *sfpdbp, unsigned int entry_size);
char *sfpdb_get_earliest_entry(spflogdbentry_t *sfpdbp);
int sfplogger_save_buffers_to_file(char *filename);
int sfplogger_get_default_db_id();
int sfplogger_get_at_db_id();
int sfplogger_get_ecm_db_id();
int sfplogger_get_is_commercial_mode();
void sfplogger_delete_main_memory_db();
int sfplocator_read_locator_file(char allocate);
int sfplocator_get_offset_in_dictionary(int line_number, int hashed_file_name);
void sfplogger_update_configuration_from_file();
void sfplogger_send_stream_to_output();
char sfplogger_get_db_id_by_module_id(char module_id);
int sfplogger_get_module_names(char *buffer, int buffersize);
void sfplogger_get_module_severity_and_name(int moduleId, char **mduleName, char **severity);
int sfplogger_get_severity_by_name(char **mduleName, char **severity);
int sfplogger_set_severity_by_name(char **mduleName, char severity);
int sfplogger_send_AppLog();

// clang-format off
#define SFP_LOG_VERBOSE(moduleid, str)  sfp_log(0, 0, 0, (SFPLOG_VERBOSE << 28)  | (moduleid & 0XFFFFFFF), __LINE__, __FILE__)
#define SFP_LOG_DEBUG(moduleid, str)    sfp_log(0, 0, 0, (SFPLOG_DEBUG << 28)    | (moduleid & 0XFFFFFFF), __LINE__, __FILE__)
#define SFP_LOG_INFO(moduleid, str)     sfp_log(0, 0, 0, (SFPLOG_INFO << 28)     | (moduleid & 0XFFFFFFF), __LINE__, __FILE__)
#define SFP_LOG_NORMAL(moduleid, str)   sfp_log(0, 0, 0, (SFPLOG_NORMAL << 28)   | (moduleid & 0XFFFFFFF), __LINE__, __FILE__)
#define SFP_LOG_WARNING(moduleid, str)  sfp_log(0, 0, 0, (SFPLOG_WARNING << 28)  | (moduleid & 0XFFFFFFF), __LINE__, __FILE__)
#define SFP_LOG_ERROR(moduleid, str)    sfp_log(0, 0, 0, (SFPLOG_ERROR << 28)    | (moduleid & 0XFFFFFFF), __LINE__, __FILE__)
#define SFP_LOG_CRITICAL(moduleid, str) sfp_log(0, 0, 0, (SFPLOG_CRITICAL << 28) | (moduleid & 0XFFFFFFF), __LINE__, __FILE__)

#define SFP_LOG_3_VERBOSE(moduleid, str, param1, param2, param3)   sfp_log(param1, param2, param3, (SFPLOG_VERBOSE << 28)  | (moduleid & 0XFFFFFFF), __LINE__, __FILE__)
#define SFP_LOG_3_DEBUG(moduleid, str, param1, param2, param3)     sfp_log(param1, param2, param3, (SFPLOG_DEBUG << 28)    | (moduleid & 0XFFFFFFF), __LINE__, __FILE__)
#define SFP_LOG_3_INFO(moduleid, str, param1, param2, param3)      sfp_log(param1, param2, param3, (SFPLOG_INFO << 28)     | (moduleid & 0XFFFFFFF), __LINE__, __FILE__)
#define SFP_LOG_3_NORMAL(moduleid, str, param1, param2, param3)    sfp_log(param1, param2, param3, (SFPLOG_NORMAL << 28)   | (moduleid & 0XFFFFFFF), __LINE__, __FILE__)
#define SFP_LOG_3_WARNING(moduleid, str, param1, param2, param3)   sfp_log(param1, param2, param3, (SFPLOG_WARNING << 28)  | (moduleid & 0XFFFFFFF), __LINE__, __FILE__)
#define SFP_LOG_3_ERROR(moduleid, str, param1, param2, param3)     sfp_log(param1, param2, param3, (SFPLOG_ERROR << 28)    | (moduleid & 0XFFFFFFF), __LINE__, __FILE__)
#define SFP_LOG_3_CRITICAL(moduleid, str, param1, param2, param3)  sfp_log(param1, param2, param3, (SFPLOG_CRITICAL << 28) | (moduleid & 0XFFFFFFF), __LINE__, __FILE__)
#define SFP_LOG_3(moduleid, severity, str, param1, param2, param3) sfp_log(param1, param2, param3, (severity << 28)        | (moduleid & 0XFFFFFFF), __LINE__, __FILE__)
// clang-format on

#define CONFIG_SFP_TIMESTAMP
#ifdef CONFIG_SFP_TIMESTAMP
#define SFP_LOG_DEBUG_TIMESTAMP(str, param1) SFP_LOG_3_DEBUG(SFPLOG_TIMESTAMP, str, param1, 0, 0)
#else
#define SFP_LOG_DEBUG_TIMESTAMP(str, param1)
#endif

// clang-format off
#define SFP_STR_LOG_VERBOSE(moduleid, str)   sfp_log_str(moduleid, SFPLOG_VERBOSE,  str)
#define SFP_STR_LOG_DEBUG(moduleid, str)     sfp_log_str(moduleid, SFPLOG_DEBUG,    str)
#define SFP_STR_LOG_INFO(moduleid, str)      sfp_log_str(moduleid, SFPLOG_INFO,     str)
#define SFP_STR_LOG_NORMAL(moduleid, str)    sfp_log_str(moduleid, SFPLOG_NORMAL,   str)
#define SFP_STR_LOG_WARNING(moduleid, str)   sfp_log_str(moduleid, SFPLOG_WARNING,  str)
#define SFP_STR_LOG_ERROR(moduleid, str)     sfp_log_str(moduleid, SFPLOG_ERROR,    str)
#define SFP_STR_LOG_CRITICAL(moduleid, str)  sfp_log_str(moduleid, SFPLOG_CRITICAL, str)
// clang-format on

#define SFP_LOG_STR_COMPLEX(moduleid, severity, str, storestr) \
  sfp_log_complex_str(storestr, (severity << 28) | (moduleid & 0XFFFFFFF), __LINE__, __FILE__)

#endif
