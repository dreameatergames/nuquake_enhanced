TARGET = libminilzo.a
OBJS = minilzo.o

KOS_CFLAGS += -ffast-math -Os

.PHONY: link

link: | $(OBJS) ;
	$(KOS_AR) rcs $(CURDIR)/$(TARGET) $(CURDIR)/$(OBJS)

defaultall: create_kos_link $(OBJS) subdirs linklib

include $(KOS_BASE)/addons/Makefile.prefab

