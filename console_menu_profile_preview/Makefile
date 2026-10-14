ifeq ($(strip $(DEVKITPRO)),)
$(error DEVKITPRO is required)
endif
TOPDIR ?= $(CURDIR)
include $(DEVKITPRO)/wups/share/wups_rules
TARGET := wiiu_library_profiles_v12_menu_preview
BUILD := build
SOURCES := src
INCLUDES := src
WUT_ROOT := $(DEVKITPRO)/wut
WUPS_ROOT := $(DEVKITPRO)/wups
CFLAGS := -Wall -Wextra -O2 -ffunction-sections $(MACHDEP) $(INCLUDE)
CXXFLAGS := $(CFLAGS) -std=c++20
ASFLAGS := -g $(ARCH)
LDFLAGS := -g $(ARCH) $(RPXSPECS) -Wl,-Map,$(notdir $*.map) $(WUPSSPECS)
LIBS := -lwups -lwut
LIBDIRS := $(PORTLIBS) $(WUPS_ROOT) $(WUT_ROOT)
ifneq ($(BUILD),$(notdir $(CURDIR)))
export OUTPUT := $(CURDIR)/$(TARGET)
export TOPDIR := $(CURDIR)
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)
CFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
export OFILES := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o)
export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) $(foreach dir,$(LIBDIRS),-I$(dir)/include) -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)
export LD := $(CXX)
.PHONY: all clean
all:
	@mkdir -p $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
clean:
	@rm -rf $(BUILD) $(TARGET).wps $(TARGET).elf
else
DEPENDS := $(OFILES:.o=.d)
all: $(OUTPUT).wps
$(OUTPUT).wps: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)
-include $(DEPENDS)
endif
