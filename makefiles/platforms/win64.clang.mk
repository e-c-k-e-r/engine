ARCH 			= win64
CDIR 			=
CC 				= clang
COMPILER 		= $(CC)
CXX 			= clang++
OPTIMIZATIONS 	= -O3 -fstrict-aliasing -DUF_NO_EXCEPTIONS # -flto # -march=native 
WARNINGS 		= -Wall -Wno-missing-braces -Wno-deprecated-literal-operator -Wno-pointer-arith -Wno-unused-function -Wno-unused-variable -Wno-switch -Wno-reorder-ctor -Wno-ignored-attributes -Wno-c++11-narrowing -Wno-unknown-pragmas -Wno-nullability-completeness -Wno-defaulted-function-deleted -Wno-mismatched-tags
SANITIZE 		= -fuse-ld=lld # -fsanitize=address,undefined # -fuse-ld=lld -fno-omit-frame-pointer
FLAGS 			+= -std=c++2b $(OPTIMIZATIONS) $(WARNINGS) $(SANITIZE) -fcolor-diagnostics -fansi-escape-codes

# MSYS2
INCS 			+= -I/clang64/include/ 
LIBS 			+= -L/clang64/lib/ -L/mingw64/lib/

TARGET_EXTENSION 		= .exe
DLIB_EXTENSION 			= .dll
SLIB_EXTENSION 			= .a