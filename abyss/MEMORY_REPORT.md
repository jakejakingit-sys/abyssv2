# Memory report

**Host x86-64 numbers. NOT a calculator link-map measurement.**

```
memory (HOST sizeof - not a measurement of the calculator build)
  sizeof Game=2192 Player=28 LevelState=1888 SaveData=100  save file=107 bytes
  level data: 21270 bytes for 20 levels (archived AppVars, read in place); only ONE level (<= 1440 bytes) is copied to RAM

```

| Item | Bytes | Where |
|---|---|---|
| ABYSSCOR (common sprites + palettes) | 33238 | archive (AppVar limit 65,535) |
| ABYSSBOS (guardians + logo) | 37391 | archive |
| ABYSSLV1..4 (levels) | 5158 / 5107 / 5752 / 5959 | archive |
| Sprite cache (tiles, player incl. upside-down set, creatures, HUD) | ~26,000 | RAM, malloc; falls back to flash if it fails |
| Current level + flags, Game state, wall ids | see sizeof above + 1.4 KB | RAM |
| Save file | 107 | archive, written on events only |
Not measured: code size, real RAM use, FPS.
