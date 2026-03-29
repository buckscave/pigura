/*
 * PIGURA OS - FONT.C
 * ===================
 * Manajemen font Pigura OS.
 *
 * Modul ini mengelola pendaftaran, pencarian, dan
 * pelepasan font dalam sistem. Font didaftarkan ke
 * tabel global dan dapat diakses melalui nama.
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
 * DEFINISI STRUKTUR FONT (lengkap)
 * ===========================================================================
 * Hanya dilihat oleh font.c.
 * Pengguna luar menggunakan font_t (opaque pointer).
 */

struct font {
    tak_bertanda32_t magic;
    tak_bertanda32_t jenis;        /* FONT_JENIS_* */
    char nama[32];                 /* Nama font */
    font_metrik_t metrik;          /* Metrik global */

    /* Data glyph (untuk font bitmap) */
    glyph_bitmap_t *daftar_glyph;
    tak_bertanda16_t jumlah_glyph;
    tak_bertanda32_t kode_awal;    /* Kode Unicode awal */
    tak_bertanda32_t kode_akhir;   /* Kode Unicode akhir */

    /* Data TTF (untuk font TrueType) */
    void *ttf_konteks;             /* Konteks parser TTF */
    tak_bertanda64_t ttf_ukuran;   /* Ukuran data TTF */
};

/*
 * ===========================================================================
 * VARIABEL GLOBAL
 * ===========================================================================
 */

static font_t g_font_daftar[FONT_DAFTAR_MAKS];
static tak_bertanda32_t g_font_jumlah = 0;
static bool_t g_font_diinit = SALAH;

/*
 * ===========================================================================
 * FUNGSI HELPER INTERNAL
 * ===========================================================================
 */

/*
 * Cari slot kosong dalam tabel font.
 * Return: index slot, atau -1 jika penuh.
 */
static tanda32_t cari_slot_kosong(void)
{
    tak_bertanda32_t i;
    for (i = 0; i < FONT_DAFTAR_MAKS; i++) {
        if (g_font_daftar[i].magic != FONT_MAGIC) {
            return (tanda32_t)i;
        }
    }
    return -1;
}

/*
 * Salin nama font dengan batas panjang.
 */
static void salin_nama_font(char *tujuan, const char *sumber,
                            tak_bertanda32_t maks)
{
    tak_bertanda32_t i;
    for (i = 0; i < maks - 1; i++) {
        if (sumber[i] == '\0') {
            break;
        }
        tujuan[i] = sumber[i];
    }
    tujuan[i] = '\0';
}

/*
 * ===========================================================================
 * FONT - INISIALISASI
 * ===========================================================================
 */

status_t font_init(void)
{
    if (g_font_diinit) {
        return STATUS_SUDAH_ADA;
    }
    kernel_memset(g_font_daftar, 0, sizeof(g_font_daftar));
    g_font_jumlah = 0;
    g_font_diinit = BENAR;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FONT - SHUTDOWN
 * ===========================================================================
 */

void font_shutdown(void)
{
    tak_bertanda32_t i;
    for (i = 0; i < FONT_DAFTAR_MAKS; i++) {
        if (g_font_daftar[i].magic == FONT_MAGIC) {
            font_hapus(&g_font_daftar[i]);
        }
    }
    g_font_jumlah = 0;
    g_font_diinit = SALAH;
}

/*
 * ===========================================================================
 * FONT - DAFTAR BITMAP (dari data terdegar)
 * ===========================================================================
 */

font_t *font_daftar_bitmap(const char *nama,
                           const glyph_bitmap_t *daftar_glyph,
                           tak_bertanda16_t jumlah_glyph,
                           tak_bertanda8_t ukuran_pt)
{
    tanda32_t idx;
    font_t *f;
    tak_bertanda32_t i;

    if (!g_font_diinit || nama == NULL) {
        return NULL;
    }
    if (daftar_glyph == NULL || jumlah_glyph == 0) {
        return NULL;
    }

    idx = cari_slot_kosong();
    if (idx < 0) {
        return NULL;
    }

    f = &g_font_daftar[idx];
    kernel_memset(f, 0, sizeof(font_t));

    /* Alokasi dan salin daftar glyph */
    f->daftar_glyph = (glyph_bitmap_t *)kmalloc(
        (ukuran_t)(jumlah_glyph * sizeof(glyph_bitmap_t)));
    if (f->daftar_glyph == NULL) {
        return NULL;
    }
    kernel_memset(f->daftar_glyph, 0,
        (ukuran_t)(jumlah_glyph * sizeof(glyph_bitmap_t)));

    for (i = 0; i < jumlah_glyph; i++) {
        status_t st = glyph_salin_bitmap(
            &daftar_glyph[i], &f->daftar_glyph[i]);
        if (st != STATUS_BERHASIL) {
            tak_bertanda32_t j;
            for (j = 0; j < i; j++) {
                glyph_hapus_bitmap(
                    &f->daftar_glyph[j]);
            }
            kfree(f->daftar_glyph);
            f->daftar_glyph = NULL;
            return NULL;
        }
    }

    f->magic = FONT_MAGIC;
    f->jenis = FONT_JENIS_TERDEGAR;
    salin_nama_font(f->nama, nama, 32);
    f->jumlah_glyph = jumlah_glyph;
    f->kode_awal = 32;   /* Spasi */
    f->kode_akhir = 32 + jumlah_glyph - 1;

    f->metrik.ukuran = ukuran_pt;
    f->metrik.tinggi_asc = ukuran_pt;
    f->metrik.tinggi_desc = 0;
    f->metrik.tinggi_baris = ukuran_pt + 2;
    f->metrik.lebar_maks = ukuran_pt;
    f->metrik.dpi = 72;

    g_font_jumlah++;
    return f;
}

/*
 * ===========================================================================
 * FONT - DAFTAR BERKAS (placeholder untuk font dari berkas)
 * ===========================================================================
 */

font_t *font_daftar_berkas(const char *nama_berkas,
                           const char *nama)
{
    (void)nama_berkas;
    (void)nama;
    return NULL;
}

/*
 * ===========================================================================
 * FONT - DAFTAR TTF
 * ===========================================================================
 */

font_t *font_daftar_ttf(const char *nama_berkas,
                        const char *nama,
                        tak_bertanda8_t ukuran_pt)
{
    (void)nama_berkas;
    (void)nama;
    (void)ukuran_pt;
    return NULL;
}

/*
 * ===========================================================================
 * FONT - HAPUS
 * ===========================================================================
 */

status_t font_hapus(font_t *f)
{
    tak_bertanda32_t i;

    if (f == NULL || f->magic != FONT_MAGIC) {
        return STATUS_PARAM_NULL;
    }

    /* Bebaskan semua glyph bitmap */
    if (f->daftar_glyph != NULL) {
        for (i = 0; i < f->jumlah_glyph; i++) {
            glyph_hapus_bitmap(&f->daftar_glyph[i]);
        }
        kfree(f->daftar_glyph);
        f->daftar_glyph = NULL;
    }

    /* Bebaskan konteks TTF jika ada */
    if (f->ttf_konteks != NULL) {
        font_ttf_tutup(f->ttf_konteks);
        f->ttf_konteks = NULL;
    }

    kernel_memset(f, 0, sizeof(font_t));
    if (g_font_jumlah > 0) {
        g_font_jumlah--;
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FONT - CARI
 * ===========================================================================
 */

font_t *font_cari(const char *nama)
{
    tak_bertanda32_t i;

    if (!g_font_diinit || nama == NULL) {
        return NULL;
    }

    for (i = 0; i < FONT_DAFTAR_MAKS; i++) {
        if (g_font_daftar[i].magic != FONT_MAGIC) {
            continue;
        }
        if (kernel_strcmp(g_font_daftar[i].nama, nama)
            == 0) {
            return &g_font_daftar[i];
        }
    }

    return NULL;
}

/*
 * ===========================================================================
 * FONT - DAPAT METRIK
 * ===========================================================================
 */

status_t font_dapat_metrik(const font_t *f,
                           font_metrik_t *metrik)
{
    if (f == NULL || metrik == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (f->magic != FONT_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    kernel_memcpy(metrik, &f->metrik, sizeof(font_metrik_t));
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FONT - DAPAT GLYPH
 * ===========================================================================
 */

const glyph_bitmap_t *font_dapat_glyph(const font_t *f,
                                       tak_bertanda32_t kode)
{
    tak_bertanda32_t idx;

    if (f == NULL || f->magic != FONT_MAGIC) {
        return NULL;
    }
    if (f->daftar_glyph == NULL) {
        return NULL;
    }

    /* Cek apakah kode dalam rentang glyph */
    if (kode < f->kode_awal || kode > f->kode_akhir) {
        return NULL;
    }

    idx = kode - f->kode_awal;
    if (idx >= f->jumlah_glyph) {
        return NULL;
    }

    return &f->daftar_glyph[idx];
}
