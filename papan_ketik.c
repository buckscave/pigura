/*******************************************************************************
 * PAPAN_KETIK.C - Implementasi Handler Keyboard
 *
 * Modul ini menangani input dari keyboard melalui evdev.
 *
 * Copyright (c) 2024 Pigura OS Project
 ******************************************************************************/

#include "pigura.h"
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>

/*******************************************************************************
 * KEYMAP US DEFAULT
 ******************************************************************************/

static const PemetaanTombol keymap_us_default[PAPAN_KETIK_KEYMAP_MAKS] = {
    [KEY_ESC]       = {'\033', '\033', '\033', '\033'},
    [KEY_1]         = {'1', '!', '1', '1'},
    [KEY_2]         = {'2', '@', '2', '2'},
    [KEY_3]         = {'3', '#', '3', '3'},
    [KEY_4]         = {'4', '$', '4', '4'},
    [KEY_5]         = {'5', '%', '5', '5'},
    [KEY_6]         = {'6', '^', '6', '6'},
    [KEY_7]         = {'7', '&', '7', '7'},
    [KEY_8]         = {'8', '*', '8', '8'},
    [KEY_9]         = {'9', '(', '9', '9'},
    [KEY_0]         = {'0', ')', '0', '0'},
    [KEY_MINUS]     = {'-', '_', '-', '-'},
    [KEY_EQUAL]     = {'=', '+', '=', '='},
    [KEY_BACKSPACE] = {'\b', '\b', '\b', '\b'},
    [KEY_TAB]       = {'\t', '\t', '\t', '\t'},
    [KEY_Q]         = {'q', 'Q', '\021', 'q'},
    [KEY_W]         = {'w', 'W', '\027', 'w'},
    [KEY_E]         = {'e', 'E', '\005', 'e'},
    [KEY_R]         = {'r', 'R', '\022', 'r'},
    [KEY_T]         = {'t', 'T', '\024', 't'},
    [KEY_Y]         = {'y', 'Y', '\031', 'y'},
    [KEY_U]         = {'u', 'U', '\025', 'u'},
    [KEY_I]         = {'i', 'I', '\011', 'i'},
    [KEY_O]         = {'o', 'O', '\017', 'o'},
    [KEY_P]         = {'p', 'P', '\020', 'p'},
    [KEY_LEFTBRACE] = {'[', '{', '[', '['},
    [KEY_RIGHTBRACE]= {']', '}', ']', ']'},
    [KEY_ENTER]     = {'\n', '\n', '\n', '\n'},
    [KEY_A]         = {'a', 'A', '\001', 'a'},
    [KEY_S]         = {'s', 'S', '\023', 's'},
    [KEY_D]         = {'d', 'D', '\004', 'd'},
    [KEY_F]         = {'f', 'F', '\006', 'f'},
    [KEY_G]         = {'g', 'G', '\007', 'g'},
    [KEY_H]         = {'h', 'H', '\010', 'h'},
    [KEY_J]         = {'j', 'J', '\012', 'j'},
    [KEY_K]         = {'k', 'K', '\013', 'k'},
    [KEY_L]         = {'l', 'L', '\014', 'l'},
    [KEY_SEMICOLON] = {';', ':', ';', ';'},
    [KEY_APOSTROPHE]= {'\'', '"', '\'', '\''},
    [KEY_GRAVE]     = {'`', '~', '`', '`'},
    [KEY_BACKSLASH] = {'\\', '|', '\\', '\\'},
    [KEY_Z]         = {'z', 'Z', '\032', 'z'},
    [KEY_X]         = {'x', 'X', '\030', 'x'},
    [KEY_C]         = {'c', 'C', '\003', 'c'},
    [KEY_V]         = {'v', 'V', '\026', 'v'},
    [KEY_B]         = {'b', 'B', '\002', 'b'},
    [KEY_N]         = {'n', 'N', '\016', 'n'},
    [KEY_M]         = {'m', 'M', '\015', 'm'},
    [KEY_COMMA]     = {',', '<', ',', ','},
    [KEY_DOT]       = {'.', '>', '.', '.'},
    [KEY_SLASH]     = {'/', '?', '/', '/'},
    [KEY_SPACE]     = {' ', ' ', ' ', ' '},
    [KEY_KPASTERISK]= {'*', '*', '*', '*'},
    [KEY_KPMINUS]   = {'-', '-', '-', '-'},
    [KEY_KPPLUS]    = {'+', '+', '+', '+'},
    [KEY_KPENTER]   = {'\n', '\n', '\n', '\n'},
    [KEY_KPSLASH]   = {'/', '/', '/', '/'},
};

/*******************************************************************************
 * FUNGSI IMPLEMENTASI
 ******************************************************************************/

/* Inisialisasi handler keyboard */
int papan_ketik_init(HandlerPapanKetik *kbd, const char *jalur) {
    int i;
    const char *perangkat_default[] = {
        "/dev/input/event0", "/dev/input/event1", "/dev/input/event2",
        "/dev/input/event3", "/dev/input/event4", "/dev/input/event5",
        "/dev/input/event6", "/dev/input/event7", NULL
    };
    
    if (!kbd) return 0;
    
    /* Salin keymap default */
    memcpy(kbd->keymap, keymap_us_default, sizeof(keymap_us_default));
    
    /* Inisialisasi state */
    kbd->state.shift_ditekan = 0;
    kbd->state.ctrl_ditekan = 0;
    kbd->state.alt_ditekan = 0;
    kbd->state.super_ditekan = 0;
    kbd->state.caps_lock = 0;
    kbd->state.num_lock = 0;
    kbd->state.scroll_lock = 0;
    
    /* Inisialisasi buffer */
    kbd->buffer_pos = 0;
    kbd->buffer[0] = '\0';
    
    /* Set default repeat */
    kbd->ulangi_delay = 500;
    kbd->ulangi_rate = 30;
    
    /* Buka perangkat */
    if (jalur) {
        kbd->fd = open(jalur, O_RDONLY | O_NONBLOCK);
        if (kbd->fd >= 0) {
            strncpy(kbd->jalur_perang, jalur, MAKS_JALUR - 1);
            kbd->jalur_perang[MAKS_JALUR - 1] = '\0';
            return 1;
        }
    }
    
    /* Coba deteksi otomatis */
    for (i = 0; perangkat_default[i]; i++) {
        kbd->fd = open(perangkat_default[i], O_RDONLY | O_NONBLOCK);
        if (kbd->fd >= 0) {
            strncpy(kbd->jalur_perang, perangkat_default[i], MAKS_JALUR - 1);
            kbd->jalur_perang[MAKS_JALUR - 1] = '\0';
            return 1;
        }
    }
    
    return 0;
}

/* Tutup handler keyboard */
void papan_ketik_tutup(HandlerPapanKetik *kbd) {
    if (!kbd) return;
    if (kbd->fd >= 0) {
        close(kbd->fd);
        kbd->fd = -1;
    }
}

/* Konversi kode ke karakter */
static char papan_ketik_ke_karakter(HandlerPapanKetik *kbd, int kode) {
    PemetaanTombol *mapping;
    
    if (kode < 0 || kode >= PAPAN_KETIK_KEYMAP_MAKS) return 0;
    
    mapping = &kbd->keymap[kode];
    
    if (kbd->state.ctrl_ditekan) return mapping->ctrl;
    if (kbd->state.alt_ditekan) return mapping->alt;
    if (kbd->state.shift_ditekan) {
        if (kbd->state.caps_lock && mapping->normal >= 'a' && mapping->normal <= 'z') {
            return mapping->normal;
        }
        return mapping->shift;
    }
    
    if (kbd->state.caps_lock && mapping->normal >= 'a' && mapping->normal <= 'z') {
        return mapping->shift;
    }
    return mapping->normal;
}

/* Generate escape sequence untuk tombol khusus */
static void papan_ketik_esc_seq(HandlerPapanKetik *kbd, int kode) {
    /* Tidak digunakan - implementasi sederhana */
    (void)kbd;
    (void)kode;
}

/* Baca input dari keyboard */
int papan_ketik_baca(HandlerPapanKetik *kbd, char *buffer, int ukuran) {
    struct input_event ev;
    ssize_t n;
    int terbaca = 0;
    char ch;
    
    if (!kbd || !buffer || ukuran <= 0 || kbd->fd < 0) return 0;
    
    /* Salin buffer internal */
    if (kbd->buffer_pos > 0) {
        int salin = (kbd->buffer_pos < ukuran - 1) ? kbd->buffer_pos : ukuran - 1;
        memcpy(buffer, kbd->buffer, salin);
        buffer[salin] = '\0';
        terbaca = salin;
        kbd->buffer_pos = 0;
    }
    
    /* Baca event baru */
    while ((n = read(kbd->fd, &ev, sizeof(ev))) == sizeof(ev)) {
        if (ev.type == EV_KEY) {
            /* Update modifier state */
            switch (ev.code) {
                case KEY_LEFTSHIFT: case KEY_RIGHTSHIFT:
                    kbd->state.shift_ditekan = (ev.value >= 1);
                    break;
                case KEY_LEFTCTRL: case KEY_RIGHTCTRL:
                    kbd->state.ctrl_ditekan = (ev.value >= 1);
                    break;
                case KEY_LEFTALT: case KEY_RIGHTALT:
                    kbd->state.alt_ditekan = (ev.value >= 1);
                    break;
                case KEY_CAPSLOCK:
                    if (ev.value == 1) kbd->state.caps_lock = !kbd->state.caps_lock;
                    break;
            }
            
            /* Proses key press */
            if (ev.value >= 1 && terbaca < ukuran - 1) {
                ch = papan_ketik_ke_karakter(kbd, ev.code);
                if (ch != 0) {
                    buffer[terbaca++] = ch;
                } else {
                    papan_ketik_esc_seq(kbd, ev.code);
                }
            }
        }
    }
    
    buffer[terbaca] = '\0';
    return terbaca;
}

/* Cek apakah ada data tersedia */
int papan_ketik_data_tersedia(HandlerPapanKetik *kbd) {
    fd_set read_fds;
    struct timeval timeout;
    
    if (!kbd || kbd->fd < 0) return 0;
    if (kbd->buffer_pos > 0) return 1;
    
    FD_ZERO(&read_fds);
    FD_SET(kbd->fd, &read_fds);
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    
    return select(kbd->fd + 1, &read_fds, NULL, NULL, &timeout) > 0;
}

/* Set pengaturan repeat */
void papan_ketik_set_ulangi(HandlerPapanKetik *kbd, int delay, int rate) {
    if (!kbd) return;
    kbd->ulangi_delay = delay;
    kbd->ulangi_rate = rate;
}

/* Dapatkan state keyboard */
StatePapanKetik* papan_ketik_state(HandlerPapanKetik *kbd) {
    return kbd ? &kbd->state : NULL;
}

/* Set keymap custom */
void papan_ketik_set_keymap(HandlerPapanKetik *kbd, PemetaanTombol *keymap, int ukuran) {
    if (!kbd || !keymap) return;
    if (ukuran > PAPAN_KETIK_KEYMAP_MAKS) ukuran = PAPAN_KETIK_KEYMAP_MAKS;
    memcpy(kbd->keymap, keymap, ukuran * sizeof(PemetaanTombol));
}

/* Dapatkan jalur perangkat */
const char* papan_ketik_jalur(HandlerPapanKetik *kbd) {
    return kbd ? kbd->jalur_perang : NULL;
}
