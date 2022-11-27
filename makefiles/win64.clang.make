ARCH 			= win64
CDIR 			=
CC 				= clang
CXX 			= clang++
OPTIMIZATIONS 	= -O3 -fstrict-aliasing -DUF_NO_EXCEPTIONS -flto # -march=native 
WARNINGS 		= -g -Wall -Wno-pointer-arith -Wno-unused-function -Wno-unused-variable -Wno-switch -Wno-reorder-ctor -Wno-ignored-attributes -Wno-c++11-narrowing -Wno-unknown-pragmas -Wno-nullability-completeness -Wno-defaulted-function-deleted -Wno-mismatched-tags
SANITIZE 		= -fuse-ld=lld # -fsanitize=address -fno-omit-frame-pointer
FLAGS 			+= -std=c++17 $(OPTIMIZATIONS) $(WARNINGS) $(SANITIZE) -fcolor-diagnostics -fansi-escape-codes