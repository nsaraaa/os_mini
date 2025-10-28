# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -pthread -g -O2
LDFLAGS = -pthread

# Detect OS for crypt library
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    LDFLAGS += -lcrypt
endif

# Target executable
TARGET = os_mini_server

# Source files
SRCS = main.c \
       server.c \
       auth.c \
       client_queue.c \
       command_parser.c \
       task_queue.c \
       worker_pool.c \
       locking.c \
       metadata.c \
       quota.c \
       file_ops.c \
       persistence.c \
       module3_integration.c

# Object files
OBJS = $(SRCS:.c=.o)

# Header files
HEADERS = server.h \
          auth.h \
          client_queue.h \
          command_parser.h \
          task_queue.h \
          worker_pool.h \
          locking.h \
          metadata.h \
          quota.h \
          file_ops.h \
          persistence.h \
          module3_integration.h

# Default target
all: $(TARGET)

# Link object files to create executable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Compile source files to object files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJS) $(TARGET)
	rm -rf server_storage
	@echo "Clean complete"

# Clean and rebuild
rebuild: clean all

# Run the server
run: $(TARGET)
	./$(TARGET)

# Run with custom port
run-port: $(TARGET)
	./$(TARGET) 8080

# Create necessary directories
setup:
	mkdir -p server_storage/users
	mkdir -p server_storage/metadata
	@echo "Directory structure created"

# Run with valgrind for memory leak detection
valgrind: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TARGET)

# Run with thread sanitizer (requires recompilation)
tsan:
	$(CC) $(CFLAGS) -fsanitize=thread $(SRCS) -o $(TARGET)_tsan $(LDFLAGS)
	./$(TARGET)_tsan

# Help target
help:
	@echo "Available targets:"
	@echo "  all      - Build the server (default)"
	@echo "  clean    - Remove build artifacts"
	@echo "  rebuild  - Clean and rebuild"
	@echo "  run      - Build and run the server"
	@echo "  setup    - Create necessary directories"
	@echo "  valgrind - Run with Valgrind memory checker"
	@echo "  tsan     - Run with Thread Sanitizer"
	@echo "  help     - Show this help message"

.PHONY: all clean rebuild run run-port setup valgrind tsan help
