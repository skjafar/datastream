#!/usr/bin/env python3
"""
Mock ESP32 Discovery Server
Mimics the ESP32 board's discovery response behavior for testing
"""

import socket
import struct
import sys

# Protocol constants (matching test_discovery.py)
DS_DISCOVERY_MAGIC = 0xDEADBEEF
DS_DISCOVERY_REQUEST = 0x01
DS_DISCOVERY_RESPONSE = 0x02

# Mock board configuration
BOARD_TYPE = 1  # CUSTOM_1
FIRMWARE_VERSION = 0x0100  # Version 1.0
BOARD_ID = 0x12345678
TCP_PORT = 2009
UDP_PORT = 2011
MAC_ADDRESS = bytes([0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF])
BOARD_NAME = "MockESP32-Test"

def create_discovery_response(client_ip: int) -> bytes:
    """Create a discovery response packet (44 bytes)"""

    # Pack the discovery response structure
    response = struct.pack('<I B B H I I H H 6s 2s 16s',
        DS_DISCOVERY_MAGIC,      # magic
        DS_DISCOVERY_RESPONSE,   # command
        BOARD_TYPE,              # board_type
        FIRMWARE_VERSION,        # firmware_version
        BOARD_ID,                # board_id (board_serial)
        client_ip,               # ip_address (use client IP for now)
        TCP_PORT,                # tcp_port
        UDP_PORT,                # udp_port
        MAC_ADDRESS,             # mac_address (6 bytes)
        b'\x00\x00',            # reserved (2 bytes)
        BOARD_NAME.encode('utf-8').ljust(16, b'\x00')  # board_name (16 bytes)
    )

    return response

def validate_discovery_request(data: bytes) -> bool:
    """Validate incoming discovery request"""
    if len(data) < 5:
        return False

    try:
        magic, command = struct.unpack('<I B', data[:5])
        return magic == DS_DISCOVERY_MAGIC and command == DS_DISCOVERY_REQUEST
    except struct.error:
        return False

def main():
    port = 2011

    # Create UDP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

    # Bind to the discovery port
    sock.bind(('0.0.0.0', port))

    print(f"Mock ESP32 Discovery Server")
    print(f"Listening on 0.0.0.0:{port}")
    print(f"Board: {BOARD_NAME}, Type: {BOARD_TYPE}, Serial: 0x{BOARD_ID:08X}")
    print("-" * 60)
    print("Waiting for discovery requests... Press Ctrl+C to stop\n")

    try:
        while True:
            # Receive discovery request
            data, client_addr = sock.recvfrom(1024)

            client_ip = client_addr[0]
            client_port = client_addr[1]

            print(f"Received {len(data)} bytes from {client_ip}:{client_port}")
            print(f"Request hex: {data.hex()}")

            # Validate discovery request
            if validate_discovery_request(data):
                print("✓ Valid discovery request detected")

                # Convert client IP to network byte order (as ESP would)
                try:
                    client_ip_int = int.from_bytes(socket.inet_aton(client_ip), byteorder='little')
                except:
                    client_ip_int = 0

                # Create discovery response
                response = create_discovery_response(client_ip_int)

                print(f"Sending discovery response: {len(response)} bytes to {client_ip}:{client_port}")
                print(f"Response hex (first 12 bytes): {response[:12].hex()}")

                # Send response back to client
                sent = sock.sendto(response, client_addr)

                if sent == len(response):
                    print(f"✓ Discovery response sent successfully: {sent} bytes")
                else:
                    print(f"✗ Partial send: {sent}/{len(response)} bytes")

            else:
                print("✗ Invalid discovery request (wrong magic or command)")

            print("-" * 60)

    except KeyboardInterrupt:
        print("\n\nMock server stopped")
    finally:
        sock.close()

if __name__ == "__main__":
    main()
