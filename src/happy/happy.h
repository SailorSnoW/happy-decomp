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

/* A particle, 24 bytes [verified: stride in 0x004021A0 and 0x00402260].
 * Positions and speeds are fixed point. */
typedef struct PARTICLE {
    fixed x;
    fixed y;
    fixed dx;
    fixed dy;
    /* vertical acceleration (draw_particles only) */
    fixed ddy;
    unsigned char color;
    unsigned char active;
    /* frames left (draw_particles only) */
    unsigned char life;
} PARTICLE;

/* 20 bytes [verified: stride in draw_objects and FUN_00403130]; one per
 * object of DAT_00414140. */
typedef struct T_00416d00 {
    /* [hypothesis] saved position (FUN_00403130 copies it back) */
    int x;
    int y;
    int f08;
    int f0c;
    /* [hypothesis] upside down: the sprite is flipped vertically */
    int f10;
} T_00416d00;

/* 335 bytes [verified: stride in load_level]. f000 is a .map file name
 * [verified: passed to map_load]. */
typedef struct T_0043f5c0 {
    char f000[255];
    char f0ff[80];
} T_0043f5c0;

/* data.c */
extern char *passwords[21];
extern int DAT_00412088;
extern int DAT_0041208c;
extern int DAT_00412090;
extern int DAT_00413b20;
extern int DAT_00413b24;
extern int DAT_00413b28;
extern fixed wind;
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
extern T_00416d00 DAT_00416d00[100];
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
extern PARTICLE snow[512];
extern volatile int timer_ticks;
extern unsigned char DAT_00442088;
extern int DAT_004450a4;
extern unsigned char DAT_004450a8;
extern PARTICLE particles[256];
extern int scroll_active;

extern int DAT_00413b2c;
extern int DAT_00413b30;
extern int DAT_00413b34;
extern int DAT_0044004c;
extern int DAT_00414cc0[512];
extern int DAT_004154c0[512];
extern fixed DAT_0042f500;
extern fixed DAT_0042f504;
extern int DAT_0042f508;
extern int DAT_0042f510;
extern unsigned char DAT_0042f514;
extern unsigned char DAT_0042f515;
extern int DAT_0042f520;
extern int DAT_0042f53c;
extern int DAT_0042f540;
extern int DAT_0042f544;
extern int DAT_0042f548;
extern unsigned char DAT_0042f54c;
extern unsigned char DAT_0042f54d;
extern unsigned char DAT_0042f54e;
extern unsigned char DAT_0042f54f;
extern unsigned char DAT_0042f550;
extern int DAT_0042f554;
extern int DAT_0042f558;
extern int DAT_0042f55c;
extern int DAT_0042f560;
extern unsigned char DAT_0042f564;
extern unsigned char DAT_0043f5a0;
extern T_0043f5c0 DAT_0043f5c0[8];
extern char DAT_00440038;
extern char DAT_00440048;
extern T_0043f5c0 DAT_004404e0[18];
extern unsigned char DAT_00442080;
extern unsigned char DAT_004450a0;
extern unsigned char DAT_004450b0;

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

/* block_03.c */
void draw_particles(int ox, int oy);
void draw_snow(int ox, int oy, int from, int to);
void draw_objects(int ox, int oy);
void FUN_00402620(int a, int b, int c);
void init_level_palette(void);

/* block_04.c */
void FUN_00403030(void);
void load_level_objects(char *filename);
void FUN_00403130(void);
void load_level(int n, int keep, int custom);
void reset_player(int full);

/* block_05.c */
void spawn_particle(int x, int y, int color, int dir);
void open_exit(void);
void FUN_004036c0(void);
void start_level(int n, int keep, int custom);
void FUN_00403a40(void);
unsigned char take_item(int x, int y);
void FUN_00403ce0(int i);
void FUN_00403da0(int i);
int FUN_00403e00(BITMAP *spr1, BITMAP *spr2, int x1, int y1, int x2, int y2);
void FUN_00403f60(void);
void FUN_00404110(void);
void update_objects(void);
void kill_player(void);
void FUN_00404770(void);
void FUN_00404920(void);
void drop_item(void);
void pause_game(void);
void FUN_00404c50(void);
void bonus_room(void);

/* block_06.c */
int wait_or_esc(int ms);

/* block_07.c */
void anim_tick(int a, int b);

#endif
