ARCH 					= win64
CC						= $(shell cat "./bin/exe/default.config")
TARGET_NAME 			= program
TARGET_EXTENSION 		= exe
TARGET_LIB_EXTENSION 	= dll
RENDERER 				= vulkan
PREFIX 					= $(ARCH).$(CC)

include makefiles/$(PREFIX).make

.PHONY: $(ARCH)-$(CC)

.FORCE:

CXX 					:= $(CDIR)$(CXX)
BIN_DIR 				+= ./bin

ENGINE_SRC_DIR 			+= ./engine/src
ENGINE_INC_DIR 			+= ./engine/inc
ENGINE_LIB_DIR 			+= ./engine/lib

EXT_SRC_DIR 			+= ./ext
CLIENT_SRC_DIR 			+= ./client

UF_LIBS 				+= 
EXT_LIBS 				+=
FLAGS 				  	+= 
LIB_NAME 				+= uf
EXT_LIB_NAME 			+= ext

7Z 						+= /c/Program\ Files/7-Zip/7z.exe

#VULKAN_SDK_PATH 		+= /c/VulkanSDK/1.2.154.0/
#VULKAN_SDK_PATH 		+= /c/VulkanSDK/1.2.162.0/
#VULKAN_SDK_PATH 		+= /c/VulkanSDK/1.2.176.1/
#VULKAN_SDK_PATH 		+= /c/VulkanSDK/1.2.182.0/
#VULKAN_SDK_PATH 		+= /c/VulkanSDK/1.2.198.1/
#VULKAN_SDK_PATH 		+= /c/VulkanSDK/1.3.204.1/
VULKAN_SDK_PATH 		+= /c/VulkanSDK/1.3.211.0/

#GLSLC 		+= $(VULKAN_SDK_PATH)/Bin/glslangValidator
GLSLC 					+= $(VULKAN_SDK_PATH)/Bin/glslc
SPV_OPTIMIZER 			+= $(VULKAN_SDK_PATH)/Bin/spirv-opt
# Base Engine's DLL
INC_DIR 				+= $(ENGINE_INC_DIR)/$(ARCH)/$(CC)
LIB_DIR 				+= $(ENGINE_LIB_DIR)/$(ARCH)

INCS 					+= -I$(ENGINE_INC_DIR) -I$(INC_DIR) -I./dep/ #-I/mingw64/include 
LIBS 					+= -L$(ENGINE_LIB_DIR) -L$(LIB_DIR) -L$(LIB_DIR)/$(CC)
	
LINKS 					+= $(UF_LIBS) $(EXT_LIBS) $(DEPS)
DEPS 					+=

ifneq (,$(findstring win64,$(ARCH)))
	REQ_DEPS 			+= $(RENDERER) json:nlohmann png zlib openal ogg freetype curl luajit reactphysics meshoptimizer xatlas simd ctti gltf # ncurses openvr draco discord bullet ultralight-ux
	FLAGS 				+= 
	DEPS 				+= -lgdi32
else ifneq (,$(findstring dreamcast,$(ARCH)))
	REQ_DEPS 			+= simd opengl gldc json:nlohmann lua reactphysics png zlib ctti ogg openal aldc # gltf freetype bullet meshoptimizer draco luajit ultralight-ux ncurses curl openvr discord
endif
ifneq (,$(findstring vulkan,$(REQ_DEPS)))
	FLAGS 				+= -DVK_USE_PLATFORM_WIN32_KHR -DUF_USE_VULKAN
	DEPS 				+= -lvulkan -lspirv-cross #-lVulkanMemoryAllocator
	INCS 				+= -I$(VULKAN_SDK_PATH)/include
	LIBS 				+= -L$(VULKAN_SDK_PATH)/Lib
endif
ifneq (,$(findstring opengl,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_OPENGL -DUF_USE_OPENGL_FIXED_FUNCTION

	ifneq (,$(findstring dreamcast,$(ARCH)))
		ifneq (,$(findstring gldc,$(REQ_DEPS)))
			DEPS 		+= -lGLdc
			FLAGS 		+= -DUF_USE_OPENGL_GLDC
		else
			DEPS 		+= -lGL
		endif
	else
		ifneq (,$(findstring gldc,$(REQ_DEPS)))
			DEPS 			+= -lGLdc -lSDL2
			FLAGS 			+= -DUF_USE_OPENGL_GLDC
		else
			FLAGS 			+= -DUF_USE_GLEW
			DEPS 			+= -lglew32 -lopengl32 -lglu32
		endif
	endif

endif
ifneq (,$(findstring json,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_JSON
	DEPS 				+=
	ifneq (,$(findstring nlohmann,$(REQ_DEPS)))
		FLAGS 			+= -DUF_JSON_USE_NLOHMANN
	endif
endif
ifneq (,$(findstring gltf,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_GLTF
endif
ifneq (,$(findstring png,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_PNG -DUF_USE_ZLIB
	DEPS 				+= -lpng -lz
endif
ifneq (,$(findstring openal,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_OPENAL -DUF_USE_ALUT
	ifneq (,$(findstring dreamcast,$(ARCH)))
		ifneq (,$(findstring aldc,$(REQ_DEPS)))
			DEPS 		+= -lALdc
			FLAGS 		+= -DUF_USE_OPENAL_ALDC
		else
			DEPS 		+= -lAL
		endif
	else
		DEPS 			+= -lopenal -lalut
	endif
endif
ifneq (,$(findstring ogg,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_VORBIS
	ifneq (,$(findstring dreamcast,$(ARCH)))
		DEPS 				+= -lvorbis -logg
	else
		DEPS 				+= -lvorbis -lvorbisfile -logg
	endif
endif
ifneq (,$(findstring freetype,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_FREETYPE 
	DEPS 				+= -lfreetype -lbz2
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
		ifneq (,$(findstring dreamcast,$(ARCH)))
			DEPS 			+= -llua
			INCS 			+= -I/opt/dreamcast/kos-ports/include/lua
		endif
	endif
endif
ifneq (,$(findstring ultralight-ux,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_ULTRALIGHT_UX
	DEPS 				+= -lUltralight -lUltralightCore -lWebCore -lAppCore
endif
ifneq (,$(findstring bullet,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_BULLET
	ifneq (,$(findstring dreamcast,$(ARCH)))
		DEPS 				+= -lbulletdynamics -lbulletcollision -lbulletlinearmath
	else
		DEPS 				+= -lBulletDynamics -lBulletCollision -lLinearMath
		INCS 				+= -I./dep/bullet/
	endif
endif
ifneq (,$(findstring reactphysics,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_REACTPHYSICS
	DEPS 				+= -lreactphysics3d
endif
ifneq (,$(findstring simd,$(REQ_DEPS)))
	ifneq (,$(findstring dreamcast,$(ARCH)))
			FLAGS 				+= -DUF_ENV_DREAMCAST_SIMD
	else
			FLAGS 				+= -DUF_USE_SIMD -DUF_ALIGN_FOR_SIMD -DUF_MATRIX_ALIGNED #-DUF_VECTOR_ALIGNED #-march=native
	endif
endif
ifneq (,$(findstring meshoptimizer,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_MESHOPTIMIZER
	DEPS 				+= -lmeshoptimizer
endif
ifneq (,$(findstring draco,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_DRACO
	DEPS 				+= -ldraco
endif
ifneq (,$(findstring xatlas,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_XATLAS
endif
ifneq (,$(findstring ctti,$(REQ_DEPS)))
	FLAGS 				+= -DUF_CTTI -fno-rtti
else
	FLAGS 				+= -DUF_RTTI -rtti
endif

# SRCS_DLL 				+= $(wildcard $(ENGINE_SRC_DIR)/*.cpp) $(wildcard $(ENGINE_SRC_DIR)/*/*.cpp) $(wildcard $(ENGINE_SRC_DIR)/*/*/*.cpp) $(wildcard $(ENGINE_SRC_DIR)/*/*/*/*.cpp) $(wildcard $(ENGINE_SRC_DIR)/*/*/*/*/*.cpp)
SRCS_DLL 				+= $(wildcard $(ENGINE_SRC_DIR)/*.cpp) $(wildcard $(ENGINE_SRC_DIR)/*/*.cpp) $(wildcard $(ENGINE_SRC_DIR)/*/*/*.cpp) $(wildcard $(ENGINE_SRC_DIR)/*/*/*/*.cpp) $(wildcard $(ENGINE_SRC_DIR)/*/*/*/*/*.cpp) $(wildcard $(EXT_SRC_DIR)/*.cpp) $(wildcard $(EXT_SRC_DIR)/*/*.cpp) $(wildcard $(EXT_SRC_DIR)/*/*/*.cpp) $(wildcard $(EXT_SRC_DIR)/*/*/*/*.cpp) $(wildcard $(EXT_SRC_DIR)/*/*/*/*/*.cpp)
OBJS_DLL 				+= $(patsubst %.cpp,%.$(PREFIX).o,$(SRCS_DLL))
BASE_DLL 				+= lib$(LIB_NAME)
IM_DLL 					+= $(ENGINE_LIB_DIR)/$(ARCH)/$(CC)/$(BASE_DLL).$(TARGET_LIB_EXTENSION).a
EX_DLL 					+= $(BIN_DIR)/exe/lib/$(ARCH)/$(CC)/$(BASE_DLL).$(TARGET_LIB_EXTENSION)
# External Engine's DLL
EXT_INC_DIR 			+= $(INC_DIR)
EXT_LB_FLAGS 			+= $(LIB_DIR)
EXT_DEPS 				+= -l$(LIB_NAME) $(DEPS)
EXT_LINKS 				+= $(UF_LIBS) $(EXT_LIBS) $(EXT_DEPS)
#-Wl,-subsystem,windows

EXT_LIB_DIR 			+= $(ENGINE_LIB_DIR)/$(ARCH)
EXT_INCS 				+= -I$(ENGINE_INC_DIR) -I$(EXT_INC_DIR) -I$(VULKAN_SDK_PATH)/include -I/mingw64/include
EXT_LIBS 				+= -L$(ENGINE_LIB_DIR) -L$(EXT_LIB_DIR) -L$(EXT_LIB_DIR)/$(CC) -L$(VULKAN_SDK_PATH)/Lib -L/mingw64/lib

SRCS_EXT_DLL 			+= $(wildcard $(EXT_SRC_DIR)/*.cpp) $(wildcard $(EXT_SRC_DIR)/*/*.cpp) $(wildcard $(EXT_SRC_DIR)/*/*/*.cpp) $(wildcard $(EXT_SRC_DIR)/*/*/*/*.cpp) $(wildcard $(EXT_SRC_DIR)/*/*/*/*/*.cpp)
OBJS_EXT_DLL 			+= $(patsubst %.cpp,%.$(PREFIX).o,$(SRCS_EXT_DLL))
BASE_EXT_DLL 			+= lib$(EXT_LIB_NAME)
EXT_IM_DLL 				+= $(ENGINE_LIB_DIR)/$(ARCH)/$(CC)/$(BASE_EXT_DLL).$(TARGET_LIB_EXTENSION).a
EXT_EX_DLL 				+= $(BIN_DIR)/exe/lib/$(ARCH)/$(CC)/$(BASE_EXT_DLL).$(TARGET_LIB_EXTENSION)
# Client EXE
SRCS 					+= $(wildcard $(CLIENT_SRC_DIR)/*.cpp) $(wildcard $(CLIENT_SRC_DIR)/*/*.cpp)
OBJS 					+= $(patsubst %.cpp,%.$(PREFIX).o,$(SRCS))
TARGET 					+= $(BIN_DIR)/exe/$(TARGET_NAME).$(CC).$(TARGET_EXTENSION)
# Shaders
SRCS_SHADERS 			+= $(wildcard bin/data/shaders/*.glsl) $(wildcard bin/data/shaders/*/*.glsl) $(wildcard bin/data/shaders/*/*/*.glsl)
TARGET_SHADERS 			+= $(patsubst %.glsl,%.spv,$(SRCS_SHADERS))

ifneq (,$(findstring dreamcast,$(ARCH)))
#$(ARCH): $(EX_DLL) $(EXT_EX_DLL) $(TARGET) ./bin/dreamcast/$(TARGET_NAME).cdi
$(ARCH): $(TARGET) ./bin/dreamcast/$(TARGET_NAME).cdi

SRCS_DLL 				= $(wildcard $(ENGINE_SRC_DIR)/*.cpp) $(wildcard $(ENGINE_SRC_DIR)/*/*.cpp) $(wildcard $(ENGINE_SRC_DIR)/*/*/*.cpp) $(wildcard $(ENGINE_SRC_DIR)/*/*/*/*.cpp) $(wildcard $(ENGINE_SRC_DIR)/*/*/*/*/*.cpp)
OBJS_DLL 				= $(patsubst %.cpp,%.$(PREFIX).o,$(SRCS_DLL))
OBJS 					= $(patsubst %.cpp,%.$(PREFIX).o,$(SRCS_DLL)) $(patsubst %.cpp,%.$(PREFIX).o,$(SRCS_EXT_DLL)) $(patsubst %.cpp,%.$(PREFIX).o,$(SRCS))

DEPS 					+= -lkallisti -lc -lm -lgcc -lstdc++ # -l$(LIB_NAME) -l$(EXT_LIB_NAME)

%.$(PREFIX).o: %.cpp
	$(CXX) $(FLAGS) $(INCS) -c $< -o $@

$(EX_DLL): FLAGS += -DUF_EXPORTS -DJSON_DLL_BUILD
$(EX_DLL): $(OBJS_DLL) 
	$(KOS_AR) cru $@ $^
	$(KOS_RANLIB) $@
	cp $@ $(ENGINE_LIB_DIR)/$(ARCH)/$(CC)/$(BASE_DLL).a

$(EXT_EX_DLL): FLAGS += -DEXT_EXPORTS -DJSON_DLL_BUILD
$(EXT_EX_DLL): $(OBJS_EXT_DLL) 
	$(KOS_AR) cru $@ $^
	$(KOS_RANLIB) $@
	cp $@ $(ENGINE_LIB_DIR)/$(ARCH)/$(CC)/$(BASE_EXT_DLL).a

./bin/dreamcast/romdisk.img:
	$(KOS_GENROMFS) -f ./bin/dreamcast/romdisk.img -d ./bin/dreamcast/romdisk/ -v

./bin/dreamcast/romdisk.o: ./bin/dreamcast/romdisk.img
	$(KOS_BASE)/utils/bin2o/bin2o ./bin/dreamcast/romdisk.img romdisk ./bin/dreamcast/romdisk.o

$(TARGET): $(OBJS) #./bin/dreamcast/romdisk.o
	$(CXX) $(FLAGS) $(INCS) -D_arch_dreamcast -D_arch_sub_pristine -Wall -fno-builtin -ml -m4-single-only -Wl,-Ttext=0x8c010000 -T/opt/dreamcast/kos/utils/ldscripts/shlelf.xc -nodefaultlibs $(KOS_LIB_PATHS) $(LIBS) -o $(TARGET) $(OBJS) -Wl,--start-group $(DEPS) -Wl,--end-group
	# $(KOS_STRIP) --strip-unneeded $(TARGET)	

./bin/dreamcast/$(TARGET_NAME).cdi: $(TARGET)
	cd ./bin/dreamcast/; ./elf2cdi.sh $(TARGET_NAME)

cdi:
	cd ./bin/dreamcast/; ./elf2cdi.sh $(TARGET_NAME)

else
$(ARCH): $(EX_DLL) $(TARGET) $(TARGET_SHADERS)

%.$(PREFIX).o: %.cpp
	$(CXX) $(FLAGS) $(INCS) -c $< -o $@

$(EX_DLL): FLAGS += -DUF_EXPORTS -DEXT_EXPORTS -DJSON_DLL_BUILD
#$(EX_DLL): FLAGS += -DUF_EXPORTS -DJSON_DLL_BUILD
$(EX_DLL): $(OBJS_DLL) 
	$(CXX) -shared -o $(EX_DLL) -Wl,--out-implib=$(IM_DLL) $(OBJS_DLL) $(LIBS) $(INCS) $(LINKS)
	cp $(ENGINE_LIB_DIR)/$(ARCH)/$(CC)/$(BASE_DLL).$(TARGET_LIB_EXTENSION).a $(ENGINE_LIB_DIR)/$(ARCH)/$(CC)/$(BASE_DLL).a


$(EXT_EX_DLL): FLAGS += -DEXT_EXPORTS -DJSON_DLL_BUILD
$(EXT_EX_DLL): $(OBJS_EXT_DLL) 
	$(CXX) -shared -o $(EXT_EX_DLL) -Wl,--out-implib=$(EXT_IM_DLL) $(OBJS_EXT_DLL) $(EXT_LIBS) $(EXT_INCS) $(EXT_LINKS)
	cp $(ENGINE_LIB_DIR)/$(ARCH)/$(CC)/$(BASE_EXT_DLL).$(TARGET_LIB_EXTENSION).a $(ENGINE_LIB_DIR)/$(ARCH)/$(CC)/$(BASE_EXT_DLL).a

$(TARGET): $(OBJS)
	$(CXX) $(FLAGS) $(OBJS) $(LIBS) $(INCS) $(LINKS) -l$(LIB_NAME) -o $(TARGET)
#	$(CXX) $(FLAGS) $(OBJS) $(LIBS) $(INCS) $(LINKS) -l$(LIB_NAME) -o $(TARGET)
#	$(CXX) $(FLAGS) $(OBJS) $(LIBS) $(INCS) $(LINKS) -l$(LIB_NAME) -l$(EXT_LIB_NAME) -o $(TARGET)

endif

%.spv: %.glsl
	$(GLSLC) -std=450 -o $@ $<
	$(SPV_OPTIMIZER) --preserve-bindings --preserve-spec-constants -O $@ -o $@

ifneq (,$(findstring dreamcast,$(ARCH)))
clean:
	@-rm $(EX_DLL)
	@-rm $(EXT_EX_DLL)
	@-rm $(TARGET)
	
	@-rm -f $(OBJS_DLL)
	@-rm -f $(OBJS_EXT_DLL)
	@-rm -f $(OBJS)

	@-rm ./bin/dreamcast/build/*
	@-rm ./bin/dreamcast/romdisk.*
	@-rm ./bin/dreamcast/$(TARGET_NAME).*

run:
	 $(KOS_EMU) ./bin/dreamcast/$(TARGET_NAME).cdi
else
clean:
	@-rm $(EX_DLL)
	@-rm $(EXT_EX_DLL)
	@-rm $(TARGET)
	
	@-rm -f $(OBJS_DLL)
	@-rm -f $(OBJS_EXT_DLL)
	@-rm -f $(OBJS)

run:
	./program.sh
endif
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

clean-shaders:
	-rm $(TARGET_SHADERS)

backup:
	make ARCH=dreamcast clean
	make CC=gcc clean
	make CC=clang clean
	$(7Z) a -r ../misc/backups/$(shell date +"%Y.%m.%d\ %H-%M-%S").7z .