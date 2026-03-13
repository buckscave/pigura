#ifndef VT_H
#define VT_H

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/vt.h>
#include <linux/kd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>

/* Konstanta untuk VT handling */
#define VT_MAX_PATH 256
#define VT_MAX_NAME 32
#define VT_MAX_SIGNALS 10

/* Mode VT */
#define VT_MODE_TEXT   0
#define VT_MODE_GRAPHICS 1

/* State VT */
typedef struct {
    int active;           /* VT aktif saat ini */
    int original;         /* VT asli sebelum switch */
    int requested;        /* VT yang diminta */
    int fd;              /* File descriptor konsol */
    int mode;            /* Mode saat ini (teks/grafis) */
    int saved_mode;       /* Mode yang disimpan */
    int saved_kbd_mode;   /* Mode keyboard yang disimpan */
    int saved_vt_mode;    /* Mode VT yang disimpan */
    struct vt_mode saved_vt; /* Struktur mode VT yang disimpan */
    int acquired;        /* Apakah VT sudah diakuisisi */
    int switched;        /* Apakah sudah switch ke VT baru */
    int release_on_exit;  /* Lepaskan VT saat exit */
    char console_path[VT_MAX_PATH]; /* Path ke konsol */
    char vt_name[VT_MAX_NAME];       /* Nama VT */
    struct termios saved_termios;    /* Termios yang disimpan */
} VTState;

/* Struktur untuk VT manager */
typedef struct {
    VTState state;
    int num_vts;          /* Jumlah VT yang tersedia */
    int first_vt;        /* VT pertama yang tersedia */
    int last_vt;         /* VT terakhir yang tersedia */
    int current_vt;       /* VT saat ini */
    int vt_fds[64];       /* File descriptor untuk setiap VT */
    int active_vt;       /* VT yang aktif */
    void (*switch_callback)(int new_vt, int old_vt); /* Callback saat switch VT */
    void (*acquire_callback)(int vt);                 /* Callback saat acquire VT */
    void (*release_callback)(int vt);                 /* Callback saat release VT */
    void (*signal_callback)(int signal);              /* Callback untuk signal VT */
    int signal_handlers[VT_MAX_SIGNALS];            /* Signal handlers */
} VTManager;

/* Fungsi utilitas */
static int vt_is_open(int fd) {
    return fd >= 0;
}

static int vt_get_active_vt(int console_fd) {
    struct vt_stat vtstat;
    if (ioctl(console_fd, VT_GETSTATE, &vtstat) < 0) {
        return -1;
    }
    return vtstat.v_active;
}

static int vt_get_num_vts(int console_fd) {
    struct vt_stat vtstat;
    if (ioctl(console_fd, VT_GETSTATE, &vtstat) < 0) {
        return -1;
    }
    return vtstat.v_state;
}

/* Inisialisasi VT manager */
static int vt_init(VTManager *vtm) {
    int i;
    
    /* Inisialisasi state */
    memset(&vtm->state, 0, sizeof(VTState));
    vtm->state.fd = -1;
    vtm->state.mode = VT_MODE_TEXT;
    vtm->state.saved_mode = VT_MODE_TEXT;
    vtm->state.saved_kbd_mode = -1;
    vtm->state.saved_vt_mode = -1;
    vtm->state.acquired = 0;
    vtm->state.switched = 0;
    vtm->state.release_on_exit = 1;
    
    /* Inisialisasi manager */
    vtm->num_vts = 0;
    vtm->first_vt = 1;
    vtm->last_vt = 63;
    vtm->current_vt = -1;
    vtm->active_vt = -1;
    vtm->switch_callback = NULL;
    vtm->acquire_callback = NULL;
    vtm->release_callback = NULL;
    vtm->signal_callback = NULL;
    
    /* Reset file descriptors */
    for (i = 0; i < 64; i++) {
        vtm->vt_fds[i] = -1;
    }
    
    /* Reset signal handlers */
    for (i = 0; i < VT_MAX_SIGNALS; i++) {
        vtm->signal_handlers[i] = 0;
    }
    
    /* Buka konsol utama */
    vtm->state.fd = open("/dev/console", O_RDWR);
    if (!vt_is_open(vtm->state.fd)) {
        perror("Cannot open /dev/console");
        return 0;
    }
    
    /* Dapatkan VT aktif saat ini */
    vtm->state.active = vt_get_active_vt(vtm->state.fd);
    if (vtm->state.active < 0) {
        perror("Cannot get active VT");
        close(vtm->state.fd);
        vtm->state.fd = -1;
        return 0;
    }
    
    vtm->state.original = vtm->state.active;
    vtm->current_vt = vtm->state.active;
    vtm->active_vt = vtm->state.active;
    
    /* Dapatkan jumlah VT yang tersedia */
    int num_vts = vt_get_num_vts(vtm->state.fd);
    if (num_vts > 0) {
        vtm->num_vts = num_vts;
    }
    
    /* Simpan mode keyboard saat ini */
    if (ioctl(vtm->state.fd, KDGKBMODE, &vtm->state.saved_kbd_mode) < 0) {
        perror("KDGKBMODE failed");
        /* Lanjutkan saja, ini tidak fatal */
    }
    
    /* Simpan mode VT saat ini */
    if (ioctl(vtm->state.fd, VT_GETMODE, &vtm->state.saved_vt) < 0) {
        perror("VT_GETMODE failed");
        /* Lanjutkan saja, ini tidak fatal */
    }
    
    /* Simpan termios saat ini */
    if (tcgetattr(vtm->state.fd, &vtm->state.saved_termios) < 0) {
        perror("tcgetattr failed");
        /* Lanjutkan saja, ini tidak fatal */
    }
    
    return 1;
}

/* Tutup VT manager */
static void vt_close(VTManager *vtm) {
    /* Kembalikan ke VT asli jika perlu */
    if (vtm->state.switched && vtm->state.release_on_exit) {
        vt_switch_to(vtm, vtm->state.original);
    }
    
    /* Kembalikan mode keyboard */
    if (vtm->state.saved_kbd_mode >= 0) {
        ioctl(vtm->state.fd, KDSKBMODE, vtm->state.saved_kbd_mode);
    }
    
    /* Kembalikan mode VT */
    if (vtm->state.saved_vt_mode >= 0) {
        ioctl(vtm->state.fd, VT_SETMODE, &vtm->state.saved_vt);
    }
    
    /* Kembalikan termios */
    if (vtm->state.saved_termios.c_iflag || vtm->state.saved_termios.c_oflag) {
        tcsetattr(vtm->state.fd, TCSANOW, &vtm->state.saved_termios);
    }
    
    /* Tutup semua VT yang terbuka */
    for (int i = 0; i < 64; i++) {
        if (vt_is_open(vtm->vt_fds[i])) {
            close(vtm->vt_fds[i]);
            vtm->vt_fds[i] = -1;
        }
    }
    
    /* Tutup konsol */
    if (vt_is_open(vtm->state.fd)) {
        close(vtm->state.fd);
        vtm->state.fd = -1;
    }
}

/* Buka VT tertentu */
static int vt_open(VTManager *vtm, int vt_num) {
    char vt_path[VT_MAX_PATH];
    int vt_fd;
    
    /* Validasi nomor VT */
    if (vt_num < vtm->first_vt || vt_num > vtm->last_vt) {
        fprintf(stderr, "Invalid VT number: %d\n", vt_num);
        return -1;
    }
    
    /* Jika sudah terbuka, kembalikan file descriptor */
    if (vt_is_open(vtm->vt_fds[vt_num])) {
        return vtm->vt_fds[vt_num];
    }
    
    /* Buat path ke VT */
    snprintf(vt_path, sizeof(vt_path), "/dev/tty%d", vt_num);
    
    /* Buka VT */
    vt_fd = open(vt_path, O_RDWR | O_NOCTTY);
    if (!vt_is_open(vt_fd)) {
        perror("Cannot open VT");
        return -1;
    }
    
    /* Simpan file descriptor */
    vtm->vt_fds[vt_num] = vt_fd;
    
    return vt_fd;
}

/* Switch ke VT tertentu */
static int vt_switch_to(VTManager *vtm, int vt_num) {
    int vt_fd;
    int old_vt = vtm->current_vt;
    
    /* Validasi nomor VT */
    if (vt_num < vtm->first_vt || vt_num > vtm->last_vt) {
        fprintf(stderr, "Invalid VT number: %d\n", vt_num);
        return 0;
    }
    
    /* Buka VT target */
    vt_fd = vt_open(vtm, vt_num);
    if (!vt_is_open(vt_fd)) {
        return 0;
    }
    
    /* Aktifkan VT */
    if (ioctl(vtm->state.fd, VT_ACTIVATE, vt_num) < 0) {
        perror("VT_ACTIVATE failed");
        return 0;
    }
    
    /* Tunggu sampai switch selesai */
    if (ioctl(vtm->state.fd, VT_WAITACTIVE, vt_num) < 0) {
        perror("VT_WAITACTIVE failed");
        return 0;
    }
    
    /* Update state */
    vtm->state.requested = vt_num;
    vtm->current_vt = vt_num;
    vtm->state.switched = 1;
    
    /* Panggil callback jika ada */
    if (vtm->switch_callback) {
        vtm->switch_callback(vt_num, old_vt);
    }
    
    return 1;
}

/* Acquire VT (dapatkan kontrol eksklusif) */
static int vt_acquire(VTManager *vtm, int vt_num) {
    int vt_fd;
    
    /* Validasi nomor VT */
    if (vt_num < vtm->first_vt || vt_num > vtm->last_vt) {
        fprintf(stderr, "Invalid VT number: %d\n", vt_num);
        return 0;
    }
    
    /* Buka VT target */
    vt_fd = vt_open(vtm, vt_num);
    if (!vt_is_open(vt_fd)) {
        return 0;
    }
    
    /* Jadikan VT sebagai controlling terminal */
    if (ioctl(vt_fd, TIOCSCTTY, 0) < 0) {
        perror("TIOCSCTTY failed");
        return 0;
    }
    
    /* Update state */
    vtm->state.acquired = 1;
    vtm->state.requested = vt_num;
    snprintf(vtm->state.vt_name, sizeof(vtm->state.vt_name), "/dev/tty%d", vt_num);
    snprintf(vtm->state.console_path, sizeof(vtm->state.console_path), "/dev/tty%d", vt_num);
    
    /* Panggil callback jika ada */
    if (vtm->acquire_callback) {
        vtm->acquire_callback(vt_num);
    }
    
    return 1;
}

/* Release VT (lepaskan kontrol) */
static int vt_release(VTManager *vtm) {
    if (!vtm->state.acquired) {
        return 1; /* Sudah dilepaskan */
    }
    
    /* Kembalikan ke VT asli */
    if (vtm->state.release_on_exit) {
        vt_switch_to(vtm, vtm->state.original);
    }
    
    /* Update state */
    vtm->state.acquired = 0;
    
    /* Panggil callback jika ada */
    if (vtm->release_callback) {
        vtm->release_callback(vtm->state.requested);
    }
    
    return 1;
}

/* Set mode VT (teks/grafis) */
static int vt_set_mode(VTManager *vtm, int mode) {
    int vt_fd;
    
    /* Dapatkan VT saat ini */
    vt_fd = vt_open(vtm, vtm->current_vt);
    if (!vt_is_open(vt_fd)) {
        return 0;
    }
    
    /* Set mode */
    if (mode == VT_MODE_GRAPHICS) {
        if (ioctl(vt_fd, KDSETMODE, KD_GRAPHICS) < 0) {
            perror("KDSETMODE GRAPHICS failed");
            return 0;
        }
    } else {
        if (ioctl(vt_fd, KDSETMODE, KD_TEXT) < 0) {
            perror("KDSETMODE TEXT failed");
            return 0;
        }
    }
    
    /* Update state */
    vtm->state.mode = mode;
    
    return 1;
}

/* Set mode keyboard */
static int vt_set_keyboard_mode(VTManager *vtm, int mode) {
    int vt_fd;
    
    /* Dapatkan VT saat ini */
    vt_fd = vt_open(vtm, vtm->current_vt);
    if (!vt_is_open(vt_fd)) {
        return 0;
    }
    
    /* Set mode keyboard */
    if (ioctl(vt_fd, KDSKBMODE, mode) < 0) {
        perror("KDSKBMODE failed");
        return 0;
    }
    
    return 1;
}

/* Get state VT saat ini */
static VTState* vt_get_state(VTManager *vtm) {
    return &vtm->state;
}

/* Get VT aktif saat ini */
static int vt_get_active(VTManager *vtm) {
    return vtm->state.active;
}

/* Get VT asli */
static int vt_get_original(VTManager *vtm) {
    return vtm->state.original;
}

/* Get VT saat ini */
static int vt_get_current(VTManager *vtm) {
    return vtm->current_vt;
}

/* Set callback untuk switch VT */
static void vt_set_switch_callback(VTManager *vtm, void (*callback)(int, int)) {
    vtm->switch_callback = callback;
}

/* Set callback untuk acquire VT */
static void vt_set_acquire_callback(VTManager *vtm, void (*callback)(int)) {
    vtm->acquire_callback = callback;
}

/* Set callback untuk release VT */
static void vt_set_release_callback(VTManager *vtm, void (*callback)(int)) {
    vtm->release_callback = callback;
}

/* Set callback untuk signal VT */
static void vt_set_signal_callback(VTManager *vtm, void (*callback)(int)) {
    vtm->signal_callback = callback;
}

/* Set apakah melepas VT saat exit */
static void vt_set_release_on_exit(VTManager *vtm, int release) {
    vtm->state.release_on_exit = release;
}

/* Handle signal VT */
static void vt_handle_signal(int signum) {
    /* Ini akan diimplementasikan oleh pengguna */
}

/* Register signal handler untuk VT */
static int vt_register_signal(VTManager *vtm, int signal, void (*handler)(int)) {
    struct sigaction sa;
    
    /* Validasi signal */
    if (signal < 1 || signal >= VT_MAX_SIGNALS) {
        return 0;
    }
    
    /* Setup signal handler */
    sa.sa_handler = handler ? handler : vt_handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    
    if (sigaction(signal, &sa, NULL) < 0) {
        perror("sigaction failed");
        return 0;
    }
    
    /* Simpan handler */
    vtm->signal_handlers[signal] = (int)handler;
    
    return 1;
}

/* Cek apakah VT tersedia */
static int vt_is_available(VTManager *vtm, int vt_num) {
    char vt_path[VT_MAX_PATH];
    struct stat st;
    
    /* Validasi nomor VT */
    if (vt_num < vtm->first_vt || vt_num > vtm->last_vt) {
        return 0;
    }
    
    /* Buat path ke VT */
    snprintf(vt_path, sizeof(vt_path), "/dev/tty%d", vt_num);
    
    /* Cek apakah file ada */
    if (stat(vt_path, &st) < 0) {
        return 0;
    }
    
    /* Cek apakah ini adalah character device */
    if (!S_ISCHR(st.st_mode)) {
        return 0;
    }
    
    return 1;
}

/* Dapatkan daftar VT yang tersedia */
static int vt_get_available_vts(VTManager *vtm, int *vts, int max_vts) {
    int count = 0;
    
    for (int i = vtm->first_vt; i <= vtm->last_vt && count < max_vts; i++) {
        if (vt_is_available(vtm, i)) {
            vts[count++] = i;
        }
    }
    
    return count;
}

/* Dapatkan informasi VT */
static int vt_get_info(VTManager *vtm, int vt_num, struct vt_stat *info) {
    int vt_fd;
    
    /* Validasi nomor VT */
    if (vt_num < vtm->first_vt || vt_num > vtm->last_vt) {
        return 0;
    }
    
    /* Buka VT */
    vt_fd = vt_open(vtm, vt_num);
    if (!vt_is_open(vt_fd)) {
        return 0;
    }
    
    /* Dapatkan informasi */
    if (ioctl(vt_fd, VT_GETSTATE, info) < 0) {
        perror("VT_GETSTATE failed");
        return 0;
    }
    
    return 1;
}

/* Set VT mode */
static int vt_set_vt_mode(VTManager *vtm, int vt_num, struct vt_mode *mode) {
    int vt_fd;
    
    /* Validasi nomor VT */
    if (vt_num < vtm->first_vt || vt_num > vtm->last_vt) {
        return 0;
    }
    
    /* Buka VT */
    vt_fd = vt_open(vtm, vt_num);
    if (!vt_is_open(vt_fd)) {
        return 0;
    }
    
    /* Set mode */
    if (ioctl(vt_fd, VT_SETMODE, mode) < 0) {
        perror("VT_SETMODE failed");
        return 0;
    }
    
    return 1;
}

/* Get VT mode */
static int vt_get_vt_mode(VTManager *vtm, int vt_num, struct vt_mode *mode) {
    int vt_fd;
    
    /* Validasi nomor VT */
    if (vt_num < vtm->first_vt || vt_num > vtm->last_vt) {
        return 0;
    }
    
    /* Buka VT */
    vt_fd = vt_open(vtm, vt_num);
    if (!vt_is_open(vt_fd)) {
        return 0;
    }
    
    /* Dapatkan mode */
    if (ioctl(vt_fd, VT_GETMODE, mode) < 0) {
        perror("VT_GETMODE failed");
        return 0;
    }
    
    return 1;
}

/* Set VT process */
static int vt_set_process(VTManager *vtm, int vt_num, pid_t pid) {
    int vt_fd;
    
    /* Validasi nomor VT */
    if (vt_num < vtm->first_vt || vt_num > vtm->last_vt) {
        return 0;
    }
    
    /* Buka VT */
    vt_fd = vt_open(vtm, vt_num);
    if (!vt_is_open(vt_fd)) {
        return 0;
    }
    
    /* Set process */
    if (ioctl(vt_fd, VT_SET_PROCESS, pid) < 0) {
        perror("VT_SET_PROCESS failed");
        return 0;
    }
    
    return 1;
}

/* Get VT process */
static pid_t vt_get_process(VTManager *vtm, int vt_num) {
    int vt_fd;
    pid_t pid;
    
    /* Validasi nomor VT */
    if (vt_num < vtm->first_vt || vt_num > vtm->last_vt) {
        return -1;
    }
    
    /* Buka VT */
    vt_fd = vt_open(vtm, vt_num);
    if (!vt_is_open(vt_fd)) {
        return -1;
    }
    
    /* Dapatkan process */
    if (ioctl(vt_fd, VT_GET_PROCESS, &pid) < 0) {
        perror("VT_GET_PROCESS failed");
        return -1;
    }
    
    return pid;
}

#endif /* VT_H */
