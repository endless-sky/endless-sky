# Endless Sky: Expanded - Player Industry Mod

Goal: player-driven heavy industry in the vein of X4: Foundations / EVE Online, at a
smaller scale. Players build and own facilities (planetside districts, outposts,
eventually space stations) that produce goods over time.

Work is delivered in small pieces, each one session sized, each ending with a
Windows playtest build. Keep this file updated at the end of every piece.

## Status

| Piece | Status |
| --- | --- |
| 0. Pipeline + dev menu | Done, confirmed working on Windows |
| 1. Industry MVP | Done, waiting for playtest confirmation |
| 2. Production chains and upkeep | Next (scope to be agreed) |

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

## Roadmap (each is one or more pieces; revise as we learn)

2. Production chains (inputs -> outputs), daily upkeep, several facility types.
3. Industry overview from anywhere; rename facilities; auto-sell to local market.
4. Outposts on uninhabited planets and moons.
5. Player-built space stations (new landable stellar objects) - biggest engine job.
6. Economic impact: output moves local prices; NPC haulers move goods.
