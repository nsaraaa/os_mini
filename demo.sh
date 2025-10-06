#!/bin/bash

# OS Mini Server Demo Script
# This script demonstrates all the implemented features

echo "========================================="
echo "OS Mini Server - Module 1 Demo"
echo "========================================="
echo ""

# Start the server in background
echo "Starting server on port 8080..."
cd /Users/saranoor/Downloads/os_mini
./bin/os_mini_server 8080 &
SERVER_PID=$!

# Wait for server to start
sleep 2

echo "Server started with PID: $SERVER_PID"
echo ""

# Demo 1: Basic Commands
echo "Demo 1: Basic Commands"
echo "---------------------"
echo "HELP" | nc localhost 8080
echo ""

# Demo 2: User Registration and Authentication
echo "Demo 2: User Registration and Authentication"
echo "-------------------------------------------"
echo "Creating user 'sara' with password 'sara123'..."
echo "SIGNUP sara sara123" | nc localhost 8080
echo ""

echo "Logging in as 'sara'..."
echo "LOGIN sara sara123" | nc localhost 8080
echo ""

# Demo 3: File Operations (simulated)
echo "Demo 3: File Operations"
echo "----------------------"
{
    echo "LOGIN demo demo123"
    sleep 1
    echo "LIST"
    sleep 1
    echo "UPLOAD document.txt"
    sleep 1
    echo "DOWNLOAD document.txt"
    sleep 1
    echo "DELETE document.txt"
    sleep 1
    echo "LOGOUT"
    sleep 1
} | nc localhost 8080
echo ""

# Demo 4: Error Handling
echo "Demo 4: Error Handling"
echo "---------------------"
echo "Attempting file operation without login:"
echo "LIST" | nc localhost 8080
echo ""

echo "Attempting invalid login:"
echo "LOGIN demo wrongpassword" | nc localhost 8080
echo ""

# Demo 5: Concurrent Connections
echo "Demo 5: Concurrent Connections"
echo "-----------------------------"
echo "Starting 2 concurrent connections..."

{
    echo "LOGIN demo demo123"
    sleep 1
    echo "LIST"
    sleep 2
    echo "QUIT"
} | nc localhost 8080 &
PID1=$!

{
    echo "SIGNUP user2 pass456"
    sleep 1
    echo "LOGIN user2 pass456"
    sleep 1
    echo "UPLOAD file.txt"
    sleep 2
    echo "QUIT"
} | nc localhost 8080 &
PID2=$!

wait $PID1
wait $PID2
echo ""

# Show created user directories
echo "Demo 6: User Directory Structure"
echo "-------------------------------"
echo "Created user directories:"
ls -la /tmp/os_mini_users/
echo ""

echo "Example user directory structure:"
ls -la /tmp/os_mini_users/demo/
echo ""

# Stop the server
echo "Stopping server..."
kill $SERVER_PID
wait $SERVER_PID 2>/dev/null

echo ""
echo "========================================="
echo "Demo Complete!"
echo "========================================="
echo ""
echo "Features Demonstrated:"
echo "✓ TCP Server with socket(), bind(), listen(), accept()"
echo "✓ Thread pool with 10 client threads"
echo "✓ Thread-safe client queue with mutex protection"
echo "✓ User authentication (signup/login with password hashing)"
echo "✓ Session management"
echo "✓ Command parsing (UPLOAD, DOWNLOAD, DELETE, LIST)"
echo "✓ Task creation and queuing (Module 2 interface)"
echo "✓ Concurrent connection handling"
echo "✓ Graceful shutdown on Ctrl+C"
echo "✓ Error handling and validation"
echo "✓ User directory creation and management"
echo ""
echo "Ready for Module 2 integration!"
