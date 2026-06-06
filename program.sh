#!/bin/bash
cd bin

ARCH=$(cat ./exe/default/arch)
CC=$(cat ./exe/default/cc)
RENDERER=$(cat ./exe/default/renderer)

export PATH="$(pwd)/exe/lib/${ARCH}/:$(pwd)/exe/lib/${ARCH}/${CC}/:$(pwd)/exe/lib/${ARCH}/${CC}/${RENDERER}/:${PATH}"
export LD_LIBRARY_PATH="$(pwd)/exe/lib/${ARCH}/:$(pwd)/exe/lib/${ARCH}/${CC}/:$(pwd)/exe/lib/${ARCH}/${CC}/${RENDERER}/:${LD_LIBRARY_PATH}"

echo PATH: ${PATH}
echo Executing ./exe/program.${ARCH}.${CC}.${RENDERER}.exe $@
./exe/program.${ARCH}.${CC}.${RENDERER}.exe $@
tskill program
