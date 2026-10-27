/* Globals and functions of happy.exe shared between its source files.
 * Globals are defined in data.c, in address order. */
#ifndef HAPPY_H
#define HAPPY_H

#include <allegro.h>
#include "map.h"
#include "block_01.h"

/* One high score, 24 bytes [verified: stride 0x18 in init_hiscores]. */
typedef struct HISCORE {
    char name[20];
    unsigned int score;
} HISCORE;

/* data.c */
extern int DAT_00412088;
extern int DAT_00413b20;
extern int DAT_00413b24;
extern int DAT_00413b28;
extern unsigned char midi_volume;
extern HISCORE hiscores[7];
extern fixed DAT_00414024;
extern fixed DAT_00414028;
extern volatile int fps;
extern MAP *DAT_00414130;
extern volatile int frame_count;
extern char scroll_text[80];
extern DATAFILE *DAT_00414c90;
extern int DAT_00414c94;
extern fixed DAT_00414c98;
extern int DAT_00414ca0;
extern int DAT_00414ca4;
extern DATAFILE *DAT_00415cc0;
extern int scroll_end;
extern int scroll_width;
extern BITMAP *DAT_004274e4;
extern RGB_MAP rgb_table;
extern unsigned char DAT_0042f516;
extern fixed DAT_0042f51c;
extern int DAT_0042f52c;
extern int DAT_0042f530;
extern unsigned char DAT_0042f568;
extern fixed DAT_0042f59c;
extern COLOR_MAP trans_table;
extern int scroll_x;
extern int scroll_y;
extern fixed DAT_00440044;
extern fixed DAT_004404c0;
extern PALETTE DAT_00441c80;
extern volatile int timer_ticks;
extern unsigned char DAT_00442088;
extern int DAT_004450a4;
extern unsigned char DAT_004450a8;
extern int scroll_active;

/* block_02.c */
void timer_1s(void);
void timer_15ms(void);
void save_screenshot(BITMAP *bmp, char *filename);
void init_hiscores(void);
int FUN_004019c0(void);
void init_color_tables(void);
void start_scroll_message(char *text);
void draw_scroll_message(BITMAP *bmp);
int yes_no_box(char *text);
int load_data(void);
void save_settings(void);
int load_settings(void);
void default_settings(void);
void init_game(void);
void shutdown_game(void);

/* block_06.c */
int wait_or_esc(int ms);

/* block_07.c */
void anim_tick(int a, int b);

#endif
