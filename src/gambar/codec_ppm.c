/* -------------------------------------------------------------------------- *
 * pigura/gambar/codec_ppm.c - codec PPM (encode + decode)
 * -------------------------------------------------------------------------- *
 * Format PPM (Portable PixMap) - sangat simpel, tidak butuh library.
 *
 * Format P6 (binary):
 *   Header: "P6\n{lebar} {tinggi}\n255\n"
 *   Data: RGB raw, 3 byte per piksel
 *
 * Cocok untuk debug/test karena tidak ada kompresi. Decode juga simpel.
 * -------------------------------------------------------------------------- */
#include "pigura/codec.h"
#include "pigura/galat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Encode PPM P6 (binary RGB). */
int pg_simpan_ppm(const char *nama_berkas, int lebar, int tinggi,
                  int channel, const pg_byte *pixel)
{
	FILE *f;
	int y;
	char header[64];
	int len;

	if (!nama_berkas || !pixel || lebar <= 0 || tinggi <= 0) {
		pg_set_galat(PG_GALAT_ARGUMEN, "pg_simpan_ppm: argumen buruk");
		return 0;
	}
	if (channel < 3) {
		pg_set_galat(PG_GALAT_ARGUMEN,
		             "pg_simpan_ppm: channel harus >= 3, dapat %d",
		             channel);
		return 0;
	}

	f = fopen(nama_berkas, "wb");
	if (!f) {
		pg_set_galat(PG_GALAT_IO, "pg_simpan_ppm: tidak bisa buka %s",
		             nama_berkas);
		return 0;
	}

	/* Header: "P6\n{lebar} {tinggi}\n255\n" */
	len = snprintf(header, sizeof(header), "P6\n%d %d\n255\n",
	               lebar, tinggi);
	if (fwrite(header, 1, len, f) != (size_t)len) goto gagal;

	/* Pixel data: RGB raw, top-down (baris 0 dulu).
	 * Bila channel == 4 (RGBA), ambil RGB saja. */
	for (y = 0; y < tinggi; y++) {
		int x;
		for (x = 0; x < lebar; x++) {
			const pg_byte *src = pixel + (y * lebar + x) * channel;
			unsigned char rgb[3];
			rgb[0] = src[0]; /* R */
			rgb[1] = src[1]; /* G */
			rgb[2] = src[2]; /* B */
			if (fwrite(rgb, 1, 3, f) != 3) goto gagal;
		}
	}

	fclose(f);
	return 1;

gagal:
	fclose(f);
	pg_set_galat(PG_GALAT_IO, "pg_simpan_ppm: gagal tulis %s", nama_berkas);
	return 0;
}

/* Decode PPM P6 (binary). */
pg_byte *pg_muat_ppm_dari_memori(const pg_byte *memori, int ukuran,
                                   int *lebar, int *tinggi,
                                   int *channel_aktual)
{
	const pg_byte *p;
	const pg_byte *akhir;
	pg_byte *hasil;
	int w, h, maxval;
	int i;

	if (!memori || ukuran <= 0 || !lebar || !tinggi) return NULL;

	p = memori;
	akhir = memori + ukuran;

	/* Cek magic "P6". */
	if (akhir - p < 2 || p[0] != 'P' || p[1] != '6') {
		pg_set_galat(PG_GALAT_UMUM, "pg_muat_ppm: bukan PPM P6");
		return NULL;
	}
	p += 2;

	/* Skip whitespace + comments (#...). */
#define PG_PPM_SKIP_WS() \
	do { \
		while (p < akhir && (*p == ' ' || *p == '\t' || *p == '\n' || \
		                      *p == '\r')) p++; \
		while (p < akhir && *p == '#') { \
			while (p < akhir && *p != '\n') p++; \
			while (p < akhir && (*p == ' ' || *p == '\t' || *p == '\n' || \
			                      *p == '\r')) p++; \
		} \
	} while (0)

	PG_PPM_SKIP_WS();

	/* Baca lebar. */
	w = 0;
	while (p < akhir && *p >= '0' && *p <= '9') {
		w = w * 10 + (*p - '0');
		p++;
	}
	PG_PPM_SKIP_WS();

	/* Baca tinggi. */
	h = 0;
	while (p < akhir && *p >= '0' && *p <= '9') {
		h = h * 10 + (*p - '0');
		p++;
	}
	PG_PPM_SKIP_WS();

	/* Baca maxval. */
	maxval = 0;
	while (p < akhir && *p >= '0' && *p <= '9') {
		maxval = maxval * 10 + (*p - '0');
		p++;
	}

	/* Skip 1 whitespace setelah maxval. */
	if (p < akhir) p++;

	if (w <= 0 || h <= 0 || maxval != 255) {
		pg_set_galat(PG_GALAT_UMUM,
		             "pg_muat_ppm: dimensi/maxval tidak valid");
		return NULL;
	}

	/* Alokasi hasil (RGB, 3 channel). */
	hasil = (pg_byte *)malloc((size_t)w * h * 3);
	if (!hasil) return NULL;

	/* Copy pixel data. */
	for (i = 0; i < w * h * 3; i++) {
		if (p >= akhir) {
			free(hasil);
			pg_set_galat(PG_GALAT_UMUM,
			             "pg_muat_ppm: data piksel tidak lengkap");
			return NULL;
		}
		hasil[i] = *p++;
	}

	*lebar = w;
	*tinggi = h;
	if (channel_aktual) *channel_aktual = 3;
	return hasil;
#undef PG_PPM_SKIP_WS
}
