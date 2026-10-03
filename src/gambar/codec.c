/* -------------------------------------------------------------------------- *
 * pigura/gambar/codec.c - dispatcher codec gambar (API publik)
 * -------------------------------------------------------------------------- *
 * Implementasi API publik codec.h. Menggunakan codec_internal_impl.h
 * (versi rename stb_image v2.30 ke pg_*) sebagai backend decoder.
 *
 * Struktur:
 *   codec_internal_decl.h - deklarasi types + prototypes (shared)
 *   codec_internal_impl.h - implementasi decoder (di-guard IMPL)
 *   codec.c               - dispatcher API publik + #define IMPL
 *   codec_png.c           - encoder PNG
 *   codec_jpeg.c          - encoder JPEG (baseline penuh)
 *   codec_bmp.c           - encoder BMP
 *   codec_ppm.c           - encoder + decoder PPM
 *   codec_zlib.c          - DEFLATE encoder untuk PNG
 *
 * Decoder masih monolitik di codec_internal_impl.h (stb_image intertwined).
 * Ekstraksi per-format decoder akan dilakukan di iterasi mendatang.
 * -------------------------------------------------------------------------- */
/* Disable format yang tidak relevan untuk UI toolkit.
 * Format profesional (PSD, HDR, TGA, PIC) adalah domain image editor
 * aplikasi, bukan toolkit GUI. Mereka akan mengimplementasi codec sendiri.
 * Format yang dipertahankan: PNG, JPEG, BMP, GIF, PNM, PPM (debug). */
#define PG_NO_PSD
#define PG_NO_HDR
#define PG_NO_TGA
#define PG_NO_PIC

#define PG_GAMBAR_IMPLEMENTASI
#include "codec_internal_impl.h"

#include "pigura/codec.h"
#include "pigura/permukaan.h"
#include "pigura/galat.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== Muat gambar (decode) ===== */

pg_byte *pg_muat_gambar(const char *nama_berkas,
                        int *lebar, int *tinggi,
                        int *channel_aktual, int req_channel)
{
	return pg_load(nama_berkas, lebar, tinggi,
	                 channel_aktual, req_channel);
}

pg_byte *pg_muat_gambar_dari_memori(const pg_byte *memori, int ukuran,
                                     int *lebar, int *tinggi,
                                     int *channel_aktual, int req_channel)
{
	return pg_load_from_memory(memori, ukuran, lebar, tinggi,
	                             channel_aktual, req_channel);
}

pg_word16 *pg_muat_gambar_16(const char *nama_berkas,
                              int *lebar, int *tinggi,
                              int *channel_aktual, int req_channel)
{
	return pg_load_16(nama_berkas, lebar, tinggi,
	                   channel_aktual, req_channel);
}

float *pg_muat_gambar_hdr(const char *nama_berkas,
                           int *lebar, int *tinggi,
                           int *channel_aktual, int req_channel)
{
	return pg_loadf(nama_berkas, lebar, tinggi,
	                 channel_aktual, req_channel);
}

pg_byte *pg_muat_gambar_gif(const pg_byte *memori, int ukuran,
                             int **delays,
                             int *lebar, int *tinggi, int *jumlah_frame,
                             int *channel, int req_channel)
{
	return pg_load_gif_from_memory(memori, ukuran, delays,
	                                 lebar, tinggi, jumlah_frame,
	                                 channel, req_channel);
}

/* ===== Info gambar ===== */

int pg_info_gambar(const char *nama_berkas, int *lebar, int *tinggi,
                    int *channel)
{
	return pg_info(nama_berkas, lebar, tinggi, channel);
}

int pg_info_gambar_dari_memori(const pg_byte *memori, int ukuran,
                                int *lebar, int *tinggi, int *channel)
{
	return pg_info_from_memory(memori, ukuran, lebar, tinggi, channel);
}

int pg_gambar_apa_hdr(const char *nama_berkas)
{
	return pg_is_hdr(nama_berkas);
}

int pg_gambar_apa_16_bit_dari_memori(const pg_byte *memori, int ukuran)
{
	return pg_is_16_bit_from_memory(memori, ukuran);
}

void pg_gambar_bebas(void *pixel)
{
	pg_image_free(pixel);
}

const char *pg_gambar_galat(void)
{
	return pg_failure_reason();
}

/* ===== Konfigurasi ===== */

void pg_setel_premultiply_saat_muat(int aktif)
{
	pg_set_unpremultiply_on_load(aktif ? 1 : 0);
}

void pg_setel_balik_y_saat_muat(int aktif)
{
	pg_set_flip_vertically_on_load(aktif ? 1 : 0);
}

/* ===== Helper pigura permukaan ===== */

pg_permukaan_t *pg_muat_permukaan(const char *nama_berkas)
{
	int lebar, tinggi, channel;
	pg_byte *px;
	pg_permukaan_t *s;

	if (!nama_berkas) return NULL;
	px = pg_muat_gambar(nama_berkas, &lebar, &tinggi,
	                    &channel, PG_GAMBAR_RGBA);
	if (!px) {
		pg_set_galat(PG_GALAT_IO, "pg_muat_permukaan: gagal muat %s: %s",
		             nama_berkas, pg_gambar_galat());
		return NULL;
	}
	s = pg_bungkus_permukaan(px, lebar, tinggi, lebar * 4);
	if (!s) {
		pg_gambar_bebas(px);
		return NULL;
	}
	return s;
}

int pg_simpan_permukaan(const char *nama_berkas, const pg_permukaan_t *s)
{
	const char *titik;
	int lebar, tinggi, channel;
	const pg_byte *px;

	if (!nama_berkas || !s) return 0;
	titik = strrchr(nama_berkas, '.');
	if (!titik) {
		pg_set_galat(PG_GALAT_ARGUMEN,
		             "pg_simpan_permukaan: tanpa ekstensi: %s", nama_berkas);
		return 0;
	}
	lebar = pg_permukaan_lebar((pg_permukaan_t *)s);
	tinggi = pg_permukaan_tinggi((pg_permukaan_t *)s);
	px = (const pg_byte *)pg_permukaan_piksel(s);
	channel = 4;

	if (strcmp(titik, ".png") == 0 || strcmp(titik, ".PNG") == 0) {
		return pg_simpan_png(nama_berkas, lebar, tinggi, channel, px);
	} else if (strcmp(titik, ".jpg") == 0 || strcmp(titik, ".jpeg") == 0 ||
	           strcmp(titik, ".JPG") == 0 || strcmp(titik, ".JPEG") == 0) {
		return pg_simpan_jpeg(nama_berkas, lebar, tinggi, channel, px, 90);
	} else if (strcmp(titik, ".bmp") == 0 || strcmp(titik, ".BMP") == 0) {
		return pg_simpan_bmp(nama_berkas, lebar, tinggi, channel, px);
	} else if (strcmp(titik, ".ppm") == 0 || strcmp(titik, ".PPM") == 0) {
		return pg_simpan_ppm(nama_berkas, lebar, tinggi, channel, px);
	}
	pg_set_galat(PG_GALAT_ARGUMEN,
	             "pg_simpan_permukaan: ekstensi tidak dikenal: %s", titik);
	return 0;
}

/* ===== Simpan (encode) - diimplementasi di file per-format =====
 *
 * pg_simpan_png  -> codec_png.c
 * pg_simpan_bmp  -> codec_bmp.c
 * pg_simpan_ppm  -> codec_ppm.c
 * pg_simpan_jpeg -> stub (Fase 4)
 */

/* pg_simpan_jpeg diimplementasi di codec_jpeg.c */
