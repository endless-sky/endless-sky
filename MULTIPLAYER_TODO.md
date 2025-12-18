# Endless Sky Multiplayer Implementation Plan

## Executive Summary

This document outlines a comprehensive plan to add multiplayer functionality to Endless Sky. Based on deep codebase analysis, this implementation will require significant architectural changes but can leverage existing serialization infrastructure and clean class boundaries.

**Estimated Effort**: 8-12 months for a 2-3 person team with network game programming experience
**Recommended Approach**: Client-Server Architecture (authoritative server)
**Target**: 2-8 player co-op multiplayer in shared universe

---

## I. CODEBASE ANALYSIS - KEY FINDINGS

### A. Current Architecture

**Game Engine** (`source/Engine.h/cpp` - 3055 lines)
- Double-buffered rendering architecture (calculation thread + render thread)
- Fixed 60 FPS timestep
- All game objects updated synchronously each frame
- Single PlayerInfo reference drives entire game state
- **ISSUE**: Assumes single flagship, single camera, single player

**PlayerInfo Class** (`source/PlayerInfo.h/cpp`)
- Manages player ships, missions, cargo, account, visited systems
- Tightly coupled to Engine and UI
- **ISSUE**: Global singleton pattern, not designed for multiple players

**Ship Class** (`source/Ship.h/cpp`)
- Inherits from Body (position, velocity, angle)
- Has UUID (EsUuid) for unique identification ✓
- Serializable via DataNode/DataWriter ✓
- Contains: outfits, cargo, crew, energy levels, commands
- **ISSUE**: Some state should be server-authoritative vs client-predicted

**Command System** (`source/Command.h/cpp`)
- 64-bit bitmask for all ship commands
- Separate `turn` value for analog control
- Lightweight and network-friendly ✓

**AI System** (`source/AI.h/cpp`)
- Controls all non-player ships
- Has access to player reference and all ship lists
- **ISSUE**: Needs separation between server-side AI and client-side rendering

**Serialization** (`source/DataNode.h`, `source/DataWriter.h`)
- Text-based hierarchical format
- Complete save/load system for game state ✓
- **ISSUE**: Too verbose for real-time networking, need binary protocol

### B. Technologies & Dependencies

- C++20 (CMake build system)
- SDL2 (windowing, input)
- OpenGL (rendering)
- OpenAL (audio)
- Existing libraries: PNG, JPEG, libavif, FLAC, minizip
- **NO NETWORKING LIBRARY** - must add one

### C. Critical Architectural Challenges

1. **Flagship-Centric Design**: Camera, UI, and game flow assume single flagship
2. **Global Game State**: GameData singleton manages universe (economy, governments, events)
3. **Non-Deterministic Elements**: Random fleet spawning, AI decisions using Random::Real()
4. **Frame-Perfect Simulation**: 60 FPS lockstep with immediate command application
5. **Mission System**: Per-player mission state with global universe effects
6. **No Network Layer**: Complete networking stack must be built from scratch

---

## II. MULTIPLAYER ARCHITECTURE DESIGN

### A. Architecture Choice: **Client-Server (Authoritative Server)**

**Why Not Peer-to-Peer Lockstep?**
- Requires deterministic simulation (hard with current RNG usage)
- Any client lag or desync breaks entire session
- No protection against cheating
- Difficult to handle player join/leave

**Client-Server Advantages:**
- Server is single source of truth
- Clients can predict and interpolate
- Better handles latency and packet loss
- Supports dedicated servers
- Enables future features (persistent worlds, more players)
- Better anti-cheat

### B. Network Model

```
SERVER (Authoritative)
├── Runs full game simulation (Engine + AI)
├── Processes client commands
├── Broadcasts state updates to all clients
├── Manages player connections
└── Persists universe state

CLIENTS (Predictive)
├── Send input commands to server
├── Receive state updates from server
├── Predict local player movement (client-side prediction)
├── Interpolate other players (entity interpolation)
├── Render game world
└── Handle UI and local effects
```

### C. Update Frequencies

- **Client → Server Commands**: 60 Hz (every frame)
- **Server → Client State**: 20-30 Hz (full state snapshots)
- **Server → Client Priority Events**: Immediate (combat, damage, deaths)

---

## III. IMPLEMENTATION ROADMAP

### PHASE 1: NETWORK FOUNDATION (6-8 weeks)

#### 1.1 Choose and Integrate Networking Library

**Options:**
- **ENet** (RECOMMENDED) - Lightweight, UDP-based, built for games
  - Reliable and unreliable channels
  - Simple C API, easy to integrate
  - Used by many successful games
- Alternatives: asio, RakNet, GameNetworkingSockets

**Tasks:**
- [ ] Add ENet to CMakeLists.txt and vcpkg dependencies
- [ ] Create wrapper classes for network abstraction
- [ ] Write connection/disconnection handling
- [ ] Implement basic packet sending/receiving

**New Files:**
```
source/network/NetworkManager.h/cpp       - Connection management
source/network/NetworkClient.h/cpp        - Client-side networking
source/network/NetworkServer.h/cpp        - Server-side networking
source/network/NetworkConnection.h/cpp    - Per-client connection state
source/network/NetworkConstants.h         - Port numbers, timeouts, etc.
```

#### 1.2 Binary Serialization System

**Current**: DataNode/DataWriter (text-based, too large for networking)
**Needed**: Efficient binary protocol for real-time state

**Tasks:**
- [ ] Design binary packet format (header + payload)
- [ ] Implement PacketWriter (binary serialization)
- [ ] Implement PacketReader (binary deserialization)
- [ ] Create serialization methods for core types:
  - Point, Angle (8 bytes each)
  - Command (9 bytes: 8 for bitmask + 1 for turn)
  - Ship state (position, velocity, facing, energy levels)
  - Projectile state
  - UUID (16 bytes)

**New Files:**
```
source/network/PacketWriter.h/cpp         - Binary packet serialization
source/network/PacketReader.h/cpp         - Binary packet deserialization
source/network/Packet.h                   - Packet types enum
source/network/PacketStructs.h            - Packed structs for network data
```

**Packet Design Example:**
```cpp
// Ship state packet (64 bytes, sent at 20Hz per ship)
struct ShipStatePacket {
    uint8_t packetType;           // 1 byte
    EsUuid shipId;                // 16 bytes
    Point position;               // 16 bytes (2 doubles)
    Point velocity;               // 16 bytes
    Angle facing;                 // 8 bytes
    float shields;                // 4 bytes (0-1 normalized)
    float hull;                   // 4 bytes
    float energy;                 // 4 bytes
    float fuel;                   // 4 bytes
    uint16_t flags;               // 2 bytes (isThrusting, isFiring, etc.)
};
```

#### 1.3 Protocol Definition

**Packet Types:**
```cpp
enum class PacketType : uint8_t {
    // Connection
    CONNECT_REQUEST = 1,
    CONNECT_ACCEPT = 2,
    CONNECT_REJECT = 3,
    DISCONNECT = 4,
    PING = 5,
    PONG = 6,

    // Client → Server
    CLIENT_COMMAND = 10,          // Ship commands (60Hz)
    CLIENT_CHAT = 11,
    CLIENT_READY = 12,

    // Server → Client
    SERVER_WELCOME = 20,          // Initial connection data
    SERVER_WORLD_STATE = 21,      // Full state snapshot (20Hz)
    SERVER_SHIP_UPDATE = 22,      // Individual ship update
    SERVER_PROJECTILE_SPAWN = 23,
    SERVER_SHIP_DESTROYED = 24,
    SERVER_EFFECT_SPAWN = 25,
    SERVER_CHAT = 26,
    SERVER_PLAYER_JOIN = 27,
    SERVER_PLAYER_LEAVE = 28,

    // Synchronization
    FULL_SYNC_REQUEST = 30,
    FULL_SYNC_RESPONSE = 31,
};
```

**Tasks:**
- [ ] Define all packet structures
- [ ] Implement packet versioning (protocol version field)
- [ ] Create packet handlers (dispatch table)
- [ ] Add packet compression for large state updates
- [ ] Implement packet delta compression (only send changes)

---

### PHASE 2: CORE ENGINE MODIFICATIONS (8-10 weeks)

#### 2.1 Separate Game State from Presentation

**Goal**: Decouple simulation from rendering/UI

**Tasks:**
- [ ] Create `GameState` class - holds all simulation data
  - Ship lists, projectiles, flotsam, asteroids
  - System state, planet state
  - NOT tied to rendering or UI
- [ ] Create `ClientState` class - holds local player data
  - Local PlayerInfo (what client controls)
  - Camera position and zoom
  - UI state
  - Prediction state
- [ ] Refactor Engine to use GameState instead of direct player reference
- [ ] Move rendering code into separate `Renderer` class

**New Files:**
```
source/GameState.h/cpp                    - Server-authoritative game state
source/ClientState.h/cpp                  - Client-specific state
source/Renderer.h/cpp                     - Rendering subsystem
source/multiplayer/MultiplayerEngine.h/cpp - MP-aware engine
```

**Modified Files:**
```
source/Engine.h/cpp                       - Refactor to use GameState
source/PlayerInfo.h/cpp                   - Split into server/client variants
source/AI.h/cpp                           - Server-side only AI
```

#### 2.2 Player Management System

**Current**: Single PlayerInfo
**Needed**: Multiple player tracking with separate ship control

**Tasks:**
- [ ] Create `PlayerManager` class
  - Maps player IDs to PlayerInfo objects
  - Tracks which ships belong to which players
  - Handles player join/leave
- [ ] Create `NetworkPlayer` class
  - Player ID (UUID)
  - Connection reference
  - Current ship (flagship)
  - Account, cargo, missions
  - Permissions/roles
- [ ] Modify Ship class to track owner player ID
- [ ] Update UI to show multiple players

**New Files:**
```
source/multiplayer/PlayerManager.h/cpp    - Manages all connected players
source/multiplayer/NetworkPlayer.h/cpp    - Network player representation
source/multiplayer/PlayerRegistry.h/cpp   - Player ID to data mapping
```

#### 2.3 Command Processing Pipeline

**Server-Side:**
```
Client Command → Validation → Queue → Apply to Ship → Simulate → Broadcast State
```

**Client-Side:**
```
Input → Create Command → Send to Server → Predict Locally → Wait for Server Correction
```

**Tasks:**
- [ ] Create command buffer system (queue commands by timestamp)
- [ ] Implement command validation (prevent impossible commands)
- [ ] Add command history tracking (for reconciliation)
- [ ] Create prediction/rollback system for local player

**New Files:**
```
source/multiplayer/CommandBuffer.h/cpp    - Buffered command queue
source/multiplayer/CommandValidator.h/cpp - Server-side validation
source/multiplayer/Predictor.h/cpp        - Client-side prediction
```

#### 2.4 Server Implementation

**Server Responsibilities:**
- Accept client connections
- Run authoritative game simulation
- Process client commands
- Broadcast state to all clients
- Handle player join/leave
- Persist game state
- Run AI for all NPCs

**Tasks:**
- [ ] Create dedicated server main loop
- [ ] Implement tick-rate control (60 FPS simulation, 20-30 Hz broadcast)
- [ ] Create snapshot system (full world state at intervals)
- [ ] Implement delta compression (only send what changed)
- [ ] Add server console for admin commands
- [ ] Create server configuration system

**New Files:**
```
source/server/ServerMain.cpp              - Dedicated server entry point
source/server/Server.h/cpp                - Main server class
source/server/ServerLoop.h/cpp            - Server game loop
source/server/SnapshotManager.h/cpp       - State snapshot system
source/server/ServerConfig.h/cpp          - Server configuration
```

**Build System:**
- [ ] Add `EndlessSkyServer` CMake target (separate executable)
- [ ] Create server-only compilation flags

#### 2.5 Client Implementation

**Client Responsibilities:**
- Connect to server
- Send player input commands
- Receive and apply state updates
- Predict local player movement
- Interpolate remote entities
- Render game world
- Handle UI

**Tasks:**
- [ ] Create client main loop (separate from single-player)
- [ ] Implement entity interpolation (smooth remote player movement)
- [ ] Implement client-side prediction with reconciliation
- [ ] Add lag compensation for local player
- [ ] Create connection quality indicators (ping, packet loss)
- [ ] Handle disconnection/reconnection

**New Files:**
```
source/client/MultiplayerClient.h/cpp     - Multiplayer client
source/client/EntityInterpolator.h/cpp    - Smooth entity movement
source/client/ClientReconciliation.h/cpp  - Prediction reconciliation
source/client/ConnectionMonitor.h/cpp     - Track connection quality
```

**Modified Files:**
```
source/main.cpp                           - Add multiplayer mode selection
source/Engine.cpp                         - Support both SP and MP modes
```

---

### PHASE 3: STATE SYNCHRONIZATION (6-8 weeks)

#### 3.1 Ship State Synchronization

**What Needs Syncing:**
- Position, velocity, facing (every tick, high priority)
- Energy, shields, hull, fuel (every tick, medium priority)
- Outfits, cargo (on change only, low priority)
- Commands (client→server only, 60Hz)
- Visual effects (on spawn only)

**Tasks:**
- [ ] Implement priority-based update system
  - Critical: position, combat state (every packet)
  - Important: energy levels, status (every 2-3 packets)
  - Low: cargo, outfits (on change)
- [ ] Create interest management (only sync nearby ships)
- [ ] Add snapshot interpolation for smooth movement
- [ ] Implement dead reckoning (predict movement between updates)

**New Files:**
```
source/multiplayer/StateSync.h/cpp        - State synchronization
source/multiplayer/InterestManager.h/cpp  - Relevant entity filtering
source/multiplayer/DeadReckoning.h/cpp    - Position prediction
```

#### 3.2 Projectile Synchronization

**Challenge**: Many projectiles, short-lived, need precise sync

**Approach**: Server-authoritative spawning, client-side prediction

**Tasks:**
- [ ] Server broadcasts projectile spawns (position, velocity, owner)
- [ ] Clients simulate projectile movement locally (deterministic)
- [ ] Server handles all collision detection
- [ ] Server broadcasts hits/impacts
- [ ] Implement projectile ID system (match client/server)

**New Files:**
```
source/multiplayer/ProjectileSync.h/cpp   - Projectile synchronization
source/multiplayer/CollisionAuthority.h/cpp - Server-side collision
```

#### 3.3 Visual Effects Synchronization

**Effects**: Explosions, sparks, engine flares, shield hits

**Approach**: Best-effort, client can miss some effects

**Tasks:**
- [ ] Server broadcasts critical effects (explosions, ship destruction)
- [ ] Clients generate local cosmetic effects (engine flares, shields)
- [ ] Implement effect pooling/culling for performance

#### 3.4 System/Universe State

**Shared State:**
- Current system all players are in
- NPC ships and fleets
- Asteroids and minables
- Flotsam
- System hazards

**Tasks:**
- [ ] Implement system transition synchronization
  - All players must be in same system (at least initially)
  - Coordinate hyperspace jumps
- [ ] Sync NPC ship spawning (server-authoritative)
- [ ] Sync asteroid mining (server tracks which are mined)
- [ ] Sync flotsam collection

**New Files:**
```
source/multiplayer/SystemSync.h/cpp       - System state synchronization
source/multiplayer/UniverseAuthority.h/cpp - Server universe management
```

---

### PHASE 4: MISSION & ECONOMY SYSTEM (4-6 weeks)

#### 4.1 Mission System Adaptation

**Challenges:**
- Missions can spawn NPCs globally
- Mission events can change universe state
- Missions are personal to player

**Approach Options:**

**Option A: Shared Missions** (simpler)
- All players share same missions
- Mission state tracked on server
- NPCs spawned globally
- All players get rewards

**Option B: Personal Missions** (more complex, better)
- Each player has own mission list
- Server tracks per-player mission state
- Mission NPCs visible only to mission owner
- Requires mission instancing system

**Tasks (Option B):**
- [ ] Create mission instance system
- [ ] Add per-player NPC visibility
- [ ] Implement mission state replication
- [ ] Handle mission-triggered universe changes
- [ ] Sync mission completion/failure

**New Files:**
```
source/multiplayer/MissionSync.h/cpp      - Mission synchronization
source/multiplayer/MissionInstance.h/cpp  - Per-player missions
```

#### 4.2 Economy & Trade

**Current**: Single-player economy simulation
**Needed**: Server-authoritative shared economy

**Tasks:**
- [ ] Move economy simulation to server
- [ ] Sync commodity prices to all clients
- [ ] Handle trade transactions server-side
- [ ] Prevent trade exploits (duplicate purchases)
- [ ] Implement transaction validation

**Modified Files:**
```
source/Trade.h/cpp                        - Add network sync
source/Planet.h/cpp                       - Server-authoritative shops
```

#### 4.3 Account & Persistence

**Current**: Per-player save files
**Needed**: Server-side persistence with player accounts

**Tasks:**
- [ ] Create account system (username/password or tokens)
- [ ] Implement server-side save system
  - Save all player states
  - Save universe state
  - Periodic auto-save
- [ ] Add player progression tracking
- [ ] Implement offline mode (disconnect/reconnect)

**New Files:**
```
source/multiplayer/AccountManager.h/cpp   - Player accounts
source/multiplayer/ServerSaveGame.h/cpp   - Server-side saves
source/multiplayer/Persistence.h/cpp      - Data persistence
```

---

### PHASE 5: UI & USER EXPERIENCE (4-6 weeks)

#### 5.1 Lobby & Matchmaking

**Features:**
- Server browser (LAN/Internet)
- Server creation UI
- Player list in lobby
- Chat system
- Ready-up system
- Host controls

**Tasks:**
- [ ] Create LobbyPanel UI
- [ ] Implement server browser
  - LAN discovery (UDP broadcast)
  - Internet server list (optional master server)
- [ ] Add chat system (text messages)
- [ ] Create player status display
- [ ] Implement kick/ban system (host)

**New Files:**
```
source/ui/LobbyPanel.h/cpp                - Multiplayer lobby
source/ui/ServerBrowserPanel.h/cpp        - Server browser
source/ui/CreateServerPanel.h/cpp         - Server creation
source/multiplayer/ChatSystem.h/cpp       - In-game chat
source/multiplayer/ServerBrowser.h/cpp    - Server discovery
```

#### 5.2 In-Game Multiplayer UI

**Features:**
- Player list (names, ships, status)
- Colored ship markers (identify players)
- Chat overlay
- Connection quality indicators
- Minimap with player positions

**Tasks:**
- [ ] Create PlayerListPanel
- [ ] Add player ship markers (different colors per player)
- [ ] Implement chat overlay (press Enter to chat)
- [ ] Show ping/connection status
- [ ] Add player nameplates above ships

**New Files:**
```
source/ui/PlayerListPanel.h/cpp           - In-game player list
source/ui/ChatOverlay.h/cpp               - Chat HUD
source/ui/MultiplayerHUD.h/cpp            - MP-specific HUD elements
```

**Modified Files:**
```
source/Radar.h/cpp                        - Show player ships differently
source/Engine.cpp                         - Render player nameplates
```

#### 5.3 Multi-Camera Support

**Current**: Single camera follows flagship
**Needed**: Each client has own camera

**Tasks:**
- [ ] Decouple camera from Engine
- [ ] Create CameraController for each client
- [ ] Add camera smoothing for network jitter
- [ ] Implement camera constraints (don't show out-of-sync areas)

**New Files:**
```
source/CameraController.h/cpp             - Camera management
source/multiplayer/ClientCamera.h/cpp     - MP camera handling
```

---

### PHASE 6: TESTING & OPTIMIZATION (4-6 weeks)

#### 6.1 Network Testing Framework

**Tasks:**
- [ ] Create network simulator
  - Add artificial latency (50-200ms)
  - Add packet loss (1-5%)
  - Add jitter
- [ ] Implement automated multiplayer tests
- [ ] Create test scenarios:
  - 2 players, same system, combat
  - 4 players, trading and missions
  - High ship count stress test
  - Connection drop and reconnect
- [ ] Add network profiling tools

**New Files:**
```
tests/network/NetworkSimulator.h/cpp      - Network condition simulator
tests/network/MultiplayerTests.cpp        - Automated MP tests
```

#### 6.2 Performance Optimization

**Bottlenecks:**
- Bandwidth (state updates)
- CPU (server simulation)
- Synchronization overhead

**Tasks:**
- [ ] Profile network bandwidth usage
- [ ] Implement aggressive delta compression
- [ ] Add entity culling (don't sync far-away ships)
- [ ] Optimize packet packing
- [ ] Implement server-side multithreading
  - Separate physics and networking threads
  - Parallel NPC AI processing
- [ ] Add client-side multithreading
  - Receive network updates on separate thread
  - Separate rendering from network processing

#### 6.3 Security & Anti-Cheat

**Vulnerabilities:**
- Command injection (fake commands)
- State manipulation (modified clients)
- Duplication exploits

**Tasks:**
- [ ] Implement command validation
  - Check all commands are physically possible
  - Verify timestamps
- [ ] Add server-side sanity checks
  - Impossible speeds
  - Invalid positions
  - Resource hacks
- [ ] Implement checksums for critical data
- [ ] Add rate limiting (prevent command spam)
- [ ] Create admin tools for detecting cheaters

---

### PHASE 7: POLISH & RELEASE PREP (2-4 weeks)

#### 7.1 Documentation

**Tasks:**
- [ ] Write multiplayer user guide
- [ ] Create server hosting tutorial
- [ ] Document network protocol
- [ ] Add code documentation for networking
- [ ] Create troubleshooting guide

#### 7.2 Configuration & Settings

**Tasks:**
- [ ] Add multiplayer settings panel
  - Network quality presets
  - Bandwidth limits
  - Connection timeouts
- [ ] Create server config file format
- [ ] Add command-line arguments for dedicated server
- [ ] Implement server RCON (remote console)

#### 7.3 Accessibility

**Tasks:**
- [ ] Add colorblind-friendly player markers
- [ ] Implement bandwidth optimization for slow connections
- [ ] Add reconnection grace period
- [ ] Create "spectator mode" for disconnected players

---

## IV. TECHNICAL SPECIFICATIONS

### A. Network Protocol Details

**Transport**: UDP with ENet (reliable + unreliable channels)

**Channels:**
```
Channel 0: Reliable Ordered - Connection, chat, mission updates
Channel 1: Unreliable Sequenced - Ship positions, commands
Channel 2: Reliable Unordered - Projectile spawns, effects
```

**Packet Header** (8 bytes):
```cpp
struct PacketHeader {
    uint8_t protocolVersion;      // 1 byte - Currently 1
    uint8_t packetType;           // 1 byte - PacketType enum
    uint16_t payloadSize;         // 2 bytes - Size of payload
    uint32_t sequenceNumber;      // 4 bytes - For ordering
};
```

**Bandwidth Budget** (per client):
- Upstream (client→server): ~5-10 KB/s (commands + acks)
- Downstream (server→client): ~50-100 KB/s (state updates)
- Target: Playable on 1 Mbps connection

### B. Server Architecture

**Server Loop** (60 FPS):
```cpp
while (running) {
    // 1. Receive client packets
    ProcessIncomingPackets();

    // 2. Apply validated commands to ships
    ApplyClientCommands();

    // 3. Run game simulation
    gameState.Step();

    // 4. Generate state snapshots
    CreateSnapshot();

    // 5. Send updates to clients (throttled to 20-30 Hz)
    if (shouldBroadcast) {
        BroadcastStateToClients();
    }

    // 6. Frame rate limiting
    WaitForNextFrame();
}
```

**State Snapshot Contents**:
- All ships in interest range (positions, velocities, facings)
- Energy/shields/hull levels
- Active projectiles
- Recent effects
- Delta from previous snapshot

### C. Client Architecture

**Client Loop** (60 FPS):
```cpp
while (running) {
    // 1. Process input
    HandleInput();

    // 2. Send commands to server
    SendCommandsToServer();

    // 3. Receive server packets
    ProcessServerPackets();

    // 4. Predict local player
    PredictLocalPlayer();

    // 5. Interpolate remote entities
    InterpolateRemoteEntities();

    // 6. Render
    RenderGame();

    // 7. Frame rate limiting
    WaitForNextFrame();
}
```

**Client-Side Prediction**:
1. Client applies command locally immediately (feels responsive)
2. Client sends command to server
3. Server processes command and sends back authoritative state
4. Client reconciles: if predicted state differs, smoothly correct

**Entity Interpolation**:
- Buffer 2-3 server snapshots (100-150ms of data)
- Interpolate between them for smooth movement
- Trade latency for smoothness

---

## V. FILE STRUCTURE

### New Directory Layout
```
source/
├── network/              # Core networking
│   ├── NetworkManager.h/cpp
│   ├── NetworkClient.h/cpp
│   ├── NetworkServer.h/cpp
│   ├── NetworkConnection.h/cpp
│   ├── PacketWriter.h/cpp
│   ├── PacketReader.h/cpp
│   ├── Packet.h
│   └── NetworkConstants.h
│
├── multiplayer/          # Multiplayer game logic
│   ├── PlayerManager.h/cpp
│   ├── NetworkPlayer.h/cpp
│   ├── CommandBuffer.h/cpp
│   ├── StateSync.h/cpp
│   ├── MissionSync.h/cpp
│   ├── Predictor.h/cpp
│   ├── ChatSystem.h/cpp
│   └── ServerBrowser.h/cpp
│
├── server/               # Dedicated server
│   ├── ServerMain.cpp
│   ├── Server.h/cpp
│   ├── ServerLoop.h/cpp
│   ├── SnapshotManager.h/cpp
│   ├── AccountManager.h/cpp
│   └── ServerConfig.h/cpp
│
├── client/               # Multiplayer client
│   ├── MultiplayerClient.h/cpp
│   ├── EntityInterpolator.h/cpp
│   ├── ClientReconciliation.h/cpp
│   └── ConnectionMonitor.h/cpp
│
└── ui/                   # UI additions
    ├── LobbyPanel.h/cpp
    ├── ServerBrowserPanel.h/cpp
    ├── PlayerListPanel.h/cpp
    └── ChatOverlay.h/cpp
```

---

## VI. BUILD SYSTEM CHANGES

### CMakeLists.txt Additions

```cmake
# Add ENet dependency
find_package(enet CONFIG REQUIRED)
target_link_libraries(EndlessSky PRIVATE enet::enet)

# Add networking source files
target_sources(EndlessSkyLib PRIVATE
    source/network/NetworkManager.cpp
    source/network/PacketWriter.cpp
    # ... all network files
)

# Create dedicated server executable
add_executable(EndlessSkyServer
    source/server/ServerMain.cpp
)
target_link_libraries(EndlessSkyServer PRIVATE
    EndlessSkyLib
    ExternalLibraries
    enet::enet
)

# Add MULTIPLAYER compile definition
target_compile_definitions(EndlessSkyLib PUBLIC ES_MULTIPLAYER)
```

### vcpkg.json Additions
```json
{
  "dependencies": [
    "enet",
    ...existing dependencies...
  ]
}
```

---

## VII. TESTING STRATEGY

### Unit Tests
- Packet serialization/deserialization
- Command validation
- State interpolation math
- Network buffer management

### Integration Tests
- Client-server connection
- State synchronization
- Prediction/reconciliation
- Mission synchronization

### Performance Tests
- 100+ ships in system (stress test)
- Network bandwidth usage
- Server CPU usage with 8 clients
- Memory leak detection

### Playtesting
- 2 players, normal gameplay
- 4 players, combat scenarios
- 8 players, maximum capacity
- High latency (200ms+) gameplay
- Packet loss (5%) gameplay

---

## VIII. RISKS & MITIGATION

### Technical Risks

**Risk**: Network jitter causes unplayable experience
**Mitigation**: Implement robust interpolation, increase buffer size

**Risk**: State synchronization bandwidth too high
**Mitigation**: Aggressive delta compression, interest management

**Risk**: Cheating/hacking
**Mitigation**: Server-authoritative design, validation, anti-cheat measures

**Risk**: Desync bugs (clients see different state)
**Mitigation**: Extensive testing, server reconciliation, debug tools

**Risk**: Save game compatibility breaks
**Mitigation**: Version save format, migration system

### Gameplay Risks

**Risk**: Multiplayer fundamentally incompatible with single-player balance
**Mitigation**: Separate balance settings, optional gameplay modifiers

**Risk**: Mission system too complex for multiplayer
**Mitigation**: Start with shared missions, iterate based on feedback

**Risk**: Player griefing/trolling
**Mitigation**: Vote-kick system, private servers, admin tools

---

## IX. ALTERNATIVES CONSIDERED

### Approach 1: Drop-in Co-op (Lockstep)
**Pros**: Simpler, less bandwidth
**Cons**: Requires determinism, fragile, no dedicated servers
**Decision**: Rejected - too many non-deterministic elements

### Approach 2: Async Multiplayer (Turn-based)
**Pros**: Very simple, no real-time sync
**Cons**: Fundamentally changes gameplay
**Decision**: Rejected - doesn't fit ES gameplay

### Approach 3: Shared Universe (MMO-style)
**Pros**: Persistent world, many players
**Cons**: Massive scope, server infrastructure
**Decision**: Future consideration, not initial implementation

---

## X. SUCCESS CRITERIA

### Minimum Viable Product (MVP)
- [ ] 2-4 players can connect to dedicated server
- [ ] All players can fly ships in same system
- [ ] Combat works (weapons, damage, destruction)
- [ ] Trading and outfitting work
- [ ] Shared missions work
- [ ] Game runs at 60 FPS with 4 players
- [ ] Playable with <200ms latency

### Full Release Goals
- [ ] 8 player support
- [ ] Personal missions
- [ ] Server browser
- [ ] Reconnection support
- [ ] Admin tools
- [ ] Comprehensive anti-cheat
- [ ] Complete documentation

---

## XI. IMPLEMENTATION PRIORITY

### Must Have (MVP)
1. Network layer (ENet integration)
2. Binary serialization
3. Client-server architecture
4. Basic state synchronization (ships, projectiles)
5. Lobby and connection UI

### Should Have
6. Mission synchronization (shared)
7. Economy synchronization
8. Chat system
9. Server browser
10. Admin tools

### Nice to Have
11. Personal missions (instanced)
12. Spectator mode
13. Replay system
14. Server statistics/monitoring
15. Master server (internet matchmaking)

---

## XII. MAINTENANCE & FUTURE

### Post-Launch Support
- Bug fixes and stability improvements
- Performance optimization based on telemetry
- Balance adjustments for multiplayer
- Anti-cheat updates
- Community server support

### Future Enhancements
- Larger player counts (16+)
- Persistent universe servers
- PvP arena mode
- Fleet battles (multiple ships per player)
- Cross-platform play
- Mobile/web client support

---

## XIII. CONCLUSION

Adding multiplayer to Endless Sky is a **substantial undertaking** requiring:
- **8-12 months** of focused development
- **Significant architectural refactoring**
- **New networking infrastructure**
- **Extensive testing and balancing**

However, the codebase has **strong foundations**:
- ✓ Clean class separation
- ✓ Existing UUID system for ships
- ✓ Robust serialization framework
- ✓ Well-defined command system
- ✓ Double-buffered architecture ready for threading

**Recommended Approach**: Incremental implementation following the phases above, with frequent playtesting and iteration.

**Key Success Factor**: Server-authoritative architecture with client prediction provides the best balance of responsiveness, security, and scalability.

---

## APPENDIX A: RESOURCE ESTIMATES

### Development Team
- **Lead Network Engineer**: 1 FTE
- **Gameplay Programmer**: 1 FTE
- **UI/UX Developer**: 0.5 FTE
- **QA Tester**: 0.5 FTE (ramp up to 1 FTE for testing phases)

### Timeline by Phase
- Phase 1 (Network Foundation): Weeks 1-8
- Phase 2 (Engine Modifications): Weeks 9-18
- Phase 3 (State Sync): Weeks 19-26
- Phase 4 (Mission/Economy): Weeks 27-32
- Phase 5 (UI): Weeks 33-38
- Phase 6 (Testing): Weeks 39-44
- Phase 7 (Polish): Weeks 45-48

**Total**: ~48 weeks (12 months)

---

## APPENDIX B: CODE EXAMPLES

### Example: Ship State Packet Serialization

```cpp
// source/network/PacketWriter.cpp
void PacketWriter::WriteShipState(const Ship &ship) {
    WriteUint8(PacketType::SERVER_SHIP_UPDATE);
    WriteUUID(ship.UUID());
    WritePoint(ship.Position());
    WritePoint(ship.Velocity());
    WriteAngle(ship.Facing());

    // Normalize to 0-1 range for efficient packing
    WriteFloat(ship.Shields());
    WriteFloat(ship.Hull());
    WriteFloat(ship.Energy());
    WriteFloat(ship.Fuel());

    // Pack flags into 16 bits
    uint16_t flags = 0;
    if (ship.IsThrusting()) flags |= 0x0001;
    if (ship.IsReversing()) flags |= 0x0002;
    if (ship.IsCloaked()) flags |= 0x0004;
    if (ship.IsDisabled()) flags |= 0x0008;
    // ... more flags
    WriteUint16(flags);
}
```

### Example: Client-Side Prediction

```cpp
// source/client/ClientReconciliation.cpp
void ClientReconciliation::ApplyServerUpdate(const ShipStatePacket &update) {
    auto ship = GetLocalShip();

    // Find the client prediction for this server frame
    auto prediction = predictionHistory.find(update.sequenceNumber);

    if (prediction != predictionHistory.end()) {
        // Check if prediction was correct
        Point positionError = update.position - prediction->position;

        if (positionError.Length() > RECONCILIATION_THRESHOLD) {
            // Significant error, snap to server position
            ship->SetPosition(update.position);
            ship->SetVelocity(update.velocity);

            // Re-apply all commands since this frame
            for (auto cmd : GetCommandsSince(update.sequenceNumber)) {
                ship->ApplyCommand(cmd);
                ship->Move();
            }
        }
    }

    // Clear old predictions
    predictionHistory.erase_before(update.sequenceNumber);
}
```

---

**Document Version**: 1.0
**Last Updated**: 2025-12-18
**Author**: Claude (Anthropic AI)
**Status**: Initial Planning Phase
