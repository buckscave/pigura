/*******************************************************************************
 * TETIKUS.C - Implementasi Handler Mouse
 *
 * Modul ini menangani input dari mouse melalui evdev.
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
 * FUNGSI IMPLEMENTASI
 ******************************************************************************/

/* Inisialisasi handler mouse */
int tetikus_init(HandlerTetikus *mouse, const char *jalur) {
    int i;
    const char *perangkat_default[] = {
        "/dev/input/mice", "/dev/input/event0", "/dev/input/event1",
        "/dev/input/event2", "/dev/input/event3", NULL
    };
    
    if (!mouse) return 0;
    
    /* Inisialisasi state */
    mouse->state.x = 0;
    mouse->state.y = 0;
    mouse->state.scroll = 0;
    mouse->state.x_terakhir = 0;
    mouse->state.y_terakhir = 0;
    for (i = 0; i < TETIKUS_TOMBOL_MAKS; i++) {
        mouse->state.tombol[i] = 0;
    }
    
    /* Inisialisasi buffer */
    mouse->buffer_pos = 0;
    mouse->buffer[0] = '\0';
    
    /* Set default mode dan encoding */
    mouse->mode = TETIKUS_MODE_MATI;
    mouse->encoding = TETIKUS_ENCODING_DEFAULT;
    
    /* Set default ukuran sel */
    mouse->sel_lebar = 8;
    mouse->sel_tinggi = 16;
    mouse->layar_lebar = 80;
    mouse->layar_tinggi = 25;
    
    /* Set default grab */
    mouse->grab = 0;
    
    /* Buka perangkat */
    if (jalur) {
        mouse->fd = open(jalur, O_RDONLY | O_NONBLOCK);
        if (mouse->fd >= 0) {
            strncpy(mouse->jalur_perang, jalur, MAKS_JALUR - 1);
            mouse->jalur_perang[MAKS_JALUR - 1] = '\0';
            return 1;
        }
    }
    
    /* Coba deteksi otomatis */
    for (i = 0; perangkat_default[i]; i++) {
        mouse->fd = open(perangkat_default[i], O_RDONLY | O_NONBLOCK);
        if (mouse->fd >= 0) {
            strncpy(mouse->jalur_perang, perangkat_default[i], MAKS_JALUR - 1);
            mouse->jalur_perang[MAKS_JALUR - 1] = '\0';
            return 1;
        }
    }
    
    return 0;
}

/* Tutup handler mouse */
void tetikus_tutup(HandlerTetikus *mouse) {
    if (!mouse) return;
    if (mouse->fd >= 0) {
        if (mouse->grab) {
            ioctl(mouse->fd, EVIOCGRAB, (void *)0);
        }
        close(mouse->fd);
        mouse->fd = -1;
    }
}

/* Set mode tetikus */
void tetikus_set_mode(HandlerTetikus *mouse, int mode) {
    if (mouse) mouse->mode = mode;
}

/* Set encoding tetikus */
void tetikus_set_encoding(HandlerTetikus *mouse, int encoding) {
    if (mouse) mouse->encoding = encoding;
}

/* Set ukuran sel */
void tetikus_set_ukuran_sel(HandlerTetikus *mouse, int lebar, int tinggi) {
    if (mouse) {
        mouse->sel_lebar = lebar;
        mouse->sel_tinggi = tinggi;
    }
}

/* Set ukuran layar */
void tetikus_set_ukuran_layar(HandlerTetikus *mouse, int lebar, int tinggi) {
    if (mouse) {
        mouse->layar_lebar = lebar;
        mouse->layar_tinggi = tinggi;
    }
}

/* Set grab tetikus */
void tetikus_set_grab(HandlerTetikus *mouse, int grab) {
    if (!mouse || mouse->fd < 0) return;
    
    if (grab && !mouse->grab) {
        ioctl(mouse->fd, EVIOCGRAB, (void *)1);
    } else if (!grab && mouse->grab) {
        ioctl(mouse->fd, EVIOCGRAB, (void *)0);
    }
    mouse->grab = grab;
}

/* Konversi piksel ke sel */
static void tetikus_piksel_ke_sel(HandlerTetikus *mouse, int px, int py, 
    int *sx, int *sy) {
    *sx = px / mouse->sel_lebar;
    *sy = py / mouse->sel_tinggi;
    if (*sx < 0) *sx = 0;
    if (*sx >= mouse->layar_lebar) *sx = mouse->layar_lebar - 1;
    if (*sy < 0) *sy = 0;
    if (*sy >= mouse->layar_tinggi) *sy = mouse->layar_tinggi - 1;
}

/* Generate mouse sequence */
static void tetikus_generate_seq(HandlerTetikus *mouse, int tombol, int sx, int sy, int tekan) {
    if (mouse->mode == TETIKUS_MODE_MATI) return;
    
    /* Sederhana - implementasi default encoding */
    mouse->buffer[mouse->buffer_pos++] = '\033';
    mouse->buffer[mouse->buffer_pos++] = '[';
    mouse->buffer[mouse->buffer_pos++] = 'M';
    mouse->buffer[mouse->buffer_pos++] = 32 + tombol + (tekan ? 0 : 64);
    mouse->buffer[mouse->buffer_pos++] = 32 + sx;
    mouse->buffer[mouse->buffer_pos++] = 32 + sy;
    mouse->buffer[mouse->buffer_pos] = '\0';
}

/* Baca input dari mouse */
int tetikus_baca(HandlerTetikus *mouse, char *buffer, int ukuran) {
    struct input_event ev;
    ssize_t n;
    int terbaca = 0;
    int sx, sy;
    int tombol_ditekan;
    
    if (!mouse || !buffer || ukuran <= 0 || mouse->fd < 0) return 0;
    
    /* Salin buffer internal */
    if (mouse->buffer_pos > 0) {
        int salin = (mouse->buffer_pos < ukuran - 1) ? mouse->buffer_pos : ukuran - 1;
        memcpy(buffer, mouse->buffer, salin);
        buffer[salin] = '\0';
        terbaca = salin;
        mouse->buffer_pos = 0;
    }
    
    /* Baca event baru */
    while ((n = read(mouse->fd, &ev, sizeof(ev))) == sizeof(ev)) {
        /* Update state */
        if (ev.type == EV_REL) {
            if (ev.code == REL_X) {
                mouse->state.x += ev.value;
                if (mouse->state.x < 0) mouse->state.x = 0;
            } else if (ev.code == REL_Y) {
                mouse->state.y += ev.value;
                if (mouse->state.y < 0) mouse->state.y = 0;
            } else if (ev.code == REL_WHEEL) {
                mouse->state.scroll += ev.value;
            }
        } else if (ev.type == EV_KEY) {
            if (ev.code >= BTN_LEFT && ev.code <= BTN_BACK) {
                int idx = ev.code - BTN_LEFT;
                if (idx < TETIKUS_TOMBOL_MAKS) {
                    mouse->state.tombol[idx] = ev.value;
                }
            }
        }
        
        /* Generate sequence jika ada perubahan */
        if (ev.type == EV_KEY && ev.code >= BTN_LEFT && ev.code <= BTN_BACK) {
            tetikus_piksel_ke_sel(mouse, mouse->state.x, mouse->state.y, &sx, &sy);
            tetikus_generate_seq(mouse, ev.code - BTN_LEFT + 1, sx, sy, ev.value);
        }
        
        /* Scroll wheel */
        if (ev.type == EV_REL && ev.code == REL_WHEEL) {
            tetikus_piksel_ke_sel(mouse, mouse->state.x, mouse->state.y, &sx, &sy);
            tetikus_generate_seq(mouse, ev.value > 0 ? 4 : 5, sx, sy, 1);
        }
    }
    
    /* Tambahkan data baru ke output */
    if (mouse->buffer_pos > 0 && terbaca < ukuran - 1) {
        int tambah = (mouse->buffer_pos < ukuran - terbaca - 1) ? 
            mouse->buffer_pos : ukuran - terbaca - 1;
        memcpy(buffer + terbaca, mouse->buffer, tambah);
        terbaca += tambah;
        buffer[terbaca] = '\0';
        mouse->buffer_pos = 0;
    }
    
    return terbaca;
}

/* Cek apakah ada data tersedia */
int tetikus_data_tersedia(HandlerTetikus *mouse) {
    fd_set read_fds;
    struct timeval timeout;
    
    if (!mouse || mouse->fd < 0) return 0;
    if (mouse->buffer_pos > 0) return 1;
    
    FD_ZERO(&read_fds);
    FD_SET(mouse->fd, &read_fds);
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    
    return select(mouse->fd + 1, &read_fds, NULL, NULL, &timeout) > 0;
}

/* Dapatkan state tetikus */
StateTetikus* tetikus_state(HandlerTetikus *mouse) {
    return mouse ? &mouse->state : NULL;
}

/* Dapatkan jalur perangkat */
const char* tetikus_jalur(HandlerTetikus *mouse) {
    return mouse ? mouse->jalur_perang : NULL;
}

/* Dapatkan mode tetikus */
int tetikus_mode(HandlerTetikus *mouse) {
    return mouse ? mouse->mode : TETIKUS_MODE_MATI;
}

/* Dapatkan encoding tetikus */
int tetikus_encoding(HandlerTetikus *mouse) {
    return mouse ? mouse->encoding : TETIKUS_ENCODING_DEFAULT;
}
