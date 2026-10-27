/* happy block_08: 0x00409870-0x0040A99F, start menu, custom game, main,
 * WinMain (notes/layout-happy.md §8). Functions in address order. */
#include "happy.h"

/* 0x0040A830, not decompiled yet. */
int main(int argc, char *argv[]);

/* [verified] Allegro's END_OF_MAIN() writes WinMain right after main.
 * reccmp cannot see a definition in a macro call: lookup by symbol name. */
// FUNCTION: HAPPY 0x0040a970
// WinMain
END_OF_MAIN();
