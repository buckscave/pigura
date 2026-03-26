/*
 * PIGURA OS - GAMEPAD.C
 * =====================
 * Implementasi subsistem gamepad.
 *
 * Berkas ini mengimplementasikan penanganan perangkat gamepad dengan
 * dukungan dual analog sticks, triggers, rumble, dan mapping tombol.
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
 * KONSTANTA GAMEPAD
 * ===========================================================================
 */

/* Batasan gamepad */
#define GAMEPAD_MAKS_DEVICE        8
#define GAMEPAD_AXIS_DEADZONE      4000
#define GAMEPAD_TRIGGER_THRESHOLD  30

/* Nilai analog */
#define GAMEPAD_ANALOG_MAX         32767
#define GAMEPAD_ANALOG_MIN         -32768
#define GAMEPAD_TRIGGER_MAX        255

/* Status tombol */
#define GAMEPAD_TOMBOL_DITEKAN     1
#define GAMEPAD_TOMBOL_DILEPAS     0

/* Rumble */
#define GAMEPAD_RUMBLE_MAX         65535
#define GAMEPAD_RUMBLE_DURATION    500

/*
 * ===========================================================================
 * STRUKTUR DATA GAMEPAD
 * ===========================================================================
 */

/* Data analog stick */
typedef struct {
    tanda16_t x;
    tanda16_t y;
    tanda16_t x_sebelumnya;
    tanda16_t y_sebelumnya;
} analog_stick_t;

/* Data trigger */
typedef struct {
    tak_bertanda8_t nilai;
    tak_bertanda8_t sebelumnya;
    tak_bertanda8_t threshold;
} trigger_t;

/* Data rumble */
typedef struct {
    tak_bertanda16_t weak;
    tak_bertanda16_t strong;
    tak_bertanda32_t durasi;
    bool_t aktif;
} rumble_state_t;

/* Data device gamepad */
typedef struct {
    inputdev_t *dev;
    
    /* Analog sticks */
    analog_stick_t left_stick;
    analog_stick_t right_stick;
    
    /* Triggers */
    trigger_t left_trigger;
    trigger_t right_trigger;
    
    /* Tombol */
    tak_bertanda32_t tombol_sebelumnya;
    tak_bertanda32_t tombol_saat_ini;
    
    /* Rumble state */
    rumble_state_t rumble;
    
    /* Konfigurasi */
    tak_bertanda32_t deadzone;
    bool_t rumble_support;
    
    /* Status */
    bool_t aktif;
} gamepad_device_t;

/* Konteks gamepad */
typedef struct {
    gamepad_device_t devices[GAMEPAD_MAKS_DEVICE];
    tak_bertanda32_t device_count;
    bool_t diinisialisasi;
} gamepad_konteks_t;

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

static gamepad_konteks_t g_gamepad_konteks;
static bool_t g_gamepad_diinisialisasi = SALAH;

/*
 * ===========================================================================
 * FUNGSI INTERNAL
 * ===========================================================================
 */

/*
 * gamepad_cari_device - Cari device gamepad berdasarkan ID
 */
static gamepad_device_t *gamepad_cari_device(tak_bertanda32_t id)
{
    tak_bertanda32_t i;
    
    if (!g_gamepad_diinisialisasi) {
        return NULL;
    }
    
    for (i = 0; i < g_gamepad_konteks.device_count; i++) {
        if (g_gamepad_konteks.devices[i].dev != NULL &&
            g_gamepad_konteks.devices[i].dev->id == id) {
            return &g_gamepad_konteks.devices[i];
        }
    }
    
    return NULL;
}

/*
 * gamepad_cari_slot_kosong - Cari slot device kosong
 */
static tak_bertanda32_t gamepad_cari_slot_kosong(void)
{
    tak_bertanda32_t i;
    
    for (i = 0; i < GAMEPAD_MAKS_DEVICE; i++) {
        if (g_gamepad_konteks.devices[i].dev == NULL) {
            return i;
        }
    }
    
    return INDEX_INVALID;
}

/*
 * gamepad_terapkan_deadzone - Terapkan deadzone pada analog stick
 */
static void gamepad_terapkan_deadzone(tanda16_t *x, tanda16_t *y,
                                       tak_bertanda32_t deadzone)
{
    tanda32_t abs_x;
    tanda32_t abs_y;
    tanda32_t magnitude;
    tanda32_t scaled;
    
    if (x == NULL || y == NULL) {
        return;
    }
    
    abs_x = (*x < 0) ? -*x : *x;
    abs_y = (*y < 0) ? -*y : *y;
    
    /* Hitung magnitude */
    magnitude = (abs_x > abs_y) ? abs_x : abs_y;
    
    if (magnitude < (tanda32_t)deadzone) {
        *x = 0;
        *y = 0;
        return;
    }
    
    /* Scale nilai setelah deadzone */
    if (magnitude > GAMEPAD_ANALOG_MAX) {
        magnitude = GAMEPAD_ANALOG_MAX;
    }
    
    scaled = (magnitude - (tanda32_t)deadzone) * GAMEPAD_ANALOG_MAX /
             (GAMEPAD_ANALOG_MAX - (tanda32_t)deadzone);
    
    if (scaled > GAMEPAD_ANALOG_MAX) {
        scaled = GAMEPAD_ANALOG_MAX;
    }
    
    /* Terapkan scaling */
    if (*x != 0) {
        *x = (tanda16_t)((*x * scaled) / magnitude);
    }
    if (*y != 0) {
        *y = (tanda16_t)((*y * scaled) / magnitude);
    }
}

/*
 * gamepad_hitung_perubahan_tombol - Hitung mask perubahan tombol
 */
static tak_bertanda32_t gamepad_hitung_perubahan_tombol(
    tak_bertanda32_t sebelumnya,
    tak_bertanda32_t saat_ini)
{
    return sebelumnya ^ saat_ini;
}

/*
 * gamepad_buat_event_tombol - Buat event tombol
 */
static void gamepad_buat_event_tombol(gamepad_device_t *gdev,
                                       tak_bertanda32_t tombol_id,
                                       tak_bertanda32_t status)
{
    input_event_t event;
    
    if (gdev == NULL || gdev->dev == NULL) {
        return;
    }
    
    kernel_memset(&event, 0, sizeof(input_event_t));
    
    event.magic = EVENT_MAGIC;
    event.timestamp = kernel_get_ticks();
    event.tipe = EVENT_TIPE_KEY;
    event.device_id = gdev->dev->id;
    
    event.data.key.kode = tombol_id;
    event.data.key.status = status;
    event.data.key.scancode = tombol_id;
    
    masukan_event_push(&event);
    
    if (gdev->dev->callback != NULL) {
        gdev->dev->callback(gdev->dev, &event);
    }
}

/*
 * gamepad_buat_event_axis - Buat event axis
 */
static void gamepad_buat_event_axis(gamepad_device_t *gdev,
                                     tak_bertanda32_t axis,
                                     tanda32_t nilai)
{
    input_event_t event;
    
    if (gdev == NULL || gdev->dev == NULL) {
        return;
    }
    
    kernel_memset(&event, 0, sizeof(input_event_t));
    
    event.magic = EVENT_MAGIC;
    event.timestamp = kernel_get_ticks();
    event.tipe = EVENT_TIPE_ABSOLUTE;
    event.device_id = gdev->dev->id;
    
    event.data.joystick.axis[axis] = nilai;
    event.data.joystick.axis_count = gdev->dev->num_axes;
    event.data.joystick.tombol = gdev->tombol_saat_ini;
    event.data.joystick.tombol_change = 0;
    
    masukan_event_push(&event);
    
    if (gdev->dev->callback != NULL) {
        gdev->dev->callback(gdev->dev, &event);
    }
}

/*
 * gamepad_proses_tombol - Proses perubahan tombol
 */
static void gamepad_proses_tombol(gamepad_device_t *gdev,
                                   tak_bertanda32_t tombol_baru)
{
    tak_bertanda32_t perubahan;
    tak_bertanda32_t i;
    
    if (gdev == NULL) {
        return;
    }
    
    gdev->tombol_sebelumnya = gdev->tombol_saat_ini;
    gdev->tombol_saat_ini = tombol_baru;
    
    if (gdev->dev != NULL) {
        gdev->dev->button_state = tombol_baru;
    }
    
    perubahan = gamepad_hitung_perubahan_tombol(gdev->tombol_sebelumnya,
                                                 gdev->tombol_saat_ini);
    
    if (perubahan == 0) {
        return;
    }
    
    /* Proses setiap tombol yang berubah */
    for (i = 0; i < INPUT_BUTTON_MAKS; i++) {
        if (perubahan & (1UL << i)) {
            gamepad_buat_event_tombol(gdev, i,
                (tombol_baru & (1UL << i)) ? GAMEPAD_TOMBOL_DITEKAN : 
                                               GAMEPAD_TOMBOL_DILEPAS);
        }
    }
}

/*
 * gamepad_proses_analog - Proses analog stick
 */
static void gamepad_proses_analog(gamepad_device_t *gdev,
                                   tanda16_t left_x, tanda16_t left_y,
                                   tanda16_t right_x, tanda16_t right_y)
{
    tanda16_t lx;
    tanda16_t ly;
    tanda16_t rx;
    tanda16_t ry;
    
    if (gdev == NULL) {
        return;
    }
    
    /* Simpan nilai sebelumnya */
    gdev->left_stick.x_sebelumnya = gdev->left_stick.x;
    gdev->left_stick.y_sebelumnya = gdev->left_stick.y;
    gdev->right_stick.x_sebelumnya = gdev->right_stick.x;
    gdev->right_stick.y_sebelumnya = gdev->right_stick.y;
    
    /* Terapkan deadzone */
    lx = left_x;
    ly = left_y;
    rx = right_x;
    ry = right_y;
    
    gamepad_terapkan_deadzone(&lx, &ly, gdev->deadzone);
    gamepad_terapkan_deadzone(&rx, &ry, gdev->deadzone);
    
    /* Update nilai */
    gdev->left_stick.x = lx;
    gdev->left_stick.y = ly;
    gdev->right_stick.x = rx;
    gdev->right_stick.y = ry;
    
    /* Update device state */
    if (gdev->dev != NULL) {
        gdev->dev->axis_value[JOY_AXIS_X] = lx;
        gdev->dev->axis_value[JOY_AXIS_Y] = ly;
        gdev->dev->axis_value[JOY_AXIS_RX] = rx;
        gdev->dev->axis_value[JOY_AXIS_RY] = ry;
    }
    
    /* Buat event jika ada perubahan signifikan */
    if (lx != gdev->left_stick.x_sebelumnya) {
        gamepad_buat_event_axis(gdev, JOY_AXIS_X, lx);
    }
    if (ly != gdev->left_stick.y_sebelumnya) {
        gamepad_buat_event_axis(gdev, JOY_AXIS_Y, ly);
    }
    if (rx != gdev->right_stick.x_sebelumnya) {
        gamepad_buat_event_axis(gdev, JOY_AXIS_RX, rx);
    }
    if (ry != gdev->right_stick.y_sebelumnya) {
        gamepad_buat_event_axis(gdev, JOY_AXIS_RY, ry);
    }
}

/*
 * gamepad_proses_triggers - Proses triggers
 */
static void gamepad_proses_triggers(gamepad_device_t *gdev,
                                     tak_bertanda32_t triggers)
{
    tak_bertanda8_t left;
    tak_bertanda8_t right;
    
    if (gdev == NULL) {
        return;
    }
    
    /* Extract trigger values */
    left = (tak_bertanda8_t)(triggers & 0xFF);
    right = (tak_bertanda8_t)((triggers >> 8) & 0xFF);
    
    /* Simpan nilai sebelumnya */
    gdev->left_trigger.sebelumnya = gdev->left_trigger.nilai;
    gdev->right_trigger.sebelumnya = gdev->right_trigger.nilai;
    
    /* Update nilai */
    gdev->left_trigger.nilai = left;
    gdev->right_trigger.nilai = right;
    
    /* Update device state */
    if (gdev->dev != NULL) {
        gdev->dev->axis_value[JOY_AXIS_Z] = (tanda32_t)left;
        gdev->dev->axis_value[JOY_AXIS_RZ] = (tanda32_t)right;
    }
    
    /* Buat event jika ada perubahan signifikan */
    if (left != gdev->left_trigger.sebelumnya) {
        gamepad_buat_event_axis(gdev, JOY_AXIS_Z, (tanda32_t)left);
    }
    if (right != gdev->right_trigger.sebelumnya) {
        gamepad_buat_event_axis(gdev, JOY_AXIS_RZ, (tanda32_t)right);
    }
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

/*
 * gamepad_init - Inisialisasi subsistem gamepad
 */
status_t gamepad_init(void)
{
    tak_bertanda32_t i;
    
    if (g_gamepad_diinisialisasi) {
        return STATUS_SUDAH_ADA;
    }
    
    kernel_memset(&g_gamepad_konteks, 0, sizeof(gamepad_konteks_t));
    
    /* Inisialisasi devices */
    for (i = 0; i < GAMEPAD_MAKS_DEVICE; i++) {
        g_gamepad_konteks.devices[i].dev = NULL;
        
        /* Analog sticks */
        g_gamepad_konteks.devices[i].left_stick.x = 0;
        g_gamepad_konteks.devices[i].left_stick.y = 0;
        g_gamepad_konteks.devices[i].right_stick.x = 0;
        g_gamepad_konteks.devices[i].right_stick.y = 0;
        
        /* Triggers */
        g_gamepad_konteks.devices[i].left_trigger.nilai = 0;
        g_gamepad_konteks.devices[i].right_trigger.nilai = 0;
        g_gamepad_konteks.devices[i].left_trigger.threshold = 
            GAMEPAD_TRIGGER_THRESHOLD;
        g_gamepad_konteks.devices[i].right_trigger.threshold = 
            GAMEPAD_TRIGGER_THRESHOLD;
        
        /* Tombol */
        g_gamepad_konteks.devices[i].tombol_sebelumnya = 0;
        g_gamepad_konteks.devices[i].tombol_saat_ini = 0;
        
        /* Rumble */
        g_gamepad_konteks.devices[i].rumble.weak = 0;
        g_gamepad_konteks.devices[i].rumble.strong = 0;
        g_gamepad_konteks.devices[i].rumble.durasi = 0;
        g_gamepad_konteks.devices[i].rumble.aktif = SALAH;
        g_gamepad_konteks.devices[i].rumble_support = SALAH;
        
        /* Konfigurasi */
        g_gamepad_konteks.devices[i].deadzone = GAMEPAD_AXIS_DEADZONE;
        g_gamepad_konteks.devices[i].aktif = SALAH;
    }
    
    g_gamepad_konteks.device_count = 0;
    g_gamepad_konteks.diinisialisasi = BENAR;
    g_gamepad_diinisialisasi = BENAR;
    
    return STATUS_BERHASIL;
}

/*
 * gamepad_event - Proses gamepad event
 */
status_t gamepad_event(tak_bertanda32_t id, tak_bertanda32_t buttons,
                        tanda16_t left_x, tanda16_t left_y,
                        tanda16_t right_x, tanda16_t right_y,
                        tak_bertanda32_t triggers)
{
    gamepad_device_t *gdev;
    
    if (!g_gamepad_diinisialisasi) {
        return STATUS_GAGAL;
    }
    
    gdev = gamepad_cari_device(id);
    if (gdev == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    if (!gdev->aktif) {
        return STATUS_GAGAL;
    }
    
    /* Proses tombol */
    gamepad_proses_tombol(gdev, buttons);
    
    /* Proses analog sticks */
    gamepad_proses_analog(gdev, left_x, left_y, right_x, right_y);
    
    /* Proses triggers */
    gamepad_proses_triggers(gdev, triggers);
    
    return STATUS_BERHASIL;
}

/*
 * gamepad_rumble - Set rumble/vibration
 */
status_t gamepad_rumble(tak_bertanda32_t id, tak_bertanda16_t weak,
                         tak_bertanda16_t strong)
{
    gamepad_device_t *gdev;
    
    if (!g_gamepad_diinisialisasi) {
        return STATUS_GAGAL;
    }
    
    gdev = gamepad_cari_device(id);
    if (gdev == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    if (!gdev->rumble_support) {
        return STATUS_TIDAK_DUKUNG;
    }
    
    /* Batasi nilai */
    if (weak > GAMEPAD_RUMBLE_MAX) {
        weak = GAMEPAD_RUMBLE_MAX;
    }
    if (strong > GAMEPAD_RUMBLE_MAX) {
        strong = GAMEPAD_RUMBLE_MAX;
    }
    
    /* Set rumble */
    gdev->rumble.weak = weak;
    gdev->rumble.strong = strong;
    gdev->rumble.durasi = GAMEPAD_RUMBLE_DURATION;
    gdev->rumble.aktif = BENAR;
    
    /* TODO: Kirim perintah ke device */
    
    return STATUS_BERHASIL;
}

/*
 * gamepad_registrasi_device - Registrasi device gamepad baru
 */
status_t gamepad_registrasi_device(const char *nama,
                                    bool_t rumble_support)
{
    inputdev_t *dev;
    gamepad_device_t *gdev;
    tak_bertanda32_t slot;
    char buffer[INPUTDEV_NAMA_MAKS];
    ukuran_t len;
    
    if (!g_gamepad_diinisialisasi) {
        return STATUS_GAGAL;
    }
    
    if (nama == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Cari slot kosong */
    slot = gamepad_cari_slot_kosong();
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
    dev = inputdev_register(nama, INPUT_TIPE_GAMEPAD);
    if (dev == NULL) {
        return STATUS_GAGAL;
    }
    
    /* Set kapabilitas */
    dev->num_keys = 0;
    dev->num_buttons = 15;  /* Standard gamepad buttons */
    dev->num_axes = 6;      /* Left X/Y, Right X/Y, L2/R2 triggers */
    dev->num_touches = 0;
    dev->status = INPUT_STATUS_AKTIF;
    
    /* Inisialisasi data gamepad */
    gdev = &g_gamepad_konteks.devices[slot];
    gdev->dev = dev;
    gdev->left_stick.x = 0;
    gdev->left_stick.y = 0;
    gdev->right_stick.x = 0;
    gdev->right_stick.y = 0;
    gdev->left_trigger.nilai = 0;
    gdev->right_trigger.nilai = 0;
    gdev->tombol_sebelumnya = 0;
    gdev->tombol_saat_ini = 0;
    gdev->rumble_support = rumble_support;
    gdev->deadzone = GAMEPAD_AXIS_DEADZONE;
    gdev->aktif = BENAR;
    
    g_gamepad_konteks.device_count++;
    
    return STATUS_BERHASIL;
}

/*
 * gamepad_hapus_device - Hapus device gamepad
 */
status_t gamepad_hapus_device(tak_bertanda32_t id)
{
    gamepad_device_t *gdev;
    status_t hasil;
    
    if (!g_gamepad_diinisialisasi) {
        return STATUS_GAGAL;
    }
    
    gdev = gamepad_cari_device(id);
    if (gdev == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    /* Unregister dari inputdev */
    hasil = inputdev_unregister(gdev->dev);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }
    
    /* Clear data */
    gdev->dev = NULL;
    gdev->aktif = SALAH;
    gdev->tombol_sebelumnya = 0;
    gdev->tombol_saat_ini = 0;
    
    if (g_gamepad_konteks.device_count > 0) {
        g_gamepad_konteks.device_count--;
    }
    
    return STATUS_BERHASIL;
}

/*
 * gamepad_set_deadzone - Set deadzone untuk analog sticks
 */
status_t gamepad_set_deadzone(tak_bertanda32_t id,
                               tak_bertanda32_t deadzone)
{
    gamepad_device_t *gdev;
    
    if (!g_gamepad_diinisialisasi) {
        return STATUS_GAGAL;
    }
    
    gdev = gamepad_cari_device(id);
    if (gdev == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    if (deadzone > (tak_bertanda32_t)GAMEPAD_ANALOG_MAX) {
        deadzone = GAMEPAD_ANALOG_MAX;
    }
    
    gdev->deadzone = deadzone;
    
    return STATUS_BERHASIL;
}

/*
 * gamepad_set_trigger_threshold - Set threshold untuk triggers
 */
status_t gamepad_set_trigger_threshold(tak_bertanda32_t id,
                                        tak_bertanda8_t threshold)
{
    gamepad_device_t *gdev;
    
    if (!g_gamepad_diinisialisasi) {
        return STATUS_GAGAL;
    }
    
    gdev = gamepad_cari_device(id);
    if (gdev == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    if (threshold > GAMEPAD_TRIGGER_MAX) {
        threshold = GAMEPAD_TRIGGER_MAX;
    }
    
    gdev->left_trigger.threshold = threshold;
    gdev->right_trigger.threshold = threshold;
    
    return STATUS_BERHASIL;
}

/*
 * gamepad_dapat_status - Dapatkan status gamepad
 */
status_t gamepad_dapat_status(tak_bertanda32_t id,
                               tak_bertanda32_t *tombol,
                               tanda16_t *left_x, tanda16_t *left_y,
                               tanda16_t *right_x, tanda16_t *right_y,
                               tak_bertanda8_t *left_trigger,
                               tak_bertanda8_t *right_trigger)
{
    gamepad_device_t *gdev;
    
    if (!g_gamepad_diinisialisasi) {
        return STATUS_GAGAL;
    }
    
    gdev = gamepad_cari_device(id);
    if (gdev == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    if (tombol != NULL) {
        *tombol = gdev->tombol_saat_ini;
    }
    
    if (left_x != NULL) {
        *left_x = gdev->left_stick.x;
    }
    if (left_y != NULL) {
        *left_y = gdev->left_stick.y;
    }
    if (right_x != NULL) {
        *right_x = gdev->right_stick.x;
    }
    if (right_y != NULL) {
        *right_y = gdev->right_stick.y;
    }
    
    if (left_trigger != NULL) {
        *left_trigger = gdev->left_trigger.nilai;
    }
    if (right_trigger != NULL) {
        *right_trigger = gdev->right_trigger.nilai;
    }
    
    return STATUS_BERHASIL;
}

/*
 * gamepad_dapat_jumlah_device - Dapatkan jumlah device gamepad
 */
tak_bertanda32_t gamepad_dapat_jumlah_device(void)
{
    if (!g_gamepad_diinisialisasi) {
        return 0;
    }
    
    return g_gamepad_konteks.device_count;
}

/*
 * gamepad_dapat_device - Dapatkan device gamepad berdasarkan indeks
 */
inputdev_t *gamepad_dapat_device(tak_bertanda32_t indeks)
{
    tak_bertanda32_t i;
    tak_bertanda32_t count;
    
    if (!g_gamepad_diinisialisasi) {
        return NULL;
    }
    
    count = 0;
    
    for (i = 0; i < GAMEPAD_MAKS_DEVICE; i++) {
        if (g_gamepad_konteks.devices[i].dev != NULL &&
            g_gamepad_konteks.devices[i].aktif) {
            
            if (count == indeks) {
                return g_gamepad_konteks.devices[i].dev;
            }
            
            count++;
        }
    }
    
    return NULL;
}
