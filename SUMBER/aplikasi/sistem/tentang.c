/*
 * PIGURA OS - TENTANG.C
 * =======================
 * Informasi tentang Pigura OS dan kontributor.
 *
 * Berkas ini mengimplementasikan fungsi-fungsi untuk menampilkan
 * informasi lengkap tentang sistem operasi Pigura, termasuk
 * versi, arsitektur, komponen, lisensi, dan kontributor.
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

/* Jumlah maksimum kontributor */
#define TENTANG_KONTRIBUTOR_MAKS     16

/* Panjang maksimum nama kontributor */
#define TENTANG_NAMA_MAKS            64

/* Panjang maksimum peran kontributor */
#define TENTANG_PERAN_MAKS           64

/* Jumlah maksimum komponen sistem */
#define TENTANG_KOMPONEN_MAKS        16

/* Panjang maksimum nama komponen */
#define TENTANG_KOMPONEN_NAMA_MAKS   48

/* Jumlah maksimum baris lisensi */
#define TENTANG_LISENSI_BARIS_MAKS   8

/* Panjang maksimum baris lisensi */
#define TENTANG_LISENSI_PANJANG      80

/*
 * ===========================================================================
 * STRUKTUR KONTRIBUTOR (CONTRIBUTOR STRUCTURE)
 * ===========================================================================
 */

/* Struktur data kontributor */
typedef struct {
    char nama[TENTANG_NAMA_MAKS];       /* Nama kontributor */
    char peran[TENTANG_PERAN_MAKS];     /* Peran kontributor */
    bool_t aktif;                        /* Masih aktif */
} kontributor_t;

/*
 * ===========================================================================
 * STRUKTUR KOMPONEN SISTEM (SYSTEM COMPONENT)
 * ===========================================================================
 */

/* Struktur data komponen sistem */
typedef struct {
    char nama[TENTANG_KOMPONEN_NAMA_MAKS]; /* Nama komponen */
    const char *deskripsi;                  /* Deskripsi */
} komponen_t;

/*
 * ===========================================================================
 * VARIABEL GLOBAL STATIS (STATIC GLOBAL VARIABLES)
 * ===========================================================================
 */

/* Tabel kontributor */
static kontributor_t g_kontributor[TENTANG_KONTRIBUTOR_MAKS];

/* Jumlah kontributor */
static tak_bertanda32_t g_jumlah_kontributor = 0;

/* Tabel komponen sistem */
static komponen_t g_komponen[TENTANG_KOMPONEN_MAKS];

/* Jumlah komponen */
static tak_bertanda32_t g_jumlah_komponen = 0;

/* Teks lisensi */
static const char *g_lisensi[TENTANG_LISENSI_BARIS_MAKS] = {
    "Pigura OS - Sistem Operasi Bingkai Digital",
    "Copyright (c) 2025 Tim Pigura OS",
    "",
    "Lisensi: MIT",
    "",
    "Izin diberikan tanpa biaya kepada siapa pun yang memperoleh",
    "salinan perangkat lunak ini untuk menggunakannya tanpa batasan."
};

/*
 * ===========================================================================
 * FUNGSI INISIALISASI DATA (DATA INITIALIZATION)
 * ===========================================================================
 */

/*
 * tentang_muat_data
 * -----------------
 * Memuat data kontributor dan komponen sistem.
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
static status_t tentang_muat_data(void)
{
    kernel_memset(g_komponen, 0, sizeof(g_komponen));
    kernel_memset(g_kontributor, 0, sizeof(g_kontributor));

    kernel_strncpy(g_komponen[0].nama, "Kernel", sizeof(g_komponen[0].nama) - 1);
    g_komponen[0].deskripsi = "Inti sistem operasi";

    kernel_strncpy(g_komponen[1].nama, "HAL", sizeof(g_komponen[1].nama) - 1);
    g_komponen[1].deskripsi = "Lapisan Abstraksi Perangkat Keras";

    kernel_strncpy(g_komponen[2].nama, "Memori", sizeof(g_komponen[2].nama) - 1);
    g_komponen[2].deskripsi = "Manajemen memori fisik dan virtual";

    kernel_strncpy(g_komponen[3].nama, "Proses", sizeof(g_komponen[3].nama) - 1);
    g_komponen[3].deskripsi = "Manajemen proses dan thread";

    kernel_strncpy(g_komponen[4].nama, "VFS", sizeof(g_komponen[4].nama) - 1);
    g_komponen[4].deskripsi = "Virtual File System";

    kernel_strncpy(g_komponen[5].nama, "Driver", sizeof(g_komponen[5].nama) - 1);
    g_komponen[5].deskripsi = "Driver perangkat keras";

    kernel_strncpy(g_komponen[6].nama, "Framebuffer", sizeof(g_komponen[6].nama) - 1);
    g_komponen[6].deskripsi = "Sistem framebuffer grafis";

    kernel_strncpy(g_komponen[7].nama, "LibPigura", sizeof(g_komponen[7].nama) - 1);
    g_komponen[7].deskripsi = "Pustaka grafis dan widget";

    kernel_strncpy(g_komponen[8].nama, "LibC", sizeof(g_komponen[8].nama) - 1);
    g_komponen[8].deskripsi = "Pustaka C (kustom)";

    g_jumlah_komponen = 9;

    kernel_strncpy(g_kontributor[0].nama,
                   "Tim Pigura OS",
                   TENTANG_NAMA_MAKS - 1);
    kernel_strncpy(g_kontributor[0].peran,
                   "Pengembang Inti",
                   TENTANG_PERAN_MAKS - 1);
    g_kontributor[0].aktif = BENAR;

    g_jumlah_kontributor = 1;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI TAMPILAN INFORMASI (DISPLAY FUNCTIONS)
 * ===========================================================================
 */

/*
 * tentang_tampilkan_banner
 * ------------------------
 * Menampilkan banner utama Pigura OS.
 */
void tentang_tampilkan_banner(void)
{
    kernel_puts("  _____ _____ _____  ____    ___ \n");
    kernel_puts(" |  _  | __  | __  ||    \\  |   |\n");
    kernel_puts(" |     |    -|    -||  |  |_|   |\n");
    kernel_puts(" |__|__|__|__|__|__||____/ |___|\n");
    kernel_puts("\n");
    kernel_printf(" %s %s (%s)\n",
                  PIGURA_NAMA, PIGURA_VERSI_STRING,
                  PIGURA_JULUKAN);
    kernel_printf(" Arsitektur: %s\n",
                  NAMA_ARSITEKTUR_LENGKAP);
    kernel_puts("\n");
}

/*
 * tentang_tampilkan_versi
 * -----------------------
 * Menampilkan informasi versi sistem secara detail.
 */
void tentang_tampilkan_versi(void)
{
    kernel_puts("=== Informasi Versi ===\n\n");
    kernel_printf("Nama      : %s\n", PIGURA_NAMA);
    kernel_printf("Versi     : %s\n", PIGURA_VERSI_STRING);
    kernel_printf("Julukan   : %s\n", PIGURA_JULUKAN);
    kernel_printf("Major     : %d\n", PIGURA_VERSI_MAJOR);
    kernel_printf("Minor     : %d\n", PIGURA_VERSI_MINOR);
    kernel_printf("Patch     : %d\n", PIGURA_VERSI_PATCH);
    kernel_printf("Build     : %d\n", PIGURA_VERSI_BUILD);
    kernel_printf("Numerik   : %d\n", PIGURA_VERSI_NUM);
    kernel_puts("\n");
}

/*
 * tentang_tampilkan_arsitektur
 * ----------------------------
 * Menampilkan informasi arsitektur sistem.
 */
void tentang_tampilkan_arsitektur(void)
{
    kernel_puts("=== Informasi Arsitektur ===\n\n");

#if defined(ARSITEKTUR_X86)
    kernel_puts("Arsitektur : x86 (32-bit)\n");
#elif defined(ARSITEKTUR_X86_64)
    kernel_puts("Arsitektur : x86_64 (64-bit)\n");
#elif defined(ARSITEKTUR_ARM)
    kernel_puts("Arsitektur : ARM (32-bit)\n");
#elif defined(ARSITEKTUR_ARMV7)
    kernel_puts("Arsitektur : ARMv7 (32-bit)\n");
#elif defined(ARSITEKTUR_ARM64)
    kernel_puts("Arsitektur : ARM64 (AArch64, 64-bit)\n");
#else
    kernel_puts("Arsitektur : Tidak dikenali\n");
#endif

    kernel_printf("Lebar Bit : %d\n", LEBAR_BIT);

#if defined(PIGURA_ARSITEKTUR_64BIT)
    kernel_puts("Mode       : 64-bit\n");
#elif defined(PIGURA_ARSITEKTUR_32BIT)
    kernel_puts("Mode       : 32-bit\n");
#endif

#if defined(BYTE_ORDER_LITTLE)
    kernel_puts("Byte Order : Little Endian\n");
#else
    kernel_puts("Byte Order : Big Endian\n");
#endif

#if defined(SUPPORT_PAE)
    kernel_puts("PAE        : Didukung\n");
#endif

#if defined(SUPPORT_LONG_MODE)
    kernel_puts("Long Mode  : Didukung\n");
#endif

#if defined(SUPPORT_NEON)
    kernel_puts("NEON       : Didukung\n");
#endif

#if defined(SUPPORT_VFP)
    kernel_puts("VFP        : Didukung\n");
#endif

#if defined(SUPPORT_MMU_ARM)
    kernel_puts("MMU        : Didukung (ARM)\n");
#endif

    kernel_puts("\n");
}

/*
 * tentang_tampilkan_komponen
 * --------------------------
 * Menampilkan daftar komponen utama sistem.
 */
void tentang_tampilkan_komponen(void)
{
    tak_bertanda32_t i;
    status_t hasil;

    if (g_jumlah_komponen == 0) {
        hasil = tentang_muat_data();
        if (hasil != STATUS_BERHASIL) {
            kernel_puts("Gagal memuat data komponen\n");
            return;
        }
    }

    kernel_puts("=== Komponen Sistem ===\n\n");

    for (i = 0; i < g_jumlah_komponen; i++) {
        kernel_printf("  %-16s - %s\n",
                      g_komponen[i].nama,
                      g_komponen[i].deskripsi);
    }

    kernel_printf("\nTotal: %u komponen\n\n", g_jumlah_komponen);
}

/*
 * tentang_tampilkan_kontributor
 * -----------------------------
 * Menampilkan daftar kontributor.
 */
void tentang_tampilkan_kontributor(void)
{
    tak_bertanda32_t i;
    status_t hasil;

    if (g_jumlah_komponen == 0) {
        hasil = tentang_muat_data();
        if (hasil != STATUS_BERHASIL) {
            kernel_puts("Gagal memuat data\n");
            return;
        }
    }

    kernel_puts("=== Kontributor ===\n\n");

    for (i = 0; i < g_jumlah_kontributor; i++) {
        if (!g_kontributor[i].aktif) {
            continue;
        }

        kernel_printf("  %-32s (%s)\n",
                      g_kontributor[i].nama,
                      g_kontributor[i].peran);
    }

    kernel_puts("\n");
}

/*
 * tentang_tampilkan_lisensi
 * -------------------------
 * Menampilkan teks lisensi sistem.
 */
void tentang_tampilkan_lisensi(void)
{
    int i;

    kernel_puts("=== Lisensi ===\n\n");

    for (i = 0; i < TENTANG_LISENSI_BARIS_MAKS; i++) {
        if (g_lisensi[i] != NULL) {
            kernel_puts(g_lisensi[i]);
            kernel_puts("\n");
        }
    }

    kernel_puts("\n");
}

/*
 * tentang_tampilkan_semua
 * -----------------------
 * Menampilkan semua informasi tentang sistem secara lengkap.
 */
void tentang_tampilkan_semua(void)
{
    tentang_tampilkan_banner();
    tentang_tampilkan_versi();
    tentang_tampilkan_arsitektur();
    tentang_tampilkan_komponen();
    tentang_tampilkan_kontributor();
    tentang_tampilkan_lisensi();
}

/*
 * tentang_dapat_versi_string
 * --------------------------
 * Mendapatkan string versi sistem.
 *
 * Return:
 *   Pointer ke string versi (konstan)
 */
const char *tentang_dapat_versi_string(void)
{
    return PIGURA_VERSI_STRING;
}

/*
 * tentang_dapat_nama
 * ------------------
 * Mendapatkan nama sistem operasi.
 *
 * Return:
 *   Pointer ke string nama (konstan)
 */
const char *tentang_dapat_nama(void)
{
    return PIGURA_NAMA;
}

/*
 * tentang_dapat_julukan
 * ---------------------
 * Mendapatkan julukan sistem.
 *
 * Return:
 *   Pointer ke string julukan (konstan)
 */
const char *tentang_dapat_julukan(void)
{
    return PIGURA_JULUKAN;
}

/*
 * tentang_dapat_versi_numerik
 * ---------------------------
 * Mendapatkan versi numerik sistem.
 *
 * Return:
 *   Nomor versi sebagai integer
 */
tanda32_t tentang_dapat_versi_numerik(void)
{
    return (tanda32_t)PIGURA_VERSI_NUM;
}

/*
 * tentang_inisialisasi
 * --------------------
 * Menginisialisasi modul informasi tentang.
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t tentang_inisialisasi(void)
{
    status_t hasil;

    hasil = tentang_muat_data();
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }

    kernel_printf("[tentang] Data dimuat: %u komponen, "
                  "%u kontributor\n",
                  g_jumlah_komponen, g_jumlah_kontributor);

    return STATUS_BERHASIL;
}
