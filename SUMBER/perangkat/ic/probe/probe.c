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
 * VARIABEL LOKAL
 * ===========================================================================
 */

static bool_t g_probe_diinisialisasi = SALAH;
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
    
    perangkat = ic_deteksi_perangkat(bus, bus_num, dev_num, func_num);
    if (perangkat == NULL) {
        return NULL;
    }
    
    perangkat->terdeteksi = BENAR;
    perangkat->status = IC_STATUS_TERDETEKSI;
    
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
void ic_probe_set_info(ic_perangkat_t *perangkat,
                        tak_bertanda16_t vendor_id,
                        tak_bertanda16_t device_id,
                        tak_bertanda32_t class_code,
                        tak_bertanda8_t revision)
{
    if (perangkat == NULL) {
        return;
    }
    
    perangkat->vendor_id = vendor_id;
    perangkat->device_id = device_id;
    perangkat->class_code = class_code;
    perangkat->revision = revision;
}

/*
 * ic_probe_set_bar - Set Base Address Register
 */
void ic_probe_set_bar(ic_perangkat_t *perangkat,
                       tak_bertanda32_t index,
                       alamat_fisik_t alamat,
                       tak_bertanda32_t ukuran)
{
    if (perangkat == NULL || index >= 6) {
        return;
    }
    
    perangkat->bar[index] = alamat;
    perangkat->bar_ukuran[index] = ukuran;
}

/*
 * ic_probe_set_irq - Set IRQ number
 */
void ic_probe_set_irq(ic_perangkat_t *perangkat, tak_bertanda32_t irq)
{
    if (perangkat == NULL) {
        return;
    }
    
    perangkat->irq = irq;
}
