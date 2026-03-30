/*
 * PIGURA OS - PROFIL.C
 * =====================
 * Manajemen profil pengguna Pigura OS.
 *
 * Berkas ini mengimplementasikan penyimpanan dan pengelolaan profil
 * pengguna, termasuk preferensi personal, informasi akun, batasan
 * sumber daya, dan riwayat sesi login.
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

/* Jumlah maksimum profil pengguna */
#define PROFIL_MAKS                 16

/* Panjang maksimum nama tampilan */
#define PROFIL_NAMA_TAMPILAN_MAKS   64

/* Panjang maksimum path foto profil */
#define PROFIL_FOTO_MAKS            128

/* Panjang maksimum alamat email */
#define PROFIL_EMAIL_MAKS           64

/* Jumlah maksimum entri riwayat sesi */
#define PROFIL_RIWAYAT_MAKS         16

/* Panjang maksimum zona waktu */
#define PROFIL_ZONA_WAKTU_MAKS      32

/* Jumlah maksimum entri preferensi */
#define PROFIL_PREFERENSI_MAKS      32

/* Panjang kunci dan nilai preferensi */
#define PROFIL_KUNCI_MAKS           48
#define PROFIL_NILAI_MAKS           128

/*
 * ===========================================================================
 * STRUKTUR PREFERENSI (PREFERENCE STRUCTURE)
 * ===========================================================================
 */

/* Satu entri preferensi pengguna */
typedef struct {
    char kunci[PROFIL_KUNCI_MAKS];    /* Kunci preferensi */
    char nilai[PROFIL_NILAI_MAKS];    /* Nilai preferensi */
    bool_t aktif;                      /* Apakah aktif */
} preferensi_t;

/*
 * ===========================================================================
 * STRUKTUR RIWAYAT SESI (SESSION HISTORY)
 * ===========================================================================
 */

/* Entri riwayat sesi login */
typedef struct {
    waktu_t waktu_login;               /* Waktu login */
    waktu_t waktu_logout;              /* Waktu logout */
    tanda32_t durasi;                   /* Durasi sesi (detik) */
    char metode[16];                   /* Metode login */
    bool_t berhasil;                   /* Login berhasil */
} riwayat_sesi_t;

/*
 * ===========================================================================
 * STRUKTUR BATASAN SUMBER DAYA (RESOURCE LIMIT)
 * ===========================================================================
 */

/* Batasan sumber daya per pengguna */
typedef struct {
    tak_bertanda64_t memori_maks;      /* Batas memori (byte) */
    tak_bertanda32_t proses_maks;      /* Batas proses */
    tak_bertanda32_t file_maks;        /* Batas file terbuka */
    tak_bertanda64_t cpu_maks;         /* Batas waktu CPU (detik) */
    ukuran_t stack_maks;               /* Batas ukuran stack */
    ukuran_t inti_maks;                /* Batas ukuran core dump */
} batasan_sumber_daya_t;

/*
 * ===========================================================================
 * STRUKTUR PROFIL PENGGUNA (USER PROFILE)
 * ===========================================================================
 */

/* Struktur profil lengkap pengguna */
typedef struct {
    uid_t uid;                              /* User ID */
    char nama_tampilan[PROFIL_NAMA_TAMPILAN_MAKS]; /* Nama tampilan */
    char foto[PROFIL_FOTO_MAKS];            /* Path foto profil */
    char email[PROFIL_EMAIL_MAKS];          /* Alamat email */
    char zona_waktu[PROFIL_ZONA_WAKTU_MAKS]; /* Zona waktu */
    char shell[SISTEM_SHELL_PANJANG];       /* Shell default */
    char home[SISTEM_HOME_PANJANG];         /* Home directory */

    preferensi_t preferensi[PROFIL_PREFERENSI_MAKS];
    tak_bertanda32_t jumlah_preferensi;

    riwayat_sesi_t riwayat[PROFIL_RIWAYAT_MAKS];
    tak_bertanda32_t jumlah_riwayat;
    tak_bertanda32_t riwayat_pos;

    batasan_sumber_daya_t batasan;

    bool_t aktif;                           /* Profil aktif */
    waktu_t waktu_buat;                     /* Waktu pembuatan */
    waktu_t waktu_ubah;                     /* Waktu perubahan */
} profil_t;

/*
 * ===========================================================================
 * VARIABEL GLOBAL STATIS (STATIC GLOBAL VARIABLES)
 * ===========================================================================
 */

/* Tabel profil pengguna */
static profil_t g_profil[PROFIL_MAKS];

/* Jumlah profil aktif */
static tak_bertanda32_t g_jumlah_profil = 0;

/* UID profil yang sedang aktif */
static uid_t g_uid_saat_ini = UID_INVALID;

/*
 * ===========================================================================
 * FUNGSI PEMBANTU (HELPER FUNCTIONS)
 * ===========================================================================
 */

/*
 * profil_cari_index
 * -----------------
 * Mencari index profil berdasarkan UID.
 *
 * Parameter:
 *   uid - User ID
 *
 * Return:
 *   Index jika ditemukan, -1 jika tidak
 */
static tanda32_t profil_cari_index(uid_t uid)
{
    tak_bertanda32_t i;

    for (i = 0; i < g_jumlah_profil; i++) {
        if (g_profil[i].aktif && g_profil[i].uid == uid) {
            return (tanda32_t)i;
        }
    }

    return -1;
}

/*
 * profil_cari_slot_kosong
 * ------------------------
 * Mencari slot kosong dalam tabel profil.
 *
 * Return:
 *   Index slot kosong, -1 jika penuh
 */
static tanda32_t profil_cari_slot_kosong(void)
{
    tak_bertanda32_t i;

    for (i = 0; i < PROFIL_MAKS; i++) {
        if (!g_profil[i].aktif) {
            return (tanda32_t)i;
        }
    }

    return -1;
}

/*
 * profil_buat_batasan_default
 * ---------------------------
 * Membuat batasan sumber daya default.
 *
 * Parameter:
 *   batasan - Pointer ke struktur batasan
 */
static void profil_buat_batasan_default(
    batasan_sumber_daya_t *batasan)
{
    if (batasan == NULL) {
        return;
    }

    kernel_memset(batasan, 0, sizeof(batasan_sumber_daya_t));
    batasan->memori_maks = (tak_bertanda64_t)512 * 1024 * 1024;
    batasan->proses_maks = 64;
    batasan->file_maks = 128;
    batasan->cpu_maks = 0;
    batasan->stack_maks = CONFIG_STACK_USER;
    batasan->inti_maks = 0;
}

/*
 * ===========================================================================
 * FUNGSI MANAJEMEN PROFIL (PROFILE MANAGEMENT)
 * ===========================================================================
 */

/*
 * profil_buat
 * -----------
 * Membuat profil pengguna baru.
 *
 * Parameter:
 *   uid           - User ID
 *   nama_tampilan - Nama tampilan pengguna
 *   shell         - Shell default
 *   home          - Home directory
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t profil_buat(uid_t uid, const char *nama_tampilan,
                      const char *shell, const char *home)
{
    tanda32_t index;
    waktu_t waktu_skrg;

    if (nama_tampilan == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (profil_cari_index(uid) >= 0) {
        return STATUS_SUDAH_ADA;
    }

    index = profil_cari_slot_kosong();
    if (index < 0) {
        return STATUS_PENUH;
    }

    kernel_memset(&g_profil[index], 0, sizeof(profil_t));

    g_profil[index].uid = uid;
    kernel_strncpy(g_profil[index].nama_tampilan, nama_tampilan,
                   PROFIL_NAMA_TAMPILAN_MAKS - 1);

    if (shell != NULL) {
        kernel_strncpy(g_profil[index].shell, shell,
                       SISTEM_SHELL_PANJANG - 1);
    } else {
        kernel_strncpy(g_profil[index].shell, "/sistem/bin/shell",
                       SISTEM_SHELL_PANJANG - 1);
    }

    if (home != NULL) {
        kernel_strncpy(g_profil[index].home, home,
                       SISTEM_HOME_PANJANG - 1);
    } else {
        kernel_snprintf(g_profil[index].home,
                        SISTEM_HOME_PANJANG, "/rumah/%u",
                        (unsigned)uid);
    }

    kernel_strncpy(g_profil[index].zona_waktu, CONFIG_TIMEZONE,
                   PROFIL_ZONA_WAKTU_MAKS - 1);

    g_profil[index].jumlah_preferensi = 0;
    g_profil[index].jumlah_riwayat = 0;
    g_profil[index].riwayat_pos = 0;

    profil_buat_batasan_default(&g_profil[index].batasan);

    waktu_skrg = (waktu_t)kernel_get_uptime();
    g_profil[index].waktu_buat = waktu_skrg;
    g_profil[index].waktu_ubah = waktu_skrg;

    g_profil[index].aktif = BENAR;
    g_jumlah_profil++;

    return STATUS_BERHASIL;
}

/*
 * profil_hapus
 * ------------
 * Menghapus profil pengguna.
 *
 * Parameter:
 *   uid - User ID
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t profil_hapus(uid_t uid)
{
    tanda32_t index;

    index = profil_cari_index(uid);
    if (index < 0) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    if (uid == 0) {
        return STATUS_AKSES_DITOLAK;
    }

    kernel_memset(&g_profil[index], 0, sizeof(profil_t));
    g_jumlah_profil--;

    if (g_uid_saat_ini == uid) {
        g_uid_saat_ini = UID_INVALID;
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PREFERENSI (PREFERENCE MANAGEMENT)
 * ===========================================================================
 */

/*
 * profil_set_preferensi
 * ---------------------
 * Mengatur preferensi pengguna.
 *
 * Parameter:
 *   uid   - User ID
 *   kunci - Kunci preferensi
 *   nilai - Nilai preferensi
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t profil_set_preferensi(uid_t uid, const char *kunci,
                                const char *nilai)
{
    tanda32_t index;
    tak_bertanda32_t i;
    profil_t *p;

    if (kunci == NULL || nilai == NULL) {
        return STATUS_PARAM_NULL;
    }

    index = profil_cari_index(uid);
    if (index < 0) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    p = &g_profil[index];

    for (i = 0; i < p->jumlah_preferensi; i++) {
        if (p->preferensi[i].aktif &&
            kernel_strcmp(p->preferensi[i].kunci, kunci) == 0) {
            kernel_strncpy(p->preferensi[i].nilai, nilai,
                           PROFIL_NILAI_MAKS - 1);
            p->waktu_ubah = (waktu_t)kernel_get_uptime();
            return STATUS_BERHASIL;
        }
    }

    if (p->jumlah_preferensi >= PROFIL_PREFERENSI_MAKS) {
        return STATUS_PENUH;
    }

    for (i = 0; i < PROFIL_PREFERENSI_MAKS; i++) {
        if (!p->preferensi[i].aktif) {
            kernel_strncpy(p->preferensi[i].kunci, kunci,
                           PROFIL_KUNCI_MAKS - 1);
            kernel_strncpy(p->preferensi[i].nilai, nilai,
                           PROFIL_NILAI_MAKS - 1);
            p->preferensi[i].aktif = BENAR;
            p->jumlah_preferensi++;
            p->waktu_ubah = (waktu_t)kernel_get_uptime();
            return STATUS_BERHASIL;
        }
    }

    return STATUS_PENUH;
}

/*
 * profil_ambil_preferensi
 * -----------------------
 * Mengambil preferensi pengguna.
 *
 * Parameter:
 *   uid    - User ID
 *   kunci  - Kunci preferensi
 *   nilai  - Buffer output
 *   ukuran - Ukuran buffer
 *
 * Return:
 *   STATUS_BERHASIL jika ditemukan
 */
status_t profil_ambil_preferensi(uid_t uid, const char *kunci,
                                  char *nilai, ukuran_t ukuran)
{
    tanda32_t index;
    tak_bertanda32_t i;
    profil_t *p;

    if (kunci == NULL || nilai == NULL || ukuran == 0) {
        return STATUS_PARAM_NULL;
    }

    index = profil_cari_index(uid);
    if (index < 0) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    p = &g_profil[index];

    for (i = 0; i < p->jumlah_preferensi; i++) {
        if (p->preferensi[i].aktif &&
            kernel_strcmp(p->preferensi[i].kunci, kunci) == 0) {
            kernel_strncpy(nilai, p->preferensi[i].nilai,
                           ukuran - 1);
            nilai[ukuran - 1] = '\0';
            return STATUS_BERHASIL;
        }
    }

    return STATUS_TIDAK_DITEMUKAN;
}

/*
 * ===========================================================================
 * FUNGSI RIWAYAT SESI (SESSION HISTORY MANAGEMENT)
 * ===========================================================================
 */

/*
 * profil_tambah_riwayat
 * ---------------------
 * Menambahkan entri riwayat sesi login.
 *
 * Parameter:
 *   uid       - User ID
 *   berhasil  - Apakah login berhasil
 *   metode    - Metode autentikasi
 */
void profil_tambah_riwayat(uid_t uid, bool_t berhasil,
                            const char *metode)
{
    tanda32_t index;
    profil_t *p;
    riwayat_sesi_t *riwayat;

    index = profil_cari_index(uid);
    if (index < 0) {
        return;
    }

    p = &g_profil[index];

    if (p->jumlah_riwayat < PROFIL_RIWAYAT_MAKS) {
        riwayat = &p->riwayat[p->riwayat_pos];
        p->jumlah_riwayat++;
    } else {
        riwayat = &p->riwayat[p->riwayat_pos];
    }

    kernel_memset(riwayat, 0, sizeof(riwayat_sesi_t));
    riwayat->waktu_login = (waktu_t)kernel_get_uptime();
    riwayat->waktu_logout = 0;
    riwayat->durasi = 0;
    riwayat->berhasil = berhasil;

    if (metode != NULL) {
        kernel_strncpy(riwayat->metode, metode,
                       sizeof(riwayat->metode) - 1);
    } else {
        kernel_strncpy(riwayat->metode, "lokal",
                       sizeof(riwayat->metode) - 1);
    }

    p->riwayat_pos = (p->riwayat_pos + 1) % PROFIL_RIWAYAT_MAKS;
}

/*
 * ===========================================================================
 * FUNGSI INFORMASI PROFIL (PROFILE INFO FUNCTIONS)
 * ===========================================================================
 */

/*
 * profil_tampilkan
 * ----------------
 * Menampilkan informasi profil pengguna.
 *
 * Parameter:
 *   uid - User ID yang ditampilkan
 */
void profil_tampilkan(uid_t uid)
{
    tanda32_t index;
    profil_t *p;

    index = profil_cari_index(uid);
    if (index < 0) {
        kernel_puts("Profil tidak ditemukan\n");
        return;
    }

    p = &g_profil[index];

    kernel_puts("=== Profil Pengguna ===\n");
    kernel_printf("UID        : %u\n", (unsigned)p->uid);
    kernel_puts("Nama       : ");
    kernel_puts(p->nama_tampilan);
    kernel_puts("\n");

    kernel_puts("Home       : ");
    kernel_puts(p->home);
    kernel_puts("\n");

    kernel_puts("Shell      : ");
    kernel_puts(p->shell);
    kernel_puts("\n");

    kernel_puts("Zona Waktu : ");
    kernel_puts(p->zona_waktu);
    kernel_puts("\n");

    if (kernel_strlen(p->email) > 0) {
        kernel_puts("Email      : ");
        kernel_puts(p->email);
        kernel_puts("\n");
    }

    kernel_printf("Sesi Login : %u\n",
                  (unsigned)p->jumlah_riwayat);
    kernel_printf("Preferensi : %u entri\n",
                  (unsigned)p->jumlah_preferensi);
    kernel_puts("=======================\n");
}

/*
 * profil_tampilkan_semua
 * ----------------------
 * Menampilkan daftar semua profil pengguna.
 */
void profil_tampilkan_semua(void)
{
    tak_bertanda32_t i;

    kernel_puts("=== Daftar Profil Pengguna ===\n");
    kernel_printf("Total: %u profil\n\n", g_jumlah_profil);
    kernel_puts("UID   Nama Tampilan\n");
    kernel_puts("------------------------\n");

    for (i = 0; i < g_jumlah_profil; i++) {
        if (!g_profil[i].aktif) {
            continue;
        }

        kernel_printf("%-5u %s\n",
                      (unsigned)g_profil[i].uid,
                      g_profil[i].nama_tampilan);
    }
}

/*
 * ===========================================================================
 * FUNGSI INISIALISASI (INITIALIZATION)
 * ===========================================================================
 */

/*
 * profil_inisialisasi
 * -------------------
 * Menginisialisasi sistem profil dan membuat profil root.
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t profil_inisialisasi(void)
{
    status_t hasil;

    kernel_memset(g_profil, 0, sizeof(g_profil));
    g_jumlah_profil = 0;
    g_uid_saat_ini = UID_INVALID;

    hasil = profil_buat(0, "Pengguna Root", "/sistem/bin/shell",
                        "/rumah/root");
    if (hasil != STATUS_BERHASIL) {
        kernel_printf("[profil] Gagal membuat profil root: %d\n",
                      hasil);
        return hasil;
    }

    hasil = profil_set_preferensi(0, "tema", "gelap");
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }

    hasil = profil_set_preferensi(0, "bahasa", "id_ID");
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }

    kernel_printf("[profil] Diinisialisasi: %u profil\n",
                  g_jumlah_profil);

    return STATUS_BERHASIL;
}
