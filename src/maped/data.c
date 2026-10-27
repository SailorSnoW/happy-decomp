/* Game globals of maped.exe, in address order.
 * [hypothesis] Defined here until the .obj that owns each one is known
 * (notes/layout-maped.md §6), as in src/happy/data.c. */
#include "map.h"
#include "block_01.h"

// GLOBAL: MAPED 0x004108e0
OBJ DAT_00414140[100];

// GLOBAL: MAPED 0x004113e0
char help_texts[16][HELP_TEXT_SIZE];

// GLOBAL: MAPED 0x0042a860
OBJ DAT_00440060[40];

// GLOBAL: MAPED 0x0042b0e0
DATAFILE *map_datafile;
