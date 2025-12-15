import socket
import threading
import sys
import datetime

LISTEN_HOST = '0.0.0.0' # Listen on all available interfaces
LISTEN_PORT = 6000     # The port your Atari should connect to

REMOTE_HOST = 'freechess.org'
REMOTE_PORT = 5000      # The actual freechess.org port

def hexdump(data, direction):
    """Generates a hex dump with ASCII interpretation and highlights 0xff."""
    output = f"[{direction} {len(data)} bytes @ {datetime.datetime.now().time().isoformat()}]"
    output += "\nRaw Hex: "
    hex_bytes = []
    has_ff = False
    for b in data:
        hex_str = f"{b:02x}"
        if b == 0xff:
            # Highlight IAC byte for easy spotting
            hex_bytes.append(f"\033[91m{hex_str}\033[0m") # ANSI red color
            has_ff = True
        else:
            hex_bytes.append(hex_str)
    output += " ".join(hex_bytes)
    
    # Simple ASCII decode for data visualization
    output += f"\nDecoded (ignore errors): {data.decode('ascii', errors='ignore').replace('\\n', ' ').replace('\\r', '')}\n"

    print(output)
    if has_ff:
        print("\033[91m!!! DETECTED 0xFF (Telnet IAC) BYTE IN STREAM !!!\033[0m\n")


def forward_data(source_socket, destination_socket, direction):
    """Forwards data between two sockets and logs it."""
    try:
        while True:
            data = source_socket.recv(4096)
            if not data:
                break
            
            hexdump(data, direction)
            destination_socket.sendall(data)
            
    except ConnectionResetError:
        print(f"Connection reset during {direction} forwarding.")
    except Exception as e:
        print(f"Error during {direction} forwarding: {e}")
    finally:
        source_socket.close()


def client_handler(client_socket, remote_host, remote_port):
    """Handles a single client connection, establishing connection to remote server."""
    remote_socket = None
    try:
        remote_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        remote_socket.connect((remote_host, remote_port))
        print(f"[+] Successfully connected to remote host {remote_host}:{remote_port}")

        # Start two threads to manage bi-directional data flow
        # C2S = Client to Server, S2C = Server to Client
        client_to_server_thread = threading.Thread(target=forward_data, 
                                                  args=(client_socket, remote_socket, "C2S ->"))
        server_to_client_thread = threading.Thread(target=forward_data, 
                                                  args=(remote_socket, client_socket, "<- S2C"))

        client_to_server_thread.start()
        server_to_client_thread.start()

        # Join the threads to keep the main handler alive until connections close
        client_to_server_thread.join()
        server_to_client_thread.join()

    except Exception as e:
        print(f"[-] Connection to remote host failed: {e}")
        if client_socket:
            client_socket.close()
    finally:
        if remote_socket:
            remote_socket.close()
        print(f"[*] Client session closed.")


def main():
    print(f"[*] Starting TCP proxy on {LISTEN_HOST}:{LISTEN_PORT}")
    print(f"[*] Forwarding to {REMOTE_HOST}:{REMOTE_PORT}")
    
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind((LISTEN_HOST, LISTEN_PORT))
    server.listen(5) # Max backlog of 5 connections

    while True:
        client_socket, addr = server.accept()
        print(f"[*] Accepted connection from {addr[0]}:{addr[1]}")
        
        # Start a new thread to handle the client connection
        client_handler_thread = threading.Thread(target=client_handler, 
                                                 args=(client_socket, REMOTE_HOST, REMOTE_PORT))
        client_handler_thread.start()

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n[*] Proxy shutting down.")
        sys.exit(0)


