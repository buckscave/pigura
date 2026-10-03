/* ----------------------------------------------------------------------------------------------
 * pigura layar: wayland.c - stub backend Wayland
 * ----------------------------------------------------------------------------------------------
 * Wayland backend belum diimplementasi. Stub ini mendeteksi env
 * WAYLAND_DISPLAY dan mengembalikan PG_GALAT_TANPA dengan pesan
 * jelas bila dipanggil. Tidak crash, tidak hang.
 *
 * Implementasi penuh butuh: wl_compositor, wl_shm, wl_seat,
 * wl_output, xdg_shell, plus dispatch loop wl_display_read_events.
 * Rumit; dijadwalkan v0.3+.
 *
 * Untuk sekarang pakai X11 (default desktop) atau linuxfb (embedded).
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/layar.h"
#include "pigura/galat.h"
#include "pigura/catat.h"

#include <stdlib.h>
#include <string.h>

pg_galat pg_buka_layar(pg_layar_t **out, const pg_layar_config_t *cfg)
{
	const char *wd;
	(void)cfg;
	if (!out) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
	wd = getenv("WAYLAND_DISPLAY");
	if (wd) {
		pg_set_galat(PG_GALAT_TANPA,
		             "Backend Wayland belum diimplementasi. "
		             "Pakai X11. (WAYLAND_DISPLAY=%s)", wd);
	} else {
		pg_set_galat(PG_GALAT_TANPA,
		             "Backend Wayland belum diimplementasi. "
		             "Pakai X11.");
	}
	*out = NULL;
	return PG_GALAT_TANPA;
}

pg_galat pg_tutup_layar(pg_layar_t *l)
{
	(void)l;
	return PG_OK;
}

pg_galat pg_layar_kueri(pg_layar_t *l, pg_layar_info_t *info)
{
	(void)l;
	if (!info) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
	memset(info, 0, sizeof(*info));
	return PG_GALAT_TANPA;
}

pg_galat pg_layar_kunci(pg_layar_t *l, void **piksel, int *langkah)
{
	(void)l; (void)piksel; (void)langkah;
	return PG_GALAT_TANPA;
}

pg_galat pg_layar_buka_kunci(pg_layar_t *l)
{
	(void)l;
	return PG_OK;
}

pg_galat pg_layar_presentasi(pg_layar_t *l)
{
	(void)l;
	return PG_GALAT_TANPA;
}

pg_galat pg_layar_tunggu_vsync(pg_layar_t *l)
{
	(void)l;
	return PG_GALAT_TANPA;
}

pg_galat pg_layar_pompa_peristiwa(pg_layar_t *l)
{
	(void)l;
	return PG_OK;
}

void *pg_layar_handle_native(pg_layar_t *l)
{
	(void)l;
	return NULL;
}

unsigned long pg_layar_jendela_id(pg_layar_t *l)
{
	(void)l;
	return 0;
}

pg_bool pg_layar_punya_peristiwa(pg_layar_t *l)
{
	(void)l;
	return PG_SALAH;
}

pg_bool pg_layar_peristiwa_berikutnya(pg_layar_t *l,
                                       pg_peristiwa_t *out)
{
	(void)l;
	if (out) memset(out, 0, sizeof(*out));
	return PG_SALAH;
}
