---
date: 2025-12-03T21:53:33Z
researcher: Claude Code
git_commit: 9d4f6f96c9d2e659c68580e96e5669bdea75c635
branch: master
repository: endless-sky
topic: "Feature Feasibility Analysis: Fleet Logistics, Trade Route Planner, Flight Recorder, Save System"
tags: [research, codebase, ai, map-ui, trade, save-system, player-info, feature-analysis, plugin-system, mission-scripting, implementation-ideas]
status: complete
last_updated: 2025-12-03
last_updated_by: Claude Code
last_updated_note: "Added plugin system research, mission scripting analysis, and 7 implementation proposals"
---

# Research: Feature Feasibility Analysis for Endless Sky

**Date**: 2025-12-03T21:53:33Z
**Researcher**: Claude Code
**Git Commit**: 9d4f6f96c9d2e659c68580e96e5669bdea75c635
**Branch**: master
**Repository**: endless-sky

## Research Question

Analyze the Endless Sky codebase to understand the systems relevant to four potential features:
1. **Fleet Logistics Commander** - Harvest/Mining fleet command
2. **Market Analyst Overlay** - Trade route planner with profit pathfinding
3. **Black Box Recorder** - Flight recorder and financial reporting
4. **Save System Architecture** - Understanding serialization patterns for any persistent feature

---

## Summary

The Endless Sky codebase is well-architected for extensibility. Each proposed feature has existing infrastructure to build upon:

| Feature | Key Extension Points | Complexity |
|---------|---------------------|------------|
| Fleet Mining Command | `Orders` enum, `DoMining()`, existing HARVEST command | Medium - patterns exist |
| Trade Route Planner | `DistanceMap` (Dijkstra), `MapPanel` overlay modes | Medium-High - new algorithm |
| Flight Recorder | `Account` class, `ConditionsStore`, logbook system | Medium - data plumbing |
| Save System | `DataWriter`/`DataNode` patterns | Foundation for all features |

---

## Detailed Findings

### 1. Fleet Logistics Commander (AI & State Machines)

#### Current Fleet Order System

The game already has a sophisticated order system in `source/orders/Orders.h:33-52`:

```cpp
enum class Types {
    HOLD_POSITION,
    HOLD_ACTIVE,
    MOVE_TO,
    KEEP_STATION,
    GATHER,
    ATTACK,
    FINISH_OFF,
    HOLD_FIRE,
    MINE,        // <-- ALREADY EXISTS
    HARVEST,     // <-- ALREADY EXISTS
    TYPES_COUNT
}
```

**Key Discovery**: `MINE` and `HARVEST` orders already exist in the enum!

#### Existing Mining Implementation

The `DoMining()` function at `source/AI.cpp:3185-3233` already implements NPC mining:

- Ships with `IsMining()` personality autonomously mine asteroids
- Uses state tracking via `miningAngle` and `miningRadius` maps
- Targets asteroids within 800 units at 45° of facing
- Calls `MoveToAttack()` to approach and `AutoFire()` to shoot

**Current Limitations** (documented behavior, not suggestions):
- Limited to 9 concurrent miners in system (`source/AI.cpp:1004-1037`)
- NPC mining has time limits from gamerules
- Requires `IsMining()` personality flag

#### Command System Architecture

Commands are bitmasks in `source/Command.h:30-157`:

```cpp
// Existing fleet commands (bits 30-35)
FIGHT           // Bit 30 - Fleet: Fight my target
HOLD_FIRE       // Bit 31 - Fleet: Toggle hold fire
GATHER          // Bit 32 - Fleet: Gather around me
HOLD_POSITION   // Bit 33 - Fleet: Hold position
HARVEST         // Bit 34 - Fleet: Harvest flotsam  <-- EXISTS
AMMO            // Bit 35 - Fleet: Toggle ammo usage
```

**HARVEST command already exists** at bit 34, currently for flotsam pickup.

#### Order Execution Flow

Orders are processed in `AI::FollowOrders()` at `source/AI.cpp:1714-1810`:

```cpp
case MINE:     // Lines 1775-1780
    if(target = orders.GetTarget().targetAsteroid.lock())
        return MoveToAttack(ship, command, *target);
    break;

case HARVEST:  // Lines 1781-1790
    DoHarvesting(ship, command);
    return nullptr;
```

#### Key Files for Fleet Commands

| File | Purpose | Key Lines |
|------|---------|-----------|
| `source/AI.h` | AI class declaration | 197-260 (members) |
| `source/AI.cpp` | AI::Step() main loop | 677-1233 |
| `source/AI.cpp` | DoMining() | 3185-3233 |
| `source/AI.cpp` | FollowOrders() | 1714-1810 |
| `source/orders/Orders.h` | Order types enum | 33-52 |
| `source/orders/OrderSet.h` | Order management | 25-46 |
| `source/Command.h` | Command bitmask | 30-157 |
| `source/Personality.h` | Personality traits | 40-86 |

---

### 2. Market Analyst Overlay (UI & Algorithms)

#### Existing Pathfinding: DistanceMap

The game uses Dijkstra's algorithm (not A*) in `source/DistanceMap.cpp:151-258`:

**Current optimization criteria** (`source/RouteEdge.cpp:30-39`):
1. Fuel consumption (primary)
2. Number of jumps/days (secondary)
3. Accumulated danger (tertiary)

The algorithm already supports weighted edges - **profit could be added as a weight**.

#### Route Calculation Flow

```
User clicks system → MapPanel::Select() (708)
    → Creates DistanceMap(player) (233)
    → DistanceMap::Init() runs Dijkstra (151)
    → Priority queue processes by fuel/days/danger
    → distance.Plan(system) extracts route (131)
    → Stored in player.TravelPlan()
    → DrawTravelPlan() renders path (1077)
```

#### Map Overlay System

MapPanel has an overlay mode system at `source/MapPanel.h:46-54`:

```cpp
// Negative values = special modes
SHOW_SHIPYARD    = -1   // Colors by shipyard size
SHOW_OUTFITTER   = -2   // Colors by outfitter size
SHOW_VISITED     = -3   // Planet visitation status
SHOW_SPECIAL     = -4   // Mission highlighting
SHOW_GOVERNMENT  = -5   // Government colors
SHOW_REPUTATION  = -6   // Player reputation
SHOW_DANGER      = -7   // System danger level
SHOW_STARS       = -8   // Stellar objects

// Positive values = commodity index for trade prices
0, 1, 2, ...        // Index into GameData::Commodities()
```

**Trade price coloring already exists** for positive commodity indices.

#### Trade Data Access

Per-system commodity prices in `source/System.cpp:951-955`:

```cpp
int System::Trade(const string &commodity) const
{
    auto it = trade.find(commodity);
    if(it == trade.end())
        return 0;
    return it->second.price;
}
```

Global commodity list: `GameData::Commodities()` returns `vector<Trade::Commodity>`.

#### Price Normalization (for display)

`source/MapPanel.cpp:944-952`:
```cpp
const Trade::Commodity &com = GameData::Commodities()[commodity];
int price = system.Trade(com.name);
// Normalize to [-1, 1] range
double value = (2. * (price - low)) / (high - low) - 1.;
```

#### Rendering Infrastructure

| Shader | File | Purpose |
|--------|------|---------|
| LineShader | `source/shader/LineShader.h:25` | Lines (solid and dashed) |
| RingShader | `source/shader/RingShader.h:25` | Circles/rings |
| PointerShader | `source/shader/PointerShader.h:24` | Triangular arrows |
| SpriteShader | `source/shader/SpriteShader.h:32` | Textures |

Route drawing with color coding: `source/MapPanel.cpp:1077-1159`
- Green: All ships can make it
- Yellow: Only flagship can make it
- Red: Flagship cannot make it

#### Key Files for Trade Route Planner

| File | Purpose | Key Lines |
|------|---------|-----------|
| `source/MapPanel.h` | Map overlay modes | 46-54 |
| `source/MapPanel.cpp` | Cache & rendering | 884-1073 |
| `source/MapDetailPanel.h` | Trade detail panel | 40 |
| `source/DistanceMap.h` | Dijkstra interface | 38 |
| `source/DistanceMap.cpp` | Pathfinding impl | 151-258 |
| `source/RouteEdge.cpp` | Edge comparison | 30-39 |
| `source/Trade.h` | Commodity definitions | 29-37 |
| `source/System.cpp` | Price access | 951-955 |

---

### 3. Black Box Recorder (Data & Systems)

#### Existing Financial Tracking

The `Account` class at `source/Account.h:80-94` already tracks:

```cpp
int64_t credits;                           // Current balance
map<string, int64_t> salariesIncome;       // Regular income by source
int64_t crewSalariesOwed;                  // Unpaid salaries
int64_t maintenanceDue;                    // Unpaid maintenance
int creditScore;                           // Credit rating (200-800)
vector<Mortgage> mortgages;                // All debts
vector<int64_t> history;                   // Net worth (last 100 days!)
```

**Net worth history already exists** - last 100 days stored in `history` vector.

#### Daily Accounting Step

`Account::Step()` at `source/Account.cpp:157-285` processes daily:
- Accumulates salaries and maintenance
- Pays crew salaries (or accrues debt)
- Pays maintenance (or accrues debt)
- Processes mortgage payments
- **Updates 100-day net worth history** (lines 248-251)
- Adjusts credit score

#### Derived Conditions (Read-Only Statistics)

`PlayerInfo::RegisterDerivedConditions()` at `source/PlayerInfo.cpp:3607-3800` exposes:

```cpp
// Financial conditions (lines 3642-3661)
"net worth"         // Total assets - liabilities
"credits"           // Current balance
"unpaid mortgages"  // Mortgage debt
"unpaid fines"      // Fine debt
"unpaid debts"      // Other debt
"unpaid salaries"   // Overdue salaries
"unpaid maintenance"// Overdue maintenance
"credit score"      // Rating 200-800

// Combat conditions
"combat rating"     // Experience from disabling ships
```

#### Transaction Events (Where Money Changes Hands)

| Event | Location | Notes |
|-------|----------|-------|
| Mission rewards | `source/GameAction.cpp:400+` | Via `player.Accounts().AddCredits()` |
| Trading | `source/TradingPanel.h:57-59` | Tracks tons sold, profit |
| Plunder | `source/BoardingPanel.h:37-148` | Cargo/outfit transfer |
| Tributes | `source/PlayerInfo.h:530` | `tributeReceived` map |
| Bribes | `source/GameAction.cpp` | Via payment system |
| Fines | `source/Account.cpp:193-200` | Must be positive |

#### Existing Logging System

**Date-based logbook** (`source/PlayerInfo.h:472`):
```cpp
std::map<Date, BookEntry> logbook;
```

**Category-based logs** (`source/PlayerInfo.h:473`):
```cpp
std::map<string, map<string, BookEntry>> specialLogs;
```

**Adding entries** (`source/GameAction.cpp:152-169`):
```cpp
log [<message>|scene <sprite>]           // Normal log
log <category> <heading> [<message>...]  // Special log
```

#### PlayerInfoPanel Display

`source/PlayerInfoPanel.cpp:618-708` displays:
- Net worth (line 638)
- Combat rating with level/rank (lines 643-664)
- Piracy threat calculation (lines 667-689)
- Salary income breakdown (lines 691-695)
- Tribute income breakdown (lines 697-701)

#### Key Files for Flight Recorder

| File | Purpose | Key Lines |
|------|---------|-----------|
| `source/Account.h` | Financial data structure | 80-94 |
| `source/Account.cpp` | Daily step, history | 157-285 |
| `source/PlayerInfo.h` | Player state container | 444-559 |
| `source/PlayerInfo.cpp` | Derived conditions | 3607-3800 |
| `source/PlayerInfoPanel.cpp` | Info display | 618-708 |
| `source/GameAction.cpp` | Transaction processing | 400+ |
| `source/LogbookPanel.cpp` | Log display | 101-158 |
| `source/BookEntry.h` | Log entry structure | 33-58 |

---

### 4. Save System Architecture

#### Format Characteristics

- **Text-based**: Human-readable `.txt` files
- **Indentation hierarchy**: Tab characters define parent-child
- **Token-based**: Whitespace-separated values
- **Floating precision**: 8 decimal places (`source/DataWriter.cpp:44`)

#### Core Classes

**DataWriter** (`source/DataWriter.h:34`) - Serialization:
```cpp
Write(const A &a, B... others)  // Variadic token writing
BeginChild() / EndChild()       // Indentation control
WriteToken(string)              // Auto-quoting
SaveToPath() / SaveToString()   // Persistence
```

**DataNode** (`source/DataNode.h:31`) - Deserialization:
```cpp
Token(int index)      // Get token as string
Value(int index)      // Parse as double
BoolValue(int index)  // Parse as bool
HasChildren()         // Check for children
begin() / end()       // Iterate children
PrintTrace(message)   // Error with line numbers
```

**DataFile** (`source/DataFile.h:33`) - File loading:
- Parses indentation-based hierarchy
- Handles quoted strings with spaces
- UTF-8 support, BOM handling

#### Save File Location

Platform-specific paths (`source/Files.cpp:153-169`):
- **Windows**: `%APPDATA%/endless-sky/saves/`
- **macOS**: `~/Library/Application Support/endless-sky/saves/`
- **Linux**: `~/.local/share/endless-sky/saves/`

#### Save File Structure

`PlayerInfo::Save()` at `source/PlayerInfo.cpp:4541-4889`:

```
pilot <firstname> <lastname>
date <day> <month> <year>
system <name>
planet <name>
playtime <seconds>

reputation with
    <government> <value>

tribute received
    <planet> <amount>

ship <model>
    name <name>
    uuid <uuid>
    outfits
        <outfit> <count>
    cargo
        commodities
            <commodity> <tons>

account
    credits <amount>
    history
        <net_worth_day_1>
        <net_worth_day_2>
        ...

conditions
    <condition_name> [value]

logbook
    <day> <month> <year>
        <entry>
```

#### Extension Pattern

To add new saved data:

```cpp
// 1. In Save() method:
out.Write("my new data");
out.BeginChild();
{
    out.Write("field1", value1);
    out.Write("field2", value2);
}
out.EndChild();

// 2. In Load() method:
for(const DataNode &child : node)
{
    if(child.Token(0) == "field1" && child.Size() >= 2)
        value1 = child.Value(1);
    else if(child.Token(0) == "field2" && child.Size() >= 2)
        value2 = child.Token(1);
}
```

#### Key Files for Save System

| File | Purpose | Key Lines |
|------|---------|-----------|
| `source/DataWriter.h` | Serialization interface | 34 |
| `source/DataWriter.cpp` | Write implementation | 32-166 |
| `source/DataNode.h` | Node structure | 31 |
| `source/DataNode.cpp` | Parsing/access | 107-184 |
| `source/DataFile.cpp` | File loading | 100-240 |
| `source/PlayerInfo.cpp` | Player save/load | 268-539, 4541-4889 |
| `source/Files.cpp` | Path management | 97-183 |

---

## Code References

### AI & Commands
- `source/AI.h:197-260` - AI class members
- `source/AI.cpp:677-1233` - AI::Step() main loop
- `source/AI.cpp:3185-3233` - DoMining() implementation
- `source/AI.cpp:1714-1810` - FollowOrders() dispatch
- `source/orders/Orders.h:33-52` - Order types enum
- `source/Command.h:30-157` - Command bitmask

### Map & Trade
- `source/MapPanel.h:46-54` - Overlay mode constants
- `source/MapPanel.cpp:884-1073` - UpdateCache() price coloring
- `source/DistanceMap.cpp:151-258` - Dijkstra implementation
- `source/RouteEdge.cpp:30-39` - Edge priority comparison
- `source/System.cpp:951-955` - Trade() price lookup

### Player & Accounting
- `source/Account.h:80-94` - Financial data members
- `source/Account.cpp:157-285` - Daily Step() processing
- `source/Account.cpp:248-251` - Net worth history update
- `source/PlayerInfo.cpp:3642-3661` - Financial derived conditions
- `source/PlayerInfoPanel.cpp:618-708` - Info display

### Serialization
- `source/DataWriter.cpp:131-147` - Token writing
- `source/DataFile.cpp:100-240` - Parsing algorithm
- `source/PlayerInfo.cpp:4541-4889` - Full save structure

---

## Architecture Documentation

### AI Decision Flow

```
AI::Step() (per frame)
    ├── UpdateStrengths() - Government power
    ├── CacheShipLists() - Ship lookup tables
    └── For each ship:
        ├── MovePlayer() if flagship
        ├── AskForHelp() if disabled
        ├── DoCloak() - Retreat decision
        ├── FindTarget() - Target selection
        ├── AimTurrets() / AutoFire()
        ├── ShouldFlee() check
        ├── Personality behaviors:
        │   ├── DoSwarming()
        │   ├── DoSurveillance()
        │   ├── DoHarvesting()
        │   └── DoMining()
        └── Movement:
            ├── FollowOrders() if has orders
            ├── MoveIndependent() if no parent
            └── MoveEscort() if has parent
```

### Map Rendering Pipeline

```
MapPanel::Draw()
    ├── glClear()
    ├── Galaxy background sprites
    ├── FogShader::Draw() if enabled
    ├── RingShader::Draw() view/jump range
    ├── DrawWormholes()
    ├── DrawTravelPlan() - Route lines
    ├── DrawEscorts()
    ├── DrawLinks() - Hyperspace connections
    ├── DrawSystems() - System nodes with colors
    ├── DrawNames()
    └── DrawMissions() - Pointers
```

### Save/Load Cycle

```
Save:
    PlayerInfo::Save()
        ├── Backup rotation
        ├── DataWriter creation
        ├── Section-by-section Write() calls
        └── Destructor auto-saves to file

Load:
    DataFile::Load(path)
        ├── Read file to string
        ├── Parse indentation hierarchy
        └── Build DataNode tree
    PlayerInfo::Load(DataNode)
        ├── Iterate child nodes
        ├── Token(0) switch dispatch
        └── Populate member variables
```

---

## Related Research

- None yet (first research document)

---

## Follow-up Research: Plugin System & Implementation Proposals

### 5. Plugin System Architecture

#### Plugin Discovery and Loading

Plugins are discovered at runtime from two locations (`source/GameData.cpp:1015-1038`):
- **Global plugins**: `<resources>/plugins/` (ships with game)
- **User plugins**: `<config>/plugins/` (user-installed)

**Platform-specific config paths** (`source/Files.cpp:148-182`):
- **Windows**: `%APPDATA%/endless-sky/plugins/`
- **macOS**: `~/Library/Application Support/endless-sky/plugins/`
- **Linux**: `~/.local/share/endless-sky/plugins/`

#### Plugin Validation (`source/Plugins.cpp:323-329`)

Valid plugins must contain at least one of:
- `data/` - Game object definitions (`.txt` files)
- `images/` - Sprite assets
- `sounds/` - Audio files
- `shaders/` - Custom OpenGL shaders

#### Plugin Structure

```
my-plugin/
├── plugin.txt          # Optional metadata
├── icon.png            # Optional icon for UI
├── data/
│   ├── ships.txt
│   ├── missions.txt
│   └── events.txt
├── images/
│   └── ship/
│       └── my ship.png
└── sounds/
    └── ambient/
        └── my sound.wav
```

#### Plugin Metadata (`plugin.txt`)

Parsed at `source/Plugins.cpp:218-256`:
```
name "My Plugin"
about "Description of the plugin"
version "1.0.0"
authors
    "Author Name"
tags
    "gameplay"
    "story"
dependencies
    game version "0.10.0"
    requires "Other Plugin"
    conflicts "Incompatible Plugin"
```

#### Load Order and Override System

Sources load in order (`source/GameData.cpp:1015-1038`):
1. Base game resources
2. Global plugins (alphabetical, unzipped before zipped)
3. User plugins (alphabetical, unzipped before zipped)

**Later sources override earlier ones** - plugins can completely replace base game objects by using the same name.

#### What Plugins CAN Modify (Data-Driven)

All objects in `source/UniverseObjects.cpp:339-531`:

| Object Type | Description |
|-------------|-------------|
| `ship` | Ship definitions with stats, outfits, sprites |
| `outfit` | Weapons, engines, systems |
| `mission` | Quest definitions with triggers, NPCs, rewards |
| `event` | Scheduled universe changes |
| `conversation` | Dialog trees with branching |
| `system` | Star systems with planets, fleets, hazards |
| `planet` | Planets/stations with services, descriptions |
| `government` | Factions with relationships, colors |
| `fleet` | NPC ship group templates |
| `effect` | Visual effects for weapons/explosions |
| `hazard` | Environmental dangers |
| `start` | Starting scenarios |
| `interface` | UI layout definitions |
| `phrase` | Random text generation |
| `news` | Spaceport news articles |
| `gamerules` | Global rule modifications |

#### What Plugins CANNOT Modify (Requires C++)

- Physics simulation (movement, collision)
- Rendering pipeline (OpenGL calls)
- AI decision-making algorithms
- Save/load serialization format
- Input handling framework
- UI panel architecture
- Ship attribute calculations
- Weapon damage processing
- New object types beyond those listed above
- New mission trigger types
- New condition operators

---

### 6. Mission Scripting System

#### Mission Structure (`source/Mission.h:53-309`)

```
mission "Example Mission"
    name "Display Name"
    description "What the player sees"

    # Where mission appears
    landing              # On planet landing
    job                  # Job board
    boarding             # When boarding ships
    assisting            # When helping disabled ships

    # Requirements
    to offer
        has "prerequisite mission: done"
        "reputation: Republic" > 0
        random < 50      # 50% chance

    # Locations
    source
        government "Republic"
        attributes "spaceport"
    destination
        distance 2 5
        attributes "urban"

    # Cargo/passengers
    cargo "Equipment" 10 20    # 10-20 tons
    passengers 5 10            # 5-10 passengers

    # Time limit
    deadline 10                # 10 days

    # NPCs
    npc save                   # Must survive
        government "Merchant"
        personality escort
        ship "Freighter"

    npc kill                   # Must destroy
        government "Pirate"
        personality nemesis
        fleet "Small Pirates"

    # Triggers
    on offer
        conversation "intro dialog"
    on accept
        log "Started the mission"
    on complete
        payment 50000
        "reputation: Republic" += 5
        event "consequence event" 30   # 30 days later
```

#### Condition System (`source/ConditionSet.h:33-181`)

**Operators available**:
- Arithmetic: `+`, `-`, `*`, `/`, `%`
- Comparison: `==`, `!=`, `<`, `>`, `<=`, `>=`
- Boolean: `and`, `or`, `not`, `has`
- Assignment: `=`, `+=`, `-=`, `*=`, `/=`, `<?=` (min), `>?=` (max)

**Built-in conditions** (from `PlayerInfo::RegisterDerivedConditions()`):
- `"credits"`, `"net worth"`, `"combat rating"`
- `"ships: <category>"` - count of ships by category
- `"cargo space"`, `"passenger space"`
- `"reputation: <government>"`
- `"visited <system>"`, `"visited planet <planet>"`

#### Conversation System (`source/Conversation.h:38-199`)

```
conversation "example"
    scene "ship/example"        # Background image
    `Opening narrative text.`

    choice
        `Option A`
            goto option_a
        `Option B`
            goto option_b

    label option_a
        `You chose option A.`
        accept                  # Accept mission

    label option_b
        branch
            has "some condition"
            `Conditional text.`
        `Default text.`
        decline                 # Decline mission
```

**Conversation outcomes** (`source/Conversation.h:40-53`):
- `accept` - Accept mission
- `decline` - Decline mission
- `defer` - Defer decision
- `launch` - Accept and force takeoff
- `flee` - Decline and force takeoff
- `die` - Player dies

#### Event System (`source/GameEvent.h:43-98`)

```
event "war begins"
    date 4 7 3014              # Specific date trigger

    # Modify systems
    system "Alpha"
        government "Enemy"
        fleet "Enemy Fleet" 1000

    # Modify planets
    planet "Station X"
        description `New description after event.`
        shipyard clear

    # Add/remove links
    link "System A" "System B"
    unlink "System C" "System D"
```

#### NPC Behavior Control (`source/NPC.h:51-179`, `source/Personality.h:40-88`)

**Personality flags for NPCs**:
- `pacifist` - Won't attack
- `timid` - Flees from combat
- `heroic` - Engages stronger enemies
- `nemesis` - Focuses on player
- `escort` - Follows player
- `staying` - Remains in system
- `waiting` - Waits for player arrival
- `mining` - Mines asteroids
- `harvests` - Picks up flotsam
- `disables` - Disables instead of destroys
- `plunders` - Boards and loots

**NPC spawn conditions**:
```
npc
    to spawn
        "mission progress" >= 5
    to despawn
        "mission complete"
    government "Ally"
    personality escort staying
    ship "Fighter"
```

---

### 7. Implementation Proposals

Based on the codebase analysis, here are concrete implementation approaches ranked by feasibility:

---

#### Proposal 1: "Fleet Admiral" - Smart Escort Mining (C++ Modification)

**Difficulty**: Medium | **Impact**: High | **Estimated Scope**: ~300 LOC

**The Discovery**: `Orders::Types::MINE` and `HARVEST` already exist but aren't player-accessible!

**Implementation Plan**:

1. **Add keyboard binding** (`source/MainPanel.cpp`):
   - Add 'M' key to issue MINE order to selected escorts
   - Similar pattern to existing GATHER ('G') and HOLD ('H') commands

2. **Implement `AI::IssueAsteroidTarget()`** (`source/AI.cpp`):
   ```cpp
   // Pattern from existing IssueShipTarget() at AI.cpp:477-502
   void AI::IssueAsteroidTarget(const Ship &flagship, const std::shared_ptr<Ship> &escort)
   {
       // Find nearest minable asteroid
       // Set Orders::Types::MINE with targetAsteroid
   }
   ```

3. **Modify order validation** (`source/orders/OrderSet.cpp`):
   - Ensure MINE orders are validated properly for player escorts
   - Check ship has weapons capable of mining

4. **Add HUD feedback** (`source/HUD.cpp`):
   - Display mining state icon for escorts
   - Show ore collection progress

**Key Files to Modify**:
| File | Changes |
|------|---------|
| `source/MainPanel.cpp` | Add 'M' key binding |
| `source/AI.cpp:470-502` | Add `IssueAsteroidTarget()` |
| `source/AI.cpp:1004-1037` | Adjust miner limits for player escorts |
| `source/HUD.cpp` | Add mining status display |
| `source/Preferences.cpp` | Add keybinding preference |

---

#### Proposal 2: "The Merchant Prince" - External Trade Advisor (External Tool + Plugin)

**Difficulty**: Low-Medium | **Impact**: High | **No C++ Required**

**Concept**: Python tool that reads save files and generates a plugin with trade advice missions.

**How It Works**:

1. **Parse save file** (Python):
   ```python
   # Save files are at ~/.local/share/endless-sky/saves/
   # Format is tab-indented text, easy to parse

   def parse_save(path):
       systems_visited = []
       current_system = None
       cargo_space = 0
       # ... parse DataNode-style format
   ```

2. **Analyze trade prices** (from `economy` section in save):
   ```python
   def find_best_route(systems, cargo_space):
       # Build graph of systems with trade prices
       # Run profit-weighted Dijkstra
       # Return optimal buy/sell route
   ```

3. **Generate plugin** with mission waypoints:
   ```
   # Generated: trade-advisor/data/advice.txt
   mission "Trade Advisor: Food Route"
       minor
       landing
       to offer
           has "trade advisor active"
       source "<current_system>"
       waypoint "<buy_system>"
       destination "<sell_system>"
       on offer
           conversation
               `Best route: Buy Food at <buy_system> for <buy_price>,
                sell at <sell_system> for <sell_price>.
                Estimated profit: <profit> credits.`
               accept
   ```

4. **Player workflow**:
   - Run tool: `python trade-advisor.py ~/.local/share/endless-sky/saves/MyPilot.txt`
   - Tool generates/updates plugin
   - Restart game, plugin shows trade missions on map

**Repository Structure**:
```
endless-sky-trade-advisor/
├── trade_advisor.py        # Main analysis script
├── parser.py               # Save file parser
├── pathfinding.py          # Profit-weighted Dijkstra
└── plugin_generator.py     # Generates mission files
```

---

#### Proposal 3: "The Accountant" - Transaction Ledger (C++ Modification)

**Difficulty**: Medium | **Impact**: Medium-High | **Estimated Scope**: ~500 LOC

**Concept**: Expand `Account::history` into a full categorized transaction log.

**Data Structure** (`source/Account.h`):

```cpp
// New: Add after line 94
struct Transaction {
    Date date;
    int64_t amount;
    enum class Type {
        TRADE, MISSION, PLUNDER, SALARY, TRIBUTE,
        MAINTENANCE, MORTGAGE, FINE, BRIBE, OTHER
    };
    Type type;
    std::string details;  // "Sold 50 Food at Vega"
};

// Rolling buffer of last N transactions
static constexpr size_t MAX_TRANSACTIONS = 500;
std::deque<Transaction> transactions;

// Aggregated stats (computed on load, updated on transaction)
std::map<Type, int64_t> lifetimeByType;
```

**Hook Points**:

| Location | Transaction Type | Code Pattern |
|----------|------------------|--------------|
| `TradingPanel.cpp` | TRADE | After `player.Accounts().AddCredits()` |
| `GameAction.cpp:400+` | MISSION | After payment processing |
| `BoardingPanel.cpp` | PLUNDER | After cargo/outfit transfer |
| `Account.cpp:167-186` | SALARY | In daily `Step()` |
| `Account.cpp:189-208` | MAINTENANCE | In daily `Step()` |
| `Account.cpp:212-238` | MORTGAGE | In daily `Step()` |

**UI Addition** (`source/PlayerInfoPanel.cpp`):

New section showing:
- Income breakdown by type (pie chart data)
- Top 5 most profitable systems
- Expense trends
- Link to detailed ledger view

**Save/Load** (`source/Account.cpp:84-118`):

```cpp
// In Save():
out.Write("transactions");
out.BeginChild();
for(const Transaction &t : transactions)
    out.Write(static_cast<int>(t.type), t.amount, t.date.ToString(), t.details);
out.EndChild();
```

---

#### Proposal 4: "The Diplomat" - Reputation Decay System (Pure Plugin)

**Difficulty**: Low | **Impact**: Medium | **No C++ Required**

**Concept**: Add reputation decay and recovery missions using only data files.

**Implementation** (`reputation-decay/data/events.txt`):

```
# Daily reputation decay event
event "daily reputation decay"
    # Pirate reputation decays toward 0
    "reputation: Pirate" *= 0.995

    # Merchant reputation decays slower
    "reputation: Merchant" *= 0.998

    # Schedule next decay
    event "daily reputation decay" 1
```

**Recovery Missions** (`reputation-decay/data/missions.txt`):

```
mission "Merchant Guild Apology"
    minor
    repeat
    job
    to offer
        "reputation: Merchant" < -100
        "reputation: Merchant" > -1000
        random < 15
    source
        government "Merchant"
        attributes "spaceport"
    cargo "Luxury Goods" 5 15
    destination
        government "Merchant"
        distance 2 4
    on complete
        "reputation: Merchant" += 50
        payment 8000
        dialog `The Merchant Guild acknowledges your delivery.
               "Perhaps we misjudged you," the factor admits.`
```

**Initialization** (player must accept once):
```
mission "Enable Reputation System"
    landing
    source "New Boston"
    to offer
        not "reputation system enabled"
    on accept
        set "reputation system enabled"
        event "daily reputation decay" 1
```

---

#### Proposal 5: "The Cartographer" - Custom Map Overlays (C++ Modification)

**Difficulty**: High | **Impact**: High | **Estimated Scope**: ~800 LOC

**Concept**: Add a pluggable overlay system to MapPanel that can display custom data layers defined in data files.

**New Data Format**:

```
# In plugin data file:
map overlay "Trade Profit"
    description "Shows potential profit per system"

    # Color formula using conditions
    color
        value "trade profit: Food"
        gradient
            -100 1 0 0    # Red for loss
            0 1 1 1       # White for neutral
            100 0 1 0     # Green for profit

    # Optional: Custom rendering
    highlight systems
        "trade profit: Food" > 50
```

**Implementation**:

1. **New class** `MapOverlay` (`source/MapOverlay.h`):
   ```cpp
   class MapOverlay {
   public:
       void Load(const DataNode &node);
       std::string Name() const;
       Color GetSystemColor(const System &system, const PlayerInfo &player) const;
       bool ShouldHighlight(const System &system, const PlayerInfo &player) const;
   private:
       std::string name;
       std::string description;
       // Color formula using ConditionSet
       // Highlight condition
   };
   ```

2. **Register overlays** in `UniverseObjects.cpp:339-531`:
   ```cpp
   else if(key == "map overlay")
       objects.mapOverlays.Get(child.Token(1))->Load(child);
   ```

3. **Extend MapPanel** (`source/MapPanel.cpp`):
   - Add negative indices for custom overlays
   - Call `overlay.GetSystemColor()` in `UpdateCache()`

4. **Compute derived conditions** for trade profit:
   - Add `"trade profit: <commodity>"` derived condition
   - Computes `destination_price - source_price` for current system

---

#### Proposal 6: "The Black Box" - Save File Reporter (External Tool)

**Difficulty**: Low | **Impact**: Medium | **No C++ Required**

**Concept**: Python tool that generates HTML reports from save files.

**Features**:
- Net worth graph over time (from `account.history`)
- Logbook timeline visualization
- Systems explored map (from `visited` entries)
- Fleet composition history
- Combat statistics

**Implementation**:

```python
# report_generator.py
import json
from pathlib import Path

def parse_save(save_path):
    """Parse Endless Sky save file into structured data."""
    data = {
        'pilot': {},
        'account': {'history': []},
        'logbook': [],
        'visited': [],
        'ships': [],
        'conditions': {}
    }
    # ... parse tab-indented format
    return data

def generate_report(data, output_path):
    """Generate HTML report with charts."""
    html = f"""
    <!DOCTYPE html>
    <html>
    <head>
        <title>Pilot Report: {data['pilot']['name']}</title>
        <script src="https://cdn.plot.ly/plotly-latest.min.js"></script>
    </head>
    <body>
        <h1>{data['pilot']['name']}</h1>
        <div id="networth-chart"></div>
        <script>
            var history = {json.dumps(data['account']['history'])};
            Plotly.newPlot('networth-chart', [{{
                y: history,
                type: 'scatter',
                name: 'Net Worth'
            }}]);
        </script>
        <!-- More charts... -->
    </body>
    </html>
    """
    Path(output_path).write_text(html)
```

**Could integrate with GitHub Pages** for shareable pilot histories.

---

#### Proposal 7: "The Storyteller" - Procedural Mission Generator (External Tool + Plugin)

**Difficulty**: Medium | **Impact**: High | **No C++ Required**

**Concept**: Tool that reads save file state and generates personalized mission chains.

**Mission Templates**:

```yaml
# templates/revenge.yaml
name: "Revenge for {system}"
trigger:
  condition: "destroyed pirates in {system}"
  min_reputation: -100
  government: Pirate
template: |
  mission "Revenge: {system}"
    to offer
      has "visited {system}"
      "reputation: Pirate" < -100
    source
      near "{system}" 1 3
      government "Republic"
    npc kill
      government "Pirate"
      system "{system}"
      personality nemesis waiting
      fleet "Small Pirates"
    on complete
      payment 25000
      "reputation: Republic" += 10
```

**Generator Logic**:

```python
def generate_missions(save_data, templates):
    missions = []

    for system in save_data['visited']:
        # Check if player has history with pirates here
        if has_pirate_kills(save_data, system):
            mission = templates['revenge'].format(system=system)
            missions.append(mission)

    # Generate escort missions for systems player frequents
    frequent_systems = get_frequent_systems(save_data)
    for system in frequent_systems:
        mission = templates['escort_contract'].format(
            source=system,
            destination=random_nearby_system(system)
        )
        missions.append(mission)

    return missions
```

---

### Recommended Implementation Order

For a developer new to the codebase:

| Order | Proposal | Rationale |
|-------|----------|-----------|
| 1 | **#4 Diplomat** (Plugin) | Zero risk, learn mission syntax |
| 2 | **#6 Black Box** (Tool) | Learn save format, no game changes |
| 3 | **#2 Merchant Prince** (Tool+Plugin) | Combine learnings from #4 and #6 |
| 4 | **#1 Fleet Admiral** (C++) | First C++ change, infrastructure exists |
| 5 | **#3 Accountant** (C++) | Deeper C++ with save system changes |
| 6 | **#7 Storyteller** (Tool+Plugin) | Advanced procedural generation |
| 7 | **#5 Cartographer** (C++) | Complex C++ with new data format |

---

## Open Questions

1. **Fleet Mining**: The MINE order exists but is only used for targeted asteroid attacks. How is it currently issued to player escorts (if at all)?

2. **Trade Profit Algorithm**: DistanceMap uses Dijkstra with fuel/days/danger weights. Could profit be added as a fourth weight, or would a separate algorithm be cleaner?

3. **Transaction Hooks**: Where exactly should hooks be placed to capture all credit changes for a flight recorder? The `AddCredits()` method seems like the central point.

4. **History Retention**: The current 100-day net worth history is a simple vector. What data structure would best support longer-term or categorized financial history without bloating save files?

5. **Plugin Hot-Reload**: Can plugins be reloaded without restarting the game? Current code suggests restart is required (`Plugins::HasChanged()`).

6. **Custom Overlay Performance**: Would a plugin-defined overlay system impact map rendering performance? Need to profile `UpdateCache()` with complex condition evaluation.

7. **Save File Compatibility**: When adding new data to saves (like transaction log), how to handle loading old save files that lack the new fields?
