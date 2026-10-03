/* ----------------------------------------------------------------------------------------------
 * pigura gambar: ttf.c - renderer TrueType dengan anti-aliasing
 * ----------------------------------------------------------------------------------------------
 * Parser dan rasterizer font TrueType (.ttf) tanpa dependensi
 * eksternal. Membaca file TTF, parse tabel utama (head, hhea, maxp,
 * cmap, loca, glyf, hmtx), ekstrak outline glyph, flatten kurva
 * Bezier kuadratik ke segmen garis, dan rasterize dengan
 * supersampling 4x untuk anti-aliasing kualitas setara FreeType
 * tanpa hinting.
 *
 * Algoritma rasterisasi:
 *   1. Scale outline dari unit font ke piksel target
 *   2. Supersample 4x (render pada 4x resolusi)
 *   3. Scanline fill dengan aturan even-odd
 *   4. Downsample box-filter 4x4 → 16 level coverage
 *
 * Glyph cache: hash table sederhana dengan eviction LRU.
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/font.h"
#include "pigura/permukaan.h"
#include "pigura/galat.h"
#include "pigura/catat.h"
#include "font_internal.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* ===================================================================
 * Tipe data internal
 * =================================================================== */

/* Titik outline fixed-point 16.16. */
typedef struct pg_ttf_titik {
        pg_s32 x, y;
        pg_bool on_curve;
} pg_ttf_titik_t;

/* Kerning pair (subtable format 0). */
typedef struct {
        pg_u16 left;
        pg_u16 right;
        pg_s16 value;  /* FUnits, bisa negatif */
} pg_kern_pair_t;

/* Glyph yang sudah di-rasterize (cached). */
typedef struct pg_ttf_glyph_cache {
        int glyph_id;
        int ukuran_px;
        pg_u8 *bitmap;        /* grayscale 0..255, lebar*tinggi */
        int lebar, tinggi;
        int x_offset, y_offset;
        int advance;
        int ref_count;        /* LRU */
} pg_ttf_glyph_cache_t;

#define PG_TTF_CACHE_SIZE 256

/* Struktur font TTF. */
struct pg_font_ttf {
        char      *data;          /* seluruh file di memori */
        size_t    ukuran;

        /* Offset tabel. */
        size_t head_off, head_len;
        size_t hhea_off, hhea_len;
        size_t maxp_off, maxp_len;
        size_t cmap_off, cmap_len;
        size_t loca_off, loca_len;
        size_t glyf_off, glyf_len;
        size_t hmtx_off, hmtx_len;

        /* Nilai terparse. */
        int units_per_em;
        int num_glyphs;
        int num_hmetrics;
        int index_to_loc_format; /* 0=short, 1=long */
        int ascent, descent, line_gap;

        /* cmap format 4. */
        size_t cmap4_off;
        int    cmap4_seg_count;

        /* kern table (format 0 horizontal, opsional). */
        pg_kern_pair_t *kern_pairs;
        int    n_kern_pairs;

        /* OS/2 table (opsional, untuk sTypoAscender/Descender). */
        size_t os2_off, os2_len;
        int sTypoAscender, sTypoDescender, sTypoLineGap;
        int usWinAscent, usWinDescent;

        /* Cache. */
        pg_ttf_glyph_cache_t cache[PG_TTF_CACHE_SIZE];
        int cache_next;
};

/* ===================================================================
 * Big-endian reader
 * =================================================================== */

static pg_u16 pg_be16(const void *p)
{
        const pg_u8 *b = (const pg_u8 *)p;
        return (pg_u16)((b[0] << 8) | b[1]);
}

static pg_s16 pg_be16s(const void *p)
{
        const pg_u8 *b = (const pg_u8 *)p;
        return (pg_s16)((b[0] << 8) | b[1]);
}

static pg_u32 pg_be32(const void *p)
{
        const pg_u8 *b = (const pg_u8 *)p;
        return ((pg_u32)b[0] << 24) | ((pg_u32)b[1] << 16) |
               ((pg_u32)b[2] << 8) | (pg_u32)b[3];
}

/* ===================================================================
 * Cari tabel by tag (4-byte string)
 * =================================================================== */

static int pg_ttf_cari_tabel(pg_font_ttf_t *t, const char *tag,
                              size_t *off, size_t *len)
{
        int i;
        size_t dir_off = 12; /* skip offset table (12 bytes) */
        int num_tables = pg_be16(t->data + 4);

        for (i = 0; i < num_tables; i++) {
                size_t entry = dir_off + (size_t)i * 16;
                char current_tag[5];
                memcpy(current_tag, t->data + entry, 4);
                current_tag[4] = '\0';
                if (strcmp(current_tag, tag) == 0) {
                        *off = pg_be32(t->data + entry + 8);
                        *len = pg_be32(t->data + entry + 12);
                        return 0;
                }
        }
        return -1;
}

/* ===================================================================
 * Parse tabel head
 * =================================================================== */

static int pg_ttf_parse_head(pg_font_ttf_t *t)
{
        const char *p;
        if (!t->head_off || t->head_off + 54 > t->ukuran) return -1;
        p = t->data + t->head_off;
        t->units_per_em = pg_be16(p + 18);
        t->index_to_loc_format = pg_be16s(p + 50);
        if (t->units_per_em <= 0 || t->units_per_em > 4096) return -1;
        return 0;
}

/* ===================================================================
 * Parse tabel hhea
 * =================================================================== */

static int pg_ttf_parse_hhea(pg_font_ttf_t *t)
{
        const char *p;
        if (!t->hhea_off || t->hhea_off + 36 > t->ukuran) return -1;
        p = t->data + t->hhea_off;
        t->ascent  = pg_be16s(p + 4);
        t->descent = pg_be16s(p + 6);
        t->line_gap = pg_be16s(p + 8);
        t->num_hmetrics = pg_be16(p + 34);
        if (t->num_hmetrics <= 0) return -1;
        return 0;
}

/* Parse tabel OS/2 (opsional). Berisi sTypoAscender/Descender. */
static int pg_ttf_parse_os2(pg_font_ttf_t *t)
{
        const char *p;
        if (!t->os2_off || t->os2_off + 78 > t->ukuran) return 0;
        p = t->data + t->os2_off;
        t->sTypoAscender = pg_be16s(p + 68);
        t->sTypoDescender = pg_be16s(p + 70);
        t->sTypoLineGap = pg_be16s(p + 72);
        t->usWinAscent = pg_be16(p + 74);
        t->usWinDescent = pg_be16(p + 76);
        return 0;
}

/* ===================================================================
 * Parse tabel maxp
 * =================================================================== */

static int pg_ttf_parse_maxp(pg_font_ttf_t *t)
{
        const char *p;
        if (!t->maxp_off || t->maxp_off + 6 > t->ukuran) return -1;
        p = t->data + t->maxp_off;
        t->num_glyphs = pg_be16(p + 4);
        if (t->num_glyphs <= 0) return -1;
        return 0;
}

/* ===================================================================
 * Parse tabel cmap — cari subtable format 4 (Unicode BMP)
 * =================================================================== */

static int pg_ttf_parse_cmap(pg_font_ttf_t *t)
{
        const char *p;
        int num_subtables;
        int i;
        int best_sub_off = -1;
        int best_platform = -1;

        if (!t->cmap_off) return -1;
        p = t->data + t->cmap_off;
        num_subtables = pg_be16(p + 2);

        for (i = 0; i < num_subtables; i++) {
                int platform = pg_be16(p + 4 + i * 8);
                int encoding = pg_be16(p + 6 + i * 8);
                int sub_off = pg_be32(p + 8 + i * 8);
                int format;
                size_t abs_off;

                /* Cari platform 3 (Microsoft) encoding 1 (Unicode BMP),
                 * atau platform 0 (Unicode). */
                if (platform == 3 && encoding == 1) {
                        if (best_platform < 3) {
                                best_sub_off = sub_off;
                                best_platform = 3;
                        }
                } else if (platform == 0) {
                        if (best_platform < 0) {
                                best_sub_off = sub_off;
                                best_platform = 0;
                        }
                }

                abs_off = t->cmap_off + (size_t)sub_off;
                if (abs_off + 2 > t->ukuran) continue;
                format = pg_be16(t->data + abs_off);
                if (format == 4 && (best_platform == 3 ||
                                     best_platform == 0)) {
                        t->cmap4_off = abs_off;
                        t->cmap4_seg_count = pg_be16(t->data + abs_off + 6) / 2;
                        return 0;
                }
        }
        (void)best_sub_off;
        (void)best_platform;
        return -1;
}

/* ===================================================================
 * Parse tabel kern — subtable format 0 horizontal
 * ----------------------------------------------------------------------------------------------
 * Kern table opsional; banyak font tidak memilikinya. Hanya
 * subtable format 0 (array of pairs) yang diparse. Subtable
 * lain (format 2/3) diabaikan. Pairs dalam format 0 dijamin
 * terurut (left, right) sehingga lookup pakai binary search.
 * =================================================================== */

static int pg_ttf_parse_kern(pg_font_ttf_t *t)
{
        size_t off, len;
        const char *p;
        int version, n_tables;
        int i;
        const char *kend;

        if (pg_ttf_cari_tabel(t, "kern", &off, &len) != 0) {
                /* kern opsional — bukan galat. */
                return 0;
        }
        if (off + 4 > t->ukuran) return -1;
        p = t->data + off;
        kend = t->data + t->ukuran;
        version = pg_be16(p);
        n_tables = pg_be16(p + 2);
        (void)version;
        p += 4;

        for (i = 0; i < n_tables; i++) {
                int sver, slen, scov, fmt, horiz;
                int n_pairs, j;
                const char *sp = p;

                if (sp + 6 > kend) return -1;
                sver  = pg_be16(sp);
                slen  = pg_be16(sp + 2);
                scov  = pg_be16(sp + 4);
                (void)sver;
                fmt   = scov >> 8;
                horiz = scov & 0x01;

                if (fmt == 0 && horiz) {
                        /* Format 0 horizontal — ambil pairs. */
                        if (sp + 14 > kend) return -1;
                        n_pairs = pg_be16(sp + 6);
                        if (n_pairs < 0) return -1;
                        /* Cap defensif: kern pair table > 1M pasti corrupt. */
                        if (n_pairs > 1024 * 1024) return -1;
                        if (sp + 14 + (size_t)n_pairs * 6 > kend)
                                return -1;
                        t->kern_pairs = (pg_kern_pair_t *)malloc(
                                (size_t)n_pairs * sizeof(pg_kern_pair_t));
                        if (!t->kern_pairs) return -1;
                        for (j = 0; j < n_pairs; j++) {
                                const char *pp = sp + 14 + (size_t)j * 6;
                                t->kern_pairs[j].left  = pg_be16(pp);
                                t->kern_pairs[j].right = pg_be16(pp + 2);
                                t->kern_pairs[j].value = pg_be16s(pp + 4);
                        }
                        t->n_kern_pairs = n_pairs;
                        return 0;  /* pakai subtable pertama yg cocok */
                }
                /* Skip ke subtable berikutnya. */
                p += slen;
        }
        return 0;
}

/* ===================================================================
 * cmap format 4 lookup — codepoint → glyph ID
 * =================================================================== */

static int pg_ttf_cmap_lookup(pg_font_ttf_t *t, int codepoint)
{
        const char *p;
        int seg_count;
        const pg_u16 *end_count;
        const pg_u16 *start_count;
        const pg_s16 *id_delta;
        const pg_u16 *id_range_offset;
        int i;

        if (!t->cmap4_off) return 0;
        p = t->data + t->cmap4_off;
        seg_count = t->cmap4_seg_count;
        if (seg_count <= 0) return 0;

        end_count = (const pg_u16 *)(p + 14);
        start_count = (const pg_u16 *)(p + 14 + seg_count * 2 + 2);
        id_delta = (const pg_s16 *)(p + 14 + seg_count * 4 + 2);
        id_range_offset = (const pg_u16 *)(p + 14 + seg_count * 6 + 2);

        for (i = 0; i < seg_count; i++) {
                int ec = pg_be16(&end_count[i]);
                int sc = pg_be16(&start_count[i]);
                if (codepoint >= sc && codepoint <= ec) {
                        int dro = pg_be16(&id_range_offset[i]);
                        if (dro == 0) {
                                return (pg_be16s(&id_delta[i]) +
                                        codepoint) & 0xffff;
                        }
                        {
                                const pg_u8 *glyph_arr;
                                int idx;
                                glyph_arr = (const pg_u8 *)&id_range_offset[i] +
                                              dro + (codepoint - sc) * 2;
                                if ((size_t)((const char *)glyph_arr -
                                     t->data) + 2 > t->ukuran) return 0;
                                idx = pg_be16(glyph_arr);
                                if (idx == 0) return 0;
                                return (idx +
                                        pg_be16s(&id_delta[i])) & 0xffff;
                        }
                }
        }
        return 0;
}

/* ===================================================================
 * Parse glyph outline dari tabel glyf
 * ----------------------------------------------------------------------------------------------
 * Mengembalikan array titik dan array akhir-kontur. Pemanggil
 * bertanggung jawab membebaskan titik dan contour_ends.
 *
 * Mendukung glyph composite (num_contours < 0): komponen direkursi
 * dan outline-nya digabung setelah transform (offset + matriks
 * 2x2 F2Dot14) diterapkan.
 * =================================================================== */

/* Flag komponen composite glyph. */
#define PG_CG_ARG_WORDS  0x0001  /* arg1/arg2 = s16, bukan s8 */
#define PG_CG_ARG_XY     0x0002  /* arg1/arg2 = offset X,Y (FUnits) */
#define PG_CG_ROUND_XY   0x0004  /* ROUND_XY_TO_GRID (diabaikan, no hinting) */
#define PG_CG_HAVE_SCALE 0x0008  /* 1 F2Dot14 (scale seragam) */
#define PG_CG_MORE_COMPS 0x0020  /* ada komponen lain setelah ini */
#define PG_CG_XY_SCALE   0x0040  /* 2 F2Dot14 (xScale, yScale) */
#define PG_CG_AFFINE     0x0080  /* 4 F2Dot14 (xs, s01, s10, ys) */
#define PG_CG_INSTR      0x0100  /* ada instruction stream (diabaikan) */

/* Batas kedalaman rekursi composite (defensif vs font corrupt). */
#define PG_TTF_MAX_RECURSI 8

/* Transform 2x2 + offset. Semua dalam fixed 16.16. */
typedef struct {
        pg_s32 dx, dy;   /* offset (FUnits << 16) */
        pg_s32 mxx, mxy; /* baris pertama matriks (kolom x) */
        pg_s32 myx, myy; /* baris kedua matriks (kolom y) */
} pg_ttf_xform_t;

/* Append count titik ke array dinamis (growing). 0=sukses, -1=OOM. */
static int pg_ttf_push_pts(pg_ttf_titik_t **arr, int *n, int *cap,
                            const pg_ttf_titik_t *src, int count)
{
        int new_cap;
        pg_ttf_titik_t *na;
        if (count <= 0) return 0;
        if (*n + count > *cap) {
                new_cap = (*cap < 16) ? 16 : *cap;
                while (new_cap < *n + count) new_cap *= 2;
                na = (pg_ttf_titik_t *)realloc(*arr,
                        (size_t)new_cap * sizeof(pg_ttf_titik_t));
                if (!na) return -1;
                *arr = na;
                *cap = new_cap;
        }
        memcpy(*arr + *n, src, (size_t)count * sizeof(pg_ttf_titik_t));
        *n += count;
        return 0;
}

/* Append satu int ke array dinamis. 0=sukses, -1=OOM. */
static int pg_ttf_push_int(int **arr, int *n, int *cap, int val)
{
        int new_cap;
        int *na;
        if (*n + 1 > *cap) {
                new_cap = (*cap < 16) ? 16 : *cap;
                while (new_cap < *n + 1) new_cap *= 2;
                na = (int *)realloc(*arr, (size_t)new_cap * sizeof(int));
                if (!na) return -1;
                *arr = na;
                *cap = new_cap;
        }
        (*arr)[*n] = val;
        *n += 1;
        return 0;
}

/* Gabung transform parent dengan transform komponen (offset + 2x2).
 * Hasil: out = parent * comp (aplikasi: out->apply(p) ekuivalen
 * parent->apply(comp->apply(p))). */
static void pg_ttf_xform_combine(const pg_ttf_xform_t *parent,
                                  const pg_ttf_xform_t *comp,
                                  pg_ttf_xform_t *out)
{
        pg_s64 ndx, ndy;
        pg_s64 nmxx, nmxy, nmyx, nmyy;
        /* Offset: parent_offset + parent_matrix * comp_offset. */
        ndx = (pg_s64)parent->mxx * comp->dx + (pg_s64)parent->myx * comp->dy;
        ndy = (pg_s64)parent->mxy * comp->dx + (pg_s64)parent->myy * comp->dy;
        out->dx = parent->dx + (pg_s32)(ndx >> 16);
        out->dy = parent->dy + (pg_s32)(ndy >> 16);
        /* Matriks: parent_matrix * comp_matrix. */
        nmxx = (pg_s64)parent->mxx * comp->mxx +
                (pg_s64)parent->myx * comp->mxy;
        nmxy = (pg_s64)parent->mxy * comp->mxx +
                (pg_s64)parent->myy * comp->mxy;
        nmyx = (pg_s64)parent->mxx * comp->myx +
                (pg_s64)parent->myx * comp->myy;
        nmyy = (pg_s64)parent->mxy * comp->myx +
                (pg_s64)parent->myy * comp->myy;
        out->mxx = (pg_s32)(nmxx >> 16);
        out->mxy = (pg_s32)(nmxy >> 16);
        out->myx = (pg_s32)(nmyx >> 16);
        out->myy = (pg_s32)(nmyy >> 16);
}

/* Apply transform ke titik (in-place). */
static void pg_ttf_xform_pt(pg_s32 *x, pg_s32 *y, const pg_ttf_xform_t *xf)
{
        pg_s64 nx, ny;
        nx = (pg_s64)xf->mxx * (*x) + (pg_s64)xf->myx * (*y);
        ny = (pg_s64)xf->mxy * (*x) + (pg_s64)xf->myy * (*y);
        *x = (pg_s32)(nx >> 16) + xf->dx;
        *y = (pg_s32)(ny >> 16) + xf->dy;
}

/* Recursive: tambahkan outline glyph_id (sudah ditransform xf) ke
 * buffer dinamis out_pts/out_contour_ends. 0=sukses, -1=galat. */
static int pg_ttf_get_outline_rec(pg_font_ttf_t *t, int glyph_id,
                                    int depth, const pg_ttf_xform_t *xf,
                                    pg_ttf_titik_t **out_pts,
                                    int **out_contour_ends,
                                    int *out_n_pts, int *out_n_contours,
                                    int *cap_pts, int *cap_contours)
{
        size_t loca_off;
        size_t glyph_off;
        size_t glyph_len;
        const char *p;
        const char *gend;
        int num_contours;
        int i;

        if (depth > PG_TTF_MAX_RECURSI) return -1;
        if (glyph_id < 0 || glyph_id >= t->num_glyphs) return -1;

        /* Cari offset glyph di loca. */
        if (t->index_to_loc_format == 0) {
                loca_off = t->loca_off + (size_t)glyph_id * 2;
                if (loca_off + 4 > t->ukuran) return -1;
                glyph_off = t->glyf_off +
                            (size_t)pg_be16(t->data + loca_off) * 2;
                glyph_len = (size_t)pg_be16(t->data + loca_off + 2) * 2 -
                            (size_t)pg_be16(t->data + loca_off) * 2;
        } else {
                loca_off = t->loca_off + (size_t)glyph_id * 4;
                if (loca_off + 8 > t->ukuran) return -1;
                glyph_off = t->glyf_off +
                            (size_t)pg_be32(t->data + loca_off);
                glyph_len = (size_t)pg_be32(t->data + loca_off + 4) -
                            (size_t)pg_be32(t->data + loca_off);
        }

        if (glyph_len == 0) return 0;  /* glyph kosong */
        if (glyph_off + glyph_len > t->ukuran) return -1;

        p = t->data + glyph_off;
        gend = p + glyph_len;
        num_contours = pg_be16s(p);

        if (num_contours >= 0) {
                /* ===== Simple glyph ===== */
                pg_ttf_titik_t *pts;
                int *contour_ends;
                int total_pts;
                int instr_len_off, instr_len, flags_off;
                const pg_u8 *flags;
                int *flag_arr;
                int fi = 0;
                int x_bytes = 0, y_bytes = 0;
                const pg_u8 *x_coords, *y_coords;
                int cx_acc = 0, cy_acc = 0;
                int xi = 0, yi = 0;

                if (num_contours == 0) return 0;
                if (p + 10 + (size_t)num_contours * 2 > gend) return -1;
                {
                        const pg_u16 *end_pts = (const pg_u16 *)(p + 10);
                        total_pts = pg_be16(&end_pts[num_contours - 1]) + 1;
                }
                if (total_pts <= 0) return -1;

                pts = (pg_ttf_titik_t *)calloc((size_t)total_pts,
                        sizeof(*pts));
                contour_ends = (int *)calloc((size_t)num_contours,
                        sizeof(int));
                flag_arr = (int *)calloc((size_t)total_pts, sizeof(int));
                if (!pts || !contour_ends || !flag_arr) {
                        free(pts); free(contour_ends); free(flag_arr);
                        return -1;
                }

                {
                        const pg_u16 *end_pts = (const pg_u16 *)(p + 10);
                        for (i = 0; i < num_contours; i++)
                                contour_ends[i] = pg_be16(&end_pts[i]);
                }

                instr_len_off = 10 + num_contours * 2;
                if (p + instr_len_off + 2 > gend) {
                        free(pts); free(contour_ends); free(flag_arr);
                        return -1;
                }
                instr_len = pg_be16(p + instr_len_off);
                flags_off = instr_len_off + 2 + instr_len;
                if (p + flags_off > gend) {
                        free(pts); free(contour_ends); free(flag_arr);
                        return -1;
                }
                flags = (const pg_u8 *)(p + flags_off);

                /* Unroll flag repeats. */
                {
                        int j = 0;
                        while (j < total_pts) {
                                int f, rep;
                                if ((size_t)flags_off + (size_t)fi >=
                                    glyph_len) break;
                                f = flags[fi++];
                                rep = 1;
                                if (f & 0x08) {
                                        if ((size_t)flags_off + (size_t)fi >=
                                            glyph_len) break;
                                        rep += flags[fi++];
                                }
                                while (rep > 0 && j < total_pts) {
                                        flag_arr[j++] = f;
                                        rep--;
                                }
                        }
                }

                for (i = 0; i < total_pts; i++) {
                        int f = flag_arr[i];
                        if (f & 0x02) x_bytes += 1;
                        else if (!(f & 0x10)) x_bytes += 2;
                        if (f & 0x04) y_bytes += 1;
                        else if (!(f & 0x20)) y_bytes += 2;
                }

                x_coords = (const pg_u8 *)(p + flags_off + fi);
                y_coords = x_coords + x_bytes;
                if (p + flags_off + fi + x_bytes + y_bytes > gend) {
                        free(pts); free(contour_ends); free(flag_arr);
                        return -1;
                }

                for (i = 0; i < total_pts; i++) {
                        int f = flag_arr[i];
                        if (f & 0x02) {
                                int v = x_coords[xi++];
                                if (!(f & 0x10)) v = -v;
                                cx_acc += v;
                        } else if (f & 0x10) {
                                /* sama */
                        } else {
                                cx_acc += pg_be16s(&x_coords[xi]);
                                xi += 2;
                        }
                        pts[i].x = cx_acc << 16;
                        pts[i].on_curve = (f & 0x01) ? PG_BENAR : PG_SALAH;
                }
                for (i = 0; i < total_pts; i++) {
                        int f = flag_arr[i];
                        if (f & 0x04) {
                                int v = y_coords[yi++];
                                if (!(f & 0x20)) v = -v;
                                cy_acc += v;
                        } else if (f & 0x20) {
                                /* sama */
                        } else {
                                cy_acc += pg_be16s(&y_coords[yi]);
                                yi += 2;
                        }
                        pts[i].y = cy_acc << 16;
                }
                free(flag_arr);

                /* Apply transform ke semua titik. */
                for (i = 0; i < total_pts; i++)
                        pg_ttf_xform_pt(&pts[i].x, &pts[i].y, xf);

                /* Append ke buffer dinamis. */
                if (pg_ttf_push_pts(out_pts, out_n_pts, cap_pts,
                                    pts, total_pts) != 0) {
                        free(pts); free(contour_ends); return -1;
                }
                for (i = 0; i < num_contours; i++) {
                        if (pg_ttf_push_int(out_contour_ends,
                                    out_n_contours, cap_contours,
                                    contour_ends[i] + *out_n_pts - total_pts)
                            != 0) {
                                free(pts); free(contour_ends); return -1;
                        }
                }
                /* Setelah push_int, *out_n_pts tidak berubah karena
                 * push_int hanya menambah kontur. Tapi kita perlu
                 * ingat offset sebelum push_pts supaya contour_ends
                 * benar — di atas kita pakai *out_n_pts - total_pts
                 * (base index sebelum titik baru ditambah). */
                free(pts);
                free(contour_ends);
                return 0;
        }

        /* ===== Composite glyph =====
         * Walk component records (mulai dari p+10, setelah header). */
        {
                const char *cp = p + 10;
                int rc = 0;
                while (cp + 4 <= gend) {
                        pg_u16 flags;
                        pg_u16 comp_gid;
                        pg_s32 arg1, arg2;
                        pg_ttf_xform_t comp_xf;
                        pg_ttf_xform_t combined;
                        const char *np;

                        flags   = pg_be16(cp);
                        comp_gid = pg_be16(cp + 2);
                        np = cp + 4;

                        /* Baca arg1, arg2. */
                        if (flags & PG_CG_ARG_WORDS) {
                                if (np + 4 > gend) return -1;
                                arg1 = pg_be16s(np);
                                arg2 = pg_be16s(np + 2);
                                np += 4;
                        } else {
                                if (np + 2 > gend) return -1;
                                arg1 = (pg_s8)np[0];
                                arg2 = (pg_s8)np[1];
                                np += 2;
                        }

                        /* Transform komponen (default identitas). */
                        comp_xf.mxx = 0x10000; comp_xf.mxy = 0;
                        comp_xf.myx = 0;       comp_xf.myy = 0x10000;

                        /* Baca scale/affine bila ada. */
                        if (flags & PG_CG_HAVE_SCALE) {
                                pg_s16 s;
                                if (np + 2 > gend) return -1;
                                s = pg_be16s(np);
                                np += 2;
                                /* F2Dot14 → 16.16: s * (1<<16) / (1<<14) = s<<2 */
                                comp_xf.mxx = (pg_s32)s << 2;
                                comp_xf.myy = (pg_s32)s << 2;
                        } else if (flags & PG_CG_XY_SCALE) {
                                pg_s16 sx, sy;
                                if (np + 4 > gend) return -1;
                                sx = pg_be16s(np);
                                sy = pg_be16s(np + 2);
                                np += 4;
                                comp_xf.mxx = (pg_s32)sx << 2;
                                comp_xf.myy = (pg_s32)sy << 2;
                        } else if (flags & PG_CG_AFFINE) {
                                pg_s16 a, b, c, d;
                                if (np + 8 > gend) return -1;
                                a = pg_be16s(np);
                                b = pg_be16s(np + 2);
                                c = pg_be16s(np + 4);
                                d = pg_be16s(np + 6);
                                np += 8;
                                comp_xf.mxx = (pg_s32)a << 2;
                                comp_xf.mxy = (pg_s32)b << 2;
                                comp_xf.myx = (pg_s32)c << 2;
                                comp_xf.myy = (pg_s32)d << 2;
                        }

                        /* Offset komponen (hanya ARGS_ARE_XY_VALUES
                         * yang umum; untuk point-match kita tetap
                         * pakai sebagai offset best-effort). */
                        comp_xf.dx = arg1 << 16;
                        comp_xf.dy = arg2 << 16;

                        /* Gabung dengan transform parent. */
                        pg_ttf_xform_combine(xf, &comp_xf, &combined);

                        /* Rekursi: ambil outline komponen dengan
                         * transform gabungan. */
                        rc = pg_ttf_get_outline_rec(t, comp_gid, depth + 1,
                                &combined, out_pts, out_contour_ends,
                                out_n_pts, out_n_contours,
                                cap_pts, cap_contours);
                        if (rc != 0) return rc;

                        if (!(flags & PG_CG_MORE_COMPS)) break;
                        cp = np;
                }
                return rc;
        }
}

/* Public: ambil outline satu glyph (identitas transform). */
static int pg_ttf_get_outline(pg_font_ttf_t *t, int glyph_id,
                                pg_ttf_titik_t **out_pts,
                                int **out_contour_ends,
                                int *out_num_pts,
                                int *out_num_contours)
{
        pg_ttf_titik_t *pts = NULL;
        int *contour_ends = NULL;
        int n_pts = 0, n_contours = 0;
        int cap_pts = 0, cap_contours = 0;
        pg_ttf_xform_t id;
        int rc;

        id.dx = 0; id.dy = 0;
        id.mxx = 0x10000; id.mxy = 0;
        id.myx = 0;       id.myy = 0x10000;

        rc = pg_ttf_get_outline_rec(t, glyph_id, 0, &id,
                &pts, &contour_ends, &n_pts, &n_contours,
                &cap_pts, &cap_contours);
        if (rc != 0) {
                free(pts);
                free(contour_ends);
                return rc;
        }
        /* Shrink-to-fit (optional; hemat memori). */
        if (n_pts > 0) {
                pg_ttf_titik_t *np = (pg_ttf_titik_t *)realloc(pts,
                        (size_t)n_pts * sizeof(pg_ttf_titik_t));
                if (np) pts = np;
        }
        if (n_contours > 0) {
                int *nc = (int *)realloc(contour_ends,
                        (size_t)n_contours * sizeof(int));
                if (nc) contour_ends = nc;
        }
        *out_pts = pts;
        *out_contour_ends = contour_ends;
        *out_num_pts = n_pts;
        *out_num_contours = n_contours;
        return 0;
}

/* ===================================================================
 * Ambil advance width dari hmtx
 * =================================================================== */

static int pg_ttf_get_advance(pg_font_ttf_t *t, int glyph_id)
{
        size_t off;
        if (glyph_id < t->num_hmetrics) {
                off = t->hmtx_off + (size_t)glyph_id * 4;
                if (off + 2 > t->ukuran) return 0;
                return pg_be16(t->data + off);
        }
        /* Jika glyph_id >= num_hmetrics, pakai advance terakhir. */
        off = t->hmtx_off + (size_t)(t->num_hmetrics - 1) * 4;
        if (off + 2 > t->ukuran) return 0;
        return pg_be16(t->data + off);
}

/* ===================================================================
 * Flatten kontur: konversi off-curve points ke kurva Bezier
 * ----------------------------------------------------------------------------------------------
 * TTF pakai Bezier kuadratik. Antara dua on-curve points boleh ada
 * 1 off-curve point (control point). Konvensi: bila dua off-curve
 * point berurutan, on-curve point implisit di tengah mereka.
 * ---------------------------------------------------------------------------------------------- */

/* Render satu kontur ke segmen garis. pts = titik kontur, n = jumlah.
 * Iteratif (tanpa rekursi). Tidak memodifikasi array input pts.
 * Output ditulis ke out[*out_n..], *out_n diincrement. */
static void pg_ttf_flatten_kontur(pg_ttf_titik_t *pts, int n,
                                    pg_ttf_titik_t *out, int *out_n)
{
        int i;
        int start_idx;
        pg_ttf_titik_t start_pt;
        pg_ttf_titik_t last_on_pt;

        if (n <= 0) return;

        /* Cari titik on-curve pertama. */
        start_idx = -1;
        for (i = 0; i < n; i++) {
                if (pts[i].on_curve) { start_idx = i; break; }
        }
        if (start_idx < 0) {
                /* Semua titik off-curve: titik on-curve implisit di
                 * midpoint antara titik pertama dan terakhir. Mulai
                 * iterasi dari titik pertama (start_idx = n-1, agar
                 * loop i=1..n memproses pts[0..n-1] dan kontur menutup
                 * kembali ke start_pt). */
                start_pt.x = (pts[0].x + pts[n - 1].x) / 2;
                start_pt.y = (pts[0].y + pts[n - 1].y) / 2;
                start_pt.on_curve = PG_BENAR;
                start_idx = n - 1;
        } else {
                start_pt = pts[start_idx];
        }

        /* Emit titik mulai (on-curve). */
        out[*out_n] = start_pt;
        (*out_n)++;
        last_on_pt = start_pt;

        /* Iterasi titik dari start_idx+1 hingga start_idx+n (close). */
        for (i = 1; i <= n; i++) {
                int idx = (start_idx + i) % n;
                if (pts[idx].on_curve) {
                        /* On-curve: emit langsung. */
                        out[*out_n] = pts[idx];
                        (*out_n)++;
                        last_on_pt = pts[idx];
                } else {
                        /* Off-curve: cari on-curve berikutnya (atau
                         * buat implisit midpoint bila berurutan
                         * off-curve). */
                        int next_idx = (idx + 1) % n;
                        pg_ttf_titik_t next_on;
                        if (pts[next_idx].on_curve) {
                                /* Konsumsi next_idx juga. */
                                next_on = pts[next_idx];
                                i++;
                        } else {
                                /* Implisit on-curve di tengah pts[idx]
                                 * dan pts[next_idx]. next_idx diproses
                                 * iterasi berikutnya sebagai off-curve
                                 * baru dengan last_on_pt = midpoint. */
                                next_on.x = (pts[idx].x +
                                              pts[next_idx].x) / 2;
                                next_on.y = (pts[idx].y +
                                              pts[next_idx].y) / 2;
                                next_on.on_curve = PG_BENAR;
                        }
                        /* Flatten Bezier kuadratik P0=last_on_pt,
                         * P1=pts[idx], P2=next_on.
                         * Adaptive: 1 step per ~4px panjang kurva. */
                        {
                                pg_s32 x0 = last_on_pt.x;
                                pg_s32 y0 = last_on_pt.y;
                                pg_s32 x1 = pts[idx].x;
                                pg_s32 y1 = pts[idx].y;
                                pg_s32 x2 = next_on.x;
                                pg_s32 y2 = next_on.y;
                                int dx1, dy1, dx2, dy2, approx_len;
                                int steps, s;
                                dx1 = (int)((x1 - x0) >> 16);
                                dy1 = (int)((y1 - y0) >> 16);
                                dx2 = (int)((x2 - x1) >> 16);
                                dy2 = (int)((y2 - y1) >> 16);
                                if (dx1 < 0) dx1 = -dx1;
                                if (dy1 < 0) dy1 = -dy1;
                                if (dx2 < 0) dx2 = -dx2;
                                if (dy2 < 0) dy2 = -dy2;
                                approx_len = dx1 + dy1 + dx2 + dy2;
                                steps = approx_len / 4 + 2;
                                if (steps < 3) steps = 3;
                                if (steps > 16) steps = 16;
                                for (s = 1; s < steps; s++) {
                                        pg_s32 t_cur = ((pg_s32)s << 16) /
                                                       steps;
                                        pg_s32 it_cur = 0x10000 - t_cur;
                                        pg_s64 it2_cur, t2_cur, itt_cur;
                                        pg_s32 bx, by;
                                        it2_cur = ((pg_s64)it_cur * it_cur)
                                                  >> 16;
                                        t2_cur = ((pg_s64)t_cur * t_cur)
                                                  >> 16;
                                        itt_cur = ((pg_s64)it_cur * t_cur)
                                                  >> 16;
                                        bx = (pg_s32)((it2_cur * x0 +
                                                       2 * itt_cur * x1 +
                                                       t2_cur * x2) >> 16);
                                        by = (pg_s32)((it2_cur * y0 +
                                                       2 * itt_cur * y1 +
                                                       t2_cur * y2) >> 16);
                                        out[*out_n].x = bx;
                                        out[*out_n].y = by;
                                        out[*out_n].on_curve = PG_BENAR;
                                        (*out_n)++;
                                }
                        }
                        out[*out_n] = next_on;
                        (*out_n)++;
                        last_on_pt = next_on;
                }
        }
}

/* ===================================================================
 * Rasterisasi AA dengan supersampling 4x
 * ----------------------------------------------------------------------------------------------
 * Outline sudah dalam koordinat piksel (fixed 16.16). Render di
 * buffer 4x lalu downsample.
 * =================================================================== */

static void pg_ttf_rasterize(pg_ttf_titik_t *outline, int *kontur_akhir,
                              int jumlah_kontur, int num_pts,
                              int lebar_out, int tinggi_out,
                              int min_x, int min_y,
                              pg_u8 *bitmap_out)
{
        int SS = 4;  /* 4x4 = 16 sample, cukup + cepat. */
        int sw = lebar_out * SS;
        int sh = tinggi_out * SS;
        pg_u8 *super;
        int y, x, c, i;
        int *xint;
        int xint_count;

        if (lebar_out <= 0 || tinggi_out <= 0) return;
        (void)num_pts;
        super = (pg_u8 *)calloc((size_t)sw * sh, 1);
        if (!super) return;
        xint = (int *)malloc(sizeof(int) * num_pts * 2);
        if (!xint) { free(super); return; }

        /* Scale outline ke resolusi supersample dan offset ke origin.
         * Pakai rounding (+0.5) supaya scanline di subpixel 0, 0.25,
         * 0.5, 0.75 rata, tidak bergelombang. */
        for (y = 0; y < sh; y++) {
                pg_s32 sy = (pg_s32)((min_y << 16) +
                            (((pg_s64)y << 16) + (1 << 15)) / SS);
                xint_count = 0;

                for (c = 0; c < jumlah_kontur; c++) {
                        int start = (c == 0) ? 0 : kontur_akhir[c - 1] + 1;
                        int end = kontur_akhir[c];
                        for (i = start; i <= end; i++) {
                                pg_s32 y0 = outline[i].y;
                                pg_s32 y1 = outline[(i == end) ?
                                         start : i + 1].y;
                                pg_s32 x0 = outline[i].x;
                                pg_s32 x1 = outline[(i == end) ?
                                         start : i + 1].x;
                                /* Edge crosses scanline sy? */
                                if ((y0 <= sy && y1 > sy) ||
                                    (y1 <= sy && y0 > sy)) {
                                        pg_s64 dx = x1 - x0;
                                        pg_s64 dy = y1 - y0;
                                        pg_s32 xs_fixed;
                                        int xs_sub;
                                        if (dy == 0) dy = 1;
                                        xs_fixed = x0 + (pg_s32)(
                                                ((pg_s64)(sy - y0) * dx) /
                                                dy);
                                        /* Konversi fixed 16.16 ke
                                         * sub-piksel supersample dengan
                                         * rounding (+0.5). */
                                        xs_sub = (int)(((pg_s64)(
                                                xs_fixed -
                                                (pg_s32)(min_x << 16))
                                                * SS + (1 << 15)) >> 16);
                                        xint[xint_count++] = xs_sub;
                                }
                        }
                }

                /* Sort x intersections. */
                {
                        int a, b;
                        for (a = 1; a < xint_count; a++) {
                                int key = xint[a];
                                b = a - 1;
                                while (b >= 0 && xint[b] > key) {
                                        xint[b + 1] = xint[b];
                                        b--;
                                }
                                xint[b + 1] = key;
                        }
                }

                /* Fill pairs (even-odd rule). */
                for (i = 0; i + 1 < xint_count; i += 2) {
                        int x0 = xint[i];
                        int x1 = xint[i + 1];
                        if (x0 < 0) x0 = 0;
                        if (x1 > sw) x1 = sw;
                        for (x = x0; x < x1; x++) {
                                super[y * sw + x] = 1;
                        }
                }
        }

        /* Downsample NxN box filter → alpha 0..255.
         * Untuk font kecil (<=20px): linear, tanpa gamma — sqrt bikin
         * stem 1px coverage 0.25 jadi alpha 128, kelihatan abu tebal.
         * Untuk font besar (>20px): gamma sqrt(coverage) untuk sRGB. */
        for (y = 0; y < tinggi_out; y++) {
                for (x = 0; x < lebar_out; x++) {
                        int total = 0;
                        int sy, sx;
                        for (sy = 0; sy < SS; sy++) {
                                for (sx = 0; sx < SS; sx++) {
                                        total += super[(y * SS + sy) * sw +
                                                        x * SS + sx];
                                }
                        }
                        {
                                int alpha;
                                if (tinggi_out < 24) {
                                        /* Font kecil: linear. */
                                        alpha = total * 255 /
                                                (SS * SS);
                                } else {
                                        /* Font besar: gamma correct. */
                                        float cov = (float)total /
                                                     (float)(SS * SS);
                                        float a = sqrtf(cov);
                                        alpha = (int)(a * 255.0f + 0.5f);
                                }
                                if (alpha > 255) alpha = 255;
                                bitmap_out[y * lebar_out + x] =
                                        (pg_u8)alpha;
                        }
                }
        }

        free(super);
        free(xint);
}

/* ===================================================================
 * Glyph cache — cari / insert
 * =================================================================== */

static pg_ttf_glyph_cache_t *pg_ttf_cache_cari(pg_font_ttf_t *t,
                                                int glyph_id, int ukuran)
{
        int i;
        for (i = 0; i < PG_TTF_CACHE_SIZE; i++) {
                if (t->cache[i].glyph_id == glyph_id &&
                    t->cache[i].ukuran_px == ukuran &&
                    t->cache[i].bitmap) {
                        t->cache[i].ref_count++;
                        return &t->cache[i];
                }
        }
        return NULL;
}

static pg_ttf_glyph_cache_t *pg_ttf_cache_masuk(pg_font_ttf_t *t,
                                                  int glyph_id,
                                                  int ukuran)
{
        int slot = t->cache_next;
        /* Cari slot kosong atau LRU. */
        int i;
        int min_ref = 0x7fffffff;
        for (i = 0; i < PG_TTF_CACHE_SIZE; i++) {
                if (!t->cache[i].bitmap) { slot = i; break; }
                if (t->cache[i].ref_count < min_ref) {
                        min_ref = t->cache[i].ref_count;
                        slot = i;
                }
        }
        if (t->cache[slot].bitmap) {
                free(t->cache[slot].bitmap);
                memset(&t->cache[slot], 0, sizeof(t->cache[slot]));
        }
        t->cache_next = (slot + 1) % PG_TTF_CACHE_SIZE;
        t->cache[slot].glyph_id = glyph_id;
        t->cache[slot].ukuran_px = ukuran;
        t->cache[slot].ref_count = 1;
        return &t->cache[slot];
}

/* ===================================================================
 * Render glyph ke bitmap AA
 * =================================================================== */

static int pg_ttf_render_glyph(pg_font_ttf_t *t, int glyph_id,
                                int ukuran_px,
                                pg_ttf_glyph_cache_t *cache)
{
        pg_ttf_titik_t *outline = NULL;
        int *kontur_akhir = NULL;
        int num_pts = 0, num_kontur = 0;
        pg_ttf_titik_t *flattened;
        int *flat_kontur_akhir = NULL;
        int flat_count = 0;
        int advance;
        int i, c;
        int min_x = 0x7fffffff, min_y = 0x7fffffff;
        int max_x = -0x7fffffff, max_y = -0x7fffffff;
        int lebar, tinggi;
        pg_s32 scale;

        if (pg_ttf_get_outline(t, glyph_id, &outline, &kontur_akhir,
                                &num_pts, &num_kontur) != 0) {
                return -1;
        }
        if (num_pts == 0 || num_kontur == 0) {
                /* Glyph kosong (space). */
                advance = pg_ttf_get_advance(t, glyph_id);
                cache->bitmap = NULL;
                cache->lebar = 0;
                cache->tinggi = 0;
                cache->x_offset = 0;
                cache->y_offset = 0;
                cache->advance = (advance * ukuran_px) /
                                  t->units_per_em;
                free(outline);
                free(kontur_akhir);
                return 0;
        }

        /* Scale: unit font → piksel. */
        scale = ((pg_s32)ukuran_px << 16) / t->units_per_em;

        /* Flatten semua kontur.
         *
         * Buffer: setiap titik off-curve menghasilkan hingga 8
         * sub-sample Bezier + 1 titik akhir. Dengan adaptive steps
         * (max 16 per Bezier), upper bound per off-curve point adalah
         * 16+1=17. Total = 20*(num_pts+num_kontur) untuk margin. */
        flattened = (pg_ttf_titik_t *)malloc((size_t)(num_pts +
                                                      num_kontur) *
                                              20 * sizeof(*flattened));
        if (!flattened) { free(outline); free(kontur_akhir); return -1; }

        /* Array end-of-kontur untuk array flattened (bukan outline
         * asli), supaya rasterizer tahu batas tiap kontur setelah
         * subdivisi Bezier. Tanpa ini, rasterizer memakai indeks
         * outline asli yang lebih kecil dari flattened count, sehingga
         * sebagian besar segmen Bezier tidak diproses. */
        flat_kontur_akhir = (int *)malloc((size_t)num_kontur *
                                           sizeof(int));
        if (!flat_kontur_akhir) {
                free(outline); free(kontur_akhir); free(flattened);
                return -1;
        }

        for (c = 0; c < num_kontur; c++) {
                int start = (c == 0) ? 0 : kontur_akhir[c - 1] + 1;
                int end = kontur_akhir[c];
                int n = end - start + 1;
                pg_ttf_titik_t *sub = outline + start;
                pg_ttf_flatten_kontur(sub, n, flattened, &flat_count);
                flat_kontur_akhir[c] = flat_count - 1;
        }

        /* Scale dan cari bbox. */
        for (i = 0; i < flat_count; i++) {
                flattened[i].x = (pg_s32)(((pg_s64)flattened[i].x *
                                            scale) >> 16);
                flattened[i].y = (pg_s32)(((pg_s64)flattened[i].y *
                                            scale) >> 16);
                if (flattened[i].x < min_x) min_x = flattened[i].x;
                if (flattened[i].y < min_y) min_y = flattened[i].y;
                if (flattened[i].x > max_x) max_x = flattened[i].x;
                if (flattened[i].y > max_y) max_y = flattened[i].y;
        }

        /* Invert Y: TTF y-up → permukaan y-down.
         * Pivot di ascent (dari hhea), BUKAN di max_y glyph.
         * Setelah invert: y=0 = font top (ascent), y=ascent = baseline.
         *
         * Bitmap = glyph bbox saja (hemat memory), jadi min_y
         * setelah invert = ascent - max_y_old (glyph top).
         *
         * y_offset = min_y - ascent (per-glyph, tapi baseline
         * konsisten karena min_y = ascent - max_y_old):
         *   screen_y = baseline + py + y_offset
         *   top glyph (py=0): baseline + 0 + (min_y - ascent)
         *                    = baseline + (ascent - max_y_old) - ascent
         *                    = baseline - max_y_old ✓
         */
        {
                pg_s32 ascent_fixed;
                ascent_fixed = (pg_s32)(((pg_s64)t->ascent * scale) >> 16);
                for (i = 0; i < flat_count; i++) {
                        flattened[i].y = ascent_fixed - flattened[i].y;
                }
                /* Recompute bbox setelah invert. */
                min_y = 0x7fffffff;
                max_y = -0x7fffffff;
                for (i = 0; i < flat_count; i++) {
                        if (flattened[i].y < min_y) min_y = flattened[i].y;
                        if (flattened[i].y > max_y) max_y = flattened[i].y;
                }
                /* y_offset: bitmap top (py=0) relatif ke baseline.
                 * min_y = ascent - max_y_old (glyph top setelah invert).
                 * y_offset = min_y - ascent = -max_y_old. */
                cache->y_offset = (int)(min_y >> 16) -
                                   (int)(ascent_fixed >> 16);
        }

        /* Convert min_x/min_y dari fixed ke int. */
        min_x >>= 16;
        min_y >>= 16;
        max_x = (max_x >> 16) + 1;
        max_y = (max_y >> 16) + 1;

        lebar = max_x - min_x + 2;
        tinggi = max_y - min_y + 2;
        if (lebar <= 0 || tinggi <= 0) lebar = tinggi = 0;

        if (lebar > 0 && tinggi > 0) {
                cache->bitmap = (pg_u8 *)calloc((size_t)lebar * tinggi, 1);
                if (!cache->bitmap) {
                        free(outline);
                        free(kontur_akhir);
                        free(flattened);
                        free(flat_kontur_akhir);
                        return -1;
                }
                pg_ttf_rasterize(flattened, flat_kontur_akhir,
                                  num_kontur, flat_count, lebar, tinggi,
                                  min_x, min_y, cache->bitmap);
        } else {
                cache->bitmap = NULL;
        }

        cache->lebar = lebar;
        cache->tinggi = tinggi;
        cache->x_offset = min_x;
        /* y_offset sudah di-set di blok invert y (=-ascent_piksel).
         * Jangan timpa dengan min_y (=0 setelah flip). */
        advance = pg_ttf_get_advance(t, glyph_id);
        cache->advance = (advance * ukuran_px) / t->units_per_em;
        if (cache->advance < 1 && advance > 0)
                cache->advance = 1;

        free(outline);
        free(kontur_akhir);
        free(flattened);
        free(flat_kontur_akhir);
        return 0;
}

/* ===================================================================
 * Public API: muat font TTF dari file
 * =================================================================== */

pg_font_t *pg_buat_font_ttf(const char *path, int ukuran_px)
{
        pg_font_ttf_t *t;
        FILE *fp;
        long file_size;
        size_t dibaca;
        pg_font_t *font;

        if (!path || ukuran_px <= 0) {
                pg_set_galat(PG_GALAT_ARGUMEN, "buat_font_ttf: argumen "
                             "buruk");
                return NULL;
        }
        if (ukuran_px < 6) {
                pg_set_galat(PG_GALAT_ARGUMEN, "buat_font_ttf: "
                             "ukuran_px=%d terlalu kecil (minimum 6)",
                             ukuran_px);
                return NULL;
        }

        fp = fopen(path, "rb");
        if (!fp) {
                pg_set_galat(PG_GALAT_IO, "buat_font_ttf: tidak bisa "
                             "buka %s", path);
                return NULL;
        }
        fseek(fp, 0, SEEK_END);
        file_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        if (file_size <= 0 || file_size > 50 * 1024 * 1024) {
                fclose(fp);
                pg_set_galat(PG_GALAT_ARGUMEN, "buat_font_ttf: ukuran "
                             "file tidak masuk akal: %ld", file_size);
                return NULL;
        }

        t = (pg_font_ttf_t *)calloc(1, sizeof(*t));
        if (!t) { fclose(fp); return NULL; }
        t->ukuran = (size_t)file_size;
        t->data = (char *)malloc(t->ukuran);
        if (!t->data) { free(t); fclose(fp); return NULL; }

        dibaca = fread(t->data, 1, t->ukuran, fp);
        fclose(fp);
        if (dibaca != t->ukuran) {
                free(t->data);
                free(t);
                pg_set_galat(PG_GALAT_IO, "buat_font_ttf: baca gagal");
                return NULL;
        }

        /* Validasi signature. */
        if (t->ukuran < 12) {
                pg_set_galat(PG_GALAT_ARGUMEN, "file terlalu kecil");
                goto gagal;
        }
        {
                pg_u32 sig = pg_be32(t->data);
                if (sig != 0x00010000 && sig != 0x74727565 /* 'true' */ &&
                    sig != 0x4F54544F /* 'OTTO' */) {
                        pg_set_galat(PG_GALAT_ARGUMEN, "signature TTF "
                                     "tidak dikenal: 0x%08x", sig);
                        goto gagal;
                }
        }

        /* Cari semua tabel. */
        if (pg_ttf_cari_tabel(t, "head", &t->head_off, &t->head_len) ||
            pg_ttf_cari_tabel(t, "hhea", &t->hhea_off, &t->hhea_len) ||
            pg_ttf_cari_tabel(t, "maxp", &t->maxp_off, &t->maxp_len) ||
            pg_ttf_cari_tabel(t, "cmap", &t->cmap_off, &t->cmap_len) ||
            pg_ttf_cari_tabel(t, "hmtx", &t->hmtx_off, &t->hmtx_len)) {
                pg_set_galat(PG_GALAT_UMUM, "tabel TTF wajib tidak "
                             "lengkap");
                goto gagal;
        }
        /* OS/2 opsional — bila ada, override ascent/descent. */
        pg_ttf_cari_tabel(t, "OS/2", &t->os2_off, &t->os2_len);
        if (t->os2_off) {
                pg_ttf_parse_os2(t);
                if (t->sTypoAscender != 0 || t->sTypoDescender != 0) {
                        t->ascent = t->sTypoAscender;
                        t->descent = t->sTypoDescender;
                        t->line_gap = t->sTypoLineGap;
                }
        }

        /* Tabel glyf wajib untuk TrueType outlines. Bila tidak ada
         * dan tabel CFF ada, font tersebut OpenType PostScript (CFF)
         * yang belum didukung — tolak dengan pesan jelas, bukan
         * crash atau galat generik. */
        if (pg_ttf_cari_tabel(t, "glyf", &t->glyf_off,
                              &t->glyf_len) != 0) {
                size_t cff_off_dummy, cff_len_dummy;
                if (pg_ttf_cari_tabel(t, "CFF ", &cff_off_dummy,
                                      &cff_len_dummy) == 0) {
                        pg_set_galat(PG_GALAT_TANPA, "Font CFF (OpenType "
                                     "PostScript) belum didukung. Pakai font "
                                     "TrueType (.ttf).");
                } else {
                        pg_set_galat(PG_GALAT_UMUM, "tabel glyf TTF tidak "
                                     "ditemukan");
                }
                goto gagal;
        }

        if (pg_ttf_cari_tabel(t, "loca", &t->loca_off, &t->loca_len)) {
                pg_set_galat(PG_GALAT_UMUM, "tabel TTF wajib tidak "
                             "lengkap (loca)");
                goto gagal;
        }

        if (pg_ttf_parse_head(t) || pg_ttf_parse_hhea(t) ||
            pg_ttf_parse_maxp(t) || pg_ttf_parse_cmap(t)) {
                pg_set_galat(PG_GALAT_UMUM, "parse tabel gagal");
                goto gagal;
        }

        /* Parse kern (opsional; gagal hanya bila tabel korup). */
        if (pg_ttf_parse_kern(t) != 0) {
                pg_set_galat(PG_GALAT_UMUM, "parse tabel kern gagal");
                goto gagal;
        }

        pg_info("ttf: %s — units_per_em=%d, glyphs=%d, ukuran_px=%d, "
                "kern_pairs=%d",
                path, t->units_per_em, t->num_glyphs, ukuran_px,
                t->n_kern_pairs);

        /* Wrap sebagai pg_font_t. */
        font = (pg_font_t *)calloc(1, sizeof(pg_font_t));
        if (!font) goto gagal;
        font->impl = t;
        font->jenis = PG_FONT_JENIS_TTF;
        font->ukuran_px = ukuran_px;
        return font;

gagal:
        free(t->data);
        free(t);
        return NULL;
}

/* ===================================================================
 * Public API TTF — dipanggil oleh dispatcher di font_bitmap.c
 * =================================================================== */

pg_galat pg_ttf_glyph(const pg_font_ttf_t *t, pg_u32 kode,
                       pg_glyph_t *metrik, int ukuran_px)
{
        int glyph_id;
        pg_ttf_glyph_cache_t *cache;
        if (!t || !metrik) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
        glyph_id = pg_ttf_cmap_lookup((pg_font_ttf_t *)t, (int)kode);
        cache = pg_ttf_cache_cari((pg_font_ttf_t *)t, glyph_id, ukuran_px);
        if (!cache) {
                cache = pg_ttf_cache_masuk((pg_font_ttf_t *)t,
                                            glyph_id, ukuran_px);
                if (!cache) PG_KEMBALI_GALAT(PG_GALAT_MEMORI);
                pg_ttf_render_glyph((pg_font_ttf_t *)t, glyph_id,
                                     ukuran_px, cache);
        }
        metrik->lebar    = cache->lebar;
        metrik->tinggi   = cache->tinggi;
        metrik->x_offset = cache->x_offset;
        metrik->y_offset = cache->y_offset;
        metrik->advance  = cache->advance;
        return PG_OK;
}

pg_galat pg_ttf_gambar(const pg_font_ttf_t *t, pg_u32 kode,
                        pg_permukaan_t *s, int x, int y,
                        pg_warna_t c, int ukuran_px)
{
        int glyph_id;
        pg_ttf_glyph_cache_t *cache;
        int px, py;
        int lebar_s, tinggi_s, langkah_s;
        if (!t || !s) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
        glyph_id = pg_ttf_cmap_lookup((pg_font_ttf_t *)t, (int)kode);
        cache = pg_ttf_cache_cari((pg_font_ttf_t *)t, glyph_id, ukuran_px);
        if (!cache) {
                cache = pg_ttf_cache_masuk((pg_font_ttf_t *)t,
                                            glyph_id, ukuran_px);
                if (!cache) PG_KEMBALI_GALAT(PG_GALAT_MEMORI);
                pg_ttf_render_glyph((pg_font_ttf_t *)t, glyph_id,
                                     ukuran_px, cache);
        }
        if (!cache->bitmap || cache->lebar <= 0 || cache->tinggi <= 0)
                return PG_OK;

        lebar_s = pg_permukaan_lebar(s);
        tinggi_s = pg_permukaan_tinggi(s);
        langkah_s = pg_permukaan_langkah(s);

        for (py = 0; py < cache->tinggi; py++) {
                pg_warna_t *drow;
                int dy = y + py + cache->y_offset;
                if (dy < 0 || dy >= tinggi_s) continue;
                drow = (pg_warna_t *)((char *)pg_permukaan_piksel_mut(s)
                                       + (size_t)dy * langkah_s);
                for (px = 0; px < cache->lebar; px++) {
                        int dx = x + px + cache->x_offset;
                        pg_u8 alpha;
                        if (dx < 0 || dx >= lebar_s) continue;
                        alpha = cache->bitmap[py * cache->lebar + px];
                        if (alpha == 0) continue;
                        /* Composite alpha: coverage font (alpha) *
                         * fg alpha (PG_A(c)). Sama seperti
                         * pg_blend_coverage di gambar.c. */
                        {
                                int fg_a = PG_A(c);
                                int fa = (alpha * fg_a) / 255;
                                if (fa <= 0) continue;
                                if (fa >= 255) {
                                        drow[dx] = c;
                                } else {
                                        pg_warna_t bg = drow[dx];
                                        int r, g, b, a;
                                        int ia = 255 - fa;
                                        r = (PG_R(c)*fa +
                                             PG_R(bg)*ia)/255;
                                        g = (PG_G(c)*fa +
                                             PG_G(bg)*ia)/255;
                                        b = (PG_B(c)*fa +
                                             PG_B(bg)*ia)/255;
                                        a = fa + (PG_A(bg)*ia)/255;
                                        drow[dx] =
                                            PG_RGBA(r,g,b,a);
                                }
                        }
                }
        }
        return PG_OK;
}

int pg_ttf_lebar_teks(const pg_font_ttf_t *t, const char *teks,
                      int ukuran_px)
{
        const char *p;
        int total = 0;
        int prev_gid = -1;
        if (!t || !teks) return 0;
        for (p = teks; *p; p++) {
                int glyph_id = pg_ttf_cmap_lookup((pg_font_ttf_t *)t,
                                                   (int)(unsigned char)*p);
                int adv = pg_ttf_get_advance((pg_font_ttf_t *)t,
                                              glyph_id);
                /* Kerning antar glyph bertetangga. */
                if (prev_gid >= 0) {
                        pg_s16 k = pg_ttf_kerning(t, prev_gid, glyph_id);
                        total += (k * ukuran_px) / t->units_per_em;
                }
                total += (adv * ukuran_px) / t->units_per_em;
                prev_gid = glyph_id;
        }
        return total;
}

/* Lookup kerning pair (FUnits). Mengembalikan 0 bila tidak ada
 * entry kerning atau kern table tidak ada. Pairs di tabel format 0
 * dijamin terurut (left, right) → binary search O(log n). */
pg_s16 pg_ttf_kerning(const pg_font_ttf_t *t, int left_gid,
                       int right_gid)
{
        int lo, hi, mid;
        pg_u32 key, cur;
        if (!t || t->n_kern_pairs <= 0) return 0;
        key = ((pg_u32)(pg_u16)left_gid << 16) |
              (pg_u32)(pg_u16)right_gid;
        lo = 0;
        hi = t->n_kern_pairs - 1;
        while (lo <= hi) {
                mid = (lo + hi) / 2;
                cur = ((pg_u32)t->kern_pairs[mid].left << 16) |
                      (pg_u32)t->kern_pairs[mid].right;
                if (cur == key) return t->kern_pairs[mid].value;
                if (cur < key) lo = mid + 1;
                else hi = mid - 1;
        }
        return 0;
}

/* Kerning dalam piksel untuk ukuran_px tertentu. */
int pg_ttf_kerning_px(const pg_font_ttf_t *t, int left_gid,
                       int right_gid, int ukuran_px)
{
        pg_s16 k;
        if (!t || t->units_per_em <= 0) return 0;
        k = pg_ttf_kerning(t, left_gid, right_gid);
        return ((int)k * ukuran_px) / t->units_per_em;
}

/* Glyph ID untuk codepoint (cmap lookup, wrapper untuk
 * dispatcher). */
int pg_ttf_glyph_id(const pg_font_ttf_t *t, pg_u32 kode)
{
        if (!t) return 0;
        return pg_ttf_cmap_lookup((pg_font_ttf_t *)t, (int)kode);
}

int pg_ttf_tinggi(const pg_font_ttf_t *t, int ukuran_px)
{
        int asc, desc;
        if (!t) return 0;
        asc = (t->ascent * ukuran_px) / t->units_per_em;
        desc = (-t->descent * ukuran_px) / t->units_per_em;
        return asc + desc;
}

int pg_ttf_tinggi_baris(const pg_font_ttf_t *t, int ukuran_px)
{
        int total;
        if (!t) return 0;
        total = t->ascent - t->descent + t->line_gap;
        return (total * ukuran_px) / t->units_per_em;
}

int pg_ttf_ascent(const pg_font_ttf_t *t, int ukuran_px)
{
        if (!t) return 0;
        return (t->ascent * ukuran_px) / t->units_per_em;
}

int pg_ttf_descent(const pg_font_ttf_t *t, int ukuran_px)
{
        int d;
        if (!t) return 0;
        /* descent di TTF biasanya negatif. */
        d = t->descent;
        if (d < 0) d = -d;
        return (d * ukuran_px) / t->units_per_em;
}

void pg_ttf_hancur(pg_font_ttf_t *t)
{
        int i;
        if (!t) return;
        for (i = 0; i < PG_TTF_CACHE_SIZE; i++) {
                if (t->cache[i].bitmap)
                        free(t->cache[i].bitmap);
        }
        if (t->kern_pairs) free(t->kern_pairs);
        if (t->data) free(t->data);
        free(t);
}
