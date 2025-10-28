#ifndef QUOTA_H
#define QUOTA_H

#include "metadata.h"
#include <stdbool.h>

// Quota status codes
typedef enum {
    QUOTA_OK = 0,
    QUOTA_EXCEEDED = -1,
    QUOTA_ERROR = -2
} quota_status_t;

// Function prototypes
bool check_quota(user_metadata_t* user, size_t file_size);
int update_quota_on_upload(user_metadata_t* user, size_t file_size);
int update_quota_on_delete(user_metadata_t* user, size_t file_size);
size_t get_remaining_quota(user_metadata_t* user);
double get_quota_usage_percent(user_metadata_t* user);

#endif // QUOTA_H