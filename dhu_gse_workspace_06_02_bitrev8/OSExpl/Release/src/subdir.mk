################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
LD_SRCS += \
../src/lscript.ld 

C_SRCS += \
../src/OSExpl.c \
../src/dhu.c \
../src/fpe.c \
../src/fpeinit.c \
../src/ip_utils.c \
../src/platform.c \
../src/platform_zynq.c \
../src/server.c 

OBJS += \
./src/OSExpl.o \
./src/dhu.o \
./src/fpe.o \
./src/fpeinit.o \
./src/ip_utils.o \
./src/platform.o \
./src/platform_zynq.o \
./src/server.o 

C_DEPS += \
./src/OSExpl.d \
./src/dhu.d \
./src/fpe.d \
./src/fpeinit.d \
./src/ip_utils.d \
./src/platform.d \
./src/platform_zynq.d \
./src/server.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM gcc compiler'
	arm-xilinx-eabi-gcc -Wall -O2 -c -fmessage-length=0 -MT"$@" -I../../standalone_bsp_0/ps7_cortexa9_0/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


