/* ----------------------------------------------------------------------------------------------
 * pigura layar: linuxfb.c - backend /dev/fb0 mmap untuk embedded/VT
 * ----------------------------------------------------------------------------------------------
 * Membuka /dev/fb0, memaksa 32-bpp via FBIOPUT_VSCREENINFO, lalu
 * mmap MAP_SHARED untuk akses langsung framebuffer kernel. Back
 * buffer software terpisah (calloc) dicatat oleh pigura; tiap
 * pg_layar_presentasi() menyalin back buffer ke mmap baris-per-baris.
 *
 * Bila layout framebuffer cocok dengan pg_warna_t (R@16, G@8, B@0,
 * 8-bit each), salinan pakai memcpy baris-per-baris (cepat). Bila
 * layout berbeda (BGRA vs RGBA, dsb.), konversi per piksel dengan
 * shift+mask sesuai vinfo red/green/blue.offset.
 *
 * Input tidak ditangani file ini — dipompa oleh masukan/evdev.c yang
 * membuka /dev/input/event* sendiri. pg_layar_peristiwa_berikutnya()
 * selalu mengembalikan PG_SALAH; pg_layar_punya_peristiwa() PG_SALAH.
 *
 * Hanya dikompilasi di __linux__. Dijaga oleh #ifdef di seluruh file.
 * ---------------------------------------------------------------------------------------------- */
#ifdef __linux__

#include "pigura/layar.h"
#include "pigura/galat.h"
#include "pigura/catat.h"
#include "pigura/tipe.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>

struct pg_layar {
        int           fd;
        void         *fb;
        size_t        fb_size;
        int           fb_langkah;     /* byte per baris di framebuffer */
        int           fb_lebar;       /* xres_virtual */
        int           fb_tinggi;      /* yres_virtual */
        int           r_off, g_off, b_off;  /* bit offset 32-bit */
        pg_bool       langsung_memcpy;     /* layout cocok dgn pg_warna_t */
        void         *back;
        int           lebar, tinggi, langkah;
        pg_bool       terkunci;
};

static pg_u32 pg_fb_sekarang_ms(void)
{
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (pg_u32)(ts.tv_sec * 1000u + ts.tv_nsec / 1000000u);
}

pg_galat pg_buka_layar(pg_layar_t **out, const pg_layar_config_t *cfg)
{
        pg_layar_t *l;
        pg_layar_config_t def;
        struct fb_var_screeninfo vinfo;
        struct fb_fix_screeninfo finfo;
        int lebar, tinggi;
        int fd;
        void *fb;

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

        fd = open("/dev/fb0", O_RDWR);
        if (fd < 0) {
                pg_set_galat(PG_GALAT_IO, "open /dev/fb0 gagal: %s",
                              strerror(errno));
                free(l);
                return PG_GALAT_IO;
        }
        l->fd = fd;

        if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
                pg_set_galat(PG_GALAT_IO, "FBIOGET_VSCREENINFO: %s",
                              strerror(errno));
                close(fd);
                free(l);
                return PG_GALAT_IO;
        }

        /* Paksa 32-bpp bila belum. */
        if (vinfo.bits_per_pixel != 32) {
                struct fb_var_screeninfo v;
                v = vinfo;
                v.bits_per_pixel = 32;
                v.red.offset    = 16; v.red.length    = 8;
                v.green.offset  = 8;  v.green.length  = 8;
                v.blue.offset   = 0;  v.blue.length   = 8;
                v.transp.offset = 24; v.transp.length = 0;
                v.activate      = FB_ACTIVATE_NOW;
                if (ioctl(fd, FBIOPUT_VSCREENINFO, &v) < 0) {
                        pg_peringatan("fb: gagal set 32-bpp (%s), "
                                       "pakai %d-bpp native",
                                       strerror(errno),
                                       vinfo.bits_per_pixel);
                        if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
                                close(fd);
                                free(l);
                                PG_KEMBALI_GALAT(PG_GALAT_IO);
                        }
                } else {
                        ioctl(fd, FBIOGET_VSCREENINFO, &vinfo);
                }
        }
        if (vinfo.bits_per_pixel != 32) {
                pg_set_galat(PG_GALAT_TANPA,
                              "framebuffer %d-bpp tidak didukung",
                              vinfo.bits_per_pixel);
                close(fd);
                free(l);
                return PG_GALAT_TANPA;
        }

        if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo) < 0) {
                pg_set_galat(PG_GALAT_IO, "FBIOGET_FSCREENINFO: %s",
                              strerror(errno));
                close(fd);
                free(l);
                return PG_GALAT_IO;
        }

        l->fb_size = (size_t)finfo.smem_len;
        l->fb_langkah = (int)finfo.line_length;
        l->fb_lebar = (int)vinfo.xres_virtual;
        l->fb_tinggi = (int)vinfo.yres_virtual;
        l->r_off = (int)vinfo.red.offset;
        l->g_off = (int)vinfo.green.offset;
        l->b_off = (int)vinfo.blue.offset;

        /* Cek apakah layout cocok dgn pg_warna_t (R@16, G@8, B@0). */
        if (l->r_off == 16 && l->g_off == 8 && l->b_off == 0)
                l->langsung_memcpy = PG_BENAR;
        else
                l->langsung_memcpy = PG_SALAH;

        fb = mmap(NULL, l->fb_size, PROT_READ | PROT_WRITE,
                   MAP_SHARED, fd, 0);
        if (fb == MAP_FAILED) {
                pg_set_galat(PG_GALAT_IO, "mmap fb gagal: %s",
                              strerror(errno));
                close(fd);
                free(l);
                return PG_GALAT_IO;
        }
        l->fb = fb;

        /* Back buffer software. */
        l->back = calloc((size_t)lebar * tinggi, 4);
        if (!l->back) {
                munmap(l->fb, l->fb_size);
                close(fd);
                free(l);
                PG_KEMBALI_GALAT(PG_GALAT_MEMORI);
        }

        pg_info("linuxfb: %dx%d @32bpp, langkah=%d, mmap=%zu byte, "
                "R%dG%dB%d %s",
                lebar, tinggi, l->fb_langkah, l->fb_size,
                l->r_off, l->g_off, l->b_off,
                l->langsung_memcpy ? "(memcpy)" : "(konversi)");

        *out = l;
        return PG_OK;
}

pg_galat pg_tutup_layar(pg_layar_t *l)
{
        if (!l) return PG_OK;
        if (l->back) free(l->back);
        if (l->fb) munmap(l->fb, l->fb_size);
        if (l->fd >= 0) close(l->fd);
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
        *piksel = l->back;
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

static void pg_fb_salinv_langsung(pg_layar_t *l)
{
        int y;
        int w = l->lebar;
        int h = l->tinggi;
        int s_src = l->langkah;
        int s_dst = l->fb_langkah;
        const char *src = (const char *)l->back;
        char *dst = (char *)l->fb;
        int copy = w * 4;

        if (s_dst == s_src) {
                memcpy(dst, src, (size_t)copy * h);
                return;
        }
        for (y = 0; y < h; y++) {
                memcpy(dst + y * s_dst, src + y * s_src, (size_t)copy);
        }
}

static void pg_fb_salinv_konversi(pg_layar_t *l)
{
        int x, y;
        int w = l->lebar;
        int h = l->tinggi;
        int s_dst = l->fb_langkah / 4;
        int ro = l->r_off, go = l->g_off, bo = l->b_off;
        const pg_warna_t *src = (const pg_warna_t *)l->back;
        pg_u32 *dst = (pg_u32 *)l->fb;

        for (y = 0; y < h; y++) {
                pg_u32 *drow = dst + y * s_dst;
                const pg_warna_t *srow = src + y * w;
                for (x = 0; x < w; x++) {
                        pg_warna_t c = srow[x];
                        pg_u32 r = (pg_u32)((c >> 16) & 0xff);
                        pg_u32 g = (pg_u32)((c >>  8) & 0xff);
                        pg_u32 b = (pg_u32)(c & 0xff);
                        drow[x] = (r << ro) | (g << go) | (b << bo);
                }
        }
}

pg_galat pg_layar_presentasi(pg_layar_t *l)
{
        if (!l) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
        if (!l->fb || !l->back) PG_KEMBALI_GALAT(PG_GALAT_UMUM);

        if (l->langsung_memcpy)
                pg_fb_salinv_langsung(l);
        else
                pg_fb_salinv_konversi(l);
        return PG_OK;
}

pg_galat pg_layar_tunggu_vsync(pg_layar_t *l)
{
        (void)l;
        /* linuxfb tidak expose vsync portabel tanpa pan driver. */
        return PG_GALAT_TANPA;
}

pg_galat pg_layar_pompa_peristiwa(pg_layar_t *l)
{
        /* Tidak ada antrian peristiwa di fb — input dari evdev. */
        if (!l) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
        return PG_OK;
}

void *pg_layar_handle_native(pg_layar_t *l)
{
        if (!l) return NULL;
        /* Kembalikan fd cast ke void* — evdev.c bisa ambil via cast. */
        return (void *)(long)l->fd;
}

unsigned long pg_layar_jendela_id(pg_layar_t *l)
{
        /* Tidak ada jendela di fb — evdev pakai fd dari handle_native. */
        if (!l) return 0;
        return (unsigned long)l->fd;
}

pg_bool pg_layar_punya_peristiwa(pg_layar_t *l)
{
        (void)l;
        return PG_SALAH;
}

pg_bool pg_layar_peristiwa_berikutnya(pg_layar_t *l,
                                        pg_peristiwa_t *out)
{
        if (out) {
                memset(out, 0, sizeof(*out));
                out->waktu_ms = pg_fb_sekarang_ms();
        }
        (void)l;
        return PG_SALAH;
}

#endif /* __linux__ */
