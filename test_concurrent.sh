#!/bin/bash

# Test script for concurrent connections
SERVER="localhost"
PORT="8080"

echo "Testing OS Mini Server Concurrent Connections..."
echo "=============================================="

# Test multiple concurrent connections
echo "Starting 3 concurrent connections..."

# Connection 1
{
    echo "LOGIN testuser password123"
    sleep 1
    echo "LIST"
    sleep 2
    echo "QUIT"
} | nc $SERVER $PORT &
PID1=$!

# Connection 2
{
    echo "SIGNUP user2 pass456"
    sleep 1
    echo "LOGIN user2 pass456"
    sleep 1
    echo "UPLOAD file2.txt"
    sleep 2
    echo "QUIT"
} | nc $SERVER $PORT &
PID2=$!

# Connection 3
{
    echo "HELP"
    sleep 1
    echo "SIGNUP user3 pass789"
    sleep 1
    echo "LOGIN user3 pass789"
    sleep 2
    echo "QUIT"
} | nc $SERVER $PORT &
PID3=$!

# Wait for all connections to complete
wait $PID1
wait $PID2
wait $PID3

echo ""
echo "Concurrent testing complete!"
