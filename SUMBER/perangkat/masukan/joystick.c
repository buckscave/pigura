/*
 * PIGURA OS - JOYSTICK.C
 * ======================
 * Implementasi subsistem joystick.
 *
 * Berkas ini mengimplementasikan penanganan joystick analog dan digital
 * dengan dukungan multi-axis dan multi-button.
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
 * KONSTANTA JOYSTICK
 * ===========================================================================
 */

/* Batasan joystick */
#define JOYSTICK_MAKS_DEVICE        8
#define JOYSTICK_AXIS_DEADZONE      1000
#define JOYSTICK_AXIS_MAKS          32767
#define JOYSTICK_AXIS_MIN           -32768

/* Status tombol */
#define JOYSTICK_TOMBOL_DITEKAN     1
#define JOYSTICK_TOMBOL_DILEPAS     0

/* Konstanta kalibrasi */
#define JOYSTICK_CALIB_CENTER       0
#define JOYSTICK_CALIB_RANGE        32767

/*
 * ===========================================================================
 * STRUKTUR DATA JOYSTICK
 * ===========================================================================
 */

/* Data kalibrasi axis */
typedef struct {
    tanda32_t min_val;
    tanda32_t center_val;
    tanda32_t max_val;
    bool_t dikalibrasi;
} joystick_calib_t;

/* Data device joystick */
typedef struct {
    inputdev_t *dev;
    
    /* Status tombol */
    tak_bertanda32_t tombol_sebelumnya;
    tak_bertanda32_t tombol_saat_ini;
    
    /* Status axis */
    tanda32_t axis_sebelumnya[INPUT_AXIS_MAKS];
    tanda32_t axis_saat_ini[INPUT_AXIS_MAKS];
    
    /* Kalibrasi */
    joystick_calib_t calib[INPUT_AXIS_MAKS];
    
    /* Konfigurasi */
    tak_bertanda32_t deadzone;
    bool_t aktif;
} joystick_device_t;

/* Konteks joystick */
typedef struct {
    joystick_device_t devices[JOYSTICK_MAKS_DEVICE];
    tak_bertanda32_t device_count;
    bool_t diinisialisasi;
} joystick_konteks_t;

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

static joystick_konteks_t g_joystick_konteks;
static bool_t g_joystick_diinisialisasi = SALAH;

/*
 * ===========================================================================
 * FUNGSI INTERNAL
 * ===========================================================================
 */

/*
 * joystick_cari_device - Cari device joystick berdasarkan ID
 */
static joystick_device_t *joystick_cari_device(tak_bertanda32_t id)
{
    tak_bertanda32_t i;
    
    if (!g_joystick_diinisialisasi) {
        return NULL;
    }
    
    for (i = 0; i < g_joystick_konteks.device_count; i++) {
        if (g_joystick_konteks.devices[i].dev != NULL &&
            g_joystick_konteks.devices[i].dev->id == id) {
            return &g_joystick_konteks.devices[i];
        }
    }
    
    return NULL;
}

/*
 * joystick_cari_slot_kosong - Cari slot device kosong
 */
static tak_bertanda32_t joystick_cari_slot_kosong(void)
{
    tak_bertanda32_t i;
    
    for (i = 0; i < JOYSTICK_MAKS_DEVICE; i++) {
        if (g_joystick_konteks.devices[i].dev == NULL) {
            return i;
        }
    }
    
    return INDEX_INVALID;
}

/*
 * joystick_inisialisasi_calib - Inisialisasi data kalibrasi
 */
static void joystick_inisialisasi_calib(joystick_device_t *jdev)
{
    tak_bertanda32_t i;
    
    if (jdev == NULL) {
        return;
    }
    
    for (i = 0; i < INPUT_AXIS_MAKS; i++) {
        jdev->calib[i].min_val = JOYSTICK_AXIS_MIN;
        jdev->calib[i].center_val = JOYSTICK_CALIB_CENTER;
        jdev->calib[i].max_val = JOYSTICK_AXIS_MAKS;
        jdev->calib[i].dikalibrasi = SALAH;
    }
}

/*
 * joystick_terapkan_deadzone - Terapkan deadzone pada nilai axis
 */
static tanda32_t joystick_terapkan_deadzone(tanda32_t nilai,
                                             tak_bertanda32_t deadzone)
{
    tanda32_t abs_nilai;
    
    if (nilai < 0) {
        abs_nilai = -nilai;
    } else {
        abs_nilai = nilai;
    }
    
    if (abs_nilai < (tanda32_t)deadzone) {
        return 0;
    }
    
    return nilai;
}

/*
 * joystick_normalisasi_axis - Normalisasi nilai axis
 */
static tanda32_t joystick_normalisasi_axis(tanda32_t nilai,
                                            joystick_calib_t *calib)
{
    tanda32_t hasil;
    
    if (calib == NULL) {
        return nilai;
    }
    
    if (nilai >= calib->center_val) {
        if (calib->max_val == calib->center_val) {
            return 0;
        }
        
        hasil = (nilai - calib->center_val) * JOYSTICK_CALIB_RANGE /
                (calib->max_val - calib->center_val);
    } else {
        if (calib->center_val == calib->min_val) {
            return 0;
        }
        
        hasil = (nilai - calib->center_val) * JOYSTICK_CALIB_RANGE /
                ((calib->center_val - calib->min_val) != 0 ? 
                 (calib->center_val - calib->min_val) : 1);
    }
    
    /* Batasi ke rentang valid */
    if (hasil > JOYSTICK_AXIS_MAKS) {
        hasil = JOYSTICK_AXIS_MAKS;
    }
    if (hasil < JOYSTICK_AXIS_MIN) {
        hasil = JOYSTICK_AXIS_MIN;
    }
    
    return hasil;
}

/*
 * joystick_hitung_perubahan_tombol - Hitung mask perubahan tombol
 */
static tak_bertanda32_t joystick_hitung_perubahan_tombol(
    tak_bertanda32_t sebelumnya,
    tak_bertanda32_t saat_ini)
{
    return sebelumnya ^ saat_ini;
}

/*
 * joystick_buat_event - Buat event joystick
 */
static void joystick_buat_event(joystick_device_t *jdev,
                                 input_event_t *event,
                                 tak_bertanda32_t tombol_change)
{
    tak_bertanda32_t i;
    
    if (jdev == NULL || event == NULL || jdev->dev == NULL) {
        return;
    }
    
    kernel_memset(event, 0, sizeof(input_event_t));
    
    event->magic = EVENT_MAGIC;
    event->timestamp = kernel_get_ticks();
    event->tipe = EVENT_TIPE_ABSOLUTE;
    event->device_id = jdev->dev->id;
    
    /* Data joystick */
    event->data.joystick.tombol = jdev->tombol_saat_ini;
    event->data.joystick.tombol_change = tombol_change;
    event->data.joystick.axis_count = jdev->dev->num_axes;
    
    for (i = 0; i < INPUT_AXIS_MAKS && i < jdev->dev->num_axes; i++) {
        event->data.joystick.axis[i] = jdev->axis_saat_ini[i];
    }
}

/*
 * joystick_update_tombol - Update status tombol dan buat event
 */
static void joystick_update_tombol(joystick_device_t *jdev,
                                    tak_bertanda32_t tombol_baru)
{
    tak_bertanda32_t perubahan;
    tak_bertanda32_t i;
    input_event_t event;
    
    if (jdev == NULL || jdev->dev == NULL) {
        return;
    }
    
    jdev->tombol_sebelumnya = jdev->tombol_saat_ini;
    jdev->tombol_saat_ini = tombol_baru;
    jdev->dev->button_state = tombol_baru;
    
    perubahan = joystick_hitung_perubahan_tombol(jdev->tombol_sebelumnya,
                                                  jdev->tombol_saat_ini);
    
    if (perubahan == 0) {
        return;
    }
    
    /* Buat event untuk setiap tombol yang berubah */
    for (i = 0; i < INPUT_BUTTON_MAKS; i++) {
        if (perubahan & (1UL << i)) {
            kernel_memset(&event, 0, sizeof(input_event_t));
            
            event.magic = EVENT_MAGIC;
            event.timestamp = kernel_get_ticks();
            event.tipe = EVENT_TIPE_KEY;
            event.device_id = jdev->dev->id;
            
            event.data.key.kode = i;
            event.data.key.status = (tombol_baru & (1UL << i)) ? 1 : 0;
            event.data.key.scancode = i;
            
            masukan_event_push(&event);
            
            if (jdev->dev->callback != NULL) {
                jdev->dev->callback(jdev->dev, &event);
            }
        }
    }
}

/*
 * joystick_update_axis - Update status axis dan buat event
 */
static void joystick_update_axis(joystick_device_t *jdev,
                                  tanda32_t *axis_baru)
{
    input_event_t event;
    tak_bertanda32_t i;
    bool_t ada_perubahan;
    
    if (jdev == NULL || jdev->dev == NULL || axis_baru == NULL) {
        return;
    }
    
    ada_perubahan = SALAH;
    
    for (i = 0; i < INPUT_AXIS_MAKS && i < jdev->dev->num_axes; i++) {
        jdev->axis_sebelumnya[i] = jdev->axis_saat_ini[i];
        
        /* Terapkan deadzone dan normalisasi */
        jdev->axis_saat_ini[i] = joystick_terapkan_deadzone(
            joystick_normalisasi_axis(axis_baru[i], &jdev->calib[i]),
            jdev->deadzone);
        
        jdev->dev->axis_value[i] = jdev->axis_saat_ini[i];
        
        /* Cek perubahan signifikan */
        if (jdev->axis_saat_ini[i] != jdev->axis_sebelumnya[i]) {
            ada_perubahan = BENAR;
        }
    }
    
    if (!ada_perubahan) {
        return;
    }
    
    /* Buat event axis */
    joystick_buat_event(jdev, &event, 0);
    
    masukan_event_push(&event);
    
    if (jdev->dev->callback != NULL) {
        jdev->dev->callback(jdev->dev, &event);
    }
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

/*
 * joystick_init - Inisialisasi subsistem joystick
 */
status_t joystick_init(void)
{
    tak_bertanda32_t i;
    
    if (g_joystick_diinisialisasi) {
        return STATUS_SUDAH_ADA;
    }
    
    kernel_memset(&g_joystick_konteks, 0, sizeof(joystick_konteks_t));
    
    for (i = 0; i < JOYSTICK_MAKS_DEVICE; i++) {
        g_joystick_konteks.devices[i].dev = NULL;
        g_joystick_konteks.devices[i].tombol_sebelumnya = 0;
        g_joystick_konteks.devices[i].tombol_saat_ini = 0;
        g_joystick_konteks.devices[i].deadzone = JOYSTICK_AXIS_DEADZONE;
        g_joystick_konteks.devices[i].aktif = SALAH;
        
        joystick_inisialisasi_calib(&g_joystick_konteks.devices[i]);
    }
    
    g_joystick_konteks.device_count = 0;
    g_joystick_konteks.diinisialisasi = BENAR;
    
    g_joystick_diinisialisasi = BENAR;
    
    return STATUS_BERHASIL;
}

/*
 * joystick_event - Proses joystick event
 */
status_t joystick_event(tak_bertanda32_t id, tanda32_t *axis,
                         tak_bertanda32_t buttons)
{
    joystick_device_t *jdev;
    inputdev_t *dev;
    
    if (!g_joystick_diinisialisasi) {
        return STATUS_GAGAL;
    }
    
    if (axis == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    jdev = joystick_cari_device(id);
    if (jdev == NULL) {
        /* Coba cari device dari inputdev */
        dev = inputdev_cari(id);
        if (dev == NULL) {
            return STATUS_TIDAK_DITEMUKAN;
        }
        
        /* Buat device baru jika slot tersedia */
        {
            tak_bertanda32_t slot;
            
            slot = joystick_cari_slot_kosong();
            if (slot == INDEX_INVALID) {
                return STATUS_PENUH;
            }
            
            jdev = &g_joystick_konteks.devices[slot];
            jdev->dev = dev;
            jdev->aktif = BENAR;
            joystick_inisialisasi_calib(jdev);
            
            g_joystick_konteks.device_count++;
        }
    }
    
    if (!jdev->aktif) {
        return STATUS_GAGAL;
    }
    
    /* Update tombol */
    joystick_update_tombol(jdev, buttons);
    
    /* Update axis */
    joystick_update_axis(jdev, axis);
    
    return STATUS_BERHASIL;
}

/*
 * joystick_registrasi_device - Registrasi device joystick baru
 */
status_t joystick_registrasi_device(const char *nama,
                                     tak_bertanda32_t num_axes,
                                     tak_bertanda32_t num_buttons)
{
    inputdev_t *dev;
    joystick_device_t *jdev;
    tak_bertanda32_t slot;
    
    if (!g_joystick_diinisialisasi) {
        return STATUS_GAGAL;
    }
    
    if (nama == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Cari slot kosong */
    slot = joystick_cari_slot_kosong();
    if (slot == INDEX_INVALID) {
        return STATUS_PENUH;
    }
    
    /* Registrasi device */
    dev = inputdev_register(nama, INPUT_TIPE_JOYSTICK);
    if (dev == NULL) {
        return STATUS_GAGAL;
    }
    
    /* Set kapabilitas */
    dev->num_axes = (num_axes > INPUT_AXIS_MAKS) ? 
                    INPUT_AXIS_MAKS : num_axes;
    dev->num_buttons = (num_buttons > INPUT_BUTTON_MAKS) ? 
                       INPUT_BUTTON_MAKS : num_buttons;
    dev->num_keys = 0;
    dev->num_touches = 0;
    dev->status = INPUT_STATUS_AKTIF;
    
    /* Inisialisasi data joystick */
    jdev = &g_joystick_konteks.devices[slot];
    jdev->dev = dev;
    jdev->tombol_sebelumnya = 0;
    jdev->tombol_saat_ini = 0;
    jdev->deadzone = JOYSTICK_AXIS_DEADZONE;
    jdev->aktif = BENAR;
    
    joystick_inisialisasi_calib(jdev);
    
    g_joystick_konteks.device_count++;
    
    return STATUS_BERHASIL;
}

/*
 * joystick_hapus_device - Hapus device joystick
 */
status_t joystick_hapus_device(tak_bertanda32_t id)
{
    joystick_device_t *jdev;
    status_t hasil;
    
    if (!g_joystick_diinisialisasi) {
        return STATUS_GAGAL;
    }
    
    jdev = joystick_cari_device(id);
    if (jdev == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    /* Unregister dari inputdev */
    hasil = inputdev_unregister(jdev->dev);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    /* Clear data */
    jdev->dev = NULL;
    jdev->aktif = SALAH;
    jdev->tombol_sebelumnya = 0;
    jdev->tombol_saat_ini = 0;
    
    if (g_joystick_konteks.device_count > 0) {
        g_joystick_konteks.device_count--;
    }
    
    return STATUS_BERHASIL;
}

/*
 * joystick_set_deadzone - Set deadzone untuk device
 */
status_t joystick_set_deadzone(tak_bertanda32_t id,
                                tak_bertanda32_t deadzone)
{
    joystick_device_t *jdev;
    
    if (!g_joystick_diinisialisasi) {
        return STATUS_GAGAL;
    }
    
    jdev = joystick_cari_device(id);
    if (jdev == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    if (deadzone > (tak_bertanda32_t)JOYSTICK_AXIS_MAKS) {
        deadzone = JOYSTICK_AXIS_MAKS;
    }
    
    jdev->deadzone = deadzone;
    
    return STATUS_BERHASIL;
}

/*
 * joystick_kalibrasi - Kalibrasi axis joystick
 */
status_t joystick_kalibrasi(tak_bertanda32_t id,
                             tak_bertanda32_t axis,
                             tanda32_t min_val,
                             tanda32_t center_val,
                             tanda32_t max_val)
{
    joystick_device_t *jdev;
    
    if (!g_joystick_diinisialisasi) {
        return STATUS_GAGAL;
    }
    
    jdev = joystick_cari_device(id);
    if (jdev == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    if (axis >= INPUT_AXIS_MAKS) {
        return STATUS_PARAM_INVALID;
    }
    
    jdev->calib[axis].min_val = min_val;
    jdev->calib[axis].center_val = center_val;
    jdev->calib[axis].max_val = max_val;
    jdev->calib[axis].dikalibrasi = BENAR;
    
    return STATUS_BERHASIL;
}

/*
 * joystick_dapat_status - Dapatkan status joystick
 */
status_t joystick_dapat_status(tak_bertanda32_t id,
                                tak_bertanda32_t *tombol,
                                tanda32_t *axis,
                                tak_bertanda32_t axis_count)
{
    joystick_device_t *jdev;
    tak_bertanda32_t i;
    
    if (!g_joystick_diinisialisasi) {
        return STATUS_GAGAL;
    }
    
    jdev = joystick_cari_device(id);
    if (jdev == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    if (tombol != NULL) {
        *tombol = jdev->tombol_saat_ini;
    }
    
    if (axis != NULL && axis_count > 0) {
        for (i = 0; i < axis_count && i < INPUT_AXIS_MAKS; i++) {
            axis[i] = jdev->axis_saat_ini[i];
        }
    }
    
    return STATUS_BERHASIL;
}

/*
 * joystick_dapat_jumlah_device - Dapatkan jumlah device joystick
 */
tak_bertanda32_t joystick_dapat_jumlah_device(void)
{
    if (!g_joystick_diinisialisasi) {
        return 0;
    }
    
    return g_joystick_konteks.device_count;
}

/*
 * joystick_dapat_device - Dapatkan device joystick berdasarkan indeks
 */
inputdev_t *joystick_dapat_device(tak_bertanda32_t indeks)
{
    tak_bertanda32_t i;
    tak_bertanda32_t count;
    
    if (!g_joystick_diinisialisasi) {
        return NULL;
    }
    
    count = 0;
    
    for (i = 0; i < JOYSTICK_MAKS_DEVICE; i++) {
        if (g_joystick_konteks.devices[i].dev != NULL &&
            g_joystick_konteks.devices[i].aktif) {
            
            if (count == indeks) {
                return g_joystick_konteks.devices[i].dev;
            }
            
            count++;
        }
    }
    
    return NULL;
}
