/*
 * PIGURA LIBC - TIME.H
 * =====================
 * Header untuk fungsi waktu sesuai standar C89 dan POSIX.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.1 - Perbaikan forward declarations
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
 * FORWARD DECLARATIONS
 * ============================================================
 */
struct sigevent;

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
 */
extern time_t time(time_t *tloc);

/*
 * clock - Dapatkan CPU time yang digunakan proses
 */
extern clock_t clock(void);

/*
 * difftime - Hitung selisih waktu
 */
extern double difftime(time_t time1, time_t time0);

/*
 * mktime - Konversi tm ke time_t
 */
extern time_t mktime(struct tm *tm);

/*
 * localtime - Konversi time_t ke tm (waktu lokal)
 */
extern struct tm *localtime(const time_t *timer);

/*
 * gmtime - Konversi time_t ke tm (UTC)
 */
extern struct tm *gmtime(const time_t *timer);

/*
 * localtime_r - Konversi time_t ke tm (reentrant)
 */
extern struct tm *localtime_r(const time_t *timer, struct tm *result);

/*
 * gmtime_r - Konversi time_t ke tm (reentrant, UTC)
 */
extern struct tm *gmtime_r(const time_t *timer, struct tm *result);

/*
 * asctime - Konversi tm ke string
 */
extern char *asctime(const struct tm *tm);

/*
 * ctime - Konversi time_t ke string
 */
extern char *ctime(const time_t *timer);

/*
 * ctime_r - Konversi time_t ke string (reentrant)
 */
extern char *ctime_r(const time_t *timer, char *buf);

/*
 * asctime_r - Konversi tm ke string (reentrant)
 */
extern char *asctime_r(const struct tm *tm, char *buf);

/*
 * strftime - Format waktu ke string
 */
extern size_t strftime(char *s, size_t max, const char *format,
                       const struct tm *tm);

/*
 * strptime - Parse string ke tm (POSIX)
 */
extern char *strptime(const char *s, const char *format, struct tm *tm);

/* ============================================================
 * DEKLARASI FUNGSI WAKTU (POSIX)
 * ============================================================
 */

/*
 * gettimeofday - Dapatkan waktu presisi tinggi
 */
extern int gettimeofday(struct timeval *tv, struct timezone *tz);

/*
 * settimeofday - Set waktu sistem
 */
extern int settimeofday(const struct timeval *tv, const struct timezone *tz);

/*
 * clock_gettime - Dapatkan waktu dari clock tertentu
 */
extern int clock_gettime(int clockid, struct timespec *tp);

/*
 * clock_settime - Set waktu clock
 */
extern int clock_settime(int clockid, const struct timespec *tp);

/*
 * clock_getres - Dapatkan resolusi clock
 */
extern int clock_getres(int clockid, struct timespec *res);

/*
 * nanosleep - Sleep dengan presisi nanosecond
 */
extern int nanosleep(const struct timespec *req, struct timespec *rem);

/*
 * sleep - Sleep dalam detik
 */
extern unsigned int sleep(unsigned int seconds);

/*
 * alarm - Set alarm signal
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
 */
extern int timer_create(int clockid, struct sigevent *sevp,
                        timer_t *timerid);

/*
 * timer_delete - Hapus timer
 */
extern int timer_delete(timer_t timerid);

/*
 * timer_settime - Set timer
 */
extern int timer_settime(timer_t timerid, int flags,
                         const struct itimerspec *value,
                         struct itimerspec *ovalue);

/*
 * timer_gettime - Dapatkan nilai timer
 */
extern int timer_gettime(timer_t timerid, struct itimerspec *value);

/*
 * timer_getoverrun - Dapatkan overrun count
 */
extern int timer_getoverrun(timer_t timerid);

/* ============================================================
 * INTERVAL TIMER FUNCTIONS (BSD/POSIX)
 * ============================================================
 */

/*
 * getitimer - Dapatkan interval timer
 */
extern int getitimer(int which, struct itimerval *value);

/*
 * setitimer - Set interval timer
 */
extern int setitimer(int which, const struct itimerval *value,
                     struct itimerval *ovalue);

#endif /* LIBC_TIME_H */
