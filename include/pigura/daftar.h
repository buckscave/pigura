/* ----------------------------------------------------------------------------------------------
 * pigura/daftar.h - Widget list view vertikal
 * ----------------------------------------------------------------------------------------------
 * Daftar adalah list view vertikal berisi string. Satu item dapat
 * terpilih pada satu waktu. Klik kiri memilih item berdasarkan
 * koordinat Y; roda mouse menggulir konten.
 *
 * Tinggi baris diambil dari font tinggi_baris. Item yang terpotong
 * oleh viewport atas/bawah tidak digambar.
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_DAFTAR_H
#define PIGURA_DAFTAR_H

#include "pigura/tipe.h"
#include "pigura/font.h"
#include "pigura/widget.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pg_daftar pg_daftar_t;

/* Dipanggil saat item terpilih berubah. */
typedef void (*pg_daftar_cb)(pg_daftar_t *d, int idx, void *ctx);

/* Buat list view. */
pg_daftar_t *pg_buat_daftar(pg_font_t *font);

/* Bebaskan list view. */
void pg_daftar_hancur(pg_daftar_t *d);

/* Tambah item. String disalin internal. */
void pg_daftar_tambah(pg_daftar_t *d, const char *teks);

/* Hapus semua item. */
void pg_daftar_bersih(pg_daftar_t *d);

/* Ambil indeks terpilih (-1 jika tidak ada). */
int pg_daftar_terpilih(pg_daftar_t *d);

/* Setel terpilih (diklem ke rentang). */
void pg_daftar_setel_terpilih(pg_daftar_t *d, int idx);

/* Setel callback pilihan. */
void pg_daftar_saatpilih(pg_daftar_t *d, pg_daftar_cb cb,
			  void *ctx);

/* Akses widget dasar. */
pg_widget_t *pg_daftar_widget(pg_daftar_t *d);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_DAFTAR_H */
