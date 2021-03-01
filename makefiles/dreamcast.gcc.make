ARCH 		= dreamcast
PREFIX 		= gcc
CC 			= $(KOS_CCPLUS)
FLAGS 		= -Wno-attributes -Wno-conversion-null -fdiagnostics-color=always -DUF_DISABLE_ALIGNAS -std=gnu++17
TARGET_EXTENSION = elf