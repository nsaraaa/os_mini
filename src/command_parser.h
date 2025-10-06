#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

#include "auth.h"

// Command types
typedef enum {
    CMD_UNKNOWN,
    CMD_LOGIN,
    CMD_SIGNUP,
    CMD_UPLOAD,
    CMD_DOWNLOAD,
    CMD_DELETE,
    CMD_LIST,
    CMD_LOGOUT,
    CMD_QUIT
} command_type_t;

// Task structure for Module 2 interface
typedef struct {
    command_type_t type;
    char filename[256];
    char username[64];
    char user_dir[256];
    int client_socket;
    int authenticated;
} task_t;

// Function prototypes
command_type_t parse_command(const char* input);
task_t* create_task(command_type_t type, const char* filename, user_session_t* session, int client_socket);
int send_response(int client_socket, const char* response);
int handle_authenticated_command(const char* command, user_session_t* session, int client_socket);
int push_task_to_queue(task_t* task);  // Interface with Module 2

#endif // COMMAND_PARSER_H
