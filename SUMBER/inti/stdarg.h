/*
 * PIGURA OS - STDARG.H
 * =====================
 * Definisi makro untuk akses argumen variadic.
 *
 * Berkas ini menyediakan makro standar C90 untuk mengakses argumen
 * fungsi dengan jumlah parameter yang tidak tetap (variadic functions).
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Tidak menggunakan fitur C99/C11
 * - Semua makro didefinisikan secara eksplisit
 *
 * Versi: 2.0
 * Tanggal: 2025
 */

#ifndef INTI_STDARG_H
#define INTI_STDARG_H

/* Sertakan types.h untuk definisi tipe dasar */
#include "types.h"

/*
 * ===========================================================================
 * TIPE VA_LIST (VA_LIST TYPE)
 * ===========================================================================
 * Tipe untuk menyimpan informasi tentang argumen variadic.
 * Implementasi bergantung pada arsitektur dan calling convention.
 */

/* Gunakan built-in GCC untuk portabilitas yang lebih baik */
typedef __builtin_va_list va_list;

/*
 * ===========================================================================
 * MAKRO VA_START (VA_START MACRO)
 * ===========================================================================
 * Inisialisasi va_list untuk mulai mengakses argumen variadic.
 *
 * Parameter:
 *   ap       - Objek va_list yang akan diinisialisasi
 *   terakhir - Nama parameter terakhir sebelum argumen variadic
 *
 * Penggunaan:
 *   void fungsi(int tetap, ...)
 *   {
 *       va_list ap;
 *       va_mulai(ap, tetap);
 *       ...
 *   }
 */

#define va_mulai(ap, terakhir) __builtin_va_start(ap, terakhir)

/*
 * ===========================================================================
 * MAKRO VA_ARG (VA_ARG MACRO)
 * ===========================================================================
 * Mengakses argumen berikutnya dari daftar variadic.
 *
 * Parameter:
 *   ap   - Objek va_list yang sudah diinisialisasi
 *   tipe - Tipe dari argumen yang akan diakses
 *
 * Return:
 *   Nilai argumen berikutnya dengan tipe yang ditentukan
 *
 * Catatan:
 *   - Tipe harus kompatibel dengan argumen yang sebenarnya
 *   - Tipe yang diperluas secara otomatis (char->int, float->double)
 *
 * Penggunaan:
 *   int nilai = va_ambil(ap, int);
 *   char *str = va_ambil(ap, char *);
 */

#define va_ambil(ap, tipe) __builtin_va_arg(ap, tipe)

/*
 * ===========================================================================
 * MAKRO VA_END (VA_END MACRO)
 * ===========================================================================
 * Membersihkan objek va_list setelah penggunaan.
 *
 * Parameter:
 *   ap - Objek va_list yang akan dibersihkan
 *
 * Catatan:
 *   - WAJIB dipanggil setelah selesai menggunakan va_list
 *   - Tidak memanggil va_end menyebabkan undefined behavior
 *
 * Penggunaan:
 *   va_selesai(ap);
 */

#define va_selesai(ap) __builtin_va_end(ap)

/*
 * ===========================================================================
 * MAKRO VA_COPY (VA_COPY MACRO)
 * ===========================================================================
 * Membuat salinan dari objek va_list.
 *
 * Parameter:
 *   dest - Objek va_list tujuan
 *   src  - Objek va_list sumber
 *
 * Catatan:
 *   - va_copy adalah fitur C99, tapi GCC menyediakannya sebagai extension
 *   - Setiap salinan harus di-va_end secara terpisah
 *
 * Penggunaan:
 *   va_list ap, ap_copy;
 *   va_mulai(ap, terakhir);
 *   va_salin(ap_copy, ap);
 *   ...
 *   va_selesai(ap);
 *   va_selesai(ap_copy);
 */

#ifdef __GNUC__
#define va_salin(dest, src) __builtin_va_copy(dest, src)
#else
/* Fallback untuk compiler tanpa va_copy */
#define va_salin(dest, src) ((dest) = (src))
#endif

/*
 * ===========================================================================
 * ALIAS KOMPATIBILITAS (COMPATIBILITY ALIASES)
 * ===========================================================================
 * Alias untuk kompatibilitas dengan kode yang menggunakan nama standar.
 */

#define va_start va_mulai
#define va_arg va_ambil
#define va_end va_selesai
#define va_copy va_salin

/*
 * ===========================================================================
 * MAKRO HELPER (HELPER MACROS)
 * ===========================================================================
 * Makro bantuan untuk operasi variadic yang umum.
 */

/*
 * VA_ARG_OR_DEFAULT
 * -----------------
 * Mengambil argumen atau mengembalikan nilai default jika tidak ada.
 *
 * Parameter:
 *   ap       - Objek va_list
 *   tipe     - Tipe argumen
 *   jumlah   - Jumlah argumen yang sudah diambil
 *   maks     - Jumlah maksimum argumen
 *   default  - Nilai default jika argumen habis
 *
 * Catatan:
 *   - Berguna untuk fungsi dengan argumen opsional
 */
#define VA_ARG_OR_DEFAULT(ap, tipe, jumlah, maks, default_val) \
    ((jumlah) < (maks) ? va_ambil(ap, tipe) : (default_val))

/*
 * VA_COUNT_ARGS
 * -------------
 * Menghitung jumlah argumen variadic (compile-time).
 * Catatan: Hanya bekerja hingga 8 argumen.
 */
#define VA_COUNT_ARGS(...) \
    VA_COUNT_ARGS_IMPL(__VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define VA_COUNT_ARGS_IMPL(_1, _2, _3, _4, _5, _6, _7, _8, n, ...) n

/*
 * ===========================================================================
 * FUNGSI HELPER VARIADIC (VARIADIC HELPER FUNCTIONS)
 * ===========================================================================
 * Deklarasi fungsi helper untuk operasi variadic.
 */

/*
 * va_list_size
 * ------------
 * Mendapatkan ukuran tipe yang telah diperluas untuk variadic.
 *
 * Parameter:
 *   tipe - Tipe data
 *
 * Return:
 *   Ukuran tipe setelah perluasan argumen
 *
 * Catatan:
 *   - char dan short diperluas menjadi int
 *   - float diperluas menjadi double
 */
#define VA_SIZE_PROMOTED(tipe) \
    (sizeof(tipe) < sizeof(int) ? sizeof(int) : sizeof(tipe))

/*
 * ===========================================================================
 * STRUKTUR UNTUK FORMAT STRING (FORMAT STRING STRUCTURES)
 * ===========================================================================
 * Struktur untuk parsing format string printf-like.
 */

/* Struktur informasi format */
typedef struct {
    tanda32_t lebar;        /* Lebar field */
    tanda32_t presisi;      /* Presisi */
    tak_bertanda8_t flag;   /* Flag format */
    tak_bertanda8_t panjang; /* Modifier panjang */
    tak_bertanda8_t tipe;   /* Tipe format */
    tak_bertanda8_t padding; /* Padding */
} format_info_t;

/* Flag format */
#define FORMAT_FLAG_LEFT 0x01   /* Rata kiri (-) */
#define FORMAT_FLAG_PLUS 0x02   /* Tampilkan tanda (+) */
#define FORMAT_FLAG_SPACE 0x04  /* Spasi untuk positif ( ) */
#define FORMAT_FLAG_HASH 0x08   /* Alternate form (#) */
#define FORMAT_FLAG_ZERO 0x10   /* Padding nol (0) */

/* Modifier panjang */
#define FORMAT_LEN_DEFAULT 0
#define FORMAT_LEN_CHAR 1       /* hh */
#define FORMAT_LEN_SHORT 2      /* h */
#define FORMAT_LEN_LONG 3       /* l */
#define FORMAT_LEN_LLONG 4      /* ll */
#define FORMAT_LEN_SIZE 5       /* z */
#define FORMAT_LEN_PTRDIFF 6    /* t */
#define FORMAT_LEN_LONGDBL 7    /* L */

/* Tipe format */
#define FORMAT_TYPE_INT 0       /* d, i */
#define FORMAT_TYPE_UINT 1      /* u */
#define FORMAT_TYPE_OCTAL 2     /* o */
#define FORMAT_TYPE_HEX 3       /* x, X */
#define FORMAT_TYPE_CHAR 4      /* c */
#define FORMAT_TYPE_STRING 5    /* s */
#define FORMAT_TYPE_PTR 6       /* p */
#define FORMAT_TYPE_PERCENT 7   /* %% */
#define FORMAT_TYPE_DOUBLE 8    /* f, e, E, g, G */

/*
 * ===========================================================================
 * DEKLARASI FUNGSI PARSER FORMAT (FORMAT PARSER FUNCTIONS)
 * ===========================================================================
 */

/*
 * format_parse
 * ------------
 * Parse format specifier dari string format.
 *
 * Parameter:
 *   format - String format
 *   pos    - Posisi saat ini (pointer ke %)
 *   info   - Struktur untuk menyimpan hasil parsing
 *
 * Return:
 *   Jumlah karakter yang dikonsumsi, atau negatif jika error
 */
tanda32_t format_parse(const char *format, ukuran_t pos, format_info_t *info);

/*
 * format_get_arg_size
 * -------------------
 * Mendapatkan ukuran argumen berdasarkan format.
 *
 * Parameter:
 *   info - Informasi format
 *
 * Return:
 *   Ukuran argumen dalam byte
 */
ukuran_t format_get_arg_size(const format_info_t *info);

/*
 * ===========================================================================
 * CATATAN IMPLEMENTASI (IMPLEMENTATION NOTES)
 * ===========================================================================
 *
 * 1. CALLING CONVENTION x86 (32-bit):
 *    - Argumen di-push ke stack dari kanan ke kiri
 *    - Argumen variadic diakses berurutan dari stack
 *    - va_list adalah pointer ke stack
 *
 * 2. CALLING CONVENTION x86_64:
 *    - 6 argumen pertama via register (RDI, RSI, RDX, RCX, R8, R9)
 *    - Argumen tambahan via stack
 *    - va_list lebih kompleks (perlu track register dan stack)
 *
 * 3. CALLING CONVENTION ARM (32-bit):
 *    - 4 argumen pertama via register (R0-R3)
 *    - Argumen tambahan via stack
 *
 * 4. CALLING CONVENTION ARM64:
 *    - 8 argumen pertama via register (X0-X7)
 *    - 8 argumen floating-point via register (V0-V7)
 *    - Argumen tambahan via stack
 *
 * 5. TYPE PROMOTION:
 *    - char, short -> int
 *    - unsigned char, unsigned short -> unsigned int
 *    - float -> double
 *    - array -> pointer
 *    - function -> function pointer
 */

#endif /* INTI_STDARG_H */
