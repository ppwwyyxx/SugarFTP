# $File: Makefile
# $Date: Wed Jan 01 19:18:40 2014 +0800


OBJ_DIR = obj

DEFINES = -DDEBUG

OPTFLAGS = -Wall -Wextra -Wconversion -O3

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

$(OBJ_DIR)/%.o: $(OBJ_DIR)/%.cpp
	@echo "[cc] $< ..."
	@$(CC) -c $< -o $@ $(CXXFLAGS)

$(OBJ_DIR):
	mkdir -p $@

clean:
	@rm -rf $(OBJ_DIR)
