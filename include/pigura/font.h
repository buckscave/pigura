/* ----------------------------------------------------------------------------------------------
 * pigura/font.h - Antarmuka font
 * ----------------------------------------------------------------------------------------------
 * Antarmuka pluggable untuk font rendering. Dua implementasi:
 *   1. Bitmap 8x8 (default, embedded, tanpa dependensi)
 *   2. TrueType renderer sendiri (src/gambar/ttf.c)
 *
 * Font TTF di-cache: glyph yang sudah di-rasterize disimpan di
 * memori untuk reuse. Fallback otomatis ke bitmap bila TTF tidak
 * bisa dimuat.
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_FONT_H
#define PIGURA_FONT_H

#include "pigura/tipe.h"
#include "pigura/permukaan.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pg_font pg_font_t;

/* Jenis font. */
enum pg_font_jenis {
        PG_FONT_JENIS_BITMAP = 0,
        PG_FONT_JENIS_TTF    = 1
};

/* Glyph metrics. */
typedef struct pg_glyph {
        int lebar;       /* lebar piksel */
        int tinggi;      /* tinggi piksel */
        int x_offset;    /* offset horizontal relatif cursor */
        int y_offset;    /* offset vertikal relatif baseline */
        int advance;     /* piksel maju untuk karakter berikutnya */
} pg_glyph_t;

/* Buat font bitmap 8x8 default (tanpa dependensi). */
pg_font_t *pg_buat_font_bitmap(void);

/* Buat font TrueType dari file .ttf. */
pg_font_t *pg_buat_font_ttf(const char *path, int ukuran_px);

/* Bebaskan font. */
void pg_hancur_font(pg_font_t *font);

/* Ambil jenis font. */
int pg_font_jenis(const pg_font_t *font);

/* Ambil ukuran piksel (untuk TTF). */
int pg_font_ukuran_px(const pg_font_t *font);

/* Ambil glyph untuk codepoint unicode. */
pg_galat pg_font_glyph(const pg_font_t *font, pg_u32 kode,
                        pg_glyph_t *metrik);

/* Render glyph ke permukaan pada (x,y). */
pg_galat pg_font_gambar(const pg_font_t *font, pg_u32 kode,
                         pg_permukaan_t *s, int x, int y,
                         pg_warna_t c);

/* Gambar teks string. */
pg_galat pg_font_gambar_teks(const pg_font_t *font,
                              const char *teks,
                              pg_permukaan_t *s, int x, int y,
                              pg_warna_t c);

/* Gambar teks UTF-8. Mendukung multi-byte sequences.
 * Setiap codepoint Unicode diteruskan ke pg_font_gambar().
 * Untuk font bitmap 8x8, codepoint >= 128 dipetakan ke glyph
 * placeholder (0x7F = box) — bitmap hanya punya 128 glyph ASCII.
 * Untuk TTF, codepoint Unicode apa pun yang ada di cmap font
 * akan terender.
 *
 * Byte invalid (overlong, surrogat, truncated) dilewati sebagai
 * codepoint U+FFFD (replacement). */
pg_galat pg_font_gambar_teks_utf8(const pg_font_t *font,
                                    const char *teks,
                                    pg_permukaan_t *s, int x, int y,
                                    pg_warna_t c);

/* Ukur lebar teks (piksel). */
int pg_font_lebar_teks(const pg_font_t *font, const char *teks);

/* Ukur lebar teks UTF-8 (piksel). Multi-byte sequence di-decode
 * ke codepoint, advance width tiap glyph dijumlah. */
int pg_font_lebar_teks_utf8(const pg_font_t *font,
                              const char *teks);

/* Ukur tinggi font total = ascent + descent (piksel). */
int pg_font_tinggi(const pg_font_t *font);

/* Ambil ascent font (piksel) — jarak dari baseline ke top glyph. */
int pg_font_ascent(const pg_font_t *font);

/* Ambil descent font (piksel) — jarak dari baseline ke bottom glyph. */
int pg_font_descent(const pg_font_t *font);

/* Hitung y baseline untuk center vertikal di kotak tinggi_tinggi.
 * Rumus: baseline = (tinggi_tinggi + ascent - descent) / 2 */
int pg_font_baseline_tengah(const pg_font_t *font, int tinggi_tinggi);

/* Tinggi baris (piksel). */
int pg_font_tinggi_baris(const pg_font_t *font);

/* ===== Unicode helper ===== */

/* Decode 1 codepoint UTF-8 dari *p. Advance p. Return -1 bila invalid. */
int pg_aksara_utf8_dekode(const char **p, const char *akhir);

/* Encode codepoint ke UTF-8 di buf. Return jumlah byte (1-4) atau 0. */
int pg_aksara_utf8_enkode(char *buf, int cp);

/* Hitung panjang string UTF-8 dalam codepoint. */
int pg_aksara_utf8_panjang(const char *s);

/* Cek codepoint. */
int pg_aksara_apa_whitespace(int cp);
int pg_aksara_apa_kontrol(int cp);
int pg_aksara_apa_digit(int cp);
int pg_aksara_apa_huruf(int cp);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_FONT_H */
