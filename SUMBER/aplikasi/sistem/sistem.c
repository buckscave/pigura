/*
 * PIGURA OS - SISTEM.C
 * =====================
 * Utilitas informasi dan manajemen sistem Pigura OS.
 *
 * Berkas ini menyediakan fungsi-fungsi untuk menampilkan dan mengelola
 * informasi sistem, termasuk uptime, penggunaan memori, daftar proses,
 * informasi CPU, dan konfigurasi hardware.
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

/* Jumlah maksimum entri pemantauan CPU */
#define SISTEM_CPU_CORE_MAKS     8

/* Interval refresh default (milidetik) */
#define SISTEM_REFRESH_DEFAULT   1000

/* Lebar kolom tabel proses */
#define SISTEM_KOLOM_PID         8
#define SISTEM_KOLOM_NAMA        24
#define SISTEM_KOLOM_STATUS      10
#define SISTEM_KOLOM_PRIORITAS   8
#define SISTEM_KOLOM_MEMORI      12

/*
 * ===========================================================================
 * STRUKTUR STATISTIK CPU (CPU STATISTICS STRUCTURE)
 * ===========================================================================
 */

/* Struktur untuk menyimpan statistik CPU per core */
typedef struct {
    tak_bertanda64_t user;      /* Waktu user mode */
    tak_bertanda64_t system;    /* Waktu system mode */
    tak_bertanda64_t idle;      /* Waktu idle */
    tak_bertanda64_t iowait;    /* Waktu menunggu I/O */
    tak_bertanda64_t irq;       /* Waktu penanganan IRQ */
    tak_bertanda64_t softirq;   /* Waktu softirq */
    tak_bertanda64_t total;     /* Total waktu */
} statistik_cpu_sistem_t;

/*
 * ===========================================================================
 * VARIABEL GLOBAL STATIS (STATIC GLOBAL VARIABLES)
 * ===========================================================================
 */

/* Statistik CPU per core (digunakan oleh fungsi delta) */
FUNGSI_TIDAK_DIGUNAKAN
static statistik_cpu_sistem_t g_stat_cpu[SISTEM_CPU_CORE_MAKS];

/* Snapshot statistik sebelumnya (untuk delta) */
FUNGSI_TIDAK_DIGUNAKAN
static statistik_cpu_sistem_t g_stat_cpu_lama[
    SISTEM_CPU_CORE_MAKS];

/* Flag apakah statistik sudah diambil */
FUNGSI_TIDAK_DIGUNAKAN
static bool_t g_stat_diinisialisasi = SALAH;

/*
 * ===========================================================================
 * FUNGSI PEMBANTU STRING (STRING HELPER FUNCTIONS)
 * ===========================================================================
 */

/*
 * sistem_format_ukuran
 * --------------------
 * Memformat ukuran byte menjadi string yang mudah dibaca.
 *
 * Parameter:
 *   byte     - Ukuran dalam byte
 *   buffer   - Buffer tujuan
 *   ukuran_buf - Ukuran buffer
 */
static void sistem_format_ukuran(tak_bertanda64_t byte, char *buffer,
                                  ukuran_t ukuran_buf)
{
    if (buffer == NULL || ukuran_buf == 0) {
        return;
    }

    if (byte >= (tak_bertanda64_t)1024 * 1024 * 1024) {
        kernel_snprintf(buffer, ukuran_buf, "%llu GB",
                        (byte) / ((tak_bertanda64_t)1024 * 1024 * 1024));
    } else if (byte >= (tak_bertanda64_t)1024 * 1024) {
        kernel_snprintf(buffer, ukuran_buf, "%llu MB",
                        (byte) / ((tak_bertanda64_t)1024 * 1024));
    } else if (byte >= 1024) {
        kernel_snprintf(buffer, ukuran_buf, "%llu KB",
                        (byte) / 1024);
    } else {
        kernel_snprintf(buffer, ukuran_buf, "%llu B", byte);
    }
}

/*
 * sistem_format_waktu
 * -------------------
 * Memformat uptime detik menjadi format jam:menit:detik.
 *
 * Parameter:
 *   detik   - Waktu dalam detik
 *   buffer  - Buffer tujuan
 *   ukuran_buf - Ukuran buffer
 */
static void sistem_format_waktu(tak_bertanda64_t detik, char *buffer,
                                 ukuran_t ukuran_buf)
{
    tak_bertanda64_t jam, menit, sisa;

    if (buffer == NULL || ukuran_buf == 0) {
        return;
    }

    jam = detik / 3600;
    sisa = detik % 3600;
    menit = sisa / 60;
    sisa = sisa % 60;

    kernel_snprintf(buffer, ukuran_buf, "%02llu:%02llu:%02llu",
                    jam, menit, sisa);
}

/*
 * ===========================================================================
 * FUNGSI INFORMASI SISTEM (SYSTEM INFORMATION FUNCTIONS)
 * ===========================================================================
 */

/*
 * sistem_dapat_ringkasan
 * ----------------------
 * Mengumpulkan informasi ringkasan sistem.
 *
 * Parameter:
 *   info - Pointer ke struktur ringkasan (output)
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t sistem_dapat_ringkasan(ringkasan_sistem_t *info)
{
    const info_sistem_t *kernel_info;
    tak_bertanda64_t mem_total, mem_bebas, mem_pakai;

    if (info == NULL) {
        return STATUS_PARAM_NULL;
    }

    kernel_memset(info, 0, sizeof(ringkasan_sistem_t));

    kernel_info = kernel_get_info();
    if (kernel_info != NULL) {
        info->uptime = kernel_get_uptime();
        kernel_strncpy(info->hostname, kernel_info->hostname,
                       HOST_NAME_MAX - 1);
        kernel_strncpy(info->arsitektur, kernel_info->arsitektur,
                       sizeof(info->arsitektur) - 1);
        kernel_strncpy(info->versi_kernel, kernel_info->versi,
                       sizeof(info->versi_kernel) - 1);
        info->cpu_core = kernel_info->cpu_count;
    } else {
        info->uptime = kernel_get_uptime();
        kernel_strncpy(info->hostname, CONFIG_HOSTNAME_DEFAULT,
                       HOST_NAME_MAX - 1);
        kernel_strncpy(info->arsitektur, NAMA_ARSITEKTUR,
                       sizeof(info->arsitektur) - 1);
        kernel_strncpy(info->versi_kernel,
                       PIGURA_VERSI_STRING,
                       sizeof(info->versi_kernel) - 1);
        info->cpu_core = 1;
    }

    info->proses_aktif = proses_dapat_jumlah();
    info->proses_total = proses_dapat_jumlah_total();

    mem_total = 0;
    mem_bebas = 0;
    mem_pakai = 0;
    pmm_get_stats(&mem_total, &mem_bebas, &mem_pakai);

    info->memori_total = mem_total;
    info->memori_bebas = mem_bebas;
    info->memori_pakai = mem_pakai;

    return STATUS_BERHASIL;
}

/*
 * sistem_tampilkan_info
 * ---------------------
 * Menampilkan informasi sistem ke console.
 */
void sistem_tampilkan_info(void)
{
    ringkasan_sistem_t info;
    char buf_waktu[32];
    char buf_mem[32];
    char buf_bebas[32];
    char buf_pakai[32];

    if (sistem_dapat_ringkasan(&info) != STATUS_BERHASIL) {
        kernel_puts("Gagal mengumpulkan info sistem\n");
        return;
    }

    kernel_puts("\n");
    kernel_puts("=== Informasi Sistem " PIGURA_NAMA
                " ===\n");
    kernel_puts("================================"
                "========\n");

    kernel_puts("Nama Host    : ");
    kernel_puts(info.hostname);
    kernel_puts("\n");

    kernel_puts("Kernel       : ");
    kernel_puts(PIGURA_NAMA);
    kernel_puts(" versi ");
    kernel_puts(info.versi_kernel);
    kernel_puts("\n");

    kernel_puts("Arsitektur   : ");
    kernel_puts(info.arsitektur);
    kernel_puts("\n");

    sistem_format_waktu(info.uptime, buf_waktu, sizeof(buf_waktu));
    kernel_puts("Uptime       : ");
    kernel_puts(buf_waktu);
    kernel_puts("\n");

    kernel_printf("CPU Core     : %u\n", info.cpu_core);
    kernel_printf("Proses Aktif : %u\n", info.proses_aktif);
    kernel_printf("Thread Total : %u\n", info.thread_total);

    sistem_format_ukuran(info.memori_total, buf_mem, sizeof(buf_mem));
    sistem_format_ukuran(info.memori_bebas, buf_bebas,
                         sizeof(buf_bebas));
    sistem_format_ukuran(info.memori_pakai, buf_pakai,
                         sizeof(buf_pakai));

    kernel_puts("Memori Total : ");
    kernel_puts(buf_mem);
    kernel_puts("\n");

    kernel_puts("Memori Bebas : ");
    kernel_puts(buf_bebas);
    kernel_puts("\n");

    kernel_puts("Memori Pakai : ");
    kernel_puts(buf_pakai);
    kernel_puts("\n");

    kernel_puts("================================"
                "========\n\n");
}

/*
 * sistem_tampilkan_memori
 * -----------------------
 * Menampilkan informasi memori secara detail.
 */
void sistem_tampilkan_memori(void)
{
    tak_bertanda64_t total, bebas, pakai;
    char buf_total[32], buf_bebas[32], buf_pakai[32];

    pmm_get_stats(&total, &bebas, &pakai);

    kernel_puts("\n=== Informasi Memori ===\n");

    sistem_format_ukuran(total, buf_total, sizeof(buf_total));
    sistem_format_ukuran(bebas, buf_bebas, sizeof(buf_bebas));
    sistem_format_ukuran(pakai, buf_pakai, sizeof(buf_pakai));

    kernel_puts("Total   : ");
    kernel_puts(buf_total);
    kernel_puts("\n");

    kernel_puts("Bebas   : ");
    kernel_puts(buf_bebas);
    kernel_puts("\n");

    kernel_puts("Terpakai: ");
    kernel_puts(buf_pakai);
    kernel_puts("\n");

    if (total > 0) {
        tak_bertanda32_t persen;
        persen = (tak_bertanda32_t)((pakai * 100) / total);
        kernel_printf("Penggunaan: %u%%\n", persen);
    }

    kernel_puts("========================\n\n");
}

/*
 * sistem_tampilkan_uptime
 * -----------------------
 * Menampilkan uptime sistem.
 */
void sistem_tampilkan_uptime(void)
{
    ringkasan_sistem_t info;
    char buf_waktu[32];

    if (sistem_dapat_ringkasan(&info) != STATUS_BERHASIL) {
        kernel_puts("Gagal membaca uptime\n");
        return;
    }

    sistem_format_waktu(info.uptime, buf_waktu, sizeof(buf_waktu));
    kernel_puts("Uptime: ");
    kernel_puts(buf_waktu);
    kernel_puts("\n");
}

/*
 * sistem_tampilkan_hostname
 * -------------------------
 * Menampilkan hostname sistem.
 */
void sistem_tampilkan_hostname(void)
{
    const char *host = kernel_get_hostname();

    if (host != NULL) {
        kernel_puts(host);
        kernel_puts("\n");
    } else {
        kernel_puts("(tidak dikenal)\n");
    }
}

/*
 * sistem_set_hostname
 * -------------------
 * Mengubah hostname sistem.
 *
 * Parameter:
 *   hostname - Nama host baru
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t sistem_set_hostname(const char *hostname)
{
    ukuran_t panjang;

    if (hostname == NULL) {
        return STATUS_PARAM_NULL;
    }

    panjang = kernel_strlen(hostname);
    if (panjang == 0 || panjang >= HOST_NAME_MAX) {
        return STATUS_PARAM_UKURAN;
    }

    return kernel_set_hostname(hostname);
}

/*
 * sistem_tampilkan_proses
 * -----------------------
 * Menampilkan daftar proses yang sedang berjalan.
 */
void sistem_tampilkan_proses(void)
{
    kernel_puts("\n=== Daftar Proses ===\n");
    kernel_puts("PID   Nama                    Status     "
                "Prioritas\n");
    kernel_puts("---------------------------------------"
                "-----------\n");

    proses_print_list();

    kernel_puts("---------------------------------------"
                "-----------\n\n");
}

/*
 * sistem_tampilkan_versi
 * ----------------------
 * Menampilkan versi kernel secara detail.
 */
void sistem_tampilkan_versi(void)
{
    kernel_printf("%s versi %s (%s)\n",
                  PIGURA_NAMA, PIGURA_VERSI_STRING,
                  NAMA_ARSITEKTUR_LENGKAP);
    kernel_printf("Julukan: %s\n", PIGURA_JULUKAN);
    kernel_printf("Homepage: %s\n", PIGURA_HOMEPAGE);
}
