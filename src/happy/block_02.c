/* Initialisation and shutdown: happy block_02 (0x004018B0-0x0040219F).
 * notes/layout-happy.md §8. Timers, screenshot, high scores, colour tables,
 * scrolling message, yes/no box, data files, happy.sav, init_game and
 * shutdown_game. happy only. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <allegro.h>
#include "happy.h"

/* Called once per second [verified: install_int(timer_1s, 1000) in
 * init_game]: fps = frames of the last second, then the countdowns. */
// FUNCTION: HAPPY 0x004018b0
void timer_1s(void)
{
    fps = frame_count;
    frame_count = 0;
    if (DAT_00412088)
        time_left--;
    if (player.f16)
        player.f16--;
}

/* Called every 15 ms [verified: install_int(timer_15ms, 15)]. */
// FUNCTION: HAPPY 0x004018f0
void timer_15ms(void)
{
    timer_ticks++;
}

/* Saves bmp to filename with the current palette, without the datafile
 * password [verified]. Only caller 0x00404C50 ("happy.pcx"). */
// FUNCTION: HAPPY 0x00401900
void save_screenshot(BITMAP *bmp, char *filename)
{
    PALETTE pal;
    BITMAP *sub;

    packfile_password(NULL);
    get_palette(pal);
    sub = create_sub_bitmap(bmp, 0, 0, bmp->w, bmp->h);
    save_bitmap(filename, sub, pal);
    destroy_bitmap(sub);
    // STRING: HAPPY 0x004124b0
    packfile_password("happy");
}

/* Default high scores [verified]. Callers default_settings (0x00401EA0)
 * and 0x00408930. */
// FUNCTION: HAPPY 0x00401970
void init_hiscores(void)
{
    int i;

    for (i = 0; i < 6; i++) {
        // STRING: HAPPY 0x004124b8
        strcpy(hiscores[i].name, "Johan Peitz");
        hiscores[i].score = 10000 - i * 1500;
    }
}

/* Counts the entries of DAT_00414c90 into DAT_004450a8, minus 9 (8 if
 * DAT_00413b20 is set), and derives four indices from it [verified].
 * Only caller 0x00409870, which ignores the result. [verified] Returning
 * the count is the only form found that keeps it in eax until the end
 * (lea ecx, [eax+7] instead of add eax, 7). */
// FUNCTION: HAPPY 0x004019c0
int FUN_004019c0(void)
{
    for (DAT_004450a8 = 0; DAT_00414c90[DAT_004450a8].type != DAT_END; DAT_004450a8++)
        ;
    if (DAT_00413b20 == 0)
        DAT_004450a8 -= 9;
    else
        DAT_004450a8 -= 8;
    DAT_0042f568 = 4;
    DAT_00414ca0 = DAT_004450a8 + 5;
    DAT_00414c94 = DAT_004450a8 + 6;
    DAT_004450a4 = DAT_004450a8 + 4;
    DAT_00414ca4 = DAT_004450a8 + 7;
    return DAT_004450a8;
}

/* Builds the RGB, translucency and light tables of DAT_00441c80 [verified].
 * Only caller 0x00402EA0 (palette). */
// FUNCTION: HAPPY 0x00401a30
void init_color_tables(void)
{
    create_rgb_table(&rgb_table, DAT_00441c80, NULL);
    rgb_map = &rgb_table;
    create_trans_table(&trans_table, DAT_00441c80, 128, 128, 128, NULL);
    create_light_table(&light_table, DAT_00441c80, 0, 0, 0, NULL);
    color_map = &light_table;
}

/* Starts a message that scrolls from right to left [verified: scroll_x
 * starts at 340, draw_scroll_message decrements it down to scroll_end].
 * 4 callers in the game. */
// FUNCTION: HAPPY 0x00401aa0
void start_scroll_message(char *text)
{
    strcpy(scroll_text, text);
    scroll_width = text_length((FONT *)misc_datafile[4].dat, scroll_text);
    scroll_x = 340;
    scroll_y = 180;
    scroll_end = -20 - scroll_width;
    scroll_active = 1;
}

/* Draws the scrolling message on bmp, on a dark band, and moves it one
 * pixel to the left [verified]. The band rises at the start and falls at
 * the end. Only caller 0x00402620. */
// FUNCTION: HAPPY 0x00401b20
void draw_scroll_message(BITMAP *bmp)
{
    int y;

    y = scroll_y;
    if (scroll_x > 320)
        y = scroll_x - 140;
    if (scroll_x < scroll_end + 20)
        y = scroll_end - scroll_x + 200;
    color_map = &light_table;
    drawing_mode(DRAW_MODE_TRANS, NULL, 0, 0);
    rectfill(bmp, 0, y - 1, 319, 199, 150);
    solid_mode();
    textout(bmp, (FONT *)misc_datafile[4].dat, scroll_text, scroll_x, y, -1);
    scroll_x--;
    if (scroll_x == scroll_end)
        scroll_active = 0;
}

/* Asks a yes/no question in a box on the back buffer [verified]: Y or
 * ENTER answer yes (1), N or ESC no (0); waits for all four keys to be
 * released before and after. time_left is restored on exit. Callers
 * 0x00404C50 ("Quit?"), 0x00409640. */
// FUNCTION: HAPPY 0x00401be0
int yes_no_box(char *text)
{
    int saved;
    int w;
    int answer;

    saved = time_left;
    if (text_length((FONT *)misc_datafile[4].dat, text) < 100)
        w = 100;
    else
        w = text_length((FONT *)misc_datafile[4].dat, text);
    color_map = &light_table;
    drawing_mode(DRAW_MODE_TRANS, NULL, 0, 0);
    rectfill(buffer, 150 - w / 2, 60, w / 2 + 170, 130, 100);
    solid_mode();
    rect(buffer, 150 - w / 2, 60, w / 2 + 170, 130, 175);
    textout_centre(buffer, (FONT *)misc_datafile[4].dat, text, 160, 70, -1);
    // STRING: HAPPY 0x004124c4
    textout_centre(buffer, (FONT *)misc_datafile[4].dat, "Yes / No", 160, 100, -1);
    blit_to_screen(buffer);
    while (key[KEY_Y] || key[KEY_ENTER] || key[KEY_N] || key[KEY_ESC])
        ;
    answer = 0;
    while (answer == 0) {
        if (key[KEY_Y] || key[KEY_ENTER])
            answer = 1;
        if (key[KEY_N] || key[KEY_ESC])
            answer = 2;
    }
    while (key[KEY_Y] || key[KEY_ENTER] || key[KEY_N] || key[KEY_ESC])
        ;
    time_left = saved;
    return answer != 2;
}

/* Loads data/misc.dat and data/tiles.dat; 0 if either fails [verified].
 * Only caller init_game. */
// FUNCTION: HAPPY 0x00401d60
int load_data(void)
{
    int ok;

    ok = 1;
    // STRING: HAPPY 0x004124e0
    misc_datafile = load_datafile("data/misc.dat");
    // STRING: HAPPY 0x004124d0
    map_datafile = load_datafile("data/tiles.dat");
    if (misc_datafile == NULL || map_datafile == NULL)
        ok = 0;
    return ok;
}

/* Writes happy.sav: volumes, controls, high scores, screen_mode
 * [verified]. Only caller shutdown_game. */
// FUNCTION: HAPPY 0x00401da0
void save_settings(void)
{
    FILE *f;

    /* reccmp takes the first literal of a line: one per line */
    // STRING: HAPPY 0x004124f0
    f = fopen("happy.sav",
              // STRING: HAPPY 0x004124fc
              "wb");
    fwrite(&DAT_00442088, 1, 1, f);
    fwrite(&midi_volume, 1, 1, f);
    fwrite(controls, sizeof(controls), 1, f);
    fwrite(hiscores, sizeof(hiscores), 1, f);
    fwrite(&screen_mode, sizeof(screen_mode), 1, f);
    fclose(f);
}

/* Reads happy.sav; 0 if it does not exist [verified]. screen_mode is reset
 * to 0 unless it is 1. Only caller init_game. */
// FUNCTION: HAPPY 0x00401e10
int load_settings(void)
{
    FILE *f;

    /* "rb": pooled with map_load, annotated there */
    f = fopen("happy.sav", "rb");
    if (f == NULL)
        return 0;
    fread(&DAT_00442088, 1, 1, f);
    fread(&midi_volume, 1, 1, f);
    fread(controls, sizeof(controls), 1, f);
    fread(hiscores, sizeof(hiscores), 1, f);
    fread(&screen_mode, sizeof(screen_mode), 1, f);
    fclose(f);
    if (screen_mode != 1)
        screen_mode = 0;
    return 1;
}

/* Settings when there is no happy.sav [verified]. Only caller init_game. */
// FUNCTION: HAPPY 0x00401ea0
void default_settings(void)
{
    controls[0] = KEY_RIGHT;
    controls[1] = KEY_LEFT;
    controls[4] = KEY_ALT;
    controls[5] = KEY_LCONTROL;
    controls[6] = KEY_DOWN;
    init_hiscores();
    DAT_00442088 = 200;
    midi_volume = 220;
    set_volume(-1, 220);
    screen_mode = 0;
}

/* Initialises Allegro and the game [verified]. Only caller 0x0040A830
 * (main). */
// FUNCTION: HAPPY 0x00401f00
void init_game(void)
{
    allegro_init();
    set_color_depth(8);
    packfile_password("happy");
    install_keyboard();
    use_joystick = !install_joystick(JOY_TYPE_AUTODETECT);
    install_timer();
    install_int(timer_1s, 1000);
    fps = 0;
    frame_count = 0;
    install_int(timer_15ms, 15);
    timer_ticks = 0;
    buffer = create_bitmap(320, 200);
    DAT_004274e4 = create_bitmap(640, 200);
    if (!load_data()) {
        // STRING: HAPPY 0x00412504
        allegro_message("Failed to load data.");
        exit(1);
    }
    if (!load_settings())
        default_settings();
    install_mouse();
    if (install_sound(DIGI_AUTODETECT, MIDI_AUTODETECT, NULL) == 0)
        set_volume(-1, midi_volume);
    if (screen_mode)
        set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0);
    else
        set_gfx_mode(GFX_AUTODETECT, 320, 200, 0, 0);
    text_mode(-1);
    FUN_00401000();
    FUN_00401160();
    init_help_texts();
    DAT_00440044 = itofix(1);
    player.f1c = itofix(1) / 10;
    DAT_00414024 = fixdiv(itofix(22), itofix(100));
    DAT_004404c0 = fixdiv(itofix(12), itofix(100));
    DAT_0042f59c = itofix(1) / 10;
    DAT_00414028 = itofix(1);
    DAT_00414c98 = itofix(1) / 100;
}

/* Inline function of al386vc.h, emitted out of line because the asm of
 * fixdiv calls it (notes/allegro.md). Library code, not decompiled. */
// LIBRARY: HAPPY 0x00402110
// _set_errno_erange

/* Saves the settings and frees everything [verified]. Only caller
 * 0x0040A830 (main). */
// FUNCTION: HAPPY 0x00402120
void shutdown_game(void)
{
    fade_out(32);
    save_settings();
    if (buffer)
        destroy_bitmap(buffer);
    if (DAT_004274e4)
        destroy_bitmap(DAT_004274e4);
    if (DAT_00414130)
        map_destroy(DAT_00414130);
    unload_datafile(map_datafile);
    unload_datafile(misc_datafile);
    unload_datafile(DAT_00415cc0);
    allegro_exit();
}
