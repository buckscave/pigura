/*
 * PIGURA OS - BARGULIR.C
 * =======================
 * Widget bar gulir (scrollbar) Pigura OS.
 *
 * Mendukung orientasi vertikal dan horizontal.
 * Handle dapat di-drag oleh pengguna. Mendukung
 * scroll dengan langkah (step) dan halaman.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89), 80 karakter/baris
 * - Bahasa Indonesia, multi-arch
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "widget.h"

/*
 * ===========================================================================
 * FUNGSI INTERNAL - HITUNG POSISI HANDLE
 * ===========================================================================
 *
 * Menghitung posisi dan ukuran handle berdasarkan
 * nilai saat ini dan rentang total.
 */

static void hitung_handle(widget_bar_gulir_data_t *data,
                           tak_bertanda32_t track_panjang)
{
    tanda32_t rentang;
    tanda32_t nilai;
    tak_bertanda32_t handle_panjang;

    if (data == NULL) {
        return;
    }

    rentang = data->nilai_maks - data->nilai_min;
    if (rentang <= 0) {
        data->handle_posisi = 0;
        data->handle_lebar = WIDGET_GULIR_HANDLE_MIN;
        return;
    }

    nilai = data->nilai_sekarang - data->nilai_min;
    if (nilai < 0) {
        nilai = 0;
    }
    if (nilai > rentang) {
        nilai = rentang;
    }

    /* Hitung ukuran handle berdasarkan page */
    if (data->ukuran_halaman > 0 &&
        (tak_bertanda32_t)data->ukuran_halaman <
        (tak_bertanda32_t)rentang) {
        handle_panjang = (tak_bertanda32_t)(
            ((tanda64_t)data->ukuran_halaman *
             (tanda64_t)track_panjang)
             / (tanda64_t)rentang);
        if (handle_panjang < WIDGET_GULIR_HANDLE_MIN) {
            handle_panjang = WIDGET_GULIR_HANDLE_MIN;
        }
        if (handle_panjang > track_panjang) {
            handle_panjang = track_panjang;
        }
    } else {
        handle_panjang = track_panjang;
    }

    data->handle_lebar = handle_panjang;

    /* Hitung posisi handle pada track */
    if (track_panjang > handle_panjang) {
        tak_bertanda32_t area_gerak =
            track_panjang - handle_panjang;
        data->handle_posisi = (tak_bertanda32_t)(
            ((tanda64_t)nilai * (tanda64_t)area_gerak)
             / (tanda64_t)rentang);
        if (data->handle_posisi > area_gerak) {
            data->handle_posisi = area_gerak;
        }
    } else {
        data->handle_posisi = 0;
    }
}

/*
 * ===========================================================================
 * API PUBLIK - INISIALISASI DATA BAR GULIR
 * ===========================================================================
 */

status_t bar_gulir_init_data(widget_t *w,
                              tak_bertanda32_t orientasi,
                              tanda32_t nilai_min,
                              tanda32_t nilai_maks)
{
    widget_bar_gulir_data_t *data;

    if (w == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (w->tipe != WIDGET_BAR_GULIR) {
        return STATUS_PARAM_INVALID;
    }

    data = (widget_bar_gulir_data_t *)w->data;
    if (data == NULL) {
        return STATUS_PARAM_NULL;
    }

    kernel_memset(data, 0,
        sizeof(widget_bar_gulir_data_t));

    data->orientasi = orientasi;
    data->nilai_min = nilai_min;
    data->nilai_maks = nilai_maks > nilai_min
        ? nilai_maks : nilai_min + 1;
    data->nilai_sekarang = nilai_min;
    data->langkah = 1;
    data->ukuran_halaman = 1;
    data->handle_lebar = WIDGET_GULIR_HANDLE_MIN;
    data->handle_posisi = 0;
    data->sedang_drag = SALAH;
    data->drag_offset = 0;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - SET RENTANG NILAI
 * ===========================================================================
 */

void bar_gulir_set_rentang(widget_t *w,
                            tanda32_t min_val,
                            tanda32_t max_val)
{
    widget_bar_gulir_data_t *data;

    if (w == NULL) {
        return;
    }

    data = (widget_bar_gulir_data_t *)w->data;
    if (data == NULL) {
        return;
    }

    data->nilai_min = min_val;
    data->nilai_maks = max_val > min_val
        ? max_val : min_val + 1;

    if (data->nilai_sekarang < data->nilai_min) {
        data->nilai_sekarang = data->nilai_min;
    }
    if (data->nilai_sekarang > data->nilai_maks) {
        data->nilai_sekarang = data->nilai_maks;
    }

    w->flags |= WIDGET_FLAG_KOTOR;
}

/*
 * ===========================================================================
 * API PUBLIK - SET NILAI SAAT INI
 * ===========================================================================
 */

status_t bar_gulir_set_nilai(widget_t *w, tanda32_t nilai)
{
    widget_bar_gulir_data_t *data;

    if (w == NULL) {
        return STATUS_PARAM_NULL;
    }

    data = (widget_bar_gulir_data_t *)w->data;
    if (data == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (nilai < data->nilai_min) {
        nilai = data->nilai_min;
    }
    if (nilai > data->nilai_maks) {
        nilai = data->nilai_maks;
    }

    data->nilai_sekarang = nilai;
    w->flags |= WIDGET_FLAG_KOTOR;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - DAPAT NILAI SAAT INI
 * ===========================================================================
 */

tanda32_t bar_gulir_dapat_nilai(const widget_t *w)
{
    widget_bar_gulir_data_t *data;

    if (w == NULL) {
        return 0;
    }

    data = (widget_bar_gulir_data_t *)w->data;
    if (data == NULL) {
        return 0;
    }

    return data->nilai_sekarang;
}

/*
 * ===========================================================================
 * API PUBLIK - SET UKURAN HALAMAN
 * ===========================================================================
 */

void bar_gulir_set_halaman(widget_t *w, tanda32_t ukuran)
{
    widget_bar_gulir_data_t *data;

    if (w == NULL) {
        return;
    }

    data = (widget_bar_gulir_data_t *)w->data;
    if (data == NULL) {
        return;
    }

    if (ukuran < 1) {
        ukuran = 1;
    }
    data->ukuran_halaman = ukuran;
    w->flags |= WIDGET_FLAG_KOTOR;
}

/*
 * ===========================================================================
 * API PUBLIK - SET LANGKAH
 * ===========================================================================
 */

void bar_gulir_set_langkah(widget_t *w, tanda32_t langkah)
{
    widget_bar_gulir_data_t *data;

    if (w == NULL) {
        return;
    }

    data = (widget_bar_gulir_data_t *)w->data;
    if (data == NULL) {
        return;
    }

    if (langkah < 1) {
        langkah = 1;
    }
    data->langkah = langkah;
}

/*
 * ===========================================================================
 * API PUBLIK - RENDER BAR GULIR
 * ===========================================================================
 */

status_t bar_gulir_render(widget_t *w,
                           permukaan_t *target,
                           tak_bertanda32_t ox,
                           tak_bertanda32_t oy)
{
    widget_bar_gulir_data_t *data;
    tak_bertanda32_t px, py, pw, ph;
    tak_bertanda32_t hx, hy, hw, hh;
    tak_bertanda32_t track_len;

    if (w == NULL || target == NULL) {
        return STATUS_PARAM_NULL;
    }

    data = (widget_bar_gulir_data_t *)w->data;
    if (data == NULL) {
        return STATUS_PARAM_NULL;
    }

    px = ox + (tak_bertanda32_t)w->x;
    py = oy + (tak_bertanda32_t)w->y;
    pw = w->lebar;
    ph = w->tinggi;

    /* Hitung ulang posisi handle */
    track_len = data->orientasi == 0 ? ph - 2 : pw - 2;
    hitung_handle(data, track_len);

    /* Latar track */
    permukaan_isi_kotak(target, px, py, pw, ph,
                         WIDGET_WARNA_GULIR_LINTASAN);

    /* Border track */
    permukaan_kotak(target, px, py, pw, ph,
                     WIDGET_WARNA_BORDER, SALAH);

    /* Posisi handle */
    if (data->orientasi == 0) {
        /* Vertikal */
        hw = pw > 2 ? pw - 2 : 1;
        hh = data->handle_lebar;
        hx = px + 1;
        hy = py + 1 + data->handle_posisi;
        if (hy + hh > py + ph - 1) {
            hy = py + ph - 1 - hh;
        }
    } else {
        /* Horizontal */
        hw = data->handle_lebar;
        hh = ph > 2 ? ph - 2 : 1;
        hx = px + 1 + data->handle_posisi;
        hy = py + 1;
        if (hx + hw > px + pw - 1) {
            hx = px + pw - 1 - hw;
        }
    }

    /* Gambar handle */
    permukaan_isi_kotak(target, hx, hy, hw, hh,
                         WIDGET_WARNA_GULIR_HANDLE);

    /* Highlight handle saat drag */
    if (data->sedang_drag) {
        permukaan_kotak(target, hx, hy, hw, hh,
                         WIDGET_WARNA_FOKUS, SALAH);
    }

    /* Panah atas (vertikal) atau kiri (horizontal) */
    if (data->orientasi == 0) {
        /* Panah atas */
        {
            tak_bertanda32_t mx = px + pw / 2;
            permukaan_garis(target,
                mx, py + 4, mx - 4, py + 8,
                WIDGET_WARNA_TEKS);
            permukaan_garis(target,
                mx, py + 4, mx + 4, py + 8,
                WIDGET_WARNA_TEKS);
        }
        /* Panah bawah */
        {
            tak_bertanda32_t mx = px + pw / 2;
            tak_bertanda32_t by = py + ph - 5;
            permukaan_garis(target,
                mx - 4, by - 4, mx, by,
                WIDGET_WARNA_TEKS);
            permukaan_garis(target,
                mx + 4, by - 4, mx, by,
                WIDGET_WARNA_TEKS);
        }
    }

    w->flags &= ~WIDGET_FLAG_KOTOR;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - PROSES EVENT BAR GULIR
 * ===========================================================================
 */

void bar_gulir_proses_event(widget_t *w,
                             const widget_event_t *evt)
{
    widget_bar_gulir_data_t *data;

    if (w == NULL || evt == NULL) {
        return;
    }
    if (w->tipe != WIDGET_BAR_GULIR) {
        return;
    }

    data = (widget_bar_gulir_data_t *)w->data;
    if (data == NULL) {
        return;
    }

    switch (evt->tipe) {
    case WEVENT_TEKAN:
        data->sedang_drag = BENAR;
        if (data->orientasi == 0) {
            data->drag_offset =
                (tanda32_t)evt->y -
                (tanda32_t)(w->y + 1 +
                    data->handle_posisi);
        } else {
            data->drag_offset =
                (tanda32_t)evt->x -
                (tanda32_t)(w->x + 1 +
                    data->handle_posisi);
        }
        w->flags |= WIDGET_FLAG_KOTOR;
        break;

    case WEVENT_LEPAS:
        data->sedang_drag = SALAH;
        w->flags |= WIDGET_FLAG_KOTOR;
        break;

    case WEVENT_GERAK:
        if (data->sedang_drag) {
            tanda32_t rentang = data->nilai_maks
                - data->nilai_min;
            tak_bertanda32_t track_len;
            tanda32_t delta;
            tanda32_t pos;

            if (rentang <= 0) {
                break;
            }

            track_len = data->orientasi == 0
                ? w->tinggi - 2 : w->lebar - 2;

            if (track_len <= data->handle_lebar) {
                break;
            }

            track_len -= data->handle_lebar;

            if (data->orientasi == 0) {
                delta = (tanda32_t)evt->y -
                    (tanda32_t)(w->y + 1) -
                    data->drag_offset;
            } else {
                delta = (tanda32_t)evt->x -
                    (tanda32_t)(w->x + 1) -
                    data->drag_offset;
            }

            if (delta < 0) {
                delta = 0;
            }
            if (delta > (tanda32_t)track_len) {
                delta = (tanda32_t)track_len;
            }

            pos = (tanda32_t)(
                ((tanda64_t)delta * (tanda64_t)rentang)
                / (tanda64_t)track_len);

            data->nilai_sekarang =
                data->nilai_min + pos;
            data->handle_posisi = (tak_bertanda32_t)delta;
            w->flags |= WIDGET_FLAG_KOTOR;
        }
        break;

    case WEVENT_GULIR:
        {
            tanda32_t baru = data->nilai_sekarang
                + evt->gulir_delta * data->langkah;
            bar_gulir_set_nilai(w, baru);
        }
        break;

    case WEVENT_KLIK:
        {
            tak_bertanda32_t track_len =
                data->orientasi == 0
                ? w->tinggi - 2 : w->lebar - 2;
            tak_bertanda32_t klik;

            if (data->orientasi == 0) {
                klik = (tak_bertanda32_t)evt->y -
                    (tak_bertanda32_t)(w->y + 1);
            } else {
                klik = (tak_bertanda32_t)evt->x -
                    (tak_bertanda32_t)(w->x + 1);
            }

            /* Klik di atas handle: no-op */
            if (klik >= data->handle_posisi &&
                klik <= data->handle_posisi +
                data->handle_lebar) {
                break;
            }

            /* Page up/down: scroll satu halaman */
            if (klik < data->handle_posisi) {
                data->nilai_sekarang -=
                    data->ukuran_halaman;
            } else {
                data->nilai_sekarang +=
                    data->ukuran_halaman;
            }

            if (data->nilai_sekarang < data->nilai_min) {
                data->nilai_sekarang =
                    data->nilai_min;
            }
            if (data->nilai_sekarang > data->nilai_maks) {
                data->nilai_sekarang =
                    data->nilai_maks;
            }
            w->flags |= WIDGET_FLAG_KOTOR;
        }
        break;

    default:
        break;
    }
}
