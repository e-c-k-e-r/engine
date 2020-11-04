#!/bin/bash
PREFIX=gcc

cd bin
cp lib/win64/$PREFIX/*.dll .
gdb program.$PREFIX.exe
rm *.dll

