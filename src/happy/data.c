/* Game globals of happy.exe, in address order.
 * [hypothesis] They are defined here until the .obj that owns each one is
 * known (notes/layout-happy.md §6 and §9, questions 1 and 7): this file only
 * decides where the recompiled exe puts them, not the code that uses them.
 * Uninitialised ones are tentative definitions (COMMON), like the large
 * arrays at 0x00413D3C-0x004468BF [hypothesis]. */
#include "happy.h"

/* [hypothesis] Display mode of blit_to_screen (0x00401380): 2 doubles the
 * pixels, 1 stretches to 640x400, anything else blits 320x200. Saved in
 * happy.sav, reset to 0 on loading if it is not 1 (0x00401E10); 0 selects
 * 320x200 in init_game, anything else 640x480. Initial value 32
 * [verified]. */
// GLOBAL: HAPPY 0x00412030
int screen_mode = 32;

/* [hypothesis] Clock running: timer_1s decrements time_left only when it
 * is set. Initial value 1 [verified]. */
// GLOBAL: HAPPY 0x00412088
int DAT_00412088 = 1;

/* [hypothesis] Joystick enabled: read_joystick (0x004012A0) does nothing
 * when it is 0; init_game sets it when install_joystick succeeds. */
// GLOBAL: HAPPY 0x00413b14
int use_joystick;

// GLOBAL: HAPPY 0x00413b20
int DAT_00413b20;

// GLOBAL: HAPPY 0x00413b24
int DAT_00413b24;

// GLOBAL: HAPPY 0x00413b28
int DAT_00413b28;

/* errno of LIBC (single-threaded: a variable, not a function): allegro_init
 * passes &errno; read and written by the CRT (0x0040B3D7, 0x0040B645...)
 * [verified]. Annotated by name, defined by LIBC. */
// GLOBAL: HAPPY 0x00413b3c
// errno

/* [hypothesis] MIDI volume: init_game passes it to set_volume(-1, ...).
 * Saved in happy.sav; 220 by default. */
// GLOBAL: HAPPY 0x00413d40
unsigned char midi_volume;

/* [hypothesis] High scores: "Johan Peitz" with 10000 down to 2500 by
 * default (init_hiscores fills 6 entries); happy.sav stores 168 bytes,
 * i.e. 7 entries [verified]. */
// GLOBAL: HAPPY 0x00413d60
HISCORE hiscores[7];

/* [hypothesis] State of the keys: Allegro's key[] in 0 to KEY_MAX - 1
 * (read_keyboard), joystick 0 in 116 to 124 (read_joystick). 128 entries:
 * the next global is at 0x00414020. */
// GLOBAL: HAPPY 0x00413e20
int keys[128];

/* [hypothesis] Time left in the level: timer_1s decrements it once per
 * second while DAT_00412088 is set [verified], message_box and yes_no_box
 * save and restore it [verified]. volatile [hypothesis]: written by an
 * interrupt callback, and message_box only matches with it (the load is
 * not moved before the pushes of the prologue). */
// GLOBAL: HAPPY 0x00414020
volatile int time_left;

/* fixed, 22/100 [verified: init_game] */
// GLOBAL: HAPPY 0x00414024
fixed DAT_00414024;

/* fixed, 1.0 [verified: init_game] */
// GLOBAL: HAPPY 0x00414028
fixed DAT_00414028;

/* [hypothesis] Frames shown during the last second: timer_1s copies
 * frame_count into it, then clears frame_count. */
// GLOBAL: HAPPY 0x0041402c
volatile int fps;

// GLOBAL: HAPPY 0x00414040
T_00414040 DAT_00414040[20];

/* [hypothesis] The map of the level: shutdown_game destroys it with
 * map_destroy, 0x00403070 reads its size. */
// GLOBAL: HAPPY 0x00414130
MAP *DAT_00414130;

/* [hypothesis] Frame counter, see fps. fps and frame_count are volatile
 * [hypothesis]: written by a timer callback, and timer_1s and init_game
 * only match with it (stores kept in source order). */
// GLOBAL: HAPPY 0x00414134
volatile int frame_count;

/* [hypothesis] Objects of the level, read from the .map file (0x00403070). */
// GLOBAL: HAPPY 0x00414140
OBJ DAT_00414140[100];

/* Text of the scrolling message (start_scroll_message); 80 bytes, the next
 * global is at 0x00414C90. */
// GLOBAL: HAPPY 0x00414c40
char scroll_text[80];

/* [hypothesis] A datafile whose entries FUN_004019C0 counts. */
// GLOBAL: HAPPY 0x00414c90
DATAFILE *DAT_00414c90;

// GLOBAL: HAPPY 0x00414c94
int DAT_00414c94;

/* fixed, 1/100 [verified: init_game] */
// GLOBAL: HAPPY 0x00414c98
fixed DAT_00414c98;

/* Loaded from data/misc.dat by load_data [verified]; entry 4 is the font
 * of message_box. */
// GLOBAL: HAPPY 0x00414c9c
DATAFILE *misc_datafile;

// GLOBAL: HAPPY 0x00414ca0
int DAT_00414ca0;

// GLOBAL: HAPPY 0x00414ca4
int DAT_00414ca4;

/* Unloaded by shutdown_game [verified]. */
// GLOBAL: HAPPY 0x00415cc0
DATAFILE *DAT_00415cc0;

// GLOBAL: HAPPY 0x00415ce0
char help_texts[16][HELP_TEXT_SIZE];

/* End position of the scrolling message: -20 - its width [verified]. */
// GLOBAL: HAPPY 0x004174d0
int scroll_end;

/* Light table: init_color_tables (0x00401A30) builds it with
 * create_light_table(..., 0, 0, 0, NULL) [verified] and makes it the
 * color_map; message_box draws its box through it in DRAW_MODE_TRANS. */
// GLOBAL: HAPPY 0x004174e0
COLOR_MAP light_table;

/* Width of the scrolling message in pixels [verified]. */
// GLOBAL: HAPPY 0x004274e0
int scroll_width;

/* 640x200 bitmap created by init_game [verified]. */
// GLOBAL: HAPPY 0x004274e4
BITMAP *DAT_004274e4;

/* [hypothesis] Back buffer: 320x200 (init_game), drawn on, then passed to
 * blit_to_screen. */
// GLOBAL: HAPPY 0x004274e8
BITMAP *buffer;

/* RGB_MAP built by init_color_tables and made the rgb_map [verified]. */
// GLOBAL: HAPPY 0x00427500
RGB_MAP rgb_table;

/* [hypothesis] A countdown in seconds: timer_1s decrements it down to 0. */
// GLOBAL: HAPPY 0x0042f516
unsigned char DAT_0042f516;

/* fixed, 1/10 [verified: init_game] */
// GLOBAL: HAPPY 0x0042f51c
fixed DAT_0042f51c;

// GLOBAL: HAPPY 0x0042f52c
int DAT_0042f52c;

// GLOBAL: HAPPY 0x0042f530
int DAT_0042f530;

// GLOBAL: HAPPY 0x0042f568
unsigned char DAT_0042f568;

/* [hypothesis] Control keys, as Allegro KEY_ codes: default_settings sets
 * 0 RIGHT, 1 LEFT, 4 ALT, 5 LCONTROL, 6 DOWN [verified]; 5 closes
 * message_box. Saved in happy.sav (28 bytes). */
// GLOBAL: HAPPY 0x0042f580
int controls[7];

/* fixed, 1/10 [verified: init_game] */
// GLOBAL: HAPPY 0x0042f59c
fixed DAT_0042f59c;

/* Translucency table: init_color_tables builds it with
 * create_trans_table(..., 128, 128, 128, NULL) [verified]. */
// GLOBAL: HAPPY 0x0042f5a0
COLOR_MAP trans_table;

/* Scrolling message position [verified: start_scroll_message,
 * draw_scroll_message]. */
// GLOBAL: HAPPY 0x0044003c
int scroll_x;

// GLOBAL: HAPPY 0x00440040
int scroll_y;

/* fixed, 1.0 [verified: init_game] */
// GLOBAL: HAPPY 0x00440044
fixed DAT_00440044;

/* [hypothesis] Object templates, 40 of them (the next global is at
 * 0x004404C0); FUN_00401160 fills 0 to 34. */
// GLOBAL: HAPPY 0x00440060
OBJ DAT_00440060[40];

/* fixed, 12/100 [verified: init_game] */
// GLOBAL: HAPPY 0x004404c0
fixed DAT_004404c0;

/* [hypothesis] The game palette: init_color_tables builds its three tables
 * from it. */
// GLOBAL: HAPPY 0x00441c80
PALETTE DAT_00441c80;

/* Incremented every 15 ms by timer_15ms (0x004018F0), installed by
 * install_int(timer_15ms, 15) at 0x00401F6B [verified]. */
// GLOBAL: HAPPY 0x00442084
volatile int timer_ticks;

/* [hypothesis] Digital sound volume: saved in happy.sav, 200 by default. */
// GLOBAL: HAPPY 0x00442088
unsigned char DAT_00442088;

// GLOBAL: HAPPY 0x004450a4
int DAT_004450a4;

/* Number of entries of DAT_00414c90, minus 8 or 9 (FUN_004019C0). */
// GLOBAL: HAPPY 0x004450a8
unsigned char DAT_004450a8;

/* Scrolling message active [verified: set by start_scroll_message, cleared
 * by draw_scroll_message at the end]. */
// GLOBAL: HAPPY 0x004450ac
int scroll_active;

// GLOBAL: HAPPY 0x004450b4
DATAFILE *map_datafile;
