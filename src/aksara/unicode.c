/* -------------------------------------------------------------------------- *
 * pigura/aksara/unicode.c - helper Unicode (UTF-8 decode/encode)
 * -------------------------------------------------------------------------- *
 * Fungsi-fungsi utilitas untuk manipulasi string UTF-8.
 *
 * UTF-8 encoding:
 *   1 byte: 0xxxxxxx                       (0x00-0x7F)
 *   2 byte: 110xxxxx 10xxxxxx              (0x80-0x7FF)
 *   3 byte: 1110xxxx 10xxxxxx 10xxxxxx     (0x800-0xFFFF)
 *   4 byte: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx  (0x10000-0x10FFFF)
 *
 * BMP (Basic Multilingual Plane) = codepoint 0x0000-0xFFFF.
 * Pigura TTF decoder hanya dukung BMP (cmap format 4).
 * -------------------------------------------------------------------------- */
#include "pigura/font.h"
#include <string.h>

/* Decode 1 codepoint UTF-8 dari *p. Advance p ke byte berikutnya.
 * Return codepoint, atau -1 bila invalid. */
int pg_aksara_utf8_dekode(const char **p, const char *akhir)
{
	const unsigned char *s = (const unsigned char *)*p;
	const unsigned char *e = (const unsigned char *)akhir;
	unsigned char c;
	int cp;
	int len;
	int i;

	if (s >= e) return -1;
	c = s[0];
	if (c < 0x80) {
		*p = (const char *)(s + 1);
		return c;
	}
	if ((c & 0xE0) == 0xC0) {
		len = 2;
		cp = c & 0x1F;
	} else if ((c & 0xF0) == 0xE0) {
		len = 3;
		cp = c & 0x0F;
	} else if ((c & 0xF8) == 0xF0) {
		len = 4;
		cp = c & 0x07;
	} else {
		*p = (const char *)(s + 1);
		return -1;
	}
	if (s + len > e) {
		*p = (const char *)e;
		return -1;
	}
	for (i = 1; i < len; i++) {
		unsigned char b = s[i];
		if ((b & 0xC0) != 0x80) {
			*p = (const char *)(s + i);
			return -1;
		}
		cp = (cp << 6) | (b & 0x3F);
	}
	if (cp > 0x10FFFF) return -1;
	if (cp >= 0xD800 && cp <= 0xDFFF) return -1;
	*p = (const char *)(s + len);
	return cp;
}

/* Encode codepoint ke UTF-8 di buf. Return jumlah byte ditulis (1-4),
 * atau 0 bila codepoint tidak valid. */
int pg_aksara_utf8_enkode(char *buf, int cp)
{
	if (cp < 0 || cp > 0x10FFFF) return 0;
	if (cp >= 0xD800 && cp <= 0xDFFF) return 0;
	if (cp < 0x80) {
		buf[0] = (char)cp;
		return 1;
	}
	if (cp < 0x800) {
		buf[0] = (char)(0xC0 | (cp >> 6));
		buf[1] = (char)(0x80 | (cp & 0x3F));
		return 2;
	}
	if (cp < 0x10000) {
		buf[0] = (char)(0xE0 | (cp >> 12));
		buf[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
		buf[2] = (char)(0x80 | (cp & 0x3F));
		return 3;
	}
	buf[0] = (char)(0xF0 | (cp >> 18));
	buf[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
	buf[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
	buf[3] = (char)(0x80 | (cp & 0x3F));
	return 4;
}

/* Hitung panjang string UTF-8 dalam codepoint (bukan byte). */
int pg_aksara_utf8_panjang(const char *s)
{
	int n = 0;
	const char *p = s;
	const char *akhir = s + strlen(s);
	while (p < akhir) {
		int cp = pg_aksara_utf8_dekode(&p, akhir);
		if (cp < 0) break;
		n++;
	}
	return n;
}

/* Cek apakah codepoint adalah whitespace Unicode. */
int pg_aksara_apa_whitespace(int cp)
{
	switch (cp) {
	case 0x09: case 0x0A: case 0x0B: case 0x0C: case 0x0D:
	case 0x20: case 0x85: case 0xA0:
	case 0x1680: case 0x2000: case 0x2001: case 0x2002: case 0x2003:
	case 0x2004: case 0x2005: case 0x2006: case 0x2007: case 0x2008:
	case 0x2009: case 0x200A: case 0x2028: case 0x2029: case 0x202F:
	case 0x205F: case 0x3000:
		return 1;
	}
	return 0;
}

/* Cek apakah codepoint adalah control character (Cc category). */
int pg_aksara_apa_kontrol(int cp)
{
	return (cp >= 0 && cp <= 0x1F) || (cp >= 0x7F && cp <= 0x9F);
}

/* Cek apakah codepoint adalah digit ASCII. */
int pg_aksara_apa_digit(int cp)
{
	return cp >= '0' && cp <= '9';
}

/* Cek apakah codepoint adalah huruf ASCII. */
int pg_aksara_apa_huruf(int cp)
{
	return (cp >= 'A' && cp <= 'Z') || (cp >= 'a' && cp <= 'z');
}
