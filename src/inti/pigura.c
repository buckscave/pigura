/* ----------------------------------------------------------------------------------------------
 * pigura inti: pigura.c - inisialisasi global
 * ----------------------------------------------------------------------------------------------
 * Inisialisasi subsistem pigura. Aman dipanggil berulang. Saat ini
 * menginisialisasi:
 *   - catat (logging)
 *   - timer (subsystem timer global)
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/pigura.h"
#include "pigura/timer.h"
#include "pigura/widget.h"

#include <stdlib.h>

static volatile pg_s32 g_pigura_init = 0;

pg_galat pigura_init(void)
{
        if (pg_atom_muat(&g_pigura_init))
                return PG_OK;
        if (pg_atom_cas(&g_pigura_init, 0, 1)) {
                pg_catat_init("pigura");
                pg_timer_init();
                pg_info("pigura %s siap", PIGURA_VERSI_STRING);
        }
        return PG_OK;
}

pg_galat pigura_selesai(void)
{
        if (pg_atom_muat(&g_pigura_init)) {
                pg_timer_selesai();
                /* Catatan: pg_widget_selesai() sudah dihapus. Cleanup
                 * widget sekarang lewat parent-child tree: hancurkan
                 * root window, semua anak ikut dihancurkan bila
                 * milik=BENAR. Aplikasi bertanggung jawab hancurkan
                 * root widget-nya sebelum pigura_selesai(). */
        }
        pg_atom_simpan(&g_pigura_init, 0);
        return PG_OK;
}

pg_bool pigura_sudah_init(void)
{
        return pg_atom_muat(&g_pigura_init) ? PG_BENAR : PG_SALAH;
}
