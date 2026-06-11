# Makefile for HTML Report Test Program
# Target: host native test executable
# Author: Ronan Le Meillat
# Date: 2026
#
# Copyright (c) 2026 Ronan LE MEILLAT - SCTG Development for ISMO Group Ltd.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# Compiler configuration for host native
CC := cc
CFLAGS := -Wall -Wextra -std=c11 -O0 -g
TARGET := test_html_report
SRC_FILES := test.html.c
INCLUDE_PATH := -I../src

# Default target
all: $(TARGET)

# Build test executable
$(TARGET): $(SRC_FILES)
	$(CC) $(CFLAGS) $(INCLUDE_PATH) $< html_report_test.c -o $@

# Clean build artifacts
clean:
	rm -f $(TARGET) 123456789.html

.PHONY: all clean