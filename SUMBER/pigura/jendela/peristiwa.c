/*
 * PIGURA OS - PERISTIWA.C
 * ===========================
 * Routing peristiwa input ke jendela Pigura OS.
 *
 * Modul ini menerima peristiwa mouse dan keyboard,
 * lalu meneruskannya ke jendela yang sesuai
 * berdasarkan posisi kursor dan state fokus.
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

/* Antrian peristiwa circular buffer */
static peristiwa_jendela_t g_antrian[PERISTIWA_ANTRIAN_MAKS];
static tak_bertanda32_t g_antrian_kepala = 0;
static tak_bertanda32_t g_antrian_ekor = 0;
static tak_bertanda32_t g_antrian_jumlah = 0;
static bool_t g_peristiwa_diinit = SALAH;

/*
 * ===========================================================================
 * PERISTIWA - INISIALISASI
 * ===========================================================================
 */

status_t peristiwa_init(void)
{
    if (g_peristiwa_diinit) {
        return STATUS_SUDAH_ADA;
    }
    kernel_memset(g_antrian, 0, sizeof(g_antrian));
    g_antrian_kepala = 0;
    g_antrian_ekor = 0;
    g_antrian_jumlah = 0;
    g_peristiwa_diinit = BENAR;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * PERISTIWA - SHUTDOWN
 * ===========================================================================
 */

void peristiwa_shutdown(void)
{
    g_antrian_kepala = 0;
    g_antrian_ekor = 0;
    g_antrian_jumlah = 0;
    g_peristiwa_diinit = SALAH;
}

/*
 * ===========================================================================
 * PERISTIWA - PUSH
 * ===========================================================================
 */

status_t peristiwa_push(const peristiwa_jendela_t *evt)
{
    if (!g_peristiwa_diinit || evt == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Cek apakah antrian penuh */
    if (g_antrian_jumlah >= PERISTIWA_ANTRIAN_MAKS) {
        return STATUS_PENUH;
    }

    /* Salin peristiwa ke antrian */
    g_antrian[g_antrian_ekor] = *evt;

    /* Update pointer ekor (circular buffer) */
    g_antrian_ekor++;
    if (g_antrian_ekor >= PERISTIWA_ANTRIAN_MAKS) {
        g_antrian_ekor = 0;
    }

    g_antrian_jumlah++;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * PERISTIWA - POP
 * ===========================================================================
 */

status_t peristiwa_pop(peristiwa_jendela_t *evt)
{
    if (!g_peristiwa_diinit || evt == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Cek apakah antrian kosong */
    if (g_antrian_jumlah == 0) {
        return STATUS_KOSONG;
    }

    /* Ambil dari kepala antrian */
    *evt = g_antrian[g_antrian_kepala];

    /* Bersihkan slot */
    kernel_memset(&g_antrian[g_antrian_kepala], 0,
                   sizeof(peristiwa_jendela_t));

    /* Update pointer kepala */
    g_antrian_kepala++;
    if (g_antrian_kepala >= PERISTIWA_ANTRIAN_MAKS) {
        g_antrian_kepala = 0;
    }

    g_antrian_jumlah--;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * PERISTIWA - ROUTE MOUSE
 * ===========================================================================
 *
 * Cari jendela yang berada di bawah kursor mouse.
 * Mulai dari jendela paling atas (z-order tertinggi)
 * dan turun hingga menemukan jendela yang mengandung
 * koordinat (x, y).
 */

jendela_t *peristiwa_route_mouse(tak_bertanda32_t x,
                                   tak_bertanda32_t y,
                                   tak_bertanda32_t tombol)
{
    jendela_t *fokus;
    peristiwa_jendela_t evt;
    tak_bertanda32_t i;
    tak_bertanda32_t jumlah;

    if (!g_peristiwa_diinit) {
        return NULL;
    }

    jumlah = JENDELA_MAKS;
    for (i = 0; i < jumlah; i++) {
        struct jendela *w;
        extern struct jendela g_jendela_list[];
        w = &g_jendela_list[i];

        if (w->magic != JENDELA_MAGIC) {
            continue;
        }
        if (w->status == JENDELA_STATUS_INVISIBLE) {
            continue;
        }

        /* Hit test: apakah kursor di dalam jendela? */
        if (x >= w->x && x < w->x + w->lebar &&
            y >= w->y && y < w->y + w->tinggi) {
            /* Cek apakah ini klik pada tombol tutup */
            tak_bertanda32_t area = jendela_hit_test(
                (jendela_t *)w, x, y);
            if (area == JENDELA_AREA_TUTUP) {
                /* Buat peristiwa tutup */
                evt.tipe = PERISTIWA_JENDELA_TUTUP_KLIK;
                evt.id_jendela = w->id;
                evt.x = x;
                evt.y = y;
                evt.tombol = tombol;
                peristiwa_push(&evt);
            } else {
                /* Set fokus ke jendela ini */
                fokus = wm_dapat_fokus();
                if (fokus != (jendela_t *)w) {
                    wm_set_fokus((jendela_t *)w);
                }
            }
            return (jendela_t *)w;
        }
    }

    return NULL;
}

/*
 * ===========================================================================
 * PERISTIWA - ROUTE KEYBOARD
 * ===========================================================================
 *
 * Route peristiwa keyboard ke jendela yang sedang fokus.
 */

jendela_t *peristiwa_route_keyboard(void)
{
    jendela_t *fokus;

    if (!g_peristiwa_diinit) {
        return NULL;
    }

    fokus = wm_dapat_fokus();
    if (fokus != NULL && jendela_valid(fokus)) {
        return fokus;
    }

    return NULL;
}

/*
 * ===========================================================================
 * PERISTIWA - PROSES SEMUA
 * ===========================================================================
 *
 * Proses semua peristiwa dalam antrian.
 * Untuk setiap peristiwa, lakukan aksi yang sesuai.
 */

status_t peristiwa_proses_semua(void)
{
    peristiwa_jendela_t evt;
    tak_bertanda32_t diproses = 0;
    tak_bertanda32_t batas;

    if (!g_peristiwa_diinit) {
        return STATUS_TIDAK_DUKUNG;
    }

    /* Batasi jumlah per langkah untuk mencegah infinite loop */
    batas = g_antrian_jumlah;
    if (batas > 64) {
        batas = 64;
    }

    while (diproses < batas) {
        status_t st = peristiwa_pop(&evt);
        if (st != STATUS_BERHASIL) {
            break;
        }
        diproses++;

        switch (evt.tipe) {
        case PERISTIWA_JENDELA_TUTUP_KLIK:
            {
                /* Hapus jendela yang diminta tutup */
                jendela_t *target;
                target = wm_cari_id(evt.id_jendela);
                if (target != NULL) {
                    jendela_hapus(target);
                    z_order_hapus(target);
                }
                break;
            }
        case PERISTIWA_JENDELA_FOKUS_MASUK:
            {
                jendela_t *target;
                target = wm_cari_id(evt.id_jendela);
                if (target != NULL) {
                    wm_set_fokus(target);
                }
                break;
            }
        default:
            /* Peristiwa lain: tidak ada aksi */
            break;
        }
    }

    return STATUS_BERHASIL;
}
