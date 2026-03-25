/*
 * PIGURA LIBC - TIME.H
 * =====================
 * Header untuk fungsi waktu sesuai standar C89 dan POSIX.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 */

#ifndef LIBC_TIME_H
#define LIBC_TIME_H

/* ============================================================
 * DEPENDENSI
 * ============================================================
 */
#include <stddef.h>

/* ============================================================
 * NULL POINTER
 * ============================================================
 */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* ============================================================
 * TIPE DATA
 * ============================================================
 */

/* Tipe untuk waktu dalam detik */
typedef long time_t;

/* Tipe untuk clock ticks */
typedef unsigned long clock_t;

/* Tipe untuk microseconds */
typedef long suseconds_t;

/* Tipe untuk nanoseconds */
typedef long nanoseconds_t;

/* Struktur broken-down time */
struct tm {
    int tm_sec;    /* Detik (0-60, 60 untuk leap second) */
    int tm_min;    /* Menit (0-59) */
    int tm_hour;   /* Jam (0-23) */
    int tm_mday;   /* Hari dalam bulan (1-31) */
    int tm_mon;    /* Bulan (0-11, 0 = Januari) */
    int tm_year;   /* Tahun sejak 1900 */
    int tm_wday;   /* Hari dalam minggu (0-6, 0 = Minggu) */
    int tm_yday;   /* Hari dalam tahun (0-365) */
    int tm_isdst;  /* Daylight saving time flag */
    /* Fields tambahan (POSIX) */
    long tm_gmtoff; /* Offset dari UTC dalam detik */
    const char *tm_zone; /* Timezone abbreviation */
};

/* Struktur untuk timeval (POSIX) */
struct timeval {
    time_t tv_sec;       /* Detik */
    suseconds_t tv_usec; /* Microseconds */
};

/* Struktur untuk timespec (POSIX) */
struct timespec {
    time_t tv_sec;   /* Detik */
    long tv_nsec;    /* Nanoseconds */
};

/* Struktur untuk timezone (legacy) */
struct timezone {
    int tz_minuteswest; /* Menit west of Greenwich */
    int tz_dsttime;     /* DST correction type */
};

/* Struktur untuk interval timer */
struct itimerval {
    struct timeval it_interval; /* Interval untuk timer periodik */
    struct timeval it_value;    /* Waktu sampai expire pertama */
};

/* Struktur untuk timer (realtime) */
struct itimerspec {
    struct timespec it_interval; /* Interval periodik */
    struct timespec it_value;    /* Expire time */
};

/* ============================================================
 * KONSTANTA
 * ============================================================
 */

/* Clock ticks per detik */
#define CLOCKS_PER_SEC  1000000L

/* Jumlah clock per detik (alias) */
#define CLK_TCK         CLOCKS_PER_SEC

/* Nilai per detik */
#define TIME_UNITS_PER_SEC  1000000000L  /* Nanoseconds */
#define NSEC_PER_SEC        1000000000L
#define USEC_PER_SEC        1000000L
#define MSEC_PER_SEC        1000L

/* Timer types untuk setitimer */
#define ITIMER_REAL    0  /* Timer real-time, kirim SIGALRM */
#define ITIMER_VIRTUAL 1  /* Timer virtual, kirim SIGVTALRM */
#define ITIMER_PROF    2  /* Timer profiling, kirim SIGPROF */

/* Clock types untuk clock_gettime */
#define CLOCK_REALTIME           0  /* System-wide real-time clock */
#define CLOCK_MONOTONIC          1  /* Monotonic clock */
#define CLOCK_PROCESS_CPUTIME_ID 2  /* CPU time proses */
#define CLOCK_THREAD_CPUTIME_ID  3  /* CPU time thread */
#define CLOCK_MONOTONIC_RAW      4  /* Raw monotonic clock */
#define CLOCK_REALTIME_COARSE    5  /* Faster but less precise */
#define CLOCK_MONOTONIC_COARSE   6  /* Faster monotonic */
#define CLOCK_BOOTTIME           7  /* Include time suspended */
#define CLOCK_REALTIME_ALARM     8  /* Real-time alarm clock */
#define CLOCK_BOOTTIME_ALARM     9  /* Boot-time alarm clock */

/* Timer flags */
#define TIMER_ABSTIME    1  /* Absolute time untuk timer_settime */

/* ============================================================
 * DEKLARASI FUNGSI WAKTU (C89)
 * ============================================================
 */

/*
 * time - Dapatkan waktu saat ini
 *
 * Parameter:
 *   tloc - Pointer untuk menyimpan hasil (boleh NULL)
 *
 * Return: Waktu saat ini sebagai time_t, atau -1 jika error
 */
extern time_t time(time_t *tloc);

/*
 * clock - Dapatkan CPU time yang digunakan proses
 *
 * Return: CPU time dalam clock ticks, atau -1 jika tidak tersedia
 */
extern clock_t clock(void);

/*
 * difftime - Hitung selisih waktu
 *
 * Parameter:
 *   time1 - Waktu akhir
 *   time0 - Waktu awal
 *
 * Return: Selisih dalam detik (time1 - time0)
 */
extern double difftime(time_t time1, time_t time0);

/*
 * mktime - Konversi tm ke time_t
 *
 * Parameter:
 *   tm - Struktur broken-down time
 *
 * Return: time_t, atau -1 jika error
 *
 * Catatan: Field tm_wday dan tm_yday diabaikan dan dihitung
 * ulang berdasarkan nilai lain.
 */
extern time_t mktime(struct tm *tm);

/*
 * localtime - Konversi time_t ke tm (waktu lokal)
 *
 * Parameter:
 *   timer - Pointer ke time_t
 *
 * Return: Pointer ke struktur tm, atau NULL jika error
 *
 * Catatan: Hasil disimpan di buffer statis, tidak thread-safe.
 */
extern struct tm *localtime(const time_t *timer);

/*
 * gmtime - Konversi time_t ke tm (UTC)
 *
 * Parameter:
 *   timer - Pointer ke time_t
 *
 * Return: Pointer ke struktur tm, atau NULL jika error
 *
 * Catatan: Hasil disimpan di buffer statis, tidak thread-safe.
 */
extern struct tm *gmtime(const time_t *timer);

/*
 * localtime_r - Konversi time_t ke tm (reentrant)
 *
 * Parameter:
 *   timer - Pointer ke time_t
 *   result - Buffer untuk hasil
 *
 * Return: Pointer ke result, atau NULL jika error
 */
extern struct tm *localtime_r(const time_t *timer, struct tm *result);

/*
 * gmtime_r - Konversi time_t ke tm (reentrant, UTC)
 *
 * Parameter:
 *   timer - Pointer ke time_t
 *   result - Buffer untuk hasil
 *
 * Return: Pointer ke result, atau NULL jika error
 */
extern struct tm *gmtime_r(const time_t *timer, struct tm *result);

/*
 * asctime - Konversi tm ke string
 *
 * Parameter:
 *   tm - Struktur broken-down time
 *
 * Return: String format "Wed Jun 30 21:49:08 1993\n"
 *
 * Catatan: Hasil disimpan di buffer statis, tidak thread-safe.
 */
extern char *asctime(const struct tm *tm);

/*
 * ctime - Konversi time_t ke string
 *
 * Parameter:
 *   timer - Pointer ke time_t
 *
 * Return: String format sama dengan asctime
 *
 * Catatan: Hasil disimpan di buffer statis, tidak thread-safe.
 */
extern char *ctime(const time_t *timer);

/*
 * ctime_r - Konversi time_t ke string (reentrant)
 *
 * Parameter:
 *   timer - Pointer ke time_t
 *   buf - Buffer untuk hasil (minimal 26 byte)
 *
 * Return: Pointer ke buf, atau NULL jika error
 */
extern char *ctime_r(const time_t *timer, char *buf);

/*
 * asctime_r - Konversi tm ke string (reentrant)
 *
 * Parameter:
 *   tm - Struktur broken-down time
 *   buf - Buffer untuk hasil (minimal 26 byte)
 *
 * Return: Pointer ke buf, atau NULL jika error
 */
extern char *asctime_r(const struct tm *tm, char *buf);

/*
 * strftime - Format waktu ke string
 *
 * Parameter:
 *   s     - Buffer tujuan
 *   max   - Ukuran buffer
 *   format - Format string
 *   tm    - Struktur broken-down time
 *
 * Return: Jumlah karakter yang ditulis (tanpa null), atau 0
 */
extern size_t strftime(char *s, size_t max, const char *format,
                       const struct tm *tm);

/*
 * strptime - Parse string ke tm (POSIX)
 *
 * Parameter:
 *   s      - String input
 *   format - Format string
 *   tm     - Buffer untuk hasil
 *
 * Return: Pointer ke karakter pertama yang tidak diparse,
 *         atau NULL jika error
 */
extern char *strptime(const char *s, const char *format, struct tm *tm);

/* ============================================================
 * DEKLARASI FUNGSI WAKTU (POSIX)
 * ============================================================
 */

/*
 * gettimeofday - Dapatkan waktu presisi tinggi
 *
 * Parameter:
 *   tv  - Buffer untuk waktu
 *   tz  - Buffer untuk timezone (boleh NULL, obsolete)
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int gettimeofday(struct timeval *tv, struct timezone *tz);

/*
 * settimeofday - Set waktu sistem
 *
 * Parameter:
 *   tv - Waktu baru
 *   tz - Timezone baru (boleh NULL)
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int settimeofday(const struct timeval *tv, const struct timezone *tz);

/*
 * clock_gettime - Dapatkan waktu dari clock tertentu
 *
 * Parameter:
 *   clockid - Clock ID (CLOCK_*)
 *   tp      - Buffer untuk hasil
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int clock_gettime(int clockid, struct timespec *tp);

/*
 * clock_settime - Set waktu clock
 *
 * Parameter:
 *   clockid - Clock ID
 *   tp      - Waktu baru
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int clock_settime(int clockid, const struct timespec *tp);

/*
 * clock_getres - Dapatkan resolusi clock
 *
 * Parameter:
 *   clockid - Clock ID
 *   res     - Buffer untuk resolusi
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int clock_getres(int clockid, struct timespec *res);

/*
 * nanosleep - Sleep dengan presisi nanosecond
 *
 * Parameter:
 *   req - Waktu sleep yang diminta
 *   rem - Sisa waktu jika terinterupsi (boleh NULL)
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int nanosleep(const struct timespec *req, struct timespec *rem);

/*
 * usleep - Sleep dalam microseconds
 *
 * Parameter:
 *   usec - Waktu sleep dalam microseconds
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int usleep(unsigned long usec);

/*
 * sleep - Sleep dalam detik
 *
 * Parameter:
 *   seconds - Waktu sleep dalam detik
 *
 * Return: 0 atau sisa waktu jika terinterupsi
 */
extern unsigned int sleep(unsigned int seconds);

/*
 * alarm - Set alarm signal
 *
 * Parameter:
 *   seconds - Waktu alarm (0 untuk cancel)
 *
 * Return: Waktu sisa alarm sebelumnya
 */
extern unsigned int alarm(unsigned int seconds);

/* ============================================================
 * TIMER FUNCTIONS (POSIX)
 * ============================================================
 */

/* Tipe untuk timer ID */
typedef int timer_t;

/*
 * timer_create - Buat timer
 *
 * Parameter:
 *   clockid     - Clock ID
 *   sevp        - Event untuk notifikasi (boleh NULL)
 *   timerid     - Buffer untuk timer ID
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int timer_create(int clockid, struct sigevent *sevp,
                        timer_t *timerid);

/*
 * timer_delete - Hapus timer
 *
 * Parameter:
 *   timerid - Timer ID
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int timer_delete(timer_t timerid);

/*
 * timer_settime - Set timer
 *
 * Parameter:
 *   timerid   - Timer ID
 *   flags     - Flags (TIMER_ABSTIME)
 *   value     - Nilai timer baru
 *   ovalue    - Nilai timer lama (boleh NULL)
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int timer_settime(timer_t timerid, int flags,
                         const struct itimerspec *value,
                         struct itimerspec *ovalue);

/*
 * timer_gettime - Dapatkan nilai timer
 *
 * Parameter:
 *   timerid - Timer ID
 *   value   - Buffer untuk hasil
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int timer_gettime(timer_t timerid, struct itimerspec *value);

/*
 * timer_getoverrun - Dapatkan overrun count
 *
 * Parameter:
 *   timerid - Timer ID
 *
 * Return: Overrun count, atau -1 jika error
 */
extern int timer_getoverrun(timer_t timerid);

/* ============================================================
 * INTERVAL TIMER FUNCTIONS (BSD/POSIX)
 * ============================================================
 */

/*
 * getitimer - Dapatkan interval timer
 *
 * Parameter:
 *   which    - Timer type (ITIMER_*)
 *   value    - Buffer untuk hasil
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int getitimer(int which, struct itimerval *value);

/*
 * setitimer - Set interval timer
 *
 * Parameter:
 *   which    - Timer type (ITIMER_*)
 *   value    - Nilai timer baru
 *   ovalue   - Nilai timer lama (boleh NULL)
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int setitimer(int which, const struct itimerval *value,
                     struct itimerval *ovalue);

/* ============================================================
 * FORMAT SPECIFIERS UNTUK strftime()
 * ============================================================
 */

/*
 * %a - Abbreviated weekday name
 * %A - Full weekday name
 * %b - Abbreviated month name
 * %B - Full month name
 * %c - Date and time representation
 * %C - Year divided by 100
 * %d - Day of month (01-31)
 * %D - Date (MM/DD/YY)
 * %e - Day of month ( 1-31)
 * %F - ISO 8601 date (YYYY-MM-DD)
 * %g - Week-based year (00-99)
 * %G - Week-based year
 * %h - Abbreviated month name (same as %b)
 * %H - Hour (00-23)
 * %I - Hour (01-12)
 * %j - Day of year (001-366)
 * %m - Month (01-12)
 * %M - Minute (00-59)
 * %n - Newline character
 * %p - AM/PM designation
 * %r - 12-hour clock time
 * %R - 24-hour clock time (HH:MM)
 * %S - Second (00-60)
 * %t - Tab character
 * %T - Time (HH:MM:SS)
 * %u - Weekday (1-7, 1=Monday)
 * %U - Week number (00-53, Sunday-based)
 * %V - ISO week number (01-53)
 * %w - Weekday (0-6, 0=Sunday)
 * %W - Week number (00-53, Monday-based)
 * %x - Date representation
 * %X - Time representation
 * %y - Year without century (00-99)
 * %Y - Year with century
 * %z - UTC offset
 * %Z - Timezone name
 * %% - Percent sign
 */

#endif /* LIBC_TIME_H */
