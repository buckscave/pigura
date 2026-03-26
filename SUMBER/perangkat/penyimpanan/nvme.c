/*
 * PIGURA OS - NVME.C
 * ===================
 * Implementasi driver NVMe (Non-Volatile Memory Express)
 * untuk perangkat penyimpanan SSD NVMe.
 *
 * Berkas ini menyediakan fungsi-fungsi untuk mengakses perangkat
 * penyimpanan NVMe melalui PCI Express.
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
 * KONSTANTA NVME
 * ===========================================================================
 */

#define NVME_VERSI_MAJOR 1
#define NVME_VERSI_MINOR 0

/* Magic number */
#define NVME_MAGIC           0x4E564D45  /* "NVME" */
#define NVME_DEVICE_MAGIC    0x4E564D44  /* "NVMD" */
#define NVME_QUEUE_MAGIC     0x4E564D51  /* "NVMQ" */

/* Register offset controller */
#define NVME_REG_CAP         0x0000  /* Controller Capabilities */
#define NVME_REG_VS          0x0008  /* Version */
#define NVME_REG_INTMS       0x000C  /* Interrupt Mask Set */
#define NVME_REG_INTMC       0x0010  /* Interrupt Mask Clear */
#define NVME_REG_CC          0x0014  /* Controller Configuration */
#define NVME_REG_CSTS        0x001C  /* Controller Status */
#define NVME_REG_NSSR        0x0020  /* NVM Subsystem Reset */
#define NVME_REG_AQA         0x0024  /* Admin Queue Attributes */
#define NVME_REG_ASQ         0x0028  /* Admin Submission Queue Base */
#define NVME_REG_ACQ         0x0030  /* Admin Completion Queue Base */

/* CAP register bits */
#define NVME_CAP_MQES(cap)   ((cap) & 0xFFFF)         /* Max Q Entry Size */
#define NVME_CAP_CQR(cap)    (((cap) >> 16) & 0x1)    /* Contig Queues Req */
#define NVME_CAP_AMS(cap)    (((cap) >> 17) & 0x3)    /* Arb Mech Supported */
#define NVME_CAP_TO(cap)     (((cap) >> 24) & 0xFF)   /* Timeout */
#define NVME_CAP_DSTRD(cap)  (((cap) >> 32) & 0xF)    /* Doorbell Stride */
#define NVME_CAP_NSSRS(cap)  (((cap) >> 36) & 0x1)    /* NSSR Supported */
#define NVME_CAP_CSS(cap)    (((cap) >> 37) & 0xFF)   /* Command Sets */
#define NVME_CAP_BPS(cap)    (((cap) >> 45) & 0x1)    /* Boot Partition */

/* CC register bits */
#define NVME_CC_EN           0x00000001  /* Enable */
#define NVME_CC_CSS          0x00000070  /* I/O Command Set */
#define NVME_CC_MPS          0x00000780  /* Memory Page Size */
#define NVME_CC_AMS          0x00003800  /* Arbitration Mechanism */
#define NVME_CC_SHN          0x0000C000  /* Shutdown Notification */
#define NVME_CC_IOSQES       0x00F00000  /* I/O SQ Entry Size */
#define NVME_CC_IOCQES       0x0F000000  /* I/O CQ Entry Size */

/* CSTS register bits */
#define NVME_CSTS_RDY        0x00000001  /* Ready */
#define NVME_CSTS_CFS        0x00000002  /* Controller Fatal Status */
#define NVME_CSTS_SHST       0x0000000C  /* Shutdown Status */
#define NVME_CSTS_NSSRO      0x00000010  /* NSSR Occurred */

/* Perintah Admin */
#define NVME_ADMIN_DELETE_SQ     0x00
#define NVME_ADMIN_CREATE_SQ     0x01
#define NVME_ADMIN_GET_LOG_PAGE  0x02
#define NVME_ADMIN_DELETE_CQ     0x04
#define NVME_ADMIN_CREATE_CQ     0x05
#define NVME_ADMIN_IDENTIFY      0x06
#define NVME_ADMIN_ABORT         0x08
#define NVME_ADMIN_SET_FEATURES  0x09
#define NVME_ADMIN_GET_FEATURES  0x0A
#define NVME_ADMIN_ASYNC_EVENT   0x0C
#define NVME_ADMIN_NS_MANAGE     0x0D
#define NVME_ADMIN_FW_COMMIT     0x10
#define NVME_ADMIN_FW_DOWNLOAD   0x11

/* Perintah NVM */
#define NVME_CMD_FLUSH           0x00
#define NVME_CMD_WRITE           0x01
#define NVME_CMD_READ            0x02
#define NVME_CMD_WRITE_UNCORR    0x04
#define NVME_CMD_COMPARE         0x05
#define NVME_CMD_WRITE_ZEROES    0x08
#define NVME_CMD_DSM             0x09

/* Identify CNS values */
#define NVME_IDENTIFY_NS         0x00
#define NVME_IDENTIFY_CTRL       0x01
#define NVME_IDENTIFY_NS_LIST    0x02

/* Jumlah maksimum */
#define NVME_MAX_NAMESPACES  1024
#define NVME_MAX_QUEUES      16
#define NVME_MAX_Q_ENTRIES   4096

/* Ukuran struktur */
#define NVME_SQ_ENTRY_SIZE   64
#define NVME_CQ_ENTRY_SIZE   16
#define NVME_PAGE_SIZE       4096

/*
 * ===========================================================================
 * STRUKTUR DATA NVME
 * ===========================================================================
 */

/* Submission Queue Entry */
typedef struct {
    tak_bertanda32_t cdw0;          /* Command Dword 0 */
    tak_bertanda32_t nsid;          /* Namespace ID */
    tak_bertanda64_t reserved1;
    tak_bertanda64_t mptr;          /* Metadata Pointer */
    tak_bertanda64_t prp1;          /* PRP Entry 1 */
    tak_bertanda64_t prp2;          /* PRP Entry 2 */
    tak_bertanda32_t cdw10;         /* Command Dword 10 */
    tak_bertanda32_t cdw11;         /* Command Dword 11 */
    tak_bertanda32_t cdw12;         /* Command Dword 12 */
    tak_bertanda32_t cdw13;         /* Command Dword 13 */
    tak_bertanda32_t cdw14;         /* Command Dword 14 */
    tak_bertanda32_t cdw15;         /* Command Dword 15 */
} nvme_sqe_t;

/* Completion Queue Entry */
typedef struct {
    tak_bertanda32_t cdw0;          /* Command Specific */
    tak_bertanda32_t reserved1;
    tak_bertanda16_t sqhd;          /* SQ Head Pointer */
    tak_bertanda16_t sqid;          /* SQ Identifier */
    tak_bertanda16_t cid;           /* Command Identifier */
    tak_bertanda16_t sf;            /* Status Field */
} nvme_cqe_t;

/* Identify Controller Data */
typedef struct {
    tak_bertanda16_t vid;           /* PCI Vendor ID */
    tak_bertanda16_t ssvid;         /* PCI Subsystem Vendor ID */
    tak_bertanda8_t sn[20];         /* Serial Number */
    tak_bertanda8_t mn[40];         /* Model Number */
    tak_bertanda8_t fr[8];          /* Firmware Revision */
    tak_bertanda8_t rab;            /* Recommended Arbitration Burst */
    tak_bertanda8_t ieee[3];        /* IEEE OUI Identifier */
    tak_bertanda8_t cmic;           /* Controller Multi-Path I/O */
    tak_bertanda8_t mdts;           /* Max Data Transfer Size */
    tak_bertanda16_t cntlid;        /* Controller ID */
    tak_bertanda32_t ver;           /* Version */
    tak_bertanda32_t rtd3r;         /* RTD3 Resume Latency */
    tak_bertanda32_t rtd3e;         /* RTD3 Entry Latency */
    tak_bertanda32_t oaes;          /* Optional Async Events */
    tak_bertanda32_t ctratt;        /* Controller Attributes */
    tak_bertanda8_t reserved[176];
    tak_bertanda8_t sqes;           /* SQ Entry Size */
    tak_bertanda8_t cqes;           /* CQ Entry Size */
    tak_bertanda16_t maxcmd;        /* Max Outstanding Commands */
    tak_bertanda32_t nn;            /* Number of Namespaces */
    tak_bertanda16_t oncs;          /* Optional NVM Commands */
    tak_bertanda16_t fuses;         /* Fused Operation Support */
    tak_bertanda8_t fna;            /* Format NVM Attributes */
    tak_bertanda8_t vwc;            /* Volatile Write Cache */
    tak_bertanda16_t awun;          /* Atomic Write Unit Normal */
    tak_bertanda16_t awupf;         /* Atomic Write Unit Power Fail */
    tak_bertanda8_t nvscc;          /* NVM Vendor Specific */
    tak_bertanda8_t reserved2;
    tak_bertanda16_t acwu;          /* Atomic Compare & Write Unit */
    tak_bertanda32_t sgls;          /* SGL Support */
    tak_bertanda8_t reserved3[228];
    tak_bertanda8_t subnqn[256];    /* NVM Subsystem NVMe Qualified Name */
    tak_bertanda8_t reserved4[768];
    tak_bertanda8_t vs[1024];       /* Vendor Specific */
} nvme_identify_ctrl_t;

/* Identify Namespace Data */
typedef struct {
    tak_bertanda64_t nsze;          /* Namespace Size (blocks) */
    tak_bertanda64_t ncap;          /* Namespace Capacity */
    tak_bertanda64_t nuse;          /* Namespace Utilization */
    tak_bertanda8_t nsfeat;         /* Namespace Features */
    tak_bertanda8_t nlbaf;          /* Number of LBA Formats */
    tak_bertanda8_t flbas;          /* Formatted LBA Size */
    tak_bertanda8_t mc;             /* Metadata Capabilities */
    tak_bertanda8_t dpc;            /* Data Protection Capabilities */
    tak_bertanda8_t dps;            /* Data Protection Settings */
    tak_bertanda8_t nmic;           /* Namespace Multi-path */
    tak_bertanda8_t rescap;         /* Reservation Capabilities */
    tak_bertanda8_t fpi;            /* Format Progress Indicator */
    tak_bertanda8_t reserved1[101];
    tak_bertanda64_t lbaf[16];      /* LBA Format Support */
    tak_bertanda8_t reserved2[192];
    tak_bertanda8_t vs[3712];
} nvme_identify_ns_t;

/* Queue NVMe */
typedef struct {
    tak_bertanda32_t magic;
    tak_bertanda16_t id;            /* Queue ID */
    tak_bertanda16_t size;          /* Queue Size */
    tak_bertanda16_t head;          /* Head Pointer */
    tak_bertanda16_t tail;          /* Tail Pointer */
    tak_bertanda16_t cq_head;
    tak_bertanda16_t phase;
    tak_bertanda32_t doorbell_offset;
    nvme_sqe_t *sq;                 /* Submission Queue */
    nvme_cqe_t *cq;                 /* Completion Queue */
    bool_t active;
} nvme_queue_t;

/* Perangkat NVMe */
typedef struct {
    tak_bertanda32_t magic;
    tak_bertanda32_t id;
    bool_t valid;
    bool_t present;
    tak_bertanda32_t nsid;

    /* Kapasitas */
    tak_bertanda64_t total_blocks;
    tak_bertanda64_t size_bytes;
    ukuran_t block_size;

    /* Identifikasi */
    char model[41];
    char serial[21];
    char firmware[9];

    /* Statistik */
    tak_bertanda64_t read_ops;
    tak_bertanda64_t write_ops;
    tak_bertanda64_t errors;
} nvme_device_t;

/* Konteks NVMe */
typedef struct {
    tak_bertanda32_t magic;
    bool_t diinisialisasi;
    volatile tak_bertanda32_t *mmio_base;
    tak_bertanda64_t cap;
    tak_bertanda32_t version;
    tak_bertanda32_t doorbell_stride;
    tak_bertanda32_t page_size;

    /* Queue */
    nvme_queue_t admin_q;
    nvme_queue_t io_q[NVME_MAX_QUEUES];

    /* Namespace */
    nvme_device_t namespace[NVME_MAX_NAMESPACES];
    tak_bertanda32_t jumlah_namespace;

    /* Statistik */
    tak_bertanda64_t total_read;
    tak_bertanda64_t total_write;
} nvme_konteks_t;

/*
 * ===========================================================================
 * VARIABEL GLOBAL
 * ===========================================================================
 */

static nvme_konteks_t g_nvme_konteks;
static bool_t g_nvme_diinisialisasi = SALAH;

/*
 * ===========================================================================
 * FUNGSI MMIO
 * ===========================================================================
 */

/*
 * nvme_mmio_read - Baca register MMIO
 */
static inline tak_bertanda32_t nvme_mmio_read(volatile void *addr)
{
    return *((volatile tak_bertanda32_t *)addr);
}

/*
 * nvme_mmio_read64 - Baca register MMIO 64-bit
 */
static inline tak_bertanda64_t nvme_mmio_read64(volatile void *addr)
{
    return *((volatile tak_bertanda64_t *)addr);
}

/*
 * nvme_mmio_write - Tulis register MMIO
 */
static inline void nvme_mmio_write(volatile void *addr,
                                   tak_bertanda32_t nilai)
{
    *((volatile tak_bertanda32_t *)addr) = nilai;
}

/*
 * ===========================================================================
 * FUNGSI INTERNAL
 * ===========================================================================
 */

/*
 * nvme_doorbell_offset - Hitung offset doorbell
 */
static inline tak_bertanda32_t nvme_doorbell_offset(tak_bertanda32_t qid,
                                                    bool_t is_cq)
{
    tak_bertanda32_t stride = g_nvme_konteks.doorbell_stride;
    return (0x1000 + (qid * 2 + (is_cq ? 1 : 0)) * (4 << stride));
}

/*
 * nvme_ring_doorbell - Ring doorbell
 */
static void nvme_ring_doorbell(tak_bertanda32_t qid, bool_t is_cq,
                               tak_bertanda16_t nilai)
{
    volatile tak_bertanda32_t *db;

    if (g_nvme_konteks.mmio_base == NULL) {
        return;
    }

    db = (volatile tak_bertanda32_t *)
         ((tak_bertanda8_t *)g_nvme_konteks.mmio_base +
          nvme_doorbell_offset(qid, is_cq));

    nvme_mmio_write(db, nilai);
}

/*
 * nvme_wait_ready - Tunggu controller ready
 */
static status_t nvme_wait_ready(bool_t wait_for_ready)
{
    tak_bertanda32_t csts;
    tak_bertanda32_t timeout;
    tak_bertanda32_t to_ms;

    if (g_nvme_konteks.mmio_base == NULL) {
        return STATUS_GAGAL;
    }

    /* Hitung timeout dari CAP.TO (dalam 500ms unit) */
    to_ms = (tak_bertanda32_t)NVME_CAP_TO(g_nvme_konteks.cap) * 500;
    if (to_ms == 0) {
        to_ms = 10000;
    }

    timeout = to_ms;
    while (timeout > 0) {
        csts = nvme_mmio_read((volatile void *)
               ((tak_bertanda8_t *)g_nvme_konteks.mmio_base + NVME_REG_CSTS));

        if (wait_for_ready) {
            if (csts & NVME_CSTS_RDY) {
                return STATUS_BERHASIL;
            }
        } else {
            if ((csts & NVME_CSTS_RDY) == 0) {
                return STATUS_BERHASIL;
            }
        }

        cpu_delay_ms(1);
        timeout--;
    }

    return STATUS_TIMEOUT;
}

/*
 * nvme_enable_controller - Enable controller
 */
static status_t nvme_enable_controller(void)
{
    tak_bertanda32_t cc;

    if (g_nvme_konteks.mmio_base == NULL) {
        return STATUS_GAGAL;
    }

    /* Tunggu controller tidak ready */
    nvme_wait_ready(SALAH);

    /* Konfigurasi controller */
    cc = NVME_CC_EN;
    cc |= (0 << 7);   /* MPS = 0 (4KB page) */
    cc |= (6 << 20);  /* IOSQES = 6 (64 bytes) */
    cc |= (4 << 24);  /* IOCQES = 4 (16 bytes) */

    nvme_mmio_write((volatile void *)
                    ((tak_bertanda8_t *)g_nvme_konteks.mmio_base +
                     NVME_REG_CC), cc);

    /* Tunggu ready */
    return nvme_wait_ready(BENAR);
}

/*
 * nvme_disable_controller - Disable controller
 */
static status_t nvme_disable_controller(void)
{
    tak_bertanda32_t cc;

    if (g_nvme_konteks.mmio_base == NULL) {
        return STATUS_GAGAL;
    }

    /* Clear enable bit */
    cc = nvme_mmio_read((volatile void *)
                        ((tak_bertanda8_t *)g_nvme_konteks.mmio_base +
                         NVME_REG_CC));
    cc &= ~NVME_CC_EN;
    nvme_mmio_write((volatile void *)
                    ((tak_bertanda8_t *)g_nvme_konteks.mmio_base +
                     NVME_REG_CC), cc);

    return nvme_wait_ready(SALAH);
}

/*
 * nvme_admin_cmd - Eksekusi perintah admin
 */
static status_t nvme_admin_cmd(nvme_sqe_t *cmd, nvme_cqe_t *result)
{
    nvme_queue_t *q;
    nvme_sqe_t *sqe;
    nvme_cqe_t *cqe;
    tak_bertanda16_t cid;
    tak_bertanda32_t timeout;
    static tak_bertanda16_t next_cid = 1;

    if (cmd == NULL || result == NULL) {
        return STATUS_PARAM_NULL;
    }

    q = &g_nvme_konteks.admin_q;
    if (!q->active) {
        return STATUS_GAGAL;
    }

    /* Dapatkan slot di SQ */
    sqe = &q->sq[q->tail];
    kernel_memcpy(sqe, cmd, sizeof(nvme_sqe_t));

    cid = next_cid++;
    if (next_cid == 0) {
        next_cid = 1;
    }
    sqe->cdw0 &= 0xFFFF0000;
    sqe->cdw0 |= cid;

    /* Update tail dan ring doorbell */
    q->tail = (q->tail + 1) % q->size;
    nvme_ring_doorbell(0, SALAH, q->tail);

    /* Tunggu completion */
    timeout = 5000;
    while (timeout > 0) {
        cqe = &q->cq[q->cq_head];

        if ((cqe->sf & 0x0001) == q->phase) {
            if ((cqe->cid) == cid) {
                kernel_memcpy(result, cqe, sizeof(nvme_cqe_t));

                /* Update head dan ring doorbell */
                q->cq_head = (q->cq_head + 1) % q->size;
                if (q->cq_head == 0) {
                    q->phase ^= 1;
                }
                nvme_ring_doorbell(0, BENAR, q->cq_head);

                /* Cek status */
                if ((cqe->sf >> 1) & 0x01) {
                    return STATUS_IO_ERROR;
                }
                return STATUS_BERHASIL;
            }
        }

        cpu_delay_ms(1);
        timeout--;
    }

    return STATUS_TIMEOUT;
}

/*
 * nvme_identify_controller - Identifikasi controller
 */
static status_t nvme_identify_controller(nvme_identify_ctrl_t *ctrl)
{
    nvme_sqe_t cmd;
    nvme_cqe_t result;

    if (ctrl == NULL) {
        return STATUS_PARAM_NULL;
    }

    kernel_memset(&cmd, 0, sizeof(cmd));
    kernel_memset(ctrl, 0, sizeof(*ctrl));

    cmd.cdw0 = NVME_ADMIN_IDENTIFY;
    cmd.nsid = 0;
    cmd.prp1 = (tak_bertanda64_t)(uintptr_t)ctrl;
    cmd.cdw10 = NVME_IDENTIFY_CTRL;

    return nvme_admin_cmd(&cmd, &result);
}

/*
 * nvme_identify_namespace - Identifikasi namespace
 */
static status_t nvme_identify_namespace(tak_bertanda32_t nsid,
                                        nvme_identify_ns_t *ns)
{
    nvme_sqe_t cmd;
    nvme_cqe_t result;

    if (ns == NULL) {
        return STATUS_PARAM_NULL;
    }

    kernel_memset(&cmd, 0, sizeof(cmd));
    kernel_memset(ns, 0, sizeof(*ns));

    cmd.cdw0 = NVME_ADMIN_IDENTIFY;
    cmd.nsid = nsid;
    cmd.prp1 = (tak_bertanda64_t)(uintptr_t)ns;
    cmd.cdw10 = NVME_IDENTIFY_NS;

    return nvme_admin_cmd(&cmd, &result);
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

/*
 * nvme_init - Inisialisasi driver NVMe
 */
status_t nvme_init(void)
{
    nvme_identify_ctrl_t ctrl;
    nvme_identify_ns_t ns;
    status_t hasil;
    tak_bertanda32_t i;
    tak_bertanda32_t j;

    if (g_nvme_diinisialisasi) {
        return STATUS_SUDAH_ADA;
    }

    kernel_memset(&g_nvme_konteks, 0, sizeof(nvme_konteks_t));

    /* TODO: Dapatkan MMIO base dari PCI */
    /* g_nvme_konteks.mmio_base = pci_get_bar(...); */

    /* Baca capability */
    if (g_nvme_konteks.mmio_base != NULL) {
        g_nvme_konteks.cap = nvme_mmio_read64(
            (volatile void *)((tak_bertanda8_t *)g_nvme_konteks.mmio_base +
                              NVME_REG_CAP));
        g_nvme_konteks.version = nvme_mmio_read(
            (volatile void *)((tak_bertanda8_t *)g_nvme_konteks.mmio_base +
                              NVME_REG_VS));
        g_nvme_konteks.doorbell_stride =
            (tak_bertanda32_t)NVME_CAP_DSTRD(g_nvme_konteks.cap);
        g_nvme_konteks.page_size = NVME_PAGE_SIZE;
    }

    /* Setup admin queue */
    g_nvme_konteks.admin_q.magic = NVME_QUEUE_MAGIC;
    g_nvme_konteks.admin_q.id = 0;
    g_nvme_konteks.admin_q.size = 32;
    g_nvme_konteks.admin_q.phase = 1;

    /* TODO: Alokasi memori untuk SQ dan CQ */
    /* g_nvme_konteks.admin_q.sq = kmalloc_dma(...); */
    /* g_nvme_konteks.admin_q.cq = kmalloc_dma(...); */

    if (g_nvme_konteks.mmio_base != NULL) {
        /* Configure admin queue */
        nvme_mmio_write((volatile void *)
                        ((tak_bertanda8_t *)g_nvme_konteks.mmio_base +
                         NVME_REG_AQA),
                        ((g_nvme_konteks.admin_q.size - 1) << 16) |
                        (g_nvme_konteks.admin_q.size - 1));

        /* Set ASQ dan ACQ */
        /* nvme_mmio_write(..., ASQ, sq_phys); */
        /* nvme_mmio_write(..., ACQ, cq_phys); */

        /* Enable controller */
        hasil = nvme_enable_controller();
        if (hasil != STATUS_BERHASIL) {
            return hasil;
        }

        g_nvme_konteks.admin_q.active = BENAR;

        /* Identifikasi controller */
        hasil = nvme_identify_controller(&ctrl);
        if (hasil != STATUS_BERHASIL) {
            return hasil;
        }

        /* Parse info controller */
        {
            char *p;
            for (i = 0; i < 40 && i < sizeof(ctrl.mn); i++) {
                g_nvme_konteks.namespace[0].model[i] = ctrl.mn[i];
            }
            g_nvme_konteks.namespace[0].model[i] = '\0';

            for (i = 0; i < 20 && i < sizeof(ctrl.sn); i++) {
                g_nvme_konteks.namespace[0].serial[i] = ctrl.sn[i];
            }
            g_nvme_konteks.namespace[0].serial[i] = '\0';

            for (i = 0; i < 8 && i < sizeof(ctrl.fr); i++) {
                g_nvme_konteks.namespace[0].firmware[i] = ctrl.fr[i];
            }
            g_nvme_konteks.namespace[0].firmware[i] = '\0';
        }

        /* Identifikasi namespace */
        for (j = 1; j <= ctrl.nn && j < NVME_MAX_NAMESPACES; j++) {
            hasil = nvme_identify_namespace(j, &ns);
            if (hasil == STATUS_BERHASIL && ns.nsze > 0) {
                nvme_device_t *dev = &g_nvme_konteks.namespace[j];

                dev->magic = NVME_DEVICE_MAGIC;
                dev->id = j;
                dev->nsid = j;
                dev->present = BENAR;
                dev->valid = BENAR;
                dev->total_blocks = ns.nsze;
                dev->block_size = 512;  /* Default LBA size */

                /* Hitung ukuran */
                {
                    tak_bertanda8_t lbaf = ns.flbas & 0x0F;
                    tak_bertanda32_t lba_size =
                        1 << ((ns.lbaf[lbaf] >> 0) & 0xFF);
                    dev->block_size = lba_size;
                }

                dev->size_bytes = dev->total_blocks * dev->block_size;

                /* Copy identifikasi dari controller */
                for (i = 0; i < 41; i++) {
                    dev->model[i] =
                        g_nvme_konteks.namespace[0].model[i];
                }
                for (i = 0; i < 21; i++) {
                    dev->serial[i] =
                        g_nvme_konteks.namespace[0].serial[i];
                }
                for (i = 0; i < 9; i++) {
                    dev->firmware[i] =
                        g_nvme_konteks.namespace[0].firmware[i];
                }

                g_nvme_konteks.jumlah_namespace++;
            }
        }
    }

    g_nvme_konteks.magic = NVME_MAGIC;
    g_nvme_konteks.diinisialisasi = BENAR;
    g_nvme_diinisialisasi = BENAR;

    return STATUS_BERHASIL;
}

/*
 * nvme_shutdown - Matikan driver NVMe
 */
status_t nvme_shutdown(void)
{
    if (!g_nvme_diinisialisasi) {
        return STATUS_BERHASIL;
    }

    /* Disable controller */
    if (g_nvme_konteks.mmio_base != NULL) {
        nvme_disable_controller();
    }

    /* Free queue memory */
    /* if (g_nvme_konteks.admin_q.sq) kfree_dma(...); */

    g_nvme_konteks.magic = 0;
    g_nvme_konteks.diinisialisasi = SALAH;
    g_nvme_diinisialisasi = SALAH;

    return STATUS_BERHASIL;
}

/*
 * nvme_baca - Baca blok dari namespace NVMe
 *
 * Parameter:
 *   nsid - Namespace ID
 *   lba - Alamat LBA
 *   buffer - Buffer output
 *   count - Jumlah blok
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t nvme_baca(tak_bertanda32_t nsid, tak_bertanda64_t lba,
                   void *buffer, ukuran_t count)
{
    nvme_device_t *dev;
    nvme_sqe_t cmd;
    nvme_cqe_t result;
    status_t hasil;

    if (!g_nvme_diinisialisasi) {
        return STATUS_GAGAL;
    }

    if (buffer == NULL || count == 0) {
        return STATUS_PARAM_INVALID;
    }

    if (nsid == 0 || nsid >= NVME_MAX_NAMESPACES) {
        return STATUS_PARAM_INVALID;
    }

    dev = &g_nvme_konteks.namespace[nsid];
    if (!dev->valid) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    /* Buat perintah read */
    kernel_memset(&cmd, 0, sizeof(cmd));

    cmd.cdw0 = NVME_CMD_READ;
    cmd.nsid = nsid;
    cmd.prp1 = (tak_bertanda64_t)(uintptr_t)buffer;
    cmd.cdw10 = (tak_bertanda32_t)(lba & 0xFFFFFFFF);
    cmd.cdw11 = (tak_bertanda32_t)((lba >> 32) & 0xFFFFFFFF);
    cmd.cdw12 = (tak_bertanda32_t)((count - 1) & 0xFFFF);

    /* Untuk sekarang, gunakan admin queue */
    hasil = nvme_admin_cmd(&cmd, &result);

    if (hasil == STATUS_BERHASIL) {
        dev->read_ops++;
        g_nvme_konteks.total_read += count;
    } else {
        dev->errors++;
    }

    return hasil;
}

/*
 * nvme_tulis - Tulis blok ke namespace NVMe
 *
 * Parameter:
 *   nsid - Namespace ID
 *   lba - Alamat LBA
 *   buffer - Buffer input
 *   count - Jumlah blok
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t nvme_tulis(tak_bertanda32_t nsid, tak_bertanda64_t lba,
                    const void *buffer, ukuran_t count)
{
    nvme_device_t *dev;
    nvme_sqe_t cmd;
    nvme_cqe_t result;
    status_t hasil;

    if (!g_nvme_diinisialisasi) {
        return STATUS_GAGAL;
    }

    if (buffer == NULL || count == 0) {
        return STATUS_PARAM_INVALID;
    }

    if (nsid == 0 || nsid >= NVME_MAX_NAMESPACES) {
        return STATUS_PARAM_INVALID;
    }

    dev = &g_nvme_konteks.namespace[nsid];
    if (!dev->valid) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    /* Buat perintah write */
    kernel_memset(&cmd, 0, sizeof(cmd));

    cmd.cdw0 = NVME_CMD_WRITE;
    cmd.nsid = nsid;
    cmd.prp1 = (tak_bertanda64_t)(uintptr_t)buffer;
    cmd.cdw10 = (tak_bertanda32_t)(lba & 0xFFFFFFFF);
    cmd.cdw11 = (tak_bertanda32_t)((lba >> 32) & 0xFFFFFFFF);
    cmd.cdw12 = (tak_bertanda32_t)((count - 1) & 0xFFFF);

    hasil = nvme_admin_cmd(&cmd, &result);

    if (hasil == STATUS_BERHASIL) {
        dev->write_ops++;
        g_nvme_konteks.total_write += count;
    } else {
        dev->errors++;
    }

    return hasil;
}

/*
 * nvme_cetak_info - Cetak informasi perangkat NVMe
 */
void nvme_cetak_info(void)
{
    tak_bertanda32_t i;

    if (!g_nvme_diinisialisasi) {
        return;
    }

    kernel_printf("\n=== Perangkat NVMe ===\n\n");

    kernel_printf("NVMe Versi: %u.%u.%u\n",
                 (g_nvme_konteks.version >> 16) & 0xFFFF,
                 (g_nvme_konteks.version >> 8) & 0xFF,
                 g_nvme_konteks.version & 0xFF);

    kernel_printf("Jumlah Namespace: %u\n\n",
                 g_nvme_konteks.jumlah_namespace);

    for (i = 1; i < NVME_MAX_NAMESPACES; i++) {
        nvme_device_t *dev = &g_nvme_konteks.namespace[i];

        if (dev->valid && dev->present) {
            kernel_printf("nvme%un1: ", i);
            kernel_printf("%s\n", dev->model);
            kernel_printf("  Serial: %s\n", dev->serial);
            kernel_printf("  Kapasitas: %lu MB\n",
                         dev->size_bytes / (1024 * 1024));
            kernel_printf("  Blok: %lu (%u byte/block)\n",
                         dev->total_blocks, dev->block_size);
            kernel_printf("\n");
        }
    }
}
