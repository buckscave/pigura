/*
 * PIGURA OS - MASUKAN.C
 * =====================
 * Modul pemrosesan masukan (input processing) Pigura OS.
 *
 * Modul ini menerima event mentah dari subsistem perangkat
 * masukan (driver keyboard, mouse, touchscreen) dan
 * menerjemahkannya ke event jendela yang dipahami
 * oleh window manager dan pengendali peristiwa.
 *
 * Bridge antara:
 *   - masukan.h  (perangkat masukan driver level)
 *   - jendela.h   (window manager event level)
 *   - pengendali.c (event dispatcher)
 *   - penunjuk.c  (cursor state management)
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

/* Interval default untuk key repeat dalam milidetik */
#define MASUKAN_REPEAT_DELAY_MS     500

/* Interval antara key repeat dalam milidetik */
#define MASUKAN_REPEAT_RATE_MS      50

/* Waktu tekan tombol dianggap "lama" (hold) */
#define MASUKAN_TAHAN_MS           1000

/* Mask tombol mouse kiri */
#define MASK_TOMBOL_KIRI           0x01

/* Mask tombol mouse kanan */
#define MASK_TOMBOL_KANAN          0x02

/* Mask tombol mouse tengah */
#define MASK_TOMBOL_TENGAH         0x04

/* Jumlah maksimum penyangga state keyboard */
#define MASUKAN_KEYS_MAKS          256

/*
 * ===========================================================================
 * VARIABEL STATIK - STATE PEMROSASAN MASUKAN
 * ===========================================================================
 */

/* Flag status modul */
static bool_t g_masukan_aktif = SALAH;

/* State keyboard: array boolean per key code */
static tak_bertanda8_t g_key_state[MASUKAN_KEYS_MAKS];

/* Modifier flags keyboard saat ini */
static tak_bertanda32_t g_key_modifier = KEY_MOD_NONE;

/* Waktu tekan terakhir untuk setiap tombol (key repeat) */
static tak_bertanda64_t g_key_waktu[MASUKAN_KEYS_MAKS];

/* Flag apakah key repeat sedang aktif */
static bool_t g_key_repeat_aktif = SALAH;

/* Key code yang sedang di-repeat */
static tak_bertanda32_t g_key_repeat_kode = 0;

/* Waktu repeat terakhir */
static tak_bertanda64_t g_key_repeat_waktu = 0;

/* Waktu tekan awal tombol yang sedang di-repeat */
static tak_bertanda64_t g_key_repeat_awal = 0;

/* Flag apakah tombol dianggap "ditekan lama" (hold) */
static bool_t g_key_hold[MASUKAN_KEYS_MAKS];

/* Statistik */
static tak_bertanda64_t g_total_key_press = 0;
static tak_bertanda64_t g_total_key_release = 0;
static tak_bertanda64_t g_total_mouse_move = 0;
static tak_bertanda64_t g_total_mouse_click = 0;
static tak_bertanda64_t g_total_repeat = 0;

/*
 * ===========================================================================
 * FUNGSI INTERNAL - UPDATE MODIFIER FLAGS
 * ===========================================================================
 *
 * Memperbarui flag modifier berdasarkan state tombol
 * modifier yang ditekan/dilepas.
 */

static void update_modifier(void)
{
    g_key_modifier = KEY_MOD_NONE;

    if (g_key_state[KEY_LEFTCTRL] ||
        g_key_state[KEY_RIGHTCTRL]) {
        g_key_modifier |= KEY_MOD_CTRL;
    }

    if (g_key_state[KEY_LEFTSHIFT] ||
        g_key_state[KEY_RIGHTSHIFT]) {
        g_key_modifier |= KEY_MOD_SHIFT;
    }

    if (g_key_state[KEY_LEFTALT] ||
        g_key_state[KEY_RIGHTALT]) {
        g_key_modifier |= KEY_MOD_ALT;
    }

    if (g_key_state[KEY_CAPSLOCK]) {
        g_key_modifier |= KEY_MOD_CAPSLOCK;
    }

    if (g_key_state[KEY_NUMLOCK]) {
        g_key_modifier |= KEY_MOD_NUMLOCK;
    }
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL - PROSES KEYBOARD EVENT
 * ===========================================================================
 *
 * Menerima event keyboard dari driver dan memperbarui
 * state internal keyboard.
 *
 * Parameter:
 *   event - Pointer ke event keyboard dari masukan.h
 */

static void proses_keyboard_event(const input_event_t *event)
{
    tak_bertanda32_t kode;
    tak_bertanda32_t status_tombol;
    tak_bertanda64_t waktu;

    if (event == NULL) return;

    kode = event->data.key.kode;
    status_tombol = event->data.key.status;
    waktu = event->timestamp;

    /* Batasi kode ke array state */
    if (kode >= MASUKAN_KEYS_MAKS) return;

    if (status_tombol == 1) {
        /* Tombol ditekan */
        g_key_state[kode] = 1;
        g_key_waktu[kode] = waktu;
        g_key_hold[kode] = SALAH;
        g_total_key_press++;

        /* Mulai key repeat timer */
        if (g_key_repeat_aktif) {
            g_key_repeat_kode = kode;
            g_key_repeat_awal = waktu;
            g_key_repeat_waktu = waktu;
        }
    } else if (status_tombol == 0) {
        /* Tombol dilepas */
        g_key_state[kode] = 0;
        g_key_waktu[kode] = 0;
        g_key_hold[kode] = SALAH;
        g_total_key_release++;

        /* Hentikan repeat jika ini tombol yang di-repeat */
        if (g_key_repeat_aktif &&
            g_key_repeat_kode == kode) {
            g_key_repeat_aktif = SALAH;
            g_key_repeat_kode = 0;
        }
    } else if (status_tombol == 2) {
        /* Tombol repeat (auto-repeat hardware) */
        g_total_repeat++;
    }

    update_modifier();
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL - PROSES MOUSE EVENT
 * ===========================================================================
 *
 * Menerima event mouse dari driver dan meneruskan ke
 * pengendali peristiwa.
 *
 * Parameter:
 *   event - Pointer ke event mouse dari masukan.h
 */

static void proses_mouse_event(const input_event_t *event)
{
    tanda32_t rel_x, rel_y;

    if (event == NULL) return;

    rel_x = event->data.mouse.rel_x;
    rel_y = event->data.mouse.rel_y;

    /* Update posisi kursor berdasarkan gerakan relatif */
    if (rel_x != 0 || rel_y != 0) {
        penunjuk_gerak(rel_x, rel_y);
        g_total_mouse_move++;
    }

    /* Update status tombol kursor */
    {
        tak_bertanda32_t tombol = event->data.mouse.tombol;
        tak_bertanda32_t perubahan;

        perubahan = tombol & ~penunjuk_dapat_tombol();
        penunjuk_set_tombol(tombol);

        /* Deteksi klik baru */
        if (perubahan != 0) {
            tak_bertanda32_t px = 0, py = 0;
            penunjuk_dapat_posisi(&px, &py);

            g_total_mouse_click++;

            /* Proses event mouse melalui pengendali */
            pengendali_proses_mouse(px, py, tombol);
        }
    }
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL - PROSES TOUCH EVENT
 * ===========================================================================
 *
 * Menerima event touchscreen dari driver dan menerjemahkan
 * ke event mouse equivalen untuk kompatibilitas.
 */

static void proses_touch_event(const input_event_t *event)
{
    tak_bertanda32_t tipe_touch;
    tak_bertanda32_t tx, ty;

    if (event == NULL) return;

    tipe_touch = event->data.touch.tipe;
    tx = event->data.touch.x;
    ty = event->data.touch.y;

    switch (tipe_touch) {
    case TOUCH_EVENT_DOWN:
        /* Sentuh dimulai: update posisi dan tombol */
        penunjuk_set_posisi(tx, ty);
        penunjuk_set_tombol(MOUSE_BUTTON_LEFT);
        pengendali_proses_mouse(tx, ty,
                                MOUSE_BUTTON_LEFT);
        break;

    case TOUCH_EVENT_UP:
        /* Sentuh dilepas */
        penunjuk_set_tombol(MOUSE_BUTTON_NONE);
        pengendali_proses_mouse(tx, ty,
                                MOUSE_BUTTON_NONE);
        break;

    case TOUCH_EVENT_MOVE:
        /* Gerakan sentuh */
        penunjuk_set_posisi(tx, ty);
        pengendali_proses_mouse(tx, ty,
                                MOUSE_BUTTON_LEFT);
        break;

    case TOUCH_EVENT_CANCEL:
        /* Sentuh dibatalkan */
        penunjuk_set_tombol(MOUSE_BUTTON_NONE);
        break;

    default:
        break;
    }
}

/*
 * ===========================================================================
 * API PUBLIK - INISIALISASI MODUL MASUKAN
 * ===========================================================================
 *
 * Menyiapkan state pemrosesan masukan. Mengosongkan
 * array state keyboard dan menonaktifkan key repeat.
 */

status_t masukan_proses_init(void)
{
    if (g_masukan_aktif) {
        return STATUS_SUDAH_ADA;
    }

    kernel_memset(g_key_state, 0, sizeof(g_key_state));
    kernel_memset(g_key_waktu, 0, sizeof(g_key_waktu));
    kernel_memset(g_key_hold, 0, sizeof(g_key_hold));

    g_key_modifier = KEY_MOD_NONE;
    g_key_repeat_aktif = SALAH;
    g_key_repeat_kode = 0;
    g_key_repeat_waktu = 0;
    g_key_repeat_awal = 0;

    g_total_key_press = 0;
    g_total_key_release = 0;
    g_total_mouse_move = 0;
    g_total_mouse_click = 0;
    g_total_repeat = 0;

    g_masukan_aktif = BENAR;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - SHUTDOWN MODUL MASUKAN
 * ===========================================================================
 */

void masukan_proses_shutdown(void)
{
    if (!g_masukan_aktif) {
        return;
    }

    kernel_memset(g_key_state, 0, sizeof(g_key_state));
    kernel_memset(g_key_waktu, 0, sizeof(g_key_waktu));
    kernel_memset(g_key_hold, 0, sizeof(g_key_hold));

    g_key_repeat_aktif = SALAH;
    g_masukan_aktif = SALAH;
}

/*
 * ===========================================================================
 * API PUBLIK - PROSES EVENT MASUKAN
 * ===========================================================================
 *
 * Titik masuk utama untuk event dari driver perangkat.
 * Menerima event generik dari subsistem masukan dan
 * meneruskannya ke prosesor yang sesuai berdasarkan tipe.
 *
 * Parameter:
 *   event - Pointer ke event masukan generik
 *
 * Return: STATUS_BERHASIL jika berhasil diproses,
 *         STATUS_PARAM_NULL jika event NULL.
 */

status_t masukan_proses_event(const input_event_t *event)
{
    status_t st;

    if (!g_masukan_aktif) {
        return STATUS_TIDAK_DUKUNG;
    }

    if (event == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Validasi magic number event */
    if (event->magic != EVENT_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    /* Route ke prosesor berdasarkan tipe event */
    switch (event->tipe) {
    case EVENT_TIPE_KEY:
        proses_keyboard_event(event);
        st = STATUS_BERHASIL;
        break;

    case EVENT_TIPE_RELATIVE:
        proses_mouse_event(event);
        st = STATUS_BERHASIL;
        break;

    case EVENT_TIPE_ABSOLUTE:
        /* Event absolut: update posisi langsung */
        proses_mouse_event(event);
        st = STATUS_BERHASIL;
        break;

    case EVENT_TIPE_TOUCH:
        proses_touch_event(event);
        st = STATUS_BERHASIL;
        break;

    case EVENT_TIPE_TIDAK_ADA:
    default:
        /* Event tidak dikenal: abaikan */
        st = STATUS_BERHASIL;
        break;
    }

    return st;
}

/*
 * ===========================================================================
 * API PUBLIK - LANGKAH PEMROSESAN MASUKAN
 * ===========================================================================
 *
 * Dipanggil setiap frame oleh window manager untuk:
 *   1. Memeriksa key repeat yang perlu diproses
 *   2. Mendeteksi tombol yang ditekan lama (hold)
 *   3. Proses event yang menunggu dalam antrian
 *
 * Parameter:
 *   delta_ms - Waktu yang berlalu sejak frame terakhir
 */

status_t masukan_proses_langkah(tak_bertanda32_t delta_ms)
{
    tak_bertanda64_t waktu_sekarang;
    tak_bertanda32_t i;

    if (!g_masukan_aktif) {
        return STATUS_TIDAK_DUKUNG;
    }

    waktu_sekarang = kernel_get_ticks();

    /*
     * Proses key repeat.
     * Jika ada tombol yang sedang di-press dan cukup lama,
     * kirim event repeat ke pengendali.
     */
    if (delta_ms == 0) {
        return STATUS_BERHASIL;
    }

    for (i = 0; i < MASUKAN_KEYS_MAKS; i++) {
        if (g_key_state[i] == 0) {
            continue;
        }

        /* Cek apakah tombol sudah ditekan cukup lama
           untuk dianggap "hold" */
        if (!g_key_hold[i]) {
            tak_bertanda64_t durasi;

            if (waktu_sekarang < g_key_waktu[i]) {
                continue;
            }

            durasi = waktu_sekarang - g_key_waktu[i];
            if (durasi >=
                (tak_bertanda64_t)MASUKAN_TAHAN_MS) {
                g_key_hold[i] = BENAR;
            }
        }
    }

    /*
     * Proses repeat untuk tombol yang sedang aktif.
     * Gunakan delay untuk tekan pertama, lalu rate
     * untuk repeat selanjutnya.
     */
    if (g_key_repeat_aktif &&
        g_key_repeat_kode < MASUKAN_KEYS_MAKS &&
        g_key_state[g_key_repeat_kode]) {

        tak_bertanda64_t sejak_tekan =
            waktu_sekarang - g_key_repeat_awal;
        tak_bertanda64_t sejak_repeat =
            waktu_sekarang - g_key_repeat_waktu;

        if (sejak_tekan >=
            (tak_bertanda64_t)MASUKAN_REPEAT_DELAY_MS) {
            if (sejak_repeat >=
                (tak_bertanda64_t)MASUKAN_REPEAT_RATE_MS) {
                /* Kirim event repeat ke pengendali */
                pengendali_proses_keyboard(
                    g_key_repeat_kode, 2,
                    g_key_modifier);
                g_key_repeat_waktu = waktu_sekarang;
                g_total_repeat++;
            }
        }
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - DAPAT STATE KEYBOARD
 * ===========================================================================
 *
 * Mengembalikan apakah tombol tertentu sedang ditekan.
 * Mengembalikan SALAH jika kode di luar batas.
 */

bool_t masukan_dapat_key(tak_bertanda32_t kode)
{
    if (!g_masukan_aktif) {
        return SALAH;
    }
    if (kode >= MASUKAN_KEYS_MAKS) {
        return SALAH;
    }
    return g_key_state[kode] ? BENAR : SALAH;
}

/*
 * ===========================================================================
 * API PUBLIK - DAPAT MODIFIER FLAGS
 * ===========================================================================
 */

tak_bertanda32_t masukan_dapat_modifier(void)
{
    if (!g_masukan_aktif) {
        return KEY_MOD_NONE;
    }
    return g_key_modifier;
}

/*
 * ===========================================================================
 * API PUBLIK - SET KEY REPEAT
 * ===========================================================================
 *
 * Mengaktifkan atau menonaktifkan fitur key repeat.
 * Key repeat mengirim event keyboard ulang saat tombol
 * ditekan lebih dari delay threshold.
 *
 * Parameter:
 *   aktif - BENAR untuk mengaktifkan, SALAH untuk nonaktif
 */

void masukan_set_key_repeat(bool_t aktif)
{
    g_key_repeat_aktif = aktif;
    if (!aktif) {
        g_key_repeat_kode = 0;
    }
}

/*
 * ===========================================================================
 * API PUBLIK - DAPAT STATISTIK MASUKAN
 * ===========================================================================
 */

void masukan_dapat_statistik(tak_bertanda64_t *key_press,
                              tak_bertanda64_t *key_release,
                              tak_bertanda64_t *mouse_move,
                              tak_bertanda64_t *mouse_click,
                              tak_bertanda64_t *repeat_count)
{
    if (key_press != NULL) {
        *key_press = g_total_key_press;
    }
    if (key_release != NULL) {
        *key_release = g_total_key_release;
    }
    if (mouse_move != NULL) {
        *mouse_move = g_total_mouse_move;
    }
    if (mouse_click != NULL) {
        *mouse_click = g_total_mouse_click;
    }
    if (repeat_count != NULL) {
        *repeat_count = g_total_repeat;
    }
}
