/* -------------------------------------------------------------------------- *
 * pigura/gambar/codec_bmp.c - codec BMP (encode)
 * -------------------------------------------------------------------------- *
 * Implementasi pg_simpan_bmp - simpan gambar ke format BMP 24-bit/32-bit.
 *
 * BMP format sangat simpel:
 *   - BITMAPFILEHEADER (14 byte): "BM" + size + reserved + offset pixel
 *   - BITMAPINFOHEADER (40 byte): size + width + height + planes + bpp +
 *     compression + image_size + x_ppm + y_ppm + colors_used + important
 *   - Pixel data: BGR (bukan RGB!) bottom-up, padding ke 4-byte boundary
 *
 * Tidak butuh library eksternal. Format BMP tidak punya kompresi pada
 * mode BI_RGB (paling umum).
 * -------------------------------------------------------------------------- */
#include "pigura/codec.h"
#include "pigura/galat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Struktur header BMP. */
#pragma pack(push, 1)
typedef struct {
	char     tanda[2];        /* "BM" */
	pg_u32   ukuran_berkas;   /* total ukuran file */
	pg_u16   reserved1;       /* 0 */
	pg_u16   reserved2;       /* 0 */
	pg_u32   offset_pixel;    /* offset ke data piksel */
} pg_bmp_file_header;

typedef struct {
	pg_u32   ukuran_header;   /* 40 */
	pg_s32   lebar;           /* width in pixels */
	pg_s32   tinggi;          /* height in pixels (negatif = top-down) */
	pg_u16   planes;          /* 1 */
	pg_u16   bit_per_pixel;   /* 24 atau 32 */
	pg_u32   kompresi;        /* 0 = BI_RGB */
	pg_u32   ukuran_gambar;   /* bisa 0 untuk BI_RGB */
	pg_s32   x_ppm;           /* 2835 = 72 DPI */
	pg_s32   y_ppm;           /* 2835 */
	pg_u32   warna_dipakai;    /* 0 */
	pg_u32   warna_penting;   /* 0 */
} pg_bmp_info_header;
#pragma pack(pop)

/* Tulis little-endian 32-bit. */
static void pg_tulis_le32(unsigned char *buf, pg_u32 v)
{
	buf[0] = (unsigned char)(v & 0xff);
	buf[1] = (unsigned char)((v >> 8) & 0xff);
	buf[2] = (unsigned char)((v >> 16) & 0xff);
	buf[3] = (unsigned char)((v >> 24) & 0xff);
}

/* Tulis little-endian 16-bit. */
static void pg_tulis_le16(unsigned char *buf, pg_u16 v)
{
	buf[0] = (unsigned char)(v & 0xff);
	buf[1] = (unsigned char)((v >> 8) & 0xff);
}

int pg_simpan_bmp(const char *nama_berkas, int lebar, int tinggi,
                  int channel, const pg_byte *pixel)
{
	FILE *f;
	pg_bmp_file_header fh;
	pg_bmp_info_header ih;
	int baris_stride;
	int ukuran_pixel;
	int total_ukuran;
	int y;
	unsigned char *baris_buf;
	int bpp;

	if (!nama_berkas || !pixel || lebar <= 0 || tinggi <= 0) {
		pg_set_galat(PG_GALAT_ARGUMEN, "pg_simpan_bmp: argumen buruk");
		return 0;
	}
	if (channel != 3 && channel != 4) {
		pg_set_galat(PG_GALAT_ARGUMEN,
		             "pg_simpan_bmp: channel harus 3 atau 4, dapat %d",
		             channel);
		return 0;
	}

	bpp = (channel == 4) ? 32 : 24;
	/* BMP row harus multiple of 4 byte. */
	baris_stride = ((lebar * (bpp / 8)) + 3) & ~3;
	ukuran_pixel = baris_stride * tinggi;
	total_ukuran = 14 + 40 + ukuran_pixel;

	/* File header. */
	memset(&fh, 0, sizeof(fh));
	fh.tanda[0] = 'B';
	fh.tanda[1] = 'M';
	pg_tulis_le32((unsigned char *)&fh.ukuran_berkas, (pg_u32)total_ukuran);
	pg_tulis_le32((unsigned char *)&fh.offset_pixel, 14 + 40);

	/* Info header. */
	memset(&ih, 0, sizeof(ih));
	pg_tulis_le32((unsigned char *)&ih.ukuran_header, 40);
	pg_tulis_le32((unsigned char *)&ih.lebar, (pg_u32)lebar);
	/* tinggi positif = bottom-up (konvensi BMP) */
	pg_tulis_le32((unsigned char *)&ih.tinggi, (pg_u32)tinggi);
	pg_tulis_le16((unsigned char *)&ih.planes, 1);
	pg_tulis_le16((unsigned char *)&ih.bit_per_pixel, (pg_u16)bpp);
	pg_tulis_le32((unsigned char *)&ih.kompresi, 0); /* BI_RGB */
	pg_tulis_le32((unsigned char *)&ih.ukuran_gambar, (pg_u32)ukuran_pixel);
	pg_tulis_le32((unsigned char *)&ih.x_ppm, 2835);
	pg_tulis_le32((unsigned char *)&ih.y_ppm, 2835);

	f = fopen(nama_berkas, "wb");
	if (!f) {
		pg_set_galat(PG_GALAT_IO, "pg_simpan_bmp: tidak bisa buka %s",
		             nama_berkas);
		return 0;
	}

	if (fwrite(&fh, 1, 14, f) != 14) goto gagal;
	if (fwrite(&ih, 1, 40, f) != 40) goto gagal;

	/* Alokasi buffer baris dengan padding. */
	baris_buf = (unsigned char *)calloc(baris_stride, 1);
	if (!baris_buf) goto gagal;

	/* Tulis pixel data bottom-up (baris terakhir dulu). */
	for (y = tinggi - 1; y >= 0; y--) {
		int x;
		memset(baris_buf, 0, baris_stride);
		for (x = 0; x < lebar; x++) {
			const pg_byte *src = pixel + (y * lebar + x) * channel;
			unsigned char *dst = baris_buf + x * (bpp / 8);
			/* BMP pakai BGR, bukan RGB. */
			dst[0] = src[2]; /* B */
			dst[1] = src[1]; /* G */
			dst[2] = src[0]; /* R */
			if (bpp == 32) dst[3] = src[3]; /* A */
		}
		if (fwrite(baris_buf, 1, baris_stride, f) != (size_t)baris_stride) {
			free(baris_buf);
			goto gagal;
		}
	}

	free(baris_buf);
	fclose(f);
	return 1;

gagal:
	fclose(f);
	pg_set_galat(PG_GALAT_IO, "pg_simpan_bmp: gagal tulis %s", nama_berkas);
	return 0;
}
