ARCH 				= dreamcast
CDIR 				=
CC 					= gcc
CXX 				= $(KOS_CCPLUS)
RENDERER 			= opengl
TARGET_EXTENSION 	= elf
OPTIMIZATIONS 		= -Os -ffunction-sections -fdata-sections -Wl,--gc-sections -fstrict-aliasing -ffast-math -fno-unroll-all-loops -fno-optimize-sibling-calls -fschedule-insns2 -fomit-frame-pointer -DUF_NO_EXCEPTIONS -fno-exceptions -flto # -g
WARNINGS 			= -Wno-attributes -Wno-conversion-null
FLAGS 				+= $(KOS_CPPFLAGS) -m4-single -std=c++17 $(OPTIMIZATIONS) $(WARNINGS) -fdiagnostics-color=always 
INCS 				+= $(KOS_INC_PATHS) -I/opt/dreamcast/sh-elf/sh-elf/include
LIBS 				+= $(KOS_LIB_PATHS) -L/opt/dreamcast/sh-elf/sh-elf/lib