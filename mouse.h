#ifndef MOUSE_H
#define MOUSE_H

#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>

/* Konstanta untuk mouse handling */
#define MOUSE_MAX_DEVICES 8
#define MOUSE_BUFFER_SIZE 256
#define MOUSE_MAX_BUTTONS 8

/* Mode mouse */
#define MOUSE_MODE_OFF        0
#define MOUSE_MODE_X10        1
#define MOUSE_MODE_NORMAL     2
#define MOUSE_MODE_BUTTON     3
#define MOUSE_MODE_ANY        4

/* Encoding mouse */
#define MOUSE_ENCODING_DEFAULT 0
#define MOUSE_ENCODING_UTF8    1
#define MOUSE_ENCODING_SGR     2

/* Struktur untuk state mouse */
typedef struct {
    int x;                    /* Posisi X saat ini */
    int y;                    /* Posisi Y saat ini */
    int buttons[MOUSE_MAX_BUTTONS]; /* State tombol */
    int scroll;               /* Nilai scroll */
    int last_x;              /* Posisi X terakhir */
    int last_y;              /* Posisi Y terakhir */
} MouseState;

/* Struktur untuk mouse handler */
typedef struct {
    int fd;                    /* File descriptor mouse */
    char device_path[256];     /* Path perangkat mouse */
    MouseState state;          /* State mouse saat ini */
    char buffer[MOUSE_BUFFER_SIZE];      /* Buffer output */
    int buffer_pos;            /* Posisi dalam buffer */
    int mode;                 /* Mode mouse saat ini */
    int encoding;             /* Encoding mouse saat ini */
    int cell_width;           /* Lebar sel (biasanya 8) */
    int cell_height;          /* Tinggi sel (biasanya 16) */
    int screen_width;         /* Lebar layar dalam sel */
    int screen_height;        /* Tinggi layar dalam sel */
    int grab;                 /* Grab mouse (exclusive access) */
} MouseHandler;

/* Inisialisasi mouse handler */
static int mouse_init(MouseHandler *mouse, const char *device_path) {
    /* Inisialisasi state */
    mouse->state.x = 0;
    mouse->state.y = 0;
    mouse->state.scroll = 0;
    mouse->state.last_x = 0;
    mouse->state.last_y = 0;
    
    /* Reset state tombol */
    for (int i = 0; i < MOUSE_MAX_BUTTONS; i++) {
        mouse->state.buttons[i] = 0;
    }
    
    /* Inisialisasi buffer */
    mouse->buffer_pos = 0;
    mouse->buffer[0] = '\0';
    
    /* Set default mode dan encoding */
    mouse->mode = MOUSE_MODE_OFF;
    mouse->encoding = MOUSE_ENCODING_DEFAULT;
    
    /* Set default ukuran sel */
    mouse->cell_width = 8;
    mouse->cell_height = 16;
    
    /* Set default ukuran layar */
    mouse->screen_width = 80;
    mouse->screen_height = 25;
    
    /* Set default grab */
    mouse->grab = 0;
    
    /* Coba buka perangkat mouse */
    if (device_path) {
        mouse->fd = open(device_path, O_RDONLY | O_NONBLOCK);
        if (mouse->fd >= 0) {
            strncpy(mouse->device_path, device_path, sizeof(mouse->device_path) - 1);
            
            /* Jika grab diaktifkan */
            if (mouse->grab) {
                ioctl(mouse->fd, EVIOCGRAB, (void *)1);
            }
            
            return 1;
        }
    }
    
    /* Jika device_path tidak diberikan atau gagal, coba deteksi otomatis */
    const char *devices[] = {
        "/dev/input/mice",
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
        mouse->fd = open(devices[i], O_RDONLY | O_NONBLOCK);
        if (mouse->fd >= 0) {
            strncpy(mouse->device_path, devices[i], sizeof(mouse->device_path) - 1);
            
            /* Verifikasi bahwa ini adalah mouse */
            int evtype;
            if (ioctl(mouse->fd, EVIOCGBIT(0, sizeof(evtype)), &evtype) >= 0) {
                if (evtype & (1 << EV_KEY) || evtype & (1 << EV_REL)) {
                    /* Jika grab diaktifkan */
                    if (mouse->grab) {
                        ioctl(mouse->fd, EVIOCGRAB, (void *)1);
                    }
                    
                    return 1;
                }
            }
            
            close(mouse->fd);
            mouse->fd = -1;
        }
    }
    
    return 0;
}

/* Tutup mouse handler */
static void mouse_close(MouseHandler *mouse) {
    if (mouse->fd >= 0) {
        /* Jika grab diaktifkan, lepaskan grab */
        if (mouse->grab) {
            ioctl(mouse->fd, EVIOCGRAB, (void *)0);
        }
        
        close(mouse->fd);
        mouse->fd = -1;
    }
}

/* Set mode mouse */
static void mouse_set_mode(MouseHandler *mouse, int mode) {
    mouse->mode = mode;
}

/* Set encoding mouse */
static void mouse_set_encoding(MouseHandler *mouse, int encoding) {
    mouse->encoding = encoding;
}

/* Set ukuran sel */
static void mouse_set_cell_size(MouseHandler *mouse, int width, int height) {
    mouse->cell_width = width;
    mouse->cell_height = height;
}

/* Set ukuran layar */
static void mouse_set_screen_size(MouseHandler *mouse, int width, int height) {
    mouse->screen_width = width;
    mouse->screen_height = height;
}

/* Set grab mouse */
static void mouse_set_grab(MouseHandler *mouse, int grab) {
    if (mouse->fd >= 0) {
        if (grab && !mouse->grab) {
            ioctl(mouse->fd, EVIOCGRAB, (void *)1);
        } else if (!grab && mouse->grab) {
            ioctl(mouse->fd, EVIOCGRAB, (void *)0);
        }
    }
    mouse->grab = grab;
}

/* Konversi koordinat pixel ke sel */
static void mouse_pixel_to_cell(MouseHandler *mouse, int pixel_x, int pixel_y, int *cell_x, int *cell_y) {
    *cell_x = pixel_x / mouse->cell_width;
    *cell_y = pixel_y / mouse->cell_height;
    
    /* Batasi koordinat */
    if (*cell_x < 0) *cell_x = 0;
    if (*cell_x >= mouse->screen_width) *cell_x = mouse->screen_width - 1;
    if (*cell_y < 0) *cell_y = 0;
    if (*cell_y >= mouse->screen_height) *cell_y = mouse->screen_height - 1;
}

/* Generate mouse sequence untuk mode default */
static void mouse_generate_default(MouseHandler *mouse, int button, int cell_x, int cell_y, int press) {
    /* Format: ESC [ M <b> <x+32> <y+32> */
    mouse->buffer[mouse->buffer_pos++] = '\033';
    mouse->buffer[mouse->buffer_pos++] = '[';
    mouse->buffer[mouse->buffer_pos++] = 'M';
    
    /* Button code */
    char b = 32 + button;
    if (!press) b += 64; /* Release */
    mouse->buffer[mouse->buffer_pos++] = b;
    
    /* Koordinat */
    mouse->buffer[mouse->buffer_pos++] = 32 + cell_x;
    mouse->buffer[mouse->buffer_pos++] = 32 + cell_y;
    
    mouse->buffer[mouse->buffer_pos] = '\0';
}

/* Generate mouse sequence untuk mode UTF-8 */
static void mouse_generate_utf8(MouseHandler *mouse, int button, int cell_x, int cell_y, int press) {
    /* Format: ESC [ M <b> <x+32> <y+32> (sama dengan default, tapi koordinat UTF-8) */
    mouse->buffer[mouse->buffer_pos++] = '\033';
    mouse->buffer[mouse->buffer_pos++] = '[';
    mouse->buffer[mouse->buffer_pos++] = 'M';
    
    /* Button code */
    char b = 32 + button;
    if (!press) b += 64; /* Release */
    mouse->buffer[mouse->buffer_pos++] = b;
    
    /* Koordinat UTF-8 */
    if (cell_x + 32 < 128) {
        mouse->buffer[mouse->buffer_pos++] = 32 + cell_x;
    } else {
        /* UTF-8 encoding untuk koordinat > 95 */
        mouse->buffer[mouse->buffer_pos++] = 0xC0 + ((cell_x + 32) >> 6);
        mouse->buffer[mouse->buffer_pos++] = 0x80 + ((cell_x + 32) & 0x3F);
    }
    
    if (cell_y + 32 < 128) {
        mouse->buffer[mouse->buffer_pos++] = 32 + cell_y;
    } else {
        /* UTF-8 encoding untuk koordinat > 95 */
        mouse->buffer[mouse->buffer_pos++] = 0xC0 + ((cell_y + 32) >> 6);
        mouse->buffer[mouse->buffer_pos++] = 0x80 + ((cell_y + 32) & 0x3F);
    }
    
    mouse->buffer[mouse->buffer_pos] = '\0';
}

/* Generate mouse sequence untuk mode SGR */
static void mouse_generate_sgr(MouseHandler *mouse, int button, int cell_x, int cell_y, int press) {
    /* Format: ESC [ <b> ; <x> ; <y> <M/m> */
    mouse->buffer[mouse->buffer_pos++] = '\033';
    mouse->buffer[mouse->buffer_pos++] = '[';
    mouse->buffer[mouse->buffer_pos++] = '<';
    
    /* Button code */
    char b = button;
    if (!press) b -= 4; /* Release */
    
    /* Konversi button ke string */
    char button_str[16];
    sprintf(button_str, "%d", b);
    
    /* Salin button code */
    for (int i = 0; button_str[i]; i++) {
        mouse->buffer[mouse->buffer_pos++] = button_str[i];
    }
    
    /* Separator */
    mouse->buffer[mouse->buffer_pos++] = ';';
    
    /* X coordinate */
    char x_str[16];
    sprintf(x_str, "%d", cell_x + 1);
    
    for (int i = 0; x_str[i]; i++) {
        mouse->buffer[mouse->buffer_pos++] = x_str[i];
    }
    
    /* Separator */
    mouse->buffer[mouse->buffer_pos++] = ';';
    
    /* Y coordinate */
    char y_str[16];
    sprintf(y_str, "%d", cell_y + 1);
    
    for (int i = 0; y_str[i]; i++) {
        mouse->buffer[mouse->buffer_pos++] = y_str[i];
    }
    
    /* Action (M for press, m for release) */
    mouse->buffer[mouse->buffer_pos++] = press ? 'M' : 'm';
    
    mouse->buffer[mouse->buffer_pos] = '\0';
}

/* Generate mouse sequence berdasarkan mode dan encoding */
static void mouse_generate_sequence(MouseHandler *mouse, int button, int cell_x, int cell_y, int press) {
    if (mouse->mode == MOUSE_MODE_OFF) {
        return;
    }
    
    /* Mode X10 hanya melaporkan button press, bukan release atau motion */
    if (mouse->mode == MOUSE_MODE_X10 && !press) {
        return;
    }
    
    /* Mode button hanya melaporkan button press dan release, bukan motion */
    if (mouse->mode == MOUSE_MODE_BUTTON && button == 0) {
        return;
    }
    
    /* Generate sequence berdasarkan encoding */
    switch (mouse->encoding) {
        case MOUSE_ENCODING_UTF8:
            mouse_generate_utf8(mouse, button, cell_x, cell_y, press);
            break;
        case MOUSE_ENCODING_SGR:
            mouse_generate_sgr(mouse, button, cell_x, cell_y, press);
            break;
        default:
            mouse_generate_default(mouse, button, cell_x, cell_y, press);
            break;
    }
}

/* Update state mouse berdasarkan event */
static void mouse_update_state(MouseHandler *mouse, struct input_event *ev) {
    if (ev->type == EV_REL) {
        /* Relative movement */
        if (ev->code == REL_X) {
            mouse->state.x += ev->value;
            if (mouse->state.x < 0) mouse->state.x = 0;
        } else if (ev->code == REL_Y) {
            mouse->state.y += ev->value;
            if (mouse->state.y < 0) mouse->state.y = 0;
        } else if (ev->code == REL_WHEEL) {
            mouse->state.scroll += ev->value;
        }
    } else if (ev->type == EV_ABS) {
        /* Absolute position (touchpad) */
        if (ev->code == ABS_X) {
            mouse->state.x = ev->value;
        } else if (ev->code == ABS_Y) {
            mouse->state.y = ev->value;
        }
    } else if (ev->type == EV_KEY) {
        /* Button events */
        if (ev->code >= BTN_LEFT && ev->code <= BTN_BACK) {
            int button = ev->code - BTN_LEFT;
            if (button < MOUSE_MAX_BUTTONS) {
                mouse->state.buttons[button] = ev->value;
            }
        }
    }
}

/* Proses event mouse */
static void mouse_process_event(MouseHandler *mouse, struct input_event *ev) {
    /* Update state */
    mouse_update_state(mouse, ev);
    
    /* Konversi ke koordinat sel */
    int cell_x, cell_y;
    mouse_pixel_to_cell(mouse, mouse->state.x, mouse->state.y, &cell_x, &cell_y);
    
    /* Handle button events */
    if (ev->type == EV_KEY) {
        if (ev->code >= BTN_LEFT && ev->code <= BTN_BACK) {
            int button = ev->code - BTN_LEFT;
            if (button < MOUSE_MAX_BUTTONS) {
                /* Generate sequence untuk button event */
                mouse_generate_sequence(mouse, button + 1, cell_x, cell_y, ev->value);
            }
        }
    }
    
    /* Handle scroll events */
    if (ev->type == EV_REL && ev->code == REL_WHEEL) {
        /* Scroll wheel */
        int button = (ev->value > 0) ? 4 : 5; /* 4=scroll up, 5=scroll down */
        mouse_generate_sequence(mouse, button, cell_x, cell_y, 1);
    }
    
    /* Handle motion events */
    if (ev->type == EV_REL || ev->type == EV_ABS) {
        /* Check if position changed */
        if (cell_x != mouse->state.last_x || cell_y != mouse->state.last_y) {
            /* Check if any button is pressed */
            int button_pressed = 0;
            for (int i = 0; i < MOUSE_MAX_BUTTONS; i++) {
                if (mouse->state.buttons[i]) {
                    button_pressed = 1;
                    break;
                }
            }
            
            /* Generate motion event if in any mode or button is pressed */
            if (mouse->mode == MOUSE_MODE_ANY || (button_pressed && mouse->mode >= MOUSE_MODE_BUTTON)) {
                mouse_generate_sequence(mouse, 0, cell_x, cell_y, 1);
            }
            
            /* Update last position */
            mouse->state.last_x = cell_x;
            mouse->state.last_y = cell_y;
        }
    }
}

/* Baca input dari mouse */
static int mouse_read(MouseHandler *mouse, char *buffer, int max_size) {
    struct input_event ev;
    ssize_t n;
    int bytes_read = 0;
    
    /* Salin buffer internal ke output */
    int copy_size = mouse->buffer_pos;
    if (copy_size > max_size - 1) {
        copy_size = max_size - 1;
    }
    
    if (copy_size > 0) {
        memcpy(buffer, mouse->buffer, copy_size);
        buffer[copy_size] = '\0';
        bytes_read = copy_size;
        
        /* Geser sisa buffer */
        mouse->buffer_pos -= copy_size;
        memmove(mouse->buffer, mouse->buffer + copy_size, mouse->buffer_pos);
        mouse->buffer[mouse->buffer_pos] = '\0';
    }
    
    /* Proses event baru */
    while ((n = read(mouse->fd, &ev, sizeof(ev))) == sizeof(ev)) {
        mouse_process_event(mouse, &ev);
        
        /* Tambahkan data baru ke output jika ada ruang */
        int available = max_size - bytes_read - 1;
        if (available > 0 && mouse->buffer_pos > 0) {
            int add_size = mouse->buffer_pos;
            if (add_size > available) {
                add_size = available;
            }
            
            memcpy(buffer + bytes_read, mouse->buffer, add_size);
            bytes_read += add_size;
            buffer[bytes_read] = '\0';
            
            /* Geser sisa buffer */
            mouse->buffer_pos -= add_size;
            memmove(mouse->buffer, mouse->buffer + add_size, mouse->buffer_pos);
            mouse->buffer[mouse->buffer_pos] = '\0';
        }
    }
    
    return bytes_read;
}

/* Cek apakah ada data tersedia */
static int mouse_data_available(MouseHandler *mouse) {
    if (mouse->buffer_pos > 0) {
        return 1;
    }
    
    fd_set read_fds;
    struct timeval timeout;
    
    FD_ZERO(&read_fds);
    FD_SET(mouse->fd, &read_fds);
    
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    
    return select(mouse->fd + 1, &read_fds, NULL, NULL, &timeout) > 0;
}

/* Get mouse state */
static MouseState* mouse_get_state(MouseHandler *mouse) {
    return &mouse->state;
}

/* Get device path */
static const char* mouse_get_device_path(MouseHandler *mouse) {
    return mouse->device_path;
}

/* Get mouse mode */
static int mouse_get_mode(MouseHandler *mouse) {
    return mouse->mode;
}

/* Get mouse encoding */
static int mouse_get_encoding(MouseHandler *mouse) {
    return mouse->encoding;
}

#endif /* MOUSE_H */
