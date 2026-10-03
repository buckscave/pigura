/* ----------------------------------------------------------------------------------------------
 * pigura inti: perulangan.c - loop peristiwa
 * ----------------------------------------------------------------------------------------------
 * Loop peristiwa single-threaded. Pada tiap iterasi: pompa peristiwa
 * dari pg_masukan_tarik() (yang untuk X11 baca langsung dari antrian
 * X11; untuk evdev baca dari antrian internal yang sudah diisi oleh
 * thread internal evdev), dispatch ke callback user, lalu panggil
 * callback idle bila tidak ada peristiwa.
 *
 * Integrasi timer: tiap iterasi panggil pg_timer_pompa() untuk fire
 * timer yang sudah expired. Return value = ms sampai timer berikutnya
 * expire, dipakai sebagai sleep timeout (mengganti pg_tidur_ms(1)
 * hard-coded). Timer system boleh belum init — pg_timer_pompa()
 * return sentinel 0xFFFFFFFF yang langsung dilewati.
 *
 * Single-thread di sini = aman untuk X11 (yang tidak thread-safe
 * tanpa XInitThreads). Backend evdev tetap multithread internal tapi
 * thread-nya tidak menyentuh Display.
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/perulangan.h"
#include "pigura/untaian.h"
#include "pigura/timer.h"
#include "pigura/galat.h"
#include "pigura/catat.h"

#include <stdlib.h>
#include <string.h>

#define PG_PERULANGAN_ANTRIAN 512

struct pg_perulangan {
        pg_kunci_t   *kunci;
        pg_kondisi_t *kondisi;
        pg_peristiwa_t antrian[PG_PERULANGAN_ANTRIAN];
        int           kepala, ekor, jumlah;
        volatile pg_s32 berhenti;
        pg_masukan_t *masukan;
        pg_layar_t   *layar;
        pg_idle_cb    idle;
        void         *idle_ctx;
        pg_bool      pompa_masukan;
};

pg_galat pg_buat_perulangan(pg_perulangan_t **out,
                              const pg_perulangan_config_t *cfg)
{
        pg_perulangan_t *loop;
        pg_galat e;
        if (!out || !cfg) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
        loop = (pg_perulangan_t *)calloc(1, sizeof(*loop));
        if (!loop) PG_KEMBALI_GALAT(PG_GALAT_MEMORI);
        e = pg_buat_kunci(&loop->kunci);
        if (e != PG_OK) { free(loop); return e; }
        e = pg_buat_kondisi(&loop->kondisi);
        if (e != PG_OK) {
                pg_hancur_kunci(loop->kunci);
                free(loop);
                return e;
        }
        loop->masukan    = cfg->masukan;
        loop->layar      = cfg->layar;
        loop->idle       = cfg->idle;
        loop->idle_ctx   = cfg->idle_ctx;
        loop->pompa_masukan = cfg->pompa_masukan;
        *out = loop;
        return PG_OK;
}

pg_galat pg_hancur_perulangan(pg_perulangan_t *loop)
{
        if (!loop) return PG_OK;
        pg_hentikan_perulangan(loop);
        if (loop->kondisi) pg_hancur_kondisi(loop->kondisi);
        if (loop->kunci)   pg_hancur_kunci(loop->kunci);
        free(loop);
        return PG_OK;
}

pg_galat pg_jalankan_perulangan(pg_perulangan_t *loop,
                                  pg_peristiwa_cb cb, void *ctx)
{
        pg_peristiwa_t buf[64];
        int n, i;

        if (!loop || !cb) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);

        while (!pg_atom_muat(&loop->berhenti)) {
                int ada_peristiwa = 0;

                /* Pompa SEMUA peristiwa tertunda sekaligus. */
                if (loop->pompa_masukan && loop->masukan) {
                        n = pg_masukan_tarik(loop->masukan, buf, 64);
                        if (n > 0) {
                                pg_kunci_kunci(loop->kunci);
                                for (i = 0; i < n; i++) {
                                        if (loop->jumlah <
                                            PG_PERULANGAN_ANTRIAN) {
                                                loop->antrian[loop->ekor] =
                                                        buf[i];
                                                loop->ekor = (loop->ekor + 1)
                                                        % PG_PERULANGAN_ANTRIAN;
                                                loop->jumlah++;
                                        }
                                }
                                pg_kunci_buka(loop->kunci);
                        }
                }

                /* Proses SEMUA peristiwa, TANPA render di dalamnya. */
                for (;;) {
                        pg_peristiwa_t ev;
                        pg_kunci_kunci(loop->kunci);
                        if (loop->jumlah > 0) {
                                ev = loop->antrian[loop->kepala];
                                loop->kepala = (loop->kepala + 1) %
                                        PG_PERULANGAN_ANTRIAN;
                                loop->jumlah--;
                        } else {
                                ev.tipe = PG_PERISTIWA_KOSONG;
                        }
                        pg_kunci_buka(loop->kunci);
                        if (ev.tipe == PG_PERISTIWA_KOSONG) break;
                        cb(&ev, ctx);
                        ada_peristiwa = 1;
                }

                /* Render SEKALI setelah semua peristiwa diproses.
                 * Jika tidak ada peristiwa: tidur, jangan render. */
                if (ada_peristiwa && loop->idle) {
                        loop->idle(loop->idle_ctx);
                }

                /* Pompa timer (fire yang sudah expired). Return
                 * value = ms sampai timer berikutnya expire, dipakai
                 * sebagai sleep timeout. Bila tidak ada timer aktif
                 * (return 0xFFFFFFFF) atau timer system belum init,
                 * fallback ke tidur 1 ms lama supaya event loop
                 * tetap polling event X11. */
                {
                        unsigned tidur = pg_timer_pompa();
                        if (tidur == 0xFFFFFFFFu) tidur = 1u;
                        if (!ada_peristiwa) pg_tidur_ms(tidur);
                }
        }
        return PG_OK;
}

void pg_hentikan_perulangan(pg_perulangan_t *loop)
{
        if (!loop) return;
        pg_atom_simpan(&loop->berhenti, 1);
}

pg_galat pg_perulangan_emit(pg_perulangan_t *loop, const pg_peristiwa_t *e)
{
        if (!loop || !e) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
        pg_kunci_kunci(loop->kunci);
        if (loop->jumlah < PG_PERULANGAN_ANTRIAN) {
                loop->antrian[loop->ekor] = *e;
                loop->ekor = (loop->ekor + 1) % PG_PERULANGAN_ANTRIAN;
                loop->jumlah++;
        }
        pg_kunci_buka(loop->kunci);
        return PG_OK;
}

int pg_perulangan_poll(pg_perulangan_t *loop, pg_peristiwa_t *buf, int maks)
{
        int n = 0;
        if (!loop || !buf || maks <= 0) return 0;
        pg_kunci_kunci(loop->kunci);
        while (n < maks && loop->jumlah > 0) {
                buf[n++] = loop->antrian[loop->kepala];
                loop->kepala = (loop->kepala + 1) % PG_PERULANGAN_ANTRIAN;
                loop->jumlah--;
        }
        pg_kunci_buka(loop->kunci);
        return n;
}

int pg_perulangan_tunggu(pg_perulangan_t *loop, pg_peristiwa_t *buf,
                          int maks, unsigned timeout_ms)
{
        if (!loop || !buf || maks <= 0) return 0;
        pg_kunci_kunci(loop->kunci);
        while (loop->jumlah == 0 && timeout_ms > 0) {
                pg_galat r = pg_kondisi_tunggu_batas(loop->kondisi,
                                                       loop->kunci,
                                                       timeout_ms);
                if (r != PG_OK) break;
        }
        pg_kunci_buka(loop->kunci);
        return pg_perulangan_poll(loop, buf, maks);
}
