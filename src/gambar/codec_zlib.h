/* -------------------------------------------------------------------------- *
 * pigura/gambar/codec_zlib.h - DEFLATE encoder internal header
 * -------------------------------------------------------------------------- */
#ifndef PIGURA_CODEC_ZLIB_H
#define PIGURA_CODEC_ZLIB_H
#include <stddef.h>
unsigned char *pg_zlib_deflate_encode(const unsigned char *data,
                                         size_t len, size_t *out_len);
#endif
