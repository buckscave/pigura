/*
 * PIGURA LIBC - CTYPE.H
 * ======================
 * Fungsi klasifikasi dan konversi karakter sesuai standar
 * C89/C90.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 */

#ifndef LIBC_CTYPE_H
#define LIBC_CTYPE_H

/* ============================================================
 * DEKLARASI FUNGSI KLASIFIKASI KARAKTER
 * ============================================================
 * Semua fungsi menerima int yang nilainya harus EOF atau
 * representable sebagai unsigned char. Perilaku tidak
 * terdefinisi jika argumen tidak valid.
 */

/*
 * isalnum - Cek apakah karakter alfanumerik
 * Mengembalikan non-zero jika c adalah huruf atau digit.
 */
extern int isalnum(int c);

/*
 * isalpha - Cek apakah karakter huruf
 * Mengembalikan non-zero jika c adalah huruf (A-Z atau a-z).
 */
extern int isalpha(int c);

/*
 * isblank - Cek apakah karakter blank
 * Mengembalikan non-zero jika c adalah space atau tab.
 * (C99, disertakan untuk kompatibilitas)
 */
extern int isblank(int c);

/*
 * iscntrl - Cek apakah karakter kontrol
 * Mengembalikan non-zero jika c adalah karakter kontrol
 * (0x00-0x1F atau 0x7F).
 */
extern int iscntrl(int c);

/*
 * isdigit - Cek apakah karakter digit desimal
 * Mengembalikan non-zero jika c adalah digit (0-9).
 */
extern int isdigit(int c);

/*
 * isgraph - Cek apakah karakter grafis
 * Mengembalikan non-zero jika c adalah karakter yang
 * memiliki representasi grafis (bukan space dan kontrol).
 */
extern int isgraph(int c);

/*
 * islower - Cek apakah karakter huruf kecil
 * Mengembalikan non-zero jika c adalah huruf kecil (a-z).
 */
extern int islower(int c);

/*
 * isprint - Cek apakah karakter dapat dicetak
 * Mengembalikan non-zero jika c adalah karakter yang dapat
 * dicetak termasuk space (0x20-0x7E).
 */
extern int isprint(int c);

/*
 * ispunct - Cek apakah karakter tanda baca
 * Mengembalikan non-zero jika c adalah karakter tanda baca
 * (bukan alnum, space, atau kontrol).
 */
extern int ispunct(int c);

/*
 * isspace - Cek apakah karakter whitespace
 * Mengembalikan non-zero jika c adalah whitespace:
 * space, form-feed, newline, carriage-return, tab, vertical-tab
 */
extern int isspace(int c);

/*
 * isupper - Cek apakah karakter huruf besar
 * Mengembalikan non-zero jika c adalah huruf besar (A-Z).
 */
extern int isupper(int c);

/*
 * isxdigit - Cek apakah karakter digit heksadesimal
 * Mengembalikan non-zero jika c adalah digit hex (0-9, A-F, a-f).
 */
extern int isxdigit(int c);

/* ============================================================
 * DEKLARASI FUNGSI KONVERSI KARAKTER
 * ============================================================
 */

/*
 * tolower - Konversi ke huruf kecil
 * Jika c adalah huruf besar, mengembalikan huruf kecilnya.
 * Jika bukan, mengembalikan c tanpa perubahan.
 */
extern int tolower(int c);

/*
 * toupper - Konversi ke huruf besar
 * Jika c adalah huruf kecil, mengembalikan huruf besarnya.
 * Jika bukan, mengembalikan c tanpa perubahan.
 */
extern int toupper(int c);

/* ============================================================
 * KONSTANTA KARAKTER
 * ============================================================
 */

/* EOF - End of File */
#define EOF (-1)

/* Kode karakter kontrol standar */
#define NUL   0x00   /* Null character */
#define SOH   0x01   /* Start of heading */
#define STX   0x02   /* Start of text */
#define ETX   0x03   /* End of text */
#define EOT   0x04   /* End of transmission */
#define ENQ   0x05   /* Enquiry */
#define ACK   0x06   /* Acknowledge */
#define BEL   0x07   /* Bell (alert) */
#define BS    0x08   /* Backspace */
#define HT    0x09   /* Horizontal tab */
#define LF    0x0A   /* Line feed (newline) */
#define VT    0x0B   /* Vertical tab */
#define FF    0x0C   /* Form feed */
#define CR    0x0D   /* Carriage return */
#define SO    0x0E   /* Shift out */
#define SI    0x0F   /* Shift in */
#define DLE   0x10   /* Data link escape */
#define DC1   0x11   /* Device control 1 (XON) */
#define DC2   0x12   /* Device control 2 */
#define DC3   0x13   /* Device control 3 (XOFF) */
#define DC4   0x14   /* Device control 4 */
#define NAK   0x15   /* Negative acknowledge */
#define SYN   0x16   /* Synchronous idle */
#define ETB   0x17   /* End of transmission block */
#define CAN   0x18   /* Cancel */
#define EM    0x19   /* End of medium */
#define SUB   0x1A   /* Substitute */
#define ESC   0x1B   /* Escape */
#define FS    0x1C   /* File separator */
#define GS    0x1D   /* Group separator */
#define RS    0x1E   /* Record separator */
#define US    0x1F   /* Unit separator */
#define DEL   0x7F   /* Delete */

/* Karakter whitespace */
#define SPACE 0x20   /* Space */
#define TAB   0x09   /* Tab */

/* ============================================================
 * MAKRO UNTUK AKSES CEPAT (INLINE)
 * ============================================================
 * Makro ini dapat digunakan untuk performa lebih baik tanpa
 * overhead pemanggilan fungsi.
 */

/* Mask untuk klasifikasi karakter */
#define _CTYPE_UPPER   0x01
#define _CTYPE_LOWER   0x02
#define _CTYPE_DIGIT   0x04
#define _CTYPE_SPACE   0x08
#define _CTYPE_PUNCT   0x10
#define _CTYPE_CNTRL   0x20
#define _CTYPE_BLANK   0x40
#define _CTYPE_XDIGIT  0x80

/* Tabel klasifikasi karakter (didefinisikan di ctype.c) */
extern const unsigned char _ctype_table[256];

/* Makro versi inline untuk klasifikasi */
#define _isalnum(c)  \
    (_ctype_table[(unsigned char)(c)] & (_CTYPE_UPPER | _CTYPE_LOWER | _CTYPE_DIGIT))
#define _isalpha(c)  \
    (_ctype_table[(unsigned char)(c)] & (_CTYPE_UPPER | _CTYPE_LOWER))
#define _isblank(c)  \
    (_ctype_table[(unsigned char)(c)] & _CTYPE_BLANK)
#define _iscntrl(c)  \
    (_ctype_table[(unsigned char)(c)] & _CTYPE_CNTRL)
#define _isdigit(c)  \
    (_ctype_table[(unsigned char)(c)] & _CTYPE_DIGIT)
#define _isgraph(c)  \
    (_ctype_table[(unsigned char)(c)] & \
     (_CTYPE_UPPER | _CTYPE_LOWER | _CTYPE_DIGIT | _CTYPE_PUNCT))
#define _islower(c)  \
    (_ctype_table[(unsigned char)(c)] & _CTYPE_LOWER)
#define _isprint(c)  \
    (_ctype_table[(unsigned char)(c)] & \
     (_CTYPE_UPPER | _CTYPE_LOWER | _CTYPE_DIGIT | _CTYPE_PUNCT | _CTYPE_BLANK))
#define _ispunct(c)  \
    (_ctype_table[(unsigned char)(c)] & _CTYPE_PUNCT)
#define _isspace(c)  \
    (_ctype_table[(unsigned char)(c)] & _CTYPE_SPACE)
#define _isupper(c)  \
    (_ctype_table[(unsigned char)(c)] & _CTYPE_UPPER)
#define _isxdigit(c) \
    (_ctype_table[(unsigned char)(c)] & _CTYPE_XDIGIT)

/* ============================================================
 * FUNGSI TAMBAHAN (NON-STANDAR)
 * ============================================================
 */

/*
 * isascii - Cek apakah karakter ASCII
 * Mengembalikan non-zero jika c dalam rentang 0-127.
 */
extern int isascii(int c);

/*
 * toascii - Konversi ke ASCII
 * Menghapus bit ke-8, menghasilkan nilai 0-127.
 */
extern int toascii(int c);

/* Makro versi inline */
#define _isascii(c)  (((c) & ~0x7F) == 0)
#define _toascii(c)  ((c) & 0x7F)

#endif /* LIBC_CTYPE_H */
