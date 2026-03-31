/*
 * PIGURA OS - PERMUKAAN.C
 * ========================
 * Manajemen permukaan (surface) framebuffer.
 *
 * Permukaan adalah objek memori untuk menyimpan pixel.
 * Ini BUKAN driver hardware, melainkan abstraksi software.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../../inti/kernel.h"
#include "framebuffer.h"

/*
 * ===========================================================================
 * DEFINISI STRUKTUR PERMUKAAN (lengkap)
 * ===========================================================================
 * Definisi penuh struct permukaan.
 * Hanya dilihat oleh framebuffer.c.
 * Pengguna luar hanya melihat forward declaration di framebuffer.h.
 */

struct permukaan {
    tak_bertanda32_t magic;

    /* Dimensi */
    tak_bertanda32_t lebar;
    tak_bertanda32_t tinggi;
    tak_bertanda32_t bpp;
    tak_bertanda32_t pitch;
    tak_bertanda32_t format;

    /* Buffer */
    tak_bertanda32_t *piksel;
    tak_bertanda32_t *piksel_belakang;  /* Back buffer */
    ukuran_t ukuran;

    /* Alamat fisik (untuk permukaan display) */
    tak_bertanda64_t alamat_fisik;

    /* Flags */
    tak_bertanda32_t flags;

    /* Status */
    bool_t kotor;
    bool_t aktif;
};

/*
 * ===========================================================================
 * VARIABEL GLOBAL
 * ===========================================================================
 */

static struct permukaan g_permukaan_list[PERMUKAAN_MAKS];
static tak_bertanda32_t g_permukaan_count = 0;
static bool_t g_initialized = SALAH;

/* Permukaan display utama */
static struct permukaan *g_display = NULL;

/*
 * ===========================================================================
 * FUNGSI HELPER
 * ===========================================================================
 */

static tak_bertanda32_t format_dari_bpp(tak_bertanda32_t bpp)
{
    switch (bpp) {
    case 8:  return FORMAT_RGB332;
    case 16: return FORMAT_RGB565;
    case 24: return FORMAT_RGB888;
    case 32: return FORMAT_XRGB8888;
    default: return FORMAT_INVALID;
    }
}

static tak_bertanda32_t warna_ke_pixel(struct permukaan *p,
                                        tak_bertanda8_t r,
                                        tak_bertanda8_t g,
                                        tak_bertanda8_t b,
                                        tak_bertanda8_t a)
{
    switch (p->format) {
    case FORMAT_RGB332:
        return ((r >> 5) << 5) | ((g >> 5) << 2) | (b >> 6);
    case FORMAT_RGB565:
        return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
    case FORMAT_RGB888:
        return (r << 16) | (g << 8) | b;
    case FORMAT_ARGB8888:
    case FORMAT_XRGB8888:
        return (a << 24) | (r << 16) | (g << 8) | b;
    default:
        return 0;
    }
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

status_t permukaan_init(void)
{
    if (g_initialized) {
        return STATUS_SUDAH_ADA;
    }

    kernel_memset(g_permukaan_list, 0, sizeof(g_permukaan_list));
    g_permukaan_count = 0;
    g_display = NULL;
    g_initialized = BENAR;

    return STATUS_BERHASIL;
}

permukaan_t *permukaan_buat(tak_bertanda32_t lebar,
                             tak_bertanda32_t tinggi,
                             tak_bertanda32_t bpp)
{
    struct permukaan *p;
    tak_bertanda32_t i;

    if (!g_initialized) {
        return NULL;
    }

    /* Cari slot kosong */
    for (i = 0; i < PERMUKAAN_MAKS; i++) {
        if (g_permukaan_list[i].magic != PERMUKAAN_MAGIC) {
            break;
        }
    }

    if (i >= PERMUKAAN_MAKS) {
        return NULL;  /* Penuh */
    }

    p = &g_permukaan_list[i];

    /* Inisialisasi */
    kernel_memset(p, 0, sizeof(struct permukaan));

    p->magic = PERMUKAAN_MAGIC;
    p->lebar = lebar;
    p->tinggi = tinggi;
    p->bpp = bpp;
    p->pitch = lebar * (bpp / 8);
    p->format = format_dari_bpp(bpp);
    p->ukuran = (ukuran_t)p->pitch * tinggi;

    /* Alokasi buffer */
    p->piksel = (tak_bertanda32_t *)kmalloc(p->ukuran);
    if (p->piksel == NULL) {
        p->magic = 0;
        return NULL;
    }

    kernel_memset(p->piksel, 0, p->ukuran);
    p->aktif = BENAR;
    p->kotor = SALAH;

    g_permukaan_count++;

    return (permukaan_t *)p;
}

permukaan_t *permukaan_buat_display(tak_bertanda64_t alamat_fisik,
                                      tak_bertanda32_t lebar,
                                      tak_bertanda32_t tinggi,
                                      tak_bertanda32_t bpp,
                                      tak_bertanda32_t pitch)
{
    struct permukaan *p;

    p = (struct permukaan *)permukaan_buat(lebar, tinggi, bpp);
    if (p == NULL) {
        return NULL;
    }

    /* Gunakan alamat fisik dari video driver */
    p->alamat_fisik = alamat_fisik;
    p->piksel = (tak_bertanda32_t *)(uintptr_t)alamat_fisik;
    p->pitch = pitch;
    p->ukuran = (ukuran_t)pitch * tinggi;

    g_display = p;

    return (permukaan_t *)p;
}

void permukaan_hapus(permukaan_t *handle)
{
    struct permukaan *p = (struct permukaan *)handle;

    if (p == NULL || p->magic != PERMUKAAN_MAGIC) {
        return;
    }

    /* Jangan hapus buffer display fisik */
    if (p->alamat_fisik == 0 && p->piksel != NULL) {
        kfree(p->piksel);
    }

    /* Hapus back buffer jika ada */
    if (p->piksel_belakang != NULL) {
        kfree(p->piksel_belakang);
    }

    kernel_memset(p, 0, sizeof(struct permukaan));
    g_permukaan_count--;
}

void permukaan_hapus_semua(void)
{
    tak_bertanda32_t i;

    for (i = 0; i < PERMUKAAN_MAKS; i++) {
        if (g_permukaan_list[i].magic == PERMUKAAN_MAGIC) {
            permukaan_hapus(
                (permukaan_t *)&g_permukaan_list[i]);
        }
    }
}

/* === OPERASI DASAR === */

void permukaan_tulis_piksel(permukaan_t *handle, tak_bertanda32_t x,
                             tak_bertanda32_t y, tak_bertanda32_t warna)
{
    struct permukaan *p = (struct permukaan *)handle;

    if (p == NULL || p->magic != PERMUKAAN_MAGIC) {
        return;
    }
    if (p->piksel == NULL) {
        return;
    }

    if (x >= p->lebar || y >= p->tinggi) {
        return;
    }

    p->piksel[y * (p->pitch / 4) + x] = warna;
    p->kotor = BENAR;
}

tak_bertanda32_t permukaan_baca_piksel(permukaan_t *handle,
                                        tak_bertanda32_t x,
                                        tak_bertanda32_t y)
{
    struct permukaan *p = (struct permukaan *)handle;

    if (p == NULL || p->magic != PERMUKAAN_MAGIC) {
        return 0;
    }
    if (p->piksel == NULL) {
        return 0;
    }

    if (x >= p->lebar || y >= p->tinggi) {
        return 0;
    }

    return p->piksel[y * (p->pitch / 4) + x];
}

void permukaan_isi(permukaan_t *handle, tak_bertanda32_t warna)
{
    struct permukaan *p = (struct permukaan *)handle;
    tak_bertanda32_t total_piksel;

    if (p == NULL || p->magic != PERMUKAAN_MAGIC) {
        return;
    }
    if (p->piksel == NULL) {
        return;
    }

    total_piksel = p->lebar * p->tinggi;
    kernel_memset32(p->piksel, warna, total_piksel);
    p->kotor = BENAR;
}

void permukaan_isi_kotak(permukaan_t *handle, tak_bertanda32_t x,
                          tak_bertanda32_t y, tak_bertanda32_t lebar,
                          tak_bertanda32_t tinggi,
                          tak_bertanda32_t warna)
{
    struct permukaan *p = (struct permukaan *)handle;
    tak_bertanda32_t i, j;

    if (p == NULL || p->magic != PERMUKAAN_MAGIC) {
        return;
    }

    /* Bounds check */
    if (x >= p->lebar || y >= p->tinggi) {
        return;
    }

    if (x + lebar > p->lebar) {
        lebar = p->lebar - x;
    }
    if (y + tinggi > p->tinggi) {
        tinggi = p->tinggi - y;
    }

    /* Isi per baris */
    for (j = y; j < y + tinggi; j++) {
        tak_bertanda32_t offset = j * (p->pitch / 4) + x;
        for (i = 0; i < lebar; i++) {
            p->piksel[offset + i] = warna;
        }
    }

    p->kotor = BENAR;
}

/* === GETTERS === */

tak_bertanda32_t permukaan_lebar(permukaan_t *handle)
{
    struct permukaan *p = (struct permukaan *)handle;
    if (p != NULL && p->magic == PERMUKAAN_MAGIC) {
        return p->lebar;
    }
    return 0;
}

tak_bertanda32_t permukaan_tinggi(permukaan_t *handle)
{
    struct permukaan *p = (struct permukaan *)handle;
    if (p != NULL && p->magic == PERMUKAAN_MAGIC) {
        return p->tinggi;
    }
    return 0;
}

tak_bertanda32_t permukaan_bpp(permukaan_t *handle)
{
    struct permukaan *p = (struct permukaan *)handle;
    if (p != NULL && p->magic == PERMUKAAN_MAGIC) {
        return p->bpp;
    }
    return 0;
}

tak_bertanda32_t permukaan_pitch(permukaan_t *handle)
{
    struct permukaan *p = (struct permukaan *)handle;
    if (p != NULL && p->magic == PERMUKAAN_MAGIC) {
        return p->pitch;
    }
    return 0;
}

void *permukaan_buffer(permukaan_t *handle)
{
    struct permukaan *p = (struct permukaan *)handle;
    if (p != NULL && p->magic == PERMUKAAN_MAGIC) {
        return (void *)p->piksel;
    }
    return NULL;
}

permukaan_t *permukaan_display(void)
{
    return (permukaan_t *)g_display;
}

/* === DOUBLE BUFFERING === */

status_t permukaan_aktifkan_double_buffer(permukaan_t *handle)
{
    struct permukaan *p = (struct permukaan *)handle;

    if (p == NULL || p->magic != PERMUKAAN_MAGIC) {
        return STATUS_PARAM_NULL;
    }

    if (p->piksel_belakang != NULL) {
        return STATUS_SUDAH_ADA;
    }

    p->piksel_belakang =
        (tak_bertanda32_t *)kmalloc(p->ukuran);
    if (p->piksel_belakang == NULL) {
        return STATUS_MEMORI_TIDAK_CUKUP;
    }

    kernel_memcpy(p->piksel_belakang, p->piksel, p->ukuran);
    p->flags |= PERMUKAAN_FLAG_DOUBLE_BUF;

    return STATUS_BERHASIL;
}

void permukaan_flip(permukaan_t *handle)
{
    struct permukaan *p = (struct permukaan *)handle;
    tak_bertanda32_t *temp;

    if (p == NULL || p->magic != PERMUKAAN_MAGIC) {
        return;
    }

    if (p->piksel_belakang == NULL) {
        return;  /* Tidak ada double buffer */
    }

    /* Swap buffers */
    temp = p->piksel;
    p->piksel = p->piksel_belakang;
    p->piksel_belakang = temp;

    /* Copy ke display jika ini permukaan display */
    if (p == g_display && p->alamat_fisik != 0) {
        kernel_memcpy(
            (void *)(uintptr_t)p->alamat_fisik, p->piksel, p->ukuran);
    }

    p->kotor = SALAH;
}

void permukaan_shutdown(void)
{
    permukaan_hapus_semua();
    g_initialized = SALAH;
    g_display = NULL;
}
