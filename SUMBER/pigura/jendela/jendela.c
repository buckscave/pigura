/*
 * PIGURA OS - JENDELA.C
 * =====================
 * Manajemen jendela Pigura OS.
 *
 * Modul ini mengelola siklus hidup jendela: pembuatan,
 * penghapusan, perpindahan, pengubahan ukuran, dan
 * penyimpanan properti jendela.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Tidak menggunakan fitur C99/C11
 * - Seluruh nama dalam Bahasa Indonesia
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../../inti/kernel.h"
#include "../framebuffer/framebuffer.h"
#include "jendela.h"

/*
 * ===========================================================================
 * VARIABEL GLOBAL
 * ===========================================================================
 */

static struct jendela g_jendela_list[JENDELA_MAKS];
static tak_bertanda32_t g_jendela_jumlah = 0;
static tak_bertanda32_t g_jendela_id_counter = 1;
static bool_t g_jendela_diinit = SALAH;

/*
 * ===========================================================================
 * FUNGSI HELPER INTERNAL
 * ===========================================================================
 */

static tak_bertanda32_t buat_id_baru(void)
{
    tak_bertanda32_t id = g_jendela_id_counter;
    g_jendela_id_counter++;
    if (g_jendela_id_counter == 0) {
        g_jendela_id_counter = 1;
    }
    return id;
}

static void salin_judul(char *tujuan, const char *sumber,
                        tak_bertanda32_t maks)
{
    tak_bertanda32_t i;
    for (i = 0; i < maks - 1; i++) {
        if (sumber[i] == '\0') break;
        tujuan[i] = sumber[i];
    }
    tujuan[i] = '\0';
}

/*
 * ===========================================================================
 * JENDELA - INISIALISASI
 * ===========================================================================
 */

status_t jendela_init(void)
{
    if (g_jendela_diinit) {
        return STATUS_SUDAH_ADA;
    }
    kernel_memset(g_jendela_list, 0, sizeof(g_jendela_list));
    g_jendela_jumlah = 0;
    g_jendela_id_counter = 1;
    g_jendela_diinit = BENAR;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * JENDELA - SHUTDOWN
 * ===========================================================================
 */

void jendela_shutdown(void)
{
    tak_bertanda32_t i;
    for (i = 0; i < JENDELA_MAKS; i++) {
        if (g_jendela_list[i].magic == JENDELA_MAGIC) {
            jendela_hapus(&g_jendela_list[i]);
        }
    }
    g_jendela_jumlah = 0;
    g_jendela_diinit = SALAH;
}

/*
 * ===========================================================================
 * JENDELA - BUAT
 * ===========================================================================
 */

jendela_t *jendela_buat(tak_bertanda32_t x, tak_bertanda32_t y,
                         tak_bertanda32_t lebar,
                         tak_bertanda32_t tinggi,
                         const char *judul,
                         tak_bertanda32_t flags)
{
    struct jendela *j;
    tak_bertanda32_t i;

    if (!g_jendela_diinit) {
        return NULL;
    }

    /* Batas ukuran minimum */
    if (lebar < JENDELA_LEBAR_MIN) {
        lebar = JENDELA_LEBAR_MIN;
    }
    if (tinggi < JENDELA_TINGGI_MIN) {
        tinggi = JENDELA_TINGGI_MIN;
    }

    /* Cari slot kosong */
    for (i = 0; i < JENDELA_MAKS; i++) {
        if (g_jendela_list[i].magic != JENDELA_MAGIC) {
            break;
        }
    }
    if (i >= JENDELA_MAKS) {
        return NULL;
    }

    j = &g_jendela_list[i];
    kernel_memset(j, 0, sizeof(struct jendela));

    j->magic = JENDELA_MAGIC;
    j->id = buat_id_baru();
    j->x = x;
    j->y = y;
    j->lebar = lebar;
    j->tinggi = tinggi;
    j->status = JENDELA_STATUS_VISIBLE;
    j->flags = flags | JENDELA_FLAG_AKTIF | JENDELA_FLAG_DEKORASI;
    j->warna_judul = JENDELA_WARNA_JUDUL;
    j->warna_bingkai = JENDELA_WARNA_BINGKAI;
    j->warna_latar = JENDELA_WARNA_LATAR;
    j->z_urutan = 0;
    j->permukaan_konten = NULL;

    if (judul != NULL) {
        salin_judul(j->judul, judul, JENDELA_JUDUL_PANJANG);
    } else {
        j->judul[0] = '\0';
    }

    g_jendela_jumlah++;
    return (jendela_t *)j;
}

/*
 * ===========================================================================
 * JENDELA - HAPUS
 * ===========================================================================
 */

status_t jendela_hapus(jendela_t *j)
{
    struct jendela *w = (struct jendela *)j;

    if (w == NULL || w->magic != JENDELA_MAGIC) {
        return STATUS_PARAM_NULL;
    }

    w->magic = 0;
    w->permukaan_konten = NULL;
    if (g_jendela_jumlah > 0) {
        g_jendela_jumlah--;
    }
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * JENDELA - GETTERS
 * ===========================================================================
 */

tak_bertanda32_t jendela_dapat_id(const jendela_t *j)
{
    const struct jendela *w = (const struct jendela *)j;
    if (w == NULL || w->magic != JENDELA_MAGIC) return 0;
    return w->id;
}

const char *jendela_dapat_judul(const jendela_t *j)
{
    const struct jendela *w = (const struct jendela *)j;
    if (w == NULL || w->magic != JENDELA_MAGIC) return NULL;
    return w->judul;
}

status_t jendela_dapat_posisi(const jendela_t *j,
                               tak_bertanda32_t *x,
                               tak_bertanda32_t *y)
{
    const struct jendela *w = (const struct jendela *)j;
    if (w == NULL || w->magic != JENDELA_MAGIC) {
        return STATUS_PARAM_NULL;
    }
    if (x != NULL) *x = w->x;
    if (y != NULL) *y = w->y;
    return STATUS_BERHASIL;
}

status_t jendela_dapat_ukuran(const jendela_t *j,
                               tak_bertanda32_t *lebar,
                               tak_bertanda32_t *tinggi)
{
    const struct jendela *w = (const struct jendela *)j;
    if (w == NULL || w->magic != JENDELA_MAGIC) {
        return STATUS_PARAM_NULL;
    }
    if (lebar != NULL) *lebar = w->lebar;
    if (tinggi != NULL) *tinggi = w->tinggi;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * JENDELA - SETTERS
 * ===========================================================================
 */

status_t jendela_pindah(jendela_t *j,
                          tak_bertanda32_t x,
                          tak_bertanda32_t y)
{
    struct jendela *w = (struct jendela *)j;
    if (w == NULL || w->magic != JENDELA_MAGIC) {
        return STATUS_PARAM_NULL;
    }
    w->x = x;
    w->y = y;
    return STATUS_BERHASIL;
}

status_t jendela_ubah_ukuran(jendela_t *j,
                               tak_bertanda32_t lebar,
                               tak_bertanda32_t tinggi)
{
    struct jendela *w = (struct jendela *)j;
    if (w == NULL || w->magic != JENDELA_MAGIC) {
        return STATUS_PARAM_NULL;
    }
    if (lebar < JENDELA_LEBAR_MIN) {
        lebar = JENDELA_LEBAR_MIN;
    }
    if (tinggi < JENDELA_TINGGI_MIN) {
        tinggi = JENDELA_TINGGI_MIN;
    }
    w->lebar = lebar;
    w->tinggi = tinggi;
    return STATUS_BERHASIL;
}

status_t jendela_set_judul(jendela_t *j, const char *judul)
{
    struct jendela *w = (struct jendela *)j;
    if (w == NULL || w->magic != JENDELA_MAGIC) {
        return STATUS_PARAM_NULL;
    }
    if (judul == NULL) {
        return STATUS_PARAM_NULL;
    }
    salin_judul(w->judul, judul, JENDELA_JUDUL_PANJANG);
    return STATUS_BERHASIL;
}

status_t jendela_set_flags(jendela_t *j,
                             tak_bertanda32_t flags)
{
    struct jendela *w = (struct jendela *)j;
    if (w == NULL || w->magic != JENDELA_MAGIC) {
        return STATUS_PARAM_NULL;
    }
    w->flags = flags;
    return STATUS_BERHASIL;
}

status_t jendela_set_status(jendela_t *j,
                             tak_bertanda8_t status)
{
    struct jendela *w = (struct jendela *)j;
    if (w == NULL || w->magic != JENDELA_MAGIC) {
        return STATUS_PARAM_NULL;
    }
    w->status = status;
    return STATUS_BERHASIL;
}

status_t jendela_set_warna(jendela_t *j,
                            tak_bertanda32_t judul,
                            tak_bertanda32_t bingkai,
                            tak_bertanda32_t latar)
{
    struct jendela *w = (struct jendela *)j;
    if (w == NULL || w->magic != JENDELA_MAGIC) {
        return STATUS_PARAM_NULL;
    }
    w->warna_judul = judul;
    w->warna_bingkai = bingkai;
    w->warna_latar = latar;
    return STATUS_BERHASIL;
}

status_t jendela_set_visible(jendela_t *j, bool_t visible)
{
    struct jendela *w = (struct jendela *)j;
    if (w == NULL || w->magic != JENDELA_MAGIC) {
        return STATUS_PARAM_NULL;
    }
    w->status = visible ? JENDELA_STATUS_VISIBLE
                         : JENDELA_STATUS_INVISIBLE;
    return STATUS_BERHASIL;
}

bool_t jendela_valid(const jendela_t *j)
{
    const struct jendela *w = (const struct jendela *)j;
    return (w != NULL && w->magic == JENDELA_MAGIC) ? BENAR
                                                       : SALAH;
}

/*
 * ===========================================================================
 * JENDELA - AREA DAN HIT TEST
 * ===========================================================================
 */

status_t jendela_dapat_area_layar(const jendela_t *j,
                                   rect_t *area)
{
    const struct jendela *w = (const struct jendela *)j;
    if (w == NULL || w->magic != JENDELA_MAGIC) {
        return STATUS_PARAM_NULL;
    }
    if (area == NULL) {
        return STATUS_PARAM_NULL;
    }
    area->x = w->x;
    area->y = w->y;
    area->lebar = w->lebar;
    area->tinggi = w->tinggi;
    return STATUS_BERHASIL;
}

status_t jendela_dapat_area_konten(const jendela_t *j,
                                    rect_t *area)
{
    const struct jendela *w = (const struct jendela *)j;
    tak_bertanda32_t bw = JENDELA_BINGKAI_LEBAR;
    tak_bertanda32_t jt = JENDELA_JUDUL_TINGGI;

    if (w == NULL || w->magic != JENDELA_MAGIC) {
        return STATUS_PARAM_NULL;
    }
    if (area == NULL) {
        return STATUS_PARAM_NULL;
    }

    area->x = w->x + bw;
    area->y = w->y + jt;
    area->lebar = w->lebar - (bw * 2);
    area->tinggi = w->tinggi - jt - bw;
    return STATUS_BERHASIL;
}

tak_bertanda32_t jendela_hit_test(const jendela_t *j,
                                  tak_bertanda32_t x,
                                  tak_bertanda32_t y)
{
    const struct jendela *w = (const struct jendela *)j;
    tak_bertanda32_t jx2, jy2;

    if (w == NULL || w->magic != JENDELA_MAGIC) {
        return JENDELA_AREA_TIDAK_ADA;
    }
    if (w->status == JENDELA_STATUS_INVISIBLE) {
        return JENDELA_AREA_TIDAK_ADA;
    }

    jx2 = w->x + w->lebar;
    jy2 = w->y + w->tinggi;

    if (x >= jx2 || y >= jy2) {
        return JENDELA_AREA_TIDAK_ADA;
    }

    /* Cek area judul */
    if (y < w->y + JENDELA_JUDUL_TINGGI) {
        tak_bertanda32_t tutup_x;

        /* Area tutup */
        tutup_x = jx2 - JENDELA_JUDUL_TINGGI - JENDELA_BINGKAI_LEBAR;
        if (x >= tutup_x && y < w->y + JENDELA_JUDUL_TINGGI) {
            return JENDELA_AREA_TUTUP;
        }

        /* Area minimize */
        if (x >= tutup_x - JENDELA_JUDUL_TINGGI &&
            x < tutup_x) {
            return JENDELA_AREA_MINIMIZE;
        }

        /* Area maksimalkan */
        if (x >= tutup_x - (JENDELA_JUDUL_TINGGI * 2) &&
            x < tutup_x - JENDELA_JUDUL_TINGGI) {
            return JENDELA_AREA_MAKSIMAL;
        }

        return JENDELA_AREA_JUDUL;
    }

    /* Cek area bingkai bawah */
    if (y >= jy2 - JENDELA_BINGKAI_LEBAR) {
        return JENDELA_AREA_BINGKAI;
    }

    /* Cek area bingkai kiri/kanan */
    if (x < w->x + JENDELA_BINGKAI_LEBAR ||
        x >= jx2 - JENDELA_BINGKAI_LEBAR) {
        return JENDELA_AREA_BINGKAI;
    }

    return JENDELA_AREA_ISI;
}
