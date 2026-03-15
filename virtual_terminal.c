/*******************************************************************************
 * VIRTUAL_TERMINAL.C - Implementasi Virtual Terminal Manager
 *
 * Modul ini menangani manajemen virtual terminal (VT) pada Linux.
 *
 * Copyright (c) 2024 Pigura OS Project
 ******************************************************************************/

#include "pigura.h"

/*******************************************************************************
 * KONSTANTA INTERNAL
 ******************************************************************************/

#define VT_JALUR_KONSOL "/dev/tty0"
#define VT_JALUR_FMT    "/dev/tty%d"

/*******************************************************************************
 * FUNGSI INTERNAL
 ******************************************************************************/

/* Handler sinyal VT */
static void vt_handler_sinyal(int sinyal, siginfo_t *info, void *ucontext) {
    
    (void)info;
    (void)ucontext;
    
    /* Dapatkan pointer manager dari context global */
    /* Catatan: Implementasi sederhana, bisa diperbaiki dengan signalfd */
    
    switch (sinyal) {
        case SIGUSR1:
            /* VT release */
            break;
        case SIGUSR2:
            /* VT acquire */
            break;
    }
}

/*******************************************************************************
 * FUNGSI PUBLIK
 ******************************************************************************/

/* Inisialisasi VT manager */
int vt_init(ManagerVT *vtm) {
    int i;
    
    if (!vtm) {
        return 0;
    }
    
    /* Inisialisasi state */
    memset(vtm, 0, sizeof(ManagerVT));
    
    vtm->state.fd = -1;
    vtm->state.aktif = 0;
    vtm->state.asli = 0;
    vtm->state.diminta = 0;
    vtm->state.mode = VT_MODE_GRAFIS;
    vtm->state.mode_simpan = -1;
    vtm->state.mode_kbd_simpan = -1;
    vtm->state.mode_vt_simpan = -1;
    vtm->state.diakuisisi = 0;
    vtm->state.diganti = 0;
    vtm->state.lepas_saat_keluar = 1;
    
    vtm->vt_pertama = 1;
    vtm->vt_terakhir = 63;
    vtm->vt_saat_ini = 0;
    vtm->vt_aktif = 0;
    vtm->jumlah_vt = 0;
    
    for (i = 0; i < 64; i++) {
        vtm->vt_fd[i] = -1;
    }
    
    vtm->callback_ganti = NULL;
    vtm->callback_akuisisi = NULL;
    vtm->callback_lepas = NULL;
    vtm->callback_sinyal = NULL;
    
    /* Buka konsol utama */
    vtm->state.fd = open(VT_JALUR_KONSOL, O_RDWR);
    if (vtm->state.fd < 0) {
        /* Coba alternatif */
        vtm->state.fd = open("/dev/console", O_RDWR);
        if (vtm->state.fd < 0) {
            return 0;
        }
    }
    
    /* Dapatkan VT saat ini menggunakan vt_stat */
    {
        struct vt_stat vt_stat_buf;
        if (ioctl(vtm->state.fd, VT_GETSTATE, &vt_stat_buf) < 0) {
            close(vtm->state.fd);
            vtm->state.fd = -1;
            return 0;
        }
        vtm->state.asli = vt_stat_buf.v_active;
    }
    
    vtm->vt_saat_ini = vtm->state.asli;
    vtm->vt_aktif = vtm->state.asli;
    
    strncpy(vtm->state.jalur_konsol, VT_JALUR_KONSOL, VT_JALUR_MAKS - 1);
    snprintf(vtm->state.nama_vt, VT_NAMA_MAKS, "tty%d", vtm->state.asli);
    
    return 1;
}

/* Tutup VT manager */
void vt_tutup(ManagerVT *vtm) {
    int i;
    
    if (!vtm) {
        return;
    }
    
    /* Lepaskan VT jika masih diakuisisi */
    if (vtm->state.diakuisisi) {
        vt_lepas(vtm);
    }
    
    /* Tutup semua fd VT */
    for (i = 0; i < 64; i++) {
        if (vtm->vt_fd[i] >= 0) {
            close(vtm->vt_fd[i]);
            vtm->vt_fd[i] = -1;
        }
    }
    
    /* Tutup fd konsol utama */
    if (vtm->state.fd >= 0) {
        close(vtm->state.fd);
        vtm->state.fd = -1;
    }
    
    vtm->state.aktif = 0;
    vtm->state.diakuisisi = 0;
}

/* Buka VT tertentu */
int vt_buka(ManagerVT *vtm, int nomor_vt) {
    char jalur[VT_JALUR_MAKS];
    int fd;
    
    if (!vtm || nomor_vt < 1 || nomor_vt > 63) {
        return 0;
    }
    
    /* Cek apakah sudah terbuka */
    if (vtm->vt_fd[nomor_vt] >= 0) {
        return 1;
    }
    
    /* Bentuk jalur device */
    snprintf(jalur, VT_JALUR_MAKS, VT_JALUR_FMT, nomor_vt);
    
    /* Buka device VT */
    fd = open(jalur, O_RDWR);
    if (fd < 0) {
        return 0;
    }
    
    vtm->vt_fd[nomor_vt] = fd;
    vtm->jumlah_vt++;
    
    return 1;
}

/* Switch ke VT tertentu */
int vt_ganti_ke(ManagerVT *vtm, int nomor_vt) {
    if (!vtm || vtm->state.fd < 0) {
        return 0;
    }
    
    if (nomor_vt < 1 || nomor_vt > 63) {
        return 0;
    }
    
    /* Lakukan switch VT */
    if (ioctl(vtm->state.fd, VT_ACTIVATE, nomor_vt) < 0) {
        return 0;
    }
    
    /* Tunggu sampai switch selesai */
    if (ioctl(vtm->state.fd, VT_WAITACTIVE, nomor_vt) < 0) {
        return 0;
    }
    
    vtm->vt_saat_ini = nomor_vt;
    vtm->vt_aktif = nomor_vt;
    vtm->state.aktif = nomor_vt;
    
    if (vtm->callback_ganti) {
        vtm->callback_ganti(nomor_vt, vtm->state.asli);
    }
    
    return 1;
}

/* Akuisisi VT untuk mode grafis */
int vt_akuisisi(ManagerVT *vtm, int nomor_vt) {
    struct vt_mode mode;
    struct sigaction sa;
    int fd;
    
    if (!vtm) {
        return 0;
    }
    
    /* Buka VT jika belum */
    if (!vt_buka(vtm, nomor_vt)) {
        return 0;
    }
    
    fd = vtm->vt_fd[nomor_vt];
    if (fd < 0) {
        return 0;
    }
    
    /* Simpan mode asli */
    if (ioctl(fd, KDGETMODE, &vtm->state.mode_simpan) < 0) {
        vtm->state.mode_simpan = KD_TEXT;
    }
    
    /* Simpan mode keyboard */
    if (ioctl(fd, KDGKBMODE, &vtm->state.mode_kbd_simpan) < 0) {
        vtm->state.mode_kbd_simpan = K_XLATE;
    }
    
    /* Set mode grafis */
    if (ioctl(fd, KDSETMODE, KD_GRAPHICS) < 0) {
        /* Bisa gagal jika bukan VT sebenarnya, abaikan */
    }
    
    /* Set mode keyboard ke raw */
    if (ioctl(fd, KDSKBMODE, K_RAW) < 0) {
        /* Bisa gagal, abaikan */
    }
    
    /* Setup mode VT dengan signal handling */
    memset(&mode, 0, sizeof(mode));
    mode.mode = VT_PROCESS;
    mode.relsig = SIGUSR1;
    mode.acqsig = SIGUSR2;
    mode.frsig = SIGUSR1;
    
    if (ioctl(fd, VT_SETMODE, &mode) < 0) {
        /* Kembalikan mode jika gagal */
        if (vtm->state.mode_simpan >= 0) {
            ioctl(fd, KDSETMODE, vtm->state.mode_simpan);
        }
        return 0;
    }
    
    /* Setup signal handler */
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = vt_handler_sinyal;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    
    vtm->state.diakuisisi = 1;
    vtm->state.aktif = nomor_vt;
    
    if (vtm->callback_akuisisi) {
        vtm->callback_akuisisi(nomor_vt);
    }
    
    return 1;
}

/* Lepaskan VT */
int vt_lepas(ManagerVT *vtm) {
    int fd;
    
    if (!vtm || !vtm->state.diakuisisi) {
        return 0;
    }
    
    fd = vtm->vt_fd[vtm->state.aktif];
    if (fd < 0) {
        fd = vtm->state.fd;
    }
    
    /* Kembalikan mode VT */
    if (vtm->state.mode_simpan >= 0 && fd >= 0) {
        ioctl(fd, KDSETMODE, vtm->state.mode_simpan);
    }
    
    /* Kembalikan mode keyboard */
    if (vtm->state.mode_kbd_simpan >= 0 && fd >= 0) {
        ioctl(fd, KDSKBMODE, vtm->state.mode_kbd_simpan);
    }
    
    /* Kembalikan mode VT ke auto */
    if (fd >= 0) {
        struct vt_mode mode;
        memset(&mode, 0, sizeof(mode));
        mode.mode = VT_AUTO;
        ioctl(fd, VT_SETMODE, &mode);
    }
    
    vtm->state.diakuisisi = 0;
    
    if (vtm->callback_lepas) {
        vtm->callback_lepas(vtm->state.aktif);
    }
    
    return 1;
}

/* Set mode VT */
int vt_set_mode(ManagerVT *vtm, int mode) {
    int fd;
    
    if (!vtm) {
        return 0;
    }
    
    fd = vtm->state.fd;
    if (fd < 0) {
        return 0;
    }
    
    if (mode == VT_MODE_TEKS) {
        if (ioctl(fd, KDSETMODE, KD_TEXT) < 0) {
            return 0;
        }
    } else if (mode == VT_MODE_GRAFIS) {
        if (ioctl(fd, KDSETMODE, KD_GRAPHICS) < 0) {
            return 0;
        }
    }
    
    vtm->state.mode = mode;
    return 1;
}

/* Set mode keyboard */
int vt_set_mode_kbd(ManagerVT *vtm, int mode) {
    int fd;
    
    if (!vtm) {
        return 0;
    }
    
    fd = (vtm->state.aktif > 0 && vtm->vt_fd[vtm->state.aktif] >= 0) ? 
         vtm->vt_fd[vtm->state.aktif] : vtm->state.fd;
    
    if (fd < 0) {
        return 0;
    }
    
    if (ioctl(fd, KDSKBMODE, mode) < 0) {
        return 0;
    }
    
    return 1;
}

/* Dapatkan state VT */
StateVT* vt_state(ManagerVT *vtm) {
    return vtm ? &vtm->state : NULL;
}

/* Dapatkan VT aktif */
int vt_aktif(ManagerVT *vtm) {
    struct vt_stat stat;
    
    if (!vtm || vtm->state.fd < 0) {
        return -1;
    }
    
    if (ioctl(vtm->state.fd, VT_GETSTATE, &stat) < 0) {
        return vtm->vt_aktif;
    }
    
    return stat.v_active;
}

/* Dapatkan VT asli */
int vt_asli(ManagerVT *vtm) {
    return vtm ? vtm->state.asli : -1;
}

/* Dapatkan VT saat ini */
int vt_saat_ini(ManagerVT *vtm) {
    return vtm ? vtm->vt_saat_ini : -1;
}

/* Set callback ganti VT */
void vt_set_callback_ganti(ManagerVT *vtm, void (*callback)(int, int)) {
    if (vtm) {
        vtm->callback_ganti = callback;
    }
}

/* Set callback akuisisi VT */
void vt_set_callback_akuisisi(ManagerVT *vtm, void (*callback)(int)) {
    if (vtm) {
        vtm->callback_akuisisi = callback;
    }
}

/* Set callback lepas VT */
void vt_set_callback_lepas(ManagerVT *vtm, void (*callback)(int)) {
    if (vtm) {
        vtm->callback_lepas = callback;
    }
}

/* Cek apakah VT tersedia */
int vt_tersedia(ManagerVT *vtm, int nomor_vt) {
    char jalur[VT_JALUR_MAKS];
    struct stat st;
    
    (void)vtm;
    
    if (nomor_vt < 1 || nomor_vt > 63) {
        return 0;
    }
    
    snprintf(jalur, VT_JALUR_MAKS, VT_JALUR_FMT, nomor_vt);
    
    return (stat(jalur, &st) == 0);
}

/* Dapatkan daftar VT yang tersedia */
int vt_daftar_tersedia(ManagerVT *vtm, int *daftar, int maks) {
    int i;
    int jumlah = 0;
    
    if (!vtm || !daftar || maks <= 0) {
        return 0;
    }
    
    for (i = vtm->vt_pertama; i <= vtm->vt_terakhir && jumlah < maks; i++) {
        if (vt_tersedia(vtm, i)) {
            daftar[jumlah++] = i;
        }
    }
    
    return jumlah;
}
