/*
 * PIGURA OS - MBR.C
 * ==================
 * Implementasi skema partisi MBR (Master Boot Record).
 *
 * Berkas ini menyediakan fungsi-fungsi untuk membaca dan menulis
 * struktur partisi MBR pada perangkat penyimpanan.
 *
 * Kepatuhan standar:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Mendukung semua arsitektur: x86, x86_64, ARM, ARMv7, ARM64
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../cpu/cpu.h"

/*
 * ===========================================================================
 * KONSTANTA MBR
 * ===========================================================================
 */

#define MBR_VERSI_MAJOR 1
#define MBR_VERSI_MINOR 0

#define MBR_MAGIC            0xAA55
#define MBR_ENTRY_MAGIC      0x4D425245  /* "MBRE" */

/* Offset dalam sektor MBR */
#define MBR_BOOT_CODE_SIZE   446
#define MBR_PART_TABLE_OFF   446
#define MBR_PART_ENTRY_SIZE  16
#define MBR_PART_COUNT       4
#define MBR_SIGNATURE_OFF    510

/* Tipe partisi MBR */
#define MBR_PART_TIPE_KOSONG        0x00
#define MBR_PART_TIPE_FAT12         0x01
#define MBR_PART_TIPE_XENIX_ROOT    0x02
#define MBR_PART_TIPE_XENIX_USR     0x03
#define MBR_PART_TIPE_FAT16_OLD     0x04
#define MBR_PART_TIPE_DOS_EXT_OLD   0x05
#define MBR_PART_TIPE_FAT16         0x06
#define MBR_PART_TIPE_NTFS          0x07
#define MBR_PART_TIPE_FAT32_CHS     0x0B
#define MBR_PART_TIPE_FAT32_LBA     0x0C
#define MBR_PART_TIPE_FAT16_LBA     0x0E
#define MBR_PART_TIPE_DOS_EXT       0x0F
#define MBR_PART_TIPE_LINUX_SWAP    0x82
#define MBR_PART_TIPE_LINUX         0x83
#define MBR_PART_TIPE_LINUX_EXT     0x85
#define MBR_PART_TIPE_LINUX_LVM     0x8E
#define MBR_PART_TIPE_GPT_PROTECT   0xEE
#define MBR_PART_TIPE_EFI           0xEF

/* Flag bootable */
#define MBR_BOOTABLE_FLAG   0x80

/*
 * ===========================================================================
 * STRUKTUR DATA MBR
 * ===========================================================================
 */

/* Entry partisi MBR (16 byte) */
typedef struct {
    tak_bertanda8_t boot_indicator;     /* 0x80 = bootable */
    tak_bertanda8_t chs_mulai[3];       /* CHS awal */
    tak_bertanda8_t tipe_part;          /* Tipe partisi */
    tak_bertanda8_t chs_akhir[3];       /* CHS akhir */
    tak_bertanda32_t lba_mulai;         /* LBA awal */
    tak_bertanda32_t jumlah_sektor;     /* Jumlah sektor */
} __attribute__((packed)) mbr_part_entry_t;

/* Header MBR */
typedef struct {
    tak_bertanda8_t boot_code[MBR_BOOT_CODE_SIZE];
    mbr_part_entry_t partisi[MBR_PART_COUNT];
    tak_bertanda16_t signature;
} __attribute__((packed)) mbr_header_t;

/* Entry partisi internal */
typedef struct {
    tak_bertanda32_t magic;
    tak_bertanda32_t index;
    bool_t valid;
    bool_t bootable;
    tak_bertanda8_t tipe;
    tak_bertanda64_t lba_mulai;
    tak_bertanda64_t lba_akhir;
    tak_bertanda64_t jumlah_sektor;
} mbr_entry_t;

/* Konteks MBR */
typedef struct {
    tak_bertanda32_t magic;
    bool_t diinisialisasi;
    tak_bertanda32_t jumlah_perangkat;
    mbr_entry_t entry[MBR_PART_COUNT];
} mbr_konteks_t;

/*
 * ===========================================================================
 * VARIABEL GLOBAL
 * ===========================================================================
 */

static mbr_konteks_t g_mbr_konteks;
static bool_t g_mbr_diinisialisasi = SALAH;

/*
 * ===========================================================================
 * FUNGSI INTERNAL
 * ===========================================================================
 */

/*
 * mbr_chs_ke_lba - Konversi CHS ke LBA
 */
static tak_bertanda32_t mbr_chs_ke_lba(tak_bertanda8_t *chs)
{
    tak_bertanda32_t head;
    tak_bertanda32_t sector;
    tak_bertanda32_t cylinder;

    if (chs == NULL) {
        return 0;
    }

    head = chs[0];
    sector = chs[1] & 0x3F;
    cylinder = ((tak_bertanda32_t)(chs[1] & 0xC0) << 2) | chs[2];

    /* LBA = (C * HPC + H) * SPT + (S - 1) */
    /* Asumsi: HPC = 255, SPT = 63 */
    return (cylinder * 255 + head) * 63 + (sector - 1);
}

/*
 * mbr_lba_ke_chs - Konversi LBA ke CHS
 */
static void mbr_lba_ke_chs(tak_bertanda32_t lba, tak_bertanda8_t *chs)
{
    tak_bertanda32_t cylinder;
    tak_bertanda32_t head;
    tak_bertanda32_t sector;

    if (chs == NULL) {
        return;
    }

    /* Asumsi: HPC = 255, SPT = 63 */
    cylinder = lba / (255 * 63);
    head = (lba / 63) % 255;
    sector = (lba % 63) + 1;

    /* Batasi nilai maksimum */
    if (cylinder > 1023) {
        cylinder = 1023;
        head = 254;
        sector = 63;
    }

    chs[0] = (tak_bertanda8_t)head;
    chs[1] = (tak_bertanda8_t)((sector & 0x3F) |
                               ((cylinder >> 2) & 0xC0));
    chs[2] = (tak_bertanda8_t)(cylinder & 0xFF);
}

/*
 * mbr_entry_valid - Periksa validitas entry
 */
static bool_t mbr_entry_valid(mbr_part_entry_t *entry)
{
    if (entry == NULL) {
        return SALAH;
    }

    if (entry->tipe_part == MBR_PART_TIPE_KOSONG) {
        return SALAH;
    }

    if (entry->jumlah_sektor == 0) {
        return SALAH;
    }

    return BENAR;
}

/*
 * mbr_tipe_nama - Dapatkan nama tipe partisi
 */
static const char *mbr_tipe_nama(tak_bertanda8_t tipe)
{
    switch (tipe) {
        case MBR_PART_TIPE_KOSONG:
            return "Kosong";
        case MBR_PART_TIPE_FAT12:
            return "FAT12";
        case MBR_PART_TIPE_FAT16:
        case MBR_PART_TIPE_FAT16_OLD:
        case MBR_PART_TIPE_FAT16_LBA:
            return "FAT16";
        case MBR_PART_TIPE_FAT32_CHS:
        case MBR_PART_TIPE_FAT32_LBA:
            return "FAT32";
        case MBR_PART_TIPE_NTFS:
            return "NTFS";
        case MBR_PART_TIPE_LINUX:
            return "Linux";
        case MBR_PART_TIPE_LINUX_SWAP:
            return "Linux Swap";
        case MBR_PART_TIPE_LINUX_LVM:
            return "Linux LVM";
        case MBR_PART_TIPE_DOS_EXT:
        case MBR_PART_TIPE_DOS_EXT_OLD:
            return "Extended";
        case MBR_PART_TIPE_GPT_PROTECT:
            return "GPT Protective";
        case MBR_PART_TIPE_EFI:
            return "EFI System";
        default:
            return "Tidak Diketahui";
    }
}

/*
 * mbr_parse_entry - Parse entry partisi dari buffer
 */
static void mbr_parse_entry(mbr_part_entry_t *raw, mbr_entry_t *entry,
                            tak_bertanda32_t index)
{
    if (raw == NULL || entry == NULL) {
        return;
    }

    entry->magic = MBR_ENTRY_MAGIC;
    entry->index = index;
    entry->valid = mbr_entry_valid(raw);
    entry->bootable = (raw->boot_indicator == MBR_BOOTABLE_FLAG);
    entry->tipe = raw->tipe_part;
    entry->lba_mulai = raw->lba_mulai;
    entry->jumlah_sektor = raw->jumlah_sektor;
    entry->lba_akhir = entry->lba_mulai + entry->jumlah_sektor - 1;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

/*
 * mbr_init - Inisialisasi subsistem MBR
 */
status_t mbr_init(void)
{
    tak_bertanda32_t i;

    if (g_mbr_diinisialisasi) {
        return STATUS_SUDAH_ADA;
    }

    kernel_memset(&g_mbr_konteks, 0, sizeof(mbr_konteks_t));

    /* Reset semua entry */
    for (i = 0; i < MBR_PART_COUNT; i++) {
        g_mbr_konteks.entry[i].magic = MBR_ENTRY_MAGIC;
        g_mbr_konteks.entry[i].valid = SALAH;
    }

    g_mbr_konteks.magic = MBR_MAGIC;
    g_mbr_konteks.diinisialisasi = BENAR;
    g_mbr_diinisialisasi = BENAR;

    return STATUS_BERHASIL;
}

/*
 * mbr_shutdown - Matikan subsistem MBR
 */
status_t mbr_shutdown(void)
{
    if (!g_mbr_diinisialisasi) {
        return STATUS_BERHASIL;
    }

    g_mbr_konteks.magic = 0;
    g_mbr_konteks.diinisialisasi = SALAH;
    g_mbr_diinisialisasi = SALAH;

    return STATUS_BERHASIL;
}

/*
 * mbr_baca - Baca tabel partisi MBR dari perangkat
 *
 * Parameter:
 *   dev_id - ID perangkat storage
 *   buffer - Buffer untuk membaca (minimal 512 byte)
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t mbr_baca(tak_bertanda32_t dev_id, void *buffer)
{
    status_t hasil;
    mbr_header_t *mbr;
    tak_bertanda32_t i;

    if (!g_mbr_diinisialisasi) {
        return STATUS_GAGAL;
    }

    if (buffer == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Baca sektor pertama */
    hasil = storage_baca(dev_id, 0, buffer, 1);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }

    mbr = (mbr_header_t *)buffer;

    /* Verifikasi signature */
    if (mbr->signature != MBR_MAGIC) {
        return STATUS_FORMAT_INVALID;
    }

    /* Parse semua entry */
    for (i = 0; i < MBR_PART_COUNT; i++) {
        mbr_parse_entry(&mbr->partisi[i], &g_mbr_konteks.entry[i], i);
    }

    return STATUS_BERHASIL;
}

/*
 * mbr_tulis - Tulis tabel partisi MBR ke perangkat
 *
 * Parameter:
 *   dev_id - ID perangkat storage
 *   buffer - Buffer yang berisi MBR (minimal 512 byte)
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t mbr_tulis(tak_bertanda32_t dev_id, void *buffer)
{
    status_t hasil;
    mbr_header_t *mbr;
    tak_bertanda32_t i;

    if (!g_mbr_diinisialisasi) {
        return STATUS_GAGAL;
    }

    if (buffer == NULL) {
        return STATUS_PARAM_NULL;
    }

    mbr = (mbr_header_t *)buffer;

    /* Pastikan signature benar */
    mbr->signature = MBR_MAGIC;

    /* Update entry dari internal */
    for (i = 0; i < MBR_PART_COUNT; i++) {
        mbr_part_entry_t *raw = &mbr->partisi[i];
        mbr_entry_t *entry = &g_mbr_konteks.entry[i];

        if (entry->valid && entry->tipe != MBR_PART_TIPE_KOSONG) {
            raw->boot_indicator = entry->bootable ? MBR_BOOTABLE_FLAG : 0x00;
            raw->tipe_part = entry->tipe;
            raw->lba_mulai = (tak_bertanda32_t)entry->lba_mulai;
            raw->jumlah_sektor = (tak_bertanda32_t)entry->jumlah_sektor;
            tak_bertanda32_t lba_akhir;
            mbr_lba_ke_chs(raw->lba_mulai, raw->chs_mulai);
            lba_akhir = raw->lba_mulai + raw->jumlah_sektor - 1;
            mbr_lba_ke_chs(lba_akhir, raw->chs_akhir);
        } else {
            kernel_memset(raw, 0, sizeof(mbr_part_entry_t));
        }
    }

    /* Tulis sektor */
    hasil = storage_tulis(dev_id, 0, buffer, 1);

    return hasil;
}

/*
 * mbr_jumlah_partisi - Dapatkan jumlah partisi valid
 *
 * Return: Jumlah partisi valid
 */
tak_bertanda32_t mbr_jumlah_partisi(void)
{
    tak_bertanda32_t count = 0;
    tak_bertanda32_t i;

    if (!g_mbr_diinisialisasi) {
        return 0;
    }

    for (i = 0; i < MBR_PART_COUNT; i++) {
        if (g_mbr_konteks.entry[i].valid) {
            count++;
        }
    }

    return count;
}

/*
 * mbr_dapatkan_entry - Dapatkan entry partisi
 *
 * Parameter:
 *   index - Index partisi (0-3)
 *
 * Return: Pointer ke entry atau NULL
 */
mbr_entry_t *mbr_dapatkan_entry(tak_bertanda32_t index)
{
    if (!g_mbr_diinisialisasi) {
        return NULL;
    }

    if (index >= MBR_PART_COUNT) {
        return NULL;
    }

    if (!g_mbr_konteks.entry[index].valid) {
        return NULL;
    }

    return &g_mbr_konteks.entry[index];
}

/*
 * mbr_validasi - Validasi MBR
 *
 * Parameter:
 *   buffer - Buffer yang berisi MBR
 *
 * Return: BENAR jika MBR valid
 */
bool_t mbr_validasi(void *buffer)
{
    mbr_header_t *mbr;
    tak_bertanda32_t i;
    tak_bertanda32_t valid_count = 0;

    if (buffer == NULL) {
        return SALAH;
    }

    mbr = (mbr_header_t *)buffer;

    /* Cek signature */
    if (mbr->signature != MBR_MAGIC) {
        return SALAH;
    }

    /* Hitung entry valid */
    for (i = 0; i < MBR_PART_COUNT; i++) {
        if (mbr_entry_valid(&mbr->partisi[i])) {
            valid_count++;
        }
    }

    return (valid_count > 0) ? BENAR : SALAH;
}

/*
 * mbr_cetak_info - Cetak informasi MBR
 */
void mbr_cetak_info(void)
{
    tak_bertanda32_t i;

    if (!g_mbr_diinisialisasi) {
        return;
    }

    kernel_printf("\n=== Tabel Partisi MBR ===\n\n");

    for (i = 0; i < MBR_PART_COUNT; i++) {
        mbr_entry_t *entry = &g_mbr_konteks.entry[i];

        if (entry->valid) {
            kernel_printf("Partisi %u:\n", i + 1);
            kernel_printf("  Tipe: %s (0x%02X)\n",
                         mbr_tipe_nama(entry->tipe), entry->tipe);
            kernel_printf("  Bootable: %s\n",
                         entry->bootable ? "Ya" : "Tidak");
            kernel_printf("  LBA Mulai: %lu\n", entry->lba_mulai);
            kernel_printf("  Jumlah Sektor: %lu\n", entry->jumlah_sektor);
            kernel_printf("  Ukuran: %lu MB\n",
                         entry->jumlah_sektor * 512 / (1024 * 1024));
            kernel_printf("\n");
        }
    }
}
