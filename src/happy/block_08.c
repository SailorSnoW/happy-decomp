/* happy block_08: 0x00409870-0x0040A99F, start menu, custom game, main,
 * WinMain (notes/layout-happy.md §8). Functions in address order.
 * 0x00409870 (start menu) and 0x00409F30 (custom game), among the large
 * functions left for the end, are not decompiled yet: the calls to them
 * are one of the two differences of main. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <allegro.h>
#include "happy.h"

/* [hypothesis] Start menu [verified: "HAPPYLAND ADVENTURES", "THE
 * ORIGINAL GAME", "CREATE CUSTOM GAME", "EXIT"]: the mouse (sprite 37 of
 * misc_datafile) picks a band of the screen, highlighted with colour 166,
 * and DAT_00413b20 follows it: 0 above y = 90, 1 up to 140, 2 below,
 * 3 for "EXIT" (x > 300, y < 10). Up and down move the mouse to the next
 * choice, W toggles screen_mode between 320x200 and 640x480. A click or
 * Enter leaves: returns -1 for "EXIT", otherwise loads data/xlevels.dat
 * (0) or data/levels.dat (1 and 2) into DAT_00414c90, counts its entries
 * (FUN_004019c0) and returns 0. Only caller main. */
// FUNCTION: HAPPY 0x00409870
int start_menu(void)
{
    int x;
    int y;
    int k;

    set_palette((RGB *)map_datafile[0].dat);
    position_mouse(200, 45);
    while (!mouse_b && !key[KEY_ENTER]) {
        clear_bitmap(buffer);
        y = mouse_y * 200 / SCREEN_H;
        if (screen_mode)
            x = mouse_x >> 1;
        else
            x = mouse_x;
        DAT_00413b20 = (y > 140) + 1;
        if (y < 90)
            DAT_00413b20 = 0;
        if (y < 10 && x > 300)
            DAT_00413b20 = 3;
        switch (DAT_00413b20) {
        case 0:
            rectfill(buffer, 0, 10, 319, 90, 166);
            break;
        case 1:
            rectfill(buffer, 0, 90, 319, 140, 166);
            break;
        case 2:
            rectfill(buffer, 0, 140, 319, 189, 166);
            break;
        case 3:
            rectfill(buffer, 299, 0, 317, 8, 166);
            break;
        }
        draw_sprite(buffer, (BITMAP *)misc_datafile[35].dat, 81, 14);
        draw_sprite(buffer, (BITMAP *)misc_datafile[36].dat, 180, 50);
        // STRING: HAPPY 0x00412e7c
        textout_centre(buffer, (FONT *)misc_datafile[4].dat, "HAPPYLAND ADVENTURES", 161, 101, 0);
        // STRING: HAPPY 0x00412e68
        textout_centre(buffer, (FONT *)misc_datafile[25].dat, "THE ORIGINAL GAME", 161, 121, 0);
        textout_centre(buffer, (FONT *)misc_datafile[4].dat, "HAPPYLAND ADVENTURES", 160, 100, -1);
        textout_centre(buffer, (FONT *)misc_datafile[25].dat, "THE ORIGINAL GAME", 160, 120, 175);
        // STRING: HAPPY 0x00412e54
        textout_centre(buffer, (FONT *)misc_datafile[4].dat, "CREATE CUSTOM GAME", 161, 161, 0);
        textout_centre(buffer, (FONT *)misc_datafile[4].dat, "CREATE CUSTOM GAME", 160, 160, -1);
        textprintf_centre(buffer, (FONT *)misc_datafile[25].dat, 160, 194, 175,
                          // STRING: HAPPY 0x00412e24
                          "WIDESCREEN IS %s (PRESS 'W' TO TOGGLE)",
                          // STRING: HAPPY 0x00412e50
                          screen_mode ? "ON"
                          // STRING: HAPPY 0x00412e4c
                                      : "OFF");
        textout(buffer, (FONT *)misc_datafile[25].dat, "EXIT", 301, 2, 175);
        draw_sprite(buffer, (BITMAP *)misc_datafile[37].dat, x, y);
        if (keypressed()) {
            k = readkey() >> 8;
            if (k == KEY_UP) {
                if (DAT_00413b20 == 0)
                    position_mouse((screen_mode + 1) * 304, 5);
                if (DAT_00413b20 == 1)
                    position_mouse((screen_mode + 1) * 200, SCREEN_H * 45 / 200);
                if (DAT_00413b20 == 2)
                    position_mouse((screen_mode + 1) * 200, SCREEN_H * 115 / 200);
            } else if (k == KEY_DOWN) {
                if (DAT_00413b20 == 3)
                    position_mouse((screen_mode + 1) * 200, SCREEN_H * 45 / 200);
                if (DAT_00413b20 == 0)
                    position_mouse((screen_mode + 1) * 200, SCREEN_H * 115 / 200);
                if (DAT_00413b20 == 1)
                    position_mouse((screen_mode + 1) * 200, SCREEN_H * 165 / 200);
            } else if (k == KEY_W) {
                screen_mode = !screen_mode;
                if (screen_mode)
                    set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0);
                else
                    set_gfx_mode(GFX_AUTODETECT, 320, 200, 0, 0);
                set_palette((RGB *)map_datafile[0].dat);
            }
        }
        blit_to_screen(buffer);
    }
    show_mouse(NULL);
    fade_out(4);
    if (DAT_00413b20 == 3)
        return -1;
    if (DAT_00413b20 == 0)
        // STRING: HAPPY 0x00412e10
        DAT_00414c90 = load_datafile("data/xlevels.dat");
    else
        // STRING: HAPPY 0x00412e00
        DAT_00414c90 = load_datafile("data/levels.dat");
    FUN_004019c0();
    return 0;
}

/* [hypothesis] Draws a slot panel of the custom game screen: a box filled
 * with colour 166, light top and left edges (173), dark bottom and right
 * edges (168), and "RIGHT CLICK A SLOT" / "TO REMOVE IT" centred at its
 * bottom [verified: the strings, rectfill and line through the vtable,
 * +0x38 and +0x34]. Only caller 0x00409F30, twice for each of the two
 * boxes (18, 18, 150, 170) and (158, 18, 290, 90). */
// FUNCTION: HAPPY 0x00409e40
void draw_slot_panel(int x1, int y1, int x2, int y2)
{
    rectfill(buffer, x1, y1, x2, y2, 166);
    line(buffer, x1, y1, x2 - 1, y1, 173);
    line(buffer, x1, y1, x1, y2 - 1, 173);
    line(buffer, x2, y2, x1 + 1, y2, 168);
    line(buffer, x2, y2, x2, y1 + 1, 168);
    // STRING: HAPPY 0x00412ea4
    textout_centre(buffer, (FONT *)misc_datafile[25].dat, "RIGHT CLICK A SLOT", x1 + (x2 - x1) / 2, y2 - 20, 168);
    // STRING: HAPPY 0x00412e94
    textout_centre(buffer, (FONT *)misc_datafile[25].dat, "TO REMOVE IT", x1 + (x2 - x1) / 2, y2 - 12, 168);
}

/* [hypothesis] Custom game screen [verified: "Add Level", "Add Bonus",
 * "Play!", file_select, happy.clv]: lists the level maps DAT_004404e0
 * (DAT_004450a8 of them, at most 18) and the bonus maps DAT_0043f5c0
 * (DAT_0042f568, at most 8), read from happy.clv. "Add Level" / "Add
 * Bonus" pick a .map file under ./maps/ (full path in f000, file name in
 * f0ff), a right click on a name removes it. "Play!" needs one map of
 * each kind (alert otherwise), saves happy.clv and returns 1; "EXIT"
 * returns 0. The two counts are read and written as 2 bytes each
 * [verified: size 2 at 0x00409FA2, 0x00409FB1], one byte more than the
 * globals. On a 640x480 screen the mouse coordinates are halved and
 * shifted up by 40 lines [verified]. Only caller main.
 * Shapes [verified against the listing]: the two ?: on screen_mode (VC6
 * merges the tests; an if/else gives x and y other registers); while
 * (!done) rather than do/while (the right click path then reaches the
 * test of done instead of jumping to the top of the loop); the "Play!"
 * tests nested. */
// FUNCTION: HAPPY 0x00409f30
int custom_game(void)
{
    FILE *f;
    int y;
    int x;
    int done = 0;
    char path[1024];
    int i;
    int j;
    int n;
    int b;

    DAT_004450a8 = 0;
    DAT_0042f568 = 0;
    // STRING: HAPPY 0x00412f60
    strcpy(path, "./maps/");
    // STRING: HAPPY 0x00412f54
    f = fopen("happy.clv",
              /* "rb": pooled with map_load, annotated there */
              "rb");
    if (f != NULL) {
        fread(DAT_004404e0, sizeof(DAT_004404e0), 1, f);
        fread(DAT_0043f5c0, sizeof(DAT_0043f5c0), 1, f);
        fread(&DAT_004450a8, 2, 1, f);
        fread(&DAT_0042f568, 2, 1, f);
        fclose(f);
    }
    text_mode(-1);
    set_palette(black_palette);
    clear_bitmap(buffer);
    clear_bitmap(screen);
    n = 0;
    for (i = 0; i < 336; i += 16) {
        for (j = 0; j < 208; j += 16) {
            if (++n == 2)
                n = 0;
            if (n)
                draw_rle_sprite(buffer, (RLE_SPRITE *)map_datafile[rand() % 8 + 113].dat, i, j);
        }
    }
    draw_slot_panel(18, 18, 150, 170);
    draw_slot_panel(158, 18, 290, 90);
    rectfill(buffer, 8, 178, 105, 198, 0);
    rect(buffer, 8, 178, 105, 198, makecol(200, 200, 200));
    // STRING: HAPPY 0x00412f48
    textout(buffer, (FONT *)misc_datafile[4].dat, "Add Level", 10, 180, -1);
    rectfill(buffer, 118, 178, 220, 198, 0);
    rect(buffer, 118, 178, 220, 198, makecol(200, 200, 200));
    // STRING: HAPPY 0x00412f3c
    textout(buffer, (FONT *)misc_datafile[4].dat, "Add Bonus", 120, 180, -1);
    rectfill(buffer, 258, 178, 305, 198, 0);
    rect(buffer, 258, 178, 305, 198, makecol(200, 200, 200));
    // STRING: HAPPY 0x00412f34
    textout(buffer, (FONT *)misc_datafile[4].dat, "Play!", 260, 180, -1);
    textout(buffer, (FONT *)misc_datafile[25].dat, "EXIT", 301, 1, 175);
    show_mouse(screen);
    set_mouse_sprite((BITMAP *)misc_datafile[37].dat);
    fade_in((RGB *)map_datafile[0].dat, 8);
    while (!done) {
        show_mouse(NULL);
        for (i = 0; i < DAT_004450a8; i++)
            textprintf(buffer, (FONT *)misc_datafile[25].dat, 20, 20 + i * 8, 175, "%s", DAT_004404e0[i].f0ff);
        for (i = 0; i < DAT_0042f568; i++)
            textprintf(buffer, (FONT *)misc_datafile[25].dat, 160, 20 + i * 8, 175, "%s", DAT_0043f5c0[i].f0ff);
        blit_to_screen(buffer);
        show_mouse(screen);
        while (!mouse_b)
            ;
        b = mouse_b;
        x = screen_mode ? mouse_x >> 1 : mouse_x;
        y = screen_mode ? (mouse_y - 40) * 200 / 400 : mouse_y;
        if (b == 2) {
            for (i = 0; i < DAT_004450a8; i++) {
                if (x > 20 && x < 150 && y > 20 + i * 8 && y < 26 + i * 8) {
                    DAT_004450a8--;
                    for (j = i; j < DAT_004450a8; j++)
                        DAT_004404e0[j] = DAT_004404e0[j + 1];
                }
            }
            for (i = 0; i < DAT_0042f568; i++) {
                if (x > 160 && x < 290 && y > 20 + i * 8 && y < 26 + i * 8) {
                    DAT_0042f568--;
                    for (j = i; j < DAT_0042f568; j++)
                        DAT_0043f5c0[j] = DAT_0043f5c0[j + 1];
                }
            }
            while (mouse_b & 2)
                ;
            show_mouse(NULL);
            vsync();
            draw_slot_panel(18, 18, 150, 170);
            draw_slot_panel(158, 18, 290, 90);
            blit_to_screen(buffer);
            show_mouse(screen);
        } else if (b == 1) {
            if (y > 180 && y < 200 && x > 260) {
                if (DAT_004450a8) {
                    if (DAT_0042f568)
                        done = 1;
                    else
                        // STRING: HAPPY 0x00412ef4
                        alert("ERROR",
                              // STRING: HAPPY 0x00412efc
                              "You must add at least",
                              // STRING: HAPPY 0x00412f24
                              "one bonusroom!",
                              // STRING: HAPPY 0x004124ac
                              "OK", NULL, 0, 0);
                } else {
                    alert("ERROR", "You must add at least",
                          // STRING: HAPPY 0x00412f14
                          "one level map!", "OK", NULL, 0, 0);
                }
            }
            if (y < 200 && y > 180 && x > 10 && x < 110 && DAT_004450a8 < 18) {
                // STRING: HAPPY 0x00412ed4
                if (file_select("Add a level map. (*.MAP)", path,
                                // STRING: HAPPY 0x00412ef0
                                "MAP")) {
                    strcpy(DAT_004404e0[DAT_004450a8].f000, path);
                    strcpy(DAT_004404e0[DAT_004450a8].f0ff, get_filename(path));
                    DAT_004450a8++;
                }
            }
            if (y > 180 && y < 200 && x > 120 && x < 210 && DAT_0042f568 < 8) {
                // STRING: HAPPY 0x00412eb8
                if (file_select("Add a bonus map. (*.MAP)", path, "MAP")) {
                    strcpy(DAT_0043f5c0[DAT_0042f568].f000, path);
                    strcpy(DAT_0043f5c0[DAT_0042f568].f0ff, get_filename(path));
                    DAT_0042f568++;
                }
            }
            text_mode(-1);
            if (x > 301 && y < 10 && y >= 0) {
                show_mouse(NULL);
                fade_out(8);
                return 0;
            }
        }
    }
    show_mouse(NULL);
    fade_out(8);
    f = fopen("happy.clv",
              /* "wb": pooled with save_settings, annotated there */
              "wb");
    fwrite(DAT_004404e0, sizeof(DAT_004404e0), 1, f);
    fwrite(DAT_0043f5c0, sizeof(DAT_0043f5c0), 1, f);
    fwrite(&DAT_004450a8, 2, 1, f);
    fwrite(&DAT_0042f568, 2, 1, f);
    fclose(f);
    return 1;
}

/* The game's main(), called by Allegro's _WinMain [verified: push
 * 0x0040A830 in WinMain]. Options, compared ignoring case: "-widescreen"
 * sets screen_mode to 1, "-fps" sets DAT_00413b10. Neither has an effect
 * [verified in the code]: init_game, called next, always rewrites
 * screen_mode (load_settings or default_settings), and nothing reads
 * DAT_00413b10. Then the start menu (start_menu) (negative: quit); when
 * DAT_00413b20 is 2, the custom game screen (custom_game), back to the start
 * menu when it returns 0. Introduction: play_intro for DAT_00413b20 = 0,
 * otherwise show_logo and music 19. Menu loop: main_menu
 * (DAT_00413b20 != 0) or main_menu_password; 1 plays from level 1, 4 asks
 * for a password (enter_password, 0 = back to the menu) and starts at the
 * level after the one returned [hypothesis: DAT_004450b0 is the first
 * level, copied into DAT_00440038 by FUN_00403030]; after play_game,
 * music 21 (unless DAT_00413b2c, set by play_ending) or 19. 3 shows the
 * instructions, 2 quits [verified].
 * Below 100%: VC6 rotates the start menu loop, the original does not
 * (notes/block08-diffs.md); every shape tried gives the same code, it is
 * the only difference left. */
// FUNCTION: HAPPY 0x0040a830
int main(int argc, char *argv[])
{
    int i;
    int sel;

    for (i = 1; i < argc; i++) {
        // STRING: HAPPY 0x00412f70
        if (strcmpi(argv[i], "-widescreen") == 0)
            screen_mode = 1;
        // STRING: HAPPY 0x00412f68
        if (strcmpi(argv[i], "-fps") == 0)
            DAT_00413b10 = 1;
    }
    init_game();
    do {
        if (start_menu() < 0)
            goto quit;
    } while (DAT_00413b20 == 2 && custom_game() == 0);
    if (DAT_00413b20 == 0) {
        play_intro();
    } else {
        show_logo();
        play_midi(misc_datafile[19].dat, 1);
    }
    for (;;) {
        if (DAT_00413b20 != 0)
            sel = main_menu();
        else
            sel = main_menu_password();
        if (sel == 1 || sel == 4) {
            DAT_004450b0 = 1;
            if (sel == 4) {
                i = enter_password();
                if (i == 0)
                    continue;
                DAT_004450b0 = i + 1;
            }
            play_game();
            if (DAT_00413b20 == 0) {
                if (DAT_00413b2c == 0)
                    play_midi(misc_datafile[21].dat, 1);
            } else {
                play_midi(misc_datafile[19].dat, 1);
            }
        }
        if (sel == 2)
            break;
        if (sel == 3)
            show_instructions();
    }
quit:
    shutdown_game();
    return 0;
}

/* [verified] Allegro's END_OF_MAIN() writes WinMain right after main.
 * reccmp cannot see a definition in a macro call: lookup by symbol name. */
// FUNCTION: HAPPY 0x0040a970
// WinMain
END_OF_MAIN();
