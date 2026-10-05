# Endless Sky: Expanded — Player Industry Mod

Goal: player-driven heavy industry in the vein of X4: Foundations / EVE Online, at a
smaller scale. Players build and own facilities (planetside districts, outposts,
eventually space stations) that produce goods over time.

Work is delivered in small pieces, each one session sized, each ending with a
Windows playtest build. Keep this file updated at the end of every piece.

## Status

| Piece | Status |
| --- | --- |
| 0. Pipeline + dev menu | Done, waiting for playtest confirmation |
| 1. Industry MVP | Next |

## Piece 0 — Pipeline + dev menu (done)

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

## Piece 1 — Industry MVP (agreed scope)

One facility type the player can build and own, proving data, UI, save game and daily tick.

- New data type `facility` (defined in `data/expanded/`), loaded via `GameData` like outfits.
  First and only facility: **Mining Outpost** — 150,000 credits, 2 tons Metal/day,
  50 ton storage.
- Buildable **only on New Greenland** for the MVP (planet in `data/map planets.txt`).
  Make the allowed planets part of the facility's data definition so this is easy to widen.
- Planet screen: **Industry** button in the empty right column slot (center 340 255 in
  `interface "planet"` and the small screen variant in `data/_ui/interfaces.txt`), key `n`,
  shown with a new `"has industry"` condition set in `PlanetPanel`.
- Industry sub-panel (modelled on `BankPanel`): list buildable/owned facilities and
  stockpiles, Build (spend credits), Collect (move stockpile into fleet cargo, limited
  by free space).
- Daily production hooked into `PlayerInfo::AdvanceDate`.
- Saved in a new `industry` block in the save file (`PlayerInfo::Load`/`Save`);
  saves without it load as "no facilities".
- Landing message when a facility here has goods ready.
- Tests: unit test for production/stockpile math; integration test that builds,
  advances days via the dev menu, collects.

Not in the MVP: inputs/production chains, upkeep, stations in space, price effects,
NPC haulers, remote management, custom art.

## Roadmap (each is one or more pieces; revise as we learn)

2. Production chains (inputs → outputs), daily upkeep, several facility types.
3. Industry overview from anywhere; rename facilities; auto-sell to local market.
4. Outposts on uninhabited planets and moons.
5. Player-built space stations (new landable stellar objects) — biggest engine job.
6. Economic impact: output moves local prices; NPC haulers move goods.
