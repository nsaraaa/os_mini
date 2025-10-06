#ifndef AUTH_H
#define AUTH_H

#include <pthread.h>

// Authentication status
typedef enum {
    AUTH_UNAUTHENTICATED,
    AUTH_AUTHENTICATED,
    AUTH_ERROR
} auth_status_t;

// User session structure
typedef struct {
    char username[64];
    char user_dir[256];
    int authenticated;
    time_t last_activity;
} user_session_t;

// Function prototypes
int auth_init(void);
void auth_cleanup(void);
int auth_signup(const char* username, const char* password);
int auth_login(const char* username, const char* password, user_session_t* session);
int auth_logout(user_session_t* session);
int auth_validate_session(user_session_t* session);
int auth_create_user_directory(const char* username);

#endif // AUTH_H
