# ABYSS — TI-84 CE side-scrolling dungeon adventure

**All 20 levels, 4 worlds and 4 guardians are built, with an ending.** Not yet implemented: gameplay for 6 of the 7 abilities (they unlock but do nothing), sound, relics, checkpoints.

## Send ALL of these to the calculator (the data files changed — resend every one)
| File | What | Where |
|---|---|---|
| `ABYSS.8xp` | the program | RAM |
| `ABYSSCOR.8xv` | common sprites + palettes | archive (the game archives it itself if you send it to RAM) |
| `ABYSSBOS.8xv` | guardian sprites + logo | archive |
| `ABYSSLV1..4.8xv` | levels of worlds 1–4 | archive |
CI builds them all (`make` + `make appvars`) and uploads one artifact. A missing/damaged data file shows a message instead of crashing.

## Memory model
* **Archive (read in place):** all levels and the big/rare sprites. Each AppVar is CRC-checked at start; pointers are re-acquired after every archive write.
* **RAM:** current level copy (≤ 1.4 KB), player/entity state, save struct, particle pool, wall-sprite ids, and a **sprite cache** (tiles, player, creatures, HUD copied once at start because flash is slow to read; falls back to flash if `malloc` fails).
* Graphics are paced by a governor (`MODE` key: HIGH/MEDIUM/LOW at 60 fps, then 30 fps presets).

## Controls
← → move · **2ND** jump (hold = higher) · ↑↓ ladders, ↓ drops through ledges · **ALPHA** Rune Pulse (lights runes within 4 tiles) · **MODE** graphics preset · ENTER confirm · CLEAR back/pause.

## The campaign
| World | Levels | Guardian | New hazards |
|---|---|---|---|
| 1 Forgotten Crypt | The Entrance · Forgotten Halls · Burial Grounds · Bone Chambers · Bone King's Throne | **Bone King** | spikes, crumbling floors, moving platforms, runes + gates |
| 2 Sunken Mines | Mine Entrance · Crystal Caverns · Collapsing Tunnels · Deep Shafts · Broodmother's Nest | **Broodmother** (crystal shards) | long crumbling bridges, ladder shafts |
| 3 Infernal Depths | Ashen Path · Burning Halls · Magma Bridge · Furnace Chambers · Inferno's Arena | **The Inferno** (embers) | lava, fire vents (telegraphed), timed platforms, ember rain |
| 4 The Void | Forgotten Gate · Shattered Reality · Lost Kingdom · Abyssal Core · The Forgotten One | **The Forgotten One** (5 runes, void shards) | gravity-flip zones, teleport pads, disappearing platforms |
Guardians are non-violent: dodge the telegraphed falling hazards, climb to the runes, light them with Rune Pulse, the gate opens. Finishing level 20 plays the ending.

## Verification status — read this
* **Verified (real gcc): `make -f Makefile.host` — 753 checks, 0 failures.** Physics, every mechanic (lava, vents, timed platforms, teleport, gravity, rain, crumble, runes, creatures, guardians incl. the 5-rune final), saves (every bit-flip detected), pack format/CRC, RAM/archive separation, frame-pacing governor, structure + reachability of all 20 levels, and physics searches that cross every moving-platform gap and every timed-platform sequence from all start phases.
* **Not verified by the author:** the calculator code (`main.c`, `platform_ce.c`, `convbin` rule) has been compiled by CI and run by you in earlier versions, but THIS version's new screens/effects have not run on hardware. Reachability is a heuristic plus physics searches, not a proof — if a jump is impossible, tell me the level and spot. Difficulty/balance of Worlds 3–4 is untested by a human.
* Not measured: frame rate, RAM use on device.

## Not implemented
Heavy Landing / Phase Dash / Protective Parry / Ground Wave / Whirlwind / Abyssal Resonance (unlock by level, no gameplay) · sound · relics · checkpoints · crouch/dash/death/victory animations · per-world background art beyond palette swaps.

## Build
`make -f Makefile.host` tests (regenerates sprites/levels) · `make` program · `make appvars` AppVars. Levels: `levels/world*/*.txt` (legend in `tools/levelkit.py`); authoring scripts `tools/author_world*.py`.
