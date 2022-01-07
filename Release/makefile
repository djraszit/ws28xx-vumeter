################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

uname_m := $(shell uname -m)

output_bin_dir := 

ifeq ($(uname_m),x86_64)
	output_bin_dir=bin/x86_64
else
	output_bin_dir=bin/arm
endif

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: ws28xx_esp8266_effects

# Tool invocations
ws28xx_esp8266_effects: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc  -o "ws28xx_esp8266_effects" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	@if [ ! -d $(output_bin_dir) ]; then echo 'Tworzenie katalogu $(output_bin_dir)'; mkdir -p $(output_bin_dir); fi
	@echo 'Przenoszę binarkę do $(output_bin_dir)'
	@mv ws28xx_esp8266_effects $(output_bin_dir)
	@echo 'binarka będzie w $(output_bin_dir)'
	

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) ws28xx_esp8266_effects
	-@echo ' '
	-rm $(output_bin_dir)/ws28xx_esp8266_effects

.PHONY: all clean dependents

-include ../makefile.targets
