.PHONY: win64

TARGET_NAME 			= program
BIN_DIR 				= ./bin

ENGINE_SRC_DIR 			= ./engine/src
ENGINE_INC_DIR 			= ./engine/inc
ENGINE_LIB_DIR 			= ./engine/lib

EXT_SRC_DIR 			= ./ext
CLIENT_SRC_DIR 			= ./client

UF_LIBS 				= 
# EXT_LIBS 	 			= -lpng16 -lz -lsfml-main -lsfml-system -lsfml-window -lsfml-graphics
# EXT_LIBS 	 			= -lpng16 -lz -lassimp -lsfml-main -lsfml-system -lsfml-window -lsfml-graphics -llua52
# EXT_LIBS 	 			= -lpng16 -lz -lassimp -ljsoncpp -lopenal32 -lalut -lvorbis -lvorbisfile -logg -lfreetype
EXT_LIBS 				=
#FLAGS 					= -std=c++0x -Wall -g -DUF_USE_JSON -DUF_USE_NCURSES -DUF_USE_OPENGL -DUF_USE_GLEW
FLAGS 					= -std=c++20 -Wno-c++11-narrowing -Wno-narrowing -g -DVK_USE_PLATFORM_WIN32_KHR -DUF_USE_VULKAN -DGLM_ENABLE_EXPERIMENTAL -DUF_USE_JSON -DUF_USE_NCURSES -DUF_USE_OPENAL -DUF_USE_VORBIS -DUF_USE_FREETYPE -DUSE_OPENVR_MINGW
#-march=native
LIB_NAME 				= uf
EXT_LIB_NAME 			= ext


#VULKAN_WIN64_SDK_PATH 	= /c/VulkanSDK/1.1.101.0/
#VULKAN_WIN64_SDK_PATH 	= /c/VulkanSDK/1.1.108.0/
#VULKAN_WIN64_SDK_PATH 	= /c/VulkanSDK/1.1.114.0/
VULKAN_WIN64_SDK_PATH 	= /c/VulkanSDK/1.2.141.2/
WIN64_CC 				= g++
WIN64_GLSL_VALIDATOR 	= $(VULKAN_WIN64_SDK_PATH)/Bin32/glslangValidator
# Base Engine's DLL
WIN64_INC_DIR 			= $(ENGINE_INC_DIR)/win64
WIN64_LB_FLAGS 			= $(ENGINE_LIB_DIR)/win64
WIN64_DEPS 				= -lgdi32 -lvulkan -lpng -lz -ljsoncpp -lopenal -lalut -lvorbis -lvorbisfile -logg -lfreetype -lncursesw -lcurl -ldiscord_game_sdk -lopenvr_api
#WIN64_DEPS 			= -lvulkan -lncursesw
WIN64_LINKS 			= $(UF_LIBS) $(EXT_LIBS) $(WIN64_DEPS)
WIN64_FLAGS 			= $(FLAGS) -g
#-Wl,-subsystem,windows

WIN64_LIB_DIR 			= $(ENGINE_LIB_DIR)/win64/
WIN64_INCS 				= -I$(ENGINE_INC_DIR) -I$(WIN64_INC_DIR) -I$(VULKAN_WIN64_SDK_PATH)/include
WIN64_LIBS 				= -L$(ENGINE_LIB_DIR) -L$(WIN64_LIB_DIR) -L$(VULKAN_WIN64_SDK_PATH)/Lib

SRCS_WIN64_DLL 			= $(wildcard $(ENGINE_SRC_DIR)/*.cpp) $(wildcard $(ENGINE_SRC_DIR)/*/*.cpp) $(wildcard $(ENGINE_SRC_DIR)/*/*/*.cpp) $(wildcard $(ENGINE_SRC_DIR)/*/*/*/*.cpp) $(wildcard $(ENGINE_SRC_DIR)/*/*/*/*/*.cpp)
OBJS_WIN64_DLL 			= $(patsubst %.cpp,%.win64.o,$(SRCS_WIN64_DLL))
BASE_WIN64_DLL 			= lib$(LIB_NAME)
IM_WIN64_DLL 			= $(ENGINE_LIB_DIR)/win64/$(BASE_WIN64_DLL).dll.a
EX_WIN64_DLL 			= $(BIN_DIR)/lib/win64/$(BASE_WIN64_DLL).dll
# External Engine's DLL
EXT_WIN64_INC_DIR 		= $(WIN64_INC_DIR)
EXT_WIN64_LB_FLAGS 		= $(WIN64_LIB_DIR)
EXT_WIN64_DEPS 			= -l$(LIB_NAME) $(WIN64_DEPS)
EXT_WIN64_LINKS 		= $(UF_LIBS) $(EXT_LIBS) $(EXT_WIN64_DEPS)
EXT_WIN64_FLAGS 		= $(FLAGS) 
#-Wl,-subsystem,windows

EXT_WIN64_LIB_DIR 		= $(ENGINE_LIB_DIR)/win64/
EXT_WIN64_INCS 			= -I$(ENGINE_INC_DIR) -I$(EXT_WIN64_INC_DIR) -I$(VULKAN_WIN64_SDK_PATH)/include
EXT_WIN64_LIBS 			= -L$(ENGINE_LIB_DIR) -L$(EXT_WIN64_LIB_DIR) -L$(VULKAN_WIN64_SDK_PATH)/Lib

SRCS_EXT_WIN64_DLL 		= $(wildcard $(EXT_SRC_DIR)/*.cpp) $(wildcard $(EXT_SRC_DIR)/*/*.cpp) $(wildcard $(EXT_SRC_DIR)/*/*/*.cpp) $(wildcard $(EXT_SRC_DIR)/*/*/*/*.cpp)
OBJS_EXT_WIN64_DLL 		= $(patsubst %.cpp,%.win64.o,$(SRCS_EXT_WIN64_DLL))
BASE_EXT_WIN64_DLL 		= lib$(EXT_LIB_NAME)
EXT_IM_WIN64_DLL 		= $(ENGINE_LIB_DIR)/win64/$(BASE_EXT_WIN64_DLL).dll.a
EXT_EX_WIN64_DLL 		= $(BIN_DIR)/lib/win64/$(BASE_EXT_WIN64_DLL).dll
# Client EXE
SRCS_WIN64 				= $(wildcard $(CLIENT_SRC_DIR)/*.cpp) $(wildcard $(CLIENT_SRC_DIR)/*/*.cpp)
OBJS_WIN64 				= $(patsubst %.cpp,%.win64.o,$(SRCS_WIN64))
TARGET_WIN64 			= $(BIN_DIR)/$(TARGET_NAME).exe
# Shaders
SRCS_SHADERS 			= $(wildcard bin/data/shaders/*.glsl)
TARGET_SHADERS 			= $(patsubst %.glsl,%.spv,$(SRCS_SHADERS))

# clang-win64: WIN64_CC=clang++
#	make
# gcc-win64: WIN64_CC=g++
#	make
win64: $(EX_WIN64_DLL) $(EXT_EX_WIN64_DLL) $(TARGET_WIN64) $(TARGET_SHADERS)

rm-exe64:
	-rm $(EX_WIN64_DLL)
	-rm $(EXT_EX_WIN64_DLL)
	-rm $(TARGET_WIN64)
	-rm $(TARGET_SHADERS)

%.win64.o: %.cpp
	$(WIN64_CC) $(WIN64_FLAGS) $(WIN64_INCS) -c $< -o $@

$(EX_WIN64_DLL): WIN64_FLAGS += -DUF_EXPORTS
$(EX_WIN64_DLL): $(OBJS_WIN64_DLL) 
	$(WIN64_CC) -shared -o $(EX_WIN64_DLL) -g -Wl,--out-implib=$(IM_WIN64_DLL) $(OBJS_WIN64_DLL) $(WIN64_LIBS) $(WIN64_INCS) $(WIN64_LINKS)
	cp $(ENGINE_LIB_DIR)/win64/$(BASE_WIN64_DLL).dll.a $(ENGINE_LIB_DIR)/win64/$(BASE_WIN64_DLL).a

$(EXT_EX_WIN64_DLL): WIN64_FLAGS += -DEXT_EXPORTS
$(EXT_EX_WIN64_DLL): $(OBJS_EXT_WIN64_DLL) 
	$(WIN64_CC) -shared -o $(EXT_EX_WIN64_DLL) -g -Wl,--out-implib=$(EXT_IM_WIN64_DLL) $(OBJS_EXT_WIN64_DLL) $(EXT_WIN64_LIBS) $(EXT_WIN64_INCS) $(EXT_WIN64_LINKS)
	cp $(ENGINE_LIB_DIR)/win64/$(BASE_EXT_WIN64_DLL).dll.a $(ENGINE_LIB_DIR)/win64/$(BASE_EXT_WIN64_DLL).a

$(TARGET_WIN64): $(OBJS_WIN64)
	$(WIN64_CC) $(WIN64_FLAGS) $(OBJS_WIN64) $(WIN64_LIBS) $(WIN64_INCS) $(WIN64_LINKS) -l$(LIB_NAME) -l$(EXT_LIB_NAME) -o $(TARGET_WIN64)

%.spv: %.glsl
	$(WIN64_GLSL_VALIDATOR) -V $< -o $@

clean-win64:
	@-rm $(EX_WIN64_DLL)
	@-rm $(EXT_EX_WIN64_DLL)
	@-rm $(TARGET_WIN64)
	
	@-rm -f $(OBJS_WIN64_DLL)
	@-rm -f $(OBJS_EXT_WIN64_DLL)
	@-rm -f $(OBJS_WIN64)
