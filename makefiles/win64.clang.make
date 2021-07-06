ARCH 			= win64
CDIR 			=
CC 				= clang
CXX 			= clang++
OPTIMIZATIONS 	= -O3 -fstrict-aliasing # -flto
WARNINGS 		= -g -Wall -Wno-pointer-arith -Wno-unused-function -Wno-unused-variable -Wno-switch -Wno-reorder-ctor -Wno-ignored-attributes -Wno-c++11-narrowing -Wno-unknown-pragmas
FLAGS 			+= -std=c++17 $(OPTIMIZATIONS) $(WARNINGS) -fcolor-diagnostics -fansi-escape-codes