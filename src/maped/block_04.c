/* Initialisation and shutdown: maped block_04 (0x004018D0-0x00401D8F).
 * notes/layout-maped.md §5. Screenshot, datafile counts, graphics mode,
 * colour tables, data files, initialisation and shutdown; same sequence
 * as happy block_02 [hypothesis: skeleton copied, notes/layout-maped.md
 * §6]. maped only. */
#include <stdio.h>
#include <string.h>
#include <allegro.h>
#include "maped.h"

/* Saves the screen as maped000.pcx, maped001.pcx... (extension from
 * output_format in maped.ini), up to 999 [verified]; returns 0. Only
 * caller 0x00404C10 ("Screenshot taken!"). */
// FUNCTION: MAPED 0x004018d0
int take_screenshot(void)
{
    char filename[32];
    PALETTE pal;
    BITMAP *sub;

    if (DAT_00410118 > 999)
        return 0;
    sub = create_sub_bitmap(screen, 0, 0, SCREEN_W, SCREEN_H);
    if (sub == NULL)
        return 0;
    get_palette(pal);
    // STRING: MAPED 0x0040f1b8
    sprintf(filename, "maped%03d.%s", DAT_00410118,
            // STRING: MAPED 0x0040f1c8
            get_config_string(NULL, "output_format",
                              // STRING: MAPED 0x0040f1d8
                              "pcx"));
    DAT_00410118++;
    packfile_password(NULL);
    save_bitmap(filename, sub, pal);
    // STRING: MAPED 0x0040f1b0
    packfile_password("happy");
    destroy_bitmap(sub);
    return 0;
}

/* Counts the tiles of map_datafile (entries from index 2 up to DAT_END)
 * and sets the number of choices of each editing mode, DAT_004107a0
 * [verified: pick_tile indexes it by the mode]. Bit 3 of MAP.f79 (the
 * "x" tile set, as in happy load_level) gives one item more.
 * [verified] An if/else in this position: VC6 turns it into
 * ((f79 & 8) | 0x90) >> 3; a ?: or the statement at the end of the
 * function gives other registers or another order. */
// FUNCTION: MAPED 0x004019b0
void FUN_004019b0(void)
{
    for (DAT_004107a0[0] = 0; map_datafile[DAT_004107a0[0] + 2].type != DAT_END; DAT_004107a0[0]++)
        ;
    DAT_004107a0[1] = DAT_004107a0[0];
    DAT_004107a0[2] = 10;
    if (DAT_00410110->f79 & 8)
        DAT_004107a0[3] = 19;
    else
        DAT_004107a0[3] = 18;
    DAT_004107a0[4] = 34;
    DAT_004107a0[5] = 9;
}

/* Sets the graphics mode [verified]: 640x480 if mode is 0, the screen is
 * 320 wide and the map is larger than 40x30 tiles (after saving the map to
 * autosave.map); 320x200 if mode is set or the screen is not 320 wide.
 * Windowed if run_in_a_window is set in maped.ini. Then the view size in
 * tiles and the clamped scroll and cursor positions [hypothesis], and the
 * palette. Callers 0x00401C20 (mode 1), 0x004032C0 (-1) and 0x00404C10
 * (0, F12: toggle). [verified] Shape: the two 320x200 cases written as
 * separate calls, which VC6 merges into one; with a single else
 * (!mode && SCREEN_W == 320), VC6 keeps the map pointer of the 640x480
 * test in ebp for the clamps and reloads it after the call. The nested
 * form (if (!mode) { if (SCREEN_W == 320) ... else ... } else ...) gives
 * the same code. */
// FUNCTION: MAPED 0x00401a20
void set_screen_mode(int mode)
{
    int card;

    // STRING: MAPED 0x0040f1ec
    card = get_config_int(NULL, "run_in_a_window", 0) ? GFX_AUTODETECT_WINDOWED : GFX_AUTODETECT_FULLSCREEN;
    if (mode) {
        set_gfx_mode(card, 320, 200, 0, 0);
    } else if (SCREEN_W == 320) {
        // STRING: MAPED 0x0040f1dc
        map_save("autosave.map", DAT_00410110);
        if (DAT_00410110->w > 40 && DAT_00410110->h > 30)
            set_gfx_mode(card, 640, 480, 0, 0);
    } else {
        set_gfx_mode(card, 320, 200, 0, 0);
    }
    DAT_0042acc8 = SCREEN_W / 16;
    DAT_0042acc4 = SCREEN_H / 16;
    DAT_0042a420 = MIN(DAT_00410110->w - DAT_0042acc8, MAX(DAT_0042a420, 0));
    DAT_0042a424 = MIN(DAT_00410110->h - DAT_0042acc4, MAX(DAT_0042a424, 0));
    DAT_00422408 = MIN(DAT_0042acc8 - 1, MAX(DAT_00422408, 0));
    DAT_0042240c = MIN(DAT_0042acc4 - 1, MAX(DAT_0042240c, 0));
    set_palette(DAT_0042ace0);
}

/* Takes the palette of the tiles datafile and builds the RGB and light
 * tables from it [verified]. Same as happy init_color_tables (0x00401A30)
 * without the translucency table, and lit towards 62,62,62. Callers
 * 0x00401C20 and others. */
// FUNCTION: MAPED 0x00401b90
void init_color_tables(void)
{
    set_palette((RGB *)map_datafile[0].dat);
    get_palette(DAT_0042ace0);
    create_rgb_table(&DAT_00422420, DAT_0042ace0, NULL);
    rgb_map = &DAT_00422420;
    create_light_table(&DAT_00412400, DAT_0042ace0, 62, 62, 62, NULL);
    color_map = &DAT_00412400;
}

/* Loads data/sprites.dat and data/misc.dat [verified], without checking
 * them. Only caller 0x00401C20. */
// FUNCTION: MAPED 0x00401bf0
void load_data(void)
{
    // STRING: MAPED 0x0040f20c
    DAT_004113d4 = load_datafile("data/sprites.dat");
    // STRING: MAPED 0x0040f1fc
    misc_datafile = load_datafile("data/misc.dat");
}

/* Initialisation [verified]: maped.ini, datafile password, Allegro
 * drivers, the 640x480 back buffer, an empty 50x50 map "unnamed.map" using
 * data/tiles.dat, the data files, 320x200, the object templates, the help
 * texts and the palette. Only caller main (0x00405AE0). */
// FUNCTION: MAPED 0x00401c20
void init_editor(void)
{
    // STRING: MAPED 0x0040f244
    set_config_file("maped.ini");
    packfile_password("happy");
    // STRING: MAPED 0x0040f23c
    strcpy(DAT_00410340, "./maps/");
    install_timer();
    install_keyboard();
    install_mouse();
    DAT_00422410 = create_bitmap(640, 480);
    DAT_00410110 = map_create(50, 50);
    // STRING: MAPED 0x0040f230
    strcpy(DAT_0042a440, "unnamed.map");
    // STRING: MAPED 0x0040f220
    strcpy(DAT_00410110->datafile, "data/tiles.dat");
    load_data();
    map_load_datafile(DAT_00410110->datafile);
    FUN_004019b0();
    init_color_tables();
    text_mode(-1);
    set_screen_mode(1);
    FUN_00401160();
    init_help_texts();
    FUN_00401400();
}

/* Shutdown [verified]: fades out, frees the back buffer, the map and the
 * three datafiles, then allegro_exit. Only caller main (0x00405AE0). */
// FUNCTION: MAPED 0x00401d20
void shutdown_editor(void)
{
    fade_out(32);
    destroy_bitmap(DAT_00422410);
    if (DAT_00410110)
        map_destroy(DAT_00410110);
    if (map_datafile)
        unload_datafile(map_datafile);
    if (misc_datafile)
        unload_datafile(misc_datafile);
    if (DAT_004113d4)
        unload_datafile(DAT_004113d4);
    allegro_exit();
}
