PREFIX					= gcc
include makefiles/win64.$(PREFIX).make

 .PHONY: $(ARCH)-$(PREFIX)

TARGET_NAME 			+= program
BIN_DIR 				+= ./bin

ENGINE_SRC_DIR 			+= ./engine/src
ENGINE_INC_DIR 			+= ./engine/inc
ENGINE_LIB_DIR 			+= ./engine/lib

EXT_SRC_DIR 			+= ./ext
CLIENT_SRC_DIR 			+= ./client

UF_LIBS 				+= 
EXT_LIBS 				+=
FLAGS 				  	+= -Wno-unknown-pragmas -std=c++17 -g -DVK_USE_PLATFORM_WIN32_KHR -DUF_USE_VULKAN -DGLM_ENABLE_EXPERIMENTAL -DUF_USE_JSON -DUF_USE_NCURSES -DUF_USE_OPENAL -DUF_USE_VORBIS -DUF_USE_FREETYPE -DUSE_OPENVR_MINGW
LIB_NAME 				+= uf
EXT_LIB_NAME 			+= ext

#VULKAN_SDK_PATH 		+= /c/VulkanSDK/1.1.101.0/
#VULKAN_SDK_PATH 		+= /c/VulkanSDK/1.1.108.0/
#VULKAN_SDK_PATH 		+= /c/VulkanSDK/1.1.114.0/
#VULKAN_SDK_PATH 		+= /c/VulkanSDK/1.2.141.2/
VULKAN_SDK_PATH 		+= /c/VulkanSDK/1.2.154.0/
GLSL_VALIDATOR 			+= $(VULKAN_SDK_PATH)/Bin32/glslangValidator
# Base Engine's DLL
INC_DIR 				+= $(ENGINE_INC_DIR)/$(ARCH)/$(PREFIX)
LB_FLAGS 				+= $(ENGINE_LIB_DIR)/$(ARCH)/$(PREFIX)
DEPS 					+= -lgdi32 -lvulkan -lspirv-cross -lpng -lz -ljsoncpp -lopenal -lalut -lvorbis -lvorbisfile -logg -lfreetype -lncursesw -lcurl -ldiscord_game_sdk -lopenvr_api -lluajit-5.1
#DEPS 					+= -lvulkan -lncursesw
LINKS 					+= $(UF_LIBS) $(EXT_LIBS) $(DEPS)
#-Wl,-subsystem,windows

LIB_DIR 				+= $(ENGINE_LIB_DIR)/$(ARCH)/$(PREFIX)
INCS 					+= -I$(ENGINE_INC_DIR) -I$(INC_DIR) -I$(VULKAN_SDK_PATH)/include -I/mingw64/include -I/mingw64/include/luajit-2.1
LIBS 					+= -L$(ENGINE_LIB_DIR) -L$(LIB_DIR) -L$(VULKAN_SDK_PATH)/Lib -L/mingw64/lib

SRCS_DLL 				+= $(wildcard $(ENGINE_SRC_DIR)/*.cpp) $(wildcard $(ENGINE_SRC_DIR)/*/*.cpp) $(wildcard $(ENGINE_SRC_DIR)/*/*/*.cpp) $(wildcard $(ENGINE_SRC_DIR)/*/*/*/*.cpp) $(wildcard $(ENGINE_SRC_DIR)/*/*/*/*/*.cpp)
OBJS_DLL 				+= $(patsubst %.cpp,%.$(ARCH).$(PREFIX).o,$(SRCS_DLL))
BASE_DLL 				+= lib$(LIB_NAME)
IM_DLL 					+= $(ENGINE_LIB_DIR)/$(ARCH)/$(PREFIX)/$(BASE_DLL).dll.a
EX_DLL 					+= $(BIN_DIR)/exe/lib/$(ARCH)/$(PREFIX)/$(BASE_DLL).dll
# External Engine's DLL
EXT_INC_DIR 			+= $(INC_DIR)
EXT_LB_FLAGS 			+= $(LIB_DIR)
EXT_DEPS 				+= -l$(LIB_NAME) $(DEPS)
EXT_LINKS 				+= $(UF_LIBS) $(EXT_LIBS) $(EXT_DEPS)
#-Wl,-subsystem,windows

EXT_LIB_DIR 			+= $(ENGINE_LIB_DIR)/$(ARCH)/$(PREFIX)/
EXT_INCS 				+= -I$(ENGINE_INC_DIR) -I$(EXT_INC_DIR) -I$(VULKAN_SDK_PATH)/include -I/mingw64/include
EXT_LIBS 				+= -L$(ENGINE_LIB_DIR) -L$(EXT_LIB_DIR) -L$(VULKAN_SDK_PATH)/Lib -L/mingw64/lib

SRCS_EXT_DLL 			+= $(wildcard $(EXT_SRC_DIR)/*.cpp) $(wildcard $(EXT_SRC_DIR)/*/*.cpp) $(wildcard $(EXT_SRC_DIR)/*/*/*.cpp) $(wildcard $(EXT_SRC_DIR)/*/*/*/*.cpp) $(wildcard $(EXT_SRC_DIR)/*/*/*/*/*.cpp)
OBJS_EXT_DLL 			+= $(patsubst %.cpp,%.$(ARCH).$(PREFIX).o,$(SRCS_EXT_DLL))
BASE_EXT_DLL 			+= lib$(EXT_LIB_NAME)
EXT_IM_DLL 				+= $(ENGINE_LIB_DIR)/$(ARCH)/$(PREFIX)/$(BASE_EXT_DLL).dll.a
EXT_EX_DLL 				+= $(BIN_DIR)/exe/lib/$(ARCH)/$(PREFIX)/$(BASE_EXT_DLL).dll
# Client EXE
SRCS 					+= $(wildcard $(CLIENT_SRC_DIR)/*.cpp) $(wildcard $(CLIENT_SRC_DIR)/*/*.cpp)
OBJS 					+= $(patsubst %.cpp,%.$(ARCH).$(PREFIX).o,$(SRCS))
TARGET 					+= $(BIN_DIR)/exe/$(TARGET_NAME).$(PREFIX).exe
# Shaders
SRCS_SHADERS 			+= $(wildcard bin/data/shaders/*.glsl)
TARGET_SHADERS 			+= $(patsubst %.glsl,%.spv,$(SRCS_SHADERS))

$(ARCH): $(EX_DLL) $(EXT_EX_DLL) $(TARGET) $(TARGET_SHADERS)

rm-exe64:
	-rm $(EX_DLL)
	-rm $(EXT_EX_DLL)
	-rm $(TARGET)
	-rm $(TARGET_SHADERS)

%.$(ARCH).$(PREFIX).o: %.cpp
	$(CC) $(FLAGS) $(INCS) -c $< -o $@

$(EX_DLL): FLAGS += -DUF_EXPORTS -DJSON_DLL_BUILD
$(EX_DLL): $(OBJS_DLL) 
	$(CC) -shared -o $(EX_DLL) -g -Wl,--out-implib=$(IM_DLL) $(OBJS_DLL) $(LIBS) $(INCS) $(LINKS)
	cp $(ENGINE_LIB_DIR)/$(ARCH)/$(PREFIX)/$(BASE_DLL).dll.a $(ENGINE_LIB_DIR)/$(ARCH)/$(PREFIX)/$(BASE_DLL).a

$(EXT_EX_DLL): FLAGS += -DEXT_EXPORTS -DJSON_DLL_BUILD
$(EXT_EX_DLL): $(OBJS_EXT_DLL) 
	$(CC) -shared -o $(EXT_EX_DLL) -g -Wl,--out-implib=$(EXT_IM_DLL) $(OBJS_EXT_DLL) $(EXT_LIBS) $(EXT_INCS) $(EXT_LINKS)
	cp $(ENGINE_LIB_DIR)/$(ARCH)/$(PREFIX)/$(BASE_EXT_DLL).dll.a $(ENGINE_LIB_DIR)/$(ARCH)/$(PREFIX)/$(BASE_EXT_DLL).a

$(TARGET): $(OBJS)
	$(CC) $(FLAGS) $(OBJS) $(LIBS) $(INCS) $(LINKS) -l$(LIB_NAME) -l$(EXT_LIB_NAME) -o $(TARGET)

%.spv: %.glsl
	$(GLSL_VALIDATOR) -V $< -o $@

clean-$(ARCH):
	@-rm $(EX_DLL)
	@-rm $(EXT_EX_DLL)
	@-rm $(TARGET)
	
	@-rm -f $(OBJS_DLL)
	@-rm -f $(OBJS_EXT_DLL)
	@-rm -f $(OBJS)

clean-uf-$(ARCH):
	@-rm $(EX_DLL)
	@-rm -f $(OBJS_DLL)

clean-ext-$(ARCH):
	@-rm $(EXT_EX_DLL)
	@-rm -f $(OBJS_EXT_DLL)