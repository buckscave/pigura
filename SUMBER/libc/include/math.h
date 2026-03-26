/*
 * PIGURA LIBC - MATH.H
 * =====================
 * Header untuk fungsi matematika sesuai standar C89.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.1 - Perbaikan konflik macro/fungsi
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

/* ============================================================
 * FUNGSI MATEMATIKA DASAR
 * ============================================================
 */

extern double sin(double x);
extern double cos(double x);
extern double tan(double x);
extern double asin(double x);
extern double acos(double x);
extern double atan(double x);
extern double atan2(double y, double x);

/* ============================================================
 * FUNGSI EKSPONENSIAL DAN LOGARITMA
 * ============================================================
 */

extern double exp(double x);
extern double exp2(double x);
extern double expm1(double x);
extern double log(double x);
extern double log10(double x);
extern double log2(double x);
extern double log1p(double x);
extern double logb(double x);
extern int ilogb(double x);

/* ============================================================
 * FUNGSI PANGKAT DAN AKAR
 * ============================================================
 */

extern double pow(double x, double y);
extern double sqrt(double x);
extern double cbrt(double x);
extern double hypot(double x, double y);

/* ============================================================
 * FUNGSI PEMBULATAN DAN ABSOLUT
 * ============================================================
 */

extern double ceil(double x);
extern double floor(double x);
extern double round(double x);
extern double trunc(double x);
extern double nearbyint(double x);
extern double rint(double x);
extern double fabs(double x);

/* Long versions */
extern long lround(double x);
extern long long llround(double x);
extern long lrint(double x);
extern long long llrint(double x);

/* Float versions */
extern float roundf(float x);
extern float fabsf(float x);
extern float floorf(float x);
extern float ceilf(float x);
extern float truncf(float x);

/* Long double versions */
extern long double roundl(long double x);
extern long double fabsl(long double x);

/* ============================================================
 * FUNGSI LAINNYA
 * ============================================================
 */

extern double fmod(double x, double y);
extern double remainder(double x, double y);
extern double remquo(double x, double y, int *quo);
extern double modf(double x, double *iptr);
extern double frexp(double x, int *exp);
extern double ldexp(double x, int exp);
extern double scalbn(double x, int n);
extern double scalbln(double x, long n);
extern double copysign(double x, double y);
extern double nan(const char *tagp);
extern double nextafter(double x, double y);
extern double nexttoward(double x, long double y);
extern double fdim(double x, double y);
extern double fmax(double x, double y);
extern double fmin(double x, double y);
extern double fma(double x, double y, double z);

/* ============================================================
 * KLASIFIKASI DAN MACRO
 * ============================================================
 */

/* Fungsi klasifikasi */
extern int __fpclassifyf(float x);
extern int __fpclassifyd(double x);
extern int __fpclassifyld(long double x);
extern int __signbitf(float x);
extern int __signbitd(double x);
extern int __signbitld(long double x);

/* Macro untuk klasifikasi - hanya definisikan jika belum ada implementasi fungsi */
#ifndef __LIBC_MATH_C

#define fpclassify(x) \
    (sizeof(x) == sizeof(float) ? __fpclassifyf(x) : \
     sizeof(x) == sizeof(double) ? __fpclassifyd(x) : \
     __fpclassifyld(x))

#define isfinite(x) (fpclassify(x) >= FP_ZERO)
#define isinf(x)    (fpclassify(x) == FP_INFINITE)
#define isnan(x)    (fpclassify(x) == FP_NAN)
#define isnormal(x) (fpclassify(x) == FP_NORMAL)

#define signbit(x) \
    (sizeof(x) == sizeof(float) ? __signbitf(x) : \
     sizeof(x) == sizeof(double) ? __signbitd(x) : \
     __signbitld(x))

#endif /* __LIBC_MATH_C */

/* Makro comparison */
#define isgreater(x, y)      ((x) > (y))
#define isgreaterequal(x, y) ((x) >= (y))
#define isless(x, y)         ((x) < (y))
#define islessequal(x, y)    ((x) <= (y))
#define islessgreater(x, y)  ((x) < (y) || (x) > (y))
#define isunordered(x, y)    (isnan(x) || isnan(y))

/* ============================================================
 * VERSI FLOAT DAN LONG DOUBLE
 * ============================================================
 */

extern float sinf(float x);
extern float cosf(float x);
extern float tanf(float x);
extern float asinf(float x);
extern float acosf(float x);
extern float atanf(float x);
extern float atan2f(float y, float x);
extern float expf(float x);
extern float logf(float x);
extern float log10f(float x);
extern float powf(float x, float y);
extern float sqrtf(float x);
extern float fmodf(float x, float y);
extern float hypotf(float x, float y);
extern float modff(float x, float *iptr);
extern float frexpf(float x, int *exp);
extern float ldexpf(float x, int exp);
extern float scalbnf(float x, int n);
extern float copysignf(float x, float y);

/* Long double versions */
extern long double sinl(long double x);
extern long double cosl(long double x);
extern long double tanl(long double x);
extern long double sqrtl(long double x);
extern long double powl(long double x, long double y);
extern long double fabsl(long double x);

/* ============================================================
 * ERROR HANDLING
 * ============================================================
 */

#define HUGE_VAL  ((double)INFINITY)
#define HUGE_VALF ((float)INFINITY)
#define HUGE_VALL ((long double)INFINITY)

#define MATH_ERRNO     1
#define MATH_ERREXCEPT 2

/* Variable untuk error handling */
extern int math_errhandling;

#endif /* LIBC_MATH_H */
