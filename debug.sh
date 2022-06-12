#!/bin/bash
cd bin
ARCH=$(cat ./exe/default/arch)
CC=$(cat ./exe/default/cc)
RENDERER=$(cat ./exe/default/renderer)
cp ./exe/lib/$ARCH/*.dll .
cp ./exe/lib/$ARCH/$CC/$RENDERER/*.dll .
gdb ./exe/program.$ARCH.$CC.$RENDERER.exe
rm *.dll

