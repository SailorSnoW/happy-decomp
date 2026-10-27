/* Map module: happy block_09 (0x0040A9A0-0x0040AD4F, last .obj before the
 * CRT), maped block_01 (0x00401000-0x004013FF, first .obj of .text).
 * notes/layout-happy.md §8, notes/layout-maped.md §8.
 * [hypothesis] A single source file linked into both exes; /OPT:REF drops
 * the functions an exe does not use. Functions are in address order; the
 * relative order of map_save (maped only) and of the last two functions
 * (happy only) is unknown. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "map.h"

/* [hypothesis] Resets the first three bytes of every tile; called right after
 * the allocations of map_create. */
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

/* Allocates an empty w x h map, with default header values [verified]. */
// FUNCTION: HAPPY 0x0040aa30
// FUNCTION: MAPED 0x00401090
MAP *map_create(int w, int h)
{
    MAP *m;

    m = malloc(sizeof(MAP));
    if (m == NULL)
        return NULL;
    m->tiles = malloc(w * h * sizeof(TILE));
    if (m->tiles == NULL) {
        free(m);
        return NULL;
    }
    m->w = w;
    m->h = h;
    m->f7d = 20;
    m->f7e = 30;
    m->f7f = 60;
    m->f81 = 0;
    m->f82 = 10;
    m->f83 = 40;
    m->f7a = 200;
    m->f79 = 1;
    m->f7b = 0;
    m->f7c = 0;
    m->f78 = 0;
    m->f77 = 0;
    // STRING: HAPPY 0x00412f88
    // STRING: MAPED 0x0040e03c
    strcpy(m->name, "unnamed");
    // STRING: HAPPY 0x00412f7c
    // STRING: MAPED 0x0040e030
    strcpy(m->datafile, "unnamed.dat");
    map_clear(m);
    return m;
}

/* [verified] New map with the size, the two strings and the tiles of src
 * (the other header fields are not copied). maped only; only caller
 * 0x00403EB0. */
// FUNCTION: MAPED 0x00401160
MAP *map_copy(MAP *src)
{
    MAP *m;
    int x;
    int y;
    int i;

    m = malloc(sizeof(MAP));
    if (m == NULL)
        return NULL;
    m->tiles = malloc(src->h * src->w * sizeof(TILE));
    if (m->tiles == NULL) {
        free(m);
        return NULL;
    }
    m->w = src->w;
    m->h = src->h;
    strcpy(m->name, src->name);
    strcpy(m->datafile, src->datafile);
    for (x = 0; x < m->w; x++) {
        for (y = 0; y < m->h; y++) {
            i = m->w * y + x;
            m->tiles[i] = src->tiles[i];
        }
    }
    return m;
}

/* Frees what map_create allocated [verified: free of both malloc blocks]. */
// FUNCTION: HAPPY 0x0040ab00
// FUNCTION: MAPED 0x00401260
void map_destroy(MAP *m)
{
    if (m->tiles != NULL)
        free(m->tiles);
    if (m != NULL)
        free(m);
}

/* Reads a .map file: the header once to get the size, then the whole file
 * again into a new map [verified]. NULL if the file cannot be opened or its
 * version is not 2. */
// FUNCTION: HAPPY 0x0040ab30
// FUNCTION: MAPED 0x00401290
MAP *map_load(const char *filename)
{
    unsigned char header[MAP_HEADER_SIZE];
    FILE *f;
    MAP *m;

    // STRING: HAPPY 0x00412500
    // STRING: MAPED 0x0040e044
    f = fopen(filename, "rb");
    if (f == NULL)
        return NULL;
    fread(header, MAP_HEADER_SIZE, 1, f);
    fclose(f);
    if (header[0] != 2)
        return NULL;
    m = map_create(header[1], header[2]);
    f = fopen(filename, "rb");
    if (f == NULL)
        return NULL;
    fread(m, MAP_HEADER_SIZE, 1, f);
    fread(m->tiles, m->h * m->w * sizeof(TILE), 1, f);
    fclose(f);
    return m;
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

/* Writes a .map file (version 2, header then tiles) [verified]; always
 * returns 1. maped only. */
// FUNCTION: MAPED 0x004013a0
int map_save(const char *filename, MAP *m)
{
    FILE *f;

    m->version = 2;
    // STRING: MAPED 0x0040e048
    f = fopen(filename, "wb");
    fwrite(m, MAP_HEADER_SIZE, 1, f);
    fwrite(m->tiles, m->h * m->w * sizeof(TILE), 1, f);
    fclose(f);
    return 1;
}

/* Collision type of the 16x16 tile under pixel (x, y), or 0 if the pixel
 * is outside the solid part of the tile [verified: the tests on the
 * position inside the tile]. [hypothesis] 2/5 and 3/6: left and right
 * parts, 7 to 10: slopes. happy only; callers 0x004041F0, 0x00404C50. */
// FUNCTION: HAPPY 0x0040ac40
int map_collision_at(MAP *m, int x, int y)
{
    int t;
    int dx;
    int dy;

    t = m->tiles[m->w * (y >> 4) + (x >> 4)].t2;
    if (t == 0)
        return 0;
    if (t == 1 || t == 4)
        return t;
    dx = x - (x & ~15);
    if ((t == 2 || t == 5) && dx < 10)
        return t;
    if ((t == 3 || t == 6) && dx > 5)
        return t;
    dy = y - (y & ~15);
    if (t == 7) {
        if (dy > 20 - dx)
            return 7;
    } else if (t == 8) {
        if (dy > 4 - dx)
            return 8;
    } else if (t == 9) {
        if (dy > dx + 6)
            return t;
    } else if (t == 10) {
        if (dy > dx - 10)
            return t;
    }
    return 0;
}

/* Byte 4 of the tile under pixel (x, y) [verified]. happy only; callers
 * 0x004041F0, 0x00404C50. */
// FUNCTION: HAPPY 0x0040ad20
int map_t4_at(MAP *m, int x, int y)
{
    return m->tiles[m->w * (y >> 4) + (x >> 4)].t4;
}
