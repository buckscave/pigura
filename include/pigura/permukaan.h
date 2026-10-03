/* ----------------------------------------------------------------------------------------------
 * pigura/permukaan.h - Permukaan gambar offscreen
 * ----------------------------------------------------------------------------------------------
 * pg_permukaan_t adalah buffer raster persegi yang digambar aplikasi.
 * Ia melacak "region kotor" (union kotak kotor) yang dipakai
 * kompositor untuk meminimalkan pekerjaan blit per frame.
 *
 * Permukaan adalah blok bangunan widget, jendela, dan back buffer
 * layar itu sendiri.
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_PERMUKAAN_H
#define PIGURA_PERMUKAAN_H

#include "pigura/tipe.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pg_permukaan pg_permukaan_t;

/* Alokasikan permukaan baru dengan ukuran yang diberikan. NULL jika gagal. */
pg_permukaan_t *pg_buat_permukaan(int lebar, int tinggi);

/* Kloning pixel dari permukaan lain. */
pg_permukaan_t *pg_klon_permukaan(const pg_permukaan_t *src);

/* Bebaskan permukaan. */
void pg_hancur_permukaan(pg_permukaan_t *s);

/* Bungkus buffer memori yang sudah ada. Permukaan tidak punya buffer. */
pg_permukaan_t *pg_bungkus_permukaan(void *piksel, int lebar, int tinggi,
                                      int langkah);

/* Akses geometri. */
int pg_permukaan_lebar(const pg_permukaan_t *s);
int pg_permukaan_tinggi(const pg_permukaan_t *s);
int pg_permukaan_langkah(const pg_permukaan_t *s);

/* Ambil pointer read-only ke buffer piksel. */
const void *pg_permukaan_piksel(const pg_permukaan_t *s);

/* Ambil pointer writable ke buffer piksel. */
void *pg_permukaan_piksel_mut(pg_permukaan_t *s);

/* Isi seluruh permukaan dengan warna solid. */
void pg_isi_permukaan(pg_permukaan_t *s, pg_warna_t c);

/* Isi sub-kotak. */
void pg_isi_permukaan_kotak(pg_permukaan_t *s, pg_kotak_t r, pg_warna_t c);

/* Setel satu piksel (terpotong). */
void pg_setel_piksel_permukaan(pg_permukaan_t *s, int x, int y,
                                pg_warna_t c);

/* Ambil warna piksel pada (x, y). Return 0 bila di luar batas. */
pg_warna_t pg_ambil_piksel_permukaan(const pg_permukaan_t *s, int x, int y);

/* Garis horizontal / vertikal. */
void pg_garis_h_permukaan(pg_permukaan_t *s, int x0, int x1, int y,
                           pg_warna_t c);
void pg_garis_v_permukaan(pg_permukaan_t *s, int x, int y0, int y1,
                           pg_warna_t c);

/* Outline kotak. */
void pg_kotak_permukaan(pg_permukaan_t *s, pg_kotak_t r, pg_warna_t c);

/* Blit src ke dst pada (dx,dy). Tanpa alpha. */
void pg_blit_permukaan(pg_permukaan_t *dst, int dx, int dy,
                        const pg_permukaan_t *src);

/* Blit src ke dst pada (dx,dy), dipotong ke clip. */
void pg_blit_potong_permukaan(pg_permukaan_t *dst, int dx, int dy,
                                const pg_permukaan_t *src,
                                pg_kotak_t clip);

/* Alpha-blend src ke dst pada (dx,dy). */
void pg_blit_alpha_permukaan(pg_permukaan_t *dst, int dx, int dy,
                              const pg_permukaan_t *src);

/* Blit sub-kotak src ke dst pada (dx,dy). */
void pg_blit_sub_permukaan(pg_permukaan_t *dst, int dx, int dy,
                            const pg_permukaan_t *src, pg_kotak_t sr);

/* Region kotor: tandai / ambil kotak kotor. */
void pg_permukaan_kotor(pg_permukaan_t *s, pg_kotak_t r);
pg_kotak_t pg_permukaan_ambil_kotor(const pg_permukaan_t *s);
void pg_permukaan_bersih_kotor(pg_permukaan_t *s);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_PERMUKAAN_H */
