# If you move this project you can change the directory
# to match your GBDK root directory (ex: GBDK_HOME = "C:/GBDK/"
ifndef GBDK_HOME
	GBDK_HOME = ../../gbdk2020/gbdk-2020-git/build/gbdk/
endif

ZIP = zip
LCC = $(GBDK_HOME)bin/lcc
PNG2ASSET = $(GBDK_HOME)/bin/png2asset

VERSION=1.1.0

# Configure platform specific LCC flags here:
LCCFLAGS_gb      = -Wl-yt0x1B -autobank # Set an MBC for banking (1B-ROM+MBC5+RAM+BATT)
LCCFLAGS_pocket  = -Wl-yt0x1B -autobank # Usually the same as required for .gb
LCCFLAGS_duck    = -Wl-yt0x1B -autobank # Usually the same as required for .gb
LCCFLAGS_gbc     = -Wl-yt0x1B -Wm-yc -autobank # Same as .gb with: -Wm-yc (gb & gbc) or Wm-yC (gbc exclusive)
LCCFLAGS_sms     =
LCCFLAGS_gg      =
LCCFLAGS_nes     = -autobank


# Alternate cartridges can be passed in as follows
# - 32k_nosave
# - 31k_1kflash
# - mbc5
# make CART_TYPE=<cart type>
ifndef CART_TYPE
#	CART_TYPE=32k_nosave
	CART_TYPE=mbc5
#	CART_TYPE=31k_1kflash
endif


# Set ROM Title / Name
# Title that generates a Grey CGB palette (id:0x16 -> checksum 0x58)
#                       "123456789012345"
LCCFLAGS_gb    += -Wm-yn"BOUNCENLAVALAMP" # sum(hex) = 0x458 & 0xFF = checksum 0x58

# Set CGB Boot ROM color palette to 0x13 (relies on title settings above)
# 1. Old Licensee is already 0x33 -> Use New Licensee
# 2. Sets New Licensee to "01" "(Nintendo)
# 3. (Calculated by Sum of ROM Header title bytes 0x134 through 0x142 [excluding cgb flag at 0x143]) & 0xFF = checksum val 0xNN -> CGB Boot Pal X
#    Legal chars are from 0x20 - 0x5F
#    https://gbdev.io/pandocs/Power_Up_Sequence.html#compatibility-palettes
#    https://tcrf.net/Notes:Game_Boy_Color_Bootstrap_ROM#Manual_Select_Palette_Configurations
LCCFLAGS_gb  += -Wm-yk01

### Handle cart specific flags

# MBC5 - *NO* Rumble
ifeq ($(CART_TYPE),mbc5)
	TARGETS=gb pocket
	LCCFLAGS_gb      += -Wl-yt0x1B -Wl-ya1 # Set an MBC for banking:0x1B 	MBC-5 	SRAM 	BATTERY 		8 MB
	LCCFLAGS_pocket  += -Wl-yt0x1B -Wl-ya1 # Same as for .gb
	CART_TYPE_INC_DIR = mbc5
endif

# 31K+1k cart loses 1024 bytes at the end for flash storage
ifeq ($(CART_TYPE),31k_1kflash)
	# No reason to build .pocket for the 31K + 1k flash cart
	TARGETS=gb
	# Add the flash 1K region as an exclusive no-use area for rom usage calcs
	ROMUSAGE_flags = -e:FLASH_SAVE:7C00:400
	CART_TYPE_INC_DIR = 31k_1kflash
endif

# Generic 32 Cart with no save support
ifeq ($(CART_TYPE),32k_nosave)
	TARGETS=gb pocket megaduck
	CART_TYPE_INC_DIR = 32k_nosave
endif


# Targets can be forced with this override, but normally they will be controlled per-cart type above
#
# Set platforms to build here, spaced separated. (These are in the separate Makefile.targets)
# They can also be built/cleaned individually: "make gg" and "make gg-clean"
# Possible are: gb gbc pocket megaduck sms gg
# TARGETS=gb pocket


LCCFLAGS += $(LCCFLAGS_$(EXT)) # This adds the current platform specific LCC Flags

LCCFLAGS += -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v # MBC + Autobanking related flags


# GBDK_DEBUG = ON
ifdef GBDK_DEBUG
	LCCFLAGS += -debug -v
endif

CFLAGS = -Wf-MMD -Wf-Wp-MP # Header file dependency output (-MMD) for Makefile use + per-header Phoney rules (-MP)
# Pass in defined cart type
CFLAGS += -DCART_$(CART_TYPE)
# Add include path for type of flash cart if enabled
CFLAGS += -Wf-I"$(CART_TYPE_DIR)/"

# Higher optimization setting for compiler
# Can save maybe 1k (allocs=100000) to 2K(allocs=1000000) cycles in the C version
# CFLAGS += -Wf--max-allocs-per-node1000000


# You can set the name of the ROM file here
PROJECTNAME = breakout-lavalamp_$(VERSION)_$(CART_TYPE)


# EXT?=gb # Only sets extension to default (game boy .gb) if not populated
SRCDIR      = src
OBJDIR      = obj/$(EXT)/$(CART_TYPE)
RESOBJSRC   = obj/$(EXT)/$(CART_TYPE)/res
RESDIR      = res
BUILD_DIR   = build
BINDIR      = $(BUILD_DIR)/$(EXT)
CART_TYPE_DIR  = $(SRCDIR)/cart_$(CART_TYPE_INC_DIR)
PACKAGE_DIR    = package
PACKAGE_ZIP_FILE = $(PACKAGE_DIR)/breakout_lavalamp_ROMs_$(VERSION).zip
MKDIRS      = $(OBJDIR) $(BINDIR) $(RESOBJSRC) $(PACKAGE_DIR) # See bottom of Makefile for directory auto-creation

OUTPUT_NAME     = $(BINDIR)/$(PROJECTNAME).$(EXT)
ZIP_OUTPUT_NAME = $(EXT)/$(PROJECTNAME).$(EXT)  # BUILD dir stripped off so it gets included as a folder in zip file

BINS	    = $(OBJDIR)/$(PROJECTNAME).$(EXT)
CSOURCES    = $(foreach dir,$(SRCDIR),$(notdir $(wildcard $(dir)/*.c))) $(foreach dir,$(RESDIR),$(notdir $(wildcard $(dir)/*.c)))
ASMSOURCES  = $(foreach dir,$(SRCDIR),$(notdir $(wildcard $(dir)/*.s)))
OBJS        = $(CSOURCES:%.c=$(OBJDIR)/%.o) $(ASMSOURCES:%.s=$(OBJDIR)/%.o)

CSOURCES_CART = $(foreach dir,$(CART_TYPE_DIR),$(notdir $(wildcard $(dir)/*.c)))
OBJS        += $(CSOURCES_CART:%.c=$(OBJDIR)/%.o)
OBJS        += $(ASMSOURCES_CART:%.s=$(OBJDIR)/%.o)

# For png2asset: converting PNG source pngs -> .c -> .o
PNGS    = $(foreach dir,$(RESDIR),$(notdir $(wildcard $(dir)/*.png)))
PNGSRCS    = $(PNGS:%.png=$(RESOBJSRC)/%.c)
PNGOBJS    = $(PNGSRCS:$(RESOBJSRC)/%.c=$(OBJDIR)/%.o)

.PRECIOUS: $(PNGSRCS)
# .PRECIOUS: $(PNGSRCS)

CFLAGS += -I$(OBJDIR)

# Builds all targets sequentially
all: $(TARGETS)

# Use png2asset to convert the png into C formatted data
# -c ...   : Set C output file
# Convert metasprite .pngs in res/ -> .c files in obj/<platform ext>/src/
.SECONDEXPANSION:
$(RESOBJSRC)/%.c:	$(RESDIR)/%.png $$(wildcard $(RESDIR)/%.png.meta)
	$(PNG2ASSET) $< `cat <$<.meta 2>/dev/null` -c $@

# Compile the pngs that were converted to .c files
# .c files in obj/<platform ext>/src/ -> .o files in obj/
$(OBJDIR)/%.o:	$(RESOBJSRC)/%.c
	$(LCC) $(LCCFLAGS) $(CFLAGS) -c -o $@ $<


# Dependencies (using output from -Wf-MMD -Wf-Wp-MP)
DEPS = $(OBJS:%.o=%.d)

-include $(DEPS)

# Compile .c files in "src/" to .o object files
$(OBJDIR)/%.o:	$(SRCDIR)/%.c
	$(LCC) $(LCCFLAGS) $(CFLAGS) -c -o $@ $<

# Compile .c files in "res/" to .o object files
$(OBJDIR)/%.o:	$(RESDIR)/%.c
	$(LCC) $(LCCFLAGS) $(CFLAGS) -c -o $@ $<

# Compile .s assembly files in "src/" to .o object files
$(OBJDIR)/%.o:	$(SRCDIR)/%.s
	$(LCC) $(LCCFLAGS) $(CFLAGS) -c -o $@ $<

# If needed, compile .c files in "src/" to .s assembly files
# (not required if .c is compiled directly to .o)
$(OBJDIR)/%.s:	$(SRCDIR)/%.c
	$(LCC) $(LCCFLAGS) $(CFLAGS) -S -o $@ $<

# Compile .c files in "src/<CART_TYPE_DIR>/" to .o object files
$(OBJDIR)/%.o:	$(CART_TYPE_DIR)/%.c
	$(LCC) $(CFLAGS) -c -o $@ $<

# Compile .s assembly files in "src/" to .o object files
$(OBJDIR)/%.o:	$(CART_TYPE_DIR)/%.s
	$(LCC) $(CFLAGS) -c -o $@ $<


# Convert and build png image data first so they're available when compiling the main sources
$(OBJS):	$(PNGOBJS)

# Link the compiled object files into a .gb ROM file
$(BINS):	$(OBJS)
	$(LCC) $(LCCFLAGS) $(CFLAGS) -o $(OUTPUT_NAME) $(PNGOBJS) $(OBJS)
# Optional Packaging
# - at start of line ignores error code for zip, which returns non-success if file was already "up to date"
# -j removes paths
ifdef ADD_ZIP
	-cd $(BUILD_DIR); $(ZIP) -u ../$(PACKAGE_ZIP_FILE) $(ZIP_OUTPUT_NAME)
endif


clean:
	@echo Cleaning
	@for target in $(TARGETS); do \
		$(MAKE) $$target-clean; \
	done


carts:
#	${MAKE} CART_TYPE=31k_1kflash
	${MAKE} CART_TYPE=mbc5
	${MAKE} CART_TYPE=32k_nosave

carts-clean:
#	${MAKE} CART_TYPE=31k_1kflash clean
	${MAKE} CART_TYPE=mbc5 clean
	${MAKE} CART_TYPE=32k_nosave clean


package: zip-carts

zip-carts:
#	${MAKE} ADD_ZIP=YES  CART_TYPE=31k_1kflash
	${MAKE} ADD_ZIP=YES  CART_TYPE=mbc5
	${MAKE} ADD_ZIP=YES  CART_TYPE=32k_nosave
	-$(ZIP) -u $(PACKAGE_ZIP_FILE) README.md
# -j junk path
	-$(ZIP) -j -u $(PACKAGE_ZIP_FILE) info/Which\ ROM\ File\ To\ Use.txt
#	-$(ZIP) -j -u $(PACKAGE_ZIP_FILE) info/Changelog.md


# Include available build targets
include Makefile.targets


# create necessary directories after Makefile is parsed but before build
# info prevents the command from being pasted into the makefile
ifneq ($(strip $(EXT)),)           # Only make the directories if EXT has been set by a target
$(info $(shell mkdir -p $(MKDIRS)))
endif
