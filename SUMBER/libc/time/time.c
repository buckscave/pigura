/*
 * PIGURA LIBC - TIME/TIME.C
 * =========================
 * Implementasi fungsi waktu dasar.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 *
 * Fungsi yang diimplementasikan:
 *   - time()         : Dapatkan waktu saat ini
 *   - clock()        : Dapatkan CPU time
 *   - difftime()     : Hitung selisih waktu
 *   - mktime()       : Konversi tm ke time_t
 *   - gmtime()       : Konversi time_t ke tm (UTC)
 *   - gmtime_r()     : gmtime reentrant
 *   - localtime()    : Konversi time_t ke tm (lokal)
 *   - localtime_r()  : localtime reentrant
 */

#include <time.h>
#include <errno.h>

/* ============================================================
 * DEKLARASI SYSCALL
 * ============================================================
 * Fungsi syscall untuk mendapatkan waktu dari kernel.
 */

/* Syscall untuk mendapatkan waktu */
extern long __syscall_time(long *tloc);
extern long __syscall_clock_gettime(int clk_id, void *tp);

/* Clock ID untuk clock_gettime */
#define CLOCK_REALTIME           0
#define CLOCK_MONOTONIC          1
#define CLOCK_PROCESS_CPUTIME_ID 2
#define CLOCK_THREAD_CPUTIME_ID  3

/* Struktur timespec untuk internal use */
typedef struct {
    long tv_sec;   /* Detik */
    long tv_nsec;  /* Nanodetik */
} timespec_t;

/* ============================================================
 * KONSTANTA WAKTU
 * ============================================================
 */

/* Detik per hari */
#define SECS_PER_DAY    86400L
#define SECS_PER_HOUR   3600L
#define SECS_PER_MIN    60L

/* Hari per bulan (non-leap year) */
static const int days_per_month[12] = {
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

/* ============================================================
 * VARIABEL GLOBAL
 * ============================================================
 */

/* Buffer statis untuk gmtime() dan localtime() (non-reentrant) */
static struct tm tm_buffer_gm;
static struct tm tm_buffer_local;

/* Waktu mulai proses untuk clock() */
static clock_t process_start_time = 0;
static int process_time_initialized = 0;

/* Timezone offset dari UTC (dalam detik) */
/* Default: UTC+0, bisa di-set via __set_timezone() */
static long timezone_offset = 0;
static int timezone_dst = 0;

/* ============================================================
 * FUNGSI HELPER INTERNAL
 * ============================================================
 */

/*
 * is_leap_year - Cek apakah tahun kabisat
 *
 * Parameter:
 *   year - Tahun (misal: 2024)
 *
 * Return: 1 jika kabisat, 0 jika tidak
 */
static int is_leap_year(int year) {
    return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
}

/*
 * days_in_month - Jumlah hari dalam bulan tertentu
 *
 * Parameter:
 *   month - Bulan (0-11, 0 = Januari)
 *   year  - Tahun
 *
 * Return: Jumlah hari dalam bulan
 */
static int days_in_month(int month, int year) {
    if (month == 1) {  /* Februari */
        return is_leap_year(year) ? 29 : 28;
    }
    return days_per_month[month];
}

/*
 * days_since_epoch - Hitung hari sejak epoch (1 Jan 1970)
 *
 * Parameter:
 *   year - Tahun (misal: 2024)
 *
 * Return: Jumlah hari dari 1 Jan 1970 ke 1 Jan tahun tersebut
 */
static long days_since_epoch(int year) {
    long days = 0;
    int y;

    /* Hitung hari untuk tahun penuh sebelum tahun ini */
    for (y = 1970; y < year; y++) {
        days += is_leap_year(y) ? 366 : 365;
    }

    return days;
}

/*
 * day_of_week - Hitung hari dalam minggu
 *
 * Parameter:
 *   year  - Tahun
 *   month - Bulan (0-11)
 *   day   - Hari (1-31)
 *
 * Return: Hari dalam minggu (0 = Minggu, 1 = Senin, dst.)
 *
 * Menggunakan algoritma Zeller's congruence.
 */
static int day_of_week(int year, int month, int day) {
    int a, y, m;

    /* Sesuaikan bulan: Maret = 1, ..., Februari = 12 */
    if (month < 2) {
        month += 12;
        year--;
    }

    a = year / 100;
    y = year % 100;
    m = month;

    /* Zeller's formula untuk Gregorian calendar */
    /* Hasil: 0 = Sabtu, 1 = Minggu, ..., 6 = Jumat */
    /* Sesuaikan sehingga 0 = Minggu */
    return (day + (13 * (m + 1)) / 5 + y + y / 4 + a / 4 + 5 * a) % 7;
}

/*
 * day_of_year - Hitung hari dalam tahun
 *
 * Parameter:
 *   year  - Tahun
 *   month - Bulan (0-11)
 *   day   - Hari (1-31)
 *
 * Return: Hari dalam tahun (0-365, 0 = 1 Januari)
 */
static int day_of_year(int year, int month, int day) {
    int doy = day - 1;  /* 0-indexed */
    int m;

    for (m = 0; m < month; m++) {
        doy += days_in_month(m, year);
    }

    return doy;
}

/* ============================================================
 * FUNGSI WAKTU DASAR
 * ============================================================
 */

/*
 * time - Dapatkan waktu saat ini
 *
 * Parameter:
 *   tloc - Pointer untuk menyimpan waktu (boleh NULL)
 *
 * Return: Waktu saat ini dalam detik sejak epoch,
 *         atau (time_t)-1 jika error
 */
time_t time(time_t *tloc) {
    time_t current_time;

    /* Panggil syscall */
    current_time = (time_t)__syscall_time(NULL);

    if (current_time == (time_t)-1) {
        return (time_t)-1;
    }

    /* Simpan ke tloc jika tidak NULL */
    if (tloc != NULL) {
        *tloc = current_time;
    }

    return current_time;
}

/*
 * clock - Dapatkan CPU time yang digunakan proses
 *
 * Return: CPU time dalam clock ticks, atau (clock_t)-1 jika error
 *
 * Catatan: Mengembalikan jumlah clock ticks sejak proses dimulai.
 *          Untuk konversi ke detik, bagi dengan CLOCKS_PER_SEC.
 */
clock_t clock(void) {
    timespec_t ts;
    clock_t ticks;

    /* Inisialisasi waktu mulai jika belum */
    if (!process_time_initialized) {
        __syscall_clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
        process_start_time = (clock_t)(ts.tv_sec * CLOCKS_PER_SEC +
                                       ts.tv_nsec / (1000000000L /
                                                     CLOCKS_PER_SEC));
        process_time_initialized = 1;
        return (clock_t)0;
    }

    /* Dapatkan waktu CPU saat ini */
    __syscall_clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);

    /* Konversi ke clock ticks */
    ticks = (clock_t)(ts.tv_sec * CLOCKS_PER_SEC +
                      ts.tv_nsec / (1000000000L / CLOCKS_PER_SEC));

    /* Return elapsed time */
    return ticks - process_start_time;
}

/*
 * difftime - Hitung selisih antara dua waktu
 *
 * Parameter:
 *   time1 - Waktu akhir
 *   time0 - Waktu awal
 *
 * Return: Selisih time1 - time0 dalam detik
 */
double difftime(time_t time1, time_t time0) {
    /* time_t biasanya signed integer, perlu handle dengan hati-hati */
    /* untuk menghindari overflow pada perhitungan */
    
    if (time1 >= time0) {
        return (double)(time1 - time0);
    } else {
        return -(double)(time0 - time1);
    }
}

/* ============================================================
 * FUNGSI KONVERSI time_t KE struct tm
 * ============================================================
 */

/*
 * gmtime_r - Konversi time_t ke tm (UTC, reentrant)
 *
 * Parameter:
 *   timer - Pointer ke time_t
 *   result - Buffer untuk hasil
 *
 * Return: Pointer ke result, atau NULL jika error
 */
struct tm *gmtime_r(const time_t *timer, struct tm *result) {
    time_t t;
    long days;
    long secs;
    int year;
    int month;
    int day;
    int is_leap;

    /* Validasi parameter */
    if (timer == NULL || result == NULL) {
        return NULL;
    }

    t = *timer;

    /* Handle nilai negatif (sebelum epoch) */
    if (t < 0) {
        /* Tidak didukung dalam implementasi ini */
        return NULL;
    }

    /* Hitung total hari dan detik dalam hari */
    days = t / SECS_PER_DAY;
    secs = t % SECS_PER_DAY;

    /* Hitung tahun */
    year = 1970;
    while (1) {
        is_leap = is_leap_year(year);
        long days_in_year = is_leap ? 366 : 365;
        
        if (days < days_in_year) {
            break;
        }
        
        days -= days_in_year;
        year++;
    }

    /* Hitung bulan */
    month = 0;
    while (1) {
        int dim = days_in_month(month, year);
        
        if (days < dim) {
            break;
        }
        
        days -= dim;
        month++;
    }

    /* Hari dalam bulan (1-indexed) */
    day = (int)days + 1;

    /* Isi struktur tm */
    result->tm_year = year - 1900;
    result->tm_mon = month;
    result->tm_mday = day;
    result->tm_hour = (int)(secs / SECS_PER_HOUR);
    secs %= SECS_PER_HOUR;
    result->tm_min = (int)(secs / SECS_PER_MIN);
    result->tm_sec = (int)(secs % SECS_PER_MIN);
    result->tm_wday = day_of_week(year, month, day);
    result->tm_yday = day_of_year(year, month, day);
    result->tm_isdst = 0;  /* UTC tidak memiliki DST */

    return result;
}

/*
 * gmtime - Konversi time_t ke tm (UTC, non-reentrant)
 *
 * Parameter:
 *   timer - Pointer ke time_t
 *
 * Return: Pointer ke struct tm, atau NULL jika error
 *
 * Peringatan: Tidak reentrant. Gunakan gmtime_r().
 */
struct tm *gmtime(const time_t *timer) {
    return gmtime_r(timer, &tm_buffer_gm);
}

/*
 * localtime_r - Konversi time_t ke tm (lokal, reentrant)
 *
 * Parameter:
 *   timer  - Pointer ke time_t
 *   result - Buffer untuk hasil
 *
 * Return: Pointer ke result, atau NULL jika error
 *
 * Catatan: Menggunakan timezone offset global.
 */
struct tm *localtime_r(const time_t *timer, struct tm *result) {
    time_t local_time;
    time_t adjusted_time;

    /* Validasi parameter */
    if (timer == NULL || result == NULL) {
        return NULL;
    }

    /* Adjust untuk timezone */
    local_time = *timer + timezone_offset;

    /* Adjust untuk DST jika aktif */
    if (timezone_dst) {
        local_time += SECS_PER_HOUR;
    }

    /* Gunakan gmtime_r untuk konversi dasar */
    adjusted_time = local_time;
    if (gmtime_r(&adjusted_time, result) == NULL) {
        return NULL;
    }

    /* Set DST flag */
    result->tm_isdst = timezone_dst;

    return result;
}

/*
 * localtime - Konversi time_t ke tm (lokal, non-reentrant)
 *
 * Parameter:
 *   timer - Pointer ke time_t
 *
 * Return: Pointer ke struct tm, atau NULL jika error
 *
 * Peringatan: Tidak reentrant. Gunakan localtime_r().
 */
struct tm *localtime(const time_t *timer) {
    return localtime_r(timer, &tm_buffer_local);
}

/* ============================================================
 * FUNGSI KONVERSI struct tm KE time_t
 * ============================================================
 */

/*
 * mktime - Konversi tm ke time_t
 *
 * Parameter:
 *   tm - Pointer ke struct tm
 *
 * Return: time_t, atau (time_t)-1 jika error
 *
 * Catatan: tm akan di-normalize jika nilai tidak valid.
 *          tm_wday dan tm_yday akan dihitung ulang.
 */
time_t mktime(struct tm *tm) {
    time_t result;
    int year;
    int month;
    int day;
    int hour;
    int min;
    int sec;
    long days;
    long secs;

    /* Validasi parameter */
    if (tm == NULL) {
        return (time_t)-1;
    }

    /* Extract dan normalize nilai */
    sec = tm->tm_sec;
    min = tm->tm_min;
    hour = tm->tm_hour;
    day = tm->tm_mday;
    month = tm->tm_mon;
    year = tm->tm_year + 1900;

    /* Handle overflow detik */
    while (sec >= 60) {
        sec -= 60;
        min++;
    }
    while (sec < 0) {
        sec += 60;
        min--;
    }

    /* Handle overflow menit */
    while (min >= 60) {
        min -= 60;
        hour++;
    }
    while (min < 0) {
        min += 60;
        hour--;
    }

    /* Handle overflow jam */
    while (hour >= 24) {
        hour -= 24;
        day++;
    }
    while (hour < 0) {
        hour += 24;
        day--;
    }

    /* Handle overflow hari */
    while (day > days_in_month(month, year)) {
        day -= days_in_month(month, year);
        month++;
        if (month >= 12) {
            month = 0;
            year++;
        }
    }
    while (day < 1) {
        month--;
        if (month < 0) {
            month = 11;
            year--;
        }
        day += days_in_month(month, year);
    }

    /* Handle overflow bulan */
    while (month >= 12) {
        month -= 12;
        year++;
    }
    while (month < 0) {
        month += 12;
        year--;
    }

    /* Hitung total hari sejak epoch */
    days = days_since_epoch(year);
    days += day_of_year(year, month, day);

    /* Hitung total detik */
    secs = days * SECS_PER_DAY;
    secs += hour * SECS_PER_HOUR;
    secs += min * SECS_PER_MIN;
    secs += sec;

    /* Adjust untuk timezone */
    secs -= timezone_offset;
    if (tm->tm_isdst > 0) {
        secs -= SECS_PER_HOUR;
    }

    /* Update struktur tm dengan nilai yang dinormalisasi */
    tm->tm_year = year - 1900;
    tm->tm_mon = month;
    tm->tm_mday = day;
    tm->tm_hour = hour;
    tm->tm_min = min;
    tm->tm_sec = sec;
    tm->tm_wday = day_of_week(year, month, day);
    tm->tm_yday = day_of_year(year, month, day);

    result = secs;
    return result;
}

/* ============================================================
 * FUNGSI INTERNAL UNTUK KONFIGURASI
 * ============================================================
 */

/*
 * __set_timezone - Set timezone offset
 *
 * Parameter:
 *   offset - Offset dari UTC dalam detik
 *   dst    - Flag daylight saving time
 */
void __set_timezone(long offset, int dst) {
    timezone_offset = offset;
    timezone_dst = dst;
}

/*
 * __get_timezone - Dapatkan timezone offset
 *
 * Return: Offset dari UTC dalam detik
 */
long __get_timezone(void) {
    return timezone_offset;
}

/*
 * __get_daylight - Dapatkan flag DST
 *
 * Return: 1 jika DST aktif, 0 jika tidak
 */
int __get_daylight(void) {
    return timezone_dst;
}
