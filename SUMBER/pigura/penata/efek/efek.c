/*
 * PIGURA OS - EFEK.C
 * =================
 * Dispatcher efek visual subsistem penata Pigura OS.
 *
 * Modul ini bertanggung jawab untuk menerapkan efek visual
 * ke permukaan target berdasarkan tipe efek yang diminta.
 * Bergantung pada implementasi spesifik:
 *   - blur.c      : Box blur
 *   - shadow.c    : Drop shadow
 *   - transparan.c: Alpha blending
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
 * API PUBLIK - TERAPKAN EFEK
 * ===========================================================================
 *
 * Terapkan satu efek ke area pada permukaan target.
 * Menentukan tipe efek lalu memanggil implementasi
 * spesifik yang sesuai.
 */

status_t penata_efek_terapkan(const penata_efek_t *efek,
                              permukaan_t *target,
                              const penata_rect_t *area)
{
    status_t st;

    if (efek == NULL || target == NULL || area == NULL) {
        return STATUS_PARAM_NULL;
    }

    switch (efek->tipe) {
    case PENATA_EFEK_BLUR:
        st = penata_blur_kotak(target, area,
              efek->konfig.blur.radius,
              efek->konfig.blur.iterasi);
        break;

    case PENATA_EFEK_BAYANGAN:
        st = penata_bayangan(target, area,
              efek->konfig.bayangan.offset_x,
              efek->konfig.bayangan.offset_y,
              efek->konfig.bayangan.radius,
              efek->konfig.bayangan.warna);
        break;

    case PENATA_EFEK_TRANSPARAN:
        st = penata_campur_alpha(target, area,
              (tak_bertanda8_t)efek->konfig.transparan.alpha);
        break;

    case PENATA_EFEK_TIDAK_ADA:
    default:
        st = STATUS_BERHASIL;
        break;
    }

    return st;
}

/*
 * ===========================================================================
 * API PUBLIK - TERAPKAN EFEK KE LAPISAN
 * ===========================================================================
 *
 * Terapkan semua efek yang terdaftar pada lapisan
 * ke permukaan target. Efek diterapkan secara berurutan
 * sesuai urutan pendaftaran.
 */

status_t penata_efek_terapkan_lapisan(
    const penata_lapisan_t *lapisan,
    permukaan_t *target)
{
    tak_bertanda32_t i;
    penata_rect_t area;
    status_t st;

    if (lapisan == NULL || target == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Bangun area dari posisi dan ukuran lapisan */
    area.x = lapisan->x;
    area.y = lapisan->y;
    area.lebar = lapisan->lebar;
    area.tinggi = lapisan->tinggi;

    /* Terapkan setiap efek secara berurutan */
    for (i = 0; i < lapisan->jumlah_efek; i++) {
        st = penata_efek_terapkan(
            &lapisan->efek[i], target, &area);
        if (st != STATUS_BERHASIL) {
            return st;
        }
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - TAMBAH EFEK KE LAPISAN
 * ===========================================================================
 *
 * Tambah satu efek ke daftar efek lapisan.
 */

status_t penata_lapisan_tambah_efek(penata_konteks_t *ktx,
                                    tak_bertanda32_t id,
                                    const penata_efek_t *efek)
{
    penata_lapisan_t *l;

    if (ktx == NULL || efek == NULL) {
        return STATUS_PARAM_NULL;
    }

    l = penata_lapisan_cari(ktx, id);
    if (l == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    if (l->jumlah_efek >= PENATA_EFEK_MAKS) {
        return STATUS_PENUH;
    }

    l->efek[l->jumlah_efek] = *efek;
    l->jumlah_efek++;

    /* Aktifkan flag efek */
    l->flags |= PENATA_FLAG_EFEK;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - HAPUS EFEK DARI LAPISAN
 * ===========================================================================
 */

status_t penata_lapisan_hapus_efek(penata_konteks_t *ktx,
                                   tak_bertanda32_t id,
                                   tak_bertanda32_t indeks)
{
    penata_lapisan_t *l;
    tak_bertanda32_t i;

    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }

    l = penata_lapisan_cari(ktx, id);
    if (l == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    if (indeks >= l->jumlah_efek) {
        return STATUS_PARAM_JARAK;
    }

    /* Geser efek setelah indeks ke kiri */
    for (i = indeks; i + 1 < l->jumlah_efek; i++) {
        l->efek[i] = l->efek[i + 1];
    }
    l->jumlah_efek--;

    if (l->jumlah_efek == 0) {
        l->flags &= ~(tak_bertanda32_t)PENATA_FLAG_EFEK;
    }

    return STATUS_BERHASIL;
}
