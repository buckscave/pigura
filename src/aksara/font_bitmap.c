/* ----------------------------------------------------------------------------------------------
 * pigura gambar: font_bitmap.c - font bitmap 8x8 + dispatcher font
 * ----------------------------------------------------------------------------------------------
 * Implementasi pg_font_t dengan renderer bitmap 8x8 ASCII (default,
 * tanpa dependensi). Juga berisi dispatcher API publik yang merutekan
 * ke bitmap atau TTF berdasarkan font->jenis.
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/font.h"
#include "pigura/permukaan.h"
#include "pigura/galat.h"
#include "pigura/catat.h"
#include "pigura/utf8.h"
#include "font_internal.h"

#include <stdlib.h>
#include <string.h>

/* ===================================================================
 * Font bitmap 8x8 — embedded, tanpa dependensi
 * =================================================================== */

static const unsigned char pg_font8x8[128][8] = {
        {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* spasi */
        {0x18,0x3c,0x3c,0x18,0x18,0x00,0x18,0x00}, /* !  */
        {0x36,0x36,0x00,0x00,0x00,0x00,0x00,0x00}, /* "  */
        {0x36,0x36,0x7f,0x36,0x7f,0x36,0x36,0x00}, /* #  */
        {0x0c,0x3e,0x03,0x1e,0x30,0x1f,0x0c,0x00}, /* $  */
        {0x00,0x63,0x33,0x18,0x0c,0x66,0x63,0x00}, /* %  */
        {0x1c,0x36,0x1c,0x3b,0x6e,0x66,0x3b,0x00}, /* &  */
        {0x18,0x18,0x08,0x00,0x00,0x00,0x00,0x00}, /* '  */
        {0x0e,0x1c,0x18,0x18,0x18,0x1c,0x0e,0x00}, /* (  */
        {0x70,0x38,0x18,0x18,0x18,0x38,0x70,0x00}, /* )  */
        {0x00,0x66,0x3c,0xff,0x3c,0x66,0x00,0x00}, /* *  */
        {0x00,0x0c,0x0c,0x3f,0x0c,0x0c,0x00,0x00}, /* +  */
        {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x08}, /* ,  */
        {0x00,0x00,0x00,0x3f,0x00,0x00,0x00,0x00}, /* -  */
        {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00}, /* .  */
        {0x03,0x07,0x0e,0x1c,0x38,0x70,0x60,0x00}, /* /  */
        {0x3e,0x63,0x73,0x7b,0x6f,0x67,0x3e,0x00}, /* 0  */
        {0x0c,0x0e,0x0c,0x0c,0x0c,0x0c,0x3f,0x00}, /* 1  */
        {0x1e,0x33,0x30,0x1c,0x06,0x33,0x3f,0x00}, /* 2  */
        {0x1e,0x33,0x30,0x1c,0x30,0x33,0x1e,0x00}, /* 3  */
        {0x38,0x3c,0x36,0x33,0x7f,0x30,0x78,0x00}, /* 4  */
        {0x3f,0x03,0x1f,0x30,0x30,0x33,0x1e,0x00}, /* 5  */
        {0x1c,0x06,0x03,0x1f,0x33,0x33,0x1e,0x00}, /* 6  */
        {0x3f,0x33,0x30,0x18,0x0c,0x0c,0x0c,0x00}, /* 7  */
        {0x1e,0x33,0x33,0x1e,0x33,0x33,0x1e,0x00}, /* 8  */
        {0x1e,0x33,0x33,0x3e,0x30,0x18,0x0c,0x00}, /* 9  */
        {0x00,0x00,0x18,0x18,0x00,0x18,0x18,0x00}, /* :  */
        {0x00,0x00,0x18,0x18,0x00,0x18,0x18,0x08}, /* ;  */
        {0x30,0x18,0x0c,0x06,0x0c,0x18,0x30,0x00}, /* <  */
        {0x00,0x00,0x3f,0x00,0x3f,0x00,0x00,0x00}, /* =  */
        {0x06,0x0c,0x18,0x30,0x18,0x0c,0x06,0x00}, /* >  */
        {0x1e,0x33,0x30,0x18,0x0c,0x00,0x0c,0x00}, /* ?  */
        {0x3e,0x63,0x7b,0x7b,0x7b,0x03,0x1e,0x00}, /* @  */
        {0x0c,0x1e,0x33,0x33,0x3f,0x33,0x33,0x00}, /* A  */
        {0x3f,0x66,0x66,0x3e,0x66,0x66,0x3f,0x00}, /* B  */
        {0x3c,0x66,0x03,0x03,0x03,0x66,0x3c,0x00}, /* C  */
        {0x1f,0x36,0x66,0x66,0x66,0x36,0x1f,0x00}, /* D  */
        {0x7f,0x46,0x16,0x1e,0x16,0x46,0x7f,0x00}, /* E  */
        {0x7f,0x46,0x16,0x1e,0x16,0x06,0x0f,0x00}, /* F  */
        {0x3c,0x66,0x03,0x03,0x73,0x66,0x7c,0x00}, /* G  */
        {0x33,0x33,0x33,0x3f,0x33,0x33,0x33,0x00}, /* H  */
        {0x1e,0x0c,0x0c,0x0c,0x0c,0x0c,0x1e,0x00}, /* I  */
        {0x78,0x30,0x30,0x30,0x33,0x33,0x1e,0x00}, /* J  */
        {0x67,0x66,0x36,0x1e,0x36,0x66,0x67,0x00}, /* K  */
        {0x0f,0x06,0x06,0x06,0x46,0x66,0x7f,0x00}, /* L  */
        {0x63,0x77,0x7f,0x7f,0x6b,0x63,0x63,0x00}, /* M  */
        {0x63,0x67,0x6f,0x7b,0x73,0x63,0x63,0x00}, /* N  */
        {0x1c,0x36,0x63,0x63,0x63,0x36,0x1c,0x00}, /* O  */
        {0x3f,0x66,0x66,0x3e,0x06,0x06,0x0f,0x00}, /* P  */
        {0x1e,0x33,0x33,0x33,0x3b,0x1e,0x38,0x00}, /* Q  */
        {0x3f,0x66,0x66,0x3e,0x36,0x66,0x67,0x00}, /* R  */
        {0x1e,0x33,0x07,0x0e,0x38,0x33,0x1e,0x00}, /* S  */
        {0x3f,0x2d,0x0c,0x0c,0x0c,0x0c,0x1e,0x00}, /* T  */
        {0x33,0x33,0x33,0x33,0x33,0x33,0x3f,0x00}, /* U  */
        {0x33,0x33,0x33,0x33,0x33,0x1e,0x0c,0x00}, /* V  */
        {0x63,0x63,0x6b,0x7f,0x7f,0x77,0x63,0x00}, /* W  */
        {0x63,0x36,0x1c,0x08,0x1c,0x36,0x63,0x00}, /* X  */
        {0x33,0x33,0x33,0x1e,0x0c,0x0c,0x1e,0x00}, /* Y  */
        {0x7f,0x63,0x31,0x18,0x4c,0x66,0x7f,0x00}, /* Z  */
        {0x1e,0x0c,0x0c,0x0c,0x0c,0x0c,0x1e,0x00}, /* [  */
        {0x60,0x70,0x38,0x1c,0x0e,0x07,0x03,0x00}, /* \  */
        {0x78,0x18,0x18,0x18,0x18,0x18,0x78,0x00}, /* ]  */
        {0x00,0x08,0x1c,0x36,0x00,0x00,0x00,0x00}, /* ^  */
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff}, /* _  */
        {0x0c,0x0c,0x18,0x00,0x00,0x00,0x00,0x00}, /* `  */
        {0x00,0x00,0x1e,0x30,0x3e,0x33,0x3e,0x00}, /* a  */
        {0x07,0x06,0x06,0x3e,0x66,0x66,0x3b,0x00}, /* b  */
        {0x00,0x00,0x1e,0x03,0x03,0x33,0x1e,0x00}, /* c  */
        {0x38,0x30,0x30,0x3e,0x33,0x33,0x3e,0x00}, /* d  */
        {0x00,0x00,0x1e,0x33,0x3f,0x03,0x1e,0x00}, /* e  */
        {0x1c,0x36,0x06,0x0f,0x06,0x06,0x0f,0x00}, /* f  */
        {0x00,0x00,0x3e,0x33,0x33,0x3e,0x30,0x1f}, /* g  */
        {0x07,0x06,0x36,0x6e,0x66,0x66,0x67,0x00}, /* h  */
        {0x0c,0x00,0x0e,0x0c,0x0c,0x0c,0x1e,0x00}, /* i  */
        {0x30,0x00,0x30,0x30,0x33,0x33,0x1e,0x00}, /* j  */
        {0x07,0x06,0x66,0x36,0x1e,0x36,0x67,0x00}, /* k  */
        {0x0e,0x0c,0x0c,0x0c,0x0c,0x0c,0x1e,0x00}, /* l  */
        {0x00,0x00,0x33,0x7f,0x7f,0x6b,0x63,0x00}, /* m  */
        {0x00,0x00,0x1f,0x33,0x33,0x33,0x33,0x00}, /* n  */
        {0x00,0x00,0x1e,0x33,0x33,0x33,0x1e,0x00}, /* o  */
        {0x00,0x00,0x3b,0x66,0x66,0x3e,0x06,0x0f}, /* p  */
        {0x00,0x00,0x3e,0x33,0x33,0x3e,0x30,0x78}, /* q  */
        {0x00,0x00,0x3b,0x66,0x06,0x06,0x0f,0x00}, /* r  */
        {0x00,0x00,0x3e,0x03,0x1e,0x30,0x1e,0x00}, /* s  */
        {0x0c,0x0c,0x3f,0x0c,0x0c,0x0c,0x38,0x00}, /* t  */
        {0x00,0x00,0x33,0x33,0x33,0x33,0x3e,0x00}, /* u  */
        {0x00,0x00,0x33,0x33,0x33,0x1e,0x0c,0x00}, /* v  */
        {0x00,0x00,0x63,0x6b,0x7f,0x7f,0x36,0x00}, /* w  */
        {0x00,0x00,0x63,0x36,0x1c,0x36,0x63,0x00}, /* x  */
        {0x00,0x00,0x33,0x33,0x33,0x3e,0x30,0x1f}, /* y  */
        {0x00,0x00,0x3f,0x19,0x0c,0x26,0x3f,0x00}, /* z  */
        {0x38,0x0c,0x0c,0x07,0x0c,0x0c,0x38,0x00}, /* {  */
        {0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00}, /* |  */
        {0x07,0x18,0x18,0x70,0x18,0x18,0x07,0x00}, /* }  */
        {0x00,0x00,0x32,0x7e,0x4c,0x00,0x00,0x00}, /* ~  */
        {0xff,0x81,0x81,0x81,0x81,0x81,0xff,0x00}  /* box */
};

/* Impl bitmap — kosong, data ada di static array. */
typedef struct {
        int dummy;
} pg_bitmap_impl;

pg_font_t *pg_buat_font_bitmap(void)
{
        pg_font_t *font;
        pg_bitmap_impl *impl;
        impl = (pg_bitmap_impl *)calloc(1, sizeof(*impl));
        if (!impl) {
                pg_set_galat(PG_GALAT_MEMORI, "buat_font_bitmap: oom");
                return NULL;
        }
        font = (pg_font_t *)calloc(1, sizeof(*font));
        if (!font) { free(impl); return NULL; }
        font->jenis = PG_FONT_JENIS_BITMAP;
        font->ukuran_px = 8;
        font->impl = impl;
        return font;
}

static pg_galat pg_bitmap_glyph(pg_u32 kode, pg_glyph_t *m)
{
        if (!m) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
        if (kode >= 128) kode = 0x7f;
        m->lebar    = 8;
        m->tinggi   = 8;
        m->x_offset = 0;
        m->y_offset = 0;
        m->advance  = 8;
        return PG_OK;
}

static pg_galat pg_bitmap_gambar(pg_u32 kode, pg_permukaan_t *s,
                                  int x, int y, pg_warna_t c)
{
        const unsigned char *g;
        int row, col;
        unsigned char k;
        if (!s) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
        k = (unsigned char)kode;
        if (k >= 128) k = 0x7f;
        g = pg_font8x8[k];
        for (row = 0; row < 8; row++) {
                for (col = 0; col < 8; col++) {
                        if (g[row] & (0x80 >> col))
                                pg_setel_piksel_permukaan(s, x + col,
                                                           y + row, c);
                }
        }
        return PG_OK;
}

/* ===================================================================
 * Dispatcher API publik
 * ----------------------------------------------------------------------------------------------
 * Merutekan ke bitmap atau TTF berdasarkan font->jenis.
 * =================================================================== */

void pg_hancur_font(pg_font_t *font)
{
        if (!font) return;
        if (font->jenis == PG_FONT_JENIS_TTF) {
                pg_ttf_hancur((pg_font_ttf_t *)font->impl);
        } else {
                free(font->impl);
        }
        free(font);
}

int pg_font_jenis(const pg_font_t *font)
{
        return font ? font->jenis : PG_FONT_JENIS_BITMAP;
}

int pg_font_ukuran_px(const pg_font_t *font)
{
        return font ? font->ukuran_px : 0;
}

pg_galat pg_font_glyph(const pg_font_t *font, pg_u32 kode,
                        pg_glyph_t *metrik)
{
        if (!font || !metrik) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
        if (font->jenis == PG_FONT_JENIS_BITMAP)
                return pg_bitmap_glyph(kode, metrik);
        return pg_ttf_glyph((pg_font_ttf_t *)font->impl, kode, metrik,
                             font->ukuran_px);
}

pg_galat pg_font_gambar(const pg_font_t *font, pg_u32 kode,
                         pg_permukaan_t *s, int x, int y,
                         pg_warna_t c)
{
        if (!font || !s) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
        if (font->jenis == PG_FONT_JENIS_BITMAP)
                return pg_bitmap_gambar(kode, s, x, y, c);
        return pg_ttf_gambar((pg_font_ttf_t *)font->impl, kode, s,
                              x, y, c, font->ukuran_px);
}

pg_galat pg_font_gambar_teks(const pg_font_t *font,
                              const char *teks,
                              pg_permukaan_t *s, int x, int y,
                              pg_warna_t c)
{
        const char *p;
        int cx = x;
        int prev_gid = -1;  /* untuk kerning (TTF only) */
        if (!font || !teks || !s) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
        for (p = teks; *p; p++) {
                pg_glyph_t g;
                int gid;
                if (*p == '\n') {
                        cx = x;
                        y += pg_font_tinggi_baris(font);
                        prev_gid = -1;
                        continue;
                }
                /* Untuk TTF, kerning antar glyph bertetangga
                 * ditambahkan ke cursor sebelum render. */
                if (font->jenis == PG_FONT_JENIS_TTF) {
                        gid = pg_ttf_glyph_id((pg_font_ttf_t *)font->impl,
                                              (pg_u32)(unsigned char)*p);
                        if (prev_gid >= 0) {
                                cx += pg_ttf_kerning_px(
                                        (pg_font_ttf_t *)font->impl,
                                        prev_gid, gid, font->ukuran_px);
                        }
                } else {
                        gid = -1;
                }
                pg_font_glyph(font, (pg_u32)(unsigned char)*p, &g);
                pg_font_gambar(font, (pg_u32)(unsigned char)*p, s,
                                cx, y, c);
                cx += g.advance;
                prev_gid = gid;
        }
        return PG_OK;
}

int pg_font_lebar_teks(const pg_font_t *font, const char *teks)
{
        const char *p;
        int total = 0;
        if (!font || !teks) return 0;
        if (font->jenis == PG_FONT_JENIS_BITMAP) {
                for (p = teks; *p; p++) total += 8;
                return total;
        }
        return pg_ttf_lebar_teks((pg_font_ttf_t *)font->impl, teks,
                                  font->ukuran_px);
}

pg_galat pg_font_gambar_teks_utf8(const pg_font_t *font,
                                    const char *teks,
                                    pg_permukaan_t *s, int x, int y,
                                    pg_warna_t c)
{
        const char *p;
        int cx = x;
        int prev_gid = -1;
        if (!font || !teks || !s) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);

        for (p = teks; *p; ) {
                pg_u32 cp;
                int adv;
                pg_glyph_t g;
                int gid;

                adv = pg_utf8_decode(p, &cp);
                if (adv <= 0) {
                        /* Invalid byte: maju 1, tampilkan replacement
                         * (U+FFFD tidak ada di bitmap 8x8 -> box). */
                        cp = 0xFFFDu;
                        adv = 1;
                }
                p += adv;

                if (cp == (pg_u32)'\n') {
                        cx = x;
                        y += pg_font_tinggi_baris(font);
                        prev_gid = -1;
                        continue;
                }

                if (font->jenis == PG_FONT_JENIS_TTF) {
                        gid = pg_ttf_glyph_id(
                                (pg_font_ttf_t *)font->impl, cp);
                        if (prev_gid >= 0) {
                                cx += pg_ttf_kerning_px(
                                        (pg_font_ttf_t *)font->impl,
                                        prev_gid, gid,
                                        font->ukuran_px);
                        }
                } else {
                        gid = -1;
                }

                pg_font_glyph(font, cp, &g);
                pg_font_gambar(font, cp, s, cx, y, c);
                cx += g.advance;
                prev_gid = gid;
        }
        return PG_OK;
}

int pg_font_lebar_teks_utf8(const pg_font_t *font, const char *teks)
{
        const char *p;
        int total = 0;
        int prev_gid = -1;
        if (!font || !teks) return 0;

        for (p = teks; *p; ) {
                pg_u32 cp;
                int adv;
                pg_glyph_t g;
                int gid;

                adv = pg_utf8_decode(p, &cp);
                if (adv <= 0) {
                        cp = 0xFFFDu;
                        adv = 1;
                }
                p += adv;

                if (cp == (pg_u32)'\n') {
                        prev_gid = -1;
                        continue;
                }

                if (font->jenis == PG_FONT_JENIS_TTF) {
                        gid = pg_ttf_glyph_id(
                                (pg_font_ttf_t *)font->impl, cp);
                        if (prev_gid >= 0) {
                                total += pg_ttf_kerning_px(
                                        (pg_font_ttf_t *)font->impl,
                                        prev_gid, gid,
                                        font->ukuran_px);
                        }
                        if (pg_font_glyph(font, cp, &g) != PG_OK)
                                continue;
                        total += g.advance;
                        prev_gid = gid;
                } else {
                        /* Bitmap 8x8: advance 8 piksel per codepoint.
                         * Tidak ada kerning. */
                        total += 8;
                        prev_gid = -1;
                }
        }
        return total;
}

int pg_font_tinggi(const pg_font_t *font)
{
        if (!font) return 0;
        if (font->jenis == PG_FONT_JENIS_BITMAP) return 10;
        return pg_ttf_tinggi((pg_font_ttf_t *)font->impl,
                              font->ukuran_px);
}

int pg_font_tinggi_baris(const pg_font_t *font)
{
        if (!font) return 0;
        if (font->jenis == PG_FONT_JENIS_BITMAP) return 10;
        return pg_ttf_tinggi_baris((pg_font_ttf_t *)font->impl,
                                     font->ukuran_px);
}

int pg_font_ascent(const pg_font_t *font)
{
        if (!font) return 0;
        if (font->jenis == PG_FONT_JENIS_BITMAP) return 8;
        return pg_ttf_ascent((pg_font_ttf_t *)font->impl,
                              font->ukuran_px);
}

int pg_font_descent(const pg_font_t *font)
{
        if (!font) return 0;
        if (font->jenis == PG_FONT_JENIS_BITMAP) return 2;
        return pg_ttf_descent((pg_font_ttf_t *)font->impl,
                               font->ukuran_px);
}

int pg_font_baseline_tengah(const pg_font_t *font, int tinggi_tinggi)
{
        int asc, desc;
        if (!font) return tinggi_tinggi / 2;
        asc = pg_font_ascent(font);
        desc = pg_font_descent(font);
        /* baseline = (tinggi_tinggi + ascent - descent) / 2
         * Center glyph vertikal di dalam kotak. */
        return (tinggi_tinggi + asc - desc) / 2;
}
