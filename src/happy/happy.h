/* Globals and functions of happy.exe shared between its source files.
 * Globals are defined in data.c, in address order. */
#ifndef HAPPY_H
#define HAPPY_H

#include <allegro.h>
#include "map.h"

/* data.c */
extern volatile int timer_ticks;
extern int DAT_0042f52c;
extern int DAT_0042f530;
extern int DAT_00413b24;
extern int DAT_00413b28;

/* block_06.c */
int wait_or_esc(int ms);

/* block_07.c */
void anim_tick(int a, int b);

#endif
