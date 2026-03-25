/*
 * PIGURA LIBC - STDIO/PRINTF.C
 * =============================
 * Implementasi fungsi formatted output.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

/* ============================================================
 * KONFIGURASI
 * ============================================================
 */

/* Maksimum panjang output */
#define PRINTF_MAX_OUTPUT    4096

/* Maksimum padding */
#define PRINTF_MAX_PAD       256

/* ============================================================
 * STRUKTUR OUTPUT
 * ============================================================
 */

/* Callback untuk output karakter */
typedef int (*putchar_fn_t)(int c, void *data);

/* Konteks output ke string */
typedef struct {
    char *buffer;       /* Buffer output */
    size_t size;        /* Ukuran buffer */
    size_t pos;         /* Posisi saat ini */
} str_ctx_t;

/* Konteks output ke file */
typedef struct {
    FILE *fp;           /* File stream */
    int count;          /* Jumlah karakter yang ditulis */
} file_ctx_t;

/* ============================================================
 * FUNGSI OUTPUT HELPER
 * ============================================================
 */

/* Output ke file */
static int _put_to_file(int c, void *data) {
    file_ctx_t *ctx = (file_ctx_t *)data;

    if (fputc(c, ctx->fp) == EOF) {
        return -1;
    }

    ctx->count++;
    return 0;
}

/* Output ke string */
static int _put_to_str(int c, void *data) {
    str_ctx_t *ctx = (str_ctx_t *)data;

    if (ctx->pos < ctx->size - 1) {
        ctx->buffer[ctx->pos++] = (char)c;
        ctx->buffer[ctx->pos] = '\0';
    }

    return 0;
}

/* Output karakter tunggal */
static int _put_char(char c, putchar_fn_t putfn, void *data) {
    return putfn((int)(unsigned char)c, data);
}

/* Output string */
static int _put_str(const char *s, putchar_fn_t putfn, void *data) {
    int count = 0;

    if (s == NULL) {
        s = "(null)";
    }

    while (*s != '\0') {
        if (_put_char(*s, putfn, data) < 0) {
            return -1;
        }
        count++;
        s++;
    }

    return count;
}

/* Output padding */
static int _put_pad(char c, int count, putchar_fn_t putfn, void *data) {
    int i;
    int written = 0;

    for (i = 0; i < count && i < PRINTF_MAX_PAD; i++) {
        if (_put_char(c, putfn, data) < 0) {
            return -1;
        }
        written++;
    }

    return written;
}

/* ============================================================
 * KONVERSI ANGKA KE STRING
 * ============================================================
 */

/* Konversi integer ke string */
static int _int_to_str(unsigned long value, char *buffer, int base,
                       int uppercase) {
    static const char digits_lower[] = "0123456789abcdef";
    static const char digits_upper[] = "0123456789ABCDEF";
    const char *digits;
    int len = 0;
    int i;
    char temp;

    if (base < 2 || base > 36) {
        buffer[0] = '\0';
        return 0;
    }

    digits = uppercase ? digits_upper : digits_lower;

    /* Handle 0 */
    if (value == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return 1;
    }

    /* Konversi digit */
    while (value > 0) {
        buffer[len++] = digits[value % base];
        value /= base;
    }

    buffer[len] = '\0';

    /* Reverse string */
    for (i = 0; i < len / 2; i++) {
        temp = buffer[i];
        buffer[i] = buffer[len - 1 - i];
        buffer[len - 1 - i] = temp;
    }

    return len;
}

/* Konversi float ke string (sederhana) */
static int _float_to_str(double value, char *buffer, int precision) {
    long int_part;
    unsigned long frac_part;
    double frac;
    int len = 0;
    int i;
    char temp[32];

    /* Handle negatif */
    if (value < 0) {
        buffer[len++] = '-';
        value = -value;
    }

    /* Ambil bagian integer */
    int_part = (long)value;

    /* Konversi bagian integer */
    if (int_part == 0) {
        buffer[len++] = '0';
    } else {
        int int_len = 0;
        long temp_int = int_part;

        while (temp_int > 0) {
            temp[int_len++] = '0' + (temp_int % 10);
            temp_int /= 10;
        }

        /* Reverse dan copy */
        for (i = int_len - 1; i >= 0; i--) {
            buffer[len++] = temp[i];
        }
    }

    /* Tambahkan bagian desimal */
    if (precision > 0) {
        buffer[len++] = '.';

        frac = value - (double)int_part;

        /* Kalikan untuk presisi */
        for (i = 0; i < precision; i++) {
            frac *= 10.0;
        }

        frac_part = (unsigned long)(frac + 0.5);

        /* Konversi fraction */
        if (frac_part == 0) {
            for (i = 0; i < precision; i++) {
                buffer[len++] = '0';
            }
        } else {
            char frac_buf[32];
            int frac_len = 0;

            while (frac_part > 0 || frac_len < precision) {
                frac_buf[frac_len++] = '0' + (frac_part % 10);
                frac_part /= 10;

                if (frac_len >= precision) {
                    break;
                }
            }

            /* Pad dengan nol jika perlu */
            while (frac_len < precision) {
                buffer[len++] = '0';
                frac_len++;
            }

            /* Reverse */
            for (i = frac_len - 1; i >= 0; i--) {
                buffer[len++] = frac_buf[i];
            }
        }
    }

    buffer[len] = '\0';
    return len;
}

/* ============================================================
 * FORMAT SPECIFIER PARSER
 * ============================================================
 */

/* Flags */
#define FLAG_LEFT      0x01   /* '-' left justify */
#define FLAG_PLUS      0x02   /* '+' tampilkan + */
#define FLAG_SPACE     0x04   /* ' ' spasi sebelum positif */
#define FLAG_HASH      0x08   /* '#' alternate form */
#define FLAG_ZERO      0x10   /* '0' pad dengan nol */

/* Length modifier */
#define LEN_NONE       0
#define LEN_HH         1      /* hh */
#define LEN_H          2      /* h */
#define LEN_L          3      /* l */
#define LEN_LL         4      /* ll */
#define LEN_J          5      /* j */
#define LEN_Z          6      /* z */
#define LEN_T          7      /* t */
#define LEN_LDBL       8      /* L */

/* Parse format specifier */
static const char *_parse_format(const char *format, int *flags, int *width,
                                  int *precision, int *length, int *spec) {
    const char *p = format;

    *flags = 0;
    *width = -1;
    *precision = -1;
    *length = LEN_NONE;
    *spec = 0;

    /* Parse flags */
    while (1) {
        switch (*p) {
            case '-':
                *flags |= FLAG_LEFT;
                p++;
                break;
            case '+':
                *flags |= FLAG_PLUS;
                p++;
                break;
            case ' ':
                *flags |= FLAG_SPACE;
                p++;
                break;
            case '#':
                *flags |= FLAG_HASH;
                p++;
                break;
            case '0':
                *flags |= FLAG_ZERO;
                p++;
                break;
            default:
                goto done_flags;
        }
    }
done_flags:

    /* Parse width */
    if (isdigit((unsigned char)*p)) {
        *width = 0;
        while (isdigit((unsigned char)*p)) {
            *width = *width * 10 + (*p - '0');
            p++;
        }
    } else if (*p == '*') {
        /* Width dari argument (tidak didukung dalam versi ini) */
        p++;
    }

    /* Parse precision */
    if (*p == '.') {
        p++;
        *precision = 0;

        if (isdigit((unsigned char)*p)) {
            while (isdigit((unsigned char)*p)) {
                *precision = *precision * 10 + (*p - '0');
                p++;
            }
        } else if (*p == '*') {
            /* Precision dari argument (tidak didukung) */
            p++;
        }
    }

    /* Parse length modifier */
    switch (*p) {
        case 'h':
            p++;
            if (*p == 'h') {
                *length = LEN_HH;
                p++;
            } else {
                *length = LEN_H;
            }
            break;
        case 'l':
            p++;
            if (*p == 'l') {
                *length = LEN_LL;
                p++;
            } else {
                *length = LEN_L;
            }
            break;
        case 'j':
            *length = LEN_J;
            p++;
            break;
        case 'z':
            *length = LEN_Z;
            p++;
            break;
        case 't':
            *length = LEN_T;
            p++;
            break;
        case 'L':
            *length = LEN_LDBL;
            p++;
            break;
        default:
            break;
    }

    /* Parse conversion specifier */
    *spec = (unsigned char)*p;

    return p;
}

/* ============================================================
 * CORE PRINTF ENGINE
 * ============================================================
 */

static int _do_printf(const char *format, va_list ap,
                      putchar_fn_t putfn, void *data) {
    const char *p;
    int count = 0;
    int flags;
    int width;
    int precision;
    int length;
    int spec;
    char num_buf[64];
    int num_len;
    int pad_len;
    int i;
    long sval;
    unsigned long uval;
    double dval;
    char *strval;
    int is_negative;

    if (format == NULL) {
        return 0;
    }

    for (p = format; *p != '\0'; p++) {
        if (*p != '%') {
            /* Karakter biasa */
            if (_put_char(*p, putfn, data) < 0) {
                return -1;
            }
            count++;
            continue;
        }

        /* Skip '%' */
        p++;

        /* Handle '%%' */
        if (*p == '%') {
            if (_put_char('%', putfn, data) < 0) {
                return -1;
            }
            count++;
            continue;
        }

        /* Parse format specifier */
        p = _parse_format(p, &flags, &width, &precision, &length, &spec);

        /* Default precision untuk string dan float */
        if (precision < 0) {
            if (spec == 's') {
                precision = -1;  /* Tidak ada limit */
            } else if (spec == 'f' || spec == 'F' ||
                       spec == 'e' || spec == 'E' ||
                       spec == 'g' || spec == 'G') {
                precision = 6;
            }
        }

        /* Handle conversion */
        num_buf[0] = '\0';
        num_len = 0;
        is_negative = 0;

        switch (spec) {
            case 'd':
            case 'i':
                /* Signed integer */
                switch (length) {
                    case LEN_HH:
                        sval = (signed char)va_arg(ap, int);
                        break;
                    case LEN_H:
                        sval = (short)va_arg(ap, int);
                        break;
                    case LEN_L:
                        sval = va_arg(ap, long);
                        break;
                    case LEN_LL:
                        sval = va_arg(ap, long long);
                        break;
                    default:
                        sval = va_arg(ap, int);
                        break;
                }

                if (sval < 0) {
                    is_negative = 1;
                    uval = (unsigned long)(-sval);
                } else {
                    uval = (unsigned long)sval;
                }

                num_len = _int_to_str(uval, num_buf, 10, 0);
                break;

            case 'u':
                /* Unsigned integer */
                switch (length) {
                    case LEN_HH:
                        uval = (unsigned char)va_arg(ap, unsigned int);
                        break;
                    case LEN_H:
                        uval = (unsigned short)va_arg(ap, unsigned int);
                        break;
                    case LEN_L:
                        uval = va_arg(ap, unsigned long);
                        break;
                    case LEN_LL:
                        uval = va_arg(ap, unsigned long long);
                        break;
                    default:
                        uval = va_arg(ap, unsigned int);
                        break;
                }

                num_len = _int_to_str(uval, num_buf, 10, 0);
                break;

            case 'o':
                /* Octal */
                uval = va_arg(ap, unsigned int);
                num_len = _int_to_str(uval, num_buf, 8, 0);

                if ((flags & FLAG_HASH) && num_buf[0] != '0') {
                    /* Prepend '0' */
                    for (i = num_len; i >= 0; i--) {
                        num_buf[i + 1] = num_buf[i];
                    }
                    num_buf[0] = '0';
                    num_len++;
                }
                break;

            case 'x':
            case 'X':
                /* Hexadecimal */
                uval = va_arg(ap, unsigned int);
                num_len = _int_to_str(uval, num_buf, 16, (spec == 'X'));

                if ((flags & FLAG_HASH) && uval != 0) {
                    /* Prepend '0x' atau '0X' */
                    for (i = num_len; i >= 0; i--) {
                        num_buf[i + 2] = num_buf[i];
                    }
                    num_buf[0] = '0';
                    num_buf[1] = spec;
                    num_len += 2;
                }
                break;

            case 'f':
            case 'F':
                /* Float */
                if (length == LEN_LDBL) {
                    dval = va_arg(ap, long double);
                } else {
                    dval = va_arg(ap, double);
                }

                if (dval < 0) {
                    is_negative = 1;
                }

                num_len = _float_to_str(dval, num_buf,
                                        (precision >= 0) ? precision : 6);
                break;

            case 'c':
                /* Character */
                num_buf[0] = (char)va_arg(ap, int);
                num_buf[1] = '\0';
                num_len = 1;
                break;

            case 's':
                /* String */
                strval = va_arg(ap, char *);

                if (strval == NULL) {
                    strval = "(null)";
                }

                /* Output dengan padding */
                if (width > 0 && !(flags & FLAG_LEFT)) {
                    int slen = strlen(strval);
                    if (precision >= 0 && precision < slen) {
                        slen = precision;
                    }
                    if (slen < width) {
                        count += _put_pad(' ', width - slen, putfn, data);
                    }
                }

                if (precision >= 0) {
                    for (i = 0; i < precision && strval[i] != '\0'; i++) {
                        if (_put_char(strval[i], putfn, data) < 0) {
                            return -1;
                        }
                        count++;
                    }
                } else {
                    count += _put_str(strval, putfn, data);
                }

                if (width > 0 && (flags & FLAG_LEFT)) {
                    int slen = strlen(strval);
                    if (precision >= 0 && precision < slen) {
                        slen = precision;
                    }
                    if (slen < width) {
                        count += _put_pad(' ', width - slen, putfn, data);
                    }
                }

                continue;

            case 'p':
                /* Pointer */
                uval = (unsigned long)(size_t)va_arg(ap, void *);
                num_buf[0] = '0';
                num_buf[1] = 'x';
                num_len = 2 + _int_to_str(uval, num_buf + 2, 16, 0);
                break;

            case 'n':
                /* Write count */
                switch (length) {
                    case LEN_HH:
                        *(signed char *)va_arg(ap, signed char *) = count;
                        break;
                    case LEN_H:
                        *(short *)va_arg(ap, short *) = count;
                        break;
                    case LEN_L:
                        *(long *)va_arg(ap, long *) = count;
                        break;
                    case LEN_LL:
                        *(long long *)va_arg(ap, long long *) = count;
                        break;
                    default:
                        *(int *)va_arg(ap, int *) = count;
                        break;
                }
                continue;

            default:
                /* Unknown specifier, output as-is */
                if (_put_char('%', putfn, data) < 0) {
                    return -1;
                }
                count++;
                if (_put_char(spec, putfn, data) < 0) {
                    return -1;
                }
                count++;
                continue;
        }

        /* Calculate padding */
        pad_len = 0;
        if (width > num_len) {
            pad_len = width - num_len;

            /* Account for sign */
            if (is_negative || (flags & FLAG_PLUS) || (flags & FLAG_SPACE)) {
                if (pad_len > 0) {
                    pad_len--;
                }
            }
        }

        /* Output */
        if (!(flags & FLAG_LEFT)) {
            /* Right justify */
            char pad_char = (flags & FLAG_ZERO) ? '0' : ' ';

            if (pad_char == '0') {
                /* Output sign first */
                if (is_negative) {
                    if (_put_char('-', putfn, data) < 0) return -1;
                    count++;
                } else if (flags & FLAG_PLUS) {
                    if (_put_char('+', putfn, data) < 0) return -1;
                    count++;
                } else if (flags & FLAG_SPACE) {
                    if (_put_char(' ', putfn, data) < 0) return -1;
                    count++;
                }
            }

            count += _put_pad(pad_char, pad_len, putfn, data);

            if (pad_char == ' ') {
                /* Output sign after padding */
                if (is_negative) {
                    if (_put_char('-', putfn, data) < 0) return -1;
                    count++;
                } else if (flags & FLAG_PLUS) {
                    if (_put_char('+', putfn, data) < 0) return -1;
                    count++;
                } else if (flags & FLAG_SPACE) {
                    if (_put_char(' ', putfn, data) < 0) return -1;
                    count++;
                }
            }
        } else {
            /* Left justify - output sign first */
            if (is_negative) {
                if (_put_char('-', putfn, data) < 0) return -1;
                count++;
            } else if (flags & FLAG_PLUS) {
                if (_put_char('+', putfn, data) < 0) return -1;
                count++;
            } else if (flags & FLAG_SPACE) {
                if (_put_char(' ', putfn, data) < 0) return -1;
                count++;
            }
        }

        /* Output number */
        count += _put_str(num_buf, putfn, data);

        /* Left justify padding */
        if (flags & FLAG_LEFT) {
            count += _put_pad(' ', pad_len, putfn, data);
        }
    }

    return count;
}

/* ============================================================
 * FUNGSI PUBLIK
 * ============================================================
 */

/* printf - Output formatted ke stdout */
int printf(const char *format, ...) {
    va_list ap;
    int result;
    file_ctx_t ctx;

    ctx.fp = stdout;
    ctx.count = 0;

    va_start(ap, format);
    result = _do_printf(format, ap, _put_to_file, &ctx);
    va_end(ap);

    return (result < 0) ? -1 : ctx.count;
}

/* fprintf - Output formatted ke file */
int fprintf(FILE *stream, const char *format, ...) {
    va_list ap;
    int result;
    file_ctx_t ctx;

    if (stream == NULL) {
        errno = EBADF;
        return -1;
    }

    ctx.fp = stream;
    ctx.count = 0;

    va_start(ap, format);
    result = _do_printf(format, ap, _put_to_file, &ctx);
    va_end(ap);

    return (result < 0) ? -1 : ctx.count;
}

/* sprintf - Output formatted ke string */
int sprintf(char *str, const char *format, ...) {
    va_list ap;
    int result;
    str_ctx_t ctx;

    if (str == NULL) {
        errno = EINVAL;
        return -1;
    }

    ctx.buffer = str;
    ctx.size = PRINTF_MAX_OUTPUT;
    ctx.pos = 0;

    va_start(ap, format);
    result = _do_printf(format, ap, _put_to_str, &ctx);
    va_end(ap);

    /* Null terminate */
    if (ctx.pos < ctx.size) {
        ctx.buffer[ctx.pos] = '\0';
    }

    return (result < 0) ? -1 : (int)ctx.pos;
}

/* vprintf - printf dengan va_list */
int vprintf(const char *format, va_list ap) {
    file_ctx_t ctx;

    ctx.fp = stdout;
    ctx.count = 0;

    if (_do_printf(format, ap, _put_to_file, &ctx) < 0) {
        return -1;
    }

    return ctx.count;
}

/* vfprintf - fprintf dengan va_list */
int vfprintf(FILE *stream, const char *format, va_list ap) {
    file_ctx_t ctx;

    if (stream == NULL) {
        errno = EBADF;
        return -1;
    }

    ctx.fp = stream;
    ctx.count = 0;

    if (_do_printf(format, ap, _put_to_file, &ctx) < 0) {
        return -1;
    }

    return ctx.count;
}

/* vsprintf - sprintf dengan va_list */
int vsprintf(char *str, const char *format, va_list ap) {
    str_ctx_t ctx;

    if (str == NULL) {
        errno = EINVAL;
        return -1;
    }

    ctx.buffer = str;
    ctx.size = PRINTF_MAX_OUTPUT;
    ctx.pos = 0;

    if (_do_printf(format, ap, _put_to_str, &ctx) < 0) {
        return -1;
    }

    /* Null terminate */
    ctx.buffer[ctx.pos] = '\0';

    return (int)ctx.pos;
}
