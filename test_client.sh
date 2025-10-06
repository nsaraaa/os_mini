#!/bin/bash

# Simple test client script for OS Mini Server
# Usage: ./test_client.sh [port]

PORT=${1:-8080}
SERVER="localhost"

echo "Connecting to $SERVER:$PORT"
echo "Type commands or 'quit' to exit"
echo "Available commands:"
echo "  SIGNUP username password"
echo "  LOGIN username password" 
echo "  LOGOUT"
echo "  UPLOAD filename"
echo "  DOWNLOAD filename"
echo "  DELETE filename"
echo "  LIST"
echo "  HELP"
echo "  QUIT"
echo "----------------------------------------"

# Use netcat to connect to the server
nc $SERVER $PORT
