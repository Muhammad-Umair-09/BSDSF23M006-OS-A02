# ==============================
# Makefile for Feature 6: ls-v1.5.0 (Colorized Output)
# ==============================

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11

# Directory structure
SRC_DIR = src
BIN_DIR = bin
OBJ_DIR = obj

# Files
TARGET = $(BIN_DIR)/lsv1.6.0
SRC = $(SRC_DIR)/lsv1.6.0.c
OBJ = $(OBJ_DIR)/lsv1.6.0.o

# ==============================
# Default target
# ==============================
all: $(TARGET)

# Linking step
$(TARGET): $(OBJ)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)
	@echo "Build complete: $(TARGET)"

# Compilation step
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "🧩 Compiled: $< → $@"

# ==============================
# Clean target (removes binaries and objects)
# ==============================
clean:
	rm -f $(OBJ_DIR)/*.o $(BIN_DIR)/lsv1.5.0
	@echo "🧹 Cleaned build files (directories kept)."

# ==============================
# Run target (build + execute)
# ==============================
run: all
	@echo "🔹 Running ls-v1.5.0:"
	@$(TARGET)

.PHONY: all clean run
