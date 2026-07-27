ARCH 			= linux
CDIR 			=
CC 				= clang
COMPILER 		= $(CC)
CXX 			= clang++
OPTIMIZATIONS 	= -O3 -fstrict-aliasing -DUF_NO_EXCEPTIONS # -flto # -march=native 
WARNINGS 		= -Wall -Wno-pointer-arith -Wno-unused-function -Wno-unused-variable -Wno-switch -Wno-reorder-ctor -Wno-ignored-attributes -Wno-c++11-narrowing -Wno-unknown-pragmas -Wno-nullability-completeness -Wno-defaulted-function-deleted -Wno-mismatched-tags
SANITIZE 		= -fsanitize=address # -fuse-ld=lld -fno-omit-frame-pointer
FLAGS 			+= $(SANITIZE) -fcolor-diagnostics -fansi-escape-codes

TARGET_EXTENSION 		= 
DLIB_EXTENSION 			= .so
SLIB_EXTENSION 			= 