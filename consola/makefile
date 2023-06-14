################################################################################
# Makefile for "consola" project
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

CONFIG := consola.config
INSTRUCCIONES := instrucciones.txt
INS_T1 := BASE_1
INS_T2 := BASE_2

TARGET := consola

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(DEPS) $(TARGET)

run: $(TARGET)
	./$(TARGET) $(CONFIG) $(INSTRUCCIONES)

valgrind: $(TARGET)
	valgrind ./$(TARGET) $(CONFIG) $(INSTRUCCIONES)
	
test1: $(TARGET)
	valgrind ./$(TARGET) $(CONFIG) $(INS_T1)

test2: $(TARGET)
	valgrind ./$(TARGET) $(CONFIG) $(INS_T2)