#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include "metadata.h"

// Function prototypes
int save_user_metadata(user_metadata_t* user);
user_metadata_t* load_user_metadata(const char* username);
int save_all_metadata(void);
int load_all_metadata(void);
void get_metadata_path(const char* username, char* path, size_t path_size);

#endif // PERSISTENCE_H