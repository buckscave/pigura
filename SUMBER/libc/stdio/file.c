/*
 * PIGURA LIBC - STDIO/FILE.C
 * ============================
 * Implementasi fungsi operasi file: fopen, fclose, freopen,
 * fread, fwrite, fileno.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

/* ============================================================
 * VARIABEL GLOBAL
 * ============================================================
 */

/* Stream standar */
static FILE _stdin;
static FILE _stdout;
static FILE _stderr;

FILE *stdin = &_stdin;
FILE *stdout = &_stdout;
FILE *stderr = &_stderr;

/* Pool FILE untuk alokasi (visible ke flush.c untuk fflush NULL) */
FILE _file_pool[FOPEN_MAX];
int _file_pool_used[FOPEN_MAX];
int _file_pool_initialized = 0;

/* Buffer untuk stdin/stdout/stderr */
static unsigned char _stdin_buf[BUFSIZ];
static unsigned char _stdout_buf[BUFSIZ];
static unsigned char _stderr_buf[BUFSIZ];

/* ============================================================
 * FUNGSI INTERNAL
 * ============================================================
 */

/* Inisialisasi pool FILE */
static void _init_file_pool(void) {
    int i;

    if (_file_pool_initialized) {
        return;
    }

    for (i = 0; i < FOPEN_MAX; i++) {
        _file_pool[i].fd = -1;
        _file_pool[i].flags = 0;
        _file_pool[i].buf = NULL;
        _file_pool[i].buf_size = 0;
        _file_pool[i].buf_pos = 0;
        _file_pool[i].buf_len = 0;
        _file_pool[i].buf_mode = _IOFBF;
        _file_pool[i].mode[0] = '\0';
        _file_pool_used[i] = 0;
    }

    _file_pool_initialized = 1;
}

/* Alokasikan FILE dari pool */
static FILE *_alloc_file(void) {
    int i;

    _init_file_pool();

    for (i = 0; i < FOPEN_MAX; i++) {
        if (!_file_pool_used[i]) {
            _file_pool_used[i] = 1;
            return &_file_pool[i];
        }
    }

    /* Tidak ada slot tersedia */
    errno = EMFILE;
    return NULL;
}

/* Bebaskan FILE ke pool */
static void _free_file(FILE *fp) {
    int i;

    if (fp == NULL) {
        return;
    }

    for (i = 0; i < FOPEN_MAX; i++) {
        if (&_file_pool[i] == fp) {
            _file_pool_used[i] = 0;
            break;
        }
    }

    /* Bebaskan buffer jika dialokasi sistem */
    if (fp->buf != NULL && (fp->flags & F_BUF)) {
        free(fp->buf);
        fp->buf = NULL;
    }

    fp->fd = -1;
    fp->flags = 0;
    fp->buf_size = 0;
    fp->buf_pos = 0;
    fp->buf_len = 0;
}

/* Parse mode string ke flags */
static int _parse_mode(const char *mode, int *oflags, int *flags) {
    const char *p;
    int binary;
    int plus;

    if (mode == NULL || mode[0] == '\0') {
        errno = EINVAL;
        return -1;
    }

    *oflags = 0;
    *flags = 0;
    binary = 0;
    plus = 0;

    /* Parse karakter pertama */
    switch (mode[0]) {
        case 'r':
            *oflags = O_RDONLY;
            *flags = F_READ;
            break;
        case 'w':
            *oflags = O_WRONLY | O_CREAT | O_TRUNC;
            *flags = F_WRITE;
            break;
        case 'a':
            *oflags = O_WRONLY | O_CREAT | O_APPEND;
            *flags = F_WRITE;
            break;
        default:
            errno = EINVAL;
            return -1;
    }

    /* Parse karakter tambahan */
    for (p = mode + 1; *p != '\0'; p++) {
        switch (*p) {
            case 'b':
                binary = 1;
                *flags |= F_BIN;
                break;
            case '+':
                plus = 1;
                *flags |= F_READ | F_WRITE;
                /* Update oflags */
                if (mode[0] == 'r') {
                    *oflags = O_RDWR;
                } else {
                    *oflags = O_RDWR | O_CREAT;
                    if (mode[0] == 'w') {
                        *oflags |= O_TRUNC;
                    } else {
                        *oflags |= O_APPEND;
                    }
                }
                break;
            default:
                /* Abaikan karakter tidak dikenal */
                break;
        }
    }

    return 0;
}

/* ============================================================
 * FOPEN
 * ============================================================
 * Buka file dan buat stream.
 *
 * Parameter:
 *   filename - Nama file
 *   mode     - Mode pembukaan ("r", "w", "a", "rb", "wb", dll)
 *
 * Return: Pointer ke FILE, atau NULL jika gagal
 */
FILE *fopen(const char *filename, const char *mode) {
    FILE *fp;
    int oflags;
    int flags;
    int fd;

    /* Validasi parameter */
    if (filename == NULL || mode == NULL) {
        errno = EINVAL;
        return NULL;
    }

    /* Parse mode */
    if (_parse_mode(mode, &oflags, &flags) < 0) {
        return NULL;
    }

    /* Buka file */
    fd = open(filename, oflags, 0666);

    if (fd < 0) {
        return NULL;
    }

    /* Alokasikan FILE */
    fp = _alloc_file();
    if (fp == NULL) {
        close(fd);
        return NULL;
    }

    /* Inisialisasi FILE */
    fp->fd = fd;
    fp->flags = flags | F_OPEN;

    /* Salin mode */
    strncpy(fp->mode, mode, sizeof(fp->mode) - 1);
    fp->mode[sizeof(fp->mode) - 1] = '\0';

    /* Alokasikan buffer default */
    fp->buf = (unsigned char *)malloc(BUFSIZ);
    if (fp->buf != NULL) {
        fp->buf_size = BUFSIZ;
        fp->flags |= F_BUF;
        fp->buf_mode = _IOFBF;
    } else {
        /* Fallback ke unbuffered */
        fp->buf = NULL;
        fp->buf_size = 0;
        fp->buf_mode = _IONBF;
    }

    fp->buf_pos = 0;
    fp->buf_len = 0;

    return fp;
}

/* ============================================================
 * FREOPEN
 * ============================================================
 * Buka ulang file dengan stream yang ada.
 *
 * Parameter:
 *   filename - Nama file (NULL untuk menutup)
 *   mode     - Mode pembukaan
 *   stream   - Stream yang ada
 *
 * Return: Pointer ke stream, atau NULL jika gagal
 */
FILE *freopen(const char *filename, const char *mode, FILE *stream) {
    int oflags;
    int flags;
    int fd;

    /* Validasi parameter */
    if (stream == NULL) {
        errno = EINVAL;
        return NULL;
    }

    /* Tutup file yang ada jika terbuka */
    if (stream->flags & F_OPEN) {
        /* Flush buffer */
        fflush(stream);

        /* Tutup file descriptor */
        if (stream->fd >= 0) {
            close(stream->fd);
            stream->fd = -1;
        }

        stream->flags &= ~F_OPEN;
    }

    /* Jika filename NULL, hanya tutup */
    if (filename == NULL) {
        _free_file(stream);
        return NULL;
    }

    /* Validasi mode */
    if (mode == NULL) {
        errno = EINVAL;
        return NULL;
    }

    /* Parse mode */
    if (_parse_mode(mode, &oflags, &flags) < 0) {
        _free_file(stream);
        return NULL;
    }

    /* Buka file baru */
    fd = open(filename, oflags, 0666);

    if (fd < 0) {
        _free_file(stream);
        return NULL;
    }

    /* Reset state stream */
    stream->fd = fd;
    stream->flags = flags | F_OPEN;
    stream->buf_pos = 0;
    stream->buf_len = 0;

    /* Salin mode */
    strncpy(stream->mode, mode, sizeof(stream->mode) - 1);
    stream->mode[sizeof(stream->mode) - 1] = '\0';

    /* Buat buffer jika belum ada */
    if (stream->buf == NULL) {
        stream->buf = (unsigned char *)malloc(BUFSIZ);
        if (stream->buf != NULL) {
            stream->buf_size = BUFSIZ;
            stream->flags |= F_BUF;
            stream->buf_mode = _IOFBF;
        } else {
            stream->buf_mode = _IONBF;
        }
    }

    return stream;
}

/* ============================================================
 * FCLOSE
 * ============================================================
 * Tutup stream file.
 *
 * Parameter:
 *   stream - Stream yang ditutup
 *
 * Return: 0 jika berhasil, EOF jika gagal
 */
int fclose(FILE *stream) {
    int ret = 0;

    /* Validasi parameter */
    if (stream == NULL) {
        errno = EBADF;
        return EOF;
    }

    /* Cek apakah file terbuka */
    if (!(stream->flags & F_OPEN)) {
        errno = EBADF;
        return EOF;
    }

    /* Flush buffer jika mode tulis */
    if (stream->flags & F_WRITE) {
        if (fflush(stream) != 0) {
            ret = EOF;
        }
    }

    /* Tutup file descriptor */
    if (stream->fd >= 0) {
        if (close(stream->fd) < 0) {
            ret = EOF;
        }
        stream->fd = -1;
    }

    /* Bebaskan FILE */
    _free_file(stream);

    return ret;
}

/* ============================================================
 * FREAD
 * ============================================================
 * Baca blok data dari stream.
 *
 * Parameter:
 *   ptr    - Buffer tujuan
 *   size   - Ukuran setiap elemen
 *   nmemb  - Jumlah elemen
 *   stream - Stream input
 *
 * Return: Jumlah elemen yang berhasil dibaca
 */
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    unsigned char *buf = (unsigned char *)ptr;
    size_t total;
    size_t count;
    int c;

    /* Validasi parameter */
    if (ptr == NULL || stream == NULL) {
        return 0;
    }

    if (size == 0 || nmemb == 0) {
        return 0;
    }

    /* Cek apakah stream bisa dibaca */
    if (!(stream->flags & F_READ)) {
        stream->flags |= F_ERR;
        return 0;
    }

    /* Cek EOF */
    if (stream->flags & F_EOF) {
        return 0;
    }

    total = size * nmemb;
    count = 0;

    /* Baca data */
    while (count < total) {
        c = fgetc(stream);
        if (c == EOF) {
            break;
        }
        buf[count++] = (unsigned char)c;
    }

    return count / size;
}

/* ============================================================
 * FWRITE
 * ============================================================
 * Tulis blok data ke stream.
 *
 * Parameter:
 *   ptr    - Buffer sumber
 *   size   - Ukuran setiap elemen
 *   nmemb  - Jumlah elemen
 *   stream - Stream output
 *
 * Return: Jumlah elemen yang berhasil ditulis
 */
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
    const unsigned char *buf = (const unsigned char *)ptr;
    size_t total;
    size_t count;

    /* Validasi parameter */
    if (ptr == NULL || stream == NULL) {
        return 0;
    }

    if (size == 0 || nmemb == 0) {
        return 0;
    }

    /* Cek apakah stream bisa ditulis */
    if (!(stream->flags & F_WRITE)) {
        stream->flags |= F_ERR;
        return 0;
    }

    total = size * nmemb;
    count = 0;

    /* Tulis data */
    while (count < total) {
        if (fputc(buf[count], stream) == EOF) {
            break;
        }
        count++;
    }

    return count / size;
}

/* ============================================================
 * FILENO
 * ============================================================
 * Dapatkan file descriptor dari stream.
 *
 * Parameter:
 *   stream - Stream yang diperiksa
 *
 * Return: File descriptor, atau -1 jika error
 */
int fileno(FILE *stream) {
    if (stream == NULL) {
        errno = EBADF;
        return -1;
    }

    return stream->fd;
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
 * __STDIO_INIT
 * ============================================================
 * Inisialisasi stdio. Dipanggil saat startup program.
 */
void __stdio_init(void) {
    /* Inisialisasi stdin */
    _stdin.fd = STDIN_FILENO;
    _stdin.flags = F_READ | F_OPEN;
    _stdin.buf = _stdin_buf;
    _stdin.buf_size = BUFSIZ;
    _stdin.buf_pos = 0;
    _stdin.buf_len = 0;
    _stdin.buf_mode = _IOLBF;
    strncpy(_stdin.mode, "r", sizeof(_stdin.mode) - 1); _stdin.mode[sizeof(_stdin.mode) - 1] = '\0';

    /* Inisialisasi stdout */
    _stdout.fd = STDOUT_FILENO;
    _stdout.flags = F_WRITE | F_OPEN;
    _stdout.buf = _stdout_buf;
    _stdout.buf_size = BUFSIZ;
    _stdout.buf_pos = 0;
    _stdout.buf_len = 0;
    _stdout.buf_mode = _IOLBF;
    strncpy(_stdout.mode, "w", sizeof(_stdout.mode) - 1); _stdout.mode[sizeof(_stdout.mode) - 1] = '\0';

    /* Inisialisasi stderr */
    _stderr.fd = STDERR_FILENO;
    _stderr.flags = F_WRITE | F_OPEN;
    _stderr.buf = _stderr_buf;
    _stderr.buf_size = BUFSIZ;
    _stderr.buf_pos = 0;
    _stderr.buf_len = 0;
    _stderr.buf_mode = _IONBF;  /* stderr unbuffered */
    strncpy(_stderr.mode, "w", sizeof(_stderr.mode) - 1); _stderr.mode[sizeof(_stderr.mode) - 1] = '\0';

    /* Init pool */
    _init_file_pool();
}

/* ============================================================
 * __STDIO_CLEANUP
 * ============================================================
 * Bersihkan stdio. Dipanggil saat program berakhir.
 */
void __stdio_cleanup(void) {
    int i;

    /* Flush semua stream yang terbuka */
    fflush(NULL);

    /* Tutup semua file dalam pool */
    for (i = 0; i < FOPEN_MAX; i++) {
        if (_file_pool_used[i] && _file_pool[i].fd >= 0) {
            fclose(&_file_pool[i]);
        }
    }
}
