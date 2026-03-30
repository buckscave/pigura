/*
 * PIGURA OS - SISTEM.H
 * =====================
 * Header bersama untuk subsistem aplikasi/sistem/.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) + POSIX tertib dan ketat
 * - Batas 80 karakter per baris
 * - Bahasa Indonesia untuk semua identifier
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#ifndef APLIKASI_SISTEM_H
#define APLIKASI_SISTEM_H

#include "../../inti/kernel.h"

/*
 * ===========================================================================
 * KONSTANTA BERSAMA (SHARED CONSTANTS)
 * ===========================================================================
 */

/* Panjang nama pengguna untuk login, shell, profil */
#define SISTEM_NAMA_PANJANG         32

/* Panjang path shell default */
#define SISTEM_SHELL_PANJANG       64

/* Panjang path home directory */
#define SISTEM_HOME_PANJANG        64

/* Panjang nama host */
#define SISTEM_HOST_PANJANG        64

/* Panjang hash sandi */
#define SISTEM_SANDI_HASH_PANJANG  64

/*
 * ===========================================================================
 * DEKLARASI FUNGSI INIT (INIT FUNCTIONS)
 * ===========================================================================
 */

status_t init_daftarkan_layanan(const char *nama, const char *path,
                                const char *argumen, tanda32_t runlevel,
                                tak_bertanda32_t urutan, bool_t otomatis,
                                bool_t kritikal);
status_t init_hapus_layanan(const char *nama);
status_t init_masuk_runlevel(tanda32_t runlevel);
tanda32_t init_dapat_runlevel(void);
status_t init_mulai(void);
tanda32_t init_loop_utama(void);
void init_shutdown(bool_t reboot);
bool_t init_status(void);
void init_tampilkan_status(void);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI SISTEM (SYSTEM FUNCTIONS)
 * ===========================================================================
 */

typedef struct {
    tak_bertanda64_t uptime;
    tak_bertanda32_t proses_aktif;
    tak_bertanda32_t proses_total;
    tak_bertanda32_t thread_total;
    tak_bertanda64_t memori_total;
    tak_bertanda64_t memori_bebas;
    tak_bertanda64_t memori_pakai;
    tak_bertanda32_t cpu_core;
    char hostname[SISTEM_HOST_PANJANG];
    char arsitektur[16];
    char versi_kernel[16];
} ringkasan_sistem_t;

status_t sistem_dapat_ringkasan(ringkasan_sistem_t *info);
void sistem_tampilkan_info(void);
void sistem_tampilkan_memori(void);
void sistem_tampilkan_uptime(void);
void sistem_tampilkan_hostname(void);
status_t sistem_set_hostname(const char *hostname);
void sistem_tampilkan_proses(void);
void sistem_tampilkan_versi(void);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI LOGIN (LOGIN FUNCTIONS)
 * ===========================================================================
 */

status_t login_tambah_pengguna(const char *nama, const char *sandi,
                                uid_t uid, gid_t gid,
                                const char *shell, const char *home);
status_t login_hapus_pengguna(const char *nama);
status_t login_ubah_sandi(const char *nama, const char *sandi_lama,
                           const char *sandi_baru);
status_t login_autentikasi(const char *nama, const char *sandi);
void login_logout(void);
bool_t login_sesi_aktif(void);
uid_t login_dapat_uid_saat_ini(void);
gid_t login_dapat_gid_saat_ini(void);
status_t login_inisialisasi(void);
void login_tampilkan_info(void);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI SHELL (SHELL FUNCTIONS)
 * ===========================================================================
 */

status_t shell_inisialisasi(const char *nama_pengguna, uid_t uid);
tanda32_t shell_jalankan(void);
void shell_matikan(void);
bool_t shell_berjalan(void);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI PENGATURAN (SETTINGS FUNCTIONS)
 * ===========================================================================
 */

status_t pengaturan_set(tanda32_t kategori, const char *kunci,
                         const char *nilai);
status_t pengaturan_ambil(const char *kunci, char *nilai,
                           ukuran_t ukuran);
status_t pengaturan_ambil_bilangan(const char *kunci,
                                    tanda32_t *nilai,
                                    tanda32_t nilai_bawa);
status_t pengaturan_hapus(const char *kunci);
status_t pengaturan_inisialisasi(void);
void pengaturan_tampilkan_semua(void);
void pengaturan_tampilkan_kategori(tanda32_t kategori);
tak_bertanda32_t pengaturan_jumlah_perubahan(void);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI PROFIL (PROFILE FUNCTIONS)
 * ===========================================================================
 */

status_t profil_buat(uid_t uid, const char *nama_tampilan,
                      const char *shell, const char *home);
status_t profil_hapus(uid_t uid);
status_t profil_set_preferensi(uid_t uid, const char *kunci,
                                const char *nilai);
status_t profil_ambil_preferensi(uid_t uid, const char *kunci,
                                  char *nilai, ukuran_t ukuran);
void profil_tambah_riwayat(uid_t uid, bool_t berhasil,
                            const char *metode);
void profil_tampilkan(uid_t uid);
void profil_tampilkan_semua(void);
status_t profil_inisialisasi(void);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI TENTANG (ABOUT FUNCTIONS)
 * ===========================================================================
 */

void tentang_tampilkan_banner(void);
void tentang_tampilkan_versi(void);
void tentang_tampilkan_arsitektur(void);
void tentang_tampilkan_komponen(void);
void tentang_tampilkan_kontributor(void);
void tentang_tampilkan_lisensi(void);
void tentang_tampilkan_semua(void);
const char *tentang_dapat_versi_string(void);
const char *tentang_dapat_nama(void);
const char *tentang_dapat_julukan(void);
tanda32_t tentang_dapat_versi_numerik(void);
status_t tentang_inisialisasi(void);

#endif /* APLIKASI_SISTEM_H */
