# If you move this project you can change the directory
# to match your GBDK root directory (ex: GBDK_HOME = "C:/GBDK/"
ifndef GBDK_HOME
	GBDK_HOME = ../../gbdk2020/gbdk-2020-git/build/gbdk/
endif

LCC = $(GBDK_HOME)bin/lcc
PNG2ASSET = $(GBDK_HOME)/bin/png2asset

# Set platforms to build here, spaced separated. (These are in the separate Makefile.targets)
# They can also be built/cleaned individually: "make gg" and "make gg-clean"
# Possible are: gb gbc pocket megaduck sms gg
TARGETS=gb # gbc gg sms nes

# Configure platform specific LCC flags here:
LCCFLAGS_gb      = -Wl-yt0x1B -autobank # Set an MBC for banking (1B-ROM+MBC5+RAM+BATT)
LCCFLAGS_pocket  = -Wl-yt0x1B -autobank # Usually the same as required for .gb
LCCFLAGS_duck    = -Wl-yt0x1B -autobank # Usually the same as required for .gb
LCCFLAGS_gbc     = -Wl-yt0x1B -Wm-yc -autobank # Same as .gb with: -Wm-yc (gb & gbc) or Wm-yC (gbc exclusive)
LCCFLAGS_sms     =
LCCFLAGS_gg      =
LCCFLAGS_nes     = -autobank



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


LCCFLAGS += $(LCCFLAGS_$(EXT)) # This adds the current platform specific LCC Flags

LCCFLAGS += -Wl-j -Wm-yoA -Wm-ya4 -Wb-ext=.rel -Wb-v # MBC + Autobanking related flags

# P2AFLAGS_gb     = -map -bpp 2 -max_palettes 4 -pack_mode gb -noflip
# P2AFLAGS_gbc    = -map -use_map_attributes -bpp 2 -max_palettes 8 -pack_mode gb
# P2AFLAGS_pocket = -map -bpp 2 -max_palettes 4 -pack_mode gb -noflip
# P2AFLAGS_duck   = -map -bpp 2 -max_palettes 4 -pack_mode gb -noflip
# P2AFLAGS_sms    = -map -use_map_attributes -bpp 4 -max_palettes 2 -pack_mode sms
# P2AFLAGS_gg     = -map -use_map_attributes -bpp 4 -max_palettes 2 -pack_mode sms
# P2AFLAGS_nes    = -map -bpp 2 -max_palettes 4 -pack_mode nes -noflip -use_nes_attributes -b 0

# P2AFLAGS = $(P2AFLAGS_$(EXT))

# GBDK_DEBUG = ON
ifdef GBDK_DEBUG
	LCCFLAGS += -debug -v
endif


# You can set the name of the ROM file here
PROJECTNAME = breakout-lavalamp

CFLAGS = -Wf-MMD -Wf-Wp-MP # Header file dependency output (-MMD) for Makefile use + per-header Phoney rules (-MP)

# EXT?=gb # Only sets extension to default (game boy .gb) if not populated
SRCDIR      = src
OBJDIR      = obj/$(EXT)
RESOBJSRC   = obj/$(EXT)/res
RESDIR      = res
BINDIR      = build/$(EXT)
MKDIRS      = $(OBJDIR) $(BINDIR) $(RESOBJSRC) # See bottom of Makefile for directory auto-creation

BINS	    = $(OBJDIR)/$(PROJECTNAME).$(EXT)
CSOURCES    = $(foreach dir,$(SRCDIR),$(notdir $(wildcard $(dir)/*.c))) $(foreach dir,$(RESDIR),$(notdir $(wildcard $(dir)/*.c)))
ASMSOURCES  = $(foreach dir,$(SRCDIR),$(notdir $(wildcard $(dir)/*.s)))
OBJS        = $(CSOURCES:%.c=$(OBJDIR)/%.o) $(ASMSOURCES:%.s=$(OBJDIR)/%.o)

# For png2asset: converting map source pngs -> .c -> .o
MAPPNGS    = $(foreach dir,$(RESDIR),$(notdir $(wildcard $(dir)/*.png)))
MAPSRCS    = $(MAPPNGS:%.png=$(RESOBJSRC)/%.c)
MAPOBJS    = $(MAPSRCS:$(RESOBJSRC)/%.c=$(OBJDIR)/%.o)

.PRECIOUS: $(MAPSRCS)

CFLAGS += -I$(OBJDIR) 

# Builds all targets sequentially
all: $(TARGETS)

# Use png2asset to convert the png into C formatted map data
# -c ...   : Set C output file
# Convert metasprite .pngs in res/ -> .c files in obj/<platform ext>/src/
$(RESOBJSRC)/%.c:	$(RESDIR)/%.png
	$(PNG2ASSET) $< $(P2AFLAGS) -c $@

# Compile the map pngs that were converted to .c files
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

# Convert and build maps first so they're available when compiling the main sources
$(OBJS):	$(MAPOBJS)

# Link the compiled object files into a .gb ROM file
$(BINS):	$(OBJS)
	$(LCC) $(LCCFLAGS) $(CFLAGS) -o $(BINDIR)/$(PROJECTNAME).$(EXT) $(MAPOBJS) $(OBJS)

clean:
	@echo Cleaning
	@for target in $(TARGETS); do \
		$(MAKE) $$target-clean; \
	done

# Include available build targets
include Makefile.targets


# create necessary directories after Makefile is parsed but before build
# info prevents the command from being pasted into the makefile
ifneq ($(strip $(EXT)),)           # Only make the directories if EXT has been set by a target
$(info $(shell mkdir -p $(MKDIRS)))
endif
