# Endless Sky: Expanded - Player Industry Mod

Goal: player-driven heavy industry in the vein of X4: Foundations / EVE Online, at a
smaller scale. Players build and own facilities (planetside districts, outposts,
eventually space stations) that produce goods over time.

Work is delivered in pieces, each ending with a Windows playtest build. The user asked
for larger pieces (several roadmap items at once) to save round trips. Keep this file
updated at the end of every piece.

## Status

| Piece | Status |
| --- | --- |
| 0. Pipeline + dev menu | Done, confirmed working on Windows |
| 1. Industry MVP | Done, confirmed working on Windows |
| 2. Chains, upkeep, expansion, outposts, auto-sell, overview | Done, confirmed working on Windows |
| 2b. Outpost story (Varga Deepworks), produced-only goods and quests | Done, waiting for playtest |
| 3. Space stations, warehouses and freight routes | Done, waiting for playtest |
| 4. Next | To be agreed |

## Piece 0 - Pipeline + dev menu (done)

- `.github/workflows/playtest.yml`: every push to `claude/**` or `playtest/**` builds
  Windows only and replaces the `playtest` pre-release
  (https://github.com/voidicebreaker/endless-sky-expanded/releases/tag/playtest).
  The upstream workflows (`ci.yml`, `cd.yaml`) only run on master/PRs and are left untouched.
- Main menu credits start with "Welcome to Endless Sky: Expanded!" so testers can tell
  the build apart; CI appends the commit hash and build time below the version line.
- Developer menu: `source/DevPanel.{h,cpp}`, opened with `` ` `` or F12 on the planet
  screen (`PlanetPanel::KeyDown`) or in flight (`MainPanel::KeyDown`); key repeats are ignored
  so holding the key does not open and close it. Advance 1/7/30 days, add credits.
  Always enabled for now; gate it (e.g. behind `-d` or a preference) before any public release.
- Integration test: `tests/integration/config/plugins/integration-tests/data/tests/tests_dev_menu.txt`.

## Piece 1 - Industry MVP (done)

One facility type the player can build and own, proving data, UI, save game and daily tick.

- `source/Facility.{h,cpp}`: facility *type*, loaded from `facility "<name>"` nodes
  (registered in `UniverseObjects`, exposed as `GameData::Facilities()`). Fields:
  `description`, `cost`, `output "<commodity>" <tons/day>`, `storage`, `planet "<name>"...`.
  Planets are stored by true name so the class has no GameData dependency (unit testable).
- `data/expanded/facilities.txt`: the **Mining Outpost** - 150,000 credits, 2 tons Metal/day,
  50 ton storage, buildable only on **New Greenland**.
- `source/Industry.{h,cpp}`: what the player owns (`PlayerInfo::GetIndustry()`), a list of
  holdings (type, planet true name, stockpile). At most one of each type per planet.
  Saved as an `industry` block in the save file; unknown facility types are dropped
  with a warning on load. `AdvanceDay()` is called from `PlayerInfo::AdvanceDate`.
- `source/IndustryPanel.{h,cpp}`: planet sub-panel drawn in the planet "content" box like
  the bank (`SetTrapAllEvents(false)` so the planet buttons keep working). Rows per facility,
  Build / Collect button, Up/Down + Enter, description of the selected facility, status line.
  Collect moves goods into the pooled fleet cargo (`player.Cargo()`), limited by free space;
  collected goods have no cost basis.
- Planet screen: **Industry** button (key `n`) in the right column, shown when the
  `"has industry"` condition is set (a facility can be built here or is owned here).
  Landing message when a facility here has goods waiting.
- New condition `"facilities owned"` (number of holdings), usable by missions.
- Tests: `tests/unit/src/test_industry.cpp` (load, production cap, collect, save/load) and
  integration test `tests_industry.txt` (build with dev-menu credits, wait a week, collect,
  take off, check cargo).

## Piece 2 - Production chains and holdings management (done)

Combined roadmap items 2-4 (and part of 6).

- `Facility` now has `upkeep`, any number of `input`/`output` lines, and `attributes`
  (buildable on planets with any listed attribute, in addition to `planet` names).
  `UniverseObjects::CheckReferences` warns about facilities using unknown commodities.
- `Industry::Holding` has `count` (Expand = build again; production, upkeep and per-commodity
  storage all scale with it), a `stock` map for inputs and outputs, `autoSell`, and a `status`
  (not saved: Starting up / Running / needs inputs / storage full / can't pay upkeep).
  Old saves' `stockpile` loads as stock of the first output.
- Daily production (`Industry::AdvanceDay`, called from `PlayerInfo::AdvanceIndustry`):
  upkeep is paid only if the credits on hand cover it (otherwise the facility idles);
  each unit runs once if every input is available and no output is full; auto-sell sells
  all output stock at the planet's system price and calls `GameData::AddPurchase`, so
  sales lower local prices. A "daily" message summarizes upkeep and sales.
- Uninhabited worlds: the Industry button is available when the planet is uninhabited or
  its services are usable. Outposts there cannot auto-sell (no market).
- Industry panel: facility list (left), details (right: status, cost, upkeep, uses, makes,
  stock or description), buttons Build/Expand (E), Supply (U, moves inputs from cargo and
  removes their cost basis), Collect (C), Sell on/off (A); Enter = collect if owned else build.
  Tab toggles the **All holdings** overview (planet, facility xN, status, goods; total upkeep),
  available on any planet once the player owns something.
- Content (`data/expanded/facilities.txt`): 11 facilities. Raw: Mining Outpost (Metal),
  Ore Extractor and Deep Core Mine (uninhabited; Metal, Heavy Metals), Hydroponics Farm (Food),
  Petrochemical Plant (Plastic). Processing: Textile Mill, Machine Works, Pharmaceutical Lab,
  Electronics Foundry, Industrial Fabricator, Luxury Workshop. New Greenland can build
  Mining Outpost, Hydroponics Farm, Textile Mill and Machine Works.
- Tests: unit tests for chains, upkeep shortfall, expansion, auto-sell, supply caps, save/load
  and legacy saves; the integration test now builds an outpost and a Machine Works, supplies
  it and checks the Equipment produced.
- Not verified visually in the container (screen capture of the GL window does not work under
  Xvfb here); layout needs a playtest check, especially in the small-screen layout.

## Piece 2b - Outpost story and produced-only quests (done)

- Writing style: match Endless Sky (dry, specific, understated humor; second person,
  present tense in conversations; ASCII only, no em dashes). Avoid AI-isms.
- `Facility`: `uninhabited` (buildable on any uninhabited planet, replacing the
  `attributes uninhabited` match, which missed most uninhabited worlds), `requires "<condition>"`
  (hidden until the player has the condition), `flavor` lines.
- Flavor: each facility with flavor has a 1 in 20 chance per day to send a status report
  (`<planet>` replaced). Reports go to the message log ("low") and to the holding's last five
  `reports` (saved), and the latest one shows in the Industry panel if there is room.
- Produced-only goods (`data/expanded/commodities.txt`, special commodities, never sold):
  Core Samples (Survey Drill), Pressure Crystals (Crystal Bore, Varga Deep Bore),
  Refined Isotopes (Isotope Separator: Heavy Metals -> Refined Isotopes, research worlds).
- Engine support for quests: mission action `commodity "<name>" <tons>` (negative takes it from
  pooled cargo / ships in the system, with cost basis; checked in `MissionAction::CanBeDone`),
  conditions `"commodity: <name>"`, `"facility: <name>"` (units owned) and `"outposts owned"`.
- Story (`data/expanded/varga claims.txt`): Edda Lund (New Greenland Historical Society),
  descendant of Varga Deepworks driller Jonas Lund; Dr. Arsen Petrosyan (physicist, Hermes).
  1. Old Claims: offered on landing (human space, not New Greenland) once an outpost exists;
     meet Edda; unlocks the Survey Drill.
  2. Core Samples for Edda: 20 Core Samples -> Varga Ore Hopper (25 cargo for 20 space),
     50,000 credits, unlocks the Crystal Bore.
  3. Isotopes for Hermes: unlocks the Isotope Separator on offer; 15 Refined Isotopes ->
     Varga Isotope Cell (3.6 energy, 7 heat, 22 mass), 150,000 credits.
  4. The Last Survey: 10 each of Pressure Crystals, Refined Isotopes, Core Samples ->
     Varga Survey Array (asteroid scan 60, cargo scan 15), 250,000 credits, unlocks the
     Varga Deep Bore, sets "expanded: varga charter" (Varga's unfiled orbital station charter,
     the hook for space stations).
  News on New Greenland and Hermes after the relevant steps.
- Tests: unit tests for the new facility options and reports; integration test
  `tests_varga.txt` (deliver core samples, check the samples are taken and the rewards given).

## Piece 3 - Space stations, warehouses and freight routes (done)

- Stations: a facility with `station` (data: "Orbital Station", 2.5M, 1,000/day upkeep,
  1,000 t warehouse) is offered on any planet with Industry access in a system without a
  player station. Building it asks for a name (default "<system> Station", validated by
  `PlayerInfo::IsValidStationName`: unused planet/system name, no quotes) and calls
  `PlayerInfo::FoundStation`, which generates event-style changes (a `planet` with attributes
  `station` and `"player station"`, government "Expanded Holdings", default spaceport with all
  services, landscape land/station1; and `system ... add object` with sprite
  planet/station-depot-a0, orbit 400 beyond the outermost object), appends them to `dataChanges`
  (saved under "changes" and re-applied on load) and applies them with `AddChanges`. The station
  facility itself becomes a holding on the new planet (upkeep, warehouse, flavor; Expand adds
  warehouse space). If the player has "expanded: varga charter", the station description
  mentions it. Verified: station survives save and reload and can be landed on.
- Station modules (`attributes "player station"`): Warehouse Module, Zero-G Foundry
  (Heavy Metals + Plastic -> Electronics), Crystal Refinery (requires the Varga charter;
  Pressure Crystals + Refined Isotopes -> Electronics + Luxury Goods).
- Warehouses: any facility can add `warehouse <tons>` to its planet; shared storage for any
  commodity (`Industry::Store/Retrieve/WarehouseCapacity`), saved per planet. Storage Depot
  (uninhabited, 300 t) puts warehouses at outposts. The Industry panel shows a Warehouse row
  first when a planet has one: Store (all cargo commodities) / Load (all, up to free space).
- Freight routes (`Industry::Route`): commodity, from, to, tons a day, buy, sell. Run daily
  after production and before auto-sell. Source order: facility outputs, warehouse, market (if
  buy). Destination order: facilities that use it, warehouse, market (if sell). Moves
  min(tons, room, available), limited by credits. Freight = tons x (5 + 10 x jumps), jumps by
  breadth-first search over hyperspace links (`IndustryWorld` in PlayerInfo.cpp, which also
  provides market prices and records trades with `GameData::AddPurchase`).
- `Industry::World` replaces the old auto-sell lambda (markets + jumps), so the daily logic
  stays testable with a mock. The daily message lists upkeep, freight, purchases and sales.
- Industry panel has three views on Tab: this planet, freight routes (list plus an editor with
  arrow buttons for commodity/from/to/tons and Buy/Sell toggles; R new, X delete, -/+ tons),
  and all holdings. Route endpoints: planets with the player's facilities, plus the current one.
- Tests: unit tests for warehouses, routes (delivery, unreachable, buy and sell, room limits,
  affordability) and saving routes/warehouses; integration test `tests_stations.txt` (found a
  station, land on it, store cargo in its warehouse).

## Roadmap (revise as we learn)

- Economic impact beyond auto-sell: visible NPC haulers for freight routes; facilities affect
  planet supply.
- Station visuals and growth: sprite changes as a station gains modules; defense.
- Quality of life: rename holdings, industry tab in Player Info, balance pass on prices.
