#!/bin/sh
TARGET_NAME=$1
DIR=/grimgram/project/bin/exe/

rm ./build/*
sh-elf-objcopy.exe -O binary $DIR/*.elf ./$TARGET_NAME.bin
$KOS_BASE/utils/scramble/scramble.exe ./$TARGET_NAME.bin ./build/1ST_READ.bin
$KOS_BASE/utils/makeip/makeip.exe -g "$TARGET_NAME" ./build/IP.BIN
xorrisofs -C 0,11702 -V DC_GAME -G ./build/IP.BIN -r -J -l -o ./$TARGET_NAME.iso ./build/ ../data/ ./config.json
$KOS_BASE/utils/img4dc/cdi4dc.exe ./$TARGET_NAME.iso ./$TARGET_NAME.cdi > /dev/null
# cp ./$TARGET_NAME.cdi /x/programs/vidya/emulators/redream/CDI/.