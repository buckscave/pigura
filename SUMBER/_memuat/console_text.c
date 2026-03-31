/*
 * =============================================================================
 * PIGURA OS - CONSOLE TEXT MODE (VGA)
 * =============================================================================
 *
 * Berkas ini berisi implementasi console text mode menggunakan VGA buffer.
 * Console ini menggunakan memori video VGA di alamat 0xB8000 dengan mode
 * teks 80x25 karakter.
 *
 * Format memori VGA text:
 *   Setiap karakter = 2 byte:
 *     - Byte pertama: Karakter ASCII
 *     - Byte kedua:   Atribut (4 bit background, 4 bit foreground)
 *
 * Arsitektur: x86/x86_64
 * Versi: 1.0
 * =============================================================================
 */

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

/* Kompatibilitas C89: inline tidak tersedia */
#ifndef inline
#ifdef __GNUC__
    #define inline __inline__
#else
    #define inline
#endif
#endif

/* =============================================================================
 * KONSTANTA
 * =============================================================================
 */

/* Alamat memori video VGA */
#define VGA_BUFFER_ALAMAT       0xB8000

/* Ukuran layar */
#define VGA_LEBAR               80
#define VGA_TINGGI              25
#define VGA_JUMLAH_SEL          (VGA_LEBAR * VGA_TINGGI)

/* Port VGA */
#define VGA_PORT_CTRL           0x3D4       /* Control register */
#define VGA_PORT_DATA           0x3D5       /* Data register */

/* Register VGA */
#define VGA_REG_CURSOR_AWAL     0x0A        /* Cursor start */
#define VGA_REG_CURSOR_AKHIR    0x0B        /* Cursor end */
#define VGA_REG_CURSOR_LOKASI   0x0E        /* Cursor location high */
#define VGA_REG_CURSOR_LOK      0x0F        /* Cursor location low */

/* Warna teks VGA */
#define VGA_WARNA_HITAM         0x0
#define VGA_WARNA_BIRU          0x1
#define VGA_WARNA_HIJAU         0x2
#define VGA_WARNA_CYAN          0x3
#define VGA_WARNA_MERAH         0x4
#define VGA_WARNA_MAGENTA       0x5
#define VGA_WARNA_COKLAT        0x6
#define VGA_WARNA_ABU_ABU       0x7
#define VGA_WARNA_ABU_TERANG    0x8
#define VGA_WARNA_BIRU_TERANG   0x9
#define VGA_WARNA_HIJAU_TERANG  0xA
#define VGA_WARNA_CYAN_TERANG   0xB
#define VGA_WARNA_MERAH_TERANG  0xC
#define VGA_WARNA_MAGENTA_TER   0xD
#define VGA_WARNA_KUNING        0xE
#define VGA_WARNA_PUTIH         0xF

/* ============================================================================
 * Warna Pigura OS (alias dalam Bahasa Indonesia)
 * ============================================================================
 */
#define VGA_HITAM               VGA_WARNA_HITAM
#define VGA_BIRU                VGA_WARNA_BIRU
#define VGA_HIJAU               VGA_WARNA_HIJAU
#define VGA_CYAN                VGA_WARNA_CYAN
#define VGA_MERAH               VGA_WARNA_MERAH
#define VGA_MAGENTA             VGA_WARNA_MAGENTA
#define VGA_COKLAT              VGA_WARNA_COKLAT
#define VGA_ABU_ABU             VGA_WARNA_ABU_ABU
#define VGA_ABU_TERANG          VGA_WARNA_ABU_TERANG
#define VGA_BIRU_TERANG         VGA_WARNA_BIRU_TERANG
#define VGA_HIJAU_TERANG        VGA_WARNA_HIJAU_TERANG
#define VGA_CYAN_TERANG         VGA_WARNA_CYAN_TERANG
#define VGA_MERAH_TERANG        VGA_WARNA_MERAH_TERANG
#define VGA_MAGENTA_TER         VGA_WARNA_MAGENTA_TER
#define VGA_KUNING              VGA_WARNA_KUNING
#define VGA_PUTIH               VGA_WARNA_PUTIH

/* Escape sequence untuk warna ANSI */
#define ANSI_ESCAPE             0x1B
#define ANSI_RESET              "[0m"
#define ANSI_WARNA_DEPAN(n)     "[3"#n"m"
#define ANSI_WARNA_LATAR(n)     "[4"#n"m"

/* =============================================================================
 * STRUKTUR DATA
 * =============================================================================
 */

/* Entry VGA (karakter + atribut) */
struct vga_entry {
    char karakter;
    uint8_t atribut;
} __attribute__((packed));

/* State console */
struct console_state {
    int baris;                          /* Posisi baris cursor */
    int kolom;                          /* Posisi kolom cursor */
    uint8_t warna_depan;                /* Warna foreground */
    uint8_t warna_latar;                /* Warna background */
    uint8_t atribut;                    /* Atribut VGA */
    struct vga_entry *buffer;           /* Pointer ke VGA buffer */
};

/* =============================================================================
 * VARIABEL GLOBAL
 * =============================================================================
 */

static struct console_state g_console;

/* =============================================================================
 * FUNGSI INTERNAL
 * =============================================================================
 */

/*
 * _outb
 * -----
 * Menulis byte ke port I/O.
 */
static inline void _outb(uint8_t nilai, uint16_t port)
{
    __asm__ __volatile__(
        "outb %0, %1\n\t"
        :
        : "a"(nilai), "Nd"(port)
    );
}

/*
 * _inb
 * ----
 * Membaca byte dari port I/O.
 */
static inline uint8_t _inb(uint16_t port)
{
    uint8_t nilai;
    __asm__ __volatile__(
        "inb %1, %0\n\t"
        : "=a"(nilai)
        : "Nd"(port)
    );
    return nilai;
}

/*
 * _buat_atribut
 * -------------
 * Membuat byte atribut VGA dari warna depan dan latar.
 *
 * Parameter:
 *   depan - Warna foreground (0-15)
 *   latar - Warna background (0-7)
 *
 * Return:
 *   Byte atribut VGA
 */
static inline uint8_t _buat_atribut(uint8_t depan, uint8_t latar)
{
    return (latar << 4) | (depan & 0x0F);
}

/*
 * _update_cursor_hardware
 * -----------------------
 * Memperbarui posisi cursor hardware VGA.
 *
 * Parameter:
 *   baris - Posisi baris (0-24)
 *   kolom - Posisi kolom (0-79)
 */
static void _update_cursor_hardware(int baris, int kolom)
{
    uint16_t posisi;

    posisi = (uint16_t)(baris * VGA_LEBAR + kolom);

    /* Set cursor low byte */
    _outb(VGA_REG_CURSOR_LOK, VGA_PORT_CTRL);
    _outb((uint8_t)(posisi & 0xFF), VGA_PORT_DATA);

    /* Set cursor high byte */
    _outb(VGA_REG_CURSOR_LOKASI, VGA_PORT_CTRL);
    _outb((uint8_t)((posisi >> 8) & 0xFF), VGA_PORT_DATA);
}

/*
 * _get_cursor_hardware
 * --------------------
 * Mendapatkan posisi cursor hardware VGA.
 *
 * Return:
 *   Posisi linear cursor (0-1999)
 */
static uint16_t _get_cursor_hardware(void)
{
    uint16_t posisi;

    _outb(VGA_REG_CURSOR_LOK, VGA_PORT_CTRL);
    posisi = (uint16_t)_inb(VGA_PORT_DATA);

    _outb(VGA_REG_CURSOR_LOKASI, VGA_PORT_CTRL);
    posisi |= ((uint16_t)_inb(VGA_PORT_DATA) << 8);

    return posisi;
}

/*
 * _putchar_di_posisi
 * ------------------
 * Menulis karakter di posisi tertentu tanpa mengubah cursor.
 *
 * Parameter:
 *   baris - Posisi baris
 *   kolom - Posisi kolom
 *   c     - Karakter ASCII
 */
static void _putchar_di_posisi(int baris, int kolom, char c)
{
    int index;

    if (baris < 0 || baris >= VGA_TINGGI || kolom < 0 || kolom >= VGA_LEBAR) {
        return;
    }

    index = baris * VGA_LEBAR + kolom;
    g_console.buffer[index].karakter = c;
    g_console.buffer[index].atribut = g_console.atribut;
}

/*
 * _scroll
 * -------
 * Menggulir layar ke atas satu baris.
 */
static void _scroll(void)
{
    int i;
    int index_sumber;
    int index_tujuan;
    struct vga_entry *buffer;

    buffer = g_console.buffer;

    /* Copy baris 1-24 ke baris 0-23 */
    for (i = 0; i < VGA_LEBAR * (VGA_TINGGI - 1); i++) {
        index_sumber = i + VGA_LEBAR;
        index_tujuan = i;

        buffer[index_tujuan].karakter = buffer[index_sumber].karakter;
        buffer[index_tujuan].atribut = buffer[index_sumber].atribut;
    }

    /* Clear baris terakhir */
    for (i = 0; i < VGA_LEBAR; i++) {
        index_tujuan = (VGA_TINGGI - 1) * VGA_LEBAR + i;

        buffer[index_tujuan].karakter = ' ';
        buffer[index_tujuan].atribut = g_console.atribut;
    }

    /* Update cursor ke baris terakhir */
    g_console.baris = VGA_TINGGI - 1;
    g_console.kolom = 0;
}

/*
 * _newline
 * --------
 * Menangani karakter newline.
 */
static void _newline(void)
{
    g_console.kolom = 0;
    g_console.baris++;

    if (g_console.baris >= VGA_TINGGI) {
        _scroll();
    }
}

/*
 * _tab
 * -----
 * Menangani karakter tab (8 spasi).
 */
static void _tab(void)
{
    int i;
    int spasi;

    spasi = 8 - (g_console.kolom % 8);

    for (i = 0; i < spasi; i++) {
        _putchar_di_posisi(g_console.baris, g_console.kolom, ' ');
        g_console.kolom++;

        if (g_console.kolom >= VGA_LEBAR) {
            _newline();
            break;
        }
    }
}

/*
 * _backspace
 * ----------
 * Menangani karakter backspace.
 */
static void _backspace(void)
{
    if (g_console.kolom > 0) {
        g_console.kolom--;
    } else if (g_console.baris > 0) {
        g_console.baris--;
        g_console.kolom = VGA_LEBAR - 1;
    }

    _putchar_di_posisi(g_console.baris, g_console.kolom, ' ');
}

/*
 * _carriage_return
 * ----------------
 * Menangani karakter carriage return.
 */
static void _carriage_return(void)
{
    g_console.kolom = 0;
}

/* =============================================================================
 * FUNGSI PUBLIC
 * =============================================================================
 */

/*
 * console_init
 * ------------
 * Menginisialisasi console VGA.
 *
 * Return:
 *   0 jika berhasil
 */
int console_init(void)
{
    int i;

    /* Setup buffer VGA */
    g_console.buffer = (struct vga_entry *)VGA_BUFFER_ALAMAT;

    /* Set warna default */
    g_console.warna_depan = VGA_PUTIH;
    g_console.warna_latar = VGA_HITAM;
    g_console.atribut = _buat_atribut(g_console.warna_depan,
                                       g_console.warna_latar);

    /* Baca posisi cursor dari hardware */
    uint16_t posisi = _get_cursor_hardware();
    g_console.baris = posisi / VGA_LEBAR;
    g_console.kolom = posisi % VGA_LEBAR;

    /* Clear layar */
    for (i = 0; i < VGA_JUMLAH_SEL; i++) {
        g_console.buffer[i].karakter = ' ';
        g_console.buffer[i].atribut = g_console.atribut;
    }

    /* Reset cursor ke posisi 0,0 */
    g_console.baris = 0;
    g_console.kolom = 0;
    _update_cursor_hardware(g_console.baris, g_console.kolom);

    return 0;
}

/*
 * console_putchar
 * ---------------
 * Menampilkan satu karakter ke console.
 *
 * Parameter:
 *   c - Karakter yang ditampilkan
 *
 * Return:
 *   Karakter yang ditampilkan, atau EOF jika error
 */
int console_putchar(int c)
{
    char ch;

    ch = (char)c;

    /* Tangani karakter khusus */
    switch (ch) {
        case '\n':
            _newline();
            break;

        case '\r':
            _carriage_return();
            break;

        case '\t':
            _tab();
            break;

        case '\b':
            _backspace();
            break;

        default:
            /* Karakter normal */
            _putchar_di_posisi(g_console.baris, g_console.kolom, ch);
            g_console.kolom++;

            if (g_console.kolom >= VGA_LEBAR) {
                _newline();
            }
            break;
    }

    /* Update cursor hardware */
    _update_cursor_hardware(g_console.baris, g_console.kolom);

    return c;
}

/*
 * console_puts
 * ------------
 * Menampilkan string null-terminated ke console.
 *
 * Parameter:
 *   str - String yang ditampilkan
 *
 * Return:
 *   Jumlah karakter yang ditampilkan, atau -1 jika error
 */
int console_puts(const char *str)
{
    int jumlah;

    if (str == (const char *)0) {
        return -1;
    }

    jumlah = 0;

    while (*str != '\0') {
        console_putchar((int)*str);
        str++;
        jumlah++;
    }

    return jumlah;
}

/*
 * console_write
 * -------------
 * Menampilkan buffer ke console.
 *
 * Parameter:
 *   buffer - Buffer yang ditampilkan
 *   ukuran - Jumlah byte yang ditampilkan
 *
 * Return:
 *   Jumlah byte yang ditampilkan
 */
int console_write(const char *buffer, int ukuran)
{
    int i;

    if (buffer == (const char *)0 || ukuran < 0) {
        return -1;
    }

    for (i = 0; i < ukuran; i++) {
        console_putchar((int)buffer[i]);
    }

    return ukuran;
}

/*
 * console_clear
 * -------------
 * Mengosongkan layar console.
 */
void console_clear(void)
{
    int i;

    for (i = 0; i < VGA_JUMLAH_SEL; i++) {
        g_console.buffer[i].karakter = ' ';
        g_console.buffer[i].atribut = g_console.atribut;
    }

    g_console.baris = 0;
    g_console.kolom = 0;
    _update_cursor_hardware(g_console.baris, g_console.kolom);
}

/*
 * console_set_color
 * -----------------
 * Mengatur warna teks console.
 *
 * Parameter:
 *   depan - Warna foreground (0-15)
 *   latar - Warna background (0-7)
 */
void console_set_color(uint8_t depan, uint8_t latar)
{
    g_console.warna_depan = depan & 0x0F;
    g_console.warna_latar = latar & 0x07;
    g_console.atribut = _buat_atribut(g_console.warna_depan,
                                       g_console.warna_latar);
}

/*
 * console_set_cursor
 * ------------------
 * Mengatur posisi cursor.
 *
 * Parameter:
 *   baris - Posisi baris (0-24)
 *   kolom - Posisi kolom (0-79)
 *
 * Return:
 *   0 jika berhasil, -1 jika posisi tidak valid
 */
int console_set_cursor(int baris, int kolom)
{
    if (baris < 0 || baris >= VGA_TINGGI ||
        kolom < 0 || kolom >= VGA_LEBAR) {
        return -1;
    }

    g_console.baris = baris;
    g_console.kolom = kolom;
    _update_cursor_hardware(g_console.baris, g_console.kolom);

    return 0;
}

/*
 * console_get_cursor
 * ------------------
 * Mendapatkan posisi cursor.
 *
 * Parameter:
 *   baris - Pointer untuk menyimpan posisi baris
 *   kolom - Pointer untuk menyimpan posisi kolom
 */
void console_get_cursor(int *baris, int *kolom)
{
    if (baris != (int *)0) {
        *baris = g_console.baris;
    }

    if (kolom != (int *)0) {
        *kolom = g_console.kolom;
    }
}

/*
 * console_get_size
 * ----------------
 * Mendapatkan ukuran console.
 *
 * Parameter:
 *   lebar  - Pointer untuk menyimpan lebar
 *   tinggi - Pointer untuk menyimpan tinggi
 */
void console_get_size(int *lebar, int *tinggi)
{
    if (lebar != (int *)0) {
        *lebar = VGA_LEBAR;
    }

    if (tinggi != (int *)0) {
        *tinggi = VGA_TINGGI;
    }
}

/*
 * console_show_cursor
 * -------------------
 * Menampilkan cursor.
 */
void console_show_cursor(void)
{
    uint8_t nilai;

    _outb(VGA_REG_CURSOR_AWAL, VGA_PORT_CTRL);
    nilai = _inb(VGA_PORT_DATA);
    nilai &= 0xC0;                     /* Clear bits 0-5 */
    nilai |= 0x00;                     /* Scan line start */
    _outb(nilai, VGA_PORT_DATA);

    _outb(VGA_REG_CURSOR_AKHIR, VGA_PORT_CTRL);
    nilai = _inb(VGA_PORT_DATA);
    nilai &= 0xE0;                     /* Clear bits 0-4 */
    nilai |= 0x0F;                     /* Scan line end */
    _outb(nilai, VGA_PORT_DATA);
}

/*
 * console_hide_cursor
 * -------------------
 * Menyembunyikan cursor.
 */
void console_hide_cursor(void)
{
    _outb(VGA_REG_CURSOR_AWAL, VGA_PORT_CTRL);
    _outb(0x20, VGA_PORT_DATA);         /* Bit 5 = 1 disables cursor */
}
