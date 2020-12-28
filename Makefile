ifeq ($(TARGET),)
TARGET = mame
endif

# set this the operating system you're building for
# (actually you'll probably need your own main makefile anyways)
# MAMEOS = msdos
#MAMEOS = gp2x
MAMEOS = rpi

# extension for executables
# EXE = .exe
EXE =

# CPU core include paths
VPATH=src $(wildcard src/cpu/*)

# compiler, linker and utilities
MD = @mkdir -p
RM = rm -f
CC  ?= $(CROSS_COMPILE)gcc
CXX ?= $(CROSS_COMPILE)g++
AS  ?= $(CROSS_COMPILE)as
STRIP ?= $(CROSS_COMPILE)strip
PKGCONF ?= pkg-config

EMULATOR = $(TARGET)$(EXE)

DEFS = -DGP2X -DLSB_FIRST -DALIGN_INTS -DALIGN_SHORTS -DINLINE="static __inline" -Dasm="__asm__ __volatile__" -DMAME_UNDERCLOCK -DENABLE_AUTOFIRE -DBIGCASE
##sq DEFS = -DGP2X -DLSB_FIRST -DALIGN_INTS -DALIGN_SHORTS -DINLINE="static __inline" -Dasm="__asm__ __volatile__" -DMAME_UNDERCLOCK -DMAME_FASTSOUND -DENABLE_AUTOFIRE -DBIGCASE

HAS_PKGCONF := $(if $(shell which $(PKGCONF)),yes,no)

ifeq ($(HAS_PKGCONF),yes)
  SDL_CFLAGS ?= $(shell $(PKGCONF) --cflags sdl)
  SDL_LIBS ?= $(shell $(PKGCONF) --libs sdl)
else
  SDL_CFLAGS ?= -I/usr/include/SDL
  SDL_LIBS ?= -lSDL
endif


CFLAGS += -fsigned-char $(DEVLIBS) \
	-Isrc -Isrc/$(MAMEOS) -Isrc/zlib \
	$(SDL_CFLAGS) \
	-I$(SDKSTAGE)/opt/vc/include -I$(SDKSTAGE)/opt/vc/include/interface/vcos/pthreads \
	-I$(SDKSTAGE)/opt/vc/include/interface/vmcs_host/linux \
	-I/usr/include/glib-2.0 -I/usr/lib/arm-linux-gnueabihf/glib-2.0/include \
	-O3 -ffast-math -fno-builtin -fsingle-precision-constant \
	-Wall -Wno-sign-compare -Wunused -Wpointer-arith -Wcast-align -Waggregate-return -Wshadow \
	-Wno-narrowing

LDFLAGS = $(CFLAGS)

LIBS = -lm -ldl -lpthread -lrt $(SDL_LIBS) -L$(SDKSTAGE)/opt/vc/lib -lbcm_host -lbrcmGLESv2 -lbrcmEGL -lglib-2.0 -lasound

OBJ = obj_$(TARGET)_$(MAMEOS)
OBJDIRS = $(OBJ) $(OBJ)/cpu $(OBJ)/sound $(OBJ)/$(MAMEOS) \
	$(OBJ)/drivers $(OBJ)/machine $(OBJ)/vidhrdw $(OBJ)/sndhrdw \
	$(OBJ)/zlib

all:	maketree $(EMULATOR)

# include the various .mak files
include src/core.mak
include src/$(TARGET).mak
include src/rules.mak
include src/sound.mak
include src/$(MAMEOS)/$(MAMEOS).mak

# combine the various definitions to one
CDEFS = $(DEFS) $(COREDEFS) $(CPUDEFS) $(SOUNDDEFS)

$(EMULATOR): $(OBJ)/$(EMULATOR)
	$(STRIP) -o $@ $<

$(OBJ)/$(EMULATOR): $(COREOBJS) $(OSOBJS) $(DRVOBJS)
	$(CC) $(LDFLAGS) $(COREOBJS) $(OSOBJS) $(LIBS) $(DRVOBJS) -o $@

$(OBJ)/%.o: src/%.c
	@echo Compiling $<...
	$(CC) $(CDEFS) $(CFLAGS) -c $< -o $@

$(OBJ)/%.o: src/%.cpp
	@echo Compiling $<...
	$(CXX) $(CDEFS) $(CFLAGS) -fno-rtti -c $< -o $@

$(OBJ)/%.o: src/%.s
	@echo Compiling $<...
	$(CXX) $(CDEFS) $(CFLAGS) -c $< -o $@

$(OBJ)/%.o: src/%.S
	@echo Compiling $<...
	$(CXX) $(CDEFS) $(CFLAGS) -c $< -o $@

$(sort $(OBJDIRS)):
	$(MD) $@

maketree: $(sort $(OBJDIRS))

clean:
	$(RM) -r $(OBJ)
	$(RM) $(EMULATOR)
