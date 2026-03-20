/*
 * PIGURA OS - STRING & MEMORY FUNCTIONS
 * -------------------------------------
 * Implementasi fungsi-fungsi string dan memory untuk kernel.
 *
 * Berkas ini berisi implementasi fungsi-fungsi utilitas string dan memory
 * yang digunakan di seluruh kernel.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../kernel.h"

/*
 * ============================================================================
 * FUNGSI MEMORY (MEMORY FUNCTIONS)
 * ============================================================================
 */

/*
 * kernel_memcpy
 * -------------
 * Copy memory block.
 */
void *kernel_memcpy(void *dest, const void *src, ukuran_t n)
{
    tak_bertanda8_t *d;
    const tak_bertanda8_t *s;
    ukuran_t i;

    if (dest == NULL || src == NULL || n == 0) {
        return dest;
    }

    d = (tak_bertanda8_t *)dest;
    s = (const tak_bertanda8_t *)src;

    /* Copy byte per byte */
    for (i = 0; i < n; i++) {
        d[i] = s[i];
    }

    return dest;
}

/*
 * kernel_memset
 * -------------
 * Set memory block dengan nilai.
 */
void *kernel_memset(void *s, int c, ukuran_t n)
{
    tak_bertanda8_t *ptr;
    ukuran_t i;

    if (s == NULL || n == 0) {
        return s;
    }

    ptr = (tak_bertanda8_t *)s;

    for (i = 0; i < n; i++) {
        ptr[i] = (tak_bertanda8_t)c;
    }

    return s;
}

/*
 * kernel_memmove
 * --------------
 * Move memory block (handles overlap).
 */
void *kernel_memmove(void *dest, const void *src, ukuran_t n)
{
    tak_bertanda8_t *d;
    const tak_bertanda8_t *s;

    if (dest == NULL || src == NULL || n == 0) {
        return dest;
    }

    d = (tak_bertanda8_t *)dest;
    s = (const tak_bertanda8_t *)src;

    if (d < s) {
        /* Copy forward */
        ukuran_t i;
        for (i = 0; i < n; i++) {
            d[i] = s[i];
        }
    } else if (d > s) {
        /* Copy backward */
        while (n > 0) {
            n--;
            d[n] = s[n];
        }
    }

    return dest;
}

/*
 * kernel_memcmp
 * -------------
 * Compare memory blocks.
 */
int kernel_memcmp(const void *s1, const void *s2, ukuran_t n)
{
    const tak_bertanda8_t *p1;
    const tak_bertanda8_t *p2;
    ukuran_t i;

    if (s1 == NULL || s2 == NULL) {
        if (s1 == s2) {
            return 0;
        }
        return (s1 == NULL) ? -1 : 1;
    }

    p1 = (const tak_bertanda8_t *)s1;
    p2 = (const tak_bertanda8_t *)s2;

    for (i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] - p2[i];
        }
    }

    return 0;
}

/*
 * ============================================================================
 * FUNGSI STRING (STRING FUNCTIONS)
 * ============================================================================
 */

/*
 * kernel_strlen
 * -------------
 * Hitung panjang string.
 */
ukuran_t kernel_strlen(const char *s)
{
    ukuran_t len;

    if (s == NULL) {
        return 0;
    }

    len = 0;
    while (s[len] != '\0') {
        len++;
    }

    return len;
}

/*
 * kernel_strncpy
 * --------------
 * Copy string dengan batas.
 */
char *kernel_strncpy(char *dest, const char *src, ukuran_t n)
{
    ukuran_t i;

    if (dest == NULL || src == NULL || n == 0) {
        return dest;
    }

    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }

    /* Pad dengan null jika ada ruang */
    for (; i < n; i++) {
        dest[i] = '\0';
    }

    return dest;
}

/*
 * kernel_strcmp
 * -------------
 * Compare dua string.
 */
int kernel_strcmp(const char *s1, const char *s2)
{
    if (s1 == NULL || s2 == NULL) {
        if (s1 == s2) {
            return 0;
        }
        return (s1 == NULL) ? -1 : 1;
    }

    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }

    return *(const tak_bertanda8_t *)s1 - *(const tak_bertanda8_t *)s2;
}

/*
 * kernel_strncmp
 * --------------
 * Compare dua string dengan batas.
 */
int kernel_strncmp(const char *s1, const char *s2, ukuran_t n)
{
    ukuran_t i;

    if (s1 == NULL || s2 == NULL || n == 0) {
        if (n == 0) {
            return 0;
        }
        if (s1 == s2) {
            return 0;
        }
        return (s1 == NULL) ? -1 : 1;
    }

    for (i = 0; i < n; i++) {
        if (s1[i] != s2[i] || s1[i] == '\0') {
            return (tak_bertanda8_t)s1[i] - (tak_bertanda8_t)s2[i];
        }
    }

    return 0;
}

/*
 * ============================================================================
 * FUNGSI PRINTF (PRINTF FUNCTIONS)
 * ============================================================================
 */

/* State machine untuk printf */
typedef struct {
    char *buffer;           /* Buffer output */
    ukuran_t size;          /* Ukuran buffer */
    ukuran_t pos;           /* Posisi saat ini */
    int total;              /* Total karakter yang ditulis */
} printf_state_t;

/*
 * printf_putchar
 * --------------
 * Tambah karakter ke buffer.
 */
static void printf_putchar(printf_state_t *state, char c)
{
    if (state->buffer != NULL && state->pos < state->size - 1) {
        state->buffer[state->pos++] = c;
    }
    state->total++;
}

/*
 * printf_puts
 * -----------
 * Tambah string ke buffer.
 */
static void printf_puts(printf_state_t *state, const char *s)
{
    if (s == NULL) {
        s = "(null)";
    }

    while (*s) {
        printf_putchar(state, *s);
        s++;
    }
}

/*
 * printf_print_num
 * ----------------
 * Print angka dalam basis tertentu.
 */
static void printf_print_num(printf_state_t *state,
                             tak_bertanda64_t value,
                             int base,
                             int width,
                             char pad,
                             int uppercase,
                             int is_negative)
{
    char buffer[24];
    const char *digits;
    int i;
    int len;

    digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";

    /* Handle angka 0 */
    if (value == 0 && !is_negative) {
        buffer[0] = '0';
        len = 1;
    } else {
        /* Konversi ke string (dari belakang) */
        i = 23;
        buffer[i] = '\0';
        len = 0;

        while (value > 0) {
            i--;
            buffer[i] = digits[value % base];
            value /= base;
            len++;
        }

        /* Tambah tanda negatif */
        if (is_negative) {
            i--;
            buffer[i] = '-';
            len++;
        }

        /* Geser ke depan */
        {
            int j;
            for (j = 0; j <= len; j++) {
                buffer[j] = buffer[i + j];
            }
        }
    }

    /* Padding */
    if (width > len) {
        int pad_len = width - len;
        while (pad_len > 0) {
            printf_putchar(state, pad);
            pad_len--;
        }
    }

    /* Print angka */
    for (i = 0; i < len; i++) {
        printf_putchar(state, buffer[i]);
    }
}

/*
 * vsnprintf
 * ---------
 * Format string ke buffer dengan ukuran terbatas.
 */
int vsnprintf(char *str, ukuran_t size, const char *format, va_list ap)
{
    printf_state_t state;
    char c;

    state.buffer = str;
    state.size = size;
    state.pos = 0;
    state.total = 0;

    if (format == NULL) {
        if (str != NULL && size > 0) {
            str[0] = '\0';
        }
        return 0;
    }

    while ((c = *format++) != '\0') {
        if (c != '%') {
            printf_putchar(&state, c);
            continue;
        }

        /* Parse format specifier */
        c = *format++;
        if (c == '\0') {
            break;
        }

        /* Flags */
        char pad = ' ';
        if (c == '0') {
            pad = '0';
            c = *format++;
            if (c == '\0') break;
        }

        /* Width */
        int width = 0;
        while (c >= '0' && c <= '9') {
            width = width * 10 + (c - '0');
            c = *format++;
            if (c == '\0') break;
        }
        if (c == '\0') break;

        /* Length modifier */
        int is_long = 0;
        if (c == 'l') {
            is_long = 1;
            c = *format++;
            if (c == '\0') break;
        }

        /* Conversion specifier */
        switch (c) {
            case 'd':
            case 'i': {
                /* Signed integer */
                tanda64_t val;
                if (is_long) {
                    val = va_arg(ap, long);
                } else {
                    val = va_arg(ap, int);
                }
                int neg = 0;
                if (val < 0) {
                    neg = 1;
                    val = -val;
                }
                printf_print_num(&state, (tak_bertanda64_t)val, 10,
                                width, pad, 0, neg);
                break;
            }

            case 'u': {
                /* Unsigned integer */
                tak_bertanda64_t val;
                if (is_long) {
                    val = va_arg(ap, unsigned long);
                } else {
                    val = va_arg(ap, unsigned int);
                }
                printf_print_num(&state, val, 10, width, pad, 0, 0);
                break;
            }

            case 'x': {
                /* Hex lowercase */
                tak_bertanda64_t val;
                if (is_long) {
                    val = va_arg(ap, unsigned long);
                } else {
                    val = va_arg(ap, unsigned int);
                }
                printf_print_num(&state, val, 16, width, pad, 0, 0);
                break;
            }

            case 'X': {
                /* Hex uppercase */
                tak_bertanda64_t val;
                if (is_long) {
                    val = va_arg(ap, unsigned long);
                } else {
                    val = va_arg(ap, unsigned int);
                }
                printf_print_num(&state, val, 16, width, pad, 1, 0);
                break;
            }

            case 'p': {
                /* Pointer */
                void *ptr = va_arg(ap, void *);
                printf_putchar(&state, '0');
                printf_putchar(&state, 'x');
                printf_print_num(&state, (tak_bertanda64_t)(alamat_ptr_t)ptr,
                                16, 8, '0', 0, 0);
                break;
            }

            case 's': {
                /* String */
                const char *s = va_arg(ap, const char *);
                printf_puts(&state, s);
                break;
            }

            case 'c': {
                /* Character */
                char ch = (char)va_arg(ap, int);
                printf_putchar(&state, ch);
                break;
            }

            case '%': {
                /* Literal % */
                printf_putchar(&state, '%');
                break;
            }

            default: {
                /* Unknown, print as-is */
                printf_putchar(&state, '%');
                printf_putchar(&state, c);
                break;
            }
        }
    }

    /* Null terminate */
    if (state.buffer != NULL && state.size > 0) {
        state.buffer[state.pos < state.size ? state.pos : state.size - 1] = '\0';
    }

    return state.total;
}

/*
 * snprintf
 * --------
 * Format string ke buffer dengan ukuran terbatas.
 */
int snprintf(char *str, ukuran_t size, const char *format, ...)
{
    va_list ap;
    int ret;

    va_start(ap, format);
    ret = vsnprintf(str, size, format, ap);
    va_end(ap);

    return ret;
}

/*
 * ============================================================================
 * FUNGSI OUTPUT KERNEL (KERNEL OUTPUT FUNCTIONS)
 * ============================================================================
 */

/*
 * kernel_printf
 * -------------
 * Print formatted string ke console.
 */
int kernel_printf(const char *format, ...)
{
    va_list ap;
    char buffer[1024];
    int ret;

    va_start(ap, format);
    ret = vsnprintf(buffer, sizeof(buffer), format, ap);
    va_end(ap);

    if (ret > 0) {
        hal_console_puts(buffer);
    }

    return ret;
}

/*
 * kernel_printk
 * -------------
 * Print ke kernel log dengan level.
 */
int kernel_printk(int level, const char *format, ...)
{
    va_list ap;
    char buffer[1024];
    int ret;

    /* Check log level */
    if (level > LOG_LEVEL) {
        return 0;
    }

    va_start(ap, format);
    ret = vsnprintf(buffer, sizeof(buffer), format, ap);
    va_end(ap);

    if (ret > 0) {
        /* Set warna berdasarkan level */
        switch (level) {
            case LOG_ERROR:
                hal_console_print_error(buffer);
                break;
            case LOG_WARN:
                hal_console_print_warning(buffer);
                break;
            default:
                hal_console_puts(buffer);
                break;
        }
    }

    return ret;
}

/*
 * kernel_puts
 * -----------
 * Print string ke console.
 */
int kernel_puts(const char *str)
{
    if (str == NULL) {
        return -1;
    }

    hal_console_puts(str);
    hal_console_putchar('\n');

    return 0;
}

/*
 * kernel_putchar
 * --------------
 * Print satu karakter.
 */
int kernel_putchar(int c)
{
    hal_console_putchar((char)c);
    return c;
}

/*
 * kernel_clear_screen
 * -------------------
 * Bersihkan layar.
 */
void kernel_clear_screen(void)
{
    hal_console_clear();
}

/*
 * kernel_set_color
 * ----------------
 * Set warna teks.
 */
void kernel_set_color(tak_bertanda8_t fg, tak_bertanda8_t bg)
{
    hal_console_set_color(fg, bg);
}
