/*
 * PIGURA OS - TETIKUS.C
 * =====================
 * Implementasi subsistem tetikus (mouse).
 *
 * Berkas ini mengimplementasikan penanganan perangkat tetikus dengan
 * dukungan multi-button, scroll wheel, dan tracking posisi.
 *
 * Kepatuhan standar:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Mendukung semua arsitektur (x86, x86_64, ARM, ARMv7, ARM64)
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "masukan.h"
#include "../../inti/kernel.h"

/*
 * ===========================================================================
 * KONSTANTA TETIKUS
 * ===========================================================================
 */

/* Batasan tetikus */
#define TETIKUS_MAKS_DEVICE        8
#define TETIKUS_ACCELERATION_BASE  1.0

/* Mask tombol */
#define TETIKUS_MASK_KIRI          0x01
#define TETIKUS_MASK_KANAN         0x02
#define TETIKUS_MASK_TENGAH        0x04
#define TETIKUS_MASK_SISI1         0x08
#define TETIKUS_MASK_SISI2         0x10

/* Status tombol */
#define TETIKUS_TOMBOL_DITEKAN     1
#define TETIKUS_TOMBOL_DILEPAS     0

/*
 * ===========================================================================
 * STRUKTUR DATA TETIKUS
 * ===========================================================================
 */

/* Konfigurasi tetikus */
typedef struct {
    tak_bertanda32_t sensitivitas;
    bool_t akselerasi_aktif;
    tanda32_t batas_kecepatan;
} tetikus_konfig_t;

/* Data device tetikus */
typedef struct {
    inputdev_t *dev;
    
    /* Posisi */
    tanda32_t pos_x;
    tanda32_t pos_y;
    
    /* Posisi sebelumnya */
    tanda32_t pos_x_sebelumnya;
    tanda32_t pos_y_sebelumnya;
    
    /* Status tombol */
    tak_bertanda32_t tombol_sebelumnya;
    tak_bertanda32_t tombol_saat_ini;
    
    /* Wheel */
    tanda32_t wheel_accumulator;
    tanda32_t hwheel_accumulator;
    
    /* Konfigurasi */
    tetikus_konfig_t konfig;
    
    /* Status */
    bool_t aktif;
} tetikus_device_t;

/* Konteks tetikus */
typedef struct {
    tetikus_device_t devices[TETIKUS_MAKS_DEVICE];
    tak_bertanda32_t device_count;
    
    /* Posisi global (cursor) */
    tanda32_t cursor_x;
    tanda32_t cursor_y;
    
    /* Batas layar */
    tanda32_t layar_min_x;
    tanda32_t layar_max_x;
    tanda32_t layar_min_y;
    tanda32_t layar_max_y;
    
    bool_t diinisialisasi;
} tetikus_konteks_t;

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

static tetikus_konteks_t g_tetikus_konteks;
static bool_t g_tetikus_diinisialisasi = SALAH;

/*
 * ===========================================================================
 * FUNGSI INTERNAL
 * ===========================================================================
 */

/*
 * tetikus_cari_device - Cari device tetikus berdasarkan ID
 */
static tetikus_device_t *tetikus_cari_device(tak_bertanda32_t id)
{
    tak_bertanda32_t i;
    
    if (!g_tetikus_diinisialisasi) {
        return NULL;
    }
    
    for (i = 0; i < g_tetikus_konteks.device_count; i++) {
        if (g_tetikus_konteks.devices[i].dev != NULL &&
            g_tetikus_konteks.devices[i].dev->id == id) {
            return &g_tetikus_konteks.devices[i];
        }
    }
    
    return NULL;
}

/*
 * tetikus_cari_slot_kosong - Cari slot device kosong
 */
static tak_bertanda32_t tetikus_cari_slot_kosong(void)
{
    tak_bertanda32_t i;
    
    for (i = 0; i < TETIKUS_MAKS_DEVICE; i++) {
        if (g_tetikus_konteks.devices[i].dev == NULL) {
            return i;
        }
    }
    
    return INDEX_INVALID;
}

/*
 * tetikus_hitung_perubahan_tombol - Hitung mask perubahan tombol
 */
static tak_bertanda32_t tetikus_hitung_perubahan_tombol(
    tak_bertanda32_t sebelumnya,
    tak_bertanda32_t saat_ini)
{
    return sebelumnya ^ saat_ini;
}

/*
 * tetikus_terapkan_akselerasi - Terapkan akselerasi mouse
 */
static tanda32_t tetikus_terapkan_akselerasi(tanda32_t delta,
                                              tetikus_konfig_t *konfig)
{
    tanda32_t abs_delta;
    tanda32_t hasil;
    
    if (konfig == NULL || !konfig->akselerasi_aktif) {
        return delta;
    }
    
    if (delta < 0) {
        abs_delta = -delta;
    } else {
        abs_delta = delta;
    }
    
    /* Akselerasi linear sederhana */
    if (abs_delta > konfig->batas_kecepatan) {
        hasil = delta + (delta / 2);
    } else {
        hasil = delta;
    }
    
    return hasil;
}

/*
 * tetikus_clamp_posisi - Batasi posisi ke area layar
 */
static void tetikus_clamp_posisi(tanda32_t *x, tanda32_t *y)
{
    if (x == NULL || y == NULL) {
        return;
    }
    
    if (*x < g_tetikus_konteks.layar_min_x) {
        *x = g_tetikus_konteks.layar_min_x;
    }
    if (*x > g_tetikus_konteks.layar_max_x) {
        *x = g_tetikus_konteks.layar_max_x;
    }
    if (*y < g_tetikus_konteks.layar_min_y) {
        *y = g_tetikus_konteks.layar_min_y;
    }
    if (*y > g_tetikus_konteks.layar_max_y) {
        *y = g_tetikus_konteks.layar_max_y;
    }
}

/*
 * tetikus_buat_event_tombol - Buat event tombol
 */
static void tetikus_buat_event_tombol(tetikus_device_t *tdev,
                                       tak_bertanda32_t tombol_id,
                                       tak_bertanda32_t status)
{
    input_event_t event;
    
    if (tdev == NULL || tdev->dev == NULL) {
        return;
    }
    
    kernel_memset(&event, 0, sizeof(input_event_t));
    
    event.magic = EVENT_MAGIC;
    event.timestamp = kernel_get_ticks();
    event.tipe = EVENT_TIPE_KEY;
    event.device_id = tdev->dev->id;
    
    event.data.key.kode = tombol_id;
    event.data.key.status = status;
    event.data.key.scancode = tombol_id;
    
    masukan_event_push(&event);
    
    if (tdev->dev->callback != NULL) {
        tdev->dev->callback(tdev->dev, &event);
    }
}

/*
 * tetikus_buat_event_gerakan - Buat event gerakan
 */
static void tetikus_buat_event_gerakan(tetikus_device_t *tdev,
                                        tanda32_t dx,
                                        tanda32_t dy,
                                        tanda32_t wheel,
                                        tanda32_t hwheel)
{
    input_event_t event;
    
    if (tdev == NULL || tdev->dev == NULL) {
        return;
    }
    
    kernel_memset(&event, 0, sizeof(input_event_t));
    
    event.magic = EVENT_MAGIC;
    event.timestamp = kernel_get_ticks();
    event.tipe = EVENT_TIPE_RELATIVE;
    event.device_id = tdev->dev->id;
    
    event.data.mouse.tombol = tdev->tombol_saat_ini;
    event.data.mouse.tombol_status = 0;
    event.data.mouse.rel_x = dx;
    event.data.mouse.rel_y = dy;
    event.data.mouse.wheel = wheel;
    event.data.mouse.hwheel = hwheel;
    event.data.mouse.abs_x = (tak_bertanda32_t)g_tetikus_konteks.cursor_x;
    event.data.mouse.abs_y = (tak_bertanda32_t)g_tetikus_konteks.cursor_y;
    
    masukan_event_push(&event);
    
    if (tdev->dev->callback != NULL) {
        tdev->dev->callback(tdev->dev, &event);
    }
}

/*
 * tetikus_proses_tombol - Proses perubahan tombol
 */
static void tetikus_proses_tombol(tetikus_device_t *tdev,
                                   tak_bertanda32_t tombol_baru)
{
    tak_bertanda32_t perubahan;
    tak_bertanda32_t i;
    
    if (tdev == NULL) {
        return;
    }
    
    tdev->tombol_sebelumnya = tdev->tombol_saat_ini;
    tdev->tombol_saat_ini = tombol_baru;
    
    if (tdev->dev != NULL) {
        tdev->dev->button_state = tombol_baru;
    }
    
    perubahan = tetikus_hitung_perubahan_tombol(tdev->tombol_sebelumnya,
                                                 tdev->tombol_saat_ini);
    
    if (perubahan == 0) {
        return;
    }
    
    /* Proses setiap tombol yang berubah */
    for (i = 1; i <= 5; i++) {
        tak_bertanda32_t mask = 1UL << (i - 1);
        
        if (perubahan & mask) {
            tetikus_buat_event_tombol(tdev, i,
                (tombol_baru & mask) ? TETIKUS_TOMBOL_DITEKAN : 
                                        TETIKUS_TOMBOL_DILEPAS);
        }
    }
}

/*
 * tetikus_proses_gerakan - Proses gerakan mouse
 */
static void tetikus_proses_gerakan(tetikus_device_t *tdev,
                                    tanda32_t dx,
                                    tanda32_t dy,
                                    tanda32_t wheel)
{
    if (tdev == NULL) {
        return;
    }
    
    /* Terapkan akselerasi */
    dx = tetikus_terapkan_akselerasi(dx, &tdev->konfig);
    dy = tetikus_terapkan_akselerasi(dy, &tdev->konfig);
    
    /* Update posisi device */
    tdev->pos_x_sebelumnya = tdev->pos_x;
    tdev->pos_y_sebelumnya = tdev->pos_y;
    tdev->pos_x += dx;
    tdev->pos_y += dy;
    
    /* Update posisi cursor global */
    g_tetikus_konteks.cursor_x += dx;
    g_tetikus_konteks.cursor_y += dy;
    
    /* Clamp ke batas layar */
    tetikus_clamp_posisi(&g_tetikus_konteks.cursor_x,
                         &g_tetikus_konteks.cursor_y);
    
    /* Buat event gerakan */
    tetikus_buat_event_gerakan(tdev, dx, dy, wheel, 0);
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

/*
 * tetikus_init - Inisialisasi subsistem mouse
 */
status_t tetikus_init(void)
{
    tak_bertanda32_t i;
    inputdev_t *dev;
    tetikus_device_t *tdev;
    
    if (g_tetikus_diinisialisasi) {
        return STATUS_SUDAH_ADA;
    }
    
    kernel_memset(&g_tetikus_konteks, 0, sizeof(tetikus_konteks_t));
    
    /* Inisialisasi devices */
    for (i = 0; i < TETIKUS_MAKS_DEVICE; i++) {
        g_tetikus_konteks.devices[i].dev = NULL;
        g_tetikus_konteks.devices[i].pos_x = 0;
        g_tetikus_konteks.devices[i].pos_y = 0;
        g_tetikus_konteks.devices[i].tombol_sebelumnya = 0;
        g_tetikus_konteks.devices[i].tombol_saat_ini = 0;
        g_tetikus_konteks.devices[i].wheel_accumulator = 0;
        g_tetikus_konteks.devices[i].hwheel_accumulator = 0;
        g_tetikus_konteks.devices[i].konfig.sensitivitas = 1;
        g_tetikus_konteks.devices[i].konfig.akselerasi_aktif = BENAR;
        g_tetikus_konteks.devices[i].konfig.batas_kecepatan = 10;
        g_tetikus_konteks.devices[i].aktif = SALAH;
    }
    
    /* Set batas layar default */
    g_tetikus_konteks.layar_min_x = 0;
    g_tetikus_konteks.layar_max_x = 1023;
    g_tetikus_konteks.layar_min_y = 0;
    g_tetikus_konteks.layar_max_y = 767;
    g_tetikus_konteks.cursor_x = 512;
    g_tetikus_konteks.cursor_y = 384;
    
    /* Buat device tetikus utama */
    dev = inputdev_register("mouse0", INPUT_TIPE_MOUSE);
    if (dev != NULL) {
        dev->num_buttons = 5;
        dev->num_axes = 4;
        dev->abs_min_x = 0;
        dev->abs_max_x = 1023;
        dev->abs_min_y = 0;
        dev->abs_max_y = 767;
        dev->status = INPUT_STATUS_AKTIF;
        
        tdev = &g_tetikus_konteks.devices[0];
        tdev->dev = dev;
        tdev->aktif = BENAR;
        
        g_tetikus_konteks.device_count = 1;
    }
    
    g_tetikus_konteks.diinisialisasi = BENAR;
    g_tetikus_diinisialisasi = BENAR;
    
    return STATUS_BERHASIL;
}

/*
 * tetikus_event - Proses mouse event
 */
status_t tetikus_event(tak_bertanda32_t buttons, tanda32_t dx,
                        tanda32_t dy, tanda32_t wheel)
{
    tetikus_device_t *tdev;
    
    if (!g_tetikus_diinisialisasi) {
        return STATUS_GAGAL;
    }
    
    /* Gunakan device utama (mouse0) */
    tdev = &g_tetikus_konteks.devices[0];
    
    if (!tdev->aktif || tdev->dev == NULL) {
        return STATUS_GAGAL;
    }
    
    /* Proses tombol */
    tetikus_proses_tombol(tdev, buttons);
    
    /* Proses gerakan */
    tetikus_proses_gerakan(tdev, dx, dy, wheel);
    
    return STATUS_BERHASIL;
}

/*
 * tetikus_get_position - Dapatkan posisi mouse
 */
void tetikus_get_position(tak_bertanda32_t *x, tak_bertanda32_t *y)
{
    if (!g_tetikus_diinisialisasi) {
        if (x != NULL) {
            *x = 0;
        }
        if (y != NULL) {
            *y = 0;
        }
        return;
    }
    
    if (x != NULL) {
        *x = (tak_bertanda32_t)g_tetikus_konteks.cursor_x;
    }
    if (y != NULL) {
        *y = (tak_bertanda32_t)g_tetikus_konteks.cursor_y;
    }
}

/*
 * tetikus_set_position - Set posisi mouse
 */
void tetikus_set_position(tak_bertanda32_t x, tak_bertanda32_t y)
{
    if (!g_tetikus_diinisialisasi) {
        return;
    }
    
    g_tetikus_konteks.cursor_x = (tanda32_t)x;
    g_tetikus_konteks.cursor_y = (tanda32_t)y;
    
    tetikus_clamp_posisi(&g_tetikus_konteks.cursor_x,
                         &g_tetikus_konteks.cursor_y);
}

/*
 * tetikus_get_buttons - Dapatkan state tombol
 */
tak_bertanda32_t tetikus_get_buttons(void)
{
    if (!g_tetikus_diinisialisasi) {
        return 0;
    }
    
    return g_tetikus_konteks.devices[0].tombol_saat_ini;
}

/*
 * tetikus_set_batas_layar - Set batas layar
 */
status_t tetikus_set_batas_layar(tanda32_t min_x, tanda32_t max_x,
                                  tanda32_t min_y, tanda32_t max_y)
{
    if (!g_tetikus_diinisialisasi) {
        return STATUS_GAGAL;
    }
    
    if (min_x >= max_x || min_y >= max_y) {
        return STATUS_PARAM_INVALID;
    }
    
    g_tetikus_konteks.layar_min_x = min_x;
    g_tetikus_konteks.layar_max_x = max_x;
    g_tetikus_konteks.layar_min_y = min_y;
    g_tetikus_konteks.layar_max_y = max_y;
    
    /* Clamp cursor ke batas baru */
    tetikus_clamp_posisi(&g_tetikus_konteks.cursor_x,
                         &g_tetikus_konteks.cursor_y);
    
    return STATUS_BERHASIL;
}

/*
 * tetikus_set_sensitivitas - Set sensitivitas mouse
 */
status_t tetikus_set_sensitivitas(tak_bertanda32_t id,
                                   tak_bertanda32_t sensitivitas)
{
    tetikus_device_t *tdev;
    
    if (!g_tetikus_diinisialisasi) {
        return STATUS_GAGAL;
    }
    
    tdev = tetikus_cari_device(id);
    if (tdev == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    if (sensitivitas > 10) {
        sensitivitas = 10;
    }
    if (sensitivitas < 1) {
        sensitivitas = 1;
    }
    
    tdev->konfig.sensitivitas = sensitivitas;
    
    return STATUS_BERHASIL;
}

/*
 * tetikus_set_akselerasi - Set akselerasi mouse
 */
status_t tetikus_set_akselerasi(tak_bertanda32_t id,
                                 bool_t aktif,
                                 tanda32_t batas_kecepatan)
{
    tetikus_device_t *tdev;
    
    if (!g_tetikus_diinisialisasi) {
        return STATUS_GAGAL;
    }
    
    tdev = tetikus_cari_device(id);
    if (tdev == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    tdev->konfig.akselerasi_aktif = aktif;
    tdev->konfig.batas_kecepatan = batas_kecepatan;
    
    return STATUS_BERHASIL;
}

/*
 * tetikus_registrasi_device - Registrasi device tetikus baru
 */
status_t tetikus_registrasi_device(const char *nama)
{
    inputdev_t *dev;
    tetikus_device_t *tdev;
    tak_bertanda32_t slot;
    char buffer[INPUTDEV_NAMA_MAKS];
    ukuran_t len;
    
    if (!g_tetikus_diinisialisasi) {
        return STATUS_GAGAL;
    }
    
    if (nama == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Cari slot kosong */
    slot = tetikus_cari_slot_kosong();
    if (slot == INDEX_INVALID) {
        return STATUS_PENUH;
    }
    
    /* Buat nama unik jika diperlukan */
    len = kernel_strlen(nama);
    if (len >= INPUTDEV_NAMA_MAKS) {
        kernel_strncpy(buffer, nama, INPUTDEV_NAMA_MAKS - 1);
        buffer[INPUTDEV_NAMA_MAKS - 1] = '\0';
        nama = buffer;
    }
    
    /* Registrasi device */
    dev = inputdev_register(nama, INPUT_TIPE_MOUSE);
    if (dev == NULL) {
        return STATUS_GAGAL;
    }
    
    /* Set kapabilitas */
    dev->num_buttons = 5;
    dev->num_axes = 4;
    dev->abs_min_x = 0;
    dev->abs_max_x = g_tetikus_konteks.layar_max_x;
    dev->abs_min_y = 0;
    dev->abs_max_y = g_tetikus_konteks.layar_max_y;
    dev->status = INPUT_STATUS_AKTIF;
    
    /* Inisialisasi data tetikus */
    tdev = &g_tetikus_konteks.devices[slot];
    tdev->dev = dev;
    tdev->pos_x = 0;
    tdev->pos_y = 0;
    tdev->tombol_sebelumnya = 0;
    tdev->tombol_saat_ini = 0;
    tdev->wheel_accumulator = 0;
    tdev->hwheel_accumulator = 0;
    tdev->konfig.sensitivitas = 1;
    tdev->konfig.akselerasi_aktif = BENAR;
    tdev->konfig.batas_kecepatan = 10;
    tdev->aktif = BENAR;
    
    g_tetikus_konteks.device_count++;
    
    return STATUS_BERHASIL;
}

/*
 * tetikus_hapus_device - Hapus device tetikus
 */
status_t tetikus_hapus_device(tak_bertanda32_t id)
{
    tetikus_device_t *tdev;
    status_t hasil;
    
    if (!g_tetikus_diinisialisasi) {
        return STATUS_GAGAL;
    }
    
    tdev = tetikus_cari_device(id);
    if (tdev == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    /* Unregister dari inputdev */
    hasil = inputdev_unregister(tdev->dev);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    /* Clear data */
    tdev->dev = NULL;
    tdev->aktif = SALAH;
    tdev->tombol_sebelumnya = 0;
    tdev->tombol_saat_ini = 0;
    
    if (g_tetikus_konteks.device_count > 0) {
        g_tetikus_konteks.device_count--;
    }
    
    return STATUS_BERHASIL;
}

/*
 * tetikus_dapat_delta - Dapatkan delta gerakan terakhir
 */
void tetikus_dapat_delta(tak_bertanda32_t id, tanda32_t *dx, tanda32_t *dy)
{
    tetikus_device_t *tdev;
    
    if (dx != NULL) {
        *dx = 0;
    }
    if (dy != NULL) {
        *dy = 0;
    }
    
    if (!g_tetikus_diinisialisasi) {
        return;
    }
    
    tdev = tetikus_cari_device(id);
    if (tdev == NULL) {
        return;
    }
    
    if (dx != NULL) {
        *dx = tdev->pos_x - tdev->pos_x_sebelumnya;
    }
    if (dy != NULL) {
        *dy = tdev->pos_y - tdev->pos_y_sebelumnya;
    }
}

/*
 * tetikus_reset_posisi - Reset posisi ke tengah layar
 */
void tetikus_reset_posisi(void)
{
    if (!g_tetikus_diinisialisasi) {
        return;
    }
    
    g_tetikus_konteks.cursor_x = 
        (g_tetikus_konteks.layar_min_x + g_tetikus_konteks.layar_max_x) / 2;
    g_tetikus_konteks.cursor_y = 
        (g_tetikus_konteks.layar_min_y + g_tetikus_konteks.layar_max_y) / 2;
}

/*
 * tetikus_dapat_jumlah_device - Dapatkan jumlah device tetikus
 */
tak_bertanda32_t tetikus_dapat_jumlah_device(void)
{
    if (!g_tetikus_diinisialisasi) {
        return 0;
    }
    
    return g_tetikus_konteks.device_count;
}
