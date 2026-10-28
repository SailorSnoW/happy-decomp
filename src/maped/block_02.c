/* Background gradient and its sliders: maped block_02
 * (0x00401400-0x0040169F). notes/layout-maped.md §5. The palette entries
 * of the gradient, the callback of the colour sliders and the dialog
 * procedure that redraws a slider when its value changes. maped only. */
#include <allegro.h>
#include "maped.h"

/* Rebuilds colours 176-191 of the editor palette as a gradient from
 * MAP.f7d-f7f to f81-f83 (16 steps, exclusive of the start) and sets the
 * palette [verified]. Same gradient as happy init_level_palette
 * (0x00402EA0), without the background image. 5 callers. */
// FUNCTION: MAPED 0x00401400
void FUN_00401400(void)
{
    int i;
    float r;
    float g;
    float b;
    float dr;
    float dg;
    float db;

    dr = (DAT_00410110->f81 - DAT_00410110->f7d) * 0.0625;
    dg = (DAT_00410110->f82 - DAT_00410110->f7e) * 0.0625;
    db = (DAT_00410110->f83 - DAT_00410110->f7f) * 0.0625;
    r = DAT_00410110->f7d;
    g = DAT_00410110->f7e;
    b = DAT_00410110->f7f;
    for (i = 0; i < 16; i++) {
        r += dr;
        g += dg;
        b += db;
        DAT_0042ace0[176 + i].r = r;
        DAT_0042ace0[176 + i].g = g;
        DAT_0042ace0[176 + i].b = b;
    }
    set_palette(DAT_0042ace0);
}

/* Callback of the colour sliders (dp2 of d_slider_proc; dp3 points at the
 * slider's entry of slider_values) [verified: the pointers at 0x0040EC50,
 * 0x0040EC88...]. Stores the new value; sliders 0-2 set the start colour
 * of the gradient, 3-5 the end colour, then the palette is rebuilt. */
// FUNCTION: MAPED 0x004014e0
int slider_changed(void *dp3, int d2)
{
    unsigned int i;

    i = ((char *)dp3 - (char *)slider_values) / sizeof(int);
    if (slider_values[i] != d2) {
        slider_values[i] = d2;
        if (i == 0 || i == 1 || i == 2) {
            DAT_00410110->f7d = slider_values[0] / 4;
            DAT_00410110->f7e = slider_values[1] / 4;
            DAT_00410110->f7f = slider_values[2] / 4;
        } else {
            DAT_00410110->f81 = slider_values[3] / 4;
            DAT_00410110->f82 = slider_values[4] / 4;
            DAT_00410110->f83 = slider_values[5] / 4;
        }
        vsync();
        FUN_00401400();
    }
    return 0;
}

/* Dialog procedure of the colour sliders [hypothesis: stored as proc of
 * the DIALOG entry 0x0040ED38, notes/layout-maped.md §4]: a d_slider_proc
 * whose d2 follows the int that dp3 points at. On MSG_START it takes the
 * value; on MSG_IDLE, if the value changed, it redraws the slider. */
// FUNCTION: MAPED 0x004015d0
int FUN_004015d0(int msg, DIALOG *d, int c)
{
    int *value = d->dp3;

    switch (msg) {
    case MSG_START:
        d->d2 = *value;
        break;
    case MSG_IDLE:
        if (d->d2 != *value) {
            d->d2 = *value;
            show_mouse(NULL);
            object_message(d, MSG_DRAW, 0);
            show_mouse(screen);
        }
        break;
    }
    return d_slider_proc(msg, d, c);
}
