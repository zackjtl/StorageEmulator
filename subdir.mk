SUBDIRS=
SRC_FILES=Global.cpp \
					Pch.cpp

#This line for auto find source files under directory
#SRC_FILES=$(shell find $(SRC) -type f -name *.cpp)

OBJ_FILES=$(patsubst $(SRC)%.cpp,$(OBJ)%.o,$(SRC_FILES))

all:$(SUBDIRS) build

$(SUBDIRS):ECHO
	make -C $@ -f subdir.mk

ECHO:
	@echo begin compile

build:
	@echo ====== Compile current directory: $(shell pwd) ========
	$(MYCXX) $(CPFLAGS) $(INCLUDES) -c $(SRC_FILES)
	mv $(OBJ_FILES) $(OBJS_DIR)
