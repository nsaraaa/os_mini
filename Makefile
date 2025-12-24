CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pthread -D_POSIX_C_SOURCE=200809L
LDFLAGS = -pthread
ifeq ($(shell uname),Darwin)
else
LDFLAGS += -lcrypt
endif

# Directories
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
TARGET = $(BIN_DIR)/os_mini_server
SOURCES = $(wildcard $(SRC_DIR)/*.c)
# This should automatically include locking.c, task_queue.c, worker_pool.c

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

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

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
debug: CFLAGS += -g -DDEBUG
debug: $(TARGET)

# Release build
release: CFLAGS += -O2 -DNDEBUG
release: $(TARGET)

# Help target
help:
	@echo "Available targets:"
	@echo "  all       - Build the server (default)"
	@echo "  clean     - Remove build artifacts"
	@echo "  run       - Build and run server on default port 8080"
	@echo "  run-port  - Build and run server on port 9090"
	@echo "  debug     - Build with debug symbols"
	@echo "  release   - Build optimized release version"
	@echo "  install   - Install to /usr/local/bin"
	@echo "  help      - Show this help message"
	@echo "  asan      - Build with AddressSanitizer"
	@echo "  tsan      - Build with ThreadSanitizer (macOS/Clang)"

.PHONY: all clean install run run-port debug release help asan tsan

# Sanitizers
asan: CFLAGS += -g -fsanitize=address -fno-omit-frame-pointer
asan: LDFLAGS += -fsanitize=address
asan: $(TARGET)

tsan: CFLAGS += -g -fsanitize=thread -fno-omit-frame-pointer
tsan: LDFLAGS += -fsanitize=thread
tsan: $(TARGET)
