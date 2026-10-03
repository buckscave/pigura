/* ----------------------------------------------------------------------------------------------
 * pigura masukan: wayland.c - stub input Wayland
 * ----------------------------------------------------------------------------------------------
 * Backend masukan Wayland belum diimplementasi. Stub ini
 * mengembalikan PG_GALAT_TANPA dengan pesan jelas bila dipanggil.
 * Tidak crash, tidak hang.
 *
 * Implementasi penuh butuh wl_seat + wl_keyboard + wl_pointer +
 * xkb_keymap untuk layout keyboard. Dijadwalkan v0.3+ bersama
 * backend layar Wayland.
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/masukan.h"
#include "pigura/layar.h"
#include "pigura/galat.h"
#include "pigura/catat.h"

#include <stdlib.h>
#include <string.h>

pg_galat pg_buka_masukan(pg_masukan_t **out,
                          const pg_masukan_config_t *cfg,
                          pg_layar_t *layar)
{
	(void)cfg; (void)layar;
	if (!out) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
	pg_set_galat(PG_GALAT_TANPA,
	             "Backend masukan Wayland belum diimplementasi. "
	             "Pakai X11.");
	*out = NULL;
	return PG_GALAT_TANPA;
}

pg_galat pg_tutup_masukan(pg_masukan_t *in)
{
	(void)in;
	return PG_OK;
}

pg_galat pg_masukan_emit(pg_masukan_t *in, const pg_peristiwa_t *e)
{
	(void)in; (void)e;
	return PG_GALAT_TANPA;
}

int pg_masukan_tarik(pg_masukan_t *in, pg_peristiwa_t *buf, int maks)
{
	(void)in; (void)buf; (void)maks;
	return 0;
}
