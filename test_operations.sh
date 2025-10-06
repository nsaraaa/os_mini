#!/bin/bash

# Test script for file operations
SERVER="localhost"
PORT="8080"

echo "Testing OS Mini Server File Operations..."
echo "========================================"

# Test file operations after login
echo "Test: File operations after login"
{
    echo "LOGIN testuser password123"
    sleep 1
    echo "LIST"
    sleep 1
    echo "UPLOAD test.txt"
    sleep 1
    echo "DOWNLOAD test.txt"
    sleep 1
    echo "DELETE test.txt"
    sleep 1
    echo "LOGOUT"
    sleep 1
    echo "QUIT"
} | nc $SERVER $PORT

echo ""
echo "Testing complete!"
