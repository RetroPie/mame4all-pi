ifeq ($(TARGET),)
TARGET = mame
endif

ifeq ($(PLATFORM),)
PLATFORM = rpi1
endif

# set this the operating system you're building for
# (actually you'll probably need your own main makefile anyways)
# MAMEOS = msdos
# MAMEOS = gp2x
MAMEOS = rpi

# extension for executables
# EXE = .exe
EXE =

# CPU core include paths
VPATH=src $(wildcard src/cpu/*)

# compiler, linker and utilities
MD = @mkdir -p
RM = rm -f
CC  = gcc
CPP = g++
AS  = as
LD  = gcc
STRIP = strip

EMULATOR = $(TARGET)$(EXE)

DEFS = -DGP2X -DLSB_FIRST -DALIGN_INTS -DALIGN_SHORTS -DINLINE="static __inline" -Dasm="__asm__ __volatile__" -DMAME_UNDERCLOCK -DENABLE_AUTOFIRE -DBIGCASE
##sq DEFS = -DGP2X -DLSB_FIRST -DALIGN_INTS -DALIGN_SHORTS -DINLINE="static __inline" -Dasm="__asm__ __volatile__" -DMAME_UNDERCLOCK -DMAME_FASTSOUND -DENABLE_AUTOFIRE -DBIGCASE

CFLAGS += -fsigned-char $(DEVLIBS) \
	-Isrc -Isrc/$(MAMEOS) -Isrc/zlib \
	-I/usr/include/SDL \
	-I$(SDKSTAGE)/opt/vc/include -I$(SDKSTAGE)/opt/vc/include/interface/vcos/pthreads \
	-I$(SDKSTAGE)/opt/vc/include/interface/vmcs_host/linux \
	-I/usr/include/glib-2.0 -I/usr/lib/arm-linux-gnueabihf/glib-2.0/include

# Platform specific
ifeq ($(PLATFORM),rpi2)
CFLAGS += -mcpu=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard
else
CFLAGS += -march=armv6j -mfpu=vfp -mfloat-abi=hard
endif

CFLAGS += -O3 -ffast-math -fno-builtin -fsingle-precision-constant \
	-fno-common -mstructure-size-boundary=32 -fweb -frename-registers \
	-Wall -Wno-sign-compare -Wunused -Wpointer-arith -Wcast-align -Waggregate-return -Wshadow

LDFLAGS = $(CFLAGS)

LIBS = -lm -lpthread -lSDL -L$(SDKSTAGE)/opt/vc/lib -lbcm_host -lGLESv2 -lEGL -lglib-2.0 -lasound -lrt

OBJ = obj_$(TARGET)_$(MAMEOS)
OBJDIRS = $(OBJ) $(OBJ)/cpu $(OBJ)/sound $(OBJ)/$(MAMEOS) \
	$(OBJ)/drivers $(OBJ)/machine $(OBJ)/vidhrdw $(OBJ)/sndhrdw \
	$(OBJ)/zlib

all:    print-PLATFORM  maketree $(EMULATOR)

# include the various .mak files
include src/core.mak
include src/$(TARGET).mak
include src/rules.mak
include src/sound.mak
include src/$(MAMEOS)/$(MAMEOS).mak

# combine the various definitions to one
CDEFS = $(DEFS) $(COREDEFS) $(CPUDEFS) $(SOUNDDEFS)

$(EMULATOR): $(COREOBJS) $(OSOBJS) $(DRVOBJS)
	$(LD) $(LDFLAGS) $(COREOBJS) $(OSOBJS) $(LIBS) $(DRVOBJS) -o $@
	$(STRIP) $(EMULATOR)	

$(OBJ)/%.o: src/%.c
	@echo Compiling $<...
	$(CC) $(CDEFS) $(CFLAGS) -c $< -o $@

$(OBJ)/%.o: src/%.cpp
	@echo Compiling $<...
	$(CPP) $(CDEFS) $(CFLAGS) -fno-rtti -c $< -o $@

$(OBJ)/%.o: src/%.s
	@echo Compiling $<...
	$(CPP) $(CDEFS) $(CFLAGS) -c $< -o $@

$(OBJ)/%.o: src/%.S
	@echo Compiling $<...
	$(CPP) $(CDEFS) $(CFLAGS) -c $< -o $@

$(sort $(OBJDIRS)):
	$(MD) $@

maketree: $(sort $(OBJDIRS))

clean:
	$(RM) -r $(OBJ)
	$(RM) $(EMULATOR)

print-%: ; @echo $*=$($*)
