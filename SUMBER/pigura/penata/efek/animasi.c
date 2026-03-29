/*
 * PIGURA OS - ANIMASI.C
 * =====================
 * Sistem animasi subsistem penata Pigura OS.
 *
 * Menyediakan animasi linier dan easing sederhana
 * untuk transisi lapisan dan efek visual.
 *
 * Menggunakan fixed-point 16.16 untuk presisi tanpa
 * floating point (kompatibel dengan -nostdlib).
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
 * FUNGSI HELPER - EASING
 * ===========================================================================
 */

/*
 * Ease-in quadratic: percepatan dari lambat ke cepat.
 * t adalah nilai fixed-point 0..65536 (0.0..1.0).
 * Mengembalikan nilai fixed-point 0..65536.
 */
static tanda32_t ease_in(tanda32_t t)
{
    /*
     * f(t) = t^2 dalam fixed-point.
     * t^2 = (t/65536) * (t/65536) * 65536
     *      = (t * t) / 65536
     */
    tanda64_t hasil = ((tanda64_t)t * (tanda64_t)t);
    return (tanda32_t)(hasil / 65536);
}

/*
 * Ease-out quadratic: perlambatan dari cepat ke lambat.
 */
static tanda32_t ease_out(tanda32_t t)
{
    /*
     * f(t) = 1 - (1-t)^2
     * = 65536 - ease_in(65536 - t)
     */
    tanda32_t inv = 65536 - t;
    return 65536 - ease_in(inv);
}

/*
 * ===========================================================================
 * API PUBLIK - INTERPOLASI
 * ===========================================================================
 *
 * Interpolasi antara nilai_awal dan nilai_akhir berdasarkan
 * progress t (0..65536 dalam fixed-point 16.16).
 */

tanda32_t penata_animasi_interpolasi(tanda32_t awal,
                                     tanda32_t akhir,
                                     tak_bertanda32_t t,
                                     tak_bertanda32_t tipe)
{
    tanda32_t t_eff;
    tanda64_t delta, progress;

    /* Clamp t ke rentang [0, 65536] */
    if (t >= 65536) return akhir;
    if (t == 0) return awal;

    t_eff = (tanda32_t)t;

    /* Terapkan kurva easing */
    switch (tipe) {
    case PENATA_INTERPOLASI_EASE_IN:
        t_eff = ease_in(t_eff);
        break;
    case PENATA_INTERPOLASI_EASE_OUT:
        t_eff = ease_out(t_eff);
        break;
    case PENATA_INTERPOLASI_LINIER:
    default:
        break;
    }

    /* Hitung interpolasi: awal + (akhir - awal) * t_eff */
    delta = (tanda64_t)akhir - (tanda64_t)awal;
    progress = (delta * (tanda64_t)t_eff) / 65536;

    return (tanda32_t)((tanda64_t)awal + progress);
}

/*
 * ===========================================================================
 * API PUBLIK - MULAI ANIMASI
 * ===========================================================================
 *
 * Inisialisasi dan mulai animasi pada konteks penata.
 * Animasi akan diproses pada setiap pemanggilan
 * penata_animasi_langkah().
 */

status_t penata_animasi_mulai(penata_konteks_t *ktx,
                              penata_animasi_t *anim)
{
    tak_bertanda32_t i;

    if (ktx == NULL || anim == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (ktx->jumlah_animasi >= PENATA_ANIMASI_FRAMA_MAKS) {
        return STATUS_PENUH;
    }

    /* Validasi durasi */
    if (anim->durasi == 0) {
        return STATUS_PARAM_INVALID;
    }

    /* Set status dan reset waktu */
    anim->status = PENATA_ANIMASI_JALAN;
    anim->waktu_sekarang = 0;
    anim->nilai_sekarang = anim->nilai_awal;

    /* Cari slot kosong */
    for (i = 0; i < PENATA_ANIMASI_FRAMA_MAKS; i++) {
        if (ktx->animasi[i].status == PENATA_ANIMASI_STOP ||
            ktx->animasi[i].status == PENATA_ANIMASI_SELESAI) {
            /* Salin animasi ke slot */
            ktx->animasi[i] = *anim;
            ktx->jumlah_animasi++;
            return STATUS_BERHASIL;
        }
    }

    return STATUS_PENUH;
}

/*
 * ===========================================================================
 * API PUBLIK - LANGKAH ANIMASI
 * ===========================================================================
 *
 * Proses satu langkah animasi (dipanggil per frame).
 * delta_ms adalah waktu yang berlalu sejak frame terakhir.
 * Memperbarui nilai_sekarang dan memanggil fungsi callback
 * jika animasi masih berjalan.
 */

status_t penata_animasi_langkah(penata_konteks_t *ktx,
                                tak_bertanda32_t delta_ms)
{
    tak_bertanda32_t i;

    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (delta_ms == 0) {
        return STATUS_BERHASIL;
    }

    for (i = 0; i < PENATA_ANIMASI_FRAMA_MAKS; i++) {
        penata_animasi_t *a = &ktx->animasi[i];

        if (a->status != PENATA_ANIMASI_JALAN) {
            continue;
        }

        /* Tambah waktu */
        a->waktu_sekarang += delta_ms;

        /* Cek apakah animasi selesai */
        if (a->waktu_sekarang >= a->durasi) {
            a->waktu_sekarang = a->durasi;
            a->nilai_sekarang = a->nilai_akhir;
            a->status = PENATA_ANIMASI_SELESAI;

            /* Panggil callback satu kali terakhir */
            if (a->fungsi != NULL) {
                a->fungsi(a->data, a->nilai_akhir);
            }
            ktx->jumlah_animasi--;
            continue;
        }

        /* Hitung progress (0..65536 fixed-point) */
        {
            tanda32_t t;
            tanda64_t progress_fp;

            progress_fp = ((tanda64_t)a->waktu_sekarang
                << 16) / (tanda64_t)a->durasi;
            t = (tanda32_t)progress_fp;

            /* Interpolasi nilai */
            a->nilai_sekarang =
                penata_animasi_interpolasi(
                    a->nilai_awal, a->nilai_akhir, t,
                    a->tipe_interpolasi);

            /* Panggil callback */
            if (a->fungsi != NULL) {
                a->fungsi(a->data, a->nilai_sekarang);
            }
        }
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - BERHENTI ANIMASI
 * ===========================================================================
 */

void penata_animasi_berhenti(penata_animasi_t *anim)
{
    if (anim == NULL) return;

    anim->status = PENATA_ANIMASI_STOP;
    anim->waktu_sekarang = 0;
}
