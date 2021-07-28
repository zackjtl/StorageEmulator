MYCXX=$(CXX)

CPFLAGS=-std=c++11 -w -fpermissive -D IsNormalVersion -fexceptions -fpic -static-libstdc++ $(MACHINE)
LDFLAGS=-std=c++11 -w -fpermissive -static-libstdc++ $(MACHINE)

INCDIR= ./

SUBDIRS=./

OBJ_FILES=$(patsubst $(SRC)%.cpp,$(OBJ)%.o,$(SRC_FILES))

INCLUDES  = $(addprefix -I, $(shell pwd) ) $(addprefix -I$(shell pwd)/, $(INCDIR) )

BIN=test
BIN_DIR=bin
ROOT_DIR=$(shell pwd)
OBJS_DIR=$(ROOT_DIR)/objs
ALL_OBJS=$(wildcard $(OBJS_DIR)/*.o)

export MYCXX ROOT_DIR OBJS_DIR INCLUDES CPFLAGS LDFLAGS

all:CHECK_DIR $(SUBDIRS) link
CHECK_DIR:
	@echo CHECK_DIR
	@echo $(LDFLAGS)
	mkdir -p $(OBJS_DIR)

$(SUBDIRS):ECHO
	make -C $@ -f subdir.mk

ECHO:
	@echo Build $(SUBDIRS)

link:
	@echo "Linking.. "
	@$(MYCXX) $(LDFLAGS) -shared -o $(BIN_DIR)/$(BIN) $(ALL_OBJS)
	##$(MYCXX) $(LDFLAGS) -o $(BIN_DIR)/$(BIN) $(ALL_OBJS)

clean:
	@$(RM) $(OBJS_DIR)/*.o
	@rm $(BIN_DIR)/$(BIN)
