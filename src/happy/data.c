/* Game globals of happy.exe, in address order.
 * [hypothesis] They are defined here until the .obj that owns each one is
 * known (notes/layout-happy.md §6 and §9, questions 1 and 7): this file only
 * decides where the recompiled exe puts them, not the code that uses them.
 * Uninitialised ones are tentative definitions (COMMON), like the large
 * arrays at 0x00413D3C-0x004468BF [hypothesis]. */
#include "happy.h"

/* [hypothesis] Display mode of blit_to_screen (0x00401380): 2 doubles the
 * pixels, 1 stretches to 640x400, anything else blits 320x200. Saved in
 * happy.sav, reset to 0 on loading if it is not 1 (0x00401E10). Initial
 * value 32 [verified]. */
// GLOBAL: HAPPY 0x00412030
int screen_mode = 32;

/* [hypothesis] Joystick enabled: read_joystick (0x004012A0) does nothing
 * when it is 0. */
// GLOBAL: HAPPY 0x00413b14
int use_joystick;

// GLOBAL: HAPPY 0x00413b24
int DAT_00413b24;

// GLOBAL: HAPPY 0x00413b28
int DAT_00413b28;

/* [hypothesis] State of the keys: Allegro's key[] in 0 to KEY_MAX - 1
 * (read_keyboard), joystick 0 in 116 to 124 (read_joystick). 128 entries:
 * the next global is at 0x00414020. */
// GLOBAL: HAPPY 0x00413e20
int keys[128];

/* [hypothesis] Time left in the level: the timer callback 0x004018B0
 * decrements it while [0x00412088] is set [verified], message_box saves and
 * restores it [verified]. volatile [hypothesis]: written by an interrupt
 * callback, and message_box only matches with it (the load is not moved
 * before the pushes of the prologue). */
// GLOBAL: HAPPY 0x00414020
volatile int time_left;

// GLOBAL: HAPPY 0x00414040
T_00414040 DAT_00414040[20];

/* [hypothesis] Objects of the level, read from the .map file (0x00403070). */
// GLOBAL: HAPPY 0x00414140
OBJ DAT_00414140[100];

/* Loaded by 0x00401D60 [verified]; entry 4 is the font of message_box. */
// GLOBAL: HAPPY 0x00414c9c
DATAFILE *DAT_00414c9c;

// GLOBAL: HAPPY 0x00415ce0
char help_texts[16][HELP_TEXT_SIZE];

/* [hypothesis] Translucency table: 0x00401A30 passes it to
 * create_trans_table, message_box makes it the color_map. */
// GLOBAL: HAPPY 0x004174e0
COLOR_MAP trans_table;

/* [hypothesis] Back buffer: drawn on, then passed to blit_to_screen. */
// GLOBAL: HAPPY 0x004274e8
BITMAP *buffer;

// GLOBAL: HAPPY 0x0042f52c
int DAT_0042f52c;

// GLOBAL: HAPPY 0x0042f530
int DAT_0042f530;

/* Incremented every 15 ms by the timer callback 0x004018F0, installed by
 * install_int(0x004018F0, 15) at 0x00401F6B [verified]. */
/* [hypothesis] Index in keys[] of the key that closes message_box. */
// GLOBAL: HAPPY 0x0042f594
int DAT_0042f594;

/* [hypothesis] Object templates, 40 of them (the next global is at
 * 0x004404C0); FUN_00401160 fills 0 to 34. */
// GLOBAL: HAPPY 0x00440060
OBJ DAT_00440060[40];

// GLOBAL: HAPPY 0x00442084
volatile int timer_ticks;

// GLOBAL: HAPPY 0x004450b4
DATAFILE *map_datafile;
