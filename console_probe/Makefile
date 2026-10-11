# Read-only Wii U MCP scanner (.wuhb), based on the standard wut homebrew rules.
# Requires devkitPro with wiiu-dev. Not built in CI (toolchain not available).
ifeq ($(strip $(DEVKITPRO)),)
$(error Set DEVKITPRO to your local devkitPro installation path)
endif
APP_NAME := Wii U Library Profiles - Catalog Probe
APP_SHORTNAME := Library Profiles Probe
APP_AUTHOR := TopCat
include $(DEVKITPRO)/wut/share/wut_rules
TARGET := wiiu_library_profiles_probe
BUILD := build
SOURCES := src
INCLUDES := src
CFLAGS := -g -std=gnu11 -Wall -Wextra -O2 -ffunction-sections $(MACHDEP) $(INCLUDE)
ASFLAGS := -g $(ARCH)
LDFLAGS := -g $(ARCH) $(RPXSPECS) -Wl,-Map,$(notdir $*.map)
LIBS := -lwut
LIBDIRS := $(PORTLIBS) $(WUT_ROOT) $(WUT_ROOT)/usr
ifneq ($(BUILD),$(notdir $(CURDIR)))
export OUTPUT := $(CURDIR)/$(TARGET)
export TOPDIR := $(CURDIR)
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)
CFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
export OFILES := $(CFILES:.c=.o)
export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) $(foreach dir,$(LIBDIRS),-I$(dir)/include) -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)
.PHONY: all clean
all:
	@mkdir -p $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
clean:
	@rm -rf $(BUILD) $(TARGET).wuhb $(TARGET).rpx $(TARGET).elf
else
DEPENDS := $(OFILES:.o=.d)
all: $(OUTPUT).wuhb
$(OUTPUT).wuhb: $(OUTPUT).rpx
$(OUTPUT).rpx: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)
-include $(DEPENDS)
endif
