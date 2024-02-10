# KallistiOS ##version##
#
# kos-ports/libgl Makefile
# Copyright (C) 2013, 2014 Josh Pearson
# Copyright (C) 2014 Lawrence Sebald
# Copyright (C) 2018 Luke Benstead

TARGET = libGLdc.a
OBJS = GL/gldc.o
#OBJS = GL/draw.o GL/flush.o GL/framebuffer.o GL/immediate.o GL/lighting.o GL/state.o GL/texture.o GL/glu.o
#OBJS += GL/matrix.o GL/fog.o GL/error.o GL/clip.o containers/stack.o containers/named_array.o containers/aligned_vector.o GL/profiler.o
OBJS += containers/stack.o containers/named_array.o containers/aligned_vector.o GL/profiler.o

SUBDIRS =

GCCVERSIONGTEQ5 := $(shell expr `sh-elf-gcc -dumpversion | cut -f1 -d.` \>= 5)

ifeq "$(GCCVERSIONGTEQ5)" "1"
    GCC_WILD_FLAGS = -mdiv=inv -matomic-model=none -maccumulate-outgoing-args -ffp-contract=fast  -mpretend-cmove -ffinite-math-only 
	GCC5_FLAGS = -mfsca -mfsrra -mlra $(GCC_WILD_FLAGS)
endif

EXTRA_CFLAGS=  -Wall -Wextra $(GCC5_FLAGS)
KOS_CFLAGS += -ffast-math -O2 -funroll-loops -fsingle-precision-constant -Iinclude -funsafe-math-optimizations -DBUILD_LIBGL $(EXTRA_CFLAGS)

# Manipulate the CFLAGS to look our *our* version of the library and includes
INC_DIR = ./include
LIB_DIR = ./
export CFLAGS := $(CFLAGS) -I $(INC_DIR)
export OBJEXTRA := $(LIB_DIR)/libGLdc.a

link: | $(OBJS) ;
	$(KOS_AR) rcs $(TARGET) $(OBJS)

build: $(OBJS) 

samples: build
	$(KOS_MAKE) -C samples all

defaultall: create_kos_link $(OBJS) subdirs linklib samples

include $(KOS_BASE)/addons/Makefile.prefab

# creates the kos link to the headers
create_kos_link:
	rm -f ../include/GL
	ln -s ../GLdc/include ../include/GL

polymark:
	$(KOS_MAKE) -C samples/quadmark dist

offset:
	$(KOS_MAKE) -C samples/polygon_offset 
	
nehe07:
	$(KOS_MAKE) -C samples/nehe07 dist

mipmap:
	$(KOS_MAKE) -C samples/mipmap dist