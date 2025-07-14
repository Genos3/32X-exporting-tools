# CC = gcc
CC = C:\games\msys64\mingw64\bin\gcc

SRC_DIR = ../source

CFLAGS = -O2 -DPC -std=c99 -mwindows -Wall -Wfatal-errors -Wno-maybe-uninitialized -Wno-unused-function -fms-extensions -I$(SRC_DIR)
# CFLAGS += -pedantic -Wextra -Wwrite-strings
LDFLAGS = -lgdi32 -lwinmm -lshlwapi

BUILD = build
SRCS = $(SRC_DIR) $(SRC_DIR)/engine

FILES_CC = $(foreach dir, $(SRCS), $(wildcard $(dir)/*.c))

OBJS = $(addprefix $(BUILD)/, $(notdir $(FILES_CC:%.c=%.o)))

# SRCS += $(foreach dir, $(wildcard data/*), $(wildcard $(dir)/*.c))

# OBJS = $(SRCS:%.c=$(BUILD)/%.o)

TARGET = map_editor

.PHONY: all clean

all: $(TARGET)

VPATH = $(SRCS)

$(TARGET): $(OBJS)
	@echo linking
	@$(CC) $^ $(LDFLAGS) -o $@
	@echo built $(TARGET)

$(BUILD)/%.o: %.c | $(BUILD)
	@echo $<
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD):
	@mkdir -p $@

clean:
	rm -rf $(BUILD)/* $(TARGET)
