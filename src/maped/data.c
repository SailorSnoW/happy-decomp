/* Game globals of maped.exe, in address order.
 * [hypothesis] Defined here until the .obj that owns each one is known
 * (notes/layout-maped.md §6), as in src/happy/data.c. */
#include "maped.h"

/* [hypothesis] Read by 0x00402900 and 0x00404940; initial value -1
 * [verified]. */
// GLOBAL: MAPED 0x0040e050
int DAT_0040e050 = -1;

// GLOBAL: MAPED 0x0040e054
int DAT_0040e054 = -1;

/* [hypothesis] The menu of the editor, shown by do_menu in 0x00404C10
 * [verified: push 0x0040E058]; the items have no callback, so do_menu
 * returns the index of the chosen one. 32 entries, 7 initialised. */
// GLOBAL: MAPED 0x0040e058
MENU file_menu[32] = {
    // STRING: MAPED 0x0040ef18
    { "&New", NULL, NULL, 0, NULL },
    // STRING: MAPED 0x0040ef10
    { "&Load", NULL, NULL, 0, NULL },
    // STRING: MAPED 0x0040ef08
    { "&Save", NULL, NULL, 0, NULL },
    // STRING: MAPED 0x0040eefc
    { "Save &as", NULL, NULL, 0, NULL },
    // STRING: MAPED 0x0040eef4
    { "&Revert", NULL, NULL, 0, NULL },
    // STRING: MAPED 0x0040eeec
    { "&Quit", NULL, NULL, 0, NULL },
    { NULL, NULL, NULL, 0, NULL }
};

/* Import thunks of alleg40.lib for the dialog procedures named in the
 * initialisers below (notes/layout-maped.md §5) [verified: jmp [IAT]].
 * Annotated so that reccmp-datacmp can name the proc fields; never
 * compiled, like src/crt/. */
#if 0
// LIBRARY: MAPED 0x00405ca0
// _d_check_proc

// LIBRARY: MAPED 0x00405ca6
// _d_radio_proc

// LIBRARY: MAPED 0x00405cac
// _d_edit_proc

// LIBRARY: MAPED 0x00405cb2
// _d_text_proc

// LIBRARY: MAPED 0x00405cb8
// _d_button_proc

// LIBRARY: MAPED 0x00405cbe
// _d_ctext_proc

// LIBRARY: MAPED 0x00405cc4
// _d_shadow_box_proc
#endif

/* Map properties dialog [verified: title "MAP PROPERTIES"; used by
 * 0x004034B0]. 32 objects and the terminator. */
// GLOBAL: MAPED 0x0040e2d8
DIALOG properties_dialog[33] = {
    { d_shadow_box_proc, 0, 0, 320, 200, 6, 0, 0, 0, 0, 0, NULL, NULL, NULL },
    // STRING: MAPED 0x0040eedc
    { d_ctext_proc, 160, 4, 0, 0, 7, 0, 0, 0, 1, 1, "MAP PROPERTIES", NULL, NULL },
    // STRING: MAPED 0x0040eed8
    { d_button_proc, 240, 160, 60, 16, 7, 0, 0, 1, 1, 1, "OK", NULL, NULL },
    // STRING: MAPED 0x0040eed0
    { d_button_proc, 240, 180, 60, 16, 7, 0, 0, 1, 1, 1, "Cancel", NULL, NULL },
    // STRING: MAPED 0x0040eec8
    { d_text_proc, 10, 20, 0, 0, 7, 0, 0, 0, 1, 1, "name:", NULL, NULL },
    { d_edit_proc, 60, 20, 120, 8, 7, 0, 0, 0, 20, 0, DAT_00410740, NULL, NULL },
    // STRING: MAPED 0x0040eec0
    { d_text_proc, 10, 30, 0, 0, 7, 0, 0, 0, 1, 1, "time:", NULL, NULL },
    { d_edit_proc, 60, 30, 120, 8, 7, 0, 0, 0, 20, 0, DAT_0042a840, NULL, NULL },
    // STRING: MAPED 0x0040eeb0
    { d_text_proc, 10, 40, 0, 0, 7, 0, 0, 0, 1, 1, "happies needed:", NULL, NULL },
    { d_edit_proc, 130, 40, 120, 8, 7, 0, 0, 0, 20, 0, DAT_00410790, NULL, NULL },
    // STRING: MAPED 0x0040eea8
    { d_text_proc, 10, 60, 0, 0, 7, 0, 0, 0, 1, 1, "music:", NULL, NULL },
    // STRING: MAPED 0x0040ee98
    { d_radio_proc, 60, 60, 8, 8, 7, 0, 0, 0, 2, 1, "Happyland org.", NULL, NULL },
    // STRING: MAPED 0x0040ee90
    { d_radio_proc, 60, 70, 8, 8, 7, 0, 0, 0, 2, 1, "12 days", NULL, NULL },
    // STRING: MAPED 0x0040ee88
    { d_radio_proc, 60, 80, 8, 8, 7, 0, 0, 0, 2, 1, "Frosty", NULL, NULL },
    // STRING: MAPED 0x0040ee78
    { d_radio_proc, 60, 90, 8, 8, 7, 0, 0, 0, 2, 1, "Mom kiss Santa", NULL, NULL },
    // STRING: MAPED 0x0040ee68
    { d_radio_proc, 60, 100, 8, 8, 7, 0, 0, 0, 2, 1, "It's beginning", NULL, NULL },
    // STRING: MAPED 0x0040ee58
    { d_radio_proc, 60, 110, 8, 8, 7, 0, 0, 0, 2, 1, "Jingel Bells", NULL, NULL },
    // STRING: MAPED 0x0040ee4c
    { d_radio_proc, 200, 60, 8, 8, 7, 0, 0, 0, 2, 1, "Meriest...", NULL, NULL },
    // STRING: MAPED 0x0040ee40
    { d_radio_proc, 200, 70, 8, 8, 7, 0, 0, 0, 2, 1, "Chestnut", NULL, NULL },
    // STRING: MAPED 0x0040ee34
    { d_radio_proc, 200, 80, 8, 8, 7, 0, 0, 0, 2, 1, "Have XMas", NULL, NULL },
    // STRING: MAPED 0x0040ee28
    { d_radio_proc, 200, 90, 8, 8, 7, 0, 0, 0, 2, 1, "Let it Snow", NULL, NULL },
    // STRING: MAPED 0x0040ee1c
    { d_radio_proc, 200, 100, 8, 8, 7, 0, 0, 0, 2, 1, "JBells (60)", NULL, NULL },
    // STRING: MAPED 0x0040ee14
    { d_text_proc, 10, 130, 0, 0, 7, 0, 0, 0, 1, 1, "tiles:", NULL, NULL },
    // STRING: MAPED 0x0040ee0c
    { d_radio_proc, 80, 130, 8, 8, 7, 0, 0, 0, 1, 1, "summer", NULL, NULL },
    // STRING: MAPED 0x0040ee04
    { d_radio_proc, 160, 130, 8, 8, 7, 0, 0, 0, 1, 1, "winter", NULL, NULL },
    // STRING: MAPED 0x0040edfc
    { d_check_proc, 10, 150, 8, 8, 7, 0, 0, 0, 1, 1, "scroll", NULL, NULL },
    // STRING: MAPED 0x0040edf4
    { d_check_proc, 90, 150, 8, 8, 7, 0, 0, 0, 1, 1, "snow", NULL, NULL },
    // STRING: MAPED 0x0040edec
    { d_check_proc, 170, 150, 8, 8, 7, 0, 0, 0, 1, 1, "wind", NULL, NULL },
    // STRING: MAPED 0x0040ede8
    { d_text_proc, 10, 170, 0, 0, 7, 0, 0, 0, 1, 1, "bg:", NULL, NULL },
    // STRING: MAPED 0x0040ede0
    { d_radio_proc, 60, 170, 8, 8, 7, 0, 0, 0, 3, 1, "cloud1", NULL, NULL },
    // STRING: MAPED 0x0040edd8
    { d_radio_proc, 60, 180, 8, 8, 7, 0, 0, 0, 3, 1, "cloud2", NULL, NULL },
    // STRING: MAPED 0x0040edcc
    { d_radio_proc, 140, 170, 8, 8, 7, 0, 0, 0, 3, 1, "mountain", NULL, NULL },
    { NULL, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

/* Text entry dialog [verified: title "ENTER TEXT"; used by 0x00404380].
 * The edit field holds 256 characters. */
// GLOBAL: MAPED 0x0040ea10
DIALOG text_dialog[6] = {
    { d_shadow_box_proc, 0, 0, 320, 80, 6, 0, 0, 0, 0, 0, NULL, NULL, NULL },
    // STRING: MAPED 0x0040edc0
    { d_ctext_proc, 160, 4, 0, 0, 7, 0, 0, 0, 1, 1, "ENTER TEXT", NULL, NULL },
    { d_button_proc, 130, 40, 60, 16, 7, 0, 0, 1, 1, 1, "OK", NULL, NULL },
    // STRING: MAPED 0x0040edbc
    { d_text_proc, 10, 20, 0, 0, 7, 0, 0, 0, 1, 1, ":", NULL, NULL },
    { d_edit_proc, 30, 20, 280, 8, 7, 0, 0, 0, 256, 0, DAT_004107e0, NULL, NULL },
    { NULL, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

/* Values of the six colour sliders of colors_dialog (0-255) [verified:
 * dp3 of the sliders; slider_changed divides them by 4 into MAP.f7d-f7f
 * and f81-f83]. */
// GLOBAL: MAPED 0x0040eb60
int slider_values[6] = {255, 255, 255, 255, 255, 255};

/* Background colours dialog [verified: title "BACKGROUND COLORS"; run by
 * 0x00403150]: six sliders (FUN_004015d0, callback slider_changed), the
 * end colour first. */
// GLOBAL: MAPED 0x0040eb78
DIALOG colors_dialog[10] = {
    { d_shadow_box_proc, 0, 0, 300, 200, 6, 0, 0, 0, 0, 0, NULL, NULL, NULL },
    // STRING: MAPED 0x0040eda8
    { d_ctext_proc, 160, 4, 0, 0, 7, 0, 0, 0, 1, 1, "BACKGROUND COLORS", NULL, NULL },
    { d_button_proc, 120, 180, 80, 16, 7, 0, 0, 1, 1, 1, "OK", NULL, NULL },
    { FUN_004015d0, 12, 30, 256, 8, 1, 0, 0, 0, 255, 0, NULL, slider_changed, &slider_values[3] },
    { FUN_004015d0, 12, 50, 256, 8, 2, 0, 0, 0, 255, 0, NULL, slider_changed, &slider_values[4] },
    { FUN_004015d0, 12, 70, 256, 8, 4, 0, 0, 0, 255, 0, NULL, slider_changed, &slider_values[5] },
    { FUN_004015d0, 12, 110, 256, 8, 1, 0, 0, 0, 255, 0, NULL, slider_changed, &slider_values[0] },
    { FUN_004015d0, 12, 130, 256, 8, 2, 0, 0, 0, 255, 0, NULL, slider_changed, &slider_values[1] },
    { FUN_004015d0, 12, 150, 256, 8, 4, 0, 0, 0, 255, 0, NULL, slider_changed, &slider_values[2] },
    { NULL, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

// GLOBAL: MAPED 0x00410110
MAP *DAT_00410110;

// GLOBAL: MAPED 0x00410118
int DAT_00410118;

// GLOBAL: MAPED 0x00410340
char DAT_00410340[1024];

/* Edit buffers of properties_dialog: name (80 bytes), happies needed
 * (16 bytes) [verified: dp of the d_edit_proc entries; sizes up to the
 * next global]. */
// GLOBAL: MAPED 0x00410740
char DAT_00410740[80];

// GLOBAL: MAPED 0x00410790
char DAT_00410790[16];

// GLOBAL: MAPED 0x004107a0
int DAT_004107a0;

// GLOBAL: MAPED 0x004107a4
int DAT_004107a4;

// GLOBAL: MAPED 0x004107a8
int DAT_004107a8;

// GLOBAL: MAPED 0x004107ac
int DAT_004107ac;

// GLOBAL: MAPED 0x004107b0
int DAT_004107b0;

// GLOBAL: MAPED 0x004107b4
int DAT_004107b4;

/* Edit buffer of text_dialog, 256 bytes [verified: dp, d1 = 256]. */
// GLOBAL: MAPED 0x004107e0
char DAT_004107e0[256];

// GLOBAL: MAPED 0x004108e0
OBJ DAT_00414140[100];

/* [hypothesis] data/misc.dat (load_data), as in happy. */
// GLOBAL: MAPED 0x004113d0
DATAFILE *misc_datafile;

// GLOBAL: MAPED 0x004113d4
DATAFILE *DAT_004113d4;

// GLOBAL: MAPED 0x004113e0
char help_texts[16][HELP_TEXT_SIZE];

// GLOBAL: MAPED 0x00412400
COLOR_MAP DAT_00412400;

// GLOBAL: MAPED 0x00422408
int DAT_00422408;

// GLOBAL: MAPED 0x0042240c
int DAT_0042240c;

// GLOBAL: MAPED 0x00422410
BITMAP *DAT_00422410;

// GLOBAL: MAPED 0x00422420
RGB_MAP DAT_00422420;

// GLOBAL: MAPED 0x0042a420
int DAT_0042a420;

// GLOBAL: MAPED 0x0042a424
int DAT_0042a424;

// GLOBAL: MAPED 0x0042a440
char DAT_0042a440[1024];

/* Edit buffer of properties_dialog: time (32 bytes up to the next
 * global) [verified: dp of the d_edit_proc entry]. */
// GLOBAL: MAPED 0x0042a840
char DAT_0042a840[32];

// GLOBAL: MAPED 0x0042a860
OBJ DAT_00440060[40];

// GLOBAL: MAPED 0x0042acc4
int DAT_0042acc4;

// GLOBAL: MAPED 0x0042acc8
int DAT_0042acc8;

// GLOBAL: MAPED 0x0042ace0
PALETTE DAT_0042ace0;

// GLOBAL: MAPED 0x0042b0e0
DATAFILE *map_datafile;
