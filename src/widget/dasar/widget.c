/* ----------------------------------------------------------------------------------------------
 * pigura widget: widget.c - kelas dasar widget
 * ----------------------------------------------------------------------------------------------
 * Implementasi lifecycle dasar pg_widget_t: inisialisasi, manajemen
 * permukaan, geometri, visibilitas, dan dispatch catat/peristiwa.
 * Subkelas meng-override lewat vtable; field pg_widget_t sendiri
 * diatur oleh fungsi-fungsi di sini.
 *
 * Konvensi koordinat: kotak.x/y adalah posisi widget relatif terhadap
 * permukaan tujuan (parent atau layar). Saat pg_widget_catat dipanggil,
 * permukaan widget di-blit ke dest pada (kotak.x, kotak.y).
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/widget.h"
#include "pigura/permukaan.h"
#include "pigura/galat.h"
#include "pigura/catat.h"

#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------ konstanta */

/* Ambang batas waktu double-klik dalam milidetik. */
#define PG_WIDGET_DOBEL_KLIK_MS 300

/* Ambang batas gerakan seret dalam piksel (jarak Euclidean kuadrat).
 * Drag baru dipicu bila gerakan > 3 piksel dari posisi klik awal. */
#define PG_WIDGET_SERET_AMBANG_KUAD 9 /* 3*3 = 9 */

/* ------------------------------------------------------------ globals */

/* Widget yang sedang memegang fokus keyboard global. NULL bila
 * tidak ada widget yang punya fokus. */
static pg_widget_t *g_fokus_widget = NULL;
static pg_widget_t *g_capture = NULL; /* mouse capture */

/* ------------------------------------------------------------ parent-child tree */

/* Tambah anak ke daftar anak induk. Idempoten. */
pg_bool pg_widget_tambah_anak(pg_widget_t *induk, pg_widget_t *anak)
{
        pg_widget_t **baru;
        int cap_baru;
        int i;
        if (!induk || !anak) return PG_SALAH;
        /* Idempoten: cek apakah sudah terdaftar. */
        for (i = 0; i < induk->n_anak; i++) {
                if (induk->anak[i] == anak) {
                        anak->induk = induk;
                        return PG_BENAR;
                }
        }
        if (induk->n_anak >= induk->cap_anak) {
                cap_baru = induk->cap_anak ? induk->cap_anak * 2 : 4;
                baru = (pg_widget_t **)realloc(induk->anak,
                        (size_t)cap_baru * sizeof(*baru));
                if (!baru) {
                        pg_set_galat(PG_GALAT_MEMORI,
                                     "daftar anak penuh");
                        return PG_SALAH;
                }
                induk->anak = baru;
                induk->cap_anak = cap_baru;
        }
        induk->anak[induk->n_anak++] = anak;
        anak->induk = induk;
        return PG_BENAR;
}

/* Hapus anak dari daftar anak induk. No-op bila tidak ada. */
void pg_widget_hapus_anak(pg_widget_t *induk, pg_widget_t *anak)
{
        int i, j;
        if (!induk || !anak) return;
        for (i = 0; i < induk->n_anak; i++) {
                if (induk->anak[i] == anak) {
                        for (j = i; j < induk->n_anak - 1; j++)
                                induk->anak[j] = induk->anak[j + 1];
                        induk->n_anak--;
                        break;
                }
        }
        if (anak->induk == induk)
                anak->induk = NULL;
}

/* Setel mode milik. milik=BENAR: pg_widget_hancur akan
 * menghancurkan SEMUA anak secara rekursif. */
void pg_widget_milik(pg_widget_t *w, pg_bool milik)
{
        if (!w) return;
        w->milik = milik;
}

int pg_widget_jumlah_anak(const pg_widget_t *w)
{
        return w ? w->n_anak : 0;
}

pg_widget_t *pg_widget_ambil_anak(const pg_widget_t *w, int idx)
{
        if (!w || idx < 0 || idx >= w->n_anak) return NULL;
        return w->anak[idx];
}

/* Cek apakah widget boleh menerima fokus keyboard: aktif,
 * terlihat, dan bukan label/kotak (kontainer pasif). */
static pg_bool pg_widget_dapat_fokus(const pg_widget_t *w)
{
        if (!w) return PG_SALAH;
        if (!w->aktif || !w->terlihat) return PG_SALAH;
        if (w->tipe == PG_WIDGET_LABEL ||
            w->tipe == PG_WIDGET_KOTAK)
                return PG_SALAH;
        return PG_BENAR;
}

/* Cari widget berikutnya dalam anak induk yang dapat fokus,
 * mulai dari indeks setelah w. Wrap-around bila sampai ujung. */
static pg_widget_t *pg_widget_fokus_berikut_di_induk(pg_widget_t *w)
{
        pg_widget_t *induk;
        int i, k;
        if (!w) return NULL;
        induk = w->induk;
        if (!induk || induk->n_anak <= 0) return NULL;
        k = -1;
        for (i = 0; i < induk->n_anak; i++) {
                if (induk->anak[i] == w) { k = i; break; }
        }
        if (k < 0) return NULL;
        for (i = 1; i <= induk->n_anak; i++) {
                pg_widget_t *kandidat = induk->anak[
                        (k + i) % induk->n_anak];
                if (pg_widget_dapat_fokus(kandidat))
                        return kandidat;
        }
        return NULL;
}

/* Cari widget sebelumnya dalam anak induk yang dapat fokus. */
static pg_widget_t *pg_widget_fokus_sebelum_di_induk(pg_widget_t *w)
{
        pg_widget_t *induk;
        int i, k, idx;
        if (!w) return NULL;
        induk = w->induk;
        if (!induk || induk->n_anak <= 0) return NULL;
        k = -1;
        for (i = 0; i < induk->n_anak; i++) {
                if (induk->anak[i] == w) { k = i; break; }
        }
        if (k < 0) return NULL;
        for (i = 1; i <= induk->n_anak; i++) {
                idx = k - i;
                while (idx < 0) idx += induk->n_anak;
                if (pg_widget_dapat_fokus(induk->anak[idx]))
                        return induk->anak[idx];
        }
        return NULL;
}

/* ---------------------------------------------------------------- kelas */

void pg_widget_init(pg_widget_t *w, pg_widget_tipe_t tipe,
                    const pg_widget_vtable_t *vt)
{
        if (!w) return;
        memset(w, 0, sizeof(*w));
        w->tipe = tipe;
        w->vtable = vt;
        w->terlihat = PG_BENAR;
        w->aktif = PG_BENAR;
        w->kotor = PG_BENAR;
        w->latar = PG_WARNA_PANEL;  /* default opaque — hindari double blend */
        w->milik = PG_SALAH;
        w->klik_terakhir_ms = 0;
        w->klik_tombol_terakhir = PG_TETIK_KOSONG;
        w->sedang_seret = PG_SALAH;
        w->fokus = PG_SALAH;
        /* Catatan: tidak ada registry global. Widget disinkronkan
         * dengan parent-child tree lewat pg_widget_tambah_anak().
         * Aplikasi cukup hancurkan root window — semua anak
         * dihancurkan rekursif bila milik=BENAR. */
}

void pg_widget_hancur(pg_widget_t *w)
{
        int i;
        if (!w) return;
        /* Lepaskan fokus global bila widget ini fokus. */
        if (g_fokus_widget == w) {
                w->fokus = PG_SALAH;
                g_fokus_widget = NULL;
        }
        /* Lepaskan capture mouse bila widget ini sedang di-capture.
         * Tanpa ini, g_capture menjadi dangling pointer dan event
         * mouse berikutnya akan dereference dangling. */
        if (g_capture == w) {
                g_capture = NULL;
        }
        /* Reset state drag agar tidak ada callback saat_seret_selesai
         * tertinggal yang menunjuk ke widget yang sudah dihancurkan. */
        w->sedang_seret = PG_SALAH;
        w->saat_dobelklik = NULL;
        w->saat_seret_mulai = NULL;
        w->saat_seret_gerak = NULL;
        w->saat_seret_selesai = NULL;
        w->saat_fokus = NULL;
        w->saat_blur = NULL;
        w->saat_klik_kanan = NULL;
        /* Hapus diri dari daftar anak induk (bila masih terdaftar).
         * Dilakukan SEBELUM hancurkan anak sendiri agar induk tidak
         * menemukan pointer dangling. */
        if (w->induk) {
                pg_widget_hapus_anak(w->induk, w);
        }
        /* Hancurkan anak-anak bila milik=BENAR. Ini menggantikan
         * logika per-kontainer yang dulu duplikasi di vtable->hancur
         * masing-masing kontainer (pg_kotak, panel_melayang, dll.).
         *
         * Penting: putus dulu anak->induk supaya tidak terjadi
         * rekursi tak hingga saat anak juga memanggil
         * pg_widget_hapus_anak(induk, anak). */
        if (w->milik && w->anak) {
                for (i = 0; i < w->n_anak; i++) {
                        pg_widget_t *a = w->anak[i];
                        if (!a) continue;
                        a->induk = NULL;
                        pg_widget_hancur_penuh(a);
                        w->anak[i] = NULL;
                }
        }
        /* Free daftar anak universal. */
        if (w->anak) {
                free(w->anak);
                w->anak = NULL;
        }
        w->n_anak = 0;
        w->cap_anak = 0;
        /* vtable cleanup (resource internal widget turunan). */
        if (w->vtable && w->vtable->hancur)
                w->vtable->hancur(w);
        if (w->permukaan) {
                pg_hancur_permukaan(w->permukaan);
                w->permukaan = NULL;
        }
        w->vtable = NULL;
        w->impl = NULL;
}

/* ---------------------------------------------------------------- geom */

pg_kotak_t pg_widget_kotak(const pg_widget_t *w)
{
        if (!w) return pg_buat_kotak(0, 0, 0, 0);
        return w->kotak;
}

void pg_widget_setel_kotak(pg_widget_t *w, pg_kotak_t r)
{
        if (!w) return;
        /* JANGAN reallocate permukaan jika ukuran tidak berubah.
         * Ini mencegah free+calloc berulang yang menyebabkan
         * heap corruption. */
        if (w->kotak.w == r.w && w->kotak.h == r.h &&
            w->permukaan) {
                /* Hanya update posisi. */
                w->kotak.x = r.x;
                w->kotak.y = r.y;
                return;
        }
        w->kotak = r;
        if (w->min_w == 0 && w->min_h == 0) {
                w->min_w = r.w;
                w->min_h = r.h;
        }
        if (w->permukaan) {
                pg_hancur_permukaan(w->permukaan);
                w->permukaan = NULL;
        }
        if (r.w > 0 && r.h > 0)
                w->permukaan = pg_buat_permukaan(r.w, r.h);
        if (w->vtable && w->vtable->ubah_ukuran)
                w->vtable->ubah_ukuran(w, r.w, r.h);
        pg_widget_kotor(w);
}

void pg_widget_pindah(pg_widget_t *w, int x, int y)
{
        if (!w) return;
        w->kotak.x = x;
        w->kotak.y = y;
        pg_widget_kotor(w);
}

void pg_widget_ubah_ukuran(pg_widget_t *w, int w_, int h)
{
        if (!w) return;
        w->kotak.w = w_;
        w->kotak.h = h;
        if (w->min_w == 0 && w->min_h == 0) {
                w->min_w = w_;
                w->min_h = h;
        }
        if (w->permukaan) {
                pg_hancur_permukaan(w->permukaan);
                w->permukaan = NULL;
        }
        if (w_ > 0 && h > 0)
                w->permukaan = pg_buat_permukaan(w_, h);
        if (w->vtable && w->vtable->ubah_ukuran)
                w->vtable->ubah_ukuran(w, w_, h);
        pg_widget_kotor(w);
}

void pg_widget_setel_ukuran_min(pg_widget_t *w, int w_, int h)
{
        if (!w) return;
        w->min_w = w_;
        w->min_h = h;
        pg_widget_kotor(w);
}

void pg_widget_setel_radius(pg_widget_t *w, int radius)
{
        if (!w) return;
        if (radius < 0) radius = 0;
        w->radius = radius;
        pg_widget_kotor(w);
}

int pg_widget_radius(const pg_widget_t *w)
{
        return w ? w->radius : 0;
}

/* ------------------------------------------------------------ visibilitas */

/* Cek apakah `anc` adalah ancestor dari `desc` (ancestor = parent,
 * grandparent, dst.). Dipakai untuk melepaskan fokus/capture bila
 * ancestor disembunyikan — descendant yang tidak terlihat tidak
 * boleh memegang fokus global. */
static pg_bool pg_widget_ancestor(const pg_widget_t *desc,
                                    const pg_widget_t *anc)
{
        const pg_widget_t *p;
        if (!desc || !anc) return PG_SALAH;
        p = desc->induk;
        while (p) {
                if (p == anc) return PG_BENAR;
                p = p->induk;
        }
        return PG_SALAH;
}

void pg_widget_sembunyi(pg_widget_t *w)
{
        if (!w) return;
        /* Saat widget disembunyikan, ia tidak boleh lagi memegang
         * capture mouse atau fokus keyboard. Tanpa reset ini:
         *   - g_capture bisa menunjuk ke widget tidak terlihat →
         *     TETIK_NAIK tidak sampai ke widget manapun, drag
         *     "nyangkut" mengikuti mouse.
         *   - g_fokus_widget bisa menunjuk ke widget tidak terlihat
         *     → keyboard input hilang.
         * Memutus capture/fokus di sini membuat hide/show aman
         * dari sisi input routing.
         *
         * Selain itu, bila widget yang SEDANG fokus/di-capture
         * adalah DESCENDANT dari w (anak/cucu), ia juga harus
         * kehilangan fokus — karena descendant tidak terlihat
         * bila parent disembunyikan. */
        if (g_fokus_widget == w ||
            (g_fokus_widget &&
             pg_widget_ancestor(g_fokus_widget, w))) {
                g_fokus_widget->fokus = PG_SALAH;
                g_fokus_widget = NULL;
        }
        if (g_capture == w ||
            (g_capture && pg_widget_ancestor(g_capture, w))) {
                g_capture = NULL;
        }
        /* Reset state drag bila aktif — widget disembunyikan
         * di tengah operasi drag tidak boleh meninggalkan
         * state setengah-jalan. */
        w->sedang_seret = PG_SALAH;
        w->terlihat = PG_SALAH;
        pg_widget_kotor(w);
}

void pg_widget_tampil(pg_widget_t *w)
{
        if (!w) return;
        /* Tampil parent TIDAK otomatis tampil anak. Anak yang
         * sebelumnya disembunyikan eksplisit tetap disembunyikan.
         * Hanya parent yang di-set terlihat. */
        w->terlihat = PG_BENAR;
        pg_widget_kotor(w);
}

pg_bool pg_widget_terlihat(const pg_widget_t *w)
{
        return w ? w->terlihat : PG_SALAH;
}

void pg_widget_aktifkan(pg_widget_t *w)
{
        if (!w) return;
        w->aktif = PG_BENAR;
}

void pg_widget_nonaktifkan(pg_widget_t *w)
{
        if (!w) return;
        w->aktif = PG_SALAH;
}

pg_bool pg_widget_aktif(const pg_widget_t *w)
{
        return w ? w->aktif : PG_SALAH;
}

/* ---------------------------------------------------------------- warna */

void pg_widget_setel_latar(pg_widget_t *w, pg_warna_t c)
{
        if (!w) return;
        w->latar = c;
        pg_widget_kotor(w);
}

pg_warna_t pg_widget_ambil_latar(const pg_widget_t *w)
{
        return w ? w->latar : PG_HITAM;
}

/* ---------------------------------------------------------------- catat */

void pg_widget_catat(pg_widget_t *w, pg_permukaan_t *dest)
{
        if (!w || !dest) return;
        if (!w->terlihat) return;
        if (!w->permukaan) {
                if (w->kotak.w > 0 && w->kotak.h > 0)
                        w->permukaan = pg_buat_permukaan(w->kotak.w,
                                                          w->kotak.h);
        }
        if (!w->permukaan) return;
        if (w->kotor) {
                /* Bila latar transparan (alpha < 255), isi dengan
                 * transparan penuh supaya font/primitif yang di-render
                 * akan di-alpha-blend dengan benar saat blit ke parent.
                 * Bila latar opaque, isi dengan latar (cepat). */
                if (PG_A(w->latar) < 255)
                        pg_isi_permukaan(w->permukaan, PG_TRANSPARAN);
                else
                        pg_isi_permukaan(w->permukaan, w->latar);
                if (w->vtable && w->vtable->catat)
                        w->vtable->catat(w, w->permukaan);
                w->kotor = PG_SALAH;
        }
        /* Blit dengan clip = kotak widget sendiri supaya tidak
         * bleed ke widget lain. */
        pg_blit_potong_permukaan(dest, w->kotak.x, w->kotak.y,
                                   w->permukaan, w->kotak);
}

void pg_widget_catat_popup(pg_widget_t *w, pg_permukaan_t *dest)
{
        if (!w || !dest) return;
        if (!w->terlihat) return;
        if (w->vtable && w->vtable->catat_popup)
                w->vtable->catat_popup(w, dest);
}

void pg_widget_posisi_layar(const pg_widget_t *w, int *ax, int *ay)
{
        int x = 0, y = 0;
        const pg_widget_t *p = w;
        while (p) {
                x += p->kotak.x;
                y += p->kotak.y;
                p = p->induk;
        }
        if (ax) *ax = x;
        if (ay) *ay = y;
}

/* Hit-test vtable-aware: bila vtable menyediakan berisi, pakai
 * itu (mis. untuk popup overlay yang area-nya melampaui kotak
 * widget). Bila tidak, fallback ke cek kotak biasa. */
static pg_bool pg_widget_berisi_v(pg_widget_t *w, pg_titik_t p)
{
        if (w->vtable && w->vtable->berisi)
                return w->vtable->berisi(w, p);
        return pg_widget_berisi(w, p);
}

pg_bool pg_widget_tangani_peristiwa(pg_widget_t *w,
                                     const pg_peristiwa_t *e)
{
        pg_peristiwa_t te;
        pg_bool di_dalam;
        pg_bool dispatch;
        int dx, dy;

        if (!w || !e) return PG_SALAH;
        if (!w->terlihat || !w->aktif) return PG_SALAH;

        /* Routing event mouse berdasarkan posisi & capture state. */
        switch (e->tipe) {
        case PG_PERISTIWA_TETIK_TURUN:
                /* Hanya widget di bawah kursor yang dapat TURUN,
                 * dan jadi capture.
                 *
                 * PENTING: bila g_capture sudah ada (mis. dari klik
                 * sebelumnya yang membuka popup), JANGAN overwrite
                 * ke parent. Biarkan event mengalir ke child via
                 * vtable dispatch. g_capture akan di-overwrite oleh
                 * child yang benar-benar menerima klik. */
                di_dalam = pg_widget_berisi_v(w, e->tetik_pos);
                if (!di_dalam) return PG_SALAH;
                /* Set g_capture hanya bila belum ada capture ATAU
                 * capture saat ini BUKAN descendant dari w. */
                if (!g_capture || !pg_widget_ancestor(g_capture, w)) {
                        g_capture = w;
                }
                dispatch = PG_BENAR;
                break;

        case PG_PERISTIWA_TETIK_NAIK:
                /* Hanya widget yang sedang di-capture yang dapat NAIK,
                 * lalu clear capture.
                 *
                 * PENTING: bila w adalah ancestor dari g_capture
                 * (mis. w=panel_kiri, g_capture=tombol di dalam
                 * panel_kiri), kita tetap dispatch ke w supaya
                 * event mengalir ke child via vtable peristiwa.
                 * Tanpa ini, NAIK tidak pernah sampai ke child
                 * yang di-capture → click/drag nyangkut. */
                if (g_capture == w) {
                        g_capture = NULL;
                        dispatch = PG_BENAR;
                } else if (g_capture &&
                           pg_widget_ancestor(g_capture, w)) {
                        /* w adalah ancestor dari g_capture.
                         * Dispatch ke w — vtable peristiwa akan
                         * terus dispatch ke child sampai ke
                         * g_capture. JANGAN clear g_capture di
                         * sini — biarkan child yang clear saat
                         * dispatch sampai ke sana. */
                        dispatch = PG_BENAR;
                } else {
                        return PG_SALAH;
                }
                break;

        case PG_PERISTIWA_TETIK_GERAK:
                /* Jika ada capture, hanya itu yang dapat GERAK.
                 * Sama seperti NAIK: bila w adalah ancestor dari
                 * g_capture, dispatch supaya event mengalir ke
                 * child. */
                if (g_capture) {
                        if (g_capture == w) {
                                dispatch = PG_BENAR;
                        } else if (pg_widget_ancestor(g_capture, w)) {
                                dispatch = PG_BENAR;
                        } else {
                                return PG_SALAH;
                        }
                } else {
                        if (!pg_widget_berisi_v(w, e->tetik_pos))
                                return PG_SALAH;
                        dispatch = PG_BENAR;
                }
                break;

        case PG_PERISTIWA_TETIK_RODA:
                if (!pg_widget_berisi_v(w, e->tetik_pos))
                        return PG_SALAH;
                dispatch = PG_BENAR;
                break;

        default:
                /* Keyboard events: dispatch ke semua widget aktif. */
                dispatch = PG_BENAR;
                break;
        }

        if (!dispatch) return PG_SALAH;

        /* ----- Deteksi high-level event sebelum vtable dispatch. */

        /* TAB: pindah fokus ke widget berikutnya dalam chain. Shift+TAB
         * mundur ke sebelumnya. Hanya widget yang sedang fokus yang
         * konsumsi TAB; widget lain abaikan. */
        if (e->tipe == PG_PERISTIWA_TOMBOL_TURUN &&
            e->tombol == PG_TOMBOL_TAB && w->fokus) {
                if (e->modifier & PG_MOD_SHIFT)
                        pg_widget_fokus_sebelumnya(w);
                else
                        pg_widget_fokus_berikutnya(w);
                return PG_BENAR; /* consume */
        }

        /* PINTASAN (keyboard shortcut): cek semua pintasan terdaftar.
         * Hanya dipicu bila widget punya fokus. Cocok bila key sama
         * DAN mod sama persis (bitmask penuh). */
        if (e->tipe == PG_PERISTIWA_TOMBOL_TURUN && w->fokus &&
            w->n_pintasan > 0) {
                int pi;
                for (pi = 0; pi < w->n_pintasan; pi++) {
                        if (w->pintasan[pi].key == e->tombol &&
                            w->pintasan[pi].mod == e->modifier) {
                                if (w->pintasan[pi].cb)
                                        w->pintasan[pi].cb(w, w->hl_ctx);
                                return PG_BENAR; /* consume */
                        }
                }
        }

        /* DOBEL KLIK: TETIK_TURUN kiri dalam 300ms dengan tombol
         * yang sama dengan klik sebelumnya. */
        if (e->tipe == PG_PERISTIWA_TETIK_TURUN &&
            e->tetik_tombol == PG_TETIK_KIRI) {
                if (w->klik_tombol_terakhir == PG_TETIK_KIRI &&
                    e->waktu_ms >= w->klik_terakhir_ms &&
                    (e->waktu_ms - w->klik_terakhir_ms)
                      < (pg_u32)PG_WIDGET_DOBEL_KLIK_MS) {
                        if (w->saat_dobelklik)
                                w->saat_dobelklik(w, w->hl_ctx);
                }
                w->klik_terakhir_ms = e->waktu_ms;
                w->klik_tombol_terakhir = e->tetik_tombol;
                /* Catat posisi awal untuk ambang seret. */
                w->sedang_seret = PG_SALAH;
                w->seret_ofs_x = e->tetik_pos.x;
                w->seret_ofs_y = e->tetik_pos.y;
        }

        /* SERET: gerakan mouse dengan capture aktif. */
        if (e->tipe == PG_PERISTIWA_TETIK_GERAK) {
                if (g_capture == w && !w->sedang_seret) {
                        /* Cek apakah gerakan > 3 piksel dari posisi
                         * klik awal (jarak Euclidean kuadrat > 9). */
                        dx = e->tetik_pos.x - w->seret_ofs_x;
                        dy = e->tetik_pos.y - w->seret_ofs_y;
                        if (dx * dx + dy * dy >
                            PG_WIDGET_SERET_AMBANG_KUAD) {
                                w->sedang_seret = PG_BENAR;
                                if (w->saat_seret_mulai)
                                        w->saat_seret_mulai(w,
                                                e->tetik_pos.x,
                                                e->tetik_pos.y,
                                                w->hl_ctx);
                        }
                } else if (w->sedang_seret) {
                        if (w->saat_seret_gerak)
                                w->saat_seret_gerak(w,
                                        e->tetik_pos.x,
                                        e->tetik_pos.y,
                                        w->hl_ctx);
                }
        }

        /* SERET SELESAI: TETIK_NAIK setelah drag aktif. */
        if (e->tipe == PG_PERISTIWA_TETIK_NAIK && w->sedang_seret) {
                if (w->saat_seret_selesai)
                        w->saat_seret_selesai(w,
                                e->tetik_pos.x,
                                e->tetik_pos.y,
                                w->hl_ctx);
                w->sedang_seret = PG_SALAH;
        }

        /* KLIK KANAN (context menu): TETIK_TURUN dengan tombol kanan.
         *
         * Bila widget ini punya saat_klik_kanan terpasang, panggil
         * callback lalu consume (return PG_BENAR).
         *
         * Bila TIDAK punya callback, JANGAN consume — biarkan event
         * diteruskan ke vtable (yang akan dispatch ke child untuk
         * kontainer seperti pg_kotak). Ini supaya klik kanan pada
         * child yang punya saat_klik_kanan tetap terpicu meski
         * dispatch lewat parent. */
        if (e->tipe == PG_PERISTIWA_TETIK_TURUN &&
            e->tetik_tombol == PG_TETIK_KANAN) {
                if (w->saat_klik_kanan) {
                        w->saat_klik_kanan(w,
                                e->tetik_pos.x,
                                e->tetik_pos.y,
                                w->hl_ctx);
                        return PG_BENAR; /* consume */
                }
                /* Tidak ada callback — lanjut ke vtable dispatch. */
        }

        if (!w->vtable || !w->vtable->peristiwa) return PG_SALAH;

        /* Konversi koordinat ke lokal widget (relatif ke kotak.x/y). */
        te = *e;
        te.tetik_pos.x -= w->kotak.x;
        te.tetik_pos.y -= w->kotak.y;
        return w->vtable->peristiwa(w, &te);
}

void pg_widget_kotor(pg_widget_t *w)
{
        pg_widget_t *p;
        if (!w) return;
        w->kotor = PG_BENAR;
        /* Propagate kotor ke SEMUA ancestor.
         *
         * Penyebab utama "event bekerja tapi visual tidak berubah":
         * saat anak (mis. tombol) kotor, permukaan anak di-redraw,
         * TAPI permukaan parent tidak di-redraw — jadi blit anak
         * ke permukaan parent tidak terjadi. Akibatnya tombol yang
         * ditekan tidak kelihatan berubah.
         *
         * Dengan propagate kotor ke ancestor, parent juga akan
         * re-blit anak ke permukaannya di frame berikutnya. */
        p = w->induk;
        while (p) {
                p->kotor = PG_BENAR;
                p = p->induk;
        }
}

pg_bool pg_widget_berisi(const pg_widget_t *w, pg_titik_t p)
{
        if (!w) return PG_SALAH;
        return PG_TITIK_DI_KOTAK(p, w->kotak) ? PG_BENAR : PG_SALAH;
}

/* ------------------------------------------------------------ high-level */

void pg_widget_saat_dobelklik(pg_widget_t *w,
                               void (*cb)(pg_widget_t*, void*),
                               void *ctx)
{
        if (!w) return;
        w->saat_dobelklik = cb;
        w->hl_ctx = ctx;
}

void pg_widget_saat_seret(pg_widget_t *w,
                           void (*mulai)(pg_widget_t*, int, int, void*),
                           void (*gerak)(pg_widget_t*, int, int, void*),
                           void (*selesai)(pg_widget_t*, int, int, void*),
                           void *ctx)
{
        if (!w) return;
        w->saat_seret_mulai   = mulai;
        w->saat_seret_gerak   = gerak;
        w->saat_seret_selesai = selesai;
        w->hl_ctx = ctx;
}

void pg_widget_fokus(pg_widget_t *w)
{
        if (!w) return;
        if (g_fokus_widget == w) return; /* sudah fokus */
        if (g_fokus_widget) {
                g_fokus_widget->fokus = PG_SALAH;
                if (g_fokus_widget->saat_blur)
                        g_fokus_widget->saat_blur(g_fokus_widget,
                                g_fokus_widget->hl_ctx);
        }
        g_fokus_widget = w;
        w->fokus = PG_BENAR;
        if (w->saat_fokus) w->saat_fokus(w, w->hl_ctx);
}

void pg_widget_blur(pg_widget_t *w)
{
        if (!w) return;
        if (w->fokus) {
                w->fokus = PG_SALAH;
                if (w->saat_blur) w->saat_blur(w, w->hl_ctx);
        }
        if (g_fokus_widget == w) g_fokus_widget = NULL;
}

pg_bool pg_widget_punya_fokus(const pg_widget_t *w)
{
        if (!w) return PG_SALAH;
        return w->fokus;
}

void pg_widget_saat_fokus(pg_widget_t *w,
                           void (*cb)(pg_widget_t*, void*),
                           void *ctx)
{
        if (!w) return;
        w->saat_fokus = cb;
        w->saat_blur  = NULL; /* setel eksplisit bila perlu */
        w->hl_ctx = ctx;
}

void pg_widget_saat_klik_kanan(pg_widget_t *w,
    void (*cb)(pg_widget_t*, int x, int y, void*), void *ctx)
{
        if (!w) return;
        w->saat_klik_kanan = cb;
        w->hl_ctx = ctx;
}

void pg_widget_saat_pintasan(pg_widget_t *w, int key, int mod,
    void (*cb)(pg_widget_t*, void*), void *ctx)
{
        if (!w) return;
        w->hl_ctx = ctx;
        if (w->n_pintasan >= PG_PINTASAN_MAKS) return; /* penuh */
        w->pintasan[w->n_pintasan].key = key;
        w->pintasan[w->n_pintasan].mod = mod;
        w->pintasan[w->n_pintasan].cb  = cb;
        w->n_pintasan++;
}

void pg_widget_fokus_berikutnya(pg_widget_t *w)
{
        pg_widget_t *b;
        if (!w) return;
        b = pg_widget_fokus_berikut_di_induk(w);
        if (b) pg_widget_fokus(b);
}

void pg_widget_fokus_sebelumnya(pg_widget_t *w)
{
        pg_widget_t *b;
        if (!w) return;
        b = pg_widget_fokus_sebelum_di_induk(w);
        if (b) pg_widget_fokus(b);
}

void pg_widget_bebas_capture(void)
{
        g_capture = NULL;
}

void pg_widget_hancur_penuh(pg_widget_t *w)
{
        pg_hancur_cb bebas_fn;
        if (!w) return;
        /* Simpan pointer ke deallocator sebelum pg_widget_hancur
         * menge-NULL-kan vtable. */
        bebas_fn = (w->vtable && w->vtable->bebas) ?
                w->vtable->bebas : NULL;
        /* Hancurkan resource internal + anak-anak (bila milik). */
        pg_widget_hancur(w);
        /* Bebaskan struktur turunan via vtable->bebas.
         * Bila tidak ada (widget polos), struktur tidak di-free —
         * pemanggil yang bertanggung jawab. */
        if (bebas_fn) {
                bebas_fn(w);
        }
}


/* (Tidak ada pg_widget_selesai — registry global sudah dihapus.
 * Cleanup dilakukan dengan pg_widget_hancur_penuh pada root widget,
 * yang akan menghancurkan semua anak secara rekursif bila
 * milik=BENAR.) */
