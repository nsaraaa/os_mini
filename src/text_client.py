#!/usr/bin/env python3
"""
OS Mini Server - Python Test Client
Provides an interactive command-line interface to test the server
"""

import socket
import sys
import time

class OSMiniClient:
    def __init__(self, host='localhost', port=8080):
        self.host = host
        self.port = port
        self.socket = None
        self.connected = False
        
    def connect(self):
        """Connect to the server"""
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.connect((self.host, self.port))
            self.connected = True
            print(f"✓ Connected to {self.host}:{self.port}")
            
            # Receive welcome message
            response = self.receive()
            print(f"Server: {response}")
            return True
        except Exception as e:
            print(f"✗ Connection failed: {e}")
            return False
    
    def disconnect(self):
        """Disconnect from the server"""
        if self.socket:
            self.socket.close()
            self.connected = False
            print("✓ Disconnected from server")
    
    def send_command(self, command):
        """Send a command to the server"""
        if not self.connected:
            print("✗ Not connected to server")
            return None
        
        try:
            # Send command
            self.socket.sendall((command + '\n').encode())
            
            # Receive response
            response = self.receive()
            return response
        except Exception as e:
            print(f"✗ Error sending command: {e}")
            self.connected = False
            return None
    
    def receive(self):
        """Receive response from server"""
        try:
            data = self.socket.recv(4096).decode()
            return data.strip()
        except Exception as e:
            print(f"✗ Error receiving response: {e}")
            return None
    
    def signup(self, username, password):
        """Sign up a new user"""
        print(f"\n→ SIGNUP {username}")
        response = self.send_command(f"SIGNUP {username} {password}")
        print(f"← {response}")
        return response
    
    def login(self, username, password):
        """Login as a user"""
        print(f"\n→ LOGIN {username}")
        response = self.send_command(f"LOGIN {username} {password}")
        print(f"← {response}")
        return response
    
    def logout(self):
        """Logout current user"""
        print(f"\n→ LOGOUT")
        response = self.send_command("LOGOUT")
        print(f"← {response}")
        return response
    
    def upload(self, filename):
        """Upload a file"""
        print(f"\n→ UPLOAD {filename}")
        response = self.send_command(f"UPLOAD {filename}")
        print(f"← {response}")
        return response
    
    def download(self, filename):
        """Download a file"""
        print(f"\n→ DOWNLOAD {filename}")
        response = self.send_command(f"DOWNLOAD {filename}")
        print(f"← {response}")
        return response
    
    def delete(self, filename):
        """Delete a file"""
        print(f"\n→ DELETE {filename}")
        response = self.send_command(f"DELETE {filename}")
        print(f"← {response}")
        return response
    
    def list_files(self):
        """List all files"""
        print(f"\n→ LIST")
        response = self.send_command("LIST")
        print(f"← {response}")
        return response
    
    def help(self):
        """Get help"""
        print(f"\n→ HELP")
        response = self.send_command("HELP")
        print(f"← {response}")
        return response
    
    def quit(self):
        """Quit and disconnect"""
        print(f"\n→ QUIT")
        response = self.send_command("QUIT")
        print(f"← {response}")
        self.disconnect()
        return response


def run_automated_test(client):
    """Run automated test sequence"""
    print("\n" + "="*50)
    print("RUNNING AUTOMATED TEST SEQUENCE")
    print("="*50)
    
    # Test 1: Signup
    print("\n[TEST 1] Signup new user")
    client.signup("alice", "password123")
    time.sleep(0.5)
    
    # Test 2: Login
    print("\n[TEST 2] Login")
    client.login("alice", "password123")
    time.sleep(0.5)
    
    # Test 3: Upload files
    print("\n[TEST 3] Upload files")
    client.upload("document1.txt")
    time.sleep(0.5)
    client.upload("document2.txt")
    time.sleep(0.5)
    client.upload("photo.jpg")
    time.sleep(0.5)
    
    # Test 4: List files
    print("\n[TEST 4] List files")
    client.list_files()
    time.sleep(0.5)
    
    # Test 5: Download file
    print("\n[TEST 5] Download file")
    client.download("document1.txt")
    time.sleep(0.5)
    
    # Test 6: Delete file
    print("\n[TEST 6] Delete file")
    client.delete("document2.txt")
    time.sleep(0.5)
    
    # Test 7: List files again
    print("\n[TEST 7] List files after deletion")
    client.list_files()
    time.sleep(0.5)
    
    # Test 8: Logout
    print("\n[TEST 8] Logout")
    client.logout()
    time.sleep(0.5)
    
    # Test 9: Try operation without login (should fail)
    print("\n[TEST 9] Try to list files without login (should fail)")
    client.list_files()
    time.sleep(0.5)
    
    print("\n" + "="*50)
    print("AUTOMATED TEST SEQUENCE COMPLETE")
    print("="*50)


def interactive_mode(client):
    """Run in interactive mode"""
    print("\n" + "="*50)
    print("INTERACTIVE MODE")
    print("="*50)
    print("\nCommands:")
    print("  signup <username> <password>")
    print("  login <username> <password>")
    print("  logout")
    print("  upload <filename>")
    print("  download <filename>")
    print("  delete <filename>")
    print("  list")
    print("  help")
    print("  quit")
    print("="*50 + "\n")
    
    while True:
        try:
            command = input("client> ").strip()
            
            if not command:
                continue
            
            parts = command.split()
            cmd = parts[0].lower()
            
            if cmd == "quit":
                client.quit()
                break
            elif cmd == "signup" and len(parts) == 3:
                client.signup(parts[1], parts[2])
            elif cmd == "login" and len(parts) == 3:
                client.login(parts[1], parts[2])
            elif cmd == "logout":
                client.logout()
            elif cmd == "upload" and len(parts) == 2:
                client.upload(parts[1])
            elif cmd == "download" and len(parts) == 2:
                client.download(parts[1])
            elif cmd == "delete" and len(parts) == 2:
                client.delete(parts[1])
            elif cmd == "list":
                client.list_files()
            elif cmd == "help":
                client.help()
            else:
                print("✗ Invalid command. Type 'help' for available commands.")
                
        except KeyboardInterrupt:
            print("\n\n✓ Interrupted by user")
            client.quit()
            break
        except Exception as e:
            print(f"✗ Error: {e}")


def main():
    """Main function"""
    print("╔═══════════════════════════════════════════╗")
    print("║   OS Mini Server - Python Test Client    ║")
    print("╚═══════════════════════════════════════════╝")
    
    # Parse arguments
    host = 'localhost'
    port = 8080
    mode = 'interactive'
    
    if len(sys.argv) > 1:
        if sys.argv[1] == 'test':
            mode = 'automated'
        elif sys.argv[1] == 'interactive':
            mode = 'interactive'
        else:
            try:
                port = int(sys.argv[1])
            except:
                print(f"Usage: {sys.argv[0]} [port|test|interactive]")
                sys.exit(1)
    
    if len(sys.argv) > 2:
        try:
            port = int(sys.argv[2])
        except:
            pass
    
    # Create client and connect
    client = OSMiniClient(host, port)
    
    if not client.connect():
        sys.exit(1)
    
    try:
        if mode == 'automated':
            run_automated_test(client)
            client.quit()
        else:
            interactive_mode(client)
    except Exception as e:
        print(f"\n✗ Fatal error: {e}")
    finally:
        if client.connected:
            client.disconnect()


if __name__ == "__main__":
    main()
