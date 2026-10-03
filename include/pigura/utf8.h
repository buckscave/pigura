/* ----------------------------------------------------------------------------------------------
 * pigura/utf8.h - Codec Unicode UTF-8
 * ----------------------------------------------------------------------------------------------
 * Decoder dan encoder UTF-8 minimal, murni C89, tanpa dependensi.
 * Mengikuti RFC 3629.
 *
 * Aturan UTF-8:
 *   1 byte : 0xxxxxxx                       (U+0000..U+007F)
 *   2 byte : 110xxxxx 10xxxxxx              (U+0080..U+07FF)
 *   3 byte : 1110xxxx 10xxxxxx 10xxxxxx     (U+0800..U+FFFF)
 *   4 byte : 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx (U+10000..U+10FFFF)
 *
 * Surrogat pair (U+D800..U+DFFF) ditolak; codepoint > U+10FFFF
 * ditolak. Untuk string ASCII murni, fungsi berperilaku identik
 * dengan iterator byte-per-byte.
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_UTF8_H
#define PIGURA_UTF8_H

#include "pigura/tipe.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Decode 1 codepoint dari string UTF-8.
 * Mengembalikan jumlah byte yang dikonsumsi (1..4).
 * 0 = invalid sequence (str NULL atau encoding rusak).
 * Codepoint ditulis ke *codepoint (jika non-NULL).
 *
 * Catatan: tidak ada deteksi end-of-string eksplisit; pemanggil
 * wajib memastikan str menunjuk ke minimal N byte valid. Untuk
 * iterasi string null-terminated, hentikan saat str[0] == 0. */
int pg_utf8_decode(const char *str, pg_u32 *codepoint);

/* Encode 1 codepoint ke buffer out (minimal 4 byte).
 * Mengembalikan jumlah byte ditulis (1..4).
 * 0 = invalid codepoint (> U+10FFFF atau surrogat).
 * Buffer out TIDAK di-null-terminate. */
int pg_utf8_encode(pg_u32 codepoint, char *out);

/* Hitung jumlah codepoint dalam string UTF-8 null-terminated.
 * Byte invalid dilewati sebagai 1 codepoint (panjang byte). */
int pg_utf8_panjang(const char *str);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_UTF8_H */
