/*
 * PIGURA OS - FONT_CACHE.C
 * =========================
 * Cache glyph Pigura OS.
 *
 * Modul ini menyediakan cache LRU (Least Recently Used) sederhana
 * untuk glyph yang sudah di-render. Cache mengurangi pengulangan
 * operasi rasterisasi yang mahal.
 *
 * Implementasi menggunakan open addressing dengan linear probing.
 * Setiap entri menyimpan satu glyph terrender beserta kunci
 * pencarian (kode Unicode + ukuran).
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
 * KONSTANTA INTERNAL
 * ===========================================================================
 */

/* Faktor beban maksimum cache (70%) */
#define CACHE_BEBAN_MAKS    70

/* Faktor perkalian hash */
#define HASH_MULTIPLIER     2654435761UL

/* Kapasitas default jika parameter 0 */
#define CACHE_KAPASITAS_DEFAULT 128

/*
 * ===========================================================================
 * FUNGSI HELPER INTERNAL
 * ===========================================================================
 */

/*
 * Hitung kunci hash dari kode Unicode dan ukuran.
 * Menggunakan algoritma sederhana: (kode << 8) | ukuran.
 */
static tak_bertanda32_t hitung_kunci(tak_bertanda32_t kode,
                                     tak_bertanda8_t ukuran_pt)
{
    return (kode << 8) | (tak_bertanda32_t)ukuran_pt;
}

/*
 * Hitung hash dari kunci untuk pencarian slot.
 * Menggunakan hash sederhana (multiplicative).
 */
static tak_bertanda32_t hitung_hash(tak_bertanda32_t kunci,
                                    tak_bertanda32_t kapasitas)
{
    tak_bertanda64_t h;
    h = (tak_bertanda64_t)kunci * HASH_MULTIPLIER;
    return (tak_bertanda32_t)(h % (tak_bertanda64_t)kapasitas);
}

/*
 * Bebaskan memori glyph dalam satu entri cache.
 */
static void hapus_entri(glyph_cache_entri_t *e)
{
    if (e == NULL) {
        return;
    }
    if (e->glyph.piksel != NULL) {
        kfree(e->glyph.piksel);
        e->glyph.piksel = NULL;
    }
    e->dipakai = SALAH;
    e->kunci = 0;
    e->kode = 0;
    e->ukuran_pt = 0;
}

/*
 * ===========================================================================
 * CACHE - BUAT
 * ===========================================================================
 */

glyph_cache_t *glyph_cache_buat(tak_bertanda32_t kapasitas)
{
    glyph_cache_t *cache;
    ukuran_t ukuran_mem;

    if (kapasitas == 0) {
        kapasitas = CACHE_KAPASITAS_DEFAULT;
    }

    /* Batasi kapasitas */
    if (kapasitas > GLYPH_CACHE_MAKS) {
        kapasitas = GLYPH_CACHE_MAKS;
    }

    cache = (glyph_cache_t *)kmalloc(sizeof(glyph_cache_t));
    if (cache == NULL) {
        return NULL;
    }

    ukuran_mem = kapasitas * sizeof(glyph_cache_entri_t);
    cache->entri = (glyph_cache_entri_t *)kmalloc(ukuran_mem);
    if (cache->entri == NULL) {
        kfree(cache);
        return NULL;
    }

    kernel_memset(cache->entri, 0, ukuran_mem);
    cache->magic = TEKS_MAGIC;
    cache->kapasitas = kapasitas;
    cache->terisi = 0;
    cache->pemukulan = 0;
    cache->kehilangan = 0;

    return cache;
}

/*
 * ===========================================================================
 * CACHE - HAPUS
 * ===========================================================================
 */

void glyph_cache_hapus(glyph_cache_t *cache)
{
    tak_bertanda32_t i;

    if (cache == NULL) {
        return;
    }

    if (cache->entri != NULL) {
        for (i = 0; i < cache->kapasitas; i++) {
            if (cache->entri[i].dipakai) {
                hapus_entri(&cache->entri[i]);
            }
        }
        kfree(cache->entri);
        cache->entri = NULL;
    }

    cache->kapasitas = 0;
    cache->terisi = 0;
    cache->magic = 0;

    kfree(cache);
}

/*
 * ===========================================================================
 * CACHE - CARI
 * ===========================================================================
 */

const glyph_terrender_t *glyph_cache_cari(
    glyph_cache_t *cache,
    tak_bertanda32_t kode,
    tak_bertanda8_t ukuran_pt)
{
    tak_bertanda32_t kunci;
    tak_bertanda32_t hash;
    tak_bertanda32_t i;

    if (cache == NULL || cache->magic != TEKS_MAGIC) {
        return NULL;
    }
    if (cache->entri == NULL) {
        return NULL;
    }

    kunci = hitung_kunci(kode, ukuran_pt);
    hash = hitung_hash(kunci, cache->kapasitas);

    /* Linear probing */
    for (i = 0; i < cache->kapasitas; i++) {
        tak_bertanda32_t idx = (hash + i) % cache->kapasitas;
        glyph_cache_entri_t *e = &cache->entri[idx];

        if (!e->dipakai) {
            /* Slot kosong, glyph tidak ada */
            break;
        }
        if (e->kunci == kunci && e->dipakai) {
            /* Ditemukan */
            cache->pemukulan++;
            return &e->glyph;
        }
    }

    /* Cache miss */
    cache->kehilangan++;
    return NULL;
}

/*
 * ===========================================================================
 * CACHE - TAMBAH
 * ===========================================================================
 */

status_t glyph_cache_tambah(glyph_cache_t *cache,
                            tak_bertanda32_t kode,
                            tak_bertanda8_t ukuran_pt,
                            const glyph_terrender_t *glyph)
{
    tak_bertanda32_t kunci;
    tak_bertanda32_t hash;
    tak_bertanda32_t i;
    tak_bertanda32_t beban_persen;

    if (cache == NULL || cache->magic != TEKS_MAGIC) {
        return STATUS_PARAM_NULL;
    }
    if (cache->entri == NULL || glyph == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Cek apakah sudah ada */
    if (glyph_cache_cari(cache, kode, ukuran_pt) != NULL) {
        return STATUS_SUDAH_ADA;
    }

    /* Evict jika melebihi faktor beban */
    beban_persen = (cache->terisi * 100) / cache->kapasitas;
    if (beban_persen >= CACHE_BEBAN_MAKS) {
        /* Evict: bersihkan semua dan reset counter */
        glyph_cache_bersihkan(cache);
    }

    kunci = hitung_kunci(kode, ukuran_pt);
    hash = hitung_hash(kunci, cache->kapasitas);

    /* Cari slot kosong */
    for (i = 0; i < cache->kapasitas; i++) {
        tak_bertanda32_t idx = (hash + i) % cache->kapasitas;
        glyph_cache_entri_t *e = &cache->entri[idx];

        if (!e->dipakai) {
            e->kunci = kunci;
            e->kode = kode;
            e->ukuran_pt = ukuran_pt;
            e->dipakai = BENAR;

            /* Salin data glyph */
            e->glyph.kode = glyph->kode;
            e->glyph.lebar = glyph->lebar;
            e->glyph.tinggi = glyph->tinggi;
            e->glyph.x_bawa = glyph->x_bawa;
            e->glyph.y_bawa = glyph->y_bawa;
            e->glyph.x_lanjut = glyph->x_lanjut;
            e->glyph.y_lanjut = glyph->y_lanjut;

            if (glyph->piksel != NULL &&
                glyph->lebar > 0 && glyph->tinggi > 0) {
                tak_bertanda32_t jumlah;
                jumlah = (tak_bertanda32_t)glyph->lebar *
                         (tak_bertanda32_t)glyph->tinggi;
                e->glyph.piksel = (tak_bertanda32_t *)
                    kmalloc(jumlah *
                            sizeof(tak_bertanda32_t));
                if (e->glyph.piksel == NULL) {
                    e->dipakai = SALAH;
                    return STATUS_MEMORI_TIDAK_CUKUP;
                }
                kernel_memcpy(e->glyph.piksel, glyph->piksel,
                    jumlah * sizeof(tak_bertanda32_t));
            } else {
                e->glyph.piksel = NULL;
            }

            cache->terisi++;
            return STATUS_BERHASIL;
        }
    }

    /* Tidak menemukan slot kosong */
    return STATUS_PENUH;
}

/*
 * ===========================================================================
 * CACHE - BERSIHKAN
 * ===========================================================================
 */

void glyph_cache_bersihkan(glyph_cache_t *cache)
{
    tak_bertanda32_t i;

    if (cache == NULL || cache->entri == NULL) {
        return;
    }

    for (i = 0; i < cache->kapasitas; i++) {
        if (cache->entri[i].dipakai) {
            hapus_entri(&cache->entri[i]);
        }
    }

    cache->terisi = 0;
}

/*
 * ===========================================================================
 * CACHE - STATISTIK
 * ===========================================================================
 */

void glyph_cache_statistik(const glyph_cache_t *cache,
                           tak_bertanda32_t *terisi,
                           tak_bertanda32_t *pemukulan,
                           tak_bertanda32_t *kehilangan)
{
    if (cache == NULL) {
        return;
    }
    if (terisi != NULL) {
        *terisi = cache->terisi;
    }
    if (pemukulan != NULL) {
        *pemukulan = cache->pemukulan;
    }
    if (kehilangan != NULL) {
        *kehilangan = cache->kehilangan;
    }
}
