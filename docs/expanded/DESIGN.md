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
| 2. Chains, upkeep, expansion, outposts, auto-sell, overview | Done, waiting for playtest |
| 3. Space stations | Next (scope to be agreed) |

## Piece 0 - Pipeline + dev menu (done)

- `.github/workflows/playtest.yml`: every push to `claude/**` or `playtest/**` builds
  Windows only and replaces the `playtest` pre-release
  (https://github.com/voidicebreaker/endless-sky-expanded/releases/tag/playtest).
  The upstream workflows (`ci.yml`, `cd.yaml`) only run on master/PRs and are left untouched.
- Main menu credits start with "Welcome to Endless Sky: Expanded!" so testers can tell
  the build apart; CI appends the commit hash and build time below the version line.
- Developer menu: `source/DevPanel.{h,cpp}`, opened with `` ` `` or F12 on the planet
  screen (`PlanetPanel::KeyDown`). Advance 1/7/30 days, add credits.
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

## Roadmap (revise as we learn)

3. Player-built space stations (new landable stellar objects) - biggest engine job.
4. Economic impact beyond auto-sell: NPC haulers move goods; facilities affect planet supply.
5. Quality of life: rename holdings, remote status in Player Info, balance pass on prices.
