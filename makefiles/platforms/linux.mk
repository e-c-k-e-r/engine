ifneq (,$(findstring -DUF_DEV_ENV,$(FLAGS)))
	REQ_DEPS 			+= toml xatlas curl dc:texconv # meshoptimizer ffx:fsr cpptrace vall_e # ncurses openvr draco discord bullet ultralight-ux 
	FLAGS 				+= -march=native -g # -flto # -g
endif

REQ_DEPS 			+= $(RENDERER) json:nlohmann zlib lz4 luajit simd ctti gltf imgui fmt freetype openal ogg wav
FLAGS 				+= -DUF_ENV_LINUX -fPIC
DEPS 				+= -pthread -ldl -lX11 -lXrandr
INCS 				:= -I./dep/master/include $(INCS)

# Vulkan Paths
VULKAN_SDK_PATH 	?= /
GLSLC 				?= glslc
SPV_OPTIMIZER 		?= spirv-opt
SPV_LINTER 			?= spirv-lint

# Dependency Overrides
ifneq (,$(findstring vulkan,$(REQ_DEPS)))
	DEPS 			+= -lvulkan
	FLAGS 			+= -DVK_USE_PLATFORM_XLIB_KHR
endif

ifneq (,$(findstring opengl,$(REQ_DEPS)))
	ifeq (,$(findstring gldc,$(REQ_DEPS)))
		DEPS 		+= -lGLU -lglut -lGLEW
	endif
endif

ifneq (,$(findstring luajit,$(REQ_DEPS)))
	INCS 			+= -I/usr/include/luajit-2.1
endif

# Target Compilation Overrides
$(EX_DLL): FLAGS += -DUF_EXPORTS
$(EX_DLL): $(OBJS_DLL)
	$(CXX) $(FLAGS) -shared -Wl,-soname,$(BASE_DLL)$(DLIB_EXTENSION) $(OBJS_DLL) $(LIBS) $(INCS) $(LINKS) -o $(EX_DLL)
	cp $(EX_DLL) $(IM_DLL)
	@echo -n $(ARCH) > "./bin/exe/default/arch"
	@echo -n $(CC) > "./bin/exe/default/cc"
	@echo -n $(RENDERER) > "./bin/exe/default/renderer"

$(EXT_EX_DLL): FLAGS += -DEXT_EXPORTS
$(EXT_EX_DLL): $(OBJS_EXT_DLL)
	$(CXX) $(FLAGS) -shared -Wl,-soname,$(BASE_EXT_DLL)$(DLIB_EXTENSION) $(OBJS_EXT_DLL) $(EXT_LIBS) $(EXT_INCS) $(EXT_LINKS) -o $(EXT_EX_DLL)
	cp $(EXT_EX_DLL) $(EXT_IM_DLL)