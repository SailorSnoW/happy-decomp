/* Game globals of happy.exe, in address order.
 * [hypothesis] They are defined here until the .obj that owns each one is
 * known (notes/layout-happy.md §4 and §7, question 1): this file only
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

/* [hypothesis] Passwords of the levels, stored with every character + 1
 * ("opof" = "none" for entry 0; entries 16-20 begin with "HAXE-")
 * [verified: FUN_00404770 subtracts 1 from each character before printing
 * entry DAT_00440038 - 1]. */
// GLOBAL: HAPPY 0x00412034
char *passwords[21] = {
    // STRING: HAPPY 0x00412214
    "opof",
    // STRING: HAPPY 0x00412204
    "uijfgpgqsjodft",
    // STRING: HAPPY 0x004121f4
    "tpnfuijohgjtiz",
    // STRING: HAPPY 0x004121e4
    "epouuvsobspvoe",
    // STRING: HAPPY 0x004121d0
    "zpvsljohboervffo",
    // STRING: HAPPY 0x004121bc
    "nbyjnjmmjboesblf",
    // STRING: HAPPY 0x004121a8
    "xifsftuifebnodbu",
    // STRING: HAPPY 0x00412194
    "epoufbuzfmmpxtopx",
    // STRING: HAPPY 0x00412180
    "gpsftupgcmbdlpblt",
    // STRING: HAPPY 0x0041216c
    "dbozpvtffuifnppo",
    // STRING: HAPPY 0x00412158
    "ifsfdpnftzvmfujef",
    // STRING: HAPPY 0x00412144
    "xizepzpvcvjmenfvq",
    // STRING: HAPPY 0x00412130
    "uifnbobufuxptpdlt",
    // STRING: HAPPY 0x0041211c
    "dbstbsfpgufocjhhfs",
    // STRING: HAPPY 0x00412108
    "bohszcfbstbsfhsffo",
    // STRING: HAPPY 0x004120f4
    "tbdlgvmmpgqpubupft",
    // STRING: HAPPY 0x004120e0
    "IBYF.TQMJUTFDPOET",
    // STRING: HAPPY 0x004120cc
    "IBYF.DBOUUPVDINF",
    // STRING: HAPPY 0x004120bc
    "IBYF.PQFOTFTBNF",
    // STRING: HAPPY 0x004120a8
    "IBYF.HPIPNFCBEEJF",
    // STRING: HAPPY 0x00412094
    "IBYF.GVMMDBQBDJUZ"
};

/* [hypothesis] Clock running: timer_1s decrements time_left only when it
 * is set. Initial value 1 [verified]. The cheat "HAXE-SPLITSECONDS" sets
 * it to 0 (enter_password) [verified]. */
// GLOBAL: HAPPY 0x00412088
int DAT_00412088 = 1;

/* [hypothesis] Enemies kept: start_level removes the objects with bit 2
 * of f19 when it is 0. Initial value 1 [verified]. The cheat
 * "HAXE-GOHOMEBADDIE" sets it to 0 (enter_password) [verified]. */
// GLOBAL: HAPPY 0x0041208c
int DAT_0041208c = 1;

/* [hypothesis] Number of objects that can follow the player
 * (FUN_00403f60 compares player.f58 with it). Initial value 5
 * [verified]. The cheat "HAXE-FULLCAPACITY" sets it to 10
 * (enter_password) [verified]. */
// GLOBAL: HAPPY 0x00412090
int DAT_00412090 = 5;

/* Set to 1 by the command-line option "-fps" (main) [verified]; main's
 * store is its only reference in the exe [verified: byte scan], so the
 * option has no effect. */
// GLOBAL: HAPPY 0x00413b10
int DAT_00413b10;

/* [hypothesis] Joystick enabled: read_joystick (0x004012A0) does nothing
 * when it is 0; init_game sets it when install_joystick succeeds. */
// GLOBAL: HAPPY 0x00413b14
int use_joystick;

/* [hypothesis] Game chosen in start_menu [verified: set from the mouse
 * band there]: 0 "HAPPYLAND ADVENTURES" (data/xlevels.dat), 1 "THE
 * ORIGINAL GAME" (data/levels.dat), 2 "CREATE CUSTOM GAME" (custom_game,
 * then data/levels.dat), 3 "EXIT". */
// GLOBAL: HAPPY 0x00413b20
int DAT_00413b20;

// GLOBAL: HAPPY 0x00413b24
int DAT_00413b24;

// GLOBAL: HAPPY 0x00413b28
int DAT_00413b28;

/* Set to 1 at the end of play_ending [verified]. */
// GLOBAL: HAPPY 0x00413b2c
int DAT_00413b2c;

/* [hypothesis] Enemies harmless: FUN_00403f60 skips the contacts with the
 * objects of f19 bit 2 when it is set. Set by the cheat
 * "HAXE-CANTTOUCHME" (enter_password) [verified]. */
// GLOBAL: HAPPY 0x00413b30
int DAT_00413b30;

/* [hypothesis] Exit always open: start_level calls open_exit when it is
 * set. Set by the cheat "HAXE-OPENSESAME" (enter_password) [verified]. */
// GLOBAL: HAPPY 0x00413b34
int DAT_00413b34;

/* errno of LIBC (single-threaded: a variable, not a function): allegro_init
 * passes &errno; read and written by the CRT (0x0040B3D7, 0x0040B645...)
 * [verified]. Annotated by name, defined by LIBC. */
// GLOBAL: HAPPY 0x00413b3c
// errno

/* [hypothesis] Wind of the flakes, fixed point within [-4, 4]
 * [verified: draw_snow]. */
// GLOBAL: HAPPY 0x00413d3c
fixed wind;

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

/* The levels: data/xlevels.dat or data/levels.dat, loaded by start_menu
 * [verified]; FUN_004019C0 counts its entries. */
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

/* [hypothesis] x of the player's past positions (512 entries, up to
 * DAT_004154c0), see reset_player. */
// GLOBAL: HAPPY 0x00414cc0
int DAT_00414cc0[512];

/* [hypothesis] y of the player's past positions, see reset_player. */
// GLOBAL: HAPPY 0x004154c0
int DAT_004154c0[512];

/* Unloaded by shutdown_game [verified]. */
// GLOBAL: HAPPY 0x00415cc0
DATAFILE *DAT_00415cc0;

// GLOBAL: HAPPY 0x00415ce0
char help_texts[16][HELP_TEXT_SIZE];

/* One entry per object of DAT_00414140 [verified: draw_objects,
 * FUN_00403130]. 100 entries: they end at scroll_end. */
// GLOBAL: HAPPY 0x00416d00
T_00416d00 DAT_00416d00[100];

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

/* [hypothesis] State of the player and of the current level, a structure
 * (happy.h, PLAYER). */
// GLOBAL: HAPPY 0x0042f500
PLAYER player;

/* custom_game: number of bonus maps of DAT_0043f5c0 (at most 8, saved in
 * happy.clv) [verified]. */
// GLOBAL: HAPPY 0x0042f568
unsigned char DAT_0042f568;

/* [hypothesis] Control keys, as Allegro KEY_ codes: default_settings sets
 * 0 RIGHT, 1 LEFT, 4 ALT, 5 LCONTROL, 6 DOWN [verified]; 5 closes
 * message_box. Saved in happy.sav (28 bytes). set_controls names them
 * [verified]: 0 right, 1 left, 4 jump, 5 talk, 6 slide; it never sets 2
 * and 3, but refuses a key equal to them. */
// GLOBAL: HAPPY 0x0042f580
int controls[7];

/* fixed, 1/10 [verified: init_game] */
// GLOBAL: HAPPY 0x0042f59c
fixed DAT_0042f59c;

/* Translucency table: init_color_tables builds it with
 * create_trans_table(..., 128, 128, 128, NULL) [verified]. */
// GLOBAL: HAPPY 0x0042f5a0
COLOR_MAP trans_table;

/* The byte right after trans_table. */
// GLOBAL: HAPPY 0x0043f5a0
unsigned char DAT_0043f5a0;

/* .map file names, 8 entries: the next global is at 0x00440038. */
// GLOBAL: HAPPY 0x0043f5c0
T_0043f5c0 DAT_0043f5c0[8];

/* [hypothesis] Current level, 1-based: FUN_00404770 prints password
 * DAT_00440038 - 1. Signed [verified: movsx in FUN_00404770]. */
// GLOBAL: HAPPY 0x00440038
char DAT_00440038;

/* Scrolling message position [verified: start_scroll_message,
 * draw_scroll_message]. */
// GLOBAL: HAPPY 0x0044003c
int scroll_x;

// GLOBAL: HAPPY 0x00440040
int scroll_y;

/* fixed, 1.0 [verified: init_game] */
// GLOBAL: HAPPY 0x00440044
fixed DAT_00440044;

/* [hypothesis] Number of bonus items (item 18) in the level: signed
 * [verified: movsx in take_item]. */
// GLOBAL: HAPPY 0x00440048
char DAT_00440048;

// GLOBAL: HAPPY 0x0044004c
int DAT_0044004c;

/* [hypothesis] Object templates, 40 of them (the next global is at
 * 0x004404C0); FUN_00401160 fills 0 to 34. */
// GLOBAL: HAPPY 0x00440060
OBJ DAT_00440060[40];

/* fixed, 12/100 [verified: init_game] */
// GLOBAL: HAPPY 0x004404c0
fixed DAT_004404c0;

/* .map file names, 18 entries: the next global is at 0x00441C80. */
// GLOBAL: HAPPY 0x004404e0
T_0043f5c0 DAT_004404e0[18];

/* [hypothesis] The game palette: init_color_tables builds its three tables
 * from it. */
// GLOBAL: HAPPY 0x00441c80
PALETTE DAT_00441c80;

// GLOBAL: HAPPY 0x00442080
unsigned char DAT_00442080;

/* Incremented every 15 ms by timer_15ms (0x004018F0), installed by
 * install_int(timer_15ms, 15) at 0x00401F6B [verified]. */
// GLOBAL: HAPPY 0x00442084
volatile int timer_ticks;

/* [hypothesis] Digital sound volume: saved in happy.sav, 200 by default. */
// GLOBAL: HAPPY 0x00442088
unsigned char DAT_00442088;

/* [hypothesis] Flakes (draw_snow, 0-511). 512 entries: the next global is
 * at 0x004450A4. */
// GLOBAL: HAPPY 0x004420a0
PARTICLE snow[512];

// GLOBAL: HAPPY 0x004450a0
unsigned char DAT_004450a0;

// GLOBAL: HAPPY 0x004450a4
int DAT_004450a4;

/* Number of entries of DAT_00414c90, minus 8 or 9 (FUN_004019C0); in
 * custom_game, the number of level maps of DAT_004404e0 (at most 18,
 * saved in happy.clv) [verified]. */
// GLOBAL: HAPPY 0x004450a8
unsigned char DAT_004450a8;

/* Scrolling message active [verified: set by start_scroll_message, cleared
 * by draw_scroll_message at the end]. */
// GLOBAL: HAPPY 0x004450ac
int scroll_active;

// GLOBAL: HAPPY 0x004450b0
unsigned char DAT_004450b0;

// GLOBAL: HAPPY 0x004450b4
DATAFILE *map_datafile;

/* Particles (draw_particles): 256 entries [verified: loop bound]. */
// GLOBAL: HAPPY 0x004450c0
PARTICLE particles[256];
