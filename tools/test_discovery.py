#!/usr/bin/env python3
"""
Datastream 2.0 Board Discovery Test Script

This script tests the board auto-detection feature implemented in the dsUDP task.
It sends discovery requests and parses responses from datastream boards.

Author: Generated for datastream testing
Date: 2025
"""

import socket
import struct
import time
import sys
import argparse
import select
from typing import List, Dict, Optional

# Protocol constants
DS_DISCOVERY_MAGIC = 0xDEADBEEF
DS_DISCOVERY_REQUEST = 0x01
DS_DISCOVERY_RESPONSE = 0x02

# Board type definitions (matching datastream_default_config.h)
BOARD_TYPES = {
    0: "UNKNOWN",
    1: "datastream_test", 
    2: "CUSTOM_1",
    3: "CUSTOM_2"
}

class DatastreamDiscovery:
    def __init__(self, timeout: float = 3.0, port: int = 2011):
        self.timeout = timeout
        self.port = port
        self.boards = []

    def create_discovery_request(self) -> bytes:
        """Create a discovery request packet (5 bytes)"""
        return struct.pack('<I B', DS_DISCOVERY_MAGIC, DS_DISCOVERY_REQUEST)

    def parse_discovery_response(self, data: bytes, source_addr: tuple) -> Optional[Dict]:
        """Parse discovery response packet (44 bytes with MAC address)"""
        if len(data) < 44:
            print(f"Warning: Response too short ({len(data)} bytes) from {source_addr[0]}")
            return None
            
        try:
            # Unpack the discovery response structure (with MAC address)
            unpacked = struct.unpack('<I B B H I I H H 6s 2s 16s', data[:44])
            
            magic, command, board_type, firmware_version, board_serial, \
            ip_address, tcp_port, udp_port, mac_raw, reserved, board_name_raw = unpacked
            
            # Validate response
            if magic != DS_DISCOVERY_MAGIC:
                print(f"Warning: Invalid magic number 0x{magic:08X} from {source_addr[0]}")
                return None
                
            if command != DS_DISCOVERY_RESPONSE:
                print(f"Warning: Invalid command {command} from {source_addr[0]}")
                return None
            
            # Parse board name (null-terminated string)
            try:
                board_name = board_name_raw.decode('utf-8').rstrip('\x00')
            except UnicodeDecodeError:
                board_name = "INVALID_NAME"
            
            # Format MAC address
            mac_address = ':'.join([f'{b:02x}' for b in mac_raw])
            
            # Format firmware version
            fw_major = (firmware_version >> 8) & 0xFF
            fw_minor = firmware_version & 0xFF
            
            # Convert IP address from network byte order
            ip_str = socket.inet_ntoa(struct.pack('!I', ip_address)) if ip_address != 0 else source_addr[0]
            
            board_info = {
                'source_ip': source_addr[0],
                'reported_ip': ip_str,
                'board_type': board_type,
                'board_type_name': BOARD_TYPES.get(board_type, f"UNKNOWN_{board_type}"),
                'firmware_version': f"{fw_major}.{fw_minor}",
                'board_serial': f"0x{board_serial:08X}",
                'tcp_port': tcp_port,
                'udp_port': udp_port,
                'mac_address': mac_address,
                'board_name': board_name,
                'response_time': time.time()
            }
            
            return board_info
            
        except struct.error as e:
            print(f"Error parsing response from {source_addr[0]}: {e}")
            return None

    def discover_boards(self, target_ip: str = '<broadcast>') -> List[Dict]:
        """Discover datastream boards on the network"""
        print(f"Starting board discovery on port {self.port} (timeout: {self.timeout}s)")
        print(f"Target: {target_ip}")
        print("-" * 60)
        
        self.boards = []
        
        # Create UDP socket
        sock = None
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

            # Bind to INADDR_ANY to receive responses
            sock.bind(('', 0))
            local_addr = sock.getsockname()
            print(f"Socket bound to {local_addr[0] if local_addr[0] else '0.0.0.0'}:{local_addr[1]}")

            sock.settimeout(0.2)

        except socket.error as e:
            print(f"Error creating socket: {e}")
            if sock:
                sock.close()
            return []

        try:
            # Create and send discovery request
            request = self.create_discovery_request()
            print(f"Sending discovery request: {len(request)} bytes")
            print(f"Request data: {request.hex()}")

            # Send request
            if target_ip == '<broadcast>':
                # Use actual broadcast address
                sock.sendto(request, ('255.255.255.255', self.port))
            else:
                # Send to specific IP
                sock.sendto(request, (target_ip, self.port))

            actual_target = '255.255.255.255' if target_ip == '<broadcast>' else target_ip
            print(f"Discovery request sent to {actual_target}:{self.port}")
            
            # Collect responses
            start_time = time.time()
            responses_received = 0
            receive_attempts = 0

            print("\nWaiting for responses...")

            while time.time() - start_time < self.timeout:
                try:
                    receive_attempts += 1
                    if receive_attempts % 10 == 0:
                        elapsed = time.time() - start_time
                        print(f"Still listening... ({receive_attempts} attempts, {elapsed:.1f}s elapsed)")

                    data, addr = sock.recvfrom(1024)

                    # If we get here, we received data!
                    responses_received += 1

                    print(f"\n✓ Response #{responses_received} received from {addr[0]}:{addr[1]}")
                    print(f"Response size: {len(data)} bytes")
                    print(f"Response data: {data[:50].hex()}{'...' if len(data) > 50 else ''}")

                    board_info = self.parse_discovery_response(data, addr)

                    if board_info:
                        self.boards.append(board_info)
                        print(f"✓ Valid board discovered: {board_info['board_name']}")
                    else:
                        print("✗ Invalid or unrecognized response")

                except socket.timeout:
                    # Timeout - this is normal, just continue
                    continue
                except socket.error as e:
                    print(f"Socket error while receiving: {e}")
                    break
                except Exception as e:
                    print(f"Unexpected error: {type(e).__name__}: {e}")
                    break
                    
        except socket.error as e:
            print(f"Error sending discovery request: {e}")
        finally:
            if sock:
                sock.close()
        
        print(f"\nDiscovery completed. Found {len(self.boards)} board(s)")
        return self.boards

    def print_board_summary(self):
        """Print a summary of discovered boards"""
        if not self.boards:
            print("\nNo boards discovered.")
            return
        
        print(f"\n{'='*80}")
        print(f"DISCOVERED BOARDS SUMMARY ({len(self.boards)} found)")
        print(f"{'='*80}")
        
        for i, board in enumerate(self.boards, 1):
            print(f"\n[{i}] {board['board_name']}")
            print(f"    Type: {board['board_type_name']} (ID: {board['board_type']})")
            print(f"    Serial: {board['board_serial']}")
            print(f"    MAC Address: {board['mac_address']}")
            print(f"    Firmware: v{board['firmware_version']}")
            print(f"    Network: {board['source_ip']} (reported: {board['reported_ip']})")
            print(f"    Ports: TCP/{board['tcp_port']}, UDP/{board['udp_port']}")

    def test_datastream_communication(self, board: Dict) -> bool:
        """Test basic datastream communication with discovered board"""
        print(f"\nTesting datastream communication with {board['board_name']}...")
        
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sock.settimeout(2.0)
            
            # Create a simple datastream read parameter command (type=3, addr=0, value=0)
            # Assuming 6-byte packet without CRC
            ds_packet = struct.pack('<B B I', 3, 0, 0)  # READ_PARAMETER, addr=0, value=0
            
            print(f"Sending datastream packet: {ds_packet.hex()}")
            sock.sendto(ds_packet, (board['source_ip'], board['udp_port']))
            
            # Try to receive response
            response, addr = sock.recvfrom(64)
            print(f"Received datastream response: {response.hex()}")
            
            if len(response) >= 6:
                status, addr_resp, value = struct.unpack('<b B I', response[:6])
                print(f"Status: {status}, Address: {addr_resp}, Value: {value}")
                return status >= 0
            else:
                print(f"Response too short: {len(response)} bytes")
                return False
                
        except socket.timeout:
            print("Timeout waiting for datastream response")
            return False
        except socket.error as e:
            print(f"Datastream communication error: {e}")
            return False
        finally:
            sock.close()

def main():
    parser = argparse.ArgumentParser(description='Datastream Board Discovery Test')
    parser.add_argument('--port', type=int, default=2011, 
                      help='UDP port for discovery (default: 2011)')
    parser.add_argument('--timeout', type=float, default=3.0,
                      help='Discovery timeout in seconds (default: 3.0)')
    parser.add_argument('--target', type=str, default='<broadcast>',
                      help='Target IP address (default: <broadcast>)')
    parser.add_argument('--test-communication', action='store_true',
                      help='Test basic datastream communication with discovered boards')
    parser.add_argument('--verbose', action='store_true',
                      help='Enable verbose output')
    
    args = parser.parse_args()
    
    if args.verbose:
        print("Verbose mode enabled")
        print(f"Configuration:")
        print(f"  Port: {args.port}")
        print(f"  Timeout: {args.timeout}s") 
        print(f"  Target: {args.target}")
        print("")
    
    # Create discovery instance
    discovery = DatastreamDiscovery(timeout=args.timeout, port=args.port)
    
    # Perform discovery
    boards = discovery.discover_boards(target_ip=args.target)
    
    # Print results
    discovery.print_board_summary()
    
    # Test communication if requested
    if args.test_communication and boards:
        print(f"\n{'='*80}")
        print("TESTING DATASTREAM COMMUNICATION")
        print(f"{'='*80}")
        
        for board in boards:
            success = discovery.test_datastream_communication(board)
            status = "✓ SUCCESS" if success else "✗ FAILED"
            print(f"{status}: {board['board_name']} at {board['source_ip']}")
    
    # Exit with appropriate code
    sys.exit(0 if boards else 1)

if __name__ == "__main__":
    main()