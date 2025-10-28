#ifndef MODULE3_INTEGRATION_H
#define MODULE3_INTEGRATION_H

#include "command_parser.h"

// Initialize Module 3 systems
int module3_init(void);
void module3_cleanup(void);

// Handle user signup from Module 1
int module3_handle_signup(const char* username, const char* password_hash);

// Process file operations from Module 2
int module3_process_upload(task_t* task);
int module3_process_download(task_t* task);
int module3_process_delete(task_t* task);
int module3_process_list(task_t* task);

#endif // MODULE3_INTEGRATION_H