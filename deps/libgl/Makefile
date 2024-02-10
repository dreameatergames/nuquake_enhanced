# KallistiOS ##version##
#
# kos-ports/libgl Makefile
# Copyright (C) 2013, 2014 Josh Pearson
# Copyright (C) 2014 Lawrence Sebald
# Copyright (C) 2020 Luke Benstead
# Copyright (c) 2020 HaydenKow

TARGET = libGLdc.a
OBJS = GL/gldc.o
#OBJS = GL/draw.o GL/flush.o GL/framebuffer.o GL/immediate.o GL/lighting.o GL/state.o GL/texture.o GL/glu.o
#OBJS += GL/matrix.o GL/fog.o GL/error.o GL/clip.o containers/stack.o containers/named_array.o containers/aligned_vector.o GL/profiler.o
#OBJS += containers/stack.o containers/named_array.o containers/aligned_vector.o GL/profiler.o

SUBDIRS =

GCCVERSIONGTEQ5 := $(shell expr `sh-elf-gcc -dumpversion | cut -f1 -d.` \>= 5)

ifeq "$(GCCVERSIONGTEQ5)" "1"
	GCC5_FLAGS = -mfsca -mfsrra -mlra
endif
KOS_CFLAGS = -fomit-frame-pointer -ml -m4-single-only -I/opt/toolchains/dc/kos/include -I/opt/toolchains/dc/kos/kernel/arch/dreamcast/include -I/opt/toolchains/dc/kos/addons/include -I/opt/toolchains/dc/kos/../kos-ports/include -D_arch_dreamcast -D_arch_sub_pristine

BASE_CFLAGS =  -pedantic -Wall -Wextra -Wformat=0 -Wno-missing-field-initializers -fno-common -fsingle-precision-constant -DNDEBUG -DBUILD_LIBGL -std=c99
FINAL_CFLAGS = -ffast-math -Ofast
LTO_FLAGS = -flto -fuse-linker-plugin
KOS_CFLAGS += $(BASE_CFLAGS) $(FINAL_CFLAGS)

KOS_CC = sh-elf-gcc
# Manipulate the CFLAGS to look our *our* version of the library and includes
INC_DIR = $(abspath ./include)
LIB_DIR = $(abspath ./)
export CFLAGS := $(CFLAGS) -I $(INC_DIR)
export OBJEXTRA := $(LIB_DIR)/libGLdc.a

link: | GL/version.h $(OBJS) ;
	sh-elf-gcc-ar qcs $(TARGET) $(OBJS)

ifeq "$(GCCVERSIONGTEQ5)" "1"
	GCC5_FLAGS = -mfsca -mfsrra -mlra
endif
KOS_CFLAGS = -fomit-frame-pointer -ml -m4-single-only -I/opt/toolchains/dc/kos/include -I/opt/toolchains/dc/kos/kernel/arch/dreamcast/include -I/opt/toolchains/dc/kos/addons/include -I/opt/toolchains/dc/kos/../kos-ports/include -D_arch_dreamcast -D_arch_sub_pristine

#can be used to check how much stack we use per function
# -fstack-usage -Wstack-usage=512
BASE_CFLAGS =  -Wall -Wextra -Wformat=0 -Wno-missing-field-initializers -Wdouble-promotion -fno-common -fsingle-precision-constant -Xlinker -Map=output.map -DGLQUAKE -DBUILD_LIBGL -std=gnu11
FINAL_CFLAGS = -O3 -ffast-math -flto -mdiv=inv -maccumulate-outgoing-args -mpretend-cmove
KOS_CFLAGS += $(BASE_CFLAGS) -O2 $(FINAL_CFLAGS)

#These produce breakage on hardware, need verification every time they are used
#-flto -funsafe-math-optimizations -mdiv=inv -mfused-madd -maccumulate-outgoing-args -ffp-contract=fast

KOS_CC = sh-elf-gcc
# Manipulate the CFLAGS to look our *our* version of the library and includes
INC_DIR = $(abspath ./include)
LIB_DIR = $(abspath ./)
export CFLAGS := $(CFLAGS) -I $(INC_DIR)
export OBJEXTRA := $(LIB_DIR)/libGLdc.a

GL/version.h:
	rm -f $@
	@echo -e '#pragma once\n#define GLDC_VERSION "EXPERIMENTAL_$(shell git describe --abbrev=4 --dirty --always --tags)"\n' > $@


build: GL/version.h $(OBJS)

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
