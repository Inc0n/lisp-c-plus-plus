#
#         File: Makefile
#       Author: Danny He
#      License: Apache License
#         Date: 22th February 2022
#  Description: Makefile to build lisp
#

CC  = g++

HEADERS_DIR = header

LDFLAGS = -shared -export-dynamic
CFLAGS 	= -pedantic -Wall -Wno-gnu-statement-expression -I$(HEADERS_DIR)  -std=c++11
OUTPUT_DIR = build
OBJ_DIR = $(OUTPUT_DIR)/obj


BIN_TARGET  = $(OUTPUT_DIR)/lisp.out

BIN_SRC = main-repl.cpp
# BIN_OBJ = $(BIN_SRC:%.cpp=$(OBJ_DIR)/%.o)


LIB_TARGET	= $(OUTPUT_DIR)/lisp.so

LIB_SRC_DIR = lib-src
LIB_SRC = $(wildcard $(LIB_SRC_DIR)/*.cpp)
# LIB_OBJ = $($(notdir $(LIB_SRC)):%.c=$(OBJ_DIR)/%.o)
LIB_OBJ = $(LIB_SRC:$(LIB_SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
# data.c env.c lisp.c reader.c

# $(addprefix $(OBJ_DIR)/, )

all: PREP help
# all:
# cd lib-src && $(CC)  -std=c++11 -I../$(HEADERS_DIR) data.cpp env.cpp reader.cpp lisp.cpp stacktrace.cpp ../main-repl.cpp -o $(BIN_TARGET)

lib: PREP $(LIB_TARGET)
bin: PREP $(BIN_TARGET)

PREP:
	@mkdir -p $(OBJ_DIR) $(OUTPUT_DIR)

help:
	@echo make [option]
	@echo 	options:
	@echo 		bin - build the binary
	@echo 		lib - build the library

$(OBJ_DIR)/%.o : $(LIB_SRC_DIR)/%.cpp 
	$(CC) $(CFLAGS) -c $< -o $@

# obj/%.o : $(LIB_SRC_DIR)/%.c 
# 	$(CC) $(CFLAGS) -c $< -o $@

$(LIB_TARGET): $(LIB_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^
# $(LIB_SRC)
# $^, replace with the arguements 

$(BIN_TARGET): $(LIB_TARGET) $(BIN_SRC)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -r $(OBJ_DIR) $(OUTPUT_DIR)
