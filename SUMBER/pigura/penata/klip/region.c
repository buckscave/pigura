/*
 * PIGURA OS - REGION.C
 * ====================
 * Manajemen region (kumpulan persegi panjang) penata.
 *
 * Region adalah kumpulan rect yang merepresentasikan
 * area kompleks. Digunakan untuk klip area yang
 * tidak berbentuk persegi panjang sederhana.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Tidak menggunakan fitur C99/C11
 * - Seluruh nama dalam Bahasa Indonesia
 * - Mendukung: x86, x86_64, ARM, ARMv7, ARM64
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../../inti/kernel.h"
#include "../../framebuffer/framebuffer.h"
#include "../penata.h"

/*
 * ===========================================================================
 * API PUBLIK - BUAT REGION
 * ===========================================================================
 *
 * Inisialisasi region kosong.
 */

status_t penata_region_buat(penata_region_t *region)
{
    if (region == NULL) {
        return STATUS_PARAM_NULL;
    }

    kernel_memset(region, 0, sizeof(penata_region_t));
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - BERSIHKAN REGION
 * ===========================================================================
 *
 * Hapus semua rect dari region.
 */

void penata_region_bersihkan(penata_region_t *region)
{
    if (region == NULL) return;
    kernel_memset(region, 0, sizeof(penata_region_t));
}

/*
 * ===========================================================================
 * API PUBLIK - TAMBAH RECT KE REGION
 * ===========================================================================
 *
 * Tambah satu persegi panjang ke region.
 * Rect yang kosong (lebar/tinggi 0) akan diabaikan.
 */

status_t penata_region_tambah(penata_region_t *region,
                              const penata_rect_t *rect)
{
    if (region == NULL || rect == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Abaikan rect kosong */
    if (rect->lebar == 0 || rect->tinggi == 0) {
        return STATUS_BERHASIL;
    }

    /* Cek kapasitas */
    if (region->jumlah >= PENATA_REGION_RECT_MAKS) {
        return STATUS_PENUH;
    }

    region->rect[region->jumlah] = *rect;
    region->jumlah++;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - KLIP REGION TERHADAP RECT
 * ===========================================================================
 *
 * Klip setiap rect dalam region terhadap area klip.
 * Rect yang tidak beririsan dengan klip akan dihapus.
 * Rect yang beririsan akan dipotong ke area irisan.
 */

status_t penata_region_klip(penata_region_t *region,
                            const penata_rect_t *klip)
{
    tak_bertanda32_t i, j;
    penata_region_t tmp;

    if (region == NULL || klip == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (region->jumlah == 0) {
        return STATUS_BERHASIL;
    }

    /* Gunakan region sementara */
    kernel_memset(&tmp, 0, sizeof(tmp));

    for (i = 0; i < region->jumlah; i++) {
        penata_rect_t hasil;

        if (penata_intersek(&region->rect[i], klip,
                            &hasil)) {
            /* Ada irisan: tambah ke region sementara */
            if (tmp.jumlah < PENATA_REGION_RECT_MAKS) {
                tmp.rect[tmp.jumlah] = hasil;
                tmp.jumlah++;
            }
        }
        /* Tidak ada irisan: abaikan rect ini */
    }

    /* Salin hasil kembali */
    region->jumlah = tmp.jumlah;
    for (j = 0; j < tmp.jumlah; j++) {
        region->rect[j] = tmp.rect[j];
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - TITIK DALAM REGION
 * ===========================================================================
 *
 * Cek apakah titik (x, y) berada di dalam salah satu
 * rect dalam region.
 */

bool_t penata_region_titik_dalam(const penata_region_t *region,
                                 tanda32_t x, tanda32_t y)
{
    tak_bertanda32_t i;

    if (region == NULL) return SALAH;

    for (i = 0; i < region->jumlah; i++) {
        if (penata_klip_titik(&region->rect[i], x, y)) {
            return BENAR;
        }
    }

    return SALAH;
}

/*
 * ===========================================================================
 * API PUBLIK - DAPAT JUMLAH RECT
 * ===========================================================================
 */

tak_bertanda32_t penata_region_dapat_jumlah(
    const penata_region_t *region)
{
    if (region == NULL) return 0;
    return region->jumlah;
}

/*
 * ===========================================================================
 * FUNGSI - DAPAT BOUNDING BOX REGION
 * ===========================================================================
 *
 * Hitung bounding box dari seluruh rect dalam region.
 */

bool_t penata_region_dapat_bounding(
    const penata_region_t *region,
    penata_rect_t *hasil)
{
    tak_bertanda32_t i;

    if (region == NULL || hasil == NULL) return SALAH;
    if (region->jumlah == 0) {
        hasil->x = 0;
        hasil->y = 0;
        hasil->lebar = 0;
        hasil->tinggi = 0;
        return SALAH;
    }

    /* Mulai dari rect pertama */
    *hasil = region->rect[0];

    /* Union dengan rect berikutnya */
    for (i = 1; i < region->jumlah; i++) {
        penata_union(hasil, &region->rect[i], hasil);
    }

    return BENAR;
}

/*
 * ===========================================================================
 * FUNGSI - GABUNG DUA REGION
 * ===========================================================================
 *
 * Gabungkan semua rect dari region sumber ke region tujuan.
 */

status_t penata_region_gabung(penata_region_t *tujuan,
                              const penata_region_t *sumber)
{
    tak_bertanda32_t i;

    if (tujuan == NULL || sumber == NULL) {
        return STATUS_PARAM_NULL;
    }

    for (i = 0; i < sumber->jumlah; i++) {
        status_t st;
        st = penata_region_tambah(tujuan,
                                  &sumber->rect[i]);
        if (st != STATUS_BERHASIL) {
            return st;
        }
    }

    return STATUS_BERHASIL;
}
