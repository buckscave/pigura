/* ----------------------------------------------------------------------------------------------
 * pigura/untaian.h - Primitif threading
 * ----------------------------------------------------------------------------------------------
 * Pembungkus tipis POSIX pthread. Di Windows diemulasikan dengan
 * API Win32; di platform tanpa thread (beberapa embedded) seluruh
 * backend runtuh menjadi single-threaded stub.
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_UNTAIAN_H
#define PIGURA_UNTAIAN_H

#include "pigura/tipe.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pg_kunci    pg_kunci_t;
typedef struct pg_kondisi  pg_kondisi_t;
typedef struct pg_untaian  pg_untaian_t;

typedef void *(*pg_untaian_fn)(void *arg);

pg_galat pg_buat_kunci(pg_kunci_t **out);
pg_galat pg_hancur_kunci(pg_kunci_t *k);
pg_galat pg_kunci_kunci(pg_kunci_t *k);
pg_galat pg_kunci_buka(pg_kunci_t *k);

pg_galat pg_buat_kondisi(pg_kondisi_t **out);
pg_galat pg_hancur_kondisi(pg_kondisi_t *k);
pg_galat pg_kondisi_tunggu(pg_kondisi_t *k, pg_kunci_t *m);
pg_galat pg_kondisi_sinyal(pg_kondisi_t *k);
pg_galat pg_kondisi_siar(pg_kondisi_t *k);
pg_galat pg_kondisi_tunggu_batas(pg_kondisi_t *k, pg_kunci_t *m,
                                  unsigned batas_ms);

pg_galat pg_buat_untaian(pg_untaian_t **out, pg_untaian_fn fn, void *arg);
pg_galat pg_gabung_untaian(pg_untaian_t *t, void **hasil);
pg_galat pg_lepas_untaian(pg_untaian_t *t);

/* Tidur selama milidetik yang diberikan. */
void pg_tidur_ms(unsigned ms);

/* Menyerah eksekusi thread saat ini. */
void pg_menyerah(void);

/* Operasi atomik 32-bit. */
pg_s32 pg_atom_muat(volatile pg_s32 *p);
void   pg_atom_simpan(volatile pg_s32 *p, pg_s32 v);
pg_s32  pg_atom_tambah(volatile pg_s32 *p, pg_s32 delta);
pg_s32  pg_atom_cas(volatile pg_s32 *p, pg_s32 diharapkan, pg_s32 diinginkan);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_UNTAIAN_H */
