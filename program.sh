#!/bin/bash
tskill program
cd bin

ARCH=$(cat ./exe/default/arch)
CC=$(cat ./exe/default/cc)
RENDERER=$(cat ./exe/default/renderer)

export PATH="$(pwd)/exe/lib/${ARCH}/:$(pwd)/exe/lib/${ARCH}/${CC}/:$(pwd)/exe/lib/${ARCH}/${CC}/${RENDERER}/:${PATH}"

echo PATH: ${PATH}
echo Executing ./exe/program.${ARCH}.${CC}.${RENDERER}.exe $@
./exe/program.${ARCH}.${CC}.${RENDERER}.exe $@
tskill program
