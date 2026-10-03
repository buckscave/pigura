# Arsitektur pigura

## Diagram berlapis

```
┌──────────────────────────────────────────────────────────────┐
│                        Aplikasi                              │
└──────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌──────────────────────────────────────────────────────────────┐
│  (Fase 3: Widget toolkit — label, button, box, ...)         │
└──────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌────────────────────┬─────────────────────┬──────────────────┐
│  Perulangan         │  Gambar AA           │  Font            │
│  (loop peristiwa)  │  (Wu, poly, bezier) │  (bitmap, TTF)   │
└────────────────────┴─────────────────────┴──────────────────┘
                              │
                              ▼
┌──────────────────────────────────────────────────────────────┐
│  Inti: galat, catat, untaian, permukaan, peristiwa           │
└──────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌──────────────────────────────────────────────────────────────┐
│  HAL:  pg_layar_t   (backend framebuffer, per platform)      │
│        pg_masukan_t (backend input, per platform)              │
└──────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────┬─────────────────┬─────────────────────────┐
│  Linux/X11      │  Windows        │  macOS                  │
│  XCreateWindow  │  CreateWindowEx │  NSWindow (objc runtime)│
│  + XShmPutImage │  + DIB Section  │  + CGBitmapContext      │
│  XNextEvent     │  GetMessage    │  CGEventTap             │
└─────────────────┴─────────────────┴─────────────────────────┘
```

## Aliran data Fase 1

1. Aplikasi memanggil `pg_buka_layar()` → backend X11 membuat
   `XCreateSimpleWindow` + `XShmCreateImage` (bila extension
   tersedia) atau fallback `XCreateImage`.
2. Aplikasi memanggil `pg_buka_masukan()` → backend X11 menyiapkan
   event mask di jendela yang sama.
3. Aplikasi memanggil `pg_buat_perulangan()` lalu
   `pg_jalankan_perulangan()`.
4. Thread pompa masukan (background) memanggil
   `pg_masukan_tarik()` yang memanggil `pg_layar_peristiwa_berikutnya()`
   untuk membaca `XNextEvent` dan menerjemahkannya ke
   `pg_peristiwa_t`. Peristiwa didorong ke antrian perulangan.
5. Main thread blocking di `pg_kondisi_tunggu()` sampai ada peristiwa.
6. Peristiwa didispatch ke callback user.
7. Idle callback (bila ada) dipanggil tiap iterasi — di sini app
   menggambar ke back buffer dan mempresentasikannya ke layar.

## Threading

```
┌───────────────────┐      ┌──────────────────────────────┐
│ Thread masukan    │      │ Main thread (perulangan)     │
│ (background)      │      │                               │
│                   │      │                               │
│  while:           │      │  while:                       │
│    pg_masukan_    │ ───► │    pg_kondisi_tunggu()       │
│      tarik()      │      │    cb(peristiwa, ctx)        │
│    push antrian   │      │    idle_cb(ctx)              │
└───────────────────┘      └──────────────────────────────┘
```

- Thread masukan join saat `pg_hancur_perulangan()`.
- Kunci + kondisi POSIX sinkronisasi antrian antar thread.
- `pg_atom_*` untuk flag berhenti (volatile + atomic CAS).

## Model kepemilikan memori

- `pg_layar_t`, `pg_masukan_t`, `pg_perulangan_t`, `pg_font_t`
  dialokasikan oleh fungsi `_buka_` / `_buat_` dan dibebaskan oleh
  `_tutup_` / `_hancur_`.
- `pg_permukaan_t` dari `pg_buat_permukaan()` dimiliki pemanggil.
  `pg_bungkus_permukaan()` **tidak** mengambil alih buffer.
- `pg_layar_kunci()` mengembalikan pointer ke back buffer internal
  layar — pemanggil **tidak boleh** free pointer ini.

## Widget lifecycle (sejak v0.9.0)

Pigura memakai **parent-child tree murni** — tidak ada registry
global. Tiap widget punya daftar anak (`w->anak[]`) yang disinkronkan
otomatis oleh `pg_widget_tambah_anak()`. Kontainer layout
(`pg_kotak`, `pg_toolbar`, `pg_tab`, dll.) memanggil fungsi ini
secara internal saat menambah child.

**3 aturan pakai widget:**

1. **Buat** — `pg_buat_*()` (contoh `pg_buat_tombol`)
2. **Tata** — tambah ke parent via `pg_kotak_tambah()` /
   `pg_widget_tambah_anak()`
3. **Catat & tangani** — root window memanggil
   `pg_widget_catat()` dan `pg_widget_tangani_peristiwa()`
   rekursif ke seluruh tree

**Cleanup:** panggil `pg_widget_hancur_penuh(root)` (atau
`pg_kotak_hancur(root)` bila root adalah pg_kotak). Bila
`root->milik = PG_BENAR`, SEMUA anak dihancurkan rekursif
sebelum root sendiri. Set `milik` lewat `pg_widget_milik(w, BENAR)`
atau `pg_kotak_milik(k, BENAR)`.

```c
pg_kotak_widget_t *root = pg_buat_kotak_widget(
        PG_KOTAK_VERTIKAL, 4);
pg_kotak_milik(root, PG_BENAR);

/* tambah anak; root auto-punya */
pg_kotak_tambah(root, pg_tombol_widget(
        pg_buat_tombol("OK", font)), PG_SALAH);

/* satu hancurkan root + semua anak */
pg_kotak_hancur(root);
```

**Fokus chain (Tab/Shift+Tab):** tidak ada registry global;
fokus traversal berjalan di antara sibling yang dapat-fokus
(`aktif && terlihat && tipe != LABEL/KOTAK`) dalam daftar anak
parent yang sama.

**API yang sudah dihapus di v0.9.0:**
- `pg_widget_daftar()` — tidak diperlukan, otomatis via tambah_anak
- `pg_widget_selesai()` — tidak ada registry untuk dibersihkan
- `pg_widget_t.daftar_*` fields — internal dihapus

## Pelaporan galat

Dua lapisan:

1. **Return value**: setiap fungsi publik mengembalikan `pg_galat`,
   `PG_OK` (=0) saat sukses atau kode `PG_GALAT_*` negatif.
2. **Slot per-thread**: `pg_galat_terakhir()` mengembalikan kode
   galat thread ini; `pg_galat_pesan()` konversi ke string.

Pola:

```c
pg_galat e = pg_buka_layar(&layar, &cfg);
if (e != PG_OK) {
    fprintf(stderr, "buka layar: %s\n",
             pg_galat_pesan(pg_galat_terakhir()));
    return 1;
}
```

## Auto-detect platform

`config.mk` menjalankan `pkg-config --exists x11`. Bila tersedia,
backend `x11` dipilih. Bila tidak, fallback ke `linuxfb` di Linux,
`windows` di Windows, dst. Override manual:

```sh
make PIGURA_BACKEND=x11       # paksa X11
make PIGURA_BACKEND=linuxfb   # paksa /dev/fb0
```

## Roadmap

| Fase | Fokus                                 | Status       |
|------|--------------------------------------|--------------|
| 1    | Fondasi + X11 + AA + demo            | ✅ selesai   |
| 2    | TrueType renderer + AA font          | 🚧 berikut  |
| 3    | Widget toolkit (13 widget)           | 📅 menyusul |
| 4    | Windows + macOS backend              | 📅 menyusul |
| 5    | Wayland                              | 📅 menyusul |
| 6    | Mode embedded (linuxfb + evdev)      | 📅 menyusul |
| 7    | Stabilisasi & rilis publik v1.0      | 📅 menyusul |

## Kompatibilitas balik

v0.x belum stabil — ABI boleh berubah antar minor. Setelah v1.0,
ABI dibekukan dalam major series (SONAME `libpigura.so.MAJOR`).
