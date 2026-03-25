/*
 * PIGURA LIBC - STDIO.H
 * ======================
 * Fungsi input/output standar sesuai standar C89/C90.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 */

#ifndef LIBC_STDIO_H
#define LIBC_STDIO_H

/* ============================================================
 * DEPENDENSI
 * ============================================================
 */
#include <stddef.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ============================================================
 * KONSTANTA
 * ============================================================
 */

/* EOF - End of File */
#define EOF (-1)

/* NULL pointer */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Batas ukuran buffer dan path */
#define BUFSIZ      1024    /* Ukuran buffer default */
#define FILENAME_MAX 4096   /* Panjang maksimum nama file */
#define FOPEN_MAX   16      /* Jumlah maksimum file terbuka */
#define TMP_MAX     26      /* Jumlah maksimum nama temp unik */
#define L_tmpnam    20      /* Panjang buffer untuk tmpnam */

/* Mode buffering */
#define _IOFBF  0   /* Full buffering */
#define _IOLBF  1   /* Line buffering */
#define _IONBF  2   /* No buffering */

/* Posisi seek */
#define SEEK_SET 0   /* Dari awal file */
#define SEEK_CUR 1   /* Dari posisi saat ini */
#define SEEK_END 2   /* Dari akhir file */

/* Flag error file */
#define F_ERR   0x01   /* Error flag */
#define F_EOF   0x02   /* EOF flag */
#define F_READ  0x04   /* Mode baca */
#define F_WRITE 0x08   /* Mode tulis */
#define F_BIN   0x10   /* Mode biner */
#define F_BUF   0x20   /* Buffer dialokasi sistem */
#define F_OPEN  0x40   /* File terbuka */

/* ============================================================
 * TIPE FILE
 * ============================================================
 */

/* Tipe untuk operasi file */
typedef struct {
    int fd;               /* File descriptor */
    int flags;            /* Status flags */
    unsigned char *buf;   /* Buffer */
    size_t buf_size;      /* Ukuran buffer */
    size_t buf_pos;       /* Posisi dalam buffer */
    size_t buf_len;       /* Data dalam buffer */
    int buf_mode;         /* Mode buffering */
    char mode[4];         /* Mode buka file */
} FILE;

/* Tipe untuk posisi file */
typedef long fpos_t;

/* ============================================================
 * STREAM STANDAR
 * ============================================================
 */
extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

/* ============================================================
 * FUNGSI OPERASI FILE
 * ============================================================
 */

/*
 * fopen - Buka file
 *
 * Parameter:
 *   filename - Nama file
 *   mode     - Mode buka ("r", "w", "a", "rb", "wb", "ab", etc)
 *
 * Return: Pointer ke FILE, atau NULL jika gagal
 */
extern FILE *fopen(const char *filename, const char *mode);

/*
 * freopen - Buka ulang file dengan stream yang ada
 *
 * Parameter:
 *   filename - Nama file (NULL untuk menutup)
 *   mode     - Mode buka
 *   stream   - Stream yang ada
 *
 * Return: Pointer ke stream, atau NULL jika gagal
 */
extern FILE *freopen(const char *filename, const char *mode,
                     FILE *stream);

/*
 * fclose - Tutup file
 *
 * Parameter:
 *   stream - Stream yang ditutup
 *
 * Return: 0 jika berhasil, EOF jika gagal
 */
extern int fclose(FILE *stream);

/*
 * fflush - Flush buffer ke file
 *
 * Parameter:
 *   stream - Stream yang di-flush (NULL untuk semua)
 *
 * Return: 0 jika berhasil, EOF jika gagal
 */
extern int fflush(FILE *stream);

/* ============================================================
 * FUNGSI BACA/TULIS BINER
 * ============================================================
 */

/*
 * fread - Baca blok data
 *
 * Parameter:
 *   ptr    - Buffer tujuan
 *   size   - Ukuran setiap elemen
 *   nmemb  - Jumlah elemen
 *   stream - Stream input
 *
 * Return: Jumlah elemen yang berhasil dibaca
 */
extern size_t fread(void *ptr, size_t size, size_t nmemb,
                    FILE *stream);

/*
 * fwrite - Tulis blok data
 *
 * Parameter:
 *   ptr    - Buffer sumber
 *   size   - Ukuran setiap elemen
 *   nmemb  - Jumlah elemen
 *   stream - Stream output
 *
 * Return: Jumlah elemen yang berhasil ditulis
 */
extern size_t fwrite(const void *ptr, size_t size, size_t nmemb,
                     FILE *stream);

/* ============================================================
 * FUNGSI BACA/TULIS KARAKTER
 * ============================================================
 */

/*
 * fgetc - Baca satu karakter
 *
 * Parameter:
 *   stream - Stream input
 *
 * Return: Karakter yang dibaca, atau EOF
 */
extern int fgetc(FILE *stream);

/*
 * getc - Baca satu karakter (macro version)
 */
#define getc(stream) fgetc(stream)

/*
 * getchar - Baca karakter dari stdin
 */
#define getchar() fgetc(stdin)

/*
 * fgets - Baca string
 *
 * Parameter:
 *   s      - Buffer tujuan
 *   n      - Ukuran buffer
 *   stream - Stream input
 *
 * Return: s jika berhasil, NULL jika EOF/error
 */
extern char *fgets(char *s, int n, FILE *stream);

/*
 * gets - Baca string dari stdin (DEPRECATED, UNSAFE)
 *
 * Parameter:
 *   s - Buffer tujuan
 *
 * Return: s jika berhasil, NULL jika error
 *
 * Peringatan: Fungsi ini tidak aman karena tidak membatasi
 * jumlah karakter. Gunakan fgets sebagai gantinya.
 */
extern char *gets(char *s);

/*
 * fputc - Tulis satu karakter
 *
 * Parameter:
 *   c      - Karakter yang ditulis
 *   stream - Stream output
 *
 * Return: Karakter yang ditulis, atau EOF
 */
extern int fputc(int c, FILE *stream);

/*
 * putc - Tulis satu karakter (macro version)
 */
#define putc(c, stream) fputc(c, stream)

/*
 * putchar - Tulis karakter ke stdout
 */
#define putchar(c) fputc(c, stdout)

/*
 * fputs - Tulis string
 *
 * Parameter:
 *   s      - String yang ditulis
 *   stream - Stream output
 *
 * Return: Non-negative jika berhasil, EOF jika gagal
 */
extern int fputs(const char *s, FILE *stream);

/*
 * puts - Tulis string ke stdout dengan newline
 *
 * Parameter:
 *   s - String yang ditulis
 *
 * Return: Non-negative jika berhasil, EOF jika gagal
 */
extern int puts(const char *s);

/*
 * ungetc - Kembalikan karakter ke stream
 *
 * Parameter:
 *   c      - Karakter yang dikembalikan
 *   stream - Stream input
 *
 * Return: c jika berhasil, EOF jika gagal
 */
extern int ungetc(int c, FILE *stream);

/* ============================================================
 * FUNGSI POSISI FILE
 * ============================================================
 */

/*
 * fseek - Set posisi file
 *
 * Parameter:
 *   stream - Stream yang dipindahkan
 *   offset - Offset dari origin
 *   whence - Origin (SEEK_SET, SEEK_CUR, SEEK_END)
 *
 * Return: 0 jika berhasil, -1 jika gagal
 */
extern int fseek(FILE *stream, long offset, int whence);

/*
 * ftell - Dapatkan posisi file
 *
 * Parameter:
 *   stream - Stream yang diperiksa
 *
 * Return: Posisi saat ini, atau -1L jika error
 */
extern long ftell(FILE *stream);

/*
 * rewind - Reset posisi ke awal file
 *
 * Parameter:
 *   stream - Stream yang di-reset
 */
extern void rewind(FILE *stream);

/*
 * fgetpos - Dapatkan posisi file
 *
 * Parameter:
 *   stream - Stream yang diperiksa
 *   pos    - Pointer untuk menyimpan posisi
 *
 * Return: 0 jika berhasil, non-zero jika gagal
 */
extern int fgetpos(FILE *stream, fpos_t *pos);

/*
 * fsetpos - Set posisi file
 *
 * Parameter:
 *   stream - Stream yang dipindahkan
 *   pos    - Pointer ke posisi
 *
 * Return: 0 jika berhasil, non-zero jika gagal
 */
extern int fsetpos(FILE *stream, const fpos_t *pos);

/*
 * clearerr - Reset error dan EOF flag
 *
 * Parameter:
 *   stream - Stream yang di-reset
 */
extern void clearerr(FILE *stream);

/*
 * feof - Cek apakah EOF tercapai
 *
 * Parameter:
 *   stream - Stream yang diperiksa
 *
 * Return: Non-zero jika EOF
 */
extern int feof(FILE *stream);

/*
 * ferror - Cek apakah error terjadi
 *
 * Parameter:
 *   stream - Stream yang diperiksa
 *
 * Return: Non-zero jika error
 */
extern int ferror(FILE *stream);

/*
 * perror - Cetak pesan error
 *
 * Parameter:
 *   s - Pesan tambahan (boleh NULL)
 */
extern void perror(const char *s);

/* ============================================================
 * FUNGSI FORMATTED I/O
 * ============================================================
 */

/*
 * printf - Cetak formatted ke stdout
 *
 * Parameter:
 *   format - Format string
 *   ...    - Argumen
 *
 * Return: Jumlah karakter yang dicetak
 */
extern int printf(const char *format, ...);

/*
 * fprintf - Cetak formatted ke file
 *
 * Parameter:
 *   stream - Stream output
 *   format - Format string
 *   ...    - Argumen
 *
 * Return: Jumlah karakter yang dicetak
 */
extern int fprintf(FILE *stream, const char *format, ...);

/*
 * sprintf - Cetak formatted ke string
 *
 * Parameter:
 *   str    - Buffer tujuan
 *   format - Format string
 *   ...    - Argumen
 *
 * Return: Jumlah karakter yang dicetak
 *
 * Peringatan: Tidak aman jika buffer overflow.
 * Gunakan snprintf sebagai gantinya.
 */
extern int sprintf(char *str, const char *format, ...);

/*
 * snprintf - Cetak formatted ke string dengan batas
 *
 * Parameter:
 *   str    - Buffer tujuan
 *   size   - Ukuran buffer
 *   format - Format string
 *   ...    - Argumen
 *
 * Return: Jumlah karakter yang akan ditulis (tanpa null)
 */
extern int snprintf(char *str, size_t size, const char *format, ...);

/*
 * vprintf - Cetak formatted ke stdout (va_list)
 *
 * Parameter:
 *   format - Format string
 *   ap     - va_list argumen
 *
 * Return: Jumlah karakter yang dicetak
 */
extern int vprintf(const char *format, va_list ap);

/*
 * vfprintf - Cetak formatted ke file (va_list)
 *
 * Parameter:
 *   stream - Stream output
 *   format - Format string
 *   ap     - va_list argumen
 *
 * Return: Jumlah karakter yang dicetak
 */
extern int vfprintf(FILE *stream, const char *format, va_list ap);

/*
 * vsprintf - Cetak formatted ke string (va_list)
 *
 * Parameter:
 *   str    - Buffer tujuan
 *   format - Format string
 *   ap     - va_list argumen
 *
 * Return: Jumlah karakter yang dicetak
 */
extern int vsprintf(char *str, const char *format, va_list ap);

/*
 * vsnprintf - Cetak formatted ke string dengan batas (va_list)
 *
 * Parameter:
 *   str    - Buffer tujuan
 *   size   - Ukuran buffer
 *   format - Format string
 *   ap     - va_list argumen
 *
 * Return: Jumlah karakter yang akan ditulis
 */
extern int vsnprintf(char *str, size_t size, const char *format,
                     va_list ap);

/*
 * scanf - Baca formatted dari stdin
 *
 * Parameter:
 *   format - Format string
 *   ...    - Pointer ke variabel tujuan
 *
 * Return: Jumlah item yang berhasil dibaca
 */
extern int scanf(const char *format, ...);

/*
 * fscanf - Baca formatted dari file
 *
 * Parameter:
 *   stream - Stream input
 *   format - Format string
 *   ...    - Pointer ke variabel tujuan
 *
 * Return: Jumlah item yang berhasil dibaca
 */
extern int fscanf(FILE *stream, const char *format, ...);

/*
 * sscanf - Baca formatted dari string
 *
 * Parameter:
 *   str    - String sumber
 *   format - Format string
 *   ...    - Pointer ke variabel tujuan
 *
 * Return: Jumlah item yang berhasil dibaca
 */
extern int sscanf(const char *str, const char *format, ...);

/* ============================================================
 * FUNGSI MANAJEMEN FILE
 * ============================================================
 */

/*
 * remove - Hapus file
 *
 * Parameter:
 *   filename - Nama file yang dihapus
 *
 * Return: 0 jika berhasil, non-zero jika gagal
 */
extern int remove(const char *filename);

/*
 * rename - Ubah nama file
 *
 * Parameter:
 *   old - Nama lama
 *   new - Nama baru
 *
 * Return: 0 jika berhasil, non-zero jika gagal
 */
extern int rename(const char *old, const char *new);

/*
 * tmpfile - Buat file temporary
 *
 * Return: Pointer ke FILE, atau NULL jika gagal
 */
extern FILE *tmpfile(void);

/*
 * tmpnam - Buat nama file temporary
 *
 * Parameter:
 *   s - Buffer untuk nama (boleh NULL)
 *
 * Return: Nama file temporary
 */
extern char *tmpnam(char *s);

/*
 * setbuf - Set buffer untuk stream
 *
 * Parameter:
 *   stream - Stream yang diatur
 *   buf    - Buffer (NULL untuk unbuffered)
 */
extern void setbuf(FILE *stream, char *buf);

/*
 * setvbuf - Set buffer dan mode untuk stream
 *
 * Parameter:
 *   stream  - Stream yang diatur
 *   buf     - Buffer (NULL untuk auto-allocation)
 *   mode    - Mode buffering (_IOFBF, _IOLBF, _IONBF)
 *   size    - Ukuran buffer
 *
 * Return: 0 jika berhasil, non-zero jika gagal
 */
extern int setvbuf(FILE *stream, char *buf, int mode, size_t size);

/* ============================================================
 * FUNGSI MANAJEMEN BUFFER INTERNAL
 * ============================================================
 */

/*
 * __stdio_init - Inisialisasi stdio
 * Dipanggil saat startup program.
 */
extern void __stdio_init(void);

/*
 * __stdio_cleanup - Bersihkan stdio
 * Dipanggil saat program berakhir.
 */
extern void __stdio_cleanup(void);

/* ============================================================
 * KONSTANTA TAMBAHAN
 * ============================================================
 */

/* Format specifier panjang maksimum */
#define PRINTF_MAX_FORMAT  4096
#define SCANF_MAX_FORMAT   4096

/* Panjang maksimum output angka */
#define PRINTF_MAX_INT     32
#define PRINTF_MAX_FLOAT   64

/* ============================================================
 * VARIABEL GLOBAL - STREAM STANDAR
 * ============================================================
 */

/* Struktur internal untuk stream standar */
static FILE _stdin_internal;
static FILE _stdout_internal;
static FILE _stderr_internal;

/* Pointer publik ke stream standar */
FILE *stdin = &_stdin_internal;
FILE *stdout = &_stdout_internal;
FILE *stderr = &_stderr_internal;

/* Buffer untuk stream standar */
static unsigned char _stdin_buffer[BUFSIZ];
static unsigned char _stdout_buffer[BUFSIZ];
static unsigned char _stderr_buffer[BUFSIZ];

/* ============================================================
 * POOL FILE
 * ============================================================
 */

/* Pool FILE untuk alokasi dinamis */
FILE _file_pool[FOPEN_MAX];
int _file_pool_used[FOPEN_MAX];
int _file_pool_initialized = 0;

/* ============================================================
 * FLAG INISIALISASI
 * ============================================================
 */

static int _stdio_initialized = 0;

/* ============================================================
 * _INIT_FILE_POOL (Internal)
 * ============================================================
 * Inisialisasi pool FILE.
 */
static void _init_file_pool(void) {
    int i;

    if (_file_pool_initialized) {
        return;
    }

    /* Reset semua slot */
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

/* ============================================================
 * __STDIO_INIT
 * ============================================================
 * Inisialisasi stdio. Dipanggil saat startup program.
 *
 * Fungsi ini harus dipanggil sebelum menggunakan fungsi
 * stdio lainnya. Biasanya dipanggil dari _start atau
 * crt0.
 */
void __stdio_init(void) {
    /* Cegah inisialisasi ganda */
    if (_stdio_initialized) {
        return;
    }

    /* Inisialisasi stdin (file descriptor 0) */
    _stdin_internal.fd = STDIN_FILENO;
    _stdin_internal.flags = F_READ | F_OPEN;
    _stdin_internal.buf = _stdin_buffer;
    _stdin_internal.buf_size = BUFSIZ;
    _stdin_internal.buf_pos = 0;
    _stdin_internal.buf_len = 0;
    _stdin_internal.buf_mode = _IOLBF;  /* Line buffering */
    strcpy(_stdin_internal.mode, "r");

    /* Inisialisasi stdout (file descriptor 1) */
    _stdout_internal.fd = STDOUT_FILENO;
    _stdout_internal.flags = F_WRITE | F_OPEN;
    _stdout_internal.buf = _stdout_buffer;
    _stdout_internal.buf_size = BUFSIZ;
    _stdout_internal.buf_pos = 0;
    _stdout_internal.buf_len = 0;
    _stdout_internal.buf_mode = _IOLBF;  /* Line buffering */
    strcpy(_stdout_internal.mode, "w");

    /* Inisialisasi stderr (file descriptor 2) */
    _stderr_internal.fd = STDERR_FILENO;
    _stderr_internal.flags = F_WRITE | F_OPEN;
    _stderr_internal.buf = _stderr_buffer;
    _stderr_internal.buf_size = BUFSIZ;
    _stderr_internal.buf_pos = 0;
    _stderr_internal.buf_len = 0;
    _stderr_internal.buf_mode = _IONBF;  /* Unbuffered */
    strcpy(_stderr_internal.mode, "w");

    /* Inisialisasi pool FILE */
    _init_file_pool();

    _stdio_initialized = 1;
}

/* ============================================================
 * __STDIO_CLEANUP
 * ============================================================
 * Bersihkan stdio. Dipanggil saat program berakhir.
 *
 * Fungsi ini akan:
 * 1. Flush semua stream output
 * 2. Menutup semua file yang masih terbuka
 * 3. Membebaskan buffer yang dialokasi
 */
void __stdio_cleanup(void) {
    int i;

    /* Cek apakah sudah diinisialisasi */
    if (!_stdio_initialized) {
        return;
    }

    /* Flush dan tutup semua stream dalam pool */
    for (i = 0; i < FOPEN_MAX; i++) {
        if (_file_pool_used[i] && _file_pool[i].fd >= 0) {
            /* Flush jika ada data */
            if ((_file_pool[i].flags & F_WRITE) &&
                _file_pool[i].buf_pos > 0) {
                fflush(&_file_pool[i]);
            }

            /* Tutup file */
            close(_file_pool[i].fd);

            /* Bebaskan buffer jika dialokasi sistem */
            if (_file_pool[i].buf != NULL &&
                (_file_pool[i].flags & F_BUF)) {
                free(_file_pool[i].buf);
            }

            _file_pool_used[i] = 0;
        }
    }

    /* Flush stdout dan stderr */
    if (stdout != NULL && (stdout->flags & F_WRITE) &&
        stdout->buf_pos > 0) {
        fflush(stdout);
    }

    if (stderr != NULL && (stderr->flags & F_WRITE) &&
        stderr->buf_pos > 0) {
        fflush(stderr);
    }

    _stdio_initialized = 0;
}

/* ============================================================
 * __GET_FREE_FILE (Internal)
 * ============================================================
 * Dapatkan slot FILE yang kosong dari pool.
 *
 * Return: Pointer ke FILE, atau NULL jika penuh
 */
FILE *__get_free_file(void) {
    int i;

    _init_file_pool();

    for (i = 0; i < FOPEN_MAX; i++) {
        if (!_file_pool_used[i]) {
            _file_pool_used[i] = 1;
            return &_file_pool[i];
        }
    }

    return NULL;
}

/* ============================================================
 * __RELEASE_FILE (Internal)
 * ============================================================
 * Kembalikan slot FILE ke pool.
 *
 * Parameter:
 *   fp - FILE yang dibebaskan
 */
void __release_file(FILE *fp) {
    int i;

    if (fp == NULL) {
        return;
    }

    /* Cari di pool */
    for (i = 0; i < FOPEN_MAX; i++) {
        if (&_file_pool[i] == fp) {
            /* Bebaskan buffer jika dialokasi sistem */
            if (fp->buf != NULL && (fp->flags & F_BUF)) {
                free(fp->buf);
            }

            /* Reset struktur */
            fp->fd = -1;
            fp->flags = 0;
            fp->buf = NULL;
            fp->buf_size = 0;
            fp->buf_pos = 0;
            fp->buf_len = 0;

            _file_pool_used[i] = 0;
            break;
        }
    }
}

/* ============================================================
 * FDOPEN (POSIX)
 * ============================================================
 * Buka stream dari file descriptor yang ada.
 *
 * Parameter:
 *   fd   - File descriptor
 *   mode - Mode stream
 *
 * Return: Pointer ke FILE, atau NULL jika gagal
 */
FILE *fdopen(int fd, const char *mode) {
    FILE *fp;
    int flags;

    /* Validasi parameter */
    if (fd < 0 || mode == NULL || *mode == '\0') {
        return NULL;
    }

    /* Parse mode */
    flags = 0;
    switch (mode[0]) {
        case 'r':
            flags = F_READ;
            break;
        case 'w':
            flags = F_WRITE;
            break;
        case 'a':
            flags = F_WRITE;
            break;
        default:
            return NULL;
    }

    /* Cek mode '+' */
    if (strchr(mode, '+') != NULL) {
        flags |= F_READ | F_WRITE;
    }

    /* Cek mode biner */
    if (strchr(mode, 'b') != NULL) {
        flags |= F_BIN;
    }

    /* Alokasikan FILE dari pool */
    fp = __get_free_file();
    if (fp == NULL) {
        return NULL;
    }

    /* Inisialisasi */
    fp->fd = fd;
    fp->flags = flags | F_OPEN;
    fp->buf_pos = 0;
    fp->buf_len = 0;
    strncpy(fp->mode, mode, sizeof(fp->mode) - 1);
    fp->mode[sizeof(fp->mode) - 1] = '\0';

    /* Alokasikan buffer */
    fp->buf = (unsigned char *)malloc(BUFSIZ);
    if (fp->buf != NULL) {
        fp->buf_size = BUFSIZ;
        fp->flags |= F_BUF;
        fp->buf_mode = _IOFBF;
    } else {
        fp->buf_mode = _IONBF;
    }

    return fp;
}

/* ============================================================
 * POPEN (POSIX)
 * ============================================================
 * Buka pipe ke proses.
 *
 * Parameter:
 *   command - Perintah yang dijalankan
 *   type    - "r" untuk baca, "w" untuk tulis
 *
 * Return: Pointer ke FILE, atau NULL jika gagal
 */
FILE *popen(const char *command, const char *type) {
    int pipefd[2];
    pid_t pid;
    FILE *fp;

    /* Validasi parameter */
    if (command == NULL || type == NULL) {
        return NULL;
    }

    if (type[0] != 'r' && type[0] != 'w') {
        return NULL;
    }

    /* Buat pipe */
    if (pipe(pipefd) < 0) {
        return NULL;
    }

    /* Fork proses */
    pid = fork();

    if (pid < 0) {
        /* Fork gagal */
        close(pipefd[0]);
        close(pipefd[1]);
        return NULL;
    }

    if (pid == 0) {
        /* Child process */
        if (type[0] == 'r') {
            /* Parent membaca, child menulis */
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);
        } else {
            /* Parent menulis, child membaca */
            close(pipefd[1]);
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);
        }

        /* Jalankan command */
        execl("/bin/sh", "sh", "-c", command, (char *)NULL);
        _exit(127);
    }

    /* Parent process */
    if (type[0] == 'r') {
        close(pipefd[1]);
        fp = fdopen(pipefd[0], "r");
    } else {
        close(pipefd[0]);
        fp = fdopen(pipefd[1], "w");
    }

    if (fp == NULL) {
        if (type[0] == 'r') {
            close(pipefd[0]);
        } else {
            close(pipefd[1]);
        }
        return NULL;
    }

    /* Simpan PID untuk pclose */
    /* Untuk implementasi sederhana, kita simpan di fd yang tidak digunakan */
    /* Implementasi lengkap perlu struktur data tambahan */

    return fp;
}

/* ============================================================
 * PCLOSE (POSIX)
 * ============================================================
 * Tutup pipe dan tunggu proses selesai.
 *
 * Parameter:
 *   stream - Stream yang ditutup
 *
 * Return: Exit status, atau -1 jika error
 */
int pclose(FILE *stream) {
    int status;
    int fd;

    if (stream == NULL) {
        return -1;
    }

    fd = stream->fd;
    fclose(stream);

    /* Tunggu child process */
    wait(&status);

    return status;
}

/* ============================================================
 * FORWARD DECLARATIONS
 * ============================================================
 */
extern pid_t fork(void);
extern pid_t wait(int *status);
extern int execl(const char *path, const char *arg, ...);
extern void _exit(int status);
extern int dup2(int oldfd, int newfd);
extern int pipe(int pipefd[2]);

#endif /* LIBC_STDIO_H */
