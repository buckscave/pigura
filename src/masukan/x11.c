/* ----------------------------------------------------------------------------------------------
 * pigura masukan: x11.c - input X11 native untuk desktop Linux
 * ----------------------------------------------------------------------------------------------
 * Membaca peristiwa X11 (KeyPress, ButtonPress, MotionNotify) dari
 * antrian peristiwa jendela X11 yang dibuka pg_layar. Tidak butuh
 * root, tidak membuka /dev/input/event*. Mendukung layout keyboard
 * XKB secara otomatis karena X server yang menanganinya.
 *
 * Pola: pg_masukan_tarik() memanggil pg_layar_peristiwa_berikutnya()
 * untuk menarik peristiwa yang sudah diterjemahkan backend layar ke
 * pg_peristiwa_t. Tidak ada thread terpisah — perulangan utama yang
 * memompa.
 *
 * XPending/XNextEvent harus dipanggil dari satu thread saja untuk
 * Display yang sama; di X11 itu perulangan utama.
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/masukan.h"
#include "pigura/layar.h"
#include "pigura/galat.h"
#include "pigura/catat.h"
#include "pigura/untaian.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#define PG_MASUKAN_ANTRIAN 256

struct pg_masukan {
        pg_masukan_config_t cfg;
        pg_kunci_t *kunci;
        pg_peristiwa_t antrian[PG_MASUKAN_ANTRIAN];
        int           kepala, ekor, jumlah;
        pg_layar_t   *layar;
};

static void pg_masukan_dorong(pg_masukan_t *in, const pg_peristiwa_t *e)
{
        pg_kunci_kunci(in->kunci);
        if (in->jumlah < PG_MASUKAN_ANTRIAN) {
                in->antrian[in->ekor] = *e;
                in->ekor = (in->ekor + 1) % PG_MASUKAN_ANTRIAN;
                in->jumlah++;
        }
        pg_kunci_buka(in->kunci);
}

pg_galat pg_buka_masukan(pg_masukan_t **out,
                          const pg_masukan_config_t *cfg,
                          pg_layar_t *layar)
{
        pg_masukan_t *in;
        pg_galat e;
        pg_masukan_config_t def;

        if (!out) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
        if (!cfg) {
                memset(&def, 0, sizeof(def));
                cfg = &def;
        }

        in = (pg_masukan_t *)calloc(1, sizeof(*in));
        if (!in) PG_KEMBALI_GALAT(PG_GALAT_MEMORI);
        in->cfg   = *cfg;
        in->layar = layar;

        e = pg_buat_kunci(&in->kunci);
        if (e != PG_OK) { free(in); return e; }

        *out = in;
        return PG_OK;
}

pg_galat pg_tutup_masukan(pg_masukan_t *in)
{
        if (!in) return PG_OK;
        if (in->kunci) pg_hancur_kunci(in->kunci);
        free(in);
        return PG_OK;
}

pg_galat pg_masukan_emit(pg_masukan_t *in, const pg_peristiwa_t *e)
{
        if (!in || !e) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
        pg_masukan_dorong(in, e);
        return PG_OK;
}

int pg_masukan_tarik(pg_masukan_t *in, pg_peristiwa_t *buf, int maks)
{
        int n = 0;
        pg_peristiwa_t ev;

        if (!in || !buf || maks <= 0) return 0;

        /* Pompa peristiwa X11 dari pg_layar ke antrian internal. */
        if (in->layar) {
                while (pg_layar_peristiwa_berikutnya(in->layar, &ev)) {
                        pg_masukan_dorong(in, &ev);
                        if (n >= maks) break;
                }
        }

        /* Tarik dari antrian internal. */
        pg_kunci_kunci(in->kunci);
        while (n < maks && in->jumlah > 0) {
                buf[n++] = in->antrian[in->kepala];
                in->kepala = (in->kepala + 1) % PG_MASUKAN_ANTRIAN;
                in->jumlah--;
        }
        pg_kunci_buka(in->kunci);
        return n;
}
