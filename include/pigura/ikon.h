/* -----------------------------------------------------------------------------
 * pigura/ikon.h - Codepoint ikon Material Design (subset)
 * -----------------------------------------------------------------------------
 * Subset dari MaterialIcons-Regular.ttf (Google Material Design Icons).
 * Hanya berisi codepoint ikon esensial untuk toolkit desktop UI.
 *
 * Penggunaan:
 *   pg_font_t *f = pg_buat_font_ttf("MaterialIcons-Subset.ttf", 18);
 *   pg_font_gambar_teks(f, PG_IKON_CLOSE, s, x, y, warna);
 *
 * Catatan: codepoint di Private Use Area (U+E000-F8FF).
 * Encoding UTF-8: 3 byte per ikon (U+E000-EFFF -> EE.. 80.. 80..).
 * --------------------------------------------------------------------------- */
#ifndef PIGURA_IKON_H
#define PIGURA_IKON_H

#ifdef __cplusplus
extern "C" {
#endif

/* String UTF-8 byte literal. Pakai langsung sebagai argumen teks. */
/* C89-compatible: tiap byte di-escape sebagai \xHH. */

/* ===== NAVIGASI ===== */
#define PG_IKON_ARROW_BACK              "\xEE\x97\x84"  /* Kembali (U+E5C4) */
#define PG_IKON_ARROW_FORWARD           "\xEE\x97\x88"  /* Maju (U+E5C8) */
#define PG_IKON_ARROW_UPWARD            "\xEE\x97\x98"  /* Naik (U+E5D8) */
#define PG_IKON_ARROW_DOWNWARD          "\xEE\x97\x9B"  /* Turun (U+E5DB) */
#define PG_IKON_CHEVRON_LEFT            "\xEE\x97\x8B"  /* Panah kiri kecil (U+E5CB) */
#define PG_IKON_CHEVRON_RIGHT           "\xEE\x97\x8C"  /* Panah kanan kecil (U+E5CC) */
#define PG_IKON_EXPAND_MORE             "\xEE\x97\x8F"  /* Expand bawah (U+E5CF) */
#define PG_IKON_EXPAND_LESS             "\xEE\x97\x8E"  /* Expand atas (U+E5CE) */
#define PG_IKON_FIRST_PAGE              "\xEE\x97\x9C"  /* Halaman pertama (U+E5DC) */
#define PG_IKON_LAST_PAGE               "\xEE\x97\x9D"  /* Halaman terakhir (U+E5DD) */

/* ===== AKSI ===== */
#define PG_IKON_ADD                     "\xEE\x85\x85"  /* Tambah (U+E145) */
#define PG_IKON_REMOVE                  "\xEE\x85\x9B"  /* Hapus (U+E15B) */
#define PG_IKON_CLOSE                   "\xEE\x97\x8D"  /* Tutup (U+E5CD) */
#define PG_IKON_CHECK                   "\xEE\x97\x8A"  /* Centang (U+E5CA) */
#define PG_IKON_CLEAR                   "\xEE\x85\x8C"  /* Bersihkan (U+E14C) */
#define PG_IKON_DELETE                  "\xEE\xA1\xB2"  /* Hapus item (U+E872) */
#define PG_IKON_EDIT                    "\xEE\x8F\x89"  /* Edit (U+E3C9) */
#define PG_IKON_DONE                    "\xEE\xA1\xB6"  /* Selesai (U+E876) */
#define PG_IKON_REFRESH                 "\xEE\x97\x95"  /* Muat ulang (U+E5D5) */
#define PG_IKON_SEARCH                  "\xEE\xA2\xB6"  /* Cari (U+E8B6) */
#define PG_IKON_MORE_VERT               "\xEE\x97\x94"  /* Menu vertikal (U+E5D4) */
#define PG_IKON_MORE_HORIZ              "\xEE\x97\x93"  /* Menu horizontal (U+E5D3) */
#define PG_IKON_BOOKMARK                "\xEE\xA1\xA6"  /* Markah (U+E866) */
#define PG_IKON_SHARE                   "\xEE\xA0\x8D"  /* Bagikan (U+E80D) */
#define PG_IKON_PRINT                   "\xEE\xA2\xAD"  /* Cetak (U+E8AD) */

/* ===== EDIT ===== */
#define PG_IKON_UNDO                    "\xEE\x85\xA6"  /* Urungkan (U+E166) */
#define PG_IKON_REDO                    "\xEE\x85\x9A"  /* Ulangi (U+E15A) */
#define PG_IKON_CONTENT_CUT             "\xEE\x85\x8E"  /* Potong (U+E14E) */
#define PG_IKON_CONTENT_COPY            "\xEE\x85\x8D"  /* Salin (U+E14D) */
#define PG_IKON_CONTENT_PASTE           "\xEE\x85\x8F"  /* Tempel (U+E14F) */

/* ===== FILE ===== */
#define PG_IKON_FOLDER                  "\xEE\x8B\x87"  /* Folder (U+E2C7) */
#define PG_IKON_FOLDER_OPEN             "\xEE\x8B\x88"  /* Buka folder (U+E2C8) */
#define PG_IKON_CREATE_NEW_FOLDER       "\xEE\x8B\x8C"  /* Buat folder baru (U+E2CC) */
#define PG_IKON_FILE_COPY               "\xEE\x85\xB3"  /* Salin file (U+E173) */
#define PG_IKON_FILE_OPEN               "\xEE\xAB\xB3"  /* Buka file (U+EAF3) */
#define PG_IKON_SAVE                    "\xEE\x85\xA1"  /* Simpan (U+E161) */
#define PG_IKON_DOWNLOAD                "\xEF\x82\x90"  /* Unduh (U+F090) */
#define PG_IKON_UPLOAD                  "\xEF\x82\x9B"  /* Unggah (U+F09B) */

/* ===== WINDOW ===== */
#define PG_IKON_MINIMIZE                "\xEE\xA4\xB1"  /* Minimalkan window (U+E931) */
#define PG_IKON_MAXIMIZE                "\xEE\xA4\xB0"  /* Maksimalkan window (U+E930) */
#define PG_IKON_FULLSCREEN              "\xEE\x97\x90"  /* Layar penuh (U+E5D0) */
#define PG_IKON_FULLSCREEN_EXIT         "\xEE\x97\x91"  /* Keluar layar penuh (U+E5D1) */

/* ===== VIEW ===== */
#define PG_IKON_VISIBILITY              "\xEE\xA3\xB4"  /* Tampakkan (U+E8F4) */
#define PG_IKON_VISIBILITY_OFF          "\xEE\xA3\xB5"  /* Sembunyikan (U+E8F5) */
#define PG_IKON_ZOOM_IN                 "\xEE\xA3\xBF"  /* Perbesar (U+E8FF) */
#define PG_IKON_ZOOM_OUT                "\xEE\xA4\x80"  /* Perkecil (U+E900) */
#define PG_IKON_LIST                    "\xEE\xA2\x96"  /* Tampilan daftar (U+E896) */
#define PG_IKON_GRID_VIEW               "\xEE\xA6\xB0"  /* Tampilan grid (U+E9B0) */
#define PG_IKON_VIEW_LIST               "\xEE\xA3\xAF"  /* Tampilan list (U+E8EF) */

/* ===== SETTING ===== */
#define PG_IKON_SETTINGS                "\xEE\xA2\xB8"  /* Pengaturan (U+E8B8) */
#define PG_IKON_TUNE                    "\xEE\x90\xA9"  /* Filter lanjutan (U+E429) */
#define PG_IKON_FILTER_LIST             "\xEE\x85\x92"  /* Daftar filter (U+E152) */
#define PG_IKON_SORT                    "\xEE\x85\xA4"  /* Urutkan (U+E164) */

/* ===== STATUS ===== */
#define PG_IKON_INFO                    "\xEE\xA2\x8E"  /* Informasi (U+E88E) */
#define PG_IKON_WARNING                 "\xEE\x80\x82"  /* Peringatan (U+E002) */
#define PG_IKON_ERROR                   "\xEE\x80\x80"  /* Error (U+E000) */
#define PG_IKON_HELP                    "\xEE\xA2\x87"  /* Bantuan (U+E887) */
#define PG_IKON_CANCEL                  "\xEE\x97\x89"  /* Batal (U+E5C9) */
#define PG_IKON_PRIORITY_HIGH           "\xEE\x99\x85"  /* Penting (U+E645) */

/* ===== MENU ===== */
#define PG_IKON_MENU                    "\xEE\x97\x92"  /* Menu hamburger (U+E5D2) */
#define PG_IKON_MENU_OPEN               "\xEE\xA6\xBD"  /* Menu buka (U+E9BD) */
#define PG_IKON_APPS                    "\xEE\x97\x83"  /* Grid apps (U+E5C3) */
#define PG_IKON_EXIT_TO_APP             "\xEE\xA1\xB9"  /* Keluar aplikasi (U+E879) */
#define PG_IKON_HOME                    "\xEE\xA2\x8A"  /* Beranda (U+E88A) */
#define PG_IKON_PERSON                  "\xEE\x9F\xBD"  /* Pengguna (U+E7FD) */

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_IKON_H */
