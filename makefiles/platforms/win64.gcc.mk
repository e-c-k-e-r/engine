ARCH 			= win64
CDIR 			=
CC 				= gcc
COMPILER 		= $(CC)
CXX 			= g++
OPTIMIZATIONS 	= -O3 -fstrict-aliasing -DUF_NO_EXCEPTIONS # -flto
WARNINGS 		= -Wall -Wno-unknown-pragmas -Wno-unused-function -Wno-unused-variable -Wno-switch -Wno-reorder -Wno-sign-compare -Wno-unused-but-set-variable -Wno-ignored-attributes -Wno-narrowing -Wno-misleading-indentation
FLAGS 			+= -fdiagnostics-color=always

# MSYS2
INCS 			+= -I/mingw64/include/
LIBS 			+= -I/mingw64/lib/

TARGET_EXTENSION 		= .exe
DLIB_EXTENSION 			= .dll
SLIB_EXTENSION 			= .a