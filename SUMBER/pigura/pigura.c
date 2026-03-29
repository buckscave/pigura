/*
 * PIGURA OS - PIGURA.C
 * =====================
 * Modul orkestrator utama subsistem GUI Pigura OS.
 *
 * Berkas ini menjadi titik masuk tunggal (single entry point)
 * untuk seluruh subsistem grafis Pigura. Bertanggung jawab:
 *   - Inisialisasi berurutan semua submodul
 *   - Koordinasi antar subsistem
 *   - Siklus render utama (main loop)
 *   - Penanganan error pada level tertinggi
 *   - Shutdown yang aman dan terurut
 *
 * Subsistem yang dikoordinasi:
 *   1. Framebuffer (permukaan piksel)
 *   2. Jendela (window manager)
 *   3. Peristiwa (event system, penunjuk, pengendali)
 *   4. Penata (compositor, efek, lapisan)
 *   5. Widget (komponen GUI: tombol, teks, dll)
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Tidak menggunakan fitur C99/C11
 * - Seluruh nama dalam Bahasa Indonesia
 * - Mendukung: x86, x86_64, ARM, ARMv7, ARM64
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../inti/kernel.h"
#include "framebuffer/framebuffer.h"
#include "jendela/jendela.h"
#include "peristiwa/peristiwa.h"
#include "penata/penata.h"
#include "widget/widget.h"

/*
 * ===========================================================================
 * KONSTANTA INTERNAL
 * ===========================================================================
 */

/* Magic number modul pigura */
#define PIGURA_MAGIC             0x50494755  /* "PIGU" */

/* Versi modul pigura (major.minor) */
#define PIGURA_VERSI_MAJOR       1
#define PIGURA_VERSI_MINOR       0

/* Interval default render loop dalam milidetik.
 * Nilai 16ms ~60fps, 33ms ~30fps */
#define PIGURA_RENDER_INTERVAL   16

/* Batas waktu tunggu inisialisasi subsistem
 * dalam iterasi sibuk (busy-wait cap) */
#define PIGURA_INIT_TUNGGU_MAKS  1000000

/* Panjang maksimum string status */
#define PIGURA_STATUS_PANJANG    64

/* Jumlah maksimum subsistem terdaftar */
#define PIGURA_SUBSISTEM_MAKS    16

/*
 * ===========================================================================
 * FLAG STATUS PIGURA
 * ===========================================================================
 *
 * Setiap bit merepresentasikan status satu subsistem.
 * Memudahkan pengecekan cepat apakah subsistem
 * tertentu sudah siap atau belum.
 */

/* Framebuffer sudah diinisialisasi */
#define PIGURA_FB_SIAP           0x0001

/* Window manager sudah diinisialisasi */
#define PIGURA_WM_SIAP           0x0002

/* Peristiwa sudah diinisialisasi */
#define PIGURA_PERISTIWA_SIAP    0x0004

/* Penunjuk (cursor) sudah diinisialisasi */
#define PIGURA_PENUNJUK_SIAP     0x0008

/* Pengendali (dispatcher) sudah diinisialisasi */
#define PIGURA_PENGENDALI_SIAP   0x0010

/* Masukan (input processing) sudah diinisialisasi */
#define PIGURA_MASUKAN_SIAP      0x0020

/* Fokus sudah diinisialisasi */
#define PIGURA_FOKUS_SIAP        0x0040

/* Penata (compositor) sudah diinisialisasi */
#define PIGURA_PENATA_SIAP       0x0080

/* Widget (komponen GUI) sudah diinisialisasi */
#define PIGURA_WIDGET_SIAP       0x0100

/* Semua subsistem inti siap */
#define PIGURA_INTI_SIAP         0x01FF

/* Modul pigura aktif dan siap menerima event */
#define PIGURA_AKTIF             0x0200

/* Render loop sedang berjalan */
#define PIGURA_LOOP_JALAN        0x0400

/* Flag permintaan berhenti dari render loop */
#define PIGURA_MINTA_BERHENTI    0x0800

/*
 * ===========================================================================
 * STRUKTUR KONTEKS PIGURA
 * ===========================================================================
 *
 * Konteks utama yang menyimpan seluruh state pigura.
 * Hanya satu instans yang boleh aktif pada satu waktu.
 */

struct pigura_konteks {
    /* Magic number untuk validasi */
    tak_bertanda32_t magic;

    /* Versi modul */
    tak_bertanda16_t versi_major;
    tak_bertanda16_t versi_minor;

    /* Flag status subsistem (bitmask) */
    tak_bertanda32_t status_flag;

    /* Ukuran layar output */
    tak_bertanda32_t layar_lebar;
    tak_bertanda32_t layar_tinggi;
    tak_bertanda32_t layar_bpp;

    /* Counter render */
    tak_bertanda64_t jumlah_render;
    tak_bertanda64_t jumlah_frame;

    /* Waktu render frame terakhir */
    tak_bertanda64_t waktu_frame_terakhir;

    /* Delta waktu antar frame (milidetik) */
    tak_bertanda32_t delta_frame;

    /* FPS terukur */
    tak_bertanda32_t fps_terukur;

    /* Counter untuk kalkulasi FPS */
    tak_bertanda64_t fps_waktu_akumulasi;
    tak_bertanda32_t fps_frame_akumulasi;

    /* Pointer ke permukaan desktop */
    permukaan_t *permukaan_desktop;

    /* Pointer ke konteks penata (compositor) */
    penata_konteks_t *konteks_penata;

    /* Pesan error terakhir */
    char pesan_error[PIGURA_STATUS_PANJANG];
};

typedef struct pigura_konteks pigura_konteks_t;

/*
 * ===========================================================================
 * VARIABEL GLOBAL STATIK
 * ===========================================================================
 *
 * Hanya satu konteks pigura yang aktif pada satu waktu.
 * Pointer ini menjadi akses global ke state pigura.
 */

static pigura_konteks_t *g_pigura = NULL;

/* Counter jumlah inisialisasi (untuk cek berapa kali
   pigura_init dipanggil tanpa shutdown) */
static tak_bertanda32_t g_init_hitungan = 0;

/*
 * ===========================================================================
 * FUNGSI INTERNAL - CEK KONTEKS VALID
 * ===========================================================================
 *
 * Memvalidasi bahwa pointer konteks pigura tidak NULL
 * dan magic number benar.
 * Mengembalikan BENAR jika valid.
 */

static bool_t cek_konteks(const pigura_konteks_t *ktx)
{
    if (ktx == NULL) {
        return SALAH;
    }
    if (ktx->magic != PIGURA_MAGIC) {
        return SALAH;
    }
    return BENAR;
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL - SET PESAN ERROR
 * ===========================================================================
 *
 * Menyimpan pesan error ke dalam konteks pigura.
 * String akan dipotong jika melebihi batas.
 */

static void set_error(pigura_konteks_t *ktx, const char *pesan)
{
    ukuran_t i;
    ukuran_t panjang;

    if (ktx == NULL || pesan == NULL) {
        return;
    }

    panjang = kernel_strlen(pesan);
    if (panjang >= PIGURA_STATUS_PANJANG) {
        panjang = PIGURA_STATUS_PANJANG - 1;
    }

    for (i = 0; i < panjang; i++) {
        ktx->pesan_error[i] = pesan[i];
    }
    ktx->pesan_error[panjang] = '\0';
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL - INISIALISASI FRAMEBUFFER
 * ===========================================================================
 *
 * Menyiapkan subsistem framebuffer/permukaan.
 * Mengalokasikan permukaan desktop dengan ukuran layar.
 *
 * Parameter:
 *   ktx    - Pointer ke konteks pigura
 *   lebar  - Lebar layar dalam piksel
 *   tinggi - Tinggi layar dalam piksel
 *   bpp    - Bit per piksel (32 untuk ARGB8888)
 *
 * Return: STATUS_BERHASIL jika berhasil
 */

static status_t init_framebuffer(pigura_konteks_t *ktx,
                                 tak_bertanda32_t lebar,
                                 tak_bertanda32_t tinggi,
                                 tak_bertanda32_t bpp)
{
    status_t st;
    permukaan_t *desktop;

    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (lebar == 0 || tinggi == 0 || bpp == 0) {
        set_error(ktx,
            "Ukuran layar atau bpp tidak valid");
        return STATUS_PARAM_INVALID;
    }

    st = permukaan_init();
    if (st != STATUS_BERHASIL) {
        set_error(ktx,
            "Gagal inisialisasi permukaan");
        return st;
    }

    desktop = permukaan_buat(lebar, tinggi, bpp);
    if (desktop == NULL) {
        set_error(ktx,
            "Gagal membuat permukaan desktop");
        permukaan_shutdown();
        return STATUS_MEMORI_TIDAK_CUKUP;
    }

    ktx->permukaan_desktop = desktop;
    ktx->layar_lebar = lebar;
    ktx->layar_tinggi = tinggi;
    ktx->layar_bpp = bpp;
    ktx->status_flag |= PIGURA_FB_SIAP;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL - SHUTDOWN FRAMEBUFFER
 * ===========================================================================
 *
 * Membersihkan subsistem framebuffer secara aman.
 * Menghapus permukaan desktop jika masih ada.
 */

static void shutdown_framebuffer(pigura_konteks_t *ktx)
{
    if (ktx == NULL) {
        return;
    }

    if (ktx->permukaan_desktop != NULL) {
        permukaan_hapus(ktx->permukaan_desktop);
        ktx->permukaan_desktop = NULL;
    }

    permukaan_shutdown();
    ktx->status_flag &= ~PIGURA_FB_SIAP;
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL - INISIALISASI WINDOW MANAGER
 * ===========================================================================
 *
 * Menyiapkan subsistem jendela dan window manager.
 * Window manager memerlukan permukaan desktop.
 */

static status_t init_wm(pigura_konteks_t *ktx)
{
    status_t st;

    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (!(ktx->status_flag & PIGURA_FB_SIAP)) {
        set_error(ktx,
            "Framebuffer belum siap, init WM gagal");
        return STATUS_TIDAK_DUKUNG;
    }

    st = jendela_init();
    if (st != STATUS_BERHASIL) {
        set_error(ktx,
            "Gagal inisialisasi subsistem jendela");
        return st;
    }

    st = wm_init();
    if (st != STATUS_BERHASIL) {
        set_error(ktx,
            "Gagal inisialisasi window manager");
        jendela_shutdown();
        return st;
    }

    st = wm_set_desktop(ktx->permukaan_desktop);
    if (st != STATUS_BERHASIL) {
        set_error(ktx,
            "Gagal set desktop ke WM");
        wm_shutdown();
        jendela_shutdown();
        return st;
    }

    ktx->status_flag |= PIGURA_WM_SIAP;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL - SHUTDOWN WINDOW MANAGER
 * ===========================================================================
 */

static void shutdown_wm(pigura_konteks_t *ktx)
{
    if (ktx == NULL) {
        return;
    }

    if (ktx->status_flag & PIGURA_WM_SIAP) {
        wm_shutdown();
        jendela_shutdown();
        ktx->status_flag &= ~PIGURA_WM_SIAP;
    }
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL - INISIALISASI SUBSISTEM PERISTIWA
 * ===========================================================================
 *
 * Menyiapkan seluruh subsistem peristiwa secara berurutan:
 *   1. Peristiwa (antrian event)
 *   2. Penunjuk (cursor/pointer)
 *   3. Pengendali (dispatcher)
 *   4. Masukan (input processing)
 *   5. Fokus (keyboard focus)
 *
 * Setiap langkah gagal akan menghentikan proses dan
 * melakukan cleanup parsial.
 */

static status_t init_peristiwa(pigura_konteks_t *ktx)
{
    status_t st;

    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (!(ktx->status_flag & PIGURA_WM_SIAP)) {
        set_error(ktx,
            "WM belum siap, init peristiwa gagal");
        return STATUS_TIDAK_DUKUNG;
    }

    /* Langkah 1: Inisialisasi antrian peristiwa */
    st = peristiwa_init();
    if (st != STATUS_BERHASIL) {
        set_error(ktx,
            "Gagal init antrian peristiwa");
        return st;
    }
    ktx->status_flag |= PIGURA_PERISTIWA_SIAP;

    /* Langkah 2: Inisialisasi penunjuk (cursor) */
    st = penunjuk_init(ktx->layar_lebar, ktx->layar_tinggi);
    if (st != STATUS_BERHASIL) {
        set_error(ktx,
            "Gagal init subsistem penunjuk");
        peristiwa_shutdown();
        ktx->status_flag &= ~PIGURA_PERISTIWA_SIAP;
        return st;
    }
    ktx->status_flag |= PIGURA_PENUNJUK_SIAP;

    /* Langkah 3: Inisialisasi pengendali (dispatcher) */
    st = pengendali_init();
    if (st != STATUS_BERHASIL) {
        set_error(ktx,
            "Gagal init subsistem pengendali");
        penunjuk_shutdown();
        peristiwa_shutdown();
        ktx->status_flag &= ~PIGURA_PENUNJUK_SIAP;
        ktx->status_flag &= ~PIGURA_PERISTIWA_SIAP;
        return st;
    }
    ktx->status_flag |= PIGURA_PENGENDALI_SIAP;

    /* Langkah 4: Inisialisasi pemroses masukan */
    st = masukan_proses_init();
    if (st != STATUS_BERHASIL) {
        set_error(ktx,
            "Gagal init pemroses masukan");
        pengendali_shutdown();
        penunjuk_shutdown();
        peristiwa_shutdown();
        ktx->status_flag &= ~PIGURA_PENGENDALI_SIAP;
        ktx->status_flag &= ~PIGURA_PENUNJUK_SIAP;
        ktx->status_flag &= ~PIGURA_PERISTIWA_SIAP;
        return st;
    }
    ktx->status_flag |= PIGURA_MASUKAN_SIAP;

    /* Langkah 5: Inisialisasi fokus keyboard */
    st = fokus_init();
    if (st != STATUS_BERHASIL) {
        set_error(ktx,
            "Gagal init subsistem fokus");
        masukan_proses_shutdown();
        pengendali_shutdown();
        penunjuk_shutdown();
        peristiwa_shutdown();
        ktx->status_flag &= ~PIGURA_MASUKAN_SIAP;
        ktx->status_flag &= ~PIGURA_PENGENDALI_SIAP;
        ktx->status_flag &= ~PIGURA_PENUNJUK_SIAP;
        ktx->status_flag &= ~PIGURA_PERISTIWA_SIAP;
        return st;
    }
    ktx->status_flag |= PIGURA_FOKUS_SIAP;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL - SHUTDOWN SUBSISTEM PERISTIWA
 * ===========================================================================
 *
 * Shutdown dalam urutan terbalik dari inisialisasi
 * untuk menghindari dependensi ke modul yang sudah mati.
 */

static void shutdown_peristiwa(pigura_konteks_t *ktx)
{
    if (ktx == NULL) {
        return;
    }

    if (ktx->status_flag & PIGURA_FOKUS_SIAP) {
        fokus_shutdown();
        ktx->status_flag &= ~PIGURA_FOKUS_SIAP;
    }

    if (ktx->status_flag & PIGURA_MASUKAN_SIAP) {
        masukan_proses_shutdown();
        ktx->status_flag &= ~PIGURA_MASUKAN_SIAP;
    }

    if (ktx->status_flag & PIGURA_PENGENDALI_SIAP) {
        pengendali_shutdown();
        ktx->status_flag &= ~PIGURA_PENGENDALI_SIAP;
    }

    if (ktx->status_flag & PIGURA_PENUNJUK_SIAP) {
        penunjuk_shutdown();
        ktx->status_flag &= ~PIGURA_PENUNJUK_SIAP;
    }

    if (ktx->status_flag & PIGURA_PERISTIWA_SIAP) {
        peristiwa_shutdown();
        ktx->status_flag &= ~PIGURA_PERISTIWA_SIAP;
    }
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL - INISIALISASI PENATA (COMPOSITOR)
 * ===========================================================================
 *
 * Membuat konteks penata dan menghubungkannya dengan
 * permukaan desktop. Penata mengelola lapisan dan efek
 * visual yang akan di-composite ke desktop.
 */

static status_t init_penata(pigura_konteks_t *ktx)
{
    penata_konteks_t *ktx_penata;
    status_t st;

    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (!(ktx->status_flag & PIGURA_FB_SIAP)) {
        set_error(ktx,
            "Framebuffer belum siap, init penata gagal");
        return STATUS_TIDAK_DUKUNG;
    }

    ktx_penata = penata_buat(ktx->permukaan_desktop);
    if (ktx_penata == NULL) {
        set_error(ktx,
            "Gagal membuat konteks penata");
        return STATUS_MEMORI_TIDAK_CUKUP;
    }

    st = penata_init(ktx_penata, ktx->permukaan_desktop);
    if (st != STATUS_BERHASIL) {
        set_error(ktx,
            "Gagal inisialisasi penata");
        kfree(ktx_penata);
        return st;
    }

    ktx->konteks_penata = ktx_penata;
    ktx->status_flag |= PIGURA_PENATA_SIAP;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL - SHUTDOWN PENATA
 * ===========================================================================
 */

static void shutdown_penata(pigura_konteks_t *ktx)
{
    if (ktx == NULL) {
        return;
    }

    if (ktx->konteks_penata != NULL) {
        penata_hancurkan_semua(ktx->konteks_penata);
        penata_shutdown(ktx->konteks_penata);
        penata_hapus(ktx->konteks_penata);
        ktx->konteks_penata = NULL;
    }

    ktx->status_flag &= ~PIGURA_PENATA_SIAP;
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL - INISIALISASI WIDGET
 * ===========================================================================
 *
 * Menyiapkan subsistem widget (komponen GUI).
 * Widget menyediakan tombol, kotak teks, checkbox,
 * bar gulir, menu, dan dialog.
 *
 * Widget bergantung pada framebuffer untuk rendering.
 */

static status_t init_widget(pigura_konteks_t *ktx)
{
    status_t st;

    if (ktx == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (!(ktx->status_flag & PIGURA_FB_SIAP)) {
        set_error(ktx,
            "Framebuffer belum siap, init widget gagal");
        return STATUS_TIDAK_DUKUNG;
    }

    st = widget_init();
    if (st != STATUS_BERHASIL) {
        set_error(ktx,
            "Gagal inisialisasi subsistem widget");
        return st;
    }

    ktx->status_flag |= PIGURA_WIDGET_SIAP;
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL - SHUTDOWN WIDGET
 * ===========================================================================
 *
 * Mematikan subsistem widget dan membersihkan
 * seluruh widget yang masih terdaftar.
 */

static void shutdown_widget(pigura_konteks_t *ktx)
{
    if (ktx == NULL) {
        return;
    }

    if (ktx->status_flag & PIGURA_WIDGET_SIAP) {
        widget_shutdown();
        ktx->status_flag &= ~PIGURA_WIDGET_SIAP;
    }
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL - PERBARUI PENGHITUNG FPS
 * ===========================================================================
 *
 * Menghitung FPS (frame per second) berdasarkan waktu
 * yang berlalu sejak frame terakhir. Akumulasi dilakukan
 * selama satu detik, lalu dihitung rata-rata.
 *
 * Parameter:
 *   ktx       - Pointer ke konteks pigura
 *   delta_ms  - Delta waktu sejak frame terakhir
 */

static void perbarui_fps(pigura_konteks_t *ktx,
                         tak_bertanda32_t delta_ms)
{
    tak_bertanda32_t satu_detik = 1000;

    if (ktx == NULL) {
        return;
    }

    if (delta_ms == 0) {
        return;
    }

    ktx->fps_waktu_akumulasi += (tak_bertanda64_t)delta_ms;
    ktx->fps_frame_akumulasi++;

    if (ktx->fps_waktu_akumulasi >=
        (tak_bertanda64_t)satu_detik) {
        ktx->fps_terukur = ktx->fps_frame_akumulasi;
        ktx->fps_frame_akumulasi = 0;
        ktx->fps_waktu_akumulasi = 0;
    }
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL - CEK WAKTU FRAME
 * ===========================================================================
 *
 * Menghitung delta waktu sejak frame terakhir dan
 * memutuskan apakah sudah waktunya render frame baru.
 * Mengembalikan BENAR jika sudah waktunya render.
 */

static bool_t cek_waktu_frame(pigura_konteks_t *ktx)
{
    tak_bertanda64_t waktu_sekarang;
    tak_bertanda64_t delta;

    if (ktx == NULL) {
        return SALAH;
    }

    waktu_sekarang = kernel_get_ticks();

    if (ktx->waktu_frame_terakhir == 0) {
        ktx->waktu_frame_terakhir = waktu_sekarang;
        ktx->delta_frame = 0;
        return BENAR;
    }

    if (waktu_sekarang < ktx->waktu_frame_terakhir) {
        ktx->waktu_frame_terakhir = waktu_sekarang;
        ktx->delta_frame = 0;
        return BENAR;
    }

    delta = waktu_sekarang - ktx->waktu_frame_terakhir;

    if (delta < (tak_bertanda64_t)PIGURA_RENDER_INTERVAL) {
        return SALAH;
    }

    ktx->delta_frame = (tak_bertanda32_t)delta;
    ktx->waktu_frame_terakhir = waktu_sekarang;
    return BENAR;
}

/*
 * ===========================================================================
 * API PUBLIK - INISIALISASI PIGURA
 * ===========================================================================
 *
 * Titik masuk utama untuk memulai subsistem GUI Pigura.
 * Fungsi ini menginisialisasi semua submodul secara
 * berurutan dengan penanganan error yang robust.
 *
 * Urutan inisialisasi:
 *   1. Alokasi konteks pigura
 *   2. Framebuffer (permukaan, desktop)
 *   3. Jendela + Window Manager
 *   4. Peristiwa (antrian, penunjuk, pengendali, masukan, fokus)
 *   5. Penata (compositor)
 *   6. Widget (komponen GUI)
 *
 * Jika salah satu langkah gagal, semua langkah sebelumnya
 * yang sudah berhasil akan di-shutdown secara terurut
 * (cleanup rollback).
 *
 * Parameter:
 *   lebar  - Lebar layar dalam piksel
 *   tinggi - Tinggi layar dalam piksel
 *   bpp    - Bit per piksel (32 = ARGB8888)
 *
 * Return: STATUS_BERHASIL jika semua subsistem siap,
 *         kode error jika ada yang gagal.
 */

status_t pigura_init(tak_bertanda32_t lebar,
                     tak_bertanda32_t tinggi,
                     tak_bertanda32_t bpp)
{
    pigura_konteks_t *ktx;
    status_t st;

    /* Cek apakah sudah ada konteks aktif */
    if (g_pigura != NULL) {
        return STATUS_SUDAH_ADA;
    }

    /* Validasi parameter */
    if (lebar == 0 || tinggi == 0 || bpp == 0) {
        return STATUS_PARAM_INVALID;
    }

    /* Batasi ukuran layar yang wajar */
    if (lebar > 8192 || tinggi > 8192) {
        return STATUS_PARAM_INVALID;
    }

    if (bpp != 16 && bpp != 24 && bpp != 32) {
        return STATUS_PARAM_INVALID;
    }

    /* Alokasi konteks pigura */
    ktx = (pigura_konteks_t *)kmalloc(
        sizeof(pigura_konteks_t));
    if (ktx == NULL) {
        return STATUS_MEMORI_TIDAK_CUKUP;
    }

    /* Bersihkan seluruh memori konteks */
    kernel_memset(ktx, 0, sizeof(pigura_konteks_t));

    /* Set magic dan versi */
    ktx->magic = PIGURA_MAGIC;
    ktx->versi_major = PIGURA_VERSI_MAJOR;
    ktx->versi_minor = PIGURA_VERSI_MINOR;
    ktx->pesan_error[0] = '\0';

    g_pigura = ktx;
    g_init_hitungan++;

    /* Langkah 1: Framebuffer */
    st = init_framebuffer(ktx, lebar, tinggi, bpp);
    if (st != STATUS_BERHASIL) {
        kfree(ktx);
        g_pigura = NULL;
        return st;
    }

    /* Langkah 2: Window Manager */
    st = init_wm(ktx);
    if (st != STATUS_BERHASIL) {
        shutdown_framebuffer(ktx);
        kfree(ktx);
        g_pigura = NULL;
        return st;
    }

    /* Langkah 3: Subsistem Peristiwa */
    st = init_peristiwa(ktx);
    if (st != STATUS_BERHASIL) {
        shutdown_wm(ktx);
        shutdown_framebuffer(ktx);
        kfree(ktx);
        g_pigura = NULL;
        return st;
    }

    /* Langkah 4: Penata (Compositor) */
    st = init_penata(ktx);
    if (st != STATUS_BERHASIL) {
        shutdown_peristiwa(ktx);
        shutdown_wm(ktx);
        shutdown_framebuffer(ktx);
        kfree(ktx);
        g_pigura = NULL;
        return st;
    }

    /* Langkah 5: Widget (Komponen GUI) */
    st = init_widget(ktx);
    if (st != STATUS_BERHASIL) {
        shutdown_penata(ktx);
        shutdown_peristiwa(ktx);
        shutdown_wm(ktx);
        shutdown_framebuffer(ktx);
        kfree(ktx);
        g_pigura = NULL;
        return st;
    }

    /* Semua subsistem siap */
    ktx->status_flag |= PIGURA_AKTIF;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - SHUTDOWN PIGURA
 * ===========================================================================
 *
 * Mematikan seluruh subsistem GUI Pigura secara aman.
 * Shutdown dilakukan dalam urutan terbalik dari
 * inisialisasi untuk menghindari akses ke modul
 * yang sudah dimatikan.
 *
 * Urutan shutdown:
 *   1. Hentikan render loop
 *   2. Widget (komponen GUI)
 *   3. Penata (compositor)
 *   4. Peristiwa (semua submodul)
 *   5. Window Manager + Jendela
 *   6. Framebuffer
 *   7. Bebaskan konteks pigura
 */

void pigura_shutdown(void)
{
    pigura_konteks_t *ktx;

    ktx = g_pigura;
    if (ktx == NULL) {
        return;
    }

    /* Hentikan render loop jika sedang berjalan */
    ktx->status_flag |= PIGURA_MINTA_BERHENTI;
    ktx->status_flag &= ~PIGURA_LOOP_JALAN;

    /* Shutdown dalam urutan terbalik */
    shutdown_widget(ktx);
    shutdown_penata(ktx);
    shutdown_peristiwa(ktx);
    shutdown_wm(ktx);
    shutdown_framebuffer(ktx);

    /* Reset counter dan status */
    ktx->status_flag = 0;
    ktx->magic = 0;
    ktx->jumlah_render = 0;
    ktx->jumlah_frame = 0;
    ktx->fps_terukur = 0;
    ktx->fps_frame_akumulasi = 0;
    ktx->fps_waktu_akumulasi = 0;

    /* Bebaskan konteks */
    kfree(ktx);
    g_pigura = NULL;
}

/*
 * ===========================================================================
 * API PUBLIK - SIKLUS RENDER UTAMA
 * ===========================================================================
 *
 * Fungsi ini dipanggil secara berulang (dari timer interrupt
 * atau idle loop) untuk memproses event dan merender frame.
 *
 * Alur setiap langkah:
 *   1. Cek apakah sudah waktunya frame baru
 *   2. Proses event dari antrian
 *   3. Jalankan langkah window manager
 *   4. Proses langkah masukan (key repeat)
 *   5. Validasi fokus keyboard
 *   6. Render penata (compositor)
 *   7. Flip buffer jika double-buffered
 *   8. Update statistik FPS
 *
 * Return: STATUS_BERHASIL jika berhasil memproses frame,
 *         STATUS_TIDAK_DUKUNG jika pigura belum diinisialisasi.
 */

status_t pigura_langkah(void)
{
    pigura_konteks_t *ktx;
    status_t st;

    ktx = g_pigura;
    if (ktx == NULL || !cek_konteks(ktx)) {
        return STATUS_TIDAK_DUKUNG;
    }

    if (!(ktx->status_flag & PIGURA_AKTIF)) {
        return STATUS_TIDAK_DUKUNG;
    }

    /* Cek apakah ada permintaan berhenti */
    if (ktx->status_flag & PIGURA_MINTA_BERHENTI) {
        return STATUS_TIDAK_DUKUNG;
    }

    /* Cek apakah sudah waktunya frame baru */
    if (!cek_waktu_frame(ktx)) {
        return STATUS_BERHASIL;
    }

    /* Proses semua event dalam antrian */
    st = peristiwa_proses_semua();
    if (st != STATUS_BERHASIL) {
        /* Event gagal diproses: log tapi lanjutkan.
         * Error non-fatal karena event bisa empty. */
    }

    /* Jalankan satu langkah window manager */
    wm_langkah();

    /* Proses langkah masukan (key repeat, hold deteksi) */
    masukan_proses_langkah(ktx->delta_frame);

    /* Validasi fokus keyboard masih valid */
    fokus_validasi();

    /* Render penata (compositor) */
    if (ktx->konteks_penata != NULL &&
        penata_siap(ktx->konteks_penata)) {
        penata_proses(ktx->konteks_penata);
        penata_render(ktx->konteks_penata);
    }

    /* Render widget ke permukaan desktop */
    if (ktx->status_flag & PIGURA_WIDGET_SIAP) {
        widget_render_semua(ktx->permukaan_desktop);
    }

    /* Flip double buffer jika aktif */
    if (ktx->permukaan_desktop != NULL) {
        permukaan_flip(ktx->permukaan_desktop);
    }

    /* Update counter dan FPS */
    ktx->jumlah_render++;
    ktx->jumlah_frame++;
    perbarui_fps(ktx, ktx->delta_frame);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - MINTA PIGURA BERHENTI
 * ===========================================================================
 *
 * Menandai agar render loop berhenti pada iterasi berikutnya.
 * Fungsi ini tidak mematikan subsistem secara langsung,
 * hanya mengatur flag. Pigura_shutdown() tetap harus
 * dipanggil setelah loop berhenti.
 */

void pigura_minta_berhenti(void)
{
    if (g_pigura != NULL) {
        g_pigura->status_flag |= PIGURA_MINTA_BERHENTI;
    }
}

/*
 * ===========================================================================
 * API PUBLIK - CEK APAKAH PIGURA AKTIF
 * ===========================================================================
 *
 * Mengembalikan BENAR jika pigura sudah diinisialisasi
 * dan semua subsistem inti siap.
 */

bool_t pigura_aktif(void)
{
    if (g_pigura == NULL) {
        return SALAH;
    }
    if (!cek_konteks(g_pigura)) {
        return SALAH;
    }
    if (!(g_pigura->status_flag & PIGURA_AKTIF)) {
        return SALAH;
    }
    return BENAR;
}

/*
 * ===========================================================================
 * API PUBLIK - CEK APAKAH MINTA BERHENTI
 * ===========================================================================
 *
 * Mengembalikan BENAR jika ada permintaan berhenti.
 * Digunakan oleh pemanggil loop untuk menentukan
 * kapan harus keluar dari loop render.
 */

bool_t pigura_minta_berhenti_aktif(void)
{
    if (g_pigura == NULL) {
        return BENAR;
    }
    return (g_pigura->status_flag & PIGURA_MINTA_BERHENTI)
        ? BENAR : SALAH;
}

/*
 * ===========================================================================
 * API PUBLIK - DAPATKAN KONTEKS PIGURA
 * ===========================================================================
 *
 * Mengembalikan pointer ke konteks pigura global.
 * Pengguna harus memeriksa NULL sebelum menggunakan.
 * Pointer ini read-only untuk API publik.
 */

const pigura_konteks_t *pigura_dapat_konteks(void)
{
    return g_pigura;
}

/*
 * ===========================================================================
 * API PUBLIK - DAPATKAN PERMUKAAN DESKTOP
 * ===========================================================================
 *
 * Mengembalikan pointer ke permukaan desktop.
 * Permukaan ini adalah target akhir dari semua
 * operasi rendering. NULL jika belum diinisialisasi.
 */

permukaan_t *pigura_dapat_desktop(void)
{
    if (g_pigura == NULL) {
        return NULL;
    }
    return g_pigura->permukaan_desktop;
}

/*
 * ===========================================================================
 * API PUBLIK - DAPATKAN KONTEKS PENATA
 * ===========================================================================
 *
 * Mengembalikan pointer ke konteks penata (compositor).
 * NULL jika penata belum diinisialisasi.
 */

penata_konteks_t *pigura_dapat_penata(void)
{
    if (g_pigura == NULL) {
        return NULL;
    }
    return g_pigura->konteks_penata;
}

/*
 * ===========================================================================
 * API PUBLIK - DAPATKAN PESAN ERROR TERAKHIR
 * ===========================================================================
 *
 * Mengembalikan string pesan error terakhir yang
 * terjadi selama inisialisasi atau operasi.
 * NULL jika tidak ada error atau konteks NULL.
 */

const char *pigura_dapat_error(void)
{
    if (g_pigura == NULL) {
        return NULL;
    }
    return g_pigura->pesan_error;
}

/*
 * ===========================================================================
 * API PUBLIK - DAPATKAN INFORMASI LAYAR
 * ===========================================================================
 *
 * Mengisi pointer dengan informasi ukuran layar.
 * Pointer NULL akan diabaikan dengan aman.
 */

void pigura_dapat_layar(tak_bertanda32_t *lebar,
                        tak_bertanda32_t *tinggi,
                        tak_bertanda32_t *bpp)
{
    if (g_pigura == NULL) {
        return;
    }
    if (lebar != NULL) {
        *lebar = g_pigura->layar_lebar;
    }
    if (tinggi != NULL) {
        *tinggi = g_pigura->layar_tinggi;
    }
    if (bpp != NULL) {
        *bpp = g_pigura->layar_bpp;
    }
}

/*
 * ===========================================================================
 * API PUBLIK - DAPATKAN STATISTIK
 * ===========================================================================
 *
 * Mengisi pointer dengan statistik pigura:
 *   - jumlah_render: Total pemanggilan render
 *   - jumlah_frame:  Total frame yang dirender
 *   - fps:           FPS terukur
 * Pointer NULL akan diabaikan dengan aman.
 */

void pigura_dapat_statistik(tak_bertanda64_t *jumlah_render,
                            tak_bertanda64_t *jumlah_frame,
                            tak_bertanda32_t *fps)
{
    if (g_pigura == NULL) {
        return;
    }
    if (jumlah_render != NULL) {
        *jumlah_render = g_pigura->jumlah_render;
    }
    if (jumlah_frame != NULL) {
        *jumlah_frame = g_pigura->jumlah_frame;
    }
    if (fps != NULL) {
        *fps = g_pigura->fps_terukur;
    }
}

/*
 * ===========================================================================
 * API PUBLIK - DAPATKAN STATUS SUBSISTEM
 * ===========================================================================
 *
 * Mengembalikan bitmask status semua subsistem.
 * Setiap bit menunjukkan apakah subsistem tertentu
 * sudah diinisialisasi (1) atau belum (0).
 *
 * Bit yang digunakan:
 *   Bit 0: Framebuffer (PIGURA_FB_SIAP)
 *   Bit 1: Window Manager (PIGURA_WM_SIAP)
 *   Bit 2: Peristiwa (PIGURA_PERISTIWA_SIAP)
 *   Bit 3: Penunjuk (PIGURA_PENUNJUK_SIAP)
 *   Bit 4: Pengendali (PIGURA_PENGENDALI_SIAP)
 *   Bit 5: Masukan (PIGURA_MASUKAN_SIAP)
 *   Bit 6: Fokus (PIGURA_FOKUS_SIAP)
 *   Bit 7: Penata (PIGURA_PENATA_SIAP)
 *   Bit 8: Widget (PIGURA_WIDGET_SIAP)
 *   Bit 9: Aktif (PIGURA_AKTIF)
 */

tak_bertanda32_t pigura_dapat_status(void)
{
    if (g_pigura == NULL) {
        return 0;
    }
    return g_pigura->status_flag;
}

/*
 * ===========================================================================
 * API PUBLIK - DAPATKAN VERSI PIGURA
 * ===========================================================================
 *
 * Mengembalikan pointer ke string versi modul pigura.
 * String ini statis dan tidak perlu dibebaskan.
 */

const char *pigura_dapat_versi(void)
{
    return PIGURA_VERSI_STRING;
}

/*
 * ===========================================================================
 * API PUBLIK - DAPATKAN NAMA MODUL
 * ===========================================================================
 */

const char *pigura_dapat_nama(void)
{
    return PIGURA_NAMA;
}

/*
 * ===========================================================================
 * API PUBLIK - JUMLAH INISIALISASI
 * ===========================================================================
 *
 * Mengembalikan berapa kali pigura_init() telah dipanggil
 * selama sesi kernel ini. Berguna untuk debugging.
 */

tak_bertanda32_t pigura_dapat_init_hitungan(void)
{
    return g_init_hitungan;
}

/*
 * ===========================================================================
 * API PUBLIK - SET WARNA DESKTOP
 * ===========================================================================
 *
 * Mengisi seluruh permukaan desktop dengan satu warna.
 * Biasanya dipanggil sekali saat init untuk menampilkan
 * warna latar desktop.
 *
 * Parameter:
 *   warna - Warna dalam format pixel (ARGB8888)
 *
 * Return: STATUS_BERHASIL jika berhasil
 */

status_t pigura_set_desktop_warna(tak_bertanda32_t warna)
{
    if (g_pigura == NULL) {
        return STATUS_TIDAK_DUKUNG;
    }

    if (g_pigura->permukaan_desktop == NULL) {
        return STATUS_TIDAK_DUKUNG;
    }

    permukaan_isi(g_pigura->permukaan_desktop, warna);
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - RENDER SEKALI (NON-LOOP)
 * ===========================================================================
 *
 * Melakukan satu kali render lengkap tanpa memasuki loop.
 * Berguna untuk rendering on-demand tanpa timer/loop.
 *
 * Alur: proses event → WM langkah → render penata → flip
 *
 * Return: STATUS_BERHASIL jika berhasil
 */

status_t pigura_render_sekali(void)
{
    pigura_konteks_t *ktx;
    status_t st;

    ktx = g_pigura;
    if (ktx == NULL || !cek_konteks(ktx)) {
        return STATUS_TIDAK_DUKUNG;
    }

    if (!(ktx->status_flag & PIGURA_AKTIF)) {
        return STATUS_TIDAK_DUKUNG;
    }

    /* Proses event */
    st = peristiwa_proses_semua();
    if (st != STATUS_BERHASIL) {
        /* Non-fatal: lanjutkan render */
    }

    /* Langkah WM */
    wm_langkah();

    /* Validasi fokus */
    fokus_validasi();

    /* Render compositor */
    if (ktx->konteks_penata != NULL &&
        penata_siap(ktx->konteks_penata)) {
        penata_proses(ktx->konteks_penata);
        penata_render(ktx->konteks_penata);
    }

    /* Render widget */
    if (ktx->status_flag & PIGURA_WIDGET_SIAP) {
        widget_render_semua(ktx->permukaan_desktop);
    }

    /* Flip */
    if (ktx->permukaan_desktop != NULL) {
        permukaan_flip(ktx->permukaan_desktop);
    }

    ktx->jumlah_render++;
    ktx->jumlah_frame++;
    perbarui_fps(ktx, PIGURA_RENDER_INTERVAL);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * API PUBLIK - CEK WIDGET SIAP
 * ===========================================================================
 *
 * Mengembalikan BENAR jika subsistem widget sudah
 * diinisialisasi dan siap digunakan.
 */

bool_t pigura_widget_siap(void)
{
    if (g_pigura == NULL) {
        return SALAH;
    }
    if (g_pigura->status_flag & PIGURA_WIDGET_SIAP) {
        return BENAR;
    }
    return SALAH;
}

/*
 * ===========================================================================
 * API PUBLIK - RESET STATISTIK
 * ===========================================================================
 *
 * Mereset semua counter statistik (render, frame, FPS)
 * ke nol. Berguna untuk memulai pengukuran baru.
 */

void pigura_reset_statistik(void)
{
    if (g_pigura == NULL) {
        return;
    }

    g_pigura->jumlah_render = 0;
    g_pigura->jumlah_frame = 0;
    g_pigura->fps_terukur = 0;
    g_pigura->fps_frame_akumulasi = 0;
    g_pigura->fps_waktu_akumulasi = 0;
    g_pigura->waktu_frame_terakhir = 0;
    g_pigura->delta_frame = 0;
}

/*
 * ===========================================================================
 * API PUBLIK - CEK SUBSISTEM SIAP
 * ===========================================================================
 *
 * Memeriksa apakah semua subsistem inti sudah siap.
 * Mengembalikan BENAR jika semua bit PIGURA_INTI_SIAP
 * sudah diset dalam status_flag.
 */

bool_t pigura_inti_siap(void)
{
    if (g_pigura == NULL) {
        return SALAH;
    }
    if ((g_pigura->status_flag & PIGURA_INTI_SIAP) ==
        PIGURA_INTI_SIAP) {
        return BENAR;
    }
    return SALAH;
}
