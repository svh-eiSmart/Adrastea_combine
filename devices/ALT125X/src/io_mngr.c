/****************************************************************************
 *
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
#include "125X_mcu.h"
#include "mcu_exported.h"
#include "io_mngr.h"

extern iosel_cfg_t iosel_cfg_table[IOSEL_CFG_TB_SIZE];

eIoMngrRet io_mngr_iosel_set(eMcuPinIds pin_id, eIoMngrFunc io_func) {
  uint32_t iosel_cfg_reg = 0;
  uint8_t opt_val = 0;
  eIoMngrRet ret;

  if (io_mngr_iopar_validate(pin_id) == IOMNGR_IO_PAR_ERR) return IOMNGR_IO_PAR_ERR;

  iosel_cfg_reg = BASE_ADDRESS_MCU_IO_FUNC_SEL + iosel_cfg_table[pin_id].reg_offset;
  for (opt_val = 0; opt_val < IO_FUNC_OPT_SIZE; opt_val++) {
    if (iosel_cfg_table[pin_id].io_func_opt[opt_val] == io_func) break;
  }
  if (opt_val < IO_FUNC_OPT_SIZE) {
    REGISTER(iosel_cfg_reg) = opt_val;
    ret = IOMNGR_SUCCESS;
  } else {
    ret = IOMNGR_FAILED;
  }
  return ret;
}

eIoMngrFunc io_mngr_iosel_get(eMcuPinIds pin_id) {
  uint32_t iosel_cfg_reg = 0;
  uint8_t opt_val = 0;
  io_mngr_reg_ofs_t reg_ofs;

  if (pin_id >= IOSEL_CFG_TB_SIZE) return IO_FUNC_UNDEFINED;

  reg_ofs = iosel_cfg_table[pin_id].reg_offset;

  if (reg_ofs == IOMNGR_REG_OFS_UNDEFINED) return IO_FUNC_UNDEFINED;

  iosel_cfg_reg = BASE_ADDRESS_MCU_IO_FUNC_SEL + reg_ofs;
  opt_val = REGISTER(iosel_cfg_reg);

  if (opt_val >= IO_FUNC_OPT_SIZE) return IO_FUNC_UNDEFINED;

  return iosel_cfg_table[pin_id].io_func_opt[opt_val];
}

eIoMngrRet io_mngr_iopar_validate(eMcuPinIds pin_id) {
  uint32_t reg = 0;
  eIoMngrRet ret = IOMNGR_IO_PAR_ERR;
  io_mngr_reg_ofs_t reg_ofs;

  if (pin_id >= IOSEL_CFG_TB_SIZE) return ret;

  reg_ofs = iosel_cfg_table[pin_id].reg_offset;

  if (reg_ofs != IOMNGR_REG_OFS_UNDEFINED) {
    if (MCU_PIN_IS_GPM_DOMAIN(pin_id)) {
      reg = REGISTER(TOPREG(BASE_ADDRESS_GPM_IO_PAR) + reg_ofs);
    } else if (MCU_PIN_IS_PMP_DOMAIN(pin_id)) {
      reg = REGISTER(TOPREG(BASE_ADDRESS_PMP_IO_PAR) + reg_ofs);
    }
    if (reg == 1) ret = IOMNGR_SUCCESS;
  }
  return ret;
}

io_mngr_reg_ofs_t io_mngr_get_iosel_reg_ofs(eMcuPinIds pin_id) {
  if (pin_id >= IOSEL_CFG_TB_SIZE) return IOMNGR_REG_OFS_UNDEFINED;

  return iosel_cfg_table[pin_id].reg_offset;
}
