CC = gcc
FLAGS = -Wall -Wextra -ggdb

BUILD_DIR = ./build
TARGET = main

SRCS_DIR = ./srcs
SRCS = ttf.c vec2.c ear_clipping.c font.c shapes.c
OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o) $(BUILD_DIR)/main.o

INC_DIR = ./includes/
LIB_DIR = ./libs/
LIBS = GLEW X11 GL Xrandr m
FLAGS_EXTRA = -L$(LIB_DIR) -I$(INC_DIR) $(LIBS:%=-l%)

.PHONY: all clear

all: $(BUILD_DIR) $(TARGET)
	
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJS)
	$(CC) $(FLAGS) $^ -o $@ $(FLAGS_EXTRA)

$(BUILD_DIR)/%.o: $(SRCS_DIR)/%.c
	$(CC) $(FLAGS) -c $< -o $@ $(FLAGS_EXTRA)

$(BUILD_DIR)/main.o: $(SRCS_DIR)/main.c $(INC_DIR)/context.h
	$(CC) $(FLAGS) -c $< -o $@ $(FLAGS_EXTRA)

clear:
	rm -rf $(BUILD_DIR) $(TARGET)
