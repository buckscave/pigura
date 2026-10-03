/* ----------------------------------------------------------------------- *
 * demo/ikon_xpm.h - ikon XPM 16x16 untuk toolbar aplikasi
 * ----------------------------------------------------------------------- *
 * Ikon dibuat sebagai array string XPM C, dimuat dengan
 * pg_muat_xpm_dari_string() dan di-render sebagai permukaan.
 * Format: 16x16, 2-3 warna per ikon, transparan.
 * ----------------------------------------------------------------------- */
#ifndef PIGURA_IKON_XPM_H
#define PIGURA_IKON_XPM_H

/* Ikon Baru (New) - dokumen kosong */
static const char *ikon_baru[] = {
"16 16 3 1",
"  c None",
". c #FFFFFF",
"X c #404040",
"                ",
" XXXXXXXXXXXXX  ",
" X...........X  ",
" X...........X  ",
" X...........X  ",
" X...........X  ",
" X...........X  ",
" X...........X  ",
" X...........X  ",
" X...........X  ",
" X...........X  ",
" X...........X  ",
" X...........X  ",
" X...........X  ",
" XXXXXXXXXXXXX  ",
"                "
};

/* Ikon Buka (Open) - folder kuning */
static const char *ikon_buka[] = {
"16 16 3 1",
"  c None",
". c #C0C040",
"X c #404040",
"                ",
"                ",
" XXXXX          ",
" X...X          ",
" X...XXXXXXXXX  ",
" X...........X  ",
" X...........X  ",
" X...........X  ",
" X...........X  ",
" X...........X  ",
" X...........X  ",
" X...........X  ",
" X...........X  ",
" X...........X  ",
" XXXXXXXXXXXXX  ",
"                "
};

/* Ikon Simpan (Save) - disket */
static const char *ikon_simpan[] = {
"16 16 3 1",
"  c None",
". c #404040",
"X c #FFFFFF",
"                ",
" XXXXXXXXXXXXX  ",
" X........X..X  ",
" X........X..X  ",
" X........X..X  ",
" X........X..X  ",
" X........XXX   ",
" X...........X  ",
" X.XXXXXXX...X  ",
" X.XXXXXXX...X  ",
" X.XXXXXXX...X  ",
" X.XXXXXXX...X  ",
" X...........X  ",
" X...........X  ",
" XXXXXXXXXXXXX  ",
"                "
};

/* Ikon Urungkan (Undo) - panah kiri */
static const char *ikon_urungkan[] = {
"16 16 3 1",
"  c None",
". c #404040",
"X c #4080C0",
"                ",
"                ",
"    X           ",
"   XX           ",
"  XXXX          ",
" XX  XXXXXX     ",
" X      XXX     ",
"             X  ",
"             X  ",
"             X  ",
"             X  ",
"             X  ",
"                ",
"                ",
"                ",
"                "
};

/* Ikon Ulangi (Redo) - panah kanan */
static const char *ikon_ulangi[] = {
"16 16 3 1",
"  c None",
". c #404040",
"X c #4080C0",
"                ",
"                ",
"           X    ",
"           XX   ",
"          XXXX  ",
"     XXXXXX  XX ",
"     XXX      X ",
"  X             ",
"  X             ",
"  X             ",
"  X             ",
"  X             ",
"                ",
"                ",
"                ",
"                "
};

/* Ikon Perbesar (Zoom+) */
static const char *ikon_perbesar[] = {
"16 16 3 1",
"  c None",
". c #404040",
"X c #4080C0",
"                ",
"     XXXXX      ",
"    X.....X     ",
"   X..X.X..X    ",
"   X..X.X..X    ",
"   X.....X X    ",
"    X.....X     ",
"     XXXXX      ",
"        X       ",
"        X       ",
"       X        ",
"      X         ",
"                ",
"                ",
"                ",
"                "
};

/* Ikon Perkecil (Zoom-) */
static const char *ikon_perkecil[] = {
"16 16 3 1",
"  c None",
". c #404040",
"X c #4080C0",
"                ",
"     XXXXX      ",
"    X.....X     ",
"   X.......X    ",
"   X.......X    ",
"   X.......X    ",
"    X.....X     ",
"     XXXXX      ",
"        X       ",
"        X       ",
"       X        ",
"      X         ",
"                ",
"                ",
"                ",
"                "
};

/* Ikon Tool Select - kursor */
static const char *ikon_tool_select[] = {
"16 16 2 1",
"  c None",
"X c #FFFFFF",
"XX              ",
"X.X             ",
"X..X            ",
"X...X           ",
"X....X          ",
"X.....X         ",
"X......X        ",
"X.......X       ",
"X........X      ",
"X....XXXXX      ",
"X....X          ",
"X.X..X          ",
"XX...X          ",
" X...X          ",
"  X X           ",
"                "
};

/* Ikon Tool Rectangle - kotak */
static const char *ikon_tool_rect[] = {
"16 16 2 1",
"  c None",
"X c #FFFFFF",
"                ",
" XXXXXXXXXX     ",
" X........X     ",
" X........X     ",
" X........X     ",
" X........X     ",
" X........X     ",
" X........X     ",
" X........X     ",
" X........X     ",
" X........X     ",
" X........X     ",
" XXXXXXXXXX     ",
"                ",
"                ",
"                "
};

/* Ikon Tool Circle - lingkaran */
static const char *ikon_tool_circle[] = {
"16 16 2 1",
"  c None",
"X c #FFFFFF",
"                ",
"    XXXXXX      ",
"   XX....XX     ",
"  X........X    ",
" X..........X   ",
" X..........X   ",
" X..........X   ",
" X..........X   ",
" X..........X   ",
" X..........X   ",
"  X........X    ",
"   XX....XX     ",
"    XXXXXX      ",
"                ",
"                ",
"                "
};

/* Ikon Tool Text - huruf T */
static const char *ikon_tool_text[] = {
"16 16 2 1",
"  c None",
"X c #FFFFFF",
"                ",
" XXXXXXXXXXXX   ",
" XXXXXXXXXXXX   ",
"      XX        ",
"      XX        ",
"      XX        ",
"      XX        ",
"      XX        ",
"      XX        ",
"      XX        ",
"      XX        ",
"      XX        ",
"      XX        ",
"      XX        ",
"                ",
"                "
};

/* Ikon Tool Zoom - kaca pembesar */
static const char *ikon_tool_zoom[] = {
"16 16 2 1",
"  c None",
"X c #FFFFFF",
"     XXXXX      ",
"    X.....X     ",
"   X.......X    ",
"   X.......X    ",
"   X.......X    ",
"   X.......X    ",
"    X.....X     ",
"     XXXXX      ",
"        X       ",
"        X       ",
"       X        ",
"      X         ",
"     X          ",
"                ",
"                ",
"                "
};

/* Ikon Tool Hand - tangan */
static const char *ikon_tool_hand[] = {
"16 16 2 1",
"  c None",
"X c #FFFFFF",
"                ",
"    XX  XX      ",
"    XX  XX      ",
"    XX  XX      ",
"    XXXXXX      ",
"   XXXXXXXX     ",
"  XXXXXXXXXX    ",
" XXXXXXXXXXXX   ",
" XXXXXXXXXXXX   ",
" XXXXXXXXXXXX   ",
"  XXXXXXXXXX    ",
"   XXXXXXXX     ",
"    XXXXXX      ",
"                ",
"                ",
"                "
};

#endif /* PIGURA_IKON_XPM_H */
