#ifndef TERMINAL_APP_H
#define TERMINAL_APP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <signal.h>
#include <time.h>

/* Include semua pustaka yang telah dibuat */
#include "font.h"
#include "unicode.h"
#include "ansi.h"
#include "keyboard.h"
#include "mouse.h"
#include "vterm.h"
#include "shell.h"
#include "clip.h"
#include "config.h"

/* Konstanta untuk aplikasi terminal */
#define TERMINAL_MAX_FPS 60
#define TERMINAL_FRAME_TIME (1000000 / TERMINAL_MAX_FPS)
#define TERMINAL_DEFAULT_ROWS 25
#define TERMINAL_DEFAULT_COLS 80
#define TERMINAL_DEFAULT_FONT_SIZE 16
#define TERMINAL_CONFIG_FILE "terminal.ini"

/* Mode aplikasi */
#define TERMINAL_MODE_NORMAL 0
#define TERMINAL_MODE_SELECT 1
#define TERMINAL_MODE_SEARCH 2

/* Struktur warna tema */
typedef struct {
    ConfigColor foreground;
    ConfigColor background;
    ConfigColor cursor;
    ConfigColor selection_fg;
    ConfigColor selection_bg;
    ConfigColor search_highlight;
} TerminalTheme;

/* Struktur konfigurasi terminal */
typedef struct {
    int width;
    int height;
    int rows;
    int cols;
    int font_size;
    int use_unicode;
    int use_mouse;
    int scrollback_lines;
    char shell_path[256];
    int auto_save_config;
    int create_backup;
    TerminalTheme theme;
    int transparency;
    int fullscreen;
    int bell_on;
    int visual_bell;
    int cursor_blink;
    int show_scrollbar;
    int word_wrap;
} TerminalConfig;

/* Struktur status bar */
typedef struct {
    int visible;
    char left_text[64];
    char right_text[64];
    ConfigColor fg;
    ConfigColor bg;
} StatusBar;

/* Struktur selection */
typedef struct {
    int active;
    int start_row;
    int start_col;
    int end_row;
    int end_col;
} Selection;

/* Struktur search */
typedef struct {
    int active;
    char query[256];
    int current_match;
    int total_matches;
    int *match_rows;
    int match_count;
    int match_capacity;
} Search;

/* Struktur aplikasi terminal */
typedef struct {
    /* Komponen utama */
    FBInfo fb;
    TerminalBuffer term;
    ANSIParser parser;
    
    /* Input handling */
    KeyboardHandler keyboard;
    MouseHandler mouse;
    VTManager vt;
    ShellManager shell;
    ClipboardManager clipboard;
    ConfigManager config;
    
    /* Konfigurasi */
    TerminalConfig cfg;
    
    /* UI elements */
    StatusBar status_bar;
    Selection selection;
    Search search;
    
    /* State aplikasi */
    int running;
    int mode;
    int focused;
    int frame_count;
    struct timeval last_frame_time;
    int cursor_visible;
    int cursor_blink_state;
    struct timeval last_blink_time;
    
    /* Performansi */
    int fps;
    int frame_time;
    int render_time;
    
    /* Callbacks */
    void (*exit_callback)(int status);
    void (*resize_callback)(int width, int height);
} TerminalApp;

/* Fungsi utilitas */
static void terminal_app_default_config(TerminalConfig *cfg) {
    cfg->width = 640;
    cfg->height = 480;
    cfg->rows = TERMINAL_DEFAULT_ROWS;
    cfg->cols = TERMINAL_DEFAULT_COLS;
    cfg->font_size = TERMINAL_DEFAULT_FONT_SIZE;
    cfg->use_unicode = 1;
    cfg->use_mouse = 1;
    cfg->scrollback_lines = 1000;
    strcpy(cfg->shell_path, "/bin/bash");
    cfg->auto_save_config = 1;
    cfg->create_backup = 1;
    
    /* Tema default */
    cfg->theme.foreground = (ConfigColor){255, 255, 255}; /* Putih */
    cfg->theme.background = (ConfigColor){0, 0, 0};     /* Hitam */
    cfg->theme.cursor = (ConfigColor){0, 255, 0};      /* Hijau */
    cfg->theme.selection_fg = (ConfigColor){0, 0, 0};   /* Hitam */
    cfg->theme.selection_bg = (ConfigColor){255, 255, 0}; /* Kuning */
    cfg->theme.search_highlight = (ConfigColor){255, 165, 0}; /* Oranye */
    
    cfg->transparency = 0;
    cfg->fullscreen = 0;
    cfg->bell_on = 1;
    cfg->visual_bell = 1;
    cfg->cursor_blink = 1;
    cfg->show_scrollbar = 1;
    cfg->word_wrap = 1;
}

/* Inisialisasi aplikasi terminal */
static int terminal_app_init(TerminalApp *app, const char *config_file) {
    /* Inisialisasi konfigurasi default */
    terminal_app_default_config(&app->cfg);
    
    /* Inisialisasi konfigurasi manager */
    if (!config_init(&app->config, config_file ? config_file : TERMINAL_CONFIG_FILE)) {
        fprintf(stderr, "Gagal menginisialisasi konfigurasi\n");
        return 0;
    }
    
    /* Set opsi konfigurasi */
    config_set_auto_save(&app->config, app->cfg.auto_save_config);
    config_set_create_backup(&app->config, app->cfg.create_backup);
    
    /* Muat konfigurasi dari file */
    if (!config_load(&app->config, NULL)) {
        printf("File konfigurasi tidak ditemukan, menggunakan default\n");
        /* Simpan konfigurasi default */
        config_save(&app->config, NULL);
    } else {
        /* Baca konfigurasi dari file */
        app->cfg.width = config_get_int(&app->config, "display", "width", app->cfg.width);
        app->cfg.height = config_get_int(&app->config, "display", "height", app->cfg.height);
        app->cfg.rows = config_get_int(&app->config, "terminal", "rows", app->cfg.rows);
        app->cfg.cols = config_get_int(&app->config, "terminal", "cols", app->cfg.cols);
        app->cfg.font_size = config_get_int(&app->config, "terminal", "font_size", app->cfg.font_size);
        app->cfg.use_unicode = config_get_bool(&app->config, "terminal", "use_unicode", app->cfg.use_unicode);
        app->cfg.use_mouse = config_get_bool(&app->config, "terminal", "use_mouse", app->cfg.use_mouse);
        app->cfg.scrollback_lines = config_get_int(&app->config, "terminal", "scrollback_lines", app->cfg.scrollback_lines);
        strcpy(app->cfg.shell_path, config_get_string(&app->config, "terminal", "shell_path", app->cfg.shell_path));
        app->cfg.transparency = config_get_float(&app->config, "display", "transparency", app->cfg.transparency);
        app->cfg.fullscreen = config_get_bool(&app->config, "display", "fullscreen", app->cfg.fullscreen);
        app->cfg.bell_on = config_get_bool(&app->config, "terminal", "bell_on", app->cfg.bell_on);
        app->cfg.visual_bell = config_get_bool(&app->config, "terminal", "visual_bell", app->cfg.visual_bell);
        app->cfg.cursor_blink = config_get_bool(&app->config, "terminal", "cursor_blink", app->cfg.cursor_blink);
        app->cfg.show_scrollbar = config_get_bool(&app->config, "display", "show_scrollbar", app->cfg.show_scrollbar);
        app->cfg.word_wrap = config_get_bool(&app->config, "terminal", "word_wrap", app->cfg.word_wrap);
        
        /* Baca tema */
        config_get_color(&app->config, "colors", "foreground", &app->cfg.theme.foreground);
        config_get_color(&app->config, "colors", "background", &app->cfg.theme.background);
        config_get_color(&app->config, "colors", "cursor", &app->cfg.theme.cursor);
        config_get_color(&app->config, "colors", "selection_fg", &app->cfg.theme.selection_fg);
        config_get_color(&app->config, "colors", "selection_bg", &app->cfg.theme.selection_bg);
        config_get_color(&app->config, "colors", "search_highlight", &app->cfg.theme.search_highlight);
    }
    
    /* Inisialisasi framebuffer */
    if (!init_framebuffer("/dev/fb0", &app->fb)) {
        fprintf(stderr, "Gagal menginisialisasi framebuffer\n");
        config_free(&app->config);
        return 0;
    }
    
    /* Inisialisasi terminal buffer */
    if (!ansi_init_terminal(&app->term, app->cfg.rows, app->cfg.cols)) {
        fprintf(stderr, "Gagal menginisialisasi terminal buffer\n");
        cleanup_framebuffer(&app->fb);
        config_free(&app->config);
        return 0;
    }
    
    /* Inisialisasi parser ANSI */
    ansi_init_parser(&app->parser);
    
    /* Inisialisasi keyboard handler */
    if (!keyboard_init(&app->keyboard, NULL)) {
        fprintf(stderr, "Gagal menginisialisasi keyboard\n");
        ansi_free_terminal(&app->term);
        cleanup_framebuffer(&app->fb);
        config_free(&app->config);
        return 0;
    }
    
    /* Inisialisasi mouse handler */
    if (!mouse_init(&app->mouse, NULL)) {
        fprintf(stderr, "Gagal menginisialisasi mouse\n");
        keyboard_close(&app->keyboard);
        ansi_free_terminal(&app->term);
        cleanup_framebuffer(&app->fb);
        config_free(&app->config);
        return 0;
    }
    
    /* Set mode mouse */
    mouse_set_mode(&app->mouse, MOUSE_MODE_NORMAL);
    mouse_set_encoding(&app->mouse, MOUSE_ENCODING_SGR);
    mouse_set_cell_size(&app->mouse, 8, 16);
    mouse_set_screen_size(&app->mouse, app->cfg.cols, app->cfg.rows);
    
    /* Inisialisasi VT manager */
    if (!vt_init(&app->vt)) {
        fprintf(stderr, "Gagal menginisialisasi VT manager\n");
        mouse_close(&app->mouse);
        keyboard_close(&app->keyboard);
        ansi_free_terminal(&app->term);
        cleanup_framebuffer(&app->fb);
        config_free(&app->config);
        return 0;
    }
    
    /* Inisialisasi shell manager */
    if (!shell_init(&app->shell, app->cfg.shell_path)) {
        fprintf(stderr, "Gagal menginisialisasi shell manager\n");
        vt_close(&app->vt);
        mouse_close(&app->mouse);
        keyboard_close(&app->keyboard);
        ansi_free_terminal(&app->term);
        cleanup_framebuffer(&app->fb);
        config_free(&app->config);
        return 0;
    }
    
    /* Set shell callbacks */
    shell_set_output_callback(&app->shell, NULL); /* Akan diatur nanti */
    shell_set_exit_callback(&app->shell, NULL); /* Akan diatur nanti */
    
    /* Set shell size */
    shell_set_size(&app->shell, app->cfg.rows, app->cfg.cols);
    
    /* Inisialisasi clipboard manager */
    if (!clipboard_init(&app->clipboard, 4096, 1048576)) {
        fprintf(stderr, "Gagal menginisialisasi clipboard\n");
        shell_free(&app->shell);
        vt_close(&app->vt);
        mouse_close(&app->mouse);
        keyboard_close(&app->keyboard);
        ansi_free_terminal(&app->term);
        cleanup_framebuffer(&app->fb);
        config_free(&app->config);
        return 0;
    }
    
    /* Inisialisasi status bar */
    app->status_bar.visible = 1;
    strcpy(app->status_bar.left_text, "Terminal");
    strcpy(app->status_bar.right_text, "Ready");
    app->status_bar.fg = app->cfg.theme.foreground;
    app->status_bar.bg = app->cfg.theme.background;
    
    /* Inisialisasi selection */
    app->selection.active = 0;
    app->selection.start_row = 0;
    app->selection.start_col = 0;
    app->selection.end_row = 0;
    app->selection.end_col = 0;
    
    /* Inisialisasi search */
    app->search.active = 0;
    strcpy(app->search.query, "");
    app->search.current_match = -1;
    app->search.total_matches = 0;
    app->search.match_rows = NULL;
    app->search.match_count = 0;
    app->search.match_capacity = 0;
    
    /* Inisialisasi state aplikasi */
    app->running = 1;
    app->mode = TERMINAL_MODE_NORMAL;
    app->focused = 1;
    app->frame_count = 0;
    gettimeofday(&app->last_frame_time, NULL);
    app->cursor_visible = 1;
    app->cursor_blink_state = 1;
    gettimeofday(&app->last_blink_time, NULL);
    
    /* Inisialisasi performansi */
    app->fps = 0;
    app->frame_time = 0;
    app->render_time = 0;
    
    /* Inisialisasi callbacks */
    app->exit_callback = NULL;
    app->resize_callback = NULL;
    
    return 1;
}

/* Callback untuk shell output */
static void terminal_app_shell_output(const char *data, int size, void *user_data) {
    TerminalApp *app = (TerminalApp *)user_data;
    ansi_process_string(&app->term, &app->parser, data);
}

/* Callback untuk shell exit */
static void terminal_app_shell_exit(int status, void *user_data) {
    TerminalApp *app = (TerminalApp *)user_data;
    
    /* Update status bar */
    sprintf(app->status_bar.right_text, "Exit: %d", status);
    
    /* Panggil exit callback jika ada */
    if (app->exit_callback) {
        app->exit_callback(status);
    }
}

/* Callback untuk VT switch */
static void terminal_app_vt_switch(int new_vt, int old_vt, void *user_data) {
    TerminalApp *app = (TerminalApp *)user_data;
    
    if (new_vt == vt_get_current(&app->vt)) {
        app->focused = 1;
    } else {
        app->focused = 0;
    }
}

/* Update status bar */
static void terminal_app_update_status_bar(TerminalApp *app) {
    /* Update waktu */
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[32];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);
    
    sprintf(app->status_bar.right_text, "%s | %dx%d | %d FPS", 
            time_str, app->cfg.cols, app->cfg.rows, app->fps);
}

/* Render status bar */
static void terminal_app_render_status_bar(TerminalApp *app) {
    if (!app->status_bar.visible) {
        return;
    }
    
    int y = app->fb.height - 16; /* Asumsi font 16px */
    int x = 0;
    
    /* Gambar background status bar */
    for (int i = 0; i < app->fb.width && i < 16; i++) {
        for (int j = 0; j < app->fb.width; j++) {
            draw_pixel(&app->fb, j, y + i, 
                      app->status_bar.bg.r, 
                      app->status_bar.bg.g, 
                      app->status_bar.bg.b);
        }
    }
    
    /* Gambar teks kiri */
    font_draw_string(&app->fb, x, y, app->status_bar.left_text,
                    app->status_bar.fg, app->status_bar.bg);
    
    /* Gambar teks kanan */
    int right_x = app->fb.width - (strlen(app->status_bar.right_text) * 8);
    font_draw_string(&app->fb, right_x, y, app->status_bar.right_text,
                    app->status_bar.fg, app->status_bar.bg);
}

/* Render selection */
static void terminal_app_render_selection(TerminalApp *app) {
    if (!app->selection.active) {
        return;
    }
    
    /* Normalisasi selection */
    int start_row = app->selection.start_row;
    int start_col = app->selection.start_col;
    int end_row = app->selection.end_row;
    int end_col = app->selection.end_col;
    
    if (start_row > end_row || (start_row == end_row && start_col > end_col)) {
        int temp_row = start_row;
        int temp_col = start_col;
        start_row = end_row;
        start_col = end_col;
        end_row = temp_row;
        end_col = temp_col;
    }
    
    /* Render selection */
    for (int row = start_row; row <= end_row; row++) {
        int col_start = (row == start_row) ? start_col : 0;
        int col_end = (row == end_row) ? end_col : app->cfg.cols - 1;
        
        for (int col = col_start; col <= col_end; col++) {
            int x = col * 8;
            int y = row * 16;
            
            /* Gambar background selection */
            for (int i = 0; i < 16; i++) {
                for (int j = 0; j < 8; j++) {
                    draw_pixel(&app->fb, x + j, y + i,
                              app->cfg.theme.selection_bg.r,
                              app->cfg.theme.selection_bg.g,
                              app->cfg.theme.selection_bg.b);
                }
            }
            
            /* Gambar karakter dengan warna selection */
            char ch = app->term.cells[row][col].ch;
            font_draw_char(&app->fb, x, y, ch,
                          app->cfg.theme.selection_fg,
                          app->cfg.theme.selection_bg);
        }
    }
}

/* Render search highlights */
static void terminal_app_render_search(TerminalApp *app) {
    if (!app->search.active || strlen(app->search.query) == 0) {
        return;
    }
    
    /* Render semua match */
    for (int i = 0; i < app->search.match_count; i++) {
        int row = app->search.match_rows[i];
        
        /* Cari posisi query di baris ini */
        char *line = app->term.cells[row][0].ch;
        char *pos = strstr(line, app->search.query);
        
        while (pos) {
            int col = pos - line;
            int query_len = strlen(app->search.query);
            
            /* Render highlight */
            for (int j = 0; j < query_len && col + j < app->cfg.cols; j++) {
                int x = (col + j) * 8;
                int y = row * 16;
                
                /* Gambar background highlight */
                for (int k = 0; k < 16; k++) {
                    for (int l = 0; l < 8; l++) {
                        draw_pixel(&app->fb, x + l, y + k,
                                  app->cfg.theme.search_highlight.r,
                                  app->cfg.theme.search_highlight.g,
                                  app->cfg.theme.search_highlight.b);
                    }
                }
                
                /* Gambar karakter */
                char ch = app->term.cells[row][col + j].ch;
                font_draw_char(&app->fb, x, y, ch,
                              app->cfg.theme.foreground,
                              app->cfg.theme.search_highlight);
            }
            
            /* Cari berikutnya */
            pos = strstr(pos + 1, app->search.query);
        }
    }
}

/* Handle keyboard input */
static void terminal_app_handle_keyboard(TerminalApp *app) {
    char buffer[256];
    int bytes_read = keyboard_read(&app->keyboard, buffer, sizeof(buffer));
    
    if (bytes_read > 0) {
        /* Handle mode khusus */
        if (app->mode == TERMINAL_MODE_SEARCH) {
            /* Mode pencarian */
            if (buffer[0] == '\033') { /* Escape */
                app->mode = TERMINAL_MODE_NORMAL;
                app->search.active = 0;
                strcpy(app->search.query, "");
            } else if (buffer[0] == '\n') {
                /* Execute search */
                app->search.current_match = -1;
                /* Implementasi pencarian... */
            } else if (buffer[0] == '\b') {
                /* Backspace */
                int len = strlen(app->search.query);
                if (len > 0) {
                    app->search.query[len - 1] = '\0';
                }
            } else if (isprint(buffer[0])) {
                /* Tambahkan ke query */
                int len = strlen(app->search.query);
                if (len < sizeof(app->search.query) - 1) {
                    app->search.query[len] = buffer[0];
                    app->search.query[len + 1] = '\0';
                }
            }
            
            /* Update status bar */
            sprintf(app->status_bar.left_text, "Search: %s", app->search.query);
        } else if (app->mode == TERMINAL_MODE_SELECT) {
            /* Mode seleksi */
            if (buffer[0] == '\033') { /* Escape */
                app->mode = TERMINAL_MODE_NORMAL;
                app->selection.active = 0;
            } else if (buffer[0] == 'c') {
                /* Copy selection */
                if (app->selection.active) {
                    /* Implementasi copy... */
                }
            } else if (buffer[0] == 'v') {
                /* Paste */
                const char *clipboard_data = clipboard_paste_string(&app->clipboard);
                if (clipboard_data) {
                    shell_write(&app->shell, clipboard_data, strlen(clipboard_data));
                }
            }
        } else {
            /* Mode normal */
            if (buffer[0] == '\033') { /* Escape sequence */
                /* Cek kombinasi tombol */
                if (strcmp(buffer, "\033[24~") == 0) { /* F12 */
                    app->running = 0;
                } else if (strcmp(buffer, "\033[1~") == 0) { /* Home */
                    /* Implementasi home... */
                } else if (strcmp(buffer, "\033[4~") == 0) { /* End */
                    /* Implementasi end... */
                } else if (strcmp(buffer, "\033[5~") == 0) { /* Page Up */
                    /* Implementasi page up... */
                } else if (strcmp(buffer, "\033[6~") == 0) { /* Page Down */
                    /* Implementasi page down... */
                } else if (strcmp(buffer, "\033OP") == 0) { /* F1 */
                    /* Implementasi F1... */
                }
                /* Kirim ke shell */
                shell_write(&app->shell, buffer, bytes_read);
            } else if (buffer[0] == '\f') { /* Ctrl+L */
                /* Clear screen */
                ansi_process_string(&app->term, &app->parser, "\033[2J");
            } else if (buffer[0] == '\1') { /* Ctrl+A */
                /* Select all */
                app->selection.active = 1;
                app->selection.start_row = 0;
                app->selection.start_col = 0;
                app->selection.end_row = app->cfg.rows - 1;
                app->selection.end_col = app->cfg.cols - 1;
            } else if (buffer[0] == '\3') { /* Ctrl+C */
                /* Copy */
                if (app->selection.active) {
                    /* Implementasi copy... */
                }
            } else if (buffer[0] == '\16') { /* Ctrl+V */
                /* Paste */
                const char *clipboard_data = clipboard_paste_string(&app->clipboard);
                if (clipboard_data) {
                    shell_write(&app->shell, clipboard_data, strlen(clipboard_data));
                }
            } else if (buffer[0] == '\6') { /* Ctrl+F */
                /* Search */
                app->mode = TERMINAL_MODE_SEARCH;
                app->search.active = 1;
                strcpy(app->search.query, "");
                sprintf(app->status_bar.left_text, "Search: ");
            } else {
                /* Kirim ke shell */
                shell_write(&app->shell, buffer, bytes_read);
            }
        }
    }
}

/* Handle mouse input */
static void terminal_app_handle_mouse(TerminalApp *app) {
    char buffer[256];
    int bytes_read = mouse_read(&app->mouse, buffer, sizeof(buffer));
    
    if (bytes_read > 0) {
        /* Parse mouse sequence */
        if (app->cfg.use_mouse && app->mode == TERMINAL_MODE_NORMAL) {
            /* Kirim ke shell */
            shell_write(&app->shell, buffer, bytes_read);
        }
    }
}

/* Update cursor blink */
static void terminal_app_update_cursor_blink(TerminalApp *app) {
    if (!app->cfg.cursor_blink) {
        return;
    }
    
    struct timeval now;
    gettimeofday(&now, NULL);
    
    long elapsed = (now.tv_sec - app->last_blink_time.tv_sec) * 1000000 +
                  (now.tv_usec - app->last_blink_time.tv_usec);
    
    if (elapsed >= 500000) { /* 500ms */
        app->cursor_blink_state = !app->cursor_blink_state;
        app->last_blink_time = now;
    }
}

/* Render frame */
static void terminal_app_render(TerminalApp *app) {
    struct timeval start_time;
    gettimeofday(&start_time, NULL);
    
    /* Clear screen dengan warna background */
    for (int y = 0; y < app->fb.height; y++) {
        for (int x = 0; x < app->fb.width; x++) {
            draw_pixel(&app->fb, x, y,
                      app->cfg.theme.background.r,
                      app->cfg.theme.background.g,
                      app->cfg.theme.background.b);
        }
    }
    
    /* Render terminal */
    ansi_render_terminal(&app->term, &app->fb,
                         app->cfg.use_unicode ? unicode_draw_char : font_draw_char);
    
    /* Render selection */
    terminal_app_render_selection(app);
    
    /* Render search */
    terminal_app_render_search(app);
    
    /* Render status bar */
    terminal_app_render_status_bar(app);
    
    /* Update render time */
    struct timeval end_time;
    gettimeofday(&end_time, NULL);
    app->render_time = (end_time.tv_sec - start_time.tv_sec) * 1000000 +
                       (end_time.tv_usec - start_time.tv_usec);
}

/* Main loop */
static void terminal_app_run(TerminalApp *app) {
    /* Jalankan shell */
    shell_set_output_callback(&app->shell, terminal_app_shell_output, app);
    shell_set_exit_callback(&app->shell, terminal_app_shell_exit, app);
    
    if (!shell_run(&app->shell)) {
        fprintf(stderr, "Gagal menjalankan shell\n");
        return;
    }
    
    /* Set VT callbacks */
    vt_set_switch_callback(&app->vt, terminal_app_vt_switch, app);
    
    /* Set mode VT */
    vt_set_mode(&app->vt, VT_MODE_GRAPHICS);
    
    /* Set keyboard mode */
    vt_set_keyboard_mode(&app->vt, K_RAW);
    
    /* Main loop */
    fd_set read_fds;
    int max_fd;
    
    while (app->running) {
        struct timeval frame_start;
        gettimeofday(&frame_start, NULL);
        
        /* Update status bar */
        terminal_app_update_status_bar(app);
        
        /* Update cursor blink */
        terminal_app_update_cursor_blink(app);
        
        /* Handle input */
        FD_ZERO(&read_fds);
        FD_SET(app->keyboard.fd, &read_fds);
        FD_SET(app->mouse.fd, &read_fds);
        FD_SET(app->shell.master_fd, &read_fds);
        
        max_fd = app->keyboard.fd;
        if (app->mouse.fd > max_fd) max_fd = app->mouse.fd;
        if (app->shell.master_fd > max_fd) max_fd = app->shell.master_fd;
        
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 10000; /* 10ms */
        
        if (select(max_fd + 1, &read_fds, NULL, NULL, &timeout) > 0) {
            /* Handle keyboard input */
            if (FD_ISSET(app->keyboard.fd, &read_fds)) {
                terminal_app_handle_keyboard(app);
            }
            
            /* Handle mouse input */
            if (FD_ISSET(app->mouse.fd, &read_fds)) {
                terminal_app_handle_mouse(app);
            }
            
            /* Handle shell output */
            if (FD_ISSET(app->shell.master_fd, &read_fds)) {
                char buffer[4096];
                ssize_t n = read(app->shell.master_fd, buffer, sizeof(buffer));
                if (n > 0) {
                    ansi_process_string(&app->term, &app->parser, buffer);
                } else if (n == 0) {
                    /* Shell exited */
                    app->running = 0;
                }
            }
        }
        
        /* Render frame */
        terminal_app_render(app);
        
        /* Update FPS */
        app->frame_count++;
        struct timeval now;
        gettimeofday(&now, NULL);
        
        if (now.tv_sec - app->last_frame_time.tv_sec >= 1) {
            app->fps = app->frame_count;
            app->frame_count = 0;
            app->last_frame_time = now;
        }
        
        /* Frame timing */
        struct timeval frame_end;
        gettimeofday(&frame_end, NULL);
        app->frame_time = (frame_end.tv_sec - frame_start.tv_sec) * 1000000 +
                          (frame_end.tv_usec - frame_start.tv_usec);
        
        /* Limit frame rate */
        if (app->frame_time < TERMINAL_FRAME_TIME) {
            usleep(TERMINAL_FRAME_TIME - app->frame_time);
        }
    }
}

/* Set exit callback */
static void terminal_app_set_exit_callback(TerminalApp *app, void (*callback)(int)) {
    app->exit_callback = callback;
}

/* Set resize callback */
static void terminal_app_set_resize_callback(TerminalApp *app, void (*callback)(int, int)) {
    app->resize_callback = callback;
}

/* Close application */
static void terminal_app_close(TerminalApp *app) {
    /* Save configuration */
    if (app->config.modified) {
        config_save(&app->config, NULL);
    }
    
    /* Restore VT mode */
    vt_set_mode(&app->vt, VT_MODE_TEXT);
    
    /* Close all components */
    clipboard_free(&app->clipboard);
    shell_free(&app->shell);
    vt_close(&app->vt);
    mouse_close(&app->mouse);
    keyboard_close(&app->keyboard);
    ansi_free_terminal(&app->term);
    cleanup_framebuffer(&app->fb);
    config_free(&app->config);
    
    /* Free search matches */
    if (app->search.match_rows) {
        free(app->search.match_rows);
    }
}

#endif /* TERMINAL_APP_H */
