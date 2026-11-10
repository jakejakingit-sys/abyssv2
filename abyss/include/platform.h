#ifndef PLATFORM_H
#define PLATFORM_H
#include <stdint.h>
#include "save.h"
#define PACK_COUNT 6           /* 0 = ABYSSCOR (common sprites, palettes), 1..4 = ABYSSLV1..4 (levels), 5 = ABYSSBOS (guardians + logo) */
/* Returns a READ-ONLY pointer to the pack's data (AppVar contents, archived), or NULL if the AppVar is missing.
   On the calculator this archives the AppVar if it was sent to RAM. Pointers may be invalidated by any archive write
   (save, garbage collection), so callers must call assets_refresh() after platform_save_write(). */
const uint8_t *platform_pack_ptr(int id, uint32_t *len);
void platform_save_write(const SaveData *s);    /* single archive write; refreshes asset pointers afterwards */
SaveStatus platform_save_load(SaveData *s);
#endif
