#!/bin/bash
cd bin

ARCH=$(cat ./exe/default/arch)
CC=$(cat ./exe/default/cc)
RENDERER=$(cat ./exe/default/renderer)

export PATH="$(pwd)/exe/lib/${ARCH}/:$(pwd)/exe/lib/${ARCH}/${CC}/:$(pwd)/exe/lib/${ARCH}/${CC}/${RENDERER}/:${PATH}"
export LD_LIBRARY_PATH="$(pwd)/exe/lib/${ARCH}/:$(pwd)/exe/lib/${ARCH}/${CC}/:$(pwd)/exe/lib/${ARCH}/${CC}/${RENDERER}/:${LD_LIBRARY_PATH}"

echo PATH: ${PATH}
NAME=./exe/program.${ARCH}.${CC}.${RENDERER}
# Windows builds carry a .exe suffix; platform builds (e.g. Linux) do not
[ -f "${NAME}.exe" ] && NAME="${NAME}.exe"
echo Executing ${NAME} $@
${NAME} $@
tskill program
