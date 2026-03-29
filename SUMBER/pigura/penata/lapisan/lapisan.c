/*
 * PIGURA OS - LAPISAN.C
 * ====================
 * Manajemen lapisan (layer) subsistem penata Pigura OS.
 *
 * Modul ini menyediakan fungsi dasar untuk membuat, menghapus,
 * mencari, dan memvalidasi lapisan dalam konteks penata.
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
 * API PUBLIK - BUAT LAPISAN
 * ===========================================================================
 *
 * Buat lapisan baru dalam konteks penata.
 * Mengembalikan pointer ke lapisan yang dibuat,
 * atau NULL jika gagal.
 */

penata_lapisan_t *penata_lapisan_buat(penata_konteks_t *ktx)
{
    tak_bertanda32_t i;

    if (ktx == NULL) {
        return NULL;
    }

    /* Cari slot kosong */
    for (i = 0; i < PENATA_LAPISAN_MAKS; i++) {
        if (ktx->lapisan[i].magic != PENATA_MAGIC) {
            penata_lapisan_t *l = &ktx->lapisan[i];

            kernel_memset(l, 0, sizeof(penata_lapisan_t));

            l->magic = PENATA_MAGIC;
            l->id = ++ktx->id_counter;
            if (ktx->id_counter == 0) {
                ktx->id_counter = 1;
            }

            l->x = 0;
            l->y = 0;
            l->lebar = 0;
            l->tinggi = 0;
            l->flags = PENATA_FLAG_VISIBLE | PENATA_FLAG_AKTIF;
            l->z_urutan = (tanda32_t)i;
            l->alpha = 255;
            l->jumlah_efek = 0;
            l->permukaan = NULL;

            ktx->jumlah_lapisan++;

            return l;
        }
    }

    return NULL;
}

/*
 * ===========================================================================
 * API PUBLIK - HAPUS LAPISAN
 * ===========================================================================
 *
 * Hapus lapisan berdasarkan ID.
 * Membebaskan sumber daya yang terkait lapisan.
 */

status_t penata_lapisan_hapus(penata_konteks_t *ktx,
                             tak_bertanda32_t id)
{
    penata_lapisan_t *l;

    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }

    l = penata_lapisan_cari(ktx, id);
    if (l == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    /* Reset magic untuk menandai slot kosong */
    l->magic = 0;
    l->permukaan = NULL;
    l->jumlah_efek = 0;

    if (ktx->jumlah_lapisan > 0) {
        ktx->jumlah_lapisan--;
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - PINDAH LAPISAN
 * ===========================================================================
 */

status_t penata_lapisan_pindah(penata_konteks_t *ktx,
                               tak_bertanda32_t id,
                               tanda32_t x, tanda32_t y)
{
    penata_lapisan_t *l;

    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }

    l = penata_lapisan_cari(ktx, id);
    if (l == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    l->x = x;
    l->y = y;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - UBAH UKURAN LAPISAN
 * ===========================================================================
 */

status_t penata_lapisan_ubah_ukuran(penata_konteks_t *ktx,
                                    tak_bertanda32_t id,
                                    tak_bertanda32_t lebar,
                                    tak_bertanda32_t tinggi)
{
    penata_lapisan_t *l;

    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }

    l = penata_lapisan_cari(ktx, id);
    if (l == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    l->lebar = lebar;
    l->tinggi = tinggi;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - SET PERMUKAAN LAPISAN
 * ===========================================================================
 */

status_t penata_lapisan_set_permukaan(penata_konteks_t *ktx,
                                      tak_bertanda32_t id,
                                      permukaan_t *surf)
{
    penata_lapisan_t *l;

    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }

    l = penata_lapisan_cari(ktx, id);
    if (l == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    l->permukaan = surf;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - SET FLAG
 * ===========================================================================
 */

status_t penata_lapisan_set_flag(penata_konteks_t *ktx,
                                 tak_bertanda32_t id,
                                 tak_bertanda32_t flags)
{
    penata_lapisan_t *l;

    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }

    l = penata_lapisan_cari(ktx, id);
    if (l == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    l->flags = flags;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - SET ALPHA
 * ===========================================================================
 */

status_t penata_lapisan_set_alpha(penata_konteks_t *ktx,
                                  tak_bertanda32_t id,
                                  tak_bertanda8_t alpha)
{
    penata_lapisan_t *l;

    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }

    l = penata_lapisan_cari(ktx, id);
    if (l == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    l->alpha = alpha;
    l->flags |= PENATA_FLAG_TRANSPARAN;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - CARI LAPISAN
 * ===========================================================================
 */

penata_lapisan_t *penata_lapisan_cari(penata_konteks_t *ktx,
                                      tak_bertanda32_t id)
{
    tak_bertanda32_t i;

    if (ktx == NULL) return NULL;

    for (i = 0; i < PENATA_LAPISAN_MAKS; i++) {
        if (ktx->lapisan[i].magic == PENATA_MAGIC &&
            ktx->lapisan[i].id == id) {
            return &ktx->lapisan[i];
        }
    }

    return NULL;
}

/*
 * ===========================================================================
 * API PUBLIK - DAPAT LAPISAN TERATAS
 * ===========================================================================
 *
 * Dapatkan lapisan dengan z_urutan tertinggi.
 */

penata_lapisan_t *penata_lapisan_teratas(penata_konteks_t *ktx)
{
    tak_bertanda32_t i;
    penata_lapisan_t *terbaik = NULL;
    tanda32_t z_tertinggi = -1;

    if (ktx == NULL) return NULL;

    for (i = 0; i < PENATA_LAPISAN_MAKS; i++) {
        penata_lapisan_t *l = &ktx->lapisan[i];

        if (l->magic != PENATA_MAGIC) continue;
        if (!(l->flags & PENATA_FLAG_VISIBLE)) continue;

        if (l->z_urutan > z_tertinggi) {
            z_tertinggi = l->z_urutan;
            terbaik = l;
        }
    }

    return terbaik;
}

/*
 * ===========================================================================
 * API PUBLIK - VALIDASI LAPISAN
 * ===========================================================================
 */

bool_t penata_lapisan_valid(const penata_lapisan_t *l)
{
    if (l == NULL) return SALAH;
    if (l->magic != PENATA_MAGIC) return SALAH;
    return BENAR;
}
