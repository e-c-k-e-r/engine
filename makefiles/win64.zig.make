ARCH 			= win64
CDIR 			=
CC 				= /opt/zig/zig cc
CXX 			= /opt/zig/zig c++
OPTIMIZATIONS 	= -O3 -fstrict-aliasing #-flto
WARNINGS 		= -g -Wall -Wno-pointer-arith -Wno-unused-function -Wno-unused-variable -Wno-switch -Wno-reorder-ctor -Wno-ignored-attributes -Wno-c++11-narrowing -Wno-unknown-pragmas -Wno-nullability-completeness -Wno-defaulted-function-deleted -Wno-mismatched-tags
SANITIZE 		= #-fuse-ld=lld -fsanitize=address -fno-omit-frame-pointer
FLAGS 			+= -std=c++17 $(OPTIMIZATIONS) $(WARNINGS) $(SANITIZE) -I./dep/zig/include/ -target x86_64-windows-gnu