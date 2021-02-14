ARCH 					= win64
PREFIX					= gcc
TARGET_NAME 			= program
TARGET_EXTENSION 		= exe
TARGET_LIB_EXTENSION 	= dll

include makefiles/$(ARCH).$(PREFIX).make

 .PHONY: $(ARCH)-$(PREFIX)

BIN_DIR 				+= ./bin

ENGINE_SRC_DIR 			+= ./engine/src
ENGINE_INC_DIR 			+= ./engine/inc
ENGINE_LIB_DIR 			+= ./engine/lib

EXT_SRC_DIR 			+= ./ext
CLIENT_SRC_DIR 			+= ./client

UF_LIBS 				+= 
EXT_LIBS 				+=
FLAGS 				  	+= -Wno-unknown-pragmas -std=c++17 -g
LIB_NAME 				+= uf
EXT_LIB_NAME 			+= ext

#VULKAN_SDK_PATH 		+= /c/VulkanSDK/1.2.154.0/
VULKAN_SDK_PATH 		+= /c/VulkanSDK/1.2.162.0/
GLSL_VALIDATOR 			+= $(VULKAN_SDK_PATH)/Bin32/glslangValidator
# Base Engine's DLL
INC_DIR 				+= $(ENGINE_INC_DIR)/$(ARCH)/$(PREFIX)
DEPS 					+=
	
ifneq (,$(findstring win64,$(ARCH)))
	REQ_DEPS 			+= vulkan json:nlohmann png openal ogg freetype ncurses curl openvr luajit ultralight-ux bullet meshoptimizer # discord
	FLAGS 				+= 
	DEPS 				+= -lgdi32
else ifneq (,$(findstring dreamcast,$(ARCH)))
	REQ_DEPS 			+= opengl json:nlohmann #freetype ogg openal meshoptimizer # luajit ultralight-ux ncurses curl openvr discord
	FLAGS 				+= $(KOS_CPPFLAGS) -frtti -DUF_NO_EXCEPTIONS
	INCS 				+= $(KOS_INC_PATHS) -I/opt/dreamcast/sh-elf/sh-elf/include
	LIBS 				+= $(KOS_LIB_PATHS) -L/opt/dreamcast/sh-elf/sh-elf/lib
endif
ifneq (,$(findstring vulkan,$(REQ_DEPS)))
	FLAGS 				+= -DVK_USE_PLATFORM_WIN32_KHR -DUF_USE_VULKAN
	DEPS 				+= -lvulkan -lspirv-cross
endif
ifneq (,$(findstring opengl,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_OPENGL -DUF_USE_GLEW
	ifneq (,$(findstring dreamcast,$(ARCH)))
		DEPS 			+= -lGLdc
	else
		DEPS 			+= -lglew32 -lopengl32 -lglu32
	endif
endif
ifneq (,$(findstring json,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_JSON
	DEPS 				+=
	ifneq (,$(findstring nlohmann,$(REQ_DEPS)))
		FLAGS 				+= -DUF_JSON_USE_NLOHMANN
	endif
endif
ifneq (,$(findstring png,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_PNG
	DEPS 				+= -lpng -lz
endif
ifneq (,$(findstring openal,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_OPENAL
	ifneq (,$(findstring dreamcast,$(ARCH)))
		DEPS 				+= -lAL
	else
		FLAGS 				+= -DUF_USE_ALUT
		DEPS 				+= -lopenal -lalut
	endif
endif
ifneq (,$(findstring ogg,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_VORBIS
	DEPS 				+= -lvorbis -lvorbisfile -logg
endif
ifneq (,$(findstring freetype,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_FREETYPE 
	DEPS 				+= -lfreetype
endif
ifneq (,$(findstring ncurses,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_NCURSES
	DEPS 				+= -lncursesw
endif
ifneq (,$(findstring curl,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_CURL
	DEPS 				+= -lcurl
endif
ifneq (,$(findstring discord,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_DISCORD
	DEPS 				+= -ldiscord_game_sdk
endif
ifneq (,$(findstring openvr,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_OPENVR -DUSE_OPENVR_MINGW
	DEPS 				+= -lopenvr_api 
endif
ifneq (,$(findstring lua,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_LUA
	ifneq (,$(findstring luajit,$(REQ_DEPS)))
		FLAGS 				+= -DUF_USE_LUAJIT
		DEPS 				+= -lluajit-5.1
		INCS 				+= -I/mingw64/include/luajit-2.1
	else
		DEPS 				+= -llua-5.1
	endif
endif
ifneq (,$(findstring ultralight-ux,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_ULTRALIGHT_UX
	DEPS 				+= -lUltralight -lUltralightCore -lWebCore -lAppCore
endif
ifneq (,$(findstring bullet,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_BULLET
	ifneq (,$(findstring dreamcast,$(ARCH)))
		DEPS 				+= -lbulletdynamics -lbulletcollision -lbulletmath
	else
		DEPS 				+= -lBulletDynamics -lBulletCollision -lLinearMath
		INCS 				+= -I$(ENGINE_INC_DIR)/bullet/
	endif
endif
ifneq (,$(findstring simd,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_SIMD #-march=native

endif
ifneq (,$(findstring meshoptimizer,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_MESHOPTIMIZER
	DEPS 				+= -lmeshoptimizer
endif

#DEPS 					+= -lvulkan -lncursesw
LINKS 					+= $(UF_LIBS) $(EXT_LIBS) $(DEPS)
#-Wl,-subsystem,windows

LIB_DIR 				+= $(ENGINE_LIB_DIR)/$(ARCH)
INCS 					+= -I$(ENGINE_INC_DIR) -I$(INC_DIR) -I$(VULKAN_SDK_PATH)/include -I/mingw64/include
LIBS 					+= -L$(ENGINE_LIB_DIR) -L$(LIB_DIR) -L$(LIB_DIR)/$(PREFIX) -L$(VULKAN_SDK_PATH)/Lib

SRCS_DLL 				+= $(wildcard $(ENGINE_SRC_DIR)/*.cpp) $(wildcard $(ENGINE_SRC_DIR)/*/*.cpp) $(wildcard $(ENGINE_SRC_DIR)/*/*/*.cpp) $(wildcard $(ENGINE_SRC_DIR)/*/*/*/*.cpp) $(wildcard $(ENGINE_SRC_DIR)/*/*/*/*/*.cpp)
OBJS_DLL 				+= $(patsubst %.cpp,%.$(ARCH).$(PREFIX).o,$(SRCS_DLL))
BASE_DLL 				+= lib$(LIB_NAME)
IM_DLL 					+= $(ENGINE_LIB_DIR)/$(ARCH)/$(PREFIX)/$(BASE_DLL).$(TARGET_LIB_EXTENSION).a
EX_DLL 					+= $(BIN_DIR)/exe/lib/$(ARCH)/$(PREFIX)/$(BASE_DLL).$(TARGET_LIB_EXTENSION)
# External Engine's DLL
EXT_INC_DIR 			+= $(INC_DIR)
EXT_LB_FLAGS 			+= $(LIB_DIR)
EXT_DEPS 				+= -l$(LIB_NAME) $(DEPS)
EXT_LINKS 				+= $(UF_LIBS) $(EXT_LIBS) $(EXT_DEPS)
#-Wl,-subsystem,windows

EXT_LIB_DIR 			+= $(ENGINE_LIB_DIR)/$(ARCH)
EXT_INCS 				+= -I$(ENGINE_INC_DIR) -I$(EXT_INC_DIR) -I$(VULKAN_SDK_PATH)/include -I/mingw64/include
EXT_LIBS 				+= -L$(ENGINE_LIB_DIR) -L$(EXT_LIB_DIR) -L$(EXT_LIB_DIR)/$(PREFIX) -L$(VULKAN_SDK_PATH)/Lib -L/mingw64/lib

SRCS_EXT_DLL 			+= $(wildcard $(EXT_SRC_DIR)/*.cpp) $(wildcard $(EXT_SRC_DIR)/*/*.cpp) $(wildcard $(EXT_SRC_DIR)/*/*/*.cpp) $(wildcard $(EXT_SRC_DIR)/*/*/*/*.cpp) $(wildcard $(EXT_SRC_DIR)/*/*/*/*/*.cpp)
OBJS_EXT_DLL 			+= $(patsubst %.cpp,%.$(ARCH).$(PREFIX).o,$(SRCS_EXT_DLL))
BASE_EXT_DLL 			+= lib$(EXT_LIB_NAME)
EXT_IM_DLL 				+= $(ENGINE_LIB_DIR)/$(ARCH)/$(PREFIX)/$(BASE_EXT_DLL).$(TARGET_LIB_EXTENSION).a
EXT_EX_DLL 				+= $(BIN_DIR)/exe/lib/$(ARCH)/$(PREFIX)/$(BASE_EXT_DLL).$(TARGET_LIB_EXTENSION)
# Client EXE
SRCS 					+= $(wildcard $(CLIENT_SRC_DIR)/*.cpp) $(wildcard $(CLIENT_SRC_DIR)/*/*.cpp)
OBJS 					+= $(patsubst %.cpp,%.$(ARCH).$(PREFIX).o,$(SRCS))
TARGET 					+= $(BIN_DIR)/exe/$(TARGET_NAME).$(PREFIX).$(TARGET_EXTENSION)
# Shaders
SRCS_SHADERS 			+= $(wildcard bin/data/shaders/*.glsl)
TARGET_SHADERS 			+= $(patsubst %.glsl,%.spv,$(SRCS_SHADERS))

ifneq (,$(findstring dreamcast,$(ARCH)))
#$(ARCH): $(EX_DLL) $(EXT_EX_DLL) $(TARGET) ./bin/dreamcast/$(TARGET_NAME).cdi
$(ARCH): $(TARGET) ./bin/dreamcast/$(TARGET_NAME).cdi
#OBJS = $(patsubst %.cpp,%.$(ARCH).$(PREFIX).o,$(SRCS_DLL)) $(patsubst %.cpp,%.$(ARCH).$(PREFIX).o,$(SRCS_EXT_DLL)) $(patsubst %.cpp,%.$(ARCH).$(PREFIX).o,$(SRCS))
OBJS = $(patsubst %.cpp,%.$(ARCH).$(PREFIX).o,$(SRCS_DLL)) $(patsubst %.cpp,%.$(ARCH).$(PREFIX).o,$(SRCS))
DEPS 					+= -lkallisti -lc -lm -lgcc -lstdc++ -lGLdc -lAL # -l$(LIB_NAME) -l$(EXT_LIB_NAME)

%.$(ARCH).$(PREFIX).o: %.cpp
	$(CC) $(FLAGS) $(INCS) -c $< -o $@

$(EX_DLL): FLAGS += -DUF_EXPORTS -DJSON_DLL_BUILD
$(EX_DLL): $(OBJS_DLL) 
	$(KOS_AR) cru $@ $^
	$(KOS_RANLIB) $@
	cp $@ $(ENGINE_LIB_DIR)/$(ARCH)/$(PREFIX)/$(BASE_DLL).a

$(EXT_EX_DLL): FLAGS += -DEXT_EXPORTS -DJSON_DLL_BUILD
$(EXT_EX_DLL): $(OBJS_EXT_DLL) 
	$(KOS_AR) cru $@ $^
	$(KOS_RANLIB) $@
	cp $@ $(ENGINE_LIB_DIR)/$(ARCH)/$(PREFIX)/$(BASE_EXT_DLL).a

./bin/dreamcast/romdisk.img:
	$(KOS_GENROMFS) -f ./bin/dreamcast/romdisk.img -d ./bin/dreamcast/romdisk/ -v

./bin/dreamcast/romdisk.o: ./bin/dreamcast/romdisk.img
	$(KOS_BASE)/utils/bin2o/bin2o ./bin/dreamcast/romdisk.img romdisk ./bin/dreamcast/romdisk.o

$(TARGET): $(OBJS) ./bin/dreamcast/romdisk.o
	$(CC) -O2 -fomit-frame-pointer -ml -m4-single-only -ffunction-sections -fdata-sections  $(KOS_INC_PATHS) $(INCS) -D_arch_dreamcast -D_arch_sub_pristine -Wall -g -fno-builtin  -ml -m4-single-only -Wl,-Ttext=0x8c010000 -Wl,--gc-sections -T/opt/dreamcast/kos/utils/ldscripts/shlelf.xc -nodefaultlibs $(KOS_LIB_PATHS) $(LIBS) -o $(TARGET) $(OBJS) ./bin/dreamcast/romdisk.o -Wl,--start-group $(DEPS) -Wl,--end-group

./bin/dreamcast/$(TARGET_NAME).cdi: $(TARGET)
	cd ./bin/dreamcast/; ./elf2cdi.sh $(TARGET_NAME)

else
$(ARCH): $(EX_DLL) $(EXT_EX_DLL) $(TARGET) $(TARGET_SHADERS)

%.$(ARCH).$(PREFIX).o: %.cpp
	$(CC) $(FLAGS) $(INCS) -c $< -o $@

$(EX_DLL): FLAGS += -DUF_EXPORTS -DJSON_DLL_BUILD
$(EX_DLL): $(OBJS_DLL) 
	$(CC) -shared -o $(EX_DLL) -g -Wl,--out-implib=$(IM_DLL) $(OBJS_DLL) $(LIBS) $(INCS) $(LINKS)
	cp $(ENGINE_LIB_DIR)/$(ARCH)/$(PREFIX)/$(BASE_DLL).$(TARGET_LIB_EXTENSION).a $(ENGINE_LIB_DIR)/$(ARCH)/$(PREFIX)/$(BASE_DLL).a

$(EXT_EX_DLL): FLAGS += -DEXT_EXPORTS -DJSON_DLL_BUILD
$(EXT_EX_DLL): $(OBJS_EXT_DLL) 
	$(CC) -shared -o $(EXT_EX_DLL) -g -Wl,--out-implib=$(EXT_IM_DLL) $(OBJS_EXT_DLL) $(EXT_LIBS) $(EXT_INCS) $(EXT_LINKS)
	cp $(ENGINE_LIB_DIR)/$(ARCH)/$(PREFIX)/$(BASE_EXT_DLL).$(TARGET_LIB_EXTENSION).a $(ENGINE_LIB_DIR)/$(ARCH)/$(PREFIX)/$(BASE_EXT_DLL).a

$(TARGET): $(OBJS)
	$(CC) $(FLAGS) $(OBJS) $(LIBS) $(INCS) $(LINKS) -l$(LIB_NAME) -l$(EXT_LIB_NAME) -o $(TARGET)
endif

%.spv: %.glsl
	$(GLSL_VALIDATOR) -V $< -o $@

clean:
	@-rm $(EX_DLL)
	@-rm $(EXT_EX_DLL)
	@-rm $(TARGET)
	
	@-rm -f $(OBJS_DLL)
	@-rm -f $(OBJS_EXT_DLL)
	@-rm -f $(OBJS)

clean-uf:
	@-rm $(EX_DLL)
	@-rm -f $(OBJS_DLL)

clean-exf:
	@-rm $(EXT_EX_DLL)
	@-rm -f $(OBJS_EXT_DLL)

clean-exe:
	-rm $(EX_DLL)
	-rm $(EXT_EX_DLL)
	-rm $(TARGET)
	-rm $(TARGET_SHADERS)