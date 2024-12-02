ARCH 			= win64
CDIR 			=
CC 				= gcc
CXX 			= g++
OPTIMIZATIONS 	= -O3 -g -fstrict-aliasing -DUF_NO_EXCEPTIONS # -march=native # -flto
WARNINGS 		= -Wall -Wno-unknown-pragmas -Wno-unused-function -Wno-unused-variable -Wno-switch -Wno-reorder -Wno-sign-compare -Wno-unused-but-set-variable -Wno-ignored-attributes -Wno-narrowing -Wno-misleading-indentation
FLAGS 			+= -std=c++2b $(OPTIMIZATIONS) $(WARNINGS) -fdiagnostics-color=always