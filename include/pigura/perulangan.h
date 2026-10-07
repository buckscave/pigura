/* ----------------------------------------------------------------------------------------------
 * pigura/perulangan.h - Loop peristiwa
 * ----------------------------------------------------------------------------------------------
 * Perulangan peristiwa single-threaded yang didukung oleh produsen
 * multithread (thread input memompa peristiwa; thread timer
 * menjadwalkan timeout). Aplikasi memanggil pg_jalankan_perulangan()
 * dengan handler untuk tiap kategori peristiwa.
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_PERULANGAN_H
#define PIGURA_PERULANGAN_H

#include "pigura/tipe.h"
#include "pigura/aksi.h"
#include "pigura/masukan.h"
#include "pigura/layar.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pg_perulangan pg_perulangan_t;

typedef void (*pg_aksi_cb)(const pg_aksi_t *e, void *ctx);
typedef void (*pg_idle_cb)(void *ctx);

typedef struct pg_perulangan_config {
        pg_masukan_t *masukan;     /* boleh NULL */
        pg_layar_t    *layar;      /* boleh NULL */
        pg_idle_cb     idle;       /* dipanggil tiap iterasi tanpa peristiwa */
        void          *idle_ctx;
        pg_bool       pompa_masukan; /* pompa masukan dari thread bg */
} pg_perulangan_config_t;

/* Buat perulangan peristiwa baru. */
pg_galat pg_buat_perulangan(pg_perulangan_t **out,
                              const pg_perulangan_config_t *cfg);

/* Hancurkan perulangan. */
pg_galat pg_hancur_perulangan(pg_perulangan_t *loop);

/* Jalankan sampai pg_hentikan_perulangan() dipanggil. */
pg_galat pg_jalankan_perulangan(pg_perulangan_t *loop,
                                  pg_aksi_cb cb, void *ctx);

/* Sinyal berhenti. Thread-safe. */
void pg_hentikan_perulangan(pg_perulangan_t *loop);

/* Dorong peristiwa sintetik. Thread-safe. */
pg_galat pg_perulangan_kirim(pg_perulangan_t *loop,
                              const pg_aksi_t *e);

/* Polling tanpa blok. */
int pg_perulangan_poll(pg_perulangan_t *loop, pg_aksi_t *buf, int maks);

/* Tunggu peristiwa. */
int pg_perulangan_tunggu(pg_perulangan_t *loop, pg_aksi_t *buf,
                          int maks, unsigned timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_PERULANGAN_H */
