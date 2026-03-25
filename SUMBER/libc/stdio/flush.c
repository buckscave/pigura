/*
 * PIGURA LIBC - STDIO/FLUSH.C
 * =============================
 * Implementasi fungsi fflush, setbuf, setvbuf.
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

/* Pool FILE - dideklarasikan di file.c */
extern FILE _file_pool[];
extern int _file_pool_used[];
extern int _file_pool_initialized;

/* Stream standar - dideklarasikan di file.c */
extern FILE *_stdin;
extern FILE *_stdout;
extern FILE *_stderr;

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
    ssize_t written;
    size_t total;
    unsigned char *p;

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
 * SETBUF
 * ============================================================
 * Set buffer untuk stream.
 *
 * Parameter:
 *   stream - Stream yang diatur
 *   buf    - Buffer (NULL untuk unbuffered)
 */
void setbuf(FILE *stream, char *buf) {
    if (stream == NULL) {
        return;
    }

    if (buf == NULL) {
        /* Unbuffered */
        setvbuf(stream, NULL, _IONBF, 0);
    } else {
        /* Full buffering dengan buffer yang diberikan */
        setvbuf(stream, buf, _IOFBF, BUFSIZ);
    }
}

/* ============================================================
 * SETVBUF
 * ============================================================
 * Set buffer dan mode untuk stream.
 *
 * Parameter:
 *   stream  - Stream yang diatur
 *   buf     - Buffer (NULL untuk auto-allocation)
 *   mode    - Mode buffering (_IOFBF, _IOLBF, _IONBF)
 *   size    - Ukuran buffer
 *
 * Return: 0 jika berhasil, non-zero jika gagal
 */
int setvbuf(FILE *stream, char *buf, int mode, size_t size) {
    /* Validasi parameter */
    if (stream == NULL) {
        return -1;
    }

    /* Hanya boleh dipanggil sebelum operasi I/O */
    if (stream->buf_pos > 0 || stream->buf_len > 0) {
        return -1;
    }

    /* Validasi mode */
    if (mode != _IOFBF && mode != _IOLBF && mode != _IONBF) {
        return -1;
    }

    /* Bebaskan buffer lama jika dialokasi sistem */
    if (stream->buf != NULL && (stream->flags & F_BUF)) {
        free(stream->buf);
        stream->flags &= ~F_BUF;
    }

    /* Set mode dan buffer */
    stream->buf_mode = mode;

    if (mode == _IONBF) {
        /* Unbuffered */
        stream->buf = NULL;
        stream->buf_size = 0;
    } else if (buf != NULL) {
        /* Gunakan buffer yang diberikan */
        stream->buf = (unsigned char *)buf;
        stream->buf_size = size;
    } else {
        /* Alokasikan buffer baru */
        if (size == 0) {
            size = BUFSIZ;
        }

        stream->buf = (unsigned char *)malloc(size);
        if (stream->buf == NULL) {
            /* Fallback ke unbuffered */
            stream->buf_mode = _IONBF;
            stream->buf_size = 0;
            return -1;
        }
        stream->buf_size = size;
        stream->flags |= F_BUF;
    }

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
