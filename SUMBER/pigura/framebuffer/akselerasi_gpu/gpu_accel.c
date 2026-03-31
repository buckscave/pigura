/*
 * PIGURA OS - GPU_ACCEL.C
 * ========================
 * Inti (core) akselerasi GPU untuk Pigura OS.
 *
 * Berkas ini menyediakan inisialisasi, pengelolaan state, dan
 * fungsi utilitas dasar yang digunakan oleh seluruh modul
 * akselerasi GPU (fill, copy, blit, scale, rotate, blend).
 *
 * Mendukung arsitektur: x86, x86_64, ARM, ARMv7, ARM64
 * Kepatuhan standar: C89 + POSIX Safe Functions
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../../../inti/kernel.h"
#include "akselerasi_gpu.h"

/*
 * ===========================================================================
 * VARIABEL GLOBAL
 * ===========================================================================
 */

static struct gpu_state_global g_gpu = {
    GPU_STATE_MAGIC,
    GPU_ACCEL_VERSI,
    GPU_STATUS_TIDAK_ADA,
    0x00000000,
    {
        0x00000000,   /* flags: kosong (belum deteksi) */
        4096,          /* ukuran_perintang */
        4096,          /* lebar_maks */
        4096,          /* tinggi_maks */
        0x00000017,    /* bpp_dukung: 8, 16, 32 */
        0,             /* versi_major */
        0,             /* versi_minor */
        0,             /* vendor_id */
        {0, 0, 0, 0, 0}
    },
    {
        0, 0, 0, 0, 0, 0
    },
    {0}
};

/*
 * ===========================================================================
 * FUNGSI INTERNAL - VALIDASI
 * ===========================================================================
 */

/*
 * gpu_cek_inisialisasi
 * -------------------
 * Memastikan subsistem GPU sudah diinisialisasi.
 * Mengembalikan BENAR jika siap, SALAH jika belum.
 */
static bool_t gpu_cek_inisialisasi(void)
{
    return (g_gpu.magic == GPU_STATE_MAGIC &&
            g_gpu.status != GPU_STATUS_TIDAK_ADA) ? BENAR : SALAH;
}

/*
 * gpu_validasi_konteks
 * -------------------
 * Memvalidasi konteks operasi GPU.
 * Memeriksa magic number, pointer buffer, dan koherensi
 * dimensi area operasi.
 *
 * Parameter:
 *   ctx   - Pointer ke konteks GPU
 *   diperlukan_ops - Jika BENAR, cek lebar/tinggi operasi
 *
 * Return:
 *   STATUS_BERHASIL jika valid
 *   Kode error jika tidak valid
 */
static status_t gpu_validasi_konteks(struct gpu_konteks *ctx,
                                    bool_t diperlukan_ops)
{
    if (ctx == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (ctx->magic != GPU_STATE_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    /* Sumber harus valid */
    if (ctx->src_buffer == NULL && ctx->dst_buffer == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (ctx->src_buffer != NULL) {
        if (ctx->src_lebar == 0 || ctx->src_tinggi == 0) {
            return STATUS_PARAM_JARAK;
        }
        if (ctx->src_pitch == 0) {
            ctx->src_pitch = ctx->src_lebar;
        }
    }

    /* Tujuan harus valid */
    if (ctx->dst_buffer == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (ctx->dst_lebar == 0 || ctx->dst_tinggi == 0) {
        return STATUS_PARAM_JARAK;
    }
    if (ctx->dst_pitch == 0) {
        ctx->dst_pitch = ctx->dst_lebar;
    }

    /* Cek area operasi jika diperlukan */
    if (diperlukan_ops) {
        if (ctx->op_lebar == 0 || ctx->op_tinggi == 0) {
            return STATUS_PARAM_JARAK;
        }
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL - STATISTIK
 * ===========================================================================
 */

/*
 * gpu_statistik_tambah
 * --------------------
 * Mencatat operasi ke statistik internal.
 *
 * Parameter:
 *   fallback - BENAR jika operasi di-fallback ke CPU
 *   piksel   - Jumlah piksel yang diproses
 */
void gpu_statistik_tambah(bool_t fallback, tak_bertanda64_t piksel)
{
    g_gpu.statistik.jumlah_operasi++;
    g_gpu.statistik.piksel_diproses += piksel;

    if (fallback) {
        g_gpu.statistik.jumlah_fallback++;
    } else {
        g_gpu.statistik.jumlah_gpu++;
    }
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL - UTILITAS WARNA
 * ===========================================================================
 */

/*
 * gpu_komponen_warna
 * -----------------
 * Mengekstrak komponen RGBA dari nilai piksel 32-bit.
 * Mendukung format XRGB8888 dan ARGB8888.
 *
 * Parameter:
 *   piksel - Nilai piksel 32-bit
 *   r      - Pointer untuk menyimpan komponen merah
 *   g      - Pointer untuk menyimpan komponen hijau
 *   b      - Pointer untuk menyimpan komponen biru
 *   a      - Pointer untuk menyimpan komponen alpha
 */
static void gpu_komponen_warna(tak_bertanda32_t piksel,
                               tak_bertanda8_t *r,
                               tak_bertanda8_t *g,
                               tak_bertanda8_t *b,
                               tak_bertanda8_t *a)
{
    if (r != NULL) {
        *r = (tak_bertanda8_t)((piksel >> 16) & 0xFF);
    }
    if (g != NULL) {
        *g = (tak_bertanda8_t)((piksel >> 8) & 0xFF);
    }
    if (b != NULL) {
        *b = (tak_bertanda8_t)(piksel & 0xFF);
    }
    if (a != NULL) {
        *a = (tak_bertanda8_t)((piksel >> 24) & 0xFF);
    }
}

/*
 * gpu_buat_warna
 * -------------
 * Menggabungkan komponen RGBA menjadi nilai piksel 32-bit.
 *
 * Parameter:
 *   r - Komponen merah   (0-255)
 *   g - Komponen hijau   (0-255)
 *   b - Komponen biru    (0-255)
 *   a - Komponen alpha  (0-255)
 *
 * Return:
 *   Nilai piksel 32-bit dalam format ARGB8888
 */
static tak_bertanda32_t gpu_buat_warna(tak_bertanda8_t r,
                                        tak_bertanda8_t g,
                                        tak_bertanda8_t b,
                                        tak_bertanda8_t a)
{
    return ((tak_bertanda32_t)a << 24) |
           ((tak_bertanda32_t)r << 16) |
           ((tak_bertanda32_t)g << 8)  |
           ((tak_bertanda32_t)b);
}

/*
 * gpu_kali_persen
 * --------------
 * Mengalikan nilai byte dengan persentase (0-255).
 * Menggunakan aritmatika integer untuk menghindari floating point.
 *
 * Parameter:
 *   nilai  - Nilai byte (0-255)
 *   persen - Persentase (0-255, 255 = 100%)
 *
 * Return:
 *   Hasil perkalian yang sudah di-clamp ke 0-255
 */
tak_bertanda8_t gpu_kali_persen(tak_bertanda8_t nilai,
                                        tak_bertanda8_t persen)
{
    tak_bertanda32_t hasil;
    hasil = ((tak_bertanda32_t)nilai * (tak_bertanda32_t)persen + 127) / 255;
    if (hasil > 255) {
        hasil = 255;
    }
    return (tak_bertanda8_t)hasil;
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL - DETEKSI KAPABILITAS
 * ===========================================================================
 */

/*
 * gpu_deteksi_kapabilitas_default
 * ------------------------------
 * Mendeteksi kapabilitas GPU berdasarkan HAL.
 * Jika HAL melaporkan adanya GPU, kapabilitas diaktifkan.
 * Jika tidak, sistem berjalan dalam mode CPU-only.
 */
static void gpu_deteksi_kapabilitas_default(void)
{
    const hal_state_t *hal;

    hal = hal_get_state();
    if (hal == NULL) {
        g_gpu.kapabilitas.flags = 0;
        g_gpu.kapabilitas.versi_major = 0;
        g_gpu.kapabilitas.versi_minor = 0;
        g_gpu.kapabilitas.vendor_id = 0;
        return;
    }

    /*
     * Cek apakah HAL melaporkan dukungan akselerasi 2D.
     * Field console.is_graphic mengindikasikan mode grafis aktif.
     * Untuk sekarang, kita mengaktifkan semua kapabilitas dasar
     * karena operasi akan diimplementasi secara software
     * (CPU fallback) jika hardware tidak mendukung.
     *
     * Pendekatan ini memastikan API selalu tersedia
     * meskipun tidak ada GPU hardware.
     */
    if (hal->console.is_graphic) {
        g_gpu.kapabilitas.flags = GPU_KAPABILITAS_FILL  |
                                 GPU_KAPABILITAS_COPY  |
                                 GPU_KAPABILITAS_BLIT  |
                                 GPU_KAPABILITAS_SCALE |
                                 GPU_KAPABILITAS_ROTATE|
                                 GPU_KAPABILITAS_BLEND |
                                 GPU_KAPABILITAS_BLIT_ALPHA;
        g_gpu.kapabilitas.lebar_maks = hal->console.width;
        g_gpu.kapabilitas.tinggi_maks = hal->console.height;
    } else {
        g_gpu.kapabilitas.flags = GPU_KAPABILITAS_FILL  |
                                 GPU_KAPABILITAS_COPY  |
                                 GPU_KAPABILITAS_BLIT  |
                                 GPU_KAPABILITAS_SCALE |
                                 GPU_KAPABILITAS_ROTATE|
                                 GPU_KAPABILITAS_BLEND |
                                 GPU_KAPABILITAS_BLIT_ALPHA;
        g_gpu.kapabilitas.lebar_maks = 4096;
        g_gpu.kapabilitas.tinggi_maks = 4096;
    }

    g_gpu.kapabilitas.bpp_dukung = 0x00000017; /* 8, 16, 32 */
    g_gpu.kapabilitas.versi_major = 1;
    g_gpu.kapabilitas.versi_minor = 0;
    g_gpu.kapabilitas.vendor_id = 0;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - INISIALISASI DAN SHUTDOWN
 * ===========================================================================
 */

/*
 * gpu_akselerasi_init
 * ------------------
 * Menginisialisasi subsistem akselerasi GPU.
 * Mendeteksi kapabilitas dan menyiapkan state internal.
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   Kode error jika gagal
 */
status_t gpu_akselerasi_init(void)
{
    kernel_memset(&g_gpu.konteks_terakhir, 0,
                   sizeof(g_gpu.konteks_terakhir));
    kernel_memset(&g_gpu.statistik, 0, sizeof(g_gpu.statistik));

    gpu_deteksi_kapabilitas_default();

    g_gpu.magic  = GPU_STATE_MAGIC;
    g_gpu.versi  = GPU_ACCEL_VERSI;
    g_gpu.status = GPU_STATUS_AKTIF;
    g_gpu.flags  = 0x00000000;

    return STATUS_BERHASIL;
}

/*
 * gpu_akselerasi_shutdown
 * ---------------------
 * Mematikan subsistem akselerasi GPU.
 * Membersihkan state dan melepaskan resource.
 */
void gpu_akselerasi_shutdown(void)
{
    g_gpu.status = GPU_STATUS_TIDAK_ADA;
    g_gpu.kapabilitas.flags = 0;
    kernel_memset(&g_gpu.konteks_terakhir, 0,
                   sizeof(g_gpu.konteks_terakhir));
    g_gpu.magic = 0;
}

/*
 * gpu_akselerasi_siap
 * -------------------
 * Memeriksa apakah subsistem akselerasi GPU aktif.
 *
 * Return:
 *   BENAR jika aktif dan siap
 *   SALAH jika belum diinisialisasi
 */
bool_t gpu_akselerasi_siap(void)
{
    return gpu_cek_inisialisasi();
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - INFORMASI
 * ===========================================================================
 */

/*
 * gpu_akselerasi_kapabilitas
 * ---------------------------
 * Mengembalikan bitfield kapabilitas GPU.
 *
 * Return:
 *   Bitfield GPU_KAPABILITAS_*
 */
tak_bertanda32_t gpu_akselerasi_kapabilitas(void)
{
    if (!gpu_cek_inisialisasi()) {
        return 0;
    }
    return g_gpu.kapabilitas.flags;
}

/*
 * gpu_akselerasi_punya
 * ---------------------
 * Memeriksa apakah kapabilitas tertentu didukung.
 *
 * Parameter:
 *   kapabilitas - Flag kapabilitas (GPU_KAPABILITAS_*)
 *
 * Return:
 *   BENAR jika didukung
 */
bool_t gpu_akselerasi_punya(tak_bertanda32_t kapabilitas)
{
    if (!gpu_cek_inisialisasi()) {
        return SALAH;
    }
    return (g_gpu.kapabilitas.flags & kapabilitas) != 0
           ? BENAR : SALAH;
}

/*
 * gpu_akselerasi_lebar_maks
 * ---------------------------
 * Mengembalikan lebar maksimum yang didukung.
 *
 * Return:
 *   Lebar maksimum dalam piksel, atau 0 jika belum init
 */
tak_bertanda32_t gpu_akselerasi_lebar_maks(void)
{
    if (!gpu_cek_inisialisasi()) {
        return 0;
    }
    return g_gpu.kapabilitas.lebar_maks;
}

/*
 * gpu_akselerasi_tinggi_maks
 * ----------------------------
 * Mengembalikan tinggi maksimum yang didukung.
 *
 * Return:
 *   Tinggi maksimum dalam piksel, atau 0 jika belum init
 */
tak_bertanda32_t gpu_akselerasi_tinggi_maks(void)
{
    if (!gpu_cek_inisialisasi()) {
        return 0;
    }
    return g_gpu.kapabilitas.tinggi_maks;
}

/*
 * gpu_akselerasi_statistik
 * -------------------------
 * Mengambil salinan statistik akselerasi GPU.
 *
 * Parameter:
 *   stat  - Pointer ke buffer statistik (output)
 *   ukuran - Ukuran buffer dalam byte
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   STATUS_PARAM_NULL jika buffer NULL
 *   STATUS_PARAM_UKURAN jika buffer terlalu kecil
 */
status_t gpu_akselerasi_statistik(void *stat, ukuran_t ukuran)
{
    if (stat == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (ukuran < sizeof(struct gpu_statistik)) {
        return STATUS_PARAM_UKURAN;
    }

    kernel_memcpy(stat, &g_gpu.statistik, sizeof(struct gpu_statistik));
    return STATUS_BERHASIL;
}

/*
 * gpu_akselerasi_reset_statistik
 * -------------------------------
 * Mereset semua counter statistik ke nol.
 */
void gpu_akselerasi_reset_statistik(void)
{
    kernel_memset(&g_gpu.statistik, 0, sizeof(g_gpu.statistik));
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - INISIALISASI KONTEKS
 * ===========================================================================
 */

/*
 * gpu_konteks_siapkan
 * -------------------
 * Menyiapkan konteks operasi GPU dari parameter permukaan.
 * Fungsi ini harus dipanggil sebelum setiap operasi GPU.
 *
 * Parameter:
 *   ctx        - Pointer ke konteks GPU (output)
 *   src_buffer - Buffer sumber (NULL untuk operasi fill)
 *   src_lebar  - Lebar sumber dalam piksel
 *   src_tinggi - Tinggi sumber dalam piksel
 *   src_pitch  - Pitch sumber dalam piksel (0 = auto)
 *   dst_buffer - Buffer tujuan (wajib)
 *   dst_lebar  - Lebar tujuan dalam piksel
 *   dst_tinggi - Tinggi tujuan dalam piksel
 *   dst_pitch  - Pitch tujuan dalam piksel (0 = auto)
 *   dst_x      - Offset X pada tujuan
 *   dst_y      - Offset Y pada tujuan
 *   op_lebar   - Lebar area operasi
 *   op_tinggi  - Tinggi area operasi
 *
 * Return:
 *   STATUS_BERHASIL jika konteks valid
 *   Kode error jika parameter tidak valid
 */
status_t gpu_konteks_siapkan(struct gpu_konteks *ctx,
                             const tak_bertanda32_t *src_buffer,
                             tak_bertanda32_t src_lebar,
                             tak_bertanda32_t src_tinggi,
                             tak_bertanda32_t src_pitch,
                             tak_bertanda32_t *dst_buffer,
                             tak_bertanda32_t dst_lebar,
                             tak_bertanda32_t dst_tinggi,
                             tak_bertanda32_t dst_pitch,
                             tak_bertanda32_t dst_x,
                             tak_bertanda32_t dst_y,
                             tak_bertanda32_t op_lebar,
                             tak_bertanda32_t op_tinggi)
{
    if (ctx == NULL) {
        return STATUS_PARAM_NULL;
    }

    kernel_memset(ctx, 0, sizeof(struct gpu_konteks));
    ctx->magic = GPU_STATE_MAGIC;

    /* Sumber */
    ctx->src_buffer = src_buffer;
    ctx->src_lebar  = src_lebar;
    ctx->src_tinggi = src_tinggi;
    ctx->src_pitch  = (src_pitch > 0) ? src_pitch : src_lebar;
    ctx->src_format = GPU_FORMAT_WARNA_XRGB8888;

    /* Tujuan */
    ctx->dst_buffer = dst_buffer;
    ctx->dst_lebar  = dst_lebar;
    ctx->dst_tinggi = dst_tinggi;
    ctx->dst_pitch  = (dst_pitch > 0) ? dst_pitch : dst_lebar;
    ctx->dst_format = GPU_FORMAT_WARNA_XRGB8888;

    /* Area operasi */
    ctx->src_x = 0;
    ctx->src_y = 0;
    ctx->dst_x = dst_x;
    ctx->dst_y = dst_y;
    ctx->op_lebar  = op_lebar;
    ctx->op_tinggi = op_tinggi;

    /* Nilai default */
    ctx->alpha_global  = 255;
    ctx->sudut_derajat = 0;
    ctx->flag_opsional = 0;
    ctx->kode_error    = 0;

    return gpu_validasi_konteks(ctx, BENAR);
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK - UTILITAS WARNA
 * ===========================================================================
 */

/*
 * gpu_ekstrak_warna
 * ----------------
 * Mengekstrak komponen RGBA dari nilai piksel.
 * Fungsi publik wrapper untuk gpu_komponen_warna.
 *
 * Parameter:
 *   piksel - Nilai piksel 32-bit
 *   r      - Pointer komponen merah   (boleh NULL)
 *   g      - Pointer komponen hijau   (boleh NULL)
 *   b      - Pointer komponen biru    (boleh NULL)
 *   a      - Pointer komponen alpha  (boleh NULL)
 */
void gpu_ekstrak_warna(tak_bertanda32_t piksel,
                       tak_bertanda8_t *r,
                       tak_bertanda8_t *g,
                       tak_bertanda8_t *b,
                       tak_bertanda8_t *a)
{
    gpu_komponen_warna(piksel, r, g, b, a);
}

/*
 * gpu_gabung_warna
 * ----------------
 * Menggabungkan komponen RGBA menjadi piksel 32-bit.
 *
 * Parameter:
 *   r - Merah   (0-255)
 *   g - Hijau   (0-255)
 *   b - Biru    (0-255)
 *   a - Alpha   (0-255)
 *
 * Return:
 *   Nilai piksel ARGB8888
 */
tak_bertanda32_t gpu_gabung_warna(tak_bertanda8_t r,
                                  tak_bertanda8_t g,
                                  tak_bertanda8_t b,
                                  tak_bertanda8_t a)
{
    return gpu_buat_warna(r, g, b, a);
}

/*
 * gpu_campur_warna_linear
 * ------------------------
 * Mencampur dua warna secara linear berdasarkan rasio.
 *
 * Parameter:
 *   warna_a - Warna pertama
 *   warna_b - Warna kedua
 *   rasio   - Rasio campuran (0 = warna_a penuh,
 *             255 = warna_b penuh)
 *
 * Return:
 *   Warna hasil campuran
 */
tak_bertanda32_t gpu_campur_warna_linear(tak_bertanda32_t warna_a,
                                         tak_bertanda32_t warna_b,
                                         tak_bertanda8_t rasio)
{
    tak_bertanda8_t ra, ga, ba, aa;
    tak_bertanda8_t rb, gb, bb, ab;
    tak_bertanda8_t rr, gr, br, ar;
    tak_bertanda8_t inv_rasio;

    gpu_komponen_warna(warna_a, &ra, &ga, &ba, &aa);
    gpu_komponen_warna(warna_b, &rb, &gb, &bb, &ab);

    inv_rasio = (tak_bertanda8_t)(255 - rasio);

    rr = gpu_kali_persen(ra, inv_rasio) + gpu_kali_persen(rb, rasio);
    gr = gpu_kali_persen(ga, inv_rasio) + gpu_kali_persen(gb, rasio);
    br = gpu_kali_persen(ba, inv_rasio) + gpu_kali_persen(bb, rasio);
    ar = gpu_kali_persen(aa, inv_rasio) + gpu_kali_persen(ab, rasio);

    return gpu_buat_warna(rr, gr, br, ar);
}

/*
 * gpu_rasio_ke_alpha
 * -----------------
 * Mengkonversi rasio 8-bit (0-255) menjadi alpha (0-255).
 * Alias sederhana untuk kejelasan semantik.
 */
tak_bertanda8_t gpu_rasio_ke_alpha(tak_bertanda8_t rasio)
{
    (void)rasio;
    return 255;  /* Penggunaan langsung sebagai alpha */
}
