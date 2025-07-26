# Makefile for TCP Simulator

CC = gcc
CFLAGS = -Wall -Wextra -g -lpthread

SRC = app_layer.c tcp_layer.c
# 调试版本
# SRC = app_layer.c tcp_layer_debug.c

OBJ = launcher.o
TARGET = tcp_sim

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJ) $(SRC)
	$(CC) $(OBJ) $(SRC) -o $(TARGET) $(CFLAGS)

run: $(TARGET)
	./$(TARGET) -w 1000 -m 1000

clean:
	rm -f $(TARGET)
