#include <stdio.h>
#include <string.h>
#include "game.h"
#include "upgrades.h"
#include "abilities.h"
#include "animation.h"
#include "assets.h"
#include "platform.h"
#include "pacing.h"
static int fails, checks;
#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); } } while (0)
static Input NONE;

static void run_until_ground(Player *p, LevelState *l) { for (int i = 0; i < 200 && !p->ground; i++) player_step(p, l, NONE); }

static void test_physics(void) {
  puts("physics");
  LevelState l; level_load(&l, 0, 0); Player p; player_spawn(&p, l.def);
  run_until_ground(&p, &l);
  CHECK(p.ground);
  CHECK(floor_div(p.y + PH * SUBPX, TILE_SUB) == 13);          /* resting exactly on floor top (row 13) */
  /* acceleration + max speed */
  Input r = {.right = true};
  for (int i = 0; i < 30; i++) player_step(&p, &l, r);
  CHECK(p.vx == MAXV);
  for (int i = 0; i < 20; i++) player_step(&p, &l, NONE);
  CHECK(p.vx == 0);                                              /* deceleration */
  /* variable jump: tap vs hold */
  Player a = p, b = p; int ya = a.y, yb = b.y, topa = a.y, topb = b.y;
  Input jh = {.jump = true};
  player_step(&a, &l, jh); player_step(&b, &l, jh);
  for (int i = 0; i < 60; i++) { player_step(&a, &l, NONE); player_step(&b, &l, jh); if (a.y < topa) topa = a.y; if (b.y < topb) topb = b.y; }
  CHECK(ya - topa < yb - topb);                                  /* held jump goes higher */
  CHECK((yb - topb) / SUBPX >= 36 && (yb - topb) / SUBPX <= 56); /* ~3 tiles, matches reachability rule */
  /* coyote time: walk off ledge then jump within window */
  LevelState l2; level_load(&l2, 0, 0); Player c; player_spawn(&c, l2.def);
  c.x = 14 * TILE_SUB - 4 * SUBPX; run_until_ground(&c, &l2);   /* near pit edge (cols 15-17 are the pit) */
  CHECK(c.ground);
  for (int i = 0; i < 40 && c.ground; i++) player_step(&c, &l2, (Input){.right = true});
  CHECK(!c.ground);
  int vy_before = c.vy;
  player_step(&c, &l2, (Input){.right = true, .jump = true});
  CHECK(c.vy < vy_before);                                       /* jumped while airborne: coyote works */
  /* jump buffering: press jump just before landing */
  Player d; player_spawn(&d, l.def); d.y -= TILE_SUB; d.vy = 40;
  bool jumped = false;
  for (int i = 0; i < 40; i++) { player_step(&d, &l, (Input){.jump = (i == 1)}); if (d.vy < -50) jumped = true; }
  CHECK(jumped);
  /* wall: left boundary */
  Player w; player_spawn(&w, l.def); run_until_ground(&w, &l);
  for (int i = 0; i < 200; i++) player_step(&w, &l, (Input){.left = true});
  CHECK(w.x >= TILE_SUB);                                        /* never inside the border wall */
  /* no tunnelling at terminal velocity */
  Player t; player_spawn(&t, l.def); t.x = 5 * TILE_SUB; t.y = TILE_SUB + 1; t.vy = MAXFALL;
  for (int i = 0; i < 200; i++) player_step(&t, &l, NONE);
  CHECK(t.ground && t.y < 14 * TILE_SUB);
  /* one-way platform: land from above, pass from below, drop through */
  LevelState l3; level_load(&l3, 1, 0); Player o; player_spawn(&o, l3.def);
  o.x = 17 * TILE_SUB; o.y = 6 * TILE_SUB; o.vy = 0;           /* above the row-10 platform (cols 16-19) */
  run_until_ground(&o, &l3);
  CHECK(o.ground && floor_div(o.y + PH * SUBPX, TILE_SUB) == 10);
  player_step(&o, &l3, (Input){.down = true});
  for (int i = 0; i < 15; i++) player_step(&o, &l3, NONE);
  CHECK(floor_div(o.y + PH * SUBPX, TILE_SUB) > 10);           /* dropped through */
  Player u; player_spawn(&u, l3.def); u.x = 17 * TILE_SUB; u.y = 12 * TILE_SUB - 3 * SUBPX; u.vy = JUMPV;
  int mn = u.y; for (int i = 0; i < 20; i++) { player_step(&u, &l3, NONE); if (u.y < mn) mn = u.y; }
  CHECK(mn < 10 * TILE_SUB);                                     /* rose through platform from below */
  /* ladder */
  Player k; player_spawn(&k, l.def); k.x = 30 * TILE_SUB + 3 * SUBPX; k.y = 12 * TILE_SUB - 4; run_until_ground(&k, &l);
  for (int i = 0; i < 20; i++) player_step(&k, &l, (Input){.up = true});
  CHECK(k.ladder == 1 && anim_player_frame(&k, 0) >= FR_CLIMB0);  /* on ladder: climbing frame */
  for (int i = 0; i < 100; i++) player_step(&k, &l, (Input){.up = true});
  CHECK(k.y < 8 * TILE_SUB);                                     /* climbed up to ledge at row 6 */
}

static void test_collision_hazards(void) {
  puts("collision/hazards");
  LevelState l; level_load(&l, 0, 0);
  CHECK(level_tile(&l, -1, 5) == T_SOLID && level_tile(&l, 5, -1) == T_SOLID && level_tile(&l, 5, 99) == T_AIR);
  Game g; SaveData s; save_default(&s); game_start_level(&g, &s, 0);
  for (int i = 0; i < 20; i++) game_update(&g, NONE);
  g.p.x = 26 * TILE_SUB; g.p.y = 12 * TILE_SUB - 2 * SUBPX; g.p.vy = 0;
  uint8_t ev = 0; for (int i = 0; i < 5; i++) ev |= game_update(&g, NONE);
  CHECK(ev & EV_HURT); CHECK(g.hp == 80);                       /* 20 dmg, 0 armor */
  int hp = g.hp; g.invuln = 30; g.p.x = 26 * TILE_SUB; g.p.y = 12 * TILE_SUB; game_update(&g, NONE);
  CHECK(g.hp == hp);                                             /* damage cooldown */
  /* regen interrupted then resumes */
  g.invuln = 0; g.p.x = 3 * TILE_SUB; g.p.y = 12 * TILE_SUB; g.regen_wait = 120; hp = g.hp;
  for (int i = 0; i < 100; i++) game_update(&g, NONE);
  CHECK(g.hp == hp);
  for (int i = 0; i < 200; i++) game_update(&g, NONE);
  CHECK(g.hp > hp);
  /* armour reduces damage */
  save_default(&s); s.upg[UPG_ARMOR] = 3; game_start_level(&g, &s, 0);
  g.p.x = 26 * TILE_SUB; g.p.y = 12 * TILE_SUB - 2 * SUBPX; for (int i = 0; i < 5; i++) game_update(&g, NONE);
  CHECK(g.hp == 100 - (20 - 6));
}

static void test_progression_treasure(void) {
  puts("progression/treasure/death");
  SaveData s; save_default(&s); Game g; game_start_level(&g, &s, 0);
  CHECK(s.unlocked == 0);
  /* collect first coin */
  g.p.x = 8 * TILE_SUB + 3 * SUBPX; g.p.y = 12 * TILE_SUB; game_update(&g, NONE);
  CHECK(s.coins == COIN_VALUE); CHECK(s.treasure[0] != 0);
  /* same coin cannot be re-collected after re-entry */
  uint32_t c = s.coins; game_start_level(&g, &s, 0);
  g.p.x = 8 * TILE_SUB + 3 * SUBPX; g.p.y = 12 * TILE_SUB; game_update(&g, NONE);
  CHECK(s.coins == c);
  /* coins + treasure persist through death; death restarts level */
  g.hp = 0; uint8_t ev = game_update(&g, NONE);
  CHECK(ev & EV_DEAD); CHECK(s.deaths == 1); CHECK(s.coins == c); CHECK(g.hp == 100);
  CHECK(level_tile(&g.ls, 8, 12) == T_AIR);
  /* falling in a pit kills */
  g.p.x = 16 * TILE_SUB; g.p.y = 14 * TILE_SUB; ev = 0; for (int i = 0; i < 60 && !(ev & EV_DEAD); i++) ev |= game_update(&g, NONE);
  CHECK(ev & EV_DEAD);
  /* exit completes + unlocks next */
  g.p.x = 57 * TILE_SUB; g.p.y = 12 * TILE_SUB; ev = game_update(&g, NONE);
  CHECK(ev & EV_WIN); CHECK(s.completed & 1); CHECK(s.unlocked == 1);
}

static void test_upgrades(void) {
  puts("upgrades");
  SaveData s; save_default(&s); s.coins = 49;
  CHECK(upgrade_buy(&s, UPG_HEALTH) == BUY_NO_COINS && s.coins == 49 && s.upg[UPG_HEALTH] == 0);
  s.coins = 50; CHECK(upgrade_buy(&s, UPG_HEALTH) == BUY_OK); CHECK(s.coins == 0 && upgrade_max_hp(&s) == 120);
  CHECK(upgrade_cost(UPG_HEALTH, 1) > upgrade_cost(UPG_HEALTH, 0));          /* rising prices */
  s.coins = 100000;
  for (int c = 0; c < UPG_COUNT; c++) { for (int i = 0; i < 20; i++) upgrade_buy(&s, c); CHECK(s.upg[c] == UPG_MAX_LEVEL); CHECK(upgrade_buy(&s, c) == BUY_MAXED); }
  CHECK(s.coins <= 100000);                                                   /* never negative (unsigned) / never grows */
  CHECK(upgrade_cost(UPG_ARMOR, UPG_MAX_LEVEL) == -1);
  CHECK(upgrade_max_hp(&s) == 200 && upgrade_max_energy(&s) == 200);
}

static void test_abilities(void) {
  puts("abilities");
  CHECK(abilities_for_level(0) == 1);
  CHECK(abilities_for_level(1) == 1);
  CHECK(abilities_for_level(2) == 3);       /* Heavy Landing at level 3 */
  CHECK(abilities_for_level(19) == 127);
  SaveData s; save_default(&s); Game g; game_start_level(&g, &s, 1);
  CHECK(s.abilities == 1);
}

static void test_save(void) {
  puts("save/load/corruption");
  SaveData a; save_default(&a); a.coins = 1234; a.unlocked = 3; a.completed = 0x7; a.upg[0] = 2; a.treasure[1] = 0xA5; a.deaths = 9; a.abilities = 7;
  uint8_t buf[SAVE_BYTES]; save_serialize(&a, buf);
  SaveData b; CHECK(save_parse(&b, buf, SAVE_BYTES) == SAVE_OK); CHECK(memcmp(&a, &b, sizeof a) == 0);
  for (int i = 0; i < SAVE_BYTES; i++) {                          /* every single-bit flip must be rejected */
    for (int bit = 0; bit < 8; bit++) {
      uint8_t c[SAVE_BYTES]; memcpy(c, buf, sizeof c); c[i] ^= 1 << bit;
      SaveData t; if (save_parse(&t, c, SAVE_BYTES) == SAVE_OK) { fails++; checks++; printf("  FAIL undetected flip byte %d bit %d\n", i, bit); }
    }
  }
  CHECK(save_parse(&b, buf, SAVE_BYTES - 1) == SAVE_BAD_LEN);
  uint8_t bad[SAVE_BYTES]; memcpy(bad, buf, sizeof bad); bad[4] = 99;
  CHECK(save_parse(&b, bad, SAVE_BYTES) == SAVE_BAD_VERSION);
  SaveData r; memset(&r, 0xEE, sizeof r); bad[10] ^= 0xFF;
  CHECK(save_load_or_default(&r, bad, SAVE_BYTES) != SAVE_OK); CHECK(r.coins == 0 && r.unlocked == 0 && r.abilities == 1);  /* recovered */
  CHECK(save_load_or_default(&r, NULL, 0) != SAVE_OK);
  /* valid CRC but out-of-range content is still rejected */
  SaveData evil = a; evil.unlocked = 200; save_serialize(&evil, buf); CHECK(save_parse(&b, buf, SAVE_BYTES) == SAVE_BAD_RANGE);
  CHECK(crc16((const uint8_t *)"123456789", 9) == 0x29B1);          /* CRC-16/CCITT-FALSE check value */
}

/* ---- level validation: dimensions, spawn, exit, reachability (heuristic), treasure, mechanics ---- */
static bool floorish(uint8_t t) { return t == T_SOLID || t == T_ONEWAY || t == T_CRUMBLE || t == T_VENT || t == T_TIMED; }
static bool wallish(uint8_t t) { return t == T_SOLID || t == T_CRUMBLE || t == T_VENT; }       /* gates count as open for reachability */
static bool plat_cell(const LevelDef *d, int x, int y) {
  for (int i = 0; i < d->nent; i++) if (d->ents[i].type == ENT_PLATFORM && y == d->ents[i].ty - 1 && x >= d->ents[i].tx - 3 && x <= d->ents[i].tx + 4) return true;
  return false;
}
static bool standable(const LevelDef *d, int x, int y) {
  uint8_t here = level_raw(d, x, y), below = level_raw(d, x, y + 1);
  if (wallish(here) || here == T_SPIKE || here == T_LAVA) return false;
  if (here == T_GRAV && (wallish(level_raw(d, x, y - 1)) || wallish(below))) return true;      /* ceiling or floor inside a gravity zone */
  return floorish(below) || here == T_LADDER || plat_cell(d, x, y);
}
static bool line_free(const LevelDef *d, int cx, int cy, int tx, int ty) {      /* conservative jump arc: row of the higher foot must be clear */
  int row = cy < ty ? cy : ty, a = cx < tx ? cx : tx, b = cx < tx ? tx : cx;
  for (int x = a; x <= b; x++) if (wallish(level_raw(d, x, row))) return false;
  return level_raw(d, tx, ty) == T_GRAV || !wallish(level_raw(d, tx, ty - 1));      /* (gravity-zone ceilings have the ceiling above them by design) */
}
static int tele_list(const LevelDef *d, int *xs, int *ys) {
  int n = 0; for (int y = 0; y < d->h; y++) for (int x = 0; x < d->w; x++) if (level_raw(d, x, y) == T_TELE && n < 8) { xs[n] = x; ys[n++] = y; }
  return n;
}
static int reach(const LevelDef *d, uint8_t *seen) {
  memset(seen, 0, MAX_LVL_W * MAX_LVL_H);
  static int q[MAX_LVL_W * MAX_LVL_H]; int h = 0, t = 0;
  int tx_[8], ty_[8], nt = tele_list(d, tx_, ty_);
  int sx = d->spawn_tx, sy = d->spawn_ty;
  while (sy + 1 < d->h && !standable(d, sx, sy)) sy++;
  q[t++] = sy * d->w + sx; seen[sy * d->w + sx] = 1;
  while (h < t) {
    int cur = q[h++], cx = cur % d->w, cy = cur / d->w;
    bool in_zone = level_raw(d, cx, cy) == T_GRAV;
    for (int ty = 1; ty < d->h; ty++) for (int tx = cx - 4; tx <= cx + 4; tx++) {
      if (tx < 1 || tx >= d->w - 1 || seen[ty * d->w + tx] || !standable(d, tx, ty)) continue;
      bool enter_zone = !in_zone && level_raw(d, tx, ty) == T_GRAV && tx >= cx - 2 && tx <= cx + 2;   /* walking into a gravity zone lifts you to its ceiling */
      if (!enter_zone && (cy - ty > 3 || !line_free(d, cx, cy, tx, ty))) continue;
      seen[ty * d->w + tx] = 1; q[t++] = ty * d->w + tx;
    }
    if (level_raw(d, cx, cy) == T_LADDER)
      for (int dy = -1; dy <= 1; dy += 2) { int ny = cy + dy;
        if (ny > 0 && ny < d->h && !seen[ny * d->w + cx] && !wallish(level_raw(d, cx, ny)) && (level_raw(d, cx, ny) == T_LADDER || standable(d, cx, ny)))
        { seen[ny * d->w + cx] = 1; q[t++] = ny * d->w + cx; } }
    for (int k = 0; k < nt; k++) if (tx_[k] == cx && ty_[k] == cy && (k ^ 1) < nt) {            /* teleport pads link in pairs */
      int px = tx_[k ^ 1], py = ty_[k ^ 1];
      if (!seen[py * d->w + px]) { seen[py * d->w + px] = 1; q[t++] = py * d->w + px; }
    }
  }
  return t;
}
static bool seen_near(const LevelDef *d, const uint8_t *seen, int x, int y, int r) {
  for (int yy = y - r; yy <= y + r; yy++) for (int xx = x - r; xx <= x + r; xx++)
    if (xx > 0 && yy > 0 && xx < d->w && yy < d->h && seen[yy * d->w + xx]) return true;
  return false;
}
static void test_levels(void) {
  puts("level validation");
  CHECK(level_count() == 20);
  for (int i = 0; i < LEVEL_COUNT; i++) if (level_available(i)) CHECK(i == 0 || level_available(i - 1));   /* no holes in the campaign */
  static uint8_t seen[MAX_LVL_W * MAX_LVL_H];
  for (int i = 0; i < LEVEL_COUNT; i++) {
    if (!level_available(i)) continue;
    const LevelDef *d = &LEVELS[i];
    printf("  L%02d %-20s %2dx%d world %d%s\n", i + 1, d->name, d->w, d->h, d->world, d->boss ? "  [GUARDIAN]" : "");
    CHECK(d->w >= 20 && d->w <= MAX_LVL_W && d->h == MAX_LVL_H);
    CHECK(d->world == i / 5 + 1);
    CHECK(d->spawn_tx > 0 && d->spawn_tx < d->w && level_raw(d, d->spawn_tx, d->spawn_ty) == T_AIR);
    CHECK(level_raw(d, d->spawn_tx, d->spawn_ty + 1) == T_SOLID);
    int exits = 0, tre = 0, bad = 0, runes = 0, gates = 0;
    for (int y = 0; y < d->h; y++) for (int x = 0; x < d->w; x++) {
      uint8_t t = level_raw(d, x, y); if (t == T_EXIT) exits++; if (t == T_COIN || t == T_CHEST) tre++;
      if (t == T_RUNE) runes++;
      if (t == T_GATE) gates++;
      if (t >= T_TILE_COUNT || t == T_RUNE_ON || t == T_TIMED_ON) bad++;
      if (t == T_VENT) for (int k = 1; k <= 3; k++) if (wallish(level_raw(d, x, y - k)) || level_raw(d, x, y - k) == T_GATE) bad++;   /* flame column must be clear */
      if (t == T_TIMED && (x == 0 || x == d->w - 1 || y == 0)) bad++;
      if (t == T_SPIKE && level_raw(d, x, y + 1) != T_SOLID && level_raw(d, x, y + 1) != T_SPIKE) bad++;   /* spikes sit on ground */
      if ((t == T_COIN || t == T_CHEST) && (x == 0 || y == 0 || x == d->w - 1)) bad++;
    }
    for (int e = 0; e < d->nent; e++) {
      const EntDef *en = &d->ents[e];
      if (level_raw(d, en->tx, en->ty) != T_AIR) bad++;
      if (en->type == ENT_CRAWLER && !floorish(level_raw(d, en->tx, en->ty + 1))) bad++;
    }
    CHECK(exits == 1); CHECK(bad == 0); CHECK(tre >= 1 && tre <= MAX_TREASURE);
    CHECK(level_treasure_count(d) == tre);
    CHECK((gates > 0) == (runes > 0));                                         /* every gate has runes, every rune a gate */
    CHECK(d->nent <= MAX_CR + MAX_PLATS);
    if (d->boss) CHECK((d->boss == 4 ? runes == 5 : runes == 3) && gates > 0);
    { int px[8], py[8], np = tele_list(d, px, py); CHECK(np % 2 == 0); for (int k = 0; k < np; k++) CHECK(floorish(level_raw(d, px[k], py[k] + 1))); }   /* pads come in pairs, standing on ground */
    reach(d, seen);
    int ex = 0, ey = 0; for (int y = 0; y < d->h; y++) for (int x = 0; x < d->w; x++) if (level_raw(d, x, y) == T_EXIT) { ex = x; ey = y; }
    if (!seen[ey * d->w + ex]) printf("  !! L%d exit (%d,%d) unreachable\n", i + 1, ex, ey);
    CHECK(seen[ey * d->w + ex]);                                               /* exit reachable (gates assumed open) */
    for (int y = 0; y < d->h; y++) for (int x = 0; x < d->w; x++) {
      uint8_t t = level_raw(d, x, y);
      if (t == T_RUNE && !seen_near(d, seen, x, y, 4)) printf("  !! L%d rune (%d,%d) out of reach\n", i + 1, x, y);
      if (t == T_CHEST && !seen_near(d, seen, x, y, 1)) printf("  !! L%d chest (%d,%d) out of reach\n", i + 1, x, y);
      if (t == T_RUNE) CHECK(seen_near(d, seen, x, y, 4));                     /* runes within Rune Pulse range of a reachable cell */
      if (t == T_CHEST) CHECK(seen_near(d, seen, x, y, 1));
    }
  }
  LevelState l; level_load(&l, 0, 0); level_take(&l, 8, 12); level_take(&l, 31, 5);
  uint32_t b = level_treasure_bits(&l); LevelState m; level_load(&m, 0, b); CHECK(level_treasure_bits(&m) == b && level_tile(&m, 8, 12) == T_AIR);
}

static void test_mechanics(void) {
  puts("moving platforms / crumble / runes / creatures / guardian");
  SaveData s; save_default(&s); Game g;
  /* moving platform carries the player */
  game_start_level(&g, &s, 3);
  CHECK(g.ls.nplats == 2);
  MPlat *m = &g.ls.plats[0];
  g.p.x = (m->x + 10) * SUBPX; g.p.y = (m->y - PH) * SUBPX - 8;
  for (int i = 0; i < 6; i++) game_update(&g, NONE);
  CHECK(g.p.ground && g.p.on_plat == 1);
  int rel0 = g.p.x - m->x * SUBPX;
  for (int i = 0; i < 60; i++) game_update(&g, NONE);
  int rel1 = g.p.x - m->x * SUBPX;
  CHECK(g.p.on_plat == 1 && (rel1 - rel0 < 3 * SUBPX && rel0 - rel1 < 3 * SUBPX));   /* rides along, doesn't slide off */
  CHECK(plat_off(0) == -48 && plat_off(96) == 48 && plat_off(192) == -48);
  /* crumbling floor breaks after being stood on, and drops the player */
  save_default(&s); game_start_level(&g, &s, 2);
  g.p.x = 22 * TILE_SUB; g.p.y = 12 * TILE_SUB - 4;
  for (int i = 0; i < 6; i++) game_update(&g, NONE);
  CHECK(g.p.ground && level_tile(&g.ls, 22, 13) == T_CRUMBLE);
  for (int i = 0; i < CRUMBLE_FRAMES + 2; i++) game_update(&g, NONE);
  CHECK(level_tile(&g.ls, 22, 13) == T_AIR);
  /* crumble state is reset after death */
  g.hp = 0; game_update(&g, NONE);
  CHECK(level_tile(&g.ls, 22, 13) == T_CRUMBLE);
  /* Rune Pulse: costs energy, has cooldown, activates runes in range only */
  save_default(&s); game_start_level(&g, &s, 2);
  CHECK(g.ls.runes_total == 1 && !g.ls.gates_open);
  CHECK(level_tile(&g.ls, 38, 11) == T_GATE);
  g.p.x = 10 * TILE_SUB; g.p.y = 12 * TILE_SUB; game_update(&g, (Input){.pulse = true});   /* far away */
  CHECK(!g.ls.gates_open && g.energy < 100);
  for (int i = 0; i < 40; i++) game_update(&g, NONE);
  g.p.x = 34 * TILE_SUB; g.p.y = 12 * TILE_SUB; int e0 = g.energy;
  uint8_t ev = game_update(&g, (Input){.pulse = true});
  CHECK(ev & EV_RUNE); CHECK(ev & EV_GATE); CHECK(g.ls.gates_open); CHECK(g.energy < e0);
  CHECK(level_tile(&g.ls, 38, 11) == T_AIR && level_tile(&g.ls, 35, 12) == T_RUNE_ON);
  g.energy = 5; g.pulse_cd = 0; g.ls.gates_open = false; g.ls.runes_on = 0; level_reset_runtime(&g.ls);
  ev = game_update(&g, (Input){.pulse = true}); g.pulse_prev = false;
  CHECK(!(ev & EV_PULSE));                                                    /* not enough energy */
  /* crawler: patrols, turns, hurts */
  save_default(&s); game_start_level(&g, &s, 0);
  CHECK(g.ncr == 1);
  int minx = g.cr[0].x, maxx = g.cr[0].x;
  g.p.x = 3 * TILE_SUB; g.p.y = 12 * TILE_SUB;
  for (int i = 0; i < 600; i++) { game_update(&g, NONE); if (g.cr[0].x < minx) minx = g.cr[0].x; if (g.cr[0].x > maxx) maxx = g.cr[0].x; }
  CHECK(maxx - minx > 40 && maxx < 26 * 16 && minx > 17 * 16);              /* stays between the pit and the spikes */
  g.cr[0].x = g.p.x / SUBPX; g.cr[0].y = g.p.y / SUBPX + 4; g.invuln = 0; int hp0 = g.hp;
  ev = game_update(&g, NONE); CHECK((ev & EV_HURT) && g.hp < hp0);
  /* guardian: bones spawn, warn, fall and hurt; lighting all runes opens the gate and defeats the guardian */
  save_default(&s); game_start_level(&g, &s, 4);
  CHECK(g.ls.def->boss && g.ls.runes_total == 3);
  int active = 0; for (int i = 0; i < 400; i++) { game_update(&g, NONE); for (int k = 0; k < MAX_BONES; k++) if (g.bone[k].on) { active++; break; } if (g.hp <= 0) break; }
  CHECK(active > 20);
  game_start_level(&g, &s, 4); g.invuln = 0; g.bone[0] = (Bone){g.p.x / SUBPX, g.p.y / SUBPX - 14, 0, 1}; hp0 = g.hp;
  ev = game_update(&g, NONE); CHECK((ev & EV_HURT) && g.hp == hp0 - 15);
  game_start_level(&g, &s, 4); g.energy = 200;
  int rx[3] = {9, 30, 19}, ry[3] = {4, 4, 4};
  for (int r = 0; r < 3; r++) {
    g.p.x = rx[r] * TILE_SUB; g.p.y = ry[r] * TILE_SUB; g.pulse_cd = 0; g.pulse_prev = false;
    ev = game_update(&g, (Input){.pulse = true});
    CHECK(ev & EV_RUNE); CHECK((r == 2) == !!(ev & EV_GATE));
  }
  CHECK(g.ls.gates_open && g.boss_done);
  for (int i = 0; i < 10; i++) game_update(&g, NONE);
  int nb = 0; for (int k = 0; k < MAX_BONES; k++) nb += g.bone[k].on; CHECK(nb == 0);       /* guardian stops attacking */
  g.p.x = 38 * TILE_SUB; g.p.y = 12 * TILE_SUB; ev = game_update(&g, NONE);
  CHECK(ev & EV_WIN); CHECK(s.guardians & 1); CHECK(s.completed & (1 << 4));
  /* spike hitbox is only the drawn spike: standing one pixel above it is safe, overlapping it is not */
  save_default(&s); game_start_level(&g, &s, 0);
  g.p.x = 26 * TILE_SUB + 3 * SUBPX; g.p.y = 12 * TILE_SUB + 12 * SUBPX - PH * SUBPX; g.p.vy = 0;   /* bottom 4 px into the spike */
  CHECK(player_step(&g.p, &g.ls, NONE) & EV_SPIKE);
  g.p.x = 26 * TILE_SUB + 3 * SUBPX; g.p.y = 12 * TILE_SUB + 8 * SUBPX - PH * SUBPX - 2 * SUBPX; g.p.vy = 0;   /* bottom 2 px above the spike */
  CHECK(!(player_step(&g.p, &g.ls, NONE) & EV_SPIKE));
}


static void test_feel(void) {
  puts("jump feel (simulated runs on real level geometry)");
  SaveData s; save_default(&s); Game g;
  /* run at the 2-tile spike field in level 1 and jump at different distances: every take-off from 2..5 tiles away clears it */
  int ok = 0, tries = 0;
  for (int off = 16; off <= 80; off += 8) {   /* floor between pit and spikes: px 288..416 */
    game_start_level(&g, &s, 0); g.cr[0].x = 600;
    g.p.x = (26 * 16 - off - 40) * SUBPX; g.p.y = 12 * TILE_SUB; for (int i = 0; i < 5; i++) game_update(&g, NONE);
    bool hit = false, jumped = false;
    for (int i = 0; i < 160; i++) {
      int px = g.p.x / SUBPX + PW; Input in = {.right = true};
      if (!jumped && px >= 26 * 16 - off) { in.jump = true; jumped = true; }
      else if (jumped && g.p.vy < 0) in.jump = true;
      if (game_update(&g, in) & EV_HURT) hit = true;
      if (g.p.x / SUBPX > 29 * 16) break;
    }
    tries++; if (!hit && g.p.x / SUBPX > 28 * 16) ok++;
  }
  printf("  spike field: %d/%d take-off distances (16..80 px before it) clear it\n\n", ok, tries);
  CHECK(ok >= 5);
  /* 3-tile pit (cols 15-17): a run-up jump must land on the far side */
  game_start_level(&g, &s, 0); g.cr[0].x = 600;
  g.p.x = 8 * TILE_SUB; g.p.y = 12 * TILE_SUB; for (int i = 0; i < 5; i++) game_update(&g, NONE);
  bool jumped = false, dead = false;
  for (int i = 0; i < 200; i++) {
    Input in = {.right = true}; int px = g.p.x / SUBPX + PW;
    if (!jumped && px >= 15 * 16 - 8) { in.jump = true; jumped = true; } else if (jumped && g.p.vy < 0) in.jump = true;
    if (game_update(&g, in) & EV_DEAD) dead = true;
  }
  CHECK(!dead && g.p.x / SUBPX > 18 * 16);
  /* every moving-platform gap in the campaign: from a standing start at ANY point of the platform's cycle, does some sensible
     sequence of inputs (tap/hold jumps, different take-off timings) board it and reach the far ledge? */
  struct { int level, tx, ledge_end, far_start; } gaps[] = {{3, 19, 240, 400}, {5, 16, 208, 336}, {6, 55, 832, 944}, {7, 67, 1008, 1168}};
  bool crossed = true;
  for (unsigned gi = 0; gi < sizeof gaps / sizeof *gaps; gi++) {
    int feasible = 0, cycles = 0;
    for (int start = 0; start < 192; start += 16) {
      cycles++; bool any = false;
      for (int bh = 0; bh < 2 && !any; bh++) for (int bo = 0; bo < 4 && !any; bo++) for (int lh = 0; lh < 2 && !any; lh++) for (int lo = 0; lo < 4 && !any; lo++) {
        game_start_level(&g, &s, gaps[gi].level); g.ncr = 0; g.p.x = (gaps[gi].ledge_end - 18) * SUBPX; g.p.y = 12 * TILE_SUB; g.tick = start;
        for (int i = 0; i < 5; i++) game_update(&g, NONE);
        bool boarded = false, done = false;
        for (int i = 0; i < 700 && !done; i++) {
          int pi = 0; for (int k = 0; k < g.ls.nplats; k++) if (g.ls.plats[k].base == gaps[gi].tx * 16 - 8 && g.ls.plats[k].y == 13 * 16) pi = k;
          Input in = {0}; MPlat *m = &g.ls.plats[pi]; int px = g.p.x / SUBPX;
          if (g.p.on_plat) boarded = true;
          if (!boarded) {
            if (g.p.ground && m->x <= gaps[gi].ledge_end + 4 + bo * 8) { in.right = true; in.jump = true; }
            if (!g.p.ground) { in.right = true; if (bh && g.p.vy < 0) in.jump = true; }
          } else if (m->x + PLAT_W >= gaps[gi].far_start - 44 + lo * 8) {
            in.right = true; if (g.p.ground && px + PW >= m->x + PLAT_W - 6) in.jump = true;
            if (!g.p.ground && lh && g.p.vy < 0) in.jump = true;
          }
          if (game_update(&g, in) & EV_DEAD) break;
          if (boarded && g.p.ground && !g.p.on_plat && g.p.x / SUBPX >= gaps[gi].far_start + 8) done = true;
        }
        any = done;
      }
      feasible += any;
    }
    printf("  moving-platform gap in L%d: a route exists from %d/%d start phases (the platform repeats every 192 frames, so a player can wait)\n", gaps[gi].level + 1, feasible, cycles);
    if (feasible < cycles / 2) crossed = false;      /* require a wide timing window: at least half of every cycle works */
  }
  CHECK(crossed);
}


static void test_pacing(void) {
  puts("frame pacing governor");
  const uint32_t F = 546; Pacer p; pacer_init(&p);
  for (int i = 0; i < 500; i++) pacer_frame(&p, 400, F);                      /* comfortably fast: never degrades */
  CHECK(p.level == 0 && pacer_div(&p) == 1 && pacer_quality(&p) == 2);
  for (int i = 0; i < 200; i++) pacer_frame(&p, i % 10 == 0 ? 700 : 400, F);    /* 1 slow frame in 10: ignored */
  CHECK(p.level == 0);
  int steps_seen[PACER_LEVELS] = {0}, last = 0;
  for (int i = 0; i < 1000; i++) { pacer_frame(&p, 900, F); if (p.level != last) { steps_seen[p.level] = i; last = p.level; } }   /* 900 ticks: too slow for 60fps */
  CHECK(pacer_div(&p) == 2 && p.level == 3);                                  /* ends at HIGH 30fps (900 < 2*546) and stays: it fits */
  CHECK(steps_seen[1] > 0 && steps_seen[2] > steps_seen[1] && steps_seen[3] > steps_seen[2]);   /* stepped through quality first, then fps */
  CHECK(pacer_quality(&p) == 2);
  for (int i = 0; i < 2000; i++) pacer_frame(&p, 5000, F);                    /* hopelessly slow: bottoms out, never past the last preset */
  CHECK(p.level == PACER_LEVELS - 1 && pacer_quality(&p) == 0 && pacer_div(&p) == 2);
  pacer_set_level(&p, 99); CHECK(p.level == PACER_LEVELS - 1); pacer_set_level(&p, -3); CHECK(p.level == 0);
  CHECK(pacer_steps(100, F) == 1 && pacer_steps(546, F) == 1 && pacer_steps(1092, F) == 2 && pacer_steps(1638, F) == 3 && pacer_steps(99999, F) == 3);
  CHECK(pacer_name(&p) != 0);
}

typedef struct { int x0, x1; } Span;
/* Depth-first search over input timings: wait w frames, then hop right with the key held for k frames of the flight. */
static bool route_dfs(const Game *g0, const Span *sp, int n, int depth) {
  if (depth == n) return true;
  static Game pool[8]; Game *t = &pool[depth];
  for (int w = 0; w < 180; w += 6) for (int k = 8; k <= 40; k += 8) {
    *t = *g0;
    bool dead = false;
    for (int i = 0; i < w && !dead; i++) if (game_update(t, NONE) & EV_DEAD) dead = true;
    if (dead || !t->p.ground) continue;
    bool landed = false;
    for (int i = 0; i < 80 && !dead; i++) {
      Input in = {0}; in.right = i < k; in.jump = i < 1 || t->p.vy < 0;
      if (game_update(t, in) & (EV_DEAD | EV_HURT)) { dead = true; break; }
      if (i > 3 && t->p.ground) { landed = true; break; }
    }
    if (dead || !landed) continue;
    for (int i = 0; i < 10; i++) game_update(t, NONE);
    int px = t->p.x / SUBPX;
    if (!t->p.ground || px < sp[depth].x0 || px > sp[depth].x1) continue;
    if (route_dfs(t, sp, n, depth + 1)) return true;
  }
  return false;
}
static bool timed_route(int level, int start_px, const Span *sp, int n) {
  SaveData s; save_default(&s); static Game g; game_start_level(&g, &s, level); g.ncr = 0; g.bone_timer = 250;
  int ok = 0, phases = 0;
  for (int ph = 0; ph < 180; ph += 30) {                       /* start at different points of the 180-frame cycle */
    game_start_level(&g, &s, level); g.ncr = 0; g.p.x = start_px * SUBPX; g.p.y = 12 * TILE_SUB - 4; g.tick = ph; g.ls.tick = ph;
    for (int i = 0; i < 4; i++) game_update(&g, NONE);
    phases++; ok += route_dfs(&g, sp, n, 0);
  }
  printf("  L%d timed crossing: a route exists from %d/%d start phases\n", level + 1, ok, phases);
  return ok == phases;
}
static void test_world34(void) {
  puts("world 3/4 mechanics: lava / vents / timed platforms / teleport / gravity / rain / final guardian");
  SaveData s; save_default(&s); static Game g; uint8_t ev;
  /* ---- lava ---- */
  game_start_level(&g, &s, 10); g.ncr = 0; g.bone_timer = 250;
  CHECK(g.ls.def->rain == 1 && g.ls.def->world == 3);
  g.p.x = 11 * TILE_SUB; g.p.y = 205 * SUBPX; int hp0 = g.hp;                  /* feet 6 px into the lava (surface at 213 px) */
  ev = game_update(&g, NONE); CHECK((ev & EV_HURT) && g.hp == hp0 - 30);
  game_start_level(&g, &s, 10); g.ncr = 0; g.bone_timer = 250; g.p.x = 11 * TILE_SUB; g.p.y = 195 * SUBPX; hp0 = g.hp;
  ev = game_update(&g, NONE); CHECK(!(ev & EV_HURT) && g.hp == hp0);          /* hovering just above is safe */
  save_default(&s); s.upg[UPG_ARMOR] = 2; game_start_level(&g, &s, 10); g.ncr = 0; g.bone_timer = 250; g.p.x = 11 * TILE_SUB; g.p.y = 205 * SUBPX; hp0 = g.hp;
  game_update(&g, NONE); CHECK(g.hp == hp0 - 26);                              /* armour applies to lava too */
  /* ---- fire vents: 30 erupting frames per 160, damage only while erupting and only inside the column ---- */
  int cnt[3] = {0, 0, 0}; for (unsigned t = 0; t < 160; t++) cnt[vent_phase(28, t)]++;
  CHECK(cnt[0] == 100 && cnt[1] == 30 && cnt[2] == 30);
  save_default(&s); game_start_level(&g, &s, 10); g.ncr = 0; g.bone_timer = 250;
  CHECK(g.nvent == 1 && g.vent_x[0] == 28 && g.vent_y[0] == 13);
  unsigned tf = 0; while (vent_phase(28, tf) != 2) tf++;
  g.tick = tf - 1; g.p.x = 28 * TILE_SUB + 3 * SUBPX; g.p.y = (13 * 16 - PH) * SUBPX; hp0 = g.hp;
  ev = game_update(&g, NONE); CHECK((ev & EV_HURT) && g.hp == hp0 - 25);
  unsigned ti = 0; while (vent_phase(28, ti) != 0) ti++;
  game_start_level(&g, &s, 10); g.ncr = 0; g.bone_timer = 250; g.tick = ti - 1; g.p.x = 28 * TILE_SUB + 3 * SUBPX; g.p.y = (13 * 16 - PH) * SUBPX; hp0 = g.hp;
  ev = game_update(&g, NONE); CHECK(!(ev & EV_HURT) && g.hp == hp0);
  game_start_level(&g, &s, 10); g.ncr = 0; g.bone_timer = 250; g.tick = tf - 1; g.p.x = 26 * TILE_SUB; g.p.y = (13 * 16 - PH) * SUBPX; hp0 = g.hp;   /* two tiles away */
  ev = game_update(&g, NONE); CHECK(!(ev & EV_HURT));
  /* ---- timed platforms: one group is always solid, the two overlap for 40 frames, standing on one when it ends drops you ---- */
  LevelState l; level_load(&l, 10, 0); int overlap = 0, gaps = 0;
  for (unsigned t = 0; t < 360; t++) { l.tick = t; bool a = level_tile(&l, 16, 12) == T_TIMED_ON, b = level_tile(&l, 12, 12) == T_TIMED_ON; if (!a && !b) gaps++; if (a && b) overlap++; }
  CHECK(gaps == 0 && overlap == 80);                                           /* 2 cycles x 40 frames */
  l.tick = 0; CHECK(level_tile(&l, 12, 12) == T_AIR && level_tile(&l, 16, 12) == T_TIMED_ON);
  game_start_level(&g, &s, 10); g.ncr = 0; g.bone_timer = 250; g.tick = 100; g.ls.tick = 100;
  g.p.x = 16 * TILE_SUB + 4 * SUBPX; g.p.y = (12 * 16 - PH) * SUBPX;
  for (int i = 0; i < 6; i++) game_update(&g, NONE);
  CHECK(g.p.ground);                                                              /* standing on the platform that is on */
  for (int i = 0; i < 20; i++) game_update(&g, NONE);
  CHECK(g.p.y / SUBPX > 12 * 16 - PH + 2 || !g.p.ground);   /* it switched off at phase 110: we fall */
  /* ---- teleport pads ---- */
  game_start_level(&g, &s, 15); g.ncr = 0;
  CHECK(g.ntele == 2 && g.tele_x[0] == 34 && g.tele_x[1] == 46);
  g.p.x = 34 * TILE_SUB + 3 * SUBPX; g.p.y = (13 * 16 - PH) * SUBPX;
  ev = game_update(&g, NONE); CHECK(ev & EV_TELE);
  CHECK(g.p.x / TILE_SUB == 46 && g.p.y / SUBPX > 150);
  for (int i = 0; i < 80; i++) game_update(&g, NONE);
  CHECK(g.p.x / TILE_SUB == 46);                                                /* no bouncing back while standing on the exit pad */
  g.p.x = 47 * TILE_SUB; for (int i = 0; i < 10; i++) game_update(&g, NONE);     /* step off, then back on: it works again */
  g.p.x = 46 * TILE_SUB + 3 * SUBPX; g.p.y = (13 * 16 - PH) * SUBPX; g.p.vy = 0; ev = 0;
  for (int i = 0; i < 6; i++) ev |= game_update(&g, NONE);
  CHECK((ev & EV_TELE) && g.p.x / TILE_SUB == 34);
  /* ---- gravity zone: walk in -> carried to the ceiling over the spikes, walk out -> back to the floor ---- */
  game_start_level(&g, &s, 15); g.ncr = 0; g.ntele = 0;
  g.p.x = 10 * TILE_SUB + 4 * SUBPX; g.p.y = (13 * 16 - PH) * SUBPX;
  for (int i = 0; i < 12; i++) game_update(&g, (Input){.right = true});
  CHECK(g.p.grav == -1);
  bool hurt = false; int minc = 999;
  for (int i = 0; i < 90; i++) { ev = game_update(&g, (Input){.right = true}); if (ev & EV_HURT) hurt = true; if (g.p.y / SUBPX < minc) minc = g.p.y / SUBPX; }
  CHECK(minc <= 18 && !hurt);                                                   /* reached the ceiling and stayed off the spikes */
  for (int i = 0; i < 260 && g.p.x / TILE_SUB < 28; i++) { ev = game_update(&g, (Input){.right = true}); if (ev & EV_HURT) hurt = true; }
  for (int i = 0; i < 90; i++) game_update(&g, NONE);
  CHECK(!hurt && g.p.grav == 1 && g.p.ground && g.p.y / SUBPX > 150 && g.hp == upgrade_max_hp(&s));
  anim_player_frame(&g.p, 0);
  /* ---- gravity zone jump: "up" is toward the floor while inverted ---- */
  game_start_level(&g, &s, 15); g.ncr = 0; g.p.x = 15 * TILE_SUB; g.p.y = TILE_SUB; g.p.grav = -1;
  for (int i = 0; i < 6; i++) game_update(&g, NONE);
  CHECK(g.p.grav == -1 && g.p.ground);
  int y0 = g.p.y; game_update(&g, (Input){.jump = true}); for (int i = 0; i < 6; i++) game_update(&g, (Input){.jump = true});
  CHECK(g.p.y > y0 + 4 * SUBPX);
  /* ---- ember rain on a normal level; stops mattering on guardian levels only after the guardian falls ---- */
  game_start_level(&g, &s, 10); g.ncr = 0; int seen_bone = 0;
  for (int i = 0; i < 300; i++) { game_update(&g, NONE); for (int k = 0; k < MAX_BONES; k++) seen_bone += g.bone[k].on && !g.bone[k].t; if (g.hp <= 0) break; }
  CHECK(seen_bone > 20);
  /* ---- the final guardian: five runes ---- */
  save_default(&s); game_start_level(&g, &s, 19); g.ncr = 0; g.ntele = 0; g.energy = 999;
  CHECK(g.ls.def->boss == 4 && g.ls.runes_total == 5);
  int rx[5] = {4, 45, 18, 29, 23}, ry[5] = {1, 1, 6, 6, 4}; uint8_t gate_ev = 0;
  for (int r = 0; r < 5; r++) {
    g.p.x = rx[r] * TILE_SUB; g.p.y = ry[r] * TILE_SUB; g.pulse_cd = 0; g.pulse_prev = false; g.invuln = 99;
    ev = game_update(&g, (Input){.pulse = true}); CHECK(ev & EV_RUNE); gate_ev |= (r == 4 && (ev & EV_GATE)) ? 1 : 0; CHECK((r == 4) == !!(ev & EV_GATE));
  }
  CHECK(gate_ev && g.ls.gates_open && g.boss_done);
  g.p.x = 48 * TILE_SUB; g.p.y = 12 * TILE_SUB; g.p.grav = 1; ev = game_update(&g, NONE);
  CHECK((ev & EV_WIN) && (s.guardians & 8) && (s.completed & (1UL << 19)));
  /* ---- every timed-platform crossing in the campaign has a route ---- */
  { Span a[] = {{12 * 16 - 4, 13 * 16 + 10}, {16 * 16 - 4, 17 * 16 + 10}, {20 * 16, 40 * 16}}; CHECK(timed_route(10, 8 * 16 + 4, a, 3)); }
  { Span a[] = {{38 * 16 - 4, 39 * 16 + 10}, {42 * 16 - 4, 43 * 16 + 10}, {44 * 16, 60 * 16}}; CHECK(timed_route(10, 35 * 16 - 8, a, 3)); }
  { Span a[] = {{44 * 16 - 4, 45 * 16 + 10}, {48 * 16 - 4, 49 * 16 + 10}, {52 * 16 - 4, 53 * 16 + 10}, {56 * 16 - 4, 57 * 16 + 10}, {58 * 16, 70 * 16}}; CHECK(timed_route(12, 41 * 16 - 8, a, 5)); }
  { Span a[] = {{67 * 16 - 4, 68 * 16 + 10}, {71 * 16 - 4, 72 * 16 + 10}, {75 * 16 - 4, 76 * 16 + 10}, {78 * 16, 86 * 16}}; CHECK(timed_route(13, 65 * 16 - 8, a, 4)); }
  { Span a[] = {{52 * 16 - 4, 53 * 16 + 10}, {56 * 16 - 4, 57 * 16 + 10}, {60 * 16 - 4, 61 * 16 + 10}, {63 * 16, 70 * 16}}; CHECK(timed_route(15, 49 * 16 - 8, a, 4)); }
  { Span a[] = {{12 * 16 - 4, 13 * 16 + 10}, {16 * 16 - 4, 17 * 16 + 10}, {20 * 16 - 4, 21 * 16 + 10}, {23 * 16, 36 * 16}}; CHECK(timed_route(18, 10 * 16 - 8, a, 4)); }
}

static void test_assets(void) {
  puts("asset packs (archive format, CRC, sprites)");
  CHECK(assets_status() == ASSETS_OK);
  for (int i = 0; i < SPR_COUNT; i++) {
    const uint8_t *p = spr(i); CHECK(p != 0);
    if (!p) continue;
    int w = p[0], h = p[1]; CHECK(w > 0 && h > 0);
    for (int k = 0; k < w * h; k++) if (p[2 + k] >= 32) { CHECK(0); break; }
  }
  CHECK(spr(SPR_SOLID_A)[0] == 16 && spr(SPR_KING0)[0] == 48 && spr(SPR_KING2)[1] == 60 && spr(SPR_BROOD2)[1] == 40 && spr(SPR_BONE)[0] == 6 && spr(SPR_HEART)[0] == 9);
  CHECK(spr(SPR_PLAYER_R_IDLE0)[0] == 12 && spr(SPR_PLAYER_L_HURT)[1] == 16 && SPR_PLAYER(1, FR_LAND) == SPR_PLAYER_L_LAND);
  int opaque = 0; const uint8_t *s0 = spr(SPR_SOLID_A); for (int k = 0; k < 256; k++) opaque += s0[2 + k] != 0; CHECK(opaque == 256);   /* wall tile has no holes */
  CHECK(spr(-1) == 0 && spr(SPR_COUNT) == 0);
  for (int w = 0; w < 4; w++) CHECK(theme_colors(w) != 0);
  CHECK(base_palette() != 0);
  /* corruption: flipping any sampled byte of the core pack must be rejected by the CRC; refresh keeps pointers valid */
  extern const uint8_t PACK_CORE[]; extern const unsigned PACK_CORE_LEN;
  static uint8_t copy[70000]; memcpy(copy, PACK_CORE, PACK_CORE_LEN);
  CHECK(crc16(copy + 14, (int)PACK_CORE_LEN - 14) == (copy[8] | copy[9] << 8));
  for (unsigned i = 14; i < PACK_CORE_LEN; i += 997) { copy[i] ^= 0x10; CHECK(crc16(copy + 14, (int)PACK_CORE_LEN - 14) != (copy[8] | copy[9] << 8)); copy[i] ^= 0x10; }
  assets_refresh(); CHECK(assets_status() == ASSETS_OK && level_available(0));
  /* the RAM copy is independent of the archive: changing the RAM copy never touches LEVELS[].tiles */
  LevelState l; level_load(&l, 0, 0); uint8_t before = LEVELS[0].tiles[20 * 15 + 5]; l.tiles[20 * 15 + 5] ^= 7; CHECK(LEVELS[0].tiles[20 * 15 + 5] == before);
}

static void test_memory(void) {
  puts("memory (HOST sizeof - not a measurement of the calculator build)");
  printf("  sizeof Game=%zu Player=%zu LevelState=%zu SaveData=%zu  save file=%d bytes\n", sizeof(Game), sizeof(Player), sizeof(LevelState), sizeof(SaveData), SAVE_BYTES);
  CHECK(sizeof(Game) < 4096); CHECK(sizeof(LevelState) < 2048);
  size_t lvl = 0; for (int i = 0; i < LEVEL_COUNT; i++) if (level_available(i)) lvl += LEVELS[i].w * LEVELS[i].h;
  printf("  level data: %zu bytes for %d levels (archived AppVars, read in place); only ONE level (<= %d bytes) is copied to RAM\n", lvl, level_count(), MAX_LVL_W * MAX_LVL_H);
  CHECK(20 * MAX_LVL_W * MAX_LVL_H < 60000);
}

int main(void) {
  if (!assets_init()) { puts("asset packs missing/corrupt"); return 2; }
  test_physics(); test_collision_hazards(); test_progression_treasure(); test_upgrades();
  test_abilities(); test_save(); test_levels(); test_mechanics(); test_feel(); test_pacing(); test_world34(); test_assets(); test_memory();
  printf("\n%d checks, %d failed\n", checks, fails);
  return fails != 0;
}
