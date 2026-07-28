FLAGS 				+= -DUF_DEV_ENV

ifneq (,$(findstring UF_DEV_ENV,$(FLAGS)))
	REQ_DEPS 			+= meshoptimizer toml xatlas curl dc:texconv ffx:sdk openvr valve lgs # vall_e cpptrace # ncurses draco discord ultralight-ux
	FLAGS 				+= -march=native -g # -flto # -g
endif

REQ_DEPS 			+= $(RENDERER) json:nlohmann zlib lz4 luajit simd ctti gltf imgui fmt ttf openal ogg wav adp
FLAGS 				+= -DUF_ENV_WINDOWS -DUF_ENV_WIN64 -DWIN32_LEAN_AND_MEAN
DEPS 				+= -lgdi32 -ldwmapi
LINKS 				+= #-Wl,-subsystem,windows
INCS 				:= -I./dep/master/include $(INCS)

# Vulkan Paths
VULKAN_SDK_PATH		?= /c/VulkanSDK/1.4.321.1/
GLSLC 				?= $(VULKAN_SDK_PATH)/Bin/glslc
SPV_OPTIMIZER 		?= $(VULKAN_SDK_PATH)/Bin/spirv-opt
SPV_LINTER 			?= $(VULKAN_SDK_PATH)/Bin/spirv-lint

# Dependency Overrides
ifneq (,$(findstring vulkan,$(REQ_DEPS)))
	DEPS 			+= -lvulkan-1
	FLAGS 			+= -DVK_USE_PLATFORM_WIN32_KHR
endif

ifneq (,$(findstring opengl,$(REQ_DEPS)))
	ifeq (,$(findstring gldc,$(REQ_DEPS)))
		DEPS 		+= -lglew32 -lopengl32 -lglu32
	endif
endif

ifneq (,$(findstring luajit,$(REQ_DEPS)))
	ifneq (,$(findstring clang,$(CC)))
		INCS 		+= -I/clang64/include/luajit-2.1
	else
		INCS 		+= -I/mingw64/include/luajit-2.1
	endif
endif

# Target Compilation Overrides
$(EX_DLL): FLAGS += -DUF_EXPORTS
$(EX_DLL): $(OBJS_DLL)
	$(CXX) $(FLAGS) -shared -o $(EX_DLL) -Wl,--out-implib=$(IM_DLL)$(SLIB_EXTENSION) $(OBJS_DLL) $(LIBS) $(INCS) $(LINKS)
	@echo -n $(ARCH) > "./bin/exe/default/arch"
	@echo -n $(CC) > "./bin/exe/default/cc"
	@echo -n $(RENDERER) > "./bin/exe/default/renderer"

$(EXT_EX_DLL): FLAGS += -DEXT_EXPORTS
$(EXT_EX_DLL): $(OBJS_EXT_DLL)
	$(CXX) $(FLAGS) -shared -o $(EXT_EX_DLL) -Wl,--out-implib=$(EXT_IM_DLL)$(SLIB_EXTENSION) $(OBJS_EXT_DLL) $(EXT_LIBS) $(EXT_INCS) $(EXT_LINKS)