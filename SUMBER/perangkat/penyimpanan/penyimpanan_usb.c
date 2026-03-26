/*
 * PIGURA OS - PENYIMPANAN_USB.C
 * =============================
 * Implementasi USB mass storage.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../cpu/cpu.h"

/*
 * ===========================================================================
 * KONSTANTA
 * ===========================================================================
 */

#define USB_MASS_STORAGE_CLASS    0x08
#define USB_BULK_ONLY_PROTOCOL    0x50

/* SCSI commands */
#define SCSI_TEST_UNIT_READY      0x00
#define SCSI_REQUEST_SENSE        0x03
#define SCSI_INQUIRY              0x12
#define SCSI_READ_CAPACITY        0x25
#define SCSI_READ_10              0x28
#define SCSI_WRITE_10             0x2A

/*
 * ===========================================================================
 * STRUKTUR DATA
 * ===========================================================================
 */

typedef struct {
    tak_bertanda8_t direction;
    tak_bertanda8_t opcode;
    tak_bertanda16_t tag;
    tak_bertanda32_t transfer_length;
    tak_bertanda8_t flags;
    tak_bertanda8_t lun;
    tak_bertanda8_t length;
} __attribute__((packed)) cbw_t;

typedef struct {
    tak_bertanda32_t signature;
    tak_bertanda32_t tag;
    tak_bertanda32_t data_residue;
    tak_bertanda8_t status;
} __attribute__((packed)) csw_t;

typedef struct {
    tak_bertanda8_t peripheral;
    tak_bertanda8_t rmb;
    tak_bertanda8_t version;
    tak_bertanda8_t format;
    char vendor[8];
    char product[16];
    char revision[4];
} __attribute__((packed)) inquiry_response_t;

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

status_t penyimpanan_usb_init(void)
{
    return STATUS_BERHASIL;
}

status_t usb_mass_storage_detect(void *usb_device)
{
    return STATUS_TIDAK_DUKUNG;
}

status_t usb_mass_storage_read(void *device, tak_bertanda64_t lba,
                                void *buffer, ukuran_t count)
{
    return STATUS_TIDAK_DUKUNG;
}

status_t usb_mass_storage_write(void *device, tak_bertanda64_t lba,
                                 const void *buffer, ukuran_t count)
{
    return STATUS_TIDAK_DUKUNG;
}
