/*
 * PIGURA LIBC - STDIO/SEEK.C
 * ============================
 * Implementasi fungsi fseek, ftell, rewind, fgetpos, fsetpos.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>

/* ============================================================
 * FSEEK
 * ============================================================
 * Set posisi file.
 *
 * Parameter:
 *   stream - Stream yang dipindahkan
 *   offset - Offset dari origin
 *   whence - Origin (SEEK_SET, SEEK_CUR, SEEK_END)
 *
 * Return: 0 jika berhasil, -1 jika gagal
 */
int fseek(FILE *stream, long offset, int whence) {
    long new_pos;

    /* Validasi parameter */
    if (stream == NULL) {
        errno = EBADF;
        return -1;
    }

    /* Cek apakah file terbuka */
    if (!(stream->flags & F_OPEN)) {
        errno = EBADF;
        return -1;
    }

    /* Validasi whence */
    if (whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END) {
        errno = EINVAL;
        return -1;
    }

    /* Flush buffer jika ada data yang belum ditulis */
    if (stream->flags & F_WRITE) {
        if (fflush(stream) != 0) {
            return -1;
        }
    }

    /* Jika ada buffer baca, invalidate */
    if (stream->flags & F_READ) {
        stream->buf_pos = 0;
        stream->buf_len = 0;
    }

    /* Lakukan seek pada file descriptor */
    new_pos = lseek(stream->fd, offset, whence);

    if (new_pos < 0) {
        stream->flags |= F_ERR;
        return -1;
    }

    /* Reset flags */
    stream->flags &= ~(F_EOF | F_ERR);

    return 0;
}

/* ============================================================
 * FTELL
 * ============================================================
 * Dapatkan posisi file saat ini.
 *
 * Parameter:
 *   stream - Stream yang diperiksa
 *
 * Return: Posisi saat ini, atau -1L jika error
 */
long ftell(FILE *stream) {
    long pos;

    /* Validasi parameter */
    if (stream == NULL) {
        errno = EBADF;
        return -1L;
    }

    /* Cek apakah file terbuka */
    if (!(stream->flags & F_OPEN)) {
        errno = EBADF;
        return -1L;
    }

    /* Dapatkan posisi aktual dari sistem */
    pos = lseek(stream->fd, 0, SEEK_CUR);

    if (pos < 0) {
        stream->flags |= F_ERR;
        return -1L;
    }

    /* Sesuaikan dengan buffer */
    if (stream->flags & F_READ) {
        /* Buffer baca: kurangi dengan data yang belum dibaca */
        pos -= (long)(stream->buf_len - stream->buf_pos);
    } else if (stream->flags & F_WRITE) {
        /* Buffer tulis: tambahkan data yang belum di-flush */
        pos += (long)stream->buf_pos;
    }

    return pos;
}

/* ============================================================
 * REWIND
 * ============================================================
 * Reset posisi ke awal file.
 *
 * Parameter:
 *   stream - Stream yang di-reset
 */
void rewind(FILE *stream) {
    /* Validasi parameter */
    if (stream == NULL) {
        return;
    }

    /* Cek apakah file terbuka */
    if (!(stream->flags & F_OPEN)) {
        return;
    }

    /* Flush buffer jika ada */
    if (stream->flags & F_WRITE) {
        fflush(stream);
    }

    /* Reset buffer */
    stream->buf_pos = 0;
    stream->buf_len = 0;

    /* Seek ke awal */
    lseek(stream->fd, 0, SEEK_SET);

    /* Reset flags */
    stream->flags &= ~(F_EOF | F_ERR);
}

/* ============================================================
 * FGETPOS
 * ============================================================
 * Dapatkan posisi file.
 *
 * Parameter:
 *   stream - Stream yang diperiksa
 *   pos    - Pointer untuk menyimpan posisi
 *
 * Return: 0 jika berhasil, non-zero jika gagal
 */
int fgetpos(FILE *stream, fpos_t *pos) {
    long current_pos;

    /* Validasi parameter */
    if (stream == NULL || pos == NULL) {
        errno = EINVAL;
        return -1;
    }

    current_pos = ftell(stream);

    if (current_pos < 0) {
        return -1;
    }

    *pos = (fpos_t)current_pos;

    return 0;
}

/* ============================================================
 * FSETPOS
 * ============================================================
 * Set posisi file.
 *
 * Parameter:
 *   stream - Stream yang dipindahkan
 *   pos    - Pointer ke posisi
 *
 * Return: 0 jika berhasil, non-zero jika gagal
 */
int fsetpos(FILE *stream, const fpos_t *pos) {
    /* Validasi parameter */
    if (stream == NULL || pos == NULL) {
        errno = EINVAL;
        return -1;
    }

    return fseek(stream, (long)*pos, SEEK_SET);
}

/* ============================================================
 * FSEEKO
 * ============================================================
 * Set posisi file dengan tipe off_t (POSIX).
 *
 * Parameter:
 *   stream - Stream yang dipindahkan
 *   offset - Offset dari origin
 *   whence - Origin
 *
 * Return: 0 jika berhasil, -1 jika gagal
 */
int fseeko(FILE *stream, long long offset, int whence) {
    long new_pos;

    /* Validasi parameter */
    if (stream == NULL) {
        errno = EBADF;
        return -1;
    }

    if (!(stream->flags & F_OPEN)) {
        errno = EBADF;
        return -1;
    }

    if (whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END) {
        errno = EINVAL;
        return -1;
    }

    /* Flush buffer jika ada */
    if (stream->flags & F_WRITE) {
        if (fflush(stream) != 0) {
            return -1;
        }
    }

    /* Invalidate buffer baca */
    if (stream->flags & F_READ) {
        stream->buf_pos = 0;
        stream->buf_len = 0;
    }

    /* Lakukan seek */
    new_pos = lseek(stream->fd, offset, whence);

    if (new_pos < 0) {
        stream->flags |= F_ERR;
        return -1;
    }

    stream->flags &= ~(F_EOF | F_ERR);

    return 0;
}

/* ============================================================
 * FTELLO
 * ============================================================
 * Dapatkan posisi file dengan tipe off_t (POSIX).
 *
 * Parameter:
 *   stream - Stream yang diperiksa
 *
 * Return: Posisi saat ini, atau -1 jika error
 */
long long ftello(FILE *stream) {
    long long pos;

    if (stream == NULL) {
        errno = EBADF;
        return -1;
    }

    if (!(stream->flags & F_OPEN)) {
        errno = EBADF;
        return -1;
    }

    pos = lseek(stream->fd, 0, SEEK_CUR);

    if (pos < 0) {
        stream->flags |= F_ERR;
        return -1;
    }

    /* Sesuaikan dengan buffer */
    if (stream->flags & F_READ) {
        pos -= (long long)(stream->buf_len - stream->buf_pos);
    } else if (stream->flags & F_WRITE) {
        pos += (long long)stream->buf_pos;
    }

    return pos;
}
