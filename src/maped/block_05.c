/* Map and interface display: maped block_05 (0x00401D90-0x004031FF).
 * notes/layout-maped.md §8. Tile mask shapes and type labels, background,
 * objects, export of the map as an image, the editor frame and the
 * background colours dialog. maped only. */
#include <stdio.h>
#include <allegro.h>
#include "maped.h"

/* Outline of a triangle [verified: three calls to line()]. Callers
 * draw_tile_mask (0x00401DF0) only. */
// FUNCTION: MAPED 0x00401d90
void draw_triangle(BITMAP *bmp, int x1, int y1, int x2, int y2, int x3, int y3, int color)
{
    line(bmp, x1, y1, x2, y2, color);
    line(bmp, x3, y3, x2, y2, color);
    line(bmp, x1, y1, x3, y3, color);
}

/* Draws, in colour 5, the outline of a walk mask shape over the 16x16 tile
 * at (x, y) [verified]: 1-6 rectangles (top, top-left, top-right halves;
 * whole, left, right halves), 7 and 9 triangles (slopes), 8 and 10 the tile
 * with one corner cut off. [hypothesis] The shapes of TILE.t2, shown as
 * "WALKMASK" by 0x00402900. [verified] Separate ifs, not an else-if chain:
 * the tails VC6 merges between the branches differ (90.53%). */
// FUNCTION: MAPED 0x00401df0
void draw_tile_mask(BITMAP *bmp, int x, int y, int mask)
{
    if (mask == 1) {
        rect(bmp, x, y, x + 15, y + 5, 5);
    }
    if (mask == 2) {
        rect(bmp, x, y, x + 7, y + 5, 5);
    }
    if (mask == 3) {
        rect(bmp, x + 8, y, x + 15, y + 5, 5);
    }
    if (mask == 4) {
        rect(bmp, x, y, x + 15, y + 15, 5);
    }
    if (mask == 5) {
        rect(bmp, x, y, x + 7, y + 15, 5);
    }
    if (mask == 6) {
        rect(bmp, x + 8, y, x + 15, y + 15, 5);
    }
    if (mask == 7) {
        draw_triangle(bmp, x + 4, y + 15, x + 15, y + 15, x + 15, y + 4, 5);
    }
    if (mask == 8) {
        line(bmp, x, y + 3, x + 3, y, 5);
        line(bmp, x, y + 3, x, y + 15, 5);
        line(bmp, x + 15, y + 15, x, y + 15, 5);
        line(bmp, x + 15, y + 15, x + 15, y, 5);
        line(bmp, x + 3, y, x + 15, y, 5);
    }
    if (mask == 9) {
        draw_triangle(bmp, x, y + 4, x, y + 15, x + 11, y + 15, 5);
    }
    if (mask == 10) {
        line(bmp, x + 12, y, x + 15, y + 3, 5);
        line(bmp, x + 15, y + 3, x + 15, y + 15, 5);
        line(bmp, x + 15, y + 15, x, y + 15, 5);
        line(bmp, x, y + 15, x, y, 5);
        line(bmp, x, y, x + 12, y, 5);
    }
}

/* Writes the name of a tile type over the tile at (x, y), in colour 175
 * with font 25 of misc.dat [verified]: 1 EXIT, 2 SAFE, 3 DOOR, 4 DEAD,
 * 5 ICE, 6 SWAP, 7 SWI/TCH (switch), 8 SNOW/ON, 9 SNOW/OFF.
 * [hypothesis] The values of TILE.t4 (happy map_t4_at: 1 = exit). */
// FUNCTION: MAPED 0x00402030
void draw_tile_type(BITMAP *bmp, int x, int y, int type)
{
    if (type == 1) {
        // STRING: MAPED 0x0040f28c
        textout(bmp, (FONT *)misc_datafile[25].dat, "EXIT", x, y, 175);
    }
    if (type == 2) {
        // STRING: MAPED 0x0040f284
        textout(bmp, (FONT *)misc_datafile[25].dat, "SAFE", x, y, 175);
    }
    if (type == 3) {
        // STRING: MAPED 0x0040f27c
        textout(bmp, (FONT *)misc_datafile[25].dat, "DOOR", x, y, 175);
    }
    if (type == 4) {
        // STRING: MAPED 0x0040f274
        textout(bmp, (FONT *)misc_datafile[25].dat, "DEAD", x, y, 175);
    }
    if (type == 5) {
        // STRING: MAPED 0x0040f270
        textout(bmp, (FONT *)misc_datafile[25].dat, "ICE", x + 2, y + 2, 175);
    }
    if (type == 6) {
        // STRING: MAPED 0x0040f268
        textout(bmp, (FONT *)misc_datafile[25].dat, "SWAP", x, y, 175);
    }
    if (type == 7) {
        // STRING: MAPED 0x0040f264
        textout(bmp, (FONT *)misc_datafile[25].dat, "SWI", x + 2, y + 2, 175);
        // STRING: MAPED 0x0040f260
        textout(bmp, (FONT *)misc_datafile[25].dat, "TCH", x + 2, y + 9, 175);
    }
    if (type == 8) {
        // STRING: MAPED 0x0040f258
        textout(bmp, (FONT *)misc_datafile[25].dat, "SNOW", x + 1, y + 2, 175);
        // STRING: MAPED 0x0040f254
        textout(bmp, (FONT *)misc_datafile[25].dat, "ON", x + 2, y + 9, 175);
    }
    if (type == 9) {
        textout(bmp, (FONT *)misc_datafile[25].dat, "SNOW", x + 1, y + 2, 175);
        // STRING: MAPED 0x0040f250
        textout(bmp, (FONT *)misc_datafile[25].dat, "OFF", x + 2, y + 9, 175);
    }
}

/* Background of the back buffer [verified]: image MAP.f77 of misc.dat
 * stretched to the screen if f77 is 0-2, otherwise 16 horizontal bands
 * in colours 176-191 (the gradient of FUN_00401400). Only caller
 * 0x00402900. */
// FUNCTION: MAPED 0x00402280
void draw_background(void)
{
    int h;
    int i;
    int y;

    h = SCREEN_H >> 4;
    if (DAT_00410110->f77 <= 2) {
        stretch_blit((BITMAP *)misc_datafile[DAT_00410110->f77].dat, DAT_00422410,
                     0, 0, 320, 200, 0, 0, SCREEN_W, SCREEN_H);
    } else {
        clear_to_color(DAT_00422410, 191);
        for (i = 0, y = 0; i < 16; i++, y += h)
            rectfill(DAT_00422410, 0, y, SCREEN_W, y + h - 1, 176 + i);
    }
}

/* Help text n [verified: help_texts + n * 257]. Only caller draw_objects
 * (0x00402370). */
// FUNCTION: MAPED 0x00402350
char *get_help_text(int n)
{
    return help_texts[n];
}

/* Draws the objects of the map on bmp, offset by the scroll position
 * [verified]: sprite f13 of sprites.dat, lit with colour 60 when f14 is
 * set; for an object with bit 1 of f19 and f14 set, its help text f1b at
 * x = 2, 10 pixels above it. Callers 0x00402480 and 0x00402900. */
// FUNCTION: MAPED 0x00402370
void draw_objects(BITMAP *bmp)
{
    int i;

    for (i = 0; i < 100; i++) {
        if (DAT_00414140[i].f1a) {
            if (DAT_00414140[i].f14)
                draw_lit_sprite(bmp, (BITMAP *)DAT_004113d4[DAT_00414140[i].f13].dat,
                                DAT_00414140[i].x - DAT_0042a420 * 16,
                                DAT_00414140[i].y - DAT_0042a424 * 16, 60);
            else
                draw_sprite(bmp, (BITMAP *)DAT_004113d4[DAT_00414140[i].f13].dat,
                            DAT_00414140[i].x - DAT_0042a420 * 16,
                            DAT_00414140[i].y - DAT_0042a424 * 16);
            if ((DAT_00414140[i].f19 & 2) && DAT_00414140[i].f14)
                textout(bmp, (FONT *)misc_datafile[25].dat, get_help_text(DAT_00414140[i].f1b),
                        2, DAT_00414140[i].y - DAT_0042a424 * 16 - 10, 175);
        }
    }
}

/* Empty [verified: a single ret]. Only caller 0x00404C10. */
// FUNCTION: MAPED 0x00403140
void FUN_00403140(void)
{
}

/* Background colours [verified]: draws the 16 colours of the gradient at
 * the right of the screen (191 at the top), loads the sliders from
 * MAP.f7d-f7f and f81-f83 (times 4) and runs colors_dialog. Only caller
 * 0x00404C10. */
// FUNCTION: MAPED 0x00403150
void edit_colors(void)
{
    int c;
    int y;

    for (y = 0, c = 191; c > 175; c--, y += 13)
        rectfill(screen, 300, y, 320, y + 12, c);
    slider_values[0] = DAT_00410110->f7d * 4;
    slider_values[1] = DAT_00410110->f7e * 4;
    slider_values[2] = DAT_00410110->f7f * 4;
    slider_values[3] = DAT_00410110->f81 * 4;
    slider_values[4] = DAT_00410110->f82 * 4;
    slider_values[5] = DAT_00410110->f83 * 4;
    do_dialog(colors_dialog, -1);
}
