ifneq (,$(findstring ffx:sdk,$(REQ_DEPS)))
	ifneq (,$(findstring vulkan,$(REQ_DEPS)))
		FLAGS += -DUF_USE_FFX_SDK=3
		DEPS  := -lffx_fsr3_x64 -lffx_fsr2_x64 -lffx_fsr3upscaler_x64 -lffx_frameinterpolation_x64 -lffx_opticalflow_x64 -lffx_backend_vk_x64 -lamd_fidelityfx_vk $(DEPS)
	endif
endif

ifneq (,$(findstring vulkan,$(REQ_DEPS)))
	FLAGS += -DUF_USE_VULKAN
	DEPS  += -lspirv-cross-core -lspirv-cross-cpp
	INCS  += -I$(VULKAN_SDK_PATH)/include -I./dep/include/spirv_cross/
	LIBS  += -L$(VULKAN_SDK_PATH)/Lib
endif

ifneq (,$(findstring opengl,$(REQ_DEPS)))
	FLAGS += -DUF_USE_OPENGL -DUF_USE_OPENGL_FIXED_FUNCTION
	ifeq (,$(findstring gldc,$(REQ_DEPS)))
		FLAGS += -DUF_USE_GLEW
	endif
endif

ifneq (,$(findstring fmt,$(REQ_DEPS)))
	FLAGS += -DUF_USE_FMT
	ifeq (,$(findstring dreamcast,$(ARCH)))
		DEPS += -lfmt
	endif
endif

ifneq (,$(findstring ffx:fsr,$(REQ_DEPS)))
	ifneq (,$(findstring vulkan,$(REQ_DEPS)))
		FLAGS += -DUF_USE_FFX_FSR
		DEPS  += -lffx_fsr2_api_ -lffx_fsr2_api_vk_
	endif
endif

ifneq (,$(findstring imgui,$(REQ_DEPS)))
	FLAGS += -DUF_USE_IMGUI
	INCS  += -I./dep/include/imgui/ -I./dep/include/imgui/backends
endif

ifneq (,$(findstring cpptrace,$(REQ_DEPS)))
	FLAGS += -DUF_USE_CPPTRACE
	DEPS  += -lcpptrace
endif

ifneq (,$(findstring json,$(REQ_DEPS)))
	FLAGS += -DUF_USE_JSON
	ifneq (,$(findstring nlohmann,$(REQ_DEPS)))
		FLAGS += -DUF_JSON_USE_NLOHMANN
	endif
endif

ifneq (,$(findstring gltf,$(REQ_DEPS)))
	FLAGS += -DUF_USE_GLTF
	INCS  += -I./dep/include/stb/ -I./dep/include/nlohmann/
endif

ifneq (,$(findstring dc:texconv,$(REQ_DEPS)))
	FLAGS += -DUF_USE_DC_TEXCONV
endif

ifneq (,$(findstring openal,$(REQ_DEPS)))
	FLAGS += -DUF_USE_OPENAL -DUF_USE_ALUT
	ifeq (,$(findstring dreamcast,$(ARCH)))
		DEPS += -lopenal -lalut
	endif
endif

ifneq (,$(findstring ogg,$(REQ_DEPS)))
	FLAGS += -DUF_USE_VORBIS
	ifeq (,$(findstring dreamcast,$(ARCH)))
		DEPS += -lvorbis -lvorbisfile -logg
	endif
endif

ifneq (,$(findstring wav,$(REQ_DEPS)))
	FLAGS += -DUF_USE_WAV
endif

ifneq (,$(findstring zlib,$(REQ_DEPS)))
	FLAGS += -DUF_USE_ZLIB
	DEPS  += -lz
endif

ifneq (,$(findstring freetype,$(REQ_DEPS)))
	FLAGS += -DUF_USE_FREETYPE
	DEPS  += -lfreetype -lbz2
endif

ifneq (,$(findstring curl,$(REQ_DEPS)))
	FLAGS += -DUF_USE_CURL
	DEPS  += -lcurl
endif

ifneq (,$(findstring openvr,$(REQ_DEPS)))
	FLAGS += -DUF_USE_OPENVR -DUSE_OPENVR_MINGW
	DEPS  += -lopenvr_api
endif

ifneq (,$(findstring lua,$(REQ_DEPS)))
	FLAGS += -DUF_USE_LUA
	ifneq (,$(findstring luajit,$(REQ_DEPS)))
		FLAGS += -DUF_USE_LUAJIT
		DEPS  += -lluajit-5.1
	endif
endif

ifneq (,$(findstring simd,$(REQ_DEPS)))
	ifeq (,$(findstring dreamcast,$(ARCH)))
		FLAGS += -DUF_USE_SIMD
	endif
endif

ifneq (,$(findstring meshoptimizer,$(REQ_DEPS)))
	FLAGS += -DUF_USE_MESHOPT
	DEPS  += -lmeshoptimizer
endif

ifneq (,$(findstring xatlas,$(REQ_DEPS)))
	FLAGS += -DUF_USE_XATLAS
endif

ifneq (,$(findstring ctti,$(REQ_DEPS)))
	FLAGS += -DUF_CTTI -fno-rtti
else
	FLAGS += -DUF_RTTI -rtti
endif

ifneq (,$(findstring toml,$(REQ_DEPS)))
	FLAGS += -DUF_USE_TOML
endif

ifneq (,$(findstring vall_e,$(REQ_DEPS)))
	FLAGS += -DUF_USE_VALL_E
	INCS  += -I./dep/include/vall_e.cpp/
	DEPS  += -lvall_e
endif

ifneq (,$(findstring draco,$(REQ_DEPS)))
	FLAGS += -DUF_USE_DRACO
	DEPS  += -ldraco
endif

ifneq (,$(findstring ultralight-ux,$(REQ_DEPS)))
	FLAGS += -DUF_USE_ULTRALIGHT_UX
	DEPS  += -lUltralight -lUltralightCore -lWebCore -lAppCore
endif

ifneq (,$(findstring discord,$(REQ_DEPS)))
	FLAGS += -DUF_USE_DISCORD
	DEPS  += -ldiscord_game_sdk
endif

ifneq (,$(findstring ncurses,$(REQ_DEPS)))
	FLAGS += -DUF_USE_NCURSES
	DEPS  += -lncursesw
endif

ifneq (,$(findstring png,$(REQ_DEPS)))
	FLAGS += -DUF_USE_PNG
	DEPS  += -lpng
endif

ifneq (,$(findstring lz4,$(REQ_DEPS)))
	FLAGS += -DUF_USE_LZ4
	DEPS  += -llz4
endif

ifneq (,$(findstring xz,$(REQ_DEPS)))
	FLAGS += -DUF_USE_XZ -DUF_USE_LZMA
	DEPS  += -lxz -llzma
endif

ifneq (,$(findstring valve,$(REQ_DEPS)))
	FLAGS += -DUF_USE_VALVE
endif