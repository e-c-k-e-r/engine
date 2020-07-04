#!/bin/bash
cd bin
cp lib/win64/*.dll .
gdb program.exe
rm *.dll

