
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
#include "FreeRTOS.h"
#include <stdio.h>
#include <string.h>
#include "sfplogger.h"

char* sfpdb_get_next_entry(spflogdbentry_t* sfpdbp, unsigned int entry_size) {
  char *entryp, *psearchroom;
  unsigned int lengthcleared = 0, roomforentry = 0;

  entryp = sfpdbp->sfpdbp + sfpdbp->next_entry_delta;

  // Check for end of buffer. If yes and no room, fill with "empty filler" and set next entry to
  // start of buffer
  if ((sfpdbp->next_entry_delta + entry_size) >
      (sfpdbp->buffer_size))  // leave room for empty filler
  {
    if ((sfpdbp->buffer_size - sfpdbp->next_entry_delta) >= SFPLOG_EMPTY_FILLER_MIN_SIZE) {
      *entryp = SFP_MSB_EMPTY_FILLER_TYPE |
                ((char)(((sfpdbp->buffer_size - sfpdbp->next_entry_delta) >> 8) &
                        (~SFP_MSB_EMPTY_FILLER_TYPE_MASK)));
      *(entryp + 1) = (char)((sfpdbp->buffer_size - sfpdbp->next_entry_delta));
    }
    entryp = sfpdbp->sfpdbp;
    sfpdbp->next_entry_delta = 0;
  }

  sfpdbp->next_entry_delta += entry_size;

  // Check if entry is exactly the size of the one it is replacing.
  // In the case when entry with 2 bytes for empty filler is bigger, fill remaining with empty
  // filler
  psearchroom = entryp;

  do {
    if (((*psearchroom) & SFP_MSB_EMPTY_FILLER_TYPE_MASK) == SFP_MSB_EMPTY_FILLER_TYPE) {
      roomforentry =
          (((*psearchroom) & ~(char)SFP_MSB_EMPTY_FILLER_TYPE_MASK) * 256) + *(psearchroom + 1);
    }
    // First run - memory is zeroed, all entries are "SFP_TYPE"
    else if (((*psearchroom) & SFP_MSB_SFP_TYPE_MASK) == SFP_MSB_SFP_TYPE) {
      roomforentry = SFPLOG_ENTRY_SIZE;
    } else if (((*psearchroom) & SFP_MSB_EXTENDED_SFP_TYPE_MASK) == SFP_MSB_EXTENDED_SFP_TYPE) {
      roomforentry = SFPLOG_EXTENDED_ENTRY_SIZE;
    } else if (((*psearchroom) & SFP_MSB_STRING_MASK) == SFP_MSB_STRING_TYPE) {
      roomforentry = psearchroom[1] + SFPLOG_STRING_ENTRYHEADER_SIZE;
    } else if (((*psearchroom) & SFP_MSB_COMPLEX_STRING_MASK) == SFP_MSB_COMPLEX_STRING_TYPE) {
      roomforentry = psearchroom[1] + SFPLOG_COMPLEX_STRING_ENTRYHEADER_SIZE_PLUS_NULL_TERM;
    }

    lengthcleared += roomforentry;
    psearchroom += roomforentry;
  } while ((lengthcleared < (entry_size + SFPLOG_EMPTY_FILLER_MIN_SIZE)) &&
           (lengthcleared != entry_size));

  // If new entry size is different from new entry, fill remaining with "empty filler
  if (lengthcleared != entry_size) {
    char* emptyfiller = entryp + entry_size;
    *emptyfiller =
        SFP_MSB_EMPTY_FILLER_TYPE |
        ((char)(((lengthcleared - entry_size) >> 8) & (~SFP_MSB_EMPTY_FILLER_TYPE_MASK)));
    *(emptyfiller + 1) = (char)((lengthcleared - entry_size));
  }

  return entryp;
}

char* sfpdb_get_earliest_entry(spflogdbentry_t* sfpdbp) {
  char* entryp = sfpdbp->sfpdbp + sfpdbp->next_entry_delta;

  if ((*entryp) == 0)  // no filler just empty - start of buffer is the place to start print from
  {
    return sfpdbp->sfpdbp;
  }

  if (((*entryp) & SFP_MSB_EMPTY_FILLER_TYPE_MASK) ==
      SFP_MSB_EMPTY_FILLER_TYPE)  // Empty filler entry
  {
    int fillersize = (((*entryp) & (~SFP_MSB_EMPTY_FILLER_TYPE_MASK)) * 256) + (*(entryp + 1));

    if ((sfpdbp->next_entry_delta + fillersize) >= sfpdbp->buffer_size) {
      return sfpdbp->sfpdbp;
    }

    if (*(sfpdbp->sfpdbp + sfpdbp->next_entry_delta + fillersize) == 0) {
      return sfpdbp->sfpdbp;
    }
    return sfpdbp->sfpdbp + sfpdbp->next_entry_delta + fillersize;
  } else {
    if (sfpdbp->next_entry_delta >= sfpdbp->buffer_size) {
      return sfpdbp->sfpdbp;
    } else {
      return sfpdbp->sfpdbp + sfpdbp->next_entry_delta;
    }
  }
}
