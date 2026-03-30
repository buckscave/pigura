/*
 * PIGURA OS - LOGIN.C
 * ====================
 * Sistem autentikasi login untuk Pigura OS.
 *
 * Berkas ini mengimplementasikan fungsi-fungsi autentikasi pengguna
 * termasuk verifikasi nama pengguna dan kata sandi, manajemen sesi,
 * pembatasan percobaan login, dan riwayat login.
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

/* Jumlah maksimum pengguna yang terdaftar */
#define LOGIN_PENGGUNA_MAKS          16

/* Panjang maksimum kata sandi (hash) */
#define LOGIN_SANDI_PANJANG_MAKS     SISTEM_SANDI_HASH_PANJANG

/* Jumlah maksimum percobaan login */
#define LOGIN_PERCOBAAN_MAKS         5

/* Durasi kunci akun setelah gagal (detik) */
#define LOGIN_KUNCI_DURASI           60

/* Panjang maksimum nama pengguna */
#define LOGIN_NAMA_PANJANG           SISTEM_NAMA_PANJANG

/* Panjang maksimum shell pengguna */
#define LOGIN_SHELL_PANJANG          SISTEM_SHELL_PANJANG

/* Panjang maksimum direktori home */
#define LOGIN_HOME_PANJANG           SISTEM_HOME_PANJANG

/* Hash seed konstan untuk determinisme */
#define LOGIN_HASH_SEED              0x50494755UL

/*
 * ===========================================================================
 * STRUKTUR PENGGUNA (USER STRUCTURE)
 * ===========================================================================
 */

/* Struktur data satu pengguna */
typedef struct {
    char nama[LOGIN_NAMA_PANJANG];       /* Nama pengguna */
    char sandi_hash[LOGIN_SANDI_PANJANG_MAKS]; /* Hash sandi */
    uid_t uid;                            /* User ID */
    gid_t gid;                            /* Group ID */
    char shell[LOGIN_SHELL_PANJANG];      /* Shell default */
    char home[LOGIN_HOME_PANJANG];        /* Home directory */
    bool_t aktif;                         /* Akun aktif */
    bool_t terkunci;                      /* Akun terkunci */
} pengguna_t;

/*
 * ===========================================================================
 * STRUKTUR SESI (SESSION STRUCTURE)
 * ===========================================================================
 */

/* Struktur sesi login aktif */
typedef struct {
    char nama[LOGIN_NAMA_PANJANG];   /* Nama pengguna */
    uid_t uid;                       /* User ID */
    pid_t pid;                       /* PID proses */
    waktu_t waktu_mulai;             /* Waktu login */
    bool_t aktif;                    /* Sesi aktif */
} sesi_t;

/*
 * ===========================================================================
 * VARIABEL GLOBAL STATIS (STATIC GLOBAL VARIABLES)
 * ===========================================================================
 */

/* Tabel pengguna terdaftar */
static pengguna_t g_pengguna[LOGIN_PENGGUNA_MAKS];

/* Jumlah pengguna terdaftar */
static tak_bertanda32_t g_jumlah_pengguna = 0;

/* Sesi saat ini */
static sesi_t g_sesi_saat_ini;

/* Jumlah percobaan login gagal */
static tak_bertanda32_t g_percobaan_gagal = 0;

/* Waktu kunci berakhir (epoch) */
static waktu_t g_waktu_kunci = 0;

/* Indeks pengguna root (UID 0) */
static tanda32_t g_index_root = -1;

/*
 * ===========================================================================
 * FUNGSI HASH SANDI (PASSWORD HASH FUNCTIONS)
 * ===========================================================================
 */

/*
 * login_hash_sandi
 * ----------------
 * Menghitung hash sederhana dari kata sandi menggunakan
 * algoritma djb2 dengan seed. Hash ini ditujukan untuk
 * autentikasi lokal dalam lingkungan OS kustom.
 *
 * Parameter:
 *   sandi  - Kata sandi asli
 *   hash   - Buffer output hash (panjang LOGIN_SANDI_PANJANG_MAKS)
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
static status_t login_hash_sandi(const char *sandi, char *hash)
{
    tak_bertanda32_t h;
    ukuran_t i;
    ukuran_t panjang;

    if (sandi == NULL || hash == NULL) {
        return STATUS_PARAM_NULL;
    }

    panjang = kernel_strlen(sandi);
    if (panjang == 0) {
        return STATUS_PARAM_UKURAN;
    }

    h = LOGIN_HASH_SEED;

    for (i = 0; i < panjang; i++) {
        h = ((h << 5) + h) + (tak_bertanda32_t)sandi[i];
        h ^= (h >> 16);
        h *= 0x85ebca6bUL;
        h ^= (h >> 13);
    }

    kernel_snprintf(hash, LOGIN_SANDI_PANJANG_MAKS, "%08x", h);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PEMBANTU (HELPER FUNCTIONS)
 * ===========================================================================
 */

/*
 * login_cari_index
 * ----------------
 * Mencari index pengguna berdasarkan nama.
 *
 * Parameter:
 *   nama - Nama pengguna
 *
 * Return:
 *   Index jika ditemukan, -1 jika tidak
 */
static tanda32_t login_cari_index(const char *nama)
{
    tak_bertanda32_t i;

    if (nama == NULL) {
        return -1;
    }

    for (i = 0; i < g_jumlah_pengguna; i++) {
        if (g_pengguna[i].aktif &&
            kernel_strcmp(g_pengguna[i].nama, nama) == 0) {
            return (tanda32_t)i;
        }
    }

    return -1;
}

/*
 * login_cari_slot_kosong
 * -----------------------
 * Mencari slot kosong dalam tabel pengguna.
 *
 * Return:
 *   Index slot kosong, -1 jika penuh
 */
static tanda32_t login_cari_slot_kosong(void)
{
    tak_bertanda32_t i;

    for (i = 0; i < LOGIN_PENGGUNA_MAKS; i++) {
        if (!g_pengguna[i].aktif) {
            return (tanda32_t)i;
        }
    }

    return -1;
}

/*
 * ===========================================================================
 * FUNGSI MANAJEMEN PENGGUNA (USER MANAGEMENT FUNCTIONS)
 * ===========================================================================
 */

/*
 * login_tambah_pengguna
 * ---------------------
 * Menambahkan pengguna baru ke sistem.
 *
 * Parameter:
 *   nama   - Nama pengguna (unik)
 *   sandi  - Kata sandi
 *   uid    - User ID
 *   gid    - Group ID
 *   shell  - Path shell default
 *   home   - Path home directory
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t login_tambah_pengguna(const char *nama, const char *sandi,
                                uid_t uid, gid_t gid,
                                const char *shell, const char *home)
{
    tanda32_t index;
    status_t hasil;

    if (nama == NULL || sandi == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (kernel_strlen(nama) == 0 ||
        kernel_strlen(nama) >= LOGIN_NAMA_PANJANG) {
        return STATUS_PARAM_UKURAN;
    }

    if (login_cari_index(nama) >= 0) {
        return STATUS_SUDAH_ADA;
    }

    index = login_cari_slot_kosong();
    if (index < 0) {
        return STATUS_PENUH;
    }

    kernel_memset(&g_pengguna[index], 0, sizeof(pengguna_t));

    kernel_strncpy(g_pengguna[index].nama, nama,
                   LOGIN_NAMA_PANJANG - 1);

    hasil = login_hash_sandi(sandi, g_pengguna[index].sandi_hash);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }

    g_pengguna[index].uid = uid;
    g_pengguna[index].gid = gid;

    if (shell != NULL) {
        kernel_strncpy(g_pengguna[index].shell, shell,
                       LOGIN_SHELL_PANJANG - 1);
    } else {
        kernel_strncpy(g_pengguna[index].shell, "/sistem/bin/shell",
                       LOGIN_SHELL_PANJANG - 1);
    }

    if (home != NULL) {
        kernel_strncpy(g_pengguna[index].home, home,
                       LOGIN_HOME_PANJANG - 1);
    } else {
        kernel_snprintf(g_pengguna[index].home,
                        LOGIN_HOME_PANJANG, "/rumah/%s", nama);
    }

    g_pengguna[index].aktif = BENAR;
    g_pengguna[index].terkunci = SALAH;

    g_jumlah_pengguna++;

    if (uid == 0 && g_index_root < 0) {
        g_index_root = index;
    }

    return STATUS_BERHASIL;
}

/*
 * login_hapus_pengguna
 * --------------------
 * Menghapus pengguna dari sistem.
 *
 * Parameter:
 *   nama - Nama pengguna
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t login_hapus_pengguna(const char *nama)
{
    tanda32_t index;

    if (nama == NULL) {
        return STATUS_PARAM_NULL;
    }

    index = login_cari_index(nama);
    if (index < 0) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    if (g_pengguna[index].uid == 0) {
        return STATUS_AKSES_DITOLAK;
    }

    kernel_memset(&g_pengguna[index], 0, sizeof(pengguna_t));
    g_jumlah_pengguna--;

    return STATUS_BERHASIL;
}

/*
 * login_ubah_sandi
 * ----------------
 * Mengubah kata sandi pengguna.
 *
 * Parameter:
 *   nama       - Nama pengguna
 *   sandi_lama - Kata sandi lama
 *   sandi_baru - Kata sandi baru
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t login_ubah_sandi(const char *nama, const char *sandi_lama,
                           const char *sandi_baru)
{
    tanda32_t index;
    char hash_lama[LOGIN_SANDI_PANJANG_MAKS];
    status_t hasil;

    if (nama == NULL || sandi_lama == NULL || sandi_baru == NULL) {
        return STATUS_PARAM_NULL;
    }

    index = login_cari_index(nama);
    if (index < 0) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    hasil = login_hash_sandi(sandi_lama, hash_lama);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }

    if (kernel_strcmp(hash_lama,
                      g_pengguna[index].sandi_hash) != 0) {
        return STATUS_AKSES_DITOLAK;
    }

    if (kernel_strlen(sandi_baru) == 0) {
        return STATUS_PARAM_UKURAN;
    }

    hasil = login_hash_sandi(sandi_baru,
                             g_pengguna[index].sandi_hash);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI AUTENTIKASI (AUTHENTICATION FUNCTIONS)
 * ===========================================================================
 */

/*
 * login_autentikasi
 * -----------------
 * Melakukan autentikasi pengguna dengan nama dan kata sandi.
 * Menerapkan pembatasan percobaan dan penguncian akun.
 *
 * Parameter:
 *   nama  - Nama pengguna
 *   sandi - Kata sandi
 *
 * Return:
 *   STATUS_BERHASIL jika autentikasi berhasil
 */
status_t login_autentikasi(const char *nama, const char *sandi)
{
    tanda32_t index;
    char hash_input[LOGIN_SANDI_PANJANG_MAKS];
    status_t hasil;
    waktu_t waktu_skrg;
    tak_bertanda64_t uptime_skrg;

    if (nama == NULL || sandi == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (kernel_strlen(nama) == 0 || kernel_strlen(sandi) == 0) {
        g_percobaan_gagal++;
        return STATUS_PARAM_INVALID;
    }

    uptime_skrg = kernel_get_uptime();
    waktu_skrg = (waktu_t)uptime_skrg;

    if (g_percobaan_gagal >= LOGIN_PERCOBAAN_MAKS) {
        if (waktu_skrg < g_waktu_kunci) {
            return STATUS_BUSY;
        }
        g_percobaan_gagal = 0;
    }

    index = login_cari_index(nama);
    if (index < 0) {
        g_percobaan_gagal++;
        return STATUS_TIDAK_DITEMUKAN;
    }

    if (g_pengguna[index].terkunci) {
        return STATUS_AKSES_DITOLAK;
    }

    if (!g_pengguna[index].aktif) {
        return STATUS_AKSES_DITOLAK;
    }

    hasil = login_hash_sandi(sandi, hash_input);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }

    if (kernel_strcmp(hash_input,
                      g_pengguna[index].sandi_hash) != 0) {
        g_percobaan_gagal++;

        if (g_percobaan_gagal >= LOGIN_PERCOBAAN_MAKS) {
            g_pengguna[index].terkunci = BENAR;
            g_waktu_kunci = waktu_skrg + LOGIN_KUNCI_DURASI;
        }

        return STATUS_AKSES_DITOLAK;
    }

    g_percobaan_gagal = 0;

    kernel_memset(&g_sesi_saat_ini, 0, sizeof(sesi_t));
    kernel_strncpy(g_sesi_saat_ini.nama, nama,
                   LOGIN_NAMA_PANJANG - 1);
    g_sesi_saat_ini.uid = g_pengguna[index].uid;
    g_sesi_saat_ini.pid = proses_dapat_saat_ini() != NULL
                          ? proses_dapat_saat_ini()->pid : 0;
    g_sesi_saat_ini.waktu_mulai = waktu_skrg;
    g_sesi_saat_ini.aktif = BENAR;

    return STATUS_BERHASIL;
}

/*
 * login_logout
 * ------------
 * Mengakhiri sesi login saat ini.
 */
void login_logout(void)
{
    g_sesi_saat_ini.aktif = SALAH;
    kernel_memset(&g_sesi_saat_ini, 0, sizeof(sesi_t));
}

/*
 * ===========================================================================
 * FUNGSI QUERY SESI (SESSION QUERY FUNCTIONS)
 * ===========================================================================
 */

/*
 * login_sesi_aktif
 * ----------------
 * Memeriksa apakah ada sesi login aktif.
 *
 * Return:
 *   BENAR jika sesi aktif
 */
bool_t login_sesi_aktif(void)
{
    return g_sesi_saat_ini.aktif;
}

/*
 * login_dapat_uid_saat_ini
 * -------------------------
 * Mendapatkan UID pengguna yang sedang login.
 *
 * Return:
 *   UID pengguna saat ini, atau UID_INVALID jika tidak login
 */
uid_t login_dapat_uid_saat_ini(void)
{
    if (!g_sesi_saat_ini.aktif) {
        return UID_INVALID;
    }

    return g_sesi_saat_ini.uid;
}

/*
 * login_dapat_gid_saat_ini
 * -------------------------
 * Mendapatkan GID pengguna yang sedang login.
 *
 * Return:
 *   GID pengguna saat ini, atau GID_INVALID jika tidak login
 */
gid_t login_dapat_gid_saat_ini(void)
{
    tanda32_t index;

    if (!g_sesi_saat_ini.aktif) {
        return GID_INVALID;
    }

    index = login_cari_index(g_sesi_saat_ini.nama);
    if (index < 0) {
        return GID_INVALID;
    }

    return g_pengguna[index].gid;
}

/*
 * ===========================================================================
 * FUNGSI INISIALISASI (INITIALIZATION)
 * ===========================================================================
 */

/*
 * login_inisialisasi
 * ------------------
 * Menginisialisasi sistem login dan membuat akun root bawaan.
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t login_inisialisasi(void)
{
    status_t hasil;

    kernel_memset(g_pengguna, 0, sizeof(g_pengguna));
    g_jumlah_pengguna = 0;
    g_percobaan_gagal = 0;
    g_waktu_kunci = 0;
    g_index_root = -1;

    kernel_memset(&g_sesi_saat_ini, 0, sizeof(sesi_t));

    hasil = login_tambah_pengguna(
        "root", "pigura", 0, 0,
        "/sistem/bin/shell", "/rumah/root");

    if (hasil != STATUS_BERHASIL) {
        kernel_printf("[login] Gagal membuat akun root: %d\n",
                      hasil);
        return hasil;
    }

    kernel_printf("[login] Sistem login diinisialisasi\n");
    kernel_printf("[login] Terdaftar %d pengguna\n",
                  g_jumlah_pengguna);

    return STATUS_BERHASIL;
}

/*
 * login_tampilkan_info
 * --------------------
 * Menampilkan informasi sesi login saat ini.
 */
void login_tampilkan_info(void)
{
    if (!g_sesi_saat_ini.aktif) {
        kernel_puts("Belum ada sesi login aktif\n");
        return;
    }

    kernel_puts("=== Sesi Login ===\n");
    kernel_puts("Pengguna : ");
    kernel_puts(g_sesi_saat_ini.nama);
    kernel_puts("\n");

    kernel_printf("UID      : %u\n",
                  (unsigned)g_sesi_saat_ini.uid);
    kernel_printf("PID      : %u\n",
                  (unsigned)g_sesi_saat_ini.pid);

    kernel_printf("Jumlah pengguna terdaftar: %u\n",
                  g_jumlah_pengguna);

    if (g_percobaan_gagal > 0) {
        kernel_printf("Percobaan gagal: %u dari %u\n",
                      g_percobaan_gagal, LOGIN_PERCOBAAN_MAKS);
    }
}
