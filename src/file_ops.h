#ifndef FILE_OPS_H
#define FILE_OPS_H

#include "metadata.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdbool.h>

// File operation result codes
typedef enum {
    FILE_OP_SUCCESS = 0,
    FILE_OP_ERROR = -1,
    FILE_OP_NOT_FOUND = -2,
    FILE_OP_QUOTA_EXCEEDED = -3,
    FILE_OP_ALREADY_EXISTS = -4,
    FILE_OP_INVALID_PARAM = -5
} file_op_result_t;

#define STORAGE_BASE_DIR "server_storage"
#define USERS_DIR "server_storage/users"
#define METADATA_DIR "server_storage/metadata"

// Function prototypes

// Initialization
int init_file_storage(void);

// High-level file operations (these acquire locks internally)
int upload_file(const char* username, const char* filename, 
                const void* data, size_t data_size);
int download_file(const char* username, const char* filename, 
                  void** data, size_t* data_size);
int delete_file(const char* username, const char* filename);
int list_files(const char* username, char* buffer, size_t buffer_size);

// Low-level operations (caller must hold lock)
int write_file_to_disk(const char* username, const char* filename, 
                       const void* data, size_t size);
int read_file_from_disk(const char* username, const char* filename, 
                        void** data, size_t* size);

// Utility functions
bool file_exists(const char* username, const char* filename);
ssize_t get_file_size(const char* username, const char* filename);
int create_user_directory(const char* username);
void get_user_file_path(const char* username, const char* filename, 
                        char* path, size_t path_size);

#endif // FILE_OPS_H