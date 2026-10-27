/* happy block_07: 0x00407500-0x0040986F, menus, passwords, scores, game over,
 * game flow (notes/layout-happy.md §8). Functions in address order.
 * The calls to 0x00402620 (level frame) and 0x00404C50 (game loop), not
 * decompiled yet, are the only differences of show_instructions, the two
 * menus, ending_frames, play_ending and play_game. */
#include <stdlib.h>
#include <string.h>
#include <allegro.h>
#include "happy.h"

/* [hypothesis] Instructions screen [verified: "HAPPYLAND INSTRUCTIONS"
 * and the captions]: the demonstration level DAT_00414c94 runs
 * (update_objects, FUN_00402620) under captions that point at its
 * objects; the player sprite (64 of DAT_00415cc0) stands on the start
 * tile. "Password x5" only when DAT_00413b20 is 0. Escape, Space or Enter
 * leaves. The loop waits for timer_ticks without resetting it [verified:
 * no store to 0x00442084], so it only waits for the first tick. Only
 * caller main (0x0040A830). */
// FUNCTION: HAPPY 0x00407500
void show_instructions(void)
{
    clear_bitmap(screen);
    load_level(DAT_00414c94, 0, 1);
    init_level_palette();
    DAT_00442080 = 1;
    player.x = itofix(400);
    player.y = 400;
    set_palette(black_palette);
    FUN_00402620(1, 0, time_left);
    if (screen_mode == 0) {
        rectfill(screen, 0, 0, 319, 7, 0);
        // STRING: HAPPY 0x00412b20
        textout_centre(screen, (FONT *)misc_datafile[4].dat, "HAPPYLAND INSTRUCTIONS", 160, 0, -1);
        draw_sprite(screen, (BITMAP *)DAT_00415cc0[70].dat, DAT_00414130->f03 * 16, DAT_00414130->f04 * 16 + 4);
    }
    fade_in(DAT_00441c80, 8);
    clear_keybuf();
    while (!key[KEY_ESC] && !key[KEY_SPACE] && !key[KEY_ENTER]) {
        update_objects();
        FUN_00402620(0, 0, time_left);
        rectfill(buffer, 0, 0, 319, 7, 0);
        textout_centre(buffer, (FONT *)misc_datafile[4].dat, "HAPPYLAND INSTRUCTIONS", 160, 0, -1);
        // STRING: HAPPY 0x00412b10
        textout(buffer, (FONT *)misc_datafile[25].dat, "This is you.", 21, 29, 0);
        textout(buffer, (FONT *)misc_datafile[25].dat, "This is you.", 20, 28, 175);
        // STRING: HAPPY 0x00412af8
        textout(buffer, (FONT *)misc_datafile[25].dat, "Catch these fellows...", 121, 37, 0);
        textout(buffer, (FONT *)misc_datafile[25].dat, "Catch these fellows...", 120, 36, 175);
        // STRING: HAPPY 0x00412ae0
        textout(buffer, (FONT *)misc_datafile[25].dat, "...and leave them here.", 221, 29, 0);
        textout(buffer, (FONT *)misc_datafile[25].dat, "...and leave them here.", 220, 28, 175);
        // STRING: HAPPY 0x00412ac0
        textout(buffer, (FONT *)misc_datafile[25].dat, "These guys aren't your friends.", 21, 141, 0);
        textout(buffer, (FONT *)misc_datafile[25].dat, "These guys aren't your friends.", 20, 140, 175);
        // STRING: HAPPY 0x00412aac
        textout(buffer, (FONT *)misc_datafile[25].dat, "Talk to old men.", 31, 91, 0);
        textout(buffer, (FONT *)misc_datafile[25].dat, "Talk to old men.", 30, 90, 175);
        // STRING: HAPPY 0x00412a94
        textout_centre(buffer, (FONT *)misc_datafile[25].dat, "This is the level exit.", 150, 190, 0);
        textout_centre(buffer, (FONT *)misc_datafile[25].dat, "This is the level exit.", 149, 189, 175);
        // STRING: HAPPY 0x00412a78
        textout(buffer, (FONT *)misc_datafile[25].dat, "Collect stuff for points.", 121, 81, 0);
        textout(buffer, (FONT *)misc_datafile[25].dat, "Collect stuff for points.", 120, 80, 175);
        // STRING: HAPPY 0x00412a70
        textout(buffer, (FONT *)misc_datafile[25].dat, "Energy", 221, 94, 0);
        textout(buffer, (FONT *)misc_datafile[25].dat, "Energy", 220, 93, 175);
        // STRING: HAPPY 0x00412a64
        textout(buffer, (FONT *)misc_datafile[25].dat, "Low-G shoes", 221, 110, 0);
        textout(buffer, (FONT *)misc_datafile[25].dat, "Low-G shoes", 220, 109, 175);
        // STRING: HAPPY 0x00412a54
        textout(buffer, (FONT *)misc_datafile[25].dat, "Invunerability", 221, 126, 0);
        textout(buffer, (FONT *)misc_datafile[25].dat, "Invunerability", 220, 125, 175);
        // STRING: HAPPY 0x00412a48
        textout(buffer, (FONT *)misc_datafile[25].dat, "Extra time", 221, 142, 0);
        textout(buffer, (FONT *)misc_datafile[25].dat, "Extra time", 220, 141, 175);
        // STRING: HAPPY 0x00412a3c
        textout(buffer, (FONT *)misc_datafile[25].dat, "Bonusroom", 221, 158, 0);
        textout(buffer, (FONT *)misc_datafile[25].dat, "Bonusroom", 220, 157, 175);
        if (DAT_00413b20 == 0) {
            // STRING: HAPPY 0x00412a28
            textout(buffer, (FONT *)misc_datafile[25].dat, "Password        x5", 221, 174, 0);
            textout(buffer, (FONT *)misc_datafile[25].dat, "Password        x5", 220, 173, 175);
        }
        draw_sprite(buffer, (BITMAP *)DAT_00415cc0[64].dat, DAT_00414130->f03 * 16, DAT_00414130->f04 * 16 + 4);
        blit_to_screen(buffer);
        while (timer_ticks == 0)
            ;
    }
    fade_out(16);
    map_destroy(DAT_00414130);
    DAT_00414130 = NULL;
}

/* [hypothesis] Main menu [verified: "PLAY", "OPTIONS", "HELP ME", "EXIT"]
 * over the menu level DAT_00414ca0 (load_level(..., 0, 1), drawn once by
 * FUN_00402620) and the title (draw_title). Returns 1 (play), 3 (help) or
 * 2 (exit); "OPTIONS" opens options_menu. Up/down move the cursor (sprite
 * n + 81 or n + 82 of DAT_00415cc0, as in options_menu), Escape jumps to
 * "EXIT", H picks another cursor (n = 0 to 50, even). Frees the level
 * on exit. Only caller main (0x0040A830).
 * Shape [verified against the listing]: old = sel before the Space/Enter
 * test (after it, old stays in a register instead of [esp+0x20]). */
// FUNCTION: HAPPY 0x00407b30
int main_menu(void)
{
    int sel;
    int frame;
    int count;
    int ret;
    int k;
    int old;
    int n;

    sel = 0;
    frame = 0;
    count = 0;
    ret = 0;
    n = rand() % 26 * 2;
    clear_bitmap(screen);
    load_level(DAT_00414ca0, 0, 1);
    clear_objects();
    player.x = itofix(400);
    player.y = -400;
    init_level_palette();
    DAT_00442080 = 1;
    set_palette(black_palette);
    FUN_00402620(1, 0, time_left);
    blit(buffer, DAT_004274e4, 0, 0, 0, 0, 320, 200);
    draw_title();
    fade_in(DAT_00441c80, 8);
    clear_keybuf();
    do {
        timer_ticks = 0;
        if (++count == 15)
            count = 0;
        if (count == 0)
            frame++;
        if (frame == 2)
            frame = 0;
        blit(buffer, DAT_004274e4, 0, 0, 0, 0, 320, 200);
        k = 0;
        if (keypressed())
            k = readkey() >> 8;
        draw_title();
        // STRING: HAPPY 0x00412b50
        textout_centre(DAT_004274e4, (FONT *)misc_datafile[4].dat, "PLAY", 160, 70, -1);
        // STRING: HAPPY 0x00412b48
        textout_centre(DAT_004274e4, (FONT *)misc_datafile[4].dat, "OPTIONS", 160, 90, -1);
        // STRING: HAPPY 0x00412b40
        textout_centre(DAT_004274e4, (FONT *)misc_datafile[4].dat, "HELP ME", 160, 110, -1);
        // STRING: HAPPY 0x00412b38
        textout_centre(DAT_004274e4, (FONT *)misc_datafile[4].dat, "EXIT", 160, 130, -1);
        draw_sprite(DAT_004274e4, (BITMAP *)DAT_00415cc0[n + frame + 81].dat, 104, sel * 20 + 70);
        blit_to_screen(DAT_004274e4);
        old = sel;
        if (key[KEY_SPACE] || key[KEY_ENTER]) {
            if (sel == 0)
                ret = 1;
            else if (sel == 1)
                options_menu(n);
            else if (sel == 2)
                ret = 3;
            else if (sel == 3)
                ret = 2;
            while (key[KEY_SPACE] || key[KEY_ENTER])
                ;
        }
        if (k == KEY_DOWN) {
            if (sel < 3)
                sel++;
        } else if (k == KEY_UP) {
            if (sel != 0)
                sel--;
        } else if (k == KEY_ESC)
            sel = 3;
        if (sel != old)
            play_sample(misc_datafile[32].dat, DAT_00442088, 128, rand() % 800 + 600, 0);
        if (k == KEY_H)
            n = rand() % 26 * 2;
        while (timer_ticks == 0)
            ;
    } while (!ret);
    fade_out(16);
    map_destroy(DAT_00414130);
    DAT_00414130 = NULL;
    return ret;
}

/* [hypothesis] Same menu with "PASSWORD" [verified]: returns 1 (play), 4
 * (password), 3 (help) or 2 (exit). Starts from a copy of the screen
 * instead of the back buffer. Only caller main (0x0040A830); which of the
 * two menus is shown is decided there. */
// FUNCTION: HAPPY 0x00407e80
int main_menu_password(void)
{
    int sel;
    int frame;
    int count;
    int ret;
    int k;
    int old;
    int n;

    sel = 0;
    frame = 0;
    count = 0;
    ret = 0;
    n = rand() % 26 * 2;
    clear_bitmap(screen);
    load_level(DAT_00414ca0, 0, 1);
    clear_objects();
    player.x = itofix(400);
    player.y = -400;
    init_level_palette();
    DAT_00442080 = 1;
    set_palette(black_palette);
    FUN_00402620(1, 0, time_left);
    blit(screen, DAT_004274e4, 0, 0, 0, 0, 320, 200);
    draw_title();
    fade_in(DAT_00441c80, 8);
    clear_keybuf();
    do {
        timer_ticks = 0;
        if (++count == 15)
            count = 0;
        if (count == 0)
            frame++;
        if (frame == 2)
            frame = 0;
        blit(buffer, DAT_004274e4, 0, 0, 0, 0, 320, 200);
        k = 0;
        if (keypressed())
            k = readkey() >> 8;
        draw_title();
        textout_centre(DAT_004274e4, (FONT *)misc_datafile[4].dat, "PLAY", 160, 64, -1);
        // STRING: HAPPY 0x00412b58
        textout_centre(DAT_004274e4, (FONT *)misc_datafile[4].dat, "PASSWORD", 160, 82, -1);
        textout_centre(DAT_004274e4, (FONT *)misc_datafile[4].dat, "OPTIONS", 160, 100, -1);
        textout_centre(DAT_004274e4, (FONT *)misc_datafile[4].dat, "HELP ME", 160, 118, -1);
        textout_centre(DAT_004274e4, (FONT *)misc_datafile[4].dat, "EXIT", 160, 136, -1);
        draw_sprite(DAT_004274e4, (BITMAP *)DAT_00415cc0[n + frame + 81].dat, 100, sel * 18 + 62);
        blit_to_screen(DAT_004274e4);
        old = sel;
        if (key[KEY_SPACE] || key[KEY_ENTER]) {
            if (sel == 0)
                ret = 1;
            else if (sel == 1)
                ret = 4;
            else if (sel == 2)
                options_menu(n);
            else if (sel == 3)
                ret = 3;
            else if (sel == 4)
                ret = 2;
            while (key[KEY_SPACE] || key[KEY_ENTER])
                ;
        }
        if (k == KEY_DOWN) {
            if (sel < 4)
                sel++;
        } else if (k == KEY_UP) {
            if (sel != 0)
                sel--;
        } else if (k == KEY_ESC)
            sel = 4;
        if (sel != old)
            play_sample(misc_datafile[32].dat, DAT_00442088, 128, rand() % 800 + 600, 0);
        if (k == KEY_H)
            n = rand() % 26 * 2;
        while (timer_ticks == 0)
            ;
    } while (!ret);
    fade_out(16);
    map_destroy(DAT_00414130);
    DAT_00414130 = NULL;
    return ret;
}

/* [hypothesis] Asks for the name of a new high score [verified: "You
 * achieved a top score!", "Enter Your name."]: hiscores[n] gets the score
 * player.f48 and the typed name, '#' marking the cursor. Backspace erases,
 * at most 18 characters, Enter ends. Fireworks: bursts of particles at
 * random (spawn_particle). Samples 29 (start), 32 (keys), 31 (end).
 * Callers game_over, game_completed, play_ending.
 * Shape [verified against the listing]: the clamp is MID(0, 18, i) (MIN
 * tested with jg; MID(0, i, 18) gives jge). The last difference is the
 * address hiscores[0].name - 1 of name[i - 1], which reccmp cannot name
 * (like the end-of-array pointers, notes/matching.md §3). */
// FUNCTION: HAPPY 0x00408200
void enter_hiscore(int n)
{
    int i;
    int j;
    int k;
    int x;
    int y;
    int c;

    clear_bitmap(buffer);
    play_sample(misc_datafile[29].dat, DAT_00442088, 128, 1000, 0);
    // STRING: HAPPY 0x00412b7c
    textout_centre(buffer, (FONT *)misc_datafile[4].dat, "You achieved a top score!", 160, 60, -1);
    // STRING: HAPPY 0x00412b68
    textout_centre(buffer, (FONT *)misc_datafile[4].dat, "Enter Your name.", 160, 90, -1);
    // STRING: HAPPY 0x00412b64
    strcpy(hiscores[n].name, "#");
    hiscores[n].score = player.f48;
    textprintf_centre(buffer, (FONT *)misc_datafile[4].dat, 160, 130, -1, "%s", hiscores[n].name);
    blit_to_screen(buffer);
    i = 0;
    clear_keybuf();
    do {
        timer_ticks = 0;
        if (rand() % 20 == 0) {
            x = rand() % 280 + 20;
            y = rand() % 100;
            c = rand() % 255;
            for (j = rand() % 10 + 10; j > 0; j--)
                spawn_particle(x, y, c, -1);
        }
        if (keypressed()) {
            play_sample(misc_datafile[32].dat, DAT_00442088, 128, rand() % 800 + 1000, 0);
            k = readkey();
            hiscores[n].name[i] = k;
            if (k >> 8 == KEY_BACKSPACE) {
                hiscores[n].name[i] = 0;
                i--;
            } else
                i++;
            i = MID(0, 18, i);
            hiscores[n].name[i] = '#';
            hiscores[n].name[i + 1] = 0;
            if (k >> 8 == KEY_ENTER)
                hiscores[n].name[i - 1] = 0;
        }
        clear_bitmap(buffer);
        draw_particles(0, 0);
        textout_centre(buffer, (FONT *)misc_datafile[4].dat, "You achieved a top score!", 160, 60, -1);
        textout_centre(buffer, (FONT *)misc_datafile[4].dat, "Enter Your name.", 160, 90, -1);
        textprintf_centre(buffer, (FONT *)misc_datafile[4].dat, 160, 130, -1, "%s", hiscores[n].name);
        blit_to_screen(buffer);
        while (timer_ticks == 0)
            ;
    } while (k >> 8 != KEY_ENTER);
    play_sample(misc_datafile[31].dat, DAT_00442088, 128, 1000, 0);
    hiscores[n].name[i - 1] = 0;
    fade_out(4);
}

/* [hypothesis] Password screen [verified: "Enter a password.", the
 * passwords table, "HAXE-RESET"]: the typed text (at most 20 characters,
 * as in enter_hiscore; Escape returns 0) is coded (each character + 1)
 * and compared, ignoring case, with the cheats passwords[16]-[20]:
 * "HAXE-SPLITSECONDS" stops the clock (DAT_00412088 = 0),
 * "HAXE-CANTTOUCHME" sets DAT_00413b30, "HAXE-OPENSESAME" DAT_00413b34,
 * "HAXE-GOHOMEBADDIE" clears DAT_0041208c, "HAXE-FULLCAPACITY" sets
 * DAT_00412090 to 10; "HAXE-RESET" (not coded) restores the five. Then
 * with the level passwords 0-15: returns the last level that matches, or
 * 0 after a message ("No such password.", "Cheat enabled.", "All cheats
 * disabled.") and Enter. Only caller main (0x0040A830). */
// FUNCTION: HAPPY 0x00408500
int enter_password(void)
{
    int i;
    int j;
    int k;
    int msg;
    int level;
    char text[40];
    char coded[40];

    i = 0;
    msg = 0;
    blit((BITMAP *)misc_datafile[1].dat, buffer, 0, 0, 0, 0, 320, 200);
    // STRING: HAPPY 0x00412be4
    textout_centre(buffer, (FONT *)misc_datafile[4].dat, "Enter a password.", 160, 70, -1);
    // STRING: HAPPY 0x00412be0
    strcpy(text, "#");
    textprintf_centre(buffer, (FONT *)misc_datafile[4].dat, 160, 130, -1, "%s", text);
    blit_to_screen(buffer);
    fade_in(DAT_00441c80, 8);
    clear_keybuf();
    do {
        if (keypressed()) {
            play_sample(misc_datafile[32].dat, DAT_00442088, 128, rand() % 800 + 1000, 0);
            k = readkey();
            text[i] = k;
            if (k >> 8 == KEY_BACKSPACE) {
                text[i] = 0;
                i--;
            } else
                i++;
            i = MID(0, 20, i);
            text[i] = '#';
            text[i + 1] = 0;
            if (k >> 8 == KEY_ENTER)
                text[i - 1] = 0;
            if (k >> 8 == KEY_ESC) {
                fade_out(16);
                return 0;
            }
        }
        blit((BITMAP *)misc_datafile[1].dat, buffer, 0, 0, 0, 0, 320, 200);
        textout_centre(buffer, (FONT *)misc_datafile[4].dat, "Enter a password.", 160, 70, -1);
        textprintf_centre(buffer, (FONT *)misc_datafile[4].dat, 160, 130, -1, "%s", text);
        blit_to_screen(buffer);
    } while (k >> 8 != KEY_ENTER);
    j = 0;
    text[i - 1] = 0;
    for (k = 0; k < 40; k++)
        coded[k] = text[k] + 1;
    coded[i - 1] = 0;
    if (stricmp(passwords[16], coded) == 0) {
        DAT_00412088 = 0;
        msg = 1;
    }
    if (stricmp(passwords[17], coded) == 0) {
        DAT_00413b30 = 1;
        msg = 1;
    }
    if (stricmp(passwords[18], coded) == 0) {
        DAT_00413b34 = 1;
        msg = 1;
    }
    if (stricmp(passwords[19], coded) == 0) {
        DAT_0041208c = 0;
        msg = 1;
    }
    if (stricmp(passwords[20], coded) == 0) {
        DAT_00412090 = 10;
        msg = 1;
    }
    // STRING: HAPPY 0x00412bd4
    if (stricmp("HAXE-RESET", text) == 0) {
        DAT_00412088 = 1;
        DAT_00413b30 = 0;
        DAT_00413b34 = 0;
        DAT_0041208c = 1;
        DAT_00412090 = 5;
        msg = 2;
    }
    level = 0;
    for (j = 0; j < 16; j++)
        if (stricmp(passwords[j], coded) == 0)
            level = j;
    if (level == 0) {
        blit((BITMAP *)misc_datafile[1].dat, buffer, 0, 0, 0, 0, 320, 200);
        if (msg == 0)
            // STRING: HAPPY 0x00412bc0
            textout_centre(buffer, (FONT *)misc_datafile[4].dat, "No such password.", 160, 70, -1);
        else if (msg == 1)
            // STRING: HAPPY 0x00412bb0
            textout_centre(buffer, (FONT *)misc_datafile[4].dat, "Cheat enabled.", 160, 70, -1);
        else if (msg == 2)
            // STRING: HAPPY 0x00412b98
            textout_centre(buffer, (FONT *)misc_datafile[4].dat, "All cheats disabled.", 160, 70, -1);
        textprintf_centre(buffer, (FONT *)misc_datafile[4].dat, 160, 130, -1, "%s", text);
        blit_to_screen(buffer);
        while (key[KEY_ENTER])
            ;
        while (!key[KEY_ENTER])
            ;
    }
    fade_out(4);
    return level;
}

/* [hypothesis] Hall of fame [verified: "Hall of Fame", "%8ld"]: the first
 * six entries of hiscores (the seventh is only room for a shifted entry,
 * game_over), until Space or Enter is pressed. Callers game_over,
 * game_completed, play_ending. */
// FUNCTION: HAPPY 0x00408930
void show_hiscores(void)
{
    int i;

    clear_bitmap(buffer);
    // STRING: HAPPY 0x00412c00
    textout_centre(buffer, (FONT *)misc_datafile[4].dat, "Hall of Fame", 160, 20, -1);
    for (i = 0; i < 6; i++) {
        textprintf(buffer, (FONT *)misc_datafile[4].dat, 60, i * 20 + 50, -1, "%s", hiscores[i].name);
        // STRING: HAPPY 0x00412bf8
        textprintf(buffer, (FONT *)misc_datafile[4].dat, 170, i * 20 + 50, -1, "%8ld", hiscores[i].score);
    }
    blit_to_screen(buffer);
    fade_in(DAT_00441c80, 8);
    while (key[KEY_SPACE] || key[KEY_ENTER])
        ;
    while (!key[KEY_SPACE] && !key[KEY_ENTER])
        ;
    fade_out(4);
    clear_bitmap(buffer);
    clear_bitmap(screen);
}

/* [hypothesis] Game over [verified: "GAME OVER", sprite 65 of
 * DAT_00415cc0 upside down (draw_sprite_v_flip) falling from y = -32 to 210, sample 30]: then the score
 * player.f48 is inserted into hiscores (the lower entries shift down) and
 * the name asked (enter_hiscore), or a key waited for; then the hall of
 * fame. Only caller play_game. */
// FUNCTION: HAPPY 0x00408a40
void game_over(void)
{
    int y;
    int i;
    int pos;

    y = -32;
    fade_out(16);
    clear_bitmap(screen);
    fade_in(DAT_00441c80, 16);
    play_sample(misc_datafile[30].dat, DAT_00442088, 128, 1000, 0);
    do {
        timer_ticks = 0;
        clear_bitmap(buffer);
        // STRING: HAPPY 0x00412c10
        textout_centre(buffer, (FONT *)misc_datafile[4].dat, "GAME OVER", 160, 60, -1);
        draw_sprite_v_flip(buffer, (BITMAP *)DAT_00415cc0[65].dat, 152, y);
        blit_to_screen(buffer);
        y += 3;
        while (timer_ticks == 0)
            ;
    } while (y < 210);
    pos = -1;
    for (i = 5; i >= 0; i--) {
        if (player.f48 >= hiscores[i].score) {
            hiscores[i + 1] = hiscores[i];
            pos = i;
        }
    }
    if (pos >= 0)
        enter_hiscore(pos);
    else
        while (!key[KEY_SPACE] && !key[KEY_ENTER])
            ;
    fade_out(16);
    show_hiscores();
}

/* [hypothesis] End of the game [verified: "Congratulations!", "Happiness
 * achieved!", "You completed the game!" on the bitmap 3 of misc_datafile,
 * samples 29 and 31]: particles until Space or Enter, then the score is
 * inserted into hiscores as in game_over, and the hall of fame. Only
 * caller play_game. */
// FUNCTION: HAPPY 0x00408b90
void game_completed(void)
{
    int i;
    int pos;

    fade_out(16);
    blit((BITMAP *)misc_datafile[3].dat, buffer, 0, 0, 0, 0, 320, 200);
    play_sample(misc_datafile[29].dat, DAT_00442088, 128, 1000, 0);
    // STRING: HAPPY 0x00412c48
    textout_centre(buffer, (FONT *)misc_datafile[4].dat, "Congratulations!", 160, 30, -1);
    // STRING: HAPPY 0x00412c34
    textout_centre(buffer, (FONT *)misc_datafile[4].dat, "Happiness achieved!", 160, 55, -1);
    play_sample(misc_datafile[31].dat, DAT_00442088, 128, 1000, 0);
    blit_to_screen(buffer);
    fade_in(DAT_00441c80, 16);
    rest(1000);
    // STRING: HAPPY 0x00412c1c
    textout_centre(buffer, (FONT *)misc_datafile[4].dat, "You completed the game!", 160, 150, -1);
    blit_to_screen(buffer);
    while (!key[KEY_SPACE] && !key[KEY_ENTER]) {
        timer_ticks = 0;
        blit((BITMAP *)misc_datafile[3].dat, buffer, 0, 0, 0, 0, 320, 200);
        draw_particles(0, 0);
        textout_centre(buffer, (FONT *)misc_datafile[4].dat, "Congratulations!", 160, 30, -1);
        textout_centre(buffer, (FONT *)misc_datafile[4].dat, "Happiness achieved!", 160, 55, -1);
        textout_centre(buffer, (FONT *)misc_datafile[4].dat, "You completed the game!", 160, 150, -1);
        blit_to_screen(buffer);
        read_joystick();
        while (timer_ticks == 0)
            ;
    }
    pos = -1;
    for (i = 5; i >= 0; i--) {
        if (player.f48 >= hiscores[i].score) {
            hiscores[i + 1] = hiscores[i];
            pos = i;
        }
    }
    if (pos >= 0)
        enter_hiscore(pos);
    fade_out(16);
    show_hiscores();
}

/* [hypothesis] Plays `frames` frames of the ending scene [verified]: the
 * objects move (update_objects), the level is drawn (FUN_00402620), then
 * the player (sprite 64 + player.f2c of DAT_00415cc0, unless `alt`) at
 * (x1, y) and the second character (sprite 153 + DAT_00413b28) at (x2, y),
 * each mirrored when its flag is set, and sprite 156 at (130, y3); with
 * `alt`, a black rectangle, sprite 155 at (166, y) and sprite 157 at
 * (130, y3) instead. One frame per timer tick. Only caller play_ending. */
// FUNCTION: HAPPY 0x00408e00
void ending_frames(int x1, int x2, int y, int frames, int flip1, int flip2, int y3, int alt)
{
    int i;

    for (i = 0; i < frames; i++) {
        timer_ticks = 0;
        update_objects();
        FUN_00402620(0, 0, time_left);
        if (!alt) {
            if (flip1)
                draw_sprite_h_flip(buffer, (BITMAP *)DAT_00415cc0[player.f2c + 64].dat, x1, y);
            else
                draw_sprite(buffer, (BITMAP *)DAT_00415cc0[player.f2c + 64].dat, x1, y);
        }
        if (flip2)
            draw_sprite_h_flip(buffer, (BITMAP *)DAT_00415cc0[DAT_00413b28 + 153].dat, x2, y);
        else
            draw_sprite(buffer, (BITMAP *)DAT_00415cc0[DAT_00413b28 + 153].dat, x2, y);
        if (!alt)
            draw_sprite(buffer, (BITMAP *)DAT_00415cc0[156].dat, 130, y3);
        else {
            rectfill(buffer, 166, 110, 185, 142, 0);
            draw_sprite(buffer, (BITMAP *)DAT_00415cc0[155].dat, 166, y);
            draw_sprite(buffer, (BITMAP *)DAT_00415cc0[157].dat, 130, y3);
        }
        blit_to_screen(buffer);
        while (timer_ticks == 0)
            ;
    }
}

/* [hypothesis] Two animation counters (2 frames, 8 calls per frame), each
 * advanced when its argument is non-zero: the player's (player.f2c) and
 * the second character's of the ending (DAT_00413b28). Only caller:
 * play_ending. */
// FUNCTION: HAPPY 0x00409010
void anim_tick(int a, int b)
{
    if (a != 0) {
        player.f30++;
        if (player.f30 == 8) {
            player.f30 = 0;
            player.f2c++;
            if (player.f2c > 1)
                player.f2c = 0;
        }

    }
    if (b != 0) {
        DAT_00413b24++;
        if (DAT_00413b24 == 8) {
            DAT_00413b24 = 0;
            DAT_00413b28++;
            if (DAT_00413b28 > 1)
                DAT_00413b28 = 0;
        }
    }
}

/* [hypothesis] Ending cutscene [verified: the dialogue texts]: the custom
 * level DAT_00414ca4 is loaded, the player walks in from the start tile
 * (MAP.f03, f04) to x = 100 while the second character (sprite 153 +
 * DAT_00413b28) comes from x = 270; dialogue (message_box), the second
 * character walks away and comes back, a third sprite falls from the top
 * (y3 from -65), samples 28, 30, 31, 29; then the score goes into hiscores
 * as in game_over, the hall of fame, and DAT_00413b2c = 1. Music 21 of
 * misc_datafile; Escape during the first two seconds skips it all. t
 * alternates so that the characters move every other frame. Only caller
 * play_game.
 * Shape [verified against the listing]: t = 0 just before the first loop
 * keeps t in ebx for the whole function (initialised at the top, it goes
 * to the stack and ebx holds the constant 0). */
// FUNCTION: HAPPY 0x00409080
void play_ending(void)
{
    int t;
    int x1;
    int x2;
    int y;
    int y3;
    int i;
    int pos;

    clear_bitmap(screen);
    play_midi(misc_datafile[21].dat, 1);
    if (wait_or_esc(2000))
        return;
    load_level(DAT_00414ca4, 0, 1);
    init_level_palette();
    DAT_00442080 = 1;
    player.f2c = 0;
    DAT_00413b28 = 0;
    player.x = itofix(400);
    player.y = 400;
    set_palette(black_palette);
    FUN_00402620(1, 0, time_left);
    x1 = DAT_00414130->f03 * 16;
    y = DAT_00414130->f04 * 16 - 4;
    x2 = 270;
    draw_sprite(buffer, (BITMAP *)DAT_00415cc0[64].dat, x1, y);
    draw_sprite_h_flip(buffer, (BITMAP *)DAT_00415cc0[153].dat, 270, y);
    blit_to_screen(buffer);
    fade_in(DAT_00441c80, 8);
    clear_keybuf();
    t = 0;
    while (x1 < 100) {
        t++;
        if (t & 1) {
            x1++;
            x2--;
        }
        anim_tick(1, 1);
        ending_frames(x1, x2, y, 1, 0, 1, -100, 0);
    }
    // STRING: HAPPY 0x00412dcc
    message_box(-1, "Thanks for your help, really good work!", 1);
    ending_frames(x1, x2, y, 30, 0, 1, -100, 0);
    // STRING: HAPPY 0x00412d80
    message_box(-1, "You have saved Christmas! That is not a bad thing. Really, you were great!", 1);
    ending_frames(x1, x2, y, 20, 0, 1, -100, 0);
    ending_frames(x1, x2, y, 40, 0, 0, -100, 0);
    ending_frames(x1, x2, y, 20, 0, 1, -100, 0);
    ending_frames(x1, x2, y, 40, 0, 0, -100, 0);
    ending_frames(x1, x2, y, 20, 0, 1, -100, 0);
    ending_frames(x1, x2, y, 40, 0, 0, -100, 0);
    ending_frames(x1, x2, y, 10, 0, 1, -100, 0);
    // STRING: HAPPY 0x00412d50
    message_box(-1, "Well. I guess we are done then. Thanks again.", 1);
    ending_frames(x1, x2, y, 20, 0, 1, -100, 0);
    ending_frames(x1, x2, y, 40, 0, 0, -100, 0);
    ending_frames(x1, x2, y, 50, 0, 1, -100, 0);
    while (x2 < 200) {
        t++;
        if (t & 1)
            x2++;
        anim_tick(0, 1);
        ending_frames(x1, x2, y, 1, 0, 0, -100, 0);
    }
    // STRING: HAPPY 0x00412d18
    message_box(-1, "Wait! Am I not supposed to get a reward or something?.", 1);
    ending_frames(x1, x2, y, 10, 0, 0, -100, 0);
    ending_frames(x1, x2, y, 40, 0, 1, -100, 0);
    // STRING: HAPPY 0x00412cd4
    message_box(-1, "Supposed? Yeah, maybe you should have something for your trouble.", 1);
    ending_frames(x1, x2, y, 20, 0, 1, -100, 0);
    ending_frames(x1, x2, y, 40, 0, 0, -100, 0);
    ending_frames(x1, x2, y, 20, 0, 1, -100, 0);
    // STRING: HAPPY 0x00412ca8
    message_box(-1, "I might think of something. See you later.", 1);
    ending_frames(x1, x2, y, 10, 0, 1, -100, 0);
    while (x2 < 220) {
        t++;
        if (t & 1)
            x2++;
        anim_tick(0, 1);
        ending_frames(x1, x2, y, 1, 0, 0, -100, 0);
    }
    while (x2 < 280) {
        t++;
        if (t & 1) {
            x2++;
            x1++;
        }
        anim_tick(1, 1);
        ending_frames(x1, x2, y, 1, 0, 0, -100, 0);
    }
    player.f2c = 0;
    while (x2 < 320) {
        t++;
        if (t & 1)
            x2++;
        anim_tick(0, 1);
        ending_frames(x1, x2, y, 1, 0, 0, -100, 0);
    }
    ending_frames(x1, x2, y, 20, 1, 1, -100, 0);
    ending_frames(x1, x2, y, 20, 0, 1, -100, 0);
    ending_frames(x1, x2, y, 20, 1, 1, -100, 0);
    y3 = -65;
    do {
        y3 += 4;
        anim_tick(0, 1);
        ending_frames(x1, x2, y, 1, 0, 0, y3, 0);
    } while (y3 < 91);
    play_sample(misc_datafile[28].dat, DAT_00442088, 128, 1000, 0);
    ending_frames(x1, x2, y, 120, 1, 1, y3, 0);
    play_sample(misc_datafile[30].dat, DAT_00442088, 128, 1000, 0);
    ending_frames(x1, x2, y, 120, 1, 1, y3, 1);
    // STRING: HAPPY 0x00412c5c
    message_box(-1, "      Congratulations!\x94   You completed the game!\x94      Merry Christmas!", 1);
    play_sample(misc_datafile[31].dat, DAT_00442088, 128, 1000, 0);
    ending_frames(x1, x2, y, 60, 1, 1, y3, 1);
    fade_out(2);
    play_sample(misc_datafile[29].dat, DAT_00442088, 128, 1000, 0);
    map_destroy(DAT_00414130);
    DAT_00414130 = NULL;
    clear_bitmap(screen);
    fade_in(DAT_00441c80, 8);
    pos = -1;
    for (i = 5; i >= 0; i--) {
        if (player.f48 >= hiscores[i].score) {
            hiscores[i + 1] = hiscores[i];
            pos = i;
        }
    }
    if (pos >= 0)
        enter_hiscore(pos);
    fade_out(16);
    show_hiscores();
    DAT_00413b2c = 1;
}

/* [hypothesis] Plays a game [verified: the calls]: one pass of the outer
 * loop per continue (player.f5c), one pass of the inner loop per life
 * (player.f40). Each pass starts the current level DAT_00440038 (1-based;
 * level DAT_0042f568 + n - 1 of the list, or n - 1 when DAT_00413b20 is
 * 2) and plays it (0x00404C50): 1 = level completed (next level,
 * FUN_00404920, FUN_00404770 when player.f60 reaches 5, the bonus room if
 * player.f58 is set), 2 = life lost (game_over at the last one, otherwise
 * the level restarts with keep = 1), 3 = quit. When every bonus item of a
 * level was taken (player.f50 == DAT_00440048), the next pass plays a
 * random level among the first DAT_0042f568 (`secret`). Past the last
 * level (DAT_004450a8), the ending: play_ending, or game_completed when
 * DAT_00413b20 is not 0. "Continue?" between the continues (score reset,
 * 2 lives). Only caller main (0x0040A830).
 * Shape [verified against the listing]: the result of 0x00404C50 goes
 * into i, the loop counter (both in esi): with a separate variable, VC6
 * knows it is 1 when setting `secret` and writes "mov ebp, 1" instead of
 * "mov ebp, esi". The start_level argument is one ?: (the if/else pushes
 * the other arguments first); game_completed is tested first. */
// FUNCTION: HAPPY 0x00409640
void play_game(void)
{
    int keep;
    int secret;
    int quit;
    int i;

    keep = 0;
    FUN_00403030();
    secret = 0;
    quit = 0;
    while (player.f5c != 0 && !quit) {
        player.f5c--;
        while (player.f40 != 0 && !quit) {
            DAT_00442080 = 0;
            scroll_active = 0;
            if (secret) {
                DAT_00442080 = 1;
                start_level(rand() % DAT_0042f568, 0, 0);
            } else {
                for (i = 1; i <= DAT_004450a8; i++) {
                    if (i == DAT_00440038) {
                        start_level(DAT_00413b20 != 2 ? DAT_0042f568 + i - 1 : i - 1, keep, 0);
                        break;
                    }
                }
                if (i > DAT_004450a8) {
                    if (DAT_00413b20 != 0)
                        game_completed();
                    else
                        play_ending();
                    quit = 99;
                }
            }
            if (DAT_00440038 <= DAT_004450a8 || secret) {
                i = FUN_00404c50();
                if (i == 1) {
                    if (!DAT_00442080) {
                        FUN_00404920();
                        DAT_00440038++;
                        keep = 0;
                        if (player.f60 == 5)
                            FUN_00404770();
                    }
                    if (player.f58) {
                        DAT_00442080 = 1;
                        i = bonus_room();
                    }
                }
                secret = i == 1 && DAT_00440048 && player.f50 == DAT_00440048;
                if (i == 2) {
                    if (player.f40 == 0)
                        game_over();
                    else
                        keep = 1;
                } else if (i == 3)
                    quit = 1;
                fade_out(16);
            }
        }
        if (player.f5c != 0 && !quit) {
            clear_bitmap(screen);
            clear_bitmap(buffer);
            set_palette(DAT_00441c80);
            // STRING: HAPPY 0x00412df4
            if (yes_no_box("Continue?")) {
                player.f48 = 0;
                player.f64 = 0;
                player.f40 = 2;
                keep = 0;
            }
        }
    }
}
