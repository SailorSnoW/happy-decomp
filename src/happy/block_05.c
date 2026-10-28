/* Gameplay: happy block_05 (0x00403450-0x0040639F).
 * notes/layout-happy.md §8. Particles, objects and the player, events of a
 * level, pause, end of a level, the game loop (play_level) and the bonus
 * room. happy only. */
#include <stdlib.h>
#include <allegro.h>
#include "happy.h"

/* [hypothesis] Starts a particle at (x, y) pixels, in the first free slot
 * of particles (the last one if all are taken) [verified]. dir == -1: any
 * direction (dx/dy = cos/sin of a random angle), constant acceleration
 * _cos_tbl[388]; otherwise a direction within dir +/- 10 (binary angle),
 * acceleration 0.1. Lifetime 60 to 79 frames. 6 callers. */
// FUNCTION: HAPPY 0x00403450
void spawn_particle(int x, int y, int color, int dir)
{
    int i;

    for (i = 0; particles[i].active && i < 255; i++)
        ;
    particles[i].x = itofix(x);
    particles[i].y = itofix(y);
    if (dir == -1) {
        particles[i].dx = fixcos(itofix(rand() % 256));
        particles[i].dy = fixsin(itofix(rand() % 256));
        particles[i].ddy = fixsin(itofix(2));
    } else {
        particles[i].dx = fixcos(itofix(rand() % 20 + dir - 10));
        particles[i].dy = fixsin(itofix(rand() % 20 + dir - 10));
        particles[i].ddy = 0x1999;
    }
    particles[i].color = color;
    particles[i].life = rand() % 20 + 60;
    particles[i].active = 1;
}

/* [hypothesis] Opens the level exit [verified]: clears t1 and the
 * collision t2 of the cells where t4 == 3, with 5 particles (colours 164 to
 * 175) on each; player.f48 += 500, DAT_004450a0 = 1, sample 27, message
 * "Level exit is now open!". Callers 0x00403820, 0x004041F0. */
// FUNCTION: HAPPY 0x004035b0
void open_exit(void)
{
    int x;
    int y;
    int i;
    int j;

    for (x = 0; x < DAT_00414130->w; x++) {
        for (y = 0; y < DAT_00414130->h; y++) {
            i = DAT_00414130->w * y + x;
            if (DAT_00414130->tiles[i].t4 == 3) {
                DAT_00414130->tiles[i].t1 = 0;
                DAT_00414130->tiles[i].t2 = 0;
                for (j = 0; j < 5; j++)
                    spawn_particle(x * 16 + 8, y * 16 + 8, rand() % 12 + 164, -1);
            }
        }
    }
    player.f48 += 500;
    DAT_004450a0 = 1;
    play_sample(misc_datafile[27].dat, DAT_00442088, 128, 1000, 0);
    // STRING: HAPPY 0x004125a4
    start_scroll_message("Level exit is now open!");
}

/* [hypothesis] Switches: swaps the tiles 58/59 (t0 and t1) of the cells
 * where t4 == 6, setting the collision t2 to 4 or 0, and the tiles 60/61 of
 * the cells where t4 == 7 [verified]; then plays sample 33 of
 * misc_datafile. Only caller 0x00404C50. */
// FUNCTION: HAPPY 0x004036c0
void FUN_004036c0(void)
{
    int x;
    int y;
    int i;

    for (x = 0; x < DAT_00414130->w; x++) {
        for (y = 0; y < DAT_00414130->h; y++) {
            i = DAT_00414130->w * y + x;
            if (DAT_00414130->tiles[i].t4 == 6) {
                if (DAT_00414130->tiles[i].t0 == 58) {
                    DAT_00414130->tiles[i].t0 = 59;
                    DAT_00414130->tiles[i].t2 = 4;
                } else if (DAT_00414130->tiles[i].t0 == 59) {
                    DAT_00414130->tiles[i].t0 = 58;
                    DAT_00414130->tiles[i].t2 = 0;
                }
                if (DAT_00414130->tiles[i].t1 == 58) {
                    DAT_00414130->tiles[i].t1 = 59;
                    DAT_00414130->tiles[i].t2 = 4;
                } else if (DAT_00414130->tiles[i].t1 == 59) {
                    DAT_00414130->tiles[i].t1 = 58;
                    DAT_00414130->tiles[i].t2 = 0;
                }
            }
            if (DAT_00414130->tiles[i].t4 == 7) {
                if (DAT_00414130->tiles[i].t0 == 60)
                    DAT_00414130->tiles[i].t0 = 61;
                else if (DAT_00414130->tiles[i].t0 == 61)
                    DAT_00414130->tiles[i].t0 = 60;
                if (DAT_00414130->tiles[i].t1 == 60)
                    DAT_00414130->tiles[i].t1 = 61;
                else if (DAT_00414130->tiles[i].t1 == 61)
                    DAT_00414130->tiles[i].t1 = 60;
            }
        }
    }
    play_sample(misc_datafile[33].dat, DAT_00442088, 128, 2000, 0);
}

/* [hypothesis] Starts level n [verified]: stops the particles, scatters
 * the 512 flakes, loads the level (load_level(n, keep, custom)), removes
 * the enemies (bit 2 of f19) unless DAT_0041208c, plays music MAP.f78
 * (entry 8 + f78 of misc_datafile, 0 if above 11), resets the player
 * (reset_player(keep)), opens the exit if DAT_004450a0 or DAT_00413b34,
 * shows the level name over the first frame and fades in through a palette
 * where only the entries 164 to 191 are kept; time_left = MAP.f7a.
 * Callers 0x00405DB0, 0x00409640. */
// FUNCTION: HAPPY 0x00403820
void start_level(int n, int keep, int custom)
{
    PALETTE pal;
    int i;

    for (i = 0; i < 256; i++)
        particles[i].active = 0;
    for (i = 0; i < 512; i++) {
        snow[i].x = itofix(rand() % 320);
        snow[i].y = itofix(rand() % 200);
        snow[i].dx = 0;
        snow[i].dy = itofix(rand() % 8 + 3) / 10;
        snow[i].active = 1;
    }
    wind = 0;
    vsync();
    clear_bitmap(screen);
    set_palette(black_palette);
    load_level(n, keep, custom);
    if (DAT_0041208c == 0) {
        for (i = 0; i < 100; i++) {
            if (DAT_00414140[i].f1a && (DAT_00414140[i].f19 & 4))
                DAT_00414140[i].f1a = 0;
        }
    }
    if (DAT_00414130->f78 > 11)
        DAT_00414130->f78 = 0;
    play_midi((MIDI *)misc_datafile[DAT_00414130->f78 + 8].dat, 1);
    reset_player(keep);
    if (DAT_004450a0 || DAT_00413b34)
        open_exit();
    set_palette(black_palette);
    vsync();
    draw_level(0, 0, time_left);
    textout_centre(buffer, (FONT *)misc_datafile[4].dat, DAT_00414130->name, 160, 70, -1);
    blit_to_screen(buffer);
    for (i = 0; i < 256; i++) {
        if (i >= 164 && i <= 191) {
            pal[i].r = DAT_00441c80[i].r;
            pal[i].g = DAT_00441c80[i].g;
            pal[i].b = DAT_00441c80[i].b;
        } else {
            pal[i].r = pal[i].g = pal[i].b = 0;
        }
    }
    vsync();
    fade_in(pal, 2);
    fade_from(pal, DAT_00441c80, 2);
    time_left = DAT_00414130->f7a;
    DAT_0044004c = 0;
}

/* Unless player.f20 is set, advances the 2-frame animation counter
 * player.f2c every 8 calls [verified] (same counter as anim_tick,
 * 0x00409010). Callers 0x00404C50, 0x00405DB0. */
// FUNCTION: HAPPY 0x00403a40
void FUN_00403a40(void)
{
    if (player.f20 == 0 && ++player.f30 == 8) {
        player.f30 = 0;
        if (++player.f2c > 1)
            player.f2c = 0;
    }
}

/* [hypothesis] Picks up the item of the cell at (x, y) pixels: t3 of the
 * cell indexes DAT_00414040 [verified]. Adds f0 to player.f48 (score?);
 * every 250 items worth 10, an extra life (player.f40 + 1); then the
 * flags f4: 2 counts towards the bonus room (player.f50 against
 * DAT_00440048), 0x10 adds 60 s to time_left, 8 adds to player.f44 up to
 * 3 and is not taken when it is already 3, 1 sets player.f4c, 4 sets
 * player.f16 to 20, 0x20 counts the magic spheres (player.f60) unless
 * DAT_00413b20 or DAT_00442080. A taken item leaves 15 particles and
 * clears t3. Returns t3 (0: nothing). Only caller 0x00404C50. */
// FUNCTION: HAPPY 0x00403a80
unsigned char take_item(int x, int y)
{
    int i;
    int t;
    int take;
    int j;

    take = 1;
    i = DAT_00414130->w * (y >> 4) + (x >> 4);
    t = DAT_00414130->tiles[i].t3;
    if (t == 0)
        return 0;
    player.f48 += DAT_00414040[t].f0;
    if (DAT_00414040[t].f0 == 10 && ++player.f64 == 250) {
        // STRING: HAPPY 0x004125fc
        start_scroll_message("E X T R A   L I F E !");
        player.f40++;
        player.f64 = 0;
        play_sample(misc_datafile[31].dat, DAT_00442088, 128, 1000, 0);
    }
    if (DAT_00414040[t].f4 & 2) {
        player.f50++;
        if (player.f50 == DAT_00440048)
            // STRING: HAPPY 0x004125dc
            start_scroll_message("Bonusroom available at exit!");
    }
    if (DAT_00414040[t].f4 & 0x10)
        time_left += 60;
    if ((DAT_00414040[t].f4 & 8) && player.f44 == 3)
        take = 0;
    if ((DAT_00414040[t].f4 & 8) && player.f44 < 3)
        player.f44 = MIN(player.f44 + 1, 3);
    if ((DAT_00414040[t].f4 & 1) && !player.f4c & 1)
        player.f4c++;
    if (DAT_00414040[t].f4 & 4)
        player.f16 = 20;
    if ((DAT_00414040[t].f4 & 0x20) && DAT_00413b20 == 0 && !DAT_00442080) {
        if (++player.f60 == 5)
            // STRING: HAPPY 0x004125bc
            start_scroll_message("You found five magic spheres!");
        for (j = 0; j < 35; j++)
            spawn_particle(x, y, rand() % 8 + 247, -1);
    }
    if (take) {
        for (j = 0; j < 15; j++)
            spawn_particle(x, y, DAT_00414040[t].f8, -1);
        play_sample(misc_datafile[32].dat, DAT_00442088, 128, rand() % 800 + 1000, 0);
        DAT_00414130->tiles[i].t3 = 0;
    }
    return t;
}

/* Object i starts following the player [hypothesis]: player.f48 += 100,
 * player.f58 + 1, saves its position in DAT_00416d00[i], f1b = 2, f08 =
 * player.f54 - 13 * player.f58 modulo 512 and f0c = 13 * player.f58
 * [verified] ([hypothesis] an index into the 512 saved positions of
 * DAT_00414cc0, i.e. a delay behind the player). Plays sample 26. Only
 * caller FUN_00403f60. [verified] Order of the first two statements: the
 * only one of the 120 orders of the first five that matches. */
// FUNCTION: HAPPY 0x00403ce0
void FUN_00403ce0(int i)
{
    player.f58++;
    player.f48 += 100;
    DAT_00416d00[i].x = DAT_00414140[i].x;
    DAT_00416d00[i].y = DAT_00414140[i].y;
    DAT_00414140[i].f1b = 2;
    DAT_00414140[i].f08 = player.f54 - player.f58 * 13;
    DAT_00414140[i].f0c = player.f58 * 13;
    if (DAT_00414140[i].f08 < 0)
        DAT_00414140[i].f08 += 512;
    play_sample(misc_datafile[26].dat, DAT_00442088, 128, rand() % 400 + 800, 0);
}

/* [hypothesis] The player is hurt: sample 34, player.f44 - 1 (lives?),
 * player.f4d = 150, player.f4e = 16, player.f4f = 4 or -4 depending
 * on player.f08 [verified]. i (the object) is passed but not used
 * [verified: push in FUN_00403f60]. Only caller FUN_00403f60. */
// FUNCTION: HAPPY 0x00403da0
void FUN_00403da0(int i)
{
    play_sample(misc_datafile[34].dat, DAT_00442088, 128, 1000, 0);
    player.f2c = 0;
    player.f4d = 150;
    player.f4e = 16;
    player.f4f = player.f08 ? 4 : -4;
    player.f44--;
}

/* [hypothesis] Pixel-perfect collision of the bitmaps spr1 at (x1, y1) and
 * spr2 at (x2, y2): 1 if their rectangles overlap and a pixel of the
 * overlap is not the mask colour in both [verified]. Only 8-bit bitmaps are
 * tested; the mask colour is spr1's. [hypothesis] The 8-bit case of
 * check_pp_collision of the PPCol library (Ivan Baldo): same structure and
 * same order of the computations, and the code only matches with it. Only
 * caller FUN_00403f60. */
// FUNCTION: HAPPY 0x00403e00
int FUN_00403e00(BITMAP *spr1, BITMAP *spr2, int x1, int y1, int x2, int y2)
{
    int dx1;
    int dx2;
    int dy1;
    int dy2;
    int fx;
    int fy;
    int sx1;
    int sx2;
    int maxw;
    int maxh;
    int depth;
    char mask;

    if (x1 >= x2 + spr2->w || x2 >= x1 + spr1->w || y1 >= y2 + spr2->h || y2 >= y1 + spr1->h)
        return 0;
    if (x1 > x2) {
        dx1 = 0;
        dx2 = x1 - x2;
    } else {
        dx1 = x2 - x1;
        dx2 = 0;
    }
    if (y1 > y2) {
        dy1 = 0;
        dy2 = y1 - y2;
    } else {
        dy1 = y2 - y1;
        dy2 = 0;
    }
    if (spr1->w - dx1 > spr2->w - dx2)
        maxw = spr2->w - dx2;
    else
        maxw = spr1->w - dx1;
    if (spr1->h - dy1 > spr2->h - dy2)
        maxh = spr2->h - dy2;
    else
        maxh = spr1->h - dy1;
    maxw--;
    maxh--;
    fy = dy1;
    fx = dx1;
    dy1 += maxh;
    dy2 += maxh;
    sx1 = dx1 + maxw;
    sx2 = dx2 + maxw;
    depth = bitmap_color_depth(spr1);
    if (depth == 8) {
        mask = bitmap_mask_color(spr1);
        for (; dy1 >= fy; dy1--, dy2--) {
            for (dx1 = sx1, dx2 = sx2; dx1 >= fx; dx1--, dx2--) {
                if (spr1->line[dy1][dx1] != mask && spr2->line[dy2][dx2] != mask)
                    return 1;
            }
        }
    }
    return 0;
}

/* [hypothesis] Contacts between the player and the objects [verified]:
 * for each active object whose sprite touches the player's (sprite 64 +
 * player.f2c, top left = player position - (8, 21)):
 * - f19 bit 1 and controls[5] pressed: help text f1b (message_box);
 * - f19 bit 0, f1b == 0 and player.f58 < DAT_00412090: FUN_00403ce0;
 * - f19 bit 2, unless DAT_00413b30: with player.f2c == 8 and
 *   DAT_00416d00[i].f10 == 0, sample 34 and the object is flipped
 *   (f10 = 26, speed -3); otherwise, unless player.f16 or player.f4d,
 *   FUN_00403da0.
 * Only caller 0x00404C50. */
// FUNCTION: HAPPY 0x00403f60
void FUN_00403f60(void)
{
    int i;

    for (i = 0; i < 100; i++) {
        if (DAT_00414140[i].f1a) {
            if (FUN_00403e00(DAT_00415cc0[DAT_00414140[i].f13 + DAT_00414140[i].f14].dat,
                             DAT_00415cc0[player.f2c + 64].dat, DAT_00414140[i].x,
                             DAT_00414140[i].y, fixtoi(player.x) - 8,
                             fixtoi(player.y) - 21)) {
                if ((DAT_00414140[i].f19 & 2) && keys[controls[5]])
                    message_box(DAT_00414140[i].f1b, NULL, 0);
                if ((DAT_00414140[i].f19 & 1) && DAT_00414140[i].f1b == 0
                    && player.f58 < DAT_00412090)
                    FUN_00403ce0(i);
                if ((DAT_00414140[i].f19 & 4) && DAT_00413b30 == 0) {
                    if (player.f2c == 8 && DAT_00416d00[i].f10 == 0) {
                        play_sample(misc_datafile[34].dat, DAT_00442088, 128, rand() % 400 + 300, 0);
                        DAT_00416d00[i].f0c = itofix(-3);
                        DAT_00416d00[i].f08 = itofix(DAT_00414140[i].y);
                        DAT_00416d00[i].f10 = 26;
                        DAT_00416d00[i].y = DAT_00414140[i].y;
                    } else if (!player.f16 && !player.f4d && DAT_00416d00[i].f10 == 0) {
                        FUN_00403da0(i);
                    }
                }
            }
        }
    }
}

/* [hypothesis] All the Happylanders are saved [verified]: every active
 * object with bit 2 of f19 disappears in 10 particles of colour
 * DAT_00414040[18].f8 and leaves item 18 in the cell below it (not on a
 * solid cell, t2 > 3), counted in DAT_00440048; then the message. Only
 * caller 0x004041F0. */
// FUNCTION: HAPPY 0x00404110
void FUN_00404110(void)
{
    int i;
    int j;
    int k;

    for (i = 0; i < 100; i++) {
        if (DAT_00414140[i].f1a && (DAT_00414140[i].f19 & 4)) {
            for (j = 0; j < 10; j++)
                spawn_particle(DAT_00414140[i].x, DAT_00414140[i].y, DAT_00414040[18].f8, -1);
            DAT_00440048++;
            k = DAT_00414130->w * ((DAT_00414140[i].y >> 4) + 1) + (DAT_00414140[i].x >> 4);
            if (DAT_00414130->tiles[k].t3 == 18)
                DAT_00440048--;
            DAT_00414130->tiles[k].t3 = 18;
            if (DAT_00414130->tiles[k].t2 > 3) {
                DAT_00440048--;
                DAT_00414130->tiles[k].t3 = 0;
            }
            DAT_00414140[i].f1a = 0;
        }
    }
    // STRING: HAPPY 0x00412614
    start_scroll_message("All Happylanders have been saved!");
}

/* [hypothesis] The player dies [verified]: player.f40 - 1 (lives?),
 * player.f3c = 0; with the camera frozen on the player (clamped to the
 * map), sprite 65 flipped upside down jumps up (speed -2, gravity
 * DAT_004404c0, at most 6) and falls 220 pixels, drifting one pixel per
 * frame away from player.f08; the objects keep moving and the followers
 * keep receiving the player's positions. "TIME OUT!" when time_left < 0,
 * then a fade out; player.f44 = 1. Only caller 0x00404C50. */
// FUNCTION: HAPPY 0x00404480
void kill_player(void)
{
    int dir;
    int cx;
    int cy;
    int x;
    fixed y;
    fixed dy;
    fixed end;

    dir = -1;
    player.f40--;
    player.f3c = 0;
    cx = MIN(MAX(fixtoi(player.x) - 159, 0), (DAT_00414130->w - 20) * 16);
    cy = MIN(MAX(fixtoi(player.y) - 100, 0), DAT_00414130->h * 16 - 216);
    x = fixtoi(player.x);
    y = player.y;
    dy = itofix(-2);
    end = player.y + itofix(220);
    if (player.f08)
        dir = 1;
    while (y < end) {
        timer_ticks = 0;
        draw_level(0, 0, time_left);
        update_objects();
        draw_sprite_v_flip(buffer, (BITMAP *)DAT_00415cc0[65].dat, x - cx - 8, fixtoi(y) - cy - 21);
        if (time_left < 0)
            // STRING: HAPPY 0x00412638
            textout_centre(buffer, (FONT *)misc_datafile[4].dat, "TIME OUT!", 160, 70, -1);
        blit_to_screen(buffer);
        dy = MIN(dy + DAT_004404c0, itofix(6));
        y += dy;
        x += dir;
        DAT_00414cc0[player.f54] = x - 8;
        DAT_004154c0[player.f54] = fixtoi(y) - 17;
        if (++player.f54 == 512)
            player.f54 = 0;
        while (!timer_ticks)
            ;
    }
    if (time_left < 0)
        fade_out_range(2, 0, 163);
    fade_out(4);
    player.f44 = 1;
}

/* [hypothesis] "You earned a password!" screen [verified]: five sprites
 * 152 appear one by one (sample 32), then the password of level
 * DAT_00440038 is decoded (each character - 1) and printed; waits for Space
 * or Enter. Only caller 0x00409640. */
// FUNCTION: HAPPY 0x00404770
void FUN_00404770(void)
{
    char s[40];
    int x;
    int i;

    fade_out(4);
    clear_bitmap(screen);
    clear_bitmap(buffer);
    set_palette(DAT_00441c80);
    rest(400);
    for (x = 92; x < 242; x += 30) {
        draw_sprite(buffer, (BITMAP *)DAT_00415cc0[152].dat, x, 80);
        blit_to_screen(buffer);
        play_sample(misc_datafile[32].dat, DAT_00442088, 128, rand() % 800 + 600, 0);
        rest(400);
    }
    rest(400);
    fade_out(4);
    clear_bitmap(buffer);
    for (i = 0; i < 40; i++) {
        if (passwords[DAT_00440038 - 1][i])
            s[i] = passwords[DAT_00440038 - 1][i] - 1;
        else
            s[i] = 0;
    }
    // STRING: HAPPY 0x00412648
    textout_centre(buffer, (FONT *)misc_datafile[4].dat, "You earned a password!", 160, 60, -1);
    // STRING: HAPPY 0x00412644
    textprintf_centre(buffer, (FONT *)misc_datafile[4].dat, 160, 90, -1, "%s", s);
    blit_to_screen(buffer);
    fade_in(DAT_00441c80, 4);
    while (!key[KEY_SPACE] && !key[KEY_ENTER])
        ;
    fade_out(4);
}

/* [hypothesis] End of a level [verified]: fades in from a palette where
 * only the entries 164 to 191 are kept, with sample 31 and "Level
 * complete!"; then the time left is counted down, 5 points per second,
 * one timer tick per second, the level frame redrawn each time
 * (draw_level(0, 1, t)); fades out. Only caller 0x00409640. */
// FUNCTION: HAPPY 0x00404920
void FUN_00404920(void)
{
    PALETTE pal;
    int t;
    int i;

    t = time_left;
    for (i = 0; i < 256; i++) {
        if (i >= 164 && i <= 191) {
            pal[i].r = DAT_00441c80[i].r;
            pal[i].g = DAT_00441c80[i].g;
            pal[i].b = DAT_00441c80[i].b;
        } else {
            pal[i].r = pal[i].g = pal[i].b = 0;
        }
    }
    play_sample(misc_datafile[31].dat, DAT_00442088, 128, 1000, 0);
    if (screen_mode == 0)
        // STRING: HAPPY 0x00412660
        textout_centre(screen, (FONT *)misc_datafile[4].dat, "Level complete!", 160, 70, -1);
    fade_from(DAT_00441c80, pal, 2);
    while (t) {
        player.f48 += 5;
        t--;
        timer_ticks = 0;
        draw_level(0, 1, t);
        textout_centre(buffer, (FONT *)misc_datafile[4].dat, "Level complete!", 160, 70, -1);
        blit_to_screen(buffer);
        while (!timer_ticks)
            ;
    }
    fade_out(8);
}

/* [hypothesis] Moves the objects, once per frame [verified]:
 * - animation: f15 counts up to f16, then f14 (frame) up to f17;
 * - f19 bit 1: faces the player (f10);
 * - f19 bit 0 and f1b == 2 (following the player, FUN_00403ce0): after
 *   the delay f0c, takes the saved player position f08 (DAT_00414cc0 /
 *   DAT_004154c0, 512 entries); on a cell with t4 == 2, while
 *   player.f3c is set and player.f14 is 0, it is saved: sample 28,
 *   player.f48 += 250, player.f15 + 1, f1b = 1; FUN_00404110 when
 *   player.f15 reaches MAP.f7c, open_exit when it reaches MAP.f7b;
 * - f19 bit 2 (enemy): falls while DAT_00416d00[i].f10 counts down (speed
 *   f0c, gravity DAT_00414024), otherwise walks one pixel every f12 + 1
 *   frames and turns around at a wall or at the edge of a floor.
 * [verified] Order of the five statements of the rescue: 3 of the 120
 * orders match, this is one of them. Callers 0x00404480, 0x00404C50,
 * 0x00407500, 0x00408E00. */
// FUNCTION: HAPPY 0x004041f0
void update_objects(void)
{
    int i;

    for (i = 0; i < 100; i++) {
        if (DAT_00414140[i].f1a) {
            if (++DAT_00414140[i].f15 > DAT_00414140[i].f16) {
                DAT_00414140[i].f15 = 0;
                if (++DAT_00414140[i].f14 > DAT_00414140[i].f17)
                    DAT_00414140[i].f14 = 0;
            }
            if (DAT_00414140[i].f19 & 2) {
                if (fixtoi(player.x) > DAT_00414140[i].x)
                    DAT_00414140[i].f10 = 0;
                else
                    DAT_00414140[i].f10 = 1;
            }
            if (DAT_00414140[i].f1b == 2 && (DAT_00414140[i].f19 & 1)) {
                if (DAT_00414140[i].f0c)
                    DAT_00414140[i].f0c--;
                if (DAT_00414140[i].f0c == 0) {
                    DAT_00414140[i].x = DAT_00414cc0[DAT_00414140[i].f08];
                    DAT_00414140[i].y = DAT_004154c0[DAT_00414140[i].f08];
                    if (player.f3c && map_t4_at(DAT_00414130, DAT_00414140[i].x, DAT_00414140[i].y) == 2
                        && !player.f14) {
                        play_sample(misc_datafile[28].dat, DAT_00442088, 128, rand() % 400 + 800, 0);
                        player.f58--;
                        DAT_00414140[i].f1b = 1;
                        player.f14 = 17;
                        player.f15++;
                        player.f48 += 250;
                        if (DAT_00414130->f7c == player.f15)
                            FUN_00404110();
                        if (!DAT_004450a0 && DAT_00414130->f7b <= player.f15)
                            open_exit();
                    }
                }
                if (++DAT_00414140[i].f08 == 512)
                    DAT_00414140[i].f08 = 0;
            }
            if (DAT_00414140[i].f19 & 4) {
                if (DAT_00416d00[i].f10) {
                    DAT_00416d00[i].f0c += DAT_00414024;
                    DAT_00416d00[i].f08 += DAT_00416d00[i].f0c;
                    DAT_00414140[i].y = fixtoi(DAT_00416d00[i].f08);
                    if (--DAT_00416d00[i].f10 == 0)
                        DAT_00414140[i].y = DAT_00416d00[i].y;
                } else {
                    if (++DAT_00414140[i].f11 > DAT_00414140[i].f12) {
                        DAT_00414140[i].f11 = 0;
                        if (DAT_00414140[i].f10)
                            DAT_00414140[i].x--;
                        else
                            DAT_00414140[i].x++;
                    }
                    if (!map_collision_at(DAT_00414130, DAT_00414140[i].x - DAT_00414140[i].f10 * 15 + 15,
                                          DAT_00414140[i].y + 22)
                        || map_collision_at(DAT_00414130, DAT_00414140[i].x - DAT_00414140[i].f10 * 15 + 15,
                                            DAT_00414140[i].y + 16))
                        DAT_00414140[i].f10 = !DAT_00414140[i].f10;
                }
            }
        }
    }
}

/* [hypothesis] Drops a random item 1 to 8 on a random empty cell (t2 and t3
 * both 0; nothing happens otherwise), with 10 particles of the item's
 * colour, DAT_0043f5a0 + 1 and sample 32 [verified]. Only caller
 * 0x00404C50. */
// FUNCTION: HAPPY 0x00404a70
void drop_item(void)
{
    int x;
    int y;
    int i;
    int t;
    int j;

    x = rand() % DAT_00414130->w;
    y = rand() % DAT_00414130->h;
    i = DAT_00414130->w * y + x;
    if (DAT_00414130->tiles[i].t2 == 0 && DAT_00414130->tiles[i].t3 == 0) {
        t = rand() % 8 + 1;
        for (j = 0; j < 10; j++)
            spawn_particle(x * 16 + 7, y * 16 + 7, DAT_00414040[t].f8, -1);
        DAT_00414130->tiles[i].t3 = t;
        DAT_0043f5a0++;
        play_sample(misc_datafile[32].dat, DAT_00442088, 128, rand() % 800 + 600, 0);
    }
}

/* [hypothesis] Pause [verified]: stops the music, waits for the Pause key,
 * darkens the screen through light_table, writes "GAME PAUSED", waits for
 * the key to be released, restarts the music and restores time_left. Only
 * caller 0x00404C50. */
// FUNCTION: HAPPY 0x00404b80
void pause_game(void)
{
    int t;

    t = time_left;
    midi_pause();
    while (!key[KEY_PAUSE])
        ;
    color_map = &light_table;
    drawing_mode(DRAW_MODE_TRANS, NULL, 0, 0);
    rectfill(screen, 0, 0, SCREEN_W, SCREEN_H, 100);
    solid_mode();
    // STRING: HAPPY 0x00412670
    textout_centre(screen, (FONT *)misc_datafile[4].dat, "GAME PAUSED", 160, 80, -1);
    while (key[KEY_PAUSE])
        ;
    midi_resume();
    time_left = t;
}

/* [hypothesis] Plays the current level until it ends [verified: the
 * loop runs until done is set]: returns 1 when the player reaches the exit
 * (map_t4_at == 1 under him), 2 when he dies (player.f44, the energy,
 * reaches 0: kill_player), 3 when Escape is confirmed ("Quit?"). Each
 * frame: copies key[] to keys, "HURRY UP!" at 30 seconds left and a tick
 * (sample 33) each second below, draws the frame (draw_level), moves the
 * objects, then the player: acceleration fixmul(player.f18, f) up to the
 * top speed player.f18 (2.0), with f = 1.0, 0.04 on a slide (bit 3 of
 * MAP.f79 and the slide key) or 0.01 on ice (map_t4_at == 5); slopes
 * (collision types 7-10) push by DAT_0042f59c; walls (types 4-6) and
 * ceilings stop him; jump player.f10 = -5.0, cut by half when the key is
 * released; gravity DAT_00414024 (DAT_004404c0 with the low-G shoes,
 * player.f4c bit 0), at most 7.0 per frame; the landing snaps him to the
 * ground and calls FUN_004036c0 on tile type 7. Then the trail of saved
 * positions (DAT_00414cc0/DAT_004154c0), the countdowns f14, f4d, f4e,
 * the items (take_item), the snow switches (types 8 and 9), deadly tiles
 * (type 4, unless DAT_00413b30), the time limit, Pause and F12
 * (happy.pcx). Callers bonus_room and play_game. */
// FUNCTION: HAPPY 0x00404c50
int play_level(void)
{
    fixed f;
    int done = 0;
    int last = time_left;
    int py;
    int t4;
    int px;
    int c;
    int i;

    player.f0c = 0;
    player.f18 = DAT_00440044 * 2;
    player.f20 = map_collision_at(DAT_00414130, fixtoi(player.x), fixtoi(player.y) + 5) ? FALSE : TRUE;
    do {
        timer_ticks = 0;
        for (i = 0; i < KEY_MAX; i++)
            keys[i] = key[i];
        if (time_left == 30 && !DAT_0044004c) {
            DAT_0044004c = 1;
            // STRING: HAPPY 0x00412690
            start_scroll_message("HURRY UP!");
        }
        if (last != time_left && time_left < 30 && !DAT_00442080)
            play_sample(misc_datafile[33].dat, DAT_00442088, 128, 1000, 0);
        last = time_left;
        draw_level(1, 1, time_left);
        FUN_00403f60();
        update_objects();
        read_joystick();
        px = fixtoi(player.x);
        py = fixtoi(player.y);
        t4 = map_t4_at(DAT_00414130, px, py + 5);
        c = map_collision_at(DAT_00414130, px, py + 10);
        f = DAT_00414028;
        if (keys[controls[6]] && (DAT_00414130->f79 & 8))
            f = DAT_00414c98 * 4;
        if (t4 == 5)
            f = DAT_00414c98;
        if (!player.f4e) {
            if (keys[controls[0]] && !keys[controls[6]]) {
                if (player.f0c < player.f18)
                    player.f0c = MIN(player.f18, player.f0c + fixmul(player.f18, f));
                player.f08 = 0;
                FUN_00403a40();
            }
            if (keys[controls[1]] && !keys[controls[6]]) {
                if (player.f0c > -player.f18)
                    player.f0c = MAX(-player.f18, player.f0c - fixmul(player.f18, f));
                player.f08 = 1;
                FUN_00403a40();
            }
            if ((!keys[controls[0]] && !keys[controls[1]]) || keys[controls[6]]) {
                if (player.f0c > 0)
                    player.f0c = MAX(0, player.f0c - fixmul(player.f18, f));
                if (player.f0c < 0)
                    player.f0c = MIN(0, player.f0c + fixmul(player.f18, f));
                player.f2c = 0;
            }
            if (keys[controls[6]] && player.f0c && (DAT_00414130->f79 & 8))
                player.f2c = 8;
            if (t4 == 5) {
                if (c == 7 || c == 8)
                    player.f0c -= DAT_0042f59c;
                if (c == 9 || c == 10)
                    player.f0c += DAT_0042f59c;
            } else {
                if (c == 7 || c == 8) {
                    if (player.f0c > 0)
                        player.f0c -= DAT_0042f59c * 10;
                    else
                        player.f0c -= DAT_0042f59c;
                }
                if (c == 9 || c == 10) {
                    if (player.f0c < 0)
                        player.f0c += DAT_0042f59c * 10;
                    else
                        player.f0c += DAT_0042f59c;
                }
            }
            player.x += player.f0c;
        } else {
            player.x += itofix(player.f4f);
        }
        player.f34 = map_collision_at(DAT_00414130, fixtoi(player.x) - 8, fixtoi(player.y) - 2);
        player.f38 = map_collision_at(DAT_00414130, fixtoi(player.x) + 7, fixtoi(player.y) - 2);
        if (player.f34 > 3 && player.f34 < 7) {
            player.f0c = 0;
            player.x = itofix(px);
        }
        if (player.f38 > 3 && player.f38 < 7) {
            player.f0c = 0;
            player.x = itofix(px);
        }
        player.f34 = map_collision_at(DAT_00414130, fixtoi(player.x) - 8, fixtoi(player.y) - 16);
        player.f38 = map_collision_at(DAT_00414130, fixtoi(player.x) + 7, fixtoi(player.y) - 16);
        if (player.f34 > 3 || player.f38 > 3) {
            player.x = itofix(px);
            player.f0c = 0;
        }
        if (!keys[controls[4]])
            player.f28 = 0;
        if (!player.f4e && keys[controls[4]] && !player.f20 && !player.f28) {
            player.f10 = itofix(-5);
            player.f28 = 1;
            player.f20 = 1;
            player.f2c = 2;
        }
        if (!keys[controls[4]] && player.f10 < 0)
            player.f10 >>= 1;
        player.f34 = map_collision_at(DAT_00414130, fixtoi(player.x) - 8, fixtoi(player.y) + 5);
        player.f38 = map_collision_at(DAT_00414130, fixtoi(player.x) + 7, fixtoi(player.y) + 5);
        if (player.f34 == 0 && player.f38 == 0) {
            player.f20 = 1;
            if (!player.f4d)
                player.f2c = 2;
        }
        if (player.f20) {
            if (player.f4c & 1)
                player.f10 += DAT_004404c0;
            else
                player.f10 += DAT_00414024;
        } else if (player.f34 > 6 || player.f38 > 6) {
            while (!map_collision_at(DAT_00414130, fixtoi(player.x) - 8, fixtoi(player.y)))
                player.y += DAT_00440044;
            while (!map_collision_at(DAT_00414130, fixtoi(player.x) + 7, fixtoi(player.y)))
                player.y += DAT_00440044;
        }
        if (player.f20 && abs(player.f10) < itofix(3))
            player.f2c = 1;
        player.y += MIN(player.f10, itofix(7));
        player.f34 = map_collision_at(DAT_00414130, fixtoi(player.x) - 8, fixtoi(player.y) - 16);
        player.f38 = map_collision_at(DAT_00414130, fixtoi(player.x) + 7, fixtoi(player.y) - 16);
        if (player.f34 > 3 || player.f38 > 3) {
            player.y = itofix(py);
            player.f10 = 0;
            player.f20 = 1;
        }
        player.f34 = map_collision_at(DAT_00414130, fixtoi(player.x) - 8, fixtoi(player.y) - 2);
        player.f38 = map_collision_at(DAT_00414130, fixtoi(player.x) + 7, fixtoi(player.y) - 2);
        if (player.f34 == 0 && player.f38 == 0)
            player.f24 = 0;
        if (player.f10 > 0) {
            if ((player.f34 || player.f38) && !player.f24) {
                if (map_t4_at(DAT_00414130, fixtoi(player.x), fixtoi(player.y - 8)) == 7 ||
                    map_t4_at(DAT_00414130, fixtoi(player.x), fixtoi(player.y + 7)) == 7)
                    FUN_004036c0();
                player.f20 = 0;
                player.f10 = 0;
                while (map_collision_at(DAT_00414130, fixtoi(player.x) - 8, fixtoi(player.y) - 2))
                    player.y -= DAT_00440044;
                while (map_collision_at(DAT_00414130, fixtoi(player.x) + 7, fixtoi(player.y) - 2))
                    player.y -= DAT_00440044;
                if (DAT_00414130->f79 & 8) {
                    for (i = 0; i < 20; i++)
                        spawn_particle(fixtoi(player.x), fixtoi(player.y) - 2, 175, -1);
                }
            }
        } else {
            if (player.f34 > 3 || player.f38 > 3) {
                player.f20 = 0;
                player.f10 = 0;
                while (map_collision_at(DAT_00414130, fixtoi(player.x) - 8, fixtoi(player.y) - 2) > 3)
                    player.y -= DAT_00440044;
                while (map_collision_at(DAT_00414130, fixtoi(player.x) + 7, fixtoi(player.y) - 2) > 3)
                    player.y -= DAT_00440044;
            }
            player.f34 = map_collision_at(DAT_00414130, fixtoi(player.x) - 8, fixtoi(player.y) - 2);
            player.f38 = map_collision_at(DAT_00414130, fixtoi(player.x) + 7, fixtoi(player.y) - 2);
            if ((player.f34 < 4 && player.f34) || (player.f38 && player.f38 < 4))
                player.f24 = 1;
        }
        DAT_00414cc0[player.f54] = fixtoi(player.x) - 8;
        DAT_004154c0[player.f54] = fixtoi(player.y) - 17;
        if (++player.f54 == 512)
            player.f54 = 0;
        if (player.f14)
            player.f14--;
        if (player.f4d)
            player.f4d--;
        if (player.f4e)
            player.f4e--;
        take_item(fixtoi(player.x), fixtoi(player.y) - 14);
        take_item(fixtoi(player.x), fixtoi(player.y) - 6);
        if (map_t4_at(DAT_00414130, fixtoi(player.x), fixtoi(player.y) - 4) == 1)
            done = 1;
        if (map_t4_at(DAT_00414130, fixtoi(player.x), fixtoi(player.y) - 4) == 8)
            DAT_00414130->f79 |= 2;
        if (map_t4_at(DAT_00414130, fixtoi(player.x), fixtoi(player.y) - 4) == 9)
            DAT_00414130->f79 &= ~2;
        if (map_t4_at(DAT_00414130, fixtoi(player.x), fixtoi(player.y) - 11) == 4 && !DAT_00413b30) {
            player.f44 = 0;
            play_sample(misc_datafile[34].dat, DAT_00442088, 128, 1000, 0);
        }
        if (time_left <= -1) {
            if (!DAT_00442080)
                player.f44 = 0;
            else if (rand() % 30 < 1 && DAT_0043f5a0 < 50)
                drop_item();
        }
        if (player.f16)
            spawn_particle(fixtoi(player.x), fixtoi(player.y) - 8, rand() % 256, -1);
        // STRING: HAPPY 0x00412688
        if (keys[KEY_ESC] && yes_no_box("Quit?"))
            done = 3;
        while (!timer_ticks)
            ;
        if (!player.f44) {
            kill_player();
            done = 2;
        }
        if (keys[KEY_PAUSE])
            pause_game();
        if (keys[KEY_F12])
            // STRING: HAPPY 0x0041267c
            save_screenshot(buffer, "happy.pcx");
    } while (!done);
    return done;
}

/* [hypothesis] The bonus room [verified]: the player (sprite 64 + frame)
 * walks in from the left, sprites 67 and 64 alternate five times, then the
 * objects that follow him (f1b == 2, bit 0 of f19; at most 5 [hypothesis:
 * size of the list]) walk in behind him; "Thanks for taking me/us with
 * you." and "Here's a special treat!"; the player falls (sprite 65
 * upside down), sprite 71 flashes 30 times (draw_character, colours 145
 * to 174), then the bonus level DAT_004450a4 is started (start_level(...,
 * 0, 1)) with DAT_00442080 = 1 and played (0x00404C50), whose result it
 * returns [verified: play_game reads eax after the call]. Only caller
 * 0x00409640.
 * pos is one variable for the x of the first walk and the y of the fall,
 * as in the original: with separate variables, VC6 reloads x and y in the
 * other order after the inner loop of the fall (notes/matching.md §3a). */
// FUNCTION: HAPPY 0x00405db0
int bonus_room(void)
{
    int list[5];
    int c;
    int n;
    int count;
    int x;
    int frame;
    int i;
    int j;
    int pos;

    play_midi(NULL, 0);
    n = -1;
    for (i = 0; i < 100; i++) {
        if (DAT_00414140[i].f1a && (DAT_00414140[i].f19 & 1) && DAT_00414140[i].f1b == 2)
            list[++n] = i;
    }
    clear_bitmap(screen);
    set_palette(DAT_00441c80);
    pos = -16;
    player.f2c = 0;
    player.f20 = 0;
    while (pos < 144) {
        timer_ticks = 0;
        pos++;
        clear_bitmap(buffer);
        draw_sprite(buffer, (BITMAP *)DAT_00415cc0[player.f2c + 64].dat, pos, 70);
        FUN_00403a40();
        blit_to_screen(buffer);
        while (!timer_ticks)
            ;
    }
    for (i = 0; i < 5; i++) {
        timer_ticks = 0;
        clear_bitmap(buffer);
        draw_sprite(buffer, (BITMAP *)DAT_00415cc0[67].dat, 144, 70);
        blit_to_screen(buffer);
        while (timer_ticks < 16)
            ;
        timer_ticks = 0;
        clear_bitmap(buffer);
        draw_sprite(buffer, (BITMAP *)DAT_00415cc0[64].dat, 144, 70);
        blit_to_screen(buffer);
        while (timer_ticks < 16)
            ;
    }
    c = 0;
    frame = 0;
    x = -16;
    count = n + 1;
    while (x < 121) {
        timer_ticks = 0;
        clear_bitmap(buffer);
        if (x < 50)
            draw_sprite(buffer, (BITMAP *)DAT_00415cc0[64].dat, 144, 70);
        else
            draw_sprite_h_flip(buffer, (BITMAP *)DAT_00415cc0[64].dat, 144, 70);
        for (j = 0; j < count; j++) {
            draw_sprite(buffer, (BITMAP *)DAT_00415cc0[DAT_00414140[list[j]].f13 + frame].dat,
                        x - j * 20, 74);
            if (++c > 25)
                c = 0;
            if (c == 0 && ++frame > 1)
                frame = 0;
        }
        blit_to_screen(buffer);
        while (!timer_ticks)
            ;
        x++;
    }
    if (n)
        // STRING: HAPPY 0x004126d4
        textout_centre(buffer, (FONT *)misc_datafile[4].dat, "Thanks for taking us with you.", 160, 150, -1);
    else
        // STRING: HAPPY 0x004126b4
        textout_centre(buffer, (FONT *)misc_datafile[4].dat, "Thanks for taking me with you.", 160, 150, -1);
    // STRING: HAPPY 0x0041269c
    textout_centre(buffer, (FONT *)misc_datafile[4].dat, "Here's a special treat!", 160, 170, -1);
    blit_to_screen(buffer);
    while (timer_ticks < 180)
        ;
    pos = 70;
    do {
        timer_ticks = 0;
        clear_bitmap(buffer);
        for (j = 0; j < count; j++) {
            draw_sprite(buffer, (BITMAP *)DAT_00415cc0[DAT_00414140[list[j]].f13 + frame].dat,
                        x - j * 20, 74);
            if (++c > 25)
                c = 0;
            if (c == 0 && ++frame > 1)
                frame = 0;
        }
        draw_sprite_v_flip(buffer, (BITMAP *)DAT_00415cc0[65].dat, 144, pos);
        blit_to_screen(buffer);
        pos++;
        x -= 4;
        while (!timer_ticks)
            ;
    } while (pos < 156);
    vsync();
    clear_bitmap(buffer);
    for (i = 0; i < 30; i++) {
        timer_ticks = 0;
        clear_bitmap(buffer);
        draw_sprite(buffer, (BITMAP *)DAT_00415cc0[71].dat, 144, 156);
        blit_to_screen(buffer);
        while (timer_ticks < 2)
            ;
        timer_ticks = 0;
        clear_bitmap(buffer);
        draw_character(buffer, (BITMAP *)DAT_00415cc0[71].dat, 144, 156, i + 145);
        blit_to_screen(buffer);
        while (!timer_ticks)
            ;
    }
    if (DAT_00414130 != NULL) {
        map_destroy(DAT_00414130);
        DAT_00414130 = NULL;
    }
    start_level(DAT_004450a4, 0, 1);
    reset_player(0);
    player.f2c = 1;
    player.f08 = 0;
    DAT_00442080 = 1;
    time_left = DAT_00414130->f7a;
    return play_level();
}
