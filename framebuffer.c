/*******************************************************************************
 * FRAMEBUFFER.C - Implementasi Framebuffer
 *
 * Modul ini menangani inisialisasi dan operasi framebuffer Linux.
 *
 * Copyright (c) 2024 Pigura OS Project
 ******************************************************************************/

#include "pigura.h"

/*******************************************************************************
 * FUNGSI INISIALISASI DAN CLEANUP
 ******************************************************************************/

/* Inisialisasi framebuffer */
int fb_init(const char *jalur, InfoFramebuffer *fb) {
    int fd;
    struct fb_fix_screeninfo finfo;
    struct fb_var_screeninfo vinfo;
    const char *dev_jalur;
    
    if (!fb) {
        return 0;
    }
    
    /* Tentukan jalur device */
    dev_jalur = jalur ? jalur : "/dev/fb0";
    
    /* Buka device framebuffer */
    fd = open(dev_jalur, O_RDWR);
    if (fd < 0) {
        return 0;
    }
    
    /* Dapatkan informasi fix screen */
    if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo) < 0) {
        close(fd);
        return 0;
    }
    
    /* Dapatkan informasi variable screen */
    if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        close(fd);
        return 0;
    }
    
    /* Validasi bit per piksel */
    if (vinfo.bits_per_pixel != 16 && vinfo.bits_per_pixel != 24 &&
        vinfo.bits_per_pixel != 32) {
        close(fd);
        return 0;
    }
    
    /* Map framebuffer ke memori */
    fb->ptr = mmap(NULL, (size_t)finfo.smem_len, PROT_READ | PROT_WRITE,
                   MAP_SHARED, fd, 0);
    
    if (fb->ptr == MAP_FAILED) {
        close(fd);
        fb->ptr = NULL;
        return 0;
    }
    
    /* Simpan informasi */
    fb->fd = fd;
    fb->lebar = (int)vinfo.xres;
    fb->tinggi = (int)vinfo.yres;
    fb->bpp = (int)vinfo.bits_per_pixel;
    fb->panjang_baris = (int)finfo.line_length;
    fb->ukuran_mem = (size_t)finfo.smem_len;
    
    return 1;
}

/* Bersihkan framebuffer dengan warna tertentu */
void fb_bersihkan_warna(InfoFramebuffer *fb, WarnaRGB warna) {
    int x, y;
    
    if (!fb || !fb->ptr) {
        return;
    }
    
    for (y = 0; y < fb->tinggi; y++) {
        for (x = 0; x < fb->lebar; x++) {
            fb_gambar_piksel(fb, x, y, warna.r, warna.g, warna.b);
        }
    }
}

/* Bersihkan framebuffer dengan warna hitam */
void fb_bersihkan(InfoFramebuffer *fb) {
    WarnaRGB hitam;
    
    hitam.r = 0;
    hitam.g = 0;
    hitam.b = 0;
    
    fb_bersihkan_warna(fb, hitam);
}

/* Tutup framebuffer */
void fb_tutup(InfoFramebuffer *fb) {
    if (!fb) {
        return;
    }
    
    if (fb->ptr && fb->ukuran_mem > 0) {
        munmap(fb->ptr, fb->ukuran_mem);
        fb->ptr = NULL;
    }
    
    if (fb->fd >= 0) {
        close(fb->fd);
        fb->fd = -1;
    }
    
    fb->lebar = 0;
    fb->tinggi = 0;
    fb->bpp = 0;
    fb->panjang_baris = 0;
    fb->ukuran_mem = 0;
}

/*******************************************************************************
 * FUNGSI MENGGAMBAR
 ******************************************************************************/

/* Gambar satu piksel */
void fb_gambar_piksel(InfoFramebuffer *fb, int x, int y,
                      unsigned char r, unsigned char g, unsigned char b) {
    
    unsigned char *piksel;
    int lokasi;
    unsigned short warna16;
    
    if (!fb || !fb->ptr) {
        return;
    }
    
    /* Cek batas */
    if (x < 0 || x >= fb->lebar || y < 0 || y >= fb->tinggi) {
        return;
    }
    
    /* Hitung lokasi memori */
    lokasi = (y * fb->panjang_baris) + (x * (fb->bpp / 8));
    piksel = (unsigned char *)fb->ptr + lokasi;
    
    /* Tulis piksel sesuai format */
    if (fb->bpp == 32) {
        piksel[0] = b;
        piksel[1] = g;
        piksel[2] = r;
        piksel[3] = 0;
    } else if (fb->bpp == 24) {
        piksel[0] = b;
        piksel[1] = g;
        piksel[2] = r;
    } else if (fb->bpp == 16) {
        /* Format RGB565 */
        warna16 = (unsigned short)(((r >> 3) << 11) | 
                                   ((g >> 2) << 5) | 
                                   (b >> 3));
        *((unsigned short *)piksel) = warna16;
    }
}

/* Gambar piksel dengan struktur warna */
void fb_gambar_piksel_warna(InfoFramebuffer *fb, int x, int y, WarnaRGB warna) {
    fb_gambar_piksel(fb, x, y, warna.r, warna.g, warna.b);
}

/* Isi area persegi panjang dengan warna */
void fb_isi_area(InfoFramebuffer *fb, int x1, int y1, int x2, int y2,
                 WarnaRGB warna) {
    
    int x, y;
    int tmp;
    
    if (!fb || !fb->ptr) {
        return;
    }
    
    /* Normalisasi koordinat */
    if (x1 > x2) {
        tmp = x1;
        x1 = x2;
        x2 = tmp;
    }
    if (y1 > y2) {
        tmp = y1;
        y1 = y2;
        y2 = tmp;
    }
    
    /* Batasi ke area layar */
    if (x1 < 0) {
        x1 = 0;
    }
    if (y1 < 0) {
        y1 = 0;
    }
    if (x2 >= fb->lebar) {
        x2 = fb->lebar - 1;
    }
    if (y2 >= fb->tinggi) {
        y2 = fb->tinggi - 1;
    }
    
    /* Gambar piksel per piksel */
    for (y = y1; y <= y2; y++) {
        for (x = x1; x <= x2; x++) {
            fb_gambar_piksel(fb, x, y, warna.r, warna.g, warna.b);
        }
    }
}

/* Gambar garis menggunakan algoritma Bresenham */
void fb_gambar_garis(InfoFramebuffer *fb, int x1, int y1, int x2, int y2,
                     WarnaRGB warna) {
    
    int dx, dy, sx, sy, err, e2;
    
    if (!fb || !fb->ptr) {
        return;
    }
    
    dx = abs(x2 - x1);
    dy = abs(y2 - y1);
    sx = (x1 < x2) ? 1 : -1;
    sy = (y1 < y2) ? 1 : -1;
    err = dx - dy;
    
    while (1) {
        fb_gambar_piksel(fb, x1, y1, warna.r, warna.g, warna.b);
        
        if (x1 == x2 && y1 == y2) {
            break;
        }
        
        e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

/* Gambar garis horizontal (optimasi) */
void fb_gambar_garis_h(InfoFramebuffer *fb, int x1, int x2, int y,
                       WarnaRGB warna) {
    
    int x;
    int tmp;
    
    if (!fb || !fb->ptr) {
        return;
    }
    
    if (y < 0 || y >= fb->tinggi) {
        return;
    }
    
    if (x1 > x2) {
        tmp = x1;
        x1 = x2;
        x2 = tmp;
    }
    
    if (x1 < 0) {
        x1 = 0;
    }
    if (x2 >= fb->lebar) {
        x2 = fb->lebar - 1;
    }
    
    for (x = x1; x <= x2; x++) {
        fb_gambar_piksel(fb, x, y, warna.r, warna.g, warna.b);
    }
}

/* Gambar garis vertikal (optimasi) */
void fb_gambar_garis_v(InfoFramebuffer *fb, int x, int y1, int y2,
                       WarnaRGB warna) {
    
    int y;
    int tmp;
    
    if (!fb || !fb->ptr) {
        return;
    }
    
    if (x < 0 || x >= fb->lebar) {
        return;
    }
    
    if (y1 > y2) {
        tmp = y1;
        y1 = y2;
        y2 = tmp;
    }
    
    if (y1 < 0) {
        y1 = 0;
    }
    if (y2 >= fb->tinggi) {
        y2 = fb->tinggi - 1;
    }
    
    for (y = y1; y <= y2; y++) {
        fb_gambar_piksel(fb, x, y, warna.r, warna.g, warna.b);
    }
}

/* Gambar kotak (outline atau filled) */
void fb_gambar_kotak(InfoFramebuffer *fb, int x, int y, int lebar, int tinggi,
                     WarnaRGB warna, int isi) {
    
    if (!fb || !fb->ptr) {
        return;
    }
    
    if (lebar <= 0 || tinggi <= 0) {
        return;
    }
    
    if (isi) {
        /* Kotak terisi */
        fb_isi_area(fb, x, y, x + lebar - 1, y + tinggi - 1, warna);
    } else {
        /* Kotak outline */
        fb_gambar_garis_h(fb, x, x + lebar - 1, y, warna);
        fb_gambar_garis_h(fb, x, x + lebar - 1, y + tinggi - 1, warna);
        fb_gambar_garis_v(fb, x, y, y + tinggi - 1, warna);
        fb_gambar_garis_v(fb, x + lebar - 1, y, y + tinggi - 1, warna);
    }
}

/* Gambar lingkaran menggunakan algoritma midpoint */
void fb_gambar_lingkaran(InfoFramebuffer *fb, int cx, int cy, int radius,
                         WarnaRGB warna, int isi) {
    
    int x, y, err;
    
    if (!fb || !fb->ptr || radius <= 0) {
        return;
    }
    
    x = radius;
    y = 0;
    err = 1 - radius;
    
    while (x >= y) {
        if (isi) {
            /* Lingkaran terisi */
            fb_gambar_garis_h(fb, cx - x, cx + x, cy + y, warna);
            fb_gambar_garis_h(fb, cx - x, cx + x, cy - y, warna);
            fb_gambar_garis_h(fb, cx - y, cx + y, cy + x, warna);
            fb_gambar_garis_h(fb, cx - y, cx + y, cy - x, warna);
        } else {
            /* Lingkaran outline */
            fb_gambar_piksel(fb, cx + x, cy + y, warna.r, warna.g, warna.b);
            fb_gambar_piksel(fb, cx - x, cy + y, warna.r, warna.g, warna.b);
            fb_gambar_piksel(fb, cx + x, cy - y, warna.r, warna.g, warna.b);
            fb_gambar_piksel(fb, cx - x, cy - y, warna.r, warna.g, warna.b);
            fb_gambar_piksel(fb, cx + y, cy + x, warna.r, warna.g, warna.b);
            fb_gambar_piksel(fb, cx - y, cy + x, warna.r, warna.g, warna.b);
            fb_gambar_piksel(fb, cx + y, cy - x, warna.r, warna.g, warna.b);
            fb_gambar_piksel(fb, cx - y, cy - x, warna.r, warna.g, warna.b);
        }
        
        y++;
        
        if (err < 0) {
            err += 2 * y + 1;
        } else {
            x--;
            err += 2 * (y - x) + 1;
        }
    }
}

/*******************************************************************************
 * FUNGSI UTILITAS FRAMEBUFFER
 ******************************************************************************/

/* Dapatkan warna piksel di posisi tertentu */
int fb_ambil_piksel(InfoFramebuffer *fb, int x, int y, WarnaRGB *warna) {
    
    unsigned char *piksel;
    int lokasi;
    unsigned short warna16;
    
    if (!fb || !fb->ptr || !warna) {
        return 0;
    }
    
    if (x < 0 || x >= fb->lebar || y < 0 || y >= fb->tinggi) {
        return 0;
    }
    
    lokasi = (y * fb->panjang_baris) + (x * (fb->bpp / 8));
    piksel = (unsigned char *)fb->ptr + lokasi;
    
    if (fb->bpp == 32) {
        warna->b = piksel[0];
        warna->g = piksel[1];
        warna->r = piksel[2];
    } else if (fb->bpp == 24) {
        warna->b = piksel[0];
        warna->g = piksel[1];
        warna->r = piksel[2];
    } else if (fb->bpp == 16) {
        warna16 = *((unsigned short *)piksel);
        warna->r = (unsigned char)((warna16 >> 11) & 0x1F) << 3;
        warna->g = (unsigned char)((warna16 >> 5) & 0x3F) << 2;
        warna->b = (unsigned char)(warna16 & 0x1F) << 3;
    }
    
    return 1;
}

/* Scroll framebuffer ke atas */
void fb_scroll_atas(InfoFramebuffer *fb, int baris, WarnaRGB warna_blkg) {
    
    int y;
    int offset;
    int byte_per_baris;
    
    if (!fb || !fb->ptr || baris <= 0) {
        return;
    }
    
    byte_per_baris = fb->panjang_baris;
    offset = baris * fb->tinggi * HURUF_LEBAR;
    
    if (offset >= fb->tinggi) {
        /* Scroll seluruh layar - cukup bersihkan */
        fb_bersihkan_warna(fb, warna_blkg);
        return;
    }
    
    /* Geser memori */
    memmove(fb->ptr, 
            (unsigned char *)fb->ptr + (baris * HURUF_TINGGI * byte_per_baris),
            (size_t)((fb->tinggi - baris * HURUF_TINGGI) * byte_per_baris));
    
    /* Bersihkan area yang tersisa di bawah */
    for (y = fb->tinggi - baris * HURUF_TINGGI; y < fb->tinggi; y++) {
        fb_gambar_garis_h(fb, 0, fb->lebar - 1, y, warna_blkg);
    }
}

/* Copy area framebuffer */
void fb_salin_area(InfoFramebuffer *fb, int sx, int sy, int dx, int dy,
                   int lebar, int tinggi) {
    
    int x, y;
    WarnaRGB warna;
    
    if (!fb || !fb->ptr) {
        return;
    }
    
    /* Implementasi sederhana piksel per piksel */
    for (y = 0; y < tinggi; y++) {
        for (x = 0; x < lebar; x++) {
            if (fb_ambil_piksel(fb, sx + x, sy + y, &warna)) {
                fb_gambar_piksel(fb, dx + x, dy + y, 
                                 warna.r, warna.g, warna.b);
            }
        }
    }
}
