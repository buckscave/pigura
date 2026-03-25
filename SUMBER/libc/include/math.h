/*
 * PIGURA LIBC - MATH.H
 * =====================
 * Header untuk fungsi matematika sesuai standar C89.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 */

#ifndef LIBC_MATH_H
#define LIBC_MATH_H

/* ============================================================
 * KONSTANTA MATEMATIKA
 * ============================================================
 */

/* Konstanta fundamental */
#define M_E        2.71828182845904523536    /* e */
#define M_LOG2E    1.44269504088896340736    /* log2(e) */
#define M_LOG10E   0.434294481903251827651   /* log10(e) */
#define M_LN2      0.693147180559945309417   /* ln(2) */
#define M_LN10     2.30258509299404568402    /* ln(10) */
#define M_PI       3.14159265358979323846    /* pi */
#define M_PI_2     1.57079632679489661923    /* pi/2 */
#define M_PI_4     0.78539816339744830962    /* pi/4 */
#define M_1_PI     0.31830988618379067154    /* 1/pi */
#define M_2_PI     0.63661977236758134308    /* 2/pi */
#define M_2_SQRTPI 1.12837916709551257390    /* 2/sqrt(pi) */
#define M_SQRT2    1.41421356237309504880    /* sqrt(2) */
#define M_SQRT1_2  0.70710678118654752440    /* 1/sqrt(2) */

/* Nilai tak hingga dan NaN */
#define INFINITY   ((double)(1.0 / 0.0))
#define NAN        ((double)(0.0 / 0.0))

/* Nilai untuk klasifikasi floating-point */
#define FP_NAN       0
#define FP_INFINITE  1
#define FP_ZERO      2
#define FP_SUBNORMAL 3
#define FP_NORMAL    4

/* Makro untuk klasifikasi */
#define fpclassify(x) \
    (sizeof(x) == sizeof(float) ? __fpclassifyf(x) : \
     sizeof(x) == sizeof(double) ? __fpclassifyd(x) : \
     __fpclassifyld(x))

#define isfinite(x) (fpclassify(x) >= FP_ZERO)
#define isinf(x)    (fpclassify(x) == FP_INFINITE)
#define isnan(x)    (fpclassify(x) == FP_NAN)
#define isnormal(x) (fpclassify(x) == FP_NORMAL)

/* Makro signbit */
#define signbit(x) \
    (sizeof(x) == sizeof(float) ? __signbitf(x) : \
     sizeof(x) == sizeof(double) ? __signbitd(x) : \
     __signbitld(x))

/* Makro comparison */
#define isgreater(x, y)      ((x) > (y))
#define isgreaterequal(x, y) ((x) >= (y))
#define isless(x, y)         ((x) < (y))
#define islessequal(x, y)    ((x) <= (y))
#define islessgreater(x, y)  ((x) < (y) || (x) > (y))
#define isunordered(x, y)    (isnan(x) || isnan(y))

/* ============================================================
 * FUNGSI MATEMATIKA DASAR
 * ============================================================
 */

/*
 * sin - Sinus
 *
 * Parameter:
 *   x - Sudut dalam radian
 *
 * Return: Sinus dari x
 */
extern double sin(double x);

/*
 * cos - Cosinus
 *
 * Parameter:
 *   x - Sudut dalam radian
 *
 * Return: Cosinus dari x
 */
extern double cos(double x);

/*
 * tan - Tangen
 *
 * Parameter:
 *   x - Sudut dalam radian
 *
 * Return: Tangen dari x
 */
extern double tan(double x);

/*
 * asin - Arcus sinus
 *
 * Parameter:
 *   x - Nilai dalam rentang [-1, 1]
 *
 * Return: Arcus sinus dalam radian
 *
 * Catatan: Return NaN jika x di luar [-1, 1]
 */
extern double asin(double x);

/*
 * acos - Arcus cosinus
 *
 * Parameter:
 *   x - Nilai dalam rentang [-1, 1]
 *
 * Return: Arcus cosinus dalam radian
 *
 * Catatan: Return NaN jika x di luar [-1, 1]
 */
extern double acos(double x);

/*
 * atan - Arcus tangen
 *
 * Parameter:
 *   x - Nilai input
 *
 * Return: Arcus tangen dalam radian
 */
extern double atan(double x);

/*
 * atan2 - Arcus tangen dengan dua parameter
 *
 * Parameter:
 *   y - Koordinat y
 *   x - Koordinat x
 *
 * Return: Arcus tangen y/x dalam radian, dengan kuadran yang benar
 */
extern double atan2(double y, double x);

/* ============================================================
 * FUNGSI EKSPONENSIAL DAN LOGARITMA
 * ============================================================
 */

/*
 * exp - Eksponensial (e^x)
 *
 * Parameter:
 *   x - Eksponen
 *
 * Return: e pangkat x
 */
extern double exp(double x);

/*
 * exp2 - Eksponensial basis 2 (2^x)
 *
 * Parameter:
 *   x - Eksponen
 *
 * Return: 2 pangkat x
 */
extern double exp2(double x);

/*
 * expm1 - Eksponensial minus 1 (e^x - 1)
 *
 * Parameter:
 *   x - Eksponen
 *
 * Return: e^x - 1, dengan presisi lebih baik untuk x kecil
 */
extern double expm1(double x);

/*
 * log - Logaritma natural
 *
 * Parameter:
 *   x - Nilai positif
 *
 * Return: ln(x)
 *
 * Catatan: Domain error jika x <= 0
 */
extern double log(double x);

/*
 * log10 - Logaritma basis 10
 *
 * Parameter:
 *   x - Nilai positif
 *
 * Return: log10(x)
 */
extern double log10(double x);

/*
 * log2 - Logaritma basis 2
 *
 * Parameter:
 *   x - Nilai positif
 *
 * Return: log2(x)
 */
extern double log2(double x);

/*
 * log1p - Logaritma natural (1+x)
 *
 * Parameter:
 *   x - Nilai, x > -1
 *
 * Return: ln(1+x), dengan presisi lebih baik untuk x kecil
 */
extern double log1p(double x);

/*
 * logb - Ekstrak eksponen
 *
 * Parameter:
 *   x - Nilai floating-point
 *
 * Return: Eksponen dari representasi x
 */
extern double logb(double x);

/*
 * ilogb - Ekstrak eksponen sebagai integer
 *
 * Parameter:
 *   x - Nilai floating-point
 *
 * Return: Eksponen sebagai int
 */
extern int ilogb(double x);

/* ============================================================
 * FUNGSI PANGKAT DAN AKAR
 * ============================================================
 */

/*
 * pow - Pangkat
 *
 * Parameter:
 *   x - Basis
 *   y - Eksponen
 *
 * Return: x pangkat y
 */
extern double pow(double x, double y);

/*
 * sqrt - Akar kuadrat
 *
 * Parameter:
 *   x - Nilai non-negatif
 *
 * Return: Akar kuadrat dari x
 *
 * Catatan: Domain error jika x < 0
 */
extern double sqrt(double x);

/*
 * cbrt - Akar pangkat tiga
 *
 * Parameter:
 *   x - Nilai input
 *
 * Return: Akar pangkat tiga dari x
 */
extern double cbrt(double x);

/*
 * hypot - Sisi miring segitiga siku-siku
 *
 * Parameter:
 *   x - Sisi pertama
 *   y - Sisi kedua
 *
 * Return: sqrt(x^2 + y^2), tanpa overflow
 */
extern double hypot(double x, double y);

/* ============================================================
 * FUNGSI PEMBULATAN DAN ABSOLUT
 * ============================================================
 */

/*
 * ceil - Pembulatan ke atas
 *
 * Parameter:
 *   x - Nilai input
 *
 * Return: Bilangan bulat terkecil >= x
 */
extern double ceil(double x);

/*
 * floor - Pembulatan ke bawah
 *
 * Parameter:
 *   x - Nilai input
 *
 * Return: Bilangan bulat terbesar <= x
 */
extern double floor(double x);

/*
 * round - Pembulatan ke terdekat
 *
 * Parameter:
 *   x - Nilai input
 *
 * Return: Bilangan bulat terdekat
 */
extern double round(double x);

/*
 * roundf - Round untuk float
 */
extern float roundf(float x);

/*
 * roundl - Round untuk long double
 */
extern long double roundl(long double x);

/*
 * lround - Round ke long
 */
extern long lround(double x);

/*
 * llround - Round ke long long
 */
extern long long llround(double x);

/*
 * trunc - Truncate ke nol
 *
 * Parameter:
 *   x - Nilai input
 *
 * Return: Bagian integer dari x (ke arah nol)
 */
extern double trunc(double x);

/*
 * nearbyint - Round ke integer dengan rounding mode saat ini
 *
 * Parameter:
 *   x - Nilai input
 *
 * Return: Nilai integer terdekat
 */
extern double nearbyint(double x);

/*
 * rint - Round ke integer
 *
 * Parameter:
 *   x - Nilai input
 *
 * Return: Nilai integer terdekat
 */
extern double rint(double x);

/*
 * lrint - Round ke long integer
 */
extern long lrint(double x);

/*
 * llrint - Round ke long long integer
 */
extern long long llrint(double x);

/*
 * fabs - Nilai absolut
 *
 * Parameter:
 *   x - Nilai input
 *
 * Return: |x|
 */
extern double fabs(double x);

/*
 * fabsf - Nilai absolut untuk float
 */
extern float fabsf(float x);

/*
 * fabsl - Nilai absolut untuk long double
 */
extern long double fabsl(long double x);

/* ============================================================
 * FUNGSI LAINNYA
 * ============================================================
 */

/*
 * fmod - Sisa pembagian floating-point
 *
 * Parameter:
 *   x - Dividen
 *   y - Divisor
 *
 * Return: x - n*y, dimana n = trunc(x/y)
 *
 * Catatan: Hasil memiliki tanda yang sama dengan x
 */
extern double fmod(double x, double y);

/*
 * remainder - Sisa pembagian (IEEE 754)
 *
 * Parameter:
 *   x - Dividen
 *   y - Divisor
 *
 * Return: x - n*y, dimana n = round(x/y)
 */
extern double remainder(double x, double y);

/*
 * remquo - Sisa pembagian dengan quotient
 *
 * Parameter:
 *   x   - Dividen
 *   y   - Divisor
 *   quo - Pointer untuk menyimpan quotient
 *
 * Return: Sisa pembagian
 */
extern double remquo(double x, double y, int *quo);

/*
 * modf - Pecah menjadi bagian integer dan pecahan
 *
 * Parameter:
 *   x  - Nilai input
 *   iptr - Pointer untuk menyimpan bagian integer
 *
 * Return: Bagian pecahan (dengan tanda)
 */
extern double modf(double x, double *iptr);

/*
 * frexp - Pecah menjadi mantissa dan eksponen
 *
 * Parameter:
 *   x   - Nilai input
 *   exp - Pointer untuk menyimpan eksponen
 *
 * Return: Mantissa dalam [0.5, 1) atau [−1, −0.5]
 */
extern double frexp(double x, int *exp);

/*
 * ldexp - Kalikan dengan 2^exp
 *
 * Parameter:
 *   x   - Mantissa
 *   exp - Eksponen
 *
 * Return: x * 2^exp
 */
extern double ldexp(double x, int exp);

/*
 * scalbn - Kalikan dengan FLT_RADIX^n
 *
 * Parameter:
 *   x - Nilai input
 *   n - Eksponen
 *
 * Return: x * FLT_RADIX^n
 */
extern double scalbn(double x, int n);

/*
 * scalbln - Sama dengan scalbn, tapi n bertipe long
 */
extern double scalbln(double x, long n);

/*
 * copysign - Salin tanda
 *
 * Parameter:
 *   x - Magnitude
 *   y - Tanda
 *
 * Return: Nilai dengan magnitude x dan tanda y
 */
extern double copysign(double x, double y);

/*
 * nan - Menghasilkan quiet NaN
 *
 * Parameter:
 *   tagp - String tag (boleh NULL atau "")
 *
 * Return: NaN
 */
extern double nan(const char *tagp);

/*
 * nextafter - Nilai floating-point berikutnya
 *
 * Parameter:
 *   x - Nilai awal
 *   y - Arah
 *
 * Return: Nilai floating-point berikutnya dari x ke arah y
 */
extern double nextafter(double x, double y);

/*
 * nexttoward - Sama dengan nextafter, tapi y bertipe long double
 */
extern double nexttoward(double x, long double y);

/*
 * fdim - Positive difference
 *
 * Parameter:
 *   x - Nilai pertama
 *   y - Nilai kedua
 *
 * Return: max(x-y, 0)
 */
extern double fdim(double x, double y);

/*
 * fmax - Maksimum
 *
 * Parameter:
 *   x - Nilai pertama
 *   y - Nilai kedua
 *
 * Return: max(x, y)
 */
extern double fmax(double x, double y);

/*
 * fmin - Minimum
 *
 * Parameter:
 *   x - Nilai pertama
 *   y - Nilai kedua
 *
 * Return: min(x, y)
 */
extern double fmin(double x, double y);

/*
 * fma - Fused multiply-add
 *
 * Parameter:
 *   x - Faktor pertama
 *   y - Faktor kedua
 *   z - Addend
 *
 * Return: (x * y) + z, dengan satu pembulatan
 */
extern double fma(double x, double y, double z);

/* ============================================================
 * VERSI FLOAT DAN LONG DOUBLE
 * ============================================================
 */

/* Versi float */
extern float sinf(float x);
extern float cosf(float x);
extern float tanf(float x);
extern float asinf(float x);
extern float acosf(float x);
extern float atanf(float x);
extern float atan2f(float y, float x);
extern float expf(float x);
extern float exp2f(float x);
extern float expm1f(float x);
extern float logf(float x);
extern float log10f(float x);
extern float log2f(float x);
extern float log1pf(float x);
extern float logbf(float x);
extern int ilogbf(float x);
extern float powf(float x, float y);
extern float sqrtf(float x);
extern float cbrtf(float x);
extern float hypotf(float x, float y);
extern float ceilf(float x);
extern float floorf(float x);
extern float truncf(float x);
extern float nearbyintf(float x);
extern float rintf(float x);
extern long lrintf(float x);
extern long long llrintf(float x);
extern float fmodf(float x, float y);
extern float remainderf(float x, float y);
extern float remquof(float x, float y, int *quo);
extern float modff(float x, float *iptr);
extern float frexpf(float x, int *exp);
extern float ldexpf(float x, int exp);
extern float scalbnf(float x, int n);
extern float scalblnf(float x, long n);
extern float copysignf(float x, float y);
extern float nanf(const char *tagp);
extern float nextafterf(float x, float y);
extern float nexttowardf(float x, long double y);
extern float fdimf(float x, float y);
extern float fmaxf(float x, float y);
extern float fminf(float x, float y);
extern float fmaf(float x, float y, float z);

/* Versi long double */
extern long double sinl(long double x);
extern long double cosl(long double x);
extern long double tanl(long double x);
extern long double asinl(long double x);
extern long double acosl(long double x);
extern long double atanl(long double x);
extern long double atan2l(long double y, long double x);
extern long double expl(long double x);
extern long double exp2l(long double x);
extern long double expm1l(long double x);
extern long double logl(long double x);
extern long double log10l(long double x);
extern long double log2l(long double x);
extern long double log1pl(long double x);
extern long double logbl(long double x);
extern int ilogbl(long double x);
extern long double powl(long double x, long double y);
extern long double sqrtl(long double x);
extern long double cbrtl(long double x);
extern long double hypotl(long double x, long double y);
extern long double ceill(long double x);
extern long double floorl(long double x);
extern long double truncl(long double x);
extern long double nearbyintl(long double x);
extern long double rintl(long double x);
extern long lrintl(long double x);
extern long long llrintl(long double x);
extern long double fmodl(long double x, long double y);
extern long double remainderl(long double x, long double y);
extern long double remquol(long double x, long double y, int *quo);
extern long double modfl(long double x, long double *iptr);
extern long double frexpl(long double x, int *exp);
extern long double ldexpl(long double x, int exp);
extern long double scalbnl(long double x, int n);
extern long double scalblnl(long double x, long n);
extern long double copysignl(long double x, long double y);
extern long double nanl(const char *tagp);
extern long double nextafterl(long double x, long double y);
extern long double nexttowardl(long double x, long double y);
extern long double fdiml(long double x, long double y);
extern long double fmaxl(long double x, long double y);
extern long double fminl(long double x, long double y);
extern long double fmal(long double x, long double y, long double z);

/* ============================================================
 * FUNGSI INTERNAL
 * ============================================================
 */

/* Fungsi klasifikasi internal */
extern int __fpclassifyf(float x);
extern int __fpclassifyd(double x);
extern int __fpclassifyld(long double x);
extern int __signbitf(float x);
extern int __signbitd(double x);
extern int __signbitld(long double x);

/* Error handling */
#define HUGE_VAL  ((double)INFINITY)
#define HUGE_VALF ((float)INFINITY)
#define HUGE_VALL ((long double)INFINITY)

/* Error number untuk math */
#define MATH_ERRNO     1
#define MATH_ERREXCEPT 2
#define math_errhandling MATH_ERRNO

#endif /* LIBC_MATH_H */
