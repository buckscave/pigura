/*
 * PIGURA OS - PENGENDALI.C
 * ========================
 * Modul pengendali (dispatcher) peristiwa Pigura OS.
 *
 * Modul ini menjembatani perangkat masukan mentah dengan
 * subsistem jendela. Bertanggung jawab untuk:
 *   - Menerima event mentah dari driver masukan
 *   - Menerjemahkan event mentah ke event jendela
 *   - Mendispatch event ke jendela target yang tepat
 *   - Mengelola state drag (seret jendela)
 *   - Mendeteksi double-click pada area konten
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
 * KONSTANTA INTERNAL
 * ===========================================================================
 */

/* Interval double-click dalam milidetik */
#define PENGENDALI_DBLKLICK_MS     400

/* Jarak maksimum pergerakan kursor antara klik
   pertama dan kedua untuk dianggap double-click */
#define PENGENDALI_DBLKLIK_JARAK   5

/* Flag event pengendali */
#define PENGENDALI_FLAG_AKTIF     0x01

/*
 * ===========================================================================
 * VARIABEL STATIK - STATE PENGENDALI
 * ===========================================================================
 */

/* Flag status pengendali */
static bool_t g_aktif = SALAH;

/* ID jendela yang sedang di-drag */
static tak_bertanda32_t g_drag_id_jendela = 0;

/* Posisi awal drag (offset dari pojok kiri atas) */
static tak_bertanda32_t g_drag_offset_x = 0;
static tak_bertanda32_t g_drag_offset_y = 0;

/* Flag apakah sedang dalam mode drag */
static bool_t g_sedang_drag = SALAH;

/* State deteksi double-click */
static tak_bertanda64_t g_waktu_klik_terakhir = 0;
static tak_bertanda32_t g_x_klik_terakhir = 0;
static tak_bertanda32_t g_y_klik_terakhir = 0;

/* ID jendela klik terakhir */
static tak_bertanda32_t g_id_klik_terakhir = 0;

/* Statistik pengendali */
static tak_bertanda64_t g_total_mouse_event = 0;
static tak_bertanda64_t g_total_key_event = 0;
static tak_bertanda64_t g_total_drag = 0;

/*
 * ===========================================================================
 * FUNGSI INTERNAL - BUAT EVENT DARI MOUSE
 * ===========================================================================
 *
 * Membuat peristiwa_jendela_t berdasarkan posisi mouse,
 * tombol yang ditekan, dan tipe event.
 */

static status_t buat_event_mouse(tak_bertanda32_t tipe,
                                  tak_bertanda32_t id,
                                  tak_bertanda32_t x,
                                  tak_bertanda32_t y,
                                  tak_bertanda32_t tombol,
                                  tak_bertanda32_t lebar,
                                  tak_bertanda32_t tinggi)
{
    peristiwa_jendela_t evt;

    if (tipe > PERISTIWA_JENDELA_TUTUP_KLIK) {
        return STATUS_PARAM_INVALID;
    }

    kernel_memset(&evt, 0, sizeof(evt));
    evt.tipe = tipe;
    evt.id_jendela = id;
    evt.x = x;
    evt.y = y;
    evt.tombol = tombol;
    evt.lebar = lebar;
    evt.tinggi = tinggi;

    return peristiwa_push(&evt);
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL - CEK DOUBLE-CLICK
 * ===========================================================================
 *
 * Mengecek apakah klik saat ini merupakan double-click
 * berdasarkan waktu, jarak, dan ID jendela yang sama.
 */

static bool_t cek_double_click(tak_bertanda32_t id_jendela,
                                tak_bertanda32_t x,
                                tak_bertanda32_t y,
                                tak_bertanda64_t waktu_sekarang)
{
    tanda32_t dx, dy;

    /* Cek ID jendela sama */
    if (id_jendela != g_id_klik_terakhir) {
        return SALAH;
    }

    /* Cek interval waktu */
    if (waktu_sekarang < g_waktu_klik_terakhir) {
        return SALAH;
    }
    if (waktu_sekarang - g_waktu_klik_terakhir >
        (tak_bertanda64_t)PENGENDALI_DBLKLICK_MS) {
        return SALAH;
    }

    /* Cek jarak pergerakan */
    if (x > g_x_klik_terakhir) {
        dx = (tanda32_t)(x - g_x_klik_terakhir);
    } else {
        dx = (tanda32_t)(g_x_klik_terakhir - x);
    }
    if (y > g_y_klik_terakhir) {
        dy = (tanda32_t)(y - g_y_klik_terakhir);
    } else {
        dy = (tanda32_t)(g_y_klik_terakhir - y);
    }

    if (dx > PENGENDALI_DBLKLIK_JARAK ||
        dy > PENGENDALI_DBLKLIK_JARAK) {
        return SALAH;
    }

    return BENAR;
}

/*
 * ===========================================================================
 * API PUBLIK - INISIALISASI PENGENDALI
 * ===========================================================================
 *
 * Menyiapkan state pengendali event. Dipanggil oleh
 * subsistem peristiwa saat init.
 */

status_t pengendali_init(void)
{
    if (g_aktif) {
        return STATUS_SUDAH_ADA;
    }

    g_drag_id_jendela = 0;
    g_drag_offset_x = 0;
    g_drag_offset_y = 0;
    g_sedang_drag = SALAH;
    g_waktu_klik_terakhir = 0;
    g_x_klik_terakhir = 0;
    g_y_klik_terakhir = 0;
    g_id_klik_terakhir = 0;
    g_total_mouse_event = 0;
    g_total_key_event = 0;
    g_total_drag = 0;

    g_aktif = BENAR;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - SHUTDOWN PENGENDALI
 * ===========================================================================
 *
 * Mematikan pengendali dan mereset semua state.
 */

void pengendali_shutdown(void)
{
    if (!g_aktif) {
        return;
    }

    g_sedang_drag = SALAH;
    g_drag_id_jendela = 0;
    g_aktif = SALAH;
}

/*
 * ===========================================================================
 * API PUBLIK - PROSES EVENT MOUSE
 * ===========================================================================
 *
 * Menerima event mouse mentah dan menerjemahkannya
 * ke event jendela yang sesuai.
 *
 * Alur:
 *   1. Tombol ditekan: mulai drag jika di judul,
 *      cek double-click jika di konten.
 *   2. Tombol dilepas: akhiri drag.
 *   3. Gerakan mouse: update posisi, proses drag.
 *
 * Parameter:
 *   x       - Posisi X kursor saat ini
 *   y       - Posisi Y kursor saat ini
 *   tombol  - Status tombol (bitmask)
 */

status_t pengendali_proses_mouse(tak_bertanda32_t x,
                                  tak_bertanda32_t y,
                                  tak_bertanda32_t tombol)
{
    jendela_t *target;
    tak_bertanda64_t waktu;
    bool_t tombol_kiri;

    if (!g_aktif) {
        return STATUS_TIDAK_DUKUNG;
    }

    g_total_mouse_event++;

    /* Dapatkan waktu saat ini */
    waktu = kernel_get_ticks();

    /* Cek apakah tombol kiri ditekan */
    tombol_kiri = (tombol & MOUSE_BUTTON_LEFT) != 0;

    /*
     * Tangani kondisi drag aktif.
     * Jika sedang drag, update posisi jendela.
     */
    if (g_sedang_drag && tombol_kiri) {
        jendela_t *drag_target;
        tak_bertanda32_t posisi_x, posisi_y;

        drag_target = wm_cari_id(g_drag_id_jendela);
        if (drag_target != NULL) {
            posisi_x = x - g_drag_offset_x;
            posisi_y = y - g_drag_offset_y;
            jendela_pindah(drag_target,
                           posisi_x, posisi_y);
            g_total_drag++;
        }

        /* Jika tombol dilepas selama drag, akhiri */
        if (!(tombol & MOUSE_BUTTON_LEFT)) {
            g_sedang_drag = SALAH;
            g_drag_id_jendela = 0;
        }
        return STATUS_BERHASIL;
    }

    /* Akhiri drag jika tombol dilepas */
    if (!tombol_kiri && g_sedang_drag) {
        g_sedang_drag = SALAH;
        g_drag_id_jendela = 0;
        return STATUS_BERHASIL;
    }

    /* Route mouse untuk mendapatkan target jendela */
    target = peristiwa_route_mouse(x, y, tombol);

    if (target == NULL) {
        return STATUS_BERHASIL;
    }

    /*
     * Tangani klik tombol kiri.
     * Deteksi area klik dan mulai drag jika di judul.
     */
    if (tombol_kiri) {
        tak_bertanda32_t area;
        tak_bertanda32_t wx2, wy2;

        area = jendela_hit_test(target, x, y);

        /* Cek apakah klik di area judul bar */
        if (area == JENDELA_AREA_JUDUL) {
            /* Mulai operasi drag */
            g_sedang_drag = BENAR;
            g_drag_id_jendela = target->id;
            g_drag_offset_x = x - target->x;
            g_drag_offset_y = y - target->y;
            return STATUS_BERHASIL;
        }

        /* Cek apakah klik di area konten */
        if (area == JENDELA_AREA_ISI) {
            wx2 = target->x + target->lebar;
            wy2 = target->y + target->tinggi;

            /* Pastikan masih di dalam jendela */
            if (x >= target->x && x < wx2 &&
                y >= target->y && y < wy2) {
                /*
                 * Cek double-click pada area konten.
                 * Double-click mengirim event GAMBAR.
                 */
                if (cek_double_click(target->id,
                                      x, y, waktu)) {
                    buat_event_mouse(
                        PERISTIWA_JENDELA_GAMBAR,
                        target->id, x, y, tombol,
                        target->lebar, target->tinggi);

                    /* Reset state double-click */
                    g_waktu_klik_terakhir = 0;
                    g_id_klik_terakhir = 0;
                } else {
                    /* Catat klik untuk deteksi berikutnya */
                    g_waktu_klik_terakhir = waktu;
                    g_x_klik_terakhir = x;
                    g_y_klik_terakhir = y;
                    g_id_klik_terakhir = target->id;
                }
            }
        }
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - PROSES EVENT KEYBOARD
 * ===========================================================================
 *
 * Menerima event keyboard mentah dan meneruskan ke
 * jendela yang sedang fokus.
 *
 * Event keyboard hanya dikirim ke jendela yang memiliki
 * flag JENDELA_FLAG_FOKUS dan dalam status VISIBLE.
 *
 * Parameter:
 *   kode     - Kode tombol (scan code atau virtual key)
 *   status   - Status tombol (0=release, 1=press, 2=repeat)
 *   modifier - Flag modifier (Ctrl, Shift, Alt, dll)
 */

status_t pengendali_proses_keyboard(tak_bertanda32_t kode,
                                     tak_bertanda32_t status,
                                     tak_bertanda32_t modifier)
{
    jendela_t *fokus;
    peristiwa_jendela_t evt;

    if (!g_aktif) {
        return STATUS_TIDAK_DUKUNG;
    }

    g_total_key_event++;

    /* Dapatkan jendela fokus */
    fokus = peristiwa_route_keyboard();
    if (fokus == NULL) {
        return STATUS_BERHASIL;
    }

    /*
     * Validasi bahwa jendela fokus aktif dan terlihat.
     * Jika jendela minimize atau invisible, abaikan
     * event keyboard.
     */
    if (fokus->status != JENDELA_STATUS_VISIBLE) {
        return STATUS_BERHASIL;
    }

    if (!(fokus->flags & JENDELA_FLAG_FOKUS)) {
        return STATUS_BERHASIL;
    }

    /*
     * Buat event jendela dari event keyboard.
     * Gunakan tipe GAMBAR untuk memberitahu jendela
     * bahwa ada input yang perlu diproses.
     * Aplikasi dapat membaca state keyboard melalui
     * fungsi papanketik_get_state().
     */
    kernel_memset(&evt, 0, sizeof(evt));
    evt.tipe = PERISTIWA_JENDELA_GAMBAR;
    evt.id_jendela = fokus->id;
    evt.x = 0;
    evt.y = 0;
    evt.tombol = kode;
    evt.lebar = status;
    evt.tinggi = modifier;

    return peristiwa_push(&evt);
}

/*
 * ===========================================================================
 * API PUBLIK - DAPAT STATISTIK PENGENDALI
 * ===========================================================================
 *
 * Mengisi pointer dengan statistik pengendali.
 * Pointer NULL akan diabaikan.
 */

void pengendali_dapat_statistik(tak_bertanda64_t *mouse_event,
                                 tak_bertanda64_t *key_event,
                                 tak_bertanda64_t *drag_count)
{
    if (mouse_event != NULL) {
        *mouse_event = g_total_mouse_event;
    }
    if (key_event != NULL) {
        *key_event = g_total_key_event;
    }
    if (drag_count != NULL) {
        *drag_count = g_total_drag;
    }
}

/*
 * ===========================================================================
 * API PUBLIK - SET STATE DRAG
 * ===========================================================================
 *
 * Mengatur secara paksa state drag pengendali.
 * Digunakan oleh window manager untuk operasi
 * move/resize yang dimulai secara programatik.
 */

status_t pengendali_set_drag(tak_bertanda32_t id_jendela,
                               tak_bertanda32_t offset_x,
                               tak_bertanda32_t offset_y)
{
    if (!g_aktif) {
        return STATUS_TIDAK_DUKUNG;
    }

    g_sedang_drag = BENAR;
    g_drag_id_jendela = id_jendela;
    g_drag_offset_x = offset_x;
    g_drag_offset_y = offset_y;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - HENTIKAN DRAG
 * ===========================================================================
 *
 * Menghentikan operasi drag yang sedang berjalan.
 */

void pengendali_hentikan_drag(void)
{
    g_sedang_drag = SALAH;
    g_drag_id_jendela = 0;
    g_drag_offset_x = 0;
    g_drag_offset_y = 0;
}
