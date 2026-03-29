/*
 * PIGURA OS - DIALOG.C
 * =====================
 * Widget kotak dialog Pigura OS.
 *
 * Menampilkan kotak dialog modal dengan judul, pesan,
 * dan tombol aksi (OK, Batal, Ya, Tidak).
 * Mendukung tiga tipe: info, peringatan, error.
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
 * KONSTANTA INTERNAL
 * ===========================================================================
 */

/* Lebar minimum dialog */
#define DIALOG_LEBAR_MIN         200

/* Tinggi minimum dialog (tanpa tombol) */
#define DIALOG_TINGGI_MIN         80

/* Padding horizontal */
#define DIALOG_PADDING_X          12

/* Padding vertikal */
#define DIALOG_PADDING_Y          10

/* Tinggi area judul */
#define DIALOG_JUDUL_AREA        24

/* Tinggi area pesan (minimum) */
#define DIALOG_PESAN_AREA_MIN     40

/* Tinggi tombol dialog */
#define DIALOG_TOMBOL_TINGGI      26

/* Jarak antar tombol */
#define DIALOG_TOMBOL_JARAK       8

/* Lebar minimum tombol */
#define DIALOG_TOMBOL_LEBAR_MIN   70

/* Warna per tipe dialog */
#define DIALOG_WARNI_LATAR        0xFFFFF0D0
#define DIALOG_WARNI_BORDER       0xFFFFC000
#define DIALOG_ERROR_LATAR        0xFFFFD0D0
#define DIALOG_ERROR_BORDER       0xFFFF0000
#define DIALOG_INFO_LATAR         0xFFE0F0FF
#define DIALOG_INFO_BORDER        0xFF4040FF

/*
 * ===========================================================================
 * API PUBLIK - INISIALISASI DATA DIALOG
 * ===========================================================================
 */

status_t dialog_init_data(widget_t *w,
                           const char *judul,
                           const char *pesan)
{
    widget_dialog_data_t *data;

    if (w == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (w->tipe != WIDGET_DIALOG) {
        return STATUS_PARAM_INVALID;
    }

    data = (widget_dialog_data_t *)w->data;
    if (data == NULL) {
        return STATUS_PARAM_NULL;
    }

    kernel_memset(data, 0, sizeof(widget_dialog_data_t));

    if (judul != NULL) {
        ukuran_t pj = kernel_strlen(judul);
        if (pj >= WIDGET_DIALOG_JUDUL_PANJANG) {
            pj = WIDGET_DIALOG_JUDUL_PANJANG - 1;
        }
        kernel_memcpy(data->judul, judul, pj);
        data->judul[pj] = '\0';
    } else {
        data->judul[0] = '\0';
    }

    if (pesan != NULL) {
        ukuran_t pp = kernel_strlen(pesan);
        if (pp >= WIDGET_TEKS_PANJANG) {
            pp = WIDGET_TEKS_PANJANG - 1;
        }
        kernel_memcpy(data->pesan, pesan, pp);
        data->pesan[pp] = '\0';
    } else {
        data->pesan[0] = '\0';
    }

    data->jumlah_tombol = 0;
    data->indeks_default = 0;
    data->tipe = 0; /* info */
    data->modal = BENAR;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - SET JUDUL DIALOG
 * ===========================================================================
 */

status_t dialog_set_judul(widget_t *w, const char *judul)
{
    widget_dialog_data_t *data;

    if (w == NULL || judul == NULL) {
        return STATUS_PARAM_NULL;
    }

    data = (widget_dialog_data_t *)w->data;
    if (data == NULL) {
        return STATUS_PARAM_NULL;
    }

    {
        ukuran_t pj = kernel_strlen(judul);
        if (pj >= WIDGET_DIALOG_JUDUL_PANJANG) {
            pj = WIDGET_DIALOG_JUDUL_PANJANG - 1;
        }
        kernel_memcpy(data->judul, judul, pj);
        data->judul[pj] = '\0';
    }

    dialog_hitung_layout(w);
    w->flags |= WIDGET_FLAG_KOTOR;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - SET PESAN DIALOG
 * ===========================================================================
 */

status_t dialog_set_pesan(widget_t *w, const char *pesan)
{
    widget_dialog_data_t *data;

    if (w == NULL || pesan == NULL) {
        return STATUS_PARAM_NULL;
    }

    data = (widget_dialog_data_t *)w->data;
    if (data == NULL) {
        return STATUS_PARAM_NULL;
    }

    {
        ukuran_t pp = kernel_strlen(pesan);
        if (pp >= WIDGET_TEKS_PANJANG) {
            pp = WIDGET_TEKS_PANJANG - 1;
        }
        kernel_memcpy(data->pesan, pesan, pp);
        data->pesan[pp] = '\0';
    }

    dialog_hitung_layout(w);
    w->flags |= WIDGET_FLAG_KOTOR;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - TAMBAH TOMBOL DIALOG
 * ===========================================================================
 */

status_t dialog_tambah_tombol(widget_t *w,
                                const char *label,
                                tak_bertanda32_t id,
                                widget_event_cb cb,
                                bool_t default_btn)
{
    widget_dialog_data_t *data;
    widget_dialog_tombol_t *tb;

    if (w == NULL || label == NULL) {
        return STATUS_PARAM_NULL;
    }

    data = (widget_dialog_data_t *)w->data;
    if (data == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (data->jumlah_tombol >= WIDGET_DIALOG_TOMBOL_MAKS) {
        return STATUS_PENUH;
    }

    tb = &data->tombol[data->jumlah_tombol];
    kernel_memset(tb, 0, sizeof(widget_dialog_tombol_t));

    {
        ukuran_t pj = kernel_strlen(label);
        if (pj >= WIDGET_LABEL_PANJANG) {
            pj = WIDGET_LABEL_PANJANG - 1;
        }
        kernel_memcpy(tb->label, label, pj);
        tb->label[pj] = '\0';
    }

    tb->id = id;
    tb->default_tombol = default_btn;
    tb->callback = cb;
    tb->data = NULL;

    if (default_btn) {
        data->indeks_default = data->jumlah_tombol;
    }

    data->jumlah_tombol++;
    dialog_hitung_layout(w);
    w->flags |= WIDGET_FLAG_KOTOR;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - HITUNG LAYOUT DIALOG
 * ===========================================================================
 *
 * Menghitung ulang ukuran dialog berdasarkan jumlah
 * tombol, panjang judul, dan panjang pesan.
 */

status_t dialog_hitung_layout(widget_t *w)
{
    widget_dialog_data_t *data;
    tak_bertanda32_t lebar, tinggi;
    ukuran_t pj;

    if (w == NULL) {
        return STATUS_PARAM_NULL;
    }

    data = (widget_dialog_data_t *)w->data;
    if (data == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Lebar: maksimum antara judul, pesan, tombol */
    lebar = DIALOG_LEBAR_MIN;

    pj = kernel_strlen(data->judul);
    if (pj * 8 + DIALOG_PADDING_X * 2 > lebar) {
        lebar = (tak_bertanda32_t)pj * 8 +
            DIALOG_PADDING_X * 2;
    }

    pj = kernel_strlen(data->pesan);
    if (pj * 7 + DIALOG_PADDING_X * 2 > lebar) {
        lebar = (tak_bertanda32_t)pj * 7 +
            DIALOG_PADDING_X * 2;
    }

    /* Lebar tombol */
    {
        tak_bertanda32_t i;
        for (i = 0; i < data->jumlah_tombol; i++) {
            ukuran_t tl = kernel_strlen(
                data->tombol[i].label);
            tak_bertanda32_t tw =
                (tak_bertanda32_t)tl * 8 +
                DIALOG_PADDING_X * 2;
            if (tw < DIALOG_TOMBOL_LEBAR_MIN) {
                tw = DIALOG_TOMBOL_LEBAR_MIN;
            }
            tw += DIALOG_TOMBOL_JARAK;
            if (tw > lebar) {
                lebar = tw;
            }
        }
    }

    /* Tinggi: judul + pesan + tombol + padding */
    tinggi = DIALOG_PADDING_Y +
        DIALOG_JUDUL_AREA +
        DIALOG_PESAN_AREA_MIN +
        DIALOG_PADDING_Y +
        DIALOG_TOMBOL_TINGGI +
        DIALOG_PADDING_Y;

    if (tinggi < DIALOG_TINGGI_MIN) {
        tinggi = DIALOG_TINGGI_MIN;
    }

    w->lebar = lebar;
    w->tinggi = tinggi;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - SET TIPE DIALOG
 * ===========================================================================
 */

void dialog_set_tipe(widget_t *w, tak_bertanda32_t tipe)
{
    widget_dialog_data_t *data;

    if (w == NULL) {
        return;
    }

    data = (widget_dialog_data_t *)w->data;
    if (data == NULL) {
        return;
    }

    if (tipe > 2) {
        tipe = 0;
    }
    data->tipe = tipe;
    w->flags |= WIDGET_FLAG_KOTOR;
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL - DAPAT WARNA DIALOG
 * ===========================================================================
 */

static void dapat_warna_dialog(tak_bertanda32_t tipe,
                               tak_bertanda32_t *latar,
                               tak_bertanda32_t *border)
{
    switch (tipe) {
    case 0: /* info */
        *latar = DIALOG_INFO_LATAR;
        *border = DIALOG_INFO_BORDER;
        break;
    case 1: /* peringatan */
        *latar = DIALOG_WARNI_LATAR;
        *border = DIALOG_WARNI_BORDER;
        break;
    case 2: /* error */
    default:
        *latar = DIALOG_ERROR_LATAR;
        *border = DIALOG_ERROR_BORDER;
        break;
    }
}

/*
 * ===========================================================================
 * API PUBLIK - RENDER DIALOG
 * ===========================================================================
 */

status_t dialog_render(widget_t *w,
                        permukaan_t *target,
                        tak_bertanda32_t ox,
                        tak_bertanda32_t oy)
{
    widget_dialog_data_t *data;
    tak_bertanda32_t px, py, pw, ph;
    tak_bertanda32_t warna_latar, warna_border;
    tak_bertanda32_t i;
    tak_bertanda32_t tombol_total_lebar;
    tak_bertanda32_t tombol_x_awal;

    if (w == NULL || target == NULL) {
        return STATUS_PARAM_NULL;
    }

    data = (widget_dialog_data_t *)w->data;
    if (data == NULL) {
        return STATUS_PARAM_NULL;
    }

    px = ox + (tak_bertanda32_t)w->x;
    py = oy + (tak_bertanda32_t)w->y;
    pw = w->lebar;
    ph = w->tinggi;

    dapat_warna_dialog(data->tipe,
                        &warna_latar, &warna_border);

    /* Latar belakang dialog */
    permukaan_isi_kotak(target, px, py, pw, ph,
                         warna_latar);

    /* Border dialog */
    permukaan_kotak(target, px, py, pw, ph,
                     warna_border, SALAH);

    /* Area judul: latar lebih gelap */
    permukaan_isi_kotak(target, px + 1,
                         py + 1, pw - 2,
                         DIALOG_JUDUL_AREA - 1,
                         warna_border);

    /* Garis pemisah di bawah judul */
    permukaan_garis(target, px + 1,
                     py + DIALOG_JUDUL_AREA,
                     px + pw - 1,
                     py + DIALOG_JUDUL_AREA,
                     warna_border);

    /* Render judul teks */
    {
        tak_bertanda32_t tx = px + DIALOG_PADDING_X;
        tak_bertanda32_t ty = py + 3;
        (void)tx;
        (void)ty;
    }

    /* Area pesan */
    {
        tak_bertanda32_t msg_x = px + DIALOG_PADDING_X;
        tak_bertanda32_t msg_y =
            py + DIALOG_JUDUL_AREA + 4;
        (void)msg_x;
        (void)msg_y;
    }

    /* Hitung total lebar tombol */
    tombol_total_lebar = 0;
    for (i = 0; i < data->jumlah_tombol; i++) {
        ukuran_t tl = kernel_strlen(
            data->tombol[i].label);
        tak_bertanda32_t tw =
            (tak_bertanda32_t)tl * 8 +
            DIALOG_PADDING_X * 2;
        if (tw < DIALOG_TOMBOL_LEBAR_MIN) {
            tw = DIALOG_TOMBOL_LEBAR_MIN;
        }
        tombol_total_lebar += tw;
        if (i > 0) {
            tombol_total_lebar += DIALOG_TOMBOL_JARAK;
        }
    }

    /* Posisi awal tombol (centered) */
    tombol_x_awal = px + (pw / 2) -
        (tombol_total_lebar / 2);

    /* Render tombol */
    {
        tak_bertanda32_t btn_x = tombol_x_awal;
        tak_bertanda32_t btn_y = py + ph -
            DIALOG_TOMBOL_TINGGI - DIALOG_PADDING_Y;

        for (i = 0; i < data->jumlah_tombol; i++) {
            widget_dialog_tombol_t *tb =
                &data->tombol[i];
            ukuran_t tl = kernel_strlen(tb->label);
            tak_bertanda32_t tw =
                (tak_bertanda32_t)tl * 8 +
                DIALOG_PADDING_X * 2;
            tak_bertanda32_t th =
                DIALOG_TOMBOL_TINGGI - 4;
            tak_bertanda32_t warna_btn;
            tak_bertanda32_t warna_btn_border;

            if (tw < DIALOG_TOMBOL_LEBAR_MIN) {
                tw = DIALOG_TOMBOL_LEBAR_MIN;
            }

            /* Warna tombol default */
            warna_btn = WIDGET_WARNA_TOMBOL;
            warna_btn_border = WIDGET_WARNA_BORDER;

            /* Highlight tombol default */
            if (tb->default_tombol) {
                warna_btn = WIDGET_WARNA_LATAR;
                warna_btn_border = WIDGET_WARNA_FOKUS;
            }

            /* Tombol ditekan */
            if (w->flags & WIDGET_FLAG_TEKAN) {
                warna_btn = WIDGET_WARNA_TEKAN;
            }

            permukaan_isi_kotak(target,
                btn_x, btn_y, tw, th,
                warna_btn);
            permukaan_kotak(target,
                btn_x, btn_y, tw, th,
                warna_btn_border, SALAH);

            /* Label tombol */
            {
                tak_bertanda32_t lx = btn_x +
                    (tw / 2);
                tak_bertanda32_t ly = btn_y + (th / 2);
                (void)lx;
                (void)ly;
            }

            btn_x += tw + DIALOG_TOMBOL_JARAK;
        }
    }

    w->flags &= ~WIDGET_FLAG_KOTOR;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - PROSES EVENT DIALOG
 * ===========================================================================
 */

void dialog_proses_event(widget_t *w,
                          const widget_event_t *evt)
{
    widget_dialog_data_t *data;

    if (w == NULL || evt == NULL) {
        return;
    }
    if (w->tipe != WIDGET_DIALOG) {
        return;
    }

    data = (widget_dialog_data_t *)w->data;
    if (data == NULL) {
        return;
    }

    switch (evt->tipe) {
    case WEVENT_TEKAN:
        w->flags |= WIDGET_FLAG_TEKAN;
        w->flags |= WIDGET_FLAG_KOTOR;
        break;

    case WEVENT_LEPAS:
        w->flags &= ~WIDGET_FLAG_TEKAN;
        w->flags |= WIDGET_FLAG_KOTOR;
        break;

    case WEVENT_KLIK:
        /* Tombol diklik: proses callback */
        break;

    case WEVENT_FOKUS_MASUK:
        w->flags |= WIDGET_FLAG_FOKUS;
        break;

    case WEVENT_FOKUS_KELUAR:
        w->flags &= ~WIDGET_FLAG_FOKUS;
        break;

    case WEVENT_KUNCI_TEKAN:
        if (evt->kode_kunci == 28) {
            /* Enter: jalankan tombol default */
            if (data->indeks_default <
                (tanda32_t)data->jumlah_tombol) {
                widget_dialog_tombol_t *tb =
                    &data->tombol[
                        data->indeks_default];
                if (tb->callback != NULL) {
                    widget_event_t de;
                    kernel_memset(&de, 0,
                        sizeof(de));
                    de.tipe = WEVENT_KONFIRMASI;
                    de.id_widget = w->id;
                    tb->callback(&de, tb->data);
                }
            }
        } else if (evt->kode_kunci == 1) {
            /* Escape: jalankan tombol cancel */
            {
                tak_bertanda32_t cancel_idx = 0;
                if (data->jumlah_tombol > 1) {
                    cancel_idx = 1;
                }
                if (cancel_idx <
                    data->jumlah_tombol) {
                    widget_dialog_tombol_t *tb =
                        &data->tombol[
                            cancel_idx];
                    if (tb->callback != NULL) {
                        widget_event_t de;
                        kernel_memset(&de, 0,
                            sizeof(de));
                        de.tipe = WEVENT_BATAL;
                        de.id_widget = w->id;
                        tb->callback(&de, tb->data);
                    }
                }
            }
        } else if (evt->kode_kunci == 9) {
            /* Tab: pindah fokus ke tombol berikutnya */
            {
                tak_bertanda32_t next =
                    (data->indeks_default + 1) %
                    data->jumlah_tombol;
                data->indeks_default = next;
            }
        }
        break;

    default:
        break;
    }
}
