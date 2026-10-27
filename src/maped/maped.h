/* Globals and functions of maped.exe shared between its source files.
 * Globals are defined in data.c, in address order. */
#ifndef MAPED_H
#define MAPED_H

#include <allegro.h>
#include "map.h"
#include "block_01.h"

/* [hypothesis] The map being edited: every editing function reads it
 * (notes/data/datarefs-maped.csv, 0x00410110); same role as happy
 * DAT_00414130. */
extern MAP *DAT_00410110;

/* Values of the six colour sliders (data.c). */
extern int slider_values[6];
extern int DAT_0040e050;
extern int DAT_0040e054;
extern MENU file_menu[32];
extern DIALOG properties_dialog[33];
extern DIALOG text_dialog[6];
extern DIALOG colors_dialog[10];
extern char DAT_00410740[80];
extern char DAT_00410790[16];
extern char DAT_004107e0[256];
extern char DAT_0042a840[32];

/* [hypothesis] Palette of the editor (set_palette in FUN_00401400). */
extern PALETTE DAT_0042ace0;

/* [hypothesis] Number of the next screenshot (take_screenshot). */
extern int DAT_00410118;
/* [hypothesis] Folder of the file selector, "./maps/" at start (1024
 * bytes up to the next global). */
extern char DAT_00410340[1024];
/* Counts set by FUN_004019b0: DAT_004107a0 = DAT_004107a4 = tiles of
 * map_datafile after the first two entries [verified]. */
extern int DAT_004107a0;
extern int DAT_004107a4;
extern int DAT_004107a8;
extern int DAT_004107ac;
extern int DAT_004107b0;
extern int DAT_004107b4;
/* [hypothesis] data/sprites.dat (load_data). */
extern DATAFILE *DAT_004113d4;
/* Light table, made current by init_color_tables [verified]. */
extern COLOR_MAP DAT_00412400;
/* [hypothesis] cursor position in the view, in tiles (clamped to the view
 * by set_screen_mode) */
extern int DAT_00422408;
extern int DAT_0042240c;
/* [hypothesis] Back buffer, 640x480 (init_editor). */
extern BITMAP *DAT_00422410;
/* RGB table, made current by init_color_tables [verified]. */
extern RGB_MAP DAT_00422420;
/* [hypothesis] scroll position of the view, in tiles (clamped to the map
 * by set_screen_mode) */
extern int DAT_0042a420;
extern int DAT_0042a424;
/* [hypothesis] File name of the map, "unnamed.map" at start (1024 bytes
 * up to the next global). */
extern char DAT_0042a440[1024];
/* [hypothesis] size of the view in tiles: screen height and width / 16 */
extern int DAT_0042acc4;
extern int DAT_0042acc8;

void FUN_00401400(void);
int slider_changed(void *dp3, int d2);
int FUN_004015d0(int msg, DIALOG *d, int c);
int take_screenshot(void);
void FUN_004019b0(void);
void set_screen_mode(int mode);
void init_color_tables(void);
void load_data(void);
void init_editor(void);
void shutdown_editor(void);

#endif
