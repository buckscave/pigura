/*
 * PIGURA LIBC - STDLIB/CONV.C
 * ============================
 * Implementasi fungsi konversi string ke angka dan sebaliknya.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 *
 * Fungsi yang diimplementasikan:
 *   - atoi(), atol(), atoll()  : Konversi string ke integer
 *   - strtol(), strtoul()      : Konversi dengan error handling
 *   - strtoll(), strtoull()    : Konversi long long
 *   - atof(), strtod()         : Konversi ke floating point
 *   - itoa(), ltoa()           : Konversi integer ke string
 *   - utoa()                   : Konversi unsigned ke string
 */

#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

/* ============================================================
 * KONFIGURASI
 * ============================================================
 */

/* Nilai maksimum untuk overflow detection */
#define LONG_MAX_DIGITS    19
#define LLONG_MAX_DIGITS   19

/* ============================================================
 * IMPLEMENTASI FUNGSI ATO* (SIMPLE CONVERSION)
 * ============================================================
 */

/*
 * atoi - Konversi string ke int
 *
 * Parameter:
 *   nptr - String input
 *
 * Return: Nilai int hasil konversi
 *
 * Catatan: Tidak ada error handling. Overflow undefined.
 *          Untuk error handling, gunakan strtol().
 */
int atoi(const char *nptr) {
    int result = 0;
    int sign = 1;

    /* Skip whitespace */
    while (isspace((unsigned char)*nptr)) {
        nptr++;
    }

    /* Parse sign */
    if (*nptr == '-') {
        sign = -1;
        nptr++;
    } else if (*nptr == '+') {
        nptr++;
    }

    /* Parse digits */
    while (isdigit((unsigned char)*nptr)) {
        result = result * 10 + (*nptr - '0');
        nptr++;
    }

    return sign * result;
}

/*
 * atol - Konversi string ke long
 *
 * Parameter:
 *   nptr - String input
 *
 * Return: Nilai long hasil konversi
 */
long atol(const char *nptr) {
    long result = 0;
    int sign = 1;

    while (isspace((unsigned char)*nptr)) {
        nptr++;
    }

    if (*nptr == '-') {
        sign = -1;
        nptr++;
    } else if (*nptr == '+') {
        nptr++;
    }

    while (isdigit((unsigned char)*nptr)) {
        result = result * 10 + (*nptr - '0');
        nptr++;
    }

    return sign * result;
}

/*
 * atoll - Konversi string ke long long (C99)
 *
 * Parameter:
 *   nptr - String input
 *
 * Return: Nilai long long hasil konversi
 */
long long atoll(const char *nptr) {
    long long result = 0;
    int sign = 1;

    while (isspace((unsigned char)*nptr)) {
        nptr++;
    }

    if (*nptr == '-') {
        sign = -1;
        nptr++;
    } else if (*nptr == '+') {
        nptr++;
    }

    while (isdigit((unsigned char)*nptr)) {
        result = result * 10 + (*nptr - '0');
        nptr++;
    }

    return sign * result;
}

/*
 * atof - Konversi string ke double
 *
 * Parameter:
 *   nptr - String input
 *
 * Return: Nilai double hasil konversi
 */
double atof(const char *nptr) {
    return strtod(nptr, (char **)NULL);
}

/* ============================================================
 * IMPLEMENTASI FUNGSI STRTO* (WITH ERROR HANDLING)
 * ============================================================
 */

/*
 * strtol - Konversi string ke long dengan error handling
 *
 * Parameter:
 *   nptr   - String input
 *   endptr - Pointer untuk menyimpan posisi akhir (boleh NULL)
 *   base   - Basis angka (0, 2-36)
 *
 * Return: Nilai long hasil konversi
 *
 * Catatan: Set errno ke ERANGE jika overflow.
 */
long strtol(const char *nptr, char **endptr, int base) {
    const char *s = nptr;
    long result = 0;
    int sign = 1;
    int digit;
    int overflow = 0;
    long cutoff;
    int cutlim;

    /* Skip whitespace */
    while (isspace((unsigned char)*s)) {
        s++;
    }

    /* Parse sign */
    if (*s == '-') {
        sign = -1;
        s++;
    } else if (*s == '+') {
        s++;
    }

    /* Parse base */
    if (base == 0) {
        if (*s == '0') {
            if (s[1] == 'x' || s[1] == 'X') {
                base = 16;
                s += 2;
            } else {
                base = 8;
                s++;
            }
        } else {
            base = 10;
        }
    } else if (base == 16) {
        if (*s == '0' && (s[1] == 'x' || s[1] == 'X')) {
            s += 2;
        }
    } else if (base < 2 || base > 36) {
        errno = EINVAL;
        if (endptr != NULL) {
            *endptr = (char *)nptr;
        }
        return 0;
    }

    /* Calculate cutoff for overflow detection */
    if (sign == 1) {
        cutoff = LONG_MAX / base;
        cutlim = LONG_MAX % base;
    } else {
        cutoff = -(LONG_MIN / base);
        cutlim = -(LONG_MIN % base);
    }

    /* Parse digits */
    while (*s != '\0') {
        if (isdigit((unsigned char)*s)) {
            digit = *s - '0';
        } else if (isupper((unsigned char)*s)) {
            digit = *s - 'A' + 10;
        } else if (islower((unsigned char)*s)) {
            digit = *s - 'a' + 10;
        } else {
            break;
        }

        if (digit >= base) {
            break;
        }

        /* Check overflow */
        if (overflow) {
            s++;
            continue;
        }

        if (result > cutoff || (result == cutoff && digit > cutlim)) {
            overflow = 1;
            s++;
            continue;
        }

        result = result * base + digit;
        s++;
    }

    /* Set endptr */
    if (endptr != NULL) {
        if (s == nptr) {
            *endptr = (char *)nptr;
        } else {
            *endptr = (char *)s;
        }
    }

    /* Handle overflow */
    if (overflow) {
        errno = ERANGE;
        if (sign == 1) {
            return LONG_MAX;
        } else {
            return LONG_MIN;
        }
    }

    return sign * result;
}

/*
 * strtoul - Konversi string ke unsigned long
 *
 * Parameter:
 *   nptr   - String input
 *   endptr - Pointer untuk menyimpan posisi akhir
 *   base   - Basis angka (0, 2-36)
 *
 * Return: Nilai unsigned long hasil konversi
 */
unsigned long strtoul(const char *nptr, char **endptr, int base) {
    const char *s = nptr;
    unsigned long result = 0;
    int sign = 1;
    int digit;
    int overflow = 0;
    unsigned long cutoff;
    int cutlim;
    const char *start;

    /* Skip whitespace */
    while (isspace((unsigned char)*s)) {
        s++;
    }

    /* Parse sign */
    if (*s == '-') {
        sign = -1;
        s++;
    } else if (*s == '+') {
        s++;
    }

    start = s;

    /* Parse base */
    if (base == 0) {
        if (*s == '0') {
            if (s[1] == 'x' || s[1] == 'X') {
                base = 16;
                s += 2;
            } else {
                base = 8;
                s++;
            }
        } else {
            base = 10;
        }
    } else if (base == 16) {
        if (*s == '0' && (s[1] == 'x' || s[1] == 'X')) {
            s += 2;
        }
    } else if (base < 2 || base > 36) {
        errno = EINVAL;
        if (endptr != NULL) {
            *endptr = (char *)nptr;
        }
        return 0;
    }

    /* Calculate cutoff */
    cutoff = ULONG_MAX / base;
    cutlim = ULONG_MAX % base;

    /* Parse digits */
    while (*s != '\0') {
        if (isdigit((unsigned char)*s)) {
            digit = *s - '0';
        } else if (isupper((unsigned char)*s)) {
            digit = *s - 'A' + 10;
        } else if (islower((unsigned char)*s)) {
            digit = *s - 'a' + 10;
        } else {
            break;
        }

        if (digit >= base) {
            break;
        }

        if (overflow) {
            s++;
            continue;
        }

        if (result > cutoff || (result == cutoff && digit > cutlim)) {
            overflow = 1;
            s++;
            continue;
        }

        result = result * base + digit;
        s++;
    }

    /* Set endptr */
    if (endptr != NULL) {
        if (s == nptr && start == nptr) {
            *endptr = (char *)nptr;
        } else {
            *endptr = (char *)s;
        }
    }

    /* Handle overflow */
    if (overflow) {
        errno = ERANGE;
        return ULONG_MAX;
    }

    /* Handle negative */
    if (sign == -1) {
        result = (unsigned long)(-(long)result);
    }

    return result;
}

/*
 * strtoll - Konversi string ke long long (C99)
 */
long long strtoll(const char *nptr, char **endptr, int base) {
    const char *s = nptr;
    long long result = 0;
    int sign = 1;
    int digit;
    int overflow = 0;
    long long cutoff;
    int cutlim;

    while (isspace((unsigned char)*s)) {
        s++;
    }

    if (*s == '-') {
        sign = -1;
        s++;
    } else if (*s == '+') {
        s++;
    }

    if (base == 0) {
        if (*s == '0') {
            if (s[1] == 'x' || s[1] == 'X') {
                base = 16;
                s += 2;
            } else {
                base = 8;
                s++;
            }
        } else {
            base = 10;
        }
    } else if (base == 16) {
        if (*s == '0' && (s[1] == 'x' || s[1] == 'X')) {
            s += 2;
        }
    } else if (base < 2 || base > 36) {
        errno = EINVAL;
        if (endptr != NULL) {
            *endptr = (char *)nptr;
        }
        return 0;
    }

    if (sign == 1) {
        cutoff = LLONG_MAX / base;
        cutlim = LLONG_MAX % base;
    } else {
        cutoff = -(LLONG_MIN / base);
        cutlim = -(LLONG_MIN % base);
    }

    while (*s != '\0') {
        if (isdigit((unsigned char)*s)) {
            digit = *s - '0';
        } else if (isupper((unsigned char)*s)) {
            digit = *s - 'A' + 10;
        } else if (islower((unsigned char)*s)) {
            digit = *s - 'a' + 10;
        } else {
            break;
        }

        if (digit >= base) {
            break;
        }

        if (overflow) {
            s++;
            continue;
        }

        if (result > cutoff || (result == cutoff && digit > cutlim)) {
            overflow = 1;
            s++;
            continue;
        }

        result = result * base + digit;
        s++;
    }

    if (endptr != NULL) {
        *endptr = (s == nptr) ? (char *)nptr : (char *)s;
    }

    if (overflow) {
        errno = ERANGE;
        return (sign == 1) ? LLONG_MAX : LLONG_MIN;
    }

    return sign * result;
}

/*
 * strtoull - Konversi string ke unsigned long long (C99)
 */
unsigned long long strtoull(const char *nptr, char **endptr, int base) {
    const char *s = nptr;
    unsigned long long result = 0;
    int sign = 1;
    int digit;
    int overflow = 0;
    unsigned long long cutoff;
    int cutlim;
    const char *start;

    while (isspace((unsigned char)*s)) {
        s++;
    }

    if (*s == '-') {
        sign = -1;
        s++;
    } else if (*s == '+') {
        s++;
    }

    start = s;

    if (base == 0) {
        if (*s == '0') {
            if (s[1] == 'x' || s[1] == 'X') {
                base = 16;
                s += 2;
            } else {
                base = 8;
                s++;
            }
        } else {
            base = 10;
        }
    } else if (base == 16) {
        if (*s == '0' && (s[1] == 'x' || s[1] == 'X')) {
            s += 2;
        }
    } else if (base < 2 || base > 36) {
        errno = EINVAL;
        if (endptr != NULL) {
            *endptr = (char *)nptr;
        }
        return 0;
    }

    cutoff = ULLONG_MAX / base;
    cutlim = ULLONG_MAX % base;

    while (*s != '\0') {
        if (isdigit((unsigned char)*s)) {
            digit = *s - '0';
        } else if (isupper((unsigned char)*s)) {
            digit = *s - 'A' + 10;
        } else if (islower((unsigned char)*s)) {
            digit = *s - 'a' + 10;
        } else {
            break;
        }

        if (digit >= base) {
            break;
        }

        if (overflow) {
            s++;
            continue;
        }

        if (result > cutoff || (result == cutoff && digit > cutlim)) {
            overflow = 1;
            s++;
            continue;
        }

        result = result * base + digit;
        s++;
    }

    if (endptr != NULL) {
        *endptr = (s == start && start == nptr) ?
                  (char *)nptr : (char *)s;
    }

    if (overflow) {
        errno = ERANGE;
        return ULLONG_MAX;
    }

    if (sign == -1) {
        result = (unsigned long long)(-(long long)result);
    }

    return result;
}

/* ============================================================
 * IMPLEMENTASI FUNGSI STRTOD
 * ============================================================
 */

/*
 * strtod - Konversi string ke double
 *
 * Parameter:
 *   nptr   - String input
 *   endptr - Pointer untuk menyimpan posisi akhir
 *
 * Return: Nilai double hasil konversi
 */
double strtod(const char *nptr, char **endptr) {
    const char *s = nptr;
    double result = 0.0;
    double fraction = 0.0;
    double divisor = 10.0;
    int sign = 1;
    int exp_sign = 1;
    int exponent = 0;
    int has_digit = 0;

    /* Skip whitespace */
    while (isspace((unsigned char)*s)) {
        s++;
    }

    /* Parse sign */
    if (*s == '-') {
        sign = -1;
        s++;
    } else if (*s == '+') {
        s++;
    }

    /* Parse integer part */
    while (isdigit((unsigned char)*s)) {
        result = result * 10.0 + (*s - '0');
        s++;
        has_digit = 1;
    }

    /* Parse decimal point */
    if (*s == '.') {
        s++;

        while (isdigit((unsigned char)*s)) {
            fraction = fraction + (*s - '0') / divisor;
            divisor *= 10.0;
            s++;
            has_digit = 1;
        }

        result += fraction;
    }

    /* Parse exponent */
    if ((*s == 'e' || *s == 'E') && has_digit) {
        const char *exp_start = s;
        s++;

        if (*s == '-') {
            exp_sign = -1;
            s++;
        } else if (*s == '+') {
            s++;
        }

        if (isdigit((unsigned char)*s)) {
            while (isdigit((unsigned char)*s)) {
                exponent = exponent * 10 + (*s - '0');
                if (exponent > 308) {
                    /* Overflow */
                    exponent = 308;
                }
                s++;
            }

            /* Apply exponent */
            {
                double exp_val = 1.0;
                int i;
                for (i = 0; i < exponent; i++) {
                    if (exp_sign == 1) {
                        exp_val *= 10.0;
                    } else {
                        exp_val /= 10.0;
                    }
                }
                result *= exp_val;
            }
        } else {
            /* Invalid exponent, rollback */
            s = exp_start;
        }
    }

    /* Set endptr */
    if (endptr != NULL) {
        if (!has_digit) {
            *endptr = (char *)nptr;
        } else {
            *endptr = (char *)s;
        }
    }

    return sign * result;
}

/*
 * strtof - Konversi string ke float (C99)
 */
float strtof(const char *nptr, char **endptr) {
    return (float)strtod(nptr, endptr);
}

/*
 * strtold - Konversi string ke long double (C99)
 */
long double strtold(const char *nptr, char **endptr) {
    return (long double)strtod(nptr, endptr);
}

/* ============================================================
 * IMPLEMENTASI FUNGSI *TOA (NON-STANDAR)
 * ============================================================
 */

/*
 * Digit untuk konversi basis
 */
static const char conv_digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";

/*
 * utoa_internal - Konversi unsigned ke string (internal)
 */
static char *utoa_internal(unsigned long long value, char *str,
                           int base, int is_signed, int is_negative) {
    char *ptr;
    char *start;
    unsigned long long v;

    /* Validasi parameter */
    if (str == NULL || base < 2 || base > 36) {
        return str;
    }

    ptr = str;

    /* Handle negative */
    if (is_signed && is_negative) {
        *ptr++ = '-';
        value = (unsigned long long)(-(long long)value);
    }

    start = ptr;

    /* Handle zero */
    if (value == 0) {
        *ptr++ = '0';
    } else {
        /* Convert digits in reverse order */
        v = value;
        while (v > 0) {
            *ptr++ = conv_digits[v % base];
            v /= base;
        }
    }

    /* Null terminate */
    *ptr = '\0';

    /* Reverse the string */
    {
        char *end = ptr - 1;
        char temp;
        while (start < end) {
            temp = *start;
            *start = *end;
            *end = temp;
            start++;
            end--;
        }
    }

    return str;
}

/*
 * itoa - Konversi int ke string (NON-STANDAR)
 *
 * Parameter:
 *   value - Nilai yang dikonversi
 *   str   - Buffer tujuan
 *   base  - Basis output (2-36)
 *
 * Return: Pointer ke str
 */
char *itoa(int value, char *str, int base) {
    int is_negative = 0;

    if (str == NULL) {
        return NULL;
    }

    if (value < 0 && base == 10) {
        is_negative = 1;
    }

    return utoa_internal((unsigned long long)(unsigned int)value,
                         str, base, 1, is_negative);
}

/*
 * ltoa - Konversi long ke string (NON-STANDAR)
 */
char *ltoa(long value, char *str, int base) {
    int is_negative = 0;

    if (str == NULL) {
        return NULL;
    }

    if (value < 0 && base == 10) {
        is_negative = 1;
    }

    return utoa_internal((unsigned long long)(unsigned long)value,
                         str, base, 1, is_negative);
}

/*
 * lltoa - Konversi long long ke string (NON-STANDAR)
 */
char *lltoa(long long value, char *str, int base) {
    int is_negative = 0;

    if (str == NULL) {
        return NULL;
    }

    if (value < 0 && base == 10) {
        is_negative = 1;
    }

    return utoa_internal((unsigned long long)value, str, base,
                         1, is_negative);
}

/*
 * utoa - Konversi unsigned int ke string (NON-STANDAR)
 */
char *utoa(unsigned int value, char *str, int base) {
    if (str == NULL) {
        return NULL;
    }

    return utoa_internal((unsigned long long)value, str, base,
                         0, 0);
}

/*
 * ultoa - Konversi unsigned long ke string (NON-STANDAR)
 */
char *ultoa(unsigned long value, char *str, int base) {
    if (str == NULL) {
        return NULL;
    }

    return utoa_internal((unsigned long long)value, str, base,
                         0, 0);
}

/*
 * ulltoa - Konversi unsigned long long ke string (NON-STANDAR)
 */
char *ulltoa(unsigned long long value, char *str, int base) {
    if (str == NULL) {
        return NULL;
    }

    return utoa_internal(value, str, base, 0, 0);
}
