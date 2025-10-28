

#ifndef METADATA_H
#define METADATA_H

#include <time.h>
#include <stddef.h>
#include <sys/types.h>

#define MAX_USERNAME 64
#define MAX_FILENAME 256
#define MAX_FILES_PER_USER 1000
#define DEFAULT_QUOTA_MB 100
#define DEFAULT_QUOTA_BYTES (DEFAULT_QUOTA_MB * 1024 * 1024)

// File metadata structure
typedef struct file_metadata {
    char filename[MAX_FILENAME];
    size_t size;
    time_t uploaded_at;
    struct file_metadata *next;
} file_metadata_t;

// User metadata structure
typedef struct user_metadata {
    char username[MAX_USERNAME];
    char password_hash[128];
    size_t storage_used;
    size_t storage_quota;
    int file_count;
    file_metadata_t *files;
    time_t created_at;
    time_t last_login;
    struct user_metadata *next;
} user_metadata_t;

// Metadata manager structure
typedef struct {
    user_metadata_t *users;
    int user_count;
} metadata_manager_t;

// Function prototypes
int init_metadata(void);
void cleanup_metadata(void);

user_metadata_t* create_user(const char* username, const char* password_hash);
user_metadata_t* get_user(const char* username);
int delete_user(const char* username);

int add_file_to_user(user_metadata_t* user, const char* filename, size_t size);
int remove_file_from_user(user_metadata_t* user, const char* filename);
file_metadata_t* get_file_metadata(user_metadata_t* user, const char* filename);
file_metadata_t* get_file_list(user_metadata_t* user);

int update_user_storage(user_metadata_t* user, ssize_t delta);
size_t get_user_storage_used(user_metadata_t* user);
size_t get_user_storage_quota(user_metadata_t* user);

#endif // METADATA_H
