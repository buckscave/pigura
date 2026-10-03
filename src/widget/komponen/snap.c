/* ----------------------------------------------------------------------------------------------
 * pigura widget: snap.c - snap zone detection (drag -> dock tepi)
 * ----------------------------------------------------------------------------------------------
 * Snap detection: saat user drag floating panel mendekati tepi
 * window utama, deteksi snap zone mana yang paling dekat. Lepas
 * mouse -> panel dock ke sisi tersebut via pg_dock_kait_panel().
 *
 * Algoritma pg_snap_deteksi(mx, my, w, h, margin):
 *   ATAS   : my < margin
 *   BAWAH  : my > h - margin
 *   KIRI   : mx < margin
 *   KANAN  : mx > w - margin
 *   CENTER : |mx - w/2| < margin AND |my - h/2| < margin
 *   KOSONG : tidak dekat tepi manapun
 *
 * Prioritas (saat overlap): ATAS > BAWAH > KIRI > KANAN > CENTER.
 *
 * pg_snap_area(snap, w, h): rect preview highlight.
 *   ATAS   : full width, top 1/3 (horizontal).
 *   BAWAH  : full width, bottom 1/3 (horizontal).
 *   KIRI   : left 1/3 width, full height (vertical).
 *   KANAN  : right 1/3 width, full height (vertical).
 *   CENTER : 1/3 width, 1/3 height di tengah.
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/snap.h"

/* Deteksi snap zone dari posisi mouse (mx, my). */
pg_snap_posisi_t pg_snap_deteksi(int mx, int my,
        int lebar_layar, int tinggi_layar, int margin)
{
        int cx, cy, half_m;

        if (margin < 1) margin = PG_SNAP_MARGIN_DEFAULT;
        if (lebar_layar < margin * 2) lebar_layar = margin * 2;
        if (tinggi_layar < margin * 2) tinggi_layar = margin * 2;

        cx = lebar_layar / 2;
        cy = tinggi_layar / 2;
        half_m = margin / 2;
        if (half_m < 1) half_m = 1;

        /* Prioritas: ATAS > BAWAH > KIRI > KANAN > CENTER. */
        if (my < margin) return PG_SNAP_POS_ATAS;
        if (my >= tinggi_layar - margin) return PG_SNAP_POS_BAWAH;
        if (mx < margin) return PG_SNAP_POS_KIRI;
        if (mx >= lebar_layar - margin) return PG_SNAP_POS_KANAN;
        /* Center: dekat ke titik tengah layar. */
        if (mx >= cx - half_m && mx < cx + half_m &&
            my >= cy - half_m && my < cy + half_m)
                return PG_SNAP_POS_CENTER;
        return PG_SNAP_POS_KOSONG;
}

/* Hitung rect preview untuk posisi snap. */
pg_kotak_t pg_snap_area(pg_snap_posisi_t snap,
        int lebar_layar, int tinggi_layar)
{
        int w3, h3;

        w3 = lebar_layar / 3;
        h3 = tinggi_layar / 3;
        if (w3 < 1) w3 = 1;
        if (h3 < 1) h3 = 1;

        switch (snap) {
        case PG_SNAP_POS_ATAS:
                /* Full width, top 1/3 (horizontal). */
                return pg_buat_kotak(0, 0, lebar_layar, h3);
        case PG_SNAP_POS_BAWAH:
                /* Full width, bottom 1/3 (horizontal). */
                return pg_buat_kotak(0, tinggi_layar - h3,
                        lebar_layar, h3);
        case PG_SNAP_POS_KIRI:
                /* Left 1/3 width, full height (vertical). */
                return pg_buat_kotak(0, 0, w3, tinggi_layar);
        case PG_SNAP_POS_KANAN:
                /* Right 1/3 width, full height (vertical). */
                return pg_buat_kotak(lebar_layar - w3, 0,
                        w3, tinggi_layar);
        case PG_SNAP_POS_CENTER:
                /* Center: 1/3 width, 1/3 height di tengah. */
                return pg_buat_kotak(lebar_layar / 3, h3, w3, h3);
        case PG_SNAP_POS_KOSONG:
        default:
                return pg_buat_kotak(0, 0, 0, 0);
        }
}
