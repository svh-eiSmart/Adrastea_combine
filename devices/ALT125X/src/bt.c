/*  ---------------------------------------------------------------------------

        (c) copyright 2019 Altair Semiconductor, Ltd. All rights reserved.

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
#if defined(__GNUC__)
#include "bt.h"
#include <unwind.h>

struct bt_arg {
  int depth;
  int max_depth;
  _Unwind_Word last_ip;
};

_Unwind_Reason_Code bt_fn(_Unwind_Context *ctx, void *p) {
  struct bt_arg *arg = (struct bt_arg *)p;

  _Unwind_Word ip = _Unwind_GetIP(ctx);

  if (arg->depth > 0) {
    printf("bt#%d: pc at 0x%08x\r\n", arg->depth, (unsigned int)ip);
  }

  if (arg->last_ip == ip || arg->depth >= arg->max_depth) {
    printf("End of backtrace\r\n");
    return _URC_END_OF_STACK;
  }

  arg->last_ip = ip;
  arg->depth++;
  return _URC_NO_REASON;
}

void dump_backtrace() {
  struct bt_arg arg;
  memset(&arg, 0, sizeof(struct bt_arg));
  arg.max_depth = BACKTRACE_SIZE;
  _Unwind_Backtrace(&bt_fn, &arg);
}
#endif
