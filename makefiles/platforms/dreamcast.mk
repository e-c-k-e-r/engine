FLAGS 				+= -DUF_ENV_DREAMCAST
REQ_DEPS 			+= opengl gldc json:nlohmann zlib lz4 lua simd ctti fmt ttf ogg wav adp aica # openal aldc

INCS 				:= -I./dep/dreamcast/include $(INCS)

# Dependency Overrides
ifneq (,$(findstring gldc,$(REQ_DEPS)))
	DEPS 			+= -lGL
	FLAGS 			+= -DUF_USE_OPENGL_GLDC
endif

ifneq (,$(findstring aldc,$(REQ_DEPS)))
	DEPS 			+= -lAL -lpthread
	FLAGS 			+= -DUF_USE_OPENAL_ALDC -DUF_USE_ALUT
endif
ifneq (,$(findstring aica,$(REQ_DEPS)))
	FLAGS 			+= -DUF_USE_AICA
endif

ifneq (,$(findstring ogg,$(REQ_DEPS)))
	FLAGS 			+= -DUF_USE_TREMOR
	DEPS 			+= -ltremor
endif

ifneq (,$(findstring lua,$(REQ_DEPS)))
	DEPS 			+= -llua
	INCS 			+= -I/opt/dreamcast/kos-ports/include/lua
endif

ifneq (,$(findstring simd,$(REQ_DEPS)))
	FLAGS 			+= -DUF_ENV_DREAMCAST_SIMD
endif

DEPS 				+= -lkallisti -lc -lm -lgcc -lstdc++

SRCS_DLL 			= $(shell find $(ENGINE_SRC_DIR) -name "*.cpp") $(shell find $(DEP_SRC_DIR) -name "*.cpp")
SRCS_DLL_C 			= $(shell find $(ENGINE_SRC_DIR) -name "*.c") $(shell find $(DEP_SRC_DIR) -name "*.c")
OBJS_DLL 			= $(patsubst %.cpp,%.$(PREFIX).o,$(SRCS_DLL)) $(patsubst %.c,%.$(PREFIX).o,$(SRCS_DLL_C)) 
OBJS 				= $(patsubst %.cpp,%.$(PREFIX).o,$(SRCS_DLL)) $(patsubst %.c,%.$(PREFIX).o,$(SRCS_DLL_C)) $(patsubst %.cpp,%.$(PREFIX).o,$(SRCS_EXT_DLL)) $(patsubst %.cpp,%.$(PREFIX).o,$(SRCS))
KOS_AR     			= /opt/dreamcast/sh-elf/bin/sh-elf-gcc-ar
KOS_RANLIB 			= /opt/dreamcast/sh-elf/bin/sh-elf-gcc-ranlib
ISO_TYPE 			= cdi
ISO_IGNORE 			= --ignore ".pdb" --ignore ".glb" --ignore ".bsp" --ignore shaders/

$(PREFIX): $(TARGET) ./bin/dreamcast/$(TARGET_NAME).cdi

$(EX_DLL): FLAGS += -DUF_EXPORTS
$(EX_DLL): $(OBJS_DLL)
	#cp $@ $(ENGINE_LIB_DIR)/$(PREFIX_PATH)/$(BASE_DLL)

$(EXT_EX_DLL): FLAGS += -DEXT_EXPORTS
$(EXT_EX_DLL): $(OBJS_EXT_DLL)
	#cp $@ $(ENGINE_LIB_DIR)/$(PREFIX_PATH)/$(BASE_EXT_DLL)


./bin/dreamcast/romdisk.img:
	$(KOS_GENROMFS) -f ./bin/dreamcast/romdisk.img -d ./bin/dreamcast/romdisk/ -v

./bin/dreamcast/romdisk.o: ./bin/dreamcast/romdisk.img
	$(KOS_BASE)/utils/bin2o/bin2o ./bin/dreamcast/romdisk.img romdisk ./bin/dreamcast/romdisk.o

$(TARGET): $(OBJS) #./bin/dreamcast/romdisk.o
	$(CXX) $(FLAGS) $(INCS) -D_arch_dreamcast -D_arch_sub_pristine -Wall -fno-builtin -ml -Wl,-Ttext=0x8c010000 -T/opt/dreamcast/kos/utils/ldscripts/shlelf.xc -nostartfiles -nodefaultlibs $(KOS_LIB_PATHS) $(LIBS) -o $(TARGET) $(OBJS) $(KOS_KOS_INIT) -Wl,--start-group $(DEPS) -Wl,--end-group
	cp $(TARGET) $(TARGET).unstripped
	$(KOS_STRIP) --strip-unneeded $(TARGET)

./bin/dreamcast/$(TARGET_NAME).$(ISO_TYPE): $(TARGET)
	$(KOS_BASE)/utils/gd-rom/bin/gd-rom.exe --elf $(TARGET) --output ./bin/dreamcast/$(TARGET_NAME).$(ISO_TYPE) $(ISO_IGNORE) -g "$(TARGET_NAME)" ./bin/data/ ./bin/dreamcast/data/

cdi:
	$(KOS_BASE)/utils/gd-rom/bin/gd-rom.exe --elf $(TARGET) --output ./bin/dreamcast/$(TARGET_NAME).cdi $(ISO_IGNORE) -g "$(TARGET_NAME)" ./bin/data/ ./bin/dreamcast/data/

gdi:
	$(KOS_BASE)/utils/gd-rom/bin/gd-rom.exe --elf $(TARGET) --output ./bin/dreamcast/$(TARGET_NAME).gdi $(ISO_IGNORE) -g "$(TARGET_NAME)" ./bin/data/ ./bin/dreamcast/data/

run-cdi:
	$(KOS_EMU) ./bin/dreamcast/$(TARGET_NAME).cdi

debug-cdi:
	$(KOS_EMU_DEBUG) ./bin/dreamcast/$(TARGET_NAME).cdi $(TARGET).unstripped

run-gdi:
	$(KOS_EMU) ./bin/dreamcast/$(TARGET_NAME).gdi

debug-gdi:
	$(KOS_EMU_DEBUG) ./bin/dreamcast/$(TARGET_NAME).gdi $(TARGET).unstripped

clean-dreamcast:
	@-rm ./bin/dreamcast/build/*
	@-rm ./bin/dreamcast/romdisk.*
	@-rm ./bin/dreamcast/$(TARGET_NAME).*