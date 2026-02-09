#!/usr/bin/env python3
"""
Minimal UDP receive test to diagnose broadcast response issue
"""
import socket
import sys

def main():
    # Create socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    # Get local IP
    temp_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        temp_sock.connect(('8.8.8.8', 80))
        local_ip = temp_sock.getsockname()[0]
    finally:
        temp_sock.close()

    # Bind to specific IP
    sock.bind((local_ip, 0))
    local_addr = sock.getsockname()
    print(f"Listening on {local_addr[0]}:{local_addr[1]}")
    print(f"Waiting for UDP packets... Press Ctrl+C to stop")

    sock.settimeout(None)  # Block indefinitely

    try:
        while True:
            data, addr = sock.recvfrom(1024)
            print(f"\n✓ Received {len(data)} bytes from {addr[0]}:{addr[1]}")
            print(f"Data: {data.hex()}")
    except KeyboardInterrupt:
        print("\nStopped")
    finally:
        sock.close()

if __name__ == "__main__":
    main()
