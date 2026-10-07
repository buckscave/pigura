/* ----------------------------------------------------------------------------------------------
 * pigura layar: x11.c - backend X11 untuk desktop Linux
 * ----------------------------------------------------------------------------------------------
 * Membuat jendela X11 dan menyediakan buffer piksel yang diblitter
 * ke jendela tiap pg_layar_presentasi(). Pakai XShmPutImage bila
 * Shared Memory extension tersedia (cepat); fallback XPutImage bila
 * tidak.
 *
 * Jendela ini adalah "wadah framebuffer" — pigura menggambar ke
 * back buffer software, lalu mempresentasikannya ke jendela X11
 * seolah-olah itu framebuffer. Pola yang sama dipakai SDL/GLFW/SFML.
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/layar.h"
#include "pigura/galat.h"
#include "pigura/catat.h"
#include "pigura/tipe.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>

#ifdef HAVE_XSHM
#  include <sys/ipc.h>
#  include <sys/shm.h>
#  include <X11/extensions/XShm.h>
#endif

struct pg_layar {
        Display         *disp;
        Window           win;
        GC               gc;
        XImage          *img;
        int              lebar, tinggi, langkah;
        int              buf_lebar, buf_tinggi; /* ukuran back buffer */
        pg_warna_t      *piksel;
        pg_bool         terkunci;
        Atom             wm_hapus;

#ifdef HAVE_XSHM
        XShmSegmentInfo  shminfo;
        pg_bool         pakai_shm;
#endif
};

static pg_galat pg_layar_buat_jendela(pg_layar_t *l, int lebar, int tinggi,
                                       const char *judul)
{
        XSetWindowAttributes attr;
        unsigned long mask;
        XSizeHints hints;

        attr.event_mask = ExposureMask | StructureNotifyMask |
                          KeyPressMask | KeyReleaseMask |
                          ButtonPressMask | ButtonReleaseMask |
                          PointerMotionMask | FocusChangeMask |
                          EnterWindowMask | LeaveWindowMask;
        attr.background_pixel = BlackPixel(l->disp, 0);
        attr.border_pixel = BlackPixel(l->disp, 0);

        mask = CWEventMask | CWBackPixel | CWBorderPixel;

        l->win = XCreateWindow(l->disp,
                                DefaultRootWindow(l->disp),
                                0, 0,
                                lebar, tinggi,
                                0,
                                DefaultDepth(l->disp, 0),
                                InputOutput,
                                CopyFromParent,
                                mask, &attr);
        if (!l->win) {
                pg_set_galat(PG_GALAT_UMUM, "XCreateWindow gagal");
                return PG_GALAT_UMUM;
        }

        /* Setel judul jendela. */
        XStoreName(l->disp, l->win, judul ? judul : "pigura");

        /* Hint ukuran: izinkan resize bebas. Set minimum 100x100
         * supaya tidak terlalu kecil, tapi tidak batas maksimum. */
        memset(&hints, 0, sizeof(hints));
        hints.flags = PMinSize;
        hints.min_width = 100;
        hints.min_height = 100;
        XSetWMNormalHints(l->disp, l->win, &hints);

        /* Atom WM_DELETE_WINDOW untuk deteksi tombol close. */
        l->wm_hapus = XInternAtom(l->disp, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(l->disp, l->win, &l->wm_hapus, 1);

        /* Peta (tampilkan) jendela. */
        XMapWindow(l->disp, l->win);

        l->gc = XCreateGC(l->disp, l->win, 0, NULL);
        XSetForeground(l->disp, l->gc, WhitePixel(l->disp, 0));
        XSetBackground(l->disp, l->gc, BlackPixel(l->disp, 0));

        return PG_OK;
}

static pg_galat pg_layar_buat_image(pg_layar_t *l)
{
#ifdef HAVE_XSHM
        if (0 && XShmQueryExtension(l->disp)) {
                l->img = XShmCreateImage(l->disp,
                                          DefaultVisual(l->disp, 0),
                                          DefaultDepth(l->disp, 0),
                                          ZPixmap, NULL,
                                          &l->shminfo,
                                          l->lebar, l->tinggi);
                if (l->img) {
                        l->shminfo.shmid = shmget(IPC_PRIVATE,
                                                  l->img->bytes_per_line *
                                                  l->img->height,
                                                  IPC_CREAT | 0777);
                        if (l->shminfo.shmid < 0) {
                                XDestroyImage(l->img);
                                l->img = NULL;
                        } else {
                                l->shminfo.shmaddr = l->img->data =
                                        shmat(l->shminfo.shmid, NULL, 0);
                                l->shminfo.readOnly = False;
                                if (XShmAttach(l->disp, &l->shminfo) == 0) {
                                        shmdt(l->shminfo.shmaddr);
                                        shmctl(l->shminfo.shmid,
                                               IPC_RMID, NULL);
                                        XDestroyImage(l->img);
                                        l->img = NULL;
                                } else {
                                        l->pakai_shm = PG_BENAR;
                                        pg_info("x11: XShm aktif");
                                }
                        }
                }
        }
#endif

        if (!l->img) {
                l->img = XCreateImage(l->disp,
                                       DefaultVisual(l->disp, 0),
                                       DefaultDepth(l->disp, 0),
                                       ZPixmap, 0, 0,
                                       l->lebar, l->tinggi,
                                       32, 0);
                if (!l->img) {
                        pg_set_galat(PG_GALAT_UMUM, "XCreateImage gagal");
                        return PG_GALAT_UMUM;
                }
                l->img->data = (char *)malloc(
                        (size_t)l->img->bytes_per_line *
                        l->img->height);
                if (!l->img->data) {
                        XDestroyImage(l->img);
                        l->img = NULL;
                        PG_KEMBALI_GALAT(PG_GALAT_MEMORI);
                }
        }
        return PG_OK;
}

pg_galat pg_buka_layar(pg_layar_t **out, const pg_layar_config_t *cfg)
{
        pg_layar_t *l;
        pg_layar_config_t def;
        pg_galat e;
        int lebar, tinggi;

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
        l->langkah = lebar * (int)sizeof(pg_warna_t);

        l->disp = XOpenDisplay(NULL);
        if (!l->disp) {
                pg_set_galat(PG_GALAT_TIDAKADA, "tidak bisa membuka "
                             "display X11 — apakah X server berjalan?");
                free(l);
                return PG_GALAT_TIDAKADA;
        }

        e = pg_layar_buat_jendela(l, lebar, tinggi, cfg->judul);
        if (e != PG_OK) {
                XCloseDisplay(l->disp);
                free(l);
                return e;
        }

        e = pg_layar_buat_image(l);
        if (e != PG_OK) {
                XFreeGC(l->disp, l->gc);
                XDestroyWindow(l->disp, l->win);
                XCloseDisplay(l->disp);
                free(l);
                return e;
        }

        /* Back buffer software terpisah (X11 bisa di BGRA, kita XRGB). */
        l->piksel = (pg_warna_t *)calloc((size_t)lebar * tinggi,
                                          sizeof(pg_warna_t));
        if (!l->piksel) {
                if (l->img) XDestroyImage(l->img);
                XFreeGC(l->disp, l->gc);
                XDestroyWindow(l->disp, l->win);
                XCloseDisplay(l->disp);
                free(l);
                PG_KEMBALI_GALAT(PG_GALAT_MEMORI);
        }

        XFlush(l->disp);
        pg_info("x11: jendela %dx%d dibuat", lebar, tinggi);

        *out = l;
        return PG_OK;
}

pg_galat pg_tutup_layar(pg_layar_t *l)
{
        if (!l) return PG_OK;

        if (l->img) {
#ifdef HAVE_XSHM
                if (l->pakai_shm) {
                        XShmDetach(l->disp, &l->shminfo);
                        shmdt(l->shminfo.shmaddr);
                        shmctl(l->shminfo.shmid, IPC_RMID, NULL);
                }
#endif
                if (!l->pakai_shm && l->img->data)
                        free(l->img->data);
                l->img->data = NULL;
                XDestroyImage(l->img);
                l->img = NULL;
        }
        if (l->piksel) free(l->piksel);
        if (l->gc)     XFreeGC(l->disp, l->gc);
        if (l->win)    XDestroyWindow(l->disp, l->win);
        if (l->disp)   XCloseDisplay(l->disp);
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
        /* Jika ukuran layar berubah (ConfigureNotify update
         * l->lebar/tinggi), reallocate back buffer. */
        if (l->buf_lebar != l->lebar ||
            l->buf_tinggi != l->tinggi) {
                if (l->piksel) free(l->piksel);
                l->piksel = (pg_warna_t *)calloc(
                        (size_t)l->lebar * l->tinggi,
                        sizeof(pg_warna_t));
                if (!l->piksel)
                        PG_KEMBALI_GALAT(PG_GALAT_MEMORI);
                l->buf_lebar = l->lebar;
                l->buf_tinggi = l->tinggi;
        }
        *piksel = l->piksel;
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
        int y;
        int bpp, bpl;
        Visual *vis;
        pg_u32 r_mask, g_mask, b_mask;
        int r_shift, g_shift, b_shift;
        if (!l) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);

        /* Reallocate XImage jika ukuran berubah. */
        if (!l->img || l->img->width != l->lebar ||
            l->img->height != l->tinggi) {
                if (l->img) {
                        if (l->img->data) free(l->img->data);
                        l->img->data = NULL;
                        XDestroyImage(l->img);
                        l->img = NULL;
                }
                pg_layar_buat_image(l);
                if (!l->img) PG_KEMBALI_GALAT(PG_GALAT_UMUM);
        }

        vis = DefaultVisual(l->disp, 0);
        r_mask = (pg_u32)vis->red_mask;
        g_mask = (pg_u32)vis->green_mask;
        b_mask = (pg_u32)vis->blue_mask;
        r_shift = 0; g_shift = 0; b_shift = 0;
        {
                pg_u32 m = r_mask;
                while (m && !(m & 1)) { r_shift++; m >>= 1; }
        }
        {
                pg_u32 m = g_mask;
                while (m && !(m & 1)) { g_shift++; m >>= 1; }
        }
        {
                pg_u32 m = b_mask;
                while (m && !(m & 1)) { b_shift++; m >>= 1; }
        }

        bpp = l->img->bits_per_pixel;
        bpl = l->img->bytes_per_line;

        /* Copy back buffer ke XImage dengan konversi format sekali jalan.
         * pg_warna_t = 0x00RRGGBB. Konversi ke mask Visual X11. */
        if (bpp == 32) {
                char *dst = l->img->data;
                pg_warna_t *src = l->piksel;
                /* Jika mask = 0xFF0000/0xFF00/0xFF (standard RGB),
                 * bisa langsung copy. */
                if (r_mask == 0xFF0000 && g_mask == 0x00FF00 &&
                    b_mask == 0x0000FF) {
                        /* Final surface sudah opaque (semua widget render
                         * di atas latar). Jangan premultiply — copy RGB
                         * langsung, force alpha=255.
                         * Untuk pixel alpha=0 (pojok rounded transparan):
                         * ganti ke warna panel supaya tidak jadi hitam
                         * di X11. */
                        for (y = 0; y < l->tinggi; y++) {
                                pg_u32 *drow = (pg_u32 *)(dst + y * bpl);
                                pg_warna_t *srow = src + y * l->lebar;
                                int x;
                                for (x = 0; x < l->lebar; x++) {
                                        pg_warna_t c = srow[x];
                                        if (PG_A(c) == 0) {
                                                /* Transparan: pakai panel */
                                                drow[x] = 0xFFE6E6E6UL;
                                        } else {
                                                /* Opaque/semi: copy RGB,
                                                 * force alpha=255 */
                                                drow[x] = c | 0xFF000000UL;
                                        }
                                }
                        }
                } else {
                        int x;
                        for (y = 0; y < l->tinggi; y++) {
                                pg_u32 *drow = (pg_u32 *)(dst + y * bpl);
                                pg_warna_t *srow = src + y * l->lebar;
                                for (x = 0; x < l->lebar; x++) {
                                        pg_warna_t c = srow[x];
                                        int r, g, b;
                                        if (PG_A(c) == 0) {
                                                r = 0xE6; g = 0xE6; b = 0xE6;
                                        } else {
                                                r = PG_R(c);
                                                g = PG_G(c);
                                                b = PG_B(c);
                                        }
                                        drow[x] =
                                          ((pg_u32)r << r_shift) |
                                          ((pg_u32)g << g_shift) |
                                          ((pg_u32)b << b_shift) |
                                          (0xFF000000UL);
                                }
                        }
                }
        } else if (bpp == 24) {
                char *dst = l->img->data;
                pg_warna_t *src = l->piksel;
                int x;
                for (y = 0; y < l->tinggi; y++) {
                        char *drow = dst + y * bpl;
                        pg_warna_t *srow = src + y * l->lebar;
                        for (x = 0; x < l->lebar; x++) {
                                pg_warna_t c = srow[x];
                                int r, g, b;
                                if (PG_A(c) == 0) {
                                        r = 0xE6; g = 0xE6; b = 0xE6;
                                } else {
                                        r = PG_R(c);
                                        g = PG_G(c);
                                        b = PG_B(c);
                                }
                                drow[x*3]   = (char)b;
                                drow[x*3+1] = (char)g;
                                drow[x*3+2] = (char)r;
                        }
                }
        } else {
                /* Fallback: XPutPixel (lambat tapi aman). */
                int x;
                pg_warna_t *src = l->piksel;
                for (y = 0; y < l->tinggi; y++) {
                        pg_warna_t *srow = src + y * l->lebar;
                        for (x = 0; x < l->lebar; x++) {
                                XPutPixel(l->img, x, y, srow[x]);
                        }
                }
        }

#ifdef HAVE_XSHM
        /* XShm dinonaktifkan — race condition dengan shared memory
         * menyebabkan visual corrupt di beberapa sistem. XPutImage
         * lebih aman dan cukup cepat dengan memcpy. */
        if (0 && l->pakai_shm) {
                XShmPutImage(l->disp, l->win, l->gc, l->img,
                              0, 0, 0, 0, l->lebar, l->tinggi, False);
        } else
#endif
        {
                XPutImage(l->disp, l->win, l->gc, l->img,
                           0, 0, 0, 0, l->lebar, l->tinggi);
        }

        XFlush(l->disp);
        return PG_OK;
}

pg_galat pg_layar_tunggu_vsync(pg_layar_t *l)
{
        (void)l;
        /* X11 tidak punya vsync portabel di level core. XSync adalah
         * sinkronisasi server, bukan refresh monitor. */
        return PG_GALAT_TANPA;
}

pg_galat pg_layar_pompa_aksi(pg_layar_t *l)
{
        /* Pompa peristiwa X11 — di sini hanya drain queue tanpa
         * konsumsi; backend masukan akan baca melalui XNextEvent. */
        if (!l) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
        XFlush(l->disp);
        return PG_OK;
}

void *pg_layar_handle_native(pg_layar_t *l)
{
        if (!l) return NULL;
        return (void *)l->disp;
}

unsigned long pg_layar_jendela_id(pg_layar_t *l)
{
        if (!l) return 0;
        return (unsigned long)l->win;
}

pg_bool pg_layar_punya_aksi(pg_layar_t *l)
{
        if (!l || !l->disp) return PG_SALAH;
        return XPending(l->disp) > 0 ? PG_BENAR : PG_SALAH;
}

/* Konversi KeySym → pg_tombol (di-copy dari masukan/x11.c untuk
 * menghindari dependensi silang). */
static int pg_x11_ksym_ke_tombol(KeySym ks)
{
        switch (ks) {
        case XK_Escape:       return PG_TOMBOL_ESCAPE;
        case XK_Return:       return PG_TOMBOL_ENTER;
        case XK_Tab:          return PG_TOMBOL_TAB;
        case XK_BackSpace:   return PG_TOMBOL_BACKSPACE;
        case XK_Insert:      return PG_TOMBOL_INSERT;
        case XK_Delete:      return PG_TOMBOL_DELETE;
        case XK_Home:         return PG_TOMBOL_HOME;
        case XK_End:          return PG_TOMBOL_END;
        case XK_Page_Up:     return PG_TOMBOL_PAGEUP;
        case XK_Page_Down:   return PG_TOMBOL_PAGEDOWN;
        case XK_Left:         return PG_TOMBOL_KIRI;
        case XK_Right:        return PG_TOMBOL_KANAN;
        case XK_Up:           return PG_TOMBOL_ATAS;
        case XK_Down:         return PG_TOMBOL_BAWAH;
        case XK_space:        return PG_TOMBOL_SPASI;
        case XK_Shift_L:      return PG_TOMBOL_LSHIFT;
        case XK_Shift_R:      return PG_TOMBOL_RSHIFT;
        case XK_Control_L:    return PG_TOMBOL_LCTRL;
        case XK_Control_R:    return PG_TOMBOL_RCTRL;
        case XK_Alt_L:        return PG_TOMBOL_LALT;
        case XK_Alt_R:        return PG_TOMBOL_RALT;
        case XK_F1:           return PG_TOMBOL_F1;
        case XK_F2:           return PG_TOMBOL_F2;
        case XK_F3:           return PG_TOMBOL_F3;
        case XK_F4:           return PG_TOMBOL_F4;
        case XK_F5:           return PG_TOMBOL_F5;
        case XK_F6:           return PG_TOMBOL_F6;
        case XK_F7:           return PG_TOMBOL_F7;
        case XK_F8:           return PG_TOMBOL_F8;
        case XK_F9:           return PG_TOMBOL_F9;
        case XK_F10:          return PG_TOMBOL_F10;
        case XK_F11:          return PG_TOMBOL_F11;
        case XK_F12:          return PG_TOMBOL_F12;
        /* --- Keypad (numpad) --- */
        case XK_KP_0:        return (int)'0';
        case XK_KP_1:        return (int)'1';
        case XK_KP_2:        return (int)'2';
        case XK_KP_3:        return (int)'3';
        case XK_KP_4:        return (int)'4';
        case XK_KP_5:        return (int)'5';
        case XK_KP_6:        return (int)'6';
        case XK_KP_7:        return (int)'7';
        case XK_KP_8:        return (int)'8';
        case XK_KP_9:        return (int)'9';
        case XK_KP_Decimal:  return (int)'.';
        case XK_KP_Add:      return (int)'+';
        case XK_KP_Subtract: return (int)'-';
        case XK_KP_Multiply: return (int)'*';
        case XK_KP_Divide:   return (int)'/';
        case XK_KP_Enter:    return PG_TOMBOL_ENTER;
        case XK_KP_Space:    return PG_TOMBOL_SPASI;
        case XK_KP_Tab:      return PG_TOMBOL_TAB;
        case XK_KP_Equal:    return (int)'=';
        default:
                /* Semua printable ASCII 32..126 (simbol + alfanumerik). */
                if (ks >= 32 && ks <= 126) return (int)ks;
                return PG_TOMBOL_KOSONG;
        }
}

static pg_u32 pg_x11_sekarang_ms(void)
{
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (pg_u32)(ts.tv_sec * 1000u + ts.tv_nsec / 1000000u);
}

pg_bool pg_layar_aksi_berikutnya(pg_layar_t *l, pg_aksi_t *out)
{
        XEvent ev;
        if (!l || !l->disp || !out) return PG_SALAH;
        if (XPending(l->disp) <= 0) return PG_SALAH;

        XNextEvent(l->disp, &ev);
        memset(out, 0, sizeof(*out));
        out->waktu_ms = pg_x11_sekarang_ms();

        switch (ev.type) {
        case KeyPress: {
                KeySym ks;
                char buf[8];
                int n;
                XComposeStatus cs;
                n = XLookupString(&ev.xkey, buf, sizeof(buf), &ks, &cs);
                out->tipe = PG_AKSI_TOMBOL_TURUN;
                out->tombol = pg_x11_ksym_ke_tombol(ks);
                out->modifier = 0;
                if (ev.xkey.state & ShiftMask)   out->modifier |= PG_MOD_SHIFT;
                if (ev.xkey.state & ControlMask) out->modifier |= PG_MOD_CTRL;
                if (ev.xkey.state & Mod1Mask)    out->modifier |= PG_MOD_ALT;
                if (n > 0 && (unsigned char)buf[0] >= 32 &&
                    (unsigned char)buf[0] < 127)
                        out->unicode = (pg_u32)(unsigned char)buf[0];
                return PG_BENAR;
        }
        case KeyRelease: {
                KeySym ks;
                XLookupString(&ev.xkey, NULL, 0, &ks, NULL);
                out->tipe = PG_AKSI_TOMBOL_NAIK;
                out->tombol = pg_x11_ksym_ke_tombol(ks);
                out->modifier = 0;
                if (ev.xkey.state & ShiftMask)   out->modifier |= PG_MOD_SHIFT;
                if (ev.xkey.state & ControlMask) out->modifier |= PG_MOD_CTRL;
                if (ev.xkey.state & Mod1Mask)    out->modifier |= PG_MOD_ALT;
                return PG_BENAR;
        }
        case ButtonPress:
                out->tipe = PG_AKSI_TETIKUS_TEKAN;
                out->tetik_pos = pg_buat_titik(ev.xbutton.x, ev.xbutton.y);
                out->modifier = 0;
                if (ev.xbutton.state & ShiftMask)   out->modifier |= PG_MOD_SHIFT;
                if (ev.xbutton.state & ControlMask) out->modifier |= PG_MOD_CTRL;
                if (ev.xbutton.state & Mod1Mask)    out->modifier |= PG_MOD_ALT;
                switch (ev.xbutton.button) {
                case 1: out->tetik_tombol = PG_TETIKUS_KIRI; break;
                case 2: out->tetik_tombol = PG_TETIKUS_TENGAH; break;
                case 3: out->tetik_tombol = PG_TETIKUS_KANAN; break;
                case 4: out->tipe = PG_AKSI_TETIKUS_GULIR;
                        out->roda_dy = 1; break;
                case 5: out->tipe = PG_AKSI_TETIKUS_GULIR;
                        out->roda_dy = -1; break;
                }
                return PG_BENAR;
        case ButtonRelease:
                out->tipe = PG_AKSI_TETIKUS_LEPAS;
                out->tetik_pos = pg_buat_titik(ev.xbutton.x, ev.xbutton.y);
                switch (ev.xbutton.button) {
                case 1: out->tetik_tombol = PG_TETIKUS_KIRI; break;
                case 2: out->tetik_tombol = PG_TETIKUS_TENGAH; break;
                case 3: out->tetik_tombol = PG_TETIKUS_KANAN; break;
                }
                return PG_BENAR;
        case MotionNotify:
                out->tipe = PG_AKSI_TETIKUS_GERAK;
                out->tetik_pos = pg_buat_titik(ev.xmotion.x, ev.xmotion.y);
                return PG_BENAR;
        case ConfigureNotify:
                /* Jendela di-resize. Emit event saja — app yang
                 * handle reallocate via pg_layar_kueri + realloc.
                 * JANGAN free/alloc di sini untuk hindari race
                 * dengan render yang sedang berjalan. */
                if (ev.xconfigure.width != l->lebar ||
                    ev.xconfigure.height != l->tinggi) {
                        l->lebar = ev.xconfigure.width;
                        l->tinggi = ev.xconfigure.height;
                        l->langkah = l->lebar *
                                (int)sizeof(pg_warna_t);
                }
                out->tipe = PG_AKSI_JENDELA;
                out->jendela_aksi = PG_JENDELA_UBAH_UKURAN;
                out->jendela_id = 0;
                return PG_BENAR;
        case ClientMessage:
                /* WM_DELETE_WINDOW — tombol close diklik. */
                if ((Atom)ev.xclient.data.l[0] == l->wm_hapus) {
                        out->tipe = PG_AKSI_KELUAR;
                        return PG_BENAR;
                }
                return PG_SALAH;
        case Expose:
                /* Permintaan redraw dari X server (jendela baru di-restore
                 * dari minimize, atau sebagian jendela terbongkar).
                 * Emit event JENDELA_PERMINTAAN_GAMBAR supaya aplikasi
                 * merender ulang; jangan diabaikan. */
                out->tipe = PG_AKSI_JENDELA;
                out->jendela_aksi = PG_JENDELA_PERMINTAAN_GAMBAR;
                out->jendela_id = 0;
                return PG_BENAR;
        default:
                return PG_SALAH;
        }
}
