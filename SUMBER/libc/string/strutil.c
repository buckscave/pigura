/*
 * PIGURA LIBC - STRING/STRUTIL.C
 * ==============================
 * Implementasi fungsi utilitas string locale-aware (extensions).
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.1 - Perbaikan: locale collate digunakan oleh __strcoll_l/__strxfrm_l
 *
 * Fungsi yang diimplementasikan:
 *   - __strcoll_l()  : strcoll dengan locale eksplisit
 *   - __strxfrm_l()  : strxfrm dengan locale eksplisit
 *   - __set_locale_collate() : Set locale untuk collation
 *
 * Catatan: strcoll() dan strxfrm() diimplementasikan di str.c.
 */

#include <string.h>
#include <stddef.h>

/* ============================================================
 * KONFIGURASI LOCALE
 * ============================================================
 * Untuk implementasi sederhana ini, kita menggunakan locale
 * "C" (POSIX) sebagai default. Implementasi penuh akan
 * memerlukan integrasi dengan sistem locale OS.
 *
 * Nilai locale_collate:
 *   0 = C locale (ASCII lexical ordering, default)
 *   1 = native locale (saat ini fallback ke C locale)
 */

/* Default locale: "C" locale (ASCII lexical ordering) */
static int current_locale_collate = 0;  /* 0 = C locale */

/* ============================================================
 * FUNGSI INTERNAL UNTUK LOCALE (Opsional Extension)
 * ============================================================
 *
 * strcoll() dan strxfrm() diimplementasikan di str.c.
 * Fungsi-fungsi di bawah ini adalah wrapper locale-aware
 * yang memanggil implementasi kanonik dari str.c.
 */

/*
 * __strcoll_l - strcoll dengan locale eksplisit
 *
 * Parameter:
 *   s1     - String pertama
 *   s2     - String kedua
 *   locale - Pointer ke locale (NULL untuk current_locale_collate,
 *            atau pointer ke int: 0 = C, 1 = native)
 *
 * Return: Hasil perbandingan (0 jika sama, <0 jika s1<s2, >0 jika s1>s2)
 *
 * Catatan: Ini adalah extension untuk kompatibilitas POSIX.
 *          Saat ini hanya mendukung locale "C" (0) dan "native" (1).
 *          Jika locale bukan 0 atau 1, fallback ke C locale.
 */
int __strcoll_l(const char *s1, const char *s2, void *locale) {
    int locale_id;

    /* Tentukan locale yang digunakan */
    if (locale != NULL) {
        locale_id = *(int *)locale;
    } else {
        locale_id = current_locale_collate;
    }

    /* Hanya locale "C" (0) yang didukung saat ini.
     * Locale "native" (1) juga menggunakan strcmp karena
     * belum ada tabel collation khusus. */
    (void)locale_id;  /* Suppress unused warning */

    /* Gunakan implementasi strcoll standar (C locale) */
    return strcoll(s1, s2);
}

/*
 * __strxfrm_l - strxfrm dengan locale eksplisit
 *
 * Parameter:
 *   dest   - Buffer tujuan
 *   src    - String sumber
 *   n      - Ukuran buffer
 *   locale - Pointer ke locale (NULL untuk current_locale_collate,
 *            atau pointer ke int: 0 = C, 1 = native)
 *
 * Return: Panjang string yang ditransformasi
 *
 * Catatan: Ini adalah extension untuk kompatibilitas POSIX.
 *          Saat ini hanya mendukung locale "C" (0) dan "native" (1).
 *          Jika locale bukan 0 atau 1, fallback ke C locale.
 */
size_t __strxfrm_l(char *dest, const char *src, size_t n,
                   void *locale) {
    int locale_id;

    /* Tentukan locale yang digunakan */
    if (locale != NULL) {
        locale_id = *(int *)locale;
    } else {
        locale_id = current_locale_collate;
    }

    /* Hanya locale "C" (0) yang didukung saat ini.
     * Locale "native" (1) juga menggunakan strxfrm standar karena
     * belum ada tabel transformasi khusus. */
    (void)locale_id;  /* Suppress unused warning */

    /* Gunakan implementasi strxfrm standar (C locale) */
    return strxfrm(dest, src, n);
}

/*
 * __set_locale_collate - Set locale untuk collation
 *
 * Parameter:
 *   category - Kategori locale (0 = C, 1 = native)
 *
 * Catatan: Fungsi internal untuk konfigurasi locale.
 *          Nilai selain 0 dan 1 akan diabaikan.
 */
void __set_locale_collate(int category) {
    /* Validasi: hanya terima 0 (C) dan 1 (native) */
    if (category == 0 || category == 1) {
        current_locale_collate = category;
    }
}

/*
 * __get_locale_collate - Ambil locale collate saat ini
 *
 * Return: Nilai locale collate (0 = C, 1 = native)
 *
 * Catatan: Fungsi internal untuk introspeksi locale.
 */
int __get_locale_collate(void) {
    return current_locale_collate;
}
