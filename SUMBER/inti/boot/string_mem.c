/*
 * PIGURA OS - STRING & MEMORY FUNCTIONS
 * -------------------------------------
 * Implementasi fungsi-fungsi string dan memory untuk kernel.
 *
 * Berkas ini berisi implementasi fungsi-fungsi utilitas string dan memory
 * yang digunakan di seluruh kernel.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 *
 * Versi: 2.0
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
 *
 * Parameter:
 *   dest - Buffer tujuan
 *   src  - Buffer sumber
 *   n    - Jumlah byte
 *
 * Return: Pointer ke dest
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

    for (i = 0; i < n; i++) {
        d[i] = s[i];
    }

    return dest;
}

/*
 * kernel_memset
 * -------------
 * Set memory block dengan nilai.
 *
 * Parameter:
 *   s - Buffer
 *   c - Nilai byte
 *   n - Jumlah byte
 *
 * Return: Pointer ke s
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
 *
 * Parameter:
 *   dest - Buffer tujuan
 *   src  - Buffer sumber
 *   n    - Jumlah byte
 *
 * Return: Pointer ke dest
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
        ukuran_t i;
        for (i = 0; i < n; i++) {
            d[i] = s[i];
        }
    } else if (d > s) {
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
 *
 * Parameter:
 *   s1 - Block pertama
 *   s2 - Block kedua
 *   n  - Jumlah byte
 *
 * Return: <0, 0, >0 jika s1 < s2, s1 == s2, s1 > s2
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
 * kernel_memchr
 * -------------
 * Cari karakter dalam memory.
 *
 * Parameter:
 *   s - Buffer
 *   c - Karakter yang dicari
 *   n - Jumlah byte
 *
 * Return: Pointer ke karakter, atau NULL jika tidak ditemukan
 */
void *kernel_memchr(const void *s, int c, ukuran_t n)
{
    const tak_bertanda8_t *p;
    ukuran_t i;

    if (s == NULL || n == 0) {
        return NULL;
    }

    p = (const tak_bertanda8_t *)s;

    for (i = 0; i < n; i++) {
        if (p[i] == (tak_bertanda8_t)c) {
            return (void *)(p + i);
        }
    }

    return NULL;
}

/*
 * kernel_memset32
 * ---------------
 * Set memory dengan 32-bit value.
 *
 * Parameter:
 *   s   - Buffer
 *   val - Nilai 32-bit
 *   n   - Jumlah dword
 *
 * Return: Pointer ke s
 */
void *kernel_memset32(void *s, tak_bertanda32_t val, ukuran_t n)
{
    tak_bertanda32_t *ptr;
    ukuran_t i;

    if (s == NULL || n == 0) {
        return s;
    }

    ptr = (tak_bertanda32_t *)s;

    for (i = 0; i < n; i++) {
        ptr[i] = val;
    }

    return s;
}

/*
 * kernel_memset64
 * ---------------
 * Set memory dengan 64-bit value.
 *
 * Parameter:
 *   s   - Buffer
 *   val - Nilai 64-bit
 *   n   - Jumlah qword
 *
 * Return: Pointer ke s
 */
void *kernel_memset64(void *s, tak_bertanda64_t val, ukuran_t n)
{
    tak_bertanda64_t *ptr;
    ukuran_t i;

    if (s == NULL || n == 0) {
        return s;
    }

    ptr = (tak_bertanda64_t *)s;

    for (i = 0; i < n; i++) {
        ptr[i] = val;
    }

    return s;
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
 *
 * Parameter:
 *   s - String
 *
 * Return: Panjang string
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
 * kernel_strnlen
 * --------------
 * Hitung panjang string dengan batas.
 *
 * Parameter:
 *   s      - String
 *   maxlen - Panjang maksimum
 *
 * Return: Panjang string
 */
ukuran_t kernel_strnlen(const char *s, ukuran_t maxlen)
{
    ukuran_t len;

    if (s == NULL || maxlen == 0) {
        return 0;
    }

    len = 0;
    while (len < maxlen && s[len] != '\0') {
        len++;
    }

    return len;
}

/*
 * kernel_strcpy
 * -------------
 * Copy string.
 *
 * Parameter:
 *   dest - Buffer tujuan
 *   src  - String sumber
 *
 * Return: Pointer ke dest
 */
char *kernel_strcpy(char *dest, const char *src)
{
    ukuran_t i;

    if (dest == NULL || src == NULL) {
        return dest;
    }

    i = 0;
    while (src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';

    return dest;
}

/*
 * kernel_strncpy
 * --------------
 * Copy string dengan batas.
 *
 * Parameter:
 *   dest - Buffer tujuan
 *   src  - String sumber
 *   n    - Ukuran buffer
 *
 * Return: Pointer ke dest
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

    for (; i < n; i++) {
        dest[i] = '\0';
    }

    return dest;
}

/*
 * kernel_strcat
 * -------------
 * Concatenate string.
 *
 * Parameter:
 *   dest - Buffer tujuan
 *   src  - String sumber
 *
 * Return: Pointer ke dest
 */
char *kernel_strcat(char *dest, const char *src)
{
    ukuran_t dest_len;
    ukuran_t i;

    if (dest == NULL || src == NULL) {
        return dest;
    }

    dest_len = kernel_strlen(dest);

    for (i = 0; src[i] != '\0'; i++) {
        dest[dest_len + i] = src[i];
    }
    dest[dest_len + i] = '\0';

    return dest;
}

/*
 * kernel_strncat
 * --------------
 * Concatenate string dengan batas.
 *
 * Parameter:
 *   dest - Buffer tujuan
 *   src  - String sumber
 *   n    - Ukuran buffer tersisa
 *
 * Return: Pointer ke dest
 */
char *kernel_strncat(char *dest, const char *src, ukuran_t n)
{
    ukuran_t dest_len;
    ukuran_t i;

    if (dest == NULL || src == NULL || n == 0) {
        return dest;
    }

    dest_len = kernel_strlen(dest);

    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[dest_len + i] = src[i];
    }
    dest[dest_len + i] = '\0';

    return dest;
}

/*
 * kernel_strcmp
 * -------------
 * Compare dua string.
 *
 * Parameter:
 *   s1 - String pertama
 *   s2 - String kedua
 *
 * Return: <0, 0, >0 jika s1 < s2, s1 == s2, s1 > s2
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
 *
 * Parameter:
 *   s1 - String pertama
 *   s2 - String kedua
 *   n  - Jumlah karakter maksimum
 *
 * Return: <0, 0, >0 jika s1 < s2, s1 == s2, s1 > s2
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
 * kernel_strcasecmp
 * -----------------
 * Compare dua string (case insensitive).
 *
 * Parameter:
 *   s1 - String pertama
 *   s2 - String kedua
 *
 * Return: 0 jika sama
 */
int kernel_strcasecmp(const char *s1, const char *s2)
{
    if (s1 == NULL || s2 == NULL) {
        if (s1 == s2) {
            return 0;
        }
        return (s1 == NULL) ? -1 : 1;
    }

    while (*s1 && *s2) {
        char c1 = *s1;
        char c2 = *s2;

        /* Konversi ke lowercase */
        if (c1 >= 'A' && c1 <= 'Z') {
            c1 = c1 + ('a' - 'A');
        }
        if (c2 >= 'A' && c2 <= 'Z') {
            c2 = c2 + ('a' - 'A');
        }

        if (c1 != c2) {
            return (tak_bertanda8_t)c1 - (tak_bertanda8_t)c2;
        }

        s1++;
        s2++;
    }

    return (tak_bertanda8_t)*s1 - (tak_bertanda8_t)*s2;
}

/*
 * kernel_strchr
 * -------------
 * Cari karakter dalam string.
 *
 * Parameter:
 *   s - String
 *   c - Karakter
 *
 * Return: Pointer ke karakter, atau NULL
 */
char *kernel_strchr(const char *s, int c)
{
    if (s == NULL) {
        return NULL;
    }

    while (*s != '\0') {
        if (*s == (char)c) {
            return (char *)s;
        }
        s++;
    }

    /* Cek untuk null terminator */
    if (c == '\0') {
        return (char *)s;
    }

    return NULL;
}

/*
 * kernel_strrchr
 * --------------
 * Cari karakter terakhir dalam string.
 *
 * Parameter:
 *   s - String
 *   c - Karakter
 *
 * Return: Pointer ke karakter, atau NULL
 */
char *kernel_strrchr(const char *s, int c)
{
    const char *last;

    if (s == NULL) {
        return NULL;
    }

    last = NULL;

    while (*s != '\0') {
        if (*s == (char)c) {
            last = s;
        }
        s++;
    }

    /* Cek untuk null terminator */
    if (c == '\0') {
        return (char *)s;
    }

    return (char *)last;
}

/*
 * kernel_strstr
 * -------------
 * Cari substring dalam string.
 *
 * Parameter:
 *   haystack - String sumber
 *   needle   - Substring yang dicari
 *
 * Return: Pointer ke substring, atau NULL
 */
char *kernel_strstr(const char *haystack, const char *needle)
{
    ukuran_t needle_len;

    if (haystack == NULL || needle == NULL) {
        return NULL;
    }

    if (*needle == '\0') {
        return (char *)haystack;
    }

    needle_len = kernel_strlen(needle);

    while (*haystack != '\0') {
        if (*haystack == *needle) {
            if (kernel_strncmp(haystack, needle, needle_len) == 0) {
                return (char *)haystack;
            }
        }
        haystack++;
    }

    return NULL;
}

/*
 * kernel_strdup
 * -------------
 * Duplikasi string (harus di-free).
 *
 * Parameter:
 *   s - String
 *
 * Return: Pointer ke string baru, atau NULL
 */
char *kernel_strdup(const char *s)
{
    ukuran_t len;
    char *dup;

    if (s == NULL) {
        return NULL;
    }

    len = kernel_strlen(s);
    dup = (char *)kmalloc(len + 1);

    if (dup != NULL) {
        kernel_strcpy(dup, s);
    }

    return dup;
}

/*
 * kernel_strndup
 * --------------
 * Duplikasi string dengan batas.
 *
 * Parameter:
 *   s - String
 *   n - Panjang maksimum
 *
 * Return: Pointer ke string baru, atau NULL
 */
char *kernel_strndup(const char *s, ukuran_t n)
{
    ukuran_t len;
    char *dup;

    if (s == NULL) {
        return NULL;
    }

    len = kernel_strnlen(s, n);
    dup = (char *)kmalloc(len + 1);

    if (dup != NULL) {
        kernel_strncpy(dup, s, len);
        dup[len] = '\0';
    }

    return dup;
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

    if (value == 0 && !is_negative) {
        buffer[0] = '0';
        len = 1;
    } else {
        i = 23;
        buffer[i] = '\0';
        len = 0;

        while (value > 0) {
            i--;
            buffer[i] = digits[value % base];
            value /= base;
            len++;
        }

        if (is_negative) {
            i--;
            buffer[i] = '-';
            len++;
        }

        {
            int j;
            for (j = 0; j <= len; j++) {
                buffer[j] = buffer[i + j];
            }
        }
    }

    if (width > len) {
        int pad_len = width - len;
        while (pad_len > 0) {
            printf_putchar(state, pad);
            pad_len--;
        }
    }

    for (i = 0; i < len; i++) {
        printf_putchar(state, buffer[i]);
    }
}

/*
 * internal_vsnprintf
 * ------------------
 * Format string ke buffer dengan ukuran terbatas.
 * Fungsi internal, tidak diekspor.
 */
static int internal_vsnprintf(char *str, ukuran_t size,
                              const char *format, va_list ap)
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

        c = *format++;
        if (c == '\0') {
            break;
        }

        {
            char pad = ' ';
            if (c == '0') {
                pad = '0';
                c = *format++;
                if (c == '\0') break;
            }

            {
                int width = 0;
                while (c >= '0' && c <= '9') {
                    width = width * 10 + (c - '0');
                    c = *format++;
                    if (c == '\0') break;
                }
                if (c == '\0') break;

                {
                    int is_long = 0;
                    if (c == 'l') {
                        is_long = 1;
                        c = *format++;
                        if (c == '\0') break;
                    }

                    switch (c) {
                        case 'd':
                        case 'i': {
                            tanda64_t val;
                            int neg;
                            if (is_long) {
                                val = va_arg(ap, long);
                            } else {
                                val = va_arg(ap, int);
                            }
                            neg = 0;
                            if (val < 0) {
                                neg = 1;
                                val = -val;
                            }
                            printf_print_num(&state, (tak_bertanda64_t)val,
                                             10, width, pad, 0, neg);
                            break;
                        }

                        case 'u': {
                            tak_bertanda64_t val;
                            if (is_long) {
                                val = va_arg(ap, unsigned long);
                            } else {
                                val = va_arg(ap, unsigned int);
                            }
                            printf_print_num(&state, val, 10, width,
                                             pad, 0, 0);
                            break;
                        }

                        case 'x': {
                            tak_bertanda64_t val;
                            if (is_long) {
                                val = va_arg(ap, unsigned long);
                            } else {
                                val = va_arg(ap, unsigned int);
                            }
                            printf_print_num(&state, val, 16, width,
                                             pad, 0, 0);
                            break;
                        }

                        case 'X': {
                            tak_bertanda64_t val;
                            if (is_long) {
                                val = va_arg(ap, unsigned long);
                            } else {
                                val = va_arg(ap, unsigned int);
                            }
                            printf_print_num(&state, val, 16, width,
                                             pad, 1, 0);
                            break;
                        }

                        case 'p': {
                            void *ptr = va_arg(ap, void *);
                            printf_putchar(&state, '0');
                            printf_putchar(&state, 'x');
                            printf_print_num(&state,
                                             (tak_bertanda64_t)(uintptr_t)ptr,
                                             16, 8, '0', 0, 0);
                            break;
                        }

                        case 's': {
                            const char *s = va_arg(ap, const char *);
                            printf_puts(&state, s);
                            break;
                        }

                        case 'c': {
                            char ch = (char)va_arg(ap, int);
                            printf_putchar(&state, ch);
                            break;
                        }

                        case '%': {
                            printf_putchar(&state, '%');
                            break;
                        }

                        default: {
                            printf_putchar(&state, '%');
                            printf_putchar(&state, c);
                            break;
                        }
                    }
                }
            }
        }
    }

    if (state.buffer != NULL && state.size > 0) {
        ukuran_t term_pos;
        term_pos = (state.pos < state.size) ? state.pos : state.size - 1;
        state.buffer[term_pos] = '\0';
    }

    return state.total;
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
    ret = internal_vsnprintf(buffer, sizeof(buffer), format, ap);
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
    tak_bertanda8_t old_fg __attribute__((unused));
    tak_bertanda8_t old_bg __attribute__((unused));

    if (level > CONFIG_LOG_LEVEL) {
        return 0;
    }

    va_start(ap, format);
    ret = internal_vsnprintf(buffer, sizeof(buffer), format, ap);
    va_end(ap);

    if (ret > 0) {
        switch (level) {
            case LOG_ERROR:
                /* Print dengan warna merah untuk error */
                hal_console_set_color(VGA_MERAH, VGA_HITAM);
                hal_console_puts(buffer);
                hal_console_set_color(VGA_ABU_ABU, VGA_HITAM);
                break;
            case LOG_WARN:
                /* Print dengan warna kuning untuk warning */
                hal_console_set_color(VGA_KUNING, VGA_HITAM);
                hal_console_puts(buffer);
                hal_console_set_color(VGA_ABU_ABU, VGA_HITAM);
                break;
            default:
                hal_console_puts(buffer);
                break;
        }
    }

    return ret;
}

/*
 * kernel_vprintf
 * --------------
 * Print formatted string dengan va_list.
 */
int kernel_vprintf(const char *format, va_list args)
{
    char buffer[1024];
    int ret;

    ret = internal_vsnprintf(buffer, sizeof(buffer), format, args);

    if (ret > 0) {
        hal_console_puts(buffer);
    }

    return ret;
}

/*
 * kernel_vprintk
 * --------------
 * Print ke kernel log dengan va_list.
 */
int kernel_vprintk(int level, const char *format, va_list args)
{
    char buffer[1024];
    int ret;

    if (level > CONFIG_LOG_LEVEL) {
        return 0;
    }

    ret = internal_vsnprintf(buffer, sizeof(buffer), format, args);

    if (ret > 0) {
        switch (level) {
            case LOG_ERROR:
                hal_console_set_color(VGA_MERAH, VGA_HITAM);
                hal_console_puts(buffer);
                hal_console_set_color(VGA_ABU_ABU, VGA_HITAM);
                break;
            case LOG_WARN:
                hal_console_set_color(VGA_KUNING, VGA_HITAM);
                hal_console_puts(buffer);
                hal_console_set_color(VGA_ABU_ABU, VGA_HITAM);
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

/*
 * kernel_snprintf
 * ---------------
 * Format string ke buffer dengan batas ukuran.
 */
int kernel_snprintf(char *str, ukuran_t size, const char *format, ...)
{
    va_list ap;
    int ret;

    if (str == NULL || size == 0) {
        return 0;
    }

    va_start(ap, format);
    ret = internal_vsnprintf(str, size, format, ap);
    va_end(ap);

    return ret;
}

/*
 * kernel_vsnprintf
 * ----------------
 * Format string ke buffer dengan batas ukuran (variadic).
 */
int kernel_vsnprintf(char *str, ukuran_t size, const char *format,
                     va_list args)
{
    if (str == NULL || size == 0) {
        return 0;
    }

    return internal_vsnprintf(str, size, format, args);
}

/*
 * ============================================================================
 * FUNGSI UTILITAS KERNEL (KERNEL UTILITY FUNCTIONS)
 * ============================================================================
 */

/*
 * kernel_delay
 * ------------
 * Delay sederhana (busy wait).
 */
void kernel_delay(tak_bertanda32_t ms)
{
    hal_timer_sleep(ms);
}

/*
 * kernel_sleep
 * ------------
 * Sleep kernel untuk durasi tertentu.
 */
void kernel_sleep(tak_bertanda32_t ms)
{
    hal_timer_sleep(ms);
}

/*
 * kernel_get_uptime
 * -----------------
 * Dapatkan uptime sistem.
 */
tak_bertanda64_t kernel_get_uptime(void)
{
    return hal_timer_get_uptime();
}

/*
 * kernel_get_ticks
 * ----------------
 * Dapatkan jumlah timer ticks.
 */
tak_bertanda64_t kernel_get_ticks(void)
{
    return hal_timer_get_ticks();
}

/*
 * kernel_get_info
 * ---------------
 * Dapatkan informasi sistem.
 */
const info_sistem_t *kernel_get_info(void)
{
    return &g_info_sistem;
}

/*
 * kernel_get_arch
 * ---------------
 * Dapatkan nama arsitektur.
 */
const char *kernel_get_arch(void)
{
    return NAMA_ARSITEKTUR;
}

/*
 * kernel_get_version
 * ------------------
 * Dapatkan versi kernel.
 */
const char *kernel_get_version(void)
{
    return PIGURA_VERSI_STRING;
}

/*
 * kernel_get_hostname
 * -------------------
 * Dapatkan hostname sistem.
 */
const char *kernel_get_hostname(void)
{
    return g_info_sistem.hostname;
}

/*
 * kernel_set_hostname
 * -------------------
 * Set hostname sistem.
 */
status_t kernel_set_hostname(const char *hostname)
{
    ukuran_t len;

    if (hostname == NULL) {
        return STATUS_PARAM_NULL;
    }

    len = kernel_strlen(hostname);
    if (len == 0 || len >= sizeof(g_info_sistem.hostname)) {
        return STATUS_PARAM_UKURAN;
    }

    kernel_strncpy(g_info_sistem.hostname, hostname,
                   sizeof(g_info_sistem.hostname) - 1);
    g_info_sistem.hostname[sizeof(g_info_sistem.hostname) - 1] = '\0';

    return STATUS_BERHASIL;
}
