################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/shashank.hegde/Desktop/Adrastea/MCU_02_02_10_00_31121/middleware/hifc/src/lib/core/hifc_api.c 

OBJS += \
./hifc/core/hifc_api.o 

C_DEPS += \
./hifc/core/hifc_api.d 


# Each subdirectory must supply rules for building sources it contributes
hifc/core/hifc_api.o: C:/Users/shashank.hegde/Desktop/Adrastea/MCU_02_02_10_00_31121/middleware/hifc/src/lib/core/hifc_api.c hifc/core/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GNU ARM Cross C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -Og -ffunction-sections -fdata-sections -Wall -Wextra -Wno-unused-parameter -g -DALT1250 -DDEVICE_HEADER=\"ALT1250.h\" -DCMSIS_device_header=\"ALT1250.h\" -I"C:/Users/shashank.hegde/Desktop/Adrastea/MCU_02_02_10_00_31121/examples/ALT125X/GpsConnector/Eclipse/../source" -I"C:/Users/shashank.hegde/Desktop/Adrastea/MCU_02_02_10_00_31121/examples/ALT125X/GpsConnector/Eclipse/../include" -I"C:\Users\shashank.hegde\Desktop\Adrastea\MCU_02_02_10_00_31121\ALT125x\Chipset\Include" -I"C:\Users\shashank.hegde\Desktop\Adrastea\MCU_02_02_10_00_31121\CMSIS\Core\Include" -I"C:\Users\shashank.hegde\Desktop\Adrastea\MCU_02_02_10_00_31121\CMSIS\RTOS2\FreeRTOS\Include" -I"C:\Users\shashank.hegde\Desktop\Adrastea\MCU_02_02_10_00_31121\FreeRTOS\lib\include" -I"C:\Users\shashank.hegde\Desktop\Adrastea\MCU_02_02_10_00_31121\FreeRTOS\lib\FreeRTOS\portable\GCC\ARM_CM3" -I"C:\Users\shashank.hegde\Desktop\Adrastea\MCU_02_02_10_00_31121\ALT125x\Driver\Include" -I"C:\Users\shashank.hegde\Desktop\Adrastea\MCU_02_02_10_00_31121\ALT125x\Driver\Include\ALT1250" -I"C:\Users\shashank.hegde\Desktop\Adrastea\MCU_02_02_10_00_31121\ALT125x\Driver\Source\Private" -I"C:\Users\shashank.hegde\Desktop\Adrastea\MCU_02_02_10_00_31121\ALT125x\Driver\Source\Private\ALT1250" -I"C:\Users\shashank.hegde\Desktop\Adrastea\MCU_02_02_10_00_31121\middleware\osal\Include" -I"C:\Users\shashank.hegde\Desktop\Adrastea\MCU_02_02_10_00_31121\middleware\hifc\src\lib\core" -I"C:\Users\shashank.hegde\Desktop\Adrastea\MCU_02_02_10_00_31121\middleware\hifc\src\lib\mi" -I"C:\Users\shashank.hegde\Desktop\Adrastea\MCU_02_02_10_00_31121\middleware\serial" -I"C:\Users\shashank.hegde\Desktop\Adrastea\MCU_02_02_10_00_31121\middleware\miniconsole" -I"C:\Users\shashank.hegde\Desktop\Adrastea\MCU_02_02_10_00_31121\middleware\printf" -I"C:\Users\shashank.hegde\Desktop\Adrastea\MCU_02_02_10_00_31121\applib" -I"C:\Users\shashank.hegde\Desktop\Adrastea\MCU_02_02_10_00_31121\middleware\altcomlib\include" -I"C:\Users\shashank.hegde\Desktop\Adrastea\MCU_02_02_10_00_31121\middleware\altcomlib\include\util" -I"C:\Users\shashank.hegde\Desktop\Adrastea\MCU_02_02_10_00_31121\middleware\altcomlib\include\opt" -I"C:\Users\shashank.hegde\Desktop\Adrastea\MCU_02_02_10_00_31121\middleware\altcomlib\include\gps" -I"C:\Users\shashank.hegde\Desktop\Adrastea\MCU_02_02_10_00_31121\middleware\altcomlib\altcom\include\builder" -I"C:\Users\shashank.hegde\Desktop\Adrastea\MCU_02_02_10_00_31121\middleware\altcomlib\altcom\include\evtdisp" -I"C:\Users\shashank.hegde\Desktop\Adrastea\MCU_02_02_10_00_31121\middleware\altcomlib\altcom\include\gw" -I"C:\Users\shashank.hegde\Desktop\Adrastea\MCU_02_02_10_00_31121\middleware\altcomlib\altcom\include\api\common" -I"C:\Users\shashank.hegde\Desktop\Adrastea\MCU_02_02_10_00_31121\middleware\altcomlib\altcom\include\api\gps" -I"C:\Users\shashank.hegde\Desktop\Adrastea\MCU_02_02_10_00_31121\middleware\altcomlib\altcom\include\common" -I"C:\Users\shashank.hegde\Desktop\Adrastea\MCU_02_02_10_00_31121\middleware\altcomlib\altcom\include" -include"C:/Users/shashank.hegde/Desktop/Adrastea/MCU_02_02_10_00_31121/examples/ALT125X/GpsConnector/Eclipse/../source/config.h" -include"C:\Users\shashank.hegde\Desktop\Adrastea\MCU_02_02_10_00_31121\middleware\printf\printf.h" -std=gnu11 -funwind-tables $(EXTRA_CFLAGS) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

