/* happy block_07: 0x00407500-0x0040986F, menus, passwords, scores, game over,
 * game flow (notes/layout-happy.md §8). Functions in address order. */
#include "happy.h"

/* [hypothesis] Two animation counters (2 frames, 8 calls per frame), each
 * advanced when its argument is non-zero. Only caller: 0x00409080 (ending
 * dialogue). */
// FUNCTION: HAPPY 0x00409010
void anim_tick(int a, int b)
{
    if (a != 0) {
        DAT_0042f530++;
        if (DAT_0042f530 == 8) {
            DAT_0042f530 = 0;
            DAT_0042f52c++;
            if (DAT_0042f52c > 1)
                DAT_0042f52c = 0;
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
