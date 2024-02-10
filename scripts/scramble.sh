#!/bin/sh

#cd "${MESON_SOURCE_ROOT}"
cd "${MESON_BUILD_ROOT}"

BUILD_DIR=$1
BINARY=$2

/opt/toolchains/dc/sh-elf/bin/sh-elf-objcopy -R .stack -O binary $BINARY `basename $BINARY`.RAW
/opt/toolchains/dc/kos/utils/scramble/scramble `basename $BINARY`.RAW ../$BUILD_DIR/data_hb/1ST_READ.BIN
