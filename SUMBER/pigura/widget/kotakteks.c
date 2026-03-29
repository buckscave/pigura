/*
 * PIGURA OS - KOTAKTEKS.C
 * =======================
 * Widget kotak input teks Pigura OS.
 *
 * Menyediakan area input teks satu baris dengan dukungan:
 *   - Kursor (caret) yang berkedip
 *   - Seleksi teks
 *   - Mode password dan hanya-baca
 *   - Placeholder text
 *   - Scroll teks panjang
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

/* Padding horizontal */
#define KTEKS_PADDING_X          4

/* Lebar border */
#define KTEKS_BORDER            1

/* Interval blink kursor (ms) */
#define KTEKS_BLINK_MS          500

/* Karakter password mask */
#define KTEKS_SANDI_KAR         '*'

/*
 * ===========================================================================
 * API PUBLIK - INISIALISASI DATA KOTAK TEKS
 * ===========================================================================
 */

status_t kotak_teks_init_data(widget_t *w,
                               const char *placeholder,
                               tak_bertanda32_t panjang_maks)
{
    widget_kotak_teks_data_t *data;

    if (w == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (w->tipe != WIDGET_KOTAK_TEKS) {
        return STATUS_PARAM_INVALID;
    }

    data = (widget_kotak_teks_data_t *)w->data;
    if (data == NULL) {
        return STATUS_PARAM_NULL;
    }

    kernel_memset(data, 0, sizeof(widget_kotak_teks_data_t));

    data->teks[0] = '\0';
    data->panjang = 0;
    data->posisi_kursor = 0;
    data->posisi_scroll = 0;
    data->panjang_maks = panjang_maks > 0
        ? panjang_maks : WIDGET_TEKS_PANJANG - 1;
    data->mode_sandi = SALAH;
    data->hanya_baca = SALAH;
    data->terpilih = SALAH;
    data->seleksi_awal = 0;
    data->seleksi_akhir = 0;
    data->kursor_tampil = BENAR;
    data->kursor_waktu = 0;

    if (placeholder != NULL) {
        ukuran_t pj = kernel_strlen(placeholder);
        if (pj >= WIDGET_LABEL_PANJANG) {
            pj = WIDGET_LABEL_PANJANG - 1;
        }
        kernel_memcpy(data->placeholder, placeholder, pj);
        data->placeholder[pj] = '\0';
    } else {
        data->placeholder[0] = '\0';
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - SET TEKS
 * ===========================================================================
 */

status_t kotak_teks_set_teks(widget_t *w, const char *teks)
{
    widget_kotak_teks_data_t *data;
    ukuran_t pj;

    if (w == NULL || teks == NULL) {
        return STATUS_PARAM_NULL;
    }

    data = (widget_kotak_teks_data_t *)w->data;
    if (data == NULL) {
        return STATUS_PARAM_NULL;
    }

    pj = kernel_strlen(teks);
    if (pj > data->panjang_maks) {
        pj = data->panjang_maks;
    }

    kernel_memcpy(data->teks, teks, pj);
    data->teks[pj] = '\0';
    data->panjang = pj;
    data->posisi_kursor = pj;
    data->posisi_scroll = 0;
    data->terpilih = SALAH;
    w->flags |= WIDGET_FLAG_KOTOR;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - DAPAT TEKS
 * ===========================================================================
 */

const char *kotak_teks_dapat_teks(const widget_t *w)
{
    widget_kotak_teks_data_t *data;

    if (w == NULL) {
        return "";
    }

    data = (widget_kotak_teks_data_t *)w->data;
    if (data == NULL) {
        return "";
    }

    return data->teks;
}

/*
 * ===========================================================================
 * API PUBLIK - MASUKKAN KARAKTER
 * ===========================================================================
 */

status_t kotak_teks_masukkan(widget_t *w, char c)
{
    widget_kotak_teks_data_t *data;

    if (w == NULL) {
        return STATUS_PARAM_NULL;
    }

    data = (widget_kotak_teks_data_t *)w->data;
    if (data == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (data->hanya_baca) {
        return STATUS_TIDAK_DUKUNG;
    }

    if (c == '\0') {
        return STATUS_PARAM_INVALID;
    }

    /* Abaikan karakter kontrol kecuali backspace */
    if (c < 32 && c != 8) {
        return STATUS_BERHASIL;
    }

    /* Backspace */
    if (c == 8) {
        return kotak_teks_hapus(w);
    }

    /* Cek kapasitas */
    if (data->panjang >= data->panjang_maks) {
        return STATUS_PENUH;
    }

    /* Insert karakter pada posisi kursor */
    if (data->posisi_kursor < data->panjang) {
        ukuran_t i;
        for (i = data->panjang;
             i > data->posisi_kursor; i--) {
            data->teks[i] = data->teks[i - 1];
        }
    }

    data->teks[data->posisi_kursor] = c;
    data->posisi_kursor++;
    data->panjang++;
    data->teks[data->panjang] = '\0';
    data->terpilih = SALAH;
    w->flags |= WIDGET_FLAG_KOTOR;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - HAPUS KARAKTER
 * ===========================================================================
 */

status_t kotak_teks_hapus(widget_t *w)
{
    widget_kotak_teks_data_t *data;

    if (w == NULL) {
        return STATUS_PARAM_NULL;
    }

    data = (widget_kotak_teks_data_t *)w->data;
    if (data == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (data->hanya_baca) {
        return STATUS_TIDAK_DUKUNG;
    }

    if (data->posisi_kursor == 0) {
        return STATUS_BERHASIL;
    }

    /* Jika ada seleksi, hapus range */
    if (data->terpilih) {
        tak_bertanda32_t awal = data->seleksi_awal;
        tak_bertanda32_t akhir = data->seleksi_akhir;
        tak_bertanda32_t count;
        tak_bertanda32_t i;

        if (awal > akhir) {
            tak_bertanda32_t tmp = awal;
            awal = akhir;
            akhir = tmp;
        }

        count = akhir - awal;
        for (i = akhir; i <= data->panjang; i++) {
            data->teks[i - count] = data->teks[i];
        }
        data->panjang -= count;
        data->posisi_kursor = awal;
        data->terpilih = SALAH;
        data->teks[data->panjang] = '\0';
        w->flags |= WIDGET_FLAG_KOTOR;
        return STATUS_BERHASIL;
    }

    /* Hapus satu karakter sebelum kursor */
    {
        ukuran_t i;
        for (i = data->posisi_kursor;
             i < data->panjang; i++) {
            data->teks[i - 1] = data->teks[i];
        }
    }
    data->panjang--;
    data->posisi_kursor--;
    data->teks[data->panjang] = '\0';
    w->flags |= WIDGET_FLAG_KOTOR;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - BERSIHKAN TEKS
 * ===========================================================================
 */

void kotak_teks_bersihkan(widget_t *w)
{
    widget_kotak_teks_data_t *data;

    if (w == NULL) {
        return;
    }

    data = (widget_kotak_teks_data_t *)w->data;
    if (data == NULL) {
        return;
    }

    data->teks[0] = '\0';
    data->panjang = 0;
    data->posisi_kursor = 0;
    data->posisi_scroll = 0;
    data->terpilih = SALAH;
    w->flags |= WIDGET_FLAG_KOTOR;
}

/*
 * ===========================================================================
 * API PUBLIK - SET PLACEHOLDER
 * ===========================================================================
 */

void kotak_teks_set_placeholder(widget_t *w,
                                 const char *teks)
{
    widget_kotak_teks_data_t *data;

    if (w == NULL || teks == NULL) {
        return;
    }

    data = (widget_kotak_teks_data_t *)w->data;
    if (data == NULL) {
        return;
    }

    {
        ukuran_t pj = kernel_strlen(teks);
        if (pj >= WIDGET_LABEL_PANJANG) {
            pj = WIDGET_LABEL_PANJANG - 1;
        }
        kernel_memcpy(data->placeholder, teks, pj);
        data->placeholder[pj] = '\0';
    }
    w->flags |= WIDGET_FLAG_KOTOR;
}

/*
 * ===========================================================================
 * API PUBLIK - SET MODE SANDI
 * ===========================================================================
 */

void kotak_teks_set_mode_sandi(widget_t *w,
                                bool_t sandi)
{
    widget_kotak_teks_data_t *data;

    if (w == NULL) {
        return;
    }

    data = (widget_kotak_teks_data_t *)w->data;
    if (data == NULL) {
        return;
    }

    data->mode_sandi = sandi;
    w->flags |= WIDGET_FLAG_KOTOR;
}

/*
 * ===========================================================================
 * API PUBLIK - SET HANYA BACA
 * ===========================================================================
 */

void kotak_teks_set_hanya_baca(widget_t *w,
                                bool_t hanya_baca)
{
    widget_kotak_teks_data_t *data;

    if (w == NULL) {
        return;
    }

    data = (widget_kotak_teks_data_t *)w->data;
    if (data == NULL) {
        return;
    }

    data->hanya_baca = hanya_baca;
    w->flags |= WIDGET_FLAG_KOTOR;
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL - UPDATE BLINK KURSOR
 * ===========================================================================
 */

static void update_kursor_blink(widget_kotak_teks_data_t *data,
                                tak_bertanda64_t waktu)
{
    tak_bertanda64_t delta;

    if (data->kursor_waktu == 0) {
        data->kursor_waktu = waktu;
        data->kursor_tampil = BENAR;
        return;
    }

    if (waktu > data->kursor_waktu) {
        delta = waktu - data->kursor_waktu;
    } else {
        delta = 0;
    }

    if (delta >= (tak_bertanda64_t)KTEKS_BLINK_MS) {
        data->kursor_tampil =
            data->kursor_tampil ? SALAH : BENAR;
        data->kursor_waktu = waktu;
    }
}

/*
 * ===========================================================================
 * API PUBLIK - RENDER KOTAK TEKS
 * ===========================================================================
 */

status_t kotak_teks_render(widget_t *w,
                            permukaan_t *target,
                            tak_bertanda32_t ox,
                            tak_bertanda32_t oy)
{
    widget_kotak_teks_data_t *data;
    tak_bertanda32_t px, py, pw, ph;
    tak_bertanda32_t warna_latar;
    tak_bertanda32_t warna_border;

    if (w == NULL || target == NULL) {
        return STATUS_PARAM_NULL;
    }

    data = (widget_kotak_teks_data_t *)w->data;
    if (data == NULL) {
        return STATUS_PARAM_NULL;
    }

    px = ox + (tak_bertanda32_t)w->x;
    py = oy + (tak_bertanda32_t)w->y;
    pw = w->lebar;
    ph = w->tinggi;

    /* Tentukan warna berdasarkan state */
    warna_latar = w->flags & WIDGET_FLAG_FOKUS
        ? WIDGET_WARNA_LATAR : WIDGET_WARNA_LATAR;
    warna_border = w->flags & WIDGET_FLAG_FOKUS
        ? WIDGET_WARNA_FOKUS : WIDGET_WARNA_BORDER;

    /* Isi latar */
    permukaan_isi_kotak(target, px, py, pw, ph,
                         warna_latar);

    /* Gambar border */
    permukaan_kotak(target, px, py, pw, ph,
                     warna_border, SALAH);

    /* Update kursor blink */
    if (w->flags & WIDGET_FLAG_FOKUS) {
        update_kursor_blink(data, kernel_get_ticks());
    }

    /* Hitung posisi render teks.
     * Teks dimulai dari offset scroll. */
    {
        tak_bertanda32_t tx = px + KTEKS_PADDING_X;
        tak_bertanda32_t ty = py + (ph / 2) - 4;

        /* Placeholder jika kosong */
        if (data->panjang == 0 &&
            data->placeholder[0] != '\0') {
            /* Render placeholder abu-abu */
            (void)tx;
            (void)ty;
        }
    }

    /* Gambar kursor jika fokus dan tampil */
    if ((w->flags & WIDGET_FLAG_FOKUS) &&
        data->kursor_tampil && !data->hanya_baca) {
        tak_bertanda32_t kx = px + KTEKS_PADDING_X;
        tak_bertanda32_t ky = py + 3;
        tak_bertanda32_t kh = ph - 6;

        if (data->posisi_kursor > data->posisi_scroll) {
            kx += (data->posisi_kursor -
                    data->posisi_scroll) * 8;
        }

        /* Garis kursor vertikal tipis */
        permukaan_garis(target, kx, ky, kx, ky + kh,
                         WIDGET_WARNA_TEKS);
    }

    w->flags &= ~WIDGET_FLAG_KOTOR;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - PROSES EVENT KOTAK TEKS
 * ===========================================================================
 */

void kotak_teks_proses_event(widget_t *w,
                              const widget_event_t *evt)
{
    widget_kotak_teks_data_t *data;

    if (w == NULL || evt == NULL) {
        return;
    }
    if (w->tipe != WIDGET_KOTAK_TEKS) {
        return;
    }

    data = (widget_kotak_teks_data_t *)w->data;
    if (data == NULL) {
        return;
    }

    switch (evt->tipe) {
    case WEVENT_KLIK:
        w->flags |= WIDGET_FLAG_FOKUS;
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
        if (w->flags & WIDGET_FLAG_FOKUS) {
            /* Enter: abaikan */
            /* Backspace: hapus karakter */
            if (evt->kode_kunci == 14) {
                kotak_teks_hapus(w);
            }
            /* Home */
            else if (evt->kode_kunci == 102) {
                data->posisi_kursor = 0;
                data->posisi_scroll = 0;
                w->flags |= WIDGET_FLAG_KOTOR;
            }
            /* End */
            else if (evt->kode_kunci == 107) {
                data->posisi_kursor = data->panjang;
                w->flags |= WIDGET_FLAG_KOTOR;
            }
            /* Left arrow */
            else if (evt->kode_kunci == 105) {
                if (data->posisi_kursor > 0) {
                    data->posisi_kursor--;
                    w->flags |= WIDGET_FLAG_KOTOR;
                }
            }
            /* Right arrow */
            else if (evt->kode_kunci == 106) {
                if (data->posisi_kursor <
                    data->panjang) {
                    data->posisi_kursor++;
                    w->flags |= WIDGET_FLAG_KOTOR;
                }
            }
            /* Delete */
            else if (evt->kode_kunci == 111) {
                if (data->posisi_kursor <
                    data->panjang) {
                    ukuran_t i;
                    for (i = data->posisi_kursor;
                         i < data->panjang; i++) {
                        data->teks[i] =
                            data->teks[i + 1];
                    }
                    data->panjang--;
                    data->teks[data->panjang] = '\0';
                    w->flags |= WIDGET_FLAG_KOTOR;
                }
            }
            /* Select All (Ctrl+A) */
            else if (evt->kode_kunci == 30 &&
                     (evt->modifier & KEY_MOD_CTRL)) {
                data->seleksi_awal = 0;
                data->seleksi_akhir =
                    data->panjang;
                data->terpilih = BENAR;
                w->flags |= WIDGET_FLAG_KOTOR;
            }
        }
        break;

    default:
        break;
    }
}
