################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/MCXA154VFT_ST7789_Sensor_Board.c \
../source/lsm6ds3_lpi2c.c \
../source/semihost_hardfault.c \
../source/sht40_lpi2c.c \
../source/st7789_lpspi.c 

C_DEPS += \
./source/MCXA154VFT_ST7789_Sensor_Board.d \
./source/lsm6ds3_lpi2c.d \
./source/semihost_hardfault.d \
./source/sht40_lpi2c.d \
./source/st7789_lpspi.d 

OBJS += \
./source/MCXA154VFT_ST7789_Sensor_Board.o \
./source/lsm6ds3_lpi2c.o \
./source/semihost_hardfault.o \
./source/sht40_lpi2c.o \
./source/st7789_lpspi.o 


# Each subdirectory must supply rules for building sources it contributes
source/%.o: ../source/%.c source/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__REDLIB__ -DCPU_MCXA154VFT -DCPU_MCXA154VFT_cm33 -DSDK_OS_BAREMETAL -DSDK_DEBUGCONSOLE=1 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -DSERIAL_PORT_TYPE_UART=1 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I"C:\Users\mrankin\OneDrive\NXP_Workspace\MCXA154VFT_ST7789_Sensor_Board\drivers" -I"C:\Users\mrankin\OneDrive\NXP_Workspace\MCXA154VFT_ST7789_Sensor_Board\component\lists" -I"C:\Users\mrankin\OneDrive\NXP_Workspace\MCXA154VFT_ST7789_Sensor_Board\utilities" -I"C:\Users\mrankin\OneDrive\NXP_Workspace\MCXA154VFT_ST7789_Sensor_Board\CMSIS" -I"C:\Users\mrankin\OneDrive\NXP_Workspace\MCXA154VFT_ST7789_Sensor_Board\CMSIS\m-profile" -I"C:\Users\mrankin\OneDrive\NXP_Workspace\MCXA154VFT_ST7789_Sensor_Board\utilities\debug_console" -I"C:\Users\mrankin\OneDrive\NXP_Workspace\MCXA154VFT_ST7789_Sensor_Board\component\serial_manager" -I"C:\Users\mrankin\OneDrive\NXP_Workspace\MCXA154VFT_ST7789_Sensor_Board\device" -I"C:\Users\mrankin\OneDrive\NXP_Workspace\MCXA154VFT_ST7789_Sensor_Board\device\periph1" -I"C:\Users\mrankin\OneDrive\NXP_Workspace\MCXA154VFT_ST7789_Sensor_Board\utilities\str" -I"C:\Users\mrankin\OneDrive\NXP_Workspace\MCXA154VFT_ST7789_Sensor_Board\utilities\debug_console\config" -I"C:\Users\mrankin\OneDrive\NXP_Workspace\MCXA154VFT_ST7789_Sensor_Board\component\uart" -I"C:\Users\mrankin\OneDrive\NXP_Workspace\MCXA154VFT_ST7789_Sensor_Board\board" -I"C:\Users\mrankin\OneDrive\NXP_Workspace\MCXA154VFT_ST7789_Sensor_Board\source" -O0 -fno-common -g3 -gdwarf-4 -Wall -c -ffunction-sections -fdata-sections -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-source

clean-source:
	-$(RM) ./source/MCXA154VFT_ST7789_Sensor_Board.d ./source/MCXA154VFT_ST7789_Sensor_Board.o ./source/lsm6ds3_lpi2c.d ./source/lsm6ds3_lpi2c.o ./source/semihost_hardfault.d ./source/semihost_hardfault.o ./source/sht40_lpi2c.d ./source/sht40_lpi2c.o ./source/st7789_lpspi.d ./source/st7789_lpspi.o

.PHONY: clean-source

