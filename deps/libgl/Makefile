# KallistiOS ##version##
#
# kos-ports/libgl Makefile
# Copyright (C) 2013, 2014 Josh Pearson
# Copyright (C) 2014 Lawrence Sebald
# Copyright (C) 2020 Luke Benstead
# Copyright (c) 2020 HaydenKow

TARGET = libGLdc.a
OBJS = GL/gldc.o
OBJS = GL/draw.o GL/flush.o GL/framebuffer.o GL/immediate.o GL/lighting.o GL/state.o GL/texture.o GL/glu.o
OBJS += GL/matrix.o GL/fog.o GL/error.o GL/clip.o containers/stack.o containers/named_array.o containers/aligned_vector.o GL/profiler.o
OBJS += containers/stack.o containers/named_array.o containers/aligned_vector.o GL/profiler.o

SUBDIRS =

EXTRA_CFLAGS = -mpretend-cmove -Wall -Wextra -Wformat=0 -Wno-missing-field-initializers -fsingle-precision-constant 
KOS_CFLAGS +=  -ffast-math -Ofast -Iinclude -DNDEBUG -DBUILD_LIBGL $(EXTRA_CFLAGS)

GL/version.h:
	rm -f $@
	@echo -e '#pragma once\n#define GLDC_VERSION "$(shell git describe --abbrev=4 --dirty --always --tags)"\n' > $@

link:
	$(KOS_AR) rcs $(TARGET) $(OBJS)

build: GL/version.h $(OBJS) link


samples: build
	$(KOS_MAKE) -C samples all

defaultall: create_kos_link $(OBJS) subdirs linklib samples

include $(KOS_BASE)/addons/Makefile.prefab

# creates the kos link to the headers
create_kos_link:
	rm -f ../include/GL
	ln -s ../GLdc/include ../include/GL
