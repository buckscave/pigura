/*
 * PIGURA OS - DRIVER_UMUM/DRIVER_UMUM.C
 * ======================================
 * Framework driver generik untuk IC Detection.
 *
 * Berkas ini mengimplementasikan framework untuk driver generik
 * yang bekerja dengan parameter yang ditest dari IC.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../ic.h"

/*
 * ===========================================================================
 * KONSTANTA DRIVER
 * ===========================================================================
 */

#define DRIVER_NAMA_PANJANG  32
#define DRIVER_MAKS          16

/*
 * ===========================================================================
 * STRUKTUR DRIVER
 * ===========================================================================
 */

/* Struktur driver generik */
typedef struct {
    char nama[DRIVER_NAMA_PANJANG];
    ic_kategori_t kategori;
    bool_t terdaftar;
    
    /* Callbacks */
    status_t (*init)(ic_perangkat_t *perangkat);
    status_t (*reset)(ic_perangkat_t *perangkat);
    void (*cleanup)(ic_perangkat_t *perangkat);
    
    /* Operations */
    tak_bertanda32_t (*read)(ic_perangkat_t *perangkat, void *buf, ukuran_t size);
    tak_bertanda32_t (*write)(ic_perangkat_t *perangkat, const void *buf, ukuran_t size);
    status_t (*ioctl)(ic_perangkat_t *perangkat, tak_bertanda32_t cmd, void *arg);
} ic_driver_t;

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

static ic_driver_t g_driver_list[DRIVER_MAKS];
static tak_bertanda32_t g_driver_count = 0;
static bool_t g_driver_init = SALAH;

/*
 * ===========================================================================
 * FUNGSI REGISTRASI
 * ===========================================================================
 */

/*
 * ic_driver_register - Registrasi driver baru
 */
status_t ic_driver_register(const char *nama,
                             ic_kategori_t kategori,
                             status_t (*init)(ic_perangkat_t *),
                             status_t (*reset)(ic_perangkat_t *),
                             void (*cleanup)(ic_perangkat_t *))
{
    ic_driver_t *driver;
    tak_bertanda32_t i;
    
    if (nama == NULL || init == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (g_driver_count >= DRIVER_MAKS) {
        return STATUS_PENUH;
    }
    
    /* Cari slot kosong */
    for (i = 0; i < DRIVER_MAKS; i++) {
        if (!g_driver_list[i].terdaftar) {
            break;
        }
    }
    
    if (i >= DRIVER_MAKS) {
        return STATUS_PENUH;
    }
    
    driver = &g_driver_list[i];
    
    /* Salin nama */
    for (i = 0; i < DRIVER_NAMA_PANJANG - 1 && nama[i] != '\0'; i++) {
        driver->nama[i] = nama[i];
    }
    driver->nama[i] = '\0';
    
    driver->kategori = kategori;
    driver->init = init;
    driver->reset = reset;
    driver->cleanup = cleanup;
    driver->terdaftar = BENAR;
    
    g_driver_count++;
    
    return STATUS_BERHASIL;
}

/*
 * ic_driver_find - Cari driver untuk kategori
 */
ic_driver_t *ic_driver_find(ic_kategori_t kategori)
{
    tak_bertanda32_t i;
    
    for (i = 0; i < DRIVER_MAKS; i++) {
        if (g_driver_list[i].terdaftar &&
            g_driver_list[i].kategori == kategori) {
            return &g_driver_list[i];
        }
    }
    
    return NULL;
}

/*
 * ===========================================================================
 * FUNGSI INISIALISASI DRIVER
 * ===========================================================================
 */

/*
 * ic_driver_init_perangkat - Inisialisasi perangkat dengan driver
 */
status_t ic_driver_init_perangkat(ic_perangkat_t *perangkat)
{
    ic_driver_t *driver;
    
    if (perangkat == NULL || perangkat->entri == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Cari driver yang sesuai */
    driver = ic_driver_find(perangkat->entri->kategori);
    if (driver == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    /* Panggil init driver */
    if (driver->init != NULL) {
        return driver->init(perangkat);
    }
    
    return STATUS_BERHASIL;
}

/*
 * ic_driver_reset_perangkat - Reset perangkat
 */
status_t ic_driver_reset_perangkat(ic_perangkat_t *perangkat)
{
    ic_driver_t *driver;
    
    if (perangkat == NULL || perangkat->entri == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    driver = ic_driver_find(perangkat->entri->kategori);
    if (driver == NULL || driver->reset == NULL) {
        return STATUS_TIDAK_DUKUNG;
    }
    
    return driver->reset(perangkat);
}

/*
 * ic_driver_cleanup_perangkat - Cleanup perangkat
 */
void ic_driver_cleanup_perangkat(ic_perangkat_t *perangkat)
{
    ic_driver_t *driver;
    
    if (perangkat == NULL || perangkat->entri == NULL) {
        return;
    }
    
    driver = ic_driver_find(perangkat->entri->kategori);
    if (driver != NULL && driver->cleanup != NULL) {
        driver->cleanup(perangkat);
    }
}

/*
 * ===========================================================================
 * FUNGSI OPERASI
 * ===========================================================================
 */

/*
 * ic_driver_read - Baca dari perangkat
 */
tak_bertanda32_t ic_driver_read(ic_perangkat_t *perangkat,
                                 void *buf,
                                 ukuran_t size)
{
    ic_driver_t *driver;
    
    if (perangkat == NULL || buf == NULL || size == 0) {
        return 0;
    }
    
    driver = ic_driver_find(perangkat->entri->kategori);
    if (driver == NULL || driver->read == NULL) {
        return 0;
    }
    
    return driver->read(perangkat, buf, size);
}

/*
 * ic_driver_write - Tulis ke perangkat
 */
tak_bertanda32_t ic_driver_write(ic_perangkat_t *perangkat,
                                  const void *buf,
                                  ukuran_t size)
{
    ic_driver_t *driver;
    
    if (perangkat == NULL || buf == NULL || size == 0) {
        return 0;
    }
    
    driver = ic_driver_find(perangkat->entri->kategori);
    if (driver == NULL || driver->write == NULL) {
        return 0;
    }
    
    return driver->write(perangkat, buf, size);
}

/*
 * ===========================================================================
 * INISIALISASI FRAMEWORK
 * ===========================================================================
 */

/*
 * ic_driver_framework_init - Inisialisasi framework driver
 */
status_t ic_driver_framework_init(void)
{
    tak_bertanda32_t i;
    
    if (g_driver_init) {
        return STATUS_SUDAH_ADA;
    }
    
    /* Reset semua driver */
    for (i = 0; i < DRIVER_MAKS; i++) {
        g_driver_list[i].nama[0] = '\0';
        g_driver_list[i].kategori = IC_KATEGORI_TIDAK_DIKETAHUI;
        g_driver_list[i].terdaftar = SALAH;
        g_driver_list[i].init = NULL;
        g_driver_list[i].reset = NULL;
        g_driver_list[i].cleanup = NULL;
        g_driver_list[i].read = NULL;
        g_driver_list[i].write = NULL;
        g_driver_list[i].ioctl = NULL;
    }
    
    g_driver_count = 0;
    g_driver_init = BENAR;
    
    return STATUS_BERHASIL;
}

/*
 * ic_driver_framework_jumlah - Dapatkan jumlah driver terdaftar
 */
tak_bertanda32_t ic_driver_framework_jumlah(void)
{
    return g_driver_count;
}
