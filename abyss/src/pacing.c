#include "pacing.h"
static const char *const NAMES[PACER_LEVELS] = {"HIGH 60fps", "MEDIUM 60fps", "LOW 60fps", "HIGH 30fps", "MEDIUM 30fps", "LOW 30fps"};
void pacer_init(Pacer *p) { p->level = 0; p->win = p->miss = 0; }
void pacer_set_level(Pacer *p, int level) { p->level = (uint8_t)(level < 0 ? 0 : level >= PACER_LEVELS ? PACER_LEVELS - 1 : level); p->win = p->miss = 0; }
int pacer_div(const Pacer *p) { return 1 + p->level / 3; }
int pacer_quality(const Pacer *p) { return 2 - p->level % 3; }
const char *pacer_name(const Pacer *p) { return NAMES[p->level]; }
bool pacer_frame(Pacer *p, uint32_t work, uint32_t frame_ticks) {
  uint32_t budget = frame_ticks * (uint32_t)pacer_div(p);
  if (work > budget - budget / 16) p->miss++;            /* used more than ~94% of the frame budget */
  if (++p->win < PACER_WINDOW) return false;
  bool step = p->miss >= PACER_MISS_LIMIT && p->level < PACER_LEVELS - 1;
  if (step) p->level++;
  p->win = p->miss = 0;
  return step;
}
int pacer_steps(uint32_t elapsed, uint32_t frame_ticks) {
  uint32_t n = frame_ticks ? elapsed / frame_ticks : 1;
  return n < 1 ? 1 : n > 3 ? 3 : (int)n;
}
