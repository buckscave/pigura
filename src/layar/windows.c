/* ----------------------------------------------------------------------------------------------
 * pigura layar: windows.c - backend Win32 GDI untuk desktop Windows
 * ----------------------------------------------------------------------------------------------
 * Membuat jendela Win32 (WNDCLASSEXA + CreateWindowExA) dan menyediakan
 * back buffer piksel via CreateCompatibleDC + CreateDIBSection 32-bit
 * top-down. Tiap pg_layar_presentasi() men-BlBlt back buffer ke layar
 * jendela dan memompa pesan non-input via PeekMessageA.
 *
 * pg_layar_aksi_berikutnya() memompa satu pesan input dari antrian
 * (range 0x0100..0xFFFF — keyboard & mouse) dan menerjemahkannya ke
 * pg_aksi_t. Pesan non-input (WM_PAINT, WM_CLOSE, WM_DESTROY,
 * WM_QUIT) sudah dipompa oleh pg_layar_presentasi().
 *
 * Back buffer DIB 32-bit BI_RGB top-down memiliki layout memori
 * BB GG RR XX pada little-endian — persis sama dengan pg_warna_t
 * (0x00RRGGBB) yang disimpan little-endian, jadi pg_layar_kunci()
 * mengembalikan pointer bits DIB langsung tanpa konversi.
 *
 * Hanya dikompilasi di _WIN32. Dijaga oleh #ifdef di seluruh file.
 * ---------------------------------------------------------------------------------------------- */
#ifdef _WIN32

#include "pigura/layar.h"
#include "pigura/galat.h"
#include "pigura/catat.h"
#include "pigura/tipe.h"

#include <stdlib.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define PG_LAYAR_KELAS_NAMA "pigura_layar_kelas"
#define PG_LAYAR_FILTER_INPUT_MIN 0x0100
#define PG_LAYAR_FILTER_INPUT_MAX 0xFFFF

struct pg_layar {
	HWND            hwnd;
	HDC             hdc_layar;
	HDC             hdc_mem;
	HBITMAP         dib;
	HBITMAP         dib_lama;
	void           *bits;
	int             lebar, tinggi, langkah;
	pg_bool         terkunci;
	pg_bool         keluar;
};

/* ---- WndProc ---- */
static LRESULT CALLBACK pg_win_wndproc(HWND hwnd, UINT msg,
                                         WPARAM wp, LPARAM lp)
{
	PAINTSTRUCT ps;

	switch (msg) {
	case WM_ERASEBKGND:
		/* Kita cat sendiri seluruh klien tiap frame. */
		return 1;
	case WM_PAINT:
		BeginPaint(hwnd, &ps);
		EndPaint(hwnd, &ps);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProcA(hwnd, msg, wp, lp);
	}
}

static pg_u32 pg_win_sekarang_ms(void)
{
	return (pg_u32)GetTickCount();
}

/* Terjemahan VK_* → pg_tombol. */
static int pg_win_vk_ke_tombol(int vk)
{
	switch (vk) {
	case VK_ESCAPE:   return PG_TOMBOL_ESCAPE;
	case VK_RETURN:   return PG_TOMBOL_ENTER;
	case VK_TAB:      return PG_TOMBOL_TAB;
	case VK_BACK:     return PG_TOMBOL_BACKSPACE;
	case VK_INSERT:   return PG_TOMBOL_INSERT;
	case VK_DELETE:   return PG_TOMBOL_DELETE;
	case VK_HOME:     return PG_TOMBOL_HOME;
	case VK_END:      return PG_TOMBOL_END;
	case VK_PRIOR:    return PG_TOMBOL_PAGEUP;
	case VK_NEXT:     return PG_TOMBOL_PAGEDOWN;
	case VK_LEFT:     return PG_TOMBOL_KIRI;
	case VK_RIGHT:    return PG_TOMBOL_KANAN;
	case VK_UP:       return PG_TOMBOL_ATAS;
	case VK_DOWN:     return PG_TOMBOL_BAWAH;
	case VK_SPACE:    return PG_TOMBOL_SPASI;
	case VK_LSHIFT:   return PG_TOMBOL_LSHIFT;
	case VK_RSHIFT:   return PG_TOMBOL_RSHIFT;
	case VK_LCONTROL: return PG_TOMBOL_LCTRL;
	case VK_RCONTROL: return PG_TOMBOL_RCTRL;
	case VK_LMENU:    return PG_TOMBOL_LALT;
	case VK_RMENU:    return PG_TOMBOL_RALT;
	case VK_LWIN:     return PG_TOMBOL_LMETA;
	case VK_RWIN:     return PG_TOMBOL_RMETA;
	case VK_F1:       return PG_TOMBOL_F1;
	case VK_F2:       return PG_TOMBOL_F2;
	case VK_F3:       return PG_TOMBOL_F3;
	case VK_F4:       return PG_TOMBOL_F4;
	case VK_F5:       return PG_TOMBOL_F5;
	case VK_F6:       return PG_TOMBOL_F6;
	case VK_F7:       return PG_TOMBOL_F7;
	case VK_F8:       return PG_TOMBOL_F8;
	case VK_F9:       return PG_TOMBOL_F9;
	case VK_F10:      return PG_TOMBOL_F10;
	case VK_F11:      return PG_TOMBOL_F11;
	case VK_F12:      return PG_TOMBOL_F12;
	default:
		if (vk == VK_SHIFT)   return PG_TOMBOL_LSHIFT;
		if (vk == VK_CONTROL) return PG_TOMBOL_LCTRL;
		if (vk == VK_MENU)    return PG_TOMBOL_LALT;
		if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) return '0' + (vk - VK_NUMPAD0);
		if (vk == VK_DECIMAL)  return '.';
		if (vk == VK_ADD)      return '+';
		if (vk == VK_SUBTRACT) return '-';
		if (vk == VK_MULTIPLY) return '*';
		if (vk == VK_DIVIDE)   return '/';
		if (vk >= 32 && vk <= 126) return vk;
		return PG_TOMBOL_KOSONG;
	}
}

static int pg_win_modifier(void)
{
	int m = 0;

	if (GetKeyState(VK_SHIFT) & 0x8000)   m |= PG_MOD_SHIFT;
	if (GetKeyState(VK_CONTROL) & 0x8000) m |= PG_MOD_CTRL;
	if (GetKeyState(VK_MENU) & 0x8000)    m |= PG_MOD_ALT;
	return m;
}

static pg_titik_t pg_win_lparam_ke_titik(LPARAM lp)
{
	int x = (int)(short)LOWORD(lp);
	int y = (int)(short)HIWORD(lp);
	return pg_buat_titik(x, y);
}

/* Terjemahkan satu MSG Win32 ke pg_aksi_t. */
static pg_bool pg_win_msg_ke_peristiwa(const MSG *m, pg_aksi_t *out)
{
	short delta;

	memset(out, 0, sizeof(*out));
	out->waktu_ms = pg_win_sekarang_ms();

	switch (m->message) {
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		out->tipe = PG_AKSI_TOMBOL_TURUN;
		out->tombol = pg_win_vk_ke_tombol((int)m->wParam);
		out->modifier = pg_win_modifier();
		return PG_BENAR;
	case WM_KEYUP:
	case WM_SYSKEYUP:
		out->tipe = PG_AKSI_TOMBOL_NAIK;
		out->tombol = pg_win_vk_ke_tombol((int)m->wParam);
		out->modifier = pg_win_modifier();
		return PG_BENAR;
	case WM_LBUTTONDOWN:
		out->tipe = PG_AKSI_TETIKUS_TEKAN;
		out->tetik_tombol = PG_TETIKUS_KIRI;
		out->tetik_pos = pg_win_lparam_ke_titik(m->lParam);
		return PG_BENAR;
	case WM_LBUTTONUP:
		out->tipe = PG_AKSI_TETIKUS_LEPAS;
		out->tetik_tombol = PG_TETIKUS_KIRI;
		out->tetik_pos = pg_win_lparam_ke_titik(m->lParam);
		return PG_BENAR;
	case WM_RBUTTONDOWN:
		out->tipe = PG_AKSI_TETIKUS_TEKAN;
		out->tetik_tombol = PG_TETIKUS_KANAN;
		out->tetik_pos = pg_win_lparam_ke_titik(m->lParam);
		return PG_BENAR;
	case WM_RBUTTONUP:
		out->tipe = PG_AKSI_TETIKUS_LEPAS;
		out->tetik_tombol = PG_TETIKUS_KANAN;
		out->tetik_pos = pg_win_lparam_ke_titik(m->lParam);
		return PG_BENAR;
	case WM_MBUTTONDOWN:
		out->tipe = PG_AKSI_TETIKUS_TEKAN;
		out->tetik_tombol = PG_TETIKUS_TENGAH;
		out->tetik_pos = pg_win_lparam_ke_titik(m->lParam);
		return PG_BENAR;
	case WM_MBUTTONUP:
		out->tipe = PG_AKSI_TETIKUS_LEPAS;
		out->tetik_tombol = PG_TETIKUS_TENGAH;
		out->tetik_pos = pg_win_lparam_ke_titik(m->lParam);
		return PG_BENAR;
	case WM_MOUSEMOVE:
		out->tipe = PG_AKSI_TETIKUS_GERAK;
		out->tetik_pos = pg_win_lparam_ke_titik(m->lParam);
		return PG_BENAR;
	case WM_MOUSEWHEEL:
		delta = (short)HIWORD(m->wParam);
		out->tipe = PG_AKSI_TETIKUS_GULIR;
		out->roda_dy = delta > 0 ? 1 : -1;
		return PG_BENAR;
	case WM_CLOSE:
		out->tipe = PG_AKSI_KELUAR;
		return PG_BENAR;
	default:
		return PG_SALAH;
	}
}

pg_galat pg_buka_layar(pg_layar_t **out, const pg_layar_config_t *cfg)
{
	pg_layar_t *l;
	pg_layar_config_t def;
	WNDCLASSEXA wc;
	HBITMAP dib_lama;
	BITMAPINFO bi;
	int lebar, tinggi;
	DWORD style, ex_style;
	RECT rc;

	if (!out) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
	if (!cfg) {
		memset(&def, 0, sizeof(def));
		def.lebar = 640;
		def.tinggi = 480;
		def.judul = "pigura";
		def.double_buffer = PG_BENAR;
		cfg = &def;
	}
	lebar  = cfg->lebar  > 0 ? cfg->lebar  : 640;
	tinggi = cfg->tinggi > 0 ? cfg->tinggi : 480;

	l = (pg_layar_t *)calloc(1, sizeof(*l));
	if (!l) PG_KEMBALI_GALAT(PG_GALAT_MEMORI);
	l->lebar  = lebar;
	l->tinggi = tinggi;
	l->langkah = lebar * 4;

	/* Daftarkan kelas jendela (idempoten — return kelas sudah ada OK). */
	memset(&wc, 0, sizeof(wc));
	wc.cbSize        = sizeof(wc);
	wc.lpfnWndProc   = pg_win_wndproc;
	wc.hInstance     = GetModuleHandleA(NULL);
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszClassName = PG_LAYAR_KELAS_NAMA;
	RegisterClassExA(&wc);

	/* Hitung rect jendela termasuk border supaya klien = lebar×tinggi. */
	style = WS_OVERLAPPEDWINDOW;
	ex_style = 0;
	rc.left = 0; rc.top = 0;
	rc.right = lebar; rc.bottom = tinggi;
	AdjustWindowRectEx(&rc, style, FALSE, ex_style);

	l->hwnd = CreateWindowExA(ex_style, PG_LAYAR_KELAS_NAMA,
				   cfg->judul ? cfg->judul : "pigura",
				   style,
				   CW_USEDEFAULT, CW_USEDEFAULT,
				   rc.right - rc.left, rc.bottom - rc.top,
				   NULL, NULL, wc.hInstance, NULL);
	if (!l->hwnd) {
		pg_set_galat(PG_GALAT_UMUM, "CreateWindowExA gagal: %lu",
			      (unsigned long)GetLastError());
		free(l);
		return PG_GALAT_UMUM;
	}

	/* DC jendela + DC memory + DIB section 32-bit top-down. */
	l->hdc_layar = GetDC(l->hwnd);
	if (!l->hdc_layar) {
		DestroyWindow(l->hwnd);
		free(l);
		PG_KEMBALI_GALAT(PG_GALAT_UMUM);
	}
	l->hdc_mem = CreateCompatibleDC(l->hdc_layar);
	if (!l->hdc_mem) {
		ReleaseDC(l->hwnd, l->hdc_layar);
		DestroyWindow(l->hwnd);
		free(l);
		PG_KEMBALI_GALAT(PG_GALAT_UMUM);
	}

	memset(&bi, 0, sizeof(bi));
	bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth       = lebar;
	bi.bmiHeader.biHeight      = -tinggi; /* top-down */
	bi.bmiHeader.biPlanes      = 1;
	bi.bmiHeader.biBitCount    = 32;
	bi.bmiHeader.biCompression = BI_RGB;
	bi.bmiHeader.biSizeImage   = (DWORD)(lebar * tinggi * 4);

	l->bits = NULL;
	l->dib = CreateDIBSection(l->hdc_layar, &bi, DIB_RGB_COLORS,
				   &l->bits, NULL, 0);
	if (!l->dib || !l->bits) {
		pg_set_galat(PG_GALAT_UMUM, "CreateDIBSection gagal: %lu",
			      (unsigned long)GetLastError());
		DeleteDC(l->hdc_mem);
		ReleaseDC(l->hwnd, l->hdc_layar);
		DestroyWindow(l->hwnd);
		free(l);
		return PG_GALAT_UMUM;
	}
	dib_lama = (HBITMAP)SelectObject(l->hdc_mem, l->dib);
	l->dib_lama = dib_lama;

	ShowWindow(l->hwnd, SW_SHOWNORMAL);
	UpdateWindow(l->hwnd);
	pg_info("win32: jendela %dx%d dibuat", lebar, tinggi);

	*out = l;
	return PG_OK;
}

pg_galat pg_tutup_layar(pg_layar_t *l)
{
	if (!l) return PG_OK;

	if (l->dib_lama) SelectObject(l->hdc_mem, l->dib_lama);
	if (l->dib)      DeleteObject(l->dib);
	if (l->hdc_mem)  DeleteDC(l->hdc_mem);
	if (l->hwnd) {
		if (l->hdc_layar) ReleaseDC(l->hwnd, l->hdc_layar);
		if (IsWindow(l->hwnd)) DestroyWindow(l->hwnd);
	}
	free(l);
	return PG_OK;
}

pg_galat pg_layar_kueri(pg_layar_t *l, pg_layar_info_t *info)
{
	if (!l || !info) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
	info->lebar  = l->lebar;
	info->tinggi = l->tinggi;
	info->bpp    = 32;
	info->langkah = l->langkah;
	info->format = PG_LAYAR_FORMAT_X8R8G8B8;
	info->double_buffer = l->terkunci ? PG_SALAH : PG_BENAR;
	return PG_OK;
}

pg_galat pg_layar_kunci(pg_layar_t *l, void **piksel, int *langkah)
{
	if (!l || !piksel) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
	if (l->terkunci) PG_KEMBALI_GALAT(PG_GALAT_SIBUK);
	*piksel = l->bits;
	if (langkah) *langkah = l->langkah;
	l->terkunci = PG_BENAR;
	return PG_OK;
}

pg_galat pg_layar_buka_kunci(pg_layar_t *l)
{
	if (!l) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
	l->terkunci = PG_SALAH;
	return PG_OK;
}

pg_galat pg_layar_presentasi(pg_layar_t *l)
{
	MSG msg;

	if (!l) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
	if (!l->hwnd || !l->hdc_mem) PG_KEMBALI_GALAT(PG_GALAT_UMUM);

	BitBlt(l->hdc_layar, 0, 0, l->lebar, l->tinggi,
		l->hdc_mem, 0, 0, SRCCOPY);

	/* Pompa pesan non-input (0..0xFF: WM_PAINT, WM_CLOSE,
	 * WM_DESTROY, WM_QUIT, WM_SIZE, …). Input dipompa oleh
	 * pg_layar_aksi_berikutnya(). */
	while (PeekMessageA(&msg, NULL, 0,
			     PG_LAYAR_FILTER_INPUT_MIN - 1, PM_REMOVE)) {
		if (msg.message == WM_QUIT) {
			l->keluar = PG_BENAR;
			continue;
		}
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}
	return PG_OK;
}

pg_galat pg_layar_tunggu_vsync(pg_layar_t *l)
{
	(void)l;
	/* GDI tidak expose vsync portabel tanpa DirectDraw/DXGI. */
	return PG_GALAT_TANPA;
}

pg_galat pg_layar_pompa_aksi(pg_layar_t *l)
{
	MSG msg;
	if (!l) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
	while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT) {
			l->keluar = PG_BENAR;
			continue;
		}
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}
	return PG_OK;
}

void *pg_layar_handle_native(pg_layar_t *l)
{
	if (!l) return NULL;
	return (void *)l->hwnd;
}

unsigned long pg_layar_jendela_id(pg_layar_t *l)
{
	if (!l) return 0;
	return (unsigned long)l->hwnd;
}

pg_bool pg_layar_punya_aksi(pg_layar_t *l)
{
	MSG msg;
	if (!l) return PG_SALAH;
	if (l->keluar) return PG_BENAR;
	return PeekMessageA(&msg, NULL, PG_LAYAR_FILTER_INPUT_MIN,
			      PG_LAYAR_FILTER_INPUT_MAX, PM_NOREMOVE) ?
		PG_BENAR : PG_SALAH;
}

pg_bool pg_layar_aksi_berikutnya(pg_layar_t *l, pg_aksi_t *out)
{
	MSG msg;

	if (!l || !out) return PG_SALAH;

	/* Bila WM_QUIT sudah ditangkap presentasi, keluarkan satu
	 * peristiwa KELUAR. */
	if (l->keluar) {
		memset(out, 0, sizeof(*out));
		out->tipe = PG_AKSI_KELUAR;
		out->waktu_ms = pg_win_sekarang_ms();
		l->keluar = PG_SALAH;
		return PG_BENAR;
	}

	/* Ambil satu pesan input. */
	if (!PeekMessageA(&msg, NULL, PG_LAYAR_FILTER_INPUT_MIN,
			   PG_LAYAR_FILTER_INPUT_MAX, PM_REMOVE))
		return PG_SALAH;

	TranslateMessage(&msg);
	DispatchMessageA(&msg);

	return pg_win_msg_ke_peristiwa(&msg, out);
}

#endif /* _WIN32 */
