################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../utilities/str/fsl_str.c 

C_DEPS += \
./utilities/str/fsl_str.d 

OBJS += \
./utilities/str/fsl_str.o 


# Each subdirectory must supply rules for building sources it contributes
utilities/str/%.o: ../utilities/str/%.c utilities/str/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__REDLIB__ -DCPU_MCXA154VFT -DCPU_MCXA154VFT_cm33 -DSDK_OS_BAREMETAL -DSDK_DEBUGCONSOLE=1 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -DSERIAL_PORT_TYPE_UART=1 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I"C:\Users\mrankin\OneDrive\NXP_Workspace\MCXA154VFT_External_Crystal_Test\board" -I"C:\Users\mrankin\OneDrive\NXP_Workspace\MCXA154VFT_External_Crystal_Test\source" -I"C:\Users\mrankin\OneDrive\NXP_Workspace\MCXA154VFT_External_Crystal_Test\drivers" -I"C:\Users\mrankin\OneDrive\NXP_Workspace\MCXA154VFT_External_Crystal_Test\component\lists" -I"C:\Users\mrankin\OneDrive\NXP_Workspace\MCXA154VFT_External_Crystal_Test\utilities" -I"C:\Users\mrankin\OneDrive\NXP_Workspace\MCXA154VFT_External_Crystal_Test\CMSIS" -I"C:\Users\mrankin\OneDrive\NXP_Workspace\MCXA154VFT_External_Crystal_Test\CMSIS\m-profile" -I"C:\Users\mrankin\OneDrive\NXP_Workspace\MCXA154VFT_External_Crystal_Test\utilities\debug_console" -I"C:\Users\mrankin\OneDrive\NXP_Workspace\MCXA154VFT_External_Crystal_Test\component\serial_manager" -I"C:\Users\mrankin\OneDrive\NXP_Workspace\MCXA154VFT_External_Crystal_Test\device" -I"C:\Users\mrankin\OneDrive\NXP_Workspace\MCXA154VFT_External_Crystal_Test\device\periph1" -I"C:\Users\mrankin\OneDrive\NXP_Workspace\MCXA154VFT_External_Crystal_Test\utilities\str" -I"C:\Users\mrankin\OneDrive\NXP_Workspace\MCXA154VFT_External_Crystal_Test\utilities\debug_console\config" -I"C:\Users\mrankin\OneDrive\NXP_Workspace\MCXA154VFT_External_Crystal_Test\component\uart" -O0 -fno-common -g3 -gdwarf-4 -Wall -c -ffunction-sections -fdata-sections -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-utilities-2f-str

clean-utilities-2f-str:
	-$(RM) ./utilities/str/fsl_str.d ./utilities/str/fsl_str.o

.PHONY: clean-utilities-2f-str

