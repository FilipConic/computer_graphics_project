CC = gcc
FLAGS = -Wall -Wextra -ggdb

BUILD_DIR = ./build
TARGET = main

SRCS = main.c
OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o)

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

$(BUILD_DIR)/%.o: %.c
	$(CC) $(FLAGS) -c $< -o $@ $(FLAGS_EXTRA)

clear:
	rm -rf $(BUILD_DIR) $(TARGET)
