# pigura

**Pustaka GUI cross-platform murni C89 + POSIX, tanpa dependensi
pihak ketiga.**

`pigura` memberi Anda:

- **Backend layar** portabel: X11 (default Linux desktop), linuxfb
  (embedded/VT), Windows (GDI), macOS (CoreGraphics).
- **Backend masukan** portabel: X11 events (Linux desktop), RawInput
  (Windows), CGEventTap (macOS), evdev (embedded/VT).
- **Loop peristiwa** single-threaded dengan thread pompa masukan
  opsional.
- **Primitif gambar anti-aliasing** setara Cairo/GTK: garis Wu,
  lingkaran, ellipse, poligon, Bezier.
- **Font pluggable**: bitmap 8x8 default (embedded, tanpa dependensi),
  TrueType renderer sendiri di Fase 2.
- **Widget toolkit** lengkap menyusul di Fase 3.

Semua tanpa library eksternal — no FreeType, no SDL, no Cairo, no
HarfBuzz. Hanya C89 + POSIX + header X11/Win32/Cocoa bawaan sistem.

## Mulai cepat

```sh
git clone https://example.invalid/pigura.git
cd pigura
make                 # auto-detect X11, build libpigura.a + demo
./build/pigura_demo  # jalankan di LXDE Anda, tanpa sudo
./build/pigura_demo --uji-sendiri  # CPU-only sanity check
```

Demo membuka jendela X11 800x600, menggambar beberapa primitif AA
(lingkaran, ellipse, garis diagonal, kotak), dan menampilkan tombol
yang bisa diklik. **ESC** atau tombol close untuk keluar.

## Struktur proyek

```
pigura/
├── include/pigura/        Headers publik (master: pigura.h)
│   ├── tipe.h            Tipe data dasar
│   ├── versi.h           Versi
│   ├── galat.h           Pelaporan galat per-thread
│   ├── catat.h           Logging
│   ├── untaian.h         Threading (pembungkus pthread)
│   ├── peristiwa.h       Tipe peristiwa (event)
│   ├── permukaan.h       Permukaan gambar offscreen
│   ├── gambar.h          Primitif AA
│   ├── font.h            Antarmuka font
│   ├── layar.h           Backend framebuffer
│   ├── masukan.h         Backend input
│   ├── perulangan.h      Loop peristiwa
│   └── pigura.h          Master include
├── src/
│   ├── inti/             Core: galat, catat, untaian, permukaan,
│   │                     perulangan, pigura
│   ├── gambar/           Primitif AA + font bitmap default
│   ├── layar/            Backend layar (x11.c di Linux)
│   └── masukan/          Backend masukan (x11.c di Linux)
├── demo/main.c           Demo vertikal-slice
├── docs/
│   ├── arsitektur.md     Diagram lapisan, aliran data, threading
│   └── gaya_kode.md      Konvensi penamaan Indonesia, C89 strict
├── Makefile              Plain POSIX make, auto-detect
├── config.mk            Deteksi platform + flag toolchain
└── README.md            File ini
```

## Backend yang didukung (Fase 1)

| Platform  | Layar             | Masukan         | Status       |
|-----------|-------------------|-----------------|--------------|
| Linux/X11 | XCreateWindow+XShm | XNextEvent+XKB | ✅ penuh    |
| Linux/fb  | /dev/fb0 mmap     | evdev           | 🚧 Fase 6   |
| Windows   | GDI DIB            | RawInput        | 🚧 Fase 4   |
| macOS     | CGBitmapContext   | CGEventTap      | 🚧 Fase 4   |

## API sepintas lalu

```c
#include "pigura/pigura.h"

pigura_init();

pg_layar_t *layar;
pg_buka_layar(&layar, &(pg_layar_config_t){
    .lebar=800, .tinggi=600, .judul="aplikasi saya"
});

pg_masukan_t *masukan;
pg_buka_masukan(&masukan, NULL, layar);

pg_perulangan_t *loop;
pg_buat_perulangan(&loop, &(pg_perulangan_config_t){
    .masukan = masukan,
    .layar   = layar,
    .idle    = my_idle_cb,
    .idle_ctx = &state,
    .pompa_masukan = PG_BENAR
});

pg_jalankan_perulangan(loop, my_peristiwa_cb, &state);

/* ... di idle/peristiwa: */
void *piksel;
int langkah;
pg_layar_kunci(layar, &piksel, &langkah);
pg_permukaan_t *s = pg_bungkus_permukaan(piksel, 800, 600, langkah);
pg_isi_permukaan(s, PG_ABU_GELAP);
pg_gambar_lingkaran_isi_aa(s, 400, 300, 100, PG_BIRU, PG_PUTIH);
pg_layar_buka_kunci(layar);
pg_layar_presentasi(layar);
```

## Lisensi

Belum ditentukan. Akan diisi sebelum rilis publik v1.0.

## Status

v0.1.0 — Fase 1: fondasi, X11 + AA + demo. Lihat `docs/arsitektur.md`
untuk roadmap lengkap.

## Changelog

### v0.14.0 (2026-10-02) — Cleanup: drag real-time, ikon, dock invisible, hint

**Perbaikan kualitas sesuai feedback user.**

**1. Drag real-time (tidak laggy):**
- Tambah callback `pg_panel_saat_drag_gerak(cb, ctx)` — dipanggil
  tiap TETIK_GERAK selama drag, bukan hanya saat NAIK
- Panel sekarang follow mouse real-time
- App bisa pakai untuk feedback visual (mis. hint dock)

**2. Ikon XPM menggantikan teks:**
- Toolbar atas: ikon Baru, Buka, Simpan, Urung, Ulang (5 ikon)
- Toolbox kiri: ikon Select, Rect, Circle, Text, Zoom, Hand (6 ikon)
- Ikon dimuat sekali via `muat_xpm()` di awal, dibebaskan di akhir
- Pakai `pg_tb_setel_icon(tb, id, permukaan)` dari v0.10

**3. Dock invisible saat kosong:**
- `pg_dock_catat()` sekarang return early bila `n_panels == 0`
- Tidak ada latar abu yang mengganggu
- Dock muncul otomatis saat panel masuk, hilang saat semua panel keluar

**4. Hint outline biru saat drag ke dock:**
- Tambah API `pg_dock_catat_hint(d, dest, mx, my, flag_drag)`
- Render outline biru semi-transparan di area dock yang:
  - Flag cocok dengan panel yang di-drag
  - Mouse berada di dalam area dock
- App panggil di catat_all bila `drag_panel_aktif != NULL`

**5. State drag di app_t:**
- `drag_panel_aktif` — pointer panel yang sedang di-drag (NULL bila tidak)
- `drag_flag_aktif` — flag panel yang di-drag (untuk cek dock cocok)
- `drag_mx`, `drag_my` — posisi mouse realtime
- `cb_panel_drag_gerak` update state ini tiap GERAK
- Setelah NAIK, clear state (x=-1, y=-1 dari cb_drag_gerak)

**Cara pakai:**
1. Drag panel (header/grip) — panel follow mouse real-time
2. Arahkan ke dock yang flag cocok — outline biru muncul di dock
3. Drop di dalam dock — panel masuk, dock jadi visible (latar abu)
4. Drag keluar dock — panel jadi floating, dock hilang (invisible)

**Verifikasi:**
- Build bersih (warning unused ikon_xpm.h karena perbesar/perkecil
  tidak dipakai di toolbar — boleh dipakai nanti)
- ASan + LeakSanitizer: startup + exit bersih, no leak

### v0.13.0 (2026-10-02) — Interaksi aktif: drag, tombol, splitter resize

**Hidupkan interaksi yang sebelumnya hanya prototype visual.**

**Drag panel:**
- Drag header panel (FULL/THIN/GRIP mode) untuk pindah
- Saat drag selesai, cek apakah drop di dock yang cocok flag-nya:
  - Ya → masuk ke dock di posisi mouse (insert di paruh kiri/kanan panel)
  - Tidak → floating
- Drag panel dari dock keluar → lepas dari dock, jadi floating

**Tombol header (mode FULL/THIN):**
- **Lipat (collapse)** — panel jadi tipis, hanya header tersisa
- **Semula (restore)** — toggle docked ↔ floating
- **Tutup (close)** — sembunyikan panel (lepas dari dock)

**Splitter resize antar panel:**
- Drag grip dot di antara panel untuk resize
- Clamp otomatis: min 24px per panel, max = sisa ruang
- Distribusi awal: dibagi rata, lalu user bisa resize

**API baru (dock.h):**
- `pg_dock_indeks_insert(d, mx, my)` — cari indeks insert berdasarkan posisi mouse
- `pg_dock_indeks_panel_di(d, mx, my)` — cari panel di posisi mouse

**Perubahan internal dock.c:**
- Tambah field `ukuran_panels[]` (array dinamis) untuk simpan ukuran tiap panel
- `pg_dock_tambah_panel` grow + init array ukuran
- `pg_dock_hapus_panel` shift + reset ukuran
- `pg_dock_tata` pakai array ukuran (bukan dibagi rata lagi)
- Splitter drag: apply delta ke panel kiri/kanan, clamp min 24

**Cara pakai interaksi:**
1. Klik + drag header panel → pindah
2. Drop di atas dock yang flag cocok → masuk dock di posisi mouse
3. Drop di luar dock → floating
4. Klik tombol lipat (kotak dengan garis) → collapse
5. Klik tombol semula (kotak dengan arrow) → toggle docked/floating
6. Klik tombol tutup (X) → sembunyikan
7. Drag grip dot di antara 2 panel → resize

**Yang belum (v0.14):**
- Scrollbar otomatis di dock kalau panel overflow
- Swap panel antar posisi di dock yang sama
- Custom kanvas zoom visual

**Verifikasi:**
- Build bersih, 0 warning
- ASan + LeakSanitizer: startup + exit bersih

### v0.12.0 (2026-10-02) — Dock v4: multi-panel + 3 header mode (rewrite)

**Rewrite total sesuai spec: dock = div HTML, panel = entitas mandiri.**

Sesuai konsep yang user berikan, dock sekarang benar-benar berperan
sebagai wrapper dengan style CSS sendiri, dan panel adalah entitas
mandiri yang bisa floating atau docked.

**Struktur baru:**

- **Panel** (`pg_panel_t` di `pigura/panel.h`):
  - Flag: `TOOLBAR` | `RIBBON` | `PROPERTIES` — menentukan dock tujuan
  - Header mode otomatis berdasarkan konteks parent:
    - **FULL** (floating) — biru, judul + 3 tombol (lipat/semula/tutup)
    - **THIN** (docked di PROPERTIES) — warna body, judul + 3 tombol
    - **GRIP** (docked di TOOLBAR/RIBBON) — box gelap, no judul, no tombol
  - Grip orientasi: vertikal (dock kiri/kanan) → grip di atas;
    horizontal (dock atas/bawah) → grip di kiri
  - Collapsible: tombol lipat → hanya header tersisa

- **Dock** (`pg_dock_t` di `pigura/dock.h`):
  - Flag: `TOOLBAR` | `RIBBON` | `PROPERTIES` — hanya terima panel flag cocok
  - Posisi: `ATAS` | `BAWAH` | `KIRI` | `KANAN`
    - ATAS/BAWAH → layout horizontal, outline bawah/atas
    - KIRI/KANAN → layout vertikal, outline kanan/kiri
  - Multi-panel: array, disusun sesuai orientasi
  - Splitter antar panel (dragable, dengan grip dots)
  - Latar = warna panel
  - Outline = penanda area center

**API baru (breaking changes total):**

```c
/* Panel API */
pg_panel_t *pg_buat_panel(judul, flag, w, h, font);
void pg_panel_setel_anak(p, child);
void pg_panel_setel_header_mode(p, mode);  /* auto by dock */
void pg_panel_setel_collapsed(p, BENAR);
void pg_panel_saat_tutup(p, cb, ctx);
void pg_panel_saat_semula(p, cb, ctx);     /* toggle docked/floating */
void pg_panel_saat_lipat(p, cb, ctx);
void pg_panel_saat_drag_selesai(p, cb, ctx);
void pg_panel_catat(p, dest);
pg_bool pg_panel_tangani(p, e);

/* Dock API */
pg_dock_t *pg_buat_dock(flag, posisi);
void pg_dock_setel_area(d, x, y, w, h);
pg_bool pg_dock_tambah_panel(d, p, idx);   /* cek flag cocok */
void pg_dock_hapus_panel(d, p);
pg_bool pg_dock_punya_panel(d, p);
pg_bool pg_dock_terima_flag(d, flag);
void pg_dock_tata(d);
void pg_dock_catat(d, dest);
pg_bool pg_dock_tangani(d, e);
```

**File baru/dihapus:**

- `include/pigura/panel.h` (baru) — gantikan `panel_melayang.h`
- `src/widget/komponen/panel.c` (baru) — gantikan `panel_melayang.c`
- `include/pigura/dock.h` (rewrite total)
- `src/widget/komponen/dock.c` (rewrite total)
- `include/pigura/panel_melayang.h` → `.bak` (backup)
- `src/widget/komponen/panel_melayang.c` → `.bak` (backup)

**Cara kerja visual (saat aplikasi.c dijalankan):**

1. 4 dock terbentuk: TOOLBAR atas (2 panel), TOOLBAR kiri (1 panel),
   PROPERTIES kanan (2 panel), KANAN vertikal
2. Panel di dock TOOLBAR → header GRIP (grip dot pattern, no judul)
3. Panel di dock PROPERTIES → header THIN (warna body, judul + 3 tombol)
4. Panel kanvas floating → header FULL (biru, judul + 3 tombol)
5. Grip orientasi:
   - Dock atas (horizontal) → grip di kiri panel
   - Dock kiri (vertikal) → grip di atas panel
6. Splitter antar panel di dock yang sama (dragable, dengan dot pattern)
7. Outline dock menghadap area center

**Yang belum diimplementasikan (v0.13):**
- Interaksi drag panel masuk/keluar dock (saat ini hanya visual)
- Tombol header berfungsi (lipat/semula/tutup) — callback sudah tersedia
- Scrollbar otomatis di dock kalau overflow
- Insert panel di posisi mouse (saat ini hanya append)

**Verifikasi:**
- Build bersih, 0 warning (kecuali no previous prototype internal)
- ASan + LeakSanitizer: aplikasi startup + exit bersih, no leak

### v0.11.0 (2026-10-02) — Dock v3: wrapper pattern (rewrite total)

**Refactor dock sesuai konsep awal: dock = div HTML, panel = panel_melayang.**

Sebelum v0.11.0, dock punya `pg_dock_panel` wrapper yang menyimpan
widget generik + title bar mini buatan sendiri. Ini menyebabkan:
- Title bar hilang saat panel di-dock (diganti mini dock sendiri)
- Ukuran panel saat floating = ukuran slot dock (terlalu lebar)
- Memory management kompleks → double-free di edge case
- Konsep tidak konsisten: widget di-dock vs floating punya lifecycle berbeda

**v0.11.0 menghapus `pg_dock_panel` sepenuhnya.** Sekarang:
- Setiap panel adalah `pg_panel_melayang` (entitas tunggal)
- Punya title bar, drag, resize, close BAWAAN
- Saat docked: posisi diatur oleh `pg_dock_tata()`, panel tetap utuh
- Saat floating: panel berdiri sendiri dengan posisi bebas
- Dock hanya simpan pointer ke `pg_panel_melayang` per slot

**API baru (breaking changes):**

| Lama (v0.10) | Baru (v0.11) |
|--------------|--------------|
| `pg_dock_kait_panel(dock, widget, posisi, judul)` | `pg_dock_daftar(dock, pm, slot)` |
| `pg_dock_lepas_panel(dp)` | `pg_dock_lepas(dock, pm)` |
| `pg_dock_panel_saat_lepas(dp, cb, ctx)` | `pg_panel_melayang_saat_drag_selesai(pm, cb, ctx)` |
| `pg_dock_panel_saat_posisi(dp, cb, ctx)` | (dihapus — auto via cb_drag_selesai) |
| `pg_dock_panel_pindah_ke(dock, dp, pos)` | `pg_dock_snap(dock, pm, slot)` |
| `pg_dock_panel_di(dock, posisi)` → `pg_dock_panel_t*` | `pg_dock_panel_di(dock, slot)` → `pg_panel_melayang_t*` |
| `pg_dock_panel_docked(dp)` | `pg_dock_panel_docked(dock, pm)` |
| `pg_dock_panel_posisi(dp)` | `pg_dock_panel_slot(dock, pm)` |
| `pg_dock_panel_widget(dp)` | (tidak perlu — pm adalah panel_melayang) |
| `pg_dock_panel_setel_docked(dp, ...)` | (dihapus — pakai pg_dock_snap/lepas) |
| `struct pg_dock_panel` | (dihapus total) |

**Alur kerja baru:**

```c
/* App buat panel_melayang + isi. */
pg_panel_melayang_t *pm = pg_buat_panel_melayang("Toolbar", w, h, font);
pg_panel_melayang_setel_anak(pm, pg_toolbar_widget(tb));

/* Daftar ke dock (slot tertentu). */
pg_dock_daftar(dock, pm, PG_DOCK_ATAS);
pg_dock_tata(dock);

/* Setel cb drag selesai untuk auto-snap. */
pg_panel_melayang_saat_drag_selesai(pm, cb_drag, app);

/* Di cb_drag: cek snap zona, panggil pg_dock_snap. */
static void cb_drag(pg_panel_melayang_t *pm, int x, int y, void *ctx) {
    pg_dock_posisi_t snap = pg_dock_deteksi_snap(dock, x, y, mask);
    if (snap != PG_DOCK_FLOAT)
        pg_dock_snap(dock, pm, snap);
    else if (pg_dock_panel_docked(dock, pm))
        pg_dock_lepas(dock, pm);
}
```

**Yang dirender app:**
- Menubar, status bar (seperti biasa)
- `pg_dock_catat(dock, s)` — garis pembatas + handle grip splitter
- `pg_panel_melayang_catat(pm, s)` untuk SETIAP panel (docked + floating)

**Yang ditangani app di event handler:**
- `pg_panel_melayang_tangani(pm, e)` untuk SETIAP panel (z-order)
- `pg_dock_tangani_peristiwa(dock, e)` untuk splitter resize

**Verifikasi:**
- Build bersih, 0 warning, 0 error
- ASan + LeakSanitizer: test toggle dock 10x + snap 4 posisi + hancur root — **bersih, no leak, no double-free**
- Title bar BAWAAN panel_melayang sekarang selalu terlihat (saat docked maupun floating)
- Ukuran panel saat floating = ukuran wajar (200x40, bukan 800x600)
- Auto-orientasi toolbar tetap bekerja via cb_drag_selesai

**Yang belum diimplementasikan (v0.12):**
- Custom kanvas dengan zoom visual (transformasi render)

### v0.10.0 (2026-10-02) — Header orientasi dinamis

**Fitur baru: auto-orientasi toolbar berdasarkan sisi dock.**

Toolbar di dock sisi kiri/kanan sekarang otomatis jadi vertikal
(item tersusun atas-bawah). Toolbar di dock atas/bawah tetap
horizontal (item kiri-kanan). Berguna saat panel di-snap dari
sisi atas ke sisi kiri — orientasi menyesuaikan.

**API baru:**

- `pg_tb_setel_vertikal(tb, vertikal)` (di `pigura/toolbar.h`)
  — setel orientasi toolbar saat runtime. Re-layout otomatis.
- `pg_tb_vertikal(tb)` — ambil orientasi saat ini.
- `pg_dock_panel_saat_posisi(dp, cb, ctx)` (di `pigura/dock.h`)
  — setel callback saat posisi panel berubah (mis. snap-back
  dari floating ke sisi asal). App bisa respons dengan
  menyesuaikan orientasi widget.
- `pg_dock_panel_pindah_ke(dock, dp, posisi_baru)` — pindahkan
  panel ke posisi dock baru. Update slot di dock, panggil
  `cb_posisi` bila ada, lalu `pg_dock_tata()`.

**Perubahan internal:**

- `struct pg_dock_panel` tambah field `cb_posisi`, `cb_posisi_ctx`
- Snap-back dari floating sekarang panggil `cb_posisi` setelah
  set `docked=BENAR` dan `pg_dock_tata()`
- Helper internal `pg_dock_panel_update_slot()` untuk update slot
  dock saat pindah posisi

**Verifikasi:**

- Build bersih, 0 warning, 0 error
- ASan + LeakSanitizer: test aggressive tetap bersih
- Demo aplikasi: toolbar atas + toolbox kiri kini terdaftar
  callback posisi; status bar menampilkan "Toolbar posisi:
  vertikal/horizontal" saat panel pindah

**Yang belum diimplementasikan (v0.11):**
- Custom kanvas dengan zoom visual (transformasi render)

### v0.9.2 (2026-10-02) — Fix double-free di kontainer

**Fix kritis: double-free di toolbar, dialog_modal, tab, kolaps,
panel_melayang.**

Di v0.9.0, `pg_widget_tambah_anak()` ditambahkan ke kontainer
agar focus chain dan recursive destroy bekerja. Tapi `vtable->hancur`
setiap kontainer MASIH memanggil `_hancur` untuk child-nya secara
manual. Akibatnya, bila kontainer `milik=BENAR`, child di-free
**dua kali**:
1. Sekali oleh universal anak list di `pg_widget_hancur()`
2. Sekali lagi oleh `vtable->hancur` (mis. `pg_tb_hancur_v`)

**Fix:**
- `pg_tb_hancur_v` — tidak lagi panggil `pg_tombol_hancur()` untuk
  setiap item. Hanya NULL-kan pointer + free array metadata.
- `pg_dm_hancur_v` (dialog_modal) — sama, tidak panggil
  `pg_tombol_hancur()` untuk tombol.
- `pg_buat_toolbar`, `pg_buat_tab`, `pg_buat_kolaps`,
  `pg_buat_panel_melayang`, `pg_buat_dialog_modal` — semua
  sekarang set `milik=BENAR` secara default, sehingga universal
  anak list yang hancurkan child.

**Fix dock.c:**
- **Hapus dead code** — step 4 (Ctrl+click lepas) yang unreachable
  dihapus. Logika Ctrl+click dipindahkan ke step 3 (sebelum mulai
  drag resize).
- **Floating title bar clipping** — title bar yang `y < 0` sekarang
  di-clamp ke 0, mencegah buffer underflow saat `pg_isi_permukaan_kotak`.

**Verifikasi:**
- Build bersih, 0 warning, 0 error
- ASan + LeakSanitizer: test aggressive (toggle dock 10x, buat/hancur
  dialog 5x, hancurkan root dengan berbagai state) — **bersih, no
  leak, no double-free**
- Test dock + window pegang widget yang sama tetap bersih

### v0.9.1 (2026-10-02) — Dock splitter + snap-back

**Fitur dock baru (di atas v0.9.0 yang stabil):**

- **Splitter drag-resize** — drag handle grip di antara panel dock
  untuk resize panel atas/bawah/kiri/kanan. Clamp otomatis ke
  ukuran minimum (24px) dan maksimum (50% area dock).
- **Floating snap-back ke posisi asal** — saat panel floating
  di-drag dan dilepas, sekarang kembali ke posisi dock asal
  (sebelumnya snap ke slot pertama yang tersedia). Posisi asal
  disimpan di `dp->posisi_asal` saat `pg_dock_lepas_panel()`.
- **Handle grip lepas via Ctrl+klik** — Ctrl+klik di splitter
  handle = lepas panel ke floating (alternatif double-klik).

**Perubahan API dock (di `pigura/dock.h`):**

- `struct pg_dock` tambah field `split_aktif`, `split_mulai_pos`,
  `split_mulai_ukuran` (internal, untuk state drag splitter)
- `struct pg_dock_panel` tambah field `posisi_asal` (public readable)
- `pg_dock_lepas_panel()` sekarang simpan `posisi_asal` bila belum
  pernah diset (untuk snap-back nanti)

**Perubahan internal dock.c:**

- Tambah `pg_split_id_t` enum (NONE/ATAS/BAWAH/KIRI/KANAN)
- Tambah `pg_dock_split_hit()` — hit-test titik di area splitter
- Tambah `pg_dock_split_terapkan()` — apply delta mouse ke ukuran sisi
- `pg_dock_tangani_peristiwa()` rewrite:
  1. State splitter drag aktif: konsumsi semua event sampai NAIK
  2. Floating drag: snap back ke posisi_asal saat NAIK
  3. Splitter mulai drag: klik di grip
  4. Ctrl+klik grip: lepas panel ke floating
  5. Dispatch ke panel docked (reverse z-order)
  6. Dispatch ke floating widget body
- `pg_dock_catat()`: grip highlight saat split_aktif != NONE
- `pg_dock_tata()`: hanya pakai ukuran sisi bila panel docked
  (sebelumnya pakai walaupun tidak docked → layout kosong)

**Verifikasi:**
- Build bersih, 0 warning, 0 error
- ASan + LeakSanitizer: skenario dock + window pegang widget yang
  sama tetap bersih (tidak ada regression dari v0.9.0)

**Yang belum diimplementasikan (v0.10):**
- Header orientasi dinamis (toolbar di dock kiri jadi vertikal)
- Custom kanvas dengan zoom visual (transformasi render)

### v0.9.0 (2026-10-02) — Parent-child tree murni

**Refactor API widget: hapus registry global, pakai parent-child
tree universal seperti Qt/FLTK.**

Sumber utama bug `double free` dan `malloc corrupted` di v0.8.x
terdeteksi: registry global (`g_widget_daftar`) bertabrakan dengan
parent-child tree — widget bisa dipegang dua pemilik sekaligus
(parent + registry), dan saat keduanya dibebaskan terjadi
double-free. Sudah diperbaiki dengan **menghapus registry global
sepenuhnya**.

**API yang dihapus:**
- `pg_widget_daftar()` — tidak diperlukan
- `pg_widget_selesai()` — tidak ada registry untuk dibersihkan
- `g_widget_daftar`, `g_widget_jumlah`, `g_widget_kapasitas`

**API baru (universal, di `pigura/widget.h`):**
- `pg_widget_tambah_anak(induk, anak)` — tambah ke daftar anak
- `pg_widget_hapus_anak(induk, anak)` — lepas dari induk
- `pg_widget_milik(w, BENAR)` — setel mode kepemilikan
- `pg_widget_jumlah_anak(w)` / `pg_widget_ambil_anak(w, i)` —
  akses anak untuk iterasi
- Field `w->anak[]`, `n_anak`, `cap_anak`, `milik` di struct
  `pg_widget_t` (universal, semua widget punya)

**Perubahan internal:**
- `pg_widget_hancur()` sekarang rekursif hancurkan anak bila
  `milik=BENAR` (sebelumnya duplikasi per-kontainer)
- `pg_kotak_hancur_v()` tidak hancurkan anak — universal yang
  lakukan; vtable hanya membersihkan resource internal
- `pg_widget_fokus_berikutnya()` / `sebelumnya()` pakai tree
  traversal (sibling di `w->induk->anak[]`), bukan registry
- `pg_kotak_milik(k, BENAR)` panggil `pg_widget_milik(base, BENAR)`
- `pg_kotak_tambah_lengkap()` panggil `pg_widget_tambah_anak()`
- `pg_kotak_hapus()` panggil `pg_widget_hapus_anak()`
- `pg_kotak_bersih()` pakai `pg_widget_hancur_penuh()` (milik) /
  `pg_widget_hapus_anak()` (sewa)
- `panel_melayang`, `toolbar`, `tab`, `kolaps`, `dialog_modal` —
  semua pakai `pg_widget_tambah_anak()` bukan set manual
  `child->induk = parent`
- `pigura_selesai()` tidak lagi panggil `pg_widget_selesai()`

**3 aturan pakai widget (sebelumnya 4):**

1. Buat widget: `pg_buat_tombol()`, `pg_buat_label()`, ...
2. Tata: `pg_kotak_tambah()` / `pg_widget_tambah_anak()`
3. Hancurkan root: `pg_kotak_hancur(root)` (otomatis hancurkan
   semua anak bila `milik=BENAR`)

**Verifikasi:**
- Build bersih, 0 warning, 0 error
- AddressSanitizer + LeakSanitizer: skenario dock + window pegang
  widget yang sama (skenario yang dulu crash) sekarang **tidak ada
  leak, tidak ada double-free**

**Dokumentasi yang diperbarui:**
- `docs/arsitektur.md` — section "Widget lifecycle" baru
- `docs/komposisi_widget.md` — section "Parent-child tree" baru
- `include/pigura/widget.h` — header comment + API declaration

### v0.2.1 (2026-09-29) — Baseline Stabil

**Toolkit audit lengkap, build bersih, 0 warning, 0 galat.**

#### Audit & Cleanup
- Hapus 10 file duplikat Inggris (box.c, button.c, checkbox.c, dll)
  + 12 header duplikat (error.h, event.h, fb.h, dll) — dead code
- Update config.mk: include 10 widget docking di Makefile
  (dock, snap, tab, panel_melayang, kolaps, splitter, toolbar,
  status_bar, tooltip, dialog_modal)
- Rename `pg_dok` → `pg_dock` (typo fix, konsisten)
- Tambah field `bebas` (deallocator) ke struct `pg_widget_vtable`

#### Memory & Error Handling
- Fix x11.c: NULL check untuk calloc resize back buffer (crash OOM)
- Tambah `pg_widget_selesai()`: cleanup `g_widget_daftar` global
  registry saat exit (dipanggil di `pigura_selesai()`)
- Verifikasi NULL check untuk semua calloc/malloc/realloc di 52 file

#### Lifecycle API
- `pg_kotak_milik(k, BENAR)`: parent milik anak, hancur rekursif
- `pg_widget_hancur_penuh(w)`: hancurkan widget + struktur turunan
- `pg_kotak_bersih(k)`: reset kontainer tanpa hancurkan parent
- `pg_widget_bebas_capture()`: lepas g_capture manual
- `pg_widget_ancestor(desc, anc)`: cek ancestor chain

#### Widget Fixes
- `pg_kotak_berisi_v`: cek popup area child (dropdown popup keluar
  dari kotak row) — fix klik di popup dropdown tidak terdispatch
- `pg_tombol_setel_batas(t, SALAH)`: hilangkan border tombol untuk
  flat look di toolbar/group
- `pg_menubar_terbuka()` + `pg_menubar_tutup()`: tutup menu saat
  klik di luar menubar
- Ancestor-aware `pg_widget_sembunyi()`: descendant yang fokus/
  di-capture otomatis kehilangan state saat parent disembunyikan
- Auto-sizing `pg_buat_label()`: compute min_w/min_h dari teks+font

#### Event System
- `PG_JENDELA_PERMINTAAN_GAMBAR`: Expose event dari X11 (fix
  blank-hitam saat restore dari minimize)
- Propagate `pg_widget_kotor()` ke semua ancestor (fix visual
  tidak update saat event)
- `pg_kotak_peristiwa_v`: hapus double-subtraction coordinate

#### Backend
- X11: fix key conversion (huruf kecil vs kapital)
- X11: lazy realloc back buffer saat resize (fix crash maximize)
- X11: disable XShm (race condition)
- Semua backend: 5 layar (X11, Win, macOS, LinuxFB, Wayland stub)
  + 5 masukan (X11, evdev, Win, macOS, Wayland stub)

### v0.2.0 (2026-09-26) — Initial Release

- 15 widget dasar (label, tombol, kotak, isian_teks, gambar, geser,
  cek, radio, kemajuan, gulir, daftar, menubar, dropdown,
  jendela_widget, widget base)
- TTF renderer sendiri (cmap, kerning, composite, gamma, adaptive
  Bezier)
- AA primitif (Wu line, ellipse distance field, Bezier)
- Codec (BMP, PNG, JPEG via stb_image)
- UTF-8 text support
- Timer API
- File dialog (zenity/kdialog)
- Theme system (light/dark)

### v0.2.1 (2026-09-29) — Baseline Stabil

**Toolkit audit lengkap, build bersih, 0 warning, 0 galat.**

#### Audit & Cleanup
- Hapus 10 file duplikat Inggris + 12 header duplikat — dead code
- Update config.mk: include 10 widget docking di Makefile
- Rename pg_dok -> pg_dock (typo fix)
- Tambah field bebas (deallocator) ke struct pg_widget_vtable

#### Memory & Error Handling
- Fix x11.c: NULL check untuk calloc resize back buffer
- ~~Tambah pg_widget_selesai(): cleanup g_widget_daftar global~~
  (dihapus di v0.9.0 — lihat di bawah)
- Verifikasi NULL check untuk semua calloc/malloc/realloc

#### Lifecycle API
- pg_kotak_milik(k, BENAR): parent milik anak, hancur rekursif
- pg_widget_hancur_penuh(w): hancurkan widget + struktur turunan
- pg_kotak_bersih(k): reset kontainer tanpa hancurkan parent
- pg_widget_bebas_capture(): lepas g_capture manual
- pg_widget_ancestor(desc, anc): cek ancestor chain

#### Widget Fixes
- pg_kotak_berisi_v: cek popup area child (fix dropdown)
- pg_tombol_setel_batas(t, SALAH): hilangkan border tombol
- pg_menubar_terbuka() + pg_menubar_tutup(): tutup menu saat klik luar
- Ancestor-aware pg_widget_sembunyi(): descendant kehilangan fokus
- Auto-sizing pg_buat_label(): compute min_w/min_h dari teks+font

#### Event System
- PG_JENDELA_PERMINTAAN_GAMBAR: Expose event (fix blank restore)
- Propagate pg_widget_kotor() ke ancestor (fix visual update)
- pg_kotak_peristiwa_v: hapus double-subtraction coordinate

#### Backend
- X11: fix key conversion (huruf kecil vs kapital)
- X11: lazy realloc back buffer (fix crash maximize)
- X11: disable XShm (race condition)
- 5 layar + 5 masukan backend

### v0.2.0 (2026-09-26) — Initial Release

- 15 widget dasar + TTF renderer + AA primitif + codec + UTF-8
- Timer API + File dialog + Theme system
