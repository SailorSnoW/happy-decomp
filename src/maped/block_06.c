/* Map files and dialogs: maped block_06 (0x00403200-0x00403EAF).
 * notes/layout-maped.md §8. Loading a map and its objects, the map
 * properties dialog, a new map, the message and yes/no boxes, saving.
 * maped only. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <allegro.h>
#include "maped.h"

/* Reads the objects and the help texts that follow the tiles in the map
 * file DAT_0042a440 [verified], like happy load_level_objects (0x00403070).
 * Default help texts unless the last byte of the first one is '*'
 * [hypothesis: marker of texts saved by the editor]. Counts the objects
 * with bit 0 of f19 into MAP.f7c (the Happylanders) and gives each object
 * the sprite of its template. Only caller load_map (0x004032C0). */
// FUNCTION: MAPED 0x00403200
void load_objects(void)
{
    FILE *f;
    int pos;
    int i;

    pos = DAT_00410110->h * DAT_00410110->w * sizeof(TILE) + MAP_HEADER_SIZE;
    f = fopen(DAT_0042a440, "rb");
    fseek(f, pos, SEEK_SET);
    fread(DAT_00414140, sizeof(DAT_00414140), 1, f);
    fread(help_texts, sizeof(help_texts), 1, f);
    fclose(f);
    if (help_texts[0][256] != '*')
        init_help_texts();
    DAT_00410110->f7c = 0;
    for (i = 0; i < 100; i++) {
        if (DAT_00414140[i].f1a && (DAT_00414140[i].f19 & 1))
            DAT_00410110->f7c++;
        if (DAT_00414140[i].f1a)
            DAT_00414140[i].f13 = DAT_00440060[DAT_00414140[i].f18].f13;
    }
}

/* Loads a map [verified]: a file chosen in a file selector under
 * DAT_00410340, or, if reload is set, DAT_0042a440 again ("Revert"). On
 * failure an alert and an empty 96x12 map. Then the objects, the tiles and
 * sprites of the tile set (x versions if bit 3 of MAP.f79), the colour
 * tables; out-of-range background (f77) and music (f78) are reset, cover
 * tiles shown, and 320x200 if the map is smaller than 40x30 tiles.
 * Only caller 0x00404C10. */
// FUNCTION: MAPED 0x004032c0
void load_map(int reload)
{
    if (!reload) {
        // STRING: MAPED 0x0040f468
        if (file_select("Select a map to load. (*.MAP)", DAT_00410340,
                        // STRING: MAPED 0x0040f488
                        "MAP")) {
            map_destroy(DAT_00410110);
            strcpy(DAT_0042a440, DAT_00410340);
            DAT_00410110 = map_load(DAT_0042a440);
            if (DAT_00410110 == NULL) {
                // STRING: MAPED 0x0040f430
                alert("LOAD ERROR", NULL,
                      // STRING: MAPED 0x0040f43c
                      "Sorry, failed to load.",
                      // STRING: MAPED 0x0040f454
                      "Drat!",
                      // STRING: MAPED 0x0040f45c
                      "Too bad!", 'y', 27);
                DAT_00410110 = map_create(96, 12);
            } else {
                DAT_0042a420 = 0;
                DAT_0042a424 = 0;
                load_objects();
                if (DAT_00410110->f79 & 8)
                    // STRING: MAPED 0x0040f420
                    map_load_datafile("data/xtiles.dat");
                else
                    map_load_datafile("data/tiles.dat");
                FUN_004019b0();
                unload_datafile(DAT_004113d4);
                if (DAT_00410110->f79 & 8)
                    // STRING: MAPED 0x0040f40c
                    DAT_004113d4 = load_datafile("data/xsprites.dat");
                else
                    DAT_004113d4 = load_datafile("data/sprites.dat");
                init_color_tables();
            }
        }
    } else {
        map_destroy(DAT_00410110);
        DAT_00410110 = map_load(DAT_0042a440);
        if (DAT_00410110 == NULL) {
            alert("LOAD ERROR", NULL, "Sorry, failed to load.", "Drat!", "Too bad!", 'y', 27);
            DAT_00410110 = map_create(96, 12);
        } else {
            load_objects();
            if (DAT_00410110->f79 & 8)
                map_load_datafile("data/xtiles.dat");
            else
                map_load_datafile("data/tiles.dat");
            FUN_004019b0();
            init_color_tables();
        }
    }
    if (DAT_00410110->f77 > 2)
        DAT_00410110->f77 = 0;
    if (DAT_00410110->f78 > 11)
        DAT_00410110->f78 = 0;
    text_mode(-1);
    FUN_00401400();
    DAT_0042acc0 = 1;
    if ((DAT_00410110->w < 40 || DAT_00410110->h < 30) && SCREEN_W > 320)
        set_screen_mode(-1);
}

/* Map properties [verified]: fills properties_dialog from the map (time,
 * happies needed, name, tile set summer/winter from bit 3 of MAP.f79, the
 * check boxes scroll/snow/wind from bits 0-2, music f78, background f77),
 * runs it, and on OK (object 2) writes the values back, then reloads the
 * tiles, the palette and the sprites of the tile set. Callers new_map
 * (0x00403740) and 0x00404C10. */
// FUNCTION: MAPED 0x004034b0
void edit_properties(void)
{
    int i;
    int flags;

    show_mouse(screen);
    // STRING: MAPED 0x0040f48c
    sprintf(DAT_0042a840, "%d", DAT_00410110->f7a);
    sprintf(DAT_00410790, "%d", DAT_00410110->f7b);
    strcpy(DAT_00410740, DAT_00410110->name);
    if (DAT_00410110->f79 & 8) {
        properties_dialog[23].flags = 0;
        properties_dialog[24].flags = D_SELECTED;
    } else {
        properties_dialog[24].flags = 0;
        properties_dialog[23].flags = D_SELECTED;
    }
    properties_dialog[25].flags = (DAT_00410110->f79 & 1) << 1;
    properties_dialog[26].flags = DAT_00410110->f79 & 2;
    properties_dialog[27].flags = (DAT_00410110->f79 >> 1) & 2;
    for (i = 0; i < 11; i++)
        properties_dialog[11 + i].flags = DAT_00410110->f78 == i ? D_SELECTED : 0;
    for (i = 0; i < 3; i++)
        properties_dialog[29 + i].flags = DAT_00410110->f77 == i ? D_SELECTED : 0;
    if (popup_dialog(properties_dialog, 0) == 2) {
        for (i = 0; i < 11; i++) {
            if (properties_dialog[11 + i].flags == D_SELECTED)
                DAT_00410110->f78 = i;
        }
        for (i = 0; i < 3; i++) {
            if (properties_dialog[29 + i].flags == D_SELECTED)
                DAT_00410110->f77 = i;
        }
        DAT_00410110->f7b = atoi(DAT_00410790);
        DAT_00410110->f7a = atoi(DAT_0042a840);
        strcpy(DAT_00410110->name, DAT_00410740);
        flags = 0;
        if (properties_dialog[24].flags & D_SELECTED)
            flags = 8;
        if (properties_dialog[25].flags & D_SELECTED)
            flags += 1;
        if (properties_dialog[26].flags & D_SELECTED)
            flags += 2;
        if (properties_dialog[27].flags & D_SELECTED)
            flags += 4;
        DAT_00410110->f79 = flags;
        if (DAT_00410110->f79 & 8)
            map_load_datafile("data/xtiles.dat");
        else
            map_load_datafile("data/tiles.dat");
        set_palette((RGB *)map_datafile[0].dat);
        get_palette(DAT_0042ace0);
        FUN_00401400();
        FUN_004019b0();
        unload_datafile(DAT_004113d4);
        if (DAT_00410110->f79 & 8)
            DAT_004113d4 = load_datafile("data/xsprites.dat");
        else
            DAT_004113d4 = load_datafile("data/sprites.dat");
    }
    show_mouse(NULL);
    text_mode(-1);
}

/* New map [verified]: an empty 50x50 map "./maps/unnamed.map" using
 * data/tiles.dat, then the properties dialog. Only caller 0x00404C10. */
// FUNCTION: MAPED 0x00403740
void new_map(void)
{
    if (DAT_00410110)
        map_destroy(DAT_00410110);
    DAT_00410110 = map_create(50, 50);
    // STRING: MAPED 0x0040f490
    strcpy(DAT_0042a440, "./maps/unnamed.map");
    strcpy(DAT_00410110->datafile, "data/tiles.dat");
    map_load_datafile(DAT_00410110->datafile);
    FUN_004019b0();
    init_color_tables();
    edit_properties();
}

/* Shows text in a box in the middle of the screen [verified]; if wait is
 * set, waits for ENTER or ESC and for its release. 6 callers. */
// FUNCTION: MAPED 0x00403800
void show_message(char *text, int wait)
{
    int w;

    w = text_length(font, text);
    rectfill(screen, SCREEN_W / 2 - w / 2 - 10, SCREEN_H / 2 - 30,
             SCREEN_W / 2 + w / 2 + 10, SCREEN_H / 2 + 20, 1);
    rect(screen, SCREEN_W / 2 - w / 2 - 10, SCREEN_H / 2 - 30,
         SCREEN_W / 2 + w / 2 + 10, SCREEN_H / 2 + 20, 16);
    textout_centre(screen, font, text, SCREEN_W / 2, SCREEN_H / 2 - 15, -1);
    if (wait) {
        while (!key[KEY_ENTER] && !key[KEY_ESC])
            ;
        while (key[KEY_ENTER] || key[KEY_ESC])
            ;
    }
}

/* Appends the objects and the help texts to the map file DAT_0042a440,
 * after the header and tiles written by map_save [verified]. Only caller
 * save_map (0x00403C50). */
// FUNCTION: MAPED 0x004039b0
void save_objects(void)
{
    FILE *f;

    // STRING: MAPED 0x0040f4a4
    f = fopen(DAT_0042a440, "ab");
    fwrite(DAT_00414140, sizeof(DAT_00414140), 1, f);
    fwrite(help_texts, sizeof(help_texts), 1, f);
    fclose(f);
}

/* Asks a yes/no question in a box on the screen [verified]: Y or ENTER
 * answer yes (1), N or ESC no (0); waits for the four keys to be released
 * before and after. Same as happy yes_no_box (0x00401BE0), on the screen
 * with the default font. Callers save_map (0x00403C50) and 0x00404C10. */
// FUNCTION: MAPED 0x00403a00
int yes_no_box(char *text)
{
    int w;
    int answer;

    if (text_length(font, text) < 100)
        w = 100;
    else
        w = text_length(font, text);
    rectfill(screen, SCREEN_W / 2 - w / 2 - 10, SCREEN_H / 2 - 40,
             SCREEN_W / 2 + w / 2 + 10, SCREEN_H / 2 + 30, 1);
    rect(screen, SCREEN_W / 2 - w / 2 - 10, SCREEN_H / 2 - 40,
         SCREEN_W / 2 + w / 2 + 10, SCREEN_H / 2 + 30, 16);
    textout_centre(screen, font, text, SCREEN_W / 2, SCREEN_H / 2 - 30, -1);
    // STRING: MAPED 0x0040f4a8
    textout_centre(screen, font, "Yes / No", SCREEN_W / 2, SCREEN_H / 2, -1);
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
    return answer != 2;
}

/* Saves the map to DAT_0042a440 [verified]: asks "Overwrite <name>?" if
 * the file exists ("Not saved." on no), then the header and tiles
 * (map_save), the objects and help texts, and "<name> saved". Callers
 * save_map_as (0x00403E50) and 0x00404C10. */
// FUNCTION: MAPED 0x00403c50
void save_map(void)
{
    char msg[80];

    text_mode(-1);
    if (exists(DAT_0042a440)) {
        // STRING: MAPED 0x0040f4d8
        strcpy(msg, "Overwrite ");
        strcat(msg, get_filename(DAT_0042a440));
        // STRING: MAPED 0x0040f4d4
        strcat(msg, "?");
        if (yes_no_box(msg)) {
            // STRING: MAPED 0x0040f4c8
            show_message("Saving...", 0);
            map_save(DAT_0042a440, DAT_00410110);
            save_objects();
            strcpy(msg, get_filename(DAT_0042a440));
            // STRING: MAPED 0x0040f4c0
            strcat(msg, " saved");
            show_message(msg, 1);
        } else {
            // STRING: MAPED 0x0040f4b4
            show_message("Not saved.", 1);
        }
    } else {
        map_save(DAT_0042a440, DAT_00410110);
        save_objects();
        strcpy(msg, get_filename(DAT_0042a440));
        strcat(msg, " saved");
        show_message(msg, 1);
    }
}

/* Save as [verified]: a file selector under DAT_00410340, then save_map
 * with the chosen name. Only caller 0x00404C10. */
// FUNCTION: MAPED 0x00403e50
void save_map_as(void)
{
    // STRING: MAPED 0x0040f4e4
    if (file_select("Save as... (*.MAP)", DAT_00410340, "MAP")) {
        strcpy(DAT_0042a440, DAT_00410340);
        save_map();
    }
    text_mode(-1);
}
