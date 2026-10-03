/* ----------------------------------------------------------------------------------------------
 * pigura inti: untaian.c - pembungkus pthread
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/untaian.h"
#include "pigura/galat.h"
#include "pigura/catat.h"

#include <stdlib.h>

#if defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  define PG_Pakai_Win32 1
#elif defined(_POSIX_THREADS) || defined(__unix__) || defined(__APPLE__)
#  include <pthread.h>
#  include <time.h>
#  include <errno.h>
#  include <sched.h>
#  define PG_Pakai_Pthread 1
#endif

struct pg_kunci {
#ifdef PG_Pakai_Pthread
        pthread_mutex_t m;
#elif defined(PG_Pakai_Win32)
        CRITICAL_SECTION cs;
#endif
};

struct pg_kondisi {
#ifdef PG_Pakai_Pthread
        pthread_cond_t c;
#elif defined(PG_Pakai_Win32)
        HANDLE evt;
        int waiting;
        CRITICAL_SECTION lock;
#endif
};

struct pg_untaian {
#ifdef PG_Pakai_Pthread
        pthread_t t;
#elif defined(PG_Pakai_Win32)
        HANDLE h;
        DWORD id;
#endif
        void *hasil;
};

/*
 * Win32 trampoline: pg_untaian_fn mengembalikan void* (pointer-sized)
 * tapi LPTHREAD_START_ROUTINE harus mengembalikan DWORD (32-bit).
 * Kita jembatani dengan context kecil yang menangkap fn/arg user.
 */
typedef struct {
        pg_untaian_fn fn;
        void          *arg;
        void          *hasil;
} pg_untaian_ctx;

#ifdef PG_Pakai_Win32
static DWORD WINAPI pg_untaian_trampolin(LPVOID raw)
{
        pg_untaian_ctx *c = (pg_untaian_ctx *)raw;
        c->hasil = c->fn(c->arg);
        return 0;
}
#endif

pg_galat pg_buat_kunci(pg_kunci_t **out)
{
        pg_kunci_t *k;
        if (!out) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
        k = (pg_kunci_t *)calloc(1, sizeof(*k));
        if (!k) PG_KEMBALI_GALAT(PG_GALAT_MEMORI);
#if defined(PG_Pakai_Pthread)
        if (pthread_mutex_init(&k->m, NULL) != 0) {
                free(k);
                PG_KEMBALI_GALAT(PG_GALAT_UMUM);
        }
#elif defined(PG_Pakai_Win32)
        InitializeCriticalSection(&k->cs);
#else
        free(k);
        PG_KEMBALI_GALAT(PG_GALAT_TANPA);
#endif
        *out = k;
        return PG_OK;
}

pg_galat pg_hancur_kunci(pg_kunci_t *k)
{
        if (!k) return PG_OK;
#if defined(PG_Pakai_Pthread)
        pthread_mutex_destroy(&k->m);
#elif defined(PG_Pakai_Win32)
        DeleteCriticalSection(&k->cs);
#endif
        free(k);
        return PG_OK;
}

pg_galat pg_kunci_kunci(pg_kunci_t *k)
{
        if (!k) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
#if defined(PG_Pakai_Pthread)
        pthread_mutex_lock(&k->m);
#elif defined(PG_Pakai_Win32)
        EnterCriticalSection(&k->cs);
#endif
        return PG_OK;
}

pg_galat pg_kunci_buka(pg_kunci_t *k)
{
        if (!k) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
#if defined(PG_Pakai_Pthread)
        pthread_mutex_unlock(&k->m);
#elif defined(PG_Pakai_Win32)
        LeaveCriticalSection(&k->cs);
#endif
        return PG_OK;
}

pg_galat pg_buat_kondisi(pg_kondisi_t **out)
{
        pg_kondisi_t *k;
        if (!out) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
        k = (pg_kondisi_t *)calloc(1, sizeof(*k));
        if (!k) PG_KEMBALI_GALAT(PG_GALAT_MEMORI);
#if defined(PG_Pakai_Pthread)
        if (pthread_cond_init(&k->c, NULL) != 0) {
                free(k);
                PG_KEMBALI_GALAT(PG_GALAT_UMUM);
        }
#elif defined(PG_Pakai_Win32)
        k->evt = CreateEvent(NULL, FALSE, FALSE, NULL);
        InitializeCriticalSection(&k->lock);
        k->waiting = 0;
#else
        free(k);
        PG_KEMBALI_GALAT(PG_GALAT_TANPA);
#endif
        *out = k;
        return PG_OK;
}

pg_galat pg_hancur_kondisi(pg_kondisi_t *k)
{
        if (!k) return PG_OK;
#if defined(PG_Pakai_Pthread)
        pthread_cond_destroy(&k->c);
#elif defined(PG_Pakai_Win32)
        CloseHandle(k->evt);
        DeleteCriticalSection(&k->lock);
#endif
        free(k);
        return PG_OK;
}

pg_galat pg_kondisi_tunggu(pg_kondisi_t *k, pg_kunci_t *m)
{
        if (!k || !m) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
#if defined(PG_Pakai_Pthread)
        pthread_cond_wait(&k->c, &m->m);
        return PG_OK;
#elif defined(PG_Pakai_Win32)
        return pg_kondisi_tunggu_batas(k, m, 0xffffffff);
#else
        PG_KEMBALI_GALAT(PG_GALAT_TANPA);
#endif
}

pg_galat pg_kondisi_sinyal(pg_kondisi_t *k)
{
        if (!k) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
#if defined(PG_Pakai_Pthread)
        pthread_cond_signal(&k->c);
#elif defined(PG_Pakai_Win32)
        EnterCriticalSection(&k->lock);
        if (k->waiting > 0)
                SetEvent(k->evt);
        LeaveCriticalSection(&k->lock);
#endif
        return PG_OK;
}

pg_galat pg_kondisi_siar(pg_kondisi_t *k)
{
        if (!k) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
#if defined(PG_Pakai_Pthread)
        pthread_cond_broadcast(&k->c);
#elif defined(PG_Pakai_Win32)
        pg_kondisi_sinyal(k);
#endif
        return PG_OK;
}

pg_galat pg_kondisi_tunggu_batas(pg_kondisi_t *k, pg_kunci_t *m,
                                  unsigned batas_ms)
{
#if defined(PG_Pakai_Pthread)
        struct timespec ts;
#endif
        if (!k || !m) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
#if defined(PG_Pakai_Pthread)
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec  += batas_ms / 1000u;
        ts.tv_nsec += (long)(batas_ms % 1000u) * 1000000L;
        if (ts.tv_nsec >= 1000000000L) {
                ts.tv_sec++;
                ts.tv_nsec -= 1000000000L;
        }
        return pthread_cond_timedwait(&k->c, &m->m, &ts) ?
                PG_GALAT_UMUM : PG_OK;
#elif defined(PG_Pakai_Win32)
        {
                DWORD rv;
                EnterCriticalSection(&k->lock);
                k->waiting++;
                LeaveCriticalSection(&k->lock);

                LeaveCriticalSection(&m->cs);
                rv = WaitForSingleObject(k->evt, batas_ms);
                EnterCriticalSection(&m->cs);
                EnterCriticalSection(&k->lock);
                k->waiting--;
                LeaveCriticalSection(&k->lock);
                return rv == WAIT_OBJECT_0 ? PG_OK : PG_GALAT_UMUM;
        }
#else
        PG_KEMBALI_GALAT(PG_GALAT_TANPA);
#endif
}

pg_galat pg_buat_untaian(pg_untaian_t **out, pg_untaian_fn fn, void *arg)
{
        pg_untaian_t *t;
#ifdef PG_Pakai_Win32
        pg_untaian_ctx *ctx;
#endif
        if (!out || !fn) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
        t = (pg_untaian_t *)calloc(1, sizeof(*t));
        if (!t) PG_KEMBALI_GALAT(PG_GALAT_MEMORI);
        t->hasil = NULL;
#if defined(PG_Pakai_Pthread)
        if (pthread_create(&t->t, NULL, fn, arg) != 0) {
                free(t);
                PG_KEMBALI_GALAT(PG_GALAT_UMUM);
        }
#elif defined(PG_Pakai_Win32)
        ctx = (pg_untaian_ctx *)calloc(1, sizeof(*ctx));
        if (!ctx) { free(t); PG_KEMBALI_GALAT(PG_GALAT_MEMORI); }
        ctx->fn = fn;
        ctx->arg = arg;
        ctx->hasil = NULL;
        t->h = CreateThread(NULL, 0, pg_untaian_trampolin, ctx, 0, &t->id);
        if (!t->h) {
                free(ctx);
                free(t);
                PG_KEMBALI_GALAT(PG_GALAT_UMUM);
        }
#else
        free(t);
        PG_KEMBALI_GALAT(PG_GALAT_TANPA);
#endif
        *out = t;
        return PG_OK;
}

pg_galat pg_gabung_untaian(pg_untaian_t *t, void **hasil)
{
        if (!t) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
#if defined(PG_Pakai_Pthread)
        if (pthread_join(t->t, hasil ? hasil : &t->hasil) != 0)
                PG_KEMBALI_GALAT(PG_GALAT_UMUM);
#elif defined(PG_Pakai_Win32)
        WaitForSingleObject(t->h, INFINITE);
        if (hasil) *hasil = t->hasil;
        CloseHandle(t->h);
#endif
        free(t);
        return PG_OK;
}

pg_galat pg_lepas_untaian(pg_untaian_t *t)
{
        if (!t) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
#if defined(PG_Pakai_Pthread)
        pthread_detach(t->t);
#elif defined(PG_Pakai_Win32)
        CloseHandle(t->h);
#endif
        free(t);
        return PG_OK;
}

void pg_tidur_ms(unsigned ms)
{
#if defined(_WIN32)
        Sleep(ms);
#else
        struct timespec ts;
        ts.tv_sec  = ms / 1000u;
        ts.tv_nsec = (long)(ms % 1000u) * 1000000L;
        nanosleep(&ts, NULL);
#endif
}

void pg_menyerah(void)
{
#if defined(_WIN32)
        Sleep(0);
#else
        sched_yield();
#endif
}

/* ---- atom ops ---- */
pg_s32 pg_atom_muat(volatile pg_s32 *p)
{
#if defined(__GNUC__) || defined(__clang__)
        return __atomic_load_n((pg_s32 *)p, __ATOMIC_SEQ_CST);
#else
        return *p;
#endif
}

void pg_atom_simpan(volatile pg_s32 *p, pg_s32 v)
{
#if defined(__GNUC__) || defined(__clang__)
        __atomic_store_n((pg_s32 *)p, v, __ATOMIC_SEQ_CST);
#else
        *p = v;
#endif
}

pg_s32 pg_atom_tambah(volatile pg_s32 *p, pg_s32 delta)
{
#if defined(__GNUC__) || defined(__clang__)
        return __atomic_add_fetch((pg_s32 *)p, delta, __ATOMIC_SEQ_CST);
#else
        *p += delta;
        return *p;
#endif
}

pg_s32 pg_atom_cas(volatile pg_s32 *p, pg_s32 diharapkan, pg_s32 diinginkan)
{
#if defined(__GNUC__) || defined(__clang__)
        return __atomic_compare_exchange_n((pg_s32 *)p, &diharapkan,
                                            diinginkan, 0,
                                            __ATOMIC_SEQ_CST,
                                            __ATOMIC_SEQ_CST);
#else
        if (*p == diharapkan) { *p = diinginkan; return 1; }
        return 0;
#endif
}
