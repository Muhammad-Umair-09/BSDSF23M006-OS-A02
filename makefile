# Feature 2: Long Listing Format (v1.1.0)
# =======================================

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11

# Directory structure
SRC_DIR = src
BIN_DIR = bin
OBJ_DIR = obj

# Files
TARGET = $(BIN_DIR)/lsv1.1.0
SRC = $(SRC_DIR)/lsv1.1.0.c
OBJ = $(OBJ_DIR)/lsv1.1.0.o

# Default target
all: $(TARGET)

# Linking
$(TARGET): $(OBJ)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)
	@echo "✅ Build complete: $(TARGET)"

# Compilation
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target (keep directories)
clean:
	rm -f $(OBJ_DIR)/*.o $(BIN_DIR)/lsv1.1.0
	@echo "🧹 Cleaned build files (directories kept)."

# Run target
run: all
	@echo "🔹 Output:"
	@$(TARGET)

.PHONY: all clean run
