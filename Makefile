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
  VCSM_CFLAGS ?= $(shell $(PKGCONF) --cflags vcsm)
  VCSM_LIBS ?= $(shell $(PKGCONF) --libs vcsm)
  EGL_CFLAGS ?= $(shell $(PKGCONF) --cflags egl)
  EGL_LIBS ?= $(shell $(PKGCONF) --libs egl)
  GLIB_CFLAGS ?= $(shell $(PKGCONF) --cflags glib-2.0)
  GLIB_LIBS ?= $(shell $(PKGCONF) --libs glib-2.0)
  ALSA_CFLAGS ?= $(shell $(PKGCONF) --cflags alsa)
  ALSA_LIBS ?= $(shell $(PKGCONF) --libs alsa)
else
  SDL_CFLAGS ?= -I/usr/include/SDL
  SDL_LIBS ?= -lSDL
  VCSM_CFLAGS ?= -I$(SDKSTAGE)/opt/vc/include
  VCSM_LIBS ?= -L$(SDKSTAGE)/opt/vc/lib
  EGL_CFLAGS ?= -I$(SDKSTAGE)/opt/vc/include/interface/vcos/pthreads -I$(SDKSTAGE)/opt/vc/include/interface/vmcs_host/linux
  EGL_LIBS ?= -lbcm_host -lbrcmGLESv2 -lbrcmEGL
  GLIB_CFLAGS ?= -I/usr/include/glib-2.0 -I/usr/lib/arm-linux-gnueabihf/glib-2.0/include
  GLIB_LIBS ?= -lglib-2.0
  ALSA_CFLAGS ?=
  ALSA_LIBS ?= -lasound
endif


CFLAGS += -fsigned-char $(DEVLIBS) \
	-Isrc -Isrc/$(MAMEOS) -Isrc/zlib \
	$(SDL_CFLAGS) \
	$(VCSM_CFLAGS) \
	$(EGL_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(ALSA_CFLAGS) \
	-O3 -ffast-math -fno-builtin -fsingle-precision-constant \
	-Wall -Wno-sign-compare -Wunused -Wpointer-arith -Wcast-align -Waggregate-return -Wshadow \
	-Wno-narrowing

LDFLAGS = $(CFLAGS)

LIBS = -lm -ldl -lpthread -lrt $(SDL_LIBS) $(VCSM_LIBS) $(EGL_LIBS) $(GLIB_LIBS) $(ALSA_LIBS)

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
