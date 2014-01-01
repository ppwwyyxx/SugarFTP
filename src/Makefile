# $File: Makefile
# $Date: Wed Jan 01 19:22:18 2014 +0800


Sugar ?= mono ~/Work/SugarCpp-C\#/src/SugarCpp.CommandLine/bin/Debug/SugarCpp.CommandLine.exe
OBJ_DIR = obj
CPP_DIR = ../cpp

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

.PRECIOUS: $(CPP_DIR)/%.cpp

$(CPP_DIR)/%.cpp: %.sc
	$(Sugar) $< -o $(CPP_DIR) --nocode

$(OBJ_DIR)/%.o: $(CPP_DIR)/%.cpp $(OBJ_DIR)
	@echo "[cc] $< ..."
	@$(CC) -c $< -o $@ $(CXXFLAGS)

$(OBJ_DIR):
	mkdir -p $@

clean:
	@rm -rf $(CPP_DIR)/*.{cpp,h}
	@rm -rf $(OBJ_DIR)
