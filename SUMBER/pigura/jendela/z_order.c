/*
 * PIGURA OS - Z_ORDER.C
 * =======================
 * Pengaturan z-order (lapis kedalaman) jendela Pigura OS.
 *
 * Modul ini mengatur urutan rendering jendela menggunakan
 * linked list sederhana. Jendela dengan z-order lebih tinggi
 * digambar di atas jendela lainnya.
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
#include "jendela.h"

/*
 * ===========================================================================
 * VARIABEL GLOBAL
 * ===========================================================================
 */

/* Pool node z-order */
static z_node_t g_node_pool[JENDELA_MAKS];
static tak_bertanda32_t g_node_terpakai = 0;
static bool_t g_z_diinit = SALAH;

/* Pointer ke node paling atas (z tertinggi) */
static z_node_t *g_atas = NULL;

/* Jumlah jendela dalam z-order */
static tak_bertanda32_t g_z_count = 0;

/*
 * ===========================================================================
 * FUNGSI HELPER INTERNAL
 * ===========================================================================
 */

/*
 * Alokasi node dari pool.
 */
static z_node_t *alokasi_node(void)
{
    tak_bertanda32_t i;
    for (i = 0; i < JENDELA_MAKS; i++) {
        if (g_node_pool[i].jendela == NULL) {
            g_node_pool[i].atas = NULL;
            g_node_pool[i].bawah = NULL;
            g_node_pool[i].urutan = 0;
            g_node_pool[i].jendela = (jendela_t *)1; /* mark */
            if (g_node_terpakai < i + 1) {
                g_node_terpakai = i + 1;
            }
            return &g_node_pool[i];
        }
    }
    return NULL;
}

/*
 * Kembalikan node ke pool.
 */
static void kembalikan_node(z_node_t *node)
{
    if (node == NULL) return;
    node->jendela = NULL;
    node->atas = NULL;
    node->bawah = NULL;
    node->urutan = 0;
}

/*
 * Cari node yang berisi jendela tertentu.
 */
static z_node_t *cari_node(jendela_t *j)
{
    tak_bertanda32_t i;
    if (j == NULL) return NULL;

    for (i = 0; i < JENDELA_MAKS; i++) {
        if (g_node_pool[i].jendela == j) {
            return &g_node_pool[i];
        }
    }
    return NULL;
}

/*
 * Dapatkan z-order tertinggi saat ini.
 */
static tak_bertanda32_t dapat_z_tertinggi(void)
{
    if (g_atas != NULL) {
        return g_atas->urutan + 1;
    }
    return 0;
}

/*
 * ===========================================================================
 * Z-ORDER - INISIALISASI
 * ===========================================================================
 */

status_t z_order_init(void)
{
    if (g_z_diinit) {
        return STATUS_SUDAH_ADA;
    }
    kernel_memset(g_node_pool, 0, sizeof(g_node_pool));
    g_node_terpakai = 0;
    g_atas = NULL;
    g_z_count = 0;
    g_z_diinit = BENAR;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * Z-ORDER - TAMBAH
 * ===========================================================================
 */

status_t z_order_tambah(jendela_t *j)
{
    z_node_t *node;
    tak_bertanda32_t urutan;

    if (!g_z_diinit || j == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (!jendela_valid(j)) {
        return STATUS_PARAM_INVALID;
    }

    /* Cek apakah sudah ada di z-order */
    if (cari_node(j) != NULL) {
        return STATUS_SUDAH_ADA;
    }

    node = alokasi_node();
    if (node == NULL) {
        return STATUS_PENUH;
    }

    urutan = dapat_z_tertinggi();
    node->jendela = j;
    node->urutan = urutan;

    /* Tambahkan ke atas linked list */
    node->bawah = g_atas;
    if (g_atas != NULL) {
        g_atas->atas = node;
    }
    g_atas = node;

    g_z_count++;

    /* Simpan urutan ke struct jendela */
    {
        struct jendela *w = (struct jendela *)j;
        w->z_urutan = urutan;
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * Z-ORDER - HAPUS
 * ===========================================================================
 */

status_t z_order_hapus(jendela_t *j)
{
    z_node_t *node;

    if (!g_z_diinit || j == NULL) {
        return STATUS_PARAM_NULL;
    }

    node = cari_node(j);
    if (node == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    /* Unlink dari linked list */
    if (node->bawah != NULL) {
        node->bawah->atas = node->atas;
    }
    if (node->atas != NULL) {
        node->atas->bawah = node->bawah;
    }

    /* Update pointer atas */
    if (g_atas == node) {
        g_atas = node->bawah;
    }

    kembalikan_node(node);
    g_z_count--;

    {
        struct jendela *w = (struct jendela *)j;
        w->z_urutan = 0;
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * Z-ORDER - NAIK (ke atas)
 * ===========================================================================
 */

status_t z_order_naik(jendela_t *j)
{
    z_node_t *node;

    if (!g_z_diinit || j == NULL) {
        return STATUS_PARAM_NULL;
    }

    node = cari_node(j);
    if (node == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    /* Jika sudah di atas, tidak perlu dipindah */
    if (node->atas == NULL) {
        return STATUS_BERHASIL;
    }

    /* Unlink */
    if (node->bawah != NULL) {
        node->bawah->atas = node->atas;
    }
    node->atas->bawah = node->bawah;

    /* Sisipkan ke atas */
    node->bawah = g_atas;
    if (g_atas != NULL) {
        g_atas->atas = node;
    }
    g_atas = node;

    /* Perbarui urutan z */
    z_order_set_urutan(j, dapat_z_tertinggi());

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * Z-ORDER - TURUN (ke bawah)
 * ===========================================================================
 */

status_t z_order_turun(jendela_t *j)
{
    z_node_t *node, *terendah;

    if (!g_z_diinit || j == NULL) {
        return STATUS_PARAM_NULL;
    }

    node = cari_node(j);
    if (node == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    /* Jika sudah di bawah, tidak perlu dipindah */
    if (node->bawah == NULL) {
        return STATUS_BERHASIL;
    }

    /* Unlink */
    if (node->atas != NULL) {
        node->atas->bawah = node->bawah;
    }
    node->bawah->atas = node->atas;

    /* Cari node paling bawah */
    terendah = g_atas;
    if (terendah == NULL) {
        return STATUS_BERHASIL;
    }
    while (terendah->bawah != NULL) {
        terendah = terendah->bawah;
    }

    /* Sisipkan ke bawah */
    terendah->bawah = node;
    node->atas = terendah;
    node->bawah = NULL;

    if (g_atas == node) {
        g_atas = node->atas;
    }

    /* Perbarui urutan z */
    z_order_set_urutan(j, 0);

    /* Perbarui urutan node lain */
    {
        tak_bertanda32_t urutan = 0;
        z_node_t *n = g_atas;
        while (n != NULL) {
            n->urutan = urutan;
            {
                struct jendela *w =
                    (struct jendela *)n->jendela;
                w->z_urutan = urutan;
            }
            urutan++;
            n = n->bawah;
        }
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * Z-ORDER - DAPAT ATAS
 * ===========================================================================
 */

jendela_t *z_order_dapat_atas(void)
{
    if (!g_z_diinit || g_atas == NULL) {
        return NULL;
    }
    return g_atas->jendela;
}

/*
 * ===========================================================================
 * Z-ORDER - DAPAT BERIKUTNYA
 * ===========================================================================
 */

jendela_t *z_order_dapat_berikutnya(jendela_t *j)
{
    z_node_t *node;

    if (!g_z_diinit || j == NULL) {
        return NULL;
    }

    node = cari_node(j);
    if (node == NULL) {
        return NULL;
    }

    return (node->bawah != NULL) ? node->bawah->jendela : NULL;
}

/*
 * ===========================================================================
 * Z-ORDER - DAPAT URUTAN
 * ===========================================================================
 */

tak_bertanda32_t z_order_dapat_urutan(const jendela_t *j)
{
    const struct jendela *w = (const struct jendela *)j;
    if (w == NULL || w->magic != JENDELA_MAGIC) {
        return 0;
    }
    return w->z_urutan;
}

/*
 * ===========================================================================
 * Z-ORDER - SET URUTAN
 * ===========================================================================
 */

status_t z_order_set_urutan(jendela_t *j,
                              tak_bertanda32_t urutan)
{
    const struct jendela *w = (const struct jendela *)j;
    (void)urutan;

    if (w == NULL || w->magic != JENDELA_MAGIC) {
        return STATUS_PARAM_NULL;
    }

    /* Untuk sekarang, urutan hanya disimpan dalam struct */
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * Z-ORDER - HITUNG
 * ===========================================================================
 */

tak_bertanda32_t z_order_hitung(void)
{
    if (!g_z_diinit) {
        return 0;
    }
    return g_z_count;
}

/*
 * ===========================================================================
 * Z-ORDER - BERSIHKAN
 * ===========================================================================
 */

void z_order_bersihkan(void)
{
    tak_bertanda32_t i;

    if (!g_z_diinit) {
        return;
    }

    for (i = 0; i < JENDELA_MAKS; i++) {
        g_node_pool[i].jendela = NULL;
        g_node_pool[i].atas = NULL;
        g_node_pool[i].bawah = NULL;
        g_node_pool[i].urutan = 0;
    }
    g_node_terpakai = 0;
    g_atas = NULL;
    g_z_count = 0;
}
