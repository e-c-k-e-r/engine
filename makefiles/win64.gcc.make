ARCH 			= win64
CDIR 			=
CC 				= gcc
CXX 			= g++
OPTIMIZATIONS 	= -O3 -g -fstrict-aliasing #-flto
WARNINGS 		= -Wall -Wno-unknown-pragmas -Wno-unused-function -Wno-unused-variable -Wno-switch -Wno-reorder -Wno-sign-compare -Wno-unused-but-set-variable -Wno-ignored-attributes -Wno-narrowing -Wno-misleading-indentation
FLAGS 			+= -std=c++17 $(OPTIMIZATIONS) $(WARNINGS) -fdiagnostics-color=always