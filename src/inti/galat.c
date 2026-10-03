/* ----------------------------------------------------------------------------------------------
 * pigura inti: galat.c - slot galat per-thread
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/galat.h"
#include "pigura/catat.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#if defined(_POSIX_THREADS) || defined(__unix__) || defined(__APPLE__)
#  include <pthread.h>
#  define PG_Punya_PTHREAD 1
#endif

#define PG_GALAT_PESAN_PANJANG 256

typedef struct pg_galat_slot {
        pg_galat kode;
        char     pesan[PG_GALAT_PESAN_PANJANG];
} pg_galat_slot_t;

#ifdef PG_Punya_PTHREAD
static pthread_key_t   g_galat_key;
static pthread_once_t  g_galat_once = PTHREAD_ONCE_INIT;

static void pg_galat_hancur_key(void *p)
{
        free(p);
}

static void pg_galat_init_key(void)
{
        pthread_key_create(&g_galat_key, pg_galat_hancur_key);
}

static pg_galat_slot_t *pg_galat_slot_ambil(void)
{
        pg_galat_slot_t *s;
        pthread_once(&g_galat_once, pg_galat_init_key);
        s = (pg_galat_slot_t *)pthread_getspecific(g_galat_key);
        if (!s) {
                s = (pg_galat_slot_t *)calloc(1, sizeof(*s));
                if (!s)
                        return NULL;
                pthread_setspecific(g_galat_key, s);
        }
        return s;
}
#else
static pg_galat_slot_t g_galat_global;
static pg_galat_slot_t *pg_galat_slot_ambil(void) { return &g_galat_global; }
#endif

pg_galat pg_galat_terakhir(void)
{
        pg_galat_slot_t *s = pg_galat_slot_ambil();
        return s ? s->kode : PG_OK;
}

const char *pg_galat_pesan(pg_galat e)
{
        switch (e) {
        case PG_OK:               return "tidak ada galat";
        case PG_GALAT_UMUM:       return "galat umum";
        case PG_GALAT_MEMORI:     return "kehabisan memori";
        case PG_GALAT_ARGUMEN:    return "argumen tidak valid";
        case PG_GALAT_TIDAKADA:    return "perangkat tidak ditemukan";
        case PG_GALAT_IJIN:        return "ijin ditolak";
        case PG_GALAT_SIBUK:       return "sumber daya sibuk";
        case PG_GALAT_TANPA:       return "tidak diimplementasikan di platform ini";
        case PG_GALAT_RENTANG:     return "nilai di luar rentang";
        case PG_GALAT_IO:          return "galat i/o";
        case PG_GALAT_EOF:         return "akhir stream";
        default:                   return "galat tidak dikenal";
        }
}

void pg_set_galat(pg_galat e, const char *fmt, ...)
{
        pg_galat_slot_t *s = pg_galat_slot_ambil();
        if (!s)
                return;
        s->kode = e;
        if (fmt) {
                va_list ap;
                va_start(ap, fmt);
                vsnprintf(s->pesan, sizeof(s->pesan), fmt, ap);
                va_end(ap);
        } else {
                strncpy(s->pesan, pg_galat_pesan(e), sizeof(s->pesan) - 1);
                s->pesan[sizeof(s->pesan) - 1] = '\0';
        }
}

void pg_bersih_galat(void)
{
        pg_galat_slot_t *s = pg_galat_slot_ambil();
        if (!s)
                return;
        s->kode = PG_OK;
        s->pesan[0] = '\0';
}
