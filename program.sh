#!/bin/bash
tskill program
cd bin

ARCH=$(cat ./exe/default/arch)
CC=$(cat ./exe/default/cc)
RENDERER=$(cat ./exe/default/renderer)

export PATH="$(pwd)/exe/lib/${ARCH}/:$(pwd)/exe/lib/${ARCH}/${CC}/${RENDERER}/:${PATH}"

./exe/program.${ARCH}.${CC}.${RENDERER}.exe $@
tskill program
