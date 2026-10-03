/* ----------------------------------------------------------------------------------------------
 * pigura inti: utf8.c - codec UTF-8
 * ----------------------------------------------------------------------------------------------
 * Implementasi murni C89 dari decoder dan encoder UTF-8 sesuai
 * RFC 3629. Tidak ada dependensi eksternal.
 *
 * Strategi:
 *   - decode: periksa byte pertama untuk panjang sequence (1..4),
 *     validasi continuation byte (10xxxxxx), rangkum bit payload.
 *   - encode: bagikan codepoint ke 1..4 byte sesuai rentang.
 *
 * Keputusan desain:
 *   - Toleransi terhadap invalid sequence: kembalikan codepoint
 *     U+FFFD (replacement) dan konsumsi 1 byte supaya iterator
 *     maju. Tapi signature function (return 0) membedakan invalid
 *     vs valid, supaya pemanggil bisa menangani secara berbeda.
 *   - Surrogat pair (U+D800..U+DFFF) ditolak karena tidak valid
 *     sebagai codepoint Unicode langsung (hanya sebagai pasangan
 *     UTF-16).
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/utf8.h"

/* Bit mask bantu. */
#define PG_U8_MASK1 0x80u  /* 10000000 - cek top bit */
#define PG_U8_VAL1  0x00u  /* 0xxxxxxx */
#define PG_U8_MASK2 0xE0u  /* 11100000 - cek top 3 bit */
#define PG_U8_VAL2  0xC0u  /* 110xxxxx */
#define PG_U8_MASK3 0xF0u  /* 11110000 - cek top 4 bit */
#define PG_U8_VAL3  0xE0u  /* 1110xxxx */
#define PG_U8_MASK4 0xF8u  /* 11111000 - cek top 5 bit */
#define PG_U8_VAL4  0xF0u  /* 11110xxx */
#define PG_U8_CTMASK 0xC0u /* 11000000 - cek top 2 bit continuation */
#define PG_U8_CTVAL  0x80u /* 10xxxxxx */

/* Codepoint batas rentang (inklusif bawah, eksklusif atas). */
#define PG_U8_MAX_1B  0x80u
#define PG_U8_MAX_2B  0x800u
#define PG_U8_MAX_3B  0x10000u
#define PG_U8_MAX_4B  0x110000u

/* Surrogat UTF-16 (invalid sebagai codepoint Unicode). */
#define PG_U8_SUR_LO  0xD800u
#define PG_U8_SUR_HI  0xE000u

int pg_utf8_decode(const char *str, pg_u32 *codepoint)
{
        const unsigned char *s = (const unsigned char *)str;
        unsigned char b0, b1, b2, b3;
        pg_u32 cp;

        if (!s) {
                if (codepoint) *codepoint = 0;
                return 0;
        }

        b0 = s[0];

        /* 1 byte: 0xxxxxxx (0x00..0x7F). */
        if ((b0 & PG_U8_MASK1) == PG_U8_VAL1) {
                if (codepoint) *codepoint = (pg_u32)b0;
                return 1;
        }

        /* 2 byte: 110xxxxx 10xxxxxx (0x80..0x7FF). */
        if ((b0 & PG_U8_MASK2) == PG_U8_VAL2) {
                b1 = s[1];
                if ((b1 & PG_U8_CTMASK) != PG_U8_CTVAL) {
                        if (codepoint) *codepoint = 0xFFFDu;
                        return 0;
                }
                cp = ((pg_u32)(b0 & 0x1Fu) << 6) |
                      (pg_u32)(b1 & 0x3Fu);
                /* Overlong check: harus >= 0x80. */
                if (cp < PG_U8_MAX_1B) {
                        if (codepoint) *codepoint = 0xFFFDu;
                        return 0;
                }
                if (codepoint) *codepoint = cp;
                return 2;
        }

        /* 3 byte: 1110xxxx 10xxxxxx 10xxxxxx (0x800..0xFFFF). */
        if ((b0 & PG_U8_MASK3) == PG_U8_VAL3) {
                b1 = s[1];
                b2 = s[2];
                if (((b1 & PG_U8_CTMASK) != PG_U8_CTVAL) ||
                    ((b2 & PG_U8_CTMASK) != PG_U8_CTVAL)) {
                        if (codepoint) *codepoint = 0xFFFDu;
                        return 0;
                }
                cp = ((pg_u32)(b0 & 0x0Fu) << 12) |
                     ((pg_u32)(b1 & 0x3Fu) <<  6) |
                      (pg_u32)(b2 & 0x3Fu);
                /* Overlong: harus >= 0x800. */
                if (cp < PG_U8_MAX_2B) {
                        if (codepoint) *codepoint = 0xFFFDu;
                        return 0;
                }
                /* Surrogat UTF-16: invalid. */
                if (cp >= PG_U8_SUR_LO && cp < PG_U8_SUR_HI) {
                        if (codepoint) *codepoint = 0xFFFDu;
                        return 0;
                }
                if (codepoint) *codepoint = cp;
                return 3;
        }

        /* 4 byte: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
         *         (0x10000..0x10FFFF). */
        if ((b0 & PG_U8_MASK4) == PG_U8_VAL4) {
                b1 = s[1];
                b2 = s[2];
                b3 = s[3];
                if (((b1 & PG_U8_CTMASK) != PG_U8_CTVAL) ||
                    ((b2 & PG_U8_CTMASK) != PG_U8_CTVAL) ||
                    ((b3 & PG_U8_CTMASK) != PG_U8_CTVAL)) {
                        if (codepoint) *codepoint = 0xFFFDu;
                        return 0;
                }
                cp = ((pg_u32)(b0 & 0x07u) << 18) |
                     ((pg_u32)(b1 & 0x3Fu) << 12) |
                     ((pg_u32)(b2 & 0x3Fu) <<  6) |
                      (pg_u32)(b3 & 0x3Fu);
                /* Overlong: harus >= 0x10000. */
                if (cp < PG_U8_MAX_3B) {
                        if (codepoint) *codepoint = 0xFFFDu;
                        return 0;
                }
                /* Range maksimum: U+10FFFF. */
                if (cp >= PG_U8_MAX_4B) {
                        if (codepoint) *codepoint = 0xFFFDu;
                        return 0;
                }
                if (codepoint) *codepoint = cp;
                return 4;
        }

        /* Byte 0x80..0xBF (continuation tanpa head), 0xF5..0xFF:
         * invalid. Konsumsi 1 byte supaya pemanggil bisa lanjut. */
        if (codepoint) *codepoint = 0xFFFDu;
        return 0;
}

int pg_utf8_encode(pg_u32 codepoint, char *out)
{
        unsigned char *o = (unsigned char *)out;

        if (!o) return 0;

        /* Validasi range codepoint + tolak surrogat. */
        if (codepoint >= PG_U8_SUR_LO && codepoint < PG_U8_SUR_HI)
                return 0;
        if (codepoint >= PG_U8_MAX_4B)
                return 0;

        if (codepoint < PG_U8_MAX_1B) {
                /* 1 byte: 0xxxxxxx. */
                o[0] = (unsigned char)codepoint;
                return 1;
        }
        if (codepoint < PG_U8_MAX_2B) {
                /* 2 byte: 110xxxxx 10xxxxxx. */
                o[0] = (unsigned char)(0xC0u |
                        (unsigned char)(codepoint >> 6));
                o[1] = (unsigned char)(0x80u |
                        (unsigned char)(codepoint & 0x3Fu));
                return 2;
        }
        if (codepoint < PG_U8_MAX_3B) {
                /* 3 byte: 1110xxxx 10xxxxxx 10xxxxxx. */
                o[0] = (unsigned char)(0xE0u |
                        (unsigned char)(codepoint >> 12));
                o[1] = (unsigned char)(0x80u |
                        (unsigned char)((codepoint >> 6) & 0x3Fu));
                o[2] = (unsigned char)(0x80u |
                        (unsigned char)(codepoint & 0x3Fu));
                return 3;
        }
        /* 4 byte: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx. */
        o[0] = (unsigned char)(0xF0u |
                (unsigned char)(codepoint >> 18));
        o[1] = (unsigned char)(0x80u |
                (unsigned char)((codepoint >> 12) & 0x3Fu));
        o[2] = (unsigned char)(0x80u |
                (unsigned char)((codepoint >> 6) & 0x3Fu));
        o[3] = (unsigned char)(0x80u |
                (unsigned char)(codepoint & 0x3Fu));
        return 4;
}

int pg_utf8_panjang(const char *str)
{
        const char *p;
        int n = 0;
        if (!str) return 0;
        for (p = str; *p; ) {
                pg_u32 cp;
                int adv = pg_utf8_decode(p, &cp);
                if (adv <= 0) {
                        /* Invalid byte: skip 1, count as 1 codepoint. */
                        p++;
                } else {
                        p += adv;
                }
                n++;
        }
        return n;
}
