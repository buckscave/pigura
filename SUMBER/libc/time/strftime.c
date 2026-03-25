/*
 * PIGURA LIBC - TIME/STRFTIME.C
 * =============================
 * Implementasi fungsi formatting dan parsing waktu.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 *
 * Fungsi yang diimplementasikan:
 *   - strftime()  : Format waktu ke string
 *   - strptime()  : Parse string ke waktu
 */

#include <time.h>
#include <string.h>
#include <ctype.h>

/* ============================================================
 * KONSTANTA
 * ============================================================
 */

/* Nama hari lengkap */
static const char *weekday_full[7] = {
    "Sunday", "Monday", "Tuesday", "Wednesday",
    "Thursday", "Friday", "Saturday"
};

/* Nama hari singkat */
static const char *weekday_abbr[7] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

/* Nama bulan lengkap */
static const char *month_full[12] = {
    "January", "February", "March", "April",
    "May", "June", "July", "August",
    "September", "October", "November", "December"
};

/* Nama bulan singkat */
static const char *month_abbr[12] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

/* AM/PM */
static const char *ampm[2] = { "AM", "PM" };

/* ============================================================
 * FUNGSI HELPER INTERNAL
 * ============================================================
 */

/*
 * write_number - Tulis angka ke buffer
 *
 * Parameter:
 *   buf   - Buffer tujuan
 *   size  - Sisa ukuran buffer
 *   num   - Angka yang ditulis
 *   width - Lebar minimum (dengan leading zero)
 *   written - Counter karakter yang ditulis
 *
 * Return: Pointer ke posisi berikutnya di buf
 */
static char *write_number(char *buf, size_t size, int num, int width,
                          size_t *written) {
    char temp[12];  /* Cukup untuk 32-bit integer */
    int len = 0;
    int neg = 0;
    int i;
    int digits;

    /* Handle angka negatif */
    if (num < 0) {
        neg = 1;
        num = -num;
    }

    /* Konversi ke string (reversed) */
    if (num == 0) {
        temp[len++] = '0';
    } else {
        while (num > 0) {
            temp[len++] = '0' + (num % 10);
            num /= 10;
        }
    }

    /* Hitung jumlah digit */
    digits = len;

    /* Tulis leading zero atau space */
    if (width > digits) {
        for (i = 0; i < width - digits; i++) {
            if (*written < size - 1) {
                *buf++ = '0';
                (*written)++;
            }
        }
    }

    /* Tulis tanda negatif */
    if (neg && *written < size - 1) {
        *buf++ = '-';
        (*written)++;
    }

    /* Tulis digit (dalam urutan yang benar) */
    for (i = len - 1; i >= 0; i--) {
        if (*written < size - 1) {
            *buf++ = temp[i];
            (*written)++;
        }
    }

    return buf;
}

/*
 * write_string - Tulis string ke buffer
 *
 * Parameter:
 *   buf     - Buffer tujuan
 *   size    - Sisa ukuran buffer
 *   str     - String yang ditulis
 *   written - Counter karakter yang ditulis
 *
 * Return: Pointer ke posisi berikutnya di buf
 */
static char *write_string(char *buf, size_t size, const char *str,
                          size_t *written) {
    while (*str != '\0' && *written < size - 1) {
        *buf++ = *str++;
        (*written)++;
    }
    return buf;
}

/*
 * week_of_year - Hitung minggu dalam tahun
 *
 * Parameter:
 *   tm - Pointer ke struct tm
 *   start - Hari pertama minggu (0 = Minggu, 1 = Senin)
 *
 * Return: Nomor minggu (1-53)
 */
static int week_of_year(const struct tm *tm, int start) {
    int day = tm->tm_yday;
    int wday = tm->tm_wday;
    int week;

    /* Sesuaikan hari pertama minggu */
    if (start == 1) {
        /* Minggu dimulai dari Senin */
        wday = (wday + 6) % 7;
    }

    /* Hitung minggu */
    week = (day + wday) / 7;

    /* Jika minggu pertama tahun lalu */
    if (week == 0) {
        /* Hitung minggu terakhir tahun sebelumnya */
        /* (simplified - return 1 untuk kasus sederhana) */
        return 1;
    }

    return week;
}

/*
 * iso_week_of_year - Hitung minggu ISO
 *
 * Parameter:
 *   tm - Pointer ke struct tm
 *   year - Pointer untuk menyimpan tahun ISO
 *
 * Return: Nomor minggu ISO (1-53)
 */
static int iso_week_of_year(const struct tm *tm, int *year) {
    int week;
    int jan1_wday;
    int doy;

    *year = tm->tm_year + 1900;
    doy = tm->tm_yday;

    /* Hitung hari minggu untuk 1 Januari tahun ini */
    jan1_wday = (tm->tm_wday - doy % 7 + 7) % 7;

    /* Hitung minggu */
    week = (doy + jan1_wday) / 7;

    /* Adjust untuk minggu pertama ISO */
    if (jan1_wday <= 3) {
        week++;
    }

    if (week == 0) {
        /* Minggu terakhir tahun sebelumnya */
        (*year)--;
        week = 52;
        /* Check untuk minggu 53 */
        if (jan1_wday == 4 || (jan1_wday == 3 &&
            is_leap_year(*year - 1))) {
            week = 53;
        }
    } else if (week > 52) {
        /* Cek apakah minggu 53 valid */
        int dec31_wday = (jan1_wday + 364 +
                         (is_leap_year(*year) ? 1 : 0)) % 7;
        if (dec31_wday < 4) {
            week = 1;
            (*year)++;
        }
    }

    return week;
}

/* Helper untuk is_leap_year - didefinisikan di sini untuk kemandirian */
static int is_leap_year_local(int year) {
    return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
}

/* ============================================================
 * STRFTIME - FORMAT WAKTU KE STRING
 * ============================================================
 */

/*
 * strftime - Format waktu ke string
 *
 * Parameter:
 *   s     - Buffer tujuan
 *   maxsize - Ukuran buffer
 *   format - Format string
 *   tm    - Pointer ke struct tm
 *
 * Return: Jumlah karakter yang ditulis (tanpa NUL),
 *         atau 0 jika buffer terlalu kecil
 *
 * Format specifier yang didukung:
 *   %a - Nama hari singkat
 *   %A - Nama hari lengkap
 *   %b - Nama bulan singkat
 *   %B - Nama bulan lengkap
 *   %c - Tanggal dan waktu standar
 *   %C - Abad (tahun / 100)
 *   %d - Hari dalam bulan (01-31)
 *   %D - Tanggal (MM/DD/YY)
 *   %e - Hari dalam bulan ( 1-31)
 *   %F - Tanggal ISO (YYYY-MM-DD)
 *   %g - Tahun ISO 2 digit
 *   %G - Tahun ISO 4 digit
 *   %h - Sama dengan %b
 *   %H - Jam (00-23)
 *   %I - Jam (01-12)
 *   %j - Hari dalam tahun (001-366)
 *   %m - Bulan (01-12)
 *   %M - Menit (00-59)
 *   %n - Newline
 *   %p - AM/PM
 *   %r - Waktu 12 jam (hh:mm:ss AM/PM)
 *   %R - Waktu (HH:MM)
 *   %S - Detik (00-60)
 *   %t - Tab
 *   %T - Waktu (HH:MM:SS)
 *   %u - Hari dalam minggu (1-7, 1=Senin)
 *   %U - Minggu dalam tahun (Minggu pertama)
 *   %V - Minggu ISO
 *   %w - Hari dalam minggu (0-6, 0=Minggu)
 *   %W - Minggu dalam tahun (Senin pertama)
 *   %x - Tanggal standar
 *   %X - Waktu standar
 *   %y - Tahun 2 digit
 *   %Y - Tahun 4 digit
 *   %z - Timezone offset
 *   %Z - Nama timezone
 *   %% - Literal %
 */
size_t strftime(char *s, size_t maxsize, const char *format,
                const struct tm *tm) {
    size_t written = 0;
    char *buf = s;
    int temp_year;
    int temp_week;

    /* Validasi parameter */
    if (s == NULL || format == NULL || tm == NULL) {
        return 0;
    }

    if (maxsize == 0) {
        return 0;
    }

    /* Proses format string */
    while (*format != '\0' && written < maxsize - 1) {
        if (*format != '%') {
            /* Karakter biasa */
            *buf++ = *format++;
            written++;
            continue;
        }

        /* Format specifier */
        format++;

        if (*format == '\0') {
            break;
        }

        switch (*format) {
        case '%':
            /* Literal % */
            if (written < maxsize - 1) {
                *buf++ = '%';
                written++;
            }
            break;

        case 'a':
            /* Nama hari singkat */
            buf = write_string(buf, maxsize,
                              weekday_abbr[tm->tm_wday], &written);
            break;

        case 'A':
            /* Nama hari lengkap */
            buf = write_string(buf, maxsize,
                              weekday_full[tm->tm_wday], &written);
            break;

        case 'b':
        case 'h':
            /* Nama bulan singkat */
            buf = write_string(buf, maxsize,
                              month_abbr[tm->tm_mon], &written);
            break;

        case 'B':
            /* Nama bulan lengkap */
            buf = write_string(buf, maxsize,
                              month_full[tm->tm_mon], &written);
            break;

        case 'c':
            /* Tanggal dan waktu standar */
            /* "Wed Jun 30 21:49:08 1993" */
            buf = write_string(buf, maxsize,
                              weekday_abbr[tm->tm_wday], &written);
            buf = write_string(buf, maxsize, " ", &written);
            buf = write_string(buf, maxsize,
                              month_abbr[tm->tm_mon], &written);
            buf = write_string(buf, maxsize, " ", &written);
            if (tm->tm_mday < 10) {
                if (written < maxsize - 1) {
                    *buf++ = ' ';
                    written++;
                }
                buf = write_number(buf, maxsize, tm->tm_mday,
                                  0, &written);
            } else {
                buf = write_number(buf, maxsize, tm->tm_mday,
                                  0, &written);
            }
            buf = write_string(buf, maxsize, " ", &written);
            buf = write_number(buf, maxsize, tm->tm_hour, 2, &written);
            buf = write_string(buf, maxsize, ":", &written);
            buf = write_number(buf, maxsize, tm->tm_min, 2, &written);
            buf = write_string(buf, maxsize, ":", &written);
            buf = write_number(buf, maxsize, tm->tm_sec, 2, &written);
            buf = write_string(buf, maxsize, " ", &written);
            buf = write_number(buf, maxsize, tm->tm_year + 1900,
                              4, &written);
            break;

        case 'C':
            /* Abad */
            buf = write_number(buf, maxsize,
                              (tm->tm_year + 1900) / 100, 2, &written);
            break;

        case 'd':
            /* Hari dalam bulan (01-31) */
            buf = write_number(buf, maxsize, tm->tm_mday, 2, &written);
            break;

        case 'D':
            /* MM/DD/YY */
            buf = write_number(buf, maxsize, tm->tm_mon + 1, 2, &written);
            buf = write_string(buf, maxsize, "/", &written);
            buf = write_number(buf, maxsize, tm->tm_mday, 2, &written);
            buf = write_string(buf, maxsize, "/", &written);
            buf = write_number(buf, maxsize,
                              (tm->tm_year + 1900) % 100, 2, &written);
            break;

        case 'e':
            /* Hari dalam bulan ( 1-31) */
            if (tm->tm_mday < 10) {
                if (written < maxsize - 1) {
                    *buf++ = ' ';
                    written++;
                }
                buf = write_number(buf, maxsize, tm->tm_mday,
                                  0, &written);
            } else {
                buf = write_number(buf, maxsize, tm->tm_mday,
                                  2, &written);
            }
            break;

        case 'F':
            /* YYYY-MM-DD */
            buf = write_number(buf, maxsize, tm->tm_year + 1900,
                              4, &written);
            buf = write_string(buf, maxsize, "-", &written);
            buf = write_number(buf, maxsize, tm->tm_mon + 1, 2, &written);
            buf = write_string(buf, maxsize, "-", &written);
            buf = write_number(buf, maxsize, tm->tm_mday, 2, &written);
            break;

        case 'g':
            /* Tahun ISO 2 digit */
            iso_week_of_year(tm, &temp_year);
            buf = write_number(buf, maxsize, temp_year % 100, 2, &written);
            break;

        case 'G':
            /* Tahun ISO 4 digit */
            iso_week_of_year(tm, &temp_year);
            buf = write_number(buf, maxsize, temp_year, 4, &written);
            break;

        case 'H':
            /* Jam (00-23) */
            buf = write_number(buf, maxsize, tm->tm_hour, 2, &written);
            break;

        case 'I':
            /* Jam (01-12) */
            {
                int hour12 = tm->tm_hour % 12;
                if (hour12 == 0) hour12 = 12;
                buf = write_number(buf, maxsize, hour12, 2, &written);
            }
            break;

        case 'j':
            /* Hari dalam tahun (001-366) */
            buf = write_number(buf, maxsize, tm->tm_yday + 1, 3, &written);
            break;

        case 'm':
            /* Bulan (01-12) */
            buf = write_number(buf, maxsize, tm->tm_mon + 1, 2, &written);
            break;

        case 'M':
            /* Menit (00-59) */
            buf = write_number(buf, maxsize, tm->tm_min, 2, &written);
            break;

        case 'n':
            /* Newline */
            if (written < maxsize - 1) {
                *buf++ = '\n';
                written++;
            }
            break;

        case 'p':
            /* AM/PM */
            buf = write_string(buf, maxsize,
                              ampm[tm->tm_hour >= 12 ? 1 : 0], &written);
            break;

        case 'r':
            /* hh:mm:ss AM/PM */
            {
                int hour12 = tm->tm_hour % 12;
                if (hour12 == 0) hour12 = 12;
                buf = write_number(buf, maxsize, hour12, 2, &written);
                buf = write_string(buf, maxsize, ":", &written);
                buf = write_number(buf, maxsize, tm->tm_min, 2, &written);
                buf = write_string(buf, maxsize, ":", &written);
                buf = write_number(buf, maxsize, tm->tm_sec, 2, &written);
                buf = write_string(buf, maxsize, " ", &written);
                buf = write_string(buf, maxsize,
                                  ampm[tm->tm_hour >= 12 ? 1 : 0],
                                  &written);
            }
            break;

        case 'R':
            /* HH:MM */
            buf = write_number(buf, maxsize, tm->tm_hour, 2, &written);
            buf = write_string(buf, maxsize, ":", &written);
            buf = write_number(buf, maxsize, tm->tm_min, 2, &written);
            break;

        case 'S':
            /* Detik (00-60) */
            buf = write_number(buf, maxsize, tm->tm_sec, 2, &written);
            break;

        case 't':
            /* Tab */
            if (written < maxsize - 1) {
                *buf++ = '\t';
                written++;
            }
            break;

        case 'T':
            /* HH:MM:SS */
            buf = write_number(buf, maxsize, tm->tm_hour, 2, &written);
            buf = write_string(buf, maxsize, ":", &written);
            buf = write_number(buf, maxsize, tm->tm_min, 2, &written);
            buf = write_string(buf, maxsize, ":", &written);
            buf = write_number(buf, maxsize, tm->tm_sec, 2, &written);
            break;

        case 'u':
            /* Hari dalam minggu (1-7, 1=Senin) */
            buf = write_number(buf, maxsize,
                              tm->tm_wday == 0 ? 7 : tm->tm_wday,
                              0, &written);
            break;

        case 'U':
            /* Minggu dalam tahun (Minggu pertama) */
            buf = write_number(buf, maxsize,
                              week_of_year(tm, 0), 2, &written);
            break;

        case 'V':
            /* Minggu ISO */
            buf = write_number(buf, maxsize,
                              iso_week_of_year(tm, &temp_year), 2, &written);
            break;

        case 'w':
            /* Hari dalam minggu (0-6, 0=Minggu) */
            buf = write_number(buf, maxsize, tm->tm_wday, 0, &written);
            break;

        case 'W':
            /* Minggu dalam tahun (Senin pertama) */
            buf = write_number(buf, maxsize,
                              week_of_year(tm, 1), 2, &written);
            break;

        case 'x':
            /* Tanggal standar (MM/DD/YY) */
            buf = write_number(buf, maxsize, tm->tm_mon + 1, 2, &written);
            buf = write_string(buf, maxsize, "/", &written);
            buf = write_number(buf, maxsize, tm->tm_mday, 2, &written);
            buf = write_string(buf, maxsize, "/", &written);
            buf = write_number(buf, maxsize,
                              (tm->tm_year + 1900) % 100, 2, &written);
            break;

        case 'X':
            /* Waktu standar (HH:MM:SS) */
            buf = write_number(buf, maxsize, tm->tm_hour, 2, &written);
            buf = write_string(buf, maxsize, ":", &written);
            buf = write_number(buf, maxsize, tm->tm_min, 2, &written);
            buf = write_string(buf, maxsize, ":", &written);
            buf = write_number(buf, maxsize, tm->tm_sec, 2, &written);
            break;

        case 'y':
            /* Tahun 2 digit */
            buf = write_number(buf, maxsize,
                              (tm->tm_year + 1900) % 100, 2, &written);
            break;

        case 'Y':
            /* Tahun 4 digit */
            buf = write_number(buf, maxsize, tm->tm_year + 1900,
                              4, &written);
            break;

        case 'z':
            /* Timezone offset (+/-HHMM) */
            {
                long offset = __get_timezone();
                int sign = '+';
                int hours, mins;

                if (offset < 0) {
                    sign = '-';
                    offset = -offset;
                }

                hours = (int)(offset / 3600);
                mins = (int)((offset % 3600) / 60);

                if (written < maxsize - 1) {
                    *buf++ = sign;
                    written++;
                }
                buf = write_number(buf, maxsize, hours, 2, &written);
                buf = write_number(buf, maxsize, mins, 2, &written);
            }
            break;

        case 'Z':
            /* Nama timezone */
            buf = write_string(buf, maxsize, "UTC", &written);
            break;

        default:
            /* Unknown specifier, tulis apa adanya */
            if (written < maxsize - 1) {
                *buf++ = '%';
                written++;
            }
            if (written < maxsize - 1) {
                *buf++ = *format;
                written++;
            }
            break;
        }

        format++;
    }

    /* Null-terminate */
    *buf = '\0';

    return written;
}

/* ============================================================
 * STRPTIME - PARSE STRING KE WAKTU
 * ============================================================
 */

/*
 * parse_number - Parse angka dari string
 *
 * Parameter:
 *   str   - String yang di-parse
 *   len   - Panjang maksimum digit
 *   result - Pointer untuk menyimpan hasil
 *
 * Return: Pointer ke karakter berikutnya, atau NULL jika error
 */
static const char *parse_number(const char *str, int len, int *result) {
    int num = 0;
    int digits = 0;

    while (*str != '\0' && isdigit((unsigned char)*str) && digits < len) {
        num = num * 10 + (*str - '0');
        str++;
        digits++;
    }

    if (digits == 0) {
        return NULL;
    }

    *result = num;
    return str;
}

/*
 * parse_string - Parse string dari daftar
 *
 * Parameter:
 *   str   - String yang di-parse
 *   list  - Daftar string yang valid
 *   count - Jumlah item dalam daftar
 *   result - Pointer untuk menyimpan index hasil
 *   case_sensitive - Apakah perbandingan case-sensitive
 *
 * Return: Pointer ke karakter berikutnya, atau NULL jika error
 */
static const char *parse_string(const char *str, const char *list[],
                                int count, int *result, int case_sensitive) {
    int i;
    int len;
    int str_len;
    int match_len;
    int match_idx;

    str_len = 0;
    while (str[str_len] != '\0' && !isdigit((unsigned char)str[str_len]) &&
           !isspace((unsigned char)str[str_len])) {
        str_len++;
    }

    if (str_len == 0) {
        return NULL;
    }

    match_idx = -1;
    match_len = 0;

    for (i = 0; i < count; i++) {
        len = strlen(list[i]);

        if (case_sensitive) {
            if (len == str_len && strncmp(str, list[i], len) == 0) {
                match_idx = i;
                match_len = len;
                break;
            }
        } else {
            if (len == str_len &&
                strncasecmp(str, list[i], len) == 0) {
                match_idx = i;
                match_len = len;
                break;
            }
        }

        /* Juga cek partial match untuk singkatan */
        if (len > str_len && match_idx < 0) {
            if (case_sensitive) {
                if (strncmp(str, list[i], str_len) == 0) {
                    match_idx = i;
                    match_len = str_len;
                }
            } else {
                if (strncasecmp(str, list[i], str_len) == 0) {
                    match_idx = i;
                    match_len = str_len;
                }
            }
        }
    }

    if (match_idx < 0) {
        return NULL;
    }

    *result = match_idx;
    return str + str_len;
}

/*
 * strptime - Parse string ke struct tm
 *
 * Parameter:
 *   s      - String yang di-parse
 *   format - Format string
 *   tm     - Pointer ke struct tm untuk hasil
 *
 * Return: Pointer ke karakter berikutnya setelah parsing,
 *         atau NULL jika error
 *
 * Format specifier yang didukung:
 *   %a, %A - Nama hari
 *   %b, %B, %h - Nama bulan
 *   %d, %e - Hari dalam bulan
 *   %H     - Jam (00-23)
 *   %I     - Jam (01-12)
 *   %j     - Hari dalam tahun
 *   %m     - Bulan (01-12)
 *   %M     - Menit (00-59)
 *   %n, %t - Whitespace
 *   %p     - AM/PM
 *   %S     - Detik (00-60)
 *   %w     - Hari dalam minggu
 *   %y     - Tahun 2 digit
 *   %Y     - Tahun 4 digit
 *   %%     - Literal %
 */
char *strptime(const char *s, const char *format, struct tm *tm) {
    int temp;
    int hour12 = -1;
    int is_pm = 0;

    /* Validasi parameter */
    if (s == NULL || format == NULL || tm == NULL) {
        return NULL;
    }

    /* Proses format string */
    while (*format != '\0') {
        /* Lewati whitespace di format dan input */
        while (isspace((unsigned char)*format)) {
            format++;
        }
        while (isspace((unsigned char)*s)) {
            s++;
        }

        if (*format == '\0') {
            break;
        }

        if (*format != '%') {
            /* Karakter biasa harus cocok */
            if (*s != *format) {
                return NULL;
            }
            s++;
            format++;
            continue;
        }

        /* Format specifier */
        format++;

        if (*format == '\0') {
            break;
        }

        switch (*format) {
        case '%':
            /* Literal % */
            if (*s != '%') {
                return NULL;
            }
            s++;
            break;

        case 'a':
        case 'A':
            /* Nama hari */
            s = parse_string(s, weekday_abbr, 7, &tm->tm_wday, 0);
            if (s == NULL) {
                return NULL;
            }
            break;

        case 'b':
        case 'B':
        case 'h':
            /* Nama bulan */
            s = parse_string(s, month_abbr, 12, &tm->tm_mon, 0);
            if (s == NULL) {
                return NULL;
            }
            break;

        case 'd':
        case 'e':
            /* Hari dalam bulan */
            s = parse_number(s, 2, &tm->tm_mday);
            if (s == NULL || tm->tm_mday < 1 || tm->tm_mday > 31) {
                return NULL;
            }
            break;

        case 'H':
            /* Jam (00-23) */
            s = parse_number(s, 2, &tm->tm_hour);
            if (s == NULL || tm->tm_hour < 0 || tm->tm_hour > 23) {
                return NULL;
            }
            break;

        case 'I':
            /* Jam (01-12) */
            s = parse_number(s, 2, &hour12);
            if (s == NULL || hour12 < 1 || hour12 > 12) {
                return NULL;
            }
            break;

        case 'j':
            /* Hari dalam tahun */
            s = parse_number(s, 3, &tm->tm_yday);
            if (s == NULL || tm->tm_yday < 1 || tm->tm_yday > 366) {
                return NULL;
            }
            tm->tm_yday--;  /* 0-indexed */
            break;

        case 'm':
            /* Bulan */
            s = parse_number(s, 2, &tm->tm_mon);
            if (s == NULL || tm->tm_mon < 1 || tm->tm_mon > 12) {
                return NULL;
            }
            tm->tm_mon--;  /* 0-indexed */
            break;

        case 'M':
            /* Menit */
            s = parse_number(s, 2, &tm->tm_min);
            if (s == NULL || tm->tm_min < 0 || tm->tm_min > 59) {
                return NULL;
            }
            break;

        case 'n':
        case 't':
            /* Whitespace */
            while (isspace((unsigned char)*s)) {
                s++;
            }
            break;

        case 'p':
            /* AM/PM */
            {
                char ampm_str[3];
                int i;

                for (i = 0; i < 2 && *s != '\0'; i++) {
                    ampm_str[i] = toupper((unsigned char)*s);
                    s++;
                }
                ampm_str[i] = '\0';

                if (strcmp(ampm_str, "AM") == 0) {
                    is_pm = 0;
                } else if (strcmp(ampm_str, "PM") == 0) {
                    is_pm = 1;
                } else {
                    return NULL;
                }
            }
            break;

        case 'S':
            /* Detik */
            s = parse_number(s, 2, &tm->tm_sec);
            if (s == NULL || tm->tm_sec < 0 || tm->tm_sec > 60) {
                return NULL;
            }
            break;

        case 'w':
            /* Hari dalam minggu */
            s = parse_number(s, 1, &tm->tm_wday);
            if (s == NULL || tm->tm_wday < 0 || tm->tm_wday > 6) {
                return NULL;
            }
            break;

        case 'y':
            /* Tahun 2 digit */
            s = parse_number(s, 2, &temp);
            if (s == NULL) {
                return NULL;
            }
            /* Tahun 69-99 = 1969-1999, 00-68 = 2000-2068 */
            if (temp >= 69) {
                tm->tm_year = temp;
            } else {
                tm->tm_year = temp + 100;
            }
            break;

        case 'Y':
            /* Tahun 4 digit */
            s = parse_number(s, 4, &temp);
            if (s == NULL) {
                return NULL;
            }
            tm->tm_year = temp - 1900;
            break;

        default:
            /* Unknown specifier */
            return NULL;
        }

        format++;
    }

    /* Handle AM/PM untuk jam */
    if (hour12 >= 0) {
        if (is_pm) {
            tm->tm_hour = (hour12 == 12) ? 12 : hour12 + 12;
        } else {
            tm->tm_hour = (hour12 == 12) ? 0 : hour12;
        }
    }

    return (char *)s;
}
