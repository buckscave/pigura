/*
 * PIGURA OS - PERISTIWA.H
 * =======================
 * Header utama subsistem peristiwa (event system) Pigura OS.
 *
 * Berkas ini mendeklarasikan seluruh fungsi publik dari
 * submodul peristiwa:
 *   - peristiwa  : Antrian event circular buffer
 *   - penunjuk   : Manajemen kursor/pointer
 *   - pengendali : Dispatcher event mentah ke jendela
 *   - masukan    : Pemrosesan masukan dari driver
 *   - fokus      : Manajemen fokus keyboard
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

#ifndef PERISTIWA_H
#define PERISTIWA_H

/*
 * ===========================================================================
 * INCLUDE
 * ===========================================================================
 */

#include "../../inti/kernel.h"
#include "../jendela/jendela.h"
#include "../../perangkat/masukan/masukan.h"

/*
 * ===========================================================================
 * KONSTANTA PENUNJUK (CURSOR)
 * ===========================================================================
 */

/* Tombol mouse - tidak ada yang ditekan */
#define PENUNJUK_TOMBOL_TIDAK_ADA    0

/* Kecepatan default kursor */
#define PENUNJUK_KECEPATAN_DEFAULT   1

/* Batas area clipping minimum */
#define PENUNJUK_KLIP_MIN            0

/* Jumlah maksimum grab simultan */
#define PENUNJUK_GRAB_MAKS           4

/*
 * ===========================================================================
 * KONSTANTA PENGENDALI (DISPATCHER)
 * ===========================================================================
 */

/* Interval double-click dalam milidetik */
#define PENGENDALI_DBLKLICK_MS       400

/* Jarak maks antar klik untuk double-click */
#define PENGENDALI_DBLKLIK_JARAK     5

/*
 * ===========================================================================
 * KONSTANTA MASUKAN (INPUT PROCESSING)
 * ===========================================================================
 */

/* Interval default key repeat dalam milidetik */
#define MASUKAN_REPEAT_DELAY_MS      500

/* Interval antar key repeat dalam milidetik */
#define MASUKAN_REPEAT_RATE_MS       50

/* Waktu tekan tombol dianggap "lama" (hold) */
#define MASUKAN_TAHAN_MS            1000

/* Jumlah maksimum penyangga state keyboard */
#define MASUKAN_KEYS_MAKS           256

/*
 * ===========================================================================
 * DEKLARASI FUNGSI - PENUNJUK (CURSOR)
 * ===========================================================================
 *
 * Manajemen state kursor: posisi, tombol, visibilitas,
 * kecepatan, clipping, dan grab.
 */

/* Inisialisasi subsistem penunjuk */
status_t penunjuk_init(tak_bertanda32_t layar_lebar,
                        tak_bertanda32_t layar_tinggi);

/* Shutdown subsistem penunjuk */
void penunjuk_shutdown(void);

/* Gerakkan kursor secara relatif */
void penunjuk_gerak(tanda32_t dx, tanda32_t dy);

/* Set posisi kursor secara absolut */
void penunjuk_set_posisi(tak_bertanda32_t x,
                          tak_bertanda32_t y);

/* Dapatkan posisi kursor saat ini */
void penunjuk_dapat_posisi(tak_bertanda32_t *x,
                             tak_bertanda32_t *y);

/* Set status tombol kursor */
void penunjuk_set_tombol(tak_bertanda32_t tombol);

/* Dapatkan status tombol kursor */
tak_bertanda32_t penunjuk_dapat_tombol(void);

/* Cek apakah tombol tertentu ditekan */
bool_t penunjuk_tombol_ditekan(
    tak_bertanda32_t tombol_mask);

/* Set visibilitas kursor */
void penunjuk_set_tampil(bool_t tampil);

/* Dapatkan visibilitas kursor */
bool_t penunjuk_dapat_tampil(void);

/* Set kecepatan kursor */
status_t penunjuk_set_kecepatan(
    tak_bertanda32_t kecepatan);

/* Set area clipping kursor */
void penunjuk_set_klip(tak_bertanda32_t x1,
                        tak_bertanda32_t y1,
                        tak_bertanda32_t x2,
                        tak_bertanda32_t y2,
                        bool_t aktif);

/* Grab kursor (ambil alih) */
status_t penunjuk_grab(tak_bertanda32_t id_jendela);

/* Release kursor (lepaskan grab) */
status_t penunjuk_release(tak_bertanda32_t id_jendela);

/* Dapatkan ID jendela pemilik grab */
tak_bertanda32_t penunjuk_dapat_grab(void);

/* Cek apakah grab aktif */
bool_t penunjuk_grab_aktif(void);

/* Dapatkan statistik penunjuk */
void penunjuk_dapat_statistik(
    tak_bertanda64_t *gerakan,
    tak_bertanda64_t *klik,
    tak_bertanda64_t *grab);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI - PENGENDALI (DISPATCHER)
 * ===========================================================================
 *
 * Dispatcher event mentah dari driver ke jendela target.
 * Menangani drag, double-click, dan routing keyboard.
 */

/* Inisialisasi pengendali */
status_t pengendali_init(void);

/* Shutdown pengendali */
void pengendali_shutdown(void);

/* Proses event mouse mentah */
status_t pengendali_proses_mouse(tak_bertanda32_t x,
                                  tak_bertanda32_t y,
                                  tak_bertanda32_t tombol);

/* Proses event keyboard mentah */
status_t pengendali_proses_keyboard(
    tak_bertanda32_t kode,
    tak_bertanda32_t status,
    tak_bertanda32_t modifier);

/* Dapatkan statistik pengendali */
void pengendali_dapat_statistik(
    tak_bertanda64_t *mouse_event,
    tak_bertanda64_t *key_event,
    tak_bertanda64_t *drag_count);

/* Set state drag secara paksa */
status_t pengendali_set_drag(
    tak_bertanda32_t id_jendela,
    tak_bertanda32_t offset_x,
    tak_bertanda32_t offset_y);

/* Hentikan operasi drag */
void pengendali_hentikan_drag(void);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI - MASUKAN (INPUT PROCESSING)
 * ===========================================================================
 *
 * Pemrosesan event mentah dari driver perangkat masukan.
 * Bridge antara driver level dan window manager level.
 */

/* Inisialisasi modul masukan */
status_t masukan_proses_init(void);

/* Shutdown modul masukan */
void masukan_proses_shutdown(void);

/* Proses event masukan generik */
status_t masukan_proses_event(
    const input_event_t *event);

/* Langkah pemrosesan per frame */
status_t masukan_proses_langkah(
    tak_bertanda32_t delta_ms);

/* Cek apakah tombol keyboard ditekan */
bool_t masukan_dapat_key(tak_bertanda32_t kode);

/* Dapatkan modifier flags keyboard */
tak_bertanda32_t masukan_dapat_modifier(void);

/* Set key repeat aktif/tidak aktif */
void masukan_set_key_repeat(bool_t aktif);

/* Dapatkan statistik masukan */
void masukan_dapat_statistik(
    tak_bertanda64_t *key_press,
    tak_bertanda64_t *key_release,
    tak_bertanda64_t *mouse_move,
    tak_bertanda64_t *mouse_click,
    tak_bertanda64_t *repeat_count);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI - FOKUS (KEYBOARD FOCUS)
 * ===========================================================================
 *
 * Manajemen fokus keyboard antar jendela.
 * Termasuk tab order, validasi, dan event fokus.
 */

/* Inisialisasi modul fokus */
status_t fokus_init(void);

/* Shutdown modul fokus */
void fokus_shutdown(void);

/* Set fokus ke jendela target */
status_t fokus_set(jendela_t *target);

/* Dapatkan ID jendela yang memiliki fokus */
tak_bertanda32_t fokus_dapat_id(void);

/* Hapus fokus tanpa mengalihkan */
void fokus_hapus(void);

/* Pindah fokus ke jendela berikutnya (Tab) */
status_t fokus_pindah_berikutnya(void);

/* Pindah fokus ke jendela sebelumnya (Shift+Tab) */
status_t fokus_pindah_sebelumnya(void);

/* Cek apakah jendela memiliki fokus */
bool_t fokus_milik(const jendela_t *j);

/* Validasi fokus masih aktif dan valid */
status_t fokus_validasi(void);

/* Dapatkan statistik fokus */
void fokus_dapat_statistik(
    tak_bertanda64_t *perpindahan,
    tak_bertanda64_t *masuk,
    tak_bertanda64_t *keluar);

#endif /* PERISTIWA_H */
