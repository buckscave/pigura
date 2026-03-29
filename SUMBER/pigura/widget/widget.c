/*
 * PIGURA OS - WIDGET.C
 * ===================
 * Modul manajer widget inti Pigura OS.
 *
 * Menyediakan registri widget global, alokasi/widget,
 * hit testing, fokus, dan render orchestrator.
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
 * VARIABEL GLOBAL STATIK
 * ===========================================================================
 */

/* Konteks manajer widget */
static widget_konteks_t g_wctx;

/*
 * ===========================================================================
 * FUNGSI INTERNAL - ALOKASI DATA SPESIFIK
 * ===========================================================================
 *
 * Mengalokasikan data spesifik sesuai tipe widget.
 */

static void *alokasi_data(tak_bertanda32_t tipe)
{
    switch (tipe) {
    case WIDGET_TOMBOL:
        return (void *)kcalloc(1,
            sizeof(widget_tombol_data_t));
    case WIDGET_KOTAK_TEKS:
        return (void *)kcalloc(1,
            sizeof(widget_kotak_teks_data_t));
    case WIDGET_KOTAK_CENTANG:
        return (void *)kcalloc(1,
            sizeof(widget_kotak_centang_data_t));
    case WIDGET_BAR_GULIR:
        return (void *)kcalloc(1,
            sizeof(widget_bar_gulir_data_t));
    case WIDGET_MENU:
        return (void *)kcalloc(1,
            sizeof(widget_menu_data_t));
    case WIDGET_DIALOG:
        return (void *)kcalloc(1,
            sizeof(widget_dialog_data_t));
    default:
        return NULL;
    }
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL - DEALOKASI DATA SPESIFIK
 * ===========================================================================
 */

static void dealokasi_data(widget_t *w)
{
    if (w == NULL || w->data == NULL) {
        return;
    }
    kfree(w->data);
    w->data = NULL;
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL - CEK SLOT KOSONG
 * ===========================================================================
 */

static tak_bertanda32_t cari_slot_kosong(void)
{
    tak_bertanda32_t i;
    for (i = 0; i < WIDGET_MAKS; i++) {
        if (g_wctx.daftar[i] == NULL) {
            return i;
        }
    }
    return WIDGET_MAKS;
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL - RENDER WIDGET SESUAI TIPE
 * ===========================================================================
 */

static status_t render_widget(widget_t *w,
                                permukaan_t *target,
                                tak_bertanda32_t ox,
                                tak_bertanda32_t oy)
{
    if (w == NULL || target == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (!(w->flags & WIDGET_FLAG_TERLIHAT)) {
        return STATUS_BERHASIL;
    }

    switch (w->tipe) {
    case WIDGET_TOMBOL:
        return tombol_render(w, target, ox, oy);
    case WIDGET_KOTAK_TEKS:
        return kotak_teks_render(w, target, ox, oy);
    case WIDGET_KOTAK_CENTANG:
        return kotak_centang_render(w, target, ox, oy);
    case WIDGET_BAR_GULIR:
        return bar_gulir_render(w, target, ox, oy);
    case WIDGET_MENU:
        return menu_render(w, target, ox, oy);
    case WIDGET_DIALOG:
        return dialog_render(w, target, ox, oy);
    default:
        return STATUS_BERHASIL;
    }
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL - PROSES EVENT SESUAI TIPE
 * ===========================================================================
 */

static void proses_event_widget(widget_t *w,
                                 const widget_event_t *evt)
{
    if (w == NULL || evt == NULL) {
        return;
    }

    switch (w->tipe) {
    case WIDGET_TOMBOL:
        tombol_proses_event(w, evt);
        break;
    case WIDGET_KOTAK_TEKS:
        kotak_teks_proses_event(w, evt);
        break;
    case WIDGET_KOTAK_CENTANG:
        kotak_centang_proses_event(w, evt);
        break;
    case WIDGET_BAR_GULIR:
        bar_gulir_proses_event(w, evt);
        break;
    case WIDGET_MENU:
        menu_proses_event(w, evt);
        break;
    case WIDGET_DIALOG:
        dialog_proses_event(w, evt);
        break;
    default:
        break;
    }
}

/*
 * ===========================================================================
 * API PUBLIK - INISIALISASI MANAJER WIDGET
 * ===========================================================================
 */

status_t widget_init(void)
{
    if (g_wctx.diinisialisasi) {
        return STATUS_SUDAH_ADA;
    }

    kernel_memset(&g_wctx, 0, sizeof(widget_konteks_t));
    g_wctx.magic = WIDGET_MAGIC;
    g_wctx.id_counter = 1;
    g_wctx.diinisialisasi = BENAR;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - SHUTDOWN MANAJER WIDGET
 * ===========================================================================
 */

void widget_shutdown(void)
{
    tak_bertanda32_t i;

    if (!g_wctx.diinisialisasi) {
        return;
    }

    for (i = 0; i < WIDGET_MAKS; i++) {
        if (g_wctx.daftar[i] != NULL) {
            dealokasi_data(g_wctx.daftar[i]);
            kfree(g_wctx.daftar[i]);
            g_wctx.daftar[i] = NULL;
        }
    }

    kernel_memset(&g_wctx, 0, sizeof(widget_konteks_t));
}

/*
 * ===========================================================================
 * API PUBLIK - BUAT WIDGET BARU
 * ===========================================================================
 */

widget_t *widget_buat(tak_bertanda32_t tipe,
                      tanda32_t x, tanda32_t y,
                      tak_bertanda32_t lebar,
                      tak_bertanda32_t tinggi)
{
    widget_t *w;
    tak_bertanda32_t slot;
    void *data;

    if (!g_wctx.diinisialisasi) {
        return NULL;
    }

    if (tipe == WIDGET_TIDAK_ADA || tipe >= WIDGET_TIPE_JUMLAH) {
        return NULL;
    }

    slot = cari_slot_kosong();
    if (slot >= WIDGET_MAKS) {
        return NULL;
    }

    w = (widget_t *)kcalloc(1, sizeof(widget_t));
    if (w == NULL) {
        return NULL;
    }

    data = alokasi_data(tipe);
    if (data == NULL) {
        kfree(w);
        return NULL;
    }

    w->magic = WIDGET_MAGIC;
    w->id = g_wctx.id_counter++;
    w->tipe = tipe;
    w->x = x;
    w->y = y;
    w->lebar = lebar > 0 ? lebar : WIDGET_LEBAR_MIN;
    w->tinggi = tinggi > 0 ? tinggi : WIDGET_TINGGI_MIN;
    w->flags = WIDGET_FLAG_TERLIHAT | WIDGET_FLAG_AKTIF;
    w->warna_latar = WIDGET_WARNA_LATAR;
    w->warna_border = WIDGET_WARNA_BORDER;
    w->warna_teks = WIDGET_WARNA_TEKS;
    w->event_cb = NULL;
    w->event_data = NULL;
    w->data = data;
    w->parent = NULL;

    g_wctx.daftar[slot] = w;
    g_wctx.jumlah++;

    return w;
}

/*
 * ===========================================================================
 * API PUBLIK - HAPUS WIDGET
 * ===========================================================================
 */

status_t widget_hapus(widget_t *w)
{
    tak_bertanda32_t i;

    if (w == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (w->magic != WIDGET_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    for (i = 0; i < WIDGET_MAKS; i++) {
        if (g_wctx.daftar[i] == w) {
            dealokasi_data(w);
            w->magic = 0;
            kfree(w);
            g_wctx.daftar[i] = NULL;
            g_wctx.jumlah--;

            if (g_wctx.id_fokus == w->id) {
                g_wctx.id_fokus = 0;
            }
            if (g_wctx.id_tekan == w->id) {
                g_wctx.id_tekan = 0;
            }
            if (g_wctx.id_arah == w->id) {
                g_wctx.id_arah = 0;
            }
            return STATUS_BERHASIL;
        }
    }

    return STATUS_TIDAK_DITEMUKAN;
}

/*
 * ===========================================================================
 * API PUBLIK - HAPUS SEMUA WIDGET
 * ===========================================================================
 */

void widget_hapus_semua(void)
{
    tak_bertanda32_t i;
    for (i = 0; i < WIDGET_MAKS; i++) {
        if (g_wctx.daftar[i] != NULL) {
            dealokasi_data(g_wctx.daftar[i]);
            g_wctx.daftar[i]->magic = 0;
            kfree(g_wctx.daftar[i]);
            g_wctx.daftar[i] = NULL;
        }
    }
    g_wctx.jumlah = 0;
    g_wctx.id_fokus = 0;
    g_wctx.id_tekan = 0;
    g_wctx.id_arah = 0;
}

/*
 * ===========================================================================
 * API PUBLIK - CARI WIDGET BERDASARKAN ID
 * ===========================================================================
 */

widget_t *widget_cari_id(tak_bertanda32_t id)
{
    tak_bertanda32_t i;
    for (i = 0; i < WIDGET_MAKS; i++) {
        if (g_wctx.daftar[i] != NULL &&
            g_wctx.daftar[i]->id == id) {
            return g_wctx.daftar[i];
        }
    }
    return NULL;
}

/*
 * ===========================================================================
 * API PUBLIK - HIT TEST
 * ===========================================================================
 *
 * Mencari widget paling atas (terakhir didaftarkan) yang
 * berada di posisi (x, y). Widget harus terlihat dan aktif.
 */

widget_t *widget_hit_test(tak_bertanda32_t x,
                           tak_bertanda32_t y)
{
    tanda32_t i;

    for (i = (tanda32_t)g_wctx.jumlah - 1; i >= 0; i--) {
        widget_t *w = g_wctx.daftar[i];
        if (w == NULL) {
            continue;
        }
        if (!(w->flags & WIDGET_FLAG_TERLIHAT)) {
            continue;
        }
        if (!(w->flags & WIDGET_FLAG_AKTIF)) {
            continue;
        }
        if (x >= (tak_bertanda32_t)w->x &&
            x < (tak_bertanda32_t)(w->x + (tanda32_t)w->lebar) &&
            y >= (tak_bertanda32_t)w->y &&
            y < (tak_bertanda32_t)(w->y + (tanda32_t)w->tinggi)) {
            return w;
        }
    }
    return NULL;
}

/*
 * ===========================================================================
 * API PUBLIK - SET FOKUS
 * ===========================================================================
 */

status_t widget_set_fokus(widget_t *w)
{
    if (!g_wctx.diinisialisasi) {
        return STATUS_TIDAK_DUKUNG;
    }
    if (w == NULL) {
        g_wctx.id_fokus = 0;
        return STATUS_BERHASIL;
    }
    if (w->magic != WIDGET_MAGIC) {
        return STATUS_PARAM_INVALID;
    }
    g_wctx.id_fokus = w->id;
    w->flags |= WIDGET_FLAG_FOKUS;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - DAPAT WIDGET FOKUS
 * ===========================================================================
 */

widget_t *widget_dapat_fokus(void)
{
    if (g_wctx.id_fokus == 0) {
        return NULL;
    }
    return widget_cari_id(g_wctx.id_fokus);
}

/*
 * ===========================================================================
 * API PUBLIK - KIRIM EVENT KE WIDGET
 * ===========================================================================
 */

status_t widget_kirim_event(widget_t *w,
                             const widget_event_t *evt)
{
    if (w == NULL || evt == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (w->magic != WIDGET_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    g_wctx.total_event++;

    proses_event_widget(w, evt);

    if (w->event_cb != NULL) {
        w->event_cb(evt, w->event_data);
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - RENDER SEMUA WIDGET
 * ===========================================================================
 */

status_t widget_render_semua(permukaan_t *target)
{
    tak_bertanda32_t i;
    status_t st;

    if (!g_wctx.diinisialisasi) {
        return STATUS_TIDAK_DUKUNG;
    }
    if (target == NULL) {
        return STATUS_PARAM_NULL;
    }

    for (i = 0; i < WIDGET_MAKS; i++) {
        if (g_wctx.daftar[i] == NULL) {
            continue;
        }
        st = render_widget(g_wctx.daftar[i], target, 0, 0);
        if (st != STATUS_BERHASIL) {
            continue;
        }
    }

    g_wctx.total_render++;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - RENDER SATU WIDGET
 * ===========================================================================
 */

status_t widget_render(widget_t *w,
                        permukaan_t *target,
                        tak_bertanda32_t ox,
                        tak_bertanda32_t oy)
{
    return render_widget(w, target, ox, oy);
}

/*
 * ===========================================================================
 * API PUBLIK - SET CALLBACK EVENT
 * ===========================================================================
 */

void widget_set_callback(widget_t *w,
                          widget_event_cb cb,
                          void *data)
{
    if (w == NULL) {
        return;
    }
    w->event_cb = cb;
    w->event_data = data;
}

/*
 * ===========================================================================
 * API PUBLIK - CEK VALID
 * ===========================================================================
 */

bool_t widget_valid(const widget_t *w)
{
    if (w == NULL) {
        return SALAH;
    }
    if (w->magic != WIDGET_MAGIC) {
        return SALAH;
    }
    return BENAR;
}

/*
 * ===========================================================================
 * API PUBLIK - SET POSISI
 * ===========================================================================
 */

status_t widget_set_posisi(widget_t *w,
                            tanda32_t x, tanda32_t y)
{
    if (w == NULL) {
        return STATUS_PARAM_NULL;
    }
    w->x = x;
    w->y = y;
    w->flags |= WIDGET_FLAG_KOTOR;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - SET UKURAN
 * ===========================================================================
 */

status_t widget_set_ukuran(widget_t *w,
                            tak_bertanda32_t lebar,
                            tak_bertanda32_t tinggi)
{
    if (w == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (lebar < WIDGET_LEBAR_MIN) {
        lebar = WIDGET_LEBAR_MIN;
    }
    if (tinggi < WIDGET_TINGGI_MIN) {
        tinggi = WIDGET_TINGGI_MIN;
    }
    w->lebar = lebar;
    w->tinggi = tinggi;
    w->flags |= WIDGET_FLAG_KOTOR;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - SET / HAPUS FLAG
 * ===========================================================================
 */

void widget_set_flag(widget_t *w,
                      tak_bertanda32_t flag)
{
    if (w != NULL) {
        w->flags |= flag;
    }
}

void widget_hapus_flag(widget_t *w,
                        tak_bertanda32_t flag)
{
    if (w != NULL) {
        w->flags &= ~flag;
    }
}

/*
 * ===========================================================================
 * API PUBLIK - DAPAT JUMLAH
 * ===========================================================================
 */

tak_bertanda32_t widget_dapat_jumlah(void)
{
    return g_wctx.jumlah;
}

/*
 * ===========================================================================
 * API PUBLIK - DAPAT STATISTIK
 * ===========================================================================
 */

void widget_dapat_statistik(
    tak_bertanda64_t *total_render,
    tak_bertanda64_t *total_event,
    tak_bertanda64_t *total_klik)
{
    if (total_render != NULL) {
        *total_render = g_wctx.total_render;
    }
    if (total_event != NULL) {
        *total_event = g_wctx.total_event;
    }
    if (total_klik != NULL) {
        *total_klik = g_wctx.total_klik;
    }
}
