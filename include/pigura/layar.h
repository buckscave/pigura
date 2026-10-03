/* ----------------------------------------------------------------------------------------------
 * pigura/layar.h - Backend framebuffer (target blit akhir)
 * ----------------------------------------------------------------------------------------------
 * pg_layar_t adalah target blit akhir: sepotong memori (back buffer
 * permukaan) yang dicat kompositor dan di-flip ke layar aktual tiap
 * frame oleh backend. Mekanisme eksaknya platform-specific:
 *
 *   x11:        XCreateWindow + XShmPutImage (default desktop)
 *   linuxfb:    /dev/fb0 mmap (embedded / VT tanpa X)
 *   windows:    GDI CreateDIBSection + BitBlt ke message-only window
 *   macos:      CGBitmapContext + NSWindow via objc runtime
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_LAYAR_H
#define PIGURA_LAYAR_H

#include "pigura/tipe.h"
#include "pigura/peristiwa.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pg_layar pg_layar_t;

typedef struct pg_layar_info {
        int lebar;
        int tinggi;
        int bpp;          /* bit per piksel, selalu 32 di v0.1 */
        int langkah;      /* byte per baris */
        pg_u32 format;    /* PG_LAYAR_FORMAT_* */
        pg_bool double_buffer;
} pg_layar_info_t;

enum pg_layar_format {
        PG_LAYAR_FORMAT_X8R8G8B8 = 0,
        PG_LAYAR_FORMAT_A8R8G8B8 = 1
};

typedef struct pg_layar_config {
        int  lebar;            /* 0 = auto-detect native */
        int  tinggi;
        pg_bool double_buffer;
        pg_bool fullscreen;
        const char *judul;     /* untuk backend berjendela */
} pg_layar_config_t;

/* Buka layar. */
pg_galat pg_buka_layar(pg_layar_t **out, const pg_layar_config_t *cfg);

/* Tutup layar dan bebaskan sumber daya. */
pg_galat pg_tutup_layar(pg_layar_t *layar);

/* Kueri geometri layar. */
pg_galat pg_layar_kueri(pg_layar_t *layar, pg_layar_info_t *info);

/* Ambil pointer writable ke back buffer. */
pg_galat pg_layar_kunci(pg_layar_t *layar, void **piksel, int *langkah);

/* Buka kunci back buffer. */
pg_galat pg_layar_buka_kunci(pg_layar_t *layar);

/* Presentasikan back buffer ke layar. */
pg_galat pg_layar_presentasi(pg_layar_t *layar);

/* Tunggu vsync kalau backend mendukung. */
pg_galat pg_layar_tunggu_vsync(pg_layar_t *layar);

/* Pompa peristiwa windowing backend (kalau ada). */
pg_galat pg_layar_pompa_peristiwa(pg_layar_t *layar);

/* Ambil handle native backend (Display* di X11, HWND di Windows).
 * Berguna untuk backend masukan yang perlu share handle. */
void *pg_layar_handle_native(pg_layar_t *layar);

/* Ambil ID jendela native (Window di X11). */
unsigned long pg_layar_jendela_id(pg_layar_t *layar);

/* Cek apakah ada peristiwa pending di antrian native. */
pg_bool pg_layar_punya_peristiwa(pg_layar_t *layar);

/* Ambil peristiwa native berikutnya sebagai pigura peristiwa.
 * Mengembalikan PG_BENAR jika ada peristiwa, PG_SALAH jika kosong. */
pg_bool pg_layar_peristiwa_berikutnya(pg_layar_t *layar,
                                        pg_peristiwa_t *out);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_LAYAR_H */
