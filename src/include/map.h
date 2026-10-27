/* Map module, shared by happy.exe and maped.exe (src/common/block_09.c).
 * Layout of the structure: notes/decompme-candidates.md §2 (deduced from
 * happy 0x0040AA30); field names are hypotheses unless stated. */
#ifndef MAP_H
#define MAP_H

#include <allegro.h>

/* One cell of the map, 5 bytes [verified: index * 5 in 0x0040AC40]. */
typedef struct TILE {
    unsigned char t0;
    unsigned char t1;
    unsigned char t2;
    unsigned char t3;
    unsigned char t4;
} TILE;

/* 0x8C bytes, allocated by 0x0040AA30 [verified: malloc(0x8C)].
 * w and h are bytes 1 and 2 of the .map header (notes/game-docs.md). */
typedef struct MAP {
    unsigned char f00;
    unsigned char w;
    unsigned char h;
    unsigned char f03[2];
    char f05[0x50];
    char f55[0x22];
    unsigned char f77[0x11];
    TILE *tiles;
} MAP;

/* Datafile loaded by map_load_datafile (happy 0x004450B4, maped 0x0042B0E0).
 * Defined in each exe's data.c. */
extern DATAFILE *map_datafile;

void map_clear(MAP *m);
void map_destroy(MAP *m);
int map_load_datafile(const char *filename);

#endif
