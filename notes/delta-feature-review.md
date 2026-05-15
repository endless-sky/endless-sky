# Endless Sky Delta — Engine Feature Inventory

A cherry-pick review of features that [Endless Sky Delta](https://github.com/endless-sky/Endless-Sky-Delta)
adds beyond vanilla Endless Sky. Delta forked at vanilla 0.10.1 (March 2023)
and was archived June 1 2025; its `experimental` branch is canonical. Because
the repo is read-only, this feature set is frozen and safe to mine without
chasing a moving target.

**Scope:** engine and mechanic deltas only. Pure content (new ships, systems,
missions, balance) is excluded unless it showcases a new engine capability.

**Important caveat:** Delta continuously merged from upstream, so its
`/changelog` includes everything upstream shipped through ~0.10.12. The
features below are Delta-*exclusive* — landed in a Delta-only PR and not
present in vanilla as of the archive. None of these were upstreamed before
Delta froze, but **confirm against your local vanilla checkout before
porting** — some attributes (e.g. `cloaked ship permeability`) shipped
upstream after the relevant Delta PR.

**Sources of truth used:**

- Delta wiki Home: <https://github.com/endless-sky/Endless-Sky-Delta/wiki/Home>
- Merged PRs labeled `code`: <https://github.com/endless-sky/Endless-Sky-Delta/issues?q=is%3Apr+is%3Amerged+label%3A%22code%22>
- Merged PRs labeled `enhancement`: <https://github.com/endless-sky/Endless-Sky-Delta/issues?q=is%3Apr+is%3Amerged+label%3A%22enhancement%22>
- Merged PRs labeled `UI`: <https://github.com/endless-sky/Endless-Sky-Delta/issues?q=is%3Apr+is%3Amerged+label%3A%22UI%22>
- Delta has only two `/docs` files (`CONTRIBUTING.md`, `readme-developer.md`)
  and a 3-page wiki — no published "differences-from-vanilla" doc exists, so
  the PR list is the spine.

---

## 1. Flight Model / Ship Attributes

### Lateral Thrust as a First-Class Attribute

- **Category:** Ship attributes / Combat (maneuvering)
- **Description:** Lateral thrust (strafe) is split out of the existing thrust
  formula into its own outfit attribute family with energy/heat costs and
  engine flares at lateral hardpoints. Modders can sell strafe boosters and
  tune side-thrust independently of forward thrust. Players gain dedicated
  strafe keys and visible side-flare effects.
- **New attributes:** `lateral thrust`, `lateral thrust ratio`,
  `thrust reduction ratio`, `lateral turn ratio`, `turn reduction ratio`.
  Default ratio (0.25 if unset) configurable in `gamerules.txt`.
- **Source:** PR #10 "Lateral Thrust and Drag Reduction" + PR #151 "Moving
  Lateral Thrust to a stand-alone attribute" + PR #211 fix.
- **Complexity:** **Hard** — touches `Engine.cpp`, `Ship.cpp`, `Hardpoint.cpp`,
  the AI auto-stabilizer, plus every stock ship/engine outfit in `data/`.
  Attribute plumbing is mechanical, but AI integration and content rebalance
  are invasive.
- **Dependencies:** Pairs with #19 (auto-stability preference), #11 (HUD
  thrust bars), and the Engine Slot System (#46).

### Drag Reduction Curve

- **Category:** Ship attributes / Combat
- **Description:** Drag now only applies above max speed or below 10% of max
  velocity, rather than continuously. Makes ships feel less "molasses"
  mid-throttle and changes combat dynamics, especially for slow capitals.
- **Source:** PR #10
- **Complexity:** **Trivial** — single tweak inside `Ship::DoMovement`. Easy
  to cherry-pick or revert.
- **Dependencies:** None, but assumes Delta's tuning baseline.

### Engine Slot System

- **Category:** Ship attributes / Outfits (build constraints)
- **Description:** Every ship gets exactly 1 thruster slot, 1 steering slot,
  and 1 reverse-thruster slot. Outfits consume slots, preventing "stack 9
  huge thrusters" builds and giving small ships a real maneuverability
  advantage. Coalition ships get 1.5 slots that accept 0.25-slot modules to
  preserve their modular identity.
- **Source:** PR #46 (data-only implementation atop existing slot machinery;
  subsequent PRs #89, #91, #94, #219 add slots to specific factions).
- **Complexity:** **Trivial** if vanilla's slot-attribute machinery is
  present (it is — vanilla ships already declare `"engine slot"`-style
  limits). Otherwise **Moderate**. Mostly a `data/ships/*` rewrite.
- **Dependencies:** None code-wise. Forces a rebalance of any 3rd-party ships.

### Universal Reverse Thrust → Reverted to Dedicated Outfits

- **Category:** Ship attributes
- **Description:** PR #48 added 10% reverse thrust (with proportional
  energy/heat) to every thruster. PR #216 then *removed* it because it
  interfered with player control and AI maneuvering, redirecting reverse
  thrust to dedicated reverse-thruster outfits. The interesting carryover is
  the dedicated reverse-thruster outfit category itself.
- **Source:** PR #48 (initial) + PR #216 (revert) — final state: dedicated
  outfits only.
- **Complexity:** **Trivial** — pure data; vanilla already supports
  `"reverse thrust"` as an attribute.
- **Dependencies:** None.

### Intrasolar Ship Category & Flight-Check Exemption

- **Category:** Ship attributes / Misc
- **Description:** New `Intrasolar` ship category and `intrasolar` attribute
  that suppresses the "no hyperdrive / insufficient bays" flight-check
  warnings. Lets modders build in-system-only campaigns (e.g. scaled-up Sol
  with no hyperdrives) without nagging dialogs every takeoff.
- **Source:** PR #167 — touches `data/categories.txt`, `source/Ship.cpp`,
  `source/PlayerInfo.cpp`.
- **Complexity:** **Trivial** — a few `if` checks in flight-check logic plus
  a category constant.
- **Dependencies:** None.

### Conversation/Mission Action: Permanent Ship Attribute Mods

- **Category:** Mission scripting / Ship attributes
- **Description:** New conversation/mission action syntax to permanently add
  or set attributes on the player's flagship:
  `attributes <add|set> <attribute> <quantity>`. Enables narrative-driven
  persistent modifications (saboteur weakening hull, engineering boost
  adding fuel capacity, alien implant marking a ship for later mission
  triggers). Includes safety floors (e.g. mass cannot drop below 0.01).
- **Example:** `attributes add hull 500`, `attributes add "cargo space" 150`,
  `attributes set "parasite" 1`.
- **Source:** PR #190
- **Complexity:** **Moderate** — touches `GameAction.cpp`/`MissionAction.cpp`
  and the `Ship` attribute setter path. Save-format implications since
  modifications persist; check `SavedGame` serialization.
- **Dependencies:** None.

### Mass-Scaled Hyperjump Fuel Cost

- **Category:** Ship attributes / Economy (logistics)
- **Description:** Two new outfit attributes, `jump mass cost` (extra fuel
  per 100 t of ship mass) and `jump base mass` (subtracted before the
  per-100-t calc). Lets jump drives scale realistically with ship size — a
  Kestrel pays more per jump than a Heavy Shuttle. Default Delta tuning: 4
  fuel per 100 t, base mass 900.
- **Source:** PR #123
- **Complexity:** **Moderate** — modifies fuel-consumption math in
  `ShipJumpNavigation.cpp` (or equivalent) and ammo display. Affects every
  existing jump-capable outfit; needs a fuel-capacity rebalance.
- **Dependencies:** None code-wise; pairs with the separate
  `hyperdrive fuel` / `jump drive fuel` attributes (upstream as of 0.10.11).

---

## 2. HUD / UI

### Strategic & Per-Datum Scanner Attributes

- **Category:** HUD/UI / Outfits
- **Description:** Splits the monolithic "tactical scan power" into
  individually purchasable scan capabilities. New outfit attributes:
  `strategic scan power`, `range finder power`, `crew scan power`,
  `thermal scan power`, `energy scan power`, `fuel scan power`,
  `weapon scan power`, `maneuver scan power`. UI dynamically shows only the
  data fields the player has scanners for, and exposes derived values like
  effective gun range, turret range, and actual turn speed. Modders can
  build sensor suites with personality (Sheragi cascading ranges, Remnant
  long-range thermal, etc.).
- **Source:** PR #2 (initial) + PR #71 (Sheragi EWS scanner) + PR #182
  (further strategic scanning). Vanilla 0.10.11 adds a *related* but
  separate "tactical scan data" split — confirm overlap before porting.
- **Complexity:** **Hard** — `Engine.cpp` scan loop, `Outfit.cpp` attribute
  schema, the Information/HUD render path, and `Radar.cpp` all involved.
  Largest UI surface change in Delta.
- **Dependencies:** None directly.

### Player In-Flight HUD (Speed / Accel / Turn / Effective Ramscoop & Solar)

- **Category:** HUD/UI
- **Description:** Right-side HUD column showing flagship's current speed,
  max acceleration, max turn rate, current effective ramscoop, current
  effective solar energy. Plus per-axis thrust bars (longitudinal,
  rotational, lateral) that visualize the auto-stabilizer's hidden inputs.
  Plus a flagship motion vector indicator (off / ghost / white arrow / both).
  Gated by a "Show flagship data in HUD" preference.
- **Source:** PR #11
- **Complexity:** **Moderate** — `Engine.cpp` exposes the values,
  `Information.cpp` and the interface file render them. Self-contained but
  touches the main UI layout file.
- **Dependencies:** Lateral-thrust bars depend on PR #10/#151.

### Solar Power & Solar Wind HUD Readouts

- **Category:** HUD/UI / Outfits
- **Description:** Adds two HUD elements under the radar/navigation panel
  showing the current system's combined solar power and solar wind values
  (correctly summed for binary/trinary systems). Gated by two new outfit
  attributes (`solar power sensor`, `solar wind sensor`) so it remains
  optional kit. Useful for solar-collector and ramscoop builds.
- **Source:** PR #164
- **Complexity:** **Trivial-to-Moderate** — two attributes, two readouts.
  Touches `Information.cpp`/`Engine.cpp` and the HUD interface file.
- **Dependencies:** None.

### Auto-Stability Disable Preference

- **Category:** HUD/UI (preferences) / Combat
- **Description:** Player preference toggle to disable the AI's automatic
  use of lateral thrust for flightpath stabilization on player-controlled
  ships. AI-flown escorts unaffected. Lets pilots who want raw control fly
  without the helper.
- **Source:** PR #19
- **Complexity:** **Trivial** — one preference flag plus a guard in the
  auto-stabilizer call site.
- **Dependencies:** Lateral thrust system (#10).

### Hardpoint Tooltip System Restoration & Empty-Slot Labels

- **Category:** HUD/UI
- **Description:** Re-wires tooltip data into the ship-info and
  hardpoint-info panels (broken by upstream's panel-class split, vanilla PR
  #46), reduces tooltip dwell time from 1.0s to 0.5s, and adds
  `[Empty Gun]` / `[Empty Turret]` / `[Empty Pylon]` labels rendered in a
  dimmer color so empty slots inform without distracting from filled ones.
- **Source:** PR #156 + PR #117
- **Complexity:** **Moderate** — depends on whether vanilla still has the
  broken state Delta fixed. Probably re-implements `HardpointInfoPanel`
  tooltip wiring plus `ShipInfoPanel` text formatting.
- **Dependencies:** Helpful with the Pylon system (#106) for the new label.

### Current System / Flagship Name Layout Swap

- **Category:** HUD/UI
- **Description:** Moves the current-system name above the radar (top
  center), and puts the flagship name in the top-right where the system
  name used to live. Reduces UX confusion where players hunted for system
  info in the wrong corner.
- **Source:** PR #166
- **Complexity:** **Trivial** — pure interface-file edit, possibly minor
  `Engine.cpp` wiring.
- **Dependencies:** None.

### Yellow System-Center Pointer & Expanded Map Zoom

- **Category:** HUD/UI / Map
- **Description:** Yellow arrow HUD pointer indicating where the system
  center is (helps in scaled-up systems like Sol/Alpha Centauri). Expanded
  radar/map zoom range (from ±2× to ±3×) with smoother increments and
  improved haze/parallax behavior at extremes. Doubled radar range as a
  temporary aid.
- **Source:** PR #54 (data) + PR #59 (code)
- **Complexity:** **Moderate** — `Radar.cpp`, `MapPanel.cpp`, plus interface
  edits and a small `AlertLabel`-style HUD element for the pointer.
- **Dependencies:** None, but designed to support Delta's scaled-up Sol
  content.

### Galaxy-Map Star Display Mode (Delta's Original "Starry Map")

- **Category:** Map / UI
- **Description:** Toggleable map mode that replaces system rings with
  star-icon graphics tied to each system's `star` definition, including
  multi-star systems. Helps players hunting good solar/scoop systems.
  Vanilla later added its own (different) Starry Map as a layer; PR #208
  cleaned out Delta's older button-toggle implementation in favor of
  upstream's. The historical Delta version (PR #93) was the original.
- **Source:** PR #93 (original) + PR #208 (cleanup deferring to upstream
  layer).
- **Complexity:** **N/A as standalone** — vanilla already has its own
  version. Skip unless your fork pre-dates upstream's Starry Maps.
- **Dependencies:** None.

### Per-Game Save Directory & 5-Slot Backups

- **Category:** Misc / Save management
- **Description:** Delta uses an `endless-sky-delta/` application-support
  directory for saves and plugins (vs upstream's `endless-sky/`), so the
  two coexist on the same machine. Also bumps rotated previous-save count
  from 3 to 5. Players migrate by copying pilot files into the new path.
- **Source:** PR #78 — `source/Files.cpp`
- **Complexity:** **Trivial** — change a constant in `Files.cpp`. Worth
  doing if you ship a parallel-installable fork.
- **Dependencies:** None.

---

## 3. Combat / Outfits

### Weapon Pylons (New Hardpoint Type)

- **Category:** Outfits / Combat
- **Description:** Adds **Pylons** as a third hardpoint kind alongside guns
  and turrets, intended for missiles/rockets/torpedoes/mines/bombs (renamed
  to **Missile Pylons** in PR #121 for clarity). Decouples ammo-limited
  secondary weapons from infinite-ammo primaries so a "7-gun ship" is no
  longer mechanically identical to a "7-missile ship." Requires reworking
  ship hardpoint definitions across factions.
- **Source:** PR #106 (initial) + PR #121 (rename) + PRs #177–#179 (test
  updates).
- **Complexity:** **Invasive** — modifies `Armament.cpp`, `Hardpoint.cpp/.h`,
  `Ship.cpp`, `ShipInfoPanel`, plus every ship in `data/ships/` and probably
  mod ships too. Largest combat-system change in Delta. Save-compat
  carefully preserved per project policy, but loadout migration is a
  content rewrite.
- **Dependencies:** Empty-slot label PRs #117/#121 read better with this.
  Test-suite updates required (#177, #178, #179).

### Cloaking Differentiation (Phasing Cloak + Comms-While-Cloaked)

- **Category:** Combat / Outfits
- **Description:** Reintroduces the Archon's original phasing-cloak behavior
  (cloaked ships partially permeable to damage — note vanilla 0.10.11 added
  the `cloaked ship permeability` *attribute*, but Delta wired it to
  specific factions earlier) and gives the Remnant cloak the ability to
  scan and communicate while cloaked, matching lore.
- **Source:** PR #61
- **Complexity:** **Trivial** — pure data once the underlying attributes
  exist. The `cloaked ship permeability` attribute is upstream as of 0.10.11.
- **Dependencies:** Vanilla 0.10.11+ for the attribute; otherwise port that
  attribute too.

### Custom Outfit Prices and Sell Type

- **Category:** Economy / Outfits
- **Description:** New `data/custom sales.txt` syntax letting modders specify
  region-, planet-, or outfitter-specific outfit pricing rules. Enables
  genuine economic regionalism (premium pricing in core worlds, discounts
  in frontier outfitters, faction-specific markups). This is a re-PR of
  vanilla PR #6404 which was rejected/stalled upstream.
- **Source:** PR #122 (re-PR of upstream endless-sky #6404)
- **Complexity:** **Moderate** — adds `CustomSale.cpp/h` and
  `CustomSaleManager.cpp/h` (these files exist in Delta source; check
  whether they exist in your vanilla baseline). Hooks into `OutfitterPanel`
  and `Sale` machinery.
- **Dependencies:** None.

### Tutorial Reward: James' Engine Mod

- **Category:** Outfits (showcases additive-multiplier outfit pattern)
- **Description:** Replaces the credits reward at the end of James' tutorial
  with a unique outfit (+10% acceleration, +10% turn) that uses additive
  multipliers (e.g. `acceleration multiplier 0.1`) rather than absolute
  values. Worth listing because it's an early showcase of the
  multiplier-attribute pattern (upstream as of 0.10.7) in mission-reward
  design.
- **Source:** PR #213
- **Complexity:** **Trivial** — pure data; depends on
  `acceleration multiplier` / `turn multiplier` (upstream).
- **Dependencies:** Vanilla 0.10.7+.

---

## 4. Map / Navigation

(Map-specific items are listed under HUD/UI above: Yellow Pointer + Zoom,
Star Display Mode, Current System Layout.)

### Fleet Formations Applied to Stock Human Fleets

- **Category:** AI / Content showcasing engine
- **Description:** Adds the "Vic" formation to merchant, Navy, and Security
  fleets. Pure content, but surfaces vanilla's fleet-formations engine in
  the default game (vanilla shipped formations as of 0.10.9 but doesn't
  apply them widely). Useful as a *demo* of the system.
- **Source:** PR #209
- **Complexity:** **Trivial** — pure data. Skip if you only want the engine;
  port if you want the gameplay feel.
- **Dependencies:** Vanilla 0.10.9+ formation engine.

---

## 5. AI / Politics

(No Delta-exclusive AI engine changes were found in the merged-PR labels.
The AI tweaks Delta carries — escort cloaking, surveillance patrols,
"getaway" personality, etc. — all came from upstream and are in the shared
changelog.)

---

## 6. Cross-Cutting / Identity

### Delta Branding & Save Path Separation

- **Category:** Misc
- **Description:** PR #1 + PR #8 rebranded the game (titles, compass logo,
  version string). PR #78 (above) separated save dir. Both relevant only if
  you intend to ship a forked binary alongside vanilla.
- **Source:** PR #1, #8, #78
- **Complexity:** **Trivial**
- **Dependencies:** None.

---

## Quick Triage Matrix (suggested cherry-pick order)

| Tier | Why pick first | Features |
|---|---|---|
| **Quick wins (Trivial)** | Self-contained, low risk | Drag Reduction (#10 portion), Auto-Stability Preference (#19), Intrasolar Category (#167), Save Dir Separation (#78), Solar HUD (#164), Layout Swap (#166), Empty-Slot Labels (#117) |
| **High-value Moderate** | Significant gameplay/scripting payoff for bounded effort | Mass-Scaled Jump Fuel (#123), Custom Outfit Prices (#122), Permanent Conversation Attribute Mods (#190), Player In-Flight HUD (#11), Yellow Pointer + Zoom (#54/#59) |
| **Big swings (Hard / Invasive)** | Changes the feel of the whole game | Lateral Thrust as attribute (#10/#151), Engine Slot System (#46), Strategic Scanner Split (#2/#71/#182), Weapon Pylons (#106/#121) |
| **Skip / already upstream** | Vanilla has a version | Fleet Formations applied (#209 — engine is upstream), Starry Map (#93/#208 — superseded by upstream layer), Cloak Permeability (#61 — attribute is upstream as of 0.10.11) |

---

## Notes & gaps

- **Delta has no published "differences" doc.** The repo's `/docs` is just
  `CONTRIBUTING.md` + `readme-developer.md`. The wiki has only Home, Command
  Line Arguments, GitHub Policies. The PR list is the only systematic
  record of Delta-only work.
- **The shared changelog is misleading** — versions 0.8.0 → 0.10.12 in
  `/changelog` are mostly upstream content Delta inherited via routine
  merges (visible as the many "Update YYYY-MM-DD conflict resolution" PRs).
  Those are filtered out above.
- **Save compatibility:** Delta's stated goal is mainstream-↔-Delta save
  compatibility, so most of the above respect existing save formats. The
  Permanent Attribute Mods (#190) is the most likely place to find
  serialization changes — verify before porting.
- **Out of ~189 merged PRs, ~25 are genuine Delta-exclusive engine/UI work**
  (the `code` + `enhancement` + `UI` labels overlap heavily). The rest are
  content, balance, conflict-resolution merges, or test updates.
- **Test-suite touch:** Pylons (#106) cascaded into PRs #177/#178/#179
  fixing integration tests. Expect the same pattern if you port it.
