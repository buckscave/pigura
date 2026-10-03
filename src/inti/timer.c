/* ----------------------------------------------------------------------------------------------
 * pigura inti: timer.c - timer satu-kali & berulang
 * ----------------------------------------------------------------------------------------------
 * Implementasi timer minimal berbasis linked list global yang
 * di-sort by expiry time. pg_timer_pompa() dipanggil dari event
 * loop tiap iterasi; timer yang sudah expired akan fire callback,
 * lalu untuk timer berulang dijadwalkan ulang (interval berikutnya).
 *
 * Thread-safety: timer system memakai mutex internal supaya
 * pg_timer_buat / pg_timer_hancur / pg_timer_pompa aman dipanggil
 * dari thread yang berbeda. Callback dipanggil DENGAN kunci dilepas
 * supaya callback boleh memanggil API timer (mis. pg_timer_hancur
 * pada dirinya sendiri, atau pg_timer_buat untuk timer baru).
 *
 * Sumber waktu: clock_gettime(CLOCK_MONOTONIC) — sama dengan
 * backend X11 supaya konsisten. Pada platform tanpa clock_gettime
 * (non-POSIX), fallback ke time(NULL)*1000 (resolusi 1 detik).
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/timer.h"
#include "pigura/untaian.h"
#include "pigura/galat.h"
#include "pigura/catat.h"

#include <stdlib.h>

#if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 199309L
#  include <time.h>
#  define PG_TIMER_HAVE_MONOTONIC 1
#endif

/* Sentinel besar untuk "tidak ada timer aktif". */
#define PG_TIMER_TIDAK_ADA  0xFFFFFFFFu

struct pg_timer {
        pg_timer_t *berikutnya;
        pg_timer_cb  cb;
        void        *ctx;
        unsigned     interval_ms;  /* 0 = one-shot */
        pg_u64       expiry_ms;    /* monotonic ms saat fire */
};

/* Global state. */
static pg_kunci_t *g_kunci = NULL;
static pg_timer_t *g_kepala = NULL;
static pg_bool     g_init = PG_SALAH;

/* ----------------------------------------------------------------- waktu
 * Ambil waktu monotonic dalam milidetik. */
static pg_u64 pg_timer_sekarang_ms(void)
{
#ifdef PG_TIMER_HAVE_MONOTONIC
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (pg_u64)ts.tv_sec * 1000ull +
               (pg_u64)ts.tv_nsec / 1000000ull;
#else
        /* Fallback kasar: time() detik -> ms. */
        return (pg_u64)time(NULL) * 1000ull;
#endif
}

/* ----------------------------------------------------------------- init
 * Init global timer system. Aman dipanggil berulang. */
pg_galat pg_timer_init(void)
{
        if (g_init) return PG_OK;
        if (pg_buat_kunci(&g_kunci) != PG_OK) {
                pg_set_galat(PG_GALAT_UMUM,
                        "timer_init: gagal buat kunci");
                return PG_GALAT_UMUM;
        }
        g_kepala = NULL;
        g_init = PG_BENAR;
        pg_info("timer: subsistem siap");
        return PG_OK;
}

/* Hancurkan semua timer + kunci. Aman bila belum init. */
void pg_timer_selesai(void)
{
        pg_timer_t *cur;
        if (!g_init) return;
        if (g_kunci) pg_kunci_kunci(g_kunci);
        cur = g_kepala;
        while (cur) {
                pg_timer_t *next = cur->berikutnya;
                free(cur);
                cur = next;
        }
        g_kepala = NULL;
        if (g_kunci) pg_kunci_buka(g_kunci);
        if (g_kunci) {
                pg_hancur_kunci(g_kunci);
                g_kunci = NULL;
        }
        g_init = PG_SALAH;
}

/* Hitung timer aktif (untuk test). */
int pg_timer_aktif(void)
{
        int n = 0;
        pg_timer_t *cur;
        if (!g_init || !g_kunci) return 0;
        pg_kunci_kunci(g_kunci);
        for (cur = g_kepala; cur; cur = cur->berikutnya) n++;
        pg_kunci_buka(g_kunci);
        return n;
}

/* ----------------------------------------------------------------- buat
 * Helper: alokasi + isi timer, sisipkan ke list sorted by expiry. */
static pg_timer_t *pg_timer_buat_internal(unsigned ms,
                                            pg_timer_cb cb, void *ctx,
                                            pg_bool berulang)
{
        pg_timer_t *t;
        pg_timer_t *cur, *prev;

        if (!g_init || !g_kunci) {
                pg_set_galat(PG_GALAT_UMUM,
                        "timer_buat: subsystem belum init");
                return NULL;
        }
        if (!cb || ms == 0) {
                pg_set_galat(PG_GALAT_ARGUMEN,
                        "timer_buat: cb NULL atau ms=0");
                return NULL;
        }

        t = (pg_timer_t *)calloc(1, sizeof(*t));
        if (!t) {
                pg_set_galat(PG_GALAT_MEMORI,
                        "timer_buat: oom");
                return NULL;
        }
        t->cb = cb;
        t->ctx = ctx;
        t->interval_ms = berulang ? ms : 0;
        t->expiry_ms = pg_timer_sekarang_ms() + (pg_u64)ms;
        t->berikutnya = NULL;

        /* Sisipkan sorted by expiry (ascending). */
        pg_kunci_kunci(g_kunci);
        prev = NULL;
        cur = g_kepala;
        while (cur && cur->expiry_ms <= t->expiry_ms) {
                prev = cur;
                cur = cur->berikutnya;
        }
        t->berikutnya = cur;
        if (prev) prev->berikutnya = t;
        else       g_kepala = t;
        pg_kunci_buka(g_kunci);
        return t;
}

pg_timer_t *pg_timer_buat(unsigned ms, pg_timer_cb cb, void *ctx)
{
        return pg_timer_buat_internal(ms, cb, ctx, PG_SALAH);
}

pg_timer_t *pg_timer_berulang(unsigned ms, pg_timer_cb cb,
                                void *ctx)
{
        return pg_timer_buat_internal(ms, cb, ctx, PG_BENAR);
}

/* ----------------------------------------------------------------- hancur
 * Lepas timer dari list lalu free. Aman bila NULL atau belum init. */
void pg_timer_hancur(pg_timer_t *t)
{
        pg_timer_t *cur, *prev;
        if (!t || !g_init || !g_kunci) return;

        pg_kunci_kunci(g_kunci);
        prev = NULL;
        cur = g_kepala;
        while (cur && cur != t) {
                prev = cur;
                cur = cur->berikutnya;
        }
        if (cur == t) {
                if (prev) prev->berikutnya = t->berikutnya;
                else       g_kepala = t->berikutnya;
        }
        pg_kunci_buka(g_kunci);
        free(t);
}

/* ----------------------------------------------------------------- pompa
 * Fire semua timer yang sudah expired. Return ms sampai timer
 * berikutnya expire (atau PG_TIMER_TIDAK_ADA bila kosong). */
unsigned pg_timer_pompa(void)
{
        pg_u64 sekarang;
        pg_u64 next_expiry;
        unsigned sisa;

        if (!g_init || !g_kunci) return PG_TIMER_TIDAK_ADA;

        sekarang = pg_timer_sekarang_ms();

        /* Loop: fire semua timer yang expiry <= sekarang. Callback
         * boleh memodifikasi list (mis. hancur diri sendiri). Karena
         * itu kita ambil timer pertama yang expired di luar kunci,
         * panggil callback tanpa kunci, lalu re-check dari awal. */
        for (;;) {
                pg_timer_t *t;
                pg_timer_cb cb;
                void *ctx;
                int berulang;
                pg_u64 expiry;

                pg_kunci_kunci(g_kunci);
                t = g_kepala;
                if (!t || t->expiry_ms > sekarang) {
                        pg_kunci_buka(g_kunci);
                        break;
                }
                /* Lepas dari list untuk fire. */
                g_kepala = t->berikutnya;
                t->berikutnya = NULL;
                cb = t->cb;
                ctx = t->ctx;
                berulang = (t->interval_ms > 0);
                expiry = t->expiry_ms;
                pg_kunci_buka(g_kunci);

                /* Panggil callback TANPA kunci supaya callback boleh
                 * memanggil pg_timer_buat / pg_timer_hancur. */
                if (cb) cb(ctx);

                if (berulang) {
                        /* Jadwalkan ulang. Untuk mencegah drift,
                         * expiry berikutnya = expiry_lama + interval
                         * (bukan sekarang + interval). Bila sudah
                         * jauh tertinggal (interval > 1 period),
                         * lompat ke sekarang + interval. */
                        pg_u64 next = expiry + t->interval_ms;
                        pg_u64 now2 = pg_timer_sekarang_ms();
                        if (next <= now2) {
                                next = now2 + t->interval_ms;
                        }
                        /* Re-sisipkan sorted. */
                        pg_kunci_kunci(g_kunci);
                        {
                                pg_timer_t *prev = NULL;
                                pg_timer_t *cur = g_kepala;
                                t->expiry_ms = next;
                                while (cur && cur->expiry_ms <= next) {
                                        prev = cur;
                                        cur = cur->berikutnya;
                                }
                                t->berikutnya = cur;
                                if (prev) prev->berikutnya = t;
                                else       g_kepala = t;
                        }
                        pg_kunci_buka(g_kunci);
                } else {
                        free(t);
                }
        }

        /* Hitung sisa waktu sampai timer berikutnya expire. */
        pg_kunci_kunci(g_kunci);
        if (!g_kepala) {
                pg_kunci_buka(g_kunci);
                return PG_TIMER_TIDAK_ADA;
        }
        next_expiry = g_kepala->expiry_ms;
        pg_kunci_buka(g_kunci);

        if (next_expiry <= sekarang) return 0;
        sisa = (unsigned)(next_expiry - sekarang);
        /* Batasi ke maksimum 1 menit supaya event loop tetap
         * responsif terhadap peristiwa eksternal (jendela close
         * dsb.). */
        if (sisa > 60000u) sisa = 60000u;
        return sisa;
}
