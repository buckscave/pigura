/* -------------------------------------------------------------------------- *
 * pigura/gambar/codec_jpeg.c - codec JPEG (encode baseline penuh)
 * -------------------------------------------------------------------------- *
 * Implementasi pg_simpan_jpeg - simpan gambar ke format JPEG baseline.
 *
 * Proses:
 *   1. RGB -> YCbCr conversion
 *   2. Bagi jadi 8x8 blocks, level shift (-128)
 *   3. DCT (Discrete Cosine Transform)
 *   4. Quantization (quality-scaled)
 *   5. Zigzag scan
 *   6. Huffman encoding: DC differential + AC RLE
 *
 * 4:4:4 subsampling (no subsampling) untuk simplicity.
 * Quality 1-100, default 90.
 *
 * Standard JPEG Huffman tables (DC lum/chr, AC lum/chr) dari JPEG spec.
 * -------------------------------------------------------------------------- */
#include "pigura/codec.h"
#include "pigura/galat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ===== JPEG markers ===== */
#define JPEG_SOI  0xFFD8
#define JPEG_EOI  0xFFD9
#define JPEG_APP0 0xFFE0
#define JPEG_DQT  0xFFDB
#define JPEG_SOF0 0xFFC0
#define JPEG_DHT  0xFFC4
#define JPEG_SOS  0xFFDA

/* ===== Standard quantization tables ===== */
static const int pg_q_lum[64] = {
	16,11,10,16,24,40,51,61, 12,12,14,19,26,58,60,55,
	14,13,16,24,40,57,69,56, 14,17,22,29,51,87,80,62,
	18,22,37,56,68,109,103,77, 24,35,55,64,81,104,113,92,
	49,64,78,87,103,121,120,101, 72,92,95,98,112,100,103,99
};
static const int pg_q_chr[64] = {
	17,18,24,47,99,99,99,99, 18,21,26,66,99,99,99,99,
	24,26,56,99,99,99,99,99, 47,66,99,99,99,99,99,99,
	99,99,99,99,99,99,99,99, 99,99,99,99,99,99,99,99,
	99,99,99,99,99,99,99,99, 99,99,99,99,99,99,99,99
};
static const int pg_zigzag[64] = {
	 0, 1, 8,16, 9, 2, 3,10,
	17,24,32,25,18,11, 4, 5,
	12,19,26,33,40,48,41,34,
	27,20,13, 6, 7,14,21,28,
	35,42,49,56,57,50,43,36,
	29,22,15,23,30,37,44,51,
	58,59,52,45,38,31,39,46,
	53,60,61,54,47,55,62,63
};

/* ===== Standard Huffman tables (from JPEG spec) ===== */
static const unsigned char pg_huff_dc_lum_bits[16] = {0,1,5,1,1,1,1,1,1,0,0,0,0,0,0,0};
static const unsigned char pg_huff_dc_lum_val[12] = {0,1,2,3,4,5,6,7,8,9,10,11};
static const unsigned char pg_huff_ac_lum_bits[16] = {0,2,1,3,3,2,4,3,5,5,4,4,0,0,1,0x7d};
static const unsigned char pg_huff_ac_lum_val[162] = {
	0x01,0x02,0x03,0x00,0x04,0x11,0x05,0x12,0x21,0x31,0x41,0x06,0x13,0x51,0x61,0x07,
	0x22,0x71,0x14,0x32,0x81,0x91,0xa1,0x08,0x23,0x42,0xb1,0xc1,0x15,0x52,0xd1,0xf0,
	0x24,0x33,0x62,0x72,0x82,0x09,0x0a,0x16,0x17,0x18,0x19,0x1a,0x25,0x26,0x27,0x28,
	0x29,0x2a,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x43,0x44,0x45,0x46,0x47,0x48,0x49,
	0x4a,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x63,0x64,0x65,0x66,0x67,0x68,0x69,
	0x6a,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x83,0x84,0x85,0x86,0x87,0x88,0x89,
	0x8a,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,
	0xa8,0xa9,0xaa,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xc2,0xc3,0xc4,0xc5,
	0xc6,0xc7,0xc8,0xc9,0xca,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xe1,0xe2,
	0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,
	0xf9,0xfa
};
static const unsigned char pg_huff_dc_chr_bits[16] = {0,3,1,1,1,1,1,1,1,1,1,0,0,0,0,0};
static const unsigned char pg_huff_dc_chr_val[12] = {0,1,2,3,4,5,6,7,8,9,10,11};
static const unsigned char pg_huff_ac_chr_bits[16] = {0,2,1,2,4,4,3,4,7,5,4,4,0,1,2,0x77};
static const unsigned char pg_huff_ac_chr_val[162] = {
	0x00,0x01,0x02,0x03,0x11,0x04,0x05,0x21,0x31,0x06,0x12,0x41,0x51,0x07,0x61,0x71,
	0x13,0x22,0x32,0x81,0x08,0x14,0x42,0x91,0xa1,0xb1,0xc1,0x09,0x23,0x33,0x52,0xf0,
	0x15,0x62,0x72,0xd1,0x0a,0x16,0x24,0x34,0xe1,0x25,0xf1,0x17,0x18,0x19,0x1a,0x26,
	0x27,0x28,0x29,0x2a,0x35,0x36,0x37,0x38,0x39,0x3a,0x43,0x44,0x45,0x46,0x47,0x48,
	0x49,0x4a,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x63,0x64,0x65,0x66,0x67,0x68,
	0x69,0x6a,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x82,0x83,0x84,0x85,0x86,0x87,
	0x88,0x89,0x8a,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0xa2,0xa3,0xa4,0xa5,
	0xa6,0xa7,0xa8,0xa9,0xaa,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xc2,0xc3,
	0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,
	0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,
	0xf9,0xfa
};

/* ===== Huffman code table (precomputed) ===== */
typedef struct {
	unsigned short code; /* 16-bit max */
	unsigned char  len;   /* 1-16 bit */
} pg_huff_code;

typedef struct {
	pg_huff_code dc[257]; /* DC: index by size (0-16) */
	pg_huff_code ac[257]; /* AC: index by (run<<4)|size */
} pg_huff_table;

/* Generate canonical Huffman codes dari BITS + VALS (JPEG spec section C.2). */
static void pg_huff_build(const unsigned char *bits,
                            const unsigned char *vals,
                            pg_huff_code *out)
{
	int code = 0;
	int k = 0;
	int i, j;
	/* Inisialisasi semua sebagai tidak terpakai. */
	for (i = 0; i < 257; i++) {
		out[i].code = 0;
		out[i].len = 0;
	}
	for (i = 1; i <= 16; i++) {
		for (j = 0; j < bits[i-1]; j++) {
			int sym = vals[k++];
			out[sym].code = (unsigned short)code;
			out[sym].len = (unsigned char)i;
			code++;
		}
		code <<= 1;
	}
}

/* ===== Bit writer ===== */
typedef struct {
	FILE *f;
	unsigned buf;
	int nbits;
} pg_bit_writer;

static void pg_bw_init(pg_bit_writer *bw, FILE *f)
{
	bw->f = f;
	bw->buf = 0;
	bw->nbits = 0;
}

static void pg_bw_put_bits(pg_bit_writer *bw, unsigned code, int len)
{
	int i;
	for (i = len - 1; i >= 0; i--) {
		int bit = (code >> i) & 1;
		bw->buf = (bw->buf << 1) | bit;
		bw->nbits++;
		if (bw->nbits == 8) {
			fputc(bw->buf & 0xFF, bw->f);
			if ((bw->buf & 0xFF) == 0xFF)
				fputc(0x00, bw->f);
			bw->buf = 0;
			bw->nbits = 0;
		}
	}
}

static void pg_bw_flush(pg_bit_writer *bw)
{
	if (bw->nbits > 0) {
		bw->buf <<= (8 - bw->nbits);
		bw->buf |= (1 << (8 - bw->nbits)) - 1;
		fputc(bw->buf & 0xFF, bw->f);
		if ((bw->buf & 0xFF) == 0xFF)
			fputc(0x00, bw->f);
		bw->buf = 0;
		bw->nbits = 0;
	}
}

/* ===== DCT 8x8 (separable) ===== */
static const float PG_PI = 3.14159265358979323846f;
static const float PG_INV_SQRT2 = 0.70710678118654752440f;

static void pg_dct8x8(float *block)
{
	float tmp[64];
	int i, j, k;
	/* Row DCT */
	for (i = 0; i < 8; i++) {
		float *r = &block[i * 8];
		for (j = 0; j < 8; j++) {
			float sum = 0.0f;
			for (k = 0; k < 8; k++) {
				float c = cosf((2.0f * k + 1.0f) * j * PG_PI / 16.0f);
				sum += r[k] * c;
			}
			sum *= (j == 0) ? PG_INV_SQRT2 / 2.0f : 0.5f;
			tmp[i * 8 + j] = sum;
		}
	}
	/* Column DCT */
	for (j = 0; j < 8; j++) {
		float col[8], out[8];
		for (i = 0; i < 8; i++) col[i] = tmp[i * 8 + j];
		for (i = 0; i < 8; i++) {
			float sum = 0.0f;
			for (k = 0; k < 8; k++) {
				float c = cosf((2.0f * k + 1.0f) * i * PG_PI / 16.0f);
				sum += col[k] * c;
			}
			sum *= (i == 0) ? PG_INV_SQRT2 / 2.0f : 0.5f;
			out[i] = sum;
		}
		for (i = 0; i < 8; i++) block[i * 8 + j] = out[i];
	}
}

/* ===== Scale quantization table ===== */
static void pg_scale_qtable(const int *base, int kualitas, int *out)
{
	float scale;
	int i;
	if (kualitas < 1) kualitas = 1;
	if (kualitas > 100) kualitas = 100;
	if (kualitas < 50)
		scale = 50.0f / (float)kualitas;
	else
		scale = (100.0f - kualitas) / 50.0f;
	for (i = 0; i < 64; i++) {
		int q = (int)(base[i] * scale + 0.5f);
		if (q < 1) q = 1;
		if (q > 255) q = 255;
		out[i] = q;
	}
}

/* ===== RGB -> YCbCr ===== */
static void pg_rgb_to_ycbcr(int r, int g, int b, float *y, float *cb, float *cr)
{
	*y  =  0.299f * r + 0.587f * g + 0.114f * b - 128.0f;
	*cb = -0.16874f * r - 0.33126f * g + 0.5f * b;
	*cr =  0.5f * r - 0.41869f * g - 0.08131f * b;
}

/* ===== JPEG segment writers ===== */
static void pg_jpeg_marker(FILE *f, int m) { fputc(0xFF,f); fputc(m&0xFF,f); }
static void pg_jpeg_be16(FILE *f, int v) { fputc((v>>8)&0xFF,f); fputc(v&0xFF,f); }

static void pg_jpeg_write_dqt(FILE *f, int id, int *qt)
{
	int i;
	pg_jpeg_marker(f, JPEG_DQT);
	pg_jpeg_be16(f, 2 + 1 + 64);
	fputc(id, f);
	for (i = 0; i < 64; i++) fputc(qt[i], f);
}

static void pg_jpeg_write_sof0(FILE *f, int w, int h)
{
	pg_jpeg_marker(f, JPEG_SOF0);
	pg_jpeg_be16(f, 17);
	fputc(8, f);
	pg_jpeg_be16(f, h);
	pg_jpeg_be16(f, w);
	fputc(3, f);
	fputc(1,f); fputc(0x11,f); fputc(0,f);
	fputc(2,f); fputc(0x11,f); fputc(1,f);
	fputc(3,f); fputc(0x11,f); fputc(1,f);
}

static void pg_jpeg_write_dht(FILE *f, int cls, int id,
                               const unsigned char *bits,
                               const unsigned char *vals)
{
	int i, n = 0;
	for (i = 0; i < 16; i++) n += bits[i];
	pg_jpeg_marker(f, JPEG_DHT);
	pg_jpeg_be16(f, 2 + 1 + 16 + n);
	fputc((cls << 4) | id, f);
	for (i = 0; i < 16; i++) fputc(bits[i], f);
	for (i = 0; i < n; i++) fputc(vals[i], f);
}

static void pg_jpeg_write_sos(FILE *f)
{
	pg_jpeg_marker(f, JPEG_SOS);
	pg_jpeg_be16(f, 12);
	fputc(3, f);
	fputc(1,f); fputc(0x00,f);
	fputc(2,f); fputc(0x11,f);
	fputc(3,f); fputc(0x11,f);
	fputc(0,f); fputc(63,f); fputc(0,f);
}

/* ===== Encode 1 block (8x8) ===== */
static void pg_encode_block(pg_bit_writer *bw, float *block, int *qt,
                             int *prev_dc, pg_huff_code *dc_tab,
                             pg_huff_code *ac_tab)
{
	int zz[64];
	int i;
	int dc_val;
	int run = 0;

	/* Quantize + zigzag. */
	for (i = 0; i < 64; i++) {
		float v = block[i] / (float)qt[i];
		int q = (int)(v + (v >= 0 ? 0.5f : -0.5f));
		zz[pg_zigzag[i]] = q;
	}

	/* DC differential. */
	dc_val = zz[0] - *prev_dc;
	*prev_dc = zz[0];
	{
		int abs_v = dc_val < 0 ? -dc_val : dc_val;
		int size = 0;
		int k = abs_v;
		while (k) { size++; k >>= 1; }
		/* Emit DC code. */
		pg_bw_put_bits(bw, dc_tab[size].code, dc_tab[size].len);
		/* Emit DC value (if size > 0). */
		if (size > 0) {
			int v = dc_val;
			if (v < 0) v += (1 << size) - 1;
			pg_bw_put_bits(bw, v, size);
		}
	}

	/* AC coefficients (RLE). */
	for (i = 1; i < 64; i++) {
		if (zz[i] == 0) {
			run++;
		} else {
			while (run > 15) {
				/* ZRL: 16 zeros. */
				pg_bw_put_bits(bw, ac_tab[0xF0].code, ac_tab[0xF0].len);
				run -= 16;
			}
			{
				int v = zz[i];
				int abs_v = v < 0 ? -v : v;
				int size = 0;
				int k = abs_v;
				while (k) { size++; k >>= 1; }
				int sym = (run << 4) | size;
				pg_bw_put_bits(bw, ac_tab[sym].code, ac_tab[sym].len);
				if (v < 0) v += (1 << size) - 1;
				pg_bw_put_bits(bw, v, size);
				run = 0;
			}
		}
	}
	if (run > 0) {
		/* EOB. */
		pg_bw_put_bits(bw, ac_tab[0x00].code, ac_tab[0x00].len);
	}
}

/* ===== Encode JPEG (main) ===== */
int pg_simpan_jpeg(const char *nama_berkas, int lebar, int tinggi,
                   int channel, const pg_byte *pixel, int kualitas)
{
	FILE *f;
	int q_lum[64], q_chr[64];
	int prev_dc_y = 0, prev_dc_cb = 0, prev_dc_cr = 0;
	pg_huff_table htab_lum, htab_chr;
	pg_bit_writer bw;
	int bx, by;
	int n_blocks_x, n_blocks_y;

	if (!nama_berkas || !pixel || lebar <= 0 || tinggi <= 0) {
		pg_set_galat(PG_GALAT_ARGUMEN, "pg_simpan_jpeg: argumen buruk");
		return 0;
	}
	if (channel != 3 && channel != 4) {
		pg_set_galat(PG_GALAT_ARGUMEN,
		             "pg_simpan_jpeg: channel harus 3 atau 4, dapat %d", channel);
		return 0;
	}
	if (kualitas < 1) kualitas = 1;
	if (kualitas > 100) kualitas = 100;

	/* Build Huffman tables. */
	pg_huff_build(pg_huff_dc_lum_bits, pg_huff_dc_lum_val, htab_lum.dc);
	pg_huff_build(pg_huff_ac_lum_bits, pg_huff_ac_lum_val, htab_lum.ac);
	pg_huff_build(pg_huff_dc_chr_bits, pg_huff_dc_chr_val, htab_chr.dc);
	pg_huff_build(pg_huff_ac_chr_bits, pg_huff_ac_chr_val, htab_chr.ac);

	/* Scale quantization. */
	pg_scale_qtable(pg_q_lum, kualitas, q_lum);
	pg_scale_qtable(pg_q_chr, kualitas, q_chr);

	f = fopen(nama_berkas, "wb");
	if (!f) {
		pg_set_galat(PG_GALAT_IO, "pg_simpan_jpeg: tidak bisa buka %s",
		             nama_berkas);
		return 0;
	}

	/* SOI */
	pg_jpeg_marker(f, JPEG_SOI);
	/* APP0 JFIF */
	pg_jpeg_marker(f, JPEG_APP0);
	pg_jpeg_be16(f, 16);
	fputs("JFIF", f); fputc(0, f);
	fputc(1,f); fputc(1,f); fputc(0,f);
	pg_jpeg_be16(f, 1); pg_jpeg_be16(f, 1);
	fputc(0,f); fputc(0,f);
	/* DQT */
	pg_jpeg_write_dqt(f, 0, q_lum);
	pg_jpeg_write_dqt(f, 1, q_chr);
	/* SOF0 */
	pg_jpeg_write_sof0(f, lebar, tinggi);
	/* DHT (4 tables) */
	pg_jpeg_write_dht(f, 0, 0, pg_huff_dc_lum_bits, pg_huff_dc_lum_val);
	pg_jpeg_write_dht(f, 1, 0, pg_huff_ac_lum_bits, pg_huff_ac_lum_val);
	pg_jpeg_write_dht(f, 0, 1, pg_huff_dc_chr_bits, pg_huff_dc_chr_val);
	pg_jpeg_write_dht(f, 1, 1, pg_huff_ac_chr_bits, pg_huff_ac_chr_val);
	/* SOS */
	pg_jpeg_write_sos(f);

	/* Encode scan: blocks 8x8, 4:4:4 (no subsampling). */
	pg_bw_init(&bw, f);
	n_blocks_x = (lebar + 7) / 8;
	n_blocks_y = (tinggi + 7) / 8;

	for (by = 0; by < n_blocks_y; by++) {
		for (bx = 0; bx < n_blocks_x; bx++) {
			float block_y[64], block_cb[64], block_cr[64];
			int py, px;
			/* Extract 8x8 block dari pixel data (pad dengan 0 bila overflow). */
			for (py = 0; py < 8; py++) {
				for (px = 0; px < 8; px++) {
					int sx = bx * 8 + px;
					int sy = by * 8 + py;
					float y, cb, cr;
					if (sx < lebar && sy < tinggi) {
						const pg_byte *p = pixel + (sy * lebar + sx) * channel;
						pg_rgb_to_ycbcr(p[0], p[1], p[2], &y, &cb, &cr);
					} else {
						y = cb = cr = 0.0f;
					}
					block_y[py * 8 + px] = y;
					block_cb[py * 8 + px] = cb;
					block_cr[py * 8 + px] = cr;
				}
			}
			/* DCT + quantize + Huffman untuk Y, Cb, Cr. */
			pg_dct8x8(block_y);
			pg_encode_block(&bw, block_y, q_lum, &prev_dc_y,
			                htab_lum.dc, htab_lum.ac);
			pg_dct8x8(block_cb);
			pg_encode_block(&bw, block_cb, q_chr, &prev_dc_cb,
			                htab_chr.dc, htab_chr.ac);
			pg_dct8x8(block_cr);
			pg_encode_block(&bw, block_cr, q_chr, &prev_dc_cr,
			                htab_chr.dc, htab_chr.ac);
		}
	}

	pg_bw_flush(&bw);
	/* EOI */
	pg_jpeg_marker(f, JPEG_EOI);
	fclose(f);
	return 1;
}
