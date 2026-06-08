# Makefile for PCM-3370 Stress Test Program
# Target: Windows XP 32-bit compatible executable
# Toolchain: LLVM-MinGW for Windows XP
# Author: Cline
# Date: 2026

# Toolchain configuration
CC := $(shell pwd)/./llvm-mingw-xp-22.1.0-msvcrt-macos-universal/bin/i686-w64-mingw32-clang
CFLAGS := -Wall -Wextra -std=c11 -O2 -m32 -D_WIN32_WINNT=0x0501 -target i686-w64-mingw32
LDFLAGS := -static -luser32 -lkernel32 -ladvapi32
TARGET := pcm3370_stress_test.exe
SRC_DIR := src
SRC_FILES := $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES := $(SRC_FILES:.c=.o)

# Default target
all: $(TARGET)

# Build executable
$(TARGET): $(OBJ_FILES)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Compile source files
$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(TARGET) $(OBJ_FILES)

# Package for distribution
package: clean all
	mkdir -p dist
	cp $(TARGET) dist/
	cd dist && zip pcm3370_stress_test.zip $(TARGET)

.PHONY: all clean package