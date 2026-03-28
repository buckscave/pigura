/*
 * PIGURA OS - MASUKAN.C
 * =====================
 * Implementasi subsistem perangkat masukan.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "masukan.h"
#include "../../inti/kernel.h"

/*
 * ===========================================================================
 * VARIABEL GLOBAL
 * ===========================================================================
 */

masukan_konteks_t g_masukan_konteks;
bool_t g_masukan_diinisialisasi = SALAH;

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

static char g_masukan_error[256];

/* Keyboard scancode to key code mapping (US QWERTY) */
static const tak_bertanda32_t g_scancode_to_key[128] = {
    /* 0x00 */ KEY_TIDAK_ADA, KEY_ESC, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6,
    /* 0x08 */ KEY_7, KEY_8, KEY_9, KEY_0, KEY_MINUS, KEY_EQUAL, KEY_BACKSPACE, KEY_TAB,
    /* 0x10 */ KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_U, KEY_I,
    /* 0x18 */ KEY_O, KEY_P, KEY_LEFTBRACE, KEY_RIGHTBRACE, KEY_ENTER, KEY_LEFTCTRL, KEY_A, KEY_S,
    /* 0x20 */ KEY_D, KEY_F, KEY_G, KEY_H, KEY_J, KEY_K, KEY_L, KEY_SEMICOLON,
    /* 0x28 */ KEY_APOSTROPHE, KEY_GRAVE, KEY_LEFTSHIFT, KEY_BACKSLASH, KEY_Z, KEY_X, KEY_C, KEY_V,
    /* 0x30 */ KEY_B, KEY_N, KEY_M, KEY_COMMA, KEY_DOT, KEY_SLASH, KEY_RIGHTSHIFT, KEY_KPASTERISK,
    /* 0x38 */ KEY_LEFTALT, KEY_SPACE, KEY_CAPSLOCK, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5,
    /* 0x40 */ KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_NUMLOCK, KEY_SCROLLLOCK, KEY_KP7,
    /* 0x48 */ KEY_KP8, KEY_KP9, KEY_KPMINUS, KEY_KP4, KEY_KP5, KEY_KP6, KEY_KPPLUS, KEY_KP1,
    /* 0x50 */ KEY_KP2, KEY_KP3, KEY_KP0, KEY_KPDOT, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_F11,
    /* 0x58 */ KEY_F12, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA,
    /* 0x60 */ KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA,
    /* 0x68 */ KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA,
    /* 0x70 */ KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA,
    /* 0x78 */ KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA
};

/* Extended scancode mapping (E0 prefix) */
static const tak_bertanda32_t g_extended_key[128] = {
    /* 0x00 */ KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA,
    /* 0x08 */ KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA,
    /* 0x10 */ KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA,
    /* 0x18 */ KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_ENTER, KEY_RIGHTCTRL, KEY_TIDAK_ADA, KEY_TIDAK_ADA,
    /* 0x20 */ KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA,
    /* 0x28 */ KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA,
    /* 0x30 */ KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA,
    /* 0x38 */ KEY_RIGHTALT, KEY_TIDAK_ADA, KEY_HOME, KEY_UP, KEY_PAGEUP, KEY_TIDAK_ADA, KEY_LEFT, KEY_TIDAK_ADA,
    /* 0x40 */ KEY_RIGHT, KEY_TIDAK_ADA, KEY_END, KEY_DOWN, KEY_PAGEDOWN, KEY_INSERT, KEY_DELETE, KEY_TIDAK_ADA,
    /* 0x48 */ KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA,
    /* 0x50 */ KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA,
    /* 0x58 */ KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA,
    /* 0x60 */ KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA,
    /* 0x68 */ KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA,
    /* 0x70 */ KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA,
    /* 0x78 */ KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA
};

/*
 * ===========================================================================
 * FUNGSI INTERNAL
 * ===========================================================================
 */

/*
 * masukan_set_error - Set pesan error
 */
static void masukan_set_error(const char *pesan)
{
    ukuran_t i;
    
    if (pesan == NULL) {
        return;
    }
    
    i = 0;
    while (pesan[i] != '\0' && i < 255) {
        g_masukan_error[i] = pesan[i];
        i++;
    }
    g_masukan_error[i] = '\0';
}

/*
 * masukan_validasi - Validasi konteks
 */
static bool_t masukan_validasi(void)
{
    if (!g_masukan_diinisialisasi) {
        masukan_set_error("Masukan belum diinisialisasi");
        return SALAH;
    }
    
    if (g_masukan_konteks.magic != MASUKAN_MAGIC) {
        masukan_set_error("Konteks masukan tidak valid");
        return SALAH;
    }
    
    return BENAR;
}

/*
 * inisialisasi_konteks_masukan - Inisialisasi konteks
 */
static void inisialisasi_konteks_masukan(masukan_konteks_t *ctx)
{
    tak_bertanda32_t i;
    
    if (ctx == NULL) {
        return;
    }
    
    ctx->magic = 0;
    ctx->device_list = NULL;
    ctx->device_count = 0;
    ctx->global_head = 0;
    ctx->global_tail = 0;
    ctx->led_state = 0;
    ctx->repeat_delay = 500;  /* 500ms */
    ctx->repeat_rate = 30;    /* 30 chars/sec */
    ctx->mouse_x = 0;
    ctx->mouse_y = 0;
    ctx->mouse_buttons = 0;
    ctx->total_events = 0;
    ctx->diinisialisasi = SALAH;
    
    /* Clear global queue */
    for (i = 0; i < INPUT_EVENT_QUEUE * 2; i++) {
        ctx->global_queue[i].magic = 0;
    }
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - INISIALISASI
 * ===========================================================================
 */

status_t masukan_init(void)
{
    status_t hasil;
    
    if (g_masukan_diinisialisasi) {
        return STATUS_SUDAH_ADA;
    }
    
    /* Inisialisasi konteks */
    inisialisasi_konteks_masukan(&g_masukan_konteks);
    
    /* Inisialisasi subsistem */
    hasil = papanketik_init();
    if (hasil != STATUS_BERHASIL) {
        /* Continue anyway */
    }
    
    hasil = tetikus_init();
    if (hasil != STATUS_BERHASIL) {
        /* Continue anyway */
    }
    
    hasil = layarsentuh_init();
    if (hasil != STATUS_BERHASIL) {
        /* Continue anyway */
    }
    
    hasil = joystick_init();
    if (hasil != STATUS_BERHASIL) {
        /* Continue anyway */
    }
    
    hasil = gamepad_init();
    if (hasil != STATUS_BERHASIL) {
        /* Continue anyway */
    }
    
    hasil = hid_init();
    if (hasil != STATUS_BERHASIL) {
        /* Continue anyway */
    }
    
    /* Finalisasi */
    g_masukan_konteks.magic = MASUKAN_MAGIC;
    g_masukan_konteks.diinisialisasi = BENAR;
    g_masukan_diinisialisasi = BENAR;
    
    return STATUS_BERHASIL;
}

status_t masukan_shutdown(void)
{
    inputdev_t *dev;
    inputdev_t *next;
    
    if (!g_masukan_diinisialisasi) {
        return STATUS_BERHASIL;
    }
    
    /* Bebaskan semua device */
    dev = g_masukan_konteks.device_list;
    while (dev != NULL) {
        next = dev->berikutnya;
        dev->magic = 0;
        kfree(dev);
        dev = next;
    }
    
    /* Reset konteks */
    g_masukan_konteks.magic = 0;
    g_masukan_konteks.device_list = NULL;
    g_masukan_konteks.device_count = 0;
    g_masukan_konteks.diinisialisasi = SALAH;
    
    g_masukan_diinisialisasi = SALAH;
    
    return STATUS_BERHASIL;
}

masukan_konteks_t *masukan_konteks_dapatkan(void)
{
    if (!g_masukan_diinisialisasi) {
        return NULL;
    }
    
    if (g_masukan_konteks.magic != MASUKAN_MAGIC) {
        return NULL;
    }
    
    return &g_masukan_konteks;
}

/*
 * ===========================================================================
 * FUNGSI DEVICE
 * ===========================================================================
 */

inputdev_t *inputdev_register(const char *nama, tak_bertanda32_t tipe)
{
    inputdev_t *dev;
    
    if (!masukan_validasi()) {
        return NULL;
    }
    
    if (nama == NULL) {
        return NULL;
    }
    
    /* Alokasi device */
    dev = (inputdev_t *)kmalloc(sizeof(inputdev_t));
    if (dev == NULL) {
        masukan_set_error("Gagal alokasi memori");
        return NULL;
    }
    
    /* Inisialisasi */
    kernel_memset(dev, 0, sizeof(inputdev_t));
    
    dev->magic = INPUTDEV_MAGIC;
    dev->id = g_masukan_konteks.device_count;
    dev->tipe = tipe;
    dev->status = INPUT_STATUS_TERDETEKSI;
    dev->flags = 0;
    dev->num_keys = 0;
    dev->num_buttons = 0;
    dev->num_axes = 0;
    dev->num_touches = 0;
    dev->abs_min_x = 0;
    dev->abs_max_x = 1024;
    dev->abs_min_y = 0;
    dev->abs_max_y = 768;
    dev->key_state = 0;
    dev->button_state = 0;
    dev->touch_count = 0;
    dev->queue_head = 0;
    dev->queue_tail = 0;
    dev->queue_count = 0;
    dev->callback = NULL;
    dev->irq_handler = NULL;
    dev->priv = NULL;
    dev->berikutnya = NULL;
    
    /* Salin nama */
    {
        ukuran_t i = 0;
        while (nama[i] != '\0' && i < INPUTDEV_NAMA_MAKS - 1) {
            dev->nama[i] = nama[i];
            i++;
        }
        dev->nama[i] = '\0';
    }
    
    /* Tambah ke list */
    if (g_masukan_konteks.device_list == NULL) {
        g_masukan_konteks.device_list = dev;
    } else {
        inputdev_t *last = g_masukan_konteks.device_list;
        while (last->berikutnya != NULL) {
            last = last->berikutnya;
        }
        last->berikutnya = dev;
    }
    
    g_masukan_konteks.device_count++;
    
    return dev;
}

status_t inputdev_unregister(inputdev_t *dev)
{
    inputdev_t *prev;
    inputdev_t *curr;
    
    if (!masukan_validasi()) {
        return STATUS_GAGAL;
    }
    
    if (dev == NULL || dev->magic != INPUTDEV_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Cari di list */
    prev = NULL;
    curr = g_masukan_konteks.device_list;
    
    while (curr != NULL) {
        if (curr == dev) {
            if (prev == NULL) {
                g_masukan_konteks.device_list = curr->berikutnya;
            } else {
                prev->berikutnya = curr->berikutnya;
            }
            
            dev->magic = 0;
            kfree(dev);
            
            g_masukan_konteks.device_count--;
            
            return STATUS_BERHASIL;
        }
        
        prev = curr;
        curr = curr->berikutnya;
    }
    
    return STATUS_TIDAK_DITEMUKAN;
}

inputdev_t *inputdev_cari(tak_bertanda32_t id)
{
    inputdev_t *dev;
    
    if (!masukan_validasi()) {
        return NULL;
    }
    
    dev = g_masukan_konteks.device_list;
    
    while (dev != NULL) {
        if (dev->id == id) {
            return dev;
        }
        dev = dev->berikutnya;
    }
    
    return NULL;
}

inputdev_t *inputdev_cari_tipe(tak_bertanda32_t tipe, tak_bertanda32_t indeks)
{
    inputdev_t *dev;
    tak_bertanda32_t count = 0;
    
    if (!masukan_validasi()) {
        return NULL;
    }
    
    dev = g_masukan_konteks.device_list;
    
    while (dev != NULL) {
        if (dev->tipe == tipe) {
            if (count == indeks) {
                return dev;
            }
            count++;
        }
        dev = dev->berikutnya;
    }
    
    return NULL;
}

status_t inputdev_set_callback(inputdev_t *dev,
                                input_event_callback_t callback)
{
    if (dev == NULL || dev->magic != INPUTDEV_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    
    dev->callback = callback;
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI EVENT
 * ===========================================================================
 */

status_t masukan_event_push(input_event_t *event)
{
    masukan_konteks_t *ctx;
    
    if (event == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (!masukan_validasi()) {
        return STATUS_GAGAL;
    }
    
    ctx = &g_masukan_konteks;
    
    /* Cek apakah queue penuh */
    if (ctx->global_head == (ctx->global_tail + 1) % (INPUT_EVENT_QUEUE * 2)) {
        return STATUS_PENUH;
    }
    
    /* Copy event ke queue */
    ctx->global_queue[ctx->global_tail] = *event;
    ctx->global_queue[ctx->global_tail].magic = EVENT_MAGIC;
    
    ctx->global_tail = (ctx->global_tail + 1) % (INPUT_EVENT_QUEUE * 2);
    ctx->total_events++;
    
    return STATUS_BERHASIL;
}

status_t masukan_event_pop(input_event_t *event)
{
    masukan_konteks_t *ctx;
    
    if (event == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (!masukan_validasi()) {
        return STATUS_GAGAL;
    }
    
    ctx = &g_masukan_konteks;
    
    /* Cek apakah queue kosong */
    if (ctx->global_head == ctx->global_tail) {
        return STATUS_KOSONG;
    }
    
    /* Copy event dari queue */
    *event = ctx->global_queue[ctx->global_head];
    
    ctx->global_head = (ctx->global_head + 1) % (INPUT_EVENT_QUEUE * 2);
    
    return STATUS_BERHASIL;
}

status_t masukan_event_peek(input_event_t *event)
{
    masukan_konteks_t *ctx;
    
    if (event == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (!masukan_validasi()) {
        return STATUS_GAGAL;
    }
    
    ctx = &g_masukan_konteks;
    
    if (ctx->global_head == ctx->global_tail) {
        return STATUS_KOSONG;
    }
    
    *event = ctx->global_queue[ctx->global_head];
    
    return STATUS_BERHASIL;
}

tak_bertanda32_t masukan_event_count(void)
{
    masukan_konteks_t *ctx;
    
    if (!g_masukan_diinisialisasi) {
        return 0;
    }
    
    ctx = &g_masukan_konteks;
    
    if (ctx->global_tail >= ctx->global_head) {
        return ctx->global_tail - ctx->global_head;
    } else {
        return (INPUT_EVENT_QUEUE * 2) - ctx->global_head + ctx->global_tail;
    }
}

void masukan_event_clear(void)
{
    masukan_konteks_t *ctx;
    
    if (!g_masukan_diinisialisasi) {
        return;
    }
    
    ctx = &g_masukan_konteks;
    
    ctx->global_head = 0;
    ctx->global_tail = 0;
}

/*
 * ===========================================================================
 * FUNGSI KEYBOARD
 * ===========================================================================
 */

static inputdev_t *g_keyboard_dev = NULL;
static bool_t g_extended_mode = SALAH;

status_t papanketik_init(void)
{
    inputdev_t *dev;
    
    dev = inputdev_register("keyboard0", INPUT_TIPE_KEYBOARD);
    if (dev == NULL) {
        return STATUS_GAGAL;
    }
    
    dev->num_keys = 128;
    dev->status = INPUT_STATUS_AKTIF;
    
    g_keyboard_dev = dev;
    
    return STATUS_BERHASIL;
}

status_t papanketik_event(tak_bertanda32_t scancode, tak_bertanda32_t status)
{
    input_event_t event;
    tak_bertanda32_t key;
    
    if (g_keyboard_dev == NULL) {
        return STATUS_GAGAL;
    }
    
    /* Cek extended mode */
    if (scancode == 0xE0) {
        g_extended_mode = BENAR;
        return STATUS_BERHASIL;
    }
    
    /* Konversi scancode ke key code */
    if (g_extended_mode) {
        key = g_extended_key[scancode & 0x7F];
        g_extended_mode = SALAH;
    } else {
        key = g_scancode_to_key[scancode & 0x7F];
    }
    
    /* Buat event */
    kernel_memset(&event, 0, sizeof(event));
    event.magic = EVENT_MAGIC;
    event.timestamp = 0;  /* TODO: get timestamp */
    event.tipe = EVENT_TIPE_KEY;
    event.device_id = g_keyboard_dev->id;
    event.data.key.kode = key;
    event.data.key.status = (scancode & 0x80) ? 0 : 1;  /* Release = 0, Press = 1 */
    event.data.key.scancode = scancode & 0x7F;
    
    /* Update modifier state */
    if (status) {  /* Press */
        switch (key) {
        case KEY_LEFTCTRL:
        case KEY_RIGHTCTRL:
            g_masukan_konteks.led_state |= KEY_MOD_CTRL;
            break;
        case KEY_LEFTSHIFT:
        case KEY_RIGHTSHIFT:
            g_masukan_konteks.led_state |= KEY_MOD_SHIFT;
            break;
        case KEY_LEFTALT:
        case KEY_RIGHTALT:
            g_masukan_konteks.led_state |= KEY_MOD_ALT;
            break;
        case KEY_CAPSLOCK:
            g_masukan_konteks.led_state ^= KEY_MOD_CAPSLOCK;
            papanketik_set_leds(g_masukan_konteks.led_state);
            break;
        }
    } else {  /* Release */
        switch (key) {
        case KEY_LEFTCTRL:
        case KEY_RIGHTCTRL:
            g_masukan_konteks.led_state &= ~KEY_MOD_CTRL;
            break;
        case KEY_LEFTSHIFT:
        case KEY_RIGHTSHIFT:
            g_masukan_konteks.led_state &= ~KEY_MOD_SHIFT;
            break;
        case KEY_LEFTALT:
        case KEY_RIGHTALT:
            g_masukan_konteks.led_state &= ~KEY_MOD_ALT;
            break;
        }
    }
    
    event.data.key.modifier = g_masukan_konteks.led_state;
    
    /* Translate ke unicode */
    event.data.key.unicode = papanketik_translate(key, 
                                                  event.data.key.modifier);
    
    /* Push event */
    masukan_event_push(&event);
    
    /* Panggil callback jika ada */
    if (g_keyboard_dev->callback != NULL) {
        g_keyboard_dev->callback(g_keyboard_dev, &event);
    }
    
    return STATUS_BERHASIL;
}

status_t papanketik_set_leds(tak_bertanda32_t leds)
{
    /* Kirim ke keyboard controller */
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    tak_bertanda8_t data;
    
    data = 0;
    if (leds & KEY_MOD_SCROLLLOCK) {
        data |= 0x01;
    }
    if (leds & KEY_MOD_NUMLOCK) {
        data |= 0x02;
    }
    if (leds & KEY_MOD_CAPSLOCK) {
        data |= 0x04;
    }
    
    /* Kirim command ke keyboard */
    outb(0x60, 0xED);  /* Set LEDs command */
    /* Wait for ACK */
    /* outb(0x60, data); */
#endif
    
    return STATUS_BERHASIL;
}

tak_bertanda32_t papanketik_get_state(void)
{
    return g_masukan_konteks.led_state;
}

tak_bertanda32_t papanketik_translate(tak_bertanda32_t scancode,
                                       tak_bertanda32_t modifier)
{
    /* Key code to ASCII mapping (simplified) */
    static const char key_to_ascii[] = {
        /* 0-9 */ '\0', '\0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
        '-', '=', '\b', '\t',
        /* 10-19 */ 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
        '[', ']', '\n', '\0', 'a', 's',
        /* 20-29 */ 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
        '\0', '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
        '\0', '\0', '\0', ' '
    };
    
    static const char shift_key_to_ascii[] = {
        '\0', '\0', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')',
        '_', '+', '\b', '\t',
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
        '{', '}', '\n', '\0', 'A', 'S',
        'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
        '\0', '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',
        '\0', '\0', '\0', ' '
    };
    
    if (scancode >= sizeof(key_to_ascii)) {
        return 0;
    }
    
    /* Caps handling */
    if (modifier & KEY_MOD_SHIFT) {
        return (tak_bertanda32_t)shift_key_to_ascii[scancode];
    }
    
    if (modifier & KEY_MOD_CAPSLOCK) {
        char c = key_to_ascii[scancode];
        if (c >= 'a' && c <= 'z') {
            c -= 32;
        }
        return (tak_bertanda32_t)c;
    }
    
    return (tak_bertanda32_t)key_to_ascii[scancode];
}

/*
 * ===========================================================================
 * FUNGSI MOUSE
 * ===========================================================================
 */

static inputdev_t *g_mouse_dev = NULL;

status_t tetikus_init(void)
{
    inputdev_t *dev;
    
    dev = inputdev_register("mouse0", INPUT_TIPE_MOUSE);
    if (dev == NULL) {
        return STATUS_GAGAL;
    }
    
    dev->num_buttons = 5;
    dev->num_axes = 4;
    dev->abs_max_x = 1024;
    dev->abs_max_y = 768;
    dev->status = INPUT_STATUS_AKTIF;
    
    g_mouse_dev = dev;
    
    return STATUS_BERHASIL;
}

status_t tetikus_event(tak_bertanda32_t buttons, tanda32_t dx,
                        tanda32_t dy, tanda32_t wheel)
{
    input_event_t event;
    tak_bertanda32_t button_change;
    
    if (g_mouse_dev == NULL) {
        return STATUS_GAGAL;
    }
    
    /* Hitung button change */
    button_change = buttons ^ g_masukan_konteks.mouse_buttons;
    
    /* Update state */
    g_masukan_konteks.mouse_buttons = buttons;
    g_masukan_konteks.mouse_x += dx;
    g_masukan_konteks.mouse_y += dy;
    
    /* Clamp ke screen bounds */
    if (g_masukan_konteks.mouse_x < 0) {
        g_masukan_konteks.mouse_x = 0;
    }
    if (g_masukan_konteks.mouse_y < 0) {
        g_masukan_konteks.mouse_y = 0;
    }
    if (g_masukan_konteks.mouse_x > g_mouse_dev->abs_max_x) {
        g_masukan_konteks.mouse_x = g_mouse_dev->abs_max_x;
    }
    if (g_masukan_konteks.mouse_y > g_mouse_dev->abs_max_y) {
        g_masukan_konteks.mouse_y = g_mouse_dev->abs_max_y;
    }
    
    /* Buat event */
    kernel_memset(&event, 0, sizeof(event));
    event.magic = EVENT_MAGIC;
    event.timestamp = 0;
    event.tipe = EVENT_TIPE_RELATIVE;
    event.device_id = g_mouse_dev->id;
    event.data.mouse.tombol = buttons;
    event.data.mouse.tombol_status = button_change;
    event.data.mouse.rel_x = dx;
    event.data.mouse.rel_y = dy;
    event.data.mouse.wheel = wheel;
    event.data.mouse.abs_x = g_masukan_konteks.mouse_x;
    event.data.mouse.abs_y = g_masukan_konteks.mouse_y;
    
    /* Push event */
    masukan_event_push(&event);
    
    /* Callback */
    if (g_mouse_dev->callback != NULL) {
        g_mouse_dev->callback(g_mouse_dev, &event);
    }
    
    return STATUS_BERHASIL;
}

void tetikus_get_position(tak_bertanda32_t *x, tak_bertanda32_t *y)
{
    if (x != NULL) {
        *x = g_masukan_konteks.mouse_x;
    }
    if (y != NULL) {
        *y = g_masukan_konteks.mouse_y;
    }
}

void tetikus_set_position(tak_bertanda32_t x, tak_bertanda32_t y)
{
    g_masukan_konteks.mouse_x = x;
    g_masukan_konteks.mouse_y = y;
}

tak_bertanda32_t tetikus_get_buttons(void)
{
    return g_masukan_konteks.mouse_buttons;
}

/*
 * ===========================================================================
 * FUNGSI TOUCHSCREEN
 * ===========================================================================
 */

status_t layarsentuh_init(void)
{
    return STATUS_BERHASIL;
}

status_t layarsentuh_event(tak_bertanda32_t id, tak_bertanda32_t tipe,
                            tak_bertanda32_t x, tak_bertanda32_t y,
                            tak_bertanda32_t pressure)
{
    input_event_t event;
    inputdev_t *dev;
    
    dev = inputdev_cari_tipe(INPUT_TIPE_TOUCHSCREEN, 0);
    if (dev == NULL) {
        return STATUS_GAGAL;
    }
    
    kernel_memset(&event, 0, sizeof(event));
    event.magic = EVENT_MAGIC;
    event.timestamp = 0;
    event.tipe = EVENT_TIPE_TOUCH;
    event.device_id = dev->id;
    event.data.touch.id = id;
    event.data.touch.tipe = tipe;
    event.data.touch.x = x;
    event.data.touch.y = y;
    event.data.touch.pressure = pressure;
    
    masukan_event_push(&event);
    
    if (dev->callback != NULL) {
        dev->callback(dev, &event);
    }
    
    return STATUS_BERHASIL;
}

tak_bertanda32_t layarsentuh_get_touches(touch_event_t *touches,
                                          tak_bertanda32_t max_count)
{
    /* TODO: Implementasi tracking touch points */
    return 0;
}

/*
 * ===========================================================================
 * FUNGSI JOYSTICK/GAMEPAD
 * ===========================================================================
 */

status_t joystick_init(void) { return STATUS_BERHASIL; }

status_t joystick_event(tak_bertanda32_t id, tanda32_t *axis,
                         tak_bertanda32_t buttons)
{
    return STATUS_BERHASIL;
}

status_t gamepad_init(void) { return STATUS_BERHASIL; }

status_t gamepad_event(tak_bertanda32_t id, tak_bertanda32_t buttons,
                        tanda16_t left_x, tanda16_t left_y,
                        tanda16_t right_x, tanda16_t right_y,
                        tak_bertanda32_t triggers)
{
    return STATUS_BERHASIL;
}

status_t gamepad_rumble(tak_bertanda32_t id, tak_bertanda16_t weak,
                         tak_bertanda16_t strong)
{
    return STATUS_TIDAK_DUKUNG;
}

/*
 * ===========================================================================
 * FUNGSI HID
 * ===========================================================================
 */

status_t hid_init(void) { return STATUS_BERHASIL; }

status_t hid_parse(inputdev_t *dev, const tak_bertanda8_t *descriptor,
                    ukuran_t len)
{
    return STATUS_TIDAK_DUKUNG;
}

status_t hid_process_report(inputdev_t *dev, const tak_bertanda8_t *report,
                             ukuran_t len)
{
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI UTILITAS
 * ===========================================================================
 */

const char *masukan_nama_tipe(tak_bertanda32_t tipe)
{
    switch (tipe) {
    case INPUT_TIPE_KEYBOARD:
        return "Keyboard";
    case INPUT_TIPE_MOUSE:
        return "Mouse";
    case INPUT_TIPE_TOUCHSCREEN:
        return "Touchscreen";
    case INPUT_TIPE_JOYSTICK:
        return "Joystick";
    case INPUT_TIPE_GAMEPAD:
        return "Gamepad";
    case INPUT_TIPE_TOUCHPAD:
        return "Touchpad";
    case INPUT_TIPE_TABLET:
        return "Tablet";
    case INPUT_TIPE_STYLUS:
        return "Stylus";
    default:
        return "Tidak Diketahui";
    }
}

const char *masukan_nama_key(tak_bertanda32_t kode)
{
    static const char *key_names[] = {
        [KEY_ESC] = "Escape",
        [KEY_1] = "1", [KEY_2] = "2", [KEY_3] = "3",
        [KEY_4] = "4", [KEY_5] = "5", [KEY_6] = "6",
        [KEY_7] = "7", [KEY_8] = "8", [KEY_9] = "9",
        [KEY_0] = "0",
        [KEY_MINUS] = "Minus", [KEY_EQUAL] = "Equal",
        [KEY_BACKSPACE] = "Backspace", [KEY_TAB] = "Tab",
        [KEY_Q] = "Q", [KEY_W] = "W", [KEY_E] = "E",
        [KEY_R] = "R", [KEY_T] = "T", [KEY_Y] = "Y",
        [KEY_U] = "U", [KEY_I] = "I", [KEY_O] = "O",
        [KEY_P] = "P",
        [KEY_LEFTBRACE] = "LeftBrace", [KEY_RIGHTBRACE] = "RightBrace",
        [KEY_ENTER] = "Enter", [KEY_LEFTCTRL] = "LeftCtrl",
        [KEY_A] = "A", [KEY_S] = "S", [KEY_D] = "D",
        [KEY_F] = "F", [KEY_G] = "G", [KEY_H] = "H",
        [KEY_J] = "J", [KEY_K] = "K", [KEY_L] = "L",
        [KEY_SEMICOLON] = "Semicolon", [KEY_APOSTROPHE] = "Apostrophe",
        [KEY_GRAVE] = "Grave", [KEY_LEFTSHIFT] = "LeftShift",
        [KEY_BACKSLASH] = "Backslash",
        [KEY_Z] = "Z", [KEY_X] = "X", [KEY_C] = "C",
        [KEY_V] = "V", [KEY_B] = "B", [KEY_N] = "N",
        [KEY_M] = "M",
        [KEY_COMMA] = "Comma", [KEY_DOT] = "Dot", [KEY_SLASH] = "Slash",
        [KEY_RIGHTSHIFT] = "RightShift", [KEY_KPASTERISK] = "KeypadAsterisk",
        [KEY_LEFTALT] = "LeftAlt", [KEY_SPACE] = "Space",
        [KEY_CAPSLOCK] = "CapsLock",
        [KEY_F1] = "F1", [KEY_F2] = "F2", [KEY_F3] = "F3",
        [KEY_F4] = "F4", [KEY_F5] = "F5", [KEY_F6] = "F6",
        [KEY_F7] = "F7", [KEY_F8] = "F8", [KEY_F9] = "F9",
        [KEY_F10] = "F10",
        [KEY_NUMLOCK] = "NumLock", [KEY_SCROLLLOCK] = "ScrollLock",
        [KEY_HOME] = "Home", [KEY_UP] = "Up",
        [KEY_PAGEUP] = "PageUp", [KEY_LEFT] = "Left",
        [KEY_RIGHT] = "Right", [KEY_END] = "End",
        [KEY_DOWN] = "Down", [KEY_PAGEDOWN] = "PageDown",
        [KEY_INSERT] = "Insert", [KEY_DELETE] = "Delete"
    };
    
    if (kode < sizeof(key_names) / sizeof(key_names[0])) {
        return key_names[kode];
    }
    
    return "Unknown";
}

void masukan_cetak_info(void)
{
    inputdev_t *dev;
    
    if (!g_masukan_diinisialisasi) {
        return;
    }
    
    kernel_printf("\n=== Input Devices ===\n\n");
    
    dev = g_masukan_konteks.device_list;
    
    while (dev != NULL) {
        kernel_printf("%s (ID: %u)\n", dev->nama, dev->id);
        kernel_printf("  Type: %s\n", masukan_nama_tipe(dev->tipe));
        kernel_printf("  Keys: %u, Buttons: %u, Axes: %u\n",
                     dev->num_keys, dev->num_buttons, dev->num_axes);
        
        dev = dev->berikutnya;
    }
    
    kernel_printf("\n");
}

void masukan_cetak_event(input_event_t *event)
{
    if (event == NULL) {
        return;
    }
    
    kernel_printf("[Event] Dev=%u Type=%u ",
                 event->device_id, event->tipe);
    
    switch (event->tipe) {
    case EVENT_TIPE_KEY:
        kernel_printf("Key=%u State=%u\n",
                     event->data.key.kode, event->data.key.status);
        break;
    case EVENT_TIPE_RELATIVE:
        kernel_printf("DX=%d DY=%d Buttons=%u\n",
                     event->data.mouse.rel_x, event->data.mouse.rel_y,
                     event->data.mouse.tombol);
        break;
    case EVENT_TIPE_TOUCH:
        kernel_printf("ID=%u X=%u Y=%u\n",
                     event->data.touch.id, event->data.touch.x,
                     event->data.touch.y);
        break;
    default:
        kernel_printf("\n");
    }
}
