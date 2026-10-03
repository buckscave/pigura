/* ----------------------------------------------------------------------------------------------
 * pigura wm: wm.c - window manager + compositor untuk mode standalone
 * ----------------------------------------------------------------------------------------------
 * Implementasi WM internal pigura untuk mode framebuffer. Mengelola
 * jendela top-level sendiri (z-order, posisi, ukuran) dan mencompos-
 * ite semuanya ke permukaan target (framebuffer) setiap frame.
 *
 * Jendela disimpan sebagai doubly-linked list dengan top-of-stack
 * di head. Composite menelusuri dari tail (bottom) ke head (top),
 * blit permukaan tiap jendela ke dest dengan clip ke batas dest.
 *
 * Tidak ada dekorasi (title bar, border) di v0.1; jendela murni
 * permukaan gambar. Ini tugas widget jendela (jendela_widget.c)
 * bila ingin dekorasi.
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/wm.h"
#include "pigura/permukaan.h"
#include "pigura/galat.h"
#include "pigura/catat.h"

#include <stdlib.h>
#include <string.h>

struct pg_wm_jendela {
	pg_wm_jendela_t *prev, *next;
	pg_wm_t         *wm;
	pg_permukaan_t  *permukaan;
	char            *judul;
	int              x, y, w, h;
	pg_bool          terlihat;
};

struct pg_wm {
	pg_wm_jendela_t *head;   /* top of stack */
	pg_wm_jendela_t *tail;   /* bottom of stack */
	int              lebar, tinggi;
};

/* Duplikasi string sederhana (NULL-safe). */
static char *pg_wm_dup_judul(const char *s)
{
	size_t n;
	char *out;
	if (!s) return NULL;
	n = strlen(s) + 1;
	out = (char *)malloc(n);
	if (!out) return NULL;
	memcpy(out, s, n);
	return out;
}

/* Sisipkan j di head (top of stack). */
static void pg_wm_sisip_atas(pg_wm_t *wm, pg_wm_jendela_t *j)
{
	j->prev = NULL;
	j->next = wm->head;
	if (wm->head) wm->head->prev = j;
	else          wm->tail = j;
	wm->head = j;
}

/* Lepaskan j dari list wm. */
static void pg_wm_lepas(pg_wm_t *wm, pg_wm_jendela_t *j)
{
	if (j->prev) j->prev->next = j->next;
	else          wm->head = j->next;
	if (j->next) j->next->prev = j->prev;
	else          wm->tail = j->prev;
	j->prev = NULL;
	j->next = NULL;
}

pg_wm_t *pg_wm_buat(int lebar, int tinggi)
{
	pg_wm_t *wm;
	if (lebar <= 0 || tinggi <= 0) {
		pg_set_galat(PG_GALAT_ARGUMEN, "pg_wm_buat: "
		             "dimensi buruk %dx%d", lebar, tinggi);
		return NULL;
	}
	wm = (pg_wm_t *)calloc(1, sizeof(*wm));
	if (!wm) {
		pg_set_galat(PG_GALAT_MEMORI, "pg_wm_buat: oom");
		return NULL;
	}
	wm->lebar  = lebar;
	wm->tinggi = tinggi;
	return wm;
}

void pg_wm_hancur(pg_wm_t *wm)
{
	pg_wm_jendela_t *j, *next;
	if (!wm) return;
	j = wm->head;
	while (j) {
		next = j->next;
		if (j->permukaan) pg_hancur_permukaan(j->permukaan);
		if (j->judul)     free(j->judul);
		free(j);
		j = next;
	}
	free(wm);
}

pg_wm_jendela_t *pg_wm_buat_jendela(pg_wm_t *wm, int x, int y,
                                      int w, int h, const char *judul)
{
	pg_wm_jendela_t *j;
	if (!wm) {
		pg_set_galat(PG_GALAT_ARGUMEN, "pg_wm_buat_jendela: "
		             "wm NULL");
		return NULL;
	}
	if (w <= 0 || h <= 0) {
		pg_set_galat(PG_GALAT_ARGUMEN, "pg_wm_buat_jendela: "
		             "dimensi buruk %dx%d", w, h);
		return NULL;
	}
	j = (pg_wm_jendela_t *)calloc(1, sizeof(*j));
	if (!j) {
		pg_set_galat(PG_GALAT_MEMORI, "pg_wm_buat_jendela: oom");
		return NULL;
	}
	j->wm  = wm;
	j->x   = x;
	j->y   = y;
	j->w   = w;
	j->h   = h;
	j->terlihat = PG_BENAR;
	j->permukaan = pg_buat_permukaan(w, h);
	if (!j->permukaan) {
		free(j);
		return NULL;
	}
	if (judul) {
		j->judul = pg_wm_dup_judul(judul);
		if (!j->judul) {
			pg_hancur_permukaan(j->permukaan);
			free(j);
			return NULL;
		}
	}
	pg_wm_sisip_atas(wm, j);
	pg_info("wm: jendela %dx%d @(%d,%d) '%s' dibuat",
		w, h, x, y, judul ? judul : "");
	return j;
}

void pg_wm_hancur_jendela(pg_wm_jendela_t *j)
{
	pg_wm_t *wm;
	if (!j) return;
	wm = j->wm;
	if (wm) pg_wm_lepas(wm, j);
	if (j->permukaan) pg_hancur_permukaan(j->permukaan);
	if (j->judul)     free(j->judul);
	free(j);
}

pg_permukaan_t *pg_wm_jendela_permukaan(pg_wm_jendela_t *j)
{
	return j ? j->permukaan : NULL;
}

void pg_wm_jendela_pindah(pg_wm_jendela_t *j, int x, int y)
{
	if (!j) return;
	j->x = x;
	j->y = y;
}

void pg_wm_jendela_naik(pg_wm_jendela_t *j)
{
	pg_wm_t *wm;
	if (!j || !j->wm) return;
	wm = j->wm;
	if (wm->head == j) return;
	pg_wm_lepas(wm, j);
	pg_wm_sisip_atas(wm, j);
}

void pg_wm_composite(pg_wm_t *wm, pg_permukaan_t *dest)
{
	pg_wm_jendela_t *j;
	pg_kotak_t clip;
	int dw, dh;
	if (!wm || !dest) return;
	dw = pg_permukaan_lebar(dest);
	dh = pg_permukaan_tinggi(dest);
	clip = pg_buat_kotak(0, 0, dw, dh);
	/* Telusuri dari tail (bottom) ke head (top) supaya
	 * jendela di atas menutup yang bawah. */
	for (j = wm->tail; j != NULL; j = j->prev) {
		if (!j->terlihat)     continue;
		if (!j->permukaan)    continue;
		pg_blit_potong_permukaan(dest, j->x, j->y,
		                          j->permukaan, clip);
	}
}

pg_wm_jendela_t *pg_wm_pilih(pg_wm_t *wm, pg_titik_t p)
{
	pg_wm_jendela_t *j;
	if (!wm) return NULL;
	/* Cari dari top (head) ke bawah; jendela pertama yang
	 * berisi titik adalah yang terlihat paling atas. */
	for (j = wm->head; j != NULL; j = j->next) {
		pg_kotak_t r;
		if (!j->terlihat) continue;
		r = pg_buat_kotak(j->x, j->y, j->w, j->h);
		if (PG_TITIK_DI_KOTAK(p, r)) return j;
	}
	return NULL;
}
