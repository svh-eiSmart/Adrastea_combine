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
#include DEVICE_HEADER
#include "DRV_COMMON.h"
#include "DRV_IO.h"
#include "DRV_IOSEL.h"

#define MCU_IO_PAR_ID (1)
#define IO_CFG_IE_Msk GPM_IO_CFG_GPIO0_IO_CFG_IE_GPIO0_Msk
#define IO_CFG_IS_Msk GPM_IO_CFG_GPIO0_IO_CFG_IS_GPIO0_Msk
#define IO_CFG_SR_Msk GPM_IO_CFG_GPIO0_IO_CFG_SR_GPIO0_Msk
#define IO_CFG_PE_Msk GPM_IO_CFG_GPIO0_IO_CFG_PE_GPIO0_Msk
#define IO_CFG_PS_Msk GPM_IO_CFG_GPIO0_IO_CFG_PS_GPIO0_Msk
#define IO_CFG_DS0_Msk GPM_IO_CFG_GPIO0_IO_CFG_DS0_GPIO0_Msk
#define IO_CFG_DS1_Msk GPM_IO_CFG_GPIO0_IO_CFG_DS1_GPIO0_Msk

DRV_IO_Status DRV_IO_SetPull(MCU_PinId pin_id, IO_Pull pull_mode) {
  IOSEL_RegisterOffset reg_offset;
  uint32_t io_reg_addr = 0;
  uint32_t reg_content = 0;

  if (pin_id >= IOSEL_CFG_TB_SIZE) return DRV_IO_ERROR_PARAM;

  if (DRV_IO_ValidatePartition(pin_id) != DRV_IO_OK) return DRV_IO_ERROR_PARTITION;

  reg_offset = DRV_IOSEL_GetRegisterOffset(pin_id);
  if (reg_offset == IOSEL_REGISTER_OFFSET_UNDEFINED) return DRV_IO_ERROR_PARAM;

  if (MCU_PIN_IS_VIRTUAL_PIN(pin_id)) {
    return DRV_IO_ERROR_UNSUPPORTED;
  } else if (MCU_PIN_IS_GPM_DOMAIN(pin_id)) {
    io_reg_addr = GPM_IO_CFG_BASE + reg_offset;

  } else if (MCU_PIN_IS_PMP_DOMAIN(pin_id)) {
    io_reg_addr = PMP_IO_CFG_BASE + reg_offset;
  } else {
    return DRV_IO_ERROR_PARAM;
  }
  reg_content = REGISTER(io_reg_addr);
  switch (pull_mode) {
    case IO_PULL_NONE:
      reg_content &= ~IO_CFG_PE_Msk;
      break;
    case IO_PULL_UP:
      reg_content |= (IO_CFG_PE_Msk | IO_CFG_PS_Msk);
      break;
    case IO_PULL_DOWN:
      reg_content |= IO_CFG_PE_Msk;
      reg_content &= ~IO_CFG_PS_Msk;
      break;
    default:
      return DRV_IO_ERROR_PARAM;
      break;
  }
  REGISTER(io_reg_addr) = reg_content;
  return DRV_IO_OK;
}

DRV_IO_Status DRV_IO_GetPull(MCU_PinId pin_id, IO_Pull *pull_mode) {
  IOSEL_RegisterOffset reg_offset;
  uint32_t io_reg_addr = 0;
  uint32_t reg_content = 0;

  if (pin_id >= IOSEL_CFG_TB_SIZE) return DRV_IO_ERROR_PARAM;

  if (DRV_IO_ValidatePartition(pin_id) != DRV_IO_OK) return DRV_IO_ERROR_PARTITION;

  reg_offset = DRV_IOSEL_GetRegisterOffset(pin_id);
  if (reg_offset == IOSEL_REGISTER_OFFSET_UNDEFINED) return DRV_IO_ERROR_PARAM;

  if (MCU_PIN_IS_VIRTUAL_PIN(pin_id)) {
    return DRV_IO_ERROR_UNSUPPORTED;
  } else if (MCU_PIN_IS_GPM_DOMAIN(pin_id)) {
    io_reg_addr = GPM_IO_CFG_BASE + reg_offset;

  } else if (MCU_PIN_IS_PMP_DOMAIN(pin_id)) {
    io_reg_addr = PMP_IO_CFG_BASE + reg_offset;
  } else {
    return DRV_IO_ERROR_PARAM;
  }
  reg_content = REGISTER(io_reg_addr);
  if (reg_content & IO_CFG_PE_Msk) {
    if (reg_content & IO_CFG_PS_Msk)
      *pull_mode = IO_PULL_UP;
    else
      *pull_mode = IO_PULL_DOWN;
  } else {
    *pull_mode = IO_PULL_NONE;
  }
  return DRV_IO_OK;
}

DRV_IO_Status DRV_IO_SetDriveStrength(MCU_PinId pin_id, IO_DriveStrength ds) {
  IOSEL_RegisterOffset reg_offset;
  uint32_t io_reg_addr = 0;
  uint32_t reg_content = 0;

  if (pin_id >= IOSEL_CFG_TB_SIZE) return DRV_IO_ERROR_PARAM;

  if (DRV_IO_ValidatePartition(pin_id) != DRV_IO_OK) return DRV_IO_ERROR_PARTITION;

  reg_offset = DRV_IOSEL_GetRegisterOffset(pin_id);
  if (reg_offset == IOSEL_REGISTER_OFFSET_UNDEFINED) return DRV_IO_ERROR_PARAM;

  if (MCU_PIN_IS_VIRTUAL_PIN(pin_id)) {
    return DRV_IO_ERROR_UNSUPPORTED;
  } else if (MCU_PIN_IS_GPM_DOMAIN(pin_id)) {
    io_reg_addr = GPM_IO_CFG_BASE + reg_offset;
  } else if (MCU_PIN_IS_PMP_DOMAIN(pin_id)) {
    io_reg_addr = PMP_IO_CFG_BASE + reg_offset;
  } else {
    return DRV_IO_ERROR_PARAM;
  }

  reg_content = REGISTER(io_reg_addr);
  switch (ds) {
    case IO_DRIVE_STRENGTH_2MA:
      reg_content &= ~IO_CFG_DS0_Msk; /*DS0 = 0*/
      reg_content &= ~IO_CFG_DS1_Msk; /*DS1 = 0*/
      break;
    case IO_DRIVE_STRENGTH_4MA:
      reg_content &= ~IO_CFG_DS0_Msk; /*DS0 = 0*/
      reg_content |= IO_CFG_DS1_Msk;  /*DS1 = 1*/
      break;
    case IO_DRIVE_STRENGTH_8MA:
      reg_content |= IO_CFG_DS0_Msk;  /*DS0 = 1*/
      reg_content &= ~IO_CFG_DS1_Msk; /*DS1 = 0*/
      break;
    case IO_DRIVE_STRENGTH_12MA:
      reg_content |= IO_CFG_DS0_Msk; /*DS0 = 1*/
      reg_content |= IO_CFG_DS1_Msk; /*DS1 = 1*/
      break;
    default:
      return DRV_IO_ERROR_PARAM;
      break;
  }
  REGISTER(io_reg_addr) = reg_content;
  return DRV_IO_OK;
}

DRV_IO_Status DRV_IO_GetDriveStrength(MCU_PinId pin_id, IO_DriveStrength *ds) {
  IOSEL_RegisterOffset reg_offset;
  uint32_t io_reg_addr = 0;
  uint32_t reg_content = 0;

  if (pin_id >= IOSEL_CFG_TB_SIZE) return DRV_IO_ERROR_PARAM;

  if (DRV_IO_ValidatePartition(pin_id) != DRV_IO_OK) return DRV_IO_ERROR_PARTITION;

  reg_offset = DRV_IOSEL_GetRegisterOffset(pin_id);
  if (reg_offset == IOSEL_REGISTER_OFFSET_UNDEFINED) return DRV_IO_ERROR_PARAM;

  if (MCU_PIN_IS_VIRTUAL_PIN(pin_id)) {
    return DRV_IO_ERROR_UNSUPPORTED;
  } else if (MCU_PIN_IS_GPM_DOMAIN(pin_id)) {
    io_reg_addr = GPM_IO_CFG_BASE + reg_offset;
  } else if (MCU_PIN_IS_PMP_DOMAIN(pin_id)) {
    io_reg_addr = PMP_IO_CFG_BASE + reg_offset;
  } else {
    return DRV_IO_ERROR_PARAM;
  }
  reg_content = REGISTER(io_reg_addr);

  if (!(reg_content & IO_CFG_DS0_Msk) && !(reg_content & IO_CFG_DS1_Msk)) {
    *ds = IO_DRIVE_STRENGTH_2MA;
  } else if (!(reg_content & IO_CFG_DS0_Msk) && (reg_content & IO_CFG_DS1_Msk)) {
    *ds = IO_DRIVE_STRENGTH_4MA;
  } else if ((reg_content & IO_CFG_DS0_Msk) && !(reg_content & IO_CFG_DS1_Msk)) {
    *ds = IO_DRIVE_STRENGTH_8MA;
  } else {
    *ds = IO_DRIVE_STRENGTH_12MA;
  }
  return DRV_IO_OK;
}

DRV_IO_Status DRV_IO_SetSlewRate(MCU_PinId pin_id, IO_SlewRate slew_rate) {
  IOSEL_RegisterOffset reg_offset;
  uint32_t io_reg_addr = 0;
  uint32_t reg_content = 0;

  if (pin_id >= IOSEL_CFG_TB_SIZE) return DRV_IO_ERROR_PARAM;

  if (DRV_IO_ValidatePartition(pin_id) != DRV_IO_OK) return DRV_IO_ERROR_PARTITION;

  reg_offset = DRV_IOSEL_GetRegisterOffset(pin_id);
  if (reg_offset == IOSEL_REGISTER_OFFSET_UNDEFINED) return DRV_IO_ERROR_PARAM;

  if (MCU_PIN_IS_VIRTUAL_PIN(pin_id)) {
    return DRV_IO_ERROR_UNSUPPORTED;
  } else if (MCU_PIN_IS_GPM_DOMAIN(pin_id)) {
    io_reg_addr = GPM_IO_CFG_BASE + reg_offset;
  } else if (MCU_PIN_IS_PMP_DOMAIN(pin_id)) {
    io_reg_addr = PMP_IO_CFG_BASE + reg_offset;
  } else {
    return DRV_IO_ERROR_PARAM;
  }
  reg_content = REGISTER(io_reg_addr);

  switch (slew_rate) {
    case IO_SLEW_RATE_SLOW:
      reg_content |= IO_CFG_SR_Msk;
      break;
    case IO_SLEW_RATE_FAST:
      reg_content &= ~IO_CFG_SR_Msk;
      break;
    default:
      return DRV_IO_ERROR_PARAM;
      break;
  }
  return DRV_IO_OK;
}

DRV_IO_Status DRV_IO_GetSlewRate(MCU_PinId pin_id, IO_SlewRate *slew_rate) {
  IOSEL_RegisterOffset reg_offset;
  uint32_t io_reg_addr = 0;
  uint32_t reg_content = 0;

  if (pin_id >= IOSEL_CFG_TB_SIZE) return DRV_IO_ERROR_PARAM;

  if (DRV_IO_ValidatePartition(pin_id) != DRV_IO_OK) return DRV_IO_ERROR_PARTITION;

  reg_offset = DRV_IOSEL_GetRegisterOffset(pin_id);
  if (reg_offset == IOSEL_REGISTER_OFFSET_UNDEFINED) return DRV_IO_ERROR_PARAM;

  if (MCU_PIN_IS_VIRTUAL_PIN(pin_id)) {
    return DRV_IO_ERROR_UNSUPPORTED;
  } else if (MCU_PIN_IS_GPM_DOMAIN(pin_id)) {
    io_reg_addr = GPM_IO_CFG_BASE + reg_offset;
  } else if (MCU_PIN_IS_PMP_DOMAIN(pin_id)) {
    io_reg_addr = PMP_IO_CFG_BASE + reg_offset;
  } else {
    return DRV_IO_ERROR_PARAM;
  }
  reg_content = REGISTER(io_reg_addr);

  if (reg_content & IO_CFG_SR_Msk)
    *slew_rate = IO_SLEW_RATE_SLOW;
  else
    *slew_rate = IO_SLEW_RATE_FAST;

  return DRV_IO_OK;
}

DRV_IO_Status DRV_IO_SetMux(MCU_PinId pin_id, uint32_t mux_value) {
  IOSEL_RegisterOffset reg_offset;

  if (pin_id >= IOSEL_CFG_TB_SIZE) return DRV_IO_ERROR_PARAM;

  if (DRV_IO_ValidatePartition(pin_id) != DRV_IO_OK) return DRV_IO_ERROR_PARTITION;

  reg_offset = DRV_IOSEL_GetRegisterOffset(pin_id);
  if (reg_offset == IOSEL_REGISTER_OFFSET_UNDEFINED) return DRV_IO_ERROR_PARAM;

  REGISTER(IO_FUNC_SEL_BASE + reg_offset) = mux_value;
  return DRV_IO_OK;
}

DRV_IO_Status DRV_IO_GetMux(MCU_PinId pin_id, uint32_t *mux_value) {
  IOSEL_RegisterOffset reg_offset;

  if (pin_id >= IOSEL_CFG_TB_SIZE) return DRV_IO_ERROR_PARAM;

  if (DRV_IO_ValidatePartition(pin_id) != DRV_IO_OK) return DRV_IO_ERROR_PARTITION;

  reg_offset = DRV_IOSEL_GetRegisterOffset(pin_id);
  if (reg_offset == IOSEL_REGISTER_OFFSET_UNDEFINED) return DRV_IO_ERROR_PARAM;

  *mux_value = REGISTER(IO_FUNC_SEL_BASE + reg_offset);

  return DRV_IO_OK;
}

DRV_IO_Status DRV_IO_ValidatePartition(MCU_PinId pin_id) {
  uint32_t reg = 0;
  DRV_IO_Status ret = DRV_IO_ERROR_PARTITION;
  IOSEL_RegisterOffset reg_offset;

  if (pin_id >= IOSEL_CFG_TB_SIZE) return DRV_IO_ERROR_PARAM;

  reg_offset = DRV_IOSEL_GetRegisterOffset(pin_id);

  if (reg_offset != IOSEL_REGISTER_OFFSET_UNDEFINED) {
    if (MCU_PIN_IS_GPM_DOMAIN(pin_id)) {
      reg = REGISTER(GPM_IO_PAR_BASE + reg_offset);
    } else if (MCU_PIN_IS_PMP_DOMAIN(pin_id)) {
      reg = REGISTER(PMP_IO_PAR_BASE + reg_offset);
    }
    if (reg == MCU_IO_PAR_ID) ret = DRV_IO_OK;
  }
  return ret;
}
