auto: win64
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
EXT_LIBS 	 			= -lpng16 -lz -lassimp -ljsoncpp
#FLAGS 					= -std=c++0x -Wall -g -DUF_USE_JSON -DUF_USE_NCURSES -DUF_USE_OPENGL -DUF_USE_GLEW
FLAGS 					= -std=c++0x -Wall -g -DUF_USE_JSON -DUF_USE_NCURSES -DUF_USE_OPENGL -DUF_USE_GLEW -DUF_USE_ASSIMP -O2
#-march=native
LIB_NAME 				= uf
EXT_LIB_NAME 			= ext

# Linux GCC settings
NIX_CC 					= g++
NIX_INC_DIR 			= $(ENGINE_INC_DIR)/nix
NIX_LB_FLAGS 			= $(ENGINE_LIB_DIR)/nix
NIX_LIBS 				= 
NIX_LINKS 				= $(UF_LIBS) $(EXT_LIBS) $(NIX_LIBS)
# Base Engine's SO
NIX_LIB_DIR 			= $(ENGINE_LIB_DIR)/nix/
NIX_INCS 				= -I$(ENGINE_INC_DIR) -I$(NIX_INC_DIR)
NIX_LIBS 				= -L$(ENGINE_LIB_DIR) -L$(NIX_LIB_DIR)

SRCS_NIX_SO 			= $(wildcard $(ENGINE_SRC_DIR)/*.cpp) $(wildcard $(ENGINE_SRC_DIR)/*/*.cpp) $(wildcard $(ENGINE_SRC_DIR)/*/*/*.cpp) $(wildcard $(ENGINE_SRC_DIR)/*/*/*/*.cpp)
OBJS_NIX_SO 			= $(patsubst %.cpp,%.nix.o,$(SRCS_NIX_SO))
BASE_NIX_SO 			= lib($LIB_NAME)
STATIC_NIX_SO 			= $(ENGINE_LIB_DIR)/nix/$(BASE_NIX_SO).so.a
DYNAMIC_NIX_SO 			= $(ENGINE_LIB_DIR)/nix/$(BASE_NIX_SO).so
# External Engine's SO
EXT_NIX_INC_DIR 		= 
EXT_NIX_LB_FLAGS 		= 
EXT_NIX_DEPS 			= -l$(LIB_NAME) -l$(NIX_SO_DEPS)
EXT_NIX_LINKS 			= $(UF_LIBS) $(EXT_LIBS) $(EXT_NIX_DEPS)
EXT_NIX_FLAGS 			= $(FLAGS)

EXT_NIX_LIB_DIR 		= $(NIX_LIB_DIR)
EXT_NIX_INCS 			= -I$NIX_INCs) -I$(EXT_NIX_INC_DIR)
EXT_NIX_LIBS 			= -L$NIX_LIBs) -L$(EXT_NIX_LIB_DIR)

SRCS_EXT_NIX_SO 		= $(wildcard $(EXT_SRC_DIR)/*.cpp) $(wildcard $(EXT_SRC_DIR)/*/*.cpp) $(wildcard $(EXT_SRC_DIR)/*/*/*.cpp) $(wildcard $(EXT_SRC_DIR)/*/*/*/*.cpp)
OBJS_EXT_NIX_SO 		= $(patsubst %.cpp,%.nix.o,$(SRCS_EXT_NIX_SO))
BASE_EXT_NIX_SO 		= lib($EXT_LIB_NAME)
STATIC_EXT_NIX_SO 		= $(ENGINE_LIB_DIR)/nix/$(BASE_EXT_NIX_SO).so.a
DYNAMIC_EXT_NIX_SO 		= $(ENGINE_LIB_DIR)/nix/$(BASE_EXT_NIX_SO).so
# Client executable
SRCS_NIX 				= $(wildcard $(CLIENT_SRC)/*.cpp) $(wildcard $(CLIENT_SRC)/*/*.cpp)
OBJS_NIX 				= $(patsubst %.cpp,%.nix.o,$(SRCS_NIX))
TARGET_NIX 				= $(BIN_DIR)/$(TARGET_NAME)

nix: $(DYNAMIC_NIX_SO) $(EXT_NIX_SO) $(TARGET_NIX)
$(TARGET_NIX): $(OBJS_NIX)
	$(NIX_CC) $(FLAGS) $(OBJS_NIX) $(NIX_LIBS) $(NIX_INCS) $(NIX_LINKS) -l$(LIB_NAME) -l$(EXT_LIB_NAME) -o $(TARGET_NIX)

rm-nix:
	-rm $(DYNAMIC_NIX_SO)
	-rm $(TARGET_NIX)

%.nix.o: %.cpp
	$(NIX_CC) $(FLAGS) $(NIX_INCS) -fPIC -c $< -o $@

$(DYNAMIC_NIX_SO): FLAGS += -DUF_EXPORTS
$(DYNAMIC_NIX_SO): $(OBJS_NIX_SO) 
	$(NIX_CC) -shared -o $(DYNAMIC_NIX_SO) $(OBJS_NIX_SO) $(NIX_LIBS) $(NIX_INCS) $(NIX_LINKS)
	cp $(ENGINE_LIB_DIR)/nix/$(BASE_NIX_SO).so $(BIN_DIR)/lib/nix/$(BASE_NIX_SO).so
$(DYNAMIC_EXT_NIX_SO): FLAGS += -DEXT_EXPORTS
$(DYNAMIC_EXT_NIX_SO): $(OBJS_EXT_NIX_SO) 
	$(NIX_CC) -shared -o $(DYNAMIC_EXT_NIX_SO) $(OBJS_EXT_NIX_SO) $(EXT_NIX_LIBS) $(EXT_NIX_INCS) $(EXT_NIX_LINKS)
	cp $(ENGINE_LIB_DIR)/nix/$(BASE_EXT_NIX_SO).so $(BIN_DIR)/lib/nix/$(BASE_EXT_NIX_SO).so
clean-nix:
	@-rm -f $(OBJS_NIX_SO)
	@-rm -f $(OBJS_NIX)

WIN32_CC 				= i686-w64-mingw32-g++
# Base Engine's DLL
WIN32_INC_DIR 			= $(ENGINE_INC_DIR)/win32
WIN32_LB_FLAGS 			= $(ENGINE_LIB_DIR)/win32
WIN32_DEPS 				= -lglew32 -lopengl32 -lglu32 -lgdi32 -lncursesw
WIN32_LINKS 			= $(UF_LIBS) $(EXT_LIBS) $(WIN32_DEPS)
WIN32_FLAGS 			= $(FLAGS) 
#-Wl,-subsystem,windows

WIN32_LIB_DIR 			= $(ENGINE_LIB_DIR)/win32/
WIN32_INCS 				= -I$(ENGINE_INC_DIR) -I$(WIN32_INC_DIR)
WIN32_LIBS 				= -L$(ENGINE_LIB_DIR) -L$(WIN32_LIB_DIR)

SRCS_WIN32_DLL 			= $(wildcard $(ENGINE_SRC_DIR)/*.cpp) $(wildcard $(ENGINE_SRC_DIR)/*/*.cpp) $(wildcard $(ENGINE_SRC_DIR)/*/*/*.cpp) $(wildcard $(ENGINE_SRC_DIR)/*/*/*/*.cpp) $(wildcard $(ENGINE_SRC_DIR)/*/*/*/*/*.cpp)
OBJS_WIN32_DLL 			= $(patsubst %.cpp,%.win32.o,$(SRCS_WIN32_DLL))
BASE_WIN32_DLL 			= lib$(LIB_NAME)
IM_WIN32_DLL 			= $(ENGINE_LIB_DIR)/win32/$(BASE_WIN32_DLL).dll.a
EX_WIN32_DLL 			= $(BIN_DIR)/lib/win32/$(BASE_WIN32_DLL).dll
# External Engine's DLL
EXT_WIN32_INC_DIR 		= $(WIN32_INC_DIR)
EXT_WIN32_LB_FLAGS 		= $(WIN32_LIB_DIR)
EXT_WIN32_DEPS 			= -l$(LIB_NAME) $(WIN32_DEPS)
EXT_WIN32_LINKS 		= $(UF_LIBS) $(EXT_LIBS) $(EXT_WIN32_DEPS)
EXT_WIN32_FLAGS 		= $(FLAGS) 
#-Wl,-subsystem,windows

EXT_WIN32_LIB_DIR 		= $(ENGINE_LIB_DIR)/win32/
EXT_WIN32_INCS 			= -I$(ENGINE_INC_DIR) -I$(EXT_WIN32_INC_DIR)
EXT_WIN32_LIBS 			= -L$(ENGINE_LIB_DIR) -L$(EXT_WIN32_LIB_DIR)

SRCS_EXT_WIN32_DLL 		= $(wildcard $(EXT_SRC_DIR)/*.cpp)
OBJS_EXT_WIN32_DLL 		= $(patsubst %.cpp,%.win32.o,$(SRCS_EXT_WIN32_DLL))
BASE_EXT_WIN32_DLL 		= lib$(EXT_LIB_NAME)
EXT_IM_WIN32_DLL 		= $(ENGINE_LIB_DIR)/win32/$(BASE_EXT_WIN32_DLL).dll.a
EXT_EX_WIN32_DLL 		= $(BIN_DIR)/lib/win32/$(BASE_EXT_WIN32_DLL).dll
# Client EXE
SRCS_WIN32 				= $(wildcard $(CLIENT_SRC_DIR)/*.cpp) $(wildcard $(CLIENT_SRC_DIR)/*/*.cpp)
OBJS_WIN32 				= $(patsubst %.cpp,%.win32.o,$(SRCS_WIN32))
TARGET_WIN32 			= $(BIN_DIR)/$(TARGET_NAME)-i686.exe

win32: $(EX_WIN32_DLL) $(EXT_EX_WIN32_DLL) $(TARGET_WIN32)

rm-exe32:
	-rm $(EX_WIN32_DLL)
	-rm $(TARGET_WIN32)

%.win32.o: %.cpp
	$(WIN32_CC) $(WIN32_FLAGS) $(WIN32_INCS) -c $< -o $@

$(EX_WIN32_DLL): WIN32_FLAGS += -DUF_EXPORTS
$(EX_WIN32_DLL): $(OBJS_WIN32_DLL) 
	$(WIN32_CC) -shared -o $(EX_WIN32_DLL) -Wl,--out-implib=$(IM_WIN32_DLL) $(OBJS_WIN32_DLL) $(WIN32_LIBS) $(WIN32_INCS) $(WIN32_LINKS)
	cp $(ENGINE_LIB_DIR)/win32/$(BASE_WIN32_DLL).dll.a $(ENGINE_LIB_DIR)/win32/$(BASE_WIN32_DLL).a

$(EXT_EX_WIN32_DLL): WIN32_FLAGS += -DEXT_EXPORTS
$(EXT_EX_WIN32_DLL): $(OBJS_EXT_WIN32_DLL) 
	$(WIN32_CC) -shared -o $(EXT_EX_WIN32_DLL) -Wl,--out-implib=$(EXT_IM_WIN32_DLL) $(OBJS_EXT_WIN32_DLL) $(EXT_WIN32_LIBS) $(EXT_WIN32_INCS) $(EXT_WIN32_LINKS)
	cp $(ENGINE_LIB_DIR)/win32/$(BASE_EXT_WIN32_DLL).dll.a $(ENGINE_LIB_DIR)/win32/$(BASE_EXT_WIN32_DLL).a

$(TARGET_WIN32): $(OBJS_WIN32)
	$(WIN32_CC) $(WIN32_FLAGS) $(OBJS_WIN32) $(WIN32_LIBS) $(WIN32_INCS) $(WIN32_LINKS) -l$(LIB_NAME) -l$(EXT_LIB_NAME) -o $(TARGET_WIN32)

clean-win32:
	@-rm $(EX_WIN32_DLL)
	@-rm $(TARGET_WIN32)
	@-rm -f $(OBJS_WIN32_DLL)
	@-rm -f $(OBJS_EXT_WIN32_DLL)
	@-rm -f $(OBJS_WIN32)

#WIN64_CC 				= i686-w64-mingw32-g++
WIN64_CC 				= x86_64-w64-mingw32-g++
# Base Engine's DLL
WIN64_INC_DIR 			= $(ENGINE_INC_DIR)/win64
WIN64_LB_FLAGS 			= $(ENGINE_LIB_DIR)/win64
WIN64_DEPS 				= -lglew32 -lopengl32 -lglu32 -lgdi32 -lncursesw
#WIN64_DEPS 				= -lvulkan -lncursesw
WIN64_LINKS 			= $(UF_LIBS) $(EXT_LIBS) $(WIN64_DEPS)
WIN64_FLAGS 			= $(FLAGS) 
#-Wl,-subsystem,windows

WIN64_LIB_DIR 			= $(ENGINE_LIB_DIR)/win64/
WIN64_INCS 				= -I$(ENGINE_INC_DIR) -I$(WIN64_INC_DIR)
WIN64_LIBS 				= -L$(ENGINE_LIB_DIR) -L$(WIN64_LIB_DIR)

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
EXT_WIN64_INCS 			= -I$(ENGINE_INC_DIR) -I$(EXT_WIN64_INC_DIR)
EXT_WIN64_LIBS 			= -L$(ENGINE_LIB_DIR) -L$(EXT_WIN64_LIB_DIR)

SRCS_EXT_WIN64_DLL 		= $(wildcard $(EXT_SRC_DIR)/*.cpp) $(wildcard $(EXT_SRC_DIR)/*/*.cpp) $(wildcard $(EXT_SRC_DIR)/*/*/*.cpp)
OBJS_EXT_WIN64_DLL 		= $(patsubst %.cpp,%.win64.o,$(SRCS_EXT_WIN64_DLL))
BASE_EXT_WIN64_DLL 		= lib$(EXT_LIB_NAME)
EXT_IM_WIN64_DLL 		= $(ENGINE_LIB_DIR)/win64/$(BASE_EXT_WIN64_DLL).dll.a
EXT_EX_WIN64_DLL 		= $(BIN_DIR)/lib/win64/$(BASE_EXT_WIN64_DLL).dll
# Client EXE
SRCS_WIN64 				= $(wildcard $(CLIENT_SRC_DIR)/*.cpp) $(wildcard $(CLIENT_SRC_DIR)/*/*.cpp)
OBJS_WIN64 				= $(patsubst %.cpp,%.win64.o,$(SRCS_WIN64))
TARGET_WIN64 			= $(BIN_DIR)/$(TARGET_NAME).exe

win64: $(EX_WIN64_DLL) $(EXT_EX_WIN64_DLL) $(TARGET_WIN64)

rm-exe64:
	-rm $(EX_WIN64_DLL)
	-rm $(EXT_EX_WIN64_DLL)
	-rm $(TARGET_WIN64)

%.win64.o: %.cpp
	$(WIN64_CC) $(WIN64_FLAGS) $(WIN64_INCS) -c $< -o $@

$(EX_WIN64_DLL): WIN64_FLAGS += -DUF_EXPORTS
$(EX_WIN64_DLL): $(OBJS_WIN64_DLL) 
	$(WIN64_CC) -shared -o $(EX_WIN64_DLL) -Wl,--out-implib=$(IM_WIN64_DLL) $(OBJS_WIN64_DLL) $(WIN64_LIBS) $(WIN64_INCS) $(WIN64_LINKS)
	cp $(ENGINE_LIB_DIR)/win64/$(BASE_WIN64_DLL).dll.a $(ENGINE_LIB_DIR)/win64/$(BASE_WIN64_DLL).a

$(EXT_EX_WIN64_DLL): WIN64_FLAGS += -DEXT_EXPORTS
$(EXT_EX_WIN64_DLL): $(OBJS_EXT_WIN64_DLL) 
	$(WIN64_CC) -shared -o $(EXT_EX_WIN64_DLL) -Wl,--out-implib=$(EXT_IM_WIN64_DLL) $(OBJS_EXT_WIN64_DLL) $(EXT_WIN64_LIBS) $(EXT_WIN64_INCS) $(EXT_WIN64_LINKS)
	cp $(ENGINE_LIB_DIR)/win64/$(BASE_EXT_WIN64_DLL).dll.a $(ENGINE_LIB_DIR)/win64/$(BASE_EXT_WIN64_DLL).a

$(TARGET_WIN64): $(OBJS_WIN64)
	$(WIN64_CC) $(WIN64_FLAGS) $(OBJS_WIN64) $(WIN64_LIBS) $(WIN64_INCS) $(WIN64_LINKS) -l$(LIB_NAME) -l$(EXT_LIB_NAME) -o $(TARGET_WIN64)

clean-win64:
	@-rm $(EX_WIN64_DLL)
	@-rm $(EXT_EX_WIN64_DLL)
	@-rm $(TARGET_WIN64)
	
	@-rm -f $(OBJS_WIN64_DLL)
	@-rm -f $(OBJS_EXT_WIN64_DLL)
	@-rm -f $(OBJS_WIN64)