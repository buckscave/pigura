/* ----------------------------------------------------------------------------------------------
 * pigura/tipe.h - Tipe data umum dan konstanta
 * ----------------------------------------------------------------------------------------------
 * Tipe data dasar yang dipakai di seluruh pustaka pigura. Karena C89
 * tidak memiliki <stdint.h> maupun <stdbool.h>, kita definisikan alias
 * lebar-tetap dan pg_bool di sini agar API publik tidak bergantung
 * pada fitur C99.
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_TIPE_H
#define PIGURA_TIPE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Bilangan bulat lebar-tetap. */
typedef unsigned char  pg_u8;
typedef signed   char  pg_s8;
typedef unsigned short pg_u16;
typedef signed   short pg_s16;
typedef unsigned int   pg_u32;
typedef signed   int   pg_s32;
#if defined(_MSC_VER) || defined(__WATCOMC__)
typedef unsigned __int64 pg_u64;
typedef signed   __int64 pg_s64;
#else
typedef unsigned long long pg_u64;
typedef signed   long long pg_s64;
#endif

/* Boolean. C89 tidak punya tipe bool native. */
typedef int pg_bool;
#define PG_SALAH 0
#define PG_BENAR 1

/* Konvensi galat: 0 = sukses, negatif = galat. */
typedef int pg_galat;
#define PG_OK              0
#define PG_GALAT_UMUM     -1
#define PG_GALAT_MEMORI   -2
#define PG_GALAT_ARGUMEN  -3
#define PG_GALAT_TIDAKADA -4
#define PG_GALAT_IJIN     -5
#define PG_GALAT_SIBUK    -6
#define PG_GALAT_TANPA    -7
#define PG_GALAT_RENTANG  -8
#define PG_GALAT_IO       -9
#define PG_GALAT_EOF     -10

/* Titik 2D. */
typedef struct pg_titik {
        int x, y;
} pg_titik_t;

/* Kotak 2D (x,y adalah sudut kiri-atas). */
typedef struct pg_kotak {
        int x, y, w, h;
} pg_kotak_t;

/* Posisi relatif elemen (mis. kotak cek relatif label, ikon relatif
 * label tombol). Dipakai oleh cek dan radio. */
typedef enum {
        PG_POSISI_KIRI   = 0,
        PG_POSISI_KANAN  = 1,
        PG_POSISI_ATAS   = 2,
        PG_POSISI_BAWAH  = 3
} pg_posisi_t;

/* Warna 32-bit layout A8R8G8B8 (0xAARRGGBB).
 * Alpha 0 = transparan penuh, 255 = opaque. */
typedef pg_u32 pg_warna_t;

/* RGB dengan alpha = 255 (opaque default). */
#define PG_RGB(r,g,b)  ((pg_warna_t)(0xFF000000UL | \
                                    ((pg_u32)(pg_u8)(r) << 16) | \
                                    ((pg_u32)(pg_u8)(g) <<  8) | \
                                    ((pg_u32)(pg_u8)(b)      )))
/* RGBA dengan alpha eksplisit. */
#define PG_RGBA(r,g,b,a) ((pg_warna_t)(((pg_u32)(pg_u8)(a) << 24) | \
                                    ((pg_u32)(pg_u8)(r) << 16) | \
                                    ((pg_u32)(pg_u8)(g) <<  8) | \
                                    ((pg_u32)(pg_u8)(b)      )))
#define PG_R(c) ((pg_u8)(((pg_warna_t)(c) >> 16) & 0xff))
#define PG_G(c) ((pg_u8)(((pg_warna_t)(c) >>  8) & 0xff))
#define PG_B(c) ((pg_u8)(((pg_warna_t)(c)      ) & 0xff))
#define PG_A(c) ((pg_u8)(((pg_warna_t)(c) >> 24) & 0xff))

/* Transparan penuh (alpha = 0). */
#define PG_TRANSPARAN PG_RGBA(0, 0, 0, 0)

/* Palet umum (alpha = 255). */
#define PG_HITAM    PG_RGB(0x00, 0x00, 0x00)
#define PG_PUTIH    PG_RGB(0xff, 0xff, 0xff)
#define PG_MERAH    PG_RGB(0xff, 0x00, 0x00)
#define PG_HIJAU    PG_RGB(0x00, 0xff, 0x00)
#define PG_BIRU     PG_RGB(0x00, 0x00, 0xff)
#define PG_KUNING   PG_RGB(0xff, 0xff, 0x00)
#define PG_CYAN     PG_RGB(0x00, 0xff, 0xff)
#define PG_MAGENTA  PG_RGB(0xff, 0x00, 0xff)
#define PG_ABU      PG_RGB(0x80, 0x80, 0x80)
#define PG_ABU_GELAP PG_RGB(0x30, 0x30, 0x30)
#define PG_ABU_TERANG PG_RGB(0xc0, 0xc0, 0xc0)

/* ===== Tema warna widget (default, dapat di-override per-widget) =====
 * Sesuai spec: panel-style buttons dengan state visual jelas. */
#define PG_WARNA_PANEL          PG_RGB(0xF2, 0xF2, 0xF2) /* 242 — idle tombol */
#define PG_WARNA_HOVER_ISI      PG_RGB(0xE0, 0xE0, 0xE0) /* 224 — hover isi */
#define PG_WARNA_HOVER_OUTLINE  PG_RGB(0xCC, 0xCC, 0xCC) /* 204 — border idle/hover */
#define PG_WARNA_TEKAN_ISI      PG_RGB(0xBF, 0xBF, 0xBF) /* 191 — tekan isi */
#define PG_WARNA_TEKAN_TERANG   PG_RGB(0xF0, 0xF0, 0xF0) /* 240 — tekan atas+kiri */
#define PG_WARNA_TEKAN_GELAP    PG_RGB(0x99, 0x99, 0x99) /* 153 — tekan kanan+bawah */
#define PG_WARNA_FOKUS          PG_RGB(0x4A, 0x90, 0xE2) /* 74,144,226 — border fokus */
#define PG_WARNA_TEKS_TOMBOL    PG_RGB(0x33, 0x33, 0x33) /* 51 — teks tombol */

/* Makro geometri. */
#define PG_KOTAK_KOSONG(r)  ((r).w <= 0 || (r).h <= 0)
#define PG_TITIK_DI_KOTAK(p,r) \
        ((p).x >= (r).x && (p).x < ((r).x + (r).w) && \
         (p).y >= (r).y && (p).y < ((r).y + (r).h))

#define PG_KOTAK_IRISAN(a,b) \
        ( (a).x < (b).x + (b).w && (a).x + (a).w > (b).x && \
          (a).y < (b).y + (b).h && (a).y + (a).h > (b).y )

#define PG_MIN(a,b) ((a) < (b) ? (a) : (b))
#define PG_MAX(a,b) ((a) > (b) ? (a) : (b))
#define PG_BATAS(v,lo,hi) ((v) < (lo) ? (lo) : ((v) > (hi) ? (hi) : (v)))

/* Konstruktor pg_kotak_t tanpa compound literal C99. */
pg_kotak_t pg_buat_kotak(int x, int y, int w, int h);

/* Konstruktor pg_titik_t tanpa compound literal C99. */
pg_titik_t pg_buat_titik(int x, int y);


#define PG_SNAP_ATAS   0x01
#define PG_SNAP_BAWAH  0x02
#define PG_SNAP_KIRI   0x04
#define PG_SNAP_KANAN  0x08
#define PG_SNAP_SEMUA  0x0F

#endif /* PIGURA_TIPE_H */
