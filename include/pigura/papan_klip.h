/* ----------------------------------------------------------------------------------------------
 * pigura/papan_klip.h - Papan klip (clipboard) cross-platform
 * ----------------------------------------------------------------------------------------------
 * pg_papan_klip_t adalah handle ke clipboard sistem: tulis teks ke
 * clipboard, baca teks dari clipboard. Singleton per aplikasi - bila
 * pg_papan_klip_buka dipanggil dua kali dengan layar yang sama,
 * instance yang sama dikembalikan (refcount internal).
 *
 * Backend:
 *   x11:     XStoreBytes / XFetchBytes (XA_CUT_BUFFER0)
 *   windows: OpenClipboard + SetClipboardData(CF_TEXT)
 *   fallback: in-memory buffer (untuk uji-sendiri tanpa display)
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_PAPAN_KLIP_H
#define PIGURA_PAPAN_KLIP_H

#include "pigura/tipe.h"
#include "pigura/galat.h"

/* Forward declaration - layar adalah handle opaque. */
struct pg_layar;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pg_papan_klip pg_papan_klip_t;

/* Buka clipboard (singleton per app). layar boleh NULL: bila NULL
 * atau bila backend tidak mendukung clipboard, dipakai in-memory
 * fallback supaya API tetap dapat dipakai. */
pg_papan_klip_t *pg_papan_klip_buka(struct pg_layar *layar);

/* Tutup clipboard. Setelah ini, pointer tidak boleh dipakai. */
void pg_papan_klip_tutup(pg_papan_klip_t *k);

/* Tulis teks ke clipboard. teks boleh NULL (setel ke kosong).
 * Mengembalikan PG_OK bila sukses, kode galat negatif bila gagal. */
pg_galat pg_papan_klip_tulis_teks(pg_papan_klip_t *k,
                                    const char *teks);

/* Baca teks dari clipboard. Mengembalikan string malloc'd; caller
 * wajib free(). Mengembalikan NULL bila kosong atau galat. */
char *pg_papan_klip_baca_teks(pg_papan_klip_t *k);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_PAPAN_KLIP_H */
