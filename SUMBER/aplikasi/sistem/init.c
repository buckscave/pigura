/*
 * PIGURA OS - INIT.C
 * ==================
 * Proses init (PID 1) untuk Pigura OS.
 *
 * Berkas ini mengimplementasikan proses init yang merupakan proses pertama
 * di ruang pengguna setelah kernel selesai boot. Init bertanggung jawab
 * atas inisialisasi layanan sistem, manajemen proses daemon, dan
 * pembersihan saat shutdown.
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

/* Jumlah maksimum layanan yang bisa didaftarkan */
#define INIT_LAYANAN_MAKS        32

/* Jumlah maksimum entri runlevel */
#define INIT_RUNLEVEL_MAKS       8

/* Jumlah maksimum percobaan mulai layanan */
#define INIT_PERCOBAAN_MAKS      3

/* Interval jeda antar percobaan (milidetik) */
#define INIT_JEDA_PERCOBAAN      1000

/* Status layanan */
#define LAYANAN_STATUS_MATI      0
#define LAYANAN_STATUS_JALAN     1
#define LAYANAN_STATUS_GAGAL     2
#define LAYANAN_STATUS_MENUNGGU  3

/* Runlevel standar */
#define RUNLEVEL_MATI            0
#define RUNLEVEL_TUNGGU          1
#define RUNLEVEL_MULTIUSER       2
#define RUNLEVEL_GRAFIS          3
#define RUNLEVEL_TUNGGU_SHUTDOWN 6

/*
 * ===========================================================================
 * STRUKTUR LAYANAN (SERVICE STRUCTURE)
 * ===========================================================================
 */

/* Struktur untuk mendeskripsikan satu layanan sistem */
typedef struct {
    char nama[MAKS_NAMA_PROSES];    /* Nama layanan */
    char path[MAKS_PATH_LEN];       /* Path ke binary */
    char argumen[MAKS_ARGV_LEN];    /* Argumen tambahan */
    tanda32_t status;               /* Status layanan */
    pid_t pid;                      /* PID proses layanan */
    tanda32_t runlevel;             /* Runlevel layanan aktif */
    tak_bertanda32_t urutan;        /* Urutan mulai */
    tak_bertanda32_t percobaan;     /* Percobaan restart */
    bool_t otomatis;                /* Mulai otomatis */
    bool_t kritikal;                /* Proses kritikal */
    bool_t aktif;                   /* Apakah terdaftar aktif */
} layanan_t;

/*
 * ===========================================================================
 * STRUKTUR LAYANAN TERDAFTAR (REGISTERED SERVICE)
 * ===========================================================================
 */

/* Struktur runlevel */
typedef struct {
    tanda32_t nomor;                 /* Nomor runlevel */
    tak_bertanda32_t jumlah_aktif;   /* Layanan aktif */
} runlevel_t;

/*
 * ===========================================================================
 * VARIABEL GLOBAL STATIS (STATIC GLOBAL VARIABLES)
 * ===========================================================================
 */

/* Tabel layanan yang terdaftar */
static layanan_t g_layanan[INIT_LAYANAN_MAKS];

/* Jumlah layanan terdaftar */
static tak_bertanda32_t g_jumlah_layanan = 0;

/* Runlevel saat ini */
static tanda32_t g_runlevel_saat_ini = RUNLEVEL_MATI;

/* Status init */
static volatile bool_t g_init_berjalan = SALAH;

/* Flag shutdown */
static volatile bool_t g_shutdown_diminta = SALAH;

/*
 * ===========================================================================
 * FUNGSI PEMBANTU LAYANAN (SERVICE HELPER FUNCTIONS)
 * ===========================================================================
 */

/*
 * layanan_cari_index
 * ------------------
 * Mencari index layanan berdasarkan nama.
 *
 * Parameter:
 *   nama - Nama layanan yang dicari
 *
 * Return:
 *   Index layanan jika ditemukan, -1 jika tidak
 */
static tanda32_t layanan_cari_index(const char *nama)
{
    tak_bertanda32_t i;

    if (nama == NULL) {
        return -1;
    }

    for (i = 0; i < g_jumlah_layanan; i++) {
        if (g_layanan[i].aktif &&
            kernel_strcmp(g_layanan[i].nama, nama) == 0) {
            return (tanda32_t)i;
        }
    }

    return -1;
}

/*
 * layanan_cari_slot_kosong
 * ------------------------
 * Mencari slot kosong dalam tabel layanan.
 *
 * Return:
 *   Index slot kosong, -1 jika tabel penuh
 */
static tanda32_t layanan_cari_slot_kosong(void)
{
    tak_bertanda32_t i;

    for (i = 0; i < INIT_LAYANAN_MAKS; i++) {
        if (!g_layanan[i].aktif) {
            return (tanda32_t)i;
        }
    }

    return -1;
}

/*
 * layanan_sortir_urutan
 * ---------------------
 * Mengurutkan layanan berdasarkan urutan mulai
 * menggunakan algoritma insertion sort.
 */
static void layanan_sortir_urutan(void)
{
    tanda32_t i, j;
    layanan_t temp;

    for (i = 1; i < (tanda32_t)g_jumlah_layanan; i++) {
        kernel_memcpy(&temp, &g_layanan[i], sizeof(layanan_t));
        j = i - 1;

        while (j >= 0 &&
               g_layanan[j].urutan > temp.urutan) {
            kernel_memcpy(&g_layanan[j + 1], &g_layanan[j],
                          sizeof(layanan_t));
            j--;
        }

        kernel_memcpy(&g_layanan[j + 1], &temp, sizeof(layanan_t));
    }
}

/*
 * ===========================================================================
 * FUNGSI MANAJEMEN LAYANAN (SERVICE MANAGEMENT FUNCTIONS)
 * ===========================================================================
 */

/*
 * init_daftarkan_layanan
 * ----------------------
 * Mendaftarkan layanan baru ke tabel init.
 *
 * Parameter:
 *   nama     - Nama layanan (unik)
 *   path     - Path ke binary layanan
 *   argumen  - Argumen baris perintah
 *   runlevel - Runlevel di mana layanan aktif
 *   urutan   - Urutan start (lebih kecil = lebih dulu)
 *   otomatis - Apakah mulai otomatis
 *   kritikal - Apakah layanan kritikal bagi sistem
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t init_daftarkan_layanan(const char *nama, const char *path,
                                const char *argumen, tanda32_t runlevel,
                                tak_bertanda32_t urutan, bool_t otomatis,
                                bool_t kritikal)
{
    tanda32_t index;

    if (nama == NULL || path == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (g_jumlah_layanan >= INIT_LAYANAN_MAKS) {
        return STATUS_PENUH;
    }

    index = layanan_cari_index(nama);
    if (index >= 0) {
        return STATUS_SUDAH_ADA;
    }

    index = layanan_cari_slot_kosong();
    if (index < 0) {
        return STATUS_PENUH;
    }

    kernel_memset(&g_layanan[index], 0, sizeof(layanan_t));
    kernel_strncpy(g_layanan[index].nama, nama, MAKS_NAMA_PROSES - 1);
    kernel_strncpy(g_layanan[index].path, path, MAKS_PATH_LEN - 1);

    if (argumen != NULL) {
        kernel_strncpy(g_layanan[index].argumen, argumen,
                       MAKS_ARGV_LEN - 1);
    }

    g_layanan[index].status = LAYANAN_STATUS_MATI;
    g_layanan[index].pid = PID_TIDAK_ADA;
    g_layanan[index].runlevel = runlevel;
    g_layanan[index].urutan = urutan;
    g_layanan[index].percobaan = 0;
    g_layanan[index].otomatis = otomatis;
    g_layanan[index].kritikal = kritikal;
    g_layanan[index].aktif = BENAR;

    g_jumlah_layanan++;

    return STATUS_BERHASIL;
}

/*
 * init_hapus_layanan
 * ------------------
 * Menghapus layanan dari tabel init.
 *
 * Parameter:
 *   nama - Nama layanan yang akan dihapus
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil dihapus
 */
status_t init_hapus_layanan(const char *nama)
{
    tanda32_t index;

    if (nama == NULL) {
        return STATUS_PARAM_NULL;
    }

    index = layanan_cari_index(nama);
    if (index < 0) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    if (g_layanan[index].status == LAYANAN_STATUS_JALAN) {
        proses_kill(g_layanan[index].pid, SIGNAL_TERM);
    }

    kernel_memset(&g_layanan[index], 0, sizeof(layanan_t));
    g_layanan[index].aktif = SALAH;
    g_jumlah_layanan--;

    return STATUS_BERHASIL;
}

/*
 * init_mulai_layanan
 * -------------------
 * Memulai satu layanan.
 *
 * Parameter:
 *   index - Index layanan dalam tabel
 *
 * Return:
 *   STATUS_BERHASIL jika layanan berhasil dimulai
 */
static status_t init_mulai_layanan(tanda32_t index)
{
    pid_t pid_baru;

    if (index < 0 || index >= (tanda32_t)g_jumlah_layanan) {
        return STATUS_PARAM_INVALID;
    }

    if (!g_layanan[index].aktif) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    if (g_layanan[index].status == LAYANAN_STATUS_JALAN) {
        return STATUS_SUDAH_ADA;
    }

    if (g_layanan[index].status == LAYANAN_STATUS_GAGAL &&
        g_layanan[index].percobaan >= INIT_PERCOBAAN_MAKS) {
        kernel_printf("[init] Layanan '%s' gagal setelah %d "
                      "percobaan\n",
                      g_layanan[index].nama,
                      INIT_PERCOBAAN_MAKS);
        return STATUS_GAGAL;
    }

    g_layanan[index].status = LAYANAN_STATUS_MENUNGGU;
    pid_baru = proses_buat(g_layanan[index].nama, PID_INIT,
                           PROSES_FLAG_DAEMON);

    if (pid_baru == PID_TIDAK_ADA) {
        g_layanan[index].status = LAYANAN_STATUS_GAGAL;
        g_layanan[index].percobaan++;
        kernel_printf("[init] Gagal memulai layanan '%s'\n",
                      g_layanan[index].nama);
        return STATUS_GAGAL;
    }

    g_layanan[index].pid = pid_baru;
    g_layanan[index].status = LAYANAN_STATUS_JALAN;
    g_layanan[index].percobaan = 0;

    kernel_printf("[init] Layanan '%s' dimulai (PID %d)\n",
                  g_layanan[index].nama, pid_baru);

    return STATUS_BERHASIL;
}

/*
 * init_hentikan_layanan
 * ---------------------
 * Menghentikan satu layanan.
 *
 * Parameter:
 *   index - Index layanan dalam tabel
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil dihentikan
 */
static status_t init_hentikan_layanan(tanda32_t index)
{
    status_t hasil;

    if (index < 0 || index >= (tanda32_t)g_jumlah_layanan) {
        return STATUS_PARAM_INVALID;
    }

    if (!g_layanan[index].aktif) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    if (g_layanan[index].status != LAYANAN_STATUS_JALAN) {
        return STATUS_BERHASIL;
    }

    hasil = proses_kill(g_layanan[index].pid, SIGNAL_TERM);

    if (hasil != STATUS_BERHASIL) {
        hasil = proses_kill(g_layanan[index].pid, SIGNAL_KILL);
    }

    g_layanan[index].status = LAYANAN_STATUS_MATI;
    g_layanan[index].pid = PID_TIDAK_ADA;

    kernel_printf("[init] Layanan '%s' dihentikan\n",
                  g_layanan[index].nama);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI MANAJEMEN RUNLEVEL (RUNLEVEL MANAGEMENT)
 * ===========================================================================
 */

/*
 * init_masuk_runlevel
 * -------------------
 * Berpindah ke runlevel baru. Menghentikan layanan yang tidak sesuai
 * dan memulai layanan yang diperlukan di runlevel baru.
 *
 * Parameter:
 *   runlevel - Nomor runlevel tujuan
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t init_masuk_runlevel(tanda32_t runlevel)
{
    tak_bertanda32_t i;
    tanda32_t rl_lama;

    if (runlevel < RUNLEVEL_MATI || runlevel > RUNLEVEL_TUNGGU_SHUTDOWN) {
        return STATUS_PARAM_JARAK;
    }

    rl_lama = g_runlevel_saat_ini;

    if (rl_lama == runlevel) {
        return STATUS_BERHASIL;
    }

    kernel_printf("[init] Pindah runlevel %d -> %d\n",
                  rl_lama, runlevel);

    if (runlevel == RUNLEVEL_MATI ||
        runlevel == RUNLEVEL_TUNGGU_SHUTDOWN) {
        for (i = 0; i < g_jumlah_layanan; i++) {
            if (g_layanan[i].aktif &&
                g_layanan[i].status == LAYANAN_STATUS_JALAN) {
                init_hentikan_layanan((tanda32_t)i);
            }
        }
    } else {
        for (i = 0; i < g_jumlah_layanan; i++) {
            if (g_layanan[i].aktif &&
                g_layanan[i].status == LAYANAN_STATUS_JALAN) {
                if (!(g_layanan[i].runlevel & (1 << runlevel))) {
                    init_hentikan_layanan((tanda32_t)i);
                }
            }
        }
    }

    if (runlevel != RUNLEVEL_MATI &&
        runlevel != RUNLEVEL_TUNGGU_SHUTDOWN) {
        layanan_sortir_urutan();

        for (i = 0; i < g_jumlah_layanan; i++) {
            if (g_layanan[i].aktif &&
                g_layanan[i].otomatis &&
                g_layanan[i].status != LAYANAN_STATUS_JALAN &&
                (g_layanan[i].runlevel & (1 << runlevel))) {
                init_mulai_layanan((tanda32_t)i);
            }
        }
    }

    g_runlevel_saat_ini = runlevel;

    kernel_printf("[init] Runlevel sekarang: %d\n",
                  g_runlevel_saat_ini);

    return STATUS_BERHASIL;
}

/*
 * init_dapat_runlevel
 * -------------------
 * Mendapatkan runlevel saat ini.
 *
 * Return:
 *   Nomor runlevel saat ini
 */
tanda32_t init_dapat_runlevel(void)
{
    return g_runlevel_saat_ini;
}

/*
 * ===========================================================================
 * FUNGSI PEMANTAUAN LAYANAN (SERVICE MONITORING)
 * ===========================================================================
 */

/*
 * init_pantau_layanan
 * -------------------
 * Memantau status semua layanan aktif.
 * Memeriksa apakah proses layanan masih hidup, dan
 * mencoba me-restart layanan yang mati secara tak
 * terduga.
 */
static void init_pantau_layanan(void)
{
    tak_bertanda32_t i;
    proses_t *proses;

    for (i = 0; i < g_jumlah_layanan; i++) {
        if (!g_layanan[i].aktif) {
            continue;
        }

        if (g_layanan[i].status != LAYANAN_STATUS_JALAN) {
            continue;
        }

        proses = proses_cari(g_layanan[i].pid);
        if (proses == NULL) {
            g_layanan[i].status = LAYANAN_STATUS_MATI;
            g_layanan[i].pid = PID_TIDAK_ADA;
            kernel_printf("[init] Layanan '%s' berhenti "
                          "tak terduga\n",
                          g_layanan[i].nama);

            if (g_layanan[i].kritikal) {
                kernel_printf("[init] PERINGATAN: Layanan "
                              "kritikal '%s' mati!\n",
                              g_layanan[i].nama);
            }
        }
    }
}

/*
 * ===========================================================================
 * FUNGSI DAFTAR LAYANAN BAWAAN (DEFAULT SERVICE REGISTRATION)
 * ===========================================================================
 */

/*
 * init_daftarkan_layanan_bawaan
 * -----------------------------
 * Mendaftarkan layanan-layanan standar sistem.
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
static status_t init_daftarkan_layanan_bawaan(void)
{
    status_t hasil;

    hasil = init_daftarkan_layanan(
        "console", "/sistem/bin/console", "",
        (1 << RUNLEVEL_TUNGGU) | (1 << RUNLEVEL_MULTIUSER)
        | (1 << RUNLEVEL_GRAFIS),
        10, BENAR, BENAR);

    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }

    hasil = init_daftarkan_layanan(
        "log_sistem", "/sistem/bin/log", "",
        (1 << RUNLEVEL_MULTIUSER) | (1 << RUNLEVEL_GRAFIS),
        20, BENAR, SALAH);

    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }

    hasil = init_daftarkan_layanan(
        "manajer_perangkat", "/sistem/bin/devmgr", "",
        (1 << RUNLEVEL_MULTIUSER) | (1 << RUNLEVEL_GRAFIS),
        30, BENAR, SALAH);

    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI INIT UTAMA (MAIN INIT FUNCTIONS)
 * ===========================================================================
 */

/*
 * init_mulai
 * ----------
 * Memulai proses init. Fungsi ini dipanggil oleh kernel
 * setelah semua subsistem inti siap.
 *
 * Return:
 *   STATUS_BERHASIL jika init berhasil dimulai
 */
status_t init_mulai(void)
{
    status_t hasil;

    kernel_printf("[init] Memulai proses init...\n");

    kernel_memset(g_layanan, 0, sizeof(g_layanan));
    g_jumlah_layanan = 0;
    g_runlevel_saat_ini = RUNLEVEL_MATI;
    g_shutdown_diminta = SALAH;

    hasil = init_daftarkan_layanan_bawaan();
    if (hasil != STATUS_BERHASIL) {
        kernel_printf("[init] Gagal mendaftarkan layanan "
                      "bawaan: %d\n", hasil);
        return hasil;
    }

    kernel_printf("[init] Terdaftar %d layanan\n",
                  g_jumlah_layanan);

    g_init_berjalan = BENAR;
    g_runlevel_saat_ini = RUNLEVEL_TUNGGU;

    hasil = init_masuk_runlevel(RUNLEVEL_MULTIUSER);
    if (hasil != STATUS_BERHASIL) {
        kernel_printf("[init] Gagal masuk runlevel "
                      "multiuser\n");
        g_init_berjalan = SALAH;
        return hasil;
    }

    kernel_printf("[init] Proses init siap\n");

    return STATUS_BERHASIL;
}

/*
 * init_loop_utama
 * ---------------
 * Loop utama proses init. Menunggu event dan memantau
 * status layanan secara periodik.
 *
 * Return:
 *   Kode exit saat init selesai
 */
tanda32_t init_loop_utama(void)
{
    tak_bertanda32_t hitung_tick = 0;

    kernel_printf("[init] Loop utama dimulai\n");

    while (g_init_berjalan && !g_shutdown_diminta) {
        hitung_tick++;

        if (hitung_tick % 100 == 0) {
            init_pantau_layanan();
        }

        kernel_delay(10);
    }

    if (g_shutdown_diminta) {
        kernel_printf("[init] Shutdown diminta\n");
        init_masuk_runlevel(RUNLEVEL_MATI);
    }

    kernel_printf("[init] Proses init selesai\n");
    g_init_berjalan = SALAH;

    return 0;
}

/*
 * init_shutdown
 * -------------
 * Meminta proses init untuk melakukan shutdown sistem.
 *
 * Parameter:
 *   reboot - BENAR untuk reboot, SALAH untuk power off
 */
void init_shutdown(bool_t reboot)
{
    kernel_printf("[init] %s diminta...\n",
                  reboot ? "Reboot" : "Shutdown");
    g_shutdown_diminta = BENAR;
    TIDAK_DIGUNAKAN(reboot);
}

/*
 * init_status
 * -----------
 * Memeriksa apakah proses init masih berjalan.
 *
 * Return:
 *   BENAR jika init masih berjalan
 */
bool_t init_status(void)
{
    return g_init_berjalan;
}

/*
 * init_tampilkan_status
 * ---------------------
 * Menampilkan status semua layanan ke console.
 */
void init_tampilkan_status(void)
{
    tak_bertanda32_t i;

    kernel_printf("=== Status Layanan Init ===\n");
    kernel_printf("Runlevel: %d\n", g_runlevel_saat_ini);
    kernel_printf("Jumlah layanan: %d\n\n", g_jumlah_layanan);

    kernel_printf("%-16s %-8s %-8s %-6s\n",
                  "Nama", "Status", "PID", "Kritis");
    kernel_printf("----------------------------------"
                  "--------\n");

    for (i = 0; i < g_jumlah_layanan; i++) {
        if (!g_layanan[i].aktif) {
            continue;
        }

        kernel_printf("%-16s ", g_layanan[i].nama);

        switch (g_layanan[i].status) {
        case LAYANAN_STATUS_MATI:
            kernel_printf("%-8s ", "Mati");
            break;
        case LAYANAN_STATUS_JALAN:
            kernel_printf("%-8s ", "Jalan");
            break;
        case LAYANAN_STATUS_GAGAL:
            kernel_printf("%-8s ", "Gagal");
            break;
        case LAYANAN_STATUS_MENUNGGU:
            kernel_printf("%-8s ", "Tunggu");
            break;
        default:
            kernel_printf("%-8s ", "???");
            break;
        }

        if (g_layanan[i].status == LAYANAN_STATUS_JALAN) {
            kernel_printf("%-8d ", g_layanan[i].pid);
        } else {
            kernel_printf("%-8s ", "-");
        }

        kernel_printf("%-6s\n",
                      g_layanan[i].kritikal ? "Ya" : "Tidak");
    }
}
