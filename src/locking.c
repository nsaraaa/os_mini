#include "locking.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#define INITIAL_CAPACITY 50

static lock_manager_t lock_manager;

int lock_manager_init(void) {
    lock_manager.user_locks = malloc(sizeof(struct user_lock) * INITIAL_CAPACITY);
    if (!lock_manager.user_locks) {
        return -1;
    }
    
    lock_manager.capacity = INITIAL_CAPACITY;
    lock_manager.count = 0;
    
    if (pthread_mutex_init(&lock_manager.global_mutex, NULL) != 0) {
        free(lock_manager.user_locks);
        return -1;
    }
    
    printf("Lock manager initialized with capacity %d\n", INITIAL_CAPACITY);
    return 0;
}

void lock_manager_cleanup(void) {
    pthread_mutex_lock(&lock_manager.global_mutex);
    
    for (int i = 0; i < lock_manager.count; i++) {
        pthread_mutex_destroy(&lock_manager.user_locks[i].mutex);
    }
    
    free(lock_manager.user_locks);
    pthread_mutex_unlock(&lock_manager.global_mutex);
    pthread_mutex_destroy(&lock_manager.global_mutex);
    
    printf("Lock manager cleaned up\n");
}

// Find or create a user lock
static struct user_lock* get_user_lock(const char* username) {
    // First, try to find existing lock
    for (int i = 0; i < lock_manager.count; i++) {
        if (strcmp(lock_manager.user_locks[i].username, username) == 0) {
            return &lock_manager.user_locks[i];
        }
    }
    
    // Not found, create new one
    if (lock_manager.count >= lock_manager.capacity) {
        // Resize array (simplified - in production you'd want better growth strategy)
        int new_capacity = lock_manager.capacity * 2;
        struct user_lock* new_locks = realloc(lock_manager.user_locks, 
                                            sizeof(struct user_lock) * new_capacity);
        if (!new_locks) {
            return NULL;
        }
        lock_manager.user_locks = new_locks;
        lock_manager.capacity = new_capacity;
        printf("Lock manager resized to capacity %d\n", new_capacity);
    }
    
    // Initialize new user lock
    struct user_lock* new_lock = &lock_manager.user_locks[lock_manager.count];
    strncpy(new_lock->username, username, sizeof(new_lock->username) - 1);
    new_lock->lock_count = 0;
    
    if (pthread_mutex_init(&new_lock->mutex, NULL) != 0) {
        return NULL;
    }
    
    lock_manager.count++;
    printf("Created lock for user '%s'\n", username);
    return new_lock;
}

void lock_user(const char* username) {
    if (!username) return;
    
    pthread_mutex_lock(&lock_manager.global_mutex);
    
    struct user_lock* user_lock = get_user_lock(username);
    if (!user_lock) {
        pthread_mutex_unlock(&lock_manager.global_mutex);
        return;
    }
    
    pthread_mutex_unlock(&lock_manager.global_mutex);
    
    // Acquire the user-specific lock
    pthread_mutex_lock(&user_lock->mutex);
    user_lock->lock_count++;
    
    printf("Lock acquired for user '%s' (lock count: %d)\n", username, user_lock->lock_count);
}

void unlock_user(const char* username) {
    if (!username) return;
    
    pthread_mutex_lock(&lock_manager.global_mutex);
    
    // Find the user lock
    struct user_lock* user_lock = NULL;
    for (int i = 0; i < lock_manager.count; i++) {
        if (strcmp(lock_manager.user_locks[i].username, username) == 0) {
            user_lock = &lock_manager.user_locks[i];
            break;
        }
    }
    
    if (!user_lock) {
        pthread_mutex_unlock(&lock_manager.global_mutex);
        return;
    }
    
    pthread_mutex_unlock(&lock_manager.global_mutex);
    
    // Release the user-specific lock
    user_lock->lock_count--;
    pthread_mutex_unlock(&user_lock->mutex);
    
    printf("Lock released for user '%s' (lock count: %d)\n", username, user_lock->lock_count);
}

bool try_lock_user(const char* username) {
    if (!username) return false;
    
    pthread_mutex_lock(&lock_manager.global_mutex);
    
    struct user_lock* user_lock = get_user_lock(username);
    if (!user_lock) {
        pthread_mutex_unlock(&lock_manager.global_mutex);
        return false;
    }
    
    pthread_mutex_unlock(&lock_manager.global_mutex);
    
    // Try to acquire the user-specific lock (non-blocking)
    int result = pthread_mutex_trylock(&user_lock->mutex);
    if (result == 0) {
        user_lock->lock_count++;
        printf("Try-lock succeeded for user '%s'\n", username);
        return true;
    }
    
    printf("Try-lock failed for user '%s' (already locked)\n", username);
    return false;
}