/* -------------------------------------------------------------------------- *
 * pigura/gambar/codec.h - codec gambar (API publik)
 * -------------------------------------------------------------------------- *
 * Memuat dan menyimpan gambar untuk UI toolkit.
 *
 * Format yang didukung (UI-relevant):
 *   Muat:  PNG, JPEG, BMP, GIF, PNM/PPM, XPM
 *   Simpan: PNG (DEFLATE compression), JPEG (baseline), BMP, PPM
 *
 * Format yang TIDAK didukung (domain image editor):
 *   PSD, TIFF, HDR, TGA, PIC, WebP, AVIF
 *   Aplikasi image editor harus implementasi codec sendiri.
 *
 * API:
 *   pg_muat_gambar(nama_berkas, &lebar, &tinggi, &channel, req_channel)
 *   pg_muat_gambar_dari_memori(data, ukuran, &lebar, &tinggi, ...)
 *   pg_simpan_png(nama_berkas, lebar, tinggi, channel, pixel)
 *   pg_simpan_jpeg(nama_berkas, lebar, tinggi, channel, pixel, kualitas)
 *   pg_gambar_bebas(pixel)
 *
 * Format yang didukung (Fase 1):
 *   Muat:  PNG, JPEG, BMP, GIF, PSD, PNM
 *   Simpan: PNG, JPEG, BMP (Fase 2)
 *
 * Format yang akan didukung (Fase 2+):
 *   TIFF, WebP, XPM
 *
 * Implementasi internal ada di codec_internal.h (private, jangan include
 * dari aplikasi). Decoder diadaptasi dari stb_image v2.30 (public domain)
 * dengan rename identifiers ke pg_*.
 * -------------------------------------------------------------------------- */
#ifndef PIGURA_CODEC_H
#define PIGURA_CODEC_H

#include "pigura/tipe.h"
#include "pigura/permukaan.h"

/* Tipe byte/word untuk data piksel. */
typedef pg_u8  pg_byte;
typedef pg_u16 pg_word16;

#ifdef __cplusplus
extern "C" {
#endif

/* ===== Tipe data ===== */

/* Channel gambar. */
#define PG_GAMBAR_ABU      1
#define PG_GAMBAR_ABU_A    2
#define PG_GAMBAR_RGB      3
#define PG_GAMBAR_RGBA     4

typedef struct pg_info_gambar {
	int lebar;
	int tinggi;
	int channel;
	int kedalaman_bit;
} pg_info_gambar_t;

/* Callback I/O untuk muat dari sumber custom. */
typedef struct {
	int (*baca)  (void *pengguna, pg_byte *buf, int ukuran);
	void (*lompat)(void *pengguna, int n);
	int (*akhir)  (void *pengguna);
} pg_io_callbacks_t;

/* ===== Muat gambar (decode) ===== */

pg_byte *pg_muat_gambar(const char *nama_berkas,
                        int *lebar, int *tinggi,
                        int *channel_aktual, int req_channel);

pg_byte *pg_muat_gambar_dari_memori(const pg_byte *memori, int ukuran,
                                     int *lebar, int *tinggi,
                                     int *channel_aktual, int req_channel);

pg_word16 *pg_muat_gambar_16(const char *nama_berkas,
                              int *lebar, int *tinggi,
                              int *channel_aktual, int req_channel);

float *pg_muat_gambar_hdr(const char *nama_berkas,
                           int *lebar, int *tinggi,
                           int *channel_aktual, int req_channel);

pg_byte *pg_muat_gambar_gif(const pg_byte *memori, int ukuran,
                             int **delays,
                             int *lebar, int *tinggi, int *jumlah_frame,
                             int *channel, int req_channel);

int pg_info_gambar(const char *nama_berkas, int *lebar, int *tinggi,
                    int *channel);
int pg_info_gambar_dari_memori(const pg_byte *memori, int ukuran,
                                int *lebar, int *tinggi, int *channel);

int pg_gambar_apa_hdr(const char *nama_berkas);
int pg_gambar_apa_16_bit_dari_memori(const pg_byte *memori, int ukuran);

void pg_gambar_bebas(void *pixel);
const char *pg_gambar_galat(void);

/* ===== Simpan gambar (encode) - Fase 2 ===== */

int pg_simpan_png(const char *nama_berkas, int lebar, int tinggi,
                  int channel, const pg_byte *pixel);
int pg_simpan_jpeg(const char *nama_berkas, int lebar, int tinggi,
                   int channel, const pg_byte *pixel, int kualitas);
int pg_simpan_bmp(const char *nama_berkas, int lebar, int tinggi,
                  int channel, const pg_byte *pixel);

/* Simpan PPM (format simpel untuk debug, no compression). */
int pg_simpan_ppm(const char *nama_berkas, int lebar, int tinggi,
                  int channel, const pg_byte *pixel);

/* Muat PPM P6 dari memori (decoder simpel, no library). */
pg_byte *pg_muat_ppm_dari_memori(const pg_byte *memori, int ukuran,
                                   int *lebar, int *tinggi,
                                   int *channel_aktual);

/* Muat XPM dari array string C (format X11 Pixmap, untuk ikon).
 * Format: const char *ikon[] = { "16 16 3 1", "  c #000000", ... };
 * Return pixel data RGBA. */
pg_byte *pg_muat_xpm_dari_string(const char *xpm[],
                                    int *lebar, int *tinggi,
                                    int *channel_aktual);

/* ===== Konfigurasi ===== */

void pg_setel_premultiply_saat_muat(int aktif);
void pg_setel_balik_y_saat_muat(int aktif);

/* ===== Helper untuk pigura permukaan ===== */

pg_permukaan_t *pg_muat_permukaan(const char *nama_berkas);
int pg_simpan_permukaan(const char *nama_berkas, const pg_permukaan_t *s);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_CODEC_H */
