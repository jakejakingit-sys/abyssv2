#ifndef ABYSS_H
#define ABYSS_H
#include <stdint.h>
#include <stdbool.h>
#define TILE_PX 16
#define SUBPX 16                    /* positions are in 1/16 pixel */
#define TILE_SUB (TILE_PX * SUBPX)
#define LEVEL_COUNT 20
#define MAX_LVL_W 96
#define MAX_LVL_H 15
#define MAX_TREASURE 32
#define COIN_VALUE 5
#define CHEST_VALUE 40
#define PLAT_W 32
#define MAX_PLATS 4
enum Tile { T_AIR = 0, T_SOLID, T_ONEWAY, T_SPIKE, T_LADDER, T_COIN, T_CHEST, T_EXIT, T_CRUMBLE, T_RUNE, T_GATE, T_TORCH,
            T_RUNE_ON /* runtime only */, T_SKULL,
            T_LAVA,        /* hurts like spikes (30 dmg) */
            T_VENT,        /* solid floor tile that erupts a flame column on a timer */
            T_TIMED,       /* platform that is solid only part of the time (two alternating groups) */
            T_TIMED_ON,    /* runtime only: a T_TIMED tile that is currently solid */
            T_TELE,        /* teleport pad; pads link in pairs in row-major order (1st<->2nd, 3rd<->4th ...) */
            T_GRAV,        /* gravity-flip zone: while the player's centre is inside, gravity points up */
            T_TILE_COUNT };
enum { ENT_CRAWLER = 0, ENT_PLATFORM = 1 };
#endif
