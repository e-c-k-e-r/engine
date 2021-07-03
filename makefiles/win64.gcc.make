ARCH 			= win64
PREFIX 			= gcc
CC 				= g++
OPTIMIZATIONS 	= -O3 -fstrict-aliasing -flto
WARNINGS 		= -Wall -Wno-unknown-pragmas -Wno-unused-function -Wno-unused-variable -Wno-switch -Wno-reorder -Wno-sign-compare -Wno-unused-but-set-variable -Wno-ignored-attributes -Wno-narrowing -Wno-misleading-indentation
FLAGS 			+= -std=c++17 $(OPTIMIZATIONS) $(WARNINGS) -fdiagnostics-color=always
#-flto