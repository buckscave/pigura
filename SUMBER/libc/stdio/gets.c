/*
 * PIGURA LIBC - STDIO/GETS.C
 * ============================
 * Implementasi fungsi gets dan fgets.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 *
 * PERINGATAN: gets() tidak aman karena tidak membatasi jumlah
 * karakter yang dibaca. Gunakan fgets() sebagai gantinya.
 */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>

/* ============================================================
 * FGETC
 * ============================================================
 * Baca satu karakter dari stream.
 *
 * Parameter:
 *   stream - Stream input
 *
 * Return: Karakter yang dibaca, atau EOF
 */
int fgetc(FILE *stream) {
    unsigned char ch;
    ssize_t ret;

    /* Validasi parameter */
    if (stream == NULL) {
        errno = EBADF;
        return EOF;
    }

    /* Cek apakah stream bisa dibaca */
    if (!(stream->flags & F_READ)) {
        stream->flags |= F_ERR;
        errno = EBADF;
        return EOF;
    }

    /* Cek EOF flag */
    if (stream->flags & F_EOF) {
        return EOF;
    }

    /* Jika tidak ada buffering */
    if (stream->buf == NULL || stream->buf_mode == _IONBF) {
        /* Baca langsung dari file descriptor */
        ret = read(stream->fd, &ch, 1);

        if (ret == 0) {
            /* EOF */
            stream->flags |= F_EOF;
            return EOF;
        }

        if (ret < 0) {
            /* Error */
            stream->flags |= F_ERR;
            return EOF;
        }

        return (int)ch;
    }

    /* Cek apakah buffer kosong */
    if (stream->buf_pos >= stream->buf_len) {
        /* Isi buffer */
        ret = read(stream->fd, stream->buf, stream->buf_size);

        if (ret == 0) {
            /* EOF */
            stream->flags |= F_EOF;
            return EOF;
        }

        if (ret < 0) {
            /* Error */
            stream->flags |= F_ERR;
            return EOF;
        }

        stream->buf_len = (size_t)ret;
        stream->buf_pos = 0;
    }

    /* Ambil karakter dari buffer */
    ch = stream->buf[stream->buf_pos++];

    return (int)ch;
}

/* ============================================================
 * FGETS
 * ============================================================
 * Baca string dari stream dengan batasan ukuran.
 *
 * Parameter:
 *   s      - Buffer tujuan
 *   n      - Ukuran buffer
 *   stream - Stream input
 *
 * Return: s jika berhasil, NULL jika EOF/error
 */
char *fgets(char *s, int n, FILE *stream) {
    int c;
    int i;

    /* Validasi parameter */
    if (s == NULL || n <= 0) {
        errno = EINVAL;
        return NULL;
    }

    if (stream == NULL) {
        errno = EBADF;
        return NULL;
    }

    /* Cek apakah stream bisa dibaca */
    if (!(stream->flags & F_READ)) {
        stream->flags |= F_ERR;
        errno = EBADF;
        return NULL;
    }

    /* Cek EOF flag */
    if (stream->flags & F_EOF) {
        return NULL;
    }

    /* Baca karakter satu per satu */
    for (i = 0; i < n - 1; i++) {
        /* Baca karakter berikutnya */
        c = fgetc(stream);

        /* Cek EOF atau error */
        if (c == EOF) {
            /* Jika sudah ada data, null-terminate dan return */
            if (i > 0) {
                s[i] = '\0';
                return s;
            }
            return NULL;
        }

        /* Simpan karakter */
        s[i] = (char)c;

        /* Berhenti jika newline */
        if (c == '\n') {
            i++;
            break;
        }
    }

    /* Null-terminate */
    s[i] = '\0';

    return s;
}

/* ============================================================
 * GETS
 * ============================================================
 * Baca string dari stdin TANPA batasan ukuran.
 *
 * PERINGATAN: Fungsi ini BERBAHAYA dan DEPRECATED!
 * Buffer overflow dapat terjadi jika input melebihi
 * ukuran buffer. Gunakan fgets() sebagai gantinya.
 *
 * Parameter:
 *   s - Buffer tujuan
 *
 * Return: s jika berhasil, NULL jika error
 */
char *gets(char *s) {
    int c;
    int i;

    /* Validasi parameter */
    if (s == NULL) {
        errno = EINVAL;
        return NULL;
    }

    /*
     * CATATAN: Fungsi ini sengaja diimplementasikan tanpa
     * batasan ukuran karena itulah definisi standar C89.
     * Namun penggunaan TIDAK DISARANKAN karena berbahaya.
     * Compiler modern mungkin menghasilkan warning.
     */

    i = 0;

    /* Baca karakter sampai newline atau EOF */
    while (1) {
        c = fgetc(stdin);

        /* Cek EOF atau error */
        if (c == EOF) {
            /* Jika sudah ada data, null-terminate dan return */
            if (i > 0) {
                s[i] = '\0';
                return s;
            }
            return NULL;
        }

        /* Berhenti jika newline (tidak disimpan) */
        if (c == '\n') {
            break;
        }

        /* Simpan karakter */
        s[i++] = (char)c;
    }

    /* Null-terminate */
    s[i] = '\0';

    return s;
}

/* ============================================================
 * GETC
 * ============================================================
 * Baca satu karakter dari stream (macro version).
 * Implementasi sebagai fungsi untuk kompatibilitas.
 *
 * Parameter:
 *   stream - Stream input
 *
 * Return: Karakter yang dibaca, atau EOF
 */
int getc(FILE *stream) {
    return fgetc(stream);
}

/* ============================================================
 * GETCHAR
 * ============================================================
 * Baca satu karakter dari stdin.
 *
 * Return: Karakter yang dibaca, atau EOF
 */
int getchar(void) {
    return fgetc(stdin);
}

/* ============================================================
 * UNGETC
 * ============================================================
 * Kembalikan karakter ke stream.
 *
 * Parameter:
 *   c      - Karakter yang dikembalikan
 *   stream - Stream input
 *
 * Return: c jika berhasil, EOF jika gagal
 */
int ungetc(int c, FILE *stream) {
    /* Validasi parameter */
    if (stream == NULL) {
        errno = EBADF;
        return EOF;
    }

    /* EOF tidak bisa di-unget */
    if (c == EOF) {
        return EOF;
    }

    /* Cek apakah stream bisa dibaca */
    if (!(stream->flags & F_READ)) {
        return EOF;
    }

    /* Jika tidak ada buffer atau buffer penuh di posisi 0 */
    if (stream->buf == NULL || stream->buf_pos == 0) {
        /*
         * Untuk kasus sederhana, kita tidak mendukung ungetc
         * tanpa buffer. Return EOF.
         */
        return EOF;
    }

    /* Kembalikan karakter ke buffer */
    stream->buf_pos--;
    stream->buf[stream->buf_pos] = (unsigned char)c;

    /* Reset EOF flag */
    stream->flags &= ~F_EOF;

    return c;
}

/* ============================================================
 * GETW
 * ============================================================
 * Baca word (int) dari stream.
 *
 * Parameter:
 *   stream - Stream input
 *
 * Return: Nilai word, atau EOF jika error
 */
int getw(FILE *stream) {
    unsigned char buf[sizeof(int)];
    int i;
    int result;

    if (stream == NULL) {
        errno = EBADF;
        return EOF;
    }

    for (i = 0; i < (int)sizeof(int); i++) {
        int c = fgetc(stream);
        if (c == EOF) {
            return EOF;
        }
        buf[i] = (unsigned char)c;
    }

    /* Copy ke result (menghindari alignment issues) */
    result = 0;
    for (i = 0; i < (int)sizeof(int); i++) {
        result |= (buf[i] << (i * 8));
    }

    return result;
}
