# Fork Feature Wishlist & Spec

A consolidated spec drawing from two sources: (1) longstanding Endless Sky
community feature requests, gripes, and explicitly-rejected ideas, and (2)
mechanics from EV Nova that ES never adopted but which would unlock new
classes of content. Companion to `delta-feature-review.md`.

This document is meant as a **decision surface**, not a build plan. Per item:
what the ask/mechanic is, demand or modder leverage, complexity, and
upstream status (open / closed-wontfix / never coming). The Synthesis
section at the end pulls convergent themes together with a recommended
starting tier.

**Coverage notes.** Reddit and Steam thread bodies were not directly
fetchable in this environment; their signals come from search snippets and
cross-references to GitHub. GitHub reaction counts are exact at survey time.
The EV Nova Bible source pages 403'd; Nova mechanics are reconstructed from
the EVN Wiki, GameFAQs guides, archived Ambrosia forum threads, and
`vasi/evnova-utils` source — all multi-source corroborated, but verify field
names against the Bible directly before implementing:
<https://andrews05.github.io/evstuff/guides/evnbible.html>

---

## Table of contents

- [Part A — Endless Sky community wishlist](#part-a--endless-sky-community-wishlist)
  - [A1. UX / UI](#a1-ux--ui)
  - [A2. Mission system & modding tools](#a2-mission-system--modding-tools)
  - [A3. Combat / AI](#a3-combat--ai)
  - [A4. Fleet management](#a4-fleet-management)
  - [A5. Map / navigation / save-load](#a5-map--navigation--save-load)
  - [A6. Engine & performance](#a6-engine--performance)
  - [A7. Endgame & content-gap engine hooks](#a7-endgame--content-gap-engine-hooks)
  - [A8. Vision-doc rejected (pure cherry-pick bait)](#a8-vision-doc-rejected-pure-cherry-pick-bait)
- [Part B — EV Nova mechanics worth porting](#part-b--ev-nova-mechanics-worth-porting)
- [Part C — Synthesis & recommended starting tier](#part-c--synthesis--recommended-starting-tier)
- [Sources](#sources)

---

## Part A — Endless Sky community wishlist

### A1. UX / UI

#### A1.1 Independent UI vs. world view scaling
- **Description:** On high-DPI displays, players must crank UI scale to
  ~200% for readable text, but this also zooms the gameplay viewport,
  shrinking the field of vision. Independent scaling slider is the fix.
- **Demand:** Discussion #8071 — 4 upvotes, 26 comments. Maintainers in
  thread agreed it's valid; OP started implementation, never merged.
- **Status:** Open / acknowledged.
- **Complexity:** **Moderate** — decouple render scale from UI scale through
  layout code.

#### A1.2 Switchable hotkey profiles
- **Description:** Players want to swap keybinding profiles (south paw,
  boxer, recon) without re-typing every binding.
- **Demand:** Issue #2291 — **26 reactions** (highest-reaction QoL ticket).
- **Status:** Open, milestone 0.11.3, PR #11764 in flight.
- **Complexity:** **Trivial** — read multiple keybinding files, drop-down
  to swap.

#### A1.3 Bind controls to mouse buttons
- **Description:** Mouse 4/5/side-buttons can't be bound natively; users
  rely on third-party remappers that break for other apps.
- **Demand:** Issue #4647 — open, milestone 1.0.0; recurring in Steam input
  threads.
- **Status:** Open, no work scheduled.
- **Complexity:** **Trivial** — input event already arrives, just expose to
  keybinder.

#### A1.4 Reset to default controls
- **Description:** No "reset all keybindings" button; broken bindings
  require hand-editing `keys.txt`.
- **Demand:** Issue #4771 — recurring on QoL listings.
- **Status:** Open.
- **Complexity:** **Trivial**.

#### A1.5 Adjustable Caps-Lock / fast-forward speed
- **Description:** Fast-forward locked to 3×; users want configurable
  multipliers (¼, ½, 1, 2, 3, 4×) for travel and combat slow-mo. EV Nova
  supported both directions.
- **Demand:** Issue #3887 — 11 reactions; recurring in EV-Nova-comparison
  Steam threads.
- **Status:** Open.
- **Complexity:** **Trivial** — one float multiplier.

#### A1.6 Flagship identification in player info panel
- **Description:** Flagship highlighted in yellow — same colour as
  warnings — making it ambiguous. Requested: green or explicit star icon.
- **Demand:** Issue #9212 — 9 reactions, 25 comments (bikeshedding kills
  velocity).
- **Status:** Open.
- **Complexity:** **Trivial**.

#### A1.7 Map wind / solar power overlay
- **Description:** Solar/wind values are invisible on map; players jump in
  to measure. Toggleable map overlay (high=yellow, low=blue) mirroring
  commodity-price view.
- **Demand:** Issue #4964 — 8 reactions, 10 comments.
- **Status:** Open.
- **Complexity:** **Trivial** — data already in `System` objects, reuse
  commodity overlay code path.
- **Cross-ref:** Delta has the *flight-time* solar/wind HUD readouts (Delta
  PR #164); this is the *map-time* version.

#### A1.8 Notes / annotations on map systems
- **Description:** Pin per-system notes ("Pirate spawn", "Avoid"). Today
  done in external docs.
- **Demand:** Issue #237 — 3 reactions but extremely old (2015), recurring
  on Steam.
- **Status:** **Closed wontfix** (no public rationale).
- **Complexity:** **Moderate** — UI layer + save-file persistence.

#### A1.9 New map planet UI polish
- **Description:** Redesigned map-planet panel doesn't match house style:
  ribbons cut off, "(not yet visited)" overflows, scroll affordance is
  invisible. Polish/redo needed.
- **Demand:** Issue #7065.
- **Status:** Open.
- **Complexity:** **Moderate** — UI refactor.

#### A1.10 Game-over screen / death feedback
- **Description:** Losing leaves player on a frozen screen — no
  acknowledgment, no stats summary, no load prompt.
- **Demand:** Issue #9717 — 8 reactions, 29 comments.
- **Status:** Open.
- **Complexity:** **Moderate** — new modal panel + endgame stats aggregation.

---

### A2. Mission system & modding tools

#### A2.1 Conditional hails
- **Description:** NPCs hail with the same generic line whether you're a
  happy trader or being torn apart by a Leviathan. Modders want a
  `conditions` block on `hail` similar to missions.
- **Demand:** Issue #8379 — 9 reactions.
- **Status:** Open. Author concedes hail-system refactor needed.
- **Complexity:** **Moderate**.
- **Cross-ref:** Closely related to Nova's subtitled comm chatter (B16) —
  doing both together gives a coherent "ships emit conditional flavour
  lines" system.

#### A2.2 Action-based data format (effect-on-action nesting)
- **Description:** Every action+effect combo (`thrusting heat`,
  `cloaking ion`, `firing scramble`) requires a new keyword baked into
  engine code. Proposal nests effects inside actions:
  ```
  "thrust" 36
      "energy" 2.2
      "heat" 5.4
  ```
  Eliminates engine-side keyword sprawl.
- **Demand:** Issue #9025 — 12 reactions (highest-engagement modder issue).
- **Status:** Open, labelled refactor.
- **Complexity:** **Hard** — outfit parser, ship-state update loop, save/load
  compat.

#### A2.3 Reload plugins without restart
- **Description:** Iterating on a plugin requires full restart. A "reload
  plugins" button would massively shorten the dev loop.
- **Demand:** Issue #9137 — 7 reactions.
- **Status:** Open.
- **Complexity:** **Moderate** — caches must invalidate cleanly; some game
  state assumes universe is immutable post-load.

#### A2.4 In-game plugin browser / installer
- **Description:** RisingLeaf's libgit2-backed in-game browser pulling from
  a curated trusted-plugin index. Today: download zips, unzip, restart.
- **Demand:** Discussion #9058 — 8 upvotes, 22 comments. Broadly supported,
  no merged code.
- **Status:** Open discussion.
- **Complexity:** **Hard** — libgit2 dependency, sandbox concerns, registry
  hosting.

#### A2.5 Lua scripting for plugins
- **Description:** ES's plugin format is purely declarative; dynamic
  behaviour requires forking the C++ engine, then PRs sit 1-5 years. Lua
  would let community ship features without engine changes.
- **Demand:** Discussion #10279 — 6 upvotes, 26 comments. Long-running
  tension referencing original maintainer's 2017 reservation in #1841.
- **Status:** Effectively rejected by original dev; current maintainers
  ambivalent. **Cherry-pick candidate par excellence.**
- **Complexity:** **Engine-rewrite** for full bindings; **Hard** for an MVP
  (sol2 + small surface area on missions/AI).

#### A2.6 Mission events: pass time, teleport
- **Description:** Mission scripts can't make in-game time pass or teleport
  the player+escorts. Blocks "training montage" and many narrative cuts.
- **Demand:** Issue #6018 — 9 reactions, 14 comments.
- **Status:** Open.
- **Complexity:** **Moderate** — state mutation hooks exist; need keywords
  + safe handling of escorts left behind.

#### A2.7 Conditional properties on planets/systems/governments
- **Description:** Today a planet's `attributes`, fleet presence, system
  visibility, and outfit-description visibility are static or toggled by
  mission `add/remove`, which doesn't scale across parallel storylines (war
  + piracy + alien threat all editing the same fleet). ConditionSets
  evaluated at the object level fix this.
- **Demand:** Issue #7138 — 7 reactions.
- **Status:** Open.
- **Complexity:** **Hard** — universe state evaluation gets re-entrant; perf
  concerns.

#### A2.8 Per-outfitter license overrides
- **Description:** Outfit license requirements are global. Black-market or
  rival-faction outfitters can't sell the same item under different rules
  without duplicate definitions. Per-outfitter `add license` /
  `remove license` overrides.
- **Demand:** Issue #7129 — 7 reactions.
- **Status:** Open.
- **Complexity:** **Trivial-Moderate** — purchase validation reads from
  outfitter, not just outfit.

---

### A3. Combat / AI

#### A3.1 Missile-spammer flee AI
- **Description:** Enemies armed only with missiles flee while spamming;
  most missiles time out before reaching the player. Frustrating and
  wasteful. Players want missile boats to circle/joust — fun > realism.
- **Demand:** Issue #1537 — 11 reactions.
- **Status:** Open since 2017.
- **Complexity:** **Moderate** — new AI personality / behaviour tree branch.

#### A3.2 Pirate suicide behaviour
- **Description:** Pirates attack vastly-overgunned merchant fleets
  pointlessly. Players want pirate AI to bail when risk:reward is hopeless.
  Related: pirates focus the weakest ship (often a fresh capture), killing
  it instantly (#4220).
- **Demand:** Issue #7894 — 9 reactions, 31 comments. Cross-corroborated on
  Steam.
- **Status:** Open; #4220 closed without fix.
- **Complexity:** **Moderate** — firepower-aware spawn / engagement.

#### A3.3 Combat reinforcements never stop
- **Description:** "Rate of combat reinforcements is too high. No fight is
  ever really done." Drop random spawn rate to ~20% during active combat.
- **Demand:** Issue #4422 — 7 reactions, 19 comments. Frequently echoed on
  Steam.
- **Status:** Open, labelled experimental.
- **Complexity:** **Trivial** — gate the spawn timer on a `combat in progress`
  condition.

#### A3.4 Delay hostile spawns from incoming hyperlink
- **Description:** Hostiles can spawn directly behind the player on
  hyper-arrival. Unfair ambush for new players. Suppress arrivals on the
  inbound link briefly.
- **Demand:** Issue #3432 — 8 reactions.
- **Status:** Open.
- **Complexity:** **Trivial**.

#### A3.5 Allied AI destroying ships you're about to board
- **Description:** Player disables a Leviathan, Republic patrol jumps in,
  obliterates it before player can board. #5245 requested a "don't fire if
  shot would hit a disabled ship" toggle for player escorts.
- **Demand:** Issues #172 and #5245; recurring on Steam.
- **Status:** #5245 **closed wontfix** ("unlikely"); #172 closed without
  resolution.
- **Complexity:** **Moderate** for player-escort case (line-of-sight
  pre-check + toggle); **Hard** for allied-faction case (allied AI doesn't
  know player intent).

#### A3.6 Fighters strafe instead of circling
- **Description:** Fighters orbit; big-ship turrets can outpace the orbit;
  fighters die. Strafing runs would force turrets to swing 180° — better
  fighter survivability at a DPS cost.
- **Demand:** Issue #2220 — 9 reactions, 18 comments.
- **Status:** Open.
- **Complexity:** **Moderate** — new flight pattern in fighter AI.

#### A3.7 Weapon warmup / charge times
- **Description:** No engine concept of holding-to-charge weapons (gatling
  spin-up, capacitor charge). Adds charge rate, decay, threshold, max,
  charging energy/heat consumption, charge-scaled damage.
- **Demand:** Issue #9972 — 12 reactions.
- **Status:** Open.
- **Complexity:** **Hard** — per-weapon state, projectile inheritance,
  balance.

#### A3.8 Boarding / hand-to-hand rework
- **Description:** Boarding boils down to "most crew × highest attack
  outfit." Rework introduces combat width, terrain, equipment categories
  (primary/consumable/armor), restrictions on essential operating crew,
  defense as a ship attribute, crew-loss → recruitment-cost feedback.
  Adjacent: #9899 "allow surrender" (closed wontfix, 6 reactions, 22
  comments) and #3346 "fight crew before plundering internals" (closed
  wontfix, 6 reactions, 39 comments).
- **Demand:** Issue #6986 — **16 reactions** (highest-reacted enhancement).
- **Status:** Open as concept; surrender + internal-plunder variants both
  rejected — **fork bait**.
- **Complexity:** **Hard** — data format, save format, UI, balance.

#### A3.9 Battles too short / regen dominance
- **Description:** Tier-1 human ship HP doesn't scale with weapon DPS;
  regen outfits dominate so completely that hit-and-run is dead and light
  ships can't exploit speed. EV had shield capacitors / armor plating /
  jammers as competing options.
- **Demand:** Issue #9802 — 11 reactions, 49 comments. Discussions #7041
  ("The Problem with Regeneration") and #9862 ("Alternative solutions").
  Echoed on HN.
- **Status:** Open / debated, no consensus.
- **Complexity:** **Moderate** (tuning) to **Hard** (introducing competing
  defensive mechanics).

#### A3.10 Stealth system
- **Description:** No real stealth playstyle. Reuse missile detection modes
  (IR/radar/optical) — detection scales with mass, heat, speed, distance
  (inverse-square), with environment (ion storms, asteroids) interfering.
  Enables hit-and-run, infiltration, smuggler/spy careers.
- **Demand:** Issue #7104 — 9 reactions, 16 comments.
- **Status:** Open.
- **Complexity:** **Hard** — new sensor/visibility model touching AI,
  rendering, scanning.

#### A3.11 Smuggling / illegal-outfit overhaul
- **Description:** Navy ships scan everything indiscriminately, creating
  "an overwhelming blockade…a force of nature." Proposal: remove Navy
  scanning, introduce a dedicated ISP scanner faction with lighter ships
  and decaying reputation; add response options when caught (dump cargo /
  bribe / flee); scale fines by `quantity × illegal value`; let governments
  define illegality + fine values.
- **Demand:** Issue #6314 — 12 reactions.
- **Status:** Open.
- **Complexity:** **Hard** — new faction, scanning AI, response UI, balance.

---

### A4. Fleet management

#### A4.1 Fleet groups in management UI
- **Description:** With 100+ ships the fleet info panel is a wall of names.
  Collapsible groups with aggregated stats. Mockup attached.
- **Demand:** Issue #6640 — 8 reactions. Reinforced by multiple Steam
  threads on fleet management UI / sorting.
- **Status:** Open.
- **Complexity:** **Moderate** — data model + scrollable nested UI.

#### A4.2 Personalities for escorts
- **Description:** Backend has many AI personalities; UI exposes none.
  Large fleet battles devolve into "battle ball." Players want to assign
  Wingman / Patrol / Miner / Bait per ship, possibly via a "Tactic Chip"
  outfit.
- **Demand:** Discussion #9174 — 7 upvotes. Multiple Steam threads.
- **Status:** Open discussion.
- **Complexity:** **Moderate** — personalities exist; mostly UI +
  per-ship serialisation.

#### A4.3 Replacing fighters on the fly
- **Description:** Lost a fighter? Fly to a shipyard, buy, manually re-dock.
  No "fighter rack" outfit storing spares in cargo for redeployment.
  Fighters get treated as one-shots.
- **Demand:** Issue #10009 — 9 reactions.
- **Status:** Open.
- **Complexity:** **Moderate** — conditional outfits + persistent
  mid-mission ship state.

#### A4.4 Bays carrying multiple ship categories
- **Description:** Bays accept exactly one category. Drone bays are rare so
  drones are barely viable; fighter bays could hold drones. Primary
  category (UI display) + accepted-set.
- **Demand:** Issue #9896 — 11 reactions.
- **Status:** Open.
- **Complexity:** **Moderate** — data format + bay-allocation algorithm + UI.

#### A4.5 Jump-drive fleets scattering on arrival
- **Description:** Hyperdrive ships arrive together; jump-drive ships arrive
  at random points equidistant from system centre. Recent arrival-distance
  changes worsened this. Constrain jump-in points like hyperdrive does.
- **Demand:** Issue #7486 — 11 reactions, 33 comments.
- **Status:** Open.
- **Complexity:** **Trivial** — pick a tighter random spread.

#### A4.6 Hyperlane vs jump-drive routing toggle
- **Description:** Mixed-drive fleets get suboptimal routing because
  pathfinding picks shortest by jumps regardless of drive type, stranding
  hyperdrive ships out of fuel. Per-mission `Hyperlane / Jump / Auto`.
- **Demand:** Issue #7320 — 7 reactions.
- **Status:** Open.
- **Complexity:** **Moderate** — separate cost functions + UI.

#### A4.7 Review fleet before emergency launch
- **Description:** Spaceport defence missions auto-launch your entire
  fleet — including freighters you'd rather keep grounded. "Review fleet"
  prompt with park/unpark/flagship-swap before liftoff.
- **Demand:** Issue #7110 — 7 reactions.
- **Status:** Open.
- **Complexity:** **Trivial** — insert existing fleet panel before launch.

#### A4.8 Unpark all local
- **Description:** "I accidentally unparked 100 ships across a half dozen
  systems." Today's "Unpark All" is global; need a system-scoped variant.
- **Demand:** Discussion #7532 — 4 upvotes; PR opened.
- **Status:** PR open.
- **Complexity:** **Trivial**.

#### A4.9 Improve escort park / unpark / land symmetry
- **Description:** You can land your flagship to save an escort but can't
  tell that escort to land/warp itself; parking before time skips dodges
  crew wages by an undocumented mechanic; you can't tell a fleet to
  relocate to a specific system to park.
- **Demand:** Issue #8126.
- **Status:** Open.
- **Complexity:** **Moderate**.

---

### A5. Map / navigation / save-load

#### A5.1 In-game lore / pilot's manual
- **Description:** Crucial lore (faction histories, timeline) lives only on
  the wiki. Pre-loaded logbook entries (Pilot's Manual, History, Faction
  Histories) earned through play. "A vast majority of players likely don't
  look outside of the game for extra information about it."
- **Demand:** Issue #3663 — **17 reactions** (highest-reacted open issue).
- **Status:** Open, milestone 1.0.0, no assignee.
- **Complexity:** **Trivial-Moderate** — mostly content authoring + a small
  logbook seed mechanism.

#### A5.2 Campaign progress visibility
- **Description:** Players don't know if a campaign has ended, paused, or
  become unreachable due to a missed prerequisite. Related: #5039 missions
  silently fail when other missions take off from a planet.
- **Demand:** Discussion #11424 — 6 upvotes; cross-corroborated on Steam.
- **Status:** Open.
- **Complexity:** **Moderate** — campaign state introspection + UI panel.

#### A5.3 Mission-triggered & "last safe save" autosaves
- **Description:** Current autosave is mission-triggered and silently
  overwritten. Users have lost months of progress to a bad load. Extra
  autosave keyword with naming/branching, plus a "last safe save" mission
  action.
- **Demand:** Issues #4344, #11092, recurring Steam threads.
- **Status:** Mixed — some closed, some open.
- **Complexity:** **Trivial-Moderate**.

#### A5.4 Game overwrites last save on open
- **Description:** Just opening an old version (to test) overwrites the
  most recent save. Should write only `preferences.txt` and `keys.txt`.
- **Demand:** Issue #3510 — closed for 0.9.9 milestone but user reports
  continue in newer Steam threads.
- **Status:** Closed (allegedly fixed); reports continue.
- **Complexity:** **Trivial**.

---

### A6. Engine & performance

#### A6.1 CPU multi-threading
- **Description:** Engine runs on two threads (game + graphics). With
  turrets ratcheting AI load and modern CPUs at 8+ cores, large battles
  bottleneck on a single core.
- **Demand:** Issue #4045 — 7 reactions.
- **Status:** Open.
- **Complexity:** **Engine-rewrite** — concurrency in a tightly-coupled game
  loop is the worst kind of refactor.

#### A6.2 Soft escort limit / landing-pad caps
- **Description:** No fleet cap + late-game capture exploit = overwhelming
  forces that trivialise content. Soft fix: planets have N landing pads;
  bringing more forces management decisions without a hard cap.
- **Demand:** Issue #3857. Cross-references discussion #9949 ("Getting
  powerful ships is too easy").
- **Status:** Open / debated.
- **Complexity:** **Moderate**.

---

### A7. Endgame & content-gap engine hooks

#### A7.1 Active planetary defenses
- **Description:** Lore mentions defended planets but planets are inert in
  combat. Add planet-level hardpoints firing beams/projectiles, scale with
  hull integrity, can be disabled by damage and repair after a day.
- **Demand:** Issue #10286 — 9 reactions.
- **Status:** Open.
- **Complexity:** **Moderate-Hard** — planets need a stripped-down ship-like
  state.
- **Cross-ref:** Pairs naturally with Nova's planetary domination (B14).

#### A7.2 Capture requires-attribute tag
- **Description:** "Uncapturable" is binary. Modders want gated capture:
  "you could not capture a Model 512 without `modified reasoning node`."
  Replaces rigid flag with a flexible requires-list.
- **Demand:** Issue #2948 — 12 reactions.
- **Status:** Open.
- **Complexity:** **Trivial-Moderate**.

#### A7.3 Pacifist storyline routes
- **Description:** Major storylines (Free Worlds, jump-drive acquisition)
  require combat/capture, locking out roleplay non-violent characters. Want
  alt-routes or parallel pacifist arcs.
- **Demand:** Issue #11684; recurring Reddit/Steam.
- **Status:** Open.
- **Complexity:** **Moderate** engine-side (mostly content; engine supports
  condition-gated branches but needs A2.7 to scale).

#### A7.4 Smarter scanning ("fog of war for outfits")
- **Description:** Scanning instantly reveals exact outfit names and stats
  — "the player knows too much, too soon." Until visited at an outfitter
  or boarded, scans should show "X Guns / X Turrets" generically.
- **Demand:** Discussion #7768 — 4 upvotes, 8 comments. Echoed on Steam.
- **Status:** Open discussion.
- **Complexity:** **Moderate**.

#### A7.5 Antimissile status-effect variants
- **Description:** Antimissile turrets only have "antimissile damage" + the
  lock-jamming knob. Modders want slowing-damage and corrosive-damage
  variants — and ideally a radius field-effect form.
- **Demand:** Issue #9958 — 8 reactions.
- **Status:** Open.
- **Complexity:** **Moderate**.

#### A7.6 Inertialess flight (EV Nova parity)
- **Description:** Per-ship or outfit-attribute flight mode that maintains
  speed in facing direction, with reverse-thrust deceleration. EV Nova
  feature with vocal nostalgia constituency.
- **Demand:** Issue #4586 — 8 reactions, milestone 1.0.0.
- **Status:** Open.
- **Complexity:** **Moderate**.

---

### A8. Vision-doc rejected (pure cherry-pick bait)

These are explicit in [Endless Sky's Vision](https://github.com/endless-sky/endless-sky/wiki/Endless-Sky's-Vision)
— they will never land upstream regardless of demand.

#### A8.1 Idle / automation gameplay
- Auto-mining, passive escort trade routes. Vision: "avoid adding features
  which effectively serve to automate gameplay." Frequently mentioned on
  Steam (mining feedback discussion #9888) and Reddit fleet-management
  threads.
- **Complexity:** **Moderate** — AI behaviours + periodic income tick.

#### A8.2 Achievements
- Steam-style achievements ("Visit Every System," "Reach Max Combat
  Rating"). Maintainers reject because the open/moddable game makes them
  trivial to fake.
- **Demand:** Issue #4035 — closed wontfix, 21 comments.
- **Complexity:** **Trivial** (in-game) to **Moderate** (Steam integration).

#### A8.3 Expanded bank / financial system
- Multiple banks per system, interest accounts, reputation-affected rates.
  Vision: economy "intentionally shallow."
- **Demand:** Issue #8008 — closed wontfix, 4 reactions.
- **Complexity:** **Moderate**.

#### A8.4 Hidden-settings GUI / advanced settings page
- Surface `preferences.txt` knobs in a settings panel. Vision opposes
  "drop-down or multi-nested menus."
- **Demand:** Issue #3895 — closed wontfix.
- **Complexity:** **Trivial**.

#### A8.5 Dock for light warships / small freighters / interceptors
- Carrier-style large ships hold not just fighters/drones but small
  freighters/interceptors, deployable with longer delay (Sins-of-a-Solar-
  Empire style). Reduces large-fleet CPU drag, makes capture-and-stash
  safer.
- **Demand:** Issue #4376 — closed wontfix, 3 reactions.
- **Complexity:** **Moderate**.

#### A8.6 Surrender on boarding
- Outnumbered/outgunned enemies surrender, captured into cargo space (no
  bunks needed) instead of killed.
- **Demand:** Issue #9899 — 6 reactions, 22 comments.
- **Status:** **Closed wontfix**.
- **Complexity:** **Moderate**.
- **Cross-ref:** Implements naturally on top of Nova's persona/capture
  loop (B1, B13).

#### A8.7 Plundering internal outfits should require a fight
- Today you can strip a disabled ship's reactors, hyperdrives, batteries
  without crew opposition. Tag internals as `unplunderable` unless ship
  is captured.
- **Demand:** Issue #3346 — 6 reactions, 39 comments.
- **Status:** **Closed wontfix**.
- **Complexity:** **Trivial**.

#### A8.8 Strip visited-only shipyard/outfitter map info
- Map currently shows shipyard inventory for revealed-but-unreachable
  systems. Scope to visited planets only.
- **Demand:** Issue #9050 — closed wontfix, 4 reactions, 9 comments.
- **Complexity:** **Moderate** — MapSalesPanel needs a redesign for
  partial-visit cases.

#### A8.9 Mission/job scripts out of save file
- Save files inline the entire mission script for every available mission,
  bloating saves and confusing error attribution between plugin code vs.
  save state. Store only mission name + mutable variables.
- **Demand:** Issue #11062 — 8 reactions.
- **Status:** Closed not-planned.
- **Complexity:** **Hard** — touches save versioning/compat.

---

## Part B — EV Nova mechanics worth porting

The recurring theme: Nova's data model exposes a 1000-mission, 10,000-bit
boolean state machine with first-class hooks for time, location, ship
class, persona, rank, and outfit-flag intersection. ES expresses much of
the same behavior through `condition` expressions and `npc` blocks, but
several specific abstractions either don't exist or are far less expressive.

### B1. Persistent persons (`përs` resource)
- **What Nova did:** A `përs` defines a named ship — fixed name, fixed (or
  weighted) ship class, custom hail picture, custom hail quote(s), specific
  government, optional system/spöb anchor, optional linked mission. When a
  fleet spawns there is a ~5% roll to substitute one ship with a person
  whose conditions are met; persons can be locked to a system so a known
  character recurs in the same place. Once killed they may stay dead or
  respawn after a delay.
- **What ES does instead:** Scripted `npc` blocks inside individual missions
  and unique fleet ship names, but no first-class "this named NPC roams the
  galaxy until killed, with their own hail portrait and dialogue" entity.
  Recurring villains/allies must be hand-stitched per mission.
- **Why port:** Bounty-target NPCs, recurring rivals, named smuggler
  captains, "Han Solo at the bar," persona-driven ambient flavour — the
  "galaxy feels populated" content Nova plug-ins lived on.
- **Complexity:** **Moderate** — new data type + spawn-substitution hook +
  portrait/quote plumbing.
- **Source:** [Persons (EVN)](https://evn.fandom.com/wiki/Persons_(EVN))

### B2. Ranks (`ränk` resource) with stacking benefits
- **What Nova did:** Stock Nova has 31 rank resources. A rank confers (a) a
  daily salary, (b) outfitter/shipyard price modifiers at affiliated
  planets, (c) a comm-greeting/"salute" override from allied ships, (d)
  eligibility for rank-gated missions, and (e) a per-rank pilotlog title.
  Multiple ranks **stack** — both salaries and discounts add. Ranks bind to
  a government and propagate to allies.
- **What ES does instead:** Reputation per-government and condition-flag
  titles, but no built-in salary/discount/dialogue-modifier object. Price
  modifiers per-government are absent.
- **Why port:** Lets a campaign express "you are now a Knight of the Red
  Branch" with mechanical teeth — automatic income, automatic discount,
  automatic comm flavour — without each plug-in re-implementing it.
- **Complexity:** **Moderate**.
- **Source:** [Ranks (EVN)](https://evn.fandom.com/wiki/Ranks)

### B3. Cron events (`crön` — scheduled / contingent galactic events)
- **What Nova did:** A `crön` is time-bound: `FirstDay/Month/Year` +
  `LastDay/Month/Year` window, `Duration` (active days), `Random` %
  activation roll per eligible day, `EnableOn` bit-test gate, optional
  `Contribute`/`Require` flag intersection, an `OnStart`/`OnEnd` bit-set
  string, and a `Continuous` flag for looping. Lets a plugin say "between
  1178 and 1182, if bit 4002 is set, there's a 10% per-day chance the
  Auroran civil war starts; when it does, set bits X/Y/Z and run for 90
  days."
- **What ES does instead:** `event` blocks fire on conditions, but no
  first-class periodic / windowed / probability-per-day scheduler attached
  to a calendar. Modders fake it with self-rearming missions.
- **Why port:** Galactic-state changes that *just happen* as you play —
  wars erupting, blockades lifting, holidays, shipyards rotating stock —
  without bolting a hidden mission to every event.
- **Complexity:** **Moderate**.
- **Source:** [Nova Control Bits](https://evn.fandom.com/wiki/Nova_Control_Bits)

### B4. Mission `AvailLoc`: bar / shipyard / outfitter / trade as separate offer streams
- **What Nova did:** `AvailLoc` selects which sub-screen of the planet the
  mission appears on: 0=Mission Computer, 1=Bar, 2=Offered from ship (i.e.
  via a `përs`), 3=Spaceport, 4=Trading, 5=Shipyard, 6=Outfit. Each
  sub-screen has its own art and its own mission stream.
- **What ES does instead:** ES has the spaceport "missions" panel and the
  job board. Bar/shipyard/outfitter as distinct mission sources don't
  exist; trading-screen and shipyard-screen mission offers don't exist.
- **Why port:** "The bartender mentions a job," "the outfitter offers a
  smuggling delivery once you buy a stealth scanner," "you meet a buyer in
  the shipyard" — distinct social spaces rather than one big spaceport.
- **Complexity:** **Moderate**.
- **Source:** [Missions (EVN)](https://evn.fandom.com/wiki/Missions_(EVN))

### B5. Multiple landing graphics + descriptions per planet
- **What Nova did:** A `spöb` references separate art for spaceport, bar
  (its own scene with its own description string), shipyard, outfitter,
  and mission computer. Each gets its own painted landing image and prose.
- **What ES does instead:** ES planets have one landscape image plus the
  planet description. Bar/outfitter/shipyard share visual context.
- **Why port:** Massive flavour uplift — the "alien biotech den" outfitter
  feels different from the "back-alley bar" on the same planet. Most of
  Nova's plug-in art was bar scenes.
- **Complexity:** **Moderate** — new spöb fields + UI sub-screen art slots.
- **Source:** EV Nova Bible (spöb section).

### B6. Outfit `Contribute` / `Require` bit-flag intersection
- **What Nova did:** Every outfit, ship, and rank has two 32-bit
  `Contribute` fields summing what the player currently brings. Missions,
  outfits, crons, and shipyard entries declare two `Require` 32-bit fields;
  the engine ANDs them. Lets you express "this mission needs an Atomic
  Cargo Sensor *or* its Polaris equivalent installed," "this hyperlink
  opens with any wormhole stabilizer," "this outfit conflicts with this
  other outfit class."
- **What ES does instead:** ES uses `attributes` and `to offer` condition
  expressions, but the boolean-flag-bag composition (a generic 64-bit
  category bus shared across resources) is absent — modders express each
  capability as a named attribute or condition string.
- **Why port:** Decouples "category of capability" from specific outfit
  names. Lets multiple plug-ins independently grant the same logical
  capability without coordinating attribute strings.
- **Complexity:** **Moderate**.
- **Source:** [Nova Control Bits](https://evn.fandom.com/wiki/Nova_Control_Bits)

### B7. Mission control-bit expression mini-language (silent-start, abort, fail operators)
- **What Nova did:** Every mission carries a tiny expression language for
  test and set. Tests: `Bxxx` (bit lookup), `&` `|` `!` `()`, plus
  shortcuts like `Pxxx` (player has outfit), `Exxx` (mission xxx active),
  `Rxxx` (ship class). Sets: `Bxxx` to set, `!Bxxx` to clear, **`Sxxx`
  (silent-start mission xxx), `Axxx` (abort active mission xxx), `Fxxx`
  (fail active mission xxx)**. Combined with 6 lifecycle hooks
  (`OnAccept`/`OnRefuse`/`OnSuccess`/`OnFailure`/`OnAbort`/`OnShipDone`)
  this lets the engine cascade mission graphs declaratively.
- **What ES does instead:** ES has `to offer`/`to fail`/`to complete` and
  `on offer/accept/decline/visit/complete/fail/defer` actions — similar
  pattern. Gaps: no single operator that *aborts another active mission*
  or *starts another mission silently*; cross-mission control is via
  condition flags the other mission watches. No `OnRefuse`-chained start.
- **Why port:** The abort/fail/silent-start operators make multi-mission
  narrative trees writable as a single declaration rather than a web of
  condition-watching missions.
- **Complexity:** **Moderate** — extends existing condition system with
  cross-mission imperatives.
- **Source:** EV Nova Bible (Mission expressions); [Ambrosia "Nova Control Bits" guide](http://asw.forums.cytheraguides.com/topic/19461/)

### B8. Hidden / silently-spawned missions
- **What Nova did:** A mission can be flagged so it never appears in the
  player's mission list; combined with `AvailRandom 100` and a `Sxxx`
  self-start, modders use these to spawn ambient ships, seed background
  NPCs that hail with quotes, add scripted decorations, or implement state
  machines invisibly.
- **What ES does instead:** ES missions are always visible. Ambient NPC
  injection uses `fleet` definitions + `event`s + `npc` blocks within
  other missions — no first-class "silent mission" abstraction.
- **Why port:** Cleaner authoring of background-only events.
- **Complexity:** **Trivial-Moderate** — a mission flag plus UI suppression.
- **Source:** EV Nova Bible (mission `Flags`).

### B9. Conditional / outfit-gated hyperlinks, hidden hyperlinks, wormhole networks
- **What Nova did:** Hyperlinks between systems can be hidden or
  only-traversable when a `Require` flag matches. Wormholes are far-from-
  centre stellar objects that teleport the player to a paired wormhole.
  Hypergates are per-system spöbs that link to a network and may be gated
  by mission/outfit. The Contribute/Require system can revoke/grant
  hyperlinks dynamically.
- **What ES does instead:** ES has wormholes and hyperdrive-vs-jump
  distinctions, but it doesn't gate ordinary system-to-system hyperlinks
  on outfit possession or conditions. "You discover this hyperlink only
  after completing X" requires a custom `system`/`link` swap event.
- **Why port:** Native support for "jumpdrive Class III opens these
  routes," "the alien hyperlink network is invisible until you install the
  alien navchart," "the war severs this link."
- **Complexity:** **Moderate**.
- **Source:** [Wormhole](https://evn.fandom.com/wiki/Wormhole)

### B10. Mutually-exclusive campaign branches with proper lockout
- **What Nova did:** Nova ships with 6 mostly-exclusive storylines
  (Federation, Rebel, Auroran, Polaris, Vell-os, Pirate). The bit-set
  language is used to lock out others on irreversible commitment: `OnAccept`
  of mission A sets bits that other strings' `AvailBits` test against.
  Refusing a story can be a positive choice (refusing Vell-os pushes you
  to Polaris).
- **What ES does instead:** ES *can* implement campaign exclusivity
  through conditions, and Free Worlds vs Syndicate sort-of-does, but
  there's no formal "campaign" object, no UI to express which path the
  player has chosen, and ES missions tend to favour convergent storylines.
- **Why port:** First-class "campaign" concept (a named track that owns its
  own bit pool, has a start condition, can declare incompatible siblings)
  makes branching content easier to author and easier for the player to
  track.
- **Complexity:** **Moderate** — largely UX over existing condition layer.
- **Source:** [Storyline](https://evn.fandom.com/wiki/Storyline)
- **Cross-ref:** Pairs with A5.2 (campaign progress visibility) — together
  they constitute a real "campaign" subsystem.

### B11. Weapon `Guidance` types ES doesn't model cleanly
- **What Nova did:** Nova's `wëap` Guidance enum includes: unguided,
  turreted unguided, homing missile, turreted homing, **beam** (continuous
  ray with `BeamLength`/`BeamWidth`), **front-quadrant** (only valid in 90°
  forward arc), **bay** (deploys a fighter), **submunitions** (a projectile
  that on detonation spawns N child projectiles of another `wëap` ID,
  recursively, with their own Guidance), **point-defense** (auto-targets
  enemy projectiles), and **proximity detonator** (explodes within radius
  of any ship). Plus `ProxRadius`, `ProxSafety`, `Smoke`, `Spin`,
  `Inaccuracy`, `IonizeAmount`, `JamVuln1-4` (frequency-band ECM).
- **What ES does instead:** ES has homing, turret, anti-missile,
  fragmentation, and submunitions exists as a key. Gaps: proper
  beam-as-projectile-with-length, front-quadrant restriction, proximity
  fuze with safety arming distance, ionize/jam frequency bands (not just
  one "ion damage" stat), bay-launch deploys (a *weapon* that launches a
  fighter doesn't exist).
- **Why port:** Multi-stage missiles (MIRVs that split into beams or
  guided bomblets), prox mines, fighter-launching missile pods,
  frequency-band ECM creating rock-paper-scissors with countermeasures.
- **Complexity:** **Moderate-Hard** — weapon engine extensions, but
  localised.
- **Cross-ref:** Pairs with A3.7 (weapon warmup/charge) and A7.5
  (antimissile variants) for a coherent "expand the weapon-modelling
  vocabulary" theme.
- **Source:** [Weapons](https://evn.fandom.com/wiki/Weapons)

### B12. `OnShipDone` mission objective with `AuxShip` + scan-fail
- **What Nova did:** A mission can declare a single `ShipGoal` —
  destroy/disable/board/escort/observe/scan/rendezvous a special-flagged
  ship — with its own `ShipDoneText` shown on completion. The ship can be
  marked `AuxShip` (cosmetic, can't hit player and player's shots can't
  hit it). Missions can require the player to *not be scanned* while
  carrying mission cargo (illegal-cargo missions auto-fail on scan).
- **What ES does instead:** ES has `npc` blocks with `assist`, `disable`,
  `board`, `kill`, `evade` etc. — fairly close. Gaps: no "fail on scan
  while carrying," no clean `AuxShip`-style "decorative NPC the player
  physically can't damage even by accident," no built-in `ShipDoneText` /
  atomic completion hook for the *board* objective specifically.
- **Why port:** Smuggling missions with proper customs-scan failure,
  parade/escort missions where players can't accidentally TPK the
  dignitary, scanning/observation objectives.
- **Complexity:** **Trivial-Moderate**.

### B13. Capturable named NPCs / boarding-yields-crew
- **What Nova did:** Boarding rolls capture chance based on relative crew,
  ship strength, escort presence, Marine Platoons. Captured ships join your
  fleet. Specific `përs` ships can be boarded for unique outcomes (special
  cargo, mission triggers, dialog with the captured captain). Escape Pod
  outfit (and most fighters) saves the player.
- **What ES does instead:** ES has capturable ships and crew/marine
  mechanics — broadly equivalent. Gap: capturing a *named* NPC and that
  triggering specific mission state / dialog isn't supported cleanly
  because there are no persistent named NPCs (B1).
- **Why port:** Bounty/intercept missions where capturing the target alive
  yields different outcomes than killing them; rescue missions where
  boarding a specific disabled friendly carries them to safety.
- **Complexity:** **Trivial** once B1 lands.
- **Source:** [Boarding](https://evn.fandom.com/wiki/Boarding)

### B14. Planetary domination ("demand tribute")
- **What Nova did:** With combat rating ≥ Deadly, hailing a planet exposes
  a "Demand Tribute" option. Planet launches its `DefenseFleet` (up to 120
  ships in waves of 6, drawn from per-planet `DefenseDude` and
  `DefenseCount`); defeating the waves and re-hailing places the planet
  under your control, paying a daily `Tribute` credit amount. Domination
  flags you criminal across all same-government planets.
- **What ES does instead:** Nothing equivalent. Conquest is narrative-only
  (Free Worlds doesn't transfer planet ownership as a revenue source).
- **Why port:** End-game playstyle: warlord run. Trivial loop, huge replay
  value, organic galactic-reputation crisis.
- **Complexity:** **Moderate**.
- **Cross-ref:** Pairs with A7.1 (active planetary defenses) — together
  they make planet sieges a real game mode.
- **Source:** [Planetary Domination](https://evn.fandom.com/wiki/Planetary_Domination)

### B15. Per-system / per-government music + voice tracks
- **What Nova did:** Governments declare a `VoiceType` (0-7, with even/odd-
  only modes) selecting which subset of comm-chatter sounds their ships
  emit; -1 silences a government entirely. Music tracks similarly bind by
  system and by government context — flying into Auroran space changes
  the soundscape.
- **What ES does instead:** Essentially no music system. One of the
  longest-standing open requests (issues #233, #3594, discussion #10824).
  Ambient sound per-system is also limited.
- **Why port:** Establishes the "you crossed into hostile space" / "you're
  at a starport" auditory cue that defined Nova's atmosphere. Even just a
  `music` field on `system` and `government` would unblock modders.
- **Complexity:** **Moderate** — audio mixing/streaming infra; spec is
  trivial.
- **Cross-ref:** ES community has been asking for music for ~10 years
  (#233). This is one of the highest-ROI Nova ports.
- **Source:** [Issue #233](https://github.com/endless-sky/endless-sky/issues/233);
  EV Nova Bible (`gövt` VoiceType).

### B16. Subtitled inflight comm-chatter (combat barks)
- **What Nova did:** Ships emit barks/quotes as they fight, taunt,
  surrender, or get scanned. Each bark has portrait, subtitle text, and
  sound. Persons have unique quote pools; governments have generic pools.
  Player sees portrait + line in HUD as enemies trash-talk in real time.
- **What ES does instead:** Hails appear only on the hailing screen;
  hostile target chatter is minimal. No subtitle-with-portrait HUD layer
  for combat barks.
- **Why port:** Massive flavour multiplier — combat *feels* populated when
  ships taunt and surrender in their own voices.
- **Complexity:** **Moderate** — HUD layer + bark-pool data type.
- **Cross-ref:** Pairs with A2.1 (conditional hails) — the same data layer
  serves both.
- **Source:** [Sounds (EVN)](https://evn.fandom.com/wiki/Sounds_(EVN))

### B17. `shän` sprite layers: separate engine-glow, weapon-glow, lights, shield overlays
- **What Nova did:** A `shän` resource has independent layered sprite IDs
  for: base hull (with mask), engine glow (additive blend, fades on
  throttle), weapon glow (additive, decays per `WeapDecay` after firing),
  running lights, shield-hit overlay, and damaged/disabled variants. Frame
  counts (usually 36) define rotation steps independently per layer.
- **What ES does instead:** ES sprites are flat — engine flares are
  positioned via hardpoints (separate small sprites) — but the per-frame
  additive engine-glow / weapon-flash overlay system isn't part of the
  ship sprite spec; you bake it into the base sprite or use an attached
  effect.
- **Why port:** Lets ship art breathe (engines pulse, weapons flash, lights
  blink) without spriting every frame manually. Trivial for modders to
  author once `shän` exists; near-impossible to replicate in vanilla ES.
- **Complexity:** **Moderate** — sprite render pipeline change.
- **Source:** [Sprite](https://evn.fandom.com/wiki/Sprite)

### B18. `düde`: ship-class pool + AI personality + government as a reusable unit
- **What Nova did:** A `düde` is a logical "type of NPC" bundling up to 16
  weighted ship classes, an AI type (trader/warship/interceptor/courier/
  fearless/coward), a government, and behaviour flags ("can't hit / be hit
  by player," "always shoots first," "always boards," "won't fight").
  `flët` resources reference dudes. This is the unit of NPC-population
  composition.
- **What ES does instead:** ES has `fleet` definitions with `variant`
  weighted ship lists and a `government` — closer than people give it
  credit for. But Nova's first-class **AI personality** field on the dude
  (separate from government) and the **behaviour flags** (especially the
  can't-hit-player flag for cinematic NPCs) are richer.
- **Why port:** Cleaner separation of "who they are" (government) from
  "how they behave" (AI) from "what hulls they fly" (ships). Useful for
  civilian/military splits within one faction without inventing
  sub-governments.
- **Complexity:** **Moderate**.
- **Cross-ref:** Pairs with A4.2 (escort personality UI) — same AI
  personality vocabulary on both sides.
- **Source:** [Escorts](https://evn.fandom.com/wiki/Escorts)

### B19. `chär` records: multiple starting characters / scenarios
- **What Nova did:** A `chär` defines a starting pilot — starting ship,
  outfits, credits, system, cargo, bits set, intro cutscene picture+text,
  display name on the new-pilot screen. A plug-in can ship multiple `chär`
  records, letting the player choose between scenarios.
- **What ES does instead:** ES has a single `start` (extended via `start`
  blocks for limited customisation); multi-start is being slowly added.
  Modders can't easily ship "pick your origin: Pirate / Federation Cadet
  / Refugee" with starting state diverging meaningfully.
- **Why port:** "Choose your beginning" is a major modding hook. Nova
  plug-ins like Polycon/Anathema use it to drop the player straight into
  the story they wrote.
- **Complexity:** **Moderate**.
- **Source:** EV Nova Bible (`chär`).

### B20. Cargo missions with explicit deadlines + early-payment escalator + cascading failure
- **What Nova did:** Mission resources have `TimeLimit` (days),
  `DatePostInc` (escalator on payout for early delivery), `Cargo`/
  `CargoQty`, `PickupMode` (auto vs requires landing), and explicit
  `OnFailure` bit-sets when the deadline expires (separate from
  `OnAbort`). Missing a deadline can cascade into reputation loss,
  mission-string failure, or counter-missions.
- **What ES does instead:** ES has `deadline` and `on fail` — broadly
  equivalent. Gap: no early-completion bonus escalator built into the
  payment model; cascading "missing this triggers mission Y" requires
  manual condition wiring.
- **Why port:** Economic depth — courier work that actually rewards being
  fast vs. slow; cascading reputational consequences expressed
  declaratively.
- **Complexity:** **Trivial-Moderate**.
- **Source:** [Missions (EVN)](https://evn.fandom.com/wiki/Missions_(EVN))

### Honourable mentions
- **Vell-os ships as mind-projections** — Nova's Vell-os storyline gives
  the player no ship; the "ship" is a manifestation of telepathic strength
  (rank T6 → T0), and outfits are mental disciplines. Engine-wise this is
  creative use of B2/B6/the rank+contribute system; called out so the fork
  knows the existing primitives are powerful enough to express it.
- **Outfit purchase limits per-shop / per-pilot** — Nova outfits have `Max`
  (per-pilot) and per-spöb stock limits.
- **Pilotlog title/auto-text** — Nova generates flavour text in the pilot
  file as you complete arcs and gain ranks. UI/social-share feature ES
  could ape easily.
- **QuickTime cutscene hooks on missions** — Nova allows mission `OnAccept`
  / `OnSuccess` to trigger cutscene playback as part of the dialog flow.

---

## Part C — Synthesis & recommended starting tier

### Convergent themes (where ES community ask + Nova mechanic align)

1. **"Make the galaxy feel populated."** A2.1 (conditional hails) +
   B1 (persons) + B16 (combat barks) + B18 (dude AI personality) form a
   single coherent feature set. The data layer is ~one new resource type
   plus a HUD overlay; the content payoff is enormous.

2. **"Modders need real authoring power."** A2.5 (Lua) + A2.2 (action-based
   data format) + A2.7 (conditional properties) + B7 (silent-start /
   cross-mission abort operators) + B6 (Contribute/Require flag bus). Each
   is a force multiplier — the ROI is in the *combination*, since each
   unblocks plug-ins that would otherwise need engine work for every
   mechanic.

3. **"Boarding/capture should be a real subsystem."** A3.8 (boarding
   rework) + A8.6 (surrender) + A8.7 (plunder requires fight) + B13
   (capturable named NPCs) + A7.2 (capture requires-attribute). All
   wontfix or stuck upstream; together they make boarding a tactical
   pillar instead of an afterthought.

4. **"Planet sieges as a game mode."** A7.1 (active planetary defenses)
   + B14 (planetary domination). Either alone is half a feature; together
   they're a complete end-game loop with content hooks.

5. **"Music + audio atmosphere."** A1.7-cross + B15 (per-system /
   per-government music) + B16 (combat barks). The biggest sensory upgrade
   possible to ES, blocked by an absent audio subsystem more than by
   design disagreement.

6. **"Branching narrative as first-class."** A5.2 (campaign progress
   visibility) + B10 (mutually-exclusive campaigns) + B7 (cross-mission
   imperatives) + B19 (multiple `chär` starts). Together they let plug-in
   authors ship Nova-style 6-ending arcs with proper player-facing UX.

7. **"Smarter combat pacing."** A3.1 (missile-spammer flee) + A3.2
   (pirate suicide) + A3.3 (reinforcements never stop) + A3.4 (delay
   inbound spawns) + A3.5 (allied AI griefing) + A3.6 (fighter strafing).
   All independent, all stuck. Can be tackled incrementally as a
   "combat-pacing pass."

### Recommended starting tier

If treating this as a roadmap, here's a defensible build order. Numbering
is suggestion order, not priority:

**Tier 1 — Quick wins (Trivial, days each)**

1. A1.2 hotkey profiles, A1.3 mouse buttons, A1.4 reset controls,
   A1.5 fast-forward speed, A1.6 flagship colour — basic UX hygiene
2. A3.3 reinforcement spawn cap, A3.4 inbound-link spawn delay — combat
   feel
3. A4.5 jump-drive arrival clustering, A4.7 fleet review pre-launch,
   A4.8 unpark-all-local — fleet QoL
4. A5.4 don't overwrite save on open, A8.7 unplunderable internals
5. **B8 hidden missions, B12 OnShipDone + scan-fail, B20 deadline
   escalator** — small modding wins from Nova

**Tier 2 — Feature-set foundations (Moderate, weeks)**

6. **B15 music subsystem** — unblocks 10 years of feature requests with
   one change
7. **A2.1 conditional hails + B16 combat barks + B1 persons** — together,
   the "populated galaxy" theme
8. A2.6 mission time-pass / teleport, A2.8 per-outfitter licenses — modder
   leverage
9. B2 ranks, B19 multi-`chär` starts — campaign authoring tools
10. A7.6 inertialess flight (EV Nova parity item with vocal demand)

**Tier 3 — Big swings (Hard / Engine-rewrite, months)**

11. **A2.5 Lua scripting** — single biggest leverage item in the entire
    list, but biggest project
12. A3.8 boarding rework + A8.6/A8.7 + B13 — boarding as a real subsystem
13. **B7 cross-mission imperative operators + A2.7 conditional properties**
    — narrative authoring
14. A3.10 stealth + A3.11 smuggling overhaul — emergent playstyles
15. A7.1 active planetary defenses + B14 planetary domination — end-game
    loop
16. A6.1 multi-threading — only if perf becomes a real bottleneck

**Tier 4 — Strategic but research-heavy**

17. A2.4 in-game plugin browser (depends on a registry — may be a separate
    project)
18. B17 `shän` sprite layers (rendering pipeline change with content
    knock-on)
19. B11 Nova weapon Guidance types (each subtype is its own rabbit hole;
    pick one or two — submunitions and prox detonators are highest
    payoff)

### What to skip

- A1.10 game-over screen — nice to have, but doesn't unlock anything.
- A4.4 multi-category bays — useful but cosmetic.
- A8.2 achievements — Vision rejected for valid reasons (mod-trivial).
- A8.3 expanded banking — outsized work for a niche playstyle.
- B5 multiple landing graphics — beautiful but expensive content debt for
  every existing planet.
- Delta's Pylons (separate doc) — invasive content rewrite, fold in
  later if the fork wants Delta's combat feel.

---

## Sources

### Endless Sky community

- High-reaction open issues: <https://github.com/endless-sky/endless-sky/issues?q=is%3Aissue+is%3Aopen+sort%3Areactions-%2B1-desc>
- Discussions / Ideas: <https://github.com/endless-sky/endless-sky/discussions/categories/ideas?discussions_q=category%3AIdeas+sort%3Atop>
- Vision doc: <https://github.com/endless-sky/endless-sky/wiki/Endless-Sky's-Vision>
- StyleGoals: <https://github.com/endless-sky/endless-sky/wiki/StyleGoals>
- Steam Feature Requests board: <https://steamcommunity.com/app/404410/discussions/2/>
- HN: <https://news.ycombinator.com/item?id=38419015>
- Steambase reviews summary: <https://steambase.io/games/endless-sky/reviews>

### EV Nova

- EV Nova Bible (canonical, by Matt Burch / ATMOS):
  <https://andrews05.github.io/evstuff/guides/evnbible.html>
- EVN Wiki Bible page: <https://evn.fandom.com/wiki/Nova_Bible>
- Persons: <https://evn.fandom.com/wiki/Persons_(EVN)>
- Ranks: <https://evn.fandom.com/wiki/Ranks>
- Missions: <https://evn.fandom.com/wiki/Missions_(EVN)>
- Nova Control Bits: <https://evn.fandom.com/wiki/Nova_Control_Bits>
- Wormhole: <https://evn.fandom.com/wiki/Wormhole>
- Weapons: <https://evn.fandom.com/wiki/Weapons>
- Boarding: <https://evn.fandom.com/wiki/Boarding>
- Storyline (6 storylines / lockout): <https://evn.fandom.com/wiki/Storyline>
- Planetary Domination: <https://evn.fandom.com/wiki/Planetary_Domination>
- Sprite (`shän`): <https://evn.fandom.com/wiki/Sprite>
- Sounds: <https://evn.fandom.com/wiki/Sounds_(EVN)>
- Vell-os: <https://evn.fandom.com/wiki/Vell-os>
- Steam thread "Endless Sky vs EV Nova": <https://steamcommunity.com/app/404410/discussions/0/458606248619279472/>
- ES community port project (lots of comparative notes): <https://github.com/edelventhal/EVNToEndlessSky>
- ES Issue #233 (music): <https://github.com/endless-sky/endless-sky/issues/233>
- ES Discussion #10824 (dynamic soundscape): <https://github.com/endless-sky/endless-sky/discussions/10824>
- "[GUIDE] Nova Control Bits" Ambrosia forum archive: <http://asw.forums.cytheraguides.com/topic/19461/>
- `vasi/evnova-utils` (machine-readable Nova resource decoder, most accurate
  field reference): <https://github.com/vasi/evnova-utils/tree/master/Scripts/lib/Nova/Old>
