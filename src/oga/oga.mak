#CPUDEFS += -DHAS_CYCLONE=1
#CPUDEFS += -DHAS_DRZ80=0

OBJDIRS += $(OBJ)/cpu/m68000_cyclone $(OBJ)/cpu/z80_drz80
CPUOBJS += $(OBJ)/cpu/m68000_cyclone/cyclone.o $(OBJ)/cpu/m68000_cyclone/c68000.o
CPUOBJS += $(OBJ)/cpu/z80_drz80/drz80.o $(OBJ)/cpu/z80_drz80/drz80_z80.o

#OBJDIRS += $(OBJ)/cpu/nec_armnec
#CPUDEFS += -DHAS_ARMNEC

#CPUOBJS += $(OBJ)/cpu/nec_armnec/armV30.o $(OBJ)/cpu/nec_armnec/armV33.o $(OBJ)/cpu/nec_armnec/armnecintrf.o

OSOBJS = $(OBJ)/oga/minimal.o \
	$(OBJ)/oga/rpi.o $(OBJ)/oga/video.o $(OBJ)/oga/blit.o \
	$(OBJ)/oga/sound.o $(OBJ)/oga/input.o $(OBJ)/oga/fileio.o \
	$(OBJ)/oga/config.o $(OBJ)/oga/fronthlp.o \
	$(OBJ)/oga/fifo_buffer.o $(OBJ)/oga/thread.o \
	$(OBJ)/oga/gles2.o \
	$(OBJ)/oga/gp2x_frontend.o
