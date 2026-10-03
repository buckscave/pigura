/* ----------------------------------------------------------------------------------------------
 * pigura/snap.h - Deteksi snap zone (drag floating -> dock ke tepi)
 * ----------------------------------------------------------------------------------------------
 * Snap detection: saat user drag floating panel mendekati tepi
 * window utama, tampilkan preview snap zone. Lepas mouse -> panel
 * dock ke posisi tersebut.
 *
 * Margin default 30 piksel. Snap zone dibagi 5:
 *
 *   PG_SNAP_ATAS   : my < margin                     -> full width, top 1/3
 *   PG_SNAP_BAWAH  : my > layar_h - margin           -> full width, bot 1/3
 *   PG_SNAP_KIRI   : mx < margin                     -> left 1/3,  full height
 *   PG_SNAP_KANAN  : mx > layar_w - margin           -> right 1/3, full height
 *   PG_SNAP_CENTER : |mx-w/2| < margin && |my-h/2| < margin -> center 1/3 x 1/3
 *   PG_SNAP_KOSONG : tidak dekat tepi manapun
 *
 * Prioritas (saat overlap): ATAS > BAWAH > KIRI > KANAN > CENTER.
 *
 * pg_snap_area() mengembalikan rect preview untuk posisi snap
 * tertentu. Dipakai untuk menggambar highlight overlay. ATAS/BAWAH
 * berbentuk horizontal (lebar penuh, tinggi 1/3). KIRI/KANAN
 * berbentuk vertikal (lebar 1/3, tinggi penuh). CENTER kotak 1/3
 * x 1/3 di tengah layar.
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_SNAP_H
#define PIGURA_SNAP_H

#include "pigura/tipe.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
        PG_SNAP_POS_KOSONG = 0,
        PG_SNAP_POS_KIRI   = 1,
        PG_SNAP_POS_KANAN  = 2,
        PG_SNAP_POS_ATAS   = 3,
        PG_SNAP_POS_BAWAH  = 4,
        PG_SNAP_POS_CENTER = 5
} pg_snap_posisi_t;

/* Margin default untuk deteksi snap (piksel). */
#define PG_SNAP_MARGIN_DEFAULT 30

/* Deteksi snap zone dari posisi mouse (mx,my). Mengembalikan
 * PG_SNAP_KOSONG bila tidak dekat tepi manapun. */
pg_snap_posisi_t pg_snap_deteksi(int mx, int my,
        int lebar_layar, int tinggi_layar, int margin);

/* Hitung area snap (preview) untuk posisi snap tertentu. */
pg_kotak_t pg_snap_area(pg_snap_posisi_t snap,
        int lebar_layar, int tinggi_layar);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_SNAP_H */
