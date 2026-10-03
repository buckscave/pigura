/* ----------------------------------------------------------------------------------------------
 * pigura/font_internal.h - Definisi struct pg_font (internal)
 * ----------------------------------------------------------------------------------------------
 * Tidak publik; hanya dipakai oleh src/gambar/font_bitmap.c dan
 * src/gambar/ttf.c. Aplikasi hanya melihat pg_font_t opaque.
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_FONT_INTERNAL_H
#define PIGURA_FONT_INTERNAL_H

#include "pigura/font.h"
#include "pigura/tipe.h"

struct pg_font {
        int   jenis;        /* PG_FONT_JENIS_BITMAP atau _TTF */
        int   ukuran_px;    /* untuk TTF */
        void *impl;         /* pg_font_impl atau pg_font_ttf_t */
};

/* Forward declare impl TTF untuk dipakai font_bitmap.c. */
typedef struct pg_font_ttf pg_font_ttf_t;

/* Fungsi internal TTF yang dipakai oleh font.c dispatcher. */
pg_galat pg_ttf_glyph(const pg_font_ttf_t *t, pg_u32 kode,
                       pg_glyph_t *metrik, int ukuran_px);
pg_galat pg_ttf_gambar(const pg_font_ttf_t *t, pg_u32 kode,
                        pg_permukaan_t *s, int x, int y,
                        pg_warna_t c, int ukuran_px);
int pg_ttf_lebar_teks(const pg_font_ttf_t *t, const char *teks,
                      int ukuran_px);
int pg_ttf_tinggi(const pg_font_ttf_t *t, int ukuran_px);
int pg_ttf_tinggi_baris(const pg_font_ttf_t *t, int ukuran_px);
int pg_ttf_ascent(const pg_font_ttf_t *t, int ukuran_px);
int pg_ttf_descent(const pg_font_ttf_t *t, int ukuran_px);
void pg_ttf_hancur(pg_font_ttf_t *t);

/* Lookup kerning antar dua glyph ID (FUnits, bisa negatif).
 * Mengembalikan 0 bila tidak ada entry kerning. */
pg_s16 pg_ttf_kerning(const pg_font_ttf_t *t, int left_gid,
                       int right_gid);

/* Sama dengan pg_ttf_kerning tapi sudah diskalakan ke piksel
 * untuk ukuran_px tertentu. Untuk dipanggil dispatcher. */
int pg_ttf_kerning_px(const pg_font_ttf_t *t, int left_gid,
                       int right_gid, int ukuran_px);

/* Glyph ID untuk codepoint (cmap lookup). Untuk dipanggil
 * dispatcher agar bisa konsultasi kerning table. */
int pg_ttf_glyph_id(const pg_font_ttf_t *t, pg_u32 kode);

#endif /* PIGURA_FONT_INTERNAL_H */
