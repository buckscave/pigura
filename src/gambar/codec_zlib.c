/* -------------------------------------------------------------------------- *
 * pigura/gambar/codec_zlib.c - DEFLATE/zlib encoder
 * -------------------------------------------------------------------------- *
 * Implementasi DEFLATE compression (RFC 1951) dengan wrapper zlib
 * (RFC 1950) untuk PNG IDAT chunk.
 *
 * Strategi: pakai static Huffman codes (BTYPE=01) untuk simplicity.
 * Static Huffman tidak butuh DHT trees di stream, lebih cepat encode
 * walau rasio kompresi sedikit lebih buruk dari dynamic Huffman.
 *
 * LZ77 sliding window: 32768 byte, min match 3 byte, max match 258 byte.
 *
 * Output: zlib stream [CMF][FLG][DEFLATE blocks][Adler32]
 * -------------------------------------------------------------------------- */
#include "pigura/codec.h"
#include "codec_zlib.h"
#include "pigura/galat.h"
#include <stdlib.h>
#include <string.h>

/* ===== Adler32 ===== */
static pg_u32 pg_zlib_adler32(const unsigned char *data, size_t len)
{
	pg_u32 a = 1, b = 0;
	size_t i;
	for (i = 0; i < len; i++) {
		a = (a + data[i]) % 65521;
		b = (b + a) % 65521;
	}
	return (b << 16) | a;
}

/* ===== Static Huffman codes (RFC 1951 section 3.2.6) =====
 *
 * Literal/length codes (0-285):
 *   0-143:   8 bits, codes 00110000-10111111 (0x30-0xBF)
 *   144-255: 9 bits, codes 110010000-111111111 (0x190-0x1FF)
 *   256-279: 7 bits, codes 0000000-0010111 (0x00-0x17)
 *   280-287: 8 bits, codes 11000000-11000111 (0xC0-0xC7)
 *
 * Distance codes (0-29): 5 bits each.
 */

/* Length codes (RFC 1951 section 3.2.5).
 * Length code 257 = match length 3, 258 = 4, ..., 285 = 258. */
static const int pg_length_base[29] = {
	  3,   4,   5,   6,   7,   8,   9,  10,
	 11,  13,  15,  17,  19,  23,  27,  31,
	 35,  43,  51,  59,  67,  83,  99, 115,
	131, 163, 195, 227, 258
};
static const int pg_length_extra[29] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 1, 1, 2, 2, 2, 2,
	3, 3, 3, 3, 4, 4, 4, 4,
	5, 5, 5, 5, 0
};
static const int pg_dist_base[30] = {
	  1,   2,   3,   4,    5,    7,    9,   13,
	 17,  25,  33,  49,   65,   97,  129,  193,
	257, 385, 513, 769, 1025, 1537, 2049, 3073,
	4097, 6145, 8193, 12289, 16385, 24577
};
static const int pg_dist_extra[30] = {
	0, 0, 0, 0, 1, 1, 2, 2,
	3, 3, 4, 4, 5, 5, 6, 6,
	7, 7, 8, 8, 9, 9, 10, 10,
	11, 11, 12, 12, 13, 13
};

/* ===== Bit writer ===== */
typedef struct {
	unsigned char *buf;
	size_t cap;
	size_t pos;   /* byte position */
	int nbits;     /* bits in current byte (0-7) */
} pg_deflate_bw;

static void pg_dw_init(pg_deflate_bw *bw, unsigned char *buf, size_t cap)
{
	bw->buf = buf;
	bw->cap = cap;
	bw->pos = 0;
	bw->nbits = 0;
}

/* Tulis n bits (LSB first, sesuai DEFLATE). */
static void pg_dw_put(pg_deflate_bw *bw, unsigned code, int n)
{
	int i;
	for (i = 0; i < n; i++) {
		if (bw->nbits == 0) {
			bw->buf[bw->pos] = 0;
		}
		bw->buf[bw->pos] |= ((code >> i) & 1) << bw->nbits;
		bw->nbits++;
		if (bw->nbits == 8) {
			bw->nbits = 0;
			bw->pos++;
		}
	}
}

static void pg_dw_flush(pg_deflate_bw *bw)
{
	/* Bits tersisa di buf[pos] sudah ditulis, tinggal advance pos. */
	if (bw->nbits > 0) {
		bw->pos++;
		bw->nbits = 0;
	}
}

/* ===== Encode literal/length Huffman (static) ===== */
static void pg_dw_put_litlen(pg_deflate_bw *bw, int sym)
{
	if (sym <= 143) {
		/* 8 bits: 0x30 + sym, MSB first dalam stream DEFLATE.
		 * Tapi DEFLATE pakai LSB-first, jadi bit order dibalik. */
		unsigned code = 0x30 + sym;
		/* Reverse 8 bits. */
		unsigned r = 0;
		int i;
		for (i = 0; i < 8; i++) {
			r = (r << 1) | ((code >> i) & 1);
		}
		pg_dw_put(bw, r, 8);
	} else if (sym <= 255) {
		unsigned code = 0x190 + (sym - 144);
		unsigned r = 0;
		int i;
		for (i = 0; i < 9; i++) {
			r = (r << 1) | ((code >> i) & 1);
		}
		pg_dw_put(bw, r, 9);
	} else if (sym <= 279) {
		/* 7 bits: code = sym - 256. */
		unsigned code = sym - 256;
		unsigned r = 0;
		int i;
		for (i = 0; i < 7; i++) {
			r = (r << 1) | ((code >> i) & 1);
		}
		pg_dw_put(bw, r, 7);
	} else {
		/* 8 bits: code = 0xC0 + (sym - 280). */
		unsigned code = 0xC0 + (sym - 280);
		unsigned r = 0;
		int i;
		for (i = 0; i < 8; i++) {
			r = (r << 1) | ((code >> i) & 1);
		}
		pg_dw_put(bw, r, 8);
	}
}

static void pg_dw_put_dist(pg_deflate_bw *bw, int dist_sym)
{
	/* 5 bits, MSB first dalam stream. */
	unsigned r = 0;
	int i;
	for (i = 0; i < 5; i++) {
		r = (r << 1) | ((dist_sym >> i) & 1);
	}
	pg_dw_put(bw, r, 5);
}

/* ===== LZ77 match finder (greedy) ===== */

/* Hash 3 byte ke index table. */
static unsigned pg_hash3(const unsigned char *p)
{
	unsigned h = ((unsigned)p[0] << 16) | ((unsigned)p[1] << 8) | p[2];
	return (h * 2654435761u) >> (32 - 15); /* 15-bit hash */
}

/* Cari match terpanjang di window. Return panjang match (0 jika tidak ada). */
static int pg_find_match(const unsigned char *data, size_t pos, size_t len,
                          int *match_dist, unsigned hash_head[32768],
                          unsigned chain_prev[32768])
{
	int best_len = 0;
	int best_dist = 0;
	int max_len = (int)(len - pos);
	int max_dist;
	unsigned h;
	int cur;
	int limit;

	if (max_len < 3) return 0;
	if (max_len > 258) max_len = 258;
	max_dist = (int)pos;
	if (max_dist > 32768) max_dist = 32768;

	h = pg_hash3(data + pos);
	cur = hash_head[h];
	if (cur == 0 && pos > 0) return 0; /* tidak ada entry */

	limit = 32; /* max chain length (speed vs ratio trade-off) */
	while (cur >= 0 && cur < (int)pos && limit-- > 0) {
		int dist = (int)pos - cur;
		if (dist > max_dist) break;
		if (dist > 0) {
			int match_len = 0;
			while (match_len < max_len &&
			       data[cur + match_len] == data[pos + match_len]) {
				match_len++;
			}
			if (match_len >= 3 && match_len > best_len) {
				best_len = match_len;
				best_dist = dist;
				if (match_len >= max_len) break;
			}
		}
		cur = chain_prev[cur & 32767];
		if (cur == 0 && (int)pos > 0) break;
	}

	if (best_len >= 3) {
		*match_dist = best_dist;
		return best_len;
	}
	return 0;
}

static void pg_insert_hash(const unsigned char *data, size_t pos,
                             unsigned hash_head[32768],
                             unsigned chain_prev[32768])
{
	unsigned h;
	if (pos + 2 >= 32768 + 1) return; /* outside window */
	/* Tidak insert kalau pos+3 > len. */
	h = pg_hash3(data + pos);
	chain_prev[pos & 32767] = hash_head[h];
	hash_head[h] = (unsigned)pos;
}

/* ===== DEFLATE encode (static Huffman + LZ77) ===== */
unsigned char *pg_zlib_deflate_encode(const unsigned char *data,
                                         size_t len,
                                         size_t *out_len)
{
	unsigned char *out;
	size_t max_out;
	pg_deflate_bw bw;
	unsigned hash_head[32768];
	unsigned chain_prev[32768];
	size_t pos = 0;
	pg_u32 a32;
	size_t header;

	if (len == 0) {
		/* Empty stream: 2 byte zlib header + 1 empty block + 4 byte adler. */
		out = (unsigned char *)malloc(11);
		if (!out) return NULL;
		out[0] = 0x78; out[1] = 0x01; /* CMF, FLG */
		out[2] = 0x03; /* BFINAL=1, BTYPE=00 (stored), len=0 nlen=0xFFFF */
		/* Tapi stored block butuh 5 byte: 1 + 2 + 2. Untuk empty, tidak bisa stored.
		 * Pakai fixed Huffman dengan hanya EOB (256). */
		/* EOB (256) = 7 bits code 0. */
		out[2] = 3; /* BFINAL=1, BTYPE=01 (fixed) - tapi bit order... */
		/* Sebenarnya untuk empty lebih simpel: 1 byte 0x03 = BFINAL=1, BTYPE=00 (stored),
		 * lalu LEN=0 (2 byte LE), NLEN=0xFFFF (2 byte LE). */
		out[2] = 0x01; /* BFINAL=1, BTYPE=00 stored */
		out[3] = 0x00; out[4] = 0x00; /* LEN=0 */
		out[5] = 0xFF; out[6] = 0xFF; /* NLEN=0xFFFF */
		/* Adler32 of empty data = 1. */
		out[7] = 0; out[8] = 0; out[9] = 0; out[10] = 1;
		*out_len = 11;
		return out;
	}

	/* Worst case: 1 byte literal -> 9 bits, +5 byte header per stored block.
	 * Tapi kita pakai fixed Huffman, worst ~9/8 * len + 11 byte. */
	max_out = len * 2 + 256;
	out = (unsigned char *)malloc(max_out);
	if (!out) return NULL;

	/* Zlib header. */
	out[0] = 0x78; /* CMF: CM=8, CINFO=7 */
	out[1] = 0x01; /* FLG: FCHECK=1 (0x7801 % 31 == 0) */
	header = 2;

	pg_dw_init(&bw, out + header, max_out - header - 4);

	/* BFINAL=1, BTYPE=01 (fixed Huffman). */
	pg_dw_put(&bw, 1, 1); /* BFINAL */
	pg_dw_put(&bw, 1, 2); /* BTYPE=01 */

	/* Init hash tables. */
	memset(hash_head, 0, sizeof(hash_head));
	memset(chain_prev, 0, sizeof(chain_prev));

	while (pos < len) {
		int match_dist = 0;
		int match_len = 0;
		if (pos + 3 <= len) {
			match_len = pg_find_match(data, pos, len, &match_dist,
			                          hash_head, chain_prev);
		}
		if (match_len >= 3) {
			/* Encode match: length code + distance. */
			int len_code = 0;
			int i;
			int extra;
			for (i = 0; i < 29; i++) {
				if (match_len < pg_length_base[i] ||
				    (i < 28 && match_len >= pg_length_base[i+1])) continue;
				len_code = 257 + i;
				break;
			}
			pg_dw_put_litlen(&bw, len_code);
			/* Extra bits untuk length. */
			extra = match_len - pg_length_base[len_code - 257];
			if (pg_length_extra[len_code - 257] > 0) {
				pg_dw_put(&bw, extra, pg_length_extra[len_code - 257]);
			}
			/* Distance code. */
			{
				int dist_code = 0;
				for (i = 0; i < 30; i++) {
					if (match_dist < pg_dist_base[i] ||
					    (i < 29 && match_dist >= pg_dist_base[i+1])) continue;
					dist_code = i;
					break;
				}
				pg_dw_put_dist(&bw, dist_code);
				extra = match_dist - pg_dist_base[dist_code];
				if (pg_dist_extra[dist_code] > 0) {
					pg_dw_put(&bw, extra, pg_dist_extra[dist_code]);
				}
			}
			/* Insert hash untuk semua posisi di match. */
			{
				int j;
				for (j = 0; j < match_len; j++) {
					if (pos + j + 3 <= len) {
						pg_insert_hash(data, pos + j, hash_head, chain_prev);
					}
				}
			}
			pos += match_len;
		} else {
			/* Literal. */
			pg_dw_put_litlen(&bw, data[pos]);
			if (pos + 3 <= len) {
				pg_insert_hash(data, pos, hash_head, chain_prev);
			}
			pos++;
		}
	}

	/* End of block (256). */
	pg_dw_put_litlen(&bw, 256);
	pg_dw_flush(&bw);

	/* Adler32. */
	a32 = pg_zlib_adler32(data, len);
	out[header + bw.pos + 0] = (unsigned char)((a32 >> 24) & 0xFF);
	out[header + bw.pos + 1] = (unsigned char)((a32 >> 16) & 0xFF);
	out[header + bw.pos + 2] = (unsigned char)((a32 >> 8) & 0xFF);
	out[header + bw.pos + 3] = (unsigned char)(a32 & 0xFF);

	*out_len = header + bw.pos + 4;
	return out;
}
