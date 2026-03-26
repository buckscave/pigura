/*
 * PIGURA OS - PROBE/PROBE.C
 * =========================
 * Core probe engine untuk IC Detection.
 *
 * Berkas ini mengimplementasikan fungsi-fungsi core untuk probing
 * perangkat pada berbagai bus sistem.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../ic.h"

/*
 * ===========================================================================
 * VARIABEL GLOBAL
 * ===========================================================================
 */

bool_t g_probe_diinisialisasi = SALAH;
static tak_bertanda32_t g_probe_jumlah_total = 0;

/*
 * ===========================================================================
 * FUNGSI INISIALISASI
 * ===========================================================================
 */

/*
 * ic_probe_init - Inisialisasi sistem probe
 */
status_t ic_probe_init(void)
{
    if (g_probe_diinisialisasi) {
        return STATUS_SUDAH_ADA;
    }
    
    g_probe_jumlah_total = 0;
    g_probe_diinisialisasi = BENAR;
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI UTILITAS PROBE
 * ===========================================================================
 */

/*
 * ic_probe_tambah_perangkat - Tambah perangkat ke daftar
 */
ic_perangkat_t *ic_probe_tambah_perangkat(ic_bus_t bus,
                                           tak_bertanda8_t bus_num,
                                           tak_bertanda8_t dev_num,
                                           tak_bertanda8_t func_num)
{
    ic_perangkat_t *perangkat;
    ic_konteks_t *konteks;
    
    konteks = ic_konteks_dapatkan();
    if (konteks == NULL) {
        return NULL;
    }
    
    if (konteks->jumlah_perangkat >= IC_MAKS_PERANGKAT) {
        return NULL;
    }
    
    perangkat = &konteks->perangkat[konteks->jumlah_perangkat];
    
    /* Inisialisasi perangkat */
    perangkat->magic = IC_MAGIC;
    perangkat->id = konteks->jumlah_perangkat;
    perangkat->bus = bus;
    perangkat->bus_num = bus_num;
    perangkat->dev_num = dev_num;
    perangkat->func_num = func_num;
    perangkat->vendor_id = 0;
    perangkat->device_id = 0;
    perangkat->class_code = 0;
    perangkat->revision = 0;
    perangkat->entri = NULL;
    perangkat->param_hasil = NULL;
    perangkat->jumlah_param_hasil = 0;
    perangkat->status = IC_STATUS_TERDETEKSI;
    perangkat->terdeteksi = BENAR;
    perangkat->teridentifikasi = SALAH;
    perangkat->ditest = SALAH;
    
    konteks->jumlah_perangkat++;
    g_probe_jumlah_total++;
    
    return perangkat;
}

/*
 * ic_probe_jumlah - Dapatkan jumlah perangkat terdeteksi
 */
tak_bertanda32_t ic_probe_jumlah(void)
{
    return g_probe_jumlah_total;
}

/*
 * ===========================================================================
 * FUNGSI PROBE GENERIK
 * ===========================================================================
 */

/*
 * ic_probe_baca_vendor - Baca vendor ID dari perangkat
 */
tak_bertanda16_t ic_probe_baca_vendor(ic_perangkat_t *perangkat)
{
    if (perangkat == NULL) {
        return 0;
    }
    return perangkat->vendor_id;
}

/*
 * ic_probe_baca_device - Baca device ID dari perangkat
 */
tak_bertanda16_t ic_probe_baca_device(ic_perangkat_t *perangkat)
{
    if (perangkat == NULL) {
        return 0;
    }
    return perangkat->device_id;
}

/*
 * ic_probe_baca_class - Baca class code dari perangkat
 */
tak_bertanda32_t ic_probe_baca_class(ic_perangkat_t *perangkat)
{
    if (perangkat == NULL) {
        return 0;
    }
    return perangkat->class_code;
}

/*
 * ic_probe_set_info - Set informasi perangkat
 */
status_t ic_probe_set_info(ic_perangkat_t *perangkat,
                            tak_bertanda16_t vendor_id,
                            tak_bertanda16_t device_id,
                            tak_bertanda32_t class_code,
                            tak_bertanda8_t revision)
{
    if (perangkat == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    perangkat->vendor_id = vendor_id;
    perangkat->device_id = device_id;
    perangkat->class_code = class_code;
    perangkat->revision = revision;
    
    return STATUS_BERHASIL;
}

/*
 * ic_probe_set_bar - Set Base Address Register
 */
status_t ic_probe_set_bar(ic_perangkat_t *perangkat,
                           tak_bertanda32_t index,
                           alamat_fisik_t alamat,
                           tak_bertanda32_t ukuran)
{
    if (perangkat == NULL) {
        return STATUS_PARAM_NULL;
    }
    if (index >= 6) {
        return STATUS_PARAM_INVALID;
    }
    
    perangkat->bar[index] = alamat;
    perangkat->bar_ukuran[index] = ukuran;
    
    return STATUS_BERHASIL;
}

/*
 * ic_probe_set_irq - Set IRQ number
 */
status_t ic_probe_set_irq(ic_perangkat_t *perangkat, tak_bertanda32_t irq)
{
    if (perangkat == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    perangkat->irq = irq;
    
    return STATUS_BERHASIL;
}
