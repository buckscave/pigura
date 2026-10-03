/* ----------------------------------------------------------------------------------------------
 * pigura/catat.h - Fasilitas pencatatan (logging)
 * ----------------------------------------------------------------------------------------------
 * Logger minimal berbasis stderr dengan level severity. Bisa dimatikan
 * total saat build dengan -DPIGURA_CATAT_MATI. Arahkan output ke file
 * dengan pg_catat_set_fp().
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_CATAT_H
#define PIGURA_CATAT_H

#include <stdio.h>
#include "pigura/tipe.h"

#ifdef __cplusplus
extern "C" {
#endif

enum pg_catat_level {
        PG_CATAT_MATI   = 0,
        PG_CATAT_FATAL   = 1,
        PG_CATAT_GALAT   = 2,
        PG_CATAT_PERINGATAN = 3,
        PG_CATAT_INFO    = 4,
        PG_CATAT_DEBUG   = 5,
        PG_CATAT_REKAM   = 6
};

void pg_catat_init(const char *tag);
void pg_catat_set_level(int level);
int  pg_catat_get_level(void);
void pg_catat_set_fp(FILE *fp);
FILE *pg_catat_get_fp(void);

void pg_catat_v(int level, const char *file, int line, const char *fmt, ...);

#ifndef PIGURA_CATAT_MATI
#  define pg_catat(level, ...) \
        pg_catat_v((level), __FILE__, __LINE__, __VA_ARGS__)
#  define pg_fatal(...) pg_catat(PG_CATAT_FATAL, __VA_ARGS__)
#  define pg_galat_log(...) pg_catat(PG_CATAT_GALAT, __VA_ARGS__)
#  define pg_peringatan(...) pg_catat(PG_CATAT_PERINGATAN, __VA_ARGS__)
#  define pg_info(...) pg_catat(PG_CATAT_INFO, __VA_ARGS__)
#  define pg_debug_log(...) pg_catat(PG_CATAT_DEBUG, __VA_ARGS__)
#  define pg_rekam(...) pg_catat(PG_CATAT_REKAM, __VA_ARGS__)
#else
#  define pg_catat(level, ...) do {} while (0)
#  define pg_fatal(...) do {} while (0)
#  define pg_galat_log(...) do {} while (0)
#  define pg_peringatan(...) do {} while (0)
#  define pg_info(...) do {} while (0)
#  define pg_debug_log(...) do {} while (0)
#  define pg_rekam(...) do {} while (0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_CATAT_H */
