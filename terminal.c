/*******************************************************************************
 * TERMINAL.C - Implementasi Aplikasi Terminal Utama
 *
 * Modul ini mengintegrasikan semua komponen untuk aplikasi terminal.
 *
 * Copyright (c) 2024 Pigura OS Project
 ******************************************************************************/

#define _POSIX_C_SOURCE 200809L

#include "pigura.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/time.h>

/*******************************************************************************
 * KONSTANTA INTERNAL
 ******************************************************************************/

#define TERMINAL_SELECT_TIMEOUT  50000  /* 50ms dalam mikrosekon */
#define TERMINAL_KURSOR_BLINK_MS 500    /* Interval kedip kursor */

/*******************************************************************************
 * FUNGSI CALLBACK INTERNAL
 ******************************************************************************/

/* Callback untuk output shell */
static void terminal_callback_output(const char *data, int ukuran, 
                                     void *user_data) {
    AplikasiTerminal *app = (AplikasiTerminal *)user_data;
    
    if (!app || !data || ukuran <= 0) {
        return;
    }
    
    /* Proses output melalui parser ANSI */
    ansi_proses_string(&app->term, &app->parser, data);
}

/* Callback untuk shell keluar */
static void terminal_callback_keluar(int status, void *user_data) {
    AplikasiTerminal *app = (AplikasiTerminal *)user_data;
    
    if (!app) {
        return;
    }
    
    app->berjalan = 0;
    app->cfg.status_keluar = status;
    
    if (app->callback_keluar) {
        app->callback_keluar(status);
    }
}

/*******************************************************************************
 * FUNGSI INTERNAL
 ******************************************************************************/

/* Load konfigurasi terminal */
static int terminal_muat_konfig(AplikasiTerminal *app, const char *file_konfig) {
    ManagerKonfig *km = &app->konfig;
    
    if (!konfig_init(km, file_konfig)) {
        return 0;
    }
    
    /* Muat file jika ada */
    konfig_muat(km, file_konfig);
    
    /* Baca konfigurasi */
    app->cfg.lebar = konfig_int(km, "tampilan", "lebar", 800);
    app->cfg.tinggi = konfig_int(km, "tampilan", "tinggi", 600);
    app->cfg.baris = konfig_int(km, "terminal", "baris", TERMINAL_BARIS_DEFAULT);
    app->cfg.kolom = konfig_int(km, "terminal", "kolom", TERMINAL_KOLOM_DEFAULT);
    app->cfg.ukuran_font = konfig_int(km, "terminal", "ukuran_font", 16);
    app->cfg.pakai_unicode = konfig_bool(km, "terminal", "unicode", 1);
    app->cfg.pakai_tetikus = konfig_bool(km, "terminal", "tetikus", 1);
    app->cfg.baris_scrollback = konfig_int(km, "terminal", "scrollback", 1000);
    app->cfg.auto_simpan_konfig = konfig_bool(km, "terminal", "auto_simpan", 1);
    app->cfg.transparansi = konfig_int(km, "tampilan", "transparansi", 0);
    app->cfg.layar_penuh = konfig_bool(km, "tampilan", "layar_penuh", 0);
    app->cfg.bell_hidup = konfig_bool(km, "terminal", "bell_hidup", 1);
    app->cfg.bell_visual = konfig_bool(km, "terminal", "bell_visual", 0);
    app->cfg.kursor_berkedip = konfig_bool(km, "terminal", "kursor_berkedip", 1);
    app->cfg.tampilkan_scrollbar = konfig_bool(km, "tampilan", "scrollbar", 0);
    app->cfg.word_wrap = konfig_bool(km, "terminal", "word_wrap", 1);
    
    /* Baca jalur shell */
    {
        const char *shell = konfig_string(km, "terminal", "shell", NULL);
        if (shell) {
            strncpy(app->cfg.jalur_shell, shell, 255);
            app->cfg.jalur_shell[255] = '\0';
        } else {
            char *env_shell = getenv("SHELL");
            if (env_shell) {
                strncpy(app->cfg.jalur_shell, env_shell, 255);
            } else {
                strcpy(app->cfg.jalur_shell, "/bin/sh");
            }
            app->cfg.jalur_shell[255] = '\0';
        }
    }
    
    /* Baca tema warna */
    konfig_warna(km, "tema", "latar", &app->cfg.tema.latar);
    app->cfg.tema.latar.r = konfig_int(km, "tema", "latar_r", 255);
    app->cfg.tema.latar.g = konfig_int(km, "tema", "latar_g", 255);
    app->cfg.tema.latar.b = konfig_int(km, "tema", "latar_b", 255);
    
    konfig_warna(km, "tema", "belakang", &app->cfg.tema.belakang);
    app->cfg.tema.belakang.r = konfig_int(km, "tema", "belakang_r", 0);
    app->cfg.tema.belakang.g = konfig_int(km, "tema", "belakang_g", 0);
    app->cfg.tema.belakang.b = konfig_int(km, "tema", "belakang_b", 0);
    
    app->cfg.tema.kursor.r = konfig_int(km, "tema", "kursor_r", 255);
    app->cfg.tema.kursor.g = konfig_int(km, "tema", "kursor_g", 255);
    app->cfg.tema.kursor.b = konfig_int(km, "tema", "kursor_b", 255);
    
    return 1;
}

/* Inisialisasi komponen terminal */
static int terminal_init_komponen(AplikasiTerminal *app) {
    /* Inisialisasi font */
    if (!huruf_init()) {
        return 0;
    }
    
    /* Inisialisasi buffer terminal */
    if (!ansi_init_terminal(&app->term, app->cfg.baris, app->cfg.kolom)) {
        return 0;
    }
    
    /* Inisialisasi parser ANSI */
    ansi_init_parser(&app->parser);
    
    /* Inisialisasi keyboard */
    if (!papan_ketik_init(&app->kbd, NULL)) {
        /* Tidak fatal, bisa jalan tanpa keyboard evdev */
    }
    
    /* Inisialisasi tetikus */
    if (app->cfg.pakai_tetikus) {
        if (!tetikus_init(&app->tetikus, NULL)) {
            /* Tidak fatal */
        }
        tetikus_set_ukuran_sel(&app->tetikus, HURUF_LEBAR, HURUF_TINGGI);
        tetikus_set_ukuran_layar(&app->tetikus, app->cfg.kolom, app->cfg.baris);
    }
    
    /* Inisialisasi clipboard */
    if (!klip_init(&app->klip, KLIP_UKURAN_DEFAULT, KLIP_UKURAN_MAKS)) {
        /* Tidak fatal */
    }
    
    /* Inisialisasi VT manager */
    if (!vt_init(&app->vt)) {
        /* Tidak fatal, mungkin bukan di VT sebenarnya */
    }
    
    /* Inisialisasi shell */
    if (!cangkang_init(&app->cangkang, app->cfg.jalur_shell)) {
        return 0;
    }
    
    cangkang_set_ukuran(&app->cangkang, app->cfg.baris, app->cfg.kolom);
    cangkang_set_callback_output(&app->cangkang, terminal_callback_output, app);
    cangkang_set_callback_keluar(&app->cangkang, terminal_callback_keluar, app);
    
    /* Set warna default terminal */
    app->term.attr.rgb_latar = app->cfg.tema.latar;
    app->term.attr.rgb_blkg = app->cfg.tema.belakang;
    app->term.attr_def.rgb_latar = app->cfg.tema.latar;
    app->term.attr_def.rgb_blkg = app->cfg.tema.belakang;
    
    return 1;
}

/* Render terminal ke framebuffer */
static void terminal_render(AplikasiTerminal *app) {
    WarnaRGB latar, belakang;
    int x, y;
    char ch;
    int kursor_x, kursor_y;
    
    /* Bersihkan layar dengan warna background */
    fb_bersihkan_warna(&app->fb, app->cfg.tema.belakang);
    
    /* Render isi terminal */
    if (app->cfg.pakai_unicode) {
        ansi_render(&app->term, &app->fb, unikode_gambar_karakter);
    } else {
        ansi_render(&app->term, &app->fb, 
            (void (*)(InfoFramebuffer*, int, int, unsigned int, WarnaRGB, WarnaRGB))
            huruf_gambar_karakter);
    }
    
    /* Gambar kursor */
    if (app->term.kursor_terlihat && app->kursor_terlihat) {
        kursor_x = app->term.kursor_kolom * HURUF_LEBAR;
        kursor_y = app->term.kursor_baris * HURUF_TINGGI;
        
        /* Gambar kotak kursor */
        fb_gambar_kotak(&app->fb, kursor_x, kursor_y, 
                        HURUF_LEBAR, HURUF_TINGGI,
                        app->cfg.tema.kursor, 1);
    }
}

/* Proses input keyboard */
static void terminal_proses_keyboard(AplikasiTerminal *app) {
    char buffer[256];
    int n;
    
    if (app->kbd.fd < 0) {
        return;
    }
    
    n = papan_ketik_baca(&app->kbd, buffer, sizeof(buffer));
    if (n > 0) {
        cangkang_tulis(&app->cangkang, buffer, n);
    }
}

/* Proses input tetikus */
static void terminal_proses_tetikus(AplikasiTerminal *app) {
    char buffer[256];
    int n;
    
    if (app->tetikus.fd < 0 || app->term.mode_tetikus == 0) {
        return;
    }
    
    n = tetikus_baca(&app->tetikus, buffer, sizeof(buffer));
    if (n > 0) {
        cangkang_tulis(&app->cangkang, buffer, n);
    }
}

/* Update kursor berkedip */
static void terminal_update_kursor(AplikasiTerminal *app) {
    struct timeval sekarang;
    unsigned long selisih_ms;
    
    gettimeofday(&sekarang, NULL);
    
    selisih_ms = (sekarang.tv_sec - app->waktu_kedip_terakhir.tv_sec) * 1000 +
                 (sekarang.tv_usec - app->waktu_kedip_terakhir.tv_usec) / 1000;
    
    if (selisih_ms >= TERMINAL_KURSOR_BLINK_MS) {
        app->kursor_kedip_state = !app->kursor_kedip_state;
        app->waktu_kedip_terakhir = sekarang;
    }
    
    app->kursor_terlihat = app->kursor_kedip_state || !app->cfg.kursor_berkedip;
}

/* Update status bar */
static void terminal_update_status(AplikasiTerminal *app) {
    snprintf(app->status_bar.teks_kiri, 64, "Pigura Terminal - %s",
             app->cfg.jalur_shell);
    snprintf(app->status_bar.teks_kanan, 64, "FPS: %d", app->fps);
}

/*******************************************************************************
 * FUNGSI PUBLIK
 ******************************************************************************/

/* Inisialisasi aplikasi terminal */
int terminal_init(AplikasiTerminal *app, const char *file_konfig) {
    if (!app) {
        return 0;
    }
    
    /* Inisialisasi struktur */
    memset(app, 0, sizeof(AplikasiTerminal));
    
    /* Load konfigurasi */
    if (!terminal_muat_konfig(app, file_konfig ? file_konfig : 
                              TERMINAL_FILE_KONFIG)) {
        /* Gunakan default */
        app->cfg.lebar = 800;
        app->cfg.tinggi = 600;
        app->cfg.baris = TERMINAL_BARIS_DEFAULT;
        app->cfg.kolom = TERMINAL_KOLOM_DEFAULT;
        strcpy(app->cfg.jalur_shell, "/bin/sh");
        app->cfg.tema.latar.r = 255;
        app->cfg.tema.latar.g = 255;
        app->cfg.tema.latar.b = 255;
        app->cfg.tema.belakang.r = 0;
        app->cfg.tema.belakang.g = 0;
        app->cfg.tema.belakang.b = 0;
        app->cfg.tema.kursor.r = 255;
        app->cfg.tema.kursor.g = 255;
        app->cfg.tema.kursor.b = 255;
    }
    
    /* Inisialisasi framebuffer */
    if (!fb_init(NULL, &app->fb)) {
        return 0;
    }
    
    /* Hitung ukuran terminal berdasarkan framebuffer */
    if (app->cfg.layar_penuh) {
        app->cfg.kolom = app->fb.lebar / HURUF_LEBAR;
        app->cfg.baris = app->fb.tinggi / HURUF_TINGGI;
    }
    
    /* Inisialisasi komponen */
    if (!terminal_init_komponen(app)) {
        fb_tutup(&app->fb);
        return 0;
    }
    
    /* Inisialisasi state */
    app->berjalan = 0;
    app->mode = TERMINAL_MODE_NORMAL;
    app->fokus = 1;
    app->frame_hitung = 0;
    app->fps = 0;
    app->kursor_terlihat = 1;
    app->kursor_kedip_state = 1;
    gettimeofday(&app->waktu_frame_terakhir, NULL);
    gettimeofday(&app->waktu_kedip_terakhir, NULL);
    
    /* Inisialisasi status bar */
    app->status_bar.terlihat = 1;
    app->status_bar.latar = app->cfg.tema.latar;
    app->status_bar.belakang = app->cfg.tema.belakang;
    
    return 1;
}

/* Jalankan aplikasi terminal */
void terminal_jalankan(AplikasiTerminal *app) {
    fd_set read_fds;
    struct timeval timeout;
    int max_fd;
    int n;
    char buffer[4096];
    
    if (!app) {
        return;
    }
    
    /* Jalankan shell */
    if (!cangkang_jalankan(&app->cangkang)) {
        return;
    }
    
    /* Akuisisi VT untuk mode grafis */
    if (app->vt.state.fd >= 0) {
        vt_akuisisi(&app->vt, app->vt.state.asli);
    }
    
    app->berjalan = 1;
    
    /* Main loop */
    while (app->berjalan) {
        /* Setup select */
        FD_ZERO(&read_fds);
        max_fd = -1;
        
        /* Monitor shell output */
        if (app->cangkang.fd_master >= 0) {
            FD_SET(app->cangkang.fd_master, &read_fds);
            if (app->cangkang.fd_master > max_fd) {
                max_fd = app->cangkang.fd_master;
            }
        }
        
        /* Monitor keyboard */
        if (app->kbd.fd >= 0) {
            FD_SET(app->kbd.fd, &read_fds);
            if (app->kbd.fd > max_fd) {
                max_fd = app->kbd.fd;
            }
        }
        
        /* Monitor tetikus */
        if (app->tetikus.fd >= 0 && app->term.mode_tetikus > 0) {
            FD_SET(app->tetikus.fd, &read_fds);
            if (app->tetikus.fd > max_fd) {
                max_fd = app->tetikus.fd;
            }
        }
        
        /* Timeout untuk select */
        timeout.tv_sec = 0;
        timeout.tv_usec = TERMINAL_SELECT_TIMEOUT;
        
        /* Select */
        n = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);
        
        if (n < 0) {
            /* Error atau signal */
            if (!cangkang_berjalan(&app->cangkang)) {
                app->berjalan = 0;
                break;
            }
            continue;
        }
        
        /* Proses output shell */
        if (app->cangkang.fd_master >= 0 && 
            FD_ISSET(app->cangkang.fd_master, &read_fds)) {
            n = cangkang_baca(&app->cangkang, buffer, sizeof(buffer));
            if (n <= 0) {
                /* Shell keluar */
                app->berjalan = 0;
                break;
            }
        }
        
        /* Proses keyboard */
        if (app->kbd.fd >= 0 && FD_ISSET(app->kbd.fd, &read_fds)) {
            terminal_proses_keyboard(app);
        }
        
        /* Proses tetikus */
        if (app->tetikus.fd >= 0 && FD_ISSET(app->tetikus.fd, &read_fds)) {
            terminal_proses_tetikus(app);
        }
        
        /* Update kursor berkedip */
        terminal_update_kursor(app);
        
        /* Render */
        terminal_render(app);
        
        /* Update FPS */
        app->frame_hitung++;
        {
            struct timeval sekarang;
            gettimeofday(&sekarang, NULL);
            unsigned long selisih = 
                (sekarang.tv_sec - app->waktu_frame_terakhir.tv_sec) * 1000 +
                (sekarang.tv_usec - app->waktu_frame_terakhir.tv_usec) / 1000;
            
            if (selisih >= 1000) {
                app->fps = app->frame_hitung;
                app->frame_hitung = 0;
                app->waktu_frame_terakhir = sekarang;
                terminal_update_status(app);
            }
        }
        
        /* Cek status shell */
        if (!cangkang_berjalan(&app->cangkang)) {
            app->berjalan = 0;
        }
    }
}

/* Tutup aplikasi terminal */
void terminal_tutup(AplikasiTerminal *app) {
    if (!app) {
        return;
    }
    
    /* Hentikan shell */
    if (app->cangkang.berjalan) {
        cangkang_hentikan(&app->cangkang, SIGTERM);
    }
    cangkang_bebas(&app->cangkang);
    
    /* Lepaskan VT */
    if (app->vt.state.diakuisisi) {
        vt_lepas(&app->vt);
    }
    vt_tutup(&app->vt);
    
    /* Tutup keyboard */
    papan_ketik_tutup(&app->kbd);
    
    /* Tutup tetikus */
    tetikus_tutup(&app->tetikus);
    
    /* Bebaskan clipboard */
    klip_bebas(&app->klip);
    
    /* Bebaskan terminal buffer */
    ansi_bebas_terminal(&app->term);
    
    /* Simpan konfigurasi jika auto_simpan */
    if (app->cfg.auto_simpan_konfig && app->konfig.diubah) {
        konfig_simpan(&app->konfig, NULL);
    }
    konfig_bebas(&app->konfig);
    
    /* Tutup framebuffer */
    fb_tutup(&app->fb);
}

/* Set callback keluar */
void terminal_set_callback_keluar(AplikasiTerminal *app, 
                                   void (*callback)(int)) {
    if (app) {
        app->callback_keluar = callback;
    }
}

/* Set callback ubah ukuran */
void terminal_set_callback_ubah_ukuran(AplikasiTerminal *app,
                                        void (*callback)(int, int)) {
    if (app) {
        app->callback_ubah_ukuran = callback;
    }
}

/* Global app instance for signal handling */
static AplikasiTerminal g_app;
static int g_running = 1;

/* Signal handler */
static void signal_handler(int sig) {
    (void)sig;
    g_running = 0;
}

/* Exit callback */
static void on_exit(int status) {
    (void)status;
    g_running = 0;
}

/* Main entry point */
int main(int argc, char *argv[]) {
    const char *config_file = NULL;
    int i;

    /* Parse command line arguments */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            config_file = argv[++i];
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("Pigura Terminal v%d.%d.%d\n",
                   PIGURA_VERSI_UTAMA, PIGURA_VERSI_MINOR, PIGURA_VERSI_TAMBAHAN);
            printf("Usage: %s [-c config_file]\n", argv[0]);
            printf("  -c <file>  Use specified config file\n");
            printf("  -h         Show this help\n");
            return 0;
        }
    }

    /* Setup signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Initialize terminal application */
    printf("Initializing Pigura Terminal...\n");

    if (!terminal_init(&g_app, config_file)) {
        fprintf(stderr, "Failed to initialize terminal\n");
        return 1;
    }

    /* Set callbacks */
    terminal_set_callback_keluar(&g_app, on_exit);

    printf("Terminal initialized successfully.\n");
    printf("Framebuffer: %dx%d, %d bpp\n",
           g_app.fb.lebar, g_app.fb.tinggi, g_app.fb.bpp);
    printf("Terminal: %d rows x %d cols\n", g_app.cfg.baris, g_app.cfg.kolom);
    printf("Shell: %s\n", g_app.cfg.jalur_shell);

    /* Run terminal */
    printf("Starting shell...\n");
    terminal_jalankan(&g_app);

    /* Cleanup */
    printf("Shutting down...\n");
    terminal_tutup(&g_app);

    printf("Terminal exited with status %d\n", g_app.cfg.status_keluar);
    return g_app.cfg.status_keluar;
}
