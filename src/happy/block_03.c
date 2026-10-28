/* Level and HUD rendering, palette: happy block_03 (0x004021A0-0x0040302F).
 * notes/layout-happy.md §5. Particles, flakes, object sprites, the level
 * frame (draw_level) and the level palette. happy only. */
#include <stdlib.h>
#include <allegro.h>
#include "happy.h"

/* Moves and draws the 256 particles on the back buffer, offset by the
 * camera (ox, oy) [verified]: velocity, vertical acceleration, lifetime.
 * Callers draw_level (0x00402620), 0x00408200, 0x00408B90. */
// FUNCTION: HAPPY 0x004021a0
void draw_particles(int ox, int oy)
{
    int i;

    for (i = 0; i < 256; i++) {
        if (particles[i].active) {
            putpixel(buffer, fixtoi(particles[i].x) - ox, fixtoi(particles[i].y) - oy,
                     particles[i].color);
            particles[i].x += particles[i].dx;
            particles[i].y += particles[i].dy;
            particles[i].dy += particles[i].ddy;
            if (--particles[i].life == 0)
                particles[i].active = 0;
        }
    }
}

/* Flakes from..to-1 [verified]: drawn in colour 175, falling, wrapping
 * around the 320-pixel width, pushed by wind; a flake below the screen is
 * restarted at the top. When bit 2 of MAP.f79 is set, wind changes at
 * random within [-4, 4]. [hypothesis] Snow. oy is not used. Only caller
 * draw_level (0x00402620: flakes 0-255, then 256-511). */
// FUNCTION: HAPPY 0x00402260
void draw_snow(int ox, int oy, int from, int to)
{
    int i;
    int x;

    for (i = from; i < to; i++) {
        if (snow[i].active) {
            x = fixtoi(snow[i].x) - ox % 320;
            if (x < 0)
                x += 320;
            putpixel(buffer, x, fixtoi(snow[i].y), 175);
            snow[i].x += snow[i].dx;
            snow[i].y += snow[i].dy;
            snow[i].dx = itofix(rand() % 10 - 5) / 10 + wind;
            if (fixtoi(snow[i].y) > 200)
                snow[i].active = 0;
            if (fixtoi(snow[i].x) > 320)
                snow[i].x -= itofix(320);
            if (fixtoi(snow[i].x) < 0)
                snow[i].x += itofix(320);
        } else {
            snow[i].x = itofix(rand() % 320);
            snow[i].y = 0;
            snow[i].dx = 0;
            snow[i].dy = itofix(rand() % 8 + 3) / 10;
            snow[i].active = 1;
        }
    }
    /* [verified] the macros evaluate rand() up to four times */
    if (DAT_00414130->f79 & 4)
        wind = MAX(MIN(wind + itofix(rand() % 11 - 5) / 10, itofix(4)), itofix(-4));
}

/* Draws the sprites of the active objects, offset by the camera [verified]:
 * sprite f13 + f14 of DAT_00415cc0, flipped horizontally if f10 is set,
 * vertically if DAT_00416d00[i].f10 is set. Only caller draw_level (0x00402620). */
// FUNCTION: HAPPY 0x004024e0
void draw_objects(int ox, int oy)
{
    int i;

    for (i = 0; i < 100; i++) {
        if (DAT_00414140[i].f1a) {
            if (DAT_00416d00[i].f10 == 0) {
                if (DAT_00414140[i].f10)
                    draw_sprite_h_flip(buffer,
                        (BITMAP *)DAT_00415cc0[DAT_00414140[i].f13 + DAT_00414140[i].f14].dat,
                        DAT_00414140[i].x - ox, DAT_00414140[i].y - oy);
                else
                    draw_sprite(buffer,
                        (BITMAP *)DAT_00415cc0[DAT_00414140[i].f13 + DAT_00414140[i].f14].dat,
                        DAT_00414140[i].x - ox, DAT_00414140[i].y - oy);
            } else {
                if (DAT_00414140[i].f10)
                    draw_sprite_vh_flip(buffer,
                        (BITMAP *)DAT_00415cc0[DAT_00414140[i].f13 + DAT_00414140[i].f14].dat,
                        DAT_00414140[i].x - ox, DAT_00414140[i].y - oy);
                else
                    draw_sprite_v_flip(buffer,
                        (BITMAP *)DAT_00415cc0[DAT_00414140[i].f13 + DAT_00414140[i].f14].dat,
                        DAT_00414140[i].x - ox, DAT_00414140[i].y - oy);
            }
        }
    }
}

/* [hypothesis] Draws one frame of the level on the back buffer
 * [verified: tiles, particles, objects, player, HUD]. frame_count + 1.
 * Camera (ox, oy) on the player when the level scrolls (f79 bit 0),
 * clamped to the map, otherwise 0; tile (tx, ty) and offset (sx, sy) in
 * the tile. Background image DAT_004274e4 scrolled by (ox % 1280) / 4;
 * the flakes when f79 bit 1; the back tiles (t0: RLE sprite of
 * map_datafile up to 130, a line from 131 on) and their items (t3, sprite
 * t3 + 133 of DAT_00415cc0 below 20); particles and objects; the player
 * (sprite 64 + player.f2c, 70 + player.f2c while player.f4d counts and
 * the slide key is up, flipped when player.f08); the front tiles (t1).
 * When hud: score (player.f48), player.f64, energy, lives, spheres, bonus
 * items left, time (MAX(time, 0)) and Happylanders left, unless
 * DAT_00442080 is set, then the scrolling message. show: copies the
 * buffer to the screen (blit_to_screen). 12 callers.
 * Shapes that decide the code (notes/matching.md §3a): the tile
 * coordinates are written i * 16 - sx and j * 16 - sy in the calls (VC6
 * makes the induction variables, tests them instead of i and j, and gives
 * the original stack slots); the front tile loop has its index in k like
 * the back one; text_length goes into a local before textout (the stack
 * cleanups are grouped as in the original). */
// FUNCTION: HAPPY 0x00402620
void draw_level(int show, int hud, int time)
{
    int j;
    int oy;
    int ox;
    int i;
    int sy;
    int sx;
    int ty;
    int tx;
    int k;
    int len;
    char buf[20];

    frame_count++;
    if (DAT_00414130->f79 & 1) {
        ox = MIN(MAX(fixtoi(player.x) - 159, 0), (DAT_00414130->w - 20) * 16);
        tx = ox / 16;
        sx = ox - tx * 16;
        oy = MIN(MAX(fixtoi(player.y) - 100, 0), DAT_00414130->h * 16 - 216);
        ty = oy / 16;
        sy = oy - ty * 16;
    } else {
        ox = 0;
        tx = 0;
        sx = 0;
        oy = 0;
        ty = 0;
        sy = 0;
    }
    blit(DAT_004274e4, buffer, (ox % 1280) >> 2, 0, 0, 0, 320, 200);
    if (DAT_00414130->f79 & 2)
        draw_snow(ox, oy, 0, 256);
    if (DAT_00414130->f79 & 2)
        draw_snow(ox, oy, 256, 512);
    for (i = 0; i < 21; i++) {
        for (j = 0; j < 14; j++) {
            k = (ty + j) * DAT_00414130->w + i + tx;
            if (DAT_00414130->tiles[k].t0 &&
                DAT_00414130->tiles[k].t0 < 131)
                draw_rle_sprite(buffer, (RLE_SPRITE *)map_datafile[DAT_00414130->tiles[k].t0].dat, i * 16 - sx, j * 16 - sy);
            if (DAT_00414130->tiles[k].t0 >= 131)
                line(buffer, i * 16 - sx, j * 16 - sy, i * 16 - sx + 15, j * 16 - sy + 15, 200);
            if (DAT_00414130->tiles[k].t3 &&
                DAT_00414130->tiles[k].t3 < 20)
                draw_sprite(buffer, (BITMAP *)DAT_00415cc0[DAT_00414130->tiles[k].t3 + 133].dat, i * 16 - sx, j * 16 - sy);
        }
    }
    draw_particles(ox, oy);
    draw_objects(ox, oy);
    if (player.f3c) {
        if (player.f4d && !player.f20 && !keys[controls[6]]) {
            if (player.f08)
                draw_sprite_h_flip(buffer, (BITMAP *)DAT_00415cc0[player.f2c + 70].dat, fixtoi(player.x) - ox - 8, fixtoi(player.y) - oy - 21);
            else
                draw_sprite(buffer, (BITMAP *)DAT_00415cc0[player.f2c + 70].dat, fixtoi(player.x) - ox - 8, fixtoi(player.y) - oy - 21);
        } else {
            if (player.f08)
                draw_sprite_h_flip(buffer, (BITMAP *)DAT_00415cc0[player.f2c + 64].dat, fixtoi(player.x) - ox - 8, fixtoi(player.y) - oy - 21);
            else
                draw_sprite(buffer, (BITMAP *)DAT_00415cc0[player.f2c + 64].dat, fixtoi(player.x) - ox - 8, fixtoi(player.y) - oy - 21);
        }
    }
    for (i = 0; i < 21; i++) {
        for (j = 0; j < 14; j++) {
            k = (ty + j) * DAT_00414130->w + i + tx;
            if (DAT_00414130->tiles[k].t1 &&
                DAT_00414130->tiles[k].t1 < 131)
                draw_rle_sprite(buffer, (RLE_SPRITE *)map_datafile[DAT_00414130->tiles[k].t1].dat, i * 16 - sx, j * 16 - sy);
        }
    }
    if (hud) {
        // STRING: HAPPY 0x0041251c
        textprintf(buffer, (FONT *)misc_datafile[4].dat, 2, 2, -1, "%d", player.f48);
        textprintf_centre(buffer, (FONT *)misc_datafile[4].dat, 160, 2, -1, "%d", player.f64);
        if (player.f44 > 1)
            draw_sprite(buffer, (BITMAP *)DAT_00415cc0[147].dat, 2, 19);
        if (player.f44 > 2)
            draw_sprite(buffer, (BITMAP *)DAT_00415cc0[147].dat, 16, 19);
        for (i = 0; i < player.f40; i++)
            stretch_sprite(buffer, (BITMAP *)DAT_00415cc0[64].dat, 2 + i * 10, 34, 8, 10);
        for (i = 0; i < player.f60; i++)
            stretch_sprite(buffer, (BITMAP *)DAT_00415cc0[152].dat, 2 + i * 8, 44, 8, 8);
        if (DAT_00440048) {
            for (i = 0; i < DAT_00440048 - player.f50; i++)
                stretch_sprite(buffer, (BITMAP *)DAT_00415cc0[151].dat, 2 + i * 9, 190, 8, 8);
        }
        if (!DAT_00442080) {
            itoa(MAX(time, 0), buf, 10);
            len = text_length((FONT *)misc_datafile[4].dat, buf);
            textout(buffer, (FONT *)misc_datafile[4].dat, buf, 318 - len, 2, -1);
            itoa(MAX(DAT_00414130->f7b - player.f15, 0), buf, 10);
            len = text_length((FONT *)misc_datafile[4].dat, buf);
            textout(buffer, (FONT *)misc_datafile[4].dat, buf, 318 - len, 22, -1);
        }
        if (scroll_active)
            draw_scroll_message(buffer);
    }
    if (show)
        blit_to_screen(buffer);
}

/* Palette of the level [verified]: the palette of the tiles datafile, with
 * colours 176-191 replaced by a gradient from MAP.f7d-f7f to f81-f83
 * (16 steps, exclusive of the start); then the 640x200 background
 * (DAT_004274e4) is filled with colour 191 and gets image MAP.f77 (0-2,
 * reset to 0 above) of misc_datafile twice, side by side. 5 callers. */
// FUNCTION: HAPPY 0x00402ea0
void init_level_palette(void)
{
    int i;
    float r;
    float g;
    float b;
    float dr;
    float dg;
    float db;

    set_palette((RGB *)map_datafile[0].dat);
    get_palette(DAT_00441c80);
    dr = (DAT_00414130->f81 - DAT_00414130->f7d) * 0.0625;
    dg = (DAT_00414130->f82 - DAT_00414130->f7e) * 0.0625;
    db = (DAT_00414130->f83 - DAT_00414130->f7f) * 0.0625;
    r = DAT_00414130->f7d;
    g = DAT_00414130->f7e;
    b = DAT_00414130->f7f;
    for (i = 0; i < 16; i++) {
        r += dr;
        g += dg;
        b += db;
        DAT_00441c80[176 + i].r = r;
        DAT_00441c80[176 + i].g = g;
        DAT_00441c80[176 + i].b = b;
    }
    clear_to_color(DAT_004274e4, 191);
    if (DAT_00414130->f77 > 2)
        DAT_00414130->f77 = 0;
    blit((BITMAP *)misc_datafile[DAT_00414130->f77].dat, DAT_004274e4, 0, 0, 0, 0, 320, 200);
    blit((BITMAP *)misc_datafile[DAT_00414130->f77].dat, DAT_004274e4, 0, 0, 320, 0, 320, 200);
    init_color_tables();
    set_palette(DAT_00441c80);
}
