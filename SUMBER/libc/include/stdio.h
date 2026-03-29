/*
 * PIGURA LIBC - STDIO.H
 * ======================
 * Fungsi input/output standar sesuai standar C89/C90.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.1 - Perbaikan circular dependency dan struktur
 */

#ifndef LIBC_STDIO_H
#define LIBC_STDIO_H

/* ============================================================
 * DEPENDENSI
 * ============================================================
 */
#include <stddef.h>
#include <stdarg.h>

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

/*
 * fdopen - Associate stream dengan file descriptor
 */
extern FILE *fdopen(int fd, const char *mode);

/*
 * fmemopen - Open memory as stream
 */
extern FILE *fmemopen(void *buf, size_t size, const char *mode);

/*
 * open_memstream - Open memory stream for writing
 */
extern FILE *open_memstream(char **ptr, size_t *sizeloc);

/* ============================================================
 * FUNGSI OPERASI FILE
 * ============================================================
 */

/*
 * fopen - Buka file
 */
extern FILE *fopen(const char *filename, const char *mode);

/*
 * freopen - Buka ulang file dengan stream yang ada
 */
extern FILE *freopen(const char *filename, const char *mode, FILE *stream);

/*
 * fclose - Tutup file
 */
extern int fclose(FILE *stream);

/*
 * fflush - Flush buffer ke file
 */
extern int fflush(FILE *stream);

/* ============================================================
 * FUNGSI BACA/TULIS BINER
 * ============================================================
 */

/*
 * fread - Baca blok data
 */
extern size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);

/*
 * fwrite - Tulis blok data
 */
extern size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);

/* ============================================================
 * FUNGSI BACA/TULIS KARAKTER
 * ============================================================
 */

/*
 * fgetc - Baca satu karakter
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
 */
extern char *fgets(char *s, int n, FILE *stream);

/*
 * gets - DIHAPUS sesuai Pigura C90
 * Fungsi gets() tidak aman (tanpa batas ukuran buffer).
 * Gunakan fgets() sebagai pengganti yang aman.
 */

/*
 * fputc - Tulis satu karakter
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
 */
extern int fputs(const char *s, FILE *stream);

/*
 * puts - Tulis string ke stdout dengan newline
 */
extern int puts(const char *s);

/*
 * ungetc - Kembalikan karakter ke stream
 */
extern int ungetc(int c, FILE *stream);

/* ============================================================
 * FUNGSI POSISI FILE
 * ============================================================
 */

/*
 * fseek - Set posisi file
 */
extern int fseek(FILE *stream, long offset, int whence);

/*
 * ftell - Dapatkan posisi file
 */
extern long ftell(FILE *stream);

/*
 * rewind - Reset posisi ke awal file
 */
extern void rewind(FILE *stream);

/*
 * fgetpos - Dapatkan posisi file
 */
extern int fgetpos(FILE *stream, fpos_t *pos);

/*
 * fsetpos - Set posisi file
 */
extern int fsetpos(FILE *stream, const fpos_t *pos);

/*
 * clearerr - Reset error dan EOF flag
 */
extern void clearerr(FILE *stream);

/*
 * feof - Cek apakah EOF tercapai
 */
extern int feof(FILE *stream);

/*
 * ferror - Cek apakah error terjadi
 */
extern int ferror(FILE *stream);

/*
 * perror - Cetak pesan error
 */
extern void perror(const char *s);

/* ============================================================
 * FUNGSI FORMATTED I/O
 * ============================================================
 */

/*
 * printf - Cetak formatted ke stdout
 */
extern int printf(const char *format, ...);

/*
 * fprintf - Cetak formatted ke file
 */
extern int fprintf(FILE *stream, const char *format, ...);

/*
 * sprintf - DIHAPUS sesuai Pigura C90
 * Fungsi sprintf() tidak aman (tanpa batas ukuran buffer).
 * Gunakan snprintf() sebagai pengganti yang aman.
 */

/*
 * snprintf - Cetak formatted ke string dengan batas
 */
extern int snprintf(char *str, size_t size, const char *format, ...);

/*
 * vprintf - Cetak formatted ke stdout (va_list)
 */
extern int vprintf(const char *format, va_list ap);

/*
 * vfprintf - Cetak formatted ke file (va_list)
 */
extern int vfprintf(FILE *stream, const char *format, va_list ap);

/*
 * vsprintf - DIHAPUS sesuai Pigura C90
 * Fungsi vsprintf() tidak aman (tanpa batas ukuran buffer).
 * Gunakan vsnprintf() sebagai pengganti yang aman.
 */

/*
 * vsnprintf - Cetak formatted ke string dengan batas (va_list)
 */
extern int vsnprintf(char *str, size_t size, const char *format, va_list ap);

/*
 * scanf - Baca formatted dari stdin
 */
extern int scanf(const char *format, ...);

/*
 * fscanf - Baca formatted dari file
 */
extern int fscanf(FILE *stream, const char *format, ...);

/*
 * sscanf - Baca formatted dari string
 */
extern int sscanf(const char *str, const char *format, ...);

/* ============================================================
 * FUNGSI MANAJEMEN FILE
 * ============================================================
 */

/*
 * remove - Hapus file
 */
extern int remove(const char *filename);

/*
 * rename - Ubah nama file
 */
extern int rename(const char *old, const char *new);

/*
 * tmpfile - Buat file temporary
 */
extern FILE *tmpfile(void);

/*
 * tmpnam - Buat nama file temporary
 */
extern char *tmpnam(char *s);

/*
 * setbuf - Set buffer untuk stream
 */
extern void setbuf(FILE *stream, char *buf);

/*
 * setvbuf - Set buffer dan mode untuk stream
 */
extern int setvbuf(FILE *stream, char *buf, int mode, size_t size);

/* ============================================================
 * FUNGSI MANAJEMEN BUFFER INTERNAL
 * ============================================================
 */

/*
 * __stdio_init - Inisialisasi stdio
 */
extern void __stdio_init(void);

/*
 * __stdio_cleanup - Bersihkan stdio
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

#endif /* LIBC_STDIO_H */
