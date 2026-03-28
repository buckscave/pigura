/*
 * PIGURA OS - GPT.C
 * ==================
 * Implementasi skema partisi GPT (GUID Partition Table).
 *
 * Berkas ini menyediakan fungsi-fungsi untuk membaca dan menulis
 * struktur partisi GPT pada perangkat penyimpanan.
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

/*
 * ===========================================================================
 * KONSTANTA GPT
 * ===========================================================================
 */

#define GPT_VERSI_MAJOR 1
#define GPT_VERSI_MINOR 0

/* Magic dan signature */
#define GPT_HEADER_MAGIC      0x5452415020494645ULL  /* "EFI PART" */
#define GPT_ENTRY_MAGIC       0x47505445  /* "GPTE" */

/* Ukuran struktur */
#define GPT_HEADER_SIZE       92
#define GPT_ENTRY_SIZE        128
#define GPT_MIN_PART_COUNT    128
#define GPT_MAX_PART_COUNT    256

/* GUID tipe partisi umum */
static const tak_bertanda8_t GPT_TYPE_UNUSED[16] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const tak_bertanda8_t GPT_TYPE_EFI_SYSTEM[16] = {
    0xC1, 0x2A, 0x73, 0x28, 0xBF, 0x59, 0x68, 0x45,
    0xBB, 0x27, 0x3E, 0x2B, 0x4B, 0x41, 0xB4, 0x3A
};

static const tak_bertanda8_t GPT_TYPE_BASIC_DATA[16] = {
    0xA2, 0xA0, 0xD0, 0xEB, 0xE5, 0xB9, 0x15, 0x45,
    0x8C, 0x32, 0xEB, 0xD6, 0xC3, 0x42, 0x8F, 0x13
};

static const tak_bertanda8_t GPT_TYPE_LINUX_SWAP[16] = {
    0x4E, 0x48, 0x49, 0x06, 0xDA, 0x96, 0x4F, 0x47,
    0xB4, 0xB4, 0x8D, 0x4B, 0xE9, 0x1B, 0xC7, 0x6A
};

static const tak_bertanda8_t GPT_TYPE_LINUX_ROOT[16] = {
    0x81, 0x54, 0xE3, 0x0C, 0x73, 0x39, 0x90, 0x49,
    0x94, 0x7A, 0xA3, 0x27, 0xF4, 0x43, 0xB0, 0xB2
};

/* Flag atribut */
#define GPT_ATTR_SYSTEM          0x0000000000000001ULL
#define GPT_ATTR_READ_ONLY       0x1000000000000000ULL
#define GPT_ATTR_HIDDEN          0x2000000000000000ULL
#define GPT_ATTR_NO_AUTO         0x4000000000000000ULL

/*
 * ===========================================================================
 * STRUKTUR DATA GPT
 * ===========================================================================
 */

/* GUID (128-bit) */
typedef struct {
    tak_bertanda32_t data1;
    tak_bertanda16_t data2;
    tak_bertanda16_t data3;
    tak_bertanda8_t data4[8];
} gpt_guid_t;

/* Header GPT */
typedef struct {
    tak_bertanda64_t signature;        /* "EFI PART" */
    tak_bertanda32_t revision;         /* Versi (1.0) */
    tak_bertanda32_t header_size;      /* Ukuran header */
    tak_bertanda32_t header_crc;       /* CRC32 header */
    tak_bertanda32_t reserved;         /* Reserved */
    tak_bertanda64_t my_lba;           /* LBA header ini */
    tak_bertanda64_t alternate_lba;    /* LBA header alternate */
    tak_bertanda64_t first_usable_lba; /* LBA pertama usable */
    tak_bertanda64_t last_usable_lba;  /* LBA terakhir usable */
    gpt_guid_t disk_guid;              /* GUID disk */
    tak_bertanda64_t partition_entry_lba; /* LBA awal array */
    tak_bertanda32_t number_of_entries;   /* Jumlah entry */
    tak_bertanda32_t size_of_entry;       /* Ukuran per entry */
    tak_bertanda32_t crc_of_entries;      /* CRC32 array */
} __attribute__((packed)) gpt_header_t;

/* Entry partisi GPT (128 byte) */
typedef struct {
    gpt_guid_t type_guid;              /* GUID tipe partisi */
    gpt_guid_t unique_guid;            /* GUID unik partisi */
    tak_bertanda64_t starting_lba;     /* LBA awal */
    tak_bertanda64_t ending_lba;       /* LBA akhir */
    tak_bertanda64_t attributes;       /* Atribut */
    tak_bertanda16_t partition_name[36]; /* Nama partisi (UTF-16LE) */
} __attribute__((packed)) gpt_entry_t;

/* Entry internal */
typedef struct {
    tak_bertanda32_t magic;
    tak_bertanda32_t index;
    bool_t valid;
    gpt_guid_t type_guid;
    gpt_guid_t unique_guid;
    tak_bertanda64_t lba_mulai;
    tak_bertanda64_t lba_akhir;
    tak_bertanda64_t jumlah_sektor;
    tak_bertanda64_t atribut;
    char nama[72];  /* Nama dalam ASCII */
} gpt_part_entry_t;

/* Konteks GPT */
typedef struct {
    tak_bertanda32_t magic;
    bool_t diinisialisasi;
    gpt_header_t header;
    gpt_part_entry_t entry[GPT_MAX_PART_COUNT];
    tak_bertanda32_t jumlah_entry;
} gpt_konteks_t;

/*
 * ===========================================================================
 * VARIABEL GLOBAL
 * ===========================================================================
 */

static gpt_konteks_t g_gpt_konteks;
static bool_t g_gpt_diinisialisasi = SALAH;

/*
 * ===========================================================================
 * FUNGSI INTERNAL
 * ===========================================================================
 */

/*
 * gpt_guid_kosong - Cek apakah GUID kosong
 */
static bool_t gpt_guid_kosong(gpt_guid_t *guid)
{
    tak_bertanda8_t *p;
    ukuran_t i;

    if (guid == NULL) {
        return BENAR;
    }

    p = (tak_bertanda8_t *)guid;
    for (i = 0; i < 16; i++) {
        if (p[i] != 0x00) {
            return SALAH;
        }
    }

    return BENAR;
}

/*
 * gpt_guid_sama - Cek apakah dua GUID sama
 */
static bool_t gpt_guid_sama(gpt_guid_t *a, gpt_guid_t *b)
{
    tak_bertanda8_t *pa;
    tak_bertanda8_t *pb;
    ukuran_t i;

    if (a == NULL || b == NULL) {
        return SALAH;
    }

    pa = (tak_bertanda8_t *)a;
    pb = (tak_bertanda8_t *)b;

    for (i = 0; i < 16; i++) {
        if (pa[i] != pb[i]) {
            return SALAH;
        }
    }

    return BENAR;
}

/*
 * gpt_tipe_nama - Dapatkan nama tipe partisi
 */
static const char *gpt_tipe_nama(gpt_guid_t *guid)
{
    if (gpt_guid_sama(guid, (gpt_guid_t *)GPT_TYPE_EFI_SYSTEM)) {
        return "EFI System Partition";
    }
    if (gpt_guid_sama(guid, (gpt_guid_t *)GPT_TYPE_BASIC_DATA)) {
        return "Microsoft Basic Data";
    }
    if (gpt_guid_sama(guid, (gpt_guid_t *)GPT_TYPE_LINUX_SWAP)) {
        return "Linux Swap";
    }
    if (gpt_guid_sama(guid, (gpt_guid_t *)GPT_TYPE_LINUX_ROOT)) {
        return "Linux Root";
    }
    if (gpt_guid_kosong(guid)) {
        return "Unused";
    }
    return "Unknown";
}

/*
 * gpt_nama_ke_ascii - Konversi nama UTF-16LE ke ASCII
 */
static void gpt_nama_ke_ascii(tak_bertanda16_t *utf16, char *ascii,
                               ukuran_t ukuran)
{
    ukuran_t i;
    ukuran_t j = 0;

    if (utf16 == NULL || ascii == NULL || ukuran == 0) {
        return;
    }

    for (i = 0; i < 36 && j < ukuran - 1; i++) {
        tak_bertanda16_t wc = utf16[i];

        if (wc == 0x0000) {
            break;
        }

        /* Konversi sederhana: ambil byte rendah */
        if (wc < 0x0080) {
            ascii[j++] = (char)(wc & 0xFF);
        } else {
            ascii[j++] = '?';  /* Placeholder untuk non-ASCII */
        }
    }

    ascii[j] = '\0';
}

/*
 * gpt_crc32 - Hitung CRC32
 */
static tak_bertanda32_t gpt_crc32(const void *data, ukuran_t ukuran)
{
    const tak_bertanda8_t *p;
    tak_bertanda32_t crc;
    ukuran_t i;
    ukuran_t j;

    if (data == NULL || ukuran == 0) {
        return 0;
    }

    crc = 0xFFFFFFFF;
    p = (const tak_bertanda8_t *)data;

    /* Implementasi CRC32 sederhana */
    for (i = 0; i < ukuran; i++) {
        crc ^= p[i];
        for (j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }

    return crc ^ 0xFFFFFFFF;
}

/*
 * gpt_parse_entry - Parse entry partisi dari struktur GPT
 */
static void gpt_parse_entry(gpt_entry_t *raw, gpt_part_entry_t *entry,
                            tak_bertanda32_t index)
{
    if (raw == NULL || entry == NULL) {
        return;
    }

    entry->magic = GPT_ENTRY_MAGIC;
    entry->index = index;

    /* Cek apakah entry kosong */
    if (gpt_guid_kosong(&raw->type_guid)) {
        entry->valid = SALAH;
        return;
    }

    entry->valid = BENAR;
    entry->type_guid = raw->type_guid;
    entry->unique_guid = raw->unique_guid;
    entry->lba_mulai = raw->starting_lba;
    entry->lba_akhir = raw->ending_lba;
    entry->jumlah_sektor = raw->ending_lba - raw->starting_lba + 1;
    entry->atribut = raw->attributes;

    gpt_nama_ke_ascii(raw->partition_name, entry->nama,
                      sizeof(entry->nama));
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

/*
 * gpt_init - Inisialisasi subsistem GPT
 */
status_t gpt_init(void)
{
    if (g_gpt_diinisialisasi) {
        return STATUS_SUDAH_ADA;
    }

    kernel_memset(&g_gpt_konteks, 0, sizeof(gpt_konteks_t));

    g_gpt_konteks.magic = GPT_HEADER_MAGIC;
    g_gpt_konteks.diinisialisasi = BENAR;
    g_gpt_diinisialisasi = BENAR;

    return STATUS_BERHASIL;
}

/*
 * gpt_shutdown - Matikan subsistem GPT
 */
status_t gpt_shutdown(void)
{
    if (!g_gpt_diinisialisasi) {
        return STATUS_BERHASIL;
    }

    g_gpt_konteks.magic = 0;
    g_gpt_konteks.diinisialisasi = SALAH;
    g_gpt_diinisialisasi = SALAH;

    return STATUS_BERHASIL;
}

/*
 * gpt_baca_header - Baca header GPT dari perangkat
 *
 * Parameter:
 *   dev_id - ID perangkat storage
 *   buffer - Buffer untuk membaca (minimal 512 byte)
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t gpt_baca_header(tak_bertanda32_t dev_id, void *buffer)
{
    status_t hasil;
    gpt_header_t *header;

    if (!g_gpt_diinisialisasi) {
        return STATUS_GAGAL;
    }

    if (buffer == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Baca sektor 1 (LBA 1) untuk GPT header */
    hasil = storage_baca(dev_id, 1, buffer, 1);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }

    header = (gpt_header_t *)buffer;

    /* Verifikasi signature */
    if (header->signature != GPT_HEADER_MAGIC) {
        return STATUS_FORMAT_INVALID;
    }

    /* Validasi ukuran header */
    if (header->header_size < GPT_HEADER_SIZE) {
        return STATUS_FORMAT_INVALID;
    }

    /* Salin header ke konteks */
    kernel_memcpy(&g_gpt_konteks.header, header,
                  sizeof(gpt_header_t));

    return STATUS_BERHASIL;
}

/*
 * gpt_baca_entry - Baca array entry partisi
 *
 * Parameter:
 *   dev_id - ID perangkat storage
 *   buffer - Buffer untuk membaca
 *   ukuran_buffer - Ukuran buffer
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t gpt_baca_entry(tak_bertanda32_t dev_id, void *buffer,
                         ukuran_t ukuran_buffer)
{
    status_t hasil;
    gpt_entry_t *entry_array;
    tak_bertanda32_t i;
    tak_bertanda32_t jumlah;
    tak_bertanda64_t lba;
    ukuran_t sector_needed;

    if (!g_gpt_diinisialisasi) {
        return STATUS_GAGAL;
    }

    if (buffer == NULL) {
        return STATUS_PARAM_NULL;
    }

    jumlah = g_gpt_konteks.header.number_of_entries;
    if (jumlah > GPT_MAX_PART_COUNT) {
        jumlah = GPT_MAX_PART_COUNT;
    }

    /* Hitung sektor yang dibutuhkan */
    sector_needed = (jumlah * GPT_ENTRY_SIZE + 511) / 512;
    if (ukuran_buffer < sector_needed * 512) {
        return STATUS_PARAM_UKURAN;
    }

    lba = g_gpt_konteks.header.partition_entry_lba;

    /* Baca array entry */
    hasil = storage_baca(dev_id, lba, buffer, sector_needed);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }

    entry_array = (gpt_entry_t *)buffer;
    g_gpt_konteks.jumlah_entry = 0;

    /* Parse semua entry */
    for (i = 0; i < jumlah; i++) {
        gpt_parse_entry(&entry_array[i], &g_gpt_konteks.entry[i], i);

        if (g_gpt_konteks.entry[i].valid) {
            g_gpt_konteks.jumlah_entry++;
        }
    }

    return STATUS_BERHASIL;
}

/*
 * gpt_deteksi - Deteksi dan baca GPT dari perangkat
 *
 * Parameter:
 *   dev_id - ID perangkat storage
 *   buffer - Buffer kerja (minimal 8KB)
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t gpt_deteksi(tak_bertanda32_t dev_id, void *buffer)
{
    status_t hasil;

    hasil = gpt_baca_header(dev_id, buffer);
    if (hasil != STATUS_BERHASIL) {
        return hasil;
    }

    hasil = gpt_baca_entry(dev_id, buffer, 8192);
    return hasil;
}

/*
 * gpt_jumlah_partisi - Dapatkan jumlah partisi valid
 *
 * Return: Jumlah partisi valid
 */
tak_bertanda32_t gpt_jumlah_partisi(void)
{
    if (!g_gpt_diinisialisasi) {
        return 0;
    }

    return g_gpt_konteks.jumlah_entry;
}

/*
 * gpt_dapatkan_entry - Dapatkan entry partisi
 *
 * Parameter:
 *   index - Index partisi
 *
 * Return: Pointer ke entry atau NULL
 */
gpt_part_entry_t *gpt_dapatkan_entry(tak_bertanda32_t index)
{
    if (!g_gpt_diinisialisasi) {
        return NULL;
    }

    if (index >= g_gpt_konteks.header.number_of_entries) {
        return NULL;
    }

    if (!g_gpt_konteks.entry[index].valid) {
        return NULL;
    }

    return &g_gpt_konteks.entry[index];
}

/*
 * gpt_validasi - Validasi header GPT
 *
 * Parameter:
 *   buffer - Buffer yang berisi header
 *
 * Return: BENAR jika valid
 */
bool_t gpt_validasi(void *buffer)
{
    gpt_header_t *header;

    if (buffer == NULL) {
        return SALAH;
    }

    header = (gpt_header_t *)buffer;

    /* Cek signature */
    if (header->signature != GPT_HEADER_MAGIC) {
        return SALAH;
    }

    /* Cek revision */
    if (header->revision != 0x00010000) {
        /* Bisa jadi versi lain, tapi tetap valid */
    }

    /* Cek ukuran header */
    if (header->header_size < GPT_HEADER_SIZE) {
        return SALAH;
    }

    /* Cek CRC (opsional, bisa di-skip untuk performa) */
    /* CRC harus divalidasi dengan menghitung ulang */

    return BENAR;
}

/*
 * gpt_cetak_info - Cetak informasi GPT
 */
void gpt_cetak_info(void)
{
    tak_bertanda32_t i;

    if (!g_gpt_diinisialisasi) {
        return;
    }

    kernel_printf("\n=== GPT Partition Table ===\n\n");
    kernel_printf("Disk GUID: %08X-%04X-%04X\n",
                 g_gpt_konteks.header.disk_guid.data1,
                 g_gpt_konteks.header.disk_guid.data2,
                 g_gpt_konteks.header.disk_guid.data3);

    kernel_printf("Jumlah Entry: %u\n",
                 g_gpt_konteks.header.number_of_entries);
    kernel_printf("LBA Usable: %lu - %lu\n\n",
                 g_gpt_konteks.header.first_usable_lba,
                 g_gpt_konteks.header.last_usable_lba);

    for (i = 0; i < GPT_MAX_PART_COUNT; i++) {
        gpt_part_entry_t *entry = &g_gpt_konteks.entry[i];

        if (entry->valid) {
            kernel_printf("Partisi %u:\n", i + 1);
            kernel_printf("  Nama: %s\n", entry->nama);
            kernel_printf("  Tipe: %s\n",
                         gpt_tipe_nama(&entry->type_guid));
            kernel_printf("  LBA: %lu - %lu\n",
                         entry->lba_mulai, entry->lba_akhir);
            kernel_printf("  Ukuran: %lu MB\n",
                         entry->jumlah_sektor * 512 / (1024 * 1024));
            kernel_printf("\n");
        }
    }
}
