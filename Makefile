CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pthread -D_POSIX_C_SOURCE=200809L -g
LDFLAGS = -pthread
ifeq ($(shell uname),Darwin)
# macOS doesn't need -lcrypt
else
LDFLAGS += -lcrypt
endif

# Directories
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin

# Source files - FIXED: removed duplicate SOURCES line
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
TARGET = $(BIN_DIR)/os_mini_server

# Default target
all: $(TARGET)

# Create directories
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Build the main executable
$(TARGET): $(OBJECTS) | $(BIN_DIR)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	rm -rf server_storage
	@echo "Clean complete"

# Clean and rebuild
rebuild: clean all

# Create necessary directories for server storage
setup:
	mkdir -p server_storage/users
	mkdir -p server_storage/metadata
	@echo "Directory structure created"

# Install (optional)
install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

# Run the server
run: $(TARGET)
	./$(TARGET)

# Run with custom port
run-port: $(TARGET)
	./$(TARGET) 9090

# Debug build
debug: CFLAGS += -g -DDEBUG -O0
debug: $(TARGET)

# Release build
release: CFLAGS += -O2 -DNDEBUG
release: clean $(TARGET)

# Run with valgrind for memory leak detection
valgrind: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TARGET)

# Help target
help:
	@echo "Available targets:"
	@echo "  all       - Build the server (default)"
	@echo "  clean     - Remove build artifacts"
	@echo "  rebuild   - Clean and rebuild"
	@echo "  setup     - Create server storage directories"
	@echo "  run       - Build and run server on default port 8080"
	@echo "  run-port  - Build and run server on port 9090"
	@echo "  debug     - Build with debug symbols"
	@echo "  release   - Build optimized release version"
	@echo "  install   - Install to /usr/local/bin"
	@echo "  valgrind  - Run with Valgrind memory checker"
	@echo "  asan      - Build with AddressSanitizer"
	@echo "  tsan      - Build with ThreadSanitizer"
	@echo "  help      - Show this help message"

.PHONY: all clean rebuild setup install run run-port debug release help valgrind asan tsan

# Sanitizers
asan: CFLAGS += -g -fsanitize=address -fno-omit-frame-pointer
asan: LDFLAGS += -fsanitize=address
asan: clean $(TARGET)

tsan: CFLAGS += -g -fsanitize=thread -fno-omit-frame-pointer
tsan: LDFLAGS += -fsanitize=thread
tsan: clean $(TARGET)