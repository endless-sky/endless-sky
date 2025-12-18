# ENet Library Integration Guide

**Date**: 2025-12-18
**Status**: ✅ COMPLETE
**ENet Version**: 1.3.18
**Phase**: 1.1 - Network Foundation

---

## Overview

This document describes the integration of the [ENet library](http://enet.bespin.org/) into Endless Sky for multiplayer networking support. ENet is a reliable UDP networking library designed specifically for real-time games.

### Why ENet?

- **Lightweight**: Minimal overhead, perfect for game networking
- **Reliable UDP**: Combines reliability of TCP with speed of UDP
- **Multiple channels**: Supports reliable ordered, reliable unordered, and unreliable sequenced delivery
- **Built-in features**: Automatic fragmentation, bandwidth limiting, connection management
- **Cross-platform**: Works on Linux, Windows, macOS
- **Battle-tested**: Used in many successful multiplayer games
- **Active development**: Maintained and regularly updated

---

## Integration Status

| Component | Status | Details |
|-----------|--------|---------|
| vcpkg Dependency | ✅ Complete | Added to vcpkg.json |
| CMake Integration | ✅ Complete | find_package and link configured |
| Build Verification | ✅ Complete | Successfully built v1.3.18 |
| Connection Test | ✅ Complete | All tests passed |
| Documentation | ✅ Complete | This file |

---

## Installation

### 1. vcpkg.json Configuration

ENet has been added to the `vcpkg.json` dependencies in the `system-libs` feature:

```json
{
  "features": {
    "system-libs": {
      "dependencies": [
        "enet",
        ...other dependencies...
      ]
    }
  }
}
```

### 2. CMakeLists.txt Configuration

The root `CMakeLists.txt` includes:

```cmake
# Find ENet package
find_package(enet CONFIG REQUIRED)

# Link ENet to external libraries
target_link_libraries(ExternalLibraries INTERFACE ... enet::enet)
```

### 3. Build Process

When building Endless Sky with vcpkg enabled, ENet will be automatically:
1. Downloaded from https://github.com/lsalzman/enet
2. Built for your platform (x64-linux, x64-windows, etc.)
3. Installed to `build/vcpkg_installed/{triplet}/`
4. Linked into Endless Sky

---

## Build Instructions

### Linux

```bash
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DES_USE_VCPKG=ON
cmake --build .
```

### Windows (Visual Studio)

```bash
mkdir build
cd build
cmake .. -DES_USE_VCPKG=ON
cmake --build . --config Debug
```

### macOS

```bash
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DES_USE_VCPKG=ON
cmake --build .
```

---

## Testing

### Proof-of-Concept Test

A standalone test program (`tests/network/test_enet_connection.cpp`) verifies ENet integration.

**Features Tested:**
- ENet initialization and deinitialization
- Server creation and binding
- Client connection to server
- Reliable packet transmission (client → server)
- Reliable packet transmission (server → client)
- Graceful disconnection
- Clean shutdown

**Running the Test:**

```bash
# Compile manually (if needed)
g++ -std=c++20 \
    -I./build/vcpkg_installed/x64-linux/include \
    tests/network/test_enet_connection.cpp \
    -L./build/vcpkg_installed/x64-linux/lib \
    -lenet -lpthread \
    -o tests/network/test_enet_connection

# Run the test
./tests/network/test_enet_connection
```

**Expected Output:**

```
=== ENet Integration Test ===
ENet Version: 1.3.18
ENet initialized successfully
[SERVER] Started on port 12345
[CLIENT] Connecting to localhost:12345
[CLIENT] Connected to server
[CLIENT] Sent: "Hello from client!"
[SERVER] Client connected from 127.0.0.1:xxxxx
[SERVER] Received: "Hello from client!"
[SERVER] Sent response
[CLIENT] Received: "Hello from server!"
[SERVER] Client disconnected
[CLIENT] Disconnected
[CLIENT] Shut down
[SERVER] Test successful!
[SERVER] Shut down
ENet deinitialized

=== Test Results ===
Client connected:   ✓ PASS
Message sent:       ✓ PASS
Response received:  ✓ PASS
Server processed:   ✓ PASS

✓ ALL TESTS PASSED - ENet integration successful!
```

---

## ENet API Overview

### Core Functions

```cpp
// Initialization
int enet_initialize();
void enet_deinitialize();

// Host Creation
ENetHost* enet_host_create(
    const ENetAddress* address,  // NULL for client, bind address for server
    size_t peerCount,            // Max number of peers
    size_t channelLimit,         // Number of channels (0 = max)
    enet_uint32 incomingBandwidth,
    enet_uint32 outgoingBandwidth
);

// Connection
ENetPeer* enet_host_connect(
    ENetHost* host,
    const ENetAddress* address,
    size_t channelCount,
    enet_uint32 data
);

// Event Processing
int enet_host_service(
    ENetHost* host,
    ENetEvent* event,
    enet_uint32 timeout
);

// Packet Creation & Sending
ENetPacket* enet_packet_create(
    const void* data,
    size_t dataLength,
    enet_uint32 flags  // ENET_PACKET_FLAG_RELIABLE, etc.
);

int enet_peer_send(
    ENetPeer* peer,
    enet_uint8 channelID,
    ENetPacket* packet
);

// Disconnection
void enet_peer_disconnect(ENetPeer* peer, enet_uint32 data);
void enet_peer_reset(ENetPeer* peer);
```

### Event Types

```cpp
ENET_EVENT_TYPE_NONE        // No event occurred
ENET_EVENT_TYPE_CONNECT     // A peer connected
ENET_EVENT_TYPE_RECEIVE     // A packet was received
ENET_EVENT_TYPE_DISCONNECT  // A peer disconnected
```

### Packet Flags

```cpp
ENET_PACKET_FLAG_RELIABLE            // Guaranteed delivery
ENET_PACKET_FLAG_UNSEQUENCED         // No sequencing
ENET_PACKET_FLAG_NO_ALLOCATE         // Don't copy packet data
ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT // Can be fragmented
```

---

## Usage Examples

### Creating a Server

```cpp
#include <enet/enet.h>

// Initialize ENet
if (enet_initialize() != 0) {
    // Error handling
}

// Bind to port 12345, accept up to 32 clients, use 2 channels
ENetAddress address;
address.host = ENET_HOST_ANY;
address.port = 12345;

ENetHost* server = enet_host_create(&address, 32, 2, 0, 0);
if (server == nullptr) {
    // Error handling
}

// Event loop
ENetEvent event;
while (running) {
    while (enet_host_service(server, &event, 100) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                // Handle new connection
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                // Handle received packet
                enet_packet_destroy(event.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                // Handle disconnection
                break;
        }
    }
}

// Cleanup
enet_host_destroy(server);
enet_deinitialize();
```

### Creating a Client

```cpp
#include <enet/enet.h>

// Initialize ENet
if (enet_initialize() != 0) {
    // Error handling
}

// Create client host (NULL address = don't bind)
ENetHost* client = enet_host_create(nullptr, 1, 2, 0, 0);
if (client == nullptr) {
    // Error handling
}

// Connect to server
ENetAddress address;
enet_address_set_host(&address, "127.0.0.1");
address.port = 12345;

ENetPeer* peer = enet_host_connect(client, &address, 2, 0);
if (peer == nullptr) {
    // Error handling
}

// Wait for connection
ENetEvent event;
if (enet_host_service(client, &event, 5000) > 0 &&
    event.type == ENET_EVENT_TYPE_CONNECT) {
    // Connected!
}

// Send packet
const char* message = "Hello Server!";
ENetPacket* packet = enet_packet_create(
    message,
    strlen(message) + 1,
    ENET_PACKET_FLAG_RELIABLE
);
enet_peer_send(peer, 0, packet);
enet_host_flush(client);

// Cleanup
enet_host_destroy(client);
enet_deinitialize();
```

---

## Channel Configuration

For Endless Sky multiplayer, we'll use multiple channels:

| Channel | Type | Purpose | Update Rate |
|---------|------|---------|-------------|
| 0 | Reliable Ordered | Connection, chat, missions | As needed |
| 1 | Unreliable Sequenced | Ship positions, commands | 60 Hz |
| 2 | Reliable Unordered | Projectile spawns, effects | As needed |

---

## Performance Characteristics

### Bandwidth Usage

ENet packet overhead:
- **Header**: ~44 bytes (UDP header + ENet header)
- **Payload**: Variable (your data)
- **ACK packets**: ~28 bytes (for reliable packets)

For Endless Sky (estimated):
- **Commands** (client → server): ~50 bytes/packet × 60 Hz = 3 KB/s
- **State updates** (server → client): ~100 bytes/packet × 20 Hz = 2 KB/s
- **Total per client**: ~5-10 KB/s upstream, 50-100 KB/s downstream

### Latency

- **Local network**: <1 ms
- **LAN**: 1-5 ms
- **Internet (good)**: 20-50 ms
- **Internet (typical)**: 50-100 ms
- **Internet (poor)**: 100-200 ms

ENet Target: Playable up to 200ms latency with client prediction.

---

## Troubleshooting

### Build Issues

**Problem**: `enet/enet.h: No such file or directory`

**Solution**: Ensure vcpkg has built ENet:
```bash
./vcpkg/vcpkg install enet
```

**Problem**: CMake can't find ENet

**Solution**: Use vcpkg toolchain:
```bash
cmake .. -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake
```

### Runtime Issues

**Problem**: `enet_initialize()` returns non-zero

**Solution**: Check platform-specific requirements (Windows may need Winsock initialization)

**Problem**: Connection timeouts

**Solution**:
- Check firewall settings
- Verify port is not already in use
- Ensure server is running and accessible

**Problem**: High packet loss

**Solution**:
- Increase send buffer size: `enet_host_create(..., 0, 0)` → `enet_host_create(..., 57600, 14400)`
- Adjust bandwidth throttling
- Check network conditions

---

## References

- **ENet Official Site**: http://enet.bespin.org/
- **GitHub Repository**: https://github.com/lsalzman/enet
- **vcpkg Package**: https://vcpkg.link/ports/enet
- **ENet Tutorial**: http://enet.bespin.org/Tutorial.html
- **API Documentation**: http://enet.bespin.org/group__global.html

---

## Next Steps (Phase 1.2)

Now that ENet is integrated, the next phase involves:

1. **Network Abstraction Layer** (Phase 1.2)
   - Create `NetworkManager` class
   - Implement `NetworkClient` and `NetworkServer` classes
   - Connection management and lifecycle

2. **Binary Serialization** (Phase 1.3)
   - `PacketWriter` and `PacketReader` classes
   - Efficient binary protocol for game data

3. **Protocol Definition** (Phase 1.4)
   - Define all packet types
   - Implement packet handlers
   - Protocol versioning

---

**Document Status**: ✅ Complete
**Last Updated**: 2025-12-18
**Author**: Development Team
