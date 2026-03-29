/*
 * PIGURA OS - FOKUS.C
 * ===================
 * Manajemen fokus jendela subsistem peristiwa Pigura OS.
 *
 * Modul ini mengelola alokasi dan perpindahan fokus keyboard
 * antar jendela. Fokus menentukan jendela mana yang menerima
 * event keyboard dan ditampilkan sebagai jendela aktif.
 *
 * Fitur:
 *   - Pelacakan jendela yang memiliki fokus
 *   - Aktivasi dan deaktivasi jendela
 *   - Rantai fokus (tab order)
 *   - Validasi kelayakan jendela menerima fokus
 *   - Event fokus masuk dan keluar
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

#include "peristiwa.h"

/*
 * ===========================================================================
 * VARIABEL STATIK - STATE FOKUS
 * ===========================================================================
 */

/* Flag status modul fokus */
static bool_t g_fokus_aktif = SALAH;

/* ID jendela yang sedang memiliki fokus (0 = tidak ada) */
static tak_bertanda32_t g_id_fokus = 0;

/* Counter perpindahan fokus (statistik) */
static tak_bertanda64_t g_total_perpindahan = 0;

/* Counter event fokus masuk */
static tak_bertanda64_t g_total_masuk = 0;

/* Counter event fokus keluar */
static tak_bertanda64_t g_total_keluar = 0;

/*
 * ===========================================================================
 * FUNGSI INTERNAL - KIRIM EVENT FOKUS
 * ===========================================================================
 *
 * Membuat dan mengirim event fokus masuk/keluar ke antrian.
 */

static status_t kirim_event_fokus(tak_bertanda32_t tipe,
                                    tak_bertanda32_t id_jendela)
{
    peristiwa_jendela_t evt;

    kernel_memset(&evt, 0, sizeof(evt));
    evt.tipe = tipe;
    evt.id_jendela = id_jendela;
    evt.x = 0;
    evt.y = 0;
    evt.tombol = 0;

    return peristiwa_push(&evt);
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL - CEK LAYAK FOKUS
 * ===========================================================================
 *
 * Memvalidasi apakah jendela layak menerima fokus.
 * Jendela layak jika: valid, visible, bukan modal yang
 * ditutupi modal lain.
 */

static bool_t cek_layak_fokus(const jendela_t *j)
{
    if (j == NULL) {
        return SALAH;
    }

    if (!jendela_valid(j)) {
        return SALAH;
    }

    /* Jendela minimize tidak bisa menerima fokus */
    if (j->status == JENDELA_STATUS_MINIMIZE) {
        return SALAH;
    }

    /* Jendela invisible tidak bisa menerima fokus */
    if (j->status == JENDELA_STATUS_INVISIBLE) {
        return SALAH;
    }

    return BENAR;
}

/*
 * ===========================================================================
 * API PUBLIK - INISIALISASI MODUL FOKUS
 * ===========================================================================
 */

status_t fokus_init(void)
{
    if (g_fokus_aktif) {
        return STATUS_SUDAH_ADA;
    }

    g_id_fokus = 0;
    g_total_perpindahan = 0;
    g_total_masuk = 0;
    g_total_keluar = 0;

    g_fokus_aktif = BENAR;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - SHUTDOWN MODUL FOKUS
 * ===========================================================================
 */

void fokus_shutdown(void)
{
    if (!g_fokus_aktif) {
        return;
    }

    g_id_fokus = 0;
    g_fokus_aktif = SALAH;
}

/*
 * ===========================================================================
 * API PUBLIK - SET FOKUS KE JENDELA
 * ===========================================================================
 *
 * Mengalihkan fokus ke jendela target.
 * Jika jendela lain sedang memiliki fokus, kirim event
 * fokus keluar terlebih dahulu, lalu kirim event fokus
 * masuk ke jendela baru.
 *
 * Parameter:
 *   target - Pointer ke jendela yang akan menerima fokus
 *
 * Return: STATUS_BERHASIL jika berhasil,
 *         STATUS_PARAM_NULL jika target NULL,
 *         STATUS_TIDAK_DUKUNG jika modul belum init.
 */

status_t fokus_set(jendela_t *target)
{
    status_t st;

    if (!g_fokus_aktif) {
        return STATUS_TIDAK_DUKUNG;
    }

    if (target == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Validasi kelayakan */
    if (!cek_layak_fokus(target)) {
        return STATUS_PARAM_INVALID;
    }

    /* Cek apakah jendela sudah memiliki fokus */
    if (g_id_fokus == target->id) {
        return STATUS_BERHASIL;
    }

    /* Kirim event fokus keluar dari jendela lama */
    if (g_id_fokus != 0) {
        kirim_event_fokus(PERISTIWA_JENDELA_FOKUS_KELUAR,
                          g_id_fokus);
        g_total_keluar++;

        /* Hapus flag fokus dari jendela lama */
        {
            jendela_t *lama = wm_cari_id(g_id_fokus);
            if (lama != NULL) {
                lama->flags &=
                    ~(tak_bertanda32_t)JENDELA_FLAG_FOKUS;
            }
        }
    }

    /* Set fokus ke jendela baru */
    g_id_fokus = target->id;
    target->flags |= JENDELA_FLAG_FOKUS;
    target->flags |= JENDELA_FLAG_AKTIF;

    /* Kirim event fokus masuk ke jendela baru */
    st = kirim_event_fokus(PERISTIWA_JENDELA_FOKUS_MASUK,
                           target->id);
    if (st != STATUS_BERHASIL) {
        return st;
    }

    g_total_masuk++;
    g_total_perpindahan++;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - DAPAT JENDELA FOKUS
 * ===========================================================================
 *
 * Mengembalikan ID jendela yang sedang memiliki fokus.
 * Mengembalikan 0 jika tidak ada jendela yang fokus.
 */

tak_bertanda32_t fokus_dapat_id(void)
{
    if (!g_fokus_aktif) {
        return 0;
    }
    return g_id_fokus;
}

/*
 * ===========================================================================
 * API PUBLIK - HAPUS FOKUS
 * ===========================================================================
 *
 * Menghapus fokus dari jendela yang sedang memiliki fokus.
 * Tidak mengalihkan fokus ke jendela lain.
 */

void fokus_hapus(void)
{
    if (!g_fokus_aktif) {
        return;
    }

    if (g_id_fokus != 0) {
        kirim_event_fokus(PERISTIWA_JENDELA_FOKUS_KELUAR,
                          g_id_fokus);
        g_total_keluar++;

        /* Hapus flag fokus dari jendela */
        {
            jendela_t *lama = wm_cari_id(g_id_fokus);
            if (lama != NULL) {
                lama->flags &=
                    ~(tak_bertanda32_t)JENDELA_FLAG_FOKUS;
            }
        }
    }

    g_id_fokus = 0;
    g_total_perpindahan++;
}

/*
 * ===========================================================================
 * API PUBLIK - PINDAH FOKUS KE JENDELA BERIKUTNYA (TAB)
 * ===========================================================================
 *
 * Memindahkan fokus ke jendela berikutnya dalam urutan z-order.
 * Siklus ke jendela paling bawah setelah jendela paling atas.
 * Hanya mempertimbangkan jendela yang visible dan bukan modal.
 *
 * Return: STATUS_BERHASIL jika berhasil pindah,
 *         STATUS_KOSONG jika tidak ada jendela yang layak.
 */

status_t fokus_pindah_berikutnya(void)
{
    jendela_t *fokus_sekarang = NULL;
    jendela_t *berikutnya = NULL;
    jendela_t *pertama = NULL;
    tak_bertanda32_t jumlah;
    tak_bertanda32_t i;
    bool_t melewati_fokus = SALAH;

    if (!g_fokus_aktif) {
        return STATUS_TIDAK_DUKUNG;
    }

    /* Dapatkan jendela fokus saat ini */
    if (g_id_fokus != 0) {
        fokus_sekarang = wm_cari_id(g_id_fokus);
    }

    jumlah = z_order_hitung();

    /*
     * Iterasi semua jendela dari bawah ke atas.
     * Cari jendela pertama yang layak, dan jendela
     * yang berada setelah jendela fokus saat ini.
     */
    for (i = 0; i < jumlah; i++) {
        jendela_t *j = NULL;

        if (i == 0) {
            j = z_order_dapat_atas();
        } else {
            j = z_order_dapat_berikutnya(
                (i == 1) ? z_order_dapat_atas() : NULL);
            if (j == NULL) break;
        }

        if (j == NULL) continue;
        if (!cek_layak_fokus(j)) continue;

        /* Lewati jendela modal */
        if (j->flags & JENDELA_FLAG_MODAL) {
            continue;
        }

        /* Catat jendela pertama yang layak */
        if (pertama == NULL) {
            pertama = j;
        }

        /* Cari jendela setelah fokus saat ini */
        if (melewati_fokus) {
            berikutnya = j;
            break;
        }

        if (fokus_sekarang != NULL &&
            j->id == fokus_sekarang->id) {
            melewati_fokus = BENAR;
        }
    }

    /* Jika tidak ditemukan jendela setelah fokus,
       kembali ke jendela pertama yang layak */
    if (berikutnya == NULL) {
        berikutnya = pertama;
    }

    if (berikutnya == NULL) {
        return STATUS_KOSONG;
    }

    return fokus_set(berikutnya);
}

/*
 * ===========================================================================
 * API PUBLIK - PINDAH FOKUS KE JENDELA SEBELUMNYA
 * ===========================================================================
 *
 * Memindahkan fokus ke jendela sebelumnya dalam urutan z-order.
 * Sama seperti TAB tetapi ke arah sebaliknya (Shift+Tab).
 */

status_t fokus_pindah_sebelumnya(void)
{
    jendela_t *fokus_sekarang = NULL;
    jendela_t *sebelumnya = NULL;
    jendela_t *terakhir = NULL;
    tak_bertanda32_t jumlah;
    tak_bertanda32_t i;

    if (!g_fokus_aktif) {
        return STATUS_TIDAK_DUKUNG;
    }

    if (g_id_fokus != 0) {
        fokus_sekarang = wm_cari_id(g_id_fokus);
    }

    jumlah = z_order_hitung();

    /*
     * Iterasi dari bawah ke atas untuk menemukan
     * jendela terakhir yang layak, lalu cari jendela
     * yang berada sebelum jendela fokus saat ini.
     */
    for (i = 0; i < jumlah; i++) {
        jendela_t *j = NULL;

        if (i == 0) {
            j = z_order_dapat_atas();
        } else {
            j = z_order_dapat_berikutnya(
                (i == 1) ? z_order_dapat_atas() : NULL);
            if (j == NULL) break;
        }

        if (j == NULL) continue;
        if (!cek_layak_fokus(j)) continue;
        if (j->flags & JENDELA_FLAG_MODAL) continue;

        if (fokus_sekarang != NULL &&
            j->id == fokus_sekarang->id) {
            break;
        }

        /* Catat jendela terakhir sebelum fokus */
        sebelumnya = j;
        terakhir = j;
    }

    /* Gunakan jendela sebelumnya, atau terakhir jika
       tidak ada jendela sebelum fokus saat ini */
    if (sebelumnya == NULL && terakhir != NULL) {
        sebelumnya = terakhir;
    }

    if (sebelumnya == NULL) {
        return STATUS_KOSONG;
    }

    return fokus_set(sebelumnya);
}

/*
 * ===========================================================================
 * API PUBLIK - CEK APAKAH JENDELA MEMILIKI FOKUS
 * ===========================================================================
 */

bool_t fokus_milik(const jendela_t *j)
{
    if (!g_fokus_aktif) {
        return SALAH;
    }
    if (j == NULL) {
        return SALAH;
    }
    return (j->id == g_id_fokus) ? BENAR : SALAH;
}

/*
 * ===========================================================================
 * API PUBLIK - VALIDASI FOKUS AKTIF
 * ===========================================================================
 *
 * Memvalidasi bahwa jendela yang memiliki fokus masih
 * valid dan visible. Jika tidak, hapus fokus.
 */

status_t fokus_validasi(void)
{
    jendela_t *target;

    if (!g_fokus_aktif) {
        return STATUS_TIDAK_DUKUNG;
    }

    if (g_id_fokus == 0) {
        return STATUS_BERHASIL;
    }

    target = wm_cari_id(g_id_fokus);
    if (target == NULL) {
        /* Jendela sudah dihapus: hapus fokus */
        fokus_hapus();
        return STATUS_TIDAK_DITEMUKAN;
    }

    if (!cek_layak_fokus(target)) {
        /* Jendela tidak layak: hapus fokus */
        fokus_hapus();
        return STATUS_GAGAL;
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - DAPAT STATISTIK FOKUS
 * ===========================================================================
 */

void fokus_dapat_statistik(tak_bertanda64_t *perpindahan,
                            tak_bertanda64_t *masuk,
                            tak_bertanda64_t *keluar)
{
    if (perpindahan != NULL) {
        *perpindahan = g_total_perpindahan;
    }
    if (masuk != NULL) {
        *masuk = g_total_masuk;
    }
    if (keluar != NULL) {
        *keluar = g_total_keluar;
    }
}
