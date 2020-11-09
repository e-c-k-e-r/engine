#!/bin/bash
cd bin
PREFIX=$(cat ./exe/default.config)
cp ./exe/lib/win64/$PREFIX/*.dll .
gdb ./exe/program.$PREFIX.exe
rm *.dll

