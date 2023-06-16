################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include serial/subdir.mk
-include pwrmanager/src/subdir.mk
-include printf/subdir.mk
-include osal/Source/subdir.mk
-include miniconsole/subdir.mk
-include hifc/mi/subdir.mk
-include hifc/core/subdir.mk
-include example/subdir.mk
-include applib/subdir.mk
-include FreeRTOS/FreeRTOS/portable/MemMang/subdir.mk
-include FreeRTOS/FreeRTOS/portable/GCC/ARM_CM3/subdir.mk
-include FreeRTOS/FreeRTOS/subdir.mk
-include Driver/Source/Private/ALT1250/subdir.mk
-include Driver/Source/Private/subdir.mk
-include Driver/Source/subdir.mk
-include Chipset/Source/subdir.mk
-include CMSIS-FreeRTOS/Source/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := Gpio
BUILD_ARTIFACT_EXTENSION := elf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_SIZE += \
Gpio.siz \


# All Target
all:
	+@$(MAKE) --no-print-directory main-build && $(MAKE) --no-print-directory post-build

# Main-build Target
main-build: Gpio.elf secondary-outputs

# Tool invocations
Gpio.elf: $(OBJS) $(USER_OBJS) makefile objects.mk $(OPTIONAL_TOOL_DEPS)
	@echo 'Building target: $@'
	@echo 'Invoking: GNU ARM Cross C Linker'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -Og -ffunction-sections -fdata-sections -Wall -Wextra -Wno-unused-parameter -g -T "C:\Users\shashank.hegde\Desktop\Adrastea\MCU_02_02_10_00_31121\ALT125x\Chipset\Source\GCC\ALT1250_defs.ld" -T "C:\Users\shashank.hegde\Desktop\Adrastea\MCU_02_02_10_00_31121\ALT125x\Chipset\Source\GCC\ALT125x_flash.ld" -nostartfiles -Xlinker --gc-sections -Wl,-Map,"Gpio.map" --specs=nano.specs --specs=nosys.specs -Wl,--undefined=uxTopUsedPriority -o "Gpio.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

Gpio.siz: Gpio.elf makefile objects.mk $(OPTIONAL_TOOL_DEPS)
	@echo 'Invoking: GNU ARM Cross Print Size'
	arm-none-eabi-size --format=berkeley "Gpio.elf"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(SECONDARY_SIZE)$(ASM_DEPS)$(S_UPPER_DEPS)$(C_DEPS) Gpio.elf
	-@echo ' '

post-build:
	-@echo 'Create Flash Image'
	-"C:/Users/shashank.hegde/Desktop/Adrastea/MCU_02_02_10_00_31121/examples/ALT125X/Gpio/Eclipse/../../../../utils/genfw.sh" "Gpio.bin"
	-@echo ' '

secondary-outputs: $(SECONDARY_SIZE)

.PHONY: all clean dependents main-build post-build

-include ../makefile.targets
