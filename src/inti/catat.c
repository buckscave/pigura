/* ----------------------------------------------------------------------------------------------
 * pigura inti: catat.c - fasilitas logging
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/catat.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#if defined(_POSIX_THREADS) || defined(__unix__) || defined(__APPLE__)
#  include <pthread.h>
static pthread_mutex_t g_catat_kunci = PTHREAD_MUTEX_INITIALIZER;
#  define PG_CATAT_KUNCI()   pthread_mutex_lock(&g_catat_kunci)
#  define PG_CATAT_BUKA()    pthread_mutex_unlock(&g_catat_kunci)
#else
#  define PG_CATAT_KUNCI()
#  define PG_CATAT_BUKA()
#endif

static int   g_catat_level = PG_CATAT_INFO;
static FILE *g_catat_fp    = NULL;
static char  g_catat_tag[32] = "pigura";

static const char *level_str(int level)
{
        switch (level) {
        case PG_CATAT_FATAL:      return "FATAL";
        case PG_CATAT_GALAT:       return "GALAT";
        case PG_CATAT_PERINGATAN:  return "AWAS ";
        case PG_CATAT_INFO:        return "INFO ";
        case PG_CATAT_DEBUG:       return "DEBUG";
        case PG_CATAT_REKAM:       return "REKAM";
        default:                   return "?    ";
        }
}

void pg_catat_init(const char *tag)
{
        if (tag) {
                strncpy(g_catat_tag, tag, sizeof(g_catat_tag) - 1);
                g_catat_tag[sizeof(g_catat_tag) - 1] = '\0';
        }
        if (!g_catat_fp)
                g_catat_fp = stderr;
}

void pg_catat_set_level(int level)
{
        g_catat_level = level;
}

int pg_catat_get_level(void)
{
        return g_catat_level;
}

void pg_catat_set_fp(FILE *fp)
{
        g_catat_fp = fp ? fp : stderr;
}

FILE *pg_catat_get_fp(void)
{
        return g_catat_fp ? g_catat_fp : stderr;
}

void pg_catat_v(int level, const char *file, int line, const char *fmt, ...)
{
        FILE *fp;
        va_list ap;
        const char *base;
        time_t now;
        struct tm tm_buf;
        char timebuf[24];

        if (level > g_catat_level || level == PG_CATAT_MATI)
                return;

        fp = g_catat_fp ? g_catat_fp : stderr;
        if (!fp)
                return;

        base = file ? strrchr(file, '/') : NULL;
        if (base) base++;
        else base = file ? file : "?";

        time(&now);
#ifdef _WIN32
        tm_buf = *localtime(&now);
#else
        localtime_r(&now, &tm_buf);
#endif
        strftime(timebuf, sizeof(timebuf), "%H:%M:%S", &tm_buf);

        PG_CATAT_KUNCI();
        fprintf(fp, "[%s] [%s] [%s:%d] ", timebuf, level_str(level),
                base, line);
        va_start(ap, fmt);
        vfprintf(fp, fmt, ap);
        va_end(ap);
        fputc('\n', fp);
        fflush(fp);
        PG_CATAT_BUKA();
}
