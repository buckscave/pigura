/* ----------------------------------------------------------------------------------------------
 * pigura/galat.h - Pelaporan galat
 * ----------------------------------------------------------------------------------------------
 * Galat dilaporkan lewat slot per-thread ala errno, diakses lewat
 * pg_galat_terakhir() dan pg_galat_pesan(). Fungsi mengembalikan
 * PG_OK saat sukses atau kode PG_GALAT_* negatif; alasan terperinci
 * disimpan di slot per-thread untuk dipanggil memeriksa.
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_GALAT_H
#define PIGURA_GALAT_H

#include "pigura/tipe.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Ambil kode galat terakhir untuk thread ini. */
pg_galat pg_galat_terakhir(void);

/* Ambil deskripsi galat sebagai string. */
const char *pg_galat_pesan(pg_galat e);

/* Setel galat untuk thread ini. Utamanya dipakai internal backend. */
void pg_set_galat(pg_galat e, const char *fmt, ...);

/* Bersihkan slot galat. */
void pg_bersih_galat(void);

/* Makro kenyamanan: keluar dari fungsi dengan galat. */
#define PG_KEMBALI_GALAT(kode) do { pg_set_galat(kode, 0); return (kode); } while (0)

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_GALAT_H */
