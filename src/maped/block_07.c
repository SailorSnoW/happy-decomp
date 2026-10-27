/* Editing tools and the editor loop: maped block_07
 * (0x00403EB0-0x00405C9F). notes/layout-maped.md §8. Shifting and
 * resizing the map, objects, help texts, walk masks, the tile picker, the
 * editor loop, main and WinMain. maped only. */
#include <direct.h>
#include <stdlib.h>
#include <string.h>
#include <allegro.h>
#include "maped.h"

/* Shifts the map contents by (dx, dy) tiles [verified]: each tile takes
 * the tile (x + dx, y + dy) of a copy, clamped to the map, and the objects
 * move by (-dx, -dy) tiles. [verified] i + dx written in the clamp: VC6
 * makes it a second induction variable itself (a separate counter gives
 * other registers). Only caller 0x00404C10. */
// FUNCTION: MAPED 0x00403eb0
void shift_map(int dx, int dy)
{
    MAP *copy;
    int i;
    int j;
    int x;
    int y;

    copy = map_copy(DAT_00410110);
    if (copy == NULL)
        return;
    for (i = 0; i < DAT_00410110->w; i++) {
        for (j = 0; j < DAT_00410110->h; j++) {
            x = MAX(MIN(i + dx, DAT_00410110->w - 1), 0);
            y = MAX(MIN(j + dy, DAT_00410110->h - 1), 0);
            DAT_00410110->tiles[j * DAT_00410110->w + i] = copy->tiles[y * DAT_00410110->w + x];
        }
    }
    map_destroy(copy);
    for (i = 0; i < 100; i++) {
        if (DAT_00414140[i].f1a) {
            DAT_00414140[i].x -= dx * 16;
            DAT_00414140[i].y -= dy * 16;
        }
    }
}

/* Resizes the map by (x, y) tiles [verified]: the new size is at least
 * the view and at most 255; the header is copied, each tile from the
 * nearest tile of the old map; then the scroll and cursor positions are
 * clamped. [verified] The parameters hold the new size, then the source
 * tile: with separate variables VC6 gives the values other registers and
 * stack slots (50.99%). Only caller 0x00404C10. */
// FUNCTION: MAPED 0x00404000
void resize_map(int x, int y)
{
    MAP *m;
    int i;
    int j;

    x = MIN(MAX(DAT_00410110->w + x, DAT_0042acc8), 255);
    y = MIN(MAX(DAT_00410110->h + y, DAT_0042acc4), 255);
    m = map_create(x, y);
    if (m == NULL)
        return;
    memcpy(m, DAT_00410110, MAP_HEADER_SIZE);
    m->w = x;
    m->h = y;
    for (i = 0; i < m->w; i++) {
        for (j = 0; j < m->h; j++) {
            x = MAX(MIN(DAT_00410110->w - 1, MIN(i, m->w - 1)), 0);
            y = MAX(MIN(DAT_00410110->h - 1, MIN(j, m->h - 1)), 0);
            m->tiles[j * m->w + i] = DAT_00410110->tiles[y * DAT_00410110->w + x];
        }
    }
    map_destroy(DAT_00410110);
    DAT_00410110 = m;
    DAT_0042a420 = MIN(DAT_00410110->w - DAT_0042acc8, MAX(DAT_0042a420, 0));
    DAT_0042a424 = MIN(DAT_00410110->h - DAT_0042acc4, MAX(DAT_0042a424, 0));
    DAT_00422408 = MIN(DAT_0042acc8 - 1, MAX(DAT_00422408, 0));
    DAT_0042240c = MIN(DAT_0042acc4 - 1, MAX(DAT_0042240c, 0));
}

/* Waits until the four arrow keys are released [verified]. Only caller
 * 0x00404C10. */
// FUNCTION: MAPED 0x00404280
void wait_arrows_released(void)
{
    while (key[KEY_UP] || key[KEY_RIGHT] || key[KEY_DOWN] || key[KEY_LEFT])
        ;
}

/* Random number from lo to hi [verified]. Only caller 0x00404C10. */
// FUNCTION: MAPED 0x004042b0
int rand_range(int lo, int hi)
{
    return rand() % (hi - lo + 1) + lo;
}

/* Puts an object of template type at (x, y) in the first free slot (the
 * last one if none is free) [verified]: x as given, y on the tile grid,
 * 4 pixels higher for the templates from 26 on; counts it in MAP.f7c if
 * bit 0 of f19 is set (a Happylander). Only caller 0x00404C10. */
// FUNCTION: MAPED 0x004042d0
void add_object(int x, int y, int type)
{
    int i;

    for (i = 0; DAT_00414140[i].f1a && i < 99; i++)
        ;
    DAT_00414140[i] = DAT_00440060[type];
    DAT_00414140[i].f18 = type;
    DAT_00414140[i].x = x;
    if (type < 26)
        DAT_00414140[i].y = y & ~15;
    else
        DAT_00414140[i].y = (y & ~15) - 4;
    DAT_00414140[i].f1a = 1;
    if (DAT_00414140[i].f19 & 1)
        DAT_00410110->f7c++;
}

/* Lists the help texts 1-15 as a) to o) and reads a key [verified]: with
 * ALT, the text is edited in text_dialog and the list shown again;
 * without, returns the number of the text (0 for another key). Only caller
 * 0x00404C10. */
// FUNCTION: MAPED 0x00404380
char select_help_text(void)
{
    int i;
    int k;
    int n;

    for (;;) {
        clear_bitmap(screen);
        for (i = 1; i <= 15; i++)
            // STRING: MAPED 0x0040f538
            textprintf(screen, (FONT *)misc_datafile[25].dat, 5, i * 8 + 20, 175, "%c) %s",
                       i + 'a' - 1, help_texts[i]);
        // STRING: MAPED 0x0040f518
        textout(screen, font, "(a-o)     :  Select textstring.", 30, 170, 175);
        // STRING: MAPED 0x0040f4f8
        textout(screen, font, "(a-o)+ALT :  Edit textstring.", 30, 180, 175);
        clear_keybuf();
        k = readkey() >> 8;
        n = 0;
        for (i = 1; i <= 15; i++) {
            if (k == i)
                n = i;
        }
        if (!key[KEY_ALT])
            break;
        if (n >= 0 && n < 16) {
            strcpy(DAT_004107e0, help_texts[n]);
            do_dialog(text_dialog, -1);
            strcpy(help_texts[n], DAT_004107e0);
        }
    }
    clear_keybuf();
    text_mode(-1);
    return n;
}

/* Sets the walk mask (TILE.t2) of tile (x, y) from the numbers of its
 * base and cover tiles [verified]: tables of tile numbers for the winter
 * tile set (bit 3 of MAP.f79) and the summer one; the larger of the two
 * masks is kept (draw_tile_mask shows the shapes). Only caller
 * 0x00404C10. */
// FUNCTION: MAPED 0x00404500
void auto_mask(int x, int y)
{
    int n;
    int m;
    int c;
    int old;

    m = 0;
    c = 0;
    if (DAT_00410110->f79 & 8) {
        n = DAT_00410110->tiles[DAT_00410110->w * y + x].t0 - 1;
        if (n >= 0 && n <= 6)
            m = 4;
        if (n >= 16 && n <= 21)
            m = 4;
        if (n == 124 || n == 87)
            m = 4;
        if (n >= 40 && n <= 55)
            m = 4;
        if (n >= 58 && n <= 60)
            m = 4;
        if (n >= 93 && n <= 95)
            m = 4;
        if (n == 10 || n == 15 || n == 86)
            m = 6;
        if (n == 9 || n == 22 || n == 88)
            m = 5;
        if (n == 29 || n == 30 || n == 108 || n == 109 || n == 110 || n == 120 || n == 121)
            m = 1;
        if (n >= 61 && n <= 64)
            m = 1;
        if (n == 28)
            m = 3;
        if (n == 31)
            m = 2;
        if (n == 126 || n == 92)
            m = 7;
        if (n == 127 || n == 91)
            m = 8;
        if (n == 128 || n == 90)
            m = 10;
        if (n == 129 || n == 89)
            m = 9;
        old = m;
        n = DAT_00410110->tiles[DAT_00410110->w * y + x].t1 - 1;
        if (n >= 0 && n <= 6)
            m = 4;
        if (n >= 16 && n <= 21)
            m = 4;
        if (n == 124 || n == 87)
            m = 4;
        if (n >= 40 && n <= 55)
            m = 4;
        if (n == 10 || n == 15 || n == 86)
            m = 6;
        if (n == 9 || n == 22 || n == 88)
            m = 5;
        if (n >= 58 && n <= 60)
            m = 4;
        if (n >= 93 && n <= 95)
            m = 4;
        if (n == 29 || n == 30 || n == 109 || n == 120 || n == 121)
            m = 1;
        if (n >= 61 && n <= 64)
            m = 1;
        if (n == 28 || n == 108)
            m = 3;
        if (n == 31 || n == 110)
            m = 2;
        if (n == 126 || n == 92)
            m = 7;
        if (n == 127 || n == 91)
            m = 8;
        if (n == 128 || n == 90)
            m = 10;
        if (n == 129 || n == 89)
            m = 9;
        DAT_00410110->tiles[DAT_00410110->w * y + x].t2 = MAX(m, old);
    } else {
        n = DAT_00410110->tiles[DAT_00410110->w * y + x].t0 - 1;
        if (n >= 0 && n <= 6)
            m = 4;
        if (n >= 16 && n <= 19)
            m = 4;
        if (n == 21 || n == 22 || n == 124)
            m = 4;
        if (n >= 40 && n <= 46)
            m = 4;
        if (n >= 48 && n <= 52)
            m = 4;
        if (n >= 54 && n <= 56)
            m = 4;
        if (n == 10 || n == 15 || n == 47)
            m = 6;
        if (n == 9 || n == 20 || n == 53)
            m = 5;
        if (n == 29 || n == 30)
            m = 1;
        if (n == 28)
            m = 3;
        if (n == 31)
            m = 2;
        if (n == 126)
            m = 7;
        if (n == 127)
            m = 8;
        if (n == 128)
            m = 10;
        if (n == 129)
            m = 9;
        n = DAT_00410110->tiles[DAT_00410110->w * y + x].t1 - 1;
        if (n >= 0 && n <= 6)
            c = 4;
        if (n >= 16 && n <= 19)
            c = 4;
        if (n == 21 || n == 22 || n == 124)
            c = 4;
        if (n >= 40 && n <= 46)
            c = 4;
        if (n >= 48 && n <= 52)
            c = 4;
        if (n >= 54 && n <= 56)
            c = 4;
        if (n == 10 || n == 15 || n == 47)
            c = 6;
        if (n == 9 || n == 20 || n == 53)
            c = 5;
        if (n == 29 || n == 30)
            c = 1;
        if (n == 28)
            c = 3;
        if (n == 31)
            c = 2;
        if (n == 126)
            c = 7;
        if (n == 127)
            c = 8;
        if (n == 128)
            c = 10;
        if (n == 129)
            c = 9;
        DAT_00410110->tiles[DAT_00410110->w * y + x].t2 = MAX(m, c);
    }
}

/* Checks the slopes of the walk masks [verified]: a tile with mask 7, 8,
 * 9 or 10 must have the expected masks beside or below it. The last tile
 * that fails is marked (DAT_0040e050/e054, drawn by draw_editor) and the
 * view moves to it; otherwise "Tile masks are ok!". Only caller
 * 0x00404C10. */
// FUNCTION: MAPED 0x00404940
void check_masks(void)
{
    int x;
    int y;
    int m;
    int a;
    int b;

    DAT_0040e050 = -1;
    DAT_0040e054 = -1;
    for (x = 0; x < DAT_00410110->w; x++) {
        for (y = 0; y < DAT_00410110->h; y++) {
            m = DAT_00410110->tiles[y * DAT_00410110->w + x].t2;
            if (m == 7) {
                a = DAT_00410110->tiles[y * DAT_00410110->w + x + 1].t2;
                b = DAT_00410110->tiles[(y + 1) * DAT_00410110->w + x].t2;
                if ((a == 4 || a == 8) && b == 8)
                    continue;
            } else if (m == 8) {
                a = DAT_00410110->tiles[y * DAT_00410110->w + x + 1].t2;
                b = DAT_00410110->tiles[y * DAT_00410110->w + x - 1].t2;
                if ((a == 4 || a == 10) && (b == 7 || b == 4))
                    continue;
            } else if (m == 9) {
                a = DAT_00410110->tiles[y * DAT_00410110->w + x - 1].t2;
                b = DAT_00410110->tiles[(y + 1) * DAT_00410110->w + x].t2;
                if ((a == 4 || a == 10) && b == 10)
                    continue;
            } else if (m == 10) {
                a = DAT_00410110->tiles[y * DAT_00410110->w + x - 1].t2;
                b = DAT_00410110->tiles[y * DAT_00410110->w + x + 1].t2;
                if ((a == 4 || a == 8) && (b == 9 || b == 4))
                    continue;
            } else {
                continue;
            }
            DAT_0040e050 = x;
            DAT_0040e054 = y;
        }
    }
    if (DAT_0040e050 > -1) {
        DAT_00422408 = DAT_0040e050;
        DAT_0042240c = DAT_0040e054;
        DAT_0042a420 = DAT_0040e050 - 10;
        DAT_0042a424 = DAT_0040e054 - 6;
        DAT_0042a420 = MIN(DAT_00410110->w - DAT_0042acc8, MAX(DAT_0042a420, 0));
        DAT_0042a424 = MIN(DAT_00410110->h - DAT_0042acc4, MAX(DAT_0042a424, 0));
    } else {
        // STRING: MAPED 0x0040f540
        show_message("Tile masks are ok!", 1);
    }
}

/* Tile picker [verified]: shows the tiles of map_datafile 16 per row on
 * the screen, waits for a click and selects the tile under the mouse
 * (DAT_00422400, 1-based) if it is within the count of the current mode.
 * Only caller 0x00404C10. */
// FUNCTION: MAPED 0x00404b00
void pick_tile(void)
{
    int i;
    int y;
    int mx;
    int my;

    clear_bitmap(screen);
    for (i = 0, y = -18; i < DAT_004107a0[DAT_00422404]; i++) {
        if (i % 16 == 0)
            y += 18;
        draw_rle_sprite(screen, (RLE_SPRITE *)map_datafile[i + 1].dat, i % 16 * 18, y);
    }
    show_mouse(screen);
    while (!mouse_b)
        ;
    mx = mouse_x / 18;
    my = mouse_y / 18;
    while (mouse_b)
        ;
    show_mouse(NULL);
    if (mx <= 15) {
        my = my * 16 + mx;
        if (my <= DAT_004107a0[DAT_00422404])
            DAT_00422400 = my + 1;
    }
}

/* The editor loop [verified]. Each frame: draw_editor, the key read with
 * readkey, the cursor tile under the mouse, scrolling at the screen edges,
 * then the editing of the current mode (DAT_00422404) with the mouse:
 * button 1 sets the tile field of the mode to the selection DAT_00422400
 * (mode 0 with RANDOM: a random variant of the same tile family; modes 0
 * and 1 then recompute the walk mask), button 2 picks it, both buttons in
 * mode 0 fill the view; mode 4 places objects (button 1), lights the
 * objects under the mouse, deletes them (button 2) or, with T, gives them
 * a help text. Keys: 1-6 the mode, Z/X the selection, M/INSERT/DEL with
 * the arrows shift, grow or shrink the map, ALT+F the menu, ALT+M the
 * properties, ALT+T the tile picker, ALT+Y all walk masks, ALT+Q the
 * object templates, ALT+N/S/A/L/R new, save, save as, load, revert; F1
 * FUN_00403140, F2 cover tiles, F3 check_masks, F5 the start tile, F7
 * export, F8 screenshot, F9 RANDOM, F11 colours, F12 screen mode, ESC
 * quit. TAB slows the loop down. Only caller main (0x00405AE0). */
// FUNCTION: MAPED 0x00404c10
void editor_loop(void)
{
    int done;
    int mx;
    int my;
    int k;
    int i;
    int j;

    mx = mouse_x;
    my = mouse_y;
    done = 0;
    DAT_00422408 = 0;
    DAT_0042240c = 0;
    DAT_00422400 = 1;
    DAT_00422404 = 0;
    DAT_0042a420 = 0;
    DAT_0042a424 = 0;
    clear_keybuf();
    do {
        mx = mouse_x;
        my = mouse_y;
        if (key[KEY_TAB])
            rest(50);
        draw_editor();
        k = 0;
        if (keypressed())
            k = readkey() >> 8;
        DAT_00422408 = mouse_x / 16;
        DAT_0042240c = mouse_y / 16;
        if (mouse_x > SCREEN_W - 5)
            DAT_0042a420++;
        if (mouse_x < 4)
            DAT_0042a420--;
        if (mouse_y > SCREEN_H - 5)
            DAT_0042a424++;
        if (mouse_y < 4)
            DAT_0042a424--;
        DAT_0042a420 = MIN(DAT_00410110->w - DAT_0042acc8, MAX(DAT_0042a420, 0));
        DAT_0042a424 = MIN(DAT_00410110->h - DAT_0042acc4, MAX(DAT_0042a424, 0));
        DAT_00422408 = MIN(DAT_0042acc8 - 1, MAX(DAT_00422408, 0));
        DAT_0042240c = MIN(DAT_0042acc4 - 1, MAX(DAT_0042240c, 0));
        if (DAT_00422404 == 0) {
            if (mouse_b == 1) {
                if (DAT_00410120) {
                    if (DAT_00410110->f79 & 8) {
                        if (DAT_00422400 >= 1 && DAT_00422400 <= 6)
                            DAT_00410110->tiles[(DAT_0042a424 + DAT_0042240c) * DAT_00410110->w + DAT_0042a420 + DAT_00422408].t0 = rand_range(1, 6);
                        else if (DAT_00422400 >= 17 && DAT_00422400 <= 22)
                            DAT_00410110->tiles[(DAT_0042a424 + DAT_0042240c) * DAT_00410110->w + DAT_0042a420 + DAT_00422408].t0 = rand_range(17, 22);
                        else if (DAT_00422400 >= 24 && DAT_00422400 <= 26)
                            DAT_00410110->tiles[(DAT_0042a424 + DAT_0042240c) * DAT_00410110->w + DAT_0042a420 + DAT_00422408].t0 = rand_range(24, 26);
                        else if (DAT_00422400 >= 41 && DAT_00422400 <= 43)
                            DAT_00410110->tiles[(DAT_0042a424 + DAT_0042240c) * DAT_00410110->w + DAT_0042a420 + DAT_00422408].t0 = rand_range(41, 43);
                        else if (DAT_00422400 >= 50 && DAT_00422400 <= 53)
                            DAT_00410110->tiles[(DAT_0042a424 + DAT_0042240c) * DAT_00410110->w + DAT_0042a420 + DAT_00422408].t0 = rand_range(50, 53);
                        else
                            DAT_00410110->tiles[(DAT_0042a424 + DAT_0042240c) * DAT_00410110->w + DAT_0042a420 + DAT_00422408].t0 = DAT_00422400;
                    } else {
                        if (DAT_00422400 >= 1 && DAT_00422400 <= 6)
                            DAT_00410110->tiles[(DAT_0042a424 + DAT_0042240c) * DAT_00410110->w + DAT_0042a420 + DAT_00422408].t0 = rand_range(1, 6);
                        else if (DAT_00422400 >= 17 && DAT_00422400 <= 20)
                            DAT_00410110->tiles[(DAT_0042a424 + DAT_0042240c) * DAT_00410110->w + DAT_0042a420 + DAT_00422408].t0 = rand_range(17, 20);
                        else if (DAT_00422400 >= 24 && DAT_00422400 <= 27)
                            DAT_00410110->tiles[(DAT_0042a424 + DAT_0042240c) * DAT_00410110->w + DAT_0042a420 + DAT_00422408].t0 = rand_range(24, 27);
                        else if (DAT_00422400 >= 41 && DAT_00422400 <= 47)
                            DAT_00410110->tiles[(DAT_0042a424 + DAT_0042240c) * DAT_00410110->w + DAT_0042a420 + DAT_00422408].t0 = rand_range(41, 47);
                        else if (DAT_00422400 >= 49 && DAT_00422400 <= 53)
                            DAT_00410110->tiles[(DAT_0042a424 + DAT_0042240c) * DAT_00410110->w + DAT_0042a420 + DAT_00422408].t0 = rand_range(49, 53);
                        else if (DAT_00422400 >= 59 && DAT_00422400 <= 65)
                            DAT_00410110->tiles[(DAT_0042a424 + DAT_0042240c) * DAT_00410110->w + DAT_0042a420 + DAT_00422408].t0 = rand_range(59, 65);
                        else if (DAT_00422400 >= 106 && DAT_00422400 <= 109)
                            DAT_00410110->tiles[(DAT_0042a424 + DAT_0042240c) * DAT_00410110->w + DAT_0042a420 + DAT_00422408].t0 = rand_range(106, 109);
                        else
                            DAT_00410110->tiles[(DAT_0042a424 + DAT_0042240c) * DAT_00410110->w + DAT_0042a420 + DAT_00422408].t0 = DAT_00422400;
                    }
                } else {
                    DAT_00410110->tiles[(DAT_0042a424 + DAT_0042240c) * DAT_00410110->w + DAT_0042a420 + DAT_00422408].t0 = DAT_00422400;
                }
                auto_mask(DAT_0042a420 + DAT_00422408, DAT_0042a424 + DAT_0042240c);
            }
            if (mouse_b == 2)
                DAT_00422400 = DAT_00410110->tiles[(DAT_0042a424 + DAT_0042240c) * DAT_00410110->w + DAT_0042a420 + DAT_00422408].t0;
            if (mouse_b == 3) {
                for (i = 0; i < DAT_0042acc8; i++) {
                    for (j = 0; j < DAT_0042acc4; j++)
                        DAT_00410110->tiles[(DAT_0042a424 + j) * DAT_00410110->w + DAT_0042a420 + i].t0 = DAT_00422400;
                }
            }
        }
        if (DAT_00422404 == 1) {
            if (mouse_b & 1) {
                DAT_00410110->tiles[(DAT_0042a424 + DAT_0042240c) * DAT_00410110->w + DAT_0042a420 + DAT_00422408].t1 = DAT_00422400;
                auto_mask(DAT_0042a420 + DAT_00422408, DAT_0042a424 + DAT_0042240c);
            }
            if (mouse_b & 2)
                DAT_00422400 = DAT_00410110->tiles[(DAT_0042a424 + DAT_0042240c) * DAT_00410110->w + DAT_0042a420 + DAT_00422408].t1;
        }
        if (DAT_00422404 == 2) {
            if (mouse_b & 1)
                DAT_00410110->tiles[(DAT_0042a424 + DAT_0042240c) * DAT_00410110->w + DAT_0042a420 + DAT_00422408].t2 = DAT_00422400;
            if (mouse_b & 2)
                DAT_00422400 = DAT_00410110->tiles[(DAT_0042a424 + DAT_0042240c) * DAT_00410110->w + DAT_0042a420 + DAT_00422408].t2;
        }
        if (DAT_00422404 == 5) {
            if (mouse_b & 1)
                DAT_00410110->tiles[(DAT_0042a424 + DAT_0042240c) * DAT_00410110->w + DAT_0042a420 + DAT_00422408].t4 = DAT_00422400;
            if (mouse_b & 2)
                DAT_00422400 = DAT_00410110->tiles[(DAT_0042a424 + DAT_0042240c) * DAT_00410110->w + DAT_0042a420 + DAT_00422408].t4;
        }
        if (DAT_00422404 == 3) {
            if (mouse_b & 1)
                DAT_00410110->tiles[(DAT_0042a424 + DAT_0042240c) * DAT_00410110->w + DAT_0042a420 + DAT_00422408].t3 = DAT_00422400;
            if (mouse_b & 2)
                DAT_00422400 = DAT_00410110->tiles[(DAT_0042a424 + DAT_0042240c) * DAT_00410110->w + DAT_0042a420 + DAT_00422408].t3;
        }
        if (DAT_00422404 == 4) {
            mx = DAT_0042a420 * 16 + mx;
            my = DAT_0042a424 * 16 + my;
            if (mouse_b & 1)
                add_object(mx, my, DAT_00422400);
            for (i = 0; i < 100; i++) {
                if (DAT_00414140[i].f1a) {
                    if (ABS(DAT_00414140[i].x - mx + 8) < 8 && ABS(DAT_00414140[i].y - my + 8) < 8)
                        DAT_00414140[i].f14 = 1;
                    else
                        DAT_00414140[i].f14 = 0;
                }
            }
            if (mouse_b & 2) {
                for (i = 0; i < 100; i++) {
                    if (DAT_00414140[i].f14 && DAT_00414140[i].f1a) {
                        if (DAT_00414140[i].f19 & 1)
                            DAT_00410110->f7c--;
                        DAT_00414140[i].f1a = 0;
                    }
                }
            }
            if (key[KEY_T]) {
                for (i = 0; i < 100; i++) {
                    if (DAT_00414140[i].f14 && (DAT_00414140[i].f19 & 2) && DAT_00414140[i].f1a)
                        DAT_00414140[i].f1b = select_help_text();
                }
            }
            while (mouse_b)
                ;
        }
        if (key[KEY_1])
            DAT_00422404 = 0;
        if (key[KEY_2]) {
            DAT_00422404 = 1;
            DAT_0042acc0 = 1;
        }
        if (key[KEY_3])
            DAT_00422404 = 2;
        if (key[KEY_4])
            DAT_00422404 = 3;
        if (key[KEY_5])
            DAT_00422404 = 4;
        if (key[KEY_6])
            DAT_00422404 = 5;
        if (k == KEY_Z)
            DAT_00422400--;
        else if (k == KEY_X)
            DAT_00422400++;
        if (DAT_00422400 < 0)
            DAT_00422400 = DAT_004107a0[DAT_00422404];
        if (DAT_00422400 > DAT_004107a0[DAT_00422404])
            DAT_00422400 = 0;
        if (key[KEY_M]) {
            if (key[KEY_LEFT])
                shift_map(1, 0);
            if (key[KEY_RIGHT])
                shift_map(-1, 0);
            if (key[KEY_UP])
                shift_map(0, 1);
            if (key[KEY_DOWN])
                shift_map(0, -1);
            wait_arrows_released();
        }
        if (key[KEY_INSERT]) {
            if (key[KEY_LEFT]) {
                resize_map(1, 0);
                shift_map(-1, 0);
            }
            if (key[KEY_RIGHT])
                resize_map(1, 0);
            if (key[KEY_UP]) {
                resize_map(0, 1);
                shift_map(0, -1);
            }
            if (key[KEY_DOWN])
                resize_map(0, 1);
            wait_arrows_released();
        }
        if (key[KEY_DEL]) {
            if (key[KEY_LEFT]) {
                shift_map(1, 0);
                resize_map(-1, 0);
            }
            if (key[KEY_RIGHT])
                resize_map(-1, 0);
            if (key[KEY_UP]) {
                shift_map(0, 1);
                resize_map(0, -1);
            }
            if (key[KEY_DOWN])
                resize_map(0, -1);
            wait_arrows_released();
        }
        if (key[KEY_ALT]) {
            if (key[KEY_F]) {
                show_mouse(screen);
                i = do_menu(file_menu, mouse_x, mouse_y);
                show_mouse(NULL);
                text_mode(-1);
                switch (i) {
                case 0:
                    // STRING: MAPED 0x0040f580
                    if (yes_no_box("Make new map?"))
                        new_map();
                    break;
                case 1:
                    load_map(0);
                    break;
                case 2:
                    save_map();
                    break;
                case 3:
                    save_map_as();
                    break;
                case 4:
                    // STRING: MAPED 0x0040f570
                    if (yes_no_box("Really revert?"))
                        load_map(1);
                    break;
                case 5:
                    // STRING: MAPED 0x0040f568
                    if (yes_no_box("Quit?"))
                        done = 1;
                    break;
                }
            }
            if (key[KEY_M])
                edit_properties();
            if (key[KEY_T] && DAT_00422404 < 2)
                pick_tile();
            if (key[KEY_Y]) {
                for (i = 0; i < DAT_00410110->w; i++) {
                    for (j = 0; j < DAT_00410110->h; j++)
                        auto_mask(i, j);
                }
            }
            if (key[KEY_Q])
                FUN_00401160();
            if (key[KEY_N] && yes_no_box("Make new map?"))
                new_map();
            if (key[KEY_S])
                save_map();
            if (key[KEY_A])
                save_map_as();
            if (key[KEY_L])
                load_map(0);
            if (key[KEY_R] && yes_no_box("Really revert?"))
                load_map(1);
        }
        if (key[KEY_F1])
            FUN_00403140();
        if (k == KEY_F2)
            DAT_0042acc0 = !DAT_0042acc0;
        if (key[KEY_F3])
            check_masks();
        if (key[KEY_F5]) {
            DAT_00410110->f03 = DAT_00422408 + DAT_0042a420;
            DAT_00410110->f04 = DAT_0042240c + DAT_0042a424;
        }
        if (k == KEY_F7) {
            export_map_image(get_filename(DAT_0042a440));
        } else if (k == KEY_F8) {
            take_screenshot();
            // STRING: MAPED 0x0040f554
            show_message("Screenshot taken!", 1);
        } else if (k == KEY_F9) {
            DAT_00410120 = !DAT_00410120;
        }
        if (key[KEY_F11])
            edit_colors();
        if (k == KEY_F12)
            set_screen_mode(0);
        if (key[KEY_ESC] && yes_no_box("Quit?"))
            break;
    } while (!done);
}

/* [verified] Allegro's main: allegro_init, chdir to the folder of the
 * exe, initialisation; with one argument, loads that map as load_map
 * does; then the editor loop and the shutdown. chdir (the OLDNAMES alias)
 * and _chdir give the same call; reccmp only knows the CRT name of the
 * target, __chdir, so the source uses _chdir. */
// FUNCTION: MAPED 0x00405ae0
int main(int argc, char *argv[])
{
    char exe[1024];
    char dir[1024];
    MAP *m;

    allegro_init();
    get_executable_name(exe, sizeof(exe));
    // STRING: MAPED 0x00410128
    replace_filename(dir, exe, "", sizeof(dir));
    _chdir(dir);
    init_editor();
    if (argc == 2) {
        m = map_load(argv[1]);
        if (m != NULL) {
            strcpy(DAT_0042a440, argv[1]);
            if (DAT_00410110 != NULL)
                map_destroy(DAT_00410110);
            DAT_00410110 = m;
            DAT_0042a420 = 0;
            DAT_0042a424 = 0;
            load_objects();
            if (DAT_00410110->f79 & 8)
                map_load_datafile("data/xtiles.dat");
            else
                map_load_datafile("data/tiles.dat");
            FUN_004019b0();
            unload_datafile(DAT_004113d4);
            if (DAT_00410110->f79 & 8)
                DAT_004113d4 = load_datafile("data/xsprites.dat");
            else
                DAT_004113d4 = load_datafile("data/sprites.dat");
            init_color_tables();
            if (DAT_00410110->f77 > 2)
                DAT_00410110->f77 = 0;
            if (DAT_00410110->f78 > 11)
                DAT_00410110->f78 = 0;
            text_mode(-1);
            FUN_00401400();
            DAT_0042acc0 = 1;
        }
    }
    editor_loop();
    shutdown_editor();
    return 0;
}

/* [verified] Allegro's END_OF_MAIN() writes WinMain right after main;
 * same bytes as happy 0x0040A970 and loader 0x004014A0. Lookup by name
 * (notes/matching.md §3). */
// FUNCTION: MAPED 0x00405c70
// WinMain
END_OF_MAIN();
