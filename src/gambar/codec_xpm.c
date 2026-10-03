/* -------------------------------------------------------------------------- *
 * pigura/gambar/codec_xpm.c - codec XPM (X11 Pixmap, decode)
 * -------------------------------------------------------------------------- *
 * Format XPM (X PixMap) adalah format text yang umum di X11/Linux untuk
 * ikon dan pixmap kecil. Format ini cocok untuk UI toolkit karena
 * banyak dipakai di desktop environment Linux.
 *
 * Format XPM:
 *   static char *image[] = {
 *   "16 16 3 1",          // width height ncolors chars_per_pixel
 *   "  c #000000",        // color 0: space -> black
 *   ". c #FFFFFF",        // color 1: dot -> white
 *   "X c #FF0000",        // color 2: X -> red
 *   "                ",   // pixel data, 16 char per row
 *   "..              ",
 *   ...
 *   };
 *
 * Decoder ini parse text XPM dan return pixel data RGBA.
 * -------------------------------------------------------------------------- */
#include "pigura/codec.h"
#include "pigura/galat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Helper: parse hex color "#RRGGBB" atau "#RRGGBBAA". */
static int pg_xpm_parse_hex_color(const char *s, pg_warna_t *out)
{
	unsigned int r = 0, g = 0, b = 0, a = 255;
	if (*s == '#') s++;
	if (strlen(s) >= 6) {
		if (sscanf(s, "%2x%2x%2x", &r, &g, &b) != 3) return 0;
		if (strlen(s) >= 8) {
			if (sscanf(s + 6, "%2x", &a) != 1) a = 255;
		}
	}
	*out = ((pg_warna_t)(((pg_u8)(r) << 16) | ((pg_u8)(g) << 8) | (pg_u8)(b)));
	return 1;
}

/* Helper: parse color name (terbatas - hanya "None" = transparent). */
static int pg_xpm_parse_color_name(const char *s, pg_warna_t *out)
{
	if (strcmp(s, "None") == 0 || strcmp(s, "none") == 0) {
		*out = ((pg_warna_t)0x000000u);
		return 1;
	}
	*out = ((pg_warna_t)0x000000u);
	return 1;
}

/* Decode XPM dari array string C.
 * Berguna untuk ikon yang di-compile langsung di kode aplikasi. */
pg_byte *pg_muat_xpm_dari_string(const char *xpm[],
                                    int *lebar, int *tinggi,
                                    int *channel_aktual)
{
	int w, h, ncolors, cpp;
	int i, x, y;
	pg_byte *hasil;
	pg_warna_t palette[256];

	if (!xpm || !xpm[0]) {
		pg_set_galat(PG_GALAT_ARGUMEN, "pg_muat_xpm: input kosong");
		return NULL;
	}

	/* Parse header: "width height ncolors chars_per_pixel". */
	if (sscanf(xpm[0], "%d %d %d %d", &w, &h, &ncolors, &cpp) != 4) {
		pg_set_galat(PG_GALAT_UMUM, "pg_muat_xpm: header tidak valid");
		return NULL;
	}
	if (w <= 0 || h <= 0 || ncolors <= 0 || cpp <= 0 || cpp > 4) {
		pg_set_galat(PG_GALAT_ARGUMEN, "pg_muat_xpm: dimensi tidak valid");
		return NULL;
	}

	/* Init palette default hitam opaque. */
	for (i = 0; i < 256; i++) palette[i] = ((pg_warna_t)0x000000u);

	/* Parse ncolors baris palette.
	 * Format: "{chars} c {color}" - kita hanya handle "c" (color). */
	for (i = 0; i < ncolors; i++) {
		const char *line = xpm[1 + i];
		const char *p;
		unsigned char code = 0;
		if (!line) continue;

		/* Char code (cpp chars, kita pakai char pertama saja untuk simpel). */
		code = (unsigned char)line[0];
		p = line + cpp;
		while (*p == ' ' || *p == '\t') p++;

		/* Cari "c " lalu parse color. */
		while (*p) {
			if (p[0] == 'c' && p[1] == ' ') {
				p += 2;
				while (*p == ' ' || *p == '\t') p++;
				if (*p == '#') {
					pg_xpm_parse_hex_color(p, &palette[code]);
				} else {
					char nama[64];
					int n = 0;
					while (*p && *p != ' ' && *p != '\t' && n < 63) {
						nama[n++] = *p++;
					}
					nama[n] = 0;
					pg_xpm_parse_color_name(nama, &palette[code]);
				}
				break;
			}
			p++;
		}
	}

	/* Alokasi pixel data (RGBA). */
	hasil = (pg_byte *)malloc((size_t)w * h * 4);
	if (!hasil) {
		pg_set_galat(PG_GALAT_MEMORI, "pg_muat_xpm: OOM pixel");
		return NULL;
	}

	/* Parse pixel data (h baris setelah palette). */
	for (y = 0; y < h; y++) {
		const char *line = xpm[1 + ncolors + y];
		if (!line) {
			free(hasil);
			pg_set_galat(PG_GALAT_UMUM, "pg_muat_xpm: data tidak lengkap");
			return NULL;
		}
		for (x = 0; x < w; x++) {
			unsigned char code;
			pg_warna_t c;
			code = (unsigned char)line[x * cpp];
			c = palette[code];
			hasil[(y * w + x) * 4 + 0] = PG_R(c);
			hasil[(y * w + x) * 4 + 1] = PG_G(c);
			hasil[(y * w + x) * 4 + 2] = PG_B(c);
			hasil[(y * w + x) * 4 + 3] = ((pg_byte)((pg_warna_t)(c) >> 24) & 0xff);
		}
	}

	*lebar = w;
	*tinggi = h;
	if (channel_aktual) *channel_aktual = 4;
	return hasil;
}
