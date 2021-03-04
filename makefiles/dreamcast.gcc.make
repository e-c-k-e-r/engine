ARCH 				= dreamcast
PREFIX 				= gcc
CC 					= $(KOS_CCPLUS)
TARGET_EXTENSION 	= elf
FLAGS 				+= $(KOS_CPPFLAGS) -Wno-attributes -Wno-conversion-null -fdiagnostics-color=always -std=c++17 -O3 -ffast-math -frtti -DUF_NO_EXCEPTIONS
INCS 				+= $(KOS_INC_PATHS) -I/opt/dreamcast/sh-elf/sh-elf/include
LIBS 				+= $(KOS_LIB_PATHS) -L/opt/dreamcast/sh-elf/sh-elf/lib