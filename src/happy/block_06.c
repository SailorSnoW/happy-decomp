/* happy block_06: 0x004063A0-0x004074FF, title screen, controls, options,
 * introduction (notes/layout-happy.md §8). Functions in address order. */
#include <stdlib.h>
#include <allegro.h>
#include "happy.h"

/* [hypothesis] Draws the title and the credits on the menu background
 * DAT_004274e4 [verified]: the logo sprites 35 and 36 of misc_datafile
 * (only 35 when DAT_00413b20 is 2, the text "* HAPPYLAND ADVENTURES *"
 * when it is 1), "written by Johan Peitz (c) 2000", the SpriteLib licence
 * number and "v1.3", each with a shadow. Callers: the four menus
 * 0x00406560, 0x00406850, 0x00407B30, 0x00407E80. */
// FUNCTION: HAPPY 0x004063a0
void draw_title(void)
{
    if (DAT_00413b20 == 0) {
        draw_sprite(DAT_004274e4, (BITMAP *)misc_datafile[35].dat, 81, 4);
        draw_sprite(DAT_004274e4, (BITMAP *)misc_datafile[36].dat, 180, 40);
    }
    if (DAT_00413b20 == 1)
        // STRING: HAPPY 0x00412750
        textout_centre(DAT_004274e4, (FONT *)misc_datafile[4].dat, "* HAPPYLAND ADVENTURES *", 160, 30, -1);
    if (DAT_00413b20 == 2)
        draw_sprite(DAT_004274e4, (BITMAP *)misc_datafile[35].dat, 81, 4);
    // STRING: HAPPY 0x00412730
    textout_centre(DAT_004274e4, (FONT *)misc_datafile[4].dat, "written by Johan Peitz (c) 2000", 161, 166, 0);
    textout_centre(DAT_004274e4, (FONT *)misc_datafile[4].dat, "written by Johan Peitz (c) 2000", 160, 165, -1);
    // STRING: HAPPY 0x004126fc
    textout_centre(DAT_004274e4, (FONT *)misc_datafile[25].dat, "SpriteLib license number: 129.16.30.119-952518357", 161, 194, 0);
    textout_centre(DAT_004274e4, (FONT *)misc_datafile[25].dat, "SpriteLib license number: 129.16.30.119-952518357", 160, 193, 175);
    // STRING: HAPPY 0x004126f4
    textout(DAT_004274e4, (FONT *)misc_datafile[25].dat, "v1.3", 2, 2, 0);
    textout(DAT_004274e4, (FONT *)misc_datafile[25].dat, "v1.3", 1, 1, 175);
}

/* [hypothesis] Asks for the keys of the five actions, in this order
 * [verified]: left (controls[1]), right (controls[0]), jump (controls[4]),
 * slide (controls[6]), talk (controls[5]). Any key of key[] is accepted,
 * as are the joystick directions and buttons 1-4 (the codes 116-124 that
 * read_joystick writes into keys[]), except a key already given to another
 * action or to controls[2] or controls[3]. Sample 32 after each key. Only
 * caller options_menu ("set controls"). */
// FUNCTION: HAPPY 0x00406560
void set_controls(void)
{
    char *names[5] = {
        // STRING: HAPPY 0x004127c0
        "left",
        // STRING: HAPPY 0x004127b8
        "right",
        // STRING: HAPPY 0x004127b0
        "jump",
        // STRING: HAPPY 0x004127a8
        "slide",
        // STRING: HAPPY 0x004127a0
        "talk"
    };
    int i;
    int j;
    int k;

    controls[1] = 255;
    controls[0] = 255;
    controls[4] = 255;
    controls[6] = 255;
    controls[5] = 255;
    while (key[KEY_SPACE] || key[KEY_ENTER])
        ;
    for (i = 0; i < 5; i++) {
        clear_keybuf();
        blit(buffer, DAT_004274e4, 0, 0, 0, 0, 320, 200);
        draw_title();
        // STRING: HAPPY 0x00412794
        textout_centre(DAT_004274e4, (FONT *)misc_datafile[4].dat, "Select for", 160, 90, -1);
        textprintf_centre(DAT_004274e4, (FONT *)misc_datafile[4].dat, 160, 110, -1, "%s", names[i]);
        if (use_joystick) {
            // STRING: HAPPY 0x0041276c
            textout_centre(DAT_004274e4, (FONT *)misc_datafile[25].dat, "You can use your gamepad if you want.", 161, 141, 0);
            textout_centre(DAT_004274e4, (FONT *)misc_datafile[25].dat, "You can use your gamepad if you want.", 160, 140, 175);
        }
        blit_to_screen(DAT_004274e4);
        k = 0;
        do {
            while (k == 0) {
                for (j = 0; j < KEY_MAX; j++)
                    if (key[j])
                        k = j;
                if (use_joystick) {
                    poll_joystick();
                    if (joy_up)
                        k = 123;
                    if (joy_right)
                        k = 121;
                    if (joy_down)
                        k = 117;
                    if (joy_left)
                        k = 119;
                    if (joy_b1)
                        k = 116;
                    if (joy_b2)
                        k = 122;
                    if (joy_b3)
                        k = 118;
                    if (joy_b4)
                        k = 124;
                }
            }
            if (k == controls[1] || k == controls[0])
                k = 0;
            if (k == controls[2] || k == controls[3])
                k = 0;
            if (k == controls[5] || k == controls[6] || k == controls[4])
                k = 0;
        } while (k == 0);
        switch (i) {
        case 0:
            controls[1] = k;
            break;
        case 1:
            controls[0] = k;
            break;
        case 2:
            controls[4] = k;
            break;
        case 3:
            controls[6] = k;
            break;
        case 4:
            controls[5] = k;
            break;
        }
        play_sample(misc_datafile[32].dat, DAT_00442088, 128, rand() % 800 + 600, 0);
    }
}

/* [hypothesis] Options menu [verified]: music volume (midi_volume), sound
 * volume (DAT_00442088), "set controls" (set_controls), "main menu".
 * Up/down move the cursor (sample 32), left/right change a volume by 5
 * within 0-255 and apply the MIDI one; Space or Enter on "main menu"
 * leaves. The cursor is sprite n + 81 or n + 82 of DAT_00415cc0,
 * alternating every 15 frames; one frame per timer tick. Callers
 * 0x00407B30, 0x00407E80. */
// FUNCTION: HAPPY 0x00406850
void options_menu(int n)
{
    int done;
    int frame;
    int count;
    int sel;
    int k;
    int old;

    done = 0;
    frame = 0;
    count = 0;
    sel = 3;
    clear_keybuf();
    while (key[KEY_SPACE] || key[KEY_ENTER])
        ;
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
        // STRING: HAPPY 0x004127f8
        textprintf_centre(DAT_004274e4, (FONT *)misc_datafile[4].dat, 160, 70, -1, "music volume: %d", midi_volume);
        // STRING: HAPPY 0x004127e4
        textprintf_centre(DAT_004274e4, (FONT *)misc_datafile[4].dat, 160, 90, -1, "sound volume: %d", DAT_00442088);
        // STRING: HAPPY 0x004127d4
        textout_centre(DAT_004274e4, (FONT *)misc_datafile[4].dat, "set controls", 160, 110, -1);
        // STRING: HAPPY 0x004127c8
        textout_centre(DAT_004274e4, (FONT *)misc_datafile[4].dat, "main menu", 160, 130, -1);
        draw_sprite(DAT_004274e4, (BITMAP *)DAT_00415cc0[n + frame + 81].dat, 60, sel * 20 + 70);
        blit_to_screen(DAT_004274e4);
        if (key[KEY_SPACE] || key[KEY_ENTER]) {
            if (sel == 3)
                done = sel;
            else if (sel == 2)
                set_controls();
            while (key[KEY_SPACE] || key[KEY_ENTER])
                ;
        }
        if (key[KEY_LEFT]) {
            if (sel == 1)
                DAT_00442088 = MAX(DAT_00442088 - 5, 0);
            else if (sel == 0)
                midi_volume = MAX(midi_volume - 5, 0);
            if (sel < 2)
                set_volume(-1, midi_volume);
        }
        if (key[KEY_RIGHT]) {
            if (sel == 1)
                DAT_00442088 = MIN(DAT_00442088 + 5, 255);
            else if (sel == 0)
                midi_volume = MIN(midi_volume + 5, 255);
            if (sel < 2)
                set_volume(-1, midi_volume);
        }
        old = sel;
        if (k == KEY_DOWN) {
            if (sel < 3)
                sel++;
        } else if (k == KEY_UP && sel != 0)
            sel--;
        if (sel != old)
            play_sample(misc_datafile[32].dat, DAT_00442088, 128, rand() % 800 + 600, 0);
        while (timer_ticks == 0)
            ;
    } while (!done);
}

/* [hypothesis] Frees every object slot of DAT_00414140 (f1a = 0)
 * [verified]. Callers 0x00407B30, 0x00407E80. */
// FUNCTION: HAPPY 0x00406b80
void clear_objects(void)
{
    int i;

    for (i = 0; i < 100; i++)
        DAT_00414140[i].f1a = 0;
}

/* [hypothesis] Publisher screen [verified]: "Free Lunch Design" /
 * "presents", sample 27 at a random frequency, fade in, 2 seconds, fade
 * out. Only caller main (0x0040A830). */
// FUNCTION: HAPPY 0x00406ba0
void show_logo(void)
{
    set_palette(black_palette);
    clear_bitmap(buffer);
    clear_bitmap(screen);
    // STRING: HAPPY 0x00412818
    textout_centre(buffer, (FONT *)misc_datafile[4].dat, "Free Lunch Design", 160, 80, -1);
    // STRING: HAPPY 0x0041280c
    textout_centre(buffer, (FONT *)misc_datafile[4].dat, "presents", 160, 105, -1);
    blit_to_screen(buffer);
    play_sample(misc_datafile[27].dat, DAT_00442088, 128, rand() % 400 + 800, 0);
    fade_in((RGB *)map_datafile[0].dat, 8);
    rest(2000);
    fade_out(8);
}

/* [hypothesis] Waits ms milliseconds (ms / 15 timer ticks), or less if
 * Escape is pressed; returns 1 in that case. decomp.me: 100%
 * (notes/decompme-candidates.md §10). */
// FUNCTION: HAPPY 0x00406c80
int wait_or_esc(int ms)
{
    int n;

    n = ms / 15;
    timer_ticks = 0;
    while (timer_ticks < n && !key[KEY_ESC])
        ;
    return key[KEY_ESC] != 0;
}

/* [hypothesis] One caption of the introduction [verified]: text centred
 * on a black screen, fade in, wait_or_esc(ms), fade out; returns the
 * result of wait_or_esc (1 if Escape was pressed). Only caller
 * play_intro. */
// FUNCTION: HAPPY 0x00406cd0
int intro_caption(char *text, int ms)
{
    int esc;

    set_palette(black_palette);
    clear_bitmap(buffer);
    textout_centre(buffer, (FONT *)misc_datafile[4].dat, text, 160, 80, -1);
    blit_to_screen(buffer);
    fade_in((RGB *)map_datafile[0].dat, 8);
    esc = wait_or_esc(ms);
    fade_out(8);
    clear_bitmap(screen);
    return esc;
}

/* [hypothesis] Introduction [verified: the texts]: captions, a telephone
 * ringing (sample 27, "BRRRING!" blinking 20 + 20 + 7 times), Santa's call
 * (RLE sprites 7 and 24 of misc_datafile sliding in), then the closing
 * captions. Music 21 of misc_datafile. Escape stops it at any wait. Only
 * caller main (0x0040A830). */
// FUNCTION: HAPPY 0x00406d60
void play_intro(void)
{
    int i;

    set_palette(black_palette);
    clear_bitmap(screen);
    play_midi(misc_datafile[21].dat, 1);
    if (wait_or_esc(2000))
        return;
    // STRING: HAPPY 0x00412a04
    if (intro_caption("It's the night before Christmas...", 3000))
        return;
    // STRING: HAPPY 0x004129e4
    if (intro_caption("...and everyone is sleeping...", 3000))
        return;
    // STRING: HAPPY 0x004129d4
    if (intro_caption("...almost...", 2000))
        return;
    if (wait_or_esc(500))
        return;
    clear_bitmap(screen);
    clear_bitmap(buffer);
    // STRING: HAPPY 0x004129c8
    textout_centre(buffer, (FONT *)misc_datafile[4].dat, "BRRRING!", 160, 80, -1);
    set_palette((RGB *)map_datafile[0].dat);
    for (i = 0; i < 20; i++) {
        play_sample(misc_datafile[27].dat, DAT_00442088, 128, 1500, 0);
        blit_to_screen(buffer);
        if (wait_or_esc(30))
            return;
        clear_bitmap(screen);
        if (wait_or_esc(30))
            return;
    }
    if (wait_or_esc(1000))
        return;
    for (i = 0; i < 20; i++) {
        play_sample(misc_datafile[27].dat, DAT_00442088, 128, 1500, 0);
        blit_to_screen(buffer);
        if (wait_or_esc(30))
            return;
        clear_bitmap(screen);
        if (wait_or_esc(30))
            return;
    }
    if (wait_or_esc(1000))
        return;
    for (i = 0; i < 7; i++) {
        play_sample(misc_datafile[27].dat, DAT_00442088, 128, 1500, 0);
        blit_to_screen(buffer);
        if (wait_or_esc(30))
            return;
        clear_bitmap(screen);
        if (wait_or_esc(30))
            return;
    }
    for (i = -100; i < 50; i++) {
        timer_ticks = 0;
        clear_bitmap(buffer);
        draw_rle_sprite(buffer, (RLE_SPRITE *)misc_datafile[7].dat, i, 60);
        blit_to_screen(buffer);
        while (timer_ticks == 0)
            ;
        if (key[KEY_ESC])
            return;
    }
    // STRING: HAPPY 0x004129b0
    textout(buffer, (FONT *)misc_datafile[4].dat, "What? Why? Who is this?", 10, 10, -1);
    blit_to_screen(buffer);
    if (wait_or_esc(1000))
        return;
    for (i = 320; i > 200; i--) {
        timer_ticks = 0;
        rectfill(buffer, i, 60, 320, 199, 0);
        draw_rle_sprite(buffer, (RLE_SPRITE *)misc_datafile[24].dat, i, 60);
        blit_to_screen(buffer);
        while (timer_ticks == 0)
            ;
        if (key[KEY_ESC])
            return;
    }
    // STRING: HAPPY 0x004129a0
    textout(buffer, (FONT *)misc_datafile[4].dat, "Hi! It's Santa.", 140, 150, -1);
    // STRING: HAPPY 0x0041298c
    textout(buffer, (FONT *)misc_datafile[4].dat, "I need your help!", 140, 170, -1);
    blit_to_screen(buffer);
    if (wait_or_esc(1000))
        return;
    rectfill(buffer, 0, 0, 320, 40, 0);
    blit_to_screen(buffer);
    if (wait_or_esc(2000))
        return;
    // STRING: HAPPY 0x00412970
    textout(buffer, (FONT *)misc_datafile[4].dat, "Sure, what is it all about?", 10, 10, -1);
    blit_to_screen(buffer);
    if (wait_or_esc(1000))
        return;
    rectfill(buffer, 0, 150, 320, 200, 0);
    blit_to_screen(buffer);
    if (wait_or_esc(2000))
        return;
    // STRING: HAPPY 0x0041294c
    textout(buffer, (FONT *)misc_datafile[4].dat, "My helpers have all disappeared!", 10, 150, -1);
    blit_to_screen(buffer);
    if (wait_or_esc(1000))
        return;
    // STRING: HAPPY 0x0041292c
    textout(buffer, (FONT *)misc_datafile[4].dat, "You have to help me find them!", 10, 170, -1);
    blit_to_screen(buffer);
    if (wait_or_esc(2000))
        return;
    rectfill(buffer, 0, 0, 320, 40, 0);
    blit_to_screen(buffer);
    if (wait_or_esc(2000))
        return;
    rectfill(buffer, 0, 150, 320, 200, 0);
    // STRING: HAPPY 0x00412908
    textout(buffer, (FONT *)misc_datafile[4].dat, "Otherwise there will be no Xmas...", 10, 150, -1);
    blit_to_screen(buffer);
    if (wait_or_esc(2000))
        return;
    // STRING: HAPPY 0x004128f8
    textout(buffer, (FONT *)misc_datafile[4].dat, "       Ever!", 40, 170, -1);
    blit_to_screen(buffer);
    if (wait_or_esc(1000))
        return;
    // STRING: HAPPY 0x004128d8
    textout(buffer, (FONT *)misc_datafile[4].dat, "Sure, I will find them for you.", 10, 10, -1);
    blit_to_screen(buffer);
    if (wait_or_esc(1000))
        return;
    rectfill(buffer, 0, 150, 320, 200, 0);
    blit_to_screen(buffer);
    if (wait_or_esc(2000))
        return;
    // STRING: HAPPY 0x004128c4
    textout(buffer, (FONT *)misc_datafile[4].dat, "Phew, thanks pal!", 80, 150, -1);
    // STRING: HAPPY 0x004128a8
    textout(buffer, (FONT *)misc_datafile[4].dat, "There is still a chance!", 80, 170, -1);
    blit_to_screen(buffer);
    if (wait_or_esc(1000))
        return;
    rectfill(buffer, 0, 0, 320, 40, 0);
    blit_to_screen(buffer);
    if (wait_or_esc(2000))
        return;
    rectfill(buffer, 0, 150, 320, 200, 0);
    blit_to_screen(buffer);
    fade_out(4);
    if (wait_or_esc(1000))
        return;
    // STRING: HAPPY 0x00412898
    if (intro_caption("Once again...", 2000))
        return;
    // STRING: HAPPY 0x00412870
    if (intro_caption("...you are put upon an epic quest...", 3000))
        return;
    // STRING: HAPPY 0x00412850
    if (intro_caption("Help Santa find the helpers!", 3000))
        return;
    // STRING: HAPPY 0x0041282c
    intro_caption("Or Christmas will be lost forever!", 3000);
}
