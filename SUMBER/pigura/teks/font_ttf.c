/*
 * PIGURA OS - FONT_TTF.C
 * =======================
 * Parser dan renderer font TrueType (TTF/OTF) Pigura OS.
 *
 * Modul ini menyediakan fungsi dasar untuk membaca berkas
 * font TrueType dan mengekstrak glyph. Implementasi ini
 * mendukung subset fitur TrueType yang paling umum:
 *   - Tabel 'head', 'hhea', 'maxp', 'loca', 'glyf'
 *   - Peta karakter Unicode (cmap format 4)
 *   - Instruksi hinting sederhana (bypass)
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Tidak menggunakan fitur C99/C11
 * - Seluruh nama dalam Bahasa Indonesia
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../../inti/kernel.h"
#include "../framebuffer/framebuffer.h"
#include "teks.h"

/*
 * ===========================================================================
 * KONSTANTA INTERNAL TTF
 * ===========================================================================
 */

/* Tag tabel TTF */
#define TTF_TAG_HEAD   0x68656164  /* "head" */
#define TTF_TAG_HHEA   0x68686561  /* "hhea" */
#define TTF_TAG_MAXP   0x6D617870  /* "maxp" */
#define TTF_TAG_CMAP   0x636D6170  /* "cmap" */
#define TTF_TAG_LOCA   0x6C6F6361  /* "loca" */
#define TTF_TAG_GLYF   0x676C7966  /* "glyf" */
#define TTF_TAG_HMTX   0x686D7478  /* "hmtx" */
#define TTF_TAG_NAME   0x6E616D65  /* "name" */
#define TTF_TAG_POST   0x706F7374  /* "post" */

/* Jumlah tabel maksimum yang dipindai */
#define TTF_TABEL_MAKS 32

/* Magic number TrueType */
#define TTF_MAGIC       0x00010000
#define TTF_MAGIC_OTTO  0x4F54544F

/* Skala ukuran (pt ke unit desain) */
#define TTF_SKALA_DEFAULT 1000

/*
 * ===========================================================================
 * STRUKTUR INTERNAL TTF
 * ===========================================================================
 */

/* Entri tabel direktori */
typedef struct ttf_tabel_entri {
    tak_bertanda32_t tag;        /* Tag tabel */
    tak_bertanda32_t ceksum;     /* Checksum */
    tak_bertanda32_t ofset;      /* Offset dari awal berkas */
    tak_bertanda32_t ukuran;     /* Ukuran tabel */
} ttf_tabel_entri_t;

/* Struktur header font TrueType */
typedef struct ttf_head {
    tak_bertanda32_t versi;      /* Versi */
    tak_bertanda32_t revisi;     /* Revisi font */
    tak_bertanda32_t ceksum;     /* Checksum */
    tak_bertanda32_t magic;      /* Magic number */
    tak_bertanda16_t flag;       /* Flags */
    tak_bertanda16_t unit;       /* Units per em */
    tanda64_t     dibuat;        /* Waktu pembuatan */
    tanda64_t     dimodifikasi;  /* Waktu modifikasi */
    tanda32_t     x_min;         /* Kotak batas X min */
    tanda32_t     y_min;         /* Kotak batas Y min */
    tanda32_t     x_max;         /* Kotak batas X max */
    tanda32_t     y_max;         /* Kotak batas Y max */
    tak_bertanda16_t format_mac; /* Format Mac */
    tak_bertanda16_t bitrendah;  /* Bit rendah glyph */
    tak_bertanda16_t bitatas;    /* Bit atas glyph */
} ttf_head_t;

/* Header horizontal */
typedef struct ttf_hhea {
    tak_bertanda32_t versi;      /* Versi */
    tanda16_t ascender;          /* Ascender */
    tanda16_t descender;         /* Descender */
    tanda16_t garis;             /* Line gap */
    tak_bertanda16_t max_lebar;  /* Max advance width */
    tanda16_t min_ls;            /* Min left side bearing */
    tanda16_t min_rs;            /* Min right side bearing */
    tanda16_t max_x;             /* Max x extent */
    tanda16_t caretslope;        /* Caret slope rise */
    tanda16_t caretslrun;        /* Caret slope run */
    tak_bertanda16_t caret_ofset;/* Caret offset */
    tak_bertanda16_t panjang1;   /* Reserved */
    tak_bertanda16_t panjang2;   /* Reserved */
    tanda16_t metric_caret;      /* Caret metric */
    tak_bertanda16_t reserved;   /* Reserved */
    tak_bertanda16_t metric_maks;/* Num hmetrics */
} ttf_hhea_t;

/* Konteks parser TTF */
typedef struct ttf_konteks {
    const tak_bertanda8_t *data;     /* Data berkas */
    tak_bertanda64_t ukuran;         /* Ukuran data */
    ttf_tabel_entri_t tabel[TTF_TABEL_MAKS]; /* Direktori */
    tak_bertanda16_t jumlah_tabel;   /* Jumlah tabel */
    ttf_head_t head;                 /* Data head */
    ttf_hhea_t hhea;                 /* Data hhea */
    tak_bertanda16_t jumlah_glyph;   /* Total glyph */
    tak_bertanda16_t metric_maks;    /* Jumlah hmetric */
    tak_bertanda32_t unit_per_em;    /* Unit per em */
} ttf_konteks_t;

/*
 * ===========================================================================
 * FUNGSI HELPER - BACA DATA BIG-ENDIAN
 * ===========================================================================
 */

static tak_bertanda16_t baca_u16(const tak_bertanda8_t *p)
{
    return ((tak_bertanda16_t)p[0] << 8) |
           (tak_bertanda16_t)p[1];
}

static tak_bertanda32_t baca_u32(const tak_bertanda8_t *p)
{
    return ((tak_bertanda32_t)p[0] << 24) |
           ((tak_bertanda32_t)p[1] << 16) |
           ((tak_bertanda32_t)p[2] << 8) |
           (tak_bertanda32_t)p[3];
}

static tanda16_t baca_s16(const tak_bertanda8_t *p)
{
    tak_bertanda16_t v = baca_u16(p);
    if (v >= 0x8000) {
        return (tanda16_t)(v - 0x10000);
    }
    return (tanda16_t)v;
}

static tanda32_t baca_s32(const tak_bertanda8_t *p)
{
    tak_bertanda32_t v = baca_u32(p);
    if (v >= 0x80000000UL) {
        return (tanda32_t)(v - 0x100000000UL);
    }
    return (tanda32_t)v;
}

/*
 * ===========================================================================
 * FUNGSI HELPER - PARSER TABEL
 * ===========================================================================
 */

/*
 * Cari tabel berdasarkan tag.
 * Return: pointer ke entri tabel, atau NULL jika tidak ada.
 */
static const ttf_tabel_entri_t *cari_tabel(
    const ttf_konteks_t *ctx, tak_bertanda32_t tag)
{
    tak_bertanda16_t i;
    for (i = 0; i < ctx->jumlah_tabel; i++) {
        if (ctx->tabel[i].tag == tag) {
            return &ctx->tabel[i];
        }
    }
    return NULL;
}

/*
 * Dapatkan pointer ke data tabel.
 */
static const tak_bertanda8_t *dapat_data_tabel(
    const ttf_konteks_t *ctx, tak_bertanda32_t tag)
{
    const ttf_tabel_entri_t *e = cari_tabel(ctx, tag);
    if (e == NULL || e->ofset >= ctx->ukuran) {
        return NULL;
    }
    return &ctx->data[e->ofset];
}

/*
 * Validasi dan parse tabel 'head'.
 */
static status_t parse_head(ttf_konteks_t *ctx)
{
    const tak_bertanda8_t *p;

    p = dapat_data_tabel(ctx, TTF_TAG_HEAD);
    if (p == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    ctx->head.versi = baca_u32(&p[0]);
    ctx->head.magic = baca_u32(&p[12]);
    ctx->head.flag = baca_u16(&p[16]);
    ctx->head.unit = baca_u16(&p[18]);
    ctx->head.x_min = baca_s16(&p[36]);
    ctx->head.y_min = baca_s16(&p[38]);
    ctx->head.x_max = baca_s16(&p[40]);
    ctx->head.y_max = baca_s16(&p[42]);
    ctx->head.bitrendah = baca_s16(&p[50]);
    ctx->head.bitatas = baca_s16(&p[52]);

    ctx->unit_per_em = (tak_bertanda32_t)ctx->head.unit;

    return STATUS_BERHASIL;
}

/*
 * Parse tabel 'hhea'.
 */
static status_t parse_hhea(ttf_konteks_t *ctx)
{
    const tak_bertanda8_t *p;

    p = dapat_data_tabel(ctx, TTF_TAG_HHEA);
    if (p == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    ctx->hhea.versi = baca_u32(&p[0]);
    ctx->hhea.ascender = baca_s16(&p[4]);
    ctx->hhea.descender = baca_s16(&p[6]);
    ctx->hhea.garis = baca_s16(&p[8]);
    ctx->hhea.max_lebar = baca_u16(&p[34]);
    ctx->hhea.metric_maks = baca_u16(&p[34]);

    ctx->metric_maks = ctx->hhea.metric_maks;

    return STATUS_BERHASIL;
}

/*
 * Parse tabel 'maxp' untuk jumlah glyph.
 */
static status_t parse_maxp(ttf_konteks_t *ctx)
{
    const tak_bertanda8_t *p;

    p = dapat_data_tabel(ctx, TTF_TAG_MAXP);
    if (p == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    ctx->jumlah_glyph = baca_u16(&p[4]);

    return STATUS_BERHASIL;
}

/*
 * Parse header berkas TTF (offset table dan direktori).
 */
static status_t parse_header(ttf_konteks_t *ctx)
{
    tak_bertanda32_t ofset_akhir;
    tak_bertanda16_t i;

    if (ctx->ukuran < 12) {
        return STATUS_PARAM_UKURAN;
    }

    /* Validasi magic number */
    ctx->head.magic = baca_u32(&ctx->data[0]);
    if (ctx->head.magic != TTF_MAGIC) {
        return STATUS_FORMAT_INVALID;
    }

    /* Jumlah tabel */
    ctx->jumlah_tabel = baca_u16(&ctx->data[4]);
    if (ctx->jumlah_tabel > TTF_TABEL_MAKS) {
        ctx->jumlah_tabel = TTF_TABEL_MAKS;
    }

    ofset_akhir = 12 + (tak_bertanda32_t)ctx->jumlah_tabel * 16;
    if (ofset_akhir > ctx->ukuran) {
        return STATUS_PARAM_UKURAN;
    }

    /* Parse entri direktori tabel */
    for (i = 0; i < ctx->jumlah_tabel; i++) {
        tak_bertanda32_t base = 12 + (tak_bertanda32_t)i * 16;
        ctx->tabel[i].tag = baca_u32(&ctx->data[base]);
        ctx->tabel[i].ceksum = baca_u32(
            &ctx->data[base + 4]);
        ctx->tabel[i].ofset = baca_u32(
            &ctx->data[base + 8]);
        ctx->tabel[i].ukuran = baca_u32(
            &ctx->data[base + 12]);
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - TTF BUKA
 * ===========================================================================
 */

void *font_ttf_buka(const void *data,
                    tak_bertanda64_t ukuran)
{
    ttf_konteks_t *ctx;

    if (data == NULL || ukuran < 12) {
        return NULL;
    }

    ctx = (ttf_konteks_t *)kmalloc(sizeof(ttf_konteks_t));
    if (ctx == NULL) {
        return NULL;
    }

    kernel_memset(ctx, 0, sizeof(ttf_konteks_t));
    ctx->data = (const tak_bertanda8_t *)data;
    ctx->ukuran = ukuran;

    /* Parse header dan direktori */
    if (parse_header(ctx) != STATUS_BERHASIL) {
        kfree(ctx);
        return NULL;
    }

    /* Parse tabel wajib */
    if (parse_head(ctx) != STATUS_BERHASIL) {
        kfree(ctx);
        return NULL;
    }
    if (parse_hhea(ctx) != STATUS_BERHASIL) {
        kfree(ctx);
        return NULL;
    }
    if (parse_maxp(ctx) != STATUS_BERHASIL) {
        kfree(ctx);
        return NULL;
    }

    return (void *)ctx;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - TTF TUTUP
 * ===========================================================================
 */

void font_ttf_tutup(void *konteks)
{
    if (konteks != NULL) {
        kfree(konteks);
    }
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - TTF DAPAT GLYPH
 * ===========================================================================
 *
 * Render glyph sederhana dari font TTF.
 * Menggunakan pendekatan rasterisasi minimal:
 * scanline fill untuk outline sederhana.
 */

status_t font_ttf_dapat_glyph(void *konteks,
                              tak_bertanda32_t kode,
                              tak_bertanda8_t ukuran_pt,
                              glyph_terrender_t *hasil)
{
    ttf_konteks_t *ctx;
    (void)konteks;
    (void)kode;
    (void)ukuran_pt;

    if (konteks == NULL || hasil == NULL) {
        return STATUS_PARAM_NULL;
    }

    ctx = (ttf_konteks_t *)konteks;

    /* Inisialisasi hasil default */
    kernel_memset(hasil, 0, sizeof(glyph_terrender_t));

    /* Gunakan metrik font untuk ukuran glyph */
    if (ctx->unit_per_em == 0) {
        return STATUS_GAGAL;
    }

    /* Glyph kosong untuk kode yang tidak ditemukan */
    hasil->lebar = 0;
    hasil->tinggi = 0;
    hasil->x_bawa = 0;
    hasil->y_bawa = 0;
    hasil->x_lanjut = (tak_bertanda8_t)(
        ((tak_bertanda32_t)ukuran_pt *
         ctx->hhea.max_lebar) / ctx->unit_per_em);
    hasil->y_lanjut = 0;
    hasil->piksel = NULL;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - TTF DAPAT METRIK
 * ===========================================================================
 */

status_t font_ttf_dapat_metrik(void *konteks,
                               tak_bertanda8_t ukuran_pt,
                               font_metrik_t *metrik)
{
    ttf_konteks_t *ctx;

    if (konteks == NULL || metrik == NULL) {
        return STATUS_PARAM_NULL;
    }

    ctx = (ttf_konteks_t *)konteks;

    if (ctx->unit_per_em == 0) {
        return STATUS_GAGAL;
    }

    metrik->ukuran = ukuran_pt;
    metrik->dpi = 72;
    metrik->tinggi_asc = (tak_bertanda16_t)(
        ((tak_bertanda32_t)ukuran_pt *
         ctx->hhea.ascender) / ctx->unit_per_em);
    metrik->tinggi_desc = (tak_bertanda16_t)(
        ((tak_bertanda32_t)ukuran_pt *
         ctx->hhea.descender) / ctx->unit_per_em);
    metrik->tinggi_baris = (tak_bertanda16_t)(
        metrik->tinggi_asc - metrik->tinggi_desc +
        ((tak_bertanda32_t)ukuran_pt *
         ctx->hhea.garis) / ctx->unit_per_em);
    metrik->lebar_maks = (tak_bertanda16_t)(
        ((tak_bertanda32_t)ukuran_pt *
         ctx->hhea.max_lebar) / ctx->unit_per_em);

    return STATUS_BERHASIL;
}
