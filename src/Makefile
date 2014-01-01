# $File: Makefile
# $Date: Thu Dec 26 10:42:09 2013 +0800


Sugar ?= mono ~/Work/SugarCpp-C\#/src/SugarCpp.CommandLine/bin/Debug/SugarCpp.CommandLine.exe
OBJ_DIR = obj
TARGET = main

DEFINES = -DDEBUG

OPTFLAGS = -Wall -Wextra -Wconversion -O0 -g

CXXFLAGS += -std=c++11 -pthread
CXXFLAGS += $(DEFINES) $(OPTFLAGS)
LDFLAGS += -pthread

CC = g++
SHELL = bash
ccSOURCES = $(shell find -name "*.sc" | sed 's/^\.\///g')
OBJS = $(addprefix $(OBJ_DIR)/,$(ccSOURCES:.sc=.o))

.PHONY: all clean

all: client server

client: $(addprefix $(OBJ_DIR)/, Client.o Common.o Socket.o Command.o)
	@echo "Linking ..."
	@$(CC) $^ -o $@ $(LDFLAGS)
	@echo "done."

server: $(addprefix $(OBJ_DIR)/, Server.o Common.o Socket.o Command.o)
	@echo "Linking ..."
	@$(CC) $^ -o $@ $(LDFLAGS)
	@echo "done."

.PRECIOUS: $(OBJ_DIR)/%.cpp

$(OBJ_DIR)/%.cpp: %.sc $(OBJ_DIR)
	$(Sugar) $< -o $(OBJ_DIR) --nocode

$(OBJ_DIR)/%.o: $(OBJ_DIR)/%.cpp
	@echo "[cc] $< ..."
	@$(CC) -c $< -o $@ $(CXXFLAGS)

$(OBJ_DIR):
	mkdir -p $@

clean:
	@rm -rf $(OBJ_DIR)
