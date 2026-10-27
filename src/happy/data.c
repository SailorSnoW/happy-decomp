/* Game globals of happy.exe, in address order.
 * [hypothesis] They are defined here until the .obj that owns each one is
 * known (notes/layout-happy.md §6 and §9, questions 1 and 7): this file only
 * decides where the recompiled exe puts them, not the code that uses them.
 * Uninitialised ones are tentative definitions (COMMON), like the large
 * arrays at 0x00413D3C-0x004468BF [hypothesis]. */
#include "happy.h"

// GLOBAL: HAPPY 0x00413b24
int DAT_00413b24;

// GLOBAL: HAPPY 0x00413b28
int DAT_00413b28;

// GLOBAL: HAPPY 0x0042f52c
int DAT_0042f52c;

// GLOBAL: HAPPY 0x0042f530
int DAT_0042f530;

/* Incremented every 15 ms by the timer callback 0x004018F0, installed by
 * install_int(0x004018F0, 15) at 0x00401F6B [verified]. */
// GLOBAL: HAPPY 0x00442084
volatile int timer_ticks;

// GLOBAL: HAPPY 0x004450b4
DATAFILE *map_datafile;
