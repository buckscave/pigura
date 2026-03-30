/*
 * PIGURA OS - SHELL.C
 * =====================
 * Antarmuka baris perintah (shell) untuk Pigura OS.
 *
 * Berkas ini mengimplementasikan shell sederhana yang menyediakan
 * interpretasi perintah, eksekusi program bawaan, manajemen riwayat
 * perintah, dan penanganan sinyal dasar.
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

/* Panjang maksimum satu baris perintah */
#define SHELL_BARIS_MAKS            512

/* Jumlah maksimum argumen per perintah */
#define SHELL_ARGumen_MAKS          32

/* Jumlah maksimum token per baris */
#define SHELL_TOKEN_MAKS            64

/* Jumlah maksimum riwayat perintah */
#define SHELL_RIWAYAT_MAKS          64

/* Panjang maksimum prompt */
#define SHELL_PROMPT_MAKS           128

/* Panjang maksimum direktori kerja */
#define SHELL_CWD_MAKS              256

/* Jumlah perintah bawaan */
#define SHELL_PERINTAH_BAWAAN       12

/* Panjang maksimum nama perintah */
#define SHELL_NAMA_PERINTAH_MAKS    32

/* Jumlah maksimum alias */
#define SHELL_ALIAS_MAKS            16

/* Panjang maksimum nama dan nilai alias */
#define SHELL_ALIAS_NAMA_MAKS       32
#define SHELL_ALIAS_NILAI_MAKS      128

/*
 * ===========================================================================
 * STRUKTUR RIWAYAT (HISTORY STRUCTURE)
 * ===========================================================================
 */

/* Entri riwayat perintah */
typedef struct {
    char perintah[SHELL_BARIS_MAKS];  /* Teks perintah */
    bool_t valid;                     /* Apakah entri valid */
} riwayat_t;

/*
 * ===========================================================================
 * STRUKTUR ALIAS (ALIAS STRUCTURE)
 * ===========================================================================
 */

/* Entri alias perintah */
typedef struct {
    char nama[SHELL_ALIAS_NAMA_MAKS];     /* Nama alias */
    char nilai[SHELL_ALIAS_NILAI_MAKS];   /* Nilai alias */
    bool_t aktif;                          /* Apakah aktif */
} alias_t;

/*
 * ===========================================================================
 * STRUKTUR SHELL (SHELL CONTEXT STRUCTURE)
 * ===========================================================================
 */

/* Konteks shell aktif */
typedef struct {
    char prompt[SHELL_PROMPT_MAKS];  /* Prompt string */
    char cwd[SHELL_CWD_MAKS];       /* Direktori kerja */
    bool_t berjalan;                /* Shell sedang berjalan */
    tanda32_t kode_keluar;          /* Kode exit terakhir */
    uid_t uid;                      /* UID pengguna */
    char pengguna[SISTEM_NAMA_PANJANG]; /* Nama pengguna */
} konteks_shell_t;

/*
 * ===========================================================================
 * VARIABEL GLOBAL STATIS (STATIC GLOBAL VARIABLES)
 * ===========================================================================
 */

/* Konteks shell saat ini */
static konteks_shell_t g_shell;

/* Riwayat perintah */
static riwayat_t g_riwayat[SHELL_RIWAYAT_MAKS];

/* Indeks riwayat terakhir */
static tak_bertanda32_t g_riwayat_pos = 0;

/* Jumlah total riwayat */
static tak_bertanda32_t g_riwayat_jumlah = 0;

/* Tabel alias */
static alias_t g_alias[SHELL_ALIAS_MAKS];

/* Jumlah alias aktif */
static tak_bertanda32_t g_alias_jumlah = 0;

/*
 * ===========================================================================
 * DEKLARASI PERINTAH BAWAAN (BUILTIN COMMAND DECLARATIONS)
 * ===========================================================================
 */

typedef status_t (*fungsi_perintah_t)(int argc, char *argv[]);

typedef struct {
    char nama[SHELL_NAMA_PERINTAH_MAKS];
    fungsi_perintah_t fungsi;
    const char *bantuan;
} perintah_bawaan_t;

static status_t shell_cmd_bantuan(int argc, char *argv[]);
static status_t shell_cmd_versi(int argc, char *argv[]);
static status_t shell_cmd_jelas(int argc, char *argv[]);
static status_t shell_cmd_echo(int argc, char *argv[]);
static status_t shell_cmd_info(int argc, char *argv[]);
static status_t shell_cmd_proses(int argc, char *argv[]);
static status_t shell_cmd_memori(int argc, char *argv[]);
static status_t shell_cmd_uptime(int argc, char *argv[]);
static status_t shell_cmd_logout(int argc, char *argv[]);
static status_t shell_cmd_hostname(int argc, char *argv[]);
static status_t shell_cmd_alias(int argc, char *argv[]);
static status_t shell_cmd_riwayat(int argc, char *argv[]);

/* Tabel perintah bawaan */
static const perintah_bawaan_t g_perintah_bawaan[
    SHELL_PERINTAH_BAWAAN] = {
    { "bantuan",  shell_cmd_bantuan,
      "Tampilkan daftar perintah" },
    { "versi",    shell_cmd_versi,
      "Tampilkan versi sistem" },
    { "clear",    shell_cmd_jelas,
      "Bersihkan layar" },
    { "echo",     shell_cmd_echo,
      "Cetak teks ke layar" },
    { "info",     shell_cmd_info,
      "Tampilkan info sistem" },
    { "proses",   shell_cmd_proses,
      "Tampilkan daftar proses" },
    { "memori",   shell_cmd_memori,
      "Tampilkan info memori" },
    { "uptime",   shell_cmd_uptime,
      "Tampilkan uptime sistem" },
    { "logout",   shell_cmd_logout,
      "Keluar dari sesi" },
    { "hostname", shell_cmd_hostname,
      "Tampilkan hostname" },
    { "alias",    shell_cmd_alias,
      "Kelola alias perintah" },
    { "riwayat",  shell_cmd_riwayat,
      "Tampilkan riwayat perintah" }
};

/*
 * ===========================================================================
 * FUNGSI PEMBANTU TOKENIZER (TOKENIZER HELPER)
 * ===========================================================================
 */

/*
 * shell_is_pemisah
 * ----------------
 * Memeriksa apakah karakter adalah pemisah token.
 */
static int shell_is_pemisah(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

/*
 * shell_tokenisasi
 * ----------------
 * Memecah baris perintah menjadi token-token.
 * Token dipisahkan oleh spasi, tab, atau karakter pemisah.
 * Tanda kutip didukung secara sederhana.
 *
 * Parameter:
 *   baris   - Baris perintah input
 *   token   - Array output token
 *   maks    - Jumlah maksimum token
 *
 * Return:
 *   Jumlah token yang ditemukan
 */
static int shell_tokenisasi(char *baris, char *token[], int maks)
{
    int jumlah = 0;
    int dalam_kutip = 0;
    char *p = baris;

    if (baris == NULL || token == NULL || maks <= 0) {
        return 0;
    }

    while (*p != '\0' && jumlah < maks) {
        while (shell_is_pemisah(*p) && !dalam_kutip) {
            p++;
        }

        if (*p == '\0') {
            break;
        }

        token[jumlah] = p;

        while (*p != '\0') {
            if (*p == '"') {
                dalam_kutip = !dalam_kutip;
                p++;
                continue;
            }

            if (shell_is_pemisah(*p) && !dalam_kutip) {
                break;
            }

            p++;
        }

        if (*p != '\0') {
            *p = '\0';
            p++;
        }

        jumlah++;
    }

    return jumlah;
}

/*
 * ===========================================================================
 * FUNGSI RIWAYAT (HISTORY FUNCTIONS)
 * ===========================================================================
 */

/*
 * shell_tambah_riwayat
 * --------------------
 * Menambahkan perintah ke riwayat.
 *
 * Parameter:
 *   perintah - Teks perintah
 */
static void shell_tambah_riwayat(const char *perintah)
{
    ukuran_t panjang;

    if (perintah == NULL) {
        return;
    }

    panjang = kernel_strlen(perintah);
    if (panjang == 0) {
        return;
    }

    kernel_strncpy(g_riwayat[g_riwayat_pos].perintah,
                   perintah, SHELL_BARIS_MAKS - 1);
    g_riwayat[g_riwayat_pos].valid = BENAR;

    g_riwayat_pos = (g_riwayat_pos + 1) % SHELL_RIWAYAT_MAKS;

    if (g_riwayat_jumlah < SHELL_RIWAYAT_MAKS) {
        g_riwayat_jumlah++;
    }
}

/*
 * ===========================================================================
 * PERINTAH BAWAAN (BUILTIN COMMAND IMPLEMENTATIONS)
 * ===========================================================================
 */

static status_t shell_cmd_bantuan(int argc, char *argv[])
{
    int i;
    TIDAK_DIGUNAKAN_PARAM(argc);
    TIDAK_DIGUNAKAN_PARAM(argv);

    kernel_puts("Perintah bawaan " PIGURA_NAMA
                " Shell:\n\n");

    for (i = 0; i < SHELL_PERINTAH_BAWAAN; i++) {
        kernel_printf("  %-12s - %s\n",
                      g_perintah_bawaan[i].nama,
                      g_perintah_bawaan[i].bantuan);
    }

    kernel_puts("\nKetik nama perintah untuk menjalankan.\n");
    return STATUS_BERHASIL;
}

static status_t shell_cmd_versi(int argc, char *argv[])
{
    TIDAK_DIGUNAKAN_PARAM(argc);
    TIDAK_DIGUNAKAN_PARAM(argv);

    kernel_printf("%s versi %s (%s)\n",
                  PIGURA_NAMA, PIGURA_VERSI_STRING,
                  NAMA_ARSITEKTUR_LENGKAP);
    kernel_printf("Shell versi 1.0\n");
    return STATUS_BERHASIL;
}

static status_t shell_cmd_jelas(int argc, char *argv[])
{
    TIDAK_DIGUNAKAN_PARAM(argc);
    TIDAK_DIGUNAKAN_PARAM(argv);

    kernel_clear_screen();
    return STATUS_BERHASIL;
}

static status_t shell_cmd_echo(int argc, char *argv[])
{
    int i;

    for (i = 1; i < argc; i++) {
        kernel_puts(argv[i]);
        if (i < argc - 1) {
            kernel_puts(" ");
        }
    }

    kernel_puts("\n");
    return STATUS_BERHASIL;
}

static status_t shell_cmd_info(int argc, char *argv[])
{
    TIDAK_DIGUNAKAN_PARAM(argc);
    TIDAK_DIGUNAKAN_PARAM(argv);

    sistem_tampilkan_info();
    return STATUS_BERHASIL;
}

static status_t shell_cmd_proses(int argc, char *argv[])
{
    TIDAK_DIGUNAKAN_PARAM(argc);
    TIDAK_DIGUNAKAN_PARAM(argv);

    sistem_tampilkan_proses();
    return STATUS_BERHASIL;
}

static status_t shell_cmd_memori(int argc, char *argv[])
{
    TIDAK_DIGUNAKAN_PARAM(argc);
    TIDAK_DIGUNAKAN_PARAM(argv);

    sistem_tampilkan_memori();
    return STATUS_BERHASIL;
}

static status_t shell_cmd_uptime(int argc, char *argv[])
{
    TIDAK_DIGUNAKAN_PARAM(argc);
    TIDAK_DIGUNAKAN_PARAM(argv);

    sistem_tampilkan_uptime();
    return STATUS_BERHASIL;
}

static status_t shell_cmd_logout(int argc, char *argv[])
{
    TIDAK_DIGUNAKAN_PARAM(argc);
    TIDAK_DIGUNAKAN_PARAM(argv);

    login_logout();
    g_shell.berjalan = SALAH;
    return STATUS_BERHASIL;
}

static status_t shell_cmd_hostname(int argc, char *argv[])
{
    TIDAK_DIGUNAKAN_PARAM(argc);
    TIDAK_DIGUNAKAN_PARAM(argv);

    sistem_tampilkan_hostname();
    return STATUS_BERHASIL;
}

static status_t shell_cmd_alias(int argc, char *argv[])
{
    tak_bertanda32_t i;

    if (argc == 1) {
        kernel_puts("Alias terdaftar:\n");
        for (i = 0; i < g_alias_jumlah; i++) {
            if (g_alias[i].aktif) {
                kernel_printf("  alias %s='%s'\n",
                              g_alias[i].nama,
                              g_alias[i].nilai);
            }
        }
        return STATUS_BERHASIL;
    }

    if (kernel_strcmp(argv[1], "-h") == 0) {
        kernel_puts("Penggunaan: alias nama=nilai\n");
        return STATUS_BERHASIL;
    }

    if (argc == 2) {
        for (i = 0; i < g_alias_jumlah; i++) {
            if (g_alias[i].aktif &&
                kernel_strcmp(g_alias[i].nama,
                              argv[1]) == 0) {
                kernel_printf("alias %s='%s'\n",
                              g_alias[i].nama,
                              g_alias[i].nilai);
                return STATUS_BERHASIL;
            }
        }
        kernel_puts("alias: tidak ditemukan\n");
        return STATUS_TIDAK_DITEMUKAN;
    }

    if (g_alias_jumlah >= SHELL_ALIAS_MAKS) {
        kernel_puts("alias: tabel penuh\n");
        return STATUS_PENUH;
    }

    kernel_strncpy(g_alias[g_alias_jumlah].nama, argv[1],
                   SHELL_ALIAS_NAMA_MAKS - 1);
    kernel_strncpy(g_alias[g_alias_jumlah].nilai, argv[2],
                   SHELL_ALIAS_NILAI_MAKS - 1);
    g_alias[g_alias_jumlah].aktif = BENAR;
    g_alias_jumlah++;

    return STATUS_BERHASIL;
}

static status_t shell_cmd_riwayat(int argc, char *argv[])
{
    tak_bertanda32_t i;
    tak_bertanda32_t mulai;
    tak_bertanda32_t jumlah_tampil;
    TIDAK_DIGUNAKAN_PARAM(argv);

    if (g_riwayat_jumlah == 0) {
        kernel_puts("Riwayat kosong\n");
        return STATUS_BERHASIL;
    }

    jumlah_tampil = g_riwayat_jumlah;
    if (argc > 1) {
        int n = 0;
        ukuran_t j;
        for (j = 0; argv[1][j] != '\0'; j++) {
            if (argv[1][j] >= '0' && argv[1][j] <= '9') {
                n = n * 10 + (argv[1][j] - '0');
            }
        }
        if (n > 0 && (tak_bertanda32_t)n < jumlah_tampil) {
            jumlah_tampil = (tak_bertanda32_t)n;
        }
    }

    mulai = (g_riwayat_jumlah > jumlah_tampil)
            ? g_riwayat_jumlah - jumlah_tampil : 0;

    for (i = mulai; i < g_riwayat_jumlah; i++) {
        tak_bertanda32_t idx = i % SHELL_RIWAYAT_MAKS;
        if (g_riwayat[idx].valid) {
            kernel_printf("  %3u  %s\n", i + 1,
                          g_riwayat[idx].perintah);
        }
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI PARSIR DAN EKSEKUSI (PARSE AND EXECUTE)
 * ===========================================================================
 */

/*
 * shell_cari_perintah_bawaan
 * --------------------------
 * Mencari perintah bawaan berdasarkan nama.
 *
 * Parameter:
 *   nama - Nama perintah
 *
 * Return:
 *   Pointer ke entri perintah, atau NULL
 */
static const perintah_bawaan_t *shell_cari_perintah_bawaan(
    const char *nama)
{
    int i;

    if (nama == NULL) {
        return NULL;
    }

    for (i = 0; i < SHELL_PERINTAH_BAWAAN; i++) {
        if (kernel_strcmp(
                g_perintah_bawaan[i].nama, nama) == 0) {
            return &g_perintah_bawaan[i];
        }
    }

    return NULL;
}

/*
 * shell_cari_alias
 * ----------------
 * Mencari alias berdasarkan nama.
 *
 * Parameter:
 *   nama - Nama perintah
 *
 * Return:
 *   String nilai alias, atau NULL jika tidak ditemukan
 */
static const char *shell_cari_alias(const char *nama)
{
    tak_bertanda32_t i;

    if (nama == NULL) {
        return NULL;
    }

    for (i = 0; i < g_alias_jumlah; i++) {
        if (g_alias[i].aktif &&
            kernel_strcmp(g_alias[i].nama, nama) == 0) {
            return g_alias[i].nilai;
        }
    }

    return NULL;
}

/*
 * shell_eksekusi_baris
 * --------------------
 * Menginterpretasi dan mengeksekusi satu baris perintah.
 *
 * Parameter:
 *   baris - Baris perintah input
 *
 * Return:
 *   Status eksekusi
 */
static status_t shell_eksekusi_baris(char *baris)
{
    char *token[SHELL_TOKEN_MAKS];
    int jumlah_token;
    const perintah_bawaan_t *perintah;
    const char *alias_nilai;
    char baris_alias[SHELL_BARIS_MAKS];

    if (baris == NULL) {
        return STATUS_PARAM_NULL;
    }

    while (*baris != '\0' && shell_is_pemisah(*baris)) {
        baris++;
    }

    if (*baris == '\0' || *baris == '#') {
        return STATUS_BERHASIL;
    }

    shell_tambah_riwayat(baris);

    jumlah_token = shell_tokenisasi(baris, token, SHELL_TOKEN_MAKS);
    if (jumlah_token == 0) {
        return STATUS_BERHASIL;
    }

    alias_nilai = shell_cari_alias(token[0]);
    if (alias_nilai != NULL) {
        kernel_strncpy(baris_alias, alias_nilai,
                       SHELL_BARIS_MAKS - 1);
        jumlah_token = shell_tokenisasi(
            baris_alias, token, SHELL_TOKEN_MAKS);
        if (jumlah_token == 0) {
            return STATUS_BERHASIL;
        }
    }

    perintah = shell_cari_perintah_bawaan(token[0]);
    if (perintah != NULL) {
        return perintah->fungsi(jumlah_token, token);
    }

    kernel_puts("shell: perintah tidak dikenal: ");
    kernel_puts(token[0]);
    kernel_puts("\n");

    return STATUS_TIDAK_DUKUNG;
}

/*
 * ===========================================================================
 * FUNGSI PROMPT (PROMPT FUNCTIONS)
 * ===========================================================================
 */

/*
 * shell_buat_prompt
 * -----------------
 * Membuat string prompt berdasarkan konteks shell.
 *
 * Parameter:
 *   buffer - Buffer output prompt
 *   ukuran - Ukuran buffer
 */
static void shell_buat_prompt(char *buffer, ukuran_t ukuran)
{
    const char *host;
    ukuran_t panjang;

    if (buffer == NULL || ukuran == 0) {
        return;
    }

    host = kernel_get_hostname();
    if (host == NULL) {
        host = "pigura";
    }

    panjang = kernel_strlen(g_shell.pengguna);
    if (panjang > 0) {
        kernel_snprintf(buffer, ukuran, "%s@%s:%s$ ",
                        g_shell.pengguna, host, g_shell.cwd);
    } else {
        kernel_snprintf(buffer, ukuran, "%s:%s$ ",
                        host, g_shell.cwd);
    }
}

/*
 * ===========================================================================
 * FUNGSI SHELL UTAMA (MAIN SHELL FUNCTIONS)
 * ===========================================================================
 */

/*
 * shell_inisialisasi
 * ------------------
 * Menginisialisasi konteks shell.
 *
 * Parameter:
 *   nama_pengguna - Nama pengguna (boleh NULL)
 *   uid           - UID pengguna
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t shell_inisialisasi(const char *nama_pengguna, uid_t uid)
{
    kernel_memset(&g_shell, 0, sizeof(konteks_shell_t));
    kernel_memset(g_riwayat, 0, sizeof(g_riwayat));
    kernel_memset(g_alias, 0, sizeof(g_alias));

    g_riwayat_pos = 0;
    g_riwayat_jumlah = 0;
    g_alias_jumlah = 0;

    kernel_strncpy(g_shell.cwd, "/rumah", SHELL_CWD_MAKS - 1);

    if (nama_pengguna != NULL) {
        kernel_strncpy(g_shell.pengguna, nama_pengguna,
                       SISTEM_NAMA_PANJANG - 1);
    }

    g_shell.uid = uid;
    g_shell.berjalan = BENAR;
    g_shell.kode_keluar = 0;

    kernel_printf("[shell] Shell diinisialisasi untuk %s\n",
                  nama_pengguna != NULL ? nama_pengguna : "(anon)");

    return STATUS_BERHASIL;
}

/*
 * shell_jalankan
 * --------------
 Menjalankan shell interaktif. Fungsi ini melakukan loop
 * membaca input, memparsing, dan mengeksekusi perintah
 * sampai shell dimatikan.
 *
 * Return:
 *   Kode exit shell
 */
tanda32_t shell_jalankan(void)
{
    char baris[SHELL_BARIS_MAKS];
    char prompt[SHELL_PROMPT_MAKS];

    kernel_puts("\n" PIGURA_NAMA " Shell v1.0\n");
    kernel_puts("Ketik 'bantuan' untuk daftar perintah.\n\n");

    while (g_shell.berjalan) {
        shell_buat_prompt(prompt, sizeof(prompt));
        kernel_puts(prompt);

        kernel_memset(baris, 0, sizeof(baris));
        shell_eksekusi_baris(baris);
    }

    kernel_puts("\nSesi shell berakhir.\n");
    return g_shell.kode_keluar;
}

/*
 * shell_matikan
 * -------------
 * Memberhentikan shell.
 */
void shell_matikan(void)
{
    g_shell.berjalan = SALAH;
}

/*
 * shell_berjalan
 * --------------
 * Memeriksa apakah shell masih berjalan.
 *
 * Return:
 *   BENAR jika shell berjalan
 */
bool_t shell_berjalan(void)
{
    return g_shell.berjalan;
}
