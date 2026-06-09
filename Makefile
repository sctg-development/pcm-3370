# Makefile for PCM-3370 Stress Test Program
# Target: Windows XP 32-bit compatible executable
# Toolchain: LLVM-MinGW for Windows XP
# Author: Cline
# Date: 2026

# Toolchain configuration
CC := $(shell pwd)/./llvm-mingw-xp-22.1.0-msvcrt-macos-universal/bin/i686-w64-mingw32-clang
CFLAGS := -Wall -Wextra -std=c11 -O2 -m32 -D_WIN32_WINNT=0x0501 -target i686-w64-mingw32 -march=pentium3 -mno-sse2
CFLAGS_DEBUG := -Wall -Wextra -std=c11 -g -O0 -m32 -D_WIN32_WINNT=0x0501 -target i686-w64-mingw32 -march=pentium3 -mno-sse2
LDFLAGS := -static -lole32 -loleaut32 -lwbemuuid -luser32 -lkernel32 -ladvapi32
TARGET := pcm3370_stress_test.exe
TARGET_DEBUG := pcm3370_stress_test_debug.exe
SRC_DIR := src
SRC_FILES := $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES := $(SRC_FILES:.c=.o)
OBJ_FILES_DEBUG := $(SRC_FILES:.c=_debug.o)

# Default target
all: $(TARGET) $(TARGET_DEBUG)

# Build release executable
$(TARGET): $(OBJ_FILES)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Build debug executable
$(TARGET_DEBUG): $(OBJ_FILES_DEBUG)
	$(CC) $(CFLAGS_DEBUG) $^ -o $@ $(LDFLAGS)

# Compile source files for release
$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compile source files for debug
$(SRC_DIR)/%_debug.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS_DEBUG) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(TARGET) $(TARGET_DEBUG) $(OBJ_FILES) $(OBJ_FILES_DEBUG)

# Package for distribution
package: clean all
	mkdir -p dist
	cp $(TARGET) $(TARGET_DEBUG) dist/
	cd dist && zip pcm3370_stress_test.zip $(TARGET) $(TARGET_DEBUG)

.PHONY: all clean package