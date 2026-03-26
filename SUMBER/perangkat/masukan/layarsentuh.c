/*
 * PIGURA OS - LAYARSENTUH.C
 * =========================
 * Implementasi subsistem layarsentuh (touchscreen).
 *
 * Berkas ini mengimplementasikan penanganan perangkat touchscreen dengan
 * dukungan multi-touch, gesture detection, dan calibrasi.
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
 * KONSTANTA LAYARSENTUH
 * ===========================================================================
 */

/* Batasan touchscreen */
#define LAYARSENTUH_MAKS_DEVICE       4
#define LAYARSENTUH_MAX_TOUCHES       10
#define LAYARSENTUH_TAP_THRESHOLD     50
#define LAYARSENTUH_TAP_TIME          300

/* Gesture types */
#define GESTURE_NONE                  0
#define GESTURE_TAP                   1
#define GESTURE_DOUBLE_TAP            2
#define GESTURE_LONG_PRESS            3
#define GESTURE_SWIPE_LEFT            4
#define GESTURE_SWIPE_RIGHT           5
#define GESTURE_SWIPE_UP              6
#define GESTURE_SWIPE_DOWN            7
#define GESTURE_PINCH                 8
#define GESTURE_SPREAD                9

/* Pressure values */
#define LAYARSENTUH_PRESSURE_MIN      0
#define LAYARSENTUH_PRESSURE_MAX      255
#define LAYARSENTUH_PRESSURE_DEFAULT  128

/*
 * ===========================================================================
 * STRUKTUR DATA LAYARSENTUH
 * ===========================================================================
 */

/* Data touch point */
typedef struct {
    tak_bertanda32_t id;
    bool_t aktif;
    bool_t sebelumnya_aktif;
    
    /* Posisi */
    tak_bertanda32_t x;
    tak_bertanda32_t y;
    tak_bertanda32_t x_sebelumnya;
    tak_bertanda32_t y_sebelumnya;
    
    /* Pressure dan area */
    tak_bertanda32_t pressure;
    tak_bertanda32_t area;
    
    /* Timing */
    tak_bertanda64_t waktu_mulai;
    tak_bertanda64_t waktu_akhir;
    
    /* Gesture tracking */
    tak_bertanda32_t start_x;
    tak_bertanda32_t start_y;
    bool_t moved;
} touch_point_t;

/* Data kalibrasi */
typedef struct {
    tak_bertanda32_t min_x;
    tak_bertanda32_t max_x;
    tak_bertanda32_t min_y;
    tak_bertanda32_t max_y;
    bool_t dikalibrasi;
} kalibrasi_t;

/* Data device touchscreen */
typedef struct {
    inputdev_t *dev;
    
    /* Touch points */
    touch_point_t touches[LAYARSENTUH_MAX_TOUCHES];
    tak_bertanda32_t touch_count;
    
    /* Kalibrasi */
    kalibrasi_t kalibrasi;
    
    /* Batas layar */
    tak_bertanda32_t screen_width;
    tak_bertanda32_t screen_height;
    
    /* Konfigurasi */
    tak_bertanda32_t tap_threshold;
    tak_bertanda32_t tap_time;
    bool_t gesture_detection;
    
    /* Status */
    bool_t aktif;
} layarsentuh_device_t;

/* Konteks touchscreen */
typedef struct {
    layarsentuh_device_t devices[LAYARSENTUH_MAKS_DEVICE];
    tak_bertanda32_t device_count;
    bool_t diinisialisasi;
} layarsentuh_konteks_t;

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

static layarsentuh_konteks_t g_layarsentuh_konteks;
static bool_t g_layarsentuh_diinisialisasi = SALAH;

/*
 * ===========================================================================
 * FUNGSI INTERNAL
 * ===========================================================================
 */

/*
 * layarsentuh_cari_device - Cari device touchscreen berdasarkan ID
 */
static layarsentuh_device_t *layarsentuh_cari_device(tak_bertanda32_t id)
{
    tak_bertanda32_t i;
    
    if (!g_layarsentuh_diinisialisasi) {
        return NULL;
    }
    
    for (i = 0; i < g_layarsentuh_konteks.device_count; i++) {
        if (g_layarsentuh_konteks.devices[i].dev != NULL &&
            g_layarsentuh_konteks.devices[i].dev->id == id) {
            return &g_layarsentuh_konteks.devices[i];
        }
    }
    
    return NULL;
}

/*
 * layarsentuh_cari_slot_kosong - Cari slot device kosong
 */
static tak_bertanda32_t layarsentuh_cari_slot_kosong(void)
{
    tak_bertanda32_t i;
    
    for (i = 0; i < LAYARSENTUH_MAKS_DEVICE; i++) {
        if (g_layarsentuh_konteks.devices[i].dev == NULL) {
            return i;
        }
    }
    
    return INDEX_INVALID;
}

/*
 * layarsentuh_cari_touch_point - Cari touch point berdasarkan ID
 */
static touch_point_t *layarsentuh_cari_touch_point(layarsentuh_device_t *ldev,
                                                    tak_bertanda32_t id)
{
    tak_bertanda32_t i;
    
    if (ldev == NULL) {
        return NULL;
    }
    
    for (i = 0; i < LAYARSENTUH_MAX_TOUCHES; i++) {
        if (ldev->touches[i].aktif && ldev->touches[i].id == id) {
            return &ldev->touches[i];
        }
    }
    
    return NULL;
}

/*
 * layarsentuh_cari_slot_touch - Cari slot touch kosong
 */
static tak_bertanda32_t layarsentuh_cari_slot_touch(layarsentuh_device_t *ldev)
{
    tak_bertanda32_t i;
    
    if (ldev == NULL) {
        return INDEX_INVALID;
    }
    
    for (i = 0; i < LAYARSENTUH_MAX_TOUCHES; i++) {
        if (!ldev->touches[i].aktif) {
            return i;
        }
    }
    
    return INDEX_INVALID;
}

/*
 * layarsentuh_kalibrasi_koordinat - Kalibrasi koordinat touch
 */
static void layarsentuh_kalibrasi_koordinat(layarsentuh_device_t *ldev,
                                             tak_bertanda32_t *x,
                                             tak_bertanda32_t *y)
{
    tak_bertanda32_t calib_x;
    tak_bertanda32_t calib_y;
    
    if (ldev == NULL || x == NULL || y == NULL) {
        return;
    }
    
    if (!ldev->kalibrasi.dikalibrasi) {
        return;
    }
    
    /* Kalibrasi X */
    if (ldev->kalibrasi.max_x != ldev->kalibrasi.min_x) {
        calib_x = (*x - ldev->kalibrasi.min_x) * ldev->screen_width /
                  (ldev->kalibrasi.max_x - ldev->kalibrasi.min_x);
    } else {
        calib_x = *x;
    }
    
    /* Kalibrasi Y */
    if (ldev->kalibrasi.max_y != ldev->kalibrasi.min_y) {
        calib_y = (*y - ldev->kalibrasi.min_y) * ldev->screen_height /
                  (ldev->kalibrasi.max_y - ldev->kalibrasi.min_y);
    } else {
        calib_y = *y;
    }
    
    /* Clamp ke batas layar */
    if (calib_x > ldev->screen_width) {
        calib_x = ldev->screen_width;
    }
    if (calib_y > ldev->screen_height) {
        calib_y = ldev->screen_height;
    }
    
    *x = calib_x;
    *y = calib_y;
}

/*
 * layarsentuh_buat_event - Buat dan kirim touch event
 */
static void layarsentuh_buat_event(layarsentuh_device_t *ldev,
                                    touch_point_t *touch,
                                    tak_bertanda32_t tipe)
{
    input_event_t event;
    
    if (ldev == NULL || ldev->dev == NULL || touch == NULL) {
        return;
    }
    
    kernel_memset(&event, 0, sizeof(input_event_t));
    
    event.magic = EVENT_MAGIC;
    event.timestamp = kernel_get_ticks();
    event.tipe = EVENT_TIPE_TOUCH;
    event.device_id = ldev->dev->id;
    
    event.data.touch.id = touch->id;
    event.data.touch.tipe = tipe;
    event.data.touch.x = touch->x;
    event.data.touch.y = touch->y;
    event.data.touch.pressure = touch->pressure;
    event.data.touch.area = touch->area;
    
    masukan_event_push(&event);
    
    if (ldev->dev->callback != NULL) {
        ldev->dev->callback(ldev->dev, &event);
    }
}

/*
 * layarsentuh_detect_gesture - Deteksi gesture sederhana
 */
static tak_bertanda32_t layarsentuh_detect_gesture(layarsentuh_device_t *ldev,
                                                    touch_point_t *touch)
{
    tak_bertanda32_t dx;
    tak_bertanda32_t dy;
    tak_bertanda64_t durasi;
    
    if (ldev == NULL || touch == NULL) {
        return GESTURE_NONE;
    }
    
    if (!ldev->gesture_detection) {
        return GESTURE_NONE;
    }
    
    /* Hitung durasi */
    durasi = touch->waktu_akhir - touch->waktu_mulai;
    
    /* Cek tap */
    if (!touch->moved && durasi < ldev->tap_time) {
        return GESTURE_TAP;
    }
    
    /* Cek long press */
    if (!touch->moved && durasi > 500) {
        return GESTURE_LONG_PRESS;
    }
    
    /* Cek swipe */
    dx = (touch->x > touch->start_x) ? 
         (touch->x - touch->start_x) : (touch->start_x - touch->x);
    dy = (touch->y > touch->start_y) ? 
         (touch->y - touch->start_y) : (touch->start_y - touch->y);
    
    if (dx > ldev->tap_threshold || dy > ldev->tap_threshold) {
        if (dx > dy) {
            /* Horizontal swipe */
            if (touch->x > touch->start_x) {
                return GESTURE_SWIPE_RIGHT;
            } else {
                return GESTURE_SWIPE_LEFT;
            }
        } else {
            /* Vertical swipe */
            if (touch->y > touch->start_y) {
                return GESTURE_SWIPE_DOWN;
            } else {
                return GESTURE_SWIPE_UP;
            }
        }
    }
    
    return GESTURE_NONE;
}

/*
 * layarsentuh_touch_down - Proses touch down
 */
static void layarsentuh_touch_down(layarsentuh_device_t *ldev,
                                    tak_bertanda32_t id,
                                    tak_bertanda32_t x,
                                    tak_bertanda32_t y,
                                    tak_bertanda32_t pressure)
{
    touch_point_t *touch;
    tak_bertanda32_t slot;
    
    if (ldev == NULL) {
        return;
    }
    
    /* Cari slot kosong */
    slot = layarsentuh_cari_slot_touch(ldev);
    if (slot == INDEX_INVALID) {
        return;
    }
    
    touch = &ldev->touches[slot];
    
    /* Inisialisasi touch point */
    touch->id = id;
    touch->aktif = BENAR;
    touch->sebelumnya_aktif = SALAH;
    
    /* Kalibrasi koordinat */
    layarsentuh_kalibrasi_koordinat(ldev, &x, &y);
    
    touch->x = x;
    touch->y = y;
    touch->x_sebelumnya = x;
    touch->y_sebelumnya = y;
    touch->start_x = x;
    touch->start_y = y;
    
    touch->pressure = pressure;
    touch->area = 0;
    
    touch->waktu_mulai = kernel_get_ticks();
    touch->waktu_akhir = touch->waktu_mulai;
    
    touch->moved = SALAH;
    
    /* Update touch count */
    ldev->touch_count++;
    if (ldev->dev != NULL) {
        ldev->dev->touch_count = ldev->touch_count;
    }
    
    /* Buat event */
    layarsentuh_buat_event(ldev, touch, TOUCH_EVENT_DOWN);
}

/*
 * layarsentuh_touch_move - Proses touch move
 */
static void layarsentuh_touch_move(layarsentuh_device_t *ldev,
                                    tak_bertanda32_t id,
                                    tak_bertanda32_t x,
                                    tak_bertanda32_t y,
                                    tak_bertanda32_t pressure)
{
    touch_point_t *touch;
    tak_bertanda32_t dx;
    tak_bertanda32_t dy;
    
    if (ldev == NULL) {
        return;
    }
    
    touch = layarsentuh_cari_touch_point(ldev, id);
    if (touch == NULL) {
        return;
    }
    
    /* Simpan posisi sebelumnya */
    touch->x_sebelumnya = touch->x;
    touch->y_sebelumnya = touch->y;
    
    /* Kalibrasi koordinat */
    layarsentuh_kalibrasi_koordinat(ldev, &x, &y);
    
    touch->x = x;
    touch->y = y;
    touch->pressure = pressure;
    touch->waktu_akhir = kernel_get_ticks();
    
    /* Cek pergerakan */
    dx = (x > touch->start_x) ? 
         (x - touch->start_x) : (touch->start_x - x);
    dy = (y > touch->start_y) ? 
         (y - touch->start_y) : (touch->start_y - y);
    
    if (dx > ldev->tap_threshold || dy > ldev->tap_threshold) {
        touch->moved = BENAR;
    }
    
    /* Buat event */
    layarsentuh_buat_event(ldev, touch, TOUCH_EVENT_MOVE);
}

/*
 * layarsentuh_touch_up - Proses touch up
 */
static void layarsentuh_touch_up(layarsentuh_device_t *ldev,
                                  tak_bertanda32_t id)
{
    touch_point_t *touch;
    tak_bertanda32_t gesture;
    
    if (ldev == NULL) {
        return;
    }
    
    touch = layarsentuh_cari_touch_point(ldev, id);
    if (touch == NULL) {
        return;
    }
    
    touch->waktu_akhir = kernel_get_ticks();
    
    /* Deteksi gesture */
    gesture = layarsentuh_detect_gesture(ldev, touch);
    
    /* Buat event UP */
    layarsentuh_buat_event(ldev, touch, TOUCH_EVENT_UP);
    
    /* Buat event gesture jika terdeteksi */
    if (gesture != GESTURE_NONE && ldev->gesture_detection) {
        /* TODO: Kirim event gesture */
    }
    
    /* Nonaktifkan touch point */
    touch->sebelumnya_aktif = touch->aktif;
    touch->aktif = SALAH;
    
    /* Update touch count */
    if (ldev->touch_count > 0) {
        ldev->touch_count--;
    }
    if (ldev->dev != NULL) {
        ldev->dev->touch_count = ldev->touch_count;
    }
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

/*
 * layarsentuh_init - Inisialisasi subsistem touchscreen
 */
status_t layarsentuh_init(void)
{
    tak_bertanda32_t i;
    tak_bertanda32_t j;
    
    if (g_layarsentuh_diinisialisasi) {
        return STATUS_SUDAH_ADA;
    }
    
    kernel_memset(&g_layarsentuh_konteks, 0, sizeof(layarsentuh_konteks_t));
    
    /* Inisialisasi devices */
    for (i = 0; i < LAYARSENTUH_MAKS_DEVICE; i++) {
        g_layarsentuh_konteks.devices[i].dev = NULL;
        g_layarsentuh_konteks.devices[i].touch_count = 0;
        g_layarsentuh_konteks.devices[i].screen_width = 1024;
        g_layarsentuh_konteks.devices[i].screen_height = 768;
        g_layarsentuh_konteks.devices[i].tap_threshold = LAYARSENTUH_TAP_THRESHOLD;
        g_layarsentuh_konteks.devices[i].tap_time = LAYARSENTUH_TAP_TIME;
        g_layarsentuh_konteks.devices[i].gesture_detection = BENAR;
        g_layarsentuh_konteks.devices[i].aktif = SALAH;
        
        /* Inisialisasi kalibrasi */
        g_layarsentuh_konteks.devices[i].kalibrasi.min_x = 0;
        g_layarsentuh_konteks.devices[i].kalibrasi.max_x = 4095;
        g_layarsentuh_konteks.devices[i].kalibrasi.min_y = 0;
        g_layarsentuh_konteks.devices[i].kalibrasi.max_y = 4095;
        g_layarsentuh_konteks.devices[i].kalibrasi.dikalibrasi = BENAR;
        
        /* Inisialisasi touch points */
        for (j = 0; j < LAYARSENTUH_MAX_TOUCHES; j++) {
            g_layarsentuh_konteks.devices[i].touches[j].id = 0;
            g_layarsentuh_konteks.devices[i].touches[j].aktif = SALAH;
            g_layarsentuh_konteks.devices[i].touches[j].sebelumnya_aktif = SALAH;
            g_layarsentuh_konteks.devices[i].touches[j].x = 0;
            g_layarsentuh_konteks.devices[i].touches[j].y = 0;
            g_layarsentuh_konteks.devices[i].touches[j].pressure = 0;
        }
    }
    
    g_layarsentuh_konteks.device_count = 0;
    g_layarsentuh_konteks.diinisialisasi = BENAR;
    g_layarsentuh_diinisialisasi = BENAR;
    
    return STATUS_BERHASIL;
}

/*
 * layarsentuh_event - Proses touch event
 */
status_t layarsentuh_event(tak_bertanda32_t id, tak_bertanda32_t tipe,
                            tak_bertanda32_t x, tak_bertanda32_t y,
                            tak_bertanda32_t pressure)
{
    layarsentuh_device_t *ldev;
    
    if (!g_layarsentuh_diinisialisasi) {
        return STATUS_GAGAL;
    }
    
    /* Gunakan device utama (index 0) */
    ldev = &g_layarsentuh_konteks.devices[0];
    
    if (!ldev->aktif || ldev->dev == NULL) {
        return STATUS_GAGAL;
    }
    
    /* Proses berdasarkan tipe event */
    switch (tipe) {
    case TOUCH_EVENT_DOWN:
        layarsentuh_touch_down(ldev, id, x, y, pressure);
        break;
        
    case TOUCH_EVENT_MOVE:
        layarsentuh_touch_move(ldev, id, x, y, pressure);
        break;
        
    case TOUCH_EVENT_UP:
        layarsentuh_touch_up(ldev, id);
        break;
        
    case TOUCH_EVENT_CANCEL:
        layarsentuh_touch_up(ldev, id);
        break;
        
    default:
        return STATUS_PARAM_INVALID;
    }
    
    return STATUS_BERHASIL;
}

/*
 * layarsentuh_get_touches - Dapatkan touch points aktif
 */
tak_bertanda32_t layarsentuh_get_touches(touch_event_t *touches,
                                          tak_bertanda32_t max_count)
{
    layarsentuh_device_t *ldev;
    tak_bertanda32_t i;
    tak_bertanda32_t count;
    
    if (!g_layarsentuh_diinisialisasi) {
        return 0;
    }
    
    if (touches == NULL || max_count == 0) {
        return 0;
    }
    
    ldev = &g_layarsentuh_konteks.devices[0];
    
    count = 0;
    
    for (i = 0; i < LAYARSENTUH_MAX_TOUCHES && count < max_count; i++) {
        if (ldev->touches[i].aktif) {
            touches[count].id = ldev->touches[i].id;
            touches[count].tipe = TOUCH_EVENT_MOVE;
            touches[count].x = ldev->touches[i].x;
            touches[count].y = ldev->touches[i].y;
            touches[count].pressure = ldev->touches[i].pressure;
            touches[count].area = ldev->touches[i].area;
            
            count++;
        }
    }
    
    return count;
}

/*
 * layarsentuh_registrasi_device - Registrasi device touchscreen baru
 */
status_t layarsentuh_registrasi_device(const char *nama,
                                        tak_bertanda32_t screen_width,
                                        tak_bertanda32_t screen_height)
{
    inputdev_t *dev;
    layarsentuh_device_t *ldev;
    tak_bertanda32_t slot;
    char buffer[INPUTDEV_NAMA_MAKS];
    ukuran_t len;
    
    if (!g_layarsentuh_diinisialisasi) {
        return STATUS_GAGAL;
    }
    
    if (nama == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Cari slot kosong */
    slot = layarsentuh_cari_slot_kosong();
    if (slot == INDEX_INVALID) {
        return STATUS_PENUH;
    }
    
    /* Buat nama unik */
    len = kernel_strlen(nama);
    if (len >= INPUTDEV_NAMA_MAKS) {
        kernel_strncpy(buffer, nama, INPUTDEV_NAMA_MAKS - 1);
        buffer[INPUTDEV_NAMA_MAKS - 1] = '\0';
        nama = buffer;
    }
    
    /* Registrasi device */
    dev = inputdev_register(nama, INPUT_TIPE_TOUCHSCREEN);
    if (dev == NULL) {
        return STATUS_GAGAL;
    }
    
    /* Set kapabilitas */
    dev->num_keys = 0;
    dev->num_buttons = 0;
    dev->num_axes = 2;
    dev->num_touches = LAYARSENTUH_MAX_TOUCHES;
    dev->abs_min_x = 0;
    dev->abs_max_x = screen_width;
    dev->abs_min_y = 0;
    dev->abs_max_y = screen_height;
    dev->status = INPUT_STATUS_AKTIF;
    
    /* Inisialisasi data touchscreen */
    ldev = &g_layarsentuh_konteks.devices[slot];
    ldev->dev = dev;
    ldev->touch_count = 0;
    ldev->screen_width = screen_width;
    ldev->screen_height = screen_height;
    ldev->kalibrasi.min_x = 0;
    ldev->kalibrasi.max_x = 4095;
    ldev->kalibrasi.min_y = 0;
    ldev->kalibrasi.max_y = 4095;
    ldev->kalibrasi.dikalibrasi = BENAR;
    ldev->aktif = BENAR;
    
    g_layarsentuh_konteks.device_count++;
    
    return STATUS_BERHASIL;
}

/*
 * layarsentuh_hapus_device - Hapus device touchscreen
 */
status_t layarsentuh_hapus_device(tak_bertanda32_t id)
{
    layarsentuh_device_t *ldev;
    status_t hasil;
    
    if (!g_layarsentuh_diinisialisasi) {
        return STATUS_GAGAL;
    }
    
    ldev = layarsentuh_cari_device(id);
    if (ldev == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    /* Unregister dari inputdev */
    hasil = inputdev_unregister(ldev->dev);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    /* Clear data */
    ldev->dev = NULL;
    ldev->aktif = SALAH;
    ldev->touch_count = 0;
    
    if (g_layarsentuh_konteks.device_count > 0) {
        g_layarsentuh_konteks.device_count--;
    }
    
    return STATUS_BERHASIL;
}

/*
 * layarsentuh_set_kalibrasi - Set data kalibrasi
 */
status_t layarsentuh_set_kalibrasi(tak_bertanda32_t id,
                                    tak_bertanda32_t min_x,
                                    tak_bertanda32_t max_x,
                                    tak_bertanda32_t min_y,
                                    tak_bertanda32_t max_y)
{
    layarsentuh_device_t *ldev;
    
    if (!g_layarsentuh_diinisialisasi) {
        return STATUS_GAGAL;
    }
    
    ldev = layarsentuh_cari_device(id);
    if (ldev == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    if (min_x >= max_x || min_y >= max_y) {
        return STATUS_PARAM_INVALID;
    }
    
    ldev->kalibrasi.min_x = min_x;
    ldev->kalibrasi.max_x = max_x;
    ldev->kalibrasi.min_y = min_y;
    ldev->kalibrasi.max_y = max_y;
    ldev->kalibrasi.dikalibrasi = BENAR;
    
    return STATUS_BERHASIL;
}

/*
 * layarsentuh_set_ukuran_layar - Set ukuran layar
 */
status_t layarsentuh_set_ukuran_layar(tak_bertanda32_t id,
                                       tak_bertanda32_t width,
                                       tak_bertanda32_t height)
{
    layarsentuh_device_t *ldev;
    
    if (!g_layarsentuh_diinisialisasi) {
        return STATUS_GAGAL;
    }
    
    ldev = layarsentuh_cari_device(id);
    if (ldev == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    if (width == 0 || height == 0) {
        return STATUS_PARAM_INVALID;
    }
    
    ldev->screen_width = width;
    ldev->screen_height = height;
    
    if (ldev->dev != NULL) {
        ldev->dev->abs_max_x = width;
        ldev->dev->abs_max_y = height;
    }
    
    return STATUS_BERHASIL;
}

/*
 * layarsentuh_set_gesture_detection - Aktifkan/nonaktifkan gesture detection
 */
status_t layarsentuh_set_gesture_detection(tak_bertanda32_t id,
                                            bool_t aktif)
{
    layarsentuh_device_t *ldev;
    
    if (!g_layarsentuh_diinisialisasi) {
        return STATUS_GAGAL;
    }
    
    ldev = layarsentuh_cari_device(id);
    if (ldev == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    ldev->gesture_detection = aktif;
    
    return STATUS_BERHASIL;
}

/*
 * layarsentuh_dapat_jumlah_device - Dapatkan jumlah device touchscreen
 */
tak_bertanda32_t layarsentuh_dapat_jumlah_device(void)
{
    if (!g_layarsentuh_diinisialisasi) {
        return 0;
    }
    
    return g_layarsentuh_konteks.device_count;
}

/*
 * layarsentuh_dapat_touch_count - Dapatkan jumlah touch aktif
 */
tak_bertanda32_t layarsentuh_dapat_touch_count(tak_bertanda32_t id)
{
    layarsentuh_device_t *ldev;
    
    if (!g_layarsentuh_diinisialisasi) {
        return 0;
    }
    
    ldev = layarsentuh_cari_device(id);
    if (ldev == NULL) {
        return 0;
    }
    
    return ldev->touch_count;
}
