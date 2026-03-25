/*
 * PIGURA LIBC - STDIO/SNPRINTF.C
 * ================================
 * Implementasi fungsi snprintf dan vsnprintf.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

/* ============================================================
 * KONFIGURASI
 * ============================================================
 */

/* Maksimum ukuran output */
#define SNPRINTF_MAX_SIZE   65536

/* ============================================================
 * VSNPRINTF CORE
 * ============================================================
 */

/* Output ke buffer dengan batas */
typedef struct {
    char *buffer;       /* Buffer output */
    size_t size;        /* Ukuran buffer */
    size_t pos;         /* Posisi saat ini */
    int would_write;    /* Karakter yang akan ditulis tanpa batas */
} snprintf_ctx_t;

/* Callback output */
static int _snprintf_putchar(int c, void *data) {
    snprintf_ctx_t *ctx = (snprintf_ctx_t *)data;

    ctx->would_write++;

    if (ctx->buffer != NULL && ctx->pos < ctx->size - 1) {
        ctx->buffer[ctx->pos++] = (char)c;
        ctx->buffer[ctx->pos] = '\0';
    }

    return 0;
}

/* State untuk parsing format */
#define FMT_DEFAULT      0
#define FMT_FLAGS        1
#define FMT_WIDTH        2
#define FMT_PREC_DOT     3
#define FMT_PRECISION    4
#define FMT_LENGTH       5
#define FMT_SPEC         6

/* Forward declaration - dari printf.c */
extern int _do_printf(const char *format, va_list ap,
                      int (*putfn)(int, void *), void *data);

/* ============================================================
 * VSNPRINTF
 * ============================================================
 * Format string ke buffer dengan batas ukuran.
 *
 * Parameter:
 *   str    - Buffer tujuan
 *   size   - Ukuran buffer (termasuk null terminator)
 *   format - Format string
 *   ap     - Argument list
 *
 * Return: Jumlah karakter yang akan ditulis (tanpa null) jika
 *         buffer cukup besar. Return negatif jika error.
 */
int vsnprintf(char *str, size_t size, const char *format, va_list ap) {
    snprintf_ctx_t ctx;
    int result;

    /* Validasi parameter */
    if (format == NULL) {
        if (str != NULL && size > 0) {
            str[0] = '\0';
        }
        return 0;
    }

    /* Initialize context */
    ctx.buffer = str;
    ctx.size = size;
    ctx.pos = 0;
    ctx.would_write = 0;

    /* Null terminate jika buffer valid */
    if (str != NULL && size > 0) {
        str[0] = '\0';
    }

    /* Lakukan formatting */
    result = _do_printf(format, ap, _snprintf_putchar, &ctx);

    if (result < 0) {
        return -1;
    }

    /* Pastikan null terminated */
    if (str != NULL && size > 0) {
        if (ctx.pos >= size) {
            str[size - 1] = '\0';
        } else {
            str[ctx.pos] = '\0';
        }
    }

    return ctx.would_write;
}

/* ============================================================
 * SNPRINTF
 * ============================================================
 * Format string ke buffer dengan batas ukuran.
 *
 * Parameter:
 *   str    - Buffer tujuan
 *   size   - Ukuran buffer
 *   format - Format string
 *   ...    - Arguments
 *
 * Return: Jumlah karakter yang akan ditulis
 */
int snprintf(char *str, size_t size, const char *format, ...) {
    va_list ap;
    int result;

    va_start(ap, format);
    result = vsnprintf(str, size, format, ap);
    va_end(ap);

    return result;
}

/* ============================================================
 * VSPRINTF (untuk kompatibilitas, gunakan vsnprintf)
 * ============================================================
 * Format string ke buffer tanpa batas ukuran (tidak aman).
 *
 * Parameter:
 *   str    - Buffer tujuan
 *   format - Format string
 *   ap     - Argument list
 *
 * Return: Jumlah karakter yang ditulis
 */
int vsprintf(char *str, const char *format, va_list ap) {
    snprintf_ctx_t ctx;
    int result;

    if (str == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Initialize dengan "size tak terbatas" */
    ctx.buffer = str;
    ctx.size = SNPRINTF_MAX_SIZE;
    ctx.pos = 0;
    ctx.would_write = 0;

    result = _do_printf(format, ap, _snprintf_putchar, &ctx);

    if (result < 0) {
        return -1;
    }

    /* Null terminate */
    if (ctx.pos < ctx.size) {
        str[ctx.pos] = '\0';
    }

    return (int)ctx.pos;
}

/* ============================================================
 * SPRINTF (untuk kompatibilitas, gunakan snprintf)
 * ============================================================
 * Format string ke buffer tanpa batas (tidak aman).
 *
 * Parameter:
 *   str    - Buffer tujuan
 *   format - Format string
 *   ...    - Arguments
 *
 * Return: Jumlah karakter yang ditulis
 */
int sprintf(char *str, const char *format, ...) {
    va_list ap;
    int result;

    va_start(ap, format);
    result = vsprintf(str, format, ap);
    va_end(ap);

    return result;
}

/* ============================================================
 * VASPRINTF (GNU extension)
 * ============================================================
 * Format dan alokasi buffer secara otomatis.
 *
 * Parameter:
 *   strp   - Pointer untuk menyimpan hasil alokasi
 *   format - Format string
 *   ap     - Argument list
 *
 * Return: Jumlah karakter yang ditulis, atau -1 jika error
 */
int vasprintf(char **strp, const char *format, va_list ap) {
    va_list ap_copy;
    int len;
    char *buf;

    if (strp == NULL || format == NULL) {
        return -1;
    }

    /* Hitung panjang yang diperlukan */
    va_copy(ap_copy, ap);
    len = vsnprintf(NULL, 0, format, ap_copy);
    va_end(ap_copy);

    if (len < 0) {
        return -1;
    }

    /* Alokasikan buffer */
    buf = (char *)malloc((size_t)(len + 1));
    if (buf == NULL) {
        return -1;
    }

    /* Format ke buffer */
    vsnprintf(buf, (size_t)(len + 1), format, ap);

    *strp = buf;
    return len;
}

/* ============================================================
 * ASPRINTF (GNU extension)
 * ============================================================
 * Format dan alokasi buffer secara otomatis.
 *
 * Parameter:
 *   strp   - Pointer untuk menyimpan hasil alokasi
 *   format - Format string
 *   ...    - Arguments
 *
 * Return: Jumlah karakter yang ditulis, atau -1 jika error
 */
int asprintf(char **strp, const char *format, ...) {
    va_list ap;
    int result;

    va_start(ap, format);
    result = vasprintf(strp, format, ap);
    va_end(ap);

    return result;
}

/* ============================================================
 * DPRINTF (GNU extension)
 * ============================================================
 * Format ke file descriptor.
 *
 * Parameter:
 *   fd     - File descriptor
 *   format - Format string
 *   ...    - Arguments
 *
 * Return: Jumlah karakter yang ditulis, atau -1 jika error
 */
int dprintf(int fd, const char *format, ...) {
    va_list ap;
    char buf[4096];
    int len;
    int written;
    char *p;

    va_start(ap, format);
    len = vsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);

    if (len < 0) {
        return -1;
    }

    /* Write ke file descriptor */
    p = buf;
    written = 0;

    while (written < len) {
        int ret = write(fd, p + written, len - written);
        if (ret < 0) {
            return -1;
        }
        written += ret;
    }

    return written;
}

/* ============================================================
 * VDPRINTF (GNU extension)
 * ============================================================
 * Format ke file descriptor dengan va_list.
 *
 * Parameter:
 *   fd     - File descriptor
 *   format - Format string
 *   ap     - Argument list
 *
 * Return: Jumlah karakter yang ditulis, atau -1 jika error
 */
int vdprintf(int fd, const char *format, va_list ap) {
    char buf[4096];
    int len;
    int written;
    char *p;

    len = vsnprintf(buf, sizeof(buf), format, ap);

    if (len < 0) {
        return -1;
    }

    p = buf;
    written = 0;

    while (written < len) {
        int ret = write(fd, p + written, len - written);
        if (ret < 0) {
            return -1;
        }
        written += ret;
    }

    return written;
}

/* ============================================================
 * FORWARD DECLARATIONS
 * ============================================================
 */
extern long write(int fd, const void *buf, size_t count);
extern void *malloc(size_t size);
