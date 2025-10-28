#!/bin/bash

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}=== OS Mini Server Automated Test Suite ===${NC}"
echo

# Configuration
SERVER_PORT=9090
SERVER_BIN="./bin/os_mini_server"
TEST_DIR="test_temp"
SERVER_PID=""

# Test counters
TESTS_PASSED=0
TESTS_FAILED=0

# Cleanup function
cleanup() {
    echo -e "\n${YELLOW}Cleaning up...${NC}"
    if [ ! -z "$SERVER_PID" ]; then
        kill $SERVER_PID 2>/dev/null
        sleep 1
        kill -9 $SERVER_PID 2>/dev/null
    fi
    rm -rf "$TEST_DIR"
    rm -f test_*.txt
    make clean > /dev/null 2>&1
    echo -e "${GREEN}Cleanup complete${NC}"
}

# Error handler
trap cleanup EXIT INT TERM

# Function to run a test
run_test() {
    local test_name="$1"
    local command="$2"
    local expected="$3"
    
    echo -n "Testing $test_name... "
    
    if eval "$command" 2>/dev/null | grep -q "$expected"; then
        echo -e "${GREEN}PASS${NC}"
        ((TESTS_PASSED++))
        return 0
    else
        echo -e "${RED}FAIL${NC}"
        ((TESTS_FAILED++))
        return 1
    fi
}

# Function to send commands to server and check response
send_command() {
    local commands="$1"
    local expected="$2"
    local test_name="$3"
    
    echo -n "Testing $test_name... "
    
    local response
    response=$(echo -e "$commands" | nc localhost $SERVER_PORT 2>/dev/null)
    
    if echo "$response" | grep -q "$expected"; then
        echo -e "${GREEN}PASS${NC}"
        ((TESTS_PASSED++))
    else
        echo -e "${RED}FAIL${NC}"
        echo "Expected: $expected"
        echo "Got: $response"
        ((TESTS_FAILED++))
    fi
}

# Build the server
echo -e "${YELLOW}Building server...${NC}"
if ! make > /dev/null 2>&1; then
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi
echo -e "${GREEN}Build successful${NC}"

# Create test directory
mkdir -p "$TEST_DIR"

# Start the server
echo -e "${YELLOW}Starting server on port $SERVER_PORT...${NC}"
$SERVER_BIN $SERVER_PORT > "$TEST_DIR/server.log" 2>&1 &
SERVER_PID=$!
sleep 3 # Wait for server to start

# Check if server is running
if ! ps -p $SERVER_PID > /dev/null; then
    echo -e "${RED}Server failed to start!${NC}"
    cat "$TEST_DIR/server.log"
    exit 1
fi
echo -e "${GREEN}Server started (PID: $SERVER_PID)${NC}"

echo
echo -e "${YELLOW}=== Running Tests ===${NC}"
echo

# Test 1: Basic connectivity
send_command "QUIT" "GOODBYE" "Basic connectivity"

# Test 2: Help command
send_command "HELP" "Available commands" "Help command"

# Test 3: User signup
send_command "SIGNUP alice pass123" "SIGNUP_SUCCESS" "User signup"

# Test 4: User login
send_command "LOGIN alice pass123" "LOGIN_SUCCESS" "User login"

# Test 5: Upload file (with actual file data)
echo -n "Testing file upload... "
echo "This is test file content for upload." > "$TEST_DIR/upload_test.txt"
{
    echo "LOGIN alice pass123"
    sleep 0.1
    echo "UPLOAD test_upload.txt"
    sleep 0.1
    cat "$TEST_DIR/upload_test.txt"
    echo "END"
    sleep 1
} | nc localhost $SERVER_PORT > "$TEST_DIR/upload_response.txt" 2>/dev/null

if grep -q "UPLOAD_SUCCESS" "$TEST_DIR/upload_response.txt"; then
    echo -e "${GREEN}PASS${NC}"
    ((TESTS_PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    cat "$TEST_DIR/upload_response.txt"
    ((TESTS_FAILED++))
fi

# Test 6: List files
send_command "LOGIN alice pass123\nLIST" "test_upload.txt" "List files"

# Test 7: Download file
echo -n "Testing file download... "
{
    echo "LOGIN alice pass123"
    sleep 0.1
    echo "DOWNLOAD test_upload.txt"
    sleep 1
} | nc localhost $SERVER_PORT > "$TEST_DIR/download_response.txt" 2>/dev/null

if grep -q "DOWNLOAD_SUCCESS" "$TEST_DIR/download_response.txt"; then
    echo -e "${GREEN}PASS${NC}"
    ((TESTS_PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    cat "$TEST_DIR/download_response.txt"
    ((TESTS_FAILED++))
fi

# Test 8: Delete file
send_command "LOGIN alice pass123\nDELETE test_upload.txt" "DELETE_SUCCESS" "Delete file"

# Test 9: Verify file deleted
send_command "LOGIN alice pass123\nLIST" "No files found" "Verify file deletion"

# Test 10: Logout
send_command "LOGIN alice pass123\nLOGOUT" "LOGOUT_SUCCESS" "User logout"

# Test 11: Multiple users
send_command "SIGNUP bob pass456" "SIGNUP_SUCCESS" "Multiple user signup"
send_command "LOGIN bob pass456" "LOGIN_SUCCESS" "Multiple user login"

# Test 12: Concurrent access test
echo -n "Testing basic concurrency... "
{
    echo "LOGIN alice pass123"
    sleep 0.1
    echo "LIST"
    sleep 0.1
} | nc localhost $SERVER_PORT > "$TEST_DIR/concurrent1.txt" 2>&1 &

{
    echo "LOGIN bob pass456" 
    sleep 0.1
    echo "LIST"
    sleep 0.1
} | nc localhost $SERVER_PORT > "$TEST_DIR/concurrent2.txt" 2>&1 &

wait

if grep -q "LIST_SUCCESS" "$TEST_DIR/concurrent1.txt" && grep -q "LIST_SUCCESS" "$TEST_DIR/concurrent2.txt"; then
    echo -e "${GREEN}PASS${NC}"
    ((TESTS_PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    ((TESTS_FAILED++))
fi

# Test 13: Error handling - invalid login
send_command "LOGIN nonexistent wrongpass" "LOGIN_ERROR" "Invalid login handling"

# Test 14: Error handling - unauthorized commands
send_command "UPLOAD test.txt" "AUTH_ERROR" "Unauthorized command handling"

# Test 15: Quit command
send_command "QUIT" "GOODBYE" "Quit command"

echo
echo -e "${YELLOW}=== Test Summary ===${NC}"
echo -e "Total Tests: $((TESTS_PASSED + TESTS_FAILED))"
echo -e "${GREEN}Passed: $TESTS_PASSED${NC}"
echo -e "${RED}Failed: $TESTS_FAILED${NC}"

# Check server log for errors
echo
echo -e "${YELLOW}=== Server Log Analysis ===${NC}"
if grep -i "error\|fail\|warning" "$TEST_DIR/server.log"; then
    echo -e "${YELLOW}Warnings or errors found in server log${NC}"
else
    echo -e "${GREEN}No errors in server log${NC}"
fi

# Final cleanup
cleanup

# Exit with appropriate code
if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "\n${GREEN}🎉 All tests passed!${NC}"
    exit 0
else
    echo -e "\n${RED}❌ Some tests failed!${NC}"
    exit 1
fi