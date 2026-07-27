ARCH 			= linux
CDIR 			=
CC 				= gcc
COMPILER 		= $(CC)
CXX 			= g++
OPTIMIZATIONS 	= -O3 -fstrict-aliasing -DUF_NO_EXCEPTIONS
WARNINGS 		= -Wall -Wno-attributes -Wno-dangling-reference -Wno-unknown-pragmas -Wno-unused-function -Wno-unused-variable -Wno-switch -Wno-reorder -Wno-sign-compare -Wno-unused-but-set-variable -Wno-ignored-attributes -Wno-narrowing -Wno-misleading-indentation
FLAGS 			+= -fdiagnostics-color=always

TARGET_EXTENSION 		= 
DLIB_EXTENSION 			= .so
SLIB_EXTENSION 			= 