# Project Name
TARGET = FourSeas

# Includes FatFS source files within project.
USE_FATFS = 1

# Library Locations
LIBDAISY_DIR = ./libDaisy
DAISYSP_DIR = ./DaisySP
STMLIB_DIR = ./stmlib
SRC_DIR = ./src

# Custom
BOARD_REV ?= 4
WAVE_SAMPLES ?= 2048
CPPFLAGS += -DCURRENT_BOARD_REV=FourSeasHW::BoardRevision::REV_$(BOARD_REV)
CPPFLAGS += -DCURRENT_WAVE_SAMPLES=$(WAVE_SAMPLES)

# C++ Sources
CC_SOURCES += FourSeas.cc
CC_SOURCES += $(SRC_DIR)/cal_input.cc
CC_SOURCES += $(SRC_DIR)/settings.cc
CC_SOURCES += $(SRC_DIR)/ui.cc
CC_SOURCES += $(SRC_DIR)/app_state.cc
CC_SOURCES += $(SRC_DIR)/analog_ctrl_24.cc
CC_SOURCES += $(SRC_DIR)/parameter_24.cc
CC_SOURCES += $(SRC_DIR)/params.cc
CC_SOURCES += $(SRC_DIR)/crash_log.cc
CC_SOURCES += $(SRC_DIR)/hardware/fourSeasBoard.cc
CC_SOURCES += $(SRC_DIR)/drivers/MCP3564R.cc
CC_SOURCES += $(SRC_DIR)/drivers/MCP23008.cc
CC_SOURCES += $(SRC_DIR)/drivers/TLC59116.cc

CC_SOURCES += $(STMLIB_DIR)/dsp/units.cc

C_INCLUDES += \
-I. \
-Iresources \
-I../..

# Can't actually add to CFLAGS.. due to libDaisy stuff
# silences warning from stmlib JOIN macros
C_INCLUDES += -Wno-unused-local-typedefs

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core

include $(SYSTEM_FILES_DIR)/Makefile

ifeq ($(DEBUG), 1)
# Debug-friendly optimization
OPT = -Og
else
# Release optimization
OPT = -O3
endif

CPP_STANDARD = -std=gnu++17

### Need to override all to get support for .cc files
all: $(BUILD_DIR)/$(TARGET)_rev$(BOARD_REV)_$(WAVE_SAMPLES).elf \
	$(BUILD_DIR)/$(TARGET)_rev$(BOARD_REV)_$(WAVE_SAMPLES).hex \
	$(BUILD_DIR)/$(TARGET)_rev$(BOARD_REV)_$(WAVE_SAMPLES).bin \
	$(BUILD_DIR)/$(TARGET).elf \
	$(BUILD_DIR)/$(TARGET).hex \
	$(BUILD_DIR)/$(TARGET).bin


#######################################
# build the application
#######################################
# list of objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))

OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(CC_SOURCES:.cc=.o)))
vpath %.cc $(sort $(dir $(CC_SOURCES)))

# list of ASM program objects
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $(C_STANDARD) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.cc Makefile | $(BUILD_DIR)
	$(CXX) -c $(CPPFLAGS) $(CLIFLAGS) $(CPP_STANDARD) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.cc=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET)_rev$(BOARD_REV)_$(WAVE_SAMPLES).elf: $(OBJECTS) Makefile
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@

program-jlink:
	JLinkExe -device STM32H750IB -if SWD -speed 4000 -autoconnect 1 -CommanderScript jlink.flash
