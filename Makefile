#
#         File: Makefile
#       Author: Danny He
#      License: Apache License
#         Date: 22th February 2022
#  Description: Makefile to build lisp
#

CC  = g++

HEADERS_DIR = header
OUTPUT_DIR = build
OBJ_DIR = $(OUTPUT_DIR)/obj

LDFLAGS = -shared -export-dynamic
CFLAGS 	= -pedantic -Wall -Wno-gnu-statement-expression -I$(HEADERS_DIR)  -std=c++11

BIN_TARGET  = $(OUTPUT_DIR)/lisp.out
BIN_SRC = main-repl.cpp
# BIN_OBJ = $(BIN_SRC:%.cpp=$(OBJ_DIR)/%.o)

LIB_TARGET	= $(OUTPUT_DIR)/lisp.so
LIB_SRC_DIR = lib-src
LIB_SRC = $(wildcard $(LIB_SRC_DIR)/*.cpp)
LIB_OBJ = $(LIB_SRC:$(LIB_SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

VM_TARGET = $(OUTPUT_DIR)/vm.out
VM_SRC_DIR = vm-src
VM_SRC = $(wildcard $(VM_SRC_DIR)/*.cpp)
VM_OBJ = $(VM_SRC:$(VM_SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)


all: PREP help

lib: PREP $(LIB_TARGET)
bin: PREP $(BIN_TARGET)
vm: PREP $(VM_TARGET)

PREP:
	@mkdir -p $(OBJ_DIR) $(OUTPUT_DIR)

help:
	@echo make [option]
	@echo 	options:
	@echo 		bin - build the binary
	@echo 		lib - build the library
	@echo 		vm  - build the virtual (register) machine

$(OBJ_DIR)/%.o: $(LIB_SRC_DIR)/%.cpp 
	$(CC) $(CFLAGS) -c $< -o $@
$(OBJ_DIR)/%.o: $(VM_SRC_DIR)/%.cpp 
	$(CC) $(CFLAGS) -c $< -o $@

# obj/%.o : $(LIB_SRC_DIR)/%.c 
# 	$(CC) $(CFLAGS) -c $< -o $@

$(LIB_TARGET): $(LIB_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^
# $(LIB_SRC)
# $^, replace with the arguements 

$(BIN_TARGET): $(LIB_TARGET) $(BIN_SRC)
	$(CC) $(CFLAGS) -o $@ $^

$(VM_TARGET): $(LIB_TARGET) $(VM_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

clean:
	rm -r $(OBJ_DIR) $(OUTPUT_DIR)
