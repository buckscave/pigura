# Komposisi Widget Pigura

Pigura toolkit menyediakan 14 widget master yang bisa dikombinasi
untuk membuat komponen UI kompleks **tanpa coding widget baru**.
Pola umum: susun widget master di dalam `pg_kotak_widget_t`
(container horizontal/vertikal) lalu setel `kembang`/`padding`
untuk distribusi ruang.

Daftar widget master:

| # | Widget                  | Header                       |
|---|-------------------------|------------------------------|
| 1 | menubar                 | `pigura/menubar.h`           |
| 2 | label                   | `pigura/label.h`             |
| 3 | tombol                  | `pigura/tombol.h`            |
| 4 | geser (slider)          | `pigura/geser.h`             |
| 5 | cek (checkbox)          | `pigura/cek.h`               |
| 6 | radio                   | `pigura/radio.h`             |
| 7 | kemajuan (progress)     | `pigura/kemajuan.h`          |
| 8 | dropdown (combobox)     | `pigura/dropdown.h`          |
| 9 | daftar (list view)      | `pigura/daftar.h`            |
| 10| gulir (scroll view)     | `pigura/gulir.h`             |
| 11| jendela-widget          | `pigura/jendela_widget.h`    |
| 12| gambar-widget           | `pigura/gambar_widget.h`      |
| 13| isian_teks (line edit)   | `pigura/isian_teks.h`        |
| 14| kotak (box container)   | `pigura/kotak.h`             |

Tambahan high-level event (Fase 4):
- `pg_widget_saat_dobelklik` — double klik kiri
- `pg_widget_saat_seret` — drag kiri (mulai/gerak/selesai)
- `pg_widget_saat_fokus` / `pg_widget_blur` — keyboard focus
- `pg_widget_saat_klik_kanan` — context menu (klik kanan)
- `pg_widget_saat_pintasan` — keyboard shortcut (Ctrl+A, dll.)

---

## Pola dasar

### 1. VBox berisi label + tombol

```c
pg_kotak_widget_t *box = pg_buat_kotak_widget(
        PG_KOTAK_VERTIKAL, 4);
pg_widget_setel_kotak(pg_kotak_widget(box),
        pg_buat_kotak(10, 10, 200, 100));

pg_label_t *judul = pg_buat_label("Halo", PG_PUTIH, font);
pg_kotak_tambah(box, pg_label_widget(judul), PG_SALAH);

pg_tombol_t *ok = pg_buat_tombol("OK", font);
pg_kotak_tambah(box, pg_tombol_widget(ok), PG_SALAH);
```

### 2. HBox dengan label + input yang expand

```c
pg_kotak_widget_t *row = pg_buat_kotak_widget(
        PG_KOTAK_HORIZONTAL, 4);
pg_label_t *lbl = pg_buat_label("Nama:", PG_PUTIH, font);
pg_kotak_tambah(row, pg_label_widget(lbl), PG_SALAH);
pg_isian_teks_t *le = pg_buat_isian_teks("", 32, font);
pg_kotak_tambah(row, pg_isian_teks_widget(le),
                PG_BENAR); /* expand */
```

---

## Contoh lengkap: Panel Properti

Sebuah panel properti dengan header, tombol close, dan konten:

```
+---------------------------+
| Properti        Layer > [x]|
+---------------------------+
| Opacity: [====O====] 50%  |
| Mode:    [Normal    v]    |
| [X] Visible               |
| [ ] Locked                |
+---------------------------+
```

Kode:

```c
pg_kotak_widget_t *panel = pg_buat_kotak_widget(
        PG_KOTAK_VERTIKAL, 2);
pg_kotak_setel_padding(panel, 4);
pg_widget_setel_kotak(pg_kotak_widget(panel),
        pg_buat_kotak(0, 0, 260, 160));

/* Header bar: judul (expand) + tombol close. */
pg_kotak_widget_t *header = pg_buat_kotak_widget(
        PG_KOTAK_HORIZONTAL, 4);
pg_label_t *judul = pg_buat_label("Properti",
        PG_PUTIH, font);
pg_kotak_tambah(header, pg_label_widget(judul),
                PG_BENAR);              /* expand */
pg_tombol_t *close = pg_buat_tombol("x", font);
pg_kotak_tambah(header, pg_tombol_widget(close),
                PG_SALAH);
pg_kotak_tambah(panel, pg_kotak_widget(header),
                PG_SALAH);

/* Row opacity: label + slider + label nilai. */
pg_kotak_widget_t *row_op = pg_buat_kotak_widget(
        PG_KOTAK_HORIZONTAL, 4);
pg_label_t *l_op = pg_buat_label("Opacity:",
        PG_PUTIH, font);
pg_kotak_tambah(row_op, pg_label_widget(l_op),
                PG_SALAH);
pg_geser_t *slider = pg_buat_geser(0, 100, 50);
pg_kotak_tambah(row_op, pg_geser_widget(slider),
                PG_BENAR);              /* expand */
pg_label_t *l_val = pg_buat_label("50",
        PG_KUNING, font);
pg_kotak_tambah(row_op, pg_label_widget(l_val),
                PG_SALAH);
pg_kotak_tambah(panel, pg_kotak_widget(row_op),
                PG_SALAH);

/* Row mode: label + dropdown. */
pg_kotak_widget_t *row_md = pg_buat_kotak_widget(
        PG_KOTAK_HORIZONTAL, 4);
pg_label_t *l_md = pg_buat_label("Mode:",
        PG_PUTIH, font);
pg_kotak_tambah(row_md, pg_label_widget(l_md),
                PG_SALAH);
pg_dropdown_t *dd = pg_buat_dropdown(NULL, font);
pg_dropdown_tambah_item(dd, "Normal");
pg_dropdown_tambah_item(dd, "Multiply");
pg_dropdown_tambah_item(dd, "Screen");
pg_kotak_tambah(row_md, pg_dropdown_widget(dd),
                PG_BENAR);
pg_kotak_tambah(panel, pg_kotak_widget(row_md),
                PG_SALAH);

/* Checkboxes ( VBox ). */
pg_cek_t *vis = pg_buat_cek("Visible", font);
pg_cek_setel_dicek(vis, PG_BENAR);
pg_kotak_tambah(panel, pg_cek_widget(vis),
                PG_SALAH);
pg_cek_t *lock = pg_buat_cek("Locked", font);
pg_kotak_tambah(panel, pg_cek_widget(lock),
                PG_SALAH);
```

---

## Contoh: Dialog sederhana

Dialog modal di tengah layar, dengan title bar (jendela-widget)
berisi body VBox (pesan + HBox tombol OK/Cancel).

```
+--- Confirm ----------------------+
| Delete file 'foo.txt'?           |
|                                  |
|         [ OK ]   [ Cancel ]      |
+----------------------------------+
```

Kode:

```c
/* Body VBox. */
pg_kotak_widget_t *body = pg_buat_kotak_widget(
        PG_KOTAK_VERTIKAL, 8);
pg_kotak_setel_padding(body, 8);
pg_label_t *pesan = pg_buat_label(
        "Delete file 'foo.txt'?", PG_PUTIH, font);
pg_kotak_tambah(body, pg_label_widget(pesan),
                PG_SALAH);

/* HBox tombol. */
pg_kotak_widget_t *tombols = pg_buat_kotak_widget(
        PG_KOTAK_HORIZONTAL, 8);
pg_tombol_t *ok = pg_buat_tombol("OK", font);
pg_tombol_t *cancel = pg_buat_tombol("Cancel", font);
pg_kotak_tambah(tombols, pg_tombol_widget(ok),
                PG_BENAR);   /* expand agar tombol menempel kanan */
pg_kotak_tambah(tombols, pg_tombol_widget(cancel),
                PG_SALAH);
pg_kotak_tambah(body, pg_kotak_widget(tombols),
                PG_SALAH);

/* Jendela-widget sebagai container modal. */
pg_jendela_widget_t *win = pg_buat_jendela_widget(
        "Confirm", 280, 120, font);
pg_widget_setel_kotak(pg_jendela_widget_widget(win),
        pg_buat_kotak(260, 240, 280, 120));
pg_jendela_widget_setel_anak(win,
        pg_kotak_widget(body));
pg_jendela_widget_saatutup(win, cb_dialog_tutup, ctx);
```

---

## Contoh: Toolbar horizontal

Toolbar berisi 3 tombol yang terdistribusi merata, dengan
expand + fill.

```c
pg_kotak_widget_t *tb = pg_buat_kotak_widget(
        PG_KOTAK_HORIZONTAL, 4);
pg_widget_setel_kotak(pg_kotak_widget(tb),
        pg_buat_kotak(0, 0, 320, 32));

/* Tiap tombol min_w=100, expand=BENAR, fill=BENAR
 * supaya terbagi rata 3: (320 - 2*4) / 3 ~= 104. */
pg_tombol_t *t1 = pg_buat_tombol("New", font);
pg_widget_setel_ukuran_min(pg_tombol_widget(t1),
        100, 28);
pg_kotak_tambah_lengkap(tb,
        pg_tombol_widget(t1),
        PG_BENAR, PG_BENAR, 0);

pg_tombol_t *t2 = pg_buat_tombol("Open", font);
pg_widget_setel_ukuran_min(pg_tombol_widget(t2),
        100, 28);
pg_kotak_tambah_lengkap(tb,
        pg_tombol_widget(t2),
        PG_BENAR, PG_BENAR, 0);

pg_tombol_t *t3 = pg_buat_tombol("Save", font);
pg_widget_setel_ukuran_min(pg_tombol_widget(t3),
        100, 28);
pg_kotak_tambah_lengkap(tb,
        pg_tombol_widget(t3),
        PG_BENAR, PG_BENAR, 0);
```

---

## Contoh: Form login

VBox berisi baris-baris (tiap baris HBox label + input).
Submit button di bawah.

```c
pg_kotak_widget_t *form = pg_buat_kotak_widget(
        PG_KOTAK_VERTIKAL, 6);

/* Row username. */
pg_kotak_widget_t *r1 = pg_buat_kotak_widget(
        PG_KOTAK_HORIZONTAL, 4);
pg_label_t *l_u = pg_buat_label("User:", PG_PUTIH,
        font);
pg_kotak_tambah(r1, pg_label_widget(l_u), PG_SALAH);
pg_isian_teks_t *in_u = pg_buat_isian_teks("", 32,
        font);
pg_kotak_tambah(r1, pg_isian_teks_widget(in_u),
                PG_BENAR);
pg_kotak_tambah(form, pg_kotak_widget(r1), PG_SALAH);

/* Row password. */
pg_kotak_widget_t *r2 = pg_buat_kotak_widget(
        PG_KOTAK_HORIZONTAL, 4);
pg_label_t *l_p = pg_buat_label("Pass:", PG_PUTIH,
        font);
pg_kotak_tambah(r2, pg_label_widget(l_p), PG_SALAH);
pg_isian_teks_t *in_p = pg_buat_isian_teks("", 32,
        font);
pg_kotak_tambah(r2, pg_isian_teks_widget(in_p),
                PG_BENAR);
pg_kotak_tambah(form, pg_kotak_widget(r2), PG_SALAH);

/* Submit. */
pg_tombol_t *submit = pg_buat_tombol("Login", font);
pg_kotak_tambah(form, pg_tombol_widget(submit),
                PG_SALAH);
pg_tombol_saatklik(submit, cb_login, ctx);
```

---

## Contoh: Context menu + shortcut

Pasang context menu pada tombol dan Ctrl+A pada line edit.
Toolkit hanya ROUTE event; app yang menentukan makna.

```c
/* Context menu: klik kanan tombol -> popup menu. */
pg_widget_saat_klik_kanan(
        pg_tombol_widget(tombol),
        cb_context_menu, ctx);

/* Shortcut: Ctrl+A pada line edit -> select all. */
pg_widget_saat_pintasan(
        pg_isian_teks_widget(le),
        'a',               /* key (lowercase) */
        PG_MOD_CTRL,       /* mod bitmask */
        cb_select_all, ctx);

/* Shortcut tambahan untuk editor teks. */
pg_widget_saat_pintasan(w, 'c', PG_MOD_CTRL,
        cb_copy, ctx);
pg_widget_saat_pintasan(w, 'v', PG_MOD_CTRL,
        cb_paste, ctx);
pg_widget_saat_pintasan(w, 'x', PG_MOD_CTRL,
        cb_cut, ctx);
pg_widget_saat_pintasan(w, 'z', PG_MOD_CTRL,
        cb_undo, ctx);
pg_widget_saat_pintasan(w, 's', PG_MOD_CTRL,
        cb_save, ctx);
```

Callback:

```c
static void cb_context_menu(pg_widget_t *w, int x, int y,
                             void *ctx)
{
        /* Tampilkan menu popup di (x, y). App yang
         * implement menu sendiri (bisa pakai pg_menubar). */
        pg_demo_setel_status(ctx, "context menu!");
}

static void cb_select_all(pg_widget_t *w, void *ctx)
{
        /* App memilih semua teks di widget fokus.
         * (Line edit pigura belum punya selection state,
         * jadi ini sekadar demo routing shortcut.) */
        pg_demo_setel_status(ctx, "Ctrl+A: select all");
}
```

---

## Contoh: Tampilan gambar dari file BMP

```c
/* Muat BMP dari file. */
pg_permukaan_t *s = pg_codec_muat("logo.bmp");
if (s) {
        pg_gambar_widget_setel_piksel(
                w_gambar,
                pg_permukaan_lebar(s),
                pg_permukaan_tinggi(s),
                (const pg_warna_t *)pg_permukaan_piksel(s));
        pg_hancur_permukaan(s);
}
```

`pg_codec_decode(data, len)` melakukan hal yang sama dari buffer
memory; berguna bila gambar di-bundle di binary (mis. sebagai
`static const unsigned char logo_bmp[] = { 0x42, 0x4D, ... };`).

Format yang didukung v0.2: BMP 24/32-bit uncompressed.
PNG/JPEG akan ditolak dengan `pg_galat_terakhir() == PG_GALAT_TIDAKADA`
sampai integrasi stb_image di sesi berikutnya.

---

## Tips

### Parent-child tree (universal, sejak v0.9.0)

Sebelum v0.9.0, pigura memakai registry global (`pg_widget_daftar`)
untuk track semua widget yang pernah dibuat. Ini menyebabkan banyak
bug double-free karena widget bisa dipegang baik oleh parent-child
tree maupun oleh registry — dua sumber ownership yang saling
bertabrakan.

Sejak v0.9.0, **registry global dihapus**. Setiap widget punya
daftar anak universal di `w->anak[]` (di struct `pg_widget_t`),
disinkronkan otomatis oleh `pg_widget_tambah_anak(induk, anak)`.
Kontainer layout (`pg_kotak`, `pg_toolbar`, `pg_tab`, `pg_kolaps`,
`pg_panel_melayang`, `pg_dialog_modal`) memanggil fungsi ini
secara internal saat menambah child — pemanggil API tidak perlu
panggil eksplisit.

Fokus chain (Tab/Shift+Tab) sekarang juga pakai tree traversal:
iterasi sibling di `w->induk->anak[]` yang dapat-fokus, bukan
registry global.

API publik universal (di `pigura/widget.h`):

```c
pg_bool pg_widget_tambah_anak(pg_widget_t *induk,
                              pg_widget_t *anak);
void    pg_widget_hapus_anak(pg_widget_t *induk,
                             pg_widget_t *anak);
void    pg_widget_milik(pg_widget_t *w, pg_bool milik);
int     pg_widget_jumlah_anak(const pg_widget_t *w);
pg_widget_t *pg_widget_ambil_anak(const pg_widget_t *w, int idx);
```

### Lifecycle: milik vs sewa

Pigura toolkit membedakan dua mode kepemilikan anak pada kontainer:

**Mode milik** (`pg_kotak_milik(k, PG_BENAR)`) — parent PEMILIK
anak. `pg_kotak_hancur(k)` akan menghancurkan SEMUA anak secara
rekursif (via `pg_widget_hancur_penuh` yang memanggil
`vtable->bebas`). Pattern ini direkomendasikan untuk komposisi
deklaratif (builder pattern) — app tidak perlu track setiap
widget yang dibuat, cukup hancurkan root.

```c
pg_kotak_widget_t *root = pg_buat_kotak_widget(
        PG_KOTAK_VERTIKAL, 4);
pg_kotak_milik(root, PG_BENAR);
pg_kotak_tambah(root, pg_label_widget(
        pg_buat_label("Judul", PG_PUTIH, font)), PG_SALAH);
pg_kotak_tambah(root, pg_tombol_widget(
        pg_buat_tombol("OK", font)), PG_SALAH);
pg_kotak_tambah(root, pg_tombol_widget(
        pg_buat_tombol("Batal", font)), PG_SALAH);
/* Satu panggilan hancurkan root + 3 anak + permukaan masing-masing. */
pg_kotak_hancur(root);
```

**Mode sewa** (default, `milik=SALAH`) — parent hanya REFERENSI
anak. `pg_kotak_hancur(k)` tidak membebaskan anak. Berguna bila
anak di-share antar beberapa parent, atau app perlu reuse widget
setelah parent dihancurkan.

### Reset reuse kontainer

`pg_kotak_bersih(k)` menghapus SEMUA anak tanpa hancurkan parent.
Berguna untuk UI dinamis (mis. ganti halaman, reload konfigurasi):

```c
pg_kotak_widget_t *panel = pg_buat_kotak_widget(
        PG_KOTAK_VERTIKAL, 4);
pg_kotak_milik(panel, PG_BENAR);

/* Isi halaman 1. */
pg_kotak_tambah(panel, pg_label_widget(
        pg_buat_label("Halaman 1", c, font)), PG_SALAH);
/* ... */

/* Bersihkan — semua anak halaman 1 hancur. */
pg_kotak_bersih(panel);

/* Isi halaman 2 ke panel yang sama. */
pg_kotak_tambah(panel, pg_label_widget(
        pg_buat_label("Halaman 2", c, font)), PG_SALAH);
/* ... */

/* Akhirnya: */
pg_kotak_hancur(panel);
```

### Auto-sizing widget

Beberapa widget menghitung `min_w`/`min_h` natural dari konten
saat dibuat, sehingga layout manager bisa auto-arrange tanpa
`pg_widget_setel_kotak` eksplisit:

- **`pg_buat_label(teks, warna, font)`** — ukuran dari
  `pg_font_lebar_teks + pg_font_tinggi`.

Widget lain (tombol, isian_teks) saat ini tidak auto-size; setel
`min_w`/`min_h` eksplisit via `pg_widget_setel_ukuran_min` sebelum
tambah ke kotak.

### Visibilitas & fokus

- **Sembunyi parent → descendant kehilangan fokus.** Saat parent
  disembunyikan via `pg_widget_sembunyi`, semua descendant (anak,
  cucu) yang sedang fokus atau di-capture otomatis kehilangan
  state tersebut. Ini mencegah dangling pointer ke widget yang
  tidak terlihat.
- **Tampil parent TIDAK otomatis tampil anak.** Anak yang
  disembunyikan eksplisit tetap disembunyikan. App yang perlu
  tampilkan seluruh subtree: iterasi dan panggil `pg_widget_tampil`
  pada setiap anak.
- **Hancur widget sedang fokus → fokus dilepas otomatis.** Tidak
  perlu manual `pg_widget_blur` sebelum hancurkan.

### Ekspansi ruang

- `kembang=PG_BENAR` (alias `expand`) memberi child bagian dari
  ruang tersisa. Tanpa expand, child tetap di ukuran `min_w`/
  `min_h` natural. Set `min_w`/`min_h` lewat
  `pg_widget_setel_ukuran_min(w, w, h)` sebelum tambah ke kotak.

### Fill vs expand

- `expand=PG_BENAR, fill=PG_BENAR` -> child mengisi slot penuh.
- `expand=PG_BENAR, fill=PG_SALAH` -> child tetap `min` tapi
  posisi tengah dalam slot.

### Popup overlay

Dropdown dan menubar memakai `pg_widget_catat_popup(w, dest)`
untuk render popup di luar kotak widget. App wajib memanggil
`pg_widget_catat_popup` untuk SEMUA widget (yang tidak punya
popup akan no-op) SETELAH `pg_widget_catat` normal.

### Fokus chain

`pg_widget_fokus(w)` men-set widget aktif untuk keyboard input.
TAB pindah ke widget berikutnya dalam registry global (urutan
pembuatan). Shift+TAB mundur.

### Context menu

Klik kanan mouse otomatis di-route ke
`saat_klik_kanan(w, x, y, ctx)` bila terpasang; vtable widget
TIDAK menerima event klik kanan (di-consume widget.c). Gunakan
koordinat global `x, y` untuk menempatkan popup menu.

### Shortcut pintasan

Hanya dipicu bila widget punya fokus. Cocok bila
`key == e->tombol` DAN `mod == e->modifier` persis (bitmask
penuh, bukan subset). Maks 16 pintasan per widget.
