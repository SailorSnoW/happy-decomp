/* Map module: happy block_09 (0x0040A9A0-0x0040AD4F, last .obj before the
 * CRT), maped block_01 (0x00401000-0x004013FF, first .obj of .text).
 * notes/layout-happy.md §8, notes/layout-maped.md §8.
 * [hypothesis] A single source file linked into both exes; /OPT:REF drops
 * the functions an exe does not use. Functions are in address order. */
#include <stdlib.h>
#include <string.h>
#include "map.h"

/* [hypothesis] Resets the first three bytes of every tile; called right after
 * the allocations of 0x0040AA30. */
// FUNCTION: HAPPY 0x0040a9a0
// FUNCTION: MAPED 0x00401000
void map_clear(MAP *m)
{
    int x;
    int y;
    int i;

    if (m != NULL) {
        memset(m->tiles, 0, m->h * m->w * sizeof(TILE));
        for (x = 0; x < m->w; x++) {
            for (y = 0; y < m->h; y++) {
                i = m->w * y + x;
                m->tiles[i].t0 = 0;
                m->tiles[i].t1 = 0;
                m->tiles[i].t2 = 0;
            }
        }
    }
}

/* Frees what 0x0040AA30 allocated [verified: free of both malloc blocks]. */
// FUNCTION: HAPPY 0x0040ab00
// FUNCTION: MAPED 0x00401260
void map_destroy(MAP *m)
{
    if (m->tiles != NULL)
        free(m->tiles);
    if (m != NULL)
        free(m);
}

/* Replaces map_datafile; returns whether loading succeeded [verified].
 * [hypothesis] The tile graphics: happy's only caller, 0x00403170, loads
 * data/tiles.dat or data/xtiles.dat. */
// FUNCTION: HAPPY 0x0040ac10
// FUNCTION: MAPED 0x00401370
int map_load_datafile(const char *filename)
{
    if (map_datafile != NULL)
        unload_datafile(map_datafile);
    map_datafile = load_datafile(filename);
    return map_datafile != NULL;
}
