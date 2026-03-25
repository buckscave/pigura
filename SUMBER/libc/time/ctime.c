/*
 * PIGURA LIBC - TIME/CTIME.C
 * ==========================
 * Implementasi fungsi formatting waktu dasar.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 *
 * Fungsi yang diimplementasikan:
 *   - asctime()    : Konversi tm ke string (non-reentrant)
 *   - asctime_r()  : Konversi tm ke string (reentrant)
 *   - ctime()      : Konversi time_t ke string (non-reentrant)
 *   - ctime_r()    : Konversi time_t ke string (reentrant)
 */

#include <time.h>
#include <string.h>

/* ============================================================
 * KONSTANTA
 * ============================================================
 */

/* Nama hari singkat (Sun, Mon, ..., Sat) */
static const char *wday_abbr[7] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

/* Nama bulan singkat (Jan, Feb, ..., Dec) */
static const char *mon_abbr[12] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

/* Ukuran buffer asctime: "Wed Jun 30 21:49:08 1993\n\0" = 26 bytes */
#define ASCTIME_BUFFER_SIZE 26

/* ============================================================
 * VARIABEL GLOBAL
 * ============================================================
 */

/* Buffer statis untuk asctime() dan ctime() (non-reentrant) */
static char asctime_buffer[ASCTIME_BUFFER_SIZE];

/* ============================================================
 * FUNGSI HELPER INTERNAL
 * ============================================================
 */

/*
 * format_number_2digit - Format angka 2 digit dengan leading zero
 *
 * Parameter:
 *   num   - Angka yang akan diformat
 *   buf   - Buffer tujuan (minimal 3 karakter)
 */
static void format_number_2digit(int num, char *buf) {
    buf[0] = '0' + (num / 10);
    buf[1] = '0' + (num % 10);
    buf[2] = '\0';
}

/*
 * format_number_4digit - Format angka 4 digit dengan leading zero
 *
 * Parameter:
 *   num   - Angka yang akan diformat
 *   buf   - Buffer tujuan (minimal 5 karakter)
 */
static void format_number_4digit(int num, char *buf) {
    int temp;
    int i;

    /* Handle tahun negatif (sebelum 0) */
    if (num < 0) {
        buf[0] = '-';
        num = -num;
        i = 1;
    } else {
        i = 0;
    }

    /* Hitung digit */
    temp = num;
    
    /* Tulis digit dari belakang */
    buf[i + 3] = '0' + (temp % 10);
    temp /= 10;
    buf[i + 2] = '0' + (temp % 10);
    temp /= 10;
    buf[i + 1] = '0' + (temp % 10);
    temp /= 10;
    buf[i] = '0' + (temp % 10);
    
    buf[i + 4] = '\0';
}

/*
 * validate_tm - Validasi struktur tm
 *
 * Parameter:
 *   tm - Pointer ke struct tm
 *
 * Return: 0 jika valid, -1 jika tidak valid
 */
static int validate_tm(const struct tm *tm) {
    if (tm == NULL) {
        return -1;
    }

    /* Validasi range */
    if (tm->tm_wday < 0 || tm->tm_wday > 6) {
        return -1;
    }
    if (tm->tm_mon < 0 || tm->tm_mon > 11) {
        return -1;
    }
    if (tm->tm_mday < 1 || tm->tm_mday > 31) {
        return -1;
    }
    if (tm->tm_hour < 0 || tm->tm_hour > 23) {
        return -1;
    }
    if (tm->tm_min < 0 || tm->tm_min > 59) {
        return -1;
    }
    if (tm->tm_sec < 0 || tm->tm_sec > 60) { /* 60 untuk leap second */
        return -1;
    }

    return 0;
}

/* ============================================================
 * FUNGSI ASCTIME
 * ============================================================
 */

/*
 * asctime_r - Konversi struct tm ke string (reentrant)
 *
 * Parameter:
 *   tm     - Pointer ke struct tm
 *   buf    - Buffer tujuan (minimal 26 bytes)
 *
 * Return: Pointer ke buf, atau NULL jika error
 *
 * Format output: "Wed Jun 30 21:49:08 1993\n\0"
 *               "Www Mmm dd hh:mm:ss yyyy\n"
 *
 * Catatan: Buffer harus minimal 26 bytes. Output selalu
 *          24 karakter + newline + NUL.
 */
char *asctime_r(const struct tm *tm, char *buf) {
    const char *wday;
    const char *mon;
    char num_buf[5];

    /* Validasi parameter */
    if (buf == NULL) {
        return NULL;
    }

    if (validate_tm(tm) != 0) {
        buf[0] = '\0';
        return NULL;
    }

    /* Dapatkan nama hari dan bulan */
    wday = wday_abbr[tm->tm_wday];
    mon = mon_abbr[tm->tm_mon];

    /* Format: "Www Mmm dd hh:mm:ss yyyy\n" */
    /* Contoh: "Wed Jun 30 21:49:08 1993\n" */

    /* Hari (3 karakter) + spasi */
    buf[0] = wday[0];
    buf[1] = wday[1];
    buf[2] = wday[2];
    buf[3] = ' ';

    /* Bulan (3 karakter) + spasi */
    buf[4] = mon[0];
    buf[5] = mon[1];
    buf[6] = mon[2];
    buf[7] = ' ';

    /* Tanggal (2 digit dengan leading space) */
    if (tm->tm_mday < 10) {
        buf[8] = ' ';
        buf[9] = '0' + tm->tm_mday;
    } else {
        format_number_2digit(tm->tm_mday, num_buf);
        buf[8] = num_buf[0];
        buf[9] = num_buf[1];
    }
    buf[10] = ' ';

    /* Jam (2 digit) */
    format_number_2digit(tm->tm_hour, num_buf);
    buf[11] = num_buf[0];
    buf[12] = num_buf[1];
    buf[13] = ':';

    /* Menit (2 digit) */
    format_number_2digit(tm->tm_min, num_buf);
    buf[14] = num_buf[0];
    buf[15] = num_buf[1];
    buf[16] = ':';

    /* Detik (2 digit) */
    format_number_2digit(tm->tm_sec, num_buf);
    buf[17] = num_buf[0];
    buf[18] = num_buf[1];
    buf[19] = ' ';

    /* Tahun (4 digit) */
    format_number_4digit(tm->tm_year + 1900, num_buf);
    buf[20] = num_buf[0];
    buf[21] = num_buf[1];
    buf[22] = num_buf[2];
    buf[23] = num_buf[3];

    /* Newline dan NUL terminator */
    buf[24] = '\n';
    buf[25] = '\0';

    return buf;
}

/*
 * asctime - Konversi struct tm ke string (non-reentrant)
 *
 * Parameter:
 *   tm - Pointer ke struct tm
 *
 * Return: Pointer ke string, atau NULL jika error
 *
 * Peringatan: Tidak reentrant. Buffer internal digunakan.
 *             Gunakan asctime_r() untuk aplikasi multi-threaded.
 */
char *asctime(const struct tm *tm) {
    return asctime_r(tm, asctime_buffer);
}

/* ============================================================
 * FUNGSI CTIME
 * ============================================================
 */

/*
 * ctime_r - Konversi time_t ke string (reentrant)
 *
 * Parameter:
 *   timer - Pointer ke time_t
 *   buf   - Buffer tujuan (minimal 26 bytes)
 *
 * Return: Pointer ke buf, atau NULL jika error
 *
 * Catatan: Ekuivalen dengan asctime_r(localtime_r(timer, &tm), buf)
 */
char *ctime_r(const time_t *timer, char *buf) {
    struct tm tm_result;

    /* Validasi parameter */
    if (timer == NULL || buf == NULL) {
        return NULL;
    }

    /* Konversi ke tm lokal */
    if (localtime_r(timer, &tm_result) == NULL) {
        return NULL;
    }

    /* Konversi ke string */
    return asctime_r(&tm_result, buf);
}

/*
 * ctime - Konversi time_t ke string (non-reentrant)
 *
 * Parameter:
 *   timer - Pointer ke time_t
 *
 * Return: Pointer ke string, atau NULL jika error
 *
 * Peringatan: Tidak reentrant. Buffer internal digunakan.
 *             Gunakan ctime_r() untuk aplikasi multi-threaded.
 *
 * Catatan: Ekuivalen dengan asctime(localtime(timer))
 */
char *ctime(const time_t *timer) {
    struct tm *tm_ptr;

    /* Validasi parameter */
    if (timer == NULL) {
        return NULL;
    }

    /* Konversi ke tm lokal */
    tm_ptr = localtime(timer);
    if (tm_ptr == NULL) {
        return NULL;
    }

    /* Konversi ke string */
    return asctime(tm_ptr);
}

/* ============================================================
 * FUNGSI INTERNAL
 * ============================================================
 */

/*
 * __asctime_format - Format waktu ke buffer kustom
 *
 * Parameter:
 *   tm    - Pointer ke struct tm
 *   buf   - Buffer tujuan
 *   size  - Ukuran buffer
 *
 * Return: 0 jika berhasil, -1 jika error
 *
 * Catatan: Fungsi internal untuk formatting fleksibel.
 */
int __asctime_format(const struct tm *tm, char *buf, size_t size) {
    char temp[ASCTIME_BUFFER_SIZE];

    if (buf == NULL || size < ASCTIME_BUFFER_SIZE) {
        return -1;
    }

    if (asctime_r(tm, temp) == NULL) {
        return -1;
    }

    /* Copy tanpa newline jika buffer lebih besar */
    strncpy(buf, temp, size - 1);
    buf[size - 1] = '\0';

    return 0;
}

/*
 * __ctime_format - Format time_t ke buffer kustom
 *
 * Parameter:
 *   timer - Pointer ke time_t
 *   buf   - Buffer tujuan
 *   size  - Ukuran buffer
 *
 * Return: 0 jika berhasil, -1 jika error
 */
int __ctime_format(const time_t *timer, char *buf, size_t size) {
    char temp[ASCTIME_BUFFER_SIZE];

    if (buf == NULL || size < ASCTIME_BUFFER_SIZE) {
        return -1;
    }

    if (ctime_r(timer, temp) == NULL) {
        return -1;
    }

    strncpy(buf, temp, size - 1);
    buf[size - 1] = '\0';

    return 0;
}
