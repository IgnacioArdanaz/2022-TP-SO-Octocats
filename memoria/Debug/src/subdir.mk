################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/memoria_main.c \
../src/memoria_swap.c \
../src/memoria_utils.c 

OBJS += \
./src/memoria_main.o \
./src/memoria_swap.o \
./src/memoria_utils.o 

C_DEPS += \
./src/memoria_main.d \
./src/memoria_swap.d \
./src/memoria_utils.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/tp-2022-1c-Octocats/shared/src" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


