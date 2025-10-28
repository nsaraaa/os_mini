#!/bin/bash

# test auth flow
SERVER="localhost"
PORT="8080"

echo "Testing OS Mini Server Authentication..."
echo "========================================"

# Test 1: Help command
echo "Test 1: Help command"
echo "HELP" | nc $SERVER $PORT
echo ""

# Test 2: Signup
echo "Test 2: User signup"
echo "SIGNUP testuser password123" | nc $SERVER $PORT
echo ""

# Test 3: Login
echo "Test 3: User login"
echo "LOGIN testuser password123" | nc $SERVER $PORT
echo ""

# Test 4: Invalid login
echo "Test 4: Invalid login"
echo "LOGIN testuser wrongpass" | nc $SERVER $PORT
echo ""

# Test 5: File operations without login
echo "Test 5: File operations without login"
echo "LIST" | nc $SERVER $PORT
echo ""

echo "Testing complete!"
