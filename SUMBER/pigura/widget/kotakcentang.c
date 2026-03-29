/*
 * PIGURA OS - KOTAKCENTANG.C
 * ===========================
 * Widget kotak centang (checkbox) Pigura OS.
 *
 * Menyediakan toggle on/off dengan label teks,
 * mendukung status tristate (on/off/indeterminate).
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
 * API PUBLIK - INISIALISASI DATA KOTAK CENTANG
 * ===========================================================================
 */

status_t kotak_centang_init_data(widget_t *w,
                                  const char *label)
{
    widget_kotak_centang_data_t *data;

    if (w == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (w->tipe != WIDGET_KOTAK_CENTANG) {
        return STATUS_PARAM_INVALID;
    }

    data = (widget_kotak_centang_data_t *)w->data;
    if (data == NULL) {
        return STATUS_PARAM_NULL;
    }

    kernel_memset(data, 0,
        sizeof(widget_kotak_centang_data_t));

    if (label != NULL) {
        ukuran_t pj = kernel_strlen(label);
        if (pj >= WIDGET_LABEL_PANJANG) {
            pj = WIDGET_LABEL_PANJANG - 1;
        }
        kernel_memcpy(data->label, label, pj);
        data->label[pj] = '\0';
    } else {
        data->label[0] = '\0';
    }

    data->tercentang = SALAH;
    data->warna_centang = WIDGET_WARNA_CENTANG;
    data->ukuran = WIDGET_CENTANG_UKURAN;
    data->status_tristate = 0;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - SET LABEL
 * ===========================================================================
 */

status_t kotak_centang_set_label(widget_t *w,
                                  const char *label)
{
    widget_kotak_centang_data_t *data;

    if (w == NULL || label == NULL) {
        return STATUS_PARAM_NULL;
    }

    data = (widget_kotak_centang_data_t *)w->data;
    if (data == NULL) {
        return STATUS_PARAM_NULL;
    }

    {
        ukuran_t pj = kernel_strlen(label);
        if (pj >= WIDGET_LABEL_PANJANG) {
            pj = WIDGET_LABEL_PANJANG - 1;
        }
        kernel_memcpy(data->label, label, pj);
        data->label[pj] = '\0';
    }

    w->flags |= WIDGET_FLAG_KOTOR;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - DAPAT LABEL
 * ===========================================================================
 */

const char *kotak_centang_dapat_label(
    const widget_t *w)
{
    widget_kotak_centang_data_t *data;

    if (w == NULL) {
        return "";
    }
    data = (widget_kotak_centang_data_t *)w->data;
    if (data == NULL) {
        return "";
    }
    return data->label;
}

/*
 * ===========================================================================
 * API PUBLIK - TOGGLE STATUS
 * ===========================================================================
 */

void kotak_centang_toggle(widget_t *w)
{
    widget_kotak_centang_data_t *data;

    if (w == NULL) {
        return;
    }
    if (w->flags & WIDGET_FLAG_NONAKTIF) {
        return;
    }

    data = (widget_kotak_centang_data_t *)w->data;
    if (data == NULL) {
        return;
    }

    if (data->tercentang) {
        data->tercentang = SALAH;
        data->status_tristate = 0;
    } else {
        data->tercentang = BENAR;
        data->status_tristate = 1;
    }
    w->flags |= WIDGET_FLAG_KOTOR;
}

/*
 * ===========================================================================
 * API PUBLIK - SET STATUS CENTANG
 * ===========================================================================
 */

void kotak_centang_set_status(widget_t *w, bool_t val)
{
    widget_kotak_centang_data_t *data;

    if (w == NULL) {
        return;
    }

    data = (widget_kotak_centang_data_t *)w->data;
    if (data == NULL) {
        return;
    }

    data->tercentang = val;
    data->status_tristate = val ? 1 : 0;
    w->flags |= WIDGET_FLAG_KOTOR;
}

/*
 * ===========================================================================
 * API PUBLIK - DAPAT STATUS CENTANG
 * ===========================================================================
 */

bool_t kotak_centang_dapat_status(const widget_t *w)
{
    widget_kotak_centang_data_t *data;

    if (w == NULL) {
        return SALAH;
    }

    data = (widget_kotak_centang_data_t *)w->data;
    if (data == NULL) {
        return SALAH;
    }

    return data->tercentang;
}

/*
 * ===========================================================================
 * API PUBLIK - SET WARNA CENTANG
 * ===========================================================================
 */

void kotak_centang_set_warna(widget_t *w,
                              tak_bertanda32_t warna)
{
    widget_kotak_centang_data_t *data;

    if (w == NULL) {
        return;
    }

    data = (widget_kotak_centang_data_t *)w->data;
    if (data == NULL) {
        return;
    }

    data->warna_centang = warna;
    w->flags |= WIDGET_FLAG_KOTOR;
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL - GAMBAR TANDA CENTANG
 * ===========================================================================
 *
 * Menggambar tanda centang (checkmark) di dalam kotak.
 * Menggunakan dua garis diagonal yang membentuk V.
 */

static void gambar_tanda(permukaan_t *target,
                          tak_bertanda32_t cx,
                          tak_bertanda32_t cy,
                          tak_bertanda32_t ukuran,
                          tak_bertanda32_t warna)
{
    tak_bertanda32_t pad = ukuran / 4;
    tak_bertanda32_t x1, y1, x2, y2;

    x1 = cx + pad;
    y1 = cy + ukuran / 2;
    x2 = cx + ukuran / 2;
    y2 = cy + ukuran - pad;
    permukaan_garis(target, x1, y1, x2, y2, warna);

    x1 = cx + ukuran / 2;
    y1 = cy + ukuran - pad;
    x2 = cx + ukuran - pad;
    y2 = cy + pad;
    permukaan_garis(target, x1, y1, x2, y2, warna);
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL - GAMBAR TANDA INDETERMINATE
 * ===========================================================================
 *
 * Menggambar tanda garis horizontal di tengah kotak
 * untuk status indeterminate.
 */

static void gambar_indeterminate(permukaan_t *target,
                                  tak_bertanda32_t cx,
                                  tak_bertanda32_t cy,
                                  tak_bertanda32_t ukuran,
                                  tak_bertanda32_t warna)
{
    tak_bertanda32_t pad = ukuran / 3;
    permukaan_garis(target,
                     cx + pad, cy + ukuran / 2,
                     cx + ukuran - pad,
                     cy + ukuran / 2, warna);
}

/*
 * ===========================================================================
 * API PUBLIK - RENDER KOTAK CENTANG
 * ===========================================================================
 */

status_t kotak_centang_render(widget_t *w,
                               permukaan_t *target,
                               tak_bertanda32_t ox,
                               tak_bertanda32_t oy)
{
    widget_kotak_centang_data_t *data;
    tak_bertanda32_t px, py;
    tak_bertanda32_t warna_border;
    tak_bertanda32_t warna_teks;
    tak_bertanda32_t ukuran;
    tak_bertanda32_t cx, cy;

    if (w == NULL || target == NULL) {
        return STATUS_PARAM_NULL;
    }

    data = (widget_kotak_centang_data_t *)w->data;
    if (data == NULL) {
        return STATUS_PARAM_NULL;
    }

    px = ox + (tak_bertanda32_t)w->x;
    py = oy + (tak_bertanda32_t)w->y;
    ukuran = data->ukuran;

    /* Posisi kotak centang (vertikal centered) */
    cx = px;
    cy = py + (w->tinggi / 2) - (ukuran / 2);

    warna_border = w->flags & WIDGET_FLAG_NONAKTIF
        ? WIDGET_WARNA_TEKS_NONAKTIF : WIDGET_WARNA_BORDER;
    warna_teks = w->flags & WIDGET_FLAG_NONAKTIF
        ? WIDGET_WARNA_TEKS_NONAKTIF : WIDGET_WARNA_TEKS;

    /* Gambar kotak centang (outline) */
    permukaan_kotak(target, cx, cy, ukuran, ukuran,
                     warna_border, SALAH);

    /* Isi latar jika ditekan */
    if (w->flags & WIDGET_FLAG_TEKAN) {
        permukaan_isi_kotak(target,
                            cx + 1, cy + 1,
                            ukuran - 2, ukuran - 2,
                            WIDGET_WARNA_LATAR);
    }

    /* Gambar tanda berdasarkan status */
    if (data->status_tristate == 2) {
        /* Indeterminate */
        gambar_indeterminate(target, cx + 1, cy + 1,
                             ukuran - 2,
                             data->warna_centang);
    } else if (data->tercentang) {
        /* Centang (checkmark) */
        gambar_tanda(target, cx + 1, cy + 1,
                     ukuran - 2, data->warna_centang);
    }

    /* Fokus ring */
    if (w->flags & WIDGET_FLAG_FOKUS) {
        permukaan_kotak(target, cx - 1, cy - 1,
                         ukuran + 2, ukuran + 2,
                         WIDGET_WARNA_FOKUS, SALAH);
    }

    /* Label teks di sebelah kanan kotak */
    {
        tak_bertanda32_t lx = cx + ukuran + 4;
        tak_bertanda32_t ly = py + (w->tinggi / 2) - 4;
        (void)lx;
        (void)ly;
        (void)warna_teks;
    }

    w->flags &= ~WIDGET_FLAG_KOTOR;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - PROSES EVENT KOTAK CENTANG
 * ===========================================================================
 */

void kotak_centang_proses_event(widget_t *w,
                                 const widget_event_t *evt)
{
    if (w == NULL || evt == NULL) {
        return;
    }
    if (w->tipe != WIDGET_KOTAK_CENTANG) {
        return;
    }
    if (w->flags & WIDGET_FLAG_NONAKTIF) {
        return;
    }

    switch (evt->tipe) {
    case WEVENT_KLIK:
        kotak_centang_toggle(w);
        break;

    case WEVENT_TEKAN:
        w->flags |= WIDGET_FLAG_TEKAN;
        w->flags |= WIDGET_FLAG_KOTOR;
        break;

    case WEVENT_LEPAS:
        w->flags &= ~WIDGET_FLAG_TEKAN;
        w->flags |= WIDGET_FLAG_KOTOR;
        break;

    case WEVENT_ARAH_MASUK:
        w->flags |= WIDGET_FLAG_ARAH;
        w->flags |= WIDGET_FLAG_KOTOR;
        break;

    case WEVENT_ARAH_KELUAR:
        w->flags &= ~WIDGET_FLAG_ARAH;
        w->flags |= WIDGET_FLAG_KOTOR;
        break;

    case WEVENT_FOKUS_MASUK:
        w->flags |= WIDGET_FLAG_FOKUS;
        w->flags |= WIDGET_FLAG_KOTOR;
        break;

    case WEVENT_FOKUS_KELUAR:
        w->flags &= ~WIDGET_FLAG_FOKUS;
        w->flags |= WIDGET_FLAG_KOTOR;
        break;

    case WEVENT_KUNCI_TEKAN:
        if (evt->kode_kunci == 57 ||
            evt->kode_kunci == 28) {
            kotak_centang_toggle(w);
        }
        break;

    default:
        break;
    }
}
