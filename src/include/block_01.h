/* Shared utilities, shared by happy.exe and maped.exe
 * (src/common/block_01.c). Field names are hypotheses unless stated. */
#ifndef BLOCK_01_H
#define BLOCK_01_H

#include <allegro.h>

/* 0x1C bytes [verified: stride of the loops in 0x00401160 and 0x00403070].
 * Used both for the objects of a level (happy 0x00414140, 100 of them,
 * read from the .map file by 0x00403070) and for the object templates
 * (happy 0x00440060) [hypothesis: same type, since 0x00403070 copies
 * f12/f13 of template f18 into each object]. Only the bytes used so far
 * are named. */
typedef struct OBJ {
    unsigned char f00[0x12];
    unsigned char f12;
    unsigned char f13;
    unsigned char f14;
    unsigned char f15;
    unsigned char f16;
    unsigned char f17;
    /* [hypothesis] index of the template (0x00403070 reads the template
     * f18; template 34 has f18 = 34) */
    unsigned char f18;
    unsigned char f19;
    /* [hypothesis] slot in use (0x00403070 skips the objects where it is
     * 0, 0x00401160 clears it) */
    unsigned char f1a;
    unsigned char f1b;
} OBJ;

/* 12 bytes [verified: stride of the loop in 0x00401000]. happy only. */
typedef struct T_00414040 {
    /* [hypothesis] points (10 to 1000) */
    int f0;
    /* [hypothesis] flags (single bits 1 to 0x20) */
    unsigned char f4;
    int f8;
} T_00414040;

/* Size of a help text, terminator included [verified: stride 0x101 in
 * 0x004017C0 and 0x004014F0; 16 texts = the 0x1010 bytes that 0x00403070
 * reads from a .map file]. */
#define HELP_TEXT_SIZE 257

/* Globals used by this file, defined in each exe's data.c. The happy-only
 * functions use globals that maped does not define: in maped they are left
 * to the stubs of scripts/build.py (notes/matching.md §2). */
extern int screen_mode;
extern int use_joystick;
extern int keys[128];
extern volatile int time_left;
extern T_00414040 DAT_00414040[20];
extern OBJ DAT_00414140[100];
extern DATAFILE *misc_datafile;
extern char help_texts[16][HELP_TEXT_SIZE];
extern COLOR_MAP light_table;
extern BITMAP *buffer;
extern int controls[7];
extern OBJ DAT_00440060[40];

void FUN_00401000(void);
void FUN_00401160(void);
void read_joystick(void);
void read_keyboard(void);
void blit_to_screen(BITMAP *bmp);
void message_box(int n, char *text, int wait);
void init_help_texts(void);

#endif
