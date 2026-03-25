/*
 * PIGURA LIBC - MATH/MATH.C
 * =========================
 * Implementasi fungsi matematika.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 *
 * Implementasi menggunakan algoritma numerik
 * standar dengan presisi ganda (double).
 */

#include <math.h>
#include <errno.h>

/* ============================================================
 * KONSTANTA MATEMATIKA
 * ============================================================
 */

/* Konstanta pi dengan presisi tinggi */
#define M_PI_VALUE      3.14159265358979323846
#define M_PI_2_VALUE    1.57079632679489661923
#define M_PI_4_VALUE    0.78539816339744830962
#define M_1_PI_VALUE    0.31830988618379067154
#define M_2_PI_VALUE    0.63661977236758134308

/* Konstanta e (Euler's number) */
#define M_E_VALUE       2.7182818284590452354
#define M_LOG2E_VALUE   1.4426950408889634074
#define M_LOG10E_VALUE  0.43429448190325182765

/* Konstanta ln(2) dan ln(10) */
#define M_LN2_VALUE     0.69314718055994530942
#define M_LN10_VALUE    2.30258509299404568402

/* Batas untuk presisi */
#define TINY_VALUE      1.0e-300
#define HUGE_VALUE      1.0e300
#define EPSILON         2.2204460492503131e-16

/* Konstanta untuk sqrt(2) */
#define M_SQRT2_VALUE   1.41421356237309504880

/* ============================================================
 * VARIABEL GLOBAL
 * ============================================================
 */

/* Variabel untuk error handling */
int math_errhandling = MATH_ERRNO;

/* ============================================================
 * FUNGSI TRIGONOMETRI
 * ============================================================
 */

/*
 * sin - Fungsi sinus
 *
 * Parameter:
 *   x - Sudut dalam radian
 *
 * Return: Nilai sinus dari x
 */
double sin(double x) {
    double term;
    double sum;
    int n;
    int sign;

    /* Normalisasi ke range [-pi, pi] */
    if (x < 0.0) {
        x = -x;
        sign = -1;
    } else {
        sign = 1;
    }

    /* Reduce ke range [0, 2*pi] */
    x = x - ((long)(x / (2.0 * M_PI_VALUE))) * (2.0 * M_PI_VALUE);

    /* Reduce ke range [0, pi] */
    if (x > M_PI_VALUE) {
        x = 2.0 * M_PI_VALUE - x;
        sign = -sign;
    }

    /* Reduce ke range [0, pi/2] */
    if (x > M_PI_2_VALUE) {
        x = M_PI_VALUE - x;
        sign = -sign;
    }

    /* Gunakan Taylor series: sin(x) = x - x^3/3! + x^5/5! - ... */
    term = x;
    sum = x;

    for (n = 1; n < 20; n++) {
        term *= -x * x / ((2.0 * n) * (2.0 * n + 1.0));
        sum += term;

        /* Konvergensi check */
        if (term < EPSILON && term > -EPSILON) {
            break;
        }
    }

    return sign * sum;
}

/*
 * cos - Fungsi kosinus
 *
 * Parameter:
 *   x - Sudut dalam radian
 *
 * Return: Nilai kosinus dari x
 */
double cos(double x) {
    /* cos(x) = sin(x + pi/2) */
    return sin(x + M_PI_2_VALUE);
}

/*
 * tan - Fungsi tangen
 *
 * Parameter:
 *   x - Sudut dalam radian
 *
 * Return: Nilai tangen dari x
 */
double tan(double x) {
    double c;

    /* cos(x) = 0 pada pi/2 + k*pi */
    c = cos(x);

    if (c == 0.0) {
        errno = ERANGE;
        return (x > 0) ? HUGE_VAL : -HUGE_VAL;
    }

    return sin(x) / c;
}

/*
 * sinf - Fungsi sinus (float)
 */
float sinf(float x) {
    return (float)sin((double)x);
}

/*
 * cosf - Fungsi kosinus (float)
 */
float cosf(float x) {
    return (float)cos((double)x);
}

/*
 * tanf - Fungsi tangen (float)
 */
float tanf(float x) {
    return (float)tan((double)x);
}

/* ============================================================
 * FUNGSI INVERSE TRIGONOMETRI
 * ============================================================
 */

/*
 * asin - Fungsi arcsin
 *
 * Parameter:
 *   x - Nilai antara -1 dan 1
 *
 * Return: Sudut dalam radian [-pi/2, pi/2]
 */
double asin(double x) {
    double term;
    double sum;
    double x2;
    int n;

    /* Validasi range */
    if (x > 1.0 || x < -1.0) {
        errno = EDOM;
        return NAN;
    }

    /* Kasus khusus */
    if (x == 1.0) {
        return M_PI_2_VALUE;
    }
    if (x == -1.0) {
        return -M_PI_2_VALUE;
    }

    /* Untuk |x| kecil, gunakan Taylor series langsung */
    if (x < 0.5 && x > -0.5) {
        x2 = x * x;
        term = x;
        sum = x;

        for (n = 1; n < 30; n++) {
            term *= x2 * (2.0 * n - 1.0) * (2.0 * n - 1.0);
            term /= (2.0 * n) * (2.0 * n + 1.0);
            sum += term;

            if (term < EPSILON && term > -EPSILON) {
                break;
            }
        }

        return sum;
    }

    /* Untuk |x| besar, gunakan asin(x) = pi/2 - acos(x) */
    return M_PI_2_VALUE - acos(x);
}

/*
 * acos - Fungsi arccos
 *
 * Parameter:
 *   x - Nilai antara -1 dan 1
 *
 * Return: Sudut dalam radian [0, pi]
 */
double acos(double x) {
    double result;

    /* Validasi range */
    if (x > 1.0 || x < -1.0) {
        errno = EDOM;
        return NAN;
    }

    /* Kasus khusus */
    if (x == 1.0) {
        return 0.0;
    }
    if (x == -1.0) {
        return M_PI_VALUE;
    }

    /* acos(x) = pi/2 - asin(x) */
    result = M_PI_2_VALUE - asin(x);

    return result;
}

/*
 * atan - Fungsi arctan
 *
 * Parameter:
 *   x - Nilai apapun
 *
 * Return: Sudut dalam radian [-pi/2, pi/2]
 */
double atan(double x) {
    double term;
    double sum;
    double x2;
    int n;
    int sign;

    /* Handle infinity */
    if (x > HUGE_VALUE) {
        return M_PI_2_VALUE;
    }
    if (x < -HUGE_VALUE) {
        return -M_PI_2_VALUE;
    }

    /* Normalisasi sign */
    if (x < 0.0) {
        x = -x;
        sign = -1;
    } else {
        sign = 1;
    }

    /* Untuk x > 1, gunakan atan(x) = pi/2 - atan(1/x) */
    if (x > 1.0) {
        return sign * (M_PI_2_VALUE - atan(1.0 / x));
    }

    /* Untuk x mendekati 1, gunakan identitas lain */
    if (x > 0.5) {
        /* atan(x) = pi/4 + atan((x-1)/(x+1)) */
        return sign * (M_PI_4_VALUE + atan((x - 1.0) / (x + 1.0)));
    }

    /* Taylor series untuk |x| <= 0.5 */
    x2 = x * x;
    term = x;
    sum = x;

    for (n = 1; n < 30; n++) {
        term *= -x2;
        sum += term / (2.0 * n + 1.0);

        if (term < EPSILON && term > -EPSILON) {
            break;
        }
    }

    return sign * sum;
}

/*
 * atan2 - Fungsi arctan dengan kuadran
 *
 * Parameter:
 *   y - Koordinat y
 *   x - Koordinat x
 *
 * Return: Sudut dalam radian [-pi, pi]
 */
double atan2(double y, double x) {
    double result;

    /* Kasus khusus */
    if (x == 0.0 && y == 0.0) {
        errno = EDOM;
        return 0.0;
    }

    if (x == 0.0) {
        return (y > 0.0) ? M_PI_2_VALUE : -M_PI_2_VALUE;
    }

    if (y == 0.0) {
        return (x > 0.0) ? 0.0 : M_PI_VALUE;
    }

    /* Hitung atan(y/x) */
    result = atan(y / x);

    /* Koreksi kuadran */
    if (x < 0.0) {
        if (y > 0.0) {
            result += M_PI_VALUE;
        } else {
            result -= M_PI_VALUE;
        }
    }

    return result;
}

/* ============================================================
 * FUNGSI AKAR
 * ============================================================
 */

/*
 * sqrt - Fungsi akar kuadrat
 *
 * Parameter:
 *   x - Nilai non-negatif
 *
 * Return: Akar kuadrat dari x
 */
double sqrt(double x) {
    double result;
    double prev;
    int i;

    /* Validasi */
    if (x < 0.0) {
        errno = EDOM;
        return NAN;
    }

    /* Kasus khusus */
    if (x == 0.0 || x == 1.0) {
        return x;
    }

    /* Handle infinity */
    if (x > HUGE_VALUE) {
        return HUGE_VAL;
    }

    /* Newton-Raphson method */
    /* Initial guess */
    result = x / 2.0;

    /* Iterasi hingga konvergen */
    for (i = 0; i < 50; i++) {
        prev = result;
        result = 0.5 * (result + x / result);

        /* Konvergensi check */
        if (result == prev) {
            break;
        }

        if (result - prev < EPSILON && result - prev > -EPSILON) {
            break;
        }
    }

    return result;
}

/*
 * cbrt - Fungsi akar kubik
 *
 * Parameter:
 *   x - Nilai apapun
 *
 * Return: Akar kubik dari x
 */
double cbrt(double x) {
    double result;
    double prev;
    int sign;
    int i;

    /* Kasus khusus */
    if (x == 0.0) {
        return 0.0;
    }

    /* Handle sign */
    if (x < 0.0) {
        x = -x;
        sign = -1;
    } else {
        sign = 1;
    }

    /* Newton-Raphson untuk x^(1/3) */
    result = x / 3.0;

    for (i = 0; i < 50; i++) {
        prev = result;
        result = (2.0 * result + x / (result * result)) / 3.0;

        if (result - prev < EPSILON && result - prev > -EPSILON) {
            break;
        }
    }

    return sign * result;
}

/* ============================================================
 * FUNGSI PANGKAT DAN LOGARITMA
 * ============================================================
 */

/*
 * pow - Fungsi pangkat
 *
 * Parameter:
 *   x - Basis
 *   y - Eksponen
 *
 * Return: x^y
 */
double pow(double x, double y) {
    double result;
    int i;
    int exp_int;

    /* Kasus khusus */
    if (x == 0.0) {
        if (y > 0.0) {
            return 0.0;
        }
        if (y < 0.0) {
            errno = ERANGE;
            return HUGE_VAL;
        }
        return 1.0; /* 0^0 = 1 by convention */
    }

    if (x == 1.0) {
        return 1.0;
    }

    if (y == 0.0) {
        return 1.0;
    }

    if (y == 1.0) {
        return x;
    }

    if (y == -1.0) {
        return 1.0 / x;
    }

    /* Eksponen integer */
    if (y == (double)(exp_int = (int)y)) {
        double base;
        int neg_exp;

        if (exp_int < 0) {
            exp_int = -exp_int;
            neg_exp = 1;
        } else {
            neg_exp = 0;
        }

        result = 1.0;
        base = x;

        /* Binary exponentiation */
        while (exp_int > 0) {
            if (exp_int & 1) {
                result *= base;
            }
            base *= base;
            exp_int >>= 1;
        }

        if (neg_exp) {
            result = 1.0 / result;
        }

        return result;
    }

    /* x < 0 dengan eksponen non-integer */
    if (x < 0.0) {
        errno = EDOM;
        return NAN;
    }

    /* Gunakan identitas: x^y = exp(y * ln(x)) */
    result = exp(y * log(x));

    return result;
}

/*
 * exp - Fungsi eksponensial (e^x)
 *
 * Parameter:
 *   x - Eksponen
 *
 * Return: e^x
 */
double exp(double x) {
    double term;
    double sum;
    int n;
    int neg;

    /* Handle overflow/underflow */
    if (x > 709.78) {
        errno = ERANGE;
        return HUGE_VAL;
    }

    if (x < -745.13) {
        errno = ERANGE;
        return 0.0;
    }

    /* Handle sign */
    if (x < 0.0) {
        x = -x;
        neg = 1;
    } else {
        neg = 0;
    }

    /* Reduce: x = k + r where k integer, r < 1 */
    /* e^x = e^k * e^r */
    /* e^k dihitung terpisah */

    /* Taylor series: e^r = 1 + r + r^2/2! + r^3/3! + ... */
    term = 1.0;
    sum = 1.0;

    for (n = 1; n < 30; n++) {
        term *= x / (double)n;
        sum += term;

        if (term < EPSILON) {
            break;
        }
    }

    if (neg) {
        sum = 1.0 / sum;
    }

    return sum;
}

/*
 * log - Fungsi logaritma natural (ln)
 *
 * Parameter:
 *   x - Nilai positif
 *
 * Return: ln(x)
 */
double log(double x) {
    double result;
    double term;
    double y;
    int n;

    /* Validasi */
    if (x <= 0.0) {
        errno = EDOM;
        if (x == 0.0) {
            return -HUGE_VAL;
        }
        return NAN;
    }

    /* Kasus khusus */
    if (x == 1.0) {
        return 0.0;
    }

    /* Reduce range: x = 2^k * m where 0.5 <= m < 1 */
    /* ln(x) = k * ln(2) + ln(m) */
    /* Atau gunakan x = (1+y)/(1-y) => y = (x-1)/(x+1) */
    /* ln(x) = 2 * (y + y^3/3 + y^5/5 + ...) */

    y = (x - 1.0) / (x + 1.0);

    result = 0.0;
    term = y;
    n = 1;

    while (n < 100) {
        result += term / (double)n;
        term *= y * y;

        if (term < EPSILON && term > -EPSILON) {
            break;
        }

        n += 2;
    }

    return 2.0 * result;
}

/*
 * log10 - Fungsi logaritma basis 10
 *
 * Parameter:
 *   x - Nilai positif
 *
 * Return: log10(x)
 */
double log10(double x) {
    /* log10(x) = ln(x) / ln(10) */
    return log(x) / M_LN10_VALUE;
}

/*
 * log2 - Fungsi logaritma basis 2
 *
 * Parameter:
 *   x - Nilai positif
 *
 * Return: log2(x)
 */
double log2(double x) {
    /* log2(x) = ln(x) / ln(2) */
    return log(x) / M_LN2_VALUE;
}

/* ============================================================
 * FUNGSI PEMBULATAN
 * ============================================================
 */

/*
 * ceil - Pembulatan ke atas
 *
 * Parameter:
 *   x - Nilai apapun
 *
 * Return: Bilangan bulat terkecil >= x
 */
double ceil(double x) {
    double int_part;

    /* Handle NaN/Inf */
    if (x != x) {
        return x;
    }

    /* Ambil bagian integer */
    int_part = (double)(long long)x;

    /* Jika sudah integer, return */
    if (x == int_part) {
        return int_part;
    }

    /* Jika positif, tambah 1 */
    if (x > 0.0) {
        return int_part + 1.0;
    }

    /* Jika negatif, return bagian integer */
    return int_part;
}

/*
 * floor - Pembulatan ke bawah
 *
 * Parameter:
 *   x - Nilai apapun
 *
 * Return: Bilangan bulat terbesar <= x
 */
double floor(double x) {
    double int_part;

    /* Handle NaN/Inf */
    if (x != x) {
        return x;
    }

    /* Ambil bagian integer */
    int_part = (double)(long long)x;

    /* Jika sudah integer, return */
    if (x == int_part) {
        return int_part;
    }

    /* Jika positif, return bagian integer */
    if (x > 0.0) {
        return int_part;
    }

    /* Jika negatif, kurang 1 */
    return int_part - 1.0;
}

/*
 * round - Pembulatan ke integer terdekat
 *
 * Parameter:
 *   x - Nilai apapun
 *
 * Return: Integer terdekat
 */
double round(double x) {
    double int_part;
    double frac;

    /* Handle NaN/Inf */
    if (x != x) {
        return x;
    }

    int_part = floor(x);
    frac = x - int_part;

    if (frac >= 0.5) {
        return int_part + 1.0;
    }

    if (frac <= -0.5) {
        return int_part - 1.0;
    }

    return int_part;
}

/*
 * trunc - Potong bagian pecahan
 *
 * Parameter:
 *   x - Nilai apapun
 *
 * Return: Bagian integer dari x
 */
double trunc(double x) {
    /* Handle NaN/Inf */
    if (x != x) {
        return x;
    }

    if (x >= 0.0) {
        return floor(x);
    }

    return ceil(x);
}

/* ============================================================
 * FUNGSI NILAI ABSOLUT DAN LAINNYA
 * ============================================================
 */

/*
 * fabs - Nilai absolut
 *
 * Parameter:
 *   x - Nilai apapun
 *
 * Return: |x|
 */
double fabs(double x) {
    if (x < 0.0) {
        return -x;
    }
    return x;
}

/*
 * fmod - Sisa pembagian floating point
 *
 * Parameter:
 *   x - Pembilang
 *   y - Penyebut
 *
 * Return: x - n*y untuk suatu integer n
 */
double fmod(double x, double y) {
    double quotient;
    double result;

    /* Validasi */
    if (y == 0.0) {
        errno = EDOM;
        return NAN;
    }

    /* Handle NaN/Inf */
    if (x != x || y != y) {
        return NAN;
    }

    quotient = x / y;
    result = x - trunc(quotient) * y;

    return result;
}

/*
 * modf - Pisahkan bagian integer dan pecahan
 *
 * Parameter:
 *   x   - Nilai yang dipisah
 *   iptr - Pointer untuk bagian integer
 *
 * Return: Bagian pecahan
 */
double modf(double x, double *iptr) {
    double int_part;

    /* Handle NaN/Inf */
    if (x != x) {
        if (iptr != (double *)0) {
            *iptr = x;
        }
        return x;
    }

    int_part = trunc(x);

    if (iptr != (double *)0) {
        *iptr = int_part;
    }

    return x - int_part;
}

/*
 * frexp - Pisahkan mantissa dan eksponen
 *
 * Parameter:
 *   x   - Nilai yang dipisah
 *   exp - Pointer untuk eksponen
 *
 * Return: Mantissa dalam [0.5, 1) atau 0
 */
double frexp(double x, int *exp) {
    int e;
    double m;

    /* Handle zero */
    if (x == 0.0) {
        if (exp != (int *)0) {
            *exp = 0;
        }
        return 0.0;
    }

    /* Handle NaN/Inf */
    if (x != x || x > HUGE_VALUE || x < -HUGE_VALUE) {
        if (exp != (int *)0) {
            *exp = 0;
        }
        return x;
    }

    /* Dapatkan eksponen */
    e = 0;
    m = fabs(x);

    /* Normalisasi ke [0.5, 1) */
    while (m >= 1.0) {
        m *= 0.5;
        e++;
    }

    while (m < 0.5 && m > 0.0) {
        m *= 2.0;
        e--;
    }

    if (x < 0.0) {
        m = -m;
    }

    if (exp != (int *)0) {
        *exp = e;
    }

    return m;
}

/*
 * ldexp - Kalikan dengan 2^exp
 *
 * Parameter:
 *   x   - Nilai awal
 *   exp - Eksponen
 *
 * Return: x * 2^exp
 */
double ldexp(double x, int exp) {
    double factor;
    int i;

    /* Handle zero/NaN/Inf */
    if (x == 0.0 || x != x) {
        return x;
    }

    /* Hitung 2^exp */
    if (exp >= 0) {
        factor = 1.0;
        for (i = 0; i < exp; i++) {
            factor *= 2.0;
            /* Overflow check */
            if (factor > HUGE_VALUE) {
                errno = ERANGE;
                return (x > 0) ? HUGE_VAL : -HUGE_VAL;
            }
        }
    } else {
        factor = 1.0;
        for (i = 0; i > exp; i--) {
            factor *= 0.5;
        }
    }

    return x * factor;
}

/* ============================================================
 * FUNGSI HIPERBOLIK
 * ============================================================
 */

/*
 * sinh - Fungsi sinus hiperbolic
 */
double sinh(double x) {
    double epx;

    /* Handle overflow */
    if (x > 710.0 || x < -710.0) {
        errno = ERANGE;
        return (x > 0) ? HUGE_VAL : -HUGE_VAL;
    }

    epx = exp(x);
    return 0.5 * (epx - 1.0 / epx);
}

/*
 * cosh - Fungsi kosinus hiperbolic
 */
double cosh(double x) {
    double epx;

    /* Handle overflow */
    if (x > 710.0 || x < -710.0) {
        errno = ERANGE;
        return HUGE_VAL;
    }

    epx = exp(fabs(x));
    return 0.5 * (epx + 1.0 / epx);
}

/*
 * tanh - Fungsi tangen hiperbolic
 */
double tanh(double x) {
    double epx;

    /* Handle large values */
    if (x > 20.0) {
        return 1.0;
    }
    if (x < -20.0) {
        return -1.0;
    }

    epx = exp(x);
    return (epx - 1.0 / epx) / (epx + 1.0 / epx);
}

/* ============================================================
 * FUNGSI UTILITAS LAINNYA
 * ============================================================
 */

/*
 * isnan - Cek apakah NaN
 */
int isnan(double x) {
    return x != x;
}

/*
 * isinf - Cek apakah infinity
 */
int isinf(double x) {
    if (x > HUGE_VALUE) {
        return 1;
    }
    if (x < -HUGE_VALUE) {
        return -1;
    }
    return 0;
}

/*
 * isfinite - Cek apakah finite
 */
int isfinite(double x) {
    return !isnan(x) && !isinf(x);
}

/*
 * fpclassify - Klasifikasi floating point
 */
int fpclassify(double x) {
    if (x == 0.0) {
        return FP_ZERO;
    }
    if (isnan(x)) {
        return FP_NAN;
    }
    if (isinf(x)) {
        return FP_INFINITE;
    }
    /* Subnormal check */
    if (fabs(x) < TINY_VALUE && x != 0.0) {
        return FP_SUBNORMAL;
    }
    return FP_NORMAL;
}

/*
 * copysign - Copy sign
 */
double copysign(double x, double y) {
    if (y >= 0.0) {
        return fabs(x);
    }
    return -fabs(x);
}

/*
 * signbit - Cek sign bit
 */
int signbit(double x) {
    return x < 0.0;
}

/*
 * fmax - Nilai maksimum
 */
double fmax(double x, double y) {
    if (isnan(x)) {
        return y;
    }
    if (isnan(y)) {
        return x;
    }
    return (x > y) ? x : y;
}

/*
 * fmin - Nilai minimum
 */
double fmin(double x, double y) {
    if (isnan(x)) {
        return y;
    }
    if (isnan(y)) {
        return x;
    }
    return (x < y) ? x : y;
}

/*
 * fdim - Positive difference
 */
double fdim(double x, double y) {
    if (isnan(x) || isnan(y)) {
        return NAN;
    }
    if (x > y) {
        return x - y;
    }
    return 0.0;
}

/*
 * fma - Fused multiply-add
 */
double fma(double x, double y, double z) {
    /* Simple implementation tanpa hardware support */
    return x * y + z;
}

/*
 * hypot - Euclidean distance
 */
double hypot(double x, double y) {
    double ax = fabs(x);
    double ay = fabs(y);

    if (ax > ay) {
        return ax * sqrt(1.0 + (ay / ax) * (ay / ax));
    }
    if (ay == 0.0) {
        return ax;
    }
    return ay * sqrt(1.0 + (ax / ay) * (ax / ay));
}

/*
 * nextafter - Next representable value
 */
double nextafter(double x, double y) {
    /* Simplified implementation */
    if (x == y) {
        return y;
    }
    if (isnan(x) || isnan(y)) {
        return NAN;
    }

    /* Ini adalah implementasi sederhana */
    if (x < y) {
        return x + EPSILON;
    }
    return x - EPSILON;
}

/*
 * nan - Generate NaN
 */
double nan(const char *tagp) {
    (void)tagp;
    return NAN;
}
