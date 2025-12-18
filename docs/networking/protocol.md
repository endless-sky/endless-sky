# Endless Sky Multiplayer Protocol Specification

**Version**: 1.0
**Protocol Version**: 1
**Last Updated**: 2025-12-18

---

## Table of Contents

1. [Overview](#overview)
2. [Protocol Architecture](#protocol-architecture)
3. [Packet Format](#packet-format)
4. [Packet Types](#packet-types)
5. [Connection Flow](#connection-flow)
6. [Data Types](#data-types)
7. [Packet Structures](#packet-structures)
8. [Error Handling](#error-handling)
9. [Security Considerations](#security-considerations)
10. [Version Negotiation](#version-negotiation)

---

## Overview

The Endless Sky Multiplayer Protocol is a binary protocol built on top of ENet (reliable UDP) for real-time multiplayer gameplay. The protocol is designed for:

- **Low Latency**: Optimized for 20-30 Hz state updates
- **Reliability**: ENet provides reliable ordered/unordered channels
- **Efficiency**: Binary serialization with network byte order
- **Type Safety**: Strongly-typed packet system
- **Extensibility**: Protocol version negotiation for future updates

### Key Specifications

- **Transport**: ENet 1.3.18 (Reliable UDP)
- **Byte Order**: Big-endian (network byte order)
- **Encoding**: Binary (not text-based)
- **Header Size**: 11 bytes
- **Magic Number**: 0x45534D50 ("ESMP")
- **Current Protocol Version**: 1

---

## Protocol Architecture

### Client-Server Model

The protocol uses an **authoritative server** architecture:

```
Client                          Server
  |                               |
  |  1. CONNECT_REQUEST           |
  |------------------------------>|
  |                               |
  |  2. CONNECT_ACCEPT/REJECT    |
  |<------------------------------|
  |                               |
  |  3. CLIENT_COMMAND (60 Hz)   |
  |------------------------------>|
  |                               |
  |  4. SERVER_WORLD_STATE(20Hz) |
  |<------------------------------|
  |                               |
  |  5. PING/PONG                 |
  |<----------------------------->|
  |                               |
  |  6. DISCONNECT                |
  |------------------------------>|
```

### Update Frequencies

| Data Type | Direction | Frequency | Channel | Reliability |
|-----------|-----------|-----------|---------|-------------|
| Client Commands | Client → Server | 60 Hz | 1 | Unreliable Sequenced |
| World State | Server → Client | 20 Hz | 0 | Reliable Ordered |
| Ship Updates | Server → Client | 20 Hz | 1 | Unreliable Sequenced |
| Connection/Chat | Bidirectional | On-demand | 0 | Reliable Ordered |
| Projectiles/Effects | Server → Client | On-spawn | 2 | Reliable Unordered |

### Channel Configuration

- **Channel 0**: Reliable Ordered (connection, chat, missions)
- **Channel 1**: Unreliable Sequenced (positions, commands)
- **Channel 2**: Reliable Unordered (projectiles, effects)

---

## Packet Format

### Packet Header (11 bytes)

All packets begin with an 11-byte header:

```
Offset | Size | Field            | Description
-------|------|------------------|----------------------------------
0      | 4    | magic            | 0x45534D50 ("ESMP")
4      | 2    | protocolVersion  | Protocol version (currently 1)
6      | 1    | type             | Packet type (PacketType enum)
7      | 4    | payloadSize      | Size of payload (excludes header)
-------|------|------------------|----------------------------------
Total: 11 bytes
```

**Notes**:
- Header uses `#pragma pack(1)` to prevent padding
- All multi-byte values are in network byte order (big-endian)
- Magic number validates packet integrity
- Protocol version enables backwards compatibility

### Packet Structure

```
+------------------+
| Packet Header    | 11 bytes
+------------------+
| Payload          | Variable size (0 to 65535 bytes)
+------------------+
```

**Maximum Packet Size**: 65,546 bytes (11-byte header + 65,535-byte payload)

---

## Packet Types

### Connection Packets (1-6)

| ID | Name | Direction | Description |
|----|------|-----------|-------------|
| 1 | CONNECT_REQUEST | C → S | Client requests connection |
| 2 | CONNECT_ACCEPT | S → C | Server accepts connection |
| 3 | CONNECT_REJECT | S → C | Server rejects connection |
| 4 | DISCONNECT | Bidirectional | Graceful disconnect |
| 5 | PING | Bidirectional | Latency measurement |
| 6 | PONG | Bidirectional | Ping response |

### Client → Server Packets (10-19)

| ID | Name | Description |
|----|------|-------------|
| 10 | CLIENT_COMMAND | Ship commands (60 Hz) |
| 11 | CLIENT_CHAT | Chat message |
| 12 | CLIENT_READY | Client ready for game start |

### Server → Client Packets (20-29)

| ID | Name | Description |
|----|------|-------------|
| 20 | SERVER_WELCOME | Initial connection data |
| 21 | SERVER_WORLD_STATE | Full state snapshot (20 Hz) |
| 22 | SERVER_SHIP_UPDATE | Individual ship update |
| 23 | SERVER_PROJECTILE_SPAWN | New projectile created |
| 24 | SERVER_SHIP_DESTROYED | Ship was destroyed |
| 25 | SERVER_EFFECT_SPAWN | Visual effect spawned |
| 26 | SERVER_CHAT | Chat message broadcast |
| 27 | SERVER_PLAYER_JOIN | Player joined notification |
| 28 | SERVER_PLAYER_LEAVE | Player left notification |

### Synchronization Packets (30-39)

| ID | Name | Description |
|----|------|-------------|
| 30 | FULL_SYNC_REQUEST | Client requests full state |
| 31 | FULL_SYNC_RESPONSE | Server sends full state |

---

## Connection Flow

### Client Connection Sequence

```
1. Client → Server: CONNECT_REQUEST
   - Payload: {protocolVersion, playerName, clientVersion}

2. Server validates:
   - Protocol version compatible?
   - Server not full?
   - Player name valid?

3a. Success: Server → Client: CONNECT_ACCEPT
    - Payload: {playerUUID, serverTickRate, worldStateRate, serverName}
    - Client enters game

3b. Failure: Server → Client: CONNECT_REJECT
    - Payload: {reasonCode, reasonMessage}
    - Connection terminated
```

### Client Disconnect Sequence

```
1. Client → Server: DISCONNECT
   - Payload: {reasonCode, reasonMessage}

2. Server:
   - Broadcasts PLAYER_LEAVE to other clients
   - Removes player from game
   - Closes ENet connection

3. Server → Client: (ENet disconnect event)
```

---

## Data Types

### Primitive Types

| Type | Size | Endianness | Description |
|------|------|------------|-------------|
| uint8 | 1 byte | N/A | Unsigned 8-bit integer |
| uint16 | 2 bytes | Big-endian | Unsigned 16-bit integer |
| uint32 | 4 bytes | Big-endian | Unsigned 32-bit integer |
| uint64 | 8 bytes | Big-endian | Unsigned 64-bit integer |
| int8 | 1 byte | N/A | Signed 8-bit integer |
| int16 | 2 bytes | Big-endian | Signed 16-bit integer |
| int32 | 4 bytes | Big-endian | Signed 32-bit integer |
| int64 | 8 bytes | Big-endian | Signed 64-bit integer |
| float | 4 bytes | Big-endian | IEEE 754 single precision |
| double | 8 bytes | Big-endian | IEEE 754 double precision |

### Complex Types

| Type | Size | Format | Description |
|------|------|--------|-------------|
| string | Variable | uint16 length + data | Length-prefixed UTF-8 string (max 65535 bytes) |
| Point | 16 bytes | 2 × double | 2D position (X, Y) |
| Angle | 8 bytes | double | Angle in degrees (0-360) |
| UUID | 38 bytes | string | UUID in string format (36 chars + 2-byte length) |

**Note**: Angle is transmitted as double for precision, but stored internally as int32_t in the game.

---

## Packet Structures

### CONNECT_REQUEST (Client → Server)

```
Field               | Type   | Size
--------------------|--------|--------
clientProtocolVersion | uint16 | 2 bytes
playerName          | string | Variable
clientVersion       | string | Variable
```

**Example**:
```
Protocol Version: 1
Player Name: "SpaceCaptain"
Client Version: "0.10.0-multiplayer"
```

### CONNECT_ACCEPT (Server → Client)

```
Field               | Type   | Size
--------------------|--------|--------
playerUUID          | UUID   | 38 bytes
serverTickRate      | uint32 | 4 bytes
worldStateRate      | uint32 | 4 bytes
serverName          | string | Variable
welcomeMessage      | string | Variable
```

**Example**:
```
Player UUID: "550e8400-e29b-41d4-a716-446655440000"
Server Tick Rate: 60 (60 FPS simulation)
World State Rate: 20 (20 Hz updates)
Server Name: "Endless Sky Official #1"
Welcome Message: "Welcome to multiplayer!"
```

### CONNECT_REJECT (Server → Client)

```
Field               | Type   | Size
--------------------|--------|--------
reasonCode          | uint8  | 1 byte
reasonMessage       | string | Variable
```

**Reason Codes**:
- 0: SERVER_FULL
- 1: INCOMPATIBLE_VERSION
- 2: BANNED
- 3: INVALID_NAME
- 4: INTERNAL_ERROR
- 5: MAINTENANCE

### CLIENT_COMMAND (Client → Server, 60 Hz)

```
Field               | Type   | Size
--------------------|--------|--------
shipUUID            | UUID   | 38 bytes
commandState        | uint64 | 8 bytes
commandTurn         | double | 8 bytes
sequenceNumber      | uint32 | 4 bytes
------------------------------------
Total:                        58 bytes
```

**Command State Bits**:
```
Bit 0:  FORWARD
Bit 1:  LEFT
Bit 2:  RIGHT
Bit 3:  AUTOSTEER
Bit 4:  BACK
Bit 5:  PRIMARY (fire primary weapons)
Bit 6:  SECONDARY (fire secondary weapons)
Bit 7:  AFTERBURNER
Bit 8:  CLOAK
Bit 9:  JUMP
... (64 bits total)
```

### SERVER_SHIP_UPDATE (Server → Client, 20 Hz)

```
Field               | Type   | Size
--------------------|--------|--------
shipUUID            | UUID   | 38 bytes
positionX           | double | 8 bytes
positionY           | double | 8 bytes
velocityX           | double | 8 bytes
velocityY           | double | 8 bytes
facing              | double | 8 bytes
shields             | float  | 4 bytes
hull                | float  | 4 bytes
energy              | float  | 4 bytes
fuel                | float  | 4 bytes
flags               | uint16 | 2 bytes
shipName            | string | Variable
modelName           | string | Variable
------------------------------------
Minimum:                      88 bytes
```

**Ship Flags** (uint16 bitmask):
```
Bit 0:  THRUSTING
Bit 1:  REVERSE
Bit 2:  TURNING_LEFT
Bit 3:  TURNING_RIGHT
Bit 4:  FIRING_PRIMARY
Bit 5:  FIRING_SECONDARY
Bit 6:  AFTERBURNER
Bit 7:  CLOAKED
Bit 8:  HYPERSPACING
Bit 9:  LANDING
Bit 10: DISABLED
Bit 11: OVERHEATED
```

### PING (Bidirectional)

```
Field               | Type   | Size
--------------------|--------|--------
timestamp           | uint64 | 8 bytes
sequenceNumber      | uint32 | 4 bytes
------------------------------------
Total:                        12 bytes
```

### PONG (Bidirectional)

```
Field               | Type   | Size
--------------------|--------|--------
timestamp           | uint64 | 8 bytes
sequenceNumber      | uint32 | 4 bytes
------------------------------------
Total:                        12 bytes
```

**RTT Calculation**:
```
RTT = (currentTime - timestamp) milliseconds
```

### CHAT (Bidirectional)

```
Field               | Type   | Size
--------------------|--------|--------
senderUUID          | UUID   | 38 bytes
timestamp           | uint64 | 8 bytes
senderName          | string | Variable
message             | string | Variable
```

---

## Error Handling

### Packet Validation

All packets must:
1. Have valid magic number (0x45534D50)
2. Have supported protocol version
3. Have valid packet type
4. Have payload size ≤ 65535 bytes
5. Have payload size matching actual data

**Invalid packets are silently dropped.**

### CRC32 Checksum (Optional)

For critical packets, CRC32 checksums can be computed:

```cpp
uint32_t crc = PacketValidator::ComputeCRC32(data, size);
bool valid = PacketValidator::VerifyCRC32(data, size, crc);
```

**CRC32 Algorithm**: IEEE 802.3 (polynomial 0xEDB88320)

### Connection Timeout

- **Client → Server**: If no packet received for 10 seconds, disconnect
- **Server → Client**: If no packet received for 30 seconds, disconnect
- **Ping Interval**: 1 second (keeps connection alive)

---

## Security Considerations

### Replay Attack Prevention

- Each CLIENT_COMMAND includes incrementing `sequenceNumber`
- Server validates sequence numbers are monotonically increasing
- Out-of-order packets (old sequence numbers) are rejected

### Input Validation

Server validates all client input:
- Command state bits are valid
- Turn value is in range [-1.0, 1.0]
- String lengths don't exceed maximums
- UUID references exist in game state

### Rate Limiting

- Commands: Maximum 60 Hz (one per frame)
- Chat messages: Maximum 5 per second
- Ping: Maximum 10 per second

**Violation → Warning → Kick**

### DoS Protection

- Connection limit: 32 simultaneous clients
- Bandwidth limit: 100 KB/s per client
- Packet size limit: 65,546 bytes
- ENet flood protection enabled

---

## Version Negotiation

### Protocol Versions

| Version | Date | Changes |
|---------|------|---------|
| 1 | 2025-12-18 | Initial multiplayer protocol |

### Compatibility Rules

**Current Policy**: Exact version match required

```cpp
bool compatible = (clientVersion == serverVersion);
```

**Future Policy** (Version 2+): Backwards compatibility

```cpp
bool compatible = (serverVersion >= clientVersion) &&
                  (serverVersion - clientVersion <= 2);
```

### Version Mismatch Flow

```
1. Client sends CONNECT_REQUEST with protocolVersion = N
2. Server checks compatibility
3a. Compatible: Send CONNECT_ACCEPT
3b. Incompatible: Send CONNECT_REJECT
    - reasonCode = INCOMPATIBLE_VERSION
    - reasonMessage = "Server requires protocol version M"
```

---

## Bandwidth Calculations

### Per-Client Uplink (Client → Server)

| Packet Type | Size | Frequency | Bandwidth |
|-------------|------|-----------|-----------|
| CLIENT_COMMAND | 58 bytes | 60 Hz | 3.48 KB/s |
| PING | 12 bytes | 1 Hz | 12 bytes/s |
| **Total Uplink** | | | **~3.5 KB/s** |

### Per-Client Downlink (Server → Client, 4 players, 20 ships)

| Packet Type | Size | Frequency | Count | Bandwidth |
|-------------|------|-----------|-------|-----------|
| SERVER_SHIP_UPDATE | 100 bytes | 20 Hz | 20 ships | 40 KB/s |
| PONG | 12 bytes | 1 Hz | 1 | 12 bytes/s |
| **Total Downlink** | | | | **~40 KB/s** |

### Server Total Bandwidth (4 clients)

- **Uplink**: 3.5 KB/s × 4 = 14 KB/s
- **Downlink**: 40 KB/s × 4 = 160 KB/s
- **Total**: 174 KB/s (~1.4 Mbps)

**Scalability**: 32 clients ≈ 11 Mbps (well within modern server capacity)

---

## Implementation Reference

### Sending a Packet (Client)

```cpp
// Create packet
PacketWriter writer(PacketType::CLIENT_COMMAND);
writer.WriteUuid(shipUuid);
writer.WriteUint64(commandState);
writer.WriteDouble(commandTurn);
writer.WriteUint32(sequenceNumber);

// Send via ENet
ENetPacket *packet = enet_packet_create(
    writer.GetDataPtr(),
    writer.GetSize(),
    ENET_PACKET_FLAG_UNSEQUENCED
);
enet_peer_send(serverPeer, 1, packet);  // Channel 1
```

### Receiving a Packet (Server)

```cpp
// ENet event loop
ENetEvent event;
while(enet_host_service(host, &event, timeout) > 0) {
    if(event.type == ENET_EVENT_TYPE_RECEIVE) {
        // Create reader
        PacketReader reader(event.packet->data, event.packet->dataLength);

        if(reader.IsValid()) {
            // Dispatch to handler
            packetHandler.Dispatch(reader, connection);
        }

        enet_packet_destroy(event.packet);
    }
}
```

### Registering a Handler

```cpp
PacketHandler handler;

handler.RegisterHandler(PacketType::CLIENT_COMMAND,
    [](PacketReader &reader, NetworkConnection *conn) {
        EsUuid shipUuid = reader.ReadUuid();
        uint64_t commandState = reader.ReadUint64();
        double commandTurn = reader.ReadDouble();
        uint32_t sequenceNumber = reader.ReadUint32();

        // Process command...
    }
);
```

---

## Future Extensions

### Planned for Protocol Version 2

- **Delta Compression**: Only send changed fields in ship updates
- **Entity Culling**: Only send ships within visibility range
- **Compression**: zlib/lz4 compression for large packets
- **Raw UUID**: 16-byte binary instead of 38-byte string
- **Command Serialization**: Proper Command class support

### Planned for Protocol Version 3

- **Encryption**: Optional AES encryption for sensitive data
- **Authentication**: Token-based player authentication
- **Anti-Cheat**: Server-side validation checksums
- **Spectator Mode**: Read-only connections

---

## References

- **ENet Documentation**: http://enet.bespin.org/
- **IEEE 802.3 CRC32**: https://en.wikipedia.org/wiki/Cyclic_redundancy_check
- **Network Byte Order**: RFC 1700 (big-endian)

---

**Document Version**: 1.0
**Last Updated**: 2025-12-18
**Maintained By**: Endless Sky Development Team
