/* ----------------------------------------------------------------------------------------------
 * pigura/masukan.h - Abstraksi perangkat input
 * ----------------------------------------------------------------------------------------------
 * pg_masukan_t menangkap peristiwa keyboard dan pointer dari subsistem
 * input native platform (X11 di desktop Linux, RawInput di Windows,
 * CGEventTap di macOS, evdev di embedded/VT). Peristiwa didorong ke
 * antrian loop; app menariknya via pg_perulangan_poll().
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_MASUKAN_H
#define PIGURA_MASUKAN_H

#include "pigura/tipe.h"
#include "pigura/peristiwa.h"
#include "pigura/layar.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pg_masukan pg_masukan_t;

typedef struct pg_masukan_config {
        /*
         * Path perangkat opsional. NULL berarti auto-discover.
         * Linux evdev: papan_tombol="/dev/input/event0"
         */
        const char *papan_tombol_dev;
        const char *tetik_dev;
        pg_bool cengkam;          /* exclusive grab */
} pg_masukan_config_t;

/*
 * Buka perangkat input. Di X11 / Windows / macOS ini dipegang dari
 * antrian peristiwa windowing; di embedded ini membuka evdev.
 */
pg_galat pg_buka_masukan(pg_masukan_t **out, const pg_masukan_config_t *cfg,
                          pg_layar_t *layar);

/* Hentikan thread input dan lepaskan handle perangkat. */
pg_galat pg_tutup_masukan(pg_masukan_t *in);

/* Dorong peristiwa sintetik ke antrian keluaran input. */
pg_galat pg_masukan_emit(pg_masukan_t *in, const pg_peristiwa_t *e);

/* Pompa semua peristiwa yang tertunda. Mengembalikan jumlah yang ditarik. */
int pg_masukan_tarik(pg_masukan_t *in, pg_peristiwa_t *buf, int maks);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_MASUKAN_H */
