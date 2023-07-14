################################################################################
# Makefile for "memoria" project
################################################################################

CC := gcc
CFLAGS := -I"../shared/src" -Wall -w
LDFLAGS := -L"../shared/Debug" 
LIBS := -lcommons -lshared

SRC_DIR := src
INC_DIR := include
OBJ_DIR := Debug

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

TARGET := memoria

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(DEPS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

valgrind: $(TARGET)
	valgrind ./$(TARGET)
