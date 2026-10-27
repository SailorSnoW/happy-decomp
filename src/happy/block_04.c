/* Loading and resetting a level: happy block_04 (0x00403030-0x0040344F).
 * notes/layout-happy.md §8. happy only. */
#include <stdio.h>
#include <stdlib.h>
#include <allegro.h>
#include "happy.h"

/* Resets a few game variables [verified]. Only caller 0x00409640.
 * [verified] Order: DAT_0042f548 = 0 must come first; several orders of
 * the next four assignments give the same code, this is one of them. */
// FUNCTION: HAPPY 0x00403030
void FUN_00403030(void)
{
    DAT_0042f548 = 0;
    DAT_0042f544 = 1;
    DAT_0042f540 = 2;
    DAT_0042f55c = 2;
    DAT_0042f564 = 0;
    DAT_00440038 = DAT_004450b0;
    DAT_00413b2c = 0;
}

/* Reads the objects and the help texts of a .map file, which follow its
 * tiles, then initialises the active objects from their template
 * [verified]. Only caller load_level. */
// FUNCTION: HAPPY 0x00403070
void load_level_objects(char *filename)
{
    FILE *f;
    int i;

    f = fopen(filename, "rb");
    fseek(f, DAT_00414130->h * DAT_00414130->w * sizeof(TILE) + MAP_HEADER_SIZE, SEEK_SET);
    fread(DAT_00414140, sizeof(DAT_00414140), 1, f);
    fread(help_texts, sizeof(help_texts), 1, f);
    fclose(f);
    for (i = 0; i < 100; i++) {
        if (DAT_00414140[i].f1a) {
            DAT_00414140[i].f12 = DAT_00440060[DAT_00414140[i].f18].f12;
            DAT_00414140[i].f11 = 0;
            DAT_00414140[i].f14 = 0;
            DAT_00414140[i].f1a = 1;
            DAT_00414140[i].f13 = DAT_00440060[DAT_00414140[i].f18].f13;
        }
    }
}

/* For the active objects with f1b == 2 and bit 0 of f19, puts the object
 * back at its saved position and clears f1b and DAT_00416d00[i].f10
 * [verified]. Only caller load_level (when keep is set). */
// FUNCTION: HAPPY 0x00403130
void FUN_00403130(void)
{
    int i;

    for (i = 0; i < 100; i++) {
        if (DAT_00414140[i].f1a && DAT_00414140[i].f1b == 2 && (DAT_00414140[i].f19 & 1)) {
            DAT_00414140[i].x = DAT_00416d00[i].x;
            DAT_00414140[i].y = DAT_00416d00[i].y;
            DAT_00416d00[i].f10 = 0;
            DAT_00414140[i].f1b = 0;
        }
    }
}

/* Loads level n [verified]: from the file names of DAT_0043f5c0 or
 * DAT_004404e0 (DAT_00442080 chooses) when DAT_00413b20 is 2 and custom is
 * 0, otherwise from entry n of DAT_00414c90, written to the swap file
 * "hland.$$$" and read back. keep: keep the objects (FUN_00403130 instead
 * of load_level_objects) and MAP.f7b/f7c. Then the sprites and tiles
 * (x versions if bit 3 of MAP.f79 is set), the palette, and all
 * particles are stopped. 5 callers. */
// FUNCTION: HAPPY 0x00403170
void load_level(int n, int keep, int custom)
{
    int f7b;
    int f7c;
    FILE *f;
    void *dat;
    long size;
    int i;

    if (keep) {
        f7b = DAT_00414130->f7b;
        f7c = DAT_00414130->f7c;
    }
    if (DAT_00414130 != NULL) {
        map_destroy(DAT_00414130);
        DAT_00414130 = NULL;
    }
    if (DAT_00413b20 == 2 && custom == 0) {
        if (DAT_00442080)
            DAT_00414130 = map_load(DAT_0043f5c0[n].f000);
        else
            DAT_00414130 = map_load(DAT_004404e0[n].f000);
        if (!keep) {
            if (DAT_00442080)
                load_level_objects(DAT_0043f5c0[n].f000);
            else
                load_level_objects(DAT_004404e0[n].f000);
        } else {
            FUN_00403130();
        }
    } else {
        dat = DAT_00414c90[n].dat;
        size = DAT_00414c90[n].size;
        // STRING: HAPPY 0x00412598
        f = fopen("hland.$$$", "wb");
        if (f == NULL) {
            // STRING: HAPPY 0x00412558
            allegro_message("Swapfile could not be created.\n Disk full or write protected.");
            exit(1);
        }
        fwrite(dat, size, 1, f);
        fclose(f);
        DAT_00414130 = map_load("hland.$$$");
        if (!keep)
            load_level_objects("hland.$$$");
        else
            FUN_00403130();
        delete_file("hland.$$$");
    }
    unload_datafile(DAT_00415cc0);
    if (DAT_00414130->f79 & 8) {
        // STRING: HAPPY 0x00412544
        DAT_00415cc0 = load_datafile("data/xsprites.dat");
        // STRING: HAPPY 0x00412534
        map_load_datafile("data/xtiles.dat");
    } else {
        // STRING: HAPPY 0x00412520
        DAT_00415cc0 = load_datafile("data/sprites.dat");
        map_load_datafile("data/tiles.dat");
    }
    if (keep) {
        DAT_00414130->f7b = f7b;
        DAT_00414130->f7c = f7c;
    }
    init_level_palette();
    for (i = 0; i < 256; i++)
        particles[i].active = 0;
}

/* Puts the player back at the start of the level and clears its state
 * [verified]: position = centre of start tile MAP.f03/f04 (x + 8, y + 17
 * pixels, fixed point). full = 0 also clears DAT_0042f515 and
 * DAT_004450a0. [verified] Bug of the original: the loop writes both
 * coordinates into DAT_00414cc0 (y overwrites x) and never writes
 * DAT_004154c0. Callers 0x00403820, 0x00405DB0. */
// FUNCTION: HAPPY 0x00403350
void reset_player(int full)
{
    int i;

    DAT_0042f500 = itofix(DAT_00414130->f03 * 16 + 8);
    DAT_0042f504 = itofix(DAT_00414130->f04 * 16 + 17);
    DAT_0042f53c = 1;
    DAT_0042f510 = 0;
    DAT_0042f54c = 0;
    DAT_0042f520 = 0;
    DAT_0042f550 = 0;
    DAT_0042f54d = 0;
    DAT_0042f54e = 0;
    DAT_0042f54f = 0;
    DAT_0042f516 = 0;
    if (!full)
        DAT_0042f515 = 0;
    DAT_0042f514 = 0;
    DAT_0042f554 = 0;
    DAT_0042f508 = 0;
    DAT_0042f52c = 0;
    DAT_0042f558 = 0;
    DAT_0042f560 = 0;
    for (i = 0; i < 512; i++) {
        DAT_00414cc0[i] = fixtoi(DAT_0042f500) - 8;
        DAT_00414cc0[i] = fixtoi(DAT_0042f504) - 17;
    }
    DAT_00440048 = 0;
    if (!full)
        DAT_004450a0 = 0;
    DAT_0043f5a0 = 0;
}
