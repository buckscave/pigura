/* ----------------------------------------------------------------------------------------------
 * pigura masukan: evdev.c - input /dev/input/event* untuk embedded/VT
 * ----------------------------------------------------------------------------------------------
 * Membuka /dev/input/event* satu-per-satu, klasifikasi via
 * EVIOCGBIT(0,...) (EV_KEY + KEY_ESC untuk keyboard, EV_REL atau
 * EV_ABS + BTN_LEFT untuk mouse), lalu spawn 2 thread pembaca:
 *
 *   kbd_thread   : read EV_KEY → PG_PERISTIWA_TOMBOL_*
 *   mouse_thread : read EV_REL + EV_KEY + EV_ABS
 *                  → PG_PERISTIWA_TETIK_* (GERAK / TURUN / NAIK /
 *                  RODA)
 *
 * Posisi mouse (absolut atau relatif) di-track atomik di struct
 * pg_masukan_t; tiap gerakan roda / gerak / klik ditarik saat ini
 * untuk membentuk pg_peristiwa_t. Modifier (Shift/Ctrl/Alt/Meta)
 * juga di-track atomik.
 *
 * pg_masukan_tarik() hanya menarik dari antrian internal yang diisi
 * thread — tidak memanggil pg_layar_peristiwa_berikutnya() karena
 * linuxfb tidak punya antrian windowing.
 *
 * Hanya dikompilasi di __linux__. Dijaga oleh #ifdef di seluruh file.
 * ---------------------------------------------------------------------------------------------- */
#ifdef __linux__

#include "pigura/masukan.h"
#include "pigura/layar.h"
#include "pigura/galat.h"
#include "pigura/catat.h"
#include "pigura/tipe.h"
#include "pigura/untaian.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <linux/input.h>

#define PG_MASUKAN_ANTRIAN 256
#define PG_EVDEV_POLL_MS   100

struct pg_masukan {
	pg_masukan_config_t cfg;
	pg_kunci_t         *kunci;
	pg_peristiwa_t      antrian[PG_MASUKAN_ANTRIAN];
	int                 kepala, ekor, jumlah;
	pg_layar_t         *layar;

	int                 kbd_fd;
	int                 mouse_fd;
	pg_untaian_t       *kbd_thread;
	pg_untaian_t       *mouse_thread;
	volatile pg_s32     berhenti;
	volatile pg_s32     shift_down, ctrl_down, alt_down, meta_down;
	volatile pg_s32     mouse_x, mouse_y;
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

static pg_u32 pg_evdev_sekarang_ms(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (pg_u32)(ts.tv_sec * 1000u + ts.tv_nsec / 1000000u);
}

static int pg_evdev_modifier(pg_masukan_t *in)
{
	int m = 0;
	if (pg_atom_muat(&in->shift_down)) m |= PG_MOD_SHIFT;
	if (pg_atom_muat(&in->ctrl_down))  m |= PG_MOD_CTRL;
	if (pg_atom_muat(&in->alt_down))   m |= PG_MOD_ALT;
	if (pg_atom_muat(&in->meta_down))  m |= PG_MOD_META;
	return m;
}

/* Cek apakah sebuah fd evdev punya kemampuan tertentu. */
static pg_bool pg_evdev_punya_kemampuan(int fd, int tipe, int kode)
{
	unsigned char bits[256];
	unsigned int len;
	int i;

	memset(bits, 0, sizeof(bits));
	len = (unsigned int)ioctl(fd, EVIOCGBIT(tipe, sizeof(bits)), bits);
	if (len <= 0) return PG_SALAH;
	if ((unsigned)tipe >= sizeof(bits) * 8) {
		/* EVIOCGBIT(0,...) mengembalikan bitmask tipe event. */
	}
	i = kode / 8;
	if (i >= (int)sizeof(bits)) return PG_SALAH;
	return (bits[i] & (1 << (kode % 8))) ? PG_BENAR : PG_SALAH;
}

/* Buka /dev/input/event* pertama yang cocok dengan klasifikasi. */
static int pg_evdev_buka_perangkat(pg_bool cari_keyboard)
{
	DIR *d;
	struct dirent *e;
	int fd = -1;

	d = opendir("/dev/input");
	if (!d) {
		pg_set_galat(PG_GALAT_IO, "opendir /dev/input: %s",
			      strerror(errno));
		return -1;
	}
	while ((e = readdir(d)) != NULL) {
		char path[256];
		int tfd;

		if (strncmp(e->d_name, "event", 5) != 0) continue;
		snprintf(path, sizeof(path), "/dev/input/%s",
			 e->d_name);
		tfd = open(path, O_RDONLY);
		if (tfd < 0) continue;

		if (cari_keyboard) {
			if (pg_evdev_punya_kemampuan(tfd, EV_KEY, KEY_ESC)
			    == PG_BENAR) {
				fd = tfd;
				pg_info("evdev: keyboard=%s", path);
				break;
			}
		} else {
			if ((pg_evdev_punya_kemampuan(tfd, EV_REL,
			      REL_X) == PG_BENAR ||
			     pg_evdev_punya_kemampuan(tfd, EV_ABS,
			      ABS_X) == PG_BENAR) &&
			    pg_evdev_punya_kemampuan(tfd, EV_KEY,
			      BTN_LEFT) == PG_BENAR) {
				fd = tfd;
				pg_info("evdev: mouse=%s", path);
				break;
			}
		}
		close(tfd);
	}
	closedir(d);
	return fd;
}

/* Terjemahan KEY_* evdev → pg_tombol. */
static int pg_evdev_kode_ke_tombol(int kode)
{
	switch (kode) {
	case KEY_ESC:       return PG_TOMBOL_ESCAPE;
	case KEY_ENTER:     return PG_TOMBOL_ENTER;
	case KEY_TAB:       return PG_TOMBOL_TAB;
	case KEY_BACKSPACE: return PG_TOMBOL_BACKSPACE;
	case KEY_INSERT:    return PG_TOMBOL_INSERT;
	case KEY_DELETE:    return PG_TOMBOL_DELETE;
	case KEY_HOME:      return PG_TOMBOL_HOME;
	case KEY_END:       return PG_TOMBOL_END;
	case KEY_PAGEUP:    return PG_TOMBOL_PAGEUP;
	case KEY_PAGEDOWN:  return PG_TOMBOL_PAGEDOWN;
	case KEY_LEFT:      return PG_TOMBOL_KIRI;
	case KEY_RIGHT:     return PG_TOMBOL_KANAN;
	case KEY_UP:        return PG_TOMBOL_ATAS;
	case KEY_DOWN:      return PG_TOMBOL_BAWAH;
	case KEY_SPACE:     return PG_TOMBOL_SPASI;
	case KEY_LEFTSHIFT:  return PG_TOMBOL_LSHIFT;
	case KEY_RIGHTSHIFT: return PG_TOMBOL_RSHIFT;
	case KEY_LEFTCTRL:   return PG_TOMBOL_LCTRL;
	case KEY_RIGHTCTRL:  return PG_TOMBOL_RCTRL;
	case KEY_LEFTALT:    return PG_TOMBOL_LALT;
	case KEY_RIGHTALT:   return PG_TOMBOL_RALT;
	case KEY_LEFTMETA:   return PG_TOMBOL_LMETA;
	case KEY_RIGHTMETA:  return PG_TOMBOL_RMETA;
	case KEY_F1:  return PG_TOMBOL_F1;
	case KEY_F2:  return PG_TOMBOL_F2;
	case KEY_F3:  return PG_TOMBOL_F3;
	case KEY_F4:  return PG_TOMBOL_F4;
	case KEY_F5:  return PG_TOMBOL_F5;
	case KEY_F6:  return PG_TOMBOL_F6;
	case KEY_F7:  return PG_TOMBOL_F7;
	case KEY_F8:  return PG_TOMBOL_F8;
	case KEY_F9:  return PG_TOMBOL_F9;
	case KEY_F10: return PG_TOMBOL_F10;
	case KEY_F11: return PG_TOMBOL_F11;
	case KEY_F12: return PG_TOMBOL_F12;
	case KEY_1: return '1';
	case KEY_2: return '2';
	case KEY_3: return '3';
	case KEY_4: return '4';
	case KEY_5: return '5';
	case KEY_6: return '6';
	case KEY_7: return '7';
	case KEY_8: return '8';
	case KEY_9: return '9';
	case KEY_0: return '0';
	case KEY_A: return 'A';
	case KEY_B: return 'B';
	case KEY_C: return 'C';
	case KEY_D: return 'D';
	case KEY_E: return 'E';
	case KEY_F: return 'F';
	case KEY_G: return 'G';
	case KEY_H: return 'H';
	case KEY_I: return 'I';
	case KEY_J: return 'J';
	case KEY_K: return 'K';
	case KEY_L: return 'L';
	case KEY_M: return 'M';
	case KEY_N: return 'N';
	case KEY_O: return 'O';
	case KEY_P: return 'P';
	case KEY_Q: return 'Q';
	case KEY_R: return 'R';
	case KEY_S: return 'S';
	case KEY_T: return 'T';
	case KEY_U: return 'U';
	case KEY_V: return 'V';
	case KEY_W: return 'W';
	case KEY_X: return 'X';
	case KEY_Y: return 'Y';
	case KEY_Z: return 'Z';
	case KEY_MINUS:    return (int)'-';
	case KEY_EQUAL:    return (int)'=';
	case KEY_LEFTBRACE:  return (int)'[';
	case KEY_RIGHTBRACE: return (int)']';
	case KEY_BACKSLASH:  return (int)'\\';
	case KEY_SEMICOLON:  return (int)';';
	case KEY_APOSTROPHE: return (int)'\'';
	case KEY_GRAVE:      return (int)'`';
	case KEY_COMMA:      return (int)',';
	case KEY_DOT:        return (int)'.';
	case KEY_SLASH:      return (int)'/';
	case KEY_KP0: return (int)'0';
	case KEY_KP1: return (int)'1';
	case KEY_KP2: return (int)'2';
	case KEY_KP3: return (int)'3';
	case KEY_KP4: return (int)'4';
	case KEY_KP5: return (int)'5';
	case KEY_KP6: return (int)'6';
	case KEY_KP7: return (int)'7';
	case KEY_KP8: return (int)'8';
	case KEY_KP9: return (int)'9';
	case KEY_KPDOT:    return (int)'.';
	case KEY_KPPLUS:   return (int)'+';
	case KEY_KPMINUS:  return (int)'-';
	case KEY_KPASTERISK: return (int)'*';
	case KEY_KPSLASH:  return (int)'/';
	case KEY_KPENTER:  return PG_TOMBOL_ENTER;
	case KEY_KPEQUAL:  return (int)'=';
	default: return PG_TOMBOL_KOSONG;
	}
}

static void pg_evdev_update_modifier(pg_masukan_t *in, int kode,
                                       int nilai)
{
	pg_s32 v = nilai ? 1 : 0;
	switch (kode) {
	case KEY_LEFTSHIFT:
	case KEY_RIGHTSHIFT:
		pg_atom_simpan(&in->shift_down, v);
		break;
	case KEY_LEFTCTRL:
	case KEY_RIGHTCTRL:
		pg_atom_simpan(&in->ctrl_down, v);
		break;
	case KEY_LEFTALT:
	case KEY_RIGHTALT:
		pg_atom_simpan(&in->alt_down, v);
		break;
	case KEY_LEFTMETA:
	case KEY_RIGHTMETA:
		pg_atom_simpan(&in->meta_down, v);
		break;
	default:
		break;
	}
}

static void *pg_evdev_kbd_thread(void *arg)
{
	pg_masukan_t *in = (pg_masukan_t *)arg;
	int fd = in->kbd_fd;
	struct input_event ev;

	while (!pg_atom_muat(&in->berhenti)) {
		fd_set rfds;
		struct timeval tv;
		int rv;

		if (fd < 0) break;
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		tv.tv_sec  = 0;
		tv.tv_usec = PG_EVDEV_POLL_MS * 1000;
		rv = select(fd + 1, &rfds, NULL, NULL, &tv);
		if (rv <= 0) continue;
		if (!FD_ISSET(fd, &rfds)) continue;

		if (read(fd, &ev, sizeof(ev)) != (ssize_t)sizeof(ev))
			continue;
		if (ev.type != EV_KEY) continue;

		pg_evdev_update_modifier(in, ev.code, ev.value);

		{
			pg_peristiwa_t pe;
			int t = pg_evdev_kode_ke_tombol(ev.code);
			if (t == PG_TOMBOL_KOSONG) continue;
			if (ev.value != 0 && ev.value != 1) continue;

			memset(&pe, 0, sizeof(pe));
			pe.waktu_ms = pg_evdev_sekarang_ms();
			pe.tipe = ev.value == 1 ?
				PG_PERISTIWA_TOMBOL_TURUN :
				PG_PERISTIWA_TOMBOL_NAIK;
			pe.tombol = t;
			pe.modifier = pg_evdev_modifier(in);
			if (t >= 32 && t <= 126 && ev.value == 1)
				pe.unicode = (pg_u32)t;
			pg_masukan_dorong(in, &pe);
		}
	}
	return NULL;
}

static void *pg_evdev_mouse_thread(void *arg)
{
	pg_masukan_t *in = (pg_masukan_t *)arg;
	int fd = in->mouse_fd;
	struct input_event ev;

	while (!pg_atom_muat(&in->berhenti)) {
		fd_set rfds;
		struct timeval tv;
		int rv;

		if (fd < 0) break;
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		tv.tv_sec  = 0;
		tv.tv_usec = PG_EVDEV_POLL_MS * 1000;
		rv = select(fd + 1, &rfds, NULL, NULL, &tv);
		if (rv <= 0) continue;
		if (!FD_ISSET(fd, &rfds)) continue;

		if (read(fd, &ev, sizeof(ev)) != (ssize_t)sizeof(ev))
			continue;

		if (ev.type == EV_REL) {
			pg_peristiwa_t pe;
			if (ev.code == REL_X) {
				int nx = pg_atom_muat(&in->mouse_x) +
					ev.value;
				pg_atom_simpan(&in->mouse_x, nx);
			} else if (ev.code == REL_Y) {
				int ny = pg_atom_muat(&in->mouse_y) +
					ev.value;
				pg_atom_simpan(&in->mouse_y, ny);
			} else if (ev.code == REL_WHEEL) {
				memset(&pe, 0, sizeof(pe));
				pe.waktu_ms = pg_evdev_sekarang_ms();
				pe.tipe = PG_PERISTIWA_TETIK_RODA;
				pe.roda_dy = ev.value > 0 ? 1 : -1;
				pe.tetik_pos = pg_buat_titik(
					pg_atom_muat(&in->mouse_x),
					pg_atom_muat(&in->mouse_y));
				pg_masukan_dorong(in, &pe);
				continue;
			} else {
				continue;
			}
			memset(&pe, 0, sizeof(pe));
			pe.waktu_ms = pg_evdev_sekarang_ms();
			pe.tipe = PG_PERISTIWA_TETIK_GERAK;
			pe.tetik_pos = pg_buat_titik(
				pg_atom_muat(&in->mouse_x),
				pg_atom_muat(&in->mouse_y));
			pg_masukan_dorong(in, &pe);
		} else if (ev.type == EV_ABS) {
			pg_peristiwa_t pe;
			if (ev.code == ABS_X)
				pg_atom_simpan(&in->mouse_x, ev.value);
			else if (ev.code == ABS_Y)
				pg_atom_simpan(&in->mouse_y, ev.value);
			else continue;
			memset(&pe, 0, sizeof(pe));
			pe.waktu_ms = pg_evdev_sekarang_ms();
			pe.tipe = PG_PERISTIWA_TETIK_GERAK;
			pe.tetik_pos = pg_buat_titik(
				pg_atom_muat(&in->mouse_x),
				pg_atom_muat(&in->mouse_y));
			pg_masukan_dorong(in, &pe);
		} else if (ev.type == EV_KEY) {
			pg_peristiwa_t pe;
			int btn = 0;

			switch (ev.code) {
			case BTN_LEFT:   btn = PG_TETIK_KIRI; break;
			case BTN_RIGHT:  btn = PG_TETIK_KANAN; break;
			case BTN_MIDDLE: btn = PG_TETIK_TENGAH; break;
			default: continue;
			}
			if (ev.value != 0 && ev.value != 1) continue;
			memset(&pe, 0, sizeof(pe));
			pe.waktu_ms = pg_evdev_sekarang_ms();
			pe.tipe = ev.value == 1 ?
				PG_PERISTIWA_TETIK_TURUN :
				PG_PERISTIWA_TETIK_NAIK;
			pe.tetik_tombol = btn;
			pe.tetik_pos = pg_buat_titik(
				pg_atom_muat(&in->mouse_x),
				pg_atom_muat(&in->mouse_y));
			pg_masukan_dorong(in, &pe);
		}
	}
	return NULL;
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
	in->cfg = *cfg;
	in->layar = layar;
	in->kbd_fd = -1;
	in->mouse_fd = -1;

	e = pg_buat_kunci(&in->kunci);
	if (e != PG_OK) { free(in); return e; }

	/* Buka perangkat. Path eksplisit dari cfg dipakai bila ada. */
	if (cfg->papan_tombol_dev) {
		in->kbd_fd = open(cfg->papan_tombol_dev, O_RDONLY);
		if (in->kbd_fd >= 0)
			pg_info("evdev: keyboard=%s",
				cfg->papan_tombol_dev);
	} else {
		in->kbd_fd = pg_evdev_buka_perangkat(PG_BENAR);
	}
	if (cfg->tetik_dev) {
		in->mouse_fd = open(cfg->tetik_dev, O_RDONLY);
		if (in->mouse_fd >= 0)
			pg_info("evdev: mouse=%s", cfg->tetik_dev);
	} else {
		in->mouse_fd = pg_evdev_buka_perangkat(PG_SALAH);
	}

	if (in->kbd_fd < 0 && in->mouse_fd < 0) {
		pg_peringatan("evdev: tidak ada perangkat input "
			       "ditemukan di /dev/input");
	}

	/* Spawn thread bila fd valid. */
	pg_atom_simpan(&in->berhenti, 0);
	if (in->kbd_fd >= 0) {
		e = pg_buat_untaian(&in->kbd_thread,
			pg_evdev_kbd_thread, in);
		if (e != PG_OK) {
			close(in->kbd_fd);
			in->kbd_fd = -1;
		}
	}
	if (in->mouse_fd >= 0) {
		e = pg_buat_untaian(&in->mouse_thread,
			pg_evdev_mouse_thread, in);
		if (e != PG_OK) {
			close(in->mouse_fd);
			in->mouse_fd = -1;
		}
	}

	*out = in;
	return PG_OK;
}

pg_galat pg_tutup_masukan(pg_masukan_t *in)
{
	if (!in) return PG_OK;

	/* Sinyal berhenti + join thread. */
	pg_atom_simpan(&in->berhenti, 1);
	if (in->kbd_thread)   pg_gabung_untaian(in->kbd_thread, NULL);
	if (in->mouse_thread) pg_gabung_untaian(in->mouse_thread, NULL);

	if (in->kbd_fd >= 0)  close(in->kbd_fd);
	if (in->mouse_fd >= 0) close(in->mouse_fd);
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

	if (!in || !buf || maks <= 0) return 0;

	/* Tarik dari antrian internal — sudah diisi oleh thread
	 * kbd_thread dan mouse_thread. */
	pg_kunci_kunci(in->kunci);
	while (n < maks && in->jumlah > 0) {
		buf[n++] = in->antrian[in->kepala];
		in->kepala = (in->kepala + 1) % PG_MASUKAN_ANTRIAN;
		in->jumlah--;
	}
	pg_kunci_buka(in->kunci);
	return n;
}

#endif /* __linux__ */
