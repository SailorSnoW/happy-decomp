/* Level and HUD rendering, palette: happy block_03 (0x004021A0-0x0040302F).
 * notes/layout-happy.md §8. Particles, flakes, object sprites, the level
 * frame (0x00402620, not decompiled yet: one of the largest functions) and
 * the level palette. happy only. */
#include <stdlib.h>
#include <allegro.h>
#include "happy.h"

/* Moves and draws the 256 particles on the back buffer, offset by the
 * camera (ox, oy) [verified]: velocity, vertical acceleration, lifetime.
 * Callers 0x00402620, 0x00408200, 0x00408B90. */
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
 * 0x00402620 (flakes 0-255, then 256-511). */
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
 * vertically if DAT_00416d00[i].f10 is set. Only caller 0x00402620. */
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
