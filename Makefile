# Defaults
DEFAULTS_DIR := ./makefiles/defaults
_mkdir := $(shell mkdir -p $(DEFAULTS_DIR))

ifeq ($(origin ARCH),undefined)
	ARCH := $(shell cat "$(DEFAULTS_DIR)/arch" 2>/dev/null)
endif
ifeq ($(strip $(ARCH)),)
	ifeq ($(OS),Windows_NT)
		ARCH := win64
	else
		UNAME_S := $(shell uname -s)
		ifeq ($(UNAME_S),Linux)
			ARCH := linux
		else
			$(warning Unknown host '$(UNAME_S)', defaulting ARCH to linux)
			ARCH := linux
		endif
	endif
	_write_arch := $(shell echo -n "$(ARCH)" > "$(DEFAULTS_DIR)/arch")
endif

ifeq ($(origin COMPILER),undefined)
	COMPILER := $(shell cat "$(DEFAULTS_DIR)/cc" 2>/dev/null)
endif
ifeq ($(strip $(COMPILER)),)
	ifneq ($(shell command -v gcc 2>/dev/null),)
		COMPILER := gcc
	else ifneq ($(shell command -v clang 2>/dev/null),)
		COMPILER := clang
	else
		$(warning No gcc or clang found, defaulting COMPILER to gcc)
		COMPILER := gcc
	endif
	_write_cc := $(shell echo -n "$(COMPILER)" > "$(DEFAULTS_DIR)/cc")
endif

# to-do: deduce via existence of Vulkan/OpenGL headers
ifeq ($(origin RENDERER),undefined)
	RENDERER := $(shell cat "$(DEFAULTS_DIR)/renderer" 2>/dev/null)
endif
ifeq ($(strip $(RENDERER)),)
	RENDERER := vulkan
	_write_rend := $(shell echo -n "$(RENDERER)" > "$(DEFAULTS_DIR)/renderer")
endif

TARGET_NAME			= program
TARGET_EXTENSION	= .exe
DLIB_EXTENSION		= .dll
SLIB_EXTENSION		= .a
PREFIX				= $(ARCH).$(COMPILER).$(RENDERER)

# Basic Paths
CXX 				:= $(CDIR)$(CXX)
BIN_DIR 			+= ./bin

ENGINE_SRC_DIR 		+= ./engine/src
ENGINE_INC_DIR 		+= ./engine/inc
ENGINE_LIB_DIR 		+= ./engine/lib
DEP_SRC_DIR 		+= ./dep/src
EXT_SRC_DIR 		+= ./ext
CLIENT_SRC_DIR 		+= ./client

# Base Flags
OPTIMIZATIONS 		= -O3 -fstrict-aliasing -DUF_NO_EXCEPTIONS
WARNINGS 			= -Wall -Wno-unknown-pragmas -Wno-unused-function -Wno-unused-variable -Wno-switch -Wno-reorder -Wno-sign-compare -Wno-unused-but-set-variable -Wno-ignored-attributes -Wno-narrowing -Wno-misleading-indentation
FLAGS 				+= -std=c++2b $(OPTIMIZATIONS) $(WARNINGS) -fdiagnostics-color=always

# Base Library Definitions
LIB_NAME 			+= uf
EXT_LIB_NAME 		+= ext
PREFIX_PATH 		= $(ARCH)/$(COMPILER)/$(RENDERER)
INC_DIR 			+= $(ENGINE_INC_DIR)
LIB_DIR 			+= $(ENGINE_LIB_DIR)

INCS 				+= -I$(ENGINE_INC_DIR) -I./dep/include/
LIBS 				+= -L$(ENGINE_LIB_DIR) -L$(LIB_DIR)/$(PREFIX_PATH)/ -L$(LIB_DIR)/$(ARCH)/$(COMPILER)/ -L$(LIB_DIR)/$(ARCH)/
LINKS 				+= $(UF_LIBS) $(EXT_LIBS) $(DEPS)

# DLL
SRCS_DLL 			:= $(shell find $(ENGINE_SRC_DIR) -name "*.cpp") $(shell find $(DEP_SRC_DIR) -name "*.cpp")
SRCS_DLL_C 			:= $(shell find $(ENGINE_SRC_DIR) -name "*.c") $(shell find $(DEP_SRC_DIR) -name "*.c")
OBJS_DLL 			+= $(patsubst %.cpp,%.$(PREFIX).o,$(SRCS_DLL)) $(patsubst %.c,%.$(PREFIX).o,$(SRCS_DLL_C))
BASE_DLL 			+= lib$(LIB_NAME)

IM_DLL 				+= $(ENGINE_LIB_DIR)/$(PREFIX_PATH)/$(BASE_DLL)$(DLIB_EXTENSION)
EX_DLL 				+= $(BIN_DIR)/exe/lib/$(PREFIX_PATH)/$(BASE_DLL)$(DLIB_EXTENSION)

EXT_IM_DLL 			+= $(ENGINE_LIB_DIR)/$(PREFIX_PATH)/$(BASE_EXT_DLL)$(DLIB_EXTENSION)
EXT_EX_DLL 			+= $(BIN_DIR)/exe/lib/$(PREFIX_PATH)/$(BASE_EXT_DLL)$(DLIB_EXTENSION)

SRCS_EXT_DLL 		:= $(shell find $(EXT_SRC_DIR) -name "*.cpp")
OBJS_EXT_DLL 		+= $(patsubst %.cpp,%.$(PREFIX).o,$(SRCS_EXT_DLL))
BASE_EXT_DLL 		+= lib$(EXT_LIB_NAME)

EXT_DEPS 			+= -l$(LIB_NAME) $(DEPS)
EXT_INC_DIR 		+= $(INC_DIR)
EXT_INCS 			+= $(INCS)
EXT_LIBS 			+= $(LIBS)
EXT_LINKS 			+= $(UF_LIBS) $(EXT_LIBS) $(EXT_DEPS)

# Executable
SRCS 				:= $(shell find $(CLIENT_SRC_DIR) -name "*.cpp")
OBJS 				+= $(patsubst %.cpp,%.$(PREFIX).o,$(SRCS))
TARGET 				+= $(BIN_DIR)/exe/$(TARGET_NAME).$(PREFIX)$(TARGET_EXTENSION)

# Shaders
SRCS_SHADERS 		:= $(shell find bin/data/shaders/ -name "*.glsl")
TARGET_SHADERS 		+= $(patsubst %.glsl,%.spv,$(SRCS_SHADERS))

.DEFAULT_GOAL := $(PREFIX)

.PHONY: $(PREFIX) clean run run-debug clean-shaders backup
.FORCE:

include makefiles/platforms/$(ARCH).$(COMPILER).mk

ifneq (,$(findstring win64,$(ARCH)))
	include makefiles/platforms/win64.mk
else ifneq (,$(findstring linux,$(ARCH)))
	include makefiles/platforms/linux.mk
else ifneq (,$(findstring dreamcast,$(ARCH)))
	include makefiles/platforms/dreamcast.mk
endif

include makefiles/dependencies.mk

# Build Rules
$(PREFIX): $(EX_DLL) $(EXT_EX_DLL) $(TARGET) $(TARGET_SHADERS)

%.$(PREFIX).o: %.cpp
	$(CXX) $(FLAGS) $(INCS) -c $< -o $@

%.$(PREFIX).o: %.c
	$(CC) $(FLAGS) $(INCS) -c $< -o $@

ifneq ($(ARCH),dreamcast)

$(TARGET): $(OBJS) $(EX_DLL) $(EXT_EX_DLL)
	$(CXX) $(FLAGS) $(OBJS) $(LIBS) $(INCS) $(LINKS) -l$(LIB_NAME) -l$(EXT_LIB_NAME) -o $(TARGET)
endif

%.spv: %.glsl
	$(GLSLC) --target-env=vulkan1.2 -o $@ $<
	@-$(SPV_LINTER) $@
	@-$(SPV_OPTIMIZER) --preserve-bindings --preserve-spec-constants -O $@ -o $@

shaders: $(TARGET_SHADERS)

clean:
	@-rm $(EX_DLL)
	@-rm $(EXT_EX_DLL)
	@-rm $(TARGET)
	
	@-rm -f $(OBJS_DLL)
	@-rm -f $(OBJS_EXT_DLL)
	@-rm -f $(OBJS)

clean-shaders:
	@-rm -f $(TARGET_SHADERS)

run:
	@echo -n $(ARCH) > "./bin/exe/default/arch"
	@echo -n $(COMPILER) > "./bin/exe/default/cc"
	@echo -n $(RENDERER) > "./bin/exe/default/renderer"
	./program.sh

run-debug:
	@echo -n $(ARCH) > "./bin/exe/default/arch"
	@echo -n $(COMPILER) > "./bin/exe/default/cc"
	@echo -n $(RENDERER) > "./bin/exe/default/renderer"
	./debug.sh

