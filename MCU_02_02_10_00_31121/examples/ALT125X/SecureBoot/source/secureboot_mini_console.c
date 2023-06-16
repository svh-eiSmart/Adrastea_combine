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
#include <string.h>

/* CMSIS-RTOS V2 includes. */
#include "cmsis_os2.h"

/* DRV includes */
#include "DRV_SB_ENABLE.h" //for secure boot command
#include "DRV_OTP.h" // for otp command

void print_secureboot_usage(void) {
  printf("Usage:\n\r");
  printf("secureboot enable\n\r");
  printf("  secureboot enable - Enable MCU secure boot\n\r");
}

int validate_puk3_offset(uint32_t address) {  // Check if address is leagal
  if ((address >= 0x8000000) && (address < 0xc000000)) {
    return 0;  // flash
  }

  return -1;
}

int32_t do_secureboot(char *s) {
  uint32_t puk3_offset = 0;
  int32_t argc = 0, ret_val = 0;
  char *tok, *strgp;

  strgp = s;
  tok = strsep(&strgp, " ");
  if (!tok) {
    return (-1);
  }
  puk3_offset = (uint32_t)strtoul(tok, NULL, 16);
  argc++;

  puk3_offset = puk3_offset & 0xfffffff;
  if (validate_puk3_offset(puk3_offset) == 0) {
    if(DRV_SB_Enable_MCU(puk3_offset) == OTP_SB_ENABLE_OK)
      printf("Enabled MCU secureboot.\r\n");
    else
      printf("Failed to enable MCU secureboot.\r\n");
  } else {
    printf("Offset [%lx] is out of range.\r\n", puk3_offset);
    ret_val = -1;
  }

  return ret_val;
}

int32_t do_otp(char *s) {
  char command[20] = {0}, par2[20] = {0} , par3[20] = {0};
  int32_t argc = 0, ret_val = 0;
  char *tok, *strgp;
  DRV_OTP_STATUS status = OTP_OK;
  int32_t field_id, size_in_byte = 0, idx;
  char *buffer = NULL;

  strgp = s;
  tok = strsep(&strgp, " ");
  if (!tok) {
    return (-1);
  }
  strncpy(command, tok, sizeof(command) - 1);
  argc++;

  tok = strsep(&strgp, " ");
  if (tok) {
    strncpy(par2, tok, sizeof(par2) - 1);
    argc++;
  }
  field_id = atoi(par2);
  if(field_id >= OTP_NULL_LAST || field_id < 0) {
    return (-1);
  }
  
  tok = strsep(&strgp, " ");
  if (tok) {
    strncpy(par3, tok, sizeof(par3) - 1);
    argc++;
  }
  size_in_byte = atoi(par3);
  if(size_in_byte < 0) {
    return (-1);
  }

  switch (argc) {
    case 3:
      if (!strncmp("w", command, 2)) {
        buffer = malloc(size_in_byte);
	      if(!buffer){
          printf("Failed to memory allocation \r\n");
          return 0;
        }
        memset(buffer, 0, size_in_byte);
        //copy test data to buffer
        for (idx = 0; idx < size_in_byte; idx++) {
          buffer[idx] = idx % 256;
        }

        status = DRV_OTP_Set (field_id, (uint32_t*) buffer, size_in_byte);
        if (status == OTP_OK) {
          printf("success to write OTP id %ld and locked. \r\n", field_id);
        } else {
          printf("Failed to write OTP id %ld, error code %d \r\n", field_id, status);
        }
      } else if (!strncmp("r", command, 2)) {
        buffer = malloc(size_in_byte);
	      if(!buffer) {
          printf("Failed to memory allocation \r\n");
          return 0;
        }
		      
        memset(buffer, 0, size_in_byte);
        status = DRV_OTP_Get (field_id, (uint32_t*) buffer, size_in_byte);
        if (status == OTP_OK) {
          //print out the read value
          for (idx = 0; idx < size_in_byte; idx++) {
            printf("0x%02x ", buffer[idx]);
            if((idx+1)%32 == 0) { printf("\r\n"); }
          }
        } else {
          printf("Failed to read OTP id %ld, error code %d \r\n", field_id, status);
        }
      } else {
        printf("Wrong command format!\r\n");
      }
      printf("\r\n");
      if (buffer)
        free(buffer);

      break;
    default:
      printf("Wrong command format!\r\n");
      break;
  }

  return ret_val;
}
