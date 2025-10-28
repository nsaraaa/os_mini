#!/bin/bash

echo "=== Quick Server Test ==="

# Build
make clean
make

# Start server
./bin/os_mini_server 9090 &
SERVER_PID=$!
sleep 2

# Basic test
echo "Testing basic functionality..."
{
    echo "SIGNUP quicktest pass123"
    sleep 0.5
    echo "LOGIN quicktest pass123" 
    sleep 0.5
    echo "UPLOAD quick.txt"
    sleep 0.5
    echo "Quick test file content"
    echo "END"
    sleep 1
    echo "LIST"
    sleep 0.5
    echo "QUIT"
} | nc localhost 9090

# Kill server
kill $SERVER_PID
echo "=== Test Complete ==="