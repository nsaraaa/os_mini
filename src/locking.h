#ifndef LOCKING_H
#define LOCKING_H

#include <pthread.h>
#include <stdbool.h>

// Lock manager structure
typedef struct lock_manager {
    pthread_mutex_t global_mutex;
    // We'll use a simple array approach for now (can be enhanced to hash map)
    struct user_lock {
        char username[64];
        pthread_mutex_t mutex;
        int lock_count;
    } *user_locks;
    int capacity;
    int count;
} lock_manager_t;

// Function prototypes
int lock_manager_init(void);
void lock_manager_cleanup(void);
void lock_user(const char* username);
void unlock_user(const char* username);
bool try_lock_user(const char* username);

#endif