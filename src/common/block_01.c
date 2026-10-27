/* Shared utilities: happy block_01 (0x00401000-0x004018AF, first .obj of
 * .text), maped block_03 (0x004016A0-0x004018CF).
 * notes/layout-happy.md §8, notes/layout-maped.md §8.
 * [hypothesis] A single source file linked into both exes, like block_09:
 * maped keeps only FUN_00401160 and init_help_texts, which are adjacent
 * there (notes/shared-code.md). Tables, input, screen display, message box,
 * help texts. */
#include <stdlib.h>
#include <string.h>
#include <allegro.h>
#include "block_01.h"

/* Fills the table at 0x00414040 [verified]; entry 0 only gets f4 = 0.
 * [verified] The assignments are grouped by field (f0, then f8, then f4):
 * written entry by entry, the stores come out in another order (38.66%).
 * happy only; only caller 0x00401F00 (initialisation). */
// FUNCTION: HAPPY 0x00401000
void FUN_00401000(void)
{
    int i;

    for (i = 0; i < 20; i++)
        DAT_00414040[i].f4 = 0;
    DAT_00414040[1].f0 = 10;
    DAT_00414040[2].f0 = 10;
    DAT_00414040[3].f0 = 10;
    DAT_00414040[4].f0 = 10;
    DAT_00414040[5].f0 = 10;
    DAT_00414040[6].f0 = 10;
    DAT_00414040[7].f0 = 10;
    DAT_00414040[8].f0 = 10;
    DAT_00414040[9].f0 = 50;
    DAT_00414040[10].f0 = 10;
    DAT_00414040[11].f0 = 100;
    DAT_00414040[12].f0 = 150;
    DAT_00414040[13].f0 = 200;
    DAT_00414040[14].f0 = 0;
    DAT_00414040[15].f0 = 0;
    DAT_00414040[16].f0 = 0;
    DAT_00414040[17].f0 = 0;
    DAT_00414040[18].f0 = 500;
    DAT_00414040[19].f0 = 1000;
    DAT_00414040[1].f8 = 21;
    DAT_00414040[2].f8 = 21;
    DAT_00414040[3].f8 = 21;
    DAT_00414040[4].f8 = 21;
    DAT_00414040[5].f8 = 21;
    DAT_00414040[6].f8 = 21;
    DAT_00414040[7].f8 = 174;
    DAT_00414040[8].f8 = 21;
    DAT_00414040[9].f8 = 62;
    DAT_00414040[10].f8 = 21;
    DAT_00414040[11].f8 = 74;
    DAT_00414040[12].f8 = 32;
    DAT_00414040[13].f8 = 172;
    DAT_00414040[14].f8 = 21;
    DAT_00414040[15].f8 = 36;
    DAT_00414040[16].f8 = 4;
    DAT_00414040[17].f8 = 4;
    DAT_00414040[18].f8 = 64;
    DAT_00414040[19].f8 = 249;
    DAT_00414040[14].f4 = 8;
    DAT_00414040[15].f4 = 1;
    DAT_00414040[16].f4 = 4;
    DAT_00414040[17].f4 = 16;
    DAT_00414040[18].f4 = 2;
    DAT_00414040[19].f4 = 32;
}

/* Clears the 100 object slots and fills the object templates 0 to 34
 * [verified]. Templates 0-25: f13 = f14 = 'Q' + 2 * i; 26-33: f13 = 8 * i;
 * 34: f13 = f14 = 'I'. [verified] In the second loop, f14 is copied from
 * template i, not 26 + i. Callers: happy 0x00401F00, maped 0x00401C20 and
 * 0x00404C10. */
// FUNCTION: HAPPY 0x00401160
// FUNCTION: MAPED 0x004016a0
void FUN_00401160(void)
{
    int i;

    for (i = 0; i < 100; i++)
        DAT_00414140[i].f1a = 0;
    for (i = 0; i < 26; i++) {
        DAT_00440060[i].f13 = i * 2 + 'Q';
        DAT_00440060[i].f14 = i * 2 + 'Q';
        DAT_00440060[i].f16 = rand() % 10 + 20;
        DAT_00440060[i].f17 = 1;
        DAT_00440060[i].f19 = 1;
        DAT_00440060[i].f18 = i;
        DAT_00440060[i].f12 = 2;
    }
    for (i = 0; i < 8; i++) {
        DAT_00440060[26 + i].f13 = i * 8;
        DAT_00440060[i].f14 = DAT_00440060[i].f13;
        DAT_00440060[26 + i].f16 = rand() % 5 + 10;
        DAT_00440060[26 + i].f17 = 1;
        DAT_00440060[26 + i].f19 = 4;
        DAT_00440060[26 + i].f18 = i;
        DAT_00440060[26 + i].f12 = i;
        if (i > 3)
            DAT_00440060[26 + i].f12 = rand() % 5 + 10;
        if (i == 6)
            DAT_00440060[32].f12 = 2;
        else if (i == 7)
            DAT_00440060[33].f12 = 1;
    }
    DAT_00440060[30].f17 = 3;
    DAT_00440060[31].f17 = 7;
    DAT_00440060[34].f13 = 'I';
    DAT_00440060[34].f14 = 'I';
    DAT_00440060[34].f16 = 10;
    DAT_00440060[34].f17 = 0;
    DAT_00440060[34].f19 = 2;
    DAT_00440060[34].f18 = 34;
    DAT_00440060[26 + i].f12 = 0;
}

/* Copies the state of joystick 0 into keys[116] to keys[124] when
 * use_joystick is set [verified: Allegro joy[0], stick 0 and buttons 0-3].
 * [hypothesis] So that the game reads keyboard and joystick through the
 * same table. happy only; callers 0x004014F0, 0x00404C50, 0x00408B90. */
// FUNCTION: HAPPY 0x004012a0
void read_joystick(void)
{
    if (use_joystick) {
        poll_joystick();
        keys[123] = joy_up != 0;
        keys[119] = joy_left != 0;
        keys[121] = joy_right != 0;
        keys[117] = joy_down != 0;
        keys[116] = joy_b1 != 0;
        keys[122] = joy_b2 != 0;
        keys[118] = joy_b3 != 0;
        keys[124] = joy_b4 != 0;
    }
}

/* Copies Allegro's key[] into keys[0] to keys[KEY_MAX - 1] [verified].
 * happy only; only caller 0x004014F0 (and through it the whole game). */
// FUNCTION: HAPPY 0x00401350
void read_keyboard(void)
{
    int i;

    for (i = 0; i < KEY_MAX; i++)
        keys[i] = key[i];
}

/* Shows the 320x200 bitmap bmp on the screen, after vsync [verified]:
 * screen_mode 2 doubles every pixel, 1 stretches it to 640x400 between two
 * black bands of 40 lines, anything else blits it as is. 25 callers, with
 * the back buffer (0x004274E8). happy only. */
// FUNCTION: HAPPY 0x00401380
void blit_to_screen(BITMAP *bmp)
{
    int x;
    int y;
    int sx;
    int sy;

    acquire_screen();
    vsync();
    if (screen_mode == 2) {
        /* [verified] two counters per axis: separate induction variables
         * for sx/sy, and 199 x 319 pixels, not 200 x 320 */
        for (y = 0, sy = 0; y < 199; y++, sy += 2) {
            for (x = 0, sx = 0; x < 319; x++, sx += 2) {
                screen->line[sy][sx] = bmp->line[y][x];
                screen->line[sy][sx + 1] = bmp->line[y][x];
                screen->line[sy + 1][sx] = bmp->line[y][x];
                screen->line[sy + 1][sx + 1] = bmp->line[y][x];
            }
        }
    } else if (screen_mode == 1) {
        rectfill(screen, 0, 0, 640, 40, 0);
        rectfill(screen, 0, 439, 640, 480, 0);
        stretch_blit(bmp, screen, 0, 0, 320, 200, 0, 40, 640, 400);
    } else {
        blit(bmp, screen, 0, 0, 0, 0, 320, 200);
    }
    release_screen();
}

/* Draws text (help text n if n >= 0) in a translucent box on the back
 * buffer, word-wrapped at 293 pixels, then waits for the key
 * keys[controls[5]] (also SPACE or ENTER if wait is set) and for its
 * release [verified]. '@' ends the text; 0x94 is the line break, inserted
 * at the last space when a line is too long. time_left is restored on
 * exit. happy only; callers 0x00403F60, 0x00409080.
 * Shape of the code [verified against the listing]: do-while loops (no
 * test before the first character), unsigned width (jbe), strcpy in each
 * branch, three successive wait loops; line[] is written but never read. */
// FUNCTION: HAPPY 0x004014f0
void message_box(int n, char *text, int wait)
{
    char s[2];
    int lines;
    unsigned int last;
    int saved;
    char buf[256];
    char line[512];
    unsigned int w;
    unsigned int i;
    int j;
    int x;
    int y;

    saved = time_left;
    s[0] = ' ';
    s[1] = 0;
    if (n >= 0)
        strcpy(buf, help_texts[n]);
    else
        strcpy(buf, text);
    while (keys[controls[5]]) {
        read_joystick();
        read_keyboard();
    }
    i = 0;
    last = 0;
    j = 0;
    w = 0;
    lines = 1;
    do {
        s[0] = buf[i];
        w += text_length((FONT *)misc_datafile[4].dat, s);
        line[j] = s[0];
        line[j + 1] = 0;
        if (s[0] == ' ') {
            last = i;
        } else if (s[0] == '@') {
            buf[i] = 0;
        } else if (s[0] == '\x94') {
            j = -1;
            w = 0;
            lines++;
        }
        if (w > 293) {
            i = last;
            w = 0;
            j = -1;
            buf[i] = '\x94';
            lines++;
        }
        i++;
        j++;
    } while (i < strlen(buf));
    color_map = &light_table;
    drawing_mode(DRAW_MODE_TRANS, NULL, 0, 0);
    rectfill(buffer, 10, 40, 309, lines * 20 + 46, 100);
    solid_mode();
    rect(buffer, 10, 40, 309, lines * 20 + 46, 175);
    x = 16;
    y = 44;
    i = 0;
    do {
        s[0] = buf[i];
        if (s[0] != '\x94') {
            textout(buffer, (FONT *)misc_datafile[4].dat, s, x, y, -1);
            x += text_length((FONT *)misc_datafile[4].dat, s);
        } else {
            x = 16;
            y += 18;
        }
        i++;
    } while (i < strlen(buf));
    blit_to_screen(buffer);
    while (!keys[controls[5]] && !wait) {
        read_joystick();
        read_keyboard();
    }
    while (wait && !keys[controls[5]] && !key[KEY_SPACE] && !key[KEY_ENTER]) {
        read_joystick();
        read_keyboard();
    }
    while (keys[controls[5]] || key[KEY_SPACE] || key[KEY_ENTER]) {
        read_joystick();
        read_keyboard();
    }
    time_left = saved;
}

/* Default help texts [verified]: 0 "n/a", 1 to 9 the texts below, 10 to 15
 * "-". 0x00403070 then overwrites all 16 with those of the .map file.
 * [verified] Byte 256 of text 0 is set to '*'; its use is unknown.
 * Callers: happy 0x00401F00, maped 0x00401C20 and 0x00403200. */
// FUNCTION: HAPPY 0x004017c0
// FUNCTION: MAPED 0x004017e0
void init_help_texts(void)
{
    // STRING: HAPPY 0x004124a8
    // STRING: MAPED 0x0040f1ac
    strcpy(help_texts[0], "n/a");
    help_texts[0][256] = '*';
    // STRING: HAPPY 0x0041242c
    // STRING: MAPED 0x0040f130
    strcpy(help_texts[1], "The main object of each level is to bring the inhabitants of Happyland to safety. To make them follow you, just touch them.");
    // STRING: HAPPY 0x004123e8
    // STRING: MAPED 0x0040f0ec
    strcpy(help_texts[2], "All walls are not solid. Sometimes you can find secret passages.");
    // STRING: HAPPY 0x004123c8
    // STRING: MAPED 0x0040f0cc
    strcpy(help_texts[3], "Watch out for the spikes!");
    // STRING: HAPPY 0x0041237c
    // STRING: MAPED 0x0040f080
    strcpy(help_texts[4], "This is the place where you can leave the Happylanders. They are safe here.");
    // STRING: HAPPY 0x00412330
    // STRING: MAPED 0x0040f034
    strcpy(help_texts[5], "This is the level exit. To open it you must first save enough Happylanders.");
    // STRING: HAPPY 0x004122cc
    // STRING: MAPED 0x0040efd0
    strcpy(help_texts[6], "If you save every single one of the Happylanders, all enemies will be turned into to golden coins!");
    // STRING: HAPPY 0x00412290
    // STRING: MAPED 0x0040ef94
    strcpy(help_texts[7], "Collect all coins to access the bonusroom at level exit!");
    // STRING: HAPPY 0x00412248
    // STRING: MAPED 0x0040ef4c
    strcpy(help_texts[8], "The blue potion makes you untouchable. However, it has limited length.");
    // STRING: HAPPY 0x0041221c
    // STRING: MAPED 0x0040ef20
    strcpy(help_texts[9], "Green shoes will make you jump really high!");
    // STRING: HAPPY 0x004123e4
    // STRING: MAPED 0x0040f0e8
    strcpy(help_texts[10], "-");
    strcpy(help_texts[11], "-");
    strcpy(help_texts[12], "-");
    strcpy(help_texts[13], "-");
    strcpy(help_texts[14], "-");
    strcpy(help_texts[15], "-");
}
