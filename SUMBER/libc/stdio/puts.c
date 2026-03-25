/*
 * PIGURA LIBC - STDIO/PUTS.C
 * ============================
 * Implementasi fungsi puts, fputs, fputc, putchar.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

/* ============================================================
 * FPUTC
 * ============================================================
 * Tulis satu karakter ke stream.
 *
 * Parameter:
 *   c      - Karakter yang ditulis
 *   stream - Stream output
 *
 * Return: Karakter yang ditulis, atau EOF
 */
int fputc(int c, FILE *stream) {
    unsigned char ch;

    /* Validasi parameter */
    if (stream == NULL) {
        errno = EBADF;
        return EOF;
    }

    /* Cek apakah stream bisa ditulis */
    if (!(stream->flags & F_WRITE)) {
        stream->flags |= F_ERR;
        errno = EBADF;
        return EOF;
    }

    ch = (unsigned char)c;

    /* Jika tidak ada buffering */
    if (stream->buf == NULL || stream->buf_mode == _IONBF) {
        /* Tulis langsung */
        if (write(stream->fd, &ch, 1) != 1) {
            stream->flags |= F_ERR;
            return EOF;
        }
        return (int)ch;
    }

    /* Cek apakah buffer penuh */
    if (stream->buf_pos >= stream->buf_size) {
        if (fflush(stream) != 0) {
            return EOF;
        }
    }

    /* Tambahkan ke buffer */
    stream->buf[stream->buf_pos++] = ch;
    stream->buf_len = stream->buf_pos;

    /* Line buffering: flush jika newline */
    if (stream->buf_mode == _IOLBF && ch == '\n') {
        if (fflush(stream) != 0) {
            return EOF;
        }
    }

    return (int)ch;
}

/* ============================================================
 * PUTCHAR
 * ============================================================
 * Tulis satu karakter ke stdout.
 *
 * Parameter:
 *   c - Karakter yang ditulis
 *
 * Return: Karakter yang ditulis, atau EOF
 */
int putchar(int c) {
    return fputc(c, stdout);
}

/* ============================================================
 * FPUTS
 * ============================================================
 * Tulis string ke stream output.
 *
 * Parameter:
 *   s      - String yang ditulis
 *   stream - Stream output
 *
 * Return: Non-negative jika berhasil, EOF jika gagal
 */
int fputs(const char *s, FILE *stream) {
    size_t len;
    size_t written;
    const unsigned char *p;

    /* Validasi parameter */
    if (s == NULL) {
        errno = EINVAL;
        return EOF;
    }

    if (stream == NULL) {
        errno = EBADF;
        return EOF;
    }

    /* Cek apakah stream bisa ditulis */
    if (!(stream->flags & F_WRITE)) {
        stream->flags |= F_ERR;
        errno = EBADF;
        return EOF;
    }

    /* Hitung panjang string */
    len = strlen(s);

    /* Jika tidak ada buffering atau buffer kosong */
    if (stream->buf == NULL || stream->buf_mode == _IONBF) {
        /* Tulis langsung ke file descriptor */
        written = 0;
        p = (const unsigned char *)s;

        while (written < len) {
            ssize_t ret = write(stream->fd, p + written, len - written);
            if (ret < 0) {
                stream->flags |= F_ERR;
                return EOF;
            }
            written += (size_t)ret;
        }

        return 0;
    }

    /* Tulis melalui buffer */
    p = (const unsigned char *)s;

    for (len = 0; s[len] != '\0'; len++) {
        /* Cek apakah buffer penuh */
        if (stream->buf_pos >= stream->buf_size) {
            /* Flush buffer */
            if (fflush(stream) != 0) {
                return EOF;
            }
        }

        /* Tambahkan karakter ke buffer */
        stream->buf[stream->buf_pos++] = s[len];
        stream->buf_len = stream->buf_pos;

        /* Line buffering: flush jika newline */
        if (stream->buf_mode == _IOLBF && s[len] == '\n') {
            if (fflush(stream) != 0) {
                return EOF;
            }
        }
    }

    return 0;
}

/* ============================================================
 * PUTS
 * ============================================================
 * Tulis string ke stdout dengan newline.
 *
 * Parameter:
 *   s - String yang ditulis
 *
 * Return: Non-negative jika berhasil, EOF jika gagal
 */
int puts(const char *s) {
    /* Validasi parameter */
    if (s == NULL) {
        errno = EINVAL;
        return EOF;
    }

    /* Tulis string ke stdout */
    if (fputs(s, stdout) == EOF) {
        return EOF;
    }

    /* Tambahkan newline */
    if (fputc('\n', stdout) == EOF) {
        return EOF;
    }

    /* Flush jika line buffering */
    if (stdout->buf_mode == _IOLBF) {
        if (fflush(stdout) != 0) {
            return EOF;
        }
    }

    return 0;
}

/* ============================================================
 * PUTW
 * ============================================================
 * Tulis word (int) ke stream.
 *
 * Parameter:
 *   w      - Word yang ditulis
 *   stream - Stream output
 *
 * Return: 0 jika berhasil, EOF jika gagal
 */
int putw(int w, FILE *stream) {
    unsigned char *p;
    int i;

    if (stream == NULL) {
        errno = EBADF;
        return EOF;
    }

    p = (unsigned char *)&w;

    for (i = 0; i < (int)sizeof(int); i++) {
        if (fputc(p[i], stream) == EOF) {
            return EOF;
        }
    }

    return 0;
}
