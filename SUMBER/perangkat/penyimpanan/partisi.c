/*
 * PIGURA OS - PARTISI.C
 * ======================
 * Implementasi manajemen partisi generik.
 *
 * Berkas ini menyediakan fungsi-fungsi untuk mengelola partisi
 * pada perangkat penyimpanan, termasuk deteksi dan enumerasi.
 *
 * Kepatuhan standar:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Mendukung semua arsitektur: x86, x86_64, ARM, ARMv7, ARM64
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../cpu/cpu.h"
#include "../../inti/kernel.h"

/* Forward declarations */
extern status_t storage_baca(tak_bertanda32_t dev_id, tak_bertanda64_t lba,
                            void *buffer, ukuran_t count);
extern status_t gpt_init(void);
extern status_t mbr_init(void);

/*
 * ===========================================================================
 * KONSTANTA PARTISI
 * ===========================================================================
 */

#define PARTISI_VERSI_MAJOR 1
#define PARTISI_VERSI_MINOR 0

#define PARTISI_MAGIC        0x50415254  /* "PART" */
#define PARTISI_ENTRY_MAGIC  0x50455254  /* "PERT" */

/* Jumlah maksimum partisi per perangkat */
#define PARTISI_MAKS_PERANGKAT  16

/* Tipe partisi */
#define PARTISI_TIPE_TIDAK_ADA    0x00
#define PARTISI_TIPE_MBR          0x01
#define PARTISI_TIPE_GPT          0x02

/* Status partisi */
#define PARTISI_STATUS_TIDAK_ADA  0
#define PARTISI_STATUS_AKTIF      1
#define PARTISI_STATUS_NONAKTIF   2
#define PARTISI_STATUS_ERROR      3

/* Tipe filesystem umum */
#define FS_TIPE_TIDAK_DIKETAHUI  0x00
#define FS_TIPE_FAT12            0x01
#define FS_TIPE_FAT16            0x04
#define FS_TIPE_FAT32_CHS        0x0B
#define FS_TIPE_FAT32_LBA        0x0C
#define FS_TIPE_NTFS             0x07
#define FS_TIPE_LINUX            0x83
#define FS_TIPE_LINUX_SWAP       0x82
#define FS_TIPE_LINUX_LVM        0x8E
#define FS_TIPE_EFI_SYSTEM       0xEF

/*
 * ===========================================================================
 * STRUKTUR DATA PARTISI
 * ===========================================================================
 */

/* Entry partisi */
typedef struct {
    tak_bertanda32_t magic;
    tak_bertanda32_t index;
    tak_bertanda8_t tipe_skema;
    tak_bertanda8_t status;
    tak_bertanda8_t tipe_fs;
    tak_bertanda8_t flags;

    tak_bertanda64_t lba_mulai;
    tak_bertanda64_t lba_akhir;
    tak_bertanda64_t jumlah_sektor;
    tak_bertanda64_t ukuran_byte;

    char label[36];
    bool_t aktif;
    bool_t bootable;
    bool_t valid;
} partisi_entry_t;

/* Konteks partisi */
typedef struct {
    tak_bertanda32_t magic;
    bool_t diinisialisasi;
    tak_bertanda32_t jumlah_perangkat;

    /* Statistik */
    tak_bertanda64_t total_partisi;
    tak_bertanda64_t partisi_aktif;
} partisi_konteks_t;

/*
 * ===========================================================================
 * VARIABEL GLOBAL
 * ===========================================================================
 */

static partisi_konteks_t g_partisi_konteks;
static bool_t g_partisi_diinisialisasi = SALAH;

/*
 * ===========================================================================
 * FUNGSI INTERNAL
 * ===========================================================================
 */

/*
 * partisi_entry_reset - Reset entry partisi
 */
static void partisi_entry_reset(partisi_entry_t *entry)
{
    if (entry == NULL) {
        return;
    }

    kernel_memset(entry, 0, sizeof(partisi_entry_t));
    entry->magic = PARTISI_ENTRY_MAGIC;
    entry->tipe_skema = PARTISI_TIPE_TIDAK_ADA;
    entry->status = PARTISI_STATUS_TIDAK_ADA;
}

/*
 * partisi_entry_valid - Periksa validitas entry
 */
static bool_t partisi_entry_valid(partisi_entry_t *entry)
{
    if (entry == NULL) {
        return SALAH;
    }

    if (entry->magic != PARTISI_ENTRY_MAGIC) {
        return SALAH;
    }

    if (!entry->valid) {
        return SALAH;
    }

    if (entry->jumlah_sektor == 0) {
        return SALAH;
    }

    return BENAR;
}

/*
 * partisi_hitung_ukuran - Hitung ukuran partisi dalam byte
 */
static tak_bertanda64_t partisi_hitung_ukuran(tak_bertanda64_t sektor,
                                               ukuran_t ukuran_sektor)
{
    return sektor * ukuran_sektor;
}

/*
 * partisi_buat_label - Buat label partisi default
 */
static void partisi_buat_label(partisi_entry_t *entry,
                                tak_bertanda32_t index)
{
    char buffer[16];
    ukuran_t len;

    if (entry == NULL) {
        return;
    }

    kernel_memset(entry->label, 0, sizeof(entry->label));

    /* Buat label default "Partisi N" */
    kernel_memset(buffer, 0, sizeof(buffer));
    kernel_snprintf(buffer, sizeof(buffer), "Partisi %u", index + 1);

    len = 0;
    while (buffer[len] != '\0' && len < 35) {
        entry->label[len] = buffer[len];
        len++;
    }
    entry->label[len] = '\0';
}

/*
 * partisi_fs_nama - Dapatkan nama filesystem
 */
static const char *partisi_fs_nama(tak_bertanda8_t tipe)
{
    switch (tipe) {
        case FS_TIPE_FAT12:
            return "FAT12";
        case FS_TIPE_FAT16:
            return "FAT16";
        case FS_TIPE_FAT32_CHS:
        case FS_TIPE_FAT32_LBA:
            return "FAT32";
        case FS_TIPE_NTFS:
            return "NTFS";
        case FS_TIPE_LINUX:
            return "Linux";
        case FS_TIPE_LINUX_SWAP:
            return "Linux Swap";
        case FS_TIPE_LINUX_LVM:
            return "Linux LVM";
        case FS_TIPE_EFI_SYSTEM:
            return "EFI System";
        default:
            return "Tidak Diketahui";
    }
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

/*
 * partisi_init - Inisialisasi subsistem partisi
 */
status_t partisi_init(void)
{
    if (g_partisi_diinisialisasi) {
        return STATUS_SUDAH_ADA;
    }

    kernel_memset(&g_partisi_konteks, 0, sizeof(partisi_konteks_t));

    g_partisi_konteks.magic = PARTISI_MAGIC;
    g_partisi_konteks.diinisialisasi = BENAR;
    g_partisi_diinisialisasi = BENAR;

    return STATUS_BERHASIL;
}

/*
 * partisi_shutdown - Matikan subsistem partisi
 */
status_t partisi_shutdown(void)
{
    if (!g_partisi_diinisialisasi) {
        return STATUS_BERHASIL;
    }

    g_partisi_konteks.magic = 0;
    g_partisi_konteks.diinisialisasi = SALAH;
    g_partisi_diinisialisasi = SALAH;

    return STATUS_BERHASIL;
}

/*
 * partisi_deteksi - Deteksi partisi pada perangkat
 *
 * Parameter:
 *   storage_dev - Pointer ke perangkat storage
 *   buffer - Buffer untuk membaca sektor
 *   ukuran_buffer - Ukuran buffer
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t partisi_deteksi(void *storage_dev, void *buffer,
                          ukuran_t ukuran_buffer)
{
    status_t hasil;
    tak_bertanda8_t *buf;
    tak_bertanda16_t signature;

    if (!g_partisi_diinisialisasi) {
        return STATUS_GAGAL;
    }

    if (storage_dev == NULL || buffer == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (ukuran_buffer < 512) {
        return STATUS_PARAM_UKURAN;
    }

    buf = (tak_bertanda8_t *)buffer;

    /* Baca sektor pertama (MBR) */
    hasil = storage_baca(0, 0, buffer, 1);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }

    /* Periksa signature MBR */
    signature = (tak_bertanda16_t)(buf[511] | (buf[510] << 8));

    if (signature == 0xAA55) {
        /* MBR valid, cek apakah GPT protective */
        if (buf[450] == 0xEE) {
            /* GPT protective MBR, gunakan parser GPT */
            hasil = gpt_init();
            if (hasil == STATUS_BERHASIL) {
                /* Parse GPT header */
            }
        } else {
            /* MBR biasa */
            hasil = mbr_init();
            if (hasil == STATUS_BERHASIL) {
                /* Parse MBR entries */
            }
        }
    }

    return STATUS_BERHASIL;
}

/*
 * partisi_buat - Buat partisi baru
 *
 * Parameter:
 *   storage_dev - Pointer ke perangkat storage
 *   lba_mulai - LBA awal
 *   jumlah_sektor - Jumlah sektor
 *   tipe_fs - Tipe filesystem
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t partisi_buat(void *storage_dev, tak_bertanda64_t lba_mulai,
                      tak_bertanda64_t jumlah_sektor,
                      tak_bertanda8_t tipe_fs)
{
    partisi_entry_t entry;

    if (!g_partisi_diinisialisasi) {
        return STATUS_GAGAL;
    }

    if (storage_dev == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (jumlah_sektor == 0) {
        return STATUS_PARAM_INVALID;
    }

    partisi_entry_reset(&entry);

    entry.lba_mulai = lba_mulai;
    entry.lba_akhir = lba_mulai + jumlah_sektor - 1;
    entry.jumlah_sektor = jumlah_sektor;
    entry.ukuran_byte = jumlah_sektor * 512;
    entry.tipe_fs = tipe_fs;
    entry.status = PARTISI_STATUS_AKTIF;
    entry.valid = BENAR;
    entry.aktif = BENAR;

    partisi_buat_label(&entry, g_partisi_konteks.total_partisi);

    g_partisi_konteks.total_partisi++;
    g_partisi_konteks.partisi_aktif++;

    return STATUS_BERHASIL;
}

/*
 * partisi_hapus - Hapus partisi
 *
 * Parameter:
 *   storage_dev - Pointer ke perangkat storage
 *   index - Index partisi
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t partisi_hapus(void *storage_dev, tak_bertanda32_t index)
{
    if (!g_partisi_diinisialisasi) {
        return STATUS_GAGAL;
    }

    if (storage_dev == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (index >= PARTISI_MAKS_PERANGKAT) {
        return STATUS_PARAM_INVALID;
    }

    g_partisi_konteks.partisi_aktif--;

    return STATUS_BERHASIL;
}

/*
 * partisi_info - Dapatkan informasi partisi
 *
 * Parameter:
 *   storage_dev - Pointer ke perangkat storage
 *   index - Index partisi
 *   info - Buffer untuk informasi
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t partisi_info(void *storage_dev, tak_bertanda32_t index,
                      void *info)
{
    partisi_entry_t *entry;

    if (!g_partisi_diinisialisasi) {
        return STATUS_GAGAL;
    }

    if (storage_dev == NULL || info == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (index >= PARTISI_MAKS_PERANGKAT) {
        return STATUS_PARAM_INVALID;
    }

    entry = (partisi_entry_t *)info;

    if (!partisi_entry_valid(entry)) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    return STATUS_BERHASIL;
}

/*
 * partisi_jumlah - Dapatkan jumlah partisi
 *
 * Parameter:
 *   storage_dev - Pointer ke perangkat storage
 *
 * Return: Jumlah partisi
 */
tak_bertanda32_t partisi_jumlah(void *storage_dev)
{
    if (!g_partisi_diinisialisasi) {
        return 0;
    }

    if (storage_dev == NULL) {
        return 0;
    }

    return (tak_bertanda32_t)g_partisi_konteks.total_partisi;
}

/*
 * partisi_cetak_info - Cetak informasi partisi
 */
void partisi_cetak_info(void)
{
    if (!g_partisi_diinisialisasi) {
        return;
    }

    kernel_printf("\n=== Informasi Partisi ===\n\n");
    kernel_printf("Total Partisi: %lu\n", g_partisi_konteks.total_partisi);
    kernel_printf("Partisi Aktif: %lu\n", g_partisi_konteks.partisi_aktif);
    kernel_printf("\n");
}
