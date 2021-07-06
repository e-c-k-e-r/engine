ARCH 				= dreamcast
CDIR 				=
CC 					= gcc
CXX 				= $(KOS_CCPLUS)
TARGET_EXTENSION 	= elf
# no flto because it throws an error "undefined reference to `typeinfo for btPolyhedralConvexShape'"
OPTIMIZATIONS 		= -O3 -fstrict-aliasing -ffast-math -DUF_NO_EXCEPTIONS # -flto 
WARNINGS 			= -Wno-attributes -Wno-conversion-null
FLAGS 				+= $(KOS_CPPFLAGS) -std=c++17 $(OPTIMIZATIONS) $(WARNINGS) -fdiagnostics-color=always 
INCS 				+= $(KOS_INC_PATHS) -I/opt/dreamcast/sh-elf/sh-elf/include
LIBS 				+= $(KOS_LIB_PATHS) -L/opt/dreamcast/sh-elf/sh-elf/lib