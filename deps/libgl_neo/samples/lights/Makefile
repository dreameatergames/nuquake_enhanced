# Manipulate the CFLAGS to look our *our* version of the library and includes
INC_DIR = $(abspath ../../include)
LIB_DIR = $(abspath ../..)
export CFLAGS := $(CFLAGS) -I $(INC_DIR)
export OBJEXTRA := $(LIB_DIR)/libGLdc.a

TARGET = lights.elf
OBJS = main.o

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean:
	-rm -f $(TARGET) $(OBJS) romdisk.*

rm-elf:
	-rm -f $(TARGET) romdisk.*

$(TARGET): $(OBJS) romdisk.o
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LDFLAGS) -o $(TARGET) $(KOS_START) \
		$(OBJS) romdisk.o $(OBJEXTRA) -lm -lkosutils $(KOS_LIBS)

romdisk.img:
	$(KOS_GENROMFS) -f romdisk.img -d romdisk -v

romdisk.o: romdisk.img
	$(KOS_BASE)/utils/bin2o/bin2o romdisk.img romdisk romdisk.o

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist: $(TARGET)
	rm -f $(OBJS) romdisk.o romdisk.img
	$(KOS_STRIP) $(TARGET)
	$(KOS_CC_BASE)/sh-elf/bin/objcopy -R .stack -O binary $(TARGET) $(basename $(TARGET)).bin
	$(KOS_BASE)/utils/scramble/scramble $(basename $(TARGET)).bin 1ST_READ.BIN