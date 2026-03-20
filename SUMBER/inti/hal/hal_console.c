/*
 * PIGURA OS - HAL_CONSOLE.C
 * -------------------------
 * Implementasi console untuk Hardware Abstraction Layer.
 *
 * Berkas ini berisi implementasi fungsi-fungsi yang berkaitan dengan
 * output console menggunakan VGA text mode (80x25).
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "hal.h"
#include "../kernel.h"

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

/* VGA text mode dimensions */
#define VGA_LEBAR               80
#define VGA_TINGGI              25

/* VGA memory address */
#define VGA_MEMORI              0xB8000

/* VGA I/O ports */
#define VGA_CTRL_REGISTER       0x3D4
#define VGA_DATA_REGISTER       0x3D5
#define VGA_CURSOR_HIGH         14
#define VGA_CURSOR_LOW          15

/* Tab size */
#define TAB_UKURAN              4

/* Scroll behavior */
#define SCROLL_LINES            1

/*
 * ============================================================================
 * STRUKTUR DATA LOKAL (LOCAL DATA STRUCTURES)
 * ============================================================================
 */

/* VGA entry structure */
typedef struct {
    char karakter;
    tak_bertanda8_t warna;
} __attribute__((packed)) vga_entry_t;

/* Console state */
typedef struct {
    vga_entry_t *buffer;            /* Pointer ke VGA buffer */
    tak_bertanda32_t kolom;         /* Posisi cursor X */
    tak_bertanda32_t baris;         /* Posisi cursor Y */
    tak_bertanda8_t warna;          /* Warna saat ini */
    tak_bertanda32_t lebar;         /* Lebar layar */
    tak_bertanda32_t tinggi;        /* Tinggi layar */
    bool_t initialized;             /* Status inisialisasi */
} console_state_t;

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* State console */
static console_state_t console_state = {
    .buffer = NULL,
    .kolom = 0,
    .baris = 0,
    .warna = 0,
    .lebar = VGA_LEBAR,
    .tinggi = VGA_TINGGI,
    .initialized = SALAH
};

/* Warna default */
static const tak_bertanda8_t WARNA_DEFAULT = VGA_ENTRY(VGA_ABU_ABU, VGA_HITAM);

/* Warna untuk error */
static const tak_bertanda8_t WARNA_ERROR = VGA_ENTRY(VGA_MERAH, VGA_HITAM);

/* Warna untuk warning */
static const tak_bertanda8_t WARNA_WARN = VGA_ENTRY(VGA_KUNING, VGA_HITAM);

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * vga_entry_buat
 * --------------
 * Buat VGA entry dengan karakter dan warna.
 *
 * Parameter:
 *   c     - Karakter
 *   color - Warna
 *
 * Return: VGA entry
 */
static inline vga_entry_t vga_entry_buat(char c, tak_bertanda8_t warna)
{
    vga_entry_t entry;
    entry.karakter = c;
    entry.warna = warna;
    return entry;
}

/*
 * vga_update_cursor
 * -----------------
 * Update posisi cursor hardware.
 */
static void vga_update_cursor(tak_bertanda32_t kolom,
                              tak_bertanda32_t baris)
{
    tak_bertanda16_t pos;

    pos = (tak_bertanda16_t)(baris * VGA_LEBAR + kolom);

    /* Set cursor low byte */
    outb(VGA_CTRL_REGISTER, VGA_CURSOR_LOW);
    outb(VGA_DATA_REGISTER, (tak_bertanda8_t)(pos & 0xFF));

    /* Set cursor high byte */
    outb(VGA_CTRL_REGISTER, VGA_CURSOR_HIGH);
    outb(VGA_DATA_REGISTER, (tak_bertanda8_t)((pos >> 8) & 0xFF));
}

/*
 * vga_get_cursor
 * --------------
 * Dapatkan posisi cursor hardware.
 *
 * Return: Posisi cursor linear
 */
static tak_bertanda16_t vga_get_cursor(void)
{
    tak_bertanda16_t pos;

    outb(VGA_CTRL_REGISTER, VGA_CURSOR_LOW);
    pos = inb(VGA_DATA_REGISTER);

    outb(VGA_CTRL_REGISTER, VGA_CURSOR_HIGH);
    pos |= (tak_bertanda16_t)(inb(VGA_DATA_REGISTER) << 8);

    return pos;
}

/*
 * console_putchar_at
 * ------------------
 * Tampilkan karakter di posisi tertentu.
 *
 * Parameter:
 *   c      - Karakter
 *   warna  - Warna
 *   kolom  - Posisi X
 *   baris  - Posisi Y
 */
static void console_putchar_at(char c, tak_bertanda8_t warna,
                               tak_bertanda32_t kolom,
                               tak_bertanda32_t baris)
{
    tak_bertanda32_t index;

    if (kolom >= console_state.lebar || baris >= console_state.tinggi) {
        return;
    }

    index = baris * console_state.lebar + kolom;
    console_state.buffer[index] = vga_entry_buat(c, warna);
}

/*
 * console_scroll_up
 * -----------------
 * Scroll layar ke atas.
 *
 * Parameter:
 *   lines - Jumlah baris
 */
static void console_scroll_up(tak_bertanda32_t lines)
{
    tak_bertanda32_t i;
    tak_bertanda32_t j;
    tak_bertanda32_t src_index;
    tak_bertanda32_t dst_index;

    if (lines >= console_state.tinggi) {
        lines = console_state.tinggi;
    }

    /* Copy baris ke atas */
    for (i = 0; i < console_state.tinggi - lines; i++) {
        for (j = 0; j < console_state.lebar; j++) {
            src_index = (i + lines) * console_state.lebar + j;
            dst_index = i * console_state.lebar + j;
            console_state.buffer[dst_index] = console_state.buffer[src_index];
        }
    }

    /* Clear baris terakhir */
    for (i = console_state.tinggi - lines; i < console_state.tinggi; i++) {
        for (j = 0; j < console_state.lebar; j++) {
            console_putchar_at(' ', console_state.warna, j, i);
        }
    }
}

/*
 * console_newline
 * ---------------
 * Pindah ke baris baru.
 */
static void console_newline(void)
{
    console_state.kolom = 0;

    if (console_state.baris < console_state.tinggi - 1) {
        console_state.baris++;
    } else {
        console_scroll_up(SCROLL_LINES);
    }

    vga_update_cursor(console_state.kolom, console_state.baris);
}

/*
 * console_tab
 * -----------
 * Tab ke kolom berikutnya.
 */
static void console_tab(void)
{
    tak_bertanda32_t next_tab;

    next_tab = ((console_state.kolom / TAB_UKURAN) + 1) * TAB_UKURAN;

    if (next_tab >= console_state.lebar) {
        console_newline();
    } else {
        console_state.kolom = next_tab;
        vga_update_cursor(console_state.kolom, console_state.baris);
    }
}

/*
 * console_backspace
 * -----------------
 * Hapus karakter sebelumnya.
 */
static void console_backspace(void)
{
    if (console_state.kolom > 0) {
        console_state.kolom--;
        console_putchar_at(' ', console_state.warna,
                          console_state.kolom, console_state.baris);
        vga_update_cursor(console_state.kolom, console_state.baris);
    } else if (console_state.baris > 0) {
        console_state.baris--;
        console_state.kolom = console_state.lebar - 1;
        console_putchar_at(' ', console_state.warna,
                          console_state.kolom, console_state.baris);
        vga_update_cursor(console_state.kolom, console_state.baris);
    }
}

/*
 * console_carriage_return
 * -----------------------
 * Kembali ke awal baris.
 */
static void console_carriage_return(void)
{
    console_state.kolom = 0;
    vga_update_cursor(console_state.kolom, console_state.baris);
}

/*
 * ============================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_console_init
 * ----------------
 * Inisialisasi console.
 */
status_t hal_console_init(void)
{
    tak_bertanda32_t i;
    tak_bertanda32_t j;

    if (console_state.initialized) {
        return STATUS_SUDAH_ADA;
    }

    /* Set VGA buffer */
    console_state.buffer = (vga_entry_t *)VGA_MEMORI;

    /* Set dimensi */
    console_state.lebar = VGA_LEBAR;
    console_state.tinggi = VGA_TINGGI;

    /* Set warna default */
    console_state.warna = WARNA_DEFAULT;

    /* Clear screen */
    for (i = 0; i < console_state.tinggi; i++) {
        for (j = 0; j < console_state.lebar; j++) {
            console_putchar_at(' ', console_state.warna, j, i);
        }
    }

    /* Reset cursor */
    console_state.kolom = 0;
    console_state.baris = 0;
    vga_update_cursor(0, 0);

    console_state.initialized = BENAR;

    return STATUS_BERHASIL;
}

/*
 * hal_console_putchar
 * -------------------
 * Tampilkan satu karakter.
 */
status_t hal_console_putchar(char c)
{
    if (!console_state.initialized) {
        return STATUS_GAGAL;
    }

    /* Handle karakter khusus */
    switch (c) {
        case '\n':
            console_newline();
            break;

        case '\r':
            console_carriage_return();
            break;

        case '\t':
            console_tab();
            break;

        case '\b':
            console_backspace();
            break;

        case '\0':
            /* Null character tidak ditampilkan */
            break;

        default:
            /* Karakter printable */
            if (c >= ' ') {
                console_putchar_at(c, console_state.warna,
                                  console_state.kolom,
                                  console_state.baris);

                console_state.kolom++;

                /* Wrap ke baris baru jika perlu */
                if (console_state.kolom >= console_state.lebar) {
                    console_newline();
                } else {
                    vga_update_cursor(console_state.kolom,
                                     console_state.baris);
                }
            }
            break;
    }

    return STATUS_BERHASIL;
}

/*
 * hal_console_puts
 * ----------------
 * Tampilkan string.
 */
status_t hal_console_puts(const char *str)
{
    ukuran_t i;
    ukuran_t len;

    if (str == NULL) {
        return STATUS_PARAM_INVALID;
    }

    if (!console_state.initialized) {
        return STATUS_GAGAL;
    }

    len = kernel_strlen(str);

    for (i = 0; i < len; i++) {
        hal_console_putchar(str[i]);
    }

    return STATUS_BERHASIL;
}

/*
 * hal_console_clear
 * -----------------
 * Bersihkan layar.
 */
status_t hal_console_clear(void)
{
    tak_bertanda32_t i;
    tak_bertanda32_t j;

    if (!console_state.initialized) {
        return STATUS_GAGAL;
    }

    for (i = 0; i < console_state.tinggi; i++) {
        for (j = 0; j < console_state.lebar; j++) {
            console_putchar_at(' ', console_state.warna, j, i);
        }
    }

    console_state.kolom = 0;
    console_state.baris = 0;
    vga_update_cursor(0, 0);

    return STATUS_BERHASIL;
}

/*
 * hal_console_set_color
 * ---------------------
 * Set warna teks.
 */
status_t hal_console_set_color(tak_bertanda8_t fg, tak_bertanda8_t bg)
{
    if (!console_state.initialized) {
        return STATUS_GAGAL;
    }

    if (fg > 15 || bg > 15) {
        return STATUS_PARAM_INVALID;
    }

    console_state.warna = VGA_ENTRY(fg, bg);

    return STATUS_BERHASIL;
}

/*
 * hal_console_set_cursor
 * ----------------------
 * Set posisi cursor.
 */
status_t hal_console_set_cursor(tak_bertanda32_t x, tak_bertanda32_t y)
{
    if (!console_state.initialized) {
        return STATUS_GAGAL;
    }

    if (x >= console_state.lebar || y >= console_state.tinggi) {
        return STATUS_PARAM_INVALID;
    }

    console_state.kolom = x;
    console_state.baris = y;
    vga_update_cursor(x, y);

    return STATUS_BERHASIL;
}

/*
 * hal_console_get_cursor
 * ----------------------
 * Dapatkan posisi cursor.
 */
status_t hal_console_get_cursor(tak_bertanda32_t *x, tak_bertanda32_t *y)
{
    if (x == NULL || y == NULL) {
        return STATUS_PARAM_INVALID;
    }

    if (!console_state.initialized) {
        return STATUS_GAGAL;
    }

    *x = console_state.kolom;
    *y = console_state.baris;

    return STATUS_BERHASIL;
}

/*
 * hal_console_scroll
 * ------------------
 * Scroll layar.
 */
status_t hal_console_scroll(tak_bertanda32_t lines)
{
    if (!console_state.initialized) {
        return STATUS_GAGAL;
    }

    if (lines == 0) {
        return STATUS_BERHASIL;
    }

    console_scroll_up(lines);

    return STATUS_BERHASIL;
}

/*
 * hal_console_get_size
 * --------------------
 * Dapatkan ukuran layar.
 */
status_t hal_console_get_size(tak_bertanda32_t *width, tak_bertanda32_t *height)
{
    if (width == NULL || height == NULL) {
        return STATUS_PARAM_INVALID;
    }

    *width = console_state.lebar;
    *height = console_state.tinggi;

    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI UTILITAS CONSOLE (CONSOLE UTILITY FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_console_print_error
 * -----------------------
 * Print pesan error dengan warna merah.
 *
 * Parameter:
 *   str - String pesan
 */
void hal_console_print_error(const char *str)
{
    tak_bertanda8_t warna_lama;

    warna_lama = console_state.warna;
    console_state.warna = WARNA_ERROR;

    hal_console_puts(str);

    console_state.warna = warna_lama;
}

/*
 * hal_console_print_warning
 * -------------------------
 * Print pesan warning dengan warna kuning.
 *
 * Parameter:
 *   str - String pesan
 */
void hal_console_print_warning(const char *str)
{
    tak_bertanda8_t warna_lama;

    warna_lama = console_state.warna;
    console_state.warna = WARNA_WARN;

    hal_console_puts(str);

    console_state.warna = warna_lama;
}

/*
 * hal_console_print_hex
 * ---------------------
 * Print nilai dalam format hexadecimal.
 *
 * Parameter:
 *   value - Nilai yang akan dicetak
 *   digits - Jumlah digit (4 atau 8)
 */
void hal_console_print_hex(tak_bertanda32_t value, tak_bertanda32_t digits)
{
    char buffer[9];
    char hex_chars[] = "0123456789ABCDEF";
    tak_bertanda32_t i;

    /* Batasi digits ke 8 */
    if (digits > 8) {
        digits = 8;
    }

    /* Konversi ke hex string */
    for (i = 0; i < digits; i++) {
        buffer[digits - 1 - i] = hex_chars[value & 0xF];
        value >>= 4;
    }
    buffer[digits] = '\0';

    hal_console_puts(buffer);
}

/*
 * hal_console_print_decimal
 * -------------------------
 * Print nilai dalam format decimal.
 *
 * Parameter:
 *   value - Nilai yang akan dicetak
 */
void hal_console_print_decimal(tak_bertanda32_t value)
{
    char buffer[11];
    tak_bertanda32_t i;
    tak_bertanda32_t len;

    if (value == 0) {
        hal_console_putchar('0');
        return;
    }

    /* Konversi ke decimal string (dari belakang) */
    i = 10;
    buffer[i] = '\0';

    while (value > 0 && i > 0) {
        i--;
        buffer[i] = '0' + (value % 10);
        value /= 10;
    }

    len = 10 - i;
    hal_console_puts(&buffer[i]);
}
