/* The whole game code of loader.exe: block_01 (0x00401000-0x004014CF).
 * notes/layout-loader.md §8. main reads a list of maps and writes
 * happy.clv next to the exe; END_OF_MAIN() writes WinMain right after it.
 * loader only. */
#include <direct.h>
#include <errno.h>
#include <string.h>
#include <allegro.h>

/* One map slot of happy.clv, 335 bytes [verified: stride 0x14F of both
 * copy loops in main]: the .map file name, then the line of the list it
 * came from. Same layout as happy's T_0043f5c0 (src/happy/happy.h), which
 * reads these slots back. */
typedef struct MAPSLOT {
    char file[255];
    char name[80];
} MAPSLOT;

/* [verified] Allegro's main. The list of maps is argv[1], or a .txt file
 * picked with file_select_ex in a 640x480 window after chdir to "maps"
 * (Cancel: returns 0). The list is read with pack_fgets: lines are
 * skipped up to a first line starting with '#', then each line up to the
 * next '#' names a regular map (at most 18), then each line up to a third
 * '#' a bonus map (at most 8). Each name is taken relative to the folder
 * of the list and must exist. The 26 slots and the two counts go to
 * happy.clv in the folder of the exe (layout-loader.md §3, format of
 * happy.clv). Errors: 2 the list cannot be opened, 3 a map does not
 * exist, 4 too many maps, 5 end of file before the closing '#'; the
 * list is not closed on these paths, and the result of pack_fopen of
 * happy.clv is not checked [verified].
 * Matching: the chained assignment of the counts decides the order in
 * which VC6 reloads num_bonus and the pointer to pack_fgets after the
 * first loop (two separate assignments: 99.44%, in either order);
 * "r" / "w" are written as literals, F_READ / F_WRITE would leave
 * reccmp's STRING markers without a literal on their line. */
// FUNCTION: LOADER 0x00401000
int main(int argc, char *argv[])
{
    int num_bonus;
    int num_normal;
    char line[256];
    char file[256];
    char path[1024];
    MAPSLOT maps[26];
    PACKFILE *f;

    num_bonus = num_normal = 0;
    allegro_init();
    install_keyboard();
    install_mouse();
    if (argc > 1) {
        strncpy(path, argv[1], sizeof(path));
    } else {
        // STRING: LOADER 0x00406130
        chdir("maps");
        path[0] = '\0';
        if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, 640, 480, 0, 0) < 0)
            set_gfx_mode(GFX_SAFE, 640, 480, 0, 0);
        gui_fg_color = makecol(0, 0, 0);
        gui_bg_color = makecol(255, 255, 255);
        // STRING: LOADER 0x0040610c
        if (!file_select_ex("Select set of levels to load", path,
                            // STRING: LOADER 0x0040612c
                            "txt", sizeof(path), 0, 0))
            return 0;
        set_gfx_mode(GFX_TEXT, 80, 25, 0, 0);
    }
    // STRING: LOADER 0x00406108
    f = pack_fopen(path, "r");
    if (f == NULL) {
        // STRING: LOADER 0x004060ec
        allegro_message("Could not open file \"%s\"\n", path);
        return 2;
    }
    pack_fgets(line, 79, f);
    while (line[0] != '#')
        pack_fgets(line, 79, f);
    pack_fgets(line, 79, f);
    while (line[0] != '#') {
        if (pack_feof(f)) {
            // STRING: LOADER 0x004060b4
            allegro_message("Unexpected end of file\n");
            return 5;
        }
        if (num_normal == 18) {
            // STRING: LOADER 0x004060cc
            allegro_message("Only 18 regular maps allowed\n");
            return 4;
        }
        replace_filename(file, path, line, 254);
        if (!exists(file)) {
            // STRING: LOADER 0x00406080
            allegro_message("Can't find file \"%s\"\n", file);
            return 3;
        }
        strcpy(maps[num_normal].file, file);
        strcpy(maps[num_normal].name, line);
        num_normal++;
        pack_fgets(line, 79, f);
    }
    pack_fgets(line, 79, f);
    while (line[0] != '#') {
        if (pack_feof(f)) {
            allegro_message("Unexpected end of file\n");
            return 5;
        }
        if (num_bonus == 8) {
            // STRING: LOADER 0x00406098
            allegro_message("Only 8 bonus maps allowed\n");
            return 4;
        }
        replace_filename(file, path, line, 254);
        if (!exists(file)) {
            allegro_message("Can't find file \"%s\"\n", file);
            return 3;
        }
        strcpy(maps[18 + num_bonus].file, file);
        strcpy(maps[18 + num_bonus].name, line);
        num_bonus++;
        pack_fgets(line, 79, f);
    }
    pack_fclose(f);
    get_executable_name(path, 254);
    // STRING: LOADER 0x00406074
    replace_filename(file, path, "happy.clv", 254);
    // STRING: LOADER 0x00406070
    f = pack_fopen(file, "w");
    pack_fwrite(maps, sizeof(maps), f);
    pack_iputw(num_normal, f);
    pack_iputw(num_bonus, f);
    pack_fclose(f);
    // STRING: LOADER 0x00406030
    allegro_message("%d normal map%s and %d bonus map%s loaded. Ready to play.\n",
                    num_normal,
                    num_normal == 1 ?
                    // STRING: LOADER 0x00406708
                    "" :
                    // STRING: LOADER 0x0040606c
                    "s",
                    num_bonus,
                    num_bonus == 1 ? "" : "s");
    return 0;
}

/* [verified] Allegro's END_OF_MAIN() writes WinMain right after main.
 * reccmp cannot see a definition in a macro call: lookup by symbol name. */
// FUNCTION: LOADER 0x004014a0
// WinMain
END_OF_MAIN();
