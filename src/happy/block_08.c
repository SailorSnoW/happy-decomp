/* happy block_08: 0x00409870-0x0040A99F, start menu, custom game, main,
 * WinMain (notes/layout-happy.md §8). Functions in address order.
 * 0x00409870 (start menu) and 0x00409F30 (custom game), among the large
 * functions left for the end, are not decompiled yet: the calls to them
 * are one of the two differences of main. */
#include <string.h>
#include <allegro.h>
#include "happy.h"

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

/* The game's main(), called by Allegro's _WinMain [verified: push
 * 0x0040A830 in WinMain]. Options, compared ignoring case: "-widescreen"
 * sets screen_mode to 1, "-fps" sets DAT_00413b10. Neither has an effect
 * [verified in the code]: init_game, called next, always rewrites
 * screen_mode (load_settings or default_settings), and nothing reads
 * DAT_00413b10. Then the start menu 0x00409870 (negative: quit); when
 * DAT_00413b20 is 2, the custom game screen 0x00409F30, back to the start
 * menu when it returns 0. Introduction: play_intro for DAT_00413b20 = 0,
 * otherwise show_logo and music 19. Menu loop: main_menu
 * (DAT_00413b20 != 0) or main_menu_password; 1 plays from level 1, 4 asks
 * for a password (enter_password, 0 = back to the menu) and starts at the
 * level after the one returned [hypothesis: DAT_004450b0 is the first
 * level, copied into DAT_00440038 by FUN_00403030]; after play_game,
 * music 21 (unless DAT_00413b2c, set by play_ending) or 19. 3 shows the
 * instructions, 2 quits [verified].
 * Below 100%: VC6 rotates the start menu loop, the original does not
 * (notes/block08-diffs.md); every shape tried gives the same code. */
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
        if (FUN_00409870() < 0)
            goto quit;
    } while (DAT_00413b20 == 2 && FUN_00409f30() == 0);
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
