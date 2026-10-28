/* Map and interface display: maped block_05 (0x00401D90-0x004031FF).
 * notes/layout-maped.md §5. Tile mask shapes and type labels, background,
 * objects, export of the map as an image, the editor frame and the
 * background colours dialog. maped only. */
#include <stdio.h>
#include <string.h>
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

/* Exports the whole map as an image [verified]: a bitmap of w x h tiles,
 * the background image MAP.f77 stretched over it (show_background) or
 * black, then per tile the base tile (show_base_tiles), the item
 * (show_items; "!!" for an item past the sprites of the tile set), the
 * cover tile, lit with colour 170 if hilite_cover_tiles (show_cover_tiles),
 * the walk mask (show_masks) and the type (show_types); the objects
 * (show_npcs) and the start sprite 64 at MAP.f03/f04. The options come from
 * maped.ini (defaults 1, except hilite_cover_tiles, show_masks and
 * show_types). Saved, without datafile password, under the name of
 * filename with the extension output_format; resets the scroll position.
 * [verified] mouse_x and mouse_y are read and the values unused. The
 * outer loop has two counters, the inner one derives y from j: the other
 * combinations place the "y = 0" before or after the test differently.
 * Only caller 0x00404C10. */
// FUNCTION: MAPED 0x00402480
void export_map_image(char *filename)
{
    int i;
    int j;
    char msg[512];
    char path[1024];
    PALETTE pal;
    int mx;
    int my;
    BITMAP *bmp;
    int x;
    int y;
    int k;

    mx = mouse_x;
    my = mouse_y;
    bmp = create_bitmap(DAT_00410110->w * 16, DAT_00410110->h * 16);
    if (bmp == NULL) {
        // STRING: MAPED 0x0040f31c
        show_message("failed to create buffer for image output", 1);
        return;
    }
    packfile_password(NULL);
    get_palette(pal);
    // STRING: MAPED 0x0040f30c
    if (get_config_int(NULL, "show_background", 1))
        stretch_blit((BITMAP *)misc_datafile[DAT_00410110->f77].dat, bmp,
                     0, 0, 320, 200, 0, 0, bmp->w, bmp->h);
    else
        clear_bitmap(bmp);
    DAT_0042a424 = 0;
    DAT_0042a420 = 0;
    for (i = 0, x = 0; i < DAT_00410110->w; i++, x += 16) {
        for (j = 0; j < DAT_00410110->h; j++) {
            y = j * 16;
            k = (DAT_0042a424 + j) * DAT_00410110->w + DAT_0042a420 + i;
            // STRING: MAPED 0x0040f2fc
            if (DAT_00410110->tiles[k].t0 && get_config_int(NULL, "show_base_tiles", 1))
                draw_rle_sprite(bmp, (RLE_SPRITE *)map_datafile[DAT_00410110->tiles[k].t0].dat, x, y);
            // STRING: MAPED 0x0040f2f0
            if (DAT_00410110->tiles[k].t3 && get_config_int(NULL, "show_items", 1)) {
                if (DAT_00410110->tiles[k].t3 < ((DAT_00410110->f79 & 8) >> 3) + 19)
                    draw_sprite(bmp, (BITMAP *)DAT_004113d4[DAT_00410110->tiles[k].t3 + 133].dat, x, y);
                else
                    // STRING: MAPED 0x0040f2ec
                    textprintf(bmp, font, x, y, 175, "!!");
            }
            // STRING: MAPED 0x0040f2d8
            if (DAT_00410110->tiles[k].t1 && get_config_int(NULL, "show_cover_tiles", 1)) {
                // STRING: MAPED 0x0040f2c4
                if (get_config_int(NULL, "hilite_cover_tiles", 0))
                    draw_lit_rle_sprite(bmp, (RLE_SPRITE *)map_datafile[DAT_00410110->tiles[k].t1].dat, x, y, 170);
                else
                    draw_rle_sprite(bmp, (RLE_SPRITE *)map_datafile[DAT_00410110->tiles[k].t1].dat, x, y);
            }
            // STRING: MAPED 0x0040f2b8
            if (DAT_00410110->tiles[k].t2 && get_config_int(NULL, "show_masks", 0))
                draw_tile_mask(bmp, x, y, DAT_00410110->tiles[k].t2);
            // STRING: MAPED 0x0040f2ac
            if (DAT_00410110->tiles[k].t4 && get_config_int(NULL, "show_types", 0))
                draw_tile_type(bmp, x, y, DAT_00410110->tiles[k].t4);
        }
    }
    // STRING: MAPED 0x0040f2a0
    if (get_config_int(NULL, "show_npcs", 1))
        draw_objects(bmp);
    draw_sprite(bmp, (BITMAP *)DAT_004113d4[64].dat, (DAT_00410110->f03 - DAT_0042a420) * 16,
                (DAT_00410110->f04 - DAT_0042a424) * 16 - 4);
    replace_extension(path, get_filename(filename), get_config_string(NULL, "output_format", "pcx"), sizeof(path));
    save_bitmap(path, bmp, pal);
    packfile_password("happy");
    destroy_bitmap(bmp);
    // STRING: MAPED 0x0040f294
    sprintf(msg, "%s saved", get_filename(path));
    show_message(msg, 1);
}

/* Draws the editor frame on the back buffer and shows it [verified]: the
 * background, the visible tiles (base tile; item, or "!!" past the sprites
 * of the tile set; cover tile if DAT_0042acc0 is set, lit with colour 170
 * in mode 1; walk mask in mode 2, type in mode 5), the objects, the
 * selection DAT_00422400 under the cursor tile, the start sprite, the
 * cursor and mouse rectangles, the marked tile DAT_0040e050/e054, then the
 * status lines: size and cursor position, the mode name and the cover flag
 * (F2), "RANDOM" (DAT_00410120), "NOSCROLL (F3)", happies and the file,
 * name and time of the map. Modes of DAT_00422404 [verified: the names]:
 * 0 BASE, 1 COVER, 2 WALKMASK, 3 ITEM, 4 NPC, 5 TILETYPE. Only caller
 * 0x00404C10. */
// FUNCTION: MAPED 0x00402900
void draw_editor(void)
{
    int i;
    int j;
    int mx;
    int my;
    char label[40];
    int x;
    int y;
    int k;

    mx = mouse_x;
    my = mouse_y;
    draw_background();
    for (i = 0, x = 0; i < DAT_0042acc8; i++, x += 16) {
        for (j = 0; j < DAT_0042acc4; j++) {
            y = j * 16;
            k = (DAT_0042a424 + j) * DAT_00410110->w + DAT_0042a420 + i;
            if (DAT_00410110->tiles[k].t0)
                draw_rle_sprite(DAT_00422410, (RLE_SPRITE *)map_datafile[DAT_00410110->tiles[k].t0].dat, x, y);
            if (DAT_00410110->tiles[k].t3) {
                if (DAT_00410110->tiles[k].t3 < ((DAT_00410110->f79 & 8) >> 3) + 19)
                    draw_sprite(DAT_00422410, (BITMAP *)DAT_004113d4[DAT_00410110->tiles[k].t3 + 133].dat, x, y);
                else
                    textprintf(DAT_00422410, font, x, y, 175, "!!");
            }
            if (DAT_00410110->tiles[k].t1 && DAT_0042acc0) {
                if (DAT_00422404 == 1)
                    draw_lit_rle_sprite(DAT_00422410, (RLE_SPRITE *)map_datafile[DAT_00410110->tiles[k].t1].dat, x, y, 170);
                else
                    draw_rle_sprite(DAT_00422410, (RLE_SPRITE *)map_datafile[DAT_00410110->tiles[k].t1].dat, x, y);
            }
            if (DAT_00422404 == 2 && DAT_00410110->tiles[k].t2)
                draw_tile_mask(DAT_00422410, x, y, DAT_00410110->tiles[k].t2);
            if (DAT_00422404 == 5 && DAT_00410110->tiles[k].t4)
                draw_tile_type(DAT_00422410, x, y, DAT_00410110->tiles[k].t4);
        }
    }
    draw_objects(DAT_00422410);
    if (DAT_00422404 == 2) {
        if (DAT_00422400)
            draw_tile_mask(DAT_00422410, DAT_00422408 * 16, DAT_0042240c * 16, DAT_00422400);
    } else if (DAT_00422404 == 5) {
        if (DAT_00422400)
            draw_tile_type(DAT_00422410, DAT_00422408 * 16, DAT_0042240c * 16, DAT_00422400);
    } else if (DAT_00422404 == 3 && DAT_00422400 < ((DAT_00410110->f79 & 8) >> 3) + 19) {
        if (DAT_00422400)
            draw_sprite(DAT_00422410, (BITMAP *)DAT_004113d4[DAT_00422400 + 133].dat,
                        DAT_00422408 * 16, DAT_0042240c * 16);
    } else if (DAT_00422404 == 4 && DAT_00422400 < 35) {
        draw_sprite(DAT_00422410, (BITMAP *)DAT_004113d4[DAT_00440060[DAT_00422400].f13].dat,
                    mx, DAT_00422400 < 26 ? my & ~15 : (my & ~15) - 4);
    } else if (DAT_00422404 == 1) {
        if (DAT_00422400)
            draw_lit_rle_sprite(DAT_00422410, (RLE_SPRITE *)map_datafile[DAT_00422400].dat,
                                DAT_00422408 * 16, DAT_0042240c * 16, 170);
    } else if (DAT_00422400) {
        draw_rle_sprite(DAT_00422410, (RLE_SPRITE *)map_datafile[DAT_00422400].dat,
                        DAT_00422408 * 16, DAT_0042240c * 16);
    }
    draw_sprite(DAT_00422410, (BITMAP *)DAT_004113d4[64].dat, (DAT_00410110->f03 - DAT_0042a420) * 16,
                (DAT_00410110->f04 - DAT_0042a424) * 16 - 4);
    if (DAT_00422404 != 4)
        rect(DAT_00422410, DAT_00422408 * 16, DAT_0042240c * 16,
             DAT_00422408 * 16 + 15, DAT_0042240c * 16 + 15, 255);
    rect(DAT_00422410, mx - 1, my - 1, mx + 1, my + 1, 255);
    // STRING: MAPED 0x0040f3fc
    textprintf(DAT_00422410, (FONT *)misc_datafile[25].dat, 1, 1, 175, "%d,%d [%d,%d]",
               DAT_00410110->w, DAT_00410110->h,
               DAT_00422408 + DAT_0042a420, DAT_0042240c + DAT_0042a424);
    if (DAT_0040e050 > -1) {
        rect(DAT_00422410, (DAT_0040e050 - DAT_0042a420) * 16 - 1, (DAT_0040e054 - DAT_0042a424) * 16 - 1,
             (DAT_0040e050 - DAT_0042a420) * 16 + 17, (DAT_0040e054 - DAT_0042a424) * 16 + 17, 175);
        rect(DAT_00422410, (DAT_0040e050 - DAT_0042a420) * 16 - 2, (DAT_0040e054 - DAT_0042a424) * 16 - 2,
             (DAT_0040e050 - DAT_0042a420) * 16 + 18, (DAT_0040e054 - DAT_0042a424) * 16 + 18, 23);
    }
    switch (DAT_00422404) {
    case 0:
        // STRING: MAPED 0x0040f3f4
        strcpy(label, "BASE");
        break;
    case 1:
        // STRING: MAPED 0x0040f3ec
        strcpy(label, "COVER");
        break;
    case 2:
        // STRING: MAPED 0x0040f3e0
        strcpy(label, "WALKMASK");
        break;
    case 3:
        // STRING: MAPED 0x0040f3d8
        strcpy(label, "ITEM");
        break;
    case 4:
        // STRING: MAPED 0x0040f3d4
        strcpy(label, "NPC");
        break;
    case 5:
        // STRING: MAPED 0x0040f3c8
        strcpy(label, "TILETYPE");
        break;
    default:
        // STRING: MAPED 0x0040f3c0
        strcpy(label, "ERROR!");
        break;
    }
    if (DAT_0042acc0)
        // STRING: MAPED 0x0040f3b0
        strcat(label, ", COVER ON (F2)");
    else
        // STRING: MAPED 0x0040f39c
        strcat(label, ", COVER OFF (F2)");
    // STRING: MAPED 0x0040f398
    textprintf(DAT_00422410, (FONT *)misc_datafile[25].dat, 1, 8, 175, "%s", label);
    if (DAT_00410120)
        // STRING: MAPED 0x0040f390
        textout(DAT_00422410, (FONT *)misc_datafile[25].dat, "RANDOM", 1, 15, 175);
    if (!DAT_00410110->f79 & 1)
        // STRING: MAPED 0x0040f380
        textprintf(DAT_00422410, (FONT *)misc_datafile[25].dat, 1, SCREEN_H - 20, 175, "NOSCROLL (F3)");
    // STRING: MAPED 0x0040f370
    textprintf(DAT_00422410, (FONT *)misc_datafile[25].dat, 1, SCREEN_H - 14, 175, "Happies: %d/%d",
               DAT_00410110->f7b, DAT_00410110->f7c);
    textprintf(DAT_00422410, (FONT *)misc_datafile[25].dat, 1, SCREEN_H - 8, 175,
               // STRING: MAPED 0x0040f348
               "Save:%s        Name:%s       Time:%d",
               get_filename(DAT_0042a440), DAT_00410110->name, DAT_00410110->f7a);
    vsync();
    blit(DAT_00422410, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
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
