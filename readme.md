# OS Mini Server - Module 1: Network & Authentication Layer

A multi-threaded file server implementation with authentication and command processing capabilities.

## Features Implemented
### Phase 1
- Basic server socket structure with TCP setup
- Client queue data structure with mutex protection
- Main accept loop for incoming connections
- Thread pool for handling multiple clients
- Authentication protocol (login/signup)
- Command parser for file operations

### Authentication System
- User signup with password hashing
- User login with credential validation
- Session management
- User directory creation
- Thread-safe authentication

### Supported Commands
- `SIGNUP username password` - Create new account
- `LOGIN username password` - Login to account
- `LOGOUT` - Logout from account
- `UPLOAD filename` - Upload file (requires login)
- `DOWNLOAD filename` - Download file (requires login)
- `DELETE filename` - Delete file (requires login)
- `LIST` - List files (requires login)
- `QUIT` - Disconnect
- `HELP` - Show available commands

## Building and Running

### Prerequisites
- GCC compiler
- POSIX-compliant system (Linux, macOS)
- Make utility

### Build
```bash
make
```

### Run Server
```bash
# Default port 8080
make run

# Custom port
./bin/os_mini_server 9090
```

### Debug Build
```bash
make debug
```

### Clean Build Artifacts
```bash
make clean
```

## Testing

### Manual Testing with Test Client
```bash
./test_client.sh [port]
```

### Manual Testing with Netcat
```bash
nc localhost 8080
```

### Test Session Example
```
Welcome to OS Mini Server! Type HELP for commands.
SIGNUP testuser password123
SIGNUP_SUCCESS: Account created successfully
LOGIN testuser password123
LOGIN_SUCCESS: Authentication successful
LIST
LIST_SUCCESS: No files found
UPLOAD test.txt
UPLOAD_QUEUED: File upload queued for processing
UPLOAD_SUCCESS: File uploaded successfully
LOGOUT
LOGOUT_SUCCESS: Logged out successfully
QUIT
GOODBYE: Connection closed
```

## Architecture

### Thread Model
- **Main Thread**: Accepts incoming connections and adds them to client queue
- **Client Thread Pool**: Fixed number of threads (default: 10) that process client requests
- **Thread-Safe Queue**: Clients are queued and distributed to available threads

### Authentication Flow
1. Client connects and receives welcome message
2. Client can SIGNUP (creates user directory and password file)
3. Client can LOGIN (validates credentials against stored hash)
4. Authenticated clients can access file operations
5. Session state maintained per connection

### Task Processing
1. Authenticated commands are parsed and validated
2. Task structures are created with user context
3. Tasks are submitted to Module 2 interface (placeholder implementation)
4. Results are sent back to client
<!-- 
## Integration Points

### With Module 2 (Task Processing)
- **Task Structure**: Defined in `command_parser.h`
- **Queue Interface**: `push_task_to_queue()` function
- **Communication**: Ready for worker-client result delivery

### With Module 3 (File System)
- **User Directories**: Created in `/tmp/os_mini_users/`
- **Directory Structure**: `username/files/` for user files
- **Authentication Data**: `.password` and `.info` files -->

## Configuration

### Server Settings
- **Default Port**: 8080
- **Max Clients**: 100
- **Thread Pool Size**: 10
- **Buffer Size**: 1024 bytes

### User Data Location
- **Base Directory**: `/tmp/os_mini_users/`
- **Per User**: `username/files/` for files, `.password` for auth
<!-- 
## Error Handling

- Graceful shutdown on SIGINT/SIGTERM
- Socket error detection and recovery
- Authentication failure handling
- Malformed command responses
- Connection cleanup on client exit

## Security Considerations

- Password hashing using crypt()
- User directory isolation
- Input validation and sanitization
- Thread-safe authentication state

## Future Enhancements (Phase 2)

- Multi-client session management
- Worker-client communication mechanism
- Connection limiting per user
- Enhanced error responses
- Comprehensive logging
- Configuration file support -->
<!-- 
## Troubleshooting

### Common Issues
1. **Port already in use**: Try a different port
2. **Permission denied**: Check if port requires root privileges
3. **Compilation errors**: Ensure POSIX compatibility and pthread support -->

### Debug Mode
```bash
make debug
gdb ./bin/os_mini_server
```
<!-- 
## License

This project is part of an OS Mini implementation for educational purposes. -->
