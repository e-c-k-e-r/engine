FLAGS 				+= -DUF_ENV_DREAMCAST
REQ_DEPS 			+= opengl gldc json:nlohmann zlib lz4 lua simd ctti fmt freetype openal aldc ogg wav png

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
OBJS_DLL 			= $(patsubst %.cpp,%.$(PREFIX).o,$(SRCS_DLL))
OBJS 				= $(patsubst %.cpp,%.$(PREFIX).o,$(SRCS_DLL)) $(patsubst %.cpp,%.$(PREFIX).o,$(SRCS_EXT_DLL)) $(patsubst %.cpp,%.$(PREFIX).o,$(SRCS))

$(PREFIX): $(TARGET) ./bin/dreamcast/$(TARGET_NAME).cdi

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

run-dreamcast:
	$(KOS_EMU) ./bin/dreamcast/$(TARGET_NAME).cdi

run-debug-dreamcast:
	$(KOS_EMU_DEBUG) ./bin/dreamcast/$(TARGET_NAME).cdi

clean-dreamcast:
	@-rm ./bin/dreamcast/build/*
	@-rm ./bin/dreamcast/romdisk.*
	@-rm ./bin/dreamcast/$(TARGET_NAME).*