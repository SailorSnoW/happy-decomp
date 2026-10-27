/* Map module, shared by happy.exe and maped.exe (src/common/block_09.c).
 * Layout of the structure deduced from 0x0040AA30 (allocation, defaults)
 * and 0x0040AB30 (loading); field names are hypotheses unless stated. */
#ifndef MAP_H
#define MAP_H

#include <allegro.h>

/* One cell of the map, 5 bytes [verified: index * 5 in 0x0040AC40]. */
typedef struct TILE {
    unsigned char t0;
    unsigned char t1;
    /* [hypothesis] collision type, 0 to 10 (read by 0x0040AC40) */
    unsigned char t2;
    unsigned char t3;
    unsigned char t4;
} TILE;

/* 0x8C bytes [verified: malloc(0x8C) in 0x0040AA30]. The first 0x85 bytes
 * are the header of a .map file, read and written as is (0x0040AB30, maped
 * 0x004013A0); the tiles follow it in the file. */
typedef struct MAP {
    /* [verified] loading requires 2, saving writes 2 [hypothesis: version] */
    unsigned char version;
    unsigned char w;
    unsigned char h;
    /* [hypothesis] start tile of the player (reset_player, 0x00403350) */
    unsigned char f03;
    unsigned char f04;
    /* [hypothesis] map name, "unnamed" by default */
    char name[0x50];
    /* [hypothesis] datafile of the tiles, "unnamed.dat" by default */
    char datafile[0x22];
    /* defaults set by map_create */
    unsigned char f77;
    /* [hypothesis] music: entry 8 + f78 of misc_datafile, 0 if above 11
     * (start_level, 0x00403820) */
    unsigned char f78;
    unsigned char f79;
    /* [hypothesis] time limit in seconds: copied into time_left
     * (0x00403820, 0x00405DB0) */
    unsigned char f7a;
    /* [hypothesis] Happylanders to save before the exit opens, and in all
     * (update_objects, 0x004041F0) */
    unsigned char f7b;
    unsigned char f7c;
    unsigned char f7d;
    unsigned char f7e;
    unsigned char f7f;
    unsigned char f80;
    unsigned char f81;
    unsigned char f82;
    unsigned char f83;
    unsigned char f84;
    TILE *tiles;
} MAP;

/* Size of the header of a .map file [verified: fread of 0x85 bytes]. */
#define MAP_HEADER_SIZE 0x85

/* Datafile loaded by map_load_datafile (happy 0x004450B4, maped 0x0042B0E0).
 * Defined in each exe's data.c. */
extern DATAFILE *map_datafile;

void map_clear(MAP *m);
MAP *map_create(int w, int h);
MAP *map_copy(MAP *src);
void map_destroy(MAP *m);
MAP *map_load(const char *filename);
int map_load_datafile(const char *filename);
int map_save(const char *filename, MAP *m);
int map_collision_at(MAP *m, int x, int y);
int map_t4_at(MAP *m, int x, int y);

#endif
