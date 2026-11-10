#ifndef PACING_H
#define PACING_H
#include <stdint.h>
#include <stdbool.h>
/* Frame-pacing governor (portable, host-tested). Six presets, ordered from best-looking to cheapest:
     level 0: HIGH 60fps   1: MED 60fps   2: LOW 60fps   3: HIGH 30fps   4: MED 30fps   5: LOW 30fps
   If the game cannot finish a frame inside its budget, it steps down one level at a time. Drawing quality drops before the frame
   rate does; a locked 30 fps (exactly 2 game ticks per frame) is smoother than an uneven 40 fps. */
typedef struct { uint8_t level, win, miss; } Pacer;
#define PACER_LEVELS 6
#define PACER_WINDOW 45          /* frames per evaluation window */
#define PACER_MISS_LIMIT 20      /* missed frames per window that trigger a step down */
void pacer_init(Pacer *p);
void pacer_set_level(Pacer *p, int level);
int pacer_div(const Pacer *p);         /* 1 = 60 fps, 2 = 30 fps */
int pacer_quality(const Pacer *p);     /* 2 = high, 1 = medium, 0 = low */
const char *pacer_name(const Pacer *p);
/* call once per frame with the time the game spent WORKING on that frame (not waiting). Returns true if the level changed. */
bool pacer_frame(Pacer *p, uint32_t work_ticks, uint32_t frame_ticks);
/* game ticks to run for a frame that took `elapsed` timer ticks (1..3, so a slow frame never means slow motion) */
int pacer_steps(uint32_t elapsed, uint32_t frame_ticks);
#endif
