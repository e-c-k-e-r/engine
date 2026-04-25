ARCH 					= $(shell cat "./makefiles/default/arch")
CC						= $(shell cat "./makefiles/default/cc")
RENDERER 				= $(shell cat "./makefiles/default/renderer")
TARGET_NAME 			= program
TARGET_EXTENSION 		= .exe
DLIB_EXTENSION 			= .dll
SLIB_EXTENSION 			= .a
PREFIX 					= $(ARCH).$(CC)

include makefiles/$(PREFIX).make

PREFIX 					= $(ARCH).$(CC).$(RENDERER)
PREFIX_PATH 			= $(ARCH)/$(CC)/$(RENDERER)

.PHONY: $(PREFIX)
	@echo "Setting defaults: $(ARCH).$(CC).$(RENDERER)"

.FORCE:

CXX 					:= $(CDIR)$(CXX)
BIN_DIR 				+= ./bin

ENGINE_SRC_DIR 			+= ./engine/src
ENGINE_INC_DIR 			+= ./engine/inc
ENGINE_LIB_DIR 			+= ./engine/lib
DEP_SRC_DIR 			+= ./dep/src

EXT_SRC_DIR 			+= ./ext
CLIENT_SRC_DIR 			+= ./client


UF_LIBS 				+= 
EXT_LIBS 				+=
FLAGS 				  	+= 
LIB_NAME 				+= uf
EXT_LIB_NAME 			+= ext

7Z 						+= /c/Program\ Files/7-Zip/7z.exe

ifneq (,$(findstring win64,$(ARCH)))
	VULKAN_SDK_PATH 	= /c/VulkanSDK/1.4.321.1/
	GLSLC 				= $(VULKAN_SDK_PATH)/Bin/glslc
	SPV_OPTIMIZER 		= $(VULKAN_SDK_PATH)/Bin/spirv-opt
	SPV_LINTER 			= $(VULKAN_SDK_PATH)/Bin/spirv-lint
else
	VULKAN_SDK_PATH		= /
	GLSLC 				= glslc
	SPV_OPTIMIZER 		= spirv-opt
	SPV_LINTER 			= spirv-lint
endif


# Base Engine's DLL
INC_DIR 				+= $(ENGINE_INC_DIR)
LIB_DIR 				+= $(ENGINE_LIB_DIR)

INCS 					+= -I$(ENGINE_INC_DIR) -I./dep/include/
LIBS 					+= -L$(ENGINE_LIB_DIR) -L$(LIB_DIR)/$(PREFIX_PATH)/ -L$(LIB_DIR)/$(ARCH)/$(CC)/ -L$(LIB_DIR)/$(ARCH)/
	
LINKS 					+= $(UF_LIBS) $(EXT_LIBS) $(DEPS)
DEPS 					+= 
FLAGS 					+= -DUF_DEV_ENV

ifneq (,$(findstring win64,$(ARCH)))
	ifneq (,$(findstring -DUF_DEV_ENV,$(FLAGS)))
		REQ_DEPS 			+= meshoptimizer toml xatlas curl ffx:sdk dc:texconv # vall_e cpptrace # openvr # ncurses draco discord bullet ultralight-ux
		FLAGS 				+= -march=native -g # -flto # -g
	endif
	REQ_DEPS 			+= $(RENDERER) json:nlohmann zlib luajit r:eactphysics simd ctti gltf imgui fmt freetype openal ogg wav 
	FLAGS 				+= -DUF_ENV_WINDOWS -DUF_ENV_WIN64 -DWIN32_LEAN_AND_MEAN
	DEPS 				+= -lgdi32 -ldwmapi
	LINKS 				+= #-Wl,-subsystem,windows
	INCS 				:= -I./dep/master/include $(INCS)
else ifneq (,$(findstring linux,$(ARCH)))
	ifneq (,$(findstring -DUF_DEV_ENV,$(FLAGS)))
		REQ_DEPS 			+= toml xatlas curl dc:texconv # meshoptimizer ffx:fsr cpptrace vall_e # ncurses openvr draco discord bullet ultralight-ux 
		FLAGS 				+= -march=native -g # -flto # -g
	endif
	REQ_DEPS 			+= $(RENDERER) json:nlohmann zlib luajit r:eactphysics simd ctti gltf imgui fmt freetype openal ogg wav
	FLAGS				+= -DUF_ENV_LINUX -fPIC
	DEPS				+= -pthread -ldl -lX11 -lXrandr 
	INCS				:= -I./dep/master/include $(INCS)
else ifneq (,$(findstring dreamcast,$(ARCH)))
	FLAGS 				+= -DUF_ENV_DREAMCAST # -DUF_LEAN_AND_MEAN # this apparently crashes
	REQ_DEPS 			+= opengl gldc json:nlohmann zlib lua r:eactphysics simd ctti fmt freetype openal aldc ogg wav png # imgui
	INCS 				:= -I./dep/dreamcast/include $(INCS)
endif

ifneq (,$(findstring ffx:sdk,$(REQ_DEPS)))
	ifneq (,$(findstring vulkan,$(REQ_DEPS)))
		FLAGS 				+= -DUF_USE_FFX_SDK=3
		#INCS 				+= -I./dep/include/ffx_fsr2/
		DEPS 				+= -lffx_fsr3_x64drel -lffx_fsr2_x64drel -lffx_fsr3upscaler_x64drel -lffx_frameinterpolation_x64drel -lffx_opticalflow_x64drel -lffx_backend_vk_x64drel -lamd_fidelityfx_vkdrel
		#DEPS 				+= -lffx_fsr3_x64 -lffx_fsr2_x64 -lffx_fsr3upscaler_x64 -lffx_frameinterpolation_x64 -lffx_opticalflow_x64 -lffx_backend_vk_x64 -lamd_fidelityfx_vk
	endif
endif
ifneq (,$(findstring vulkan,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_VULKAN
	DEPS 				+= -lspirv-cross-core -lspirv-cross-cpp #-lVulkanMemoryAllocator
	INCS 				+= -I$(VULKAN_SDK_PATH)/include -I./dep/include/spirv_cross/
	LIBS 				+= -L$(VULKAN_SDK_PATH)/Lib
	
	ifneq (,$(findstring linux,$(ARCH)))
		DEPS 			+= -lvulkan
		FLAGS			+= -DVK_USE_PLATFORM_XLIB_KHR # VK_USE_PLATFORM_XCB_KHR
	else
		DEPS 			+= -lvulkan-1
		FLAGS			+= -DVK_USE_PLATFORM_WIN32_KHR
	endif
endif
ifneq (,$(findstring opengl,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_OPENGL -DUF_USE_OPENGL_FIXED_FUNCTION

	ifneq (,$(findstring dreamcast,$(ARCH)))
		ifneq (,$(findstring gldc,$(REQ_DEPS)))
			DEPS 		+= -lGL # kos-ports version saves it as libGL instead of libGLdc
			FLAGS 		+= -DUF_USE_OPENGL_GLDC
		else
			DEPS 		+= -lGL
		endif
	else
		# software rendering version through GLdc
		ifneq (,$(findstring gldc,$(REQ_DEPS)))
			DEPS 			+= -lGLdc -lSDL2
			FLAGS 			+= -DUF_USE_OPENGL_GLDC
		# OpenGL through GLEW
		else
			FLAGS 			+= -DUF_USE_GLEW
			ifneq (,$(findstring linux,$(ARCH)))
				DEPS 			+= -lGLU -lglut -lGLEW
			else
				DEPS 			+= -lglew32 -lopengl32 -lglu32
			endif
		endif
	endif

endif
ifneq (,$(findstring fmt,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_FMT
	ifneq (,$(findstring dreamcast,$(ARCH)))
		# dreamcast uses a header only version because reasons
	else
		DEPS 				+= -lfmt
	endif
endif
ifneq (,$(findstring ffx:fsr,$(REQ_DEPS)))
	ifneq (,$(findstring vulkan,$(REQ_DEPS)))
		FLAGS 				+= -DUF_USE_FFX_FSR
		#INCS 				+= -I./dep/include/ffx_fsr2/
		DEPS 				+= -lffx_fsr2_api_ -lffx_fsr2_api_vk_
	endif
endif
ifneq (,$(findstring imgui,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_IMGUI
	INCS 				+= -I./dep/include/imgui/
	INCS 				+= -I./dep/include/imgui/backends
endif
ifneq (,$(findstring cpptrace,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_CPPTRACE
	DEPS 				+= -lcpptrace
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
	INCS 				+= -I./dep/include/stb/ # saves having to edit the file
	INCS 				+= -I./dep/include/nlohmann/ # saves having to edit the file
endif
ifneq (,$(findstring dc:texconv,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_DC_TEXCONV
endif
ifneq (,$(findstring openal,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_OPENAL -DUF_USE_ALUT
	ifneq (,$(findstring dreamcast,$(ARCH)))
		ifneq (,$(findstring aldc,$(REQ_DEPS)))
			DEPS 		+= -lAL -lpthread # kos-ports version saves it as libAL instead of libALdc
			FLAGS 		+= -DUF_USE_OPENAL_ALDC -DUF_USE_ALUT
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
		FLAGS 				+= -DUF_USE_TREMOR
		DEPS 				+= -ltremor
	else
		DEPS 				+= -lvorbis -lvorbisfile -logg
	endif
endif
ifneq (,$(findstring wav,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_WAV
endif
ifneq (,$(findstring zlib,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_ZLIB
	DEPS 				+= -lz
endif
ifneq (,$(findstring freetype,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_FREETYPE 
	DEPS 				+= -lfreetype -lbz2
endif
ifneq (,$(findstring curl,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_CURL
	DEPS 				+= -lcurl
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

		# sol directly includes <luajit.h>
		ifneq (,$(findstring linux,$(ARCH)))
			INCS 				+= -I/usr/include/luajit-2.1
		else
			ifneq (,$(findstring clang,$(CC)))
				INCS 				+= -I/clang64/include/luajit-2.1
			else
				INCS 				+= -I/mingw64/include/luajit-2.1
			endif
		endif
	else
		ifneq (,$(findstring dreamcast,$(ARCH)))
			DEPS 			+= -llua
			INCS 			+= -I/opt/dreamcast/kos-ports/include/lua
		endif
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
	FLAGS 				+= -DUF_USE_MESHOPT
	DEPS 				+= -lmeshoptimizer
endif
ifneq (,$(findstring xatlas,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_XATLAS
endif
ifneq (,$(findstring ctti,$(REQ_DEPS)))
	FLAGS 				+= -DUF_CTTI -fno-rtti
else
	FLAGS 				+= -DUF_RTTI -rtti
endif
ifneq (,$(findstring toml,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_TOML
endif
ifneq (,$(findstring vall_e,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_VALL_E
	INCS 				+= -I./dep/include/vall_e.cpp/
	DEPS 				+= -lvall_e
endif
ifneq (,$(findstring draco,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_DRACO
	DEPS 				+= -ldraco
endif
ifneq (,$(findstring ultralight-ux,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_ULTRALIGHT_UX
	DEPS 				+= -lUltralight -lUltralightCore -lWebCore -lAppCore
endif
ifneq (,$(findstring discord,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_DISCORD
	DEPS 				+= -ldiscord_game_sdk
endif
ifneq (,$(findstring ncurses,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_NCURSES
	DEPS 				+= -lncursesw
endif
ifneq (,$(findstring png,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_PNG
	DEPS 				+= -lpng
endif
ifneq (,$(findstring lz4,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_LZ4
	DEPS 				+= -llz4
endif
ifneq (,$(findstring xz,$(REQ_DEPS)))
	FLAGS 				+= -DUF_USE_XZ -DUF_USE_LZMA
	DEPS 				+= -lxz -llzma
endif

SRCS_DLL 				:= $(shell find $(ENGINE_SRC_DIR) -name "*.cpp") $(shell find $(DEP_SRC_DIR) -name "*.cpp")
OBJS_DLL 				+= $(patsubst %.cpp,%.$(PREFIX).o,$(SRCS_DLL))
BASE_DLL 				+= lib$(LIB_NAME)
IM_DLL 					+= $(ENGINE_LIB_DIR)/$(PREFIX_PATH)/$(BASE_DLL)$(DLIB_EXTENSION)
EX_DLL 					+= $(BIN_DIR)/exe/lib/$(PREFIX_PATH)/$(BASE_DLL)$(DLIB_EXTENSION)
# External Engine's DLL
EXT_INC_DIR 			+= $(INC_DIR)
EXT_LB_FLAGS 			+= $(LIB_DIR)
EXT_DEPS 				+= -l$(LIB_NAME) $(DEPS)
EXT_LINKS 				+= $(UF_LIBS) $(EXT_LIBS) $(EXT_DEPS)

EXT_LIB_DIR 			+= $(ENGINE_LIB_DIR)/$(ARCH)
EXT_INCS 				+= $(INCS)
EXT_LIBS 				+= $(LIBS)

#SRCS_EXT_DLL 			+= $(wildcard $(EXT_SRC_DIR)/*.cpp) $(wildcard $(EXT_SRC_DIR)/*/*.cpp) $(wildcard $(EXT_SRC_DIR)/*/*/*.cpp) $(wildcard $(EXT_SRC_DIR)/*/*/*/*.cpp) $(wildcard $(EXT_SRC_DIR)/*/*/*/*/*.cpp)
SRCS_EXT_DLL 			:= $(shell find $(EXT_SRC_DIR) -name "*.cpp")
OBJS_EXT_DLL 			+= $(patsubst %.cpp,%.$(PREFIX).o,$(SRCS_EXT_DLL))
BASE_EXT_DLL 			+= lib$(EXT_LIB_NAME)
EXT_IM_DLL 				+= $(ENGINE_LIB_DIR)/$(PREFIX_PATH)/$(BASE_EXT_DLL)$(DLIB_EXTENSION)
EXT_EX_DLL 				+= $(BIN_DIR)/exe/lib/$(PREFIX_PATH)/$(BASE_EXT_DLL)$(DLIB_EXTENSION)
# Client EXE
#SRCS 					+= $(wildcard $(CLIENT_SRC_DIR)/*.cpp) $(wildcard $(CLIENT_SRC_DIR)/*/*.cpp)
SRCS 					:= $(shell find $(CLIENT_SRC_DIR) -name "*.cpp")
OBJS 					+= $(patsubst %.cpp,%.$(PREFIX).o,$(SRCS))
TARGET 					+= $(BIN_DIR)/exe/$(TARGET_NAME).$(PREFIX)$(TARGET_EXTENSION)
# Shaders
#SRCS_SHADERS 			+= $(wildcard bin/data/shaders/*.glsl) $(wildcard bin/data/shaders/*/*.glsl) $(wildcard bin/data/shaders/*/*/*.glsl)
SRCS_SHADERS 			:= $(shell find bin/data/shaders/ -name "*.glsl")
TARGET_SHADERS 			+= $(patsubst %.glsl,%.spv,$(SRCS_SHADERS))

ifneq (,$(findstring dreamcast,$(ARCH)))
#$(PREFIX): $(EX_DLL) $(EXT_EX_DLL) $(TARGET) ./bin/dreamcast/$(TARGET_NAME).cdi
$(PREFIX): $(TARGET) ./bin/dreamcast/$(TARGET_NAME).cdi

#SRCS_DLL 				= $(wildcard $(ENGINE_SRC_DIR)/*.cpp) $(wildcard $(ENGINE_SRC_DIR)/*/*.cpp) $(wildcard $(ENGINE_SRC_DIR)/*/*/*.cpp) $(wildcard $(ENGINE_SRC_DIR)/*/*/*/*.cpp) $(wildcard $(ENGINE_SRC_DIR)/*/*/*/*/*.cpp)
SRCS_DLL 				= $(shell find $(ENGINE_SRC_DIR) -name "*.cpp") $(shell find $(DEP_SRC_DIR) -name "*.cpp")
OBJS_DLL 				= $(patsubst %.cpp,%.$(PREFIX).o,$(SRCS_DLL))
OBJS 					= $(patsubst %.cpp,%.$(PREFIX).o,$(SRCS_DLL)) $(patsubst %.cpp,%.$(PREFIX).o,$(SRCS_EXT_DLL)) $(patsubst %.cpp,%.$(PREFIX).o,$(SRCS))

DEPS 					+= -lkallisti -lc -lm -lgcc -lstdc++ # -l$(LIB_NAME) -l$(EXT_LIB_NAME)

INCS 					+= -I$(KOS_PORTS)/include
LIBS 					+= -I$(KOS_PORTS)/lib

%.$(PREFIX).o: %.cpp
	$(CXX) $(FLAGS) $(INCS) -c $< -o $@

$(EX_DLL): FLAGS += -DUF_EXPORTS
$(EX_DLL): $(OBJS_DLL) 
	$(KOS_AR) cru $@ $^
	$(KOS_RANLIB) $@
	cp $@ $(ENGINE_LIB_DIR)/$(PREFIX_PATH)/$(BASE_DLL)

$(EXT_EX_DLL): FLAGS += -DEXT_EXPORTS
$(EXT_EX_DLL): $(OBJS_EXT_DLL) 
	$(KOS_AR) cru $@ $^
	$(KOS_RANLIB) $@
	cp $@ $(ENGINE_LIB_DIR)/$(PREFIX_PATH)/$(BASE_EXT_DLL)

./bin/dreamcast/romdisk.img:
	$(KOS_GENROMFS) -f ./bin/dreamcast/romdisk.img -d ./bin/dreamcast/romdisk/ -v

./bin/dreamcast/romdisk.o: ./bin/dreamcast/romdisk.img
	$(KOS_BASE)/utils/bin2o/bin2o ./bin/dreamcast/romdisk.img romdisk ./bin/dreamcast/romdisk.o

$(TARGET): $(OBJS) #./bin/dreamcast/romdisk.o
	$(CXX) $(FLAGS) $(INCS) -D_arch_dreamcast -D_arch_sub_pristine -Wall -fno-builtin -ml -Wl,-Ttext=0x8c010000 -T/opt/dreamcast/kos/utils/ldscripts/shlelf.xc -nodefaultlibs $(KOS_LIB_PATHS) $(LIBS) -o $(TARGET) $(OBJS) -Wl,--start-group $(DEPS) -Wl,--end-group
	cp $(TARGET) $(TARGET).unstripped
	$(KOS_STRIP) --strip-unneeded $(TARGET)

./bin/dreamcast/$(TARGET_NAME).cdi: $(TARGET)
	cd ./bin/dreamcast/; ./elf2cdi.sh $(TARGET_NAME)

cdi:
	cd ./bin/dreamcast/; ./elf2cdi.sh $(TARGET_NAME)

else
$(PREFIX): $(EX_DLL) $(EXT_EX_DLL) $(TARGET) $(TARGET_SHADERS)

%.$(PREFIX).o: %.cpp
	$(CXX) $(FLAGS) $(INCS) -c $< -o $@

ifneq (,$(findstring linux,$(ARCH)))
#$(EX_DLL): FLAGS += -DUF_EXPORTS -DEXT_EXPORTS
$(EX_DLL): FLAGS += -DUF_EXPORTS
$(EX_DLL): $(OBJS_DLL) 
	$(CXX) $(FLAGS) -shared -Wl,-soname,$(BASE_DLL)$(DLIB_EXTENSION) $(OBJS_DLL) $(LIBS) $(INCS) $(LINKS) -o $(EX_DLL)
	cp $(EX_DLL) $(IM_DLL)
	@echo -n $(ARCH) > "./bin/exe/default/arch"
	@echo -n $(CC) > "./bin/exe/default/cc"
	@echo -n $(RENDERER) > "./bin/exe/default/renderer"
	@echo "Setting defaults: $(ARCH).$(CC).$(RENDERER)"
$(EXT_EX_DLL): FLAGS += -DEXT_EXPORTS
$(EXT_EX_DLL): $(OBJS_EXT_DLL) 
	$(CXX) $(FLAGS) -shared -Wl,-soname,$(BASE_EXT_DLL)$(DLIB_EXTENSION) $(OBJS_EXT_DLL) $(EXT_LIBS) $(EXT_INCS) $(EXT_LINKS) -o $(EXT_EX_DLL)
	cp $(EXT_EX_DLL) $(EXT_IM_DLL)

else

#$(EX_DLL): FLAGS += -DUF_EXPORTS -DEXT_EXPORTS
$(EX_DLL): FLAGS += -DUF_EXPORTS
$(EX_DLL): $(OBJS_DLL) 
	$(CXX) $(FLAGS) -shared -o $(EX_DLL) -Wl,--out-implib=$(IM_DLL)$(SLIB_EXTENSION) $(OBJS_DLL) $(LIBS) $(INCS) $(LINKS)
	@echo -n $(ARCH) > "./bin/exe/default/arch"
	@echo -n $(CC) > "./bin/exe/default/cc"
	@echo -n $(RENDERER) > "./bin/exe/default/renderer"
	@echo "Setting defaults: $(ARCH).$(CC).$(RENDERER)"


$(EXT_EX_DLL): FLAGS += -DEXT_EXPORTS
$(EXT_EX_DLL): $(OBJS_EXT_DLL) 
	$(CXX) $(FLAGS) -shared -o $(EXT_EX_DLL) -Wl,--out-implib=$(EXT_IM_DLL)$(SLIB_EXTENSION) $(OBJS_EXT_DLL) $(EXT_LIBS) $(EXT_INCS) $(EXT_LINKS)

endif

$(TARGET): $(OBJS)
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

	#@-rm $(shell find $(ENGINE_SRC_DIR) -name "*.$(PREFIX).o") $(shell find $(EXT_SRC_DIR) -name "*.$(PREFIX).o") $(shell find $(DEP_SRC_DIR) -name "*.$(PREFIX).o")

ifneq (,$(findstring dreamcast,$(ARCH)))
	@-rm ./bin/dreamcast/build/*
	@-rm ./bin/dreamcast/romdisk.*
	@-rm ./bin/dreamcast/$(TARGET_NAME).*
endif

run:
ifneq (,$(findstring dreamcast,$(ARCH)))
	 $(KOS_EMU) ./bin/dreamcast/$(TARGET_NAME).cdi
else
	@echo -n $(ARCH) > "./bin/exe/default/arch"
	@echo -n $(CC) > "./bin/exe/default/cc"
	@echo -n $(RENDERER) > "./bin/exe/default/renderer"
	@echo "Setting defaults: $(ARCH).$(CC).$(RENDERER)"
	./program.sh
endif

run-debug:
ifneq (,$(findstring dreamcast,$(ARCH)))
	 $(KOS_EMU_DEBUG) ./bin/dreamcast/$(TARGET_NAME).cdi
else
	@echo -n $(ARCH) > "./bin/exe/default/arch"
	@echo -n $(CC) > "./bin/exe/default/cc"
	@echo -n $(RENDERER) > "./bin/exe/default/renderer"
	@echo "Setting defaults: $(ARCH).$(CC).$(RENDERER)"
	./debug.sh
endif

clean-zips:
	@-find ./bin/data/ -name "*.gz" -type f -delete

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
	@-rm $(shell find $(ENGINE_SRC_DIR) -name "*.o") $(shell find $(EXT_SRC_DIR) -name "*.o") $(shell find $(DEP_SRC_DIR) -name "*.o")
	$(7Z) a -bsp1 -r ../misc/backups/$(shell date +"%Y.%m.%d\ %H-%M-%S").7z . -xr!.git