/*
 * PIGURA OS - PAPANKETIK.C
 * ========================
 * Implementasi subsistem papanketik (keyboard).
 *
 * Berkas ini mengimplementasikan penanganan perangkat keyboard dengan
 * dukungan scancode translation, modifier tracking, dan key mapping.
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
 * KONSTANTA PAPANKETIK
 * ===========================================================================
 */

/* Batasan keyboard */
#define PAPANKETIK_MAKS_DEVICE      4
#define PAPANKETIK_KEYMAP_SIZE      128
#define PAPANKETIK_EXT_KEYMAP_SIZE  128

/* Status key */
#define PAPANKETIK_KEY_LEPAS        0
#define PAPANKETIK_KEY_TEKAN        1
#define PAPANKETIK_KEY_ULANG        2

/* Modifier index */
#define PAPANKETIK_MOD_CTRL         0
#define PAPANKETIK_MOD_SHIFT        1
#define PAPANKETIK_MOD_ALT          2
#define PAPANKETIK_MOD_SUPER        3
#define PAPANKETIK_MOD_CAPSLOCK     4
#define PAPANKETIK_MOD_NUMLOCK      5

/* LED mask */
#define PAPANKETIK_LED_SCROLL       0x01
#define PAPANKETIK_LED_NUM          0x02
#define PAPANKETIK_LED_CAPS         0x04

/* Key repeat */
#define PAPANKETIK_REPEAT_DELAY     500
#define PAPANKETIK_REPEAT_RATE      30

/*
 * ===========================================================================
 * STRUKTUR DATA PAPANKETIK
 * ===========================================================================
 */

/* Key state tracking */
typedef struct {
    bool_t ditekan;
    tak_bertanda64_t waktu_tekan;
    tak_bertanda32_t jumlah_ulang;
} key_state_t;

/* Data device keyboard */
typedef struct {
    inputdev_t *dev;
    
    /* Modifier state */
    tak_bertanda32_t modifier;
    
    /* LED state */
    tak_bertanda32_t led_state;
    
    /* Key states */
    key_state_t key_states[INPUT_KEY_MAKS];
    
    /* Extended mode */
    bool_t extended_mode;
    
    /* Key repeat config */
    tak_bertanda32_t repeat_delay;
    tak_bertanda32_t repeat_rate;
    
    /* Status */
    bool_t aktif;
} papanketik_device_t;

/* Konteks keyboard */
typedef struct {
    papanketik_device_t devices[PAPANKETIK_MAKS_DEVICE];
    tak_bertanda32_t device_count;
    
    /* Keymap */
    tak_bertanda32_t keymap[PAPANKETIK_KEYMAP_SIZE];
    tak_bertanda32_t ext_keymap[PAPANKETIK_EXT_KEYMAP_SIZE];
    
    /* Status */
    bool_t diinisialisasi;
} papanketik_konteks_t;

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

static papanketik_konteks_t g_papanketik_konteks;
static bool_t g_papanketik_diinisialisasi = SALAH;

/* Keyboard scancode to key code mapping (US QWERTY) */
static const tak_bertanda32_t g_default_keymap[128] = {
    /* 0x00 */ KEY_TIDAK_ADA, KEY_ESC, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6,
    /* 0x08 */ KEY_7, KEY_8, KEY_9, KEY_0, KEY_MINUS, KEY_EQUAL, 
                KEY_BACKSPACE, KEY_TAB,
    /* 0x10 */ KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_U, KEY_I,
    /* 0x18 */ KEY_O, KEY_P, KEY_LEFTBRACE, KEY_RIGHTBRACE, 
                KEY_ENTER, KEY_LEFTCTRL, KEY_A, KEY_S,
    /* 0x20 */ KEY_D, KEY_F, KEY_G, KEY_H, KEY_J, KEY_K, KEY_L, KEY_SEMICOLON,
    /* 0x28 */ KEY_APOSTROPHE, KEY_GRAVE, KEY_LEFTSHIFT, KEY_BACKSLASH, 
                KEY_Z, KEY_X, KEY_C, KEY_V,
    /* 0x30 */ KEY_B, KEY_N, KEY_M, KEY_COMMA, KEY_DOT, KEY_SLASH, 
                KEY_RIGHTSHIFT, KEY_KPASTERISK,
    /* 0x38 */ KEY_LEFTALT, KEY_SPACE, KEY_CAPSLOCK, KEY_F1, KEY_F2, 
                KEY_F3, KEY_F4, KEY_F5,
    /* 0x40 */ KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_NUMLOCK, 
                KEY_SCROLLLOCK, KEY_KP7,
    /* 0x48 */ KEY_KP8, KEY_KP9, KEY_KPMINUS, KEY_KP4, KEY_KP5, 
                KEY_KP6, KEY_KPPLUS, KEY_KP1,
    /* 0x50 */ KEY_KP2, KEY_KP3, KEY_KP0, KEY_KPDOT, KEY_TIDAK_ADA, 
                KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_F11,
    /* 0x58 */ KEY_F12, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, 
                KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA,
    /* 0x60 */ KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, 
                KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA,
    /* 0x68 */ KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, 
                KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA,
    /* 0x70 */ KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, 
                KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA,
    /* 0x78 */ KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, 
                KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA
};

/* Extended scancode mapping (E0 prefix) */
static const tak_bertanda32_t g_default_ext_keymap[128] = {
    /* 0x00 */ KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, 
                KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA,
    /* 0x08 */ KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, 
                KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA,
    /* 0x10 */ KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, 
                KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA,
    /* 0x18 */ KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, 
                KEY_ENTER, KEY_RIGHTCTRL, KEY_TIDAK_ADA, KEY_TIDAK_ADA,
    /* 0x20 */ KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, 
                KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA,
    /* 0x28 */ KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, 
                KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA,
    /* 0x30 */ KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, 
                KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA,
    /* 0x38 */ KEY_RIGHTALT, KEY_TIDAK_ADA, KEY_HOME, KEY_UP, 
                KEY_PAGEUP, KEY_TIDAK_ADA, KEY_LEFT, KEY_TIDAK_ADA,
    /* 0x40 */ KEY_RIGHT, KEY_TIDAK_ADA, KEY_END, KEY_DOWN, 
                KEY_PAGEDOWN, KEY_INSERT, KEY_DELETE, KEY_TIDAK_ADA,
    /* 0x48 */ KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, 
                KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA,
    /* 0x50 */ KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, 
                KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA,
    /* 0x58 */ KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, 
                KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA,
    /* 0x60 */ KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, 
                KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA,
    /* 0x68 */ KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, 
                KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA,
    /* 0x70 */ KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, 
                KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA,
    /* 0x78 */ KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, 
                KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA, KEY_TIDAK_ADA
};

/* Key code to ASCII mapping (normal) */
static const char g_key_to_ascii[] = {
    /* 0-9 */ '\0', '\0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
    '-', '=', '\b', '\t',
    /* 10-19 */ 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
    '[', ']', '\n', '\0', 'a', 's',
    /* 20-29 */ 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    '\0', '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
    '\0', '\0', '\0', ' '
};

/* Key code to ASCII mapping (shift) */
static const char g_shift_key_to_ascii[] = {
    '\0', '\0', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')',
    '_', '+', '\b', '\t',
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
    '{', '}', '\n', '\0', 'A', 'S',
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    '\0', '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',
    '\0', '\0', '\0', ' '
};

/*
 * ===========================================================================
 * FUNGSI INTERNAL
 * ===========================================================================
 */

/*
 * papanketik_cari_device - Cari device keyboard berdasarkan ID
 */
static papanketik_device_t *papanketik_cari_device(tak_bertanda32_t id)
{
    tak_bertanda32_t i;
    
    if (!g_papanketik_diinisialisasi) {
        return NULL;
    }
    
    for (i = 0; i < g_papanketik_konteks.device_count; i++) {
        if (g_papanketik_konteks.devices[i].dev != NULL &&
            g_papanketik_konteks.devices[i].dev->id == id) {
            return &g_papanketik_konteks.devices[i];
        }
    }
    
    return NULL;
}

/*
 * papanketik_cari_slot_kosong - Cari slot device kosong
 */
static tak_bertanda32_t papanketik_cari_slot_kosong(void)
{
    tak_bertanda32_t i;
    
    for (i = 0; i < PAPANKETIK_MAKS_DEVICE; i++) {
        if (g_papanketik_konteks.devices[i].dev == NULL) {
            return i;
        }
    }
    
    return INDEX_INVALID;
}

/*
 * papanketik_scancode_ke_keycode - Konversi scancode ke keycode
 */
static tak_bertanda32_t papanketik_scancode_ke_keycode(
    papanketik_device_t *pdev,
    tak_bertanda32_t scancode)
{
    tak_bertanda32_t keycode;
    tak_bertanda32_t index;
    
    if (pdev == NULL) {
        return KEY_TIDAK_ADA;
    }
    
    index = scancode & 0x7F;
    
    if (pdev->extended_mode) {
        if (index < PAPANKETIK_EXT_KEYMAP_SIZE) {
            keycode = g_papanketik_konteks.ext_keymap[index];
        } else {
            keycode = KEY_TIDAK_ADA;
        }
        pdev->extended_mode = SALAH;
    } else {
        if (index < PAPANKETIK_KEYMAP_SIZE) {
            keycode = g_papanketik_konteks.keymap[index];
        } else {
            keycode = KEY_TIDAK_ADA;
        }
    }
    
    return keycode;
}

/*
 * papanketik_update_modifier - Update state modifier
 */
static void papanketik_update_modifier(papanketik_device_t *pdev,
                                        tak_bertanda32_t keycode,
                                        tak_bertanda32_t status)
{
    if (pdev == NULL) {
        return;
    }
    
    if (status) {  /* Key press */
        switch (keycode) {
        case KEY_LEFTCTRL:
        case KEY_RIGHTCTRL:
            pdev->modifier |= KEY_MOD_CTRL;
            break;
        case KEY_LEFTSHIFT:
        case KEY_RIGHTSHIFT:
            pdev->modifier |= KEY_MOD_SHIFT;
            break;
        case KEY_LEFTALT:
        case KEY_RIGHTALT:
            pdev->modifier |= KEY_MOD_ALT;
            break;
        case KEY_CAPSLOCK:
            pdev->modifier ^= KEY_MOD_CAPSLOCK;
            pdev->led_state ^= PAPANKETIK_LED_CAPS;
            papanketik_set_leds(pdev->led_state);
            break;
        case KEY_NUMLOCK:
            pdev->modifier ^= KEY_MOD_NUMLOCK;
            pdev->led_state ^= PAPANKETIK_LED_NUM;
            papanketik_set_leds(pdev->led_state);
            break;
        default:
            break;
        }
    } else {  /* Key release */
        switch (keycode) {
        case KEY_LEFTCTRL:
        case KEY_RIGHTCTRL:
            pdev->modifier &= ~KEY_MOD_CTRL;
            break;
        case KEY_LEFTSHIFT:
        case KEY_RIGHTSHIFT:
            pdev->modifier &= ~KEY_MOD_SHIFT;
            break;
        case KEY_LEFTALT:
        case KEY_RIGHTALT:
            pdev->modifier &= ~KEY_MOD_ALT;
            break;
        default:
            break;
        }
    }
}

/*
 * papanketik_keycode_ke_ascii - Konversi keycode ke ASCII
 */
static char papanketik_keycode_ke_ascii(tak_bertanda32_t keycode,
                                         tak_bertanda32_t modifier)
{
    char c;
    
    if (keycode >= sizeof(g_key_to_ascii)) {
        return '\0';
    }
    
    /* Handle shift */
    if (modifier & KEY_MOD_SHIFT) {
        c = g_shift_key_to_ascii[keycode];
    } else {
        c = g_key_to_ascii[keycode];
    }
    
    /* Handle capslock untuk huruf */
    if (modifier & KEY_MOD_CAPSLOCK) {
        if (c >= 'a' && c <= 'z') {
            c = c - 'a' + 'A';
        } else if (c >= 'A' && c <= 'Z' && !(modifier & KEY_MOD_SHIFT)) {
            c = c - 'A' + 'a';
        }
    }
    
    return c;
}

/*
 * papanketik_buat_event - Buat dan kirim keyboard event
 */
static void papanketik_buat_event(papanketik_device_t *pdev,
                                   tak_bertanda32_t keycode,
                                   tak_bertanda32_t scancode,
                                   tak_bertanda32_t status)
{
    input_event_t event;
    char ascii;
    
    if (pdev == NULL || pdev->dev == NULL) {
        return;
    }
    
    kernel_memset(&event, 0, sizeof(input_event_t));
    
    event.magic = EVENT_MAGIC;
    event.timestamp = kernel_get_ticks();
    event.tipe = EVENT_TIPE_KEY;
    event.device_id = pdev->dev->id;
    
    event.data.key.kode = keycode;
    event.data.key.status = status;
    event.data.key.scancode = scancode;
    event.data.key.modifier = pdev->modifier;
    
    /* Translate ke unicode/ASCII */
    ascii = papanketik_keycode_ke_ascii(keycode, pdev->modifier);
    event.data.key.unicode = (tak_bertanda32_t)ascii;
    
    /* Push ke event queue */
    masukan_event_push(&event);
    
    /* Panggil callback jika ada */
    if (pdev->dev->callback != NULL) {
        pdev->dev->callback(pdev->dev, &event);
    }
}

/*
 * papanketik_update_key_state - Update state key untuk repeat
 */
static void papanketik_update_key_state(papanketik_device_t *pdev,
                                         tak_bertanda32_t keycode,
                                         tak_bertanda32_t status)
{
    if (pdev == NULL || keycode >= INPUT_KEY_MAKS) {
        return;
    }
    
    if (status) {
        pdev->key_states[keycode].ditekan = BENAR;
        pdev->key_states[keycode].waktu_tekan = kernel_get_ticks();
        pdev->key_states[keycode].jumlah_ulang = 0;
    } else {
        pdev->key_states[keycode].ditekan = SALAH;
        pdev->key_states[keycode].jumlah_ulang = 0;
    }
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

/*
 * papanketik_init - Inisialisasi subsistem keyboard
 */
status_t papanketik_init(void)
{
    tak_bertanda32_t i;
    inputdev_t *dev;
    papanketik_device_t *pdev;
    
    if (g_papanketik_diinisialisasi) {
        return STATUS_SUDAH_ADA;
    }
    
    kernel_memset(&g_papanketik_konteks, 0, sizeof(papanketik_konteks_t));
    
    /* Inisialisasi keymap */
    for (i = 0; i < PAPANKETIK_KEYMAP_SIZE; i++) {
        g_papanketik_konteks.keymap[i] = g_default_keymap[i];
    }
    for (i = 0; i < PAPANKETIK_EXT_KEYMAP_SIZE; i++) {
        g_papanketik_konteks.ext_keymap[i] = g_default_ext_keymap[i];
    }
    
    /* Inisialisasi devices */
    for (i = 0; i < PAPANKETIK_MAKS_DEVICE; i++) {
        g_papanketik_konteks.devices[i].dev = NULL;
        g_papanketik_konteks.devices[i].modifier = 0;
        g_papanketik_konteks.devices[i].led_state = 0;
        g_papanketik_konteks.devices[i].extended_mode = SALAH;
        g_papanketik_konteks.devices[i].repeat_delay = PAPANKETIK_REPEAT_DELAY;
        g_papanketik_konteks.devices[i].repeat_rate = PAPANKETIK_REPEAT_RATE;
        g_papanketik_konteks.devices[i].aktif = SALAH;
    }
    
    /* Buat device keyboard utama */
    dev = inputdev_register("keyboard0", INPUT_TIPE_KEYBOARD);
    if (dev != NULL) {
        dev->num_keys = 128;
        dev->num_buttons = 0;
        dev->num_axes = 0;
        dev->status = INPUT_STATUS_AKTIF;
        
        pdev = &g_papanketik_konteks.devices[0];
        pdev->dev = dev;
        pdev->aktif = BENAR;
        
        g_papanketik_konteks.device_count = 1;
    }
    
    g_papanketik_konteks.diinisialisasi = BENAR;
    g_papanketik_diinisialisasi = BENAR;
    
    return STATUS_BERHASIL;
}

/*
 * papanketik_event - Proses keyboard event
 */
status_t papanketik_event(tak_bertanda32_t scancode, tak_bertanda32_t status)
{
    (void)status;
    papanketik_device_t *pdev;
    tak_bertanda32_t keycode;
    tak_bertanda32_t key_status;
    tak_bertanda32_t release_bit;
    
    if (!g_papanketik_diinisialisasi) {
        return STATUS_GAGAL;
    }
    
    /* Gunakan device utama */
    pdev = &g_papanketik_konteks.devices[0];
    
    if (!pdev->aktif || pdev->dev == NULL) {
        return STATUS_GAGAL;
    }
    
    /* Cek extended mode (E0 prefix) */
    if (scancode == 0xE0) {
        pdev->extended_mode = BENAR;
        return STATUS_BERHASIL;
    }
    
    /* Cek release bit (bit 7 untuk PS/2) */
    release_bit = scancode & 0x80;
    
    /* Konversi scancode ke keycode */
    keycode = papanketik_scancode_ke_keycode(pdev, scancode);
    
    if (keycode == KEY_TIDAK_ADA) {
        return STATUS_BERHASIL;
    }
    
    /* Tentukan status key */
    if (release_bit) {
        key_status = PAPANKETIK_KEY_LEPAS;
    } else if (pdev->key_states[keycode].ditekan) {
        key_status = PAPANKETIK_KEY_ULANG;
    } else {
        key_status = PAPANKETIK_KEY_TEKAN;
    }
    
    /* Update modifier state */
    papanketik_update_modifier(pdev, keycode, key_status != PAPANKETIK_KEY_LEPAS);
    
    /* Update key state */
    papanketik_update_key_state(pdev, keycode, 
                                 key_status != PAPANKETIK_KEY_LEPAS);
    
    /* Update global modifier state */
    if (pdev->dev != NULL) {
        pdev->dev->key_state = pdev->modifier;
    }
    
    /* Buat event */
    papanketik_buat_event(pdev, keycode, scancode & 0x7F, key_status);
    
    return STATUS_BERHASIL;
}

/*
 * papanketik_set_leds - Set LED keyboard
 */
status_t papanketik_set_leds(tak_bertanda32_t leds)
{
#if defined(PIGURA_ARSITEKTUR_64BIT) || defined(PIGURA_ARSITEKTUR_32BIT)
    tak_bertanda8_t data;
#endif
    
    if (!g_papanketik_diinisialisasi) {
        return STATUS_GAGAL;
    }
    
    /* Update state */
    g_papanketik_konteks.devices[0].led_state = leds;
    
#if defined(PIGURA_ARSITEKTUR_64BIT) || defined(PIGURA_ARSITEKTUR_32BIT)
    /* Kirim ke keyboard controller (x86/x86_64) */
    data = 0;
    if (leds & PAPANKETIK_LED_SCROLL) {
        data |= 0x01;
    }
    if (leds & PAPANKETIK_LED_NUM) {
        data |= 0x02;
    }
    if (leds & PAPANKETIK_LED_CAPS) {
        data |= 0x04;
    }
    
    /* Kirim command ke keyboard controller */
    /* outb(0x60, 0xED); */
    /* outb(0x60, data); */
#endif
    
    return STATUS_BERHASIL;
}

/*
 * papanketik_get_state - Dapatkan state keyboard
 */
tak_bertanda32_t papanketik_get_state(void)
{
    if (!g_papanketik_diinisialisasi) {
        return 0;
    }
    
    return g_papanketik_konteks.devices[0].modifier;
}

/*
 * papanketik_translate - Translate scancode ke unicode
 */
tak_bertanda32_t papanketik_translate(tak_bertanda32_t scancode,
                                       tak_bertanda32_t modifier)
{
    tak_bertanda32_t keycode;
    char ascii;
    
    if (!g_papanketik_diinisialisasi) {
        return 0;
    }
    
    /* Konversi scancode ke keycode */
    if (scancode < PAPANKETIK_KEYMAP_SIZE) {
        keycode = g_papanketik_konteks.keymap[scancode];
    } else {
        keycode = KEY_TIDAK_ADA;
    }
    
    if (keycode == KEY_TIDAK_ADA) {
        return 0;
    }
    
    /* Konversi ke ASCII */
    ascii = papanketik_keycode_ke_ascii(keycode, modifier);
    
    return (tak_bertanda32_t)ascii;
}

/*
 * papanketik_set_repeat - Set key repeat config
 */
status_t papanketik_set_repeat(tak_bertanda32_t delay, tak_bertanda32_t rate)
{
    papanketik_device_t *pdev;
    
    if (!g_papanketik_diinisialisasi) {
        return STATUS_GAGAL;
    }
    
    pdev = &g_papanketik_konteks.devices[0];
    
    pdev->repeat_delay = delay;
    pdev->repeat_rate = rate;
    
    return STATUS_BERHASIL;
}

/*
 * papanketik_set_keymap - Set custom keymap
 */
status_t papanketik_set_keymap(const tak_bertanda32_t *keymap,
                                tak_bertanda32_t size)
{
    tak_bertanda32_t i;
    
    if (!g_papanketik_diinisialisasi) {
        return STATUS_GAGAL;
    }
    
    if (keymap == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    for (i = 0; i < size && i < PAPANKETIK_KEYMAP_SIZE; i++) {
        g_papanketik_konteks.keymap[i] = keymap[i];
    }
    
    return STATUS_BERHASIL;
}

/*
 * papanketik_set_ext_keymap - Set extended keymap
 */
status_t papanketik_set_ext_keymap(const tak_bertanda32_t *keymap,
                                    tak_bertanda32_t size)
{
    tak_bertanda32_t i;
    
    if (!g_papanketik_diinisialisasi) {
        return STATUS_GAGAL;
    }
    
    if (keymap == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    for (i = 0; i < size && i < PAPANKETIK_EXT_KEYMAP_SIZE; i++) {
        g_papanketik_konteks.ext_keymap[i] = keymap[i];
    }
    
    return STATUS_BERHASIL;
}

/*
 * papanketik_key_ditekan - Cek apakah key sedang ditekan
 */
bool_t papanketik_key_ditekan(tak_bertanda32_t keycode)
{
    papanketik_device_t *pdev;
    
    if (!g_papanketik_diinisialisasi) {
        return SALAH;
    }
    
    if (keycode >= INPUT_KEY_MAKS) {
        return SALAH;
    }
    
    pdev = &g_papanketik_konteks.devices[0];
    
    return pdev->key_states[keycode].ditekan;
}

/*
 * papanketik_registrasi_device - Registrasi device keyboard baru
 */
status_t papanketik_registrasi_device(const char *nama)
{
    inputdev_t *dev;
    papanketik_device_t *pdev;
    tak_bertanda32_t slot;
    char buffer[INPUTDEV_NAMA_MAKS];
    ukuran_t len;
    
    if (!g_papanketik_diinisialisasi) {
        return STATUS_GAGAL;
    }
    
    if (nama == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Cari slot kosong */
    slot = papanketik_cari_slot_kosong();
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
    dev = inputdev_register(nama, INPUT_TIPE_KEYBOARD);
    if (dev == NULL) {
        return STATUS_GAGAL;
    }
    
    /* Set kapabilitas */
    dev->num_keys = 128;
    dev->num_buttons = 0;
    dev->num_axes = 0;
    dev->status = INPUT_STATUS_AKTIF;
    
    /* Inisialisasi data keyboard */
    pdev = &g_papanketik_konteks.devices[slot];
    pdev->dev = dev;
    pdev->modifier = 0;
    pdev->led_state = 0;
    pdev->extended_mode = SALAH;
    pdev->repeat_delay = PAPANKETIK_REPEAT_DELAY;
    pdev->repeat_rate = PAPANKETIK_REPEAT_RATE;
    pdev->aktif = BENAR;
    
    g_papanketik_konteks.device_count++;
    
    return STATUS_BERHASIL;
}

/*
 * papanketik_dapat_jumlah_device - Dapatkan jumlah device keyboard
 */
tak_bertanda32_t papanketik_dapat_jumlah_device(void)
{
    if (!g_papanketik_diinisialisasi) {
        return 0;
    }
    
    return g_papanketik_konteks.device_count;
}
