/* -------------------------------------------------------------------------- *
 * pigura/gambar/codec_png.c - codec PNG (encode)
 * -------------------------------------------------------------------------- *
 * Implementasi pg_simpan_png - simpan gambar ke format PNG.
 *
 * PNG format:
 *   - Signature: 8 byte (89 50 4E 47 0D 0A 1A 0A)
 *   - IHDR chunk: width, height, bit depth, color type, dst.
 *   - IDAT chunk: zlib-compressed scanlines (dengan filter byte 0 = None)
 *   - IEND chunk
 *
 * Setiap chunk: [length:4][type:4][data:length][CRC32:4]
 *
 * Untuk kompresi, kita pakai DEFLATE "stored" (no compression) dengan
 * wrapper zlib (header 2 byte + Adler32 4 byte). Ini menghasilkan PNG
 * yang valid walau tidak terkompresi. Kompresi DEFLATE penuh akan
 * diimplementasi di Fase 4.
 * -------------------------------------------------------------------------- */
#include "pigura/codec.h"
#include <stddef.h> /* size_t */

#include "codec_zlib.h"
#include "pigura/galat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== Forward declarations ===== */

static pg_u32 pg_crc32_combine(pg_u32 crc, const unsigned char *data,
                                 size_t len);
static void pg_u32_be32_buf(unsigned char *buf, pg_u32 v);

/* ===== CRC32 (untuk PNG chunk checksum) ===== */

static pg_u32 pg_crc32_table[256];
static int pg_crc32_table_init = 0;

static void pg_crc32_init(void)
{
	pg_u32 i, j;
	if (pg_crc32_table_init) return;
	for (i = 0; i < 256; i++) {
		pg_u32 c = i;
		for (j = 0; j < 8; j++) {
			if (c & 1)
				c = 0xEDB88320u ^ (c >> 1);
			else
				c = c >> 1;
		}
		pg_crc32_table[i] = c;
	}
	pg_crc32_table_init = 1;
}

/* CRC32 dalam bentuk intermediate (belum di-XOR final).
 * Untuk dapat nilai final, panggil pg_crc32_final(). */
static pg_u32 pg_crc32_begin(const unsigned char *data, size_t len)
{
	pg_u32 crc = 0xFFFFFFFFu;
	size_t i;
	pg_crc32_init();
	for (i = 0; i < len; i++) {
		crc = pg_crc32_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
	}
	return crc; /* intermediate, belum final */
}

static pg_u32 pg_crc32_update(pg_u32 crc, const unsigned char *data,
                                size_t len)
{
	size_t i;
	pg_crc32_init();
	for (i = 0; i < len; i++) {
		crc = pg_crc32_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
	}
	return crc;
}

/* Final: XOR dengan 0xFFFFFFFF untuk dapat nilai CRC final. */
static pg_u32 pg_crc32_final(pg_u32 crc)
{
	return crc ^ 0xFFFFFFFFu;
}

/* Kompatibilitas: pg_crc32 = begin + final. */
static pg_u32 pg_crc32(const unsigned char *data, size_t len)
{
	return pg_crc32_final(pg_crc32_begin(data, len));
}

static pg_u32 pg_crc32_combine(pg_u32 crc, const unsigned char *data,
                                 size_t len)
{
	return pg_crc32_update(crc, data, len);
}

/* ===== Adler32 (untuk zlib checksum) ===== */

static pg_u32 pg_adler32(const unsigned char *data, size_t len)
{
	pg_u32 a = 1, b = 0;
	size_t i;
	for (i = 0; i < len; i++) {
		a = (a + data[i]) % 65521;
		b = (b + a) % 65521;
	}
	return (b << 16) | a;
}

/* ===== Zlib DEFLATE "stored" (no compression) ===== */

static unsigned char *pg_zlib_stored_encode(const unsigned char *data,
                                              size_t len,
                                              size_t *out_len)
{
	size_t n_blocks = (len + 65534) / 65535;
	size_t max_out = 2 + 5 * n_blocks + len + 4;
	unsigned char *out = (unsigned char *)malloc(max_out);
	size_t pos = 0;
	size_t offset = 0;
	pg_u32 a32;

	if (!out) return NULL;

	/* CMF: 0x78 (CM=8=deflate, CINFO=7=32K window) */
	out[pos++] = 0x78;
	/* FLG: 0x01 (FCHECK sehingga CMF*256+FLG habis dibagi 31) */
	out[pos++] = 0x01;

	/* Tulis DEFLATE stored blocks. */
	while (offset < len) {
		size_t chunk = len - offset;
		unsigned int block_len, block_nlen;
		if (chunk > 65535) chunk = 65535;
		block_len = (unsigned int)chunk;
		block_nlen = (unsigned int)(~chunk & 0xFFFF);

		/* BFINAL: 1 bila block terakhir, BTYPE: 00 (stored). */
		out[pos++] = (offset + chunk >= len) ? 0x01 : 0x00;
		out[pos++] = (unsigned char)(block_len & 0xff);
		out[pos++] = (unsigned char)((block_len >> 8) & 0xff);
		out[pos++] = (unsigned char)(block_nlen & 0xff);
		out[pos++] = (unsigned char)((block_nlen >> 8) & 0xff);

		memcpy(out + pos, data + offset, chunk);
		pos += chunk;
		offset += chunk;
	}

	/* Adler32 checksum (big-endian). */
	a32 = pg_adler32(data, len);
	out[pos++] = (unsigned char)((a32 >> 24) & 0xff);
	out[pos++] = (unsigned char)((a32 >> 16) & 0xff);
	out[pos++] = (unsigned char)((a32 >> 8) & 0xff);
	out[pos++] = (unsigned char)((a32) & 0xff);

	*out_len = pos;
	return out;
}

/* ===== Helpers ===== */

static void pg_u32_be32_buf(unsigned char *buf, pg_u32 v)
{
	buf[0] = (unsigned char)((v >> 24) & 0xff);
	buf[1] = (unsigned char)((v >> 16) & 0xff);
	buf[2] = (unsigned char)((v >> 8) & 0xff);
	buf[3] = (unsigned char)(v & 0xff);
}

static void pg_tulis_be32(FILE *f, pg_u32 v)
{
	unsigned char b[4];
	pg_u32_be32_buf(b, v);
	fwrite(b, 1, 4, f);
}

static void pg_tulis_chunk(FILE *f, const char *type,
                            const unsigned char *data, size_t len)
{
	unsigned char type_buf[4];
	pg_u32 crc;

	pg_tulis_be32(f, (pg_u32)len);
	memcpy(type_buf, type, 4);
	fwrite(type_buf, 1, 4, f);
	if (len > 0 && data) {
		fwrite(data, 1, len, f);
		crc = pg_crc32_begin(type_buf, 4);
		crc = pg_crc32_update(crc, data, len);
		crc = pg_crc32_final(crc);
	} else {
		crc = pg_crc32(type_buf, 4);
	}
	pg_tulis_be32(f, crc);
}

/* ===== Encode PNG ===== */

int pg_simpan_png(const char *nama_berkas, int lebar, int tinggi,
                  int channel, const pg_byte *pixel)
{
	FILE *f;
	static const unsigned char tanda[8] = {
		0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A
	};
	unsigned char ihdr[13];
	unsigned char color_type;
	unsigned char *raw_scanlines;
	unsigned char *zlib_data;
	size_t zlib_len;
	int y;

	if (!nama_berkas || !pixel || lebar <= 0 || tinggi <= 0) {
		pg_set_galat(PG_GALAT_ARGUMEN, "pg_simpan_png: argumen buruk");
		return 0;
	}
	if (channel != 3 && channel != 4) {
		pg_set_galat(PG_GALAT_ARGUMEN,
		             "pg_simpan_png: channel harus 3 atau 4, dapat %d",
		             channel);
		return 0;
	}

	/* Color type: 2 = RGB, 6 = RGBA. */
	color_type = (channel == 4) ? 6 : 2;

	/* IHDR: width(4) height(4) bit_depth(1) color_type(1)
	 *       compression(1) filter(1) interlace(1) = 13 byte. */
	memset(ihdr, 0, sizeof(ihdr));
	pg_u32_be32_buf(ihdr + 0, (pg_u32)lebar);
	pg_u32_be32_buf(ihdr + 4, (pg_u32)tinggi);
	ihdr[8] = 8;  /* bit depth */
	ihdr[9] = color_type;
	ihdr[10] = 0; /* compression = deflate */
	ihdr[11] = 0; /* filter = adaptive */
	ihdr[12] = 0; /* interlace = none */

	/* Raw scanlines: tiap baris diawali filter byte (0 = None). */
	{
		size_t raw_len = (size_t)(lebar * channel + 1) * tinggi;
		size_t pos = 0;
		raw_scanlines = (unsigned char *)malloc(raw_len);
		if (!raw_scanlines) {
			pg_set_galat(PG_GALAT_UMUM, "pg_simpan_png: OOM");
			return 0;
		}
		for (y = 0; y < tinggi; y++) {
			raw_scanlines[pos++] = 0; /* filter = None */
			memcpy(raw_scanlines + pos,
			       pixel + (size_t)y * lebar * channel,
			       (size_t)lebar * channel);
			pos += lebar * channel;
		}
	}

	/* Kompresi (stored/no-compression untuk sekarang). */
	zlib_data = pg_zlib_deflate_encode(raw_scanlines,
	                                   (size_t)(lebar * channel + 1) * tinggi,
	                                   &zlib_len);
	free(raw_scanlines);
	if (!zlib_data) {
		pg_set_galat(PG_GALAT_UMUM, "pg_simpan_png: zlib encode gagal");
		return 0;
	}

	/* Tulis file. */
	f = fopen(nama_berkas, "wb");
	if (!f) {
		free(zlib_data);
		pg_set_galat(PG_GALAT_IO, "pg_simpan_png: tidak bisa buka %s",
		             nama_berkas);
		return 0;
	}

	/* Signature. */
	fwrite(tanda, 1, 8, f);

	/* IHDR chunk. */
	pg_tulis_chunk(f, "IHDR", ihdr, 13);

	/* IDAT chunk. */
	pg_tulis_chunk(f, "IDAT", zlib_data, zlib_len);

	/* IEND chunk. */
	pg_tulis_chunk(f, "IEND", NULL, 0);

	free(zlib_data);
	fclose(f);
	return 1;
}
