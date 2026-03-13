#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>

/* Konstanta untuk keyboard handling */
#define KEYBOARD_MAX_DEVICES 8
#define KEYBOARD_BUFFER_SIZE 256
#define KEYBOARD_MAX_KEYMAP 256

/* Struktur untuk keymap */
typedef struct {
    char normal;       /* Karakter normal */
    char shifted;      /* Karakter dengan Shift */
    char ctrl;         /* Karakter dengan Ctrl */
    char alt;          /* Karakter dengan Alt */
} KeyMapping;

/* Struktur untuk state keyboard */
typedef struct {
    int shift_pressed;    /* Shift kiri/kanan ditekan */
    int ctrl_pressed;     /* Ctrl kiri/kanan ditekan */
    int alt_pressed;      /* Alt kiri/kanan ditekan */
    int super_pressed;    /* Super/Windows ditekan */
    int caps_lock;        /* Caps Lock aktif */
    int num_lock;         /* Num Lock aktif */
    int scroll_lock;      /* Scroll Lock aktif */
} KeyboardState;

/* Struktur untuk keyboard handler */
typedef struct {
    int fd;                    /* File descriptor keyboard */
    char device_path[256];     /* Path perangkat keyboard */
    KeyMapping keymap[KEYBOARD_MAX_KEYMAP]; /* Keymap */
    KeyboardState state;       /* State keyboard saat ini */
    char buffer[KEYBOARD_BUFFER_SIZE];      /* Buffer output */
    int buffer_pos;            /* Posisi dalam buffer */
    int key_repeat_delay;      /* Delay sebelum repeat (ms) */
    int key_repeat_rate;       /* Rate repeat (ms) */
    struct timeval last_key_time; /* Waktu key terakhir */
    int last_key_code;         /* Kode key terakhir */
    int last_key_value;        /* Nilai key terakhir */
} KeyboardHandler;

/* Keymap default untuk keyboard US */
static KeyMapping default_keymap[KEYBOARD_MAX_KEYMAP] = {
    [KEY_ESC] = {'\033', '\033', '\033', '\033'},
    [KEY_1] = {'1', '!', '1', '1'},
    [KEY_2] = {'2', '@', '2', '2'},
    [KEY_3] = {'3', '#', '3', '3'},
    [KEY_4] = {'4', '$', '4', '4'},
    [KEY_5] = {'5', '%', '5', '5'},
    [KEY_6] = {'6', '^', '6', '6'},
    [KEY_7] = {'7', '&', '7', '7'},
    [KEY_8] = {'8', '*', '8', '8'},
    [KEY_9] = {'9', '(', '9', '9'},
    [KEY_0] = {'0', ')', '0', '0'},
    [KEY_MINUS] = {'-', '_', '-', '-'},
    [KEY_EQUAL] = {'=', '+', '=', '='},
    [KEY_BACKSPACE] = {'\b', '\b', '\b', '\b'},
    [KEY_TAB] = {'\t', '\t', '\t', '\t'},
    [KEY_Q] = {'q', 'Q', '\021', 'q'},
    [KEY_W] = {'w', 'W', '\027', 'w'},
    [KEY_E] = {'e', 'E', '\005', 'e'},
    [KEY_R] = {'r', 'R', '\022', 'r'},
    [KEY_T] = {'t', 'T', '\024', 't'},
    [KEY_Y] = {'y', 'Y', '\031', 'y'},
    [KEY_U] = {'u', 'U', '\025', 'u'},
    [KEY_I] = {'i', 'I', '\011', 'i'},
    [KEY_O] = {'o', 'O', '\017', 'o'},
    [KEY_P] = {'p', 'P', '\020', 'p'},
    [KEY_LEFTBRACE] = {'[', '{', '[', '['},
    [KEY_RIGHTBRACE] = {']', '}', ']', ']'},
    [KEY_ENTER] = {'\n', '\n', '\n', '\n'},
    [KEY_LEFTCTRL] = {0, 0, 0, 0},
    [KEY_RIGHTCTRL] = {0, 0, 0, 0},
    [KEY_A] = {'a', 'A', '\001', 'a'},
    [KEY_S] = {'s', 'S', '\023', 's'},
    [KEY_D] = {'d', 'D', '\004', 'd'},
    [KEY_F] = {'f', 'F', '\006', 'f'},
    [KEY_G] = {'g', 'G', '\007', 'g'},
    [KEY_H] = {'h', 'H', '\010', 'h'},
    [KEY_J] = {'j', 'J', '\012', 'j'},
    [KEY_K] = {'k', 'K', '\013', 'k'},
    [KEY_L] = {'l', 'L', '\014', 'l'},
    [KEY_SEMICOLON] = {';', ':', ';', ';'},
    [KEY_APOSTROPHE] = {'\'', '"', '\'', '\''},
    [KEY_GRAVE] = {'`', '~', '`', '`'},
    [KEY_LEFTSHIFT] = {0, 0, 0, 0},
    [KEY_BACKSLASH] = {'\\', '|', '\\', '\\'},
    [KEY_Z] = {'z', 'Z', '\032', 'z'},
    [KEY_X] = {'x', 'X', '\030', 'x'},
    [KEY_C] = {'c', 'C', '\003', 'c'},
    [KEY_V] = {'v', 'V', '\026', 'v'},
    [KEY_B] = {'b', 'B', '\002', 'b'},
    [KEY_N] = {'n', 'N', '\016', 'n'},
    [KEY_M] = {'m', 'M', '\015', 'm'},
    [KEY_COMMA] = {',', '<', ',', ','},
    [KEY_DOT] = {'.', '>', '.', '.'},
    [KEY_SLASH] = {'/', '?', '/', '/'},
    [KEY_RIGHTSHIFT] = {0, 0, 0, 0},
    [KEY_KPASTERISK] = {'*', '*', '*', '*'},
    [KEY_LEFTALT] = {0, 0, 0, 0},
    [KEY_SPACE] = {' ', ' ', ' ', ' '},
    [KEY_CAPSLOCK] = {0, 0, 0, 0},
    [KEY_F1] = {'\033', '\033', '\033', '\033'},
    [KEY_F2] = {'\033', '\033', '\033', '\033'},
    [KEY_F3] = {'\033', '\033', '\033', '\033'},
    [KEY_F4] = {'\033', '\033', '\033', '\033'},
    [KEY_F5] = {'\033', '\033', '\033', '\033'},
    [KEY_F6] = {'\033', '\033', '\033', '\033'},
    [KEY_F7] = {'\033', '\033', '\033', '\033'},
    [KEY_F8] = {'\033', '\033', '\033', '\033'},
    [KEY_F9] = {'\033', '\033', '\033', '\033'},
    [KEY_F10] = {'\033', '\033', '\033', '\033'},
    [KEY_NUMLOCK] = {0, 0, 0, 0},
    [KEY_SCROLLLOCK] = {0, 0, 0, 0},
    [KEY_KP7] = {'7', '7', '7', '7'},
    [KEY_KP8] = {'8', '8', '8', '8'},
    [KEY_KP9] = {'9', '9', '9', '9'},
    [KEY_KPMINUS] = {'-', '-', '-', '-'},
    [KEY_KP4] = {'4', '4', '4', '4'},
    [KEY_KP5] = {'5', '5', '5', '5'},
    [KEY_KP6] = {'6', '6', '6', '6'},
    [KEY_KPPLUS] = {'+', '+', '+', '+'},
    [KEY_KP1] = {'1', '1', '1', '1'},
    [KEY_KP2] = {'2', '2', '2', '2'},
    [KEY_KP3] = {'3', '3', '3', '3'},
    [KEY_KP0] = {'0', '0', '0', '0'},
    [KEY_KPDOT] = {'.', '.', '.', '.'},
    [KEY_RIGHTALT] = {0, 0, 0, 0},
    [KEY_LEFT] = {'\033', '\033', '\033', '\033'},
    [KEY_RIGHT] = {'\033', '\033', '\033', '\033'},
    [KEY_UP] = {'\033', '\033', '\033', '\033'},
    [KEY_DOWN] = {'\033', '\033', '\033', '\033'},
    [KEY_HOME] = {'\033', '\033', '\033', '\033'},
    [KEY_END] = {'\033', '\033', '\033', '\033'},
    [KEY_PAGEUP] = {'\033', '\033', '\033', '\033'},
    [KEY_PAGEDOWN] = {'\033', '\033', '\033', '\033'},
    [KEY_INSERT] = {'\033', '\033', '\033', '\033'},
    [KEY_DELETE] = {'\033', '\033', '\033', '\033'},
    [KEY_F11] = {'\033', '\033', '\033', '\033'},
    [KEY_F12] = {'\033', '\033', '\033', '\033'},
    [KEY_KPENTER] = {'\n', '\n', '\n', '\n'},
    [KEY_KPSLASH] = {'/', '/', '/', '/'},
};

/* Inisialisasi keyboard handler */
static int keyboard_init(KeyboardHandler *kbd, const char *device_path) {
    /* Salin keymap default */
    memcpy(kbd->keymap, default_keymap, sizeof(default_keymap));
    
    /* Inisialisasi state */
    kbd->state.shift_pressed = 0;
    kbd->state.ctrl_pressed = 0;
    kbd->state.alt_pressed = 0;
    kbd->state.super_pressed = 0;
    kbd->state.caps_lock = 0;
    kbd->state.num_lock = 0;
    kbd->state.scroll_lock = 0;
    
    /* Inisialisasi buffer */
    kbd->buffer_pos = 0;
    kbd->buffer[0] = '\0';
    
    /* Set default key repeat */
    kbd->key_repeat_delay = 500;  /* 500ms */
    kbd->key_repeat_rate = 30;    /* 30ms */
    
    /* Coba buka perangkat keyboard */
    if (device_path) {
        kbd->fd = open(device_path, O_RDONLY | O_NONBLOCK);
        if (kbd->fd >= 0) {
            strncpy(kbd->device_path, device_path, sizeof(kbd->device_path) - 1);
            return 1;
        }
    }
    
    /* Jika device_path tidak diberikan atau gagal, coba deteksi otomatis */
    const char *devices[] = {
        "/dev/input/event0",
        "/dev/input/event1",
        "/dev/input/event2",
        "/dev/input/event3",
        "/dev/input/event4",
        "/dev/input/event5",
        "/dev/input/event6",
        "/dev/input/event7",
        NULL
    };
    
    for (int i = 0; devices[i]; i++) {
        kbd->fd = open(devices[i], O_RDONLY | O_NONBLOCK);
        if (kbd->fd >= 0) {
            strncpy(kbd->device_path, devices[i], sizeof(kbd->device_path) - 1);
            
            /* Verifikasi bahwa ini adalah keyboard */
            int evtype;
            if (ioctl(kbd->fd, EVIOCGBIT(0, sizeof(evtype)), &evtype) >= 0) {
                if (evtype & (1 << EV_KEY)) {
                    return 1;
                }
            }
            
            close(kbd->fd);
            kbd->fd = -1;
        }
    }
    
    return 0;
}

/* Tutup keyboard handler */
static void keyboard_close(KeyboardHandler *kbd) {
    if (kbd->fd >= 0) {
        close(kbd->fd);
        kbd->fd = -1;
    }
}

/* Update state keyboard berdasarkan event */
static void keyboard_update_state(KeyboardHandler *kbd, struct input_event *ev) {
    if (ev->type == EV_KEY) {
        switch (ev->code) {
            case KEY_LEFTSHIFT:
            case KEY_RIGHTSHIFT:
                kbd->state.shift_pressed = (ev->value == 1);
                break;
            case KEY_LEFTCTRL:
            case KEY_RIGHTCTRL:
                kbd->state.ctrl_pressed = (ev->value == 1);
                break;
            case KEY_LEFTALT:
            case KEY_RIGHTALT:
                kbd->state.alt_pressed = (ev->value == 1);
                break;
            case KEY_LEFTMETA:
            case KEY_RIGHTMETA:
                kbd->state.super_pressed = (ev->value == 1);
                break;
            case KEY_CAPSLOCK:
                if (ev->value == 1) {
                    kbd->state.caps_lock = !kbd->state.caps_lock;
                }
                break;
            case KEY_NUMLOCK:
                if (ev->value == 1) {
                    kbd->state.num_lock = !kbd->state.num_lock;
                }
                break;
            case KEY_SCROLLLOCK:
                if (ev->value == 1) {
                    kbd->state.scroll_lock = !kbd->state.scroll_lock;
                }
                break;
        }
    }
}

/* Konversi kode keyboard ke karakter */
static char keyboard_key_to_char(KeyboardHandler *kbd, int code) {
    if (code < 0 || code >= KEYBOARD_MAX_KEYMAP) {
        return 0;
    }
    
    KeyMapping *mapping = &kbd->keymap[code];
    
    /* Tentukan modifier yang aktif */
    if (kbd->state.ctrl_pressed) {
        return mapping->ctrl;
    } else if (kbd->state.alt_pressed) {
        return mapping->alt;
    } else if (kbd->state.shift_pressed) {
        /* Handle Caps Lock */
        if (kbd->state.caps_lock && mapping->normal >= 'a' && mapping->normal <= 'z') {
            return mapping->normal; /* Caps Lock membatalkan Shift untuk huruf */
        }
        return mapping->shifted;
    } else {
        /* Handle Caps Lock */
        if (kbd->state.caps_lock && mapping->normal >= 'a' && mapping->normal <= 'z') {
            return mapping->shifted; /* Caps Lock mengaktifkan Shift untuk huruf */
        }
        return mapping->normal;
    }
}

/* Generate escape sequence untuk tombol khusus */
static void keyboard_generate_escape_sequence(KeyboardHandler *kbd, int code, char *buffer, int *pos) {
    switch (code) {
        case KEY_F1:
            buffer[(*pos)++] = '\033';
            buffer[(*pos)++] = 'O';
            buffer[(*pos)++] = 'P';
            break;
        case KEY_F2:
            buffer[(*pos)++] = '\033';
            buffer[(*pos)++] = 'O';
            buffer[(*pos)++] = 'Q';
            break;
        case KEY_F3:
            buffer[(*pos)++] = '\033';
            buffer[(*pos)++] = 'O';
            buffer[(*pos)++] = 'R';
            break;
        case KEY_F4:
            buffer[(*pos)++] = '\033';
            buffer[(*pos)++] = 'O';
            buffer[(*pos)++] = 'S';
            break;
        case KEY_F5:
            buffer[(*pos)++] = '\033';
            buffer[(*pos)++] = '[';
            buffer[(*pos)++] = '1';
            buffer[(*pos)++] = '5';
            buffer[(*pos)++] = '~';
            break;
        case KEY_F6:
            buffer[(*pos)++] = '\033';
            buffer[(*pos)++] = '[';
            buffer[(*pos)++] = '1';
            buffer[(*pos)++] = '7';
            buffer[(*pos)++] = '~';
            break;
        case KEY_F7:
            buffer[(*pos)++] = '\033';
            buffer[(*pos)++] = '[';
            buffer[(*pos)++] = '1';
            buffer[(*pos)++] = '8';
            buffer[(*pos)++] = '~';
            break;
        case KEY_F8:
            buffer[(*pos)++] = '\033';
            buffer[(*pos)++] = '[';
            buffer[(*pos)++] = '1';
            buffer[(*pos)++] = '9';
            buffer[(*pos)++] = '~';
            break;
        case KEY_F9:
            buffer[(*pos)++] = '\033';
            buffer[(*pos)++] = '[';
            buffer[(*pos)++] = '2';
            buffer[(*pos)++] = '0';
            buffer[(*pos)++] = '~';
            break;
        case KEY_F10:
            buffer[(*pos)++] = '\033';
            buffer[(*pos)++] = '[';
            buffer[(*pos)++] = '2';
            buffer[(*pos)++] = '1';
            buffer[(*pos)++] = '~';
            break;
        case KEY_F11:
            buffer[(*pos)++] = '\033';
            buffer[(*pos)++] = '[';
            buffer[(*pos)++] = '2';
            buffer[(*pos)++] = '3';
            buffer[(*pos)++] = '~';
            break;
        case KEY_F12:
            buffer[(*pos)++] = '\033';
            buffer[(*pos)++] = '[';
            buffer[(*pos)++] = '2';
            buffer[(*pos)++] = '4';
            buffer[(*pos)++] = '~';
            break;
        case KEY_UP:
            buffer[(*pos)++] = '\033';
            buffer[(*pos)++] = '[';
            buffer[(*pos)++] = 'A';
            break;
        case KEY_DOWN:
            buffer[(*pos)++] = '\033';
            buffer[(*pos)++] = '[';
            buffer[(*pos)++] = 'B';
            break;
        case KEY_RIGHT:
            buffer[(*pos)++] = '\033';
            buffer[(*pos)++] = '[';
            buffer[(*pos)++] = 'C';
            break;
        case KEY_LEFT:
            buffer[(*pos)++] = '\033';
            buffer[(*pos)++] = '[';
            buffer[(*pos)++] = 'D';
            break;
        case KEY_HOME:
            buffer[(*pos)++] = '\033';
            buffer[(*pos)++] = '[';
            buffer[(*pos)++] = 'H';
            break;
        case KEY_END:
            buffer[(*pos)++] = '\033';
            buffer[(*pos)++] = '[';
            buffer[(*pos)++] = 'F';
            break;
        case KEY_PAGEUP:
            buffer[(*pos)++] = '\033';
            buffer[(*pos)++] = '[';
            buffer[(*pos)++] = '5';
            buffer[(*pos)++] = '~';
            break;
        case KEY_PAGEDOWN:
            buffer[(*pos)++] = '\033';
            buffer[(*pos)++] = '[';
            buffer[(*pos)++] = '6';
            buffer[(*pos)++] = '~';
            break;
        case KEY_INSERT:
            buffer[(*pos)++] = '\033';
            buffer[(*pos)++] = '[';
            buffer[(*pos)++] = '2';
            buffer[(*pos)++] = '~';
            break;
        case KEY_DELETE:
            buffer[(*pos)++] = '\033';
            buffer[(*pos)++] = '[';
            buffer[(*pos)++] = '3';
            buffer[(*pos)++] = '~';
            break;
    }
}

/* Proses event keyboard */
static void keyboard_process_event(KeyboardHandler *kbd, struct input_event *ev) {
    if (ev->type == EV_KEY) {
        /* Update state keyboard */
        keyboard_update_state(kbd, ev);
        
        /* Handle key press dan release */
        if (ev->value == 1 || ev->value == 2) { /* Press atau repeat */
            /* Cek apakah ini key repeat */
            if (ev->value == 2) {
                /* Key repeat - cek delay dan rate */
                struct timeval now;
                gettimeofday(&now, NULL);
                
                long elapsed = (now.tv_sec - kbd->last_key_time.tv_sec) * 1000 +
                              (now.tv_usec - kbd->last_key_time.tv_usec) / 1000;
                
                if (elapsed < kbd->key_repeat_delay || 
                    (kbd->last_key_code != ev->code && elapsed < kbd->key_repeat_rate)) {
                    return; /* Skip repeat */
                }
            }
            
            /* Update waktu key terakhir */
            gettimeofday(&kbd->last_key_time, NULL);
            kbd->last_key_code = ev->code;
            kbd->last_key_value = ev->value;
            
            /* Konversi ke karakter atau escape sequence */
            char ch = keyboard_key_to_char(kbd, ev->code);
            
            if (ch != 0) {
                /* Tambahkan karakter ke buffer */
                if (kbd->buffer_pos < KEYBOARD_BUFFER_SIZE - 1) {
                    kbd->buffer[kbd->buffer_pos++] = ch;
                    kbd->buffer[kbd->buffer_pos] = '\0';
                }
            } else {
                /* Generate escape sequence untuk tombol khusus */
                keyboard_generate_escape_sequence(kbd, ev->code, kbd->buffer, &kbd->buffer_pos);
                if (kbd->buffer_pos < KEYBOARD_BUFFER_SIZE - 1) {
                    kbd->buffer[kbd->buffer_pos] = '\0';
                }
            }
        }
    }
}

/* Baca input dari keyboard */
static int keyboard_read(KeyboardHandler *kbd, char *buffer, int max_size) {
    struct input_event ev;
    ssize_t n;
    int bytes_read = 0;
    
    /* Salin buffer internal ke output */
    int copy_size = kbd->buffer_pos;
    if (copy_size > max_size - 1) {
        copy_size = max_size - 1;
    }
    
    if (copy_size > 0) {
        memcpy(buffer, kbd->buffer, copy_size);
        buffer[copy_size] = '\0';
        bytes_read = copy_size;
        
        /* Geser sisa buffer */
        kbd->buffer_pos -= copy_size;
        memmove(kbd->buffer, kbd->buffer + copy_size, kbd->buffer_pos);
        kbd->buffer[kbd->buffer_pos] = '\0';
    }
    
    /* Proses event baru */
    while ((n = read(kbd->fd, &ev, sizeof(ev))) == sizeof(ev)) {
        keyboard_process_event(kbd, &ev);
        
        /* Tambahkan data baru ke output jika ada ruang */
        int available = max_size - bytes_read - 1;
        if (available > 0 && kbd->buffer_pos > 0) {
            int add_size = kbd->buffer_pos;
            if (add_size > available) {
                add_size = available;
            }
            
            memcpy(buffer + bytes_read, kbd->buffer, add_size);
            bytes_read += add_size;
            buffer[bytes_read] = '\0';
            
            /* Geser sisa buffer */
            kbd->buffer_pos -= add_size;
            memmove(kbd->buffer, kbd->buffer + add_size, kbd->buffer_pos);
            kbd->buffer[kbd->buffer_pos] = '\0';
        }
    }
    
    return bytes_read;
}

/* Cek apakah ada data tersedia */
static int keyboard_data_available(KeyboardHandler *kbd) {
    if (kbd->buffer_pos > 0) {
        return 1;
    }
    
    fd_set read_fds;
    struct timeval timeout;
    
    FD_ZERO(&read_fds);
    FD_SET(kbd->fd, &read_fds);
    
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    
    return select(kbd->fd + 1, &read_fds, NULL, NULL, &timeout) > 0;
}

/* Set key repeat settings */
static void keyboard_set_repeat(KeyboardHandler *kbd, int delay_ms, int rate_ms) {
    kbd->key_repeat_delay = delay_ms;
    kbd->key_repeat_rate = rate_ms;
}

/* Get keyboard state */
static KeyboardState* keyboard_get_state(KeyboardHandler *kbd) {
    return &kbd->state;
}

/* Set custom keymap */
static void keyboard_set_keymap(KeyboardHandler *kbd, KeyMapping *new_keymap, int size) {
    if (size > KEYBOARD_MAX_KEYMAP) {
        size = KEYBOARD_MAX_KEYMAP;
    }
    memcpy(kbd->keymap, new_keymap, size * sizeof(KeyMapping));
}

/* Get device path */
static const char* keyboard_get_device_path(KeyboardHandler *kbd) {
    return kbd->device_path;
}

#endif /* KEYBOARD_H */
