/* happy block_06: 0x004063A0-0x004074FF, title screen, controls, options,
 * introduction (notes/layout-happy.md §8). Functions in address order. */
#include "happy.h"

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
