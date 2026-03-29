/*
 * PIGURA OS - TOMBOL.C
 * ====================
 * Widget tombol tekan Pigura OS.
 *
 * Tombol merespons klik (press + release), hover,
 * dan menampilkan label teks. Mendukung tiga
 * state visual: normal, hover, dan pressed.
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

/* Padding horizontal dalam tombol */
#define TOMBOL_PADDING_X          8

/* Padding vertikal dalam tombol */
#define TOMBOL_PADDING_Y          4

/* Lebar border tombol */
#define TOMBOL_BORDER            1

/* Sudut border: 0=tidak ada */
#define TOMBOL_SUDUT              0

/*
 * ===========================================================================
 * API PUBLIK - INISIALISASI DATA TOMBOL
 * ===========================================================================
 */

status_t tombol_init_data(widget_t *w, const char *label)
{
    widget_tombol_data_t *data;

    if (w == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (w->tipe != WIDGET_TOMBOL) {
        return STATUS_PARAM_INVALID;
    }

    data = (widget_tombol_data_t *)w->data;
    if (data == NULL) {
        return STATUS_PARAM_NULL;
    }

    kernel_memset(data, 0, sizeof(widget_tombol_data_t));

    if (label != NULL) {
        ukuran_t panjang = kernel_strlen(label);
        if (panjang >= WIDGET_LABEL_PANJANG) {
            panjang = WIDGET_LABEL_PANJANG - 1;
        }
        kernel_memcpy(data->label, label, panjang);
        data->label[panjang] = '\0';
    } else {
        data->label[0] = '\0';
    }

    data->warna_normal = WIDGET_WARNA_TOMBOL;
    data->warna_tekan = WIDGET_WARNA_TEKAN;
    data->warna_arah = WIDGET_WARNA_LATAR;
    data->jumlah_klik = 0;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - SET LABEL TOMBOL
 * ===========================================================================
 */

status_t tombol_set_label(widget_t *w, const char *label)
{
    widget_tombol_data_t *data;

    if (w == NULL || label == NULL) {
        return STATUS_PARAM_NULL;
    }

    data = (widget_tombol_data_t *)w->data;
    if (data == NULL) {
        return STATUS_PARAM_NULL;
    }

    {
        ukuran_t panjang = kernel_strlen(label);
        if (panjang >= WIDGET_LABEL_PANJANG) {
            panjang = WIDGET_LABEL_PANJANG - 1;
        }
        kernel_memcpy(data->label, label, panjang);
        data->label[panjang] = '\0';
    }

    w->flags |= WIDGET_FLAG_KOTOR;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - DAPAT LABEL TOMBOL
 * ===========================================================================
 */

const char *tombol_dapat_label(const widget_t *w)
{
    widget_tombol_data_t *data;

    if (w == NULL) {
        return "";
    }

    data = (widget_tombol_data_t *)w->data;
    if (data == NULL) {
        return "";
    }

    return data->label;
}

/*
 * ===========================================================================
 * API PUBLIK - SET WARNA TOMBOL
 * ===========================================================================
 */

void tombol_set_warna(widget_t *w,
                       tak_bertanda32_t normal,
                       tak_bertanda32_t tekan,
                       tak_bertanda32_t arah)
{
    widget_tombol_data_t *data;

    if (w == NULL) {
        return;
    }

    data = (widget_tombol_data_t *)w->data;
    if (data == NULL) {
        return;
    }

    data->warna_normal = normal;
    data->warna_tekan = tekan;
    data->warna_arah = arah;
    w->flags |= WIDGET_FLAG_KOTOR;
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL - DAPAT WARNA AKTIF
 * ===========================================================================
 *
 * Mengembalikan warna tombol berdasarkan state saat ini.
 */

static tak_bertanda32_t dapat_warna_aktif(const widget_t *w)
{
    widget_tombol_data_t *data;

    if (w == NULL) {
        return WIDGET_WARNA_TOMBOL;
    }

    data = (widget_tombol_data_t *)w->data;
    if (data == NULL) {
        return WIDGET_WARNA_TOMBOL;
    }

    if (w->flags & WIDGET_FLAG_TEKAN) {
        return data->warna_tekan;
    }
    if (w->flags & WIDGET_FLAG_ARAH) {
        return data->warna_arah;
    }
    return data->warna_normal;
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL - DAPAT WARNA TEKS AKTIF
 * ===========================================================================
 */

static tak_bertanda32_t dapat_warna_teks(const widget_t *w)
{
    if (w == NULL) {
        return WIDGET_WARNA_TEKS;
    }
    if (w->flags & WIDGET_FLAG_NONAKTIF) {
        return WIDGET_WARNA_TEKS_NONAKTIF;
    }
    return WIDGET_WARNA_TEKS;
}

/*
 * ===========================================================================
 * API PUBLIK - RENDER TOMBOL
 * ===========================================================================
 *
 * Menggambar tombol ke permukaan target:
 *   1. Isi latar dengan warna aktif
 *   2. Gambar border
 *   3. Gambar fokus ring jika aktif
 *   4. Gambar label teks di tengah
 */

status_t tombol_render(widget_t *w,
                        permukaan_t *target,
                        tak_bertanda32_t ox,
                        tak_bertanda32_t oy)
{
    widget_tombol_data_t *data;
    tak_bertanda32_t warna_bg;
    tak_bertanda32_t warna_fg;
    tak_bertanda32_t px, py, pw, ph;
    tak_bertanda32_t tx, ty;

    if (w == NULL || target == NULL) {
        return STATUS_PARAM_NULL;
    }

    data = (widget_tombol_data_t *)w->data;
    if (data == NULL) {
        return STATUS_PARAM_NULL;
    }

    px = ox + (tak_bertanda32_t)w->x;
    py = oy + (tak_bertanda32_t)w->y;
    pw = w->lebar;
    ph = w->tinggi;

    warna_bg = dapat_warna_aktif(w);
    warna_fg = dapat_warna_teks(w);

    /* Isi latar tombol */
    permukaan_isi_kotak(target, px, py, pw, ph, warna_bg);

    /* Gambar border */
    if (w->flags & WIDGET_FLAG_BORDER) {
        permukaan_kotak(target, px, py, pw, ph,
                        w->warna_border, SALAH);
    }

    /* Fokus ring */
    if (w->flags & WIDGET_FLAG_FOKUS) {
        permukaan_kotak(target, px - 1, py - 1,
                        pw + 2, ph + 2,
                        WIDGET_WARNA_FOKUS, SALAH);
    }

    /* Gambar label teks di tengah tombol.
     * Hitung posisi teks agar centered. */
    tx = px + TOMBOL_PADDING_X;
    ty = py + (ph / 2) - (ph > 20 ? 6 : 3);

    /* Gambar label sebagai piksel placeholder.
     * Pada implementasi penuh, akan menggunakan
     * subsistem teks untuk render font. */
    (void)data;
    (void)tx;
    (void)ty;
    (void)warna_fg;

    w->flags &= ~WIDGET_FLAG_KOTOR;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - PROSES EVENT TOMBOL
 * ===========================================================================
 *
 * Menangani event mouse dan keyboard untuk tombol:
 *   - KLIK: Tambah counter, panggil callback
 *   - TEKAN: Set flag pressed
 *   - LEPAS: Hapus flag pressed
 *   - ARAH_MASUK: Set hover
 *   - ARAH_KELUAR: Hapus hover
 *   - KUNCI_TEKAN: Jika spasi/enter, tekan tombol
 */

void tombol_proses_event(widget_t *w,
                          const widget_event_t *evt)
{
    widget_tombol_data_t *data;

    if (w == NULL || evt == NULL) {
        return;
    }
    if (w->tipe != WIDGET_TOMBOL) {
        return;
    }
    if (w->flags & WIDGET_FLAG_NONAKTIF) {
        return;
    }

    data = (widget_tombol_data_t *)w->data;
    if (data == NULL) {
        return;
    }

    switch (evt->tipe) {
    case WEVENT_KLIK:
        data->jumlah_klik++;
        w->flags |= WIDGET_FLAG_KOTOR;
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
        w->flags &= ~WIDGET_FLAG_TEKAN;
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
            w->flags |= WIDGET_FLAG_TEKAN;
            w->flags |= WIDGET_FLAG_KOTOR;
        }
        break;

    case WEVENT_KUNCI_LEPAS:
        if (evt->kode_kunci == 57 ||
            evt->kode_kunci == 28) {
            w->flags &= ~WIDGET_FLAG_TEKAN;
            data->jumlah_klik++;
            w->flags |= WIDGET_FLAG_KOTOR;
        }
        break;

    default:
        break;
    }
}
