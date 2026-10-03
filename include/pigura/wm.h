/* ----------------------------------------------------------------------------------------------
 * pigura/wm.h - Window manager untuk mode standalone (framebuffer)
 * ----------------------------------------------------------------------------------------------
 * Saat pigura berjalan di mode framebuffer (tanpa X11/Win32/Cocoa),
 * WM internal mengelola jendela pigura sendiri. Berbeda dari mode
 * numpang (host WM), di sini pigura jadi host.
 *
 * Hanya aktif saat pakai backend linuxfb. Di mode numpang
 * (X11/Win32/Cocoa), WM host yang urus jendela.
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_WM_H
#define PIGURA_WM_H

#include "pigura/tipe.h"
#include "pigura/permukaan.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pg_wm pg_wm_t;
typedef struct pg_wm_jendela pg_wm_jendela_t;

/* Buat WM dengan ukuran layar. */
pg_wm_t *pg_wm_buat(int lebar, int tinggi);

/* Hancurkan WM (serta semua jendela yang masih ada). */
void pg_wm_hancur(pg_wm_t *wm);

/* Buat jendela top-level. Jendela baru otomatis di atas z-order. */
pg_wm_jendela_t *pg_wm_buat_jendela(pg_wm_t *wm, int x, int y,
                                      int w, int h, const char *judul);

/* Hancurkan jendela. */
void pg_wm_hancur_jendela(pg_wm_jendela_t *j);

/* Ambil permukaan jendela (untuk gambar). */
pg_permukaan_t *pg_wm_jendela_permukaan(pg_wm_jendela_t *j);

/* Pindah jendela ke posisi baru. */
void pg_wm_jendela_pindah(pg_wm_jendela_t *j, int x, int y);

/* Raise jendela ke atas z-order. */
void pg_wm_jendela_naik(pg_wm_jendela_t *j);

/* Composite semua jendela ke permukaan target (framebuffer).
 * Iterasi dari bottom ke top; jendela atas menutup yang bawah. */
void pg_wm_composite(pg_wm_t *wm, pg_permukaan_t *dest);

/* Hit-test: cari jendela teratas di posisi p. */
pg_wm_jendela_t *pg_wm_pilih(pg_wm_t *wm, pg_titik_t p);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_WM_H */
