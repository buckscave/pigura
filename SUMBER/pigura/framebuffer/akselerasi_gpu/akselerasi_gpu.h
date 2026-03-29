/*
 * PIGURA OS - AKSELERASI_GPU.H
 * ============================
 * Header publik untuk subsistem akselerasi GPU.
 *
 * Berkas ini mendefinisikan struktur data dan deklarasi fungsi
 * publik yang digunakan oleh seluruh modul akselerasi GPU.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Tidak menggunakan fitur C99/C11
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#ifndef AKSELERASI_GPU_H
#define AKSELERASI_GPU_H

/*
 * ===========================================================================
 * KONSTANTA GPU
 * ===========================================================================
 */

#define GPU_ACCEL_MAGIC       0x47504143  /* "GPAC" */
#define GPU_STATE_MAGIC       0x47535441  /* "GSTA" */
#define GPU_ACCEL_VERSI       1

#define GPU_AREA_MIN_PIXEL    64UL

#define GPU_STATUS_TIDAK_ADA      0
#define GPU_STATUS_SIAP           1
#define GPU_STATUS_AKTIF          2
#define GPU_STATUS_GAGAL          3
#define GPU_STATUS_FALLBACK_CPU   4

#define GPU_KAPABILITAS_FILL       0x00000001
#define GPU_KAPABILITAS_COPY       0x00000002
#define GPU_KAPABILITAS_BLIT       0x00000004
#define GPU_KAPABILITAS_SCALE      0x00000008
#define GPU_KAPABILITAS_ROTATE     0x00000010
#define GPU_KAPABILITAS_BLEND      0x00000020
#define GPU_KAPABILITAS_BLIT_ALPHA 0x00000040

#define GPU_FORMAT_WARNA_XRGB8888  0
#define GPU_FORMAT_WARNA_ARGB8888  1
#define GPU_FORMAT_WARNA_RGB565   2

/*
 * ===========================================================================
 * STRUKTUR DATA
 * ===========================================================================
 */

struct gpu_kapabilitas {
    tak_bertanda32_t flags;
    tak_bertanda32_t ukuran_perintang;
    tak_bertanda32_t lebar_maks;
    tak_bertanda32_t tinggi_maks;
    tak_bertanda32_t bpp_dukung;
    tak_bertanda8_t  versi_major;
    tak_bertanda8_t  versi_minor;
    tak_bertanda8_t  vendor_id;
    tak_bertanda8_t  cadangan[5];
};

struct gpu_statistik {
    tak_bertanda64_t jumlah_operasi;
    tak_bertanda64_t jumlah_gpu;
    tak_bertanda64_t jumlah_fallback;
    tak_bertanda64_t piksel_diproses;
    tak_bertanda64_t waktu_gpu_ns;
    tak_bertanda64_t waktu_cpu_ns;
};

struct gpu_konteks {
    tak_bertanda32_t magic;
    tak_bertanda32_t status;
    tak_bertanda32_t kapabilitas;

    const tak_bertanda32_t *src_buffer;
    tak_bertanda32_t src_lebar;
    tak_bertanda32_t src_tinggi;
    tak_bertanda32_t src_pitch;
    tak_bertanda32_t src_format;

    tak_bertanda32_t *dst_buffer;
    tak_bertanda32_t dst_lebar;
    tak_bertanda32_t dst_tinggi;
    tak_bertanda32_t dst_pitch;
    tak_bertanda32_t dst_format;

    tak_bertanda32_t src_x, src_y;
    tak_bertanda32_t dst_x, dst_y;
    tak_bertanda32_t op_lebar;
    tak_bertanda32_t op_tinggi;

    tak_bertanda32_t warna;

    tak_bertanda8_t alpha_global;
    tak_bertanda8_t sudut_derajat;
    tak_bertanda32_t flag_opsional;

    tak_bertanda32_t kode_error;
};

struct gpu_state_global {
    tak_bertanda32_t magic;
    tak_bertanda32_t versi;
    tak_bertanda32_t status;
    tak_bertanda32_t flags;

    struct gpu_kapabilitas kapabilitas;
    struct gpu_statistik  statistik;

    struct gpu_konteks konteks_terakhir;
};

#endif /* AKSELERASI_GPU_H */
