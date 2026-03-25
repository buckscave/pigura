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

#endif /* LIBC_STDIO_H */
