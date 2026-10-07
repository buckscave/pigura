/* ----------------------------------------------------------------------------------------------
 * pigura layar: macos.c - backend CoreGraphics + AppKit untuk macOS
 * ----------------------------------------------------------------------------------------------
 * Membuat NSWindow via Objective-C runtime C API (objc_msgSend,
 * objc_getClass, sel_registerName) tanpa file .m. Back buffer adalah
 * CGBitmapContext 32-bit BGRA premultiplied-skip-first (XRGB on LE)
 * yang dicocokkan layout-nya dengan pg_warna_t (0x00RRGGBB).
 *
 * pg_layar_presentasi(): create CGImageRef via CGBitmapContextCreateImage
 * lalu CGContextDrawImage ke graphics port NSView (coord NSView
 * bottom-left → flip vertical).
 *
 * pg_layar_aksi_berikutnya(): ambil NSEvent berikutnya via
 * [NSApp nextEventMatchingMask:untilDate:inMode:dequeue:] dan
 * terjemahkan type/keyCode/locationInWindow/modifierFlags.
 *
 * Hanya dikompilasi di __APPLE__. Dijaga oleh #ifdef di seluruh file.
 * ---------------------------------------------------------------------------------------------- */
#ifdef __APPLE__

#include "pigura/layar.h"
#include "pigura/galat.h"
#include "pigura/catat.h"
#include "pigura/tipe.h"

#include <stdlib.h>
#include <string.h>

#include <objc/objc-runtime.h>
#include <objc/objc.h>

#include <CoreGraphics/CoreGraphics.h>
#include <Foundation/NSGeometry.h>
#include <Foundation/NSRunLoop.h>

/* ---- Konstanta AppKit (hindari include <AppKit/AppKit.h> yang
 * butuh Objective-C compiler) ---- */

/* NSApplicationActivationPolicy */
#define PG_NSAppActRegular 0

/* NSWindowStyleMask */
#define PG_NSWinTitled          1
#define PG_NSWinClosable        2
#define PG_NSWinMiniaturizable  4
#define PG_NSWinResizable       8

/* NSBackingStoreType */
#define PG_NSBackingBuffered 2

/* NSEventType */
#define PG_NSEvtLeftMouseDown   1
#define PG_NSEvtLeftMouseUp     2
#define PG_NSEvtRightMouseDown  3
#define PG_NSEvtRightMouseUp    4
#define PG_NSEvtMouseMoved      5
#define PG_NSEvtLeftMouseDrag   6
#define PG_NSEvtOtherMouseDown  25
#define PG_NSEvtOtherMouseUp    26
#define PG_NSEvtScrollWheel     22
#define PG_NSEvtKeyDown         10
#define PG_NSEvtKeyUp           11
#define PG_NSEvtFlagsChanged    12

/* NSEventModifierFlags */
#define PG_NSModShift   (1 << 17)
#define PG_NSModControl (1 << 18)
#define PG_NSModOption  (1 << 19)
#define PG_NSModCommand (1 << 20)

#define PG_NSAnyEventMask 0xFFFFFFFFU
#define PG_NSAlphaFirstComponent 1
#define PG_NSBitmapByteOrder32Little 2

extern id NSApp;
extern id const NSDefaultRunLoopMode;
extern id const NSModalPanelRunLoopMode;

typedef id   (*pg_fn_id)(id, SEL);
typedef id   (*pg_fn_id_id)(id, SEL, id);
typedef id   (*pg_fn_id_uint)(id, SEL, unsigned);
typedef id   (*pg_fn_id_rect_uint_uint_bool)(id, SEL, NSRect,
                                              unsigned, unsigned,
                                              unsigned char);
typedef void (*pg_fn_void)(id, SEL);
typedef void (*pg_fn_void_uint)(id, SEL, unsigned);
typedef void (*pg_fn_void_id)(id, SEL, id);
typedef void (*pg_fn_void_double)(id, SEL, double);
typedef BOOL (*pg_fn_bool_id)(id, SEL, id);

struct pg_layar {
	id           window;
	id           view;
	CGContextRef ctx_bitmap;
	void        *bits;
	int          lebar, tinggi, langkah;
	pg_bool      terkunci;
	pg_bool      keluar;
};

/* ---- Helper objc_msgSend typed ---- */

static id pg_send0(id r, SEL s)
{
	return ((pg_fn_id)objc_msgSend)(r, s);
}

static id pg_send1_id(id r, SEL s, id a)
{
	return ((pg_fn_id_id)objc_msgSend)(r, s, a);
}

static id pg_send1_uint(id r, SEL s, unsigned a)
{
	return ((pg_fn_id_uint)objc_msgSend)(r, s, a);
}

static void pg_send0_void(id r, SEL s)
{
	((pg_fn_void)objc_msgSend)(r, s);
}

static void pg_send1_uint_void(id r, SEL s, unsigned a)
{
	((pg_fn_void_uint)objc_msgSend)(r, s, a);
}

static void pg_send1_id_void(id r, SEL s, id a)
{
	((pg_fn_void_id)objc_msgSend)(r, s, a);
}

static id pg_ns_class(const char *nama)
{
	return (id)objc_getClass(nama);
}

static SEL pg_sel(const char *nama)
{
	return sel_registerName(nama);
}

static pg_u32 pg_mac_sekarang_ms(void)
{
	return (pg_u32)(mach_absolute_time() / 1000000u);
}

/* ---- keyCode macOS → pg_tombol (subset HID) ---- */
static int pg_mac_kode_ke_tombol(unsigned short kc)
{
	switch (kc) {
	case 0x35: return PG_TOMBOL_ESCAPE;
	case 0x24: return PG_TOMBOL_ENTER;
	case 0x30: return PG_TOMBOL_TAB;
	case 0x33: return PG_TOMBOL_BACKSPACE;
	case 0x75: return PG_TOMBOL_DELETE;
	case 0x73: return PG_TOMBOL_HOME;
	case 0x77: return PG_TOMBOL_END;
	case 0x74: return PG_TOMBOL_PAGEUP;
	case 0x79: return PG_TOMBOL_PAGEDOWN;
	case 0x7B: return PG_TOMBOL_KIRI;
	case 0x7C: return PG_TOMBOL_KANAN;
	case 0x7E: return PG_TOMBOL_ATAS;
	case 0x7D: return PG_TOMBOL_BAWAH;
	case 0x31: return PG_TOMBOL_SPASI;
	case 0x38: return PG_TOMBOL_LSHIFT;
	case 0x3C: return PG_TOMBOL_RSHIFT;
	case 0x3B: return PG_TOMBOL_LCTRL;
	case 0x3E: return PG_TOMBOL_RCTRL;
	case 0x3A: return PG_TOMBOL_LALT;
	case 0x3D: return PG_TOMBOL_RALT;
	case 0x37: return PG_TOMBOL_LMETA;
	case 0x36: return PG_TOMBOL_RMETA;
	case 0x7A: return PG_TOMBOL_F1;
	case 0x78: return PG_TOMBOL_F2;
	case 0x63: return PG_TOMBOL_F3;
	case 0x76: return PG_TOMBOL_F4;
	case 0x60: return PG_TOMBOL_F5;
	case 0x61: return PG_TOMBOL_F6;
	case 0x62: return PG_TOMBOL_F7;
	case 0x64: return PG_TOMBOL_F8;
	case 0x65: return PG_TOMBOL_F9;
	case 0x6D: return PG_TOMBOL_F10;
	case 0x67: return PG_TOMBOL_F11;
	case 0x6F: return PG_TOMBOL_F12;
	default:
		switch (kc) {
		case 0x1D: return (int)'0';
		case 0x12: return (int)'1';
		case 0x13: return (int)'2';
		case 0x14: return (int)'3';
		case 0x15: return (int)'4';
		case 0x17: return (int)'5';
		case 0x16: return (int)'6';
		case 0x1A: return (int)'7';
		case 0x1C: return (int)'8';
		case 0x19: return (int)'9';
		case 0x0C: return (int)'Q';
		case 0x0D: return (int)'W';
		case 0x0E: return (int)'E';
		case 0x0F: return (int)'R';
		case 0x11: return (int)'T';
		case 0x10: return (int)'Y';
		case 0x20: return (int)'U';
		case 0x21: return (int)'I';
		case 0x22: return (int)'O';
		case 0x23: return (int)'P';
		case 0x00: return (int)'A';
		case 0x01: return (int)'S';
		case 0x02: return (int)'D';
		case 0x03: return (int)'F';
		case 0x05: return (int)'G';
		case 0x04: return (int)'H';
		case 0x26: return (int)'J';
		case 0x28: return (int)'K';
		case 0x25: return (int)'L';
		case 0x06: return (int)'Z';
		case 0x07: return (int)'X';
		case 0x08: return (int)'C';
		case 0x09: return (int)'V';
		case 0x0B: return (int)'B';
		case 0x2D: return (int)'N';
		case 0x2E: return (int)'M';
		case 0x1E: return (int)'-';
		case 0x2A: return (int)'[';
		case 0x2B: return (int)']';
		case 0x2C: return (int)'\\';
		case 0x29: return (int)'`';
		case 0x27: return (int)'\'';
		case 0x2F: return (int)'.';
		case 0x32: return (int)'/';
		case 0x52: return (int)'0';
		case 0x53: return (int)'1';
		case 0x54: return (int)'2';
		case 0x55: return (int)'3';
		case 0x56: return (int)'4';
		case 0x57: return (int)'5';
		case 0x58: return (int)'6';
		case 0x59: return (int)'7';
		case 0x5B: return (int)'8';
		case 0x5C: return (int)'9';
		case 0x41: return (int)'.';
		case 0x45: return (int)'+';
		case 0x4E: return (int)'-';
		case 0x43: return (int)'*';
		case 0x4B: return (int)'/';
		case 0x4C: return PG_TOMBOL_ENTER;
		default: return PG_TOMBOL_KOSONG;
		}
	}
}

static int pg_mac_modifier(id ev)
{
	unsigned long f = (unsigned long)((id(*)(id, SEL))
		objc_msgSend)(ev, pg_sel("modifierFlags"));
	int m = 0;

	if (f & PG_NSModShift)   m |= PG_MOD_SHIFT;
	if (f & PG_NSModControl) m |= PG_MOD_CTRL;
	if (f & PG_NSModOption)   m |= PG_MOD_ALT;
	if (f & PG_NSModCommand) m |= PG_MOD_META;
	return m;
}

static pg_titik_t pg_mac_lokasi_ke_titik(id ev, id view)
{
	NSPoint p;
	NSPoint lp;
	p = ((NSPoint(*)(id, SEL))objc_msgSend)(ev,
		pg_sel("locationInWindow"));
	lp = ((NSPoint(*)(id, SEL, NSPoint))objc_msgSend)(view,
		pg_sel("convertPoint:fromView:"), p, (id)NULL);
	return pg_buat_titik((int)lp.x, (int)lp.y);
}

/* ---- Implementasi publik ---- */

pg_galat pg_buka_layar(pg_layar_t **out, const pg_layar_config_t *cfg)
{
	pg_layar_t *l;
	pg_layar_config_t def;
	id cls_app, cls_win, cls_view, cls_date;
	id ns_app, window, view;
	NSRect rect;
	CGRect cbounds;
	CGColorSpaceRef cs;
	int lebar, tinggi;
	unsigned style_mask;

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

	cls_app = pg_ns_class("NSApplication");
	cls_win = pg_ns_class("NSWindow");
	cls_view = pg_ns_class("NSView");
	cls_date = pg_ns_class("NSDate");
	if (!cls_app || !cls_win || !cls_view || !cls_date) {
		pg_set_galat(PG_GALAT_TIDAKADA,
			      "kelas AppKit tidak ditemukan");
		free(l);
		return PG_GALAT_TIDAKADA;
	}

	/* NSApp = [NSApplication sharedApplication] */
	ns_app = pg_send0(cls_app, pg_sel("sharedApplication"));
	if (!ns_app) {
		pg_set_galat(PG_GALAT_UMUM,
			      "sharedApplication gagal");
		free(l);
		return PG_GALAT_UMUM;
	}
	NSApp = ns_app;
	pg_send1_uint_void(NSApp, pg_sel("setActivationPolicy:"),
			    PG_NSAppActRegular);

	/* NSRect content rect (origin doesn't matter for content). */
	rect.origin.x = 0.0;
	rect.origin.y = 0.0;
	rect.size.width  = (double)lebar;
	rect.size.height = (double)tinggi;
	style_mask = PG_NSWinTitled | PG_NSWinClosable |
		PG_NSWinMiniaturizable;

	/* window = [[NSWindow alloc] initWithContentRect:...
	 *                                       styleMask:...
	 *                                         backing:...
	 *                                           defer:NO] */
	window = ((pg_fn_id_rect_uint_uint_bool)objc_msgSend)(
		cls_win, pg_sel("alloc"), (id)0);
	if (!window) {
		pg_set_galat(PG_GALAT_MEMORI, "NSWindow alloc gagal");
		free(l);
		return PG_GALAT_MEMORI;
	}
	(void)cls_view; /* view diambil dari window */
	window = ((pg_fn_id_rect_uint_uint_bool)objc_msgSend)(
		window,
		pg_sel("initWithContentRect:styleMask:backing:defer:"),
		rect, style_mask, PG_NSBackingBuffered,
		(unsigned char)0);
	if (!window) {
		pg_set_galat(PG_GALAT_UMUM, "initWithContentRect gagal");
		free(l);
		return PG_GALAT_UMUM;
	}
	l->window = window;

	/* Setel judul. */
	if (cfg->judul) {
		id s = ((id(*)(id, SEL, const char *))objc_msgSend)(
			pg_ns_class("NSString"),
			pg_sel("stringWithUTF8String:"),
			cfg->judul);
		if (s) pg_send1_id_void(window, pg_sel("setTitle:"), s);
	}

	/* Ambil contentView default. */
	view = pg_send0(window, pg_sel("contentView"));
	if (!view) {
		pg_set_galat(PG_GALAT_UMUM, "contentView NULL");
		pg_send0_void(window, pg_sel("release"));
		free(l);
		return PG_GALAT_UMUM;
	}
	l->view = view;

	/* Buat back buffer CGBitmapContext 32-bit XRGB (alpha skip first)
	 * + byte order 32 little → BGRX in memory (cocok dgn pg_warna_t
	 * 0x00RRGGBB di LE). */
	cs = CGColorSpaceCreateDeviceRGB();
	if (!cs) {
		pg_set_galat(PG_GALAT_UMUM,
			      "CGColorSpaceCreateDeviceRGB gagal");
		pg_send0_void(window, pg_sel("release"));
		free(l);
		return PG_GALAT_UMUM;
	}
	l->ctx_bitmap = CGBitmapContextCreate(NULL, lebar, tinggi, 8,
		(size_t)lebar * 4, cs,
		kCGImageAlphaNoneSkipFirst |
		kCGBitmapByteOrder32Little);
	CGColorSpaceRelease(cs);
	if (!l->ctx_bitmap) {
		pg_set_galat(PG_GALAT_UMUM,
			      "CGBitmapContextCreate gagal");
		pg_send0_void(window, pg_sel("release"));
		free(l);
		return PG_GALAT_UMUM;
	}
	l->bits = CGBitmapContextGetData(l->ctx_bitmap);
	if (!l->bits) {
		pg_set_galat(PG_GALAT_UMUM, "bitmap data NULL");
		CGContextRelease(l->ctx_bitmap);
		pg_send0_void(window, pg_sel("release"));
		free(l);
		return PG_GALAT_UMUM;
	}
	cbounds.origin.x = 0.0;
	cbounds.origin.y = 0.0;
	cbounds.size.width  = (double)lebar;
	cbounds.size.height = (double)tinggi;
	(void)cbounds;
	(void)cls_date;

	/* Tampilkan jendela. */
	pg_send0_void(window, pg_sel("makeKeyAndOrderFront:"));
	pg_send1_uint_void(NSApp, pg_sel("activateIgnoringOtherApps:"),
			    1u);
	pg_info("macos: jendela %dx%d dibuat", lebar, tinggi);

	*out = l;
	return PG_OK;
}

pg_galat pg_tutup_layar(pg_layar_t *l)
{
	if (!l) return PG_OK;
	if (l->ctx_bitmap) CGContextRelease(l->ctx_bitmap);
	if (l->window) {
		pg_send0_void(l->window, pg_sel("close"));
		pg_send0_void(l->window, pg_sel("release"));
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
	CGImageRef img;
	id gctx;
	CGContextRef ctx;
	CGRect drect;
	double h;

	if (!l) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
	if (!l->ctx_bitmap || !l->view) PG_KEMBALI_GALAT(PG_GALAT_UMUM);

	img = CGBitmapContextCreateImage(l->ctx_bitmap);
	if (!img) PG_KEMBALI_GALAT(PG_GALAT_UMUM);

	/* [view lockFocusIfCanDraw] — fallback lockFocus. */
	pg_send0_void(l->view, pg_sel("lockFocusIfCanDraw"));

	gctx = pg_send0(pg_ns_class("NSGraphicsContext"),
		pg_sel("currentContext"));
	if (gctx) {
		ctx = (CGContextRef)((id(*)(id, SEL))objc_msgSend)(
			gctx, pg_sel("graphicsPort"));
		if (ctx) {
			/* NSView origin bottom-left → flip Y supaya
			 * bitmap top-down kita tergambar tegak. */
			h = (double)l->tinggi;
			CGContextSaveGState(ctx);
			CGContextTranslateCTM(ctx, 0.0, h);
			CGContextScaleCTM(ctx, 1.0, -1.0);
			drect.origin.x = 0.0;
			drect.origin.y = 0.0;
			drect.size.width  = (double)l->lebar;
			drect.size.height = (double)l->tinggi;
			CGContextDrawImage(ctx, drect, img);
			CGContextRestoreGState(ctx);
		}
	}
	pg_send0_void(l->view, pg_sel("unlockFocus"));
	CGImageRelease(img);

	/* Flush AppKit buffering. */
	pg_send0_void(NSApp, pg_sel("flushWindow"));
	return PG_OK;
}

pg_galat pg_layar_tunggu_vsync(pg_layar_t *l)
{
	(void)l;
	/* CoreGraphics tidak expose vsync portabel tanpa CVDisplayLink. */
	return PG_GALAT_TANPA;
}

pg_galat pg_layar_pompa_aksi(pg_layar_t *l)
{
	if (!l) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
	/* Drain pendek — pull 16 aksi, dispatch via peristiwa_berikutnya. */
	int n;
	pg_aksi_t ev;
	for (n = 0; n < 16; n++) {
		if (!pg_layar_aksi_berikutnya(l, &ev)) break;
	}
	return PG_OK;
}

void *pg_layar_handle_native(pg_layar_t *l)
{
	if (!l) return NULL;
	return (void *)l->window;
}

unsigned long pg_layar_jendela_id(pg_layar_t *l)
{
	if (!l || !l->window) return 0;
	return (unsigned long)(unsigned long long)(uintptr_t)l->window;
}

pg_bool pg_layar_punya_aksi(pg_layar_t *l)
{
	if (!l) return PG_SALAH;
	if (l->keluar) return PG_BENAR;
	/* Cek dengan dequeue:NO — peek saja. */
	id ev = ((id(*)(id, SEL, unsigned long, id, id,
		unsigned char))objc_msgSend)(
		NSApp,
		pg_sel("nextEventMatchingMask:untilDate:inMode:dequeue:"),
		(unsigned long)PG_NSAnyEventMask,
		(id)NULL, NSDefaultRunLoopMode,
		(unsigned char)0);
	return ev ? PG_BENAR : PG_SALAH;
}

pg_bool pg_layar_aksi_berikutnya(pg_layar_t *l,
                                        pg_aksi_t *out)
{
	id ev;
	int etype;
	unsigned short kc;
	int btn;

	if (!l || !out) return PG_SALAH;

	if (l->keluar) {
		memset(out, 0, sizeof(*out));
		out->tipe = PG_AKSI_KELUAR;
		out->waktu_ms = pg_mac_sekarang_ms();
		l->keluar = PG_SALAH;
		return PG_BENAR;
	}

	ev = ((id(*)(id, SEL, unsigned long, id, id,
		unsigned char))objc_msgSend)(
		NSApp,
		pg_sel("nextEventMatchingMask:untilDate:inMode:dequeue:"),
		(unsigned long)PG_NSAnyEventMask,
		(id)NULL, NSDefaultRunLoopMode,
		(unsigned char)1);
	if (!ev) return PG_SALAH;

	etype = (int)((long(*)(id, SEL))objc_msgSend)(ev, pg_sel("type"));

	memset(out, 0, sizeof(*out));
	out->waktu_ms = pg_mac_sekarang_ms();

	switch (etype) {
	case PG_NSEvtKeyDown:
		out->tipe = PG_AKSI_TOMBOL_TURUN;
		kc = (unsigned short)((unsigned short(*)(id, SEL))
			objc_msgSend)(ev, pg_sel("keyCode"));
		out->tombol = pg_mac_kode_ke_tombol(kc);
		out->modifier = pg_mac_modifier(ev);
		{ id chars = ((id(*)(id, SEL))objc_msgSend)(ev, pg_sel("characters"));
		  if (chars) {
			const char *s = ((const char *(*)(id, SEL))objc_msgSend)(chars, pg_sel("UTF8String"));
			if (s && (unsigned char)s[0] >= 32 && (unsigned char)s[0] < 127)
				out->unicode = (pg_u32)(unsigned char)s[0];
		  }
		}
		return PG_BENAR;
	case PG_NSEvtKeyUp:
		out->tipe = PG_AKSI_TOMBOL_NAIK;
		kc = (unsigned short)((unsigned short(*)(id, SEL))
			objc_msgSend)(ev, pg_sel("keyCode"));
		out->tombol = pg_mac_kode_ke_tombol(kc);
		out->modifier = pg_mac_modifier(ev);
		return PG_BENAR;
	case PG_NSEvtFlagsChanged:
		/* Modifier berubah — emit TOMBOL_TURUN/NAIK untuk key
		 * yang berubah state. Sederhana: skip untuk v0.1. */
		return PG_SALAH;
	case PG_NSEvtLeftMouseDown:
		out->tipe = PG_AKSI_TETIKUS_TEKAN;
		out->tetik_tombol = PG_TETIKUS_KIRI;
		out->tetik_pos = pg_mac_lokasi_ke_titik(ev, l->view);
		return PG_BENAR;
	case PG_NSEvtLeftMouseUp:
		out->tipe = PG_AKSI_TETIKUS_LEPAS;
		out->tetik_tombol = PG_TETIKUS_KIRI;
		out->tetik_pos = pg_mac_lokasi_ke_titik(ev, l->view);
		return PG_BENAR;
	case PG_NSEvtRightMouseDown:
		out->tipe = PG_AKSI_TETIKUS_TEKAN;
		out->tetik_tombol = PG_TETIKUS_KANAN;
		out->tetik_pos = pg_mac_lokasi_ke_titik(ev, l->view);
		return PG_BENAR;
	case PG_NSEvtRightMouseUp:
		out->tipe = PG_AKSI_TETIKUS_LEPAS;
		out->tetik_tombol = PG_TETIKUS_KANAN;
		out->tetik_pos = pg_mac_lokasi_ke_titik(ev, l->view);
		return PG_BENAR;
	case PG_NSEvtOtherMouseDown:
		out->tipe = PG_AKSI_TETIKUS_TEKAN;
		btn = (int)((long(*)(id, SEL))objc_msgSend)(ev,
			pg_sel("buttonNumber"));
		out->tetik_tombol = (btn == 2) ? PG_TETIKUS_TENGAH :
			PG_TETIKUS_KOSONG;
		out->tetik_pos = pg_mac_lokasi_ke_titik(ev, l->view);
		return PG_BENAR;
	case PG_NSEvtOtherMouseUp:
		out->tipe = PG_AKSI_TETIKUS_LEPAS;
		btn = (int)((long(*)(id, SEL))objc_msgSend)(ev,
			pg_sel("buttonNumber"));
		out->tetik_tombol = (btn == 2) ? PG_TETIKUS_TENGAH :
			PG_TETIKUS_KOSONG;
		out->tetik_pos = pg_mac_lokasi_ke_titik(ev, l->view);
		return PG_BENAR;
	case PG_NSEvtMouseMoved:
	case PG_NSEvtLeftMouseDrag:
		out->tipe = PG_AKSI_TETIKUS_GERAK;
		out->tetik_pos = pg_mac_lokasi_ke_titik(ev, l->view);
		return PG_BENAR;
	case PG_NSEvtScrollWheel:
		out->tipe = PG_AKSI_TETIKUS_GULIR;
		out->roda_dy = (((double(*)(id, SEL))objc_msgSend)(
			ev, pg_sel("scrollingDeltaY")) > 0.0) ?
			1 : -1;
		return PG_BENAR;
	default:
		return PG_SALAH;
	}
}

#endif /* __APPLE__ */
