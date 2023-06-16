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
#if (configUSE_ALT_SLEEP == 1)
#include "DRV_GPIOCTL.h"
#include "DRV_GPIO_WAKEUP.h"
#include "DRV_PM.h"
#include "DRV_SLEEP.h"
#endif

#define MCU_IO_PAR_ID (1)
#define IO_CFG_IE_Msk GPM_IO_CFG_GPIO0_IO_CFG_IE_GPIO0_Msk
#define IO_CFG_IS_Msk GPM_IO_CFG_GPIO0_IO_CFG_IS_GPIO0_Msk
#define IO_CFG_SR_Msk GPM_IO_CFG_GPIO0_IO_CFG_SR_GPIO0_Msk
#define IO_CFG_PE_Msk GPM_IO_CFG_GPIO0_IO_CFG_PE_GPIO0_Msk
#define IO_CFG_PS_Msk GPM_IO_CFG_GPIO0_IO_CFG_PS_GPIO0_Msk
#define IO_CFG_DS0_Msk GPM_IO_CFG_GPIO0_IO_CFG_DS0_GPIO0_Msk
#define IO_CFG_DS1_Msk GPM_IO_CFG_GPIO0_IO_CFG_DS1_GPIO0_Msk

#define IO_CHECK_PHY_PIN_NUMBER(p)                         \
  if (((p) < MCU_PIN_ID_START || (p) >= MCU_PIN_ID_NUM)) { \
    return DRV_IO_ERROR_PARAM;                             \
  }

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

/* 1 = enable as input, 0 = disable input */
DRV_IO_Status DRV_IO_SetInputEnable(MCU_PinId pin_id, int32_t enable) {
  IOSEL_RegisterOffset reg_offset;
  uint32_t base_address, reg = 0;

  // gpio 61 & 62 are always input enabled
  if ((pin_id == 61) || (pin_id == 62)) {
    return DRV_IO_OK;
  }

  reg_offset = DRV_IOSEL_GetRegisterOffset(pin_id);
  if (pin_id >= 1 && pin_id <= 60) {
    reg = REGISTER(GPM_IO_CFG_BASE + reg_offset);
    base_address = GPM_IO_CFG_BASE;
  } else if (pin_id >= 65 && pin_id <= 78) { /*FLASH0_CS_N0 ... FLASH0_CS_N2*/
    reg = REGISTER(PMP_IO_CFG_BASE + reg_offset);
    base_address = PMP_IO_CFG_BASE;
  } else
    return DRV_IO_ERROR_PARAM;

  if (enable) {
    REGISTER(base_address + reg_offset) = (reg | IO_CFG_IE_Msk);
  } else {
    REGISTER(base_address + reg_offset) = (reg & ~(IO_CFG_IE_Msk));
  }

  return DRV_IO_OK;
}

#if (configUSE_ALT_SLEEP == 1)
typedef struct DRV_IO_ParkingTable {
  uint8_t io_pin;
  uint8_t io_init_config;
  uint8_t direction_input;
  uint8_t isolated;
  uint8_t output_value;
  uint8_t soc_option;
  uint8_t skip;
  uint8_t user_mark;
} DRV_IO_ParkingTable;

static int32_t io_sleep_initialized = 0;
DRV_IO_ParkingTable io_parking_table[IOSEL_CFG_TB_SIZE];

static int32_t DRV_IO_Isolate(MCU_PinId pin_id) {
  IOSEL_RegisterOffset reg_offset;
  uint32_t get_val;
  int32_t res = 0;

  IO_CHECK_PHY_PIN_NUMBER(pin_id);
  if (io_parking_table[pin_id - 1].isolated == 1) {
    return (-1);
  }

  /* copy configuration register for deisolation */
  reg_offset = DRV_IOSEL_GetRegisterOffset(pin_id);
  io_parking_table[pin_id - 1].io_init_config = REGISTER(GPM_IO_CFG_BASE + reg_offset);

  /* save direction for deisolation */
  if (DRV_GPIOCTL_GetDirection(pin_id) == 0) {  // input
    io_parking_table[pin_id - 1].direction_input = 1;
  } else {  // output
    io_parking_table[pin_id - 1].direction_input = 0;
    if (DRV_GPIOCTL_GetValue(pin_id))
      io_parking_table[pin_id - 1].output_value = 1;
    else
      io_parking_table[pin_id - 1].output_value = 0;
  }

  /*remember soc option and set as GPIO*/
  if (DRV_IO_GetMux(pin_id, &get_val) != DRV_IO_OK) {
    return (-1);
  }
  io_parking_table[pin_id - 1].soc_option = (uint8_t)get_val;

  /* Set to input */
  DRV_GPIOCTL_SetDirectionIn(pin_id);

  /* Set to GPIO. function 4 is 'GPIO' function */
  if (DRV_IO_SetMux(pin_id, 4) != DRV_IO_OK) {
    res = -2;
  }

  /* Input disable */
  if (DRV_IO_SetInputEnable(pin_id, 0) != DRV_IO_OK) {
    res = -3;
  }

  /* set pull to none */
  if (DRV_IO_SetPull(pin_id, IO_PULL_NONE) != DRV_IO_OK) {
    res = -4;
  }

  /* remember that gpio was isolated */
  io_parking_table[pin_id - 1].isolated = 1;

  return res;
}

static int32_t DRV_IO_DeIsolate(MCU_PinId pin_id) {
  uint32_t base_address;
  IOSEL_RegisterOffset reg_offset;

  /* check that gpio num is correct and choose pmp or gpm gpio's */
  if (pin_id >= 1 && pin_id <= 62) {
    base_address = GPM_IO_CFG_BASE;
  } else if (pin_id >= 65 && pin_id < IOSEL_CFG_TB_SIZE) {
    base_address = PMP_IO_CFG_BASE;
  } else
    return -1;

  /* remember that gpio was deisolated */
  if (io_parking_table[pin_id - 1].isolated) {
    io_parking_table[pin_id - 1].isolated = 0;

    /* recover previous configuration */
    reg_offset = DRV_IOSEL_GetRegisterOffset(pin_id);
    REGISTER(base_address + reg_offset) = io_parking_table[pin_id - 1].io_init_config;

    /* recover direction*/
    if (io_parking_table[pin_id - 1].direction_input) {
      DRV_GPIOCTL_SetDirectionIn(pin_id);
    } else {
      DRV_GPIOCTL_SetDirectionOut(pin_id);
      DRV_GPIOCTL_SetValue(pin_id, io_parking_table[pin_id - 1].output_value);
    }
    /*recover soc option*/
    if (DRV_IO_SetMux(pin_id, io_parking_table[pin_id - 1].soc_option) != DRV_IO_OK) return -1;
  }
  return 0;
}

static int32_t DRV_IO_PARK_MarkForPark(MCU_PinId pin_id, uint8_t user_mark) {
  IO_CHECK_PHY_PIN_NUMBER(pin_id);
  io_parking_table[pin_id - 1].skip = 0;
  io_parking_table[pin_id - 1].user_mark = user_mark;

  return 0;
}

static int32_t DRV_IO_PARK_MarkForSkip(MCU_PinId pin_id, uint8_t user_mark) {
  IO_CHECK_PHY_PIN_NUMBER(pin_id);
  io_parking_table[pin_id - 1].skip = 1;
  io_parking_table[pin_id - 1].user_mark = user_mark;

  return 0;
}

/* Initialize parking array to skip = 1, this action will
 * cause in skipping(not parking) GPIOs by default*/
static void DRV_IO_PARK_InitParkTable(void) {
  int i;

  for (i = 1; i < IOSEL_CFG_TB_SIZE; i++) {
    if (io_parking_table[i - 1].user_mark == 0) DRV_IO_PARK_MarkForSkip(i, 0);
  }
}

/* Read and return parking skip value*/
static int32_t drv_io_get_skip(MCU_PinId pin_id) {
  IO_CHECK_PHY_PIN_NUMBER(pin_id);
  return io_parking_table[pin_id - 1].skip;
}

void DRV_IO_SLEEP_PickIOs(void) {
  uint32_t index, iomux;

  DRV_IO_PARK_InitParkTable();

  for (index = 1; index < IOSEL_CFG_TB_SIZE; index++) {
    if (DRV_IO_GetMux(index, &iomux) != DRV_IO_OK) continue;

    if (index == 61 || index == 62)  // 61, 62 are not real IO
      continue;

    /* candidate parking GPIO pins have to meet below conditions:
         (io_par == 0x1)  //for MCU
      && (io_mux == 0x4)  //for GPIO (default value)
      && (gpio_direction == input)
      && (gpio is not using for wakeup) */
    if ((DRV_IO_ValidatePartition(index) == DRV_IO_OK) && (iomux == 0x4) &&
        (DRV_GPIOCTL_GetDirection(index) == 0) && (DRV_GPIO_WAKEUP_ValidateUsingPin(index) == 0)) {
      if (io_parking_table[index - 1].user_mark == 0) DRV_IO_PARK_MarkForPark(index, 0);
    }
  }
}

int32_t DRV_IO_PreSleepProcess(void) {
  int i;

  DRV_IO_SLEEP_PickIOs();

  for (i = 1; i < IOSEL_CFG_TB_SIZE; i++) {
    if (!drv_io_get_skip(i)) {
      // printf("\n\risolate # %d", i);
      if (DRV_IO_Isolate(i) < 0) {
        printf("Can't isolate IO PIN # %d ", i);
        return (-1);
      }
    }
  }
  return 0;
}

int32_t DRV_IO_PostSleepProcess(void) {
  int i;

  for (i = 1; i < IOSEL_CFG_TB_SIZE; i++) {
    if (!drv_io_get_skip(i)) {
      if (DRV_IO_DeIsolate(i) < 0) {
        printf("Can't deisolate IO PIN # %d ", i);
        return (-1);
      }
    }
  }
  return 0;
}

int32_t DRV_IO_SLEEP_Initialize(void) {
  if (io_sleep_initialized) return 0;

  DRV_IO_PARK_InitParkTable();
  io_sleep_initialized = 1;

  return 0;
}

/*print the linked list (for debug purposes only)*/
void DRV_IO_DumpSleepList(void) {
  int i;

  printf(" IO Sleep List\n  PINs:");
  for (i = 1; i < IOSEL_CFG_TB_SIZE; i++) {
    if (!drv_io_get_skip(i)) printf(" %d,", i);
  }
  printf("\n\n");
}

/* add new member to the sleep list */
DRV_IO_Status DRV_IO_AddSleepPin(MCU_PinId pin_id) {
  IO_CHECK_PHY_PIN_NUMBER(pin_id);

  /* GPIO IO 61 & 61 are not real IO */
  if ((pin_id == 61) || (pin_id == 62)) return DRV_IO_ERROR_PARAM;

  if (DRV_IO_PARK_MarkForPark(pin_id, 1) != 0) return DRV_IO_ERROR_PARAM;

  return DRV_IO_OK;
}

/* clear/remove member of the sleep list */
DRV_IO_Status DRV_IO_DelSleepPin(MCU_PinId pin_id, uint8_t action) {
  IO_CHECK_PHY_PIN_NUMBER(pin_id);

  /* GPIO IO 61 & 61 are not real IO */
  if ((pin_id == 61) || (pin_id == 62)) return DRV_IO_ERROR_PARAM;

  if (!action) {  // clear
    if (DRV_IO_PARK_MarkForSkip(pin_id, 0) != 0) return DRV_IO_ERROR_PARAM;
  } else {  // remove
    if (DRV_IO_PARK_MarkForSkip(pin_id, 1) != 0) return DRV_IO_ERROR_PARAM;
  }

  return DRV_IO_OK;
}

#endif
