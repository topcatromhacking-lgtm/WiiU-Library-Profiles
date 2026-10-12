# Wii U Library Profiles v1.0 Stage A: independent WUHB, not a WUPS filter
ifeq ($(strip $(DEVKITPRO)),)
$(error DEVKITPRO required)
endif
TOPDIR ?= $(CURDIR)
APP_NAME := Wii U Library Profiles - Inventory Scanner
APP_SHORTNAME := WLP Inventory Scanner
APP_AUTHOR := TopCat
include $(DEVKITPRO)/wut/share/wut_rules
TARGET := wlp_inventory_scanner
BUILD := build
SOURCES := source
DATA :=
INCLUDES := source
CONTENT :=
ICON :=
TV_SPLASH :=
DRC_SPLASH :=
CFLAGS := -Wall -Wextra -O2 -std=gnu11 -ffunction-sections $(MACHDEP) $(INCLUDE) -D__WIIU__ -D__WUT__
ASFLAGS := -g $(ARCH)
LDFLAGS := -g $(ARCH) $(RPXSPECS) -Wl,-Map,$(notdir $*.map)
LIBS := -lwut
LIBDIRS := $(PORTLIBS) $(WUT_ROOT)
ifneq ($(BUILD),$(notdir $(CURDIR)))
export OUTPUT := $(CURDIR)/$(TARGET)
export TOPDIR := $(CURDIR)
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)
CFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
export OFILES := $(CFILES:.c=.o)
export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) $(foreach dir,$(LIBDIRS),-I$(dir)/include) -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)
export LD := $(CC)
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
