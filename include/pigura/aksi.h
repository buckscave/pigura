/* ----------------------------------------------------------------------------------------------
 * pigura/peristiwa.h - Sistem peristiwa (event)
 * ----------------------------------------------------------------------------------------------
 * Semua peristiwa input dan lifecycle dilebur jadi satu tagged union.
 * Antrian peristiwa bounded dan thread-safe; produsen (thread input)
 * push aksi, konsumen (loop utama) menariknya.
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_PERISTIWA_H
#define PIGURA_PERISTIWA_H

#include "pigura/tipe.h"

#ifdef __cplusplus
extern "C" {
#endif

enum pg_aksi_tipe {
        PG_AKSI_KOSONG      = 0,
        PG_AKSI_TOMBOL_TURUN  = 1,
        PG_AKSI_TOMBOL_NAIK   = 2,
        PG_AKSI_TETIKUS_GERAK   = 3,
        PG_AKSI_TETIKUS_TEKAN   = 4,
        PG_AKSI_TETIKUS_LEPAS    = 5,
        PG_AKSI_TETIKUS_GULIR    = 6,
        PG_AKSI_JENDELA      = 7,
        PG_AKSI_PENGGUNA      = 8,
        PG_AKSI_KELUAR        = 9,
        PG_AKSI_DOBEL_KLIK    = 10,
        PG_AKSI_SERET_MULAI  = 11,
        PG_AKSI_SERET_GERAK  = 12,
        PG_AKSI_SERET_SELESAI = 13,
        PG_AKSI_FOKUS        = 14,
        PG_AKSI_BLUR         = 15
};

/* Kode tombol gaya USB HID (subset, cukup untuk ASCII + tombol umum). */
enum pg_tombol {
        PG_TOMBOL_KOSONG = 0,
        PG_TOMBOL_ESCAPE = 1,
        PG_TOMBOL_ENTER = 2,
        PG_TOMBOL_TAB = 3,
        PG_TOMBOL_BACKSPACE = 4,
        PG_TOMBOL_INSERT = 5,
        PG_TOMBOL_DELETE = 6,
        PG_TOMBOL_HOME = 7,
        PG_TOMBOL_END = 8,
        PG_TOMBOL_PAGEUP = 9,
        PG_TOMBOL_PAGEDOWN = 10,
        PG_TOMBOL_KIRI = 11,
        PG_TOMBOL_KANAN = 12,
        PG_TOMBOL_ATAS = 13,
        PG_TOMBOL_BAWAH = 14,
        PG_TOMBOL_SPASI = 32,
        /* ASCII 32..126 dipakai langsung */
        PG_TOMBOL_F1 = 0x80,
        PG_TOMBOL_F2, PG_TOMBOL_F3, PG_TOMBOL_F4, PG_TOMBOL_F5,
        PG_TOMBOL_F6, PG_TOMBOL_F7, PG_TOMBOL_F8, PG_TOMBOL_F9,
        PG_TOMBOL_F10, PG_TOMBOL_F11, PG_TOMBOL_F12,
        PG_TOMBOL_LSHIFT = 0x90, PG_TOMBOL_RSHIFT,
        PG_TOMBOL_LCTRL, PG_TOMBOL_RCTRL,
        PG_TOMBOL_LALT, PG_TOMBOL_RALT,
        PG_TOMBOL_LMETA, PG_TOMBOL_RMETA
};

enum pg_tetik_tombol {
        PG_TETIKUS_KOSONG = 0,
        PG_TETIKUS_KIRI = 1,
        PG_TETIKUS_KANAN = 2,
        PG_TETIKUS_TENGAH = 3,
        PG_TETIKUS_X1 = 4,
        PG_TETIKUS_X2 = 5
};

enum pg_jendela_aksi {
        PG_JENDELA_KOSONG = 0,
        PG_JENDELA_UBAH_UKURAN = 1,
        PG_JENDELA_TUTUP = 2,
        PG_JENDELA_FOKUS = 3,
        PG_JENDELA_BLUR = 4,
        PG_JENDELA_PERMINTAAN_GAMBAR = 5
};

typedef struct pg_peristiwa {
        int tipe;             /* pg_aksi_tipe */
        pg_u32 waktu_ms;      /* waktu monotonic emisi */

        /* union payload */
        int tombol;           /* untuk TOMBOL_TURUN / TOMBOL_NAIK */
        int modifier;         /* bitmask: 1=shift 2=ctrl 4=alt 8=meta */
        pg_u32 unicode;       /* codepoint printable atau 0 */

        int tetik_tombol;     /* untuk TETIK_TURUN/NAIK */
        pg_titik_t tetik_pos;
        int roda_dx, roda_dy;

        int jendela_aksi;/* untuk JENDELA */
        int jendela_id;

        /* peristiwa pengguna */
        void *pengguna_ptr;
        int   pengguna_kode;
} pg_aksi_t;

/* Makro kenyamanan untuk bitfield modifier. */
#define PG_MOD_SHIFT 1
#define PG_MOD_CTRL  2
#define PG_MOD_ALT   4
#define PG_MOD_META  8

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_PERISTIWA_H */
