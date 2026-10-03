# Gaya kode pigura

pigura mengikuti konvensi penamaan Indonesia natural (predikat +
objek), bukan gaya Inggris (objek + predikat). Subset ketat C89 +
POSIX. Tanpa stub/placeholder/TODO.

## 1. Indentasi & lebar baris

- **Tab**, bukan spasi. Lebar tab 8 kolom.
- **Lebar baris maksimal 80 karakter** (termasuk komentar).
- Baris lanjutan align ke kurung buka fungsi/blok terdalam.

```c
int pg_buka_jendela(pg_jendela_t **out,
                     const pg_jendela_attr_t *attr)
{
        int x, y;
        if (!out) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
        for (y = 0; y < 8; y++) {
                for (x = 0; x < 8; x++) {
                        ...
                }
        }
}
```

## 2. Penamaan

- Prefix `pg_` untuk semua simbol publik (tipe, fungsi, makro,
  enum).
- **Urutan: predikat + objek** (gaya Indonesia):
  - `pg_buka_jendela()` bukan `pg_jendela_buka()`
  - `pg_hancur_permukaan()` bukan `pg_permukaan_hancur()`
  - `pg_isi_permukaan_kotak()` bukan `pg_permukaan_isi_kotak()`
  - `pg_gambar_garis_aa()` — modifier `aa` di akhir
- Tipe: `pg_<nama>_t` (typedef'd struct), mis. `pg_permukaan_t`.
- Getter tanpa `ambil_` prefix kalau jelas konteksnya
  (`pg_permukaan_lebar` bukan `pg_permukaan_ambil_lebar`).
- Setter pakai `setel_` prefix (`pg_permukaan_setel_bg`).
- Makro uppercase dengan underscore: `PG_KOTAK_IRISAN`,
  `PG_MOD_SHIFT`.
- Enum values uppercase dengan prefix enum:
  `PG_PERISTIWA_TOMBOL_TURUN`, `PG_TETIK_KIRI`.

### Istilah yang dipertahankan (sudah jadi kosakata IT)

| Istilah       | Alasan                                  |
|---------------|------------------------------------------|
| `widget`      | Universal di semua GUI toolkit          |
| `framebuffer` | Istilah teknis kernel Linux             |
| `compositor`  | Istilah teknis display server           |
| `RGB`         | Akronim universal                       |
| `mutex`       | Istilah POSIX                            |
| `byte`, `pixel` | Sudah jadi kata serapan               |

## 3. Header prolog

Setiap header publik mulai dengan prolog standar 80 kolom + garis
pemisah:

```c
/* ----------------------------------------------------------------------------------------------
 * pigura/<file>.h - Judul singkat
 * ----------------------------------------------------------------------------------------------
 * Paragraf penjelasan: kontrak, lifecycle, thread-safety, gotchas.
 * Maksimal ~30 baris.
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_<FILE>_H
#define PIGURA_<FILE>_H

#include "pigura/tipe.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ... deklarasi ... */

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_<FILE>_H */
```

## 4. C89 conformance

pigura target strict C89 (`-std=c89 -pedantic`). Aturan:

- **Tidak ada `//` comment** — pakai `/* ... */`.
- **Tidak ada deklarasi variabel di tengah blok** — deklarasi di
  awal blok, sebelum statement pertama.
- **Tidak ada `for (int i = 0; ...)`** — deklarasi `int i;` dulu.
- **Tidak ada `<stdbool.h>`** — pakai `pg_bool` (typedef int).
- **Tidak ada `<stdint.h>`** — pakai `pg_u8`, `pg_u16`, `pg_u32`,
  `pg_u64` dari `tipe.h`.
- **Tidak ada compound literal C99** — pakai konstruktor
  `pg_buat_kotak()` / `pg_buat_titik()`.
- **Tidak ada designated initializer C99** di library code (OK di
  user code).

Extension yang **diizinkan** (ditandai dengan `-Wno-*`):

- `long long` (untuk `pg_u64` di platform 64-bit)
- Variadic macros anonymous (`pg_info(...)`)

Bila ragu apakah fitur C89-clean, build dengan `make BUILD=debug` —
`-pedantic` akan flag yang tidak conform.

## 5. Manajemen memori

- `calloc` lebih disukai daripada `malloc` — memori nol menangkap
  lebih banyak bug.
- Selalu pasangkan `pg_buka_*()` dengan `pg_tutup_*()`. v0.x belum
  ada refcount; kepemilikan eksklusif.
- Cek **setiap** hasil alokasi — kembalikan `PG_GALAT_MEMORI`
  kalau gagal, jangan crash.
- `realloc` untuk array yang tumbuh (lihat `pg_box_add`).

```c
pg_layar_t *l = (pg_layar_t *)calloc(1, sizeof(*l));
if (!l) PG_KEMBALI_GALAT(PG_GALAT_MEMORI);
```

## 6. Penanganan galat

- Fungsi publik return `pg_galat`. 0 = sukses, negatif = galat.
- Pakai `PG_KEMBALI_GALAT(kode)` untuk setel slot + return sekaligus.
- Selalu cek return value dari fungsi internal yang bisa gagal.
- **Tidak boleh** `abort()` / `exit()` dari kode library.
- Dokumentasikan mode kegagalan di doc comment fungsi.

```c
pg_galat pg_buka_layar(pg_layar_t **out, const pg_layar_config_t *cfg)
{
        if (!out) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
        ...
}
```

## 7. Threading

- Pakai `pg_kunci_t` / `pg_kondisi_t` dari `pigura/untaian.h`,
  **jangan** raw `pthread_mutex_t` / `pthread_cond_t`.
- Urutan kunci (lihat `docs/arsitektur.md`):
  `perulangan.kunci` → `masukan.kunci`.
- Hindari `pthread_cancel()` — semua thread cek flag `volatile
  pg_s32 berhenti` dan exit bersih.

## 8. Logging

- `pg_info` untuk pesan startup/shutdown normal.
- `pg_peringatan` untuk issue yang bisa dipulihkan.
- `pg_galat_log` untuk galat yang masih return.
- `pg_debug_log` / `pg_rekam` untuk verbose — compiled out saat
  `PIGURA_CATAT_MATI=1`.
- **Tidak boleh** `printf()` langsung dari library — selalu lewat
  makro catat.

## 9. Anti-pattern yang dilarang

- `//` comment
- Variabel dideklarasikan setelah statement pertama di blok
- `<stdint.h>` di-include di mana pun selain test
- Angka magic tanpa konstanta bernama atau `#define`
- `printf()` langsung (bukan via makro catat)
- Fungsi lebih dari 50 baris body
- Simbol publik tanpa prefix `pg_`
- **STUB / TODO / placeholder** — semua fungsi harus terimplementasi
  penuh atau tidak ada sama sekali

## 10. Build verification

```sh
make clean && make 2>&1 | grep -E "warning:|error:"
# harus kosong
```

Jika ada warning, fix sebelum commit. `-Wall -Wextra -pedantic
-Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings
-Wmissing-prototypes -Wstrict-prototypes
-Wdeclaration-after-statement` menangkap sebagian besar.
