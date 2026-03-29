/*
 * PIGURA LIBC - STDIO/FLUSH.C
 * =============================
 * Implementasi fungsi fflush, setbuffer, setlinebuf.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

/* ============================================================
 * VARIABEL GLOBAL
 * ============================================================
 */

/* Pool FILE - dideklarasikan di file.c (tanpa static) */
extern FILE _file_pool[];
extern int _file_pool_used[];
extern int _file_pool_initialized;

/* Forward declaration */
int _flush_buffer(FILE *stream);

/* ============================================================
 * FFLUSH
 * ============================================================
 * Flush buffer ke file.
 *
 * Parameter:
 *   stream - Stream yang di-flush (NULL untuk semua)
 *
 * Return: 0 jika berhasil, EOF jika gagal
 */
int fflush(FILE *stream) {
    /* Jika stream NULL, flush semua stream output */
    if (stream == NULL) {
        int result = 0;
        int i;

        /* Flush stdout */
        if (stdout != NULL && (stdout->flags & F_WRITE) &&
            stdout->buf_pos > 0) {
            if (_flush_buffer(stdout) != 0) {
                result = EOF;
            }
        }

        /* Flush stderr */
        if (stderr != NULL && (stderr->flags & F_WRITE) &&
            stderr->buf_pos > 0) {
            if (_flush_buffer(stderr) != 0) {
                result = EOF;
            }
        }

        /* Flush semua file dalam pool */
        if (_file_pool_initialized) {
            for (i = 0; i < FOPEN_MAX; i++) {
                if (_file_pool_used[i] &&
                    (_file_pool[i].flags & F_WRITE) &&
                    _file_pool[i].buf_pos > 0) {
                    if (_flush_buffer(&_file_pool[i]) != 0) {
                        result = EOF;
                    }
                }
            }
        }

        return result;
    }

    /* Validasi stream */
    if (!(stream->flags & F_OPEN)) {
        errno = EBADF;
        return EOF;
    }

    /* Hanya flush jika mode tulis */
    if (!(stream->flags & F_WRITE)) {
        return 0;
    }

    /* Flush buffer */
    return _flush_buffer(stream);
}

/* ============================================================
 * _FLUSH_BUFFER (Internal)
 * ============================================================
 * Flush buffer internal stream.
 *
 * Parameter:
 *   stream - Stream yang di-flush
 *
 * Return: 0 jika berhasil, EOF jika gagal
 */
int _flush_buffer(FILE *stream) {
    ssize_t written;
    size_t total;
    unsigned char *p;

    if (stream == NULL || stream->buf == NULL) {
        return 0;
    }

    /* Tidak ada data untuk di-flush */
    if (stream->buf_pos == 0) {
        return 0;
    }

    /* Tulis buffer ke file descriptor */
    p = stream->buf;
    total = stream->buf_pos;
    written = 0;

    while (total > 0) {
        ssize_t ret = write(stream->fd, p + written, total);

        if (ret < 0) {
            stream->flags |= F_ERR;
            return EOF;
        }

        if (ret == 0) {
            /* Tidak bisa menulis */
            stream->flags |= F_ERR;
            return EOF;
        }

        written += ret;
        total -= ret;
    }

    /* Reset buffer */
    stream->buf_pos = 0;
    stream->buf_len = 0;

    return 0;
}

/* ============================================================
 * SETBUFFER (BSD Extension)
 * ============================================================
 * Set buffer dengan ukuran yang bisa ditentukan.
 *
 * Parameter:
 *   stream - Stream yang diatur
 *   buf    - Buffer (NULL untuk auto-allocation)
 *   size   - Ukuran buffer
 */
void setbuffer(FILE *stream, char *buf, size_t size) {
    if (stream == NULL) {
        return;
    }

    if (buf == NULL) {
        setvbuf(stream, NULL, _IONBF, 0);
    } else {
        setvbuf(stream, buf, _IOFBF, size);
    }
}

/* ============================================================
 * SETLINEBUF (BSD Extension)
 * ============================================================
 * Set stream ke mode line buffering.
 *
 * Parameter:
 *   stream - Stream yang diatur
 */
void setlinebuf(FILE *stream) {
    if (stream == NULL) {
        return;
    }

    setvbuf(stream, NULL, _IOLBF, 0);
}

/* ============================================================
 * __FLUSH_ALL (Internal)
 * ============================================================
 * Flush semua stream yang terbuka.
 * Dipanggil oleh __stdio_cleanup().
 *
 * Return: 0 jika berhasil, EOF jika ada error
 */
int __flush_all(void) {
    return fflush(NULL);
}
