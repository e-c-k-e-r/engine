ARCH 				= dreamcast
CDIR 				=
COMPILER 			= gcc
CC 					= $(KOS_CC)
CXX 				= $(KOS_CCPLUS)
RENDERER 			= opengl
OPTIMIZATIONS 		= -Os -ffunction-sections -fdata-sections -Wl,--gc-sections -fstrict-aliasing -ffast-math -fno-unroll-all-loops -fno-optimize-sibling-calls -fschedule-insns2 -fomit-frame-pointer -DUF_NO_EXCEPTIONS -fno-exceptions -g -flto=auto -ffat-lto-objects
WARNINGS 			= -Wall -Wno-unknown-pragmas -Wno-unused-function -Wno-unused-variable -Wno-switch -Wno-reorder -Wno-sign-compare -Wno-unused-but-set-variable -Wno-ignored-attributes -Wno-narrowing -Wno-misleading-indentation -Wno-attributes -Wno-conversion-null
FLAGS 				+= $(KOS_CPPFLAGS) -m4-single -std=c++2b  $(OPTIMIZATIONS) $(WARNINGS) -fdiagnostics-color=always 

TARGET_EXTENSION 	= .elf

# KOS
INCS 				:= $(KOS_INC_PATHS) -I/opt/dreamcast/sh-elf/sh-elf/include $(INCS)
LIBS 				:= $(KOS_LIB_PATHS) -L/opt/dreamcast/sh-elf/sh-elf/lib $(LIBS)