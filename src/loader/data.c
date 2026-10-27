/* Globals of loader.exe. The game has none [verified:
 * notes/layout-loader.md §6]; only the CRT variable that main passes to
 * install_allegro is annotated here. */

/* CRT variable, passed by allegro_init in main [verified: push 0x0040670C
 * at 0x00401017; written by 0x004042AC]. Annotated by name like happy's. */
// GLOBAL: LOADER 0x0040670c
// errno
