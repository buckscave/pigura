/*
 * PIGURA OS - PENGATURAN.C
 * ==========================
 * Manajemen konfigurasi dan pengaturan sistem Pigura OS.
 *
 * Berkas ini mengimplementasikan sistem penyimpanan dan pembacaan
 * konfigurasi sistem meliputi pengaturan tampilan, bahasa, zona waktu,
 * jaringan, dan preferensi pengguna secara keseluruhan.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) + POSIX tertib dan ketat
 * - Tidak menggunakan fitur C99/C11
 * - Batas 80 karakter per baris
 * - Bahasa Indonesia untuk semua identifier
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "sistem.h"

/*
 * ===========================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ===========================================================================
 */

/* Jumlah maksimum entri konfigurasi */
#define PENGATURAN_ENTRI_MAKS       64

/* Panjang maksimum kunci konfigurasi */
#define PENGATURAN_KUNCI_MAKS       48

/* Panjang maksimum nilai konfigurasi */
#define PENGATURAN_NILAI_MAKS       128

/* Panjang maksimum kategori */
#define PENGATURAN_KATEGORI_MAKS    32

/* Jumlah kategori konfigurasi */
#define PENGATURAN_KATEGORI_JUMLAH  8

/* Kategori konfigurasi */
#define KAT_TAMPILAN    0
#define KAT_BAHASA      1
#define KAT_WAKTU       2
#define KAT_JARINGAN    3
#define KAT_AUDIO       4
#define KAT_KEAMANAN    5
#define KAT_SISTEM      6
#define KAT_PENGGUNA    7

/* ID konfigurasi tampilan */
#define SET_TAMPILAN_LEBAR      0
#define SET_TAMPILAN_TINGGI     1
#define SET_TAMPILAN_BIT        2
#define SET_TAMPILAN_TEMA       3
#define SET_TAMPILAN_KECERLANGAN 4

/* ID konfigurasi bahasa */
#define SET_BAHASA_LOKAL        0
#define SET_BAHASA_ENCODING     1

/* ID konfigurasi waktu */
#define SET_WAKTU_ZONA          0
#define SET_WAKTU_FORMAT_TGL    1
#define SET_WAKTU_FORMAT_JAM    2
#define SET_WAKTU_NTP_SERVER    3

/* ID konfigurasi jaringan */
#define SET_JARINGAN_HOSTNAME   0
#define SET_JARINGAN_IP         1
#define SET_JARINGAN_GATEWAY    2
#define SET_JARINGAN_DNS        3

/* ID konfigurasi sistem */
#define SET_SISTEM_LOG_LEVEL    0
#define SET_SISTEM_BOOT_VERBOSE 1
#define SET_SISTEM_SPLASH       2

/*
 * ===========================================================================
 * STRUKTUR ENTRI KONFIGURASI (CONFIGURATION ENTRY)
 * ===========================================================================
 */

/* Struktur satu entri konfigurasi kunci-nilai */
typedef struct {
    char kunci[PENGATURAN_KUNCI_MAKS];   /* Kunci config */
    char nilai[PENGATURAN_NILAI_MAKS];   /* Nilai config */
    tanda32_t kategori;                   /* Kategori */
    bool_t dimodifikasi;                  /* Flag perubahan */
    bool_t aktif;                         /* Apakah aktif */
} pengaturan_entri_t;

/*
 * ===========================================================================
 * STRUKTUR INFO KATEGORI (CATEGORY INFO)
 * ===========================================================================
 */

/* Informasi satu kategori konfigurasi */
typedef struct {
    char nama[PENGATURAN_KATEGORI_MAKS];  /* Nama kategori */
    tak_bertanda32_t jumlah_entri;        /* Jumlah entri */
} kategori_info_t;

/*
 * ===========================================================================
 * VARIABEL GLOBAL STATIS (STATIC GLOBAL VARIABLES)
 * ===========================================================================
 */

/* Tabel konfigurasi utama */
static pengaturan_entri_t g_pengaturan[PENGATURAN_ENTRI_MAKS];

/* Jumlah entri aktif */
static tak_bertanda32_t g_jumlah_entri = 0;

/* Flag perubahan sejak simpan terakhir */
static bool_t g_perubahan_tersimpan = SALAH;

/* Flag inisialisasi */
static bool_t g_terinisialisasi = SALAH;

/* Nama kategori konfigurasi */
static const char *g_nama_kategori[PENGATURAN_KATEGORI_JUMLAH] = {
    "tampilan",
    "bahasa",
    "waktu",
    "jaringan",
    "audio",
    "keamanan",
    "sistem",
    "pengguna"
};

/*
 * ===========================================================================
 * FUNGSI PEMBANTU (HELPER FUNCTIONS)
 * ===========================================================================
 */

/*
 * pengaturan_hash_kunci
 * --------------------
 * Menghitung hash kunci konfigurasi menggunakan djb2.
 *
 * Parameter:
 *   kunci - String kunci
 *
 * Return:
 *   Nilai hash
 */
static tak_bertanda32_t pengaturan_hash_kunci(const char *kunci)
{
    tak_bertanda32_t hash = 5381;
    ukuran_t i;
    ukuran_t panjang;

    if (kunci == NULL) {
        return 0;
    }

    panjang = kernel_strlen(kunci);
    for (i = 0; i < panjang; i++) {
        hash = ((hash << 5) + hash) +
               (tak_bertanda32_t)(unsigned char)kunci[i];
    }

    return hash;
}

/*
 * pengaturan_cari_index
 * ---------------------
 * Mencari index entri konfigurasi berdasarkan kunci.
 *
 * Parameter:
 *   kunci - String kunci
 *
 * Return:
 *   Index jika ditemukan, -1 jika tidak
 */
static tanda32_t pengaturan_cari_index(const char *kunci)
{
    tak_bertanda32_t i;

    if (kunci == NULL) {
        return -1;
    }

    for (i = 0; i < g_jumlah_entri; i++) {
        if (g_pengaturan[i].aktif &&
            kernel_strcmp(g_pengaturan[i].kunci, kunci) == 0) {
            return (tanda32_t)i;
        }
    }

    return -1;
}

/*
 * pengaturan_cari_slot_kosong
 * ---------------------------
 * Mencari slot kosong dalam tabel konfigurasi.
 *
 * Return:
 *   Index slot kosong, -1 jika penuh
 */
static tanda32_t pengaturan_cari_slot_kosong(void)
{
    tak_bertanda32_t i;

    for (i = 0; i < PENGATURAN_ENTRI_MAKS; i++) {
        if (!g_pengaturan[i].aktif) {
            return (tanda32_t)i;
        }
    }

    return -1;
}

/*
 * pengaturan_buat_kunci
 * ---------------------
 * Membuat nama kunci lengkap dari kategori dan nama lokal.
 *
 * Parameter:
 *   kategori - Indeks kategori
 *   nama     - Nama lokal
 *   buffer   - Buffer output
 *   ukuran   - Ukuran buffer
 */
static void pengaturan_buat_kunci(tanda32_t kategori,
                                   const char *nama,
                                   char *buffer, ukuran_t ukuran)
{
    if (nama == NULL || buffer == NULL || ukuran == 0) {
        return;
    }

    if (kategori >= 0 &&
        kategori < PENGATURAN_KATEGORI_JUMLAH) {
        kernel_snprintf(buffer, ukuran, "%s.%s",
                        g_nama_kategori[kategori], nama);
    } else {
        kernel_strncpy(buffer, nama, ukuran - 1);
    }
}

/*
 * ===========================================================================
 * FUNGSI KONFIGURASI DEFAULT (DEFAULT CONFIGURATION)
 * ===========================================================================
 */

/*
 * pengaturan_muat_default
 * -----------------------
 * Memuat konfigurasi default sistem.
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
static status_t pengaturan_muat_default(void)
{
    status_t hasil;

    hasil = pengaturan_set(PENGATURAN_KATEGORI_JUMLAH,
                          "sistem.versi", PIGURA_VERSI_STRING);
    if (hasil != STATUS_BERHASIL) return hasil;

    hasil = pengaturan_set(PENGATURAN_KATEGORI_JUMLAH,
                          "sistem.nama", PIGURA_NAMA);
    if (hasil != STATUS_BERHASIL) return hasil;

    hasil = pengaturan_set(KAT_TAMPILAN, "tampilan.lebar", "1024");
    if (hasil != STATUS_BERHASIL) return hasil;

    hasil = pengaturan_set(KAT_TAMPILAN, "tampilan.tinggi", "768");
    if (hasil != STATUS_BERHASIL) return hasil;

    hasil = pengaturan_set(KAT_TAMPILAN, "tampilan.bit", "32");
    if (hasil != STATUS_BERHASIL) return hasil;

    hasil = pengaturan_set(KAT_TAMPILAN, "tampilan.tema", "gelap");
    if (hasil != STATUS_BERHASIL) return hasil;

    hasil = pengaturan_set(KAT_TAMPILAN,
                          "tampilan.kecerlangan", "80");
    if (hasil != STATUS_BERHASIL) return hasil;

    hasil = pengaturan_set(KAT_BAHASA, "bahasa.lokal",
                          CONFIG_BAHASA);
    if (hasil != STATUS_BERHASIL) return hasil;

    hasil = pengaturan_set(KAT_BAHASA, "bahasa.encoding",
                          CONFIG_ENCODING);
    if (hasil != STATUS_BERHASIL) return hasil;

    hasil = pengaturan_set(KAT_WAKTU, "waktu.zona",
                          CONFIG_TIMEZONE);
    if (hasil != STATUS_BERHASIL) return hasil;

    hasil = pengaturan_set(KAT_WAKTU, "waktu.format_tgl", "0");
    if (hasil != STATUS_BERHASIL) return hasil;

    hasil = pengaturan_set(KAT_WAKTU, "waktu.format_jam", "0");
    if (hasil != STATUS_BERHASIL) return hasil;

    hasil = pengaturan_set(KAT_JARINGAN, "jaringan.hostname",
                          CONFIG_HOSTNAME_DEFAULT);
    if (hasil != STATUS_BERHASIL) return hasil;

    hasil = pengaturan_set(KAT_SISTEM, "sistem.log_level", "3");
    if (hasil != STATUS_BERHASIL) return hasil;

    hasil = pengaturan_set(KAT_SISTEM,
                          "sistem.boot_verbose", "1");
    if (hasil != STATUS_BERHASIL) return hasil;

    hasil = pengaturan_set(KAT_SISTEM, "sistem.splash", "1");
    if (hasil != STATUS_BERHASIL) return hasil;

    g_perubahan_tersimpan = SALAH;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI MANAJEMEN KONFIGURASI (CONFIG MANAGEMENT)
 * ===========================================================================
 */

/*
 * pengaturan_set
 * --------------
 * Mengatur nilai konfigurasi. Jika kunci belum ada, akan
 * dibuat entri baru secara otomatis.
 *
 * Parameter:
 *   kategori - Kategori konfigurasi
 *   kunci    - Kunci konfigurasi (format: kategori.nama)
 *   nilai    - Nilai konfigurasi baru
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t pengaturan_set(tanda32_t kategori, const char *kunci,
                         const char *nilai)
{
    tanda32_t index;

    if (kunci == NULL || nilai == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (kernel_strlen(kunci) == 0) {
        return STATUS_PARAM_UKURAN;
    }

    index = pengaturan_cari_index(kunci);

    if (index >= 0) {
        kernel_strncpy(g_pengaturan[index].nilai, nilai,
                       PENGATURAN_NILAI_MAKS - 1);
        g_pengaturan[index].dimodifikasi = BENAR;
        g_pengaturan[index].kategori = kategori;
        g_perubahan_tersimpan = BENAR;
        return STATUS_BERHASIL;
    }

    index = pengaturan_cari_slot_kosong();
    if (index < 0) {
        return STATUS_PENUH;
    }

    kernel_memset(&g_pengaturan[index], 0,
                  sizeof(pengaturan_entri_t));
    kernel_strncpy(g_pengaturan[index].kunci, kunci,
                   PENGATURAN_KUNCI_MAKS - 1);
    kernel_strncpy(g_pengaturan[index].nilai, nilai,
                   PENGATURAN_NILAI_MAKS - 1);
    g_pengaturan[index].kategori = kategori;
    g_pengaturan[index].dimodifikasi = BENAR;
    g_pengaturan[index].aktif = BENAR;

    g_jumlah_entri++;
    g_perubahan_tersimpan = BENAR;

    return STATUS_BERHASIL;
}

/*
 * pengaturan_ambil
 * ----------------
 * Mengambil nilai konfigurasi berdasarkan kunci.
 *
 * Parameter:
 *   kunci  - Kunci konfigurasi
 *   nilai  - Buffer output nilai
 *   ukuran - Ukuran buffer output
 *
 * Return:
 *   STATUS_BERHASIL jika ditemukan
 */
status_t pengaturan_ambil(const char *kunci, char *nilai,
                           ukuran_t ukuran)
{
    tanda32_t index;

    if (kunci == NULL || nilai == NULL || ukuran == 0) {
        return STATUS_PARAM_NULL;
    }

    index = pengaturan_cari_index(kunci);
    if (index < 0) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    kernel_strncpy(nilai, g_pengaturan[index].nilai,
                   ukuran - 1);
    nilai[ukuran - 1] = '\0';

    return STATUS_BERHASIL;
}

/*
 * pengaturan_ambil_bilangan
 * ------------------------
 * Mengambil nilai konfigurasi sebagai bilangan bulat.
 *
 * Parameter:
 *   kunci      - Kunci konfigurasi
 *   nilai      - Pointer output nilai
 *   nilai_bawa - Nilai bawaan jika kunci tidak ada
 *
 * Return:
 *   STATUS_BERHASIL jika ditemukan
 */
status_t pengaturan_ambil_bilangan(const char *kunci,
                                    tanda32_t *nilai,
                                    tanda32_t nilai_bawa)
{
    char buf[PENGATURAN_NILAI_MAKS];
    status_t hasil;
    tanda32_t hasil_konversi = 0;
    ukuran_t i;

    if (kunci == NULL || nilai == NULL) {
        return STATUS_PARAM_NULL;
    }

    hasil = pengaturan_ambil(kunci, buf, sizeof(buf));
    if (hasil != STATUS_BERHASIL) {
        *nilai = nilai_bawa;
        return STATUS_TIDAK_DITEMUKAN;
    }

    hasil_konversi = 0;
    for (i = 0; buf[i] != '\0'; i++) {
        if (buf[i] >= '0' && buf[i] <= '9') {
            hasil_konversi = hasil_konversi * 10 +
                             (buf[i] - '0');
        } else {
            *nilai = nilai_bawa;
            return STATUS_GAGAL;
        }
    }

    *nilai = hasil_konversi;
    return STATUS_BERHASIL;
}

/*
 * pengaturan_hapus
 * ----------------
 * Menghapus entri konfigurasi berdasarkan kunci.
 *
 * Parameter:
 *   kunci - Kunci yang akan dihapus
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t pengaturan_hapus(const char *kunci)
{
    tanda32_t index;

    if (kunci == NULL) {
        return STATUS_PARAM_NULL;
    }

    index = pengaturan_cari_index(kunci);
    if (index < 0) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    kernel_memset(&g_pengaturan[index], 0,
                  sizeof(pengaturan_entri_t));
    g_jumlah_entri--;
    g_perubahan_tersimpan = BENAR;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI INISIALISASI (INITIALIZATION)
 * ===========================================================================
 */

/*
 * pengaturan_inisialisasi
 * -----------------------
 * Menginisialisasi sistem pengaturan dengan nilai default.
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t pengaturan_inisialisasi(void)
{
    kernel_memset(g_pengaturan, 0, sizeof(g_pengaturan));
    g_jumlah_entri = 0;
    g_perubahan_tersimpan = SALAH;

    if (pengaturan_muat_default() != STATUS_BERHASIL) {
        kernel_puts("[pengaturan] Peringatan: sebagian "
                    "konfigurasi gagal dimuat\n");
    }

    g_terinisialisasi = BENAR;
    kernel_printf("[pengaturan] Diinisialisasi: %d entri\n",
                  g_jumlah_entri);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI TAMPILAN (DISPLAY FUNCTIONS)
 * ===========================================================================
 */

/*
 * pengaturan_tampilkan_semua
 * --------------------------
 * Menampilkan semua konfigurasi yang aktif.
 */
void pengaturan_tampilkan_semua(void)
{
    tak_bertanda32_t i;

    kernel_puts("=== Konfigurasi Sistem ===\n\n");

    for (i = 0; i < g_jumlah_entri; i++) {
        if (!g_pengaturan[i].aktif) {
            continue;
        }

        kernel_printf("  %-32s = %s\n",
                      g_pengaturan[i].kunci,
                      g_pengaturan[i].nilai);
    }

    kernel_printf("\nTotal: %d entri konfigurasi\n",
                  g_jumlah_entri);
}

/*
 * pengaturan_tampilkan_kategori
 * -----------------------------
 * Menampilkan konfigurasi dalam satu kategori.
 *
 * Parameter:
 *   kategori - Indeks kategori
 */
void pengaturan_tampilkan_kategori(tanda32_t kategori)
{
    tak_bertanda32_t i;
    tak_bertanda32_t jumlah = 0;

    if (kategori >= PENGATURAN_KATEGORI_JUMLAH) {
        kernel_puts("Kategori tidak valid\n");
        return;
    }

    kernel_puts("=== Konfigurasi: ");
    kernel_puts(g_nama_kategori[kategori]);
    kernel_puts(" ===\n\n");

    for (i = 0; i < g_jumlah_entri; i++) {
        if (!g_pengaturan[i].aktif) {
            continue;
        }
        if (g_pengaturan[i].kategori == (tanda32_t)kategori) {
            kernel_printf("  %-32s = %s\n",
                          g_pengaturan[i].kunci,
                          g_pengaturan[i].nilai);
            jumlah++;
        }
    }

    kernel_printf("\nTotal: %d entri\n", jumlah);
}

/*
 * pengaturan_jumlah_perubahan
 * ---------------------------
 * Menghitung jumlah entri yang telah dimodifikasi.
 *
 * Return:
 *   Jumlah entri yang dimodifikasi
 */
tak_bertanda32_t pengaturan_jumlah_perubahan(void)
{
    tak_bertanda32_t i;
    tak_bertanda32_t hitung = 0;

    for (i = 0; i < g_jumlah_entri; i++) {
        if (g_pengaturan[i].aktif &&
            g_pengaturan[i].dimodifikasi) {
            hitung++;
        }
    }

    return hitung;
}
