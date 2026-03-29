/*
 * PIGURA OS - MENU.C
 * =================
 * Widget menu tarik-turun Pigura OS.
 *
 * Menu menampilkan daftar item yang dapat dipilih.
 * Mendukung separator, submenu, centang, dan scroll.
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
 * FUNGSI INTERNAL - DAPAT POSISI ITEM
 * ===========================================================================
 *
 * Menghitung posisi Y item dalam menu.
 */

static tak_bertanda32_t posisi_item_y(
    const widget_t *w, tak_bertanda32_t indeks)
{
    (void)w;
    return WIDGET_PADDING + 2 +
        indeks * WIDGET_MENU_TINGGI_ITEM;
}

/*
 * ===========================================================================
 * API PUBLIK - INISIALISASI DATA MENU
 * ===========================================================================
 */

status_t menu_init_data(widget_t *w)
{
    widget_menu_data_t *data;

    if (w == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (w->tipe != WIDGET_MENU) {
        return STATUS_PARAM_INVALID;
    }

    data = (widget_menu_data_t *)w->data;
    if (data == NULL) {
        return STATUS_PARAM_NULL;
    }

    kernel_memset(data, 0, sizeof(widget_menu_data_t));
    data->jumlah_item = 0;
    data->indeks_arah = -1;
    data->indeks_terpilih = -1;
    data->terbuka = SALAH;
    data->scroll_offset = 0;
    data->item_tampak = 0;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - TAMBAH ITEM MENU
 * ===========================================================================
 */

status_t menu_tambah_item(widget_t *w,
                           const char *label,
                           tak_bertanda32_t id,
                           bool_t aktif)
{
    widget_menu_data_t *data;

    if (w == NULL || label == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (w->tipe != WIDGET_MENU) {
        return STATUS_PARAM_INVALID;
    }

    data = (widget_menu_data_t *)w->data;
    if (data == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (data->jumlah_item >= WIDGET_MENU_ITEM_MAKS) {
        return STATUS_PENUH;
    }

    {
        widget_menu_item_t *item =
            &data->item[data->jumlah_item];
        ukuran_t pj = kernel_strlen(label);

        kernel_memset(item, 0, sizeof(widget_menu_item_t));

        if (pj >= WIDGET_LABEL_PANJANG) {
            pj = WIDGET_LABEL_PANJANG - 1;
        }
        kernel_memcpy(item->label, label, pj);
        item->label[pj] = '\0';
        item->id = id;
        item->aktif = aktif;
        item->pemisah = SALAH;
        item->punya_submenu = SALAH;
        item->tercentang = SALAH;
        item->data = NULL;
        item->callback = NULL;
    }

    data->jumlah_item++;
    w->flags |= WIDGET_FLAG_KOTOR;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - HAPUS ITEM MENU
 * ===========================================================================
 */

status_t menu_hapus_item(widget_t *w, tak_bertanda32_t id)
{
    widget_menu_data_t *data;
    tak_bertanda32_t i, j;

    if (w == NULL) {
        return STATUS_PARAM_NULL;
    }

    data = (widget_menu_data_t *)w->data;
    if (data == NULL) {
        return STATUS_PARAM_NULL;
    }

    for (i = 0; i < data->jumlah_item; i++) {
        if (data->item[i].id == id) {
            /* Geser item setelahnya ke atas */
            for (j = i; j < data->jumlah_item - 1;
                 j++) {
                kernel_memcpy(&data->item[j],
                    &data->item[j + 1],
                    sizeof(widget_menu_item_t));
            }
            kernel_memset(
                &data->item[data->jumlah_item - 1],
                0, sizeof(widget_menu_item_t));
            data->jumlah_item--;

            if (data->indeks_arah >=
                (tanda32_t)data->jumlah_item) {
                data->indeks_arah = -1;
            }
            w->flags |= WIDGET_FLAG_KOTOR;
            return STATUS_BERHASIL;
        }
    }

    return STATUS_TIDAK_DITEMUKAN;
}

/*
 * ===========================================================================
 * API PUBLIK - SET CALLBACK ITEM
 * ===========================================================================
 */

void menu_set_callback(widget_t *w,
                        tak_bertanda32_t id,
                        widget_event_cb cb,
                        void *udata)
{
    widget_menu_data_t *data;
    tak_bertanda32_t i;

    if (w == NULL) {
        return;
    }

    data = (widget_menu_data_t *)w->data;
    if (data == NULL) {
        return;
    }

    for (i = 0; i < data->jumlah_item; i++) {
        if (data->item[i].id == id) {
            data->item[i].callback = cb;
            data->item[i].data = udata;
            return;
        }
    }
}

/*
 * ===========================================================================
 * API PUBLIK - SET TERBUKA
 * ===========================================================================
 */

void menu_set_terbuka(widget_t *w, bool_t terbuka)
{
    widget_menu_data_t *data;

    if (w == NULL) {
        return;
    }

    data = (widget_menu_data_t *)w->data;
    if (data == NULL) {
        return;
    }

    data->terbuka = terbuka;
    if (terbuka) {
        data->indeks_arah = -1;
        data->scroll_offset = 0;
    }
    w->flags |= WIDGET_FLAG_KOTOR;
}

/*
 * ===========================================================================
 * API PUBLIK - DAPAT STATUS TERBUKA
 * ===========================================================================
 */

bool_t menu_dapat_terbuka(const widget_t *w)
{
    widget_menu_data_t *data;

    if (w == NULL) {
        return SALAH;
    }

    data = (widget_menu_data_t *)w->data;
    if (data == NULL) {
        return SALAH;
    }

    return data->terbuka;
}

/*
 * ===========================================================================
 * API PUBLIK - TAMBAH PEMISAH
 * ===========================================================================
 */

status_t menu_tambah_pemisah(widget_t *w)
{
    widget_menu_data_t *data;

    if (w == NULL) {
        return STATUS_PARAM_NULL;
    }

    data = (widget_menu_data_t *)w->data;
    if (data == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (data->jumlah_item >= WIDGET_MENU_ITEM_MAKS) {
        return STATUS_PENUH;
    }

    {
        widget_menu_item_t *item =
            &data->item[data->jumlah_item];
        kernel_memset(item, 0, sizeof(widget_menu_item_t));
        item->id = 0;
        item->aktif = SALAH;
        item->pemisah = BENAR;
    }

    data->jumlah_item++;
    w->flags |= WIDGET_FLAG_KOTOR;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - RENDER MENU
 * ===========================================================================
 */

status_t menu_render(widget_t *w,
                      permukaan_t *target,
                      tak_bertanda32_t ox,
                      tak_bertanda32_t oy)
{
    widget_menu_data_t *data;
    tak_bertanda32_t px, py, pw, ph;
    tak_bertanda32_t i;
    tak_bertanda32_t tampak;

    if (w == NULL || target == NULL) {
        return STATUS_PARAM_NULL;
    }

    data = (widget_menu_data_t *)w->data;
    if (data == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (!data->terbuka) {
        return STATUS_BERHASIL;
    }

    px = ox + (tak_bertanda32_t)w->x;
    py = oy + (tak_bertanda32_t)w->y;
    pw = w->lebar;
    ph = w->tinggi;

    /* Latar menu */
    permukaan_isi_kotak(target, px, py, pw, ph,
                         WIDGET_WARNA_MENU_LATAR);

    /* Border menu */
    permukaan_kotak(target, px, py, pw, ph,
                     WIDGET_WARNA_BORDER, SALAH);

    /* Hitung berapa item yang bisa ditampilkan */
    tampak = (ph > WIDGET_PADDING + 2)
        ? (ph - WIDGET_PADDING - 2) /
          WIDGET_MENU_TINGGI_ITEM : 0;
    data->item_tampak = tampak;

    /* Gambar setiap item yang terlihat */
    for (i = 0; i < tampak; i++) {
        tak_bertanda32_t item_idx = i +
            data->scroll_offset;
        widget_menu_item_t *item;
        tak_bertanda32_t iy;
        tak_bertanda32_t warna_bg;

        if (item_idx >= data->jumlah_item) {
            break;
        }

        item = &data->item[item_idx];
        iy = py + WIDGET_PADDING + 2 +
            i * WIDGET_MENU_TINGGI_ITEM;

        /* Separator */
        if (item->pemisah) {
            tak_bertanda32_t sy = iy +
                WIDGET_MENU_TINGGI_ITEM / 2;
            permukaan_garis(target,
                px + WIDGET_PADDING, sy,
                px + pw - WIDGET_PADDING, sy,
                WIDGET_WARNA_BORDER);
            continue;
        }

        /* Warna latar item */
        if ((tanda32_t)i == data->indeks_arah) {
            warna_bg = WIDGET_WARNA_MENU_ARAH;
        } else {
            warna_bg = WIDGET_WARNA_MENU_LATAR;
        }

        permukaan_isi_kotak(target,
            px + 1, iy + 1,
            pw - 2, WIDGET_MENU_TINGGI_ITEM - 2,
            warna_bg);

        /* Label item (placeholder render) */
        {
            tak_bertanda32_t tx = px + WIDGET_PADDING + 2;
            (void)tx;
        }

        /* Centang jika aktif */
        if (item->tercentang) {
            permukaan_kotak(target,
                px + 4, iy + 4, 8, 8,
                WIDGET_WARNA_CENTANG, BENAR);
        }

        /* Indikator submenu */
        if (item->punya_submenu) {
            tak_bertanda32_t ax = px + pw - 12;
            tak_bertanda32_t ay = iy +
                WIDGET_MENU_TINGGI_ITEM / 2;
            permukaan_garis(target,
                ax, ay - 3, ax + 3, ay,
                WIDGET_WARNA_TEKS);
            permukaan_garis(target,
                ax + 3, ay - 3, ax + 3, ay + 3,
                WIDGET_WARNA_TEKS);
            permukaan_garis(target,
                ax + 3, ay + 3, ax, ay + 3,
                WIDGET_WARNA_TEKS);
        }
    }

    w->flags &= ~WIDGET_FLAG_KOTOR;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - PROSES EVENT MENU
 * ===========================================================================
 */

void menu_proses_event(widget_t *w,
                        const widget_event_t *evt)
{
    widget_menu_data_t *data;

    if (w == NULL || evt == NULL) {
        return;
    }
    if (w->tipe != WIDGET_MENU) {
        return;
    }

    data = (widget_menu_data_t *)w->data;
    if (data == NULL) {
        return;
    }

    if (!data->terbuka) {
        if (evt->tipe == WEVENT_KLIK) {
            data->terbuka = BENAR;
            data->scroll_offset = 0;
            w->flags |= WIDGET_FLAG_KOTOR;
        }
        return;
    }

    switch (evt->tipe) {
    case WEVENT_ARAH_MASUK:
        {
            tak_bertanda32_t iy = (tak_bertanda32_t)evt->y
                - (tak_bertanda32_t)(w->y + WIDGET_PADDING);
            tak_bertanda32_t idx;

            if (WIDGET_MENU_TINGGI_ITEM == 0) {
                break;
            }

            idx = iy / WIDGET_MENU_TINGGI_ITEM;

            if (idx >= data->jumlah_item) {
                idx = data->jumlah_item - 1;
            }

            data->indeks_arah = (tanda32_t)idx;
            w->flags |= WIDGET_FLAG_KOTOR;
        }
        break;

    case WEVENT_ARAH_KELUAR:
        data->indeks_arah = -1;
        w->flags |= WIDGET_FLAG_KOTOR;
        break;

    case WEVENT_KLIK:
        {
            tak_bertanda32_t iy = (tak_bertanda32_t)evt->y
                - (tak_bertanda32_t)(w->y + WIDGET_PADDING);
            tak_bertanda32_t idx;
            widget_menu_item_t *item;

            if (WIDGET_MENU_TINGGI_ITEM == 0) {
                break;
            }

            idx = iy / WIDGET_MENU_TINGGI_ITEM;

            if (idx >= data->jumlah_item) {
                data->terbuka = SALAH;
                w->flags |= WIDGET_FLAG_KOTOR;
                break;
            }

            item = &data->item[idx];

            if (item->pemisah || !item->aktif) {
                break;
            }

            data->indeks_terpilih = (tanda32_t)idx;

            if (item->tercentang) {
                item->tercentang = SALAH;
            } else if (!item->punya_submenu) {
                item->tercentang = BENAR;
            }

            data->terbuka = SALAH;
            w->flags |= WIDGET_FLAG_KOTOR;

            if (item->callback != NULL) {
                widget_event_t menu_evt;
                kernel_memset(&menu_evt, 0,
                    sizeof(menu_evt));
                menu_evt.tipe = WEVENT_KLIK;
                menu_evt.id_widget = w->id;
                item->callback(&menu_evt, item->data);
            }
        }
        break;

    case WEVENT_KUNCI_TEKAN:
        {
            tak_bertanda32_t n = data->jumlah_item;

            if (evt->kode_kunci == 103) {
                /* Down arrow */
                if (data->indeks_arah < (tanda32_t)n - 1) {
                    data->indeks_arah++;
                    w->flags |= WIDGET_FLAG_KOTOR;
                }
            } else if (evt->kode_kunci == 108) {
                /* Up arrow */
                if (data->indeks_arah > 0) {
                    data->indeks_arah--;
                    w->flags |= WIDGET_FLAG_KOTOR;
                }
            } else if (evt->kode_kunci == 28) {
                /* Enter */
                if (data->indeks_arah >= 0 &&
                    data->indeks_arah < (tanda32_t)n) {
                    widget_menu_item_t *item =
                        &data->item[data->indeks_arah];
                    if (!item->pemisah && item->aktif) {
                        item->tercentang =
                            item->tercentang ? SALAH : BENAR;
                        if (item->callback != NULL) {
                            widget_event_t me;
                            kernel_memset(&me, 0,
                                sizeof(me));
                            me.tipe = WEVENT_KLIK;
                            me.id_widget = w->id;
                            item->callback(&me,
                                item->data);
                        }
                    }
                    data->terbuka = SALAH;
                    w->flags |= WIDGET_FLAG_KOTOR;
                }
            } else if (evt->kode_kunci == 1) {
                /* Escape */
                data->terbuka = SALAH;
                w->flags |= WIDGET_FLAG_KOTOR;
            }
        }
        break;

    case WEVENT_GULIR:
        if (evt->gulir_delta < 0) {
            if (data->scroll_offset > 0) {
                data->scroll_offset--;
                w->flags |= WIDGET_FLAG_KOTOR;
            }
        } else if (evt->gulir_delta > 0) {
            if (data->scroll_offset + data->item_tampak
                < data->jumlah_item) {
                data->scroll_offset++;
                w->flags |= WIDGET_FLAG_KOTOR;
            }
        }
        break;

    default:
        break;
    }
}
