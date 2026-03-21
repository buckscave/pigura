/*
 * PIGURA OS - CPU_ARMV7.C
 * ------------------------
 * Implementasi fungsi CPU untuk arsitektur ARMv7 (AArch32).
 *
 * Berkas ini berisi fungsi-fungsi untuk mengelola CPU ARMv7,
 * termasuk inisialisasi, identifikasi, dan kontrol CPU.
 *
 * Arsitektur: ARMv7 (Cortex-A series)
 * Versi: 1.0
 */

#include "../../inti/types.h"
#include "../../inti/konstanta.h"
#include "../../inti/hal/hal.h"

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

/* Register CP15 opcodes */
#define CP15_MIDR       0, c0, c0, 0    /* Main ID Register */
#define CP15_CTR        0, c0, c0, 1    /* Cache Type Register */
#define CP15_TCMTR      0, c0, c0, 2    /* TCM Type Register */
#define CP15_TLBTR      0, c0, c0, 3    /* TLB Type Register */
#define CP15_MPIDR      0, c0, c0, 5    /* Multiprocessor Affinity */
#define CP15_REVIDR     0, c0, c0, 6    /* Revision ID Register */

#define CP15_PFR0       0, c0, c1, 0    /* Processor Feature 0 */
#define CP15_PFR1       0, c0, c1, 1    /* Processor Feature 1 */
#define CP15_DFR0       0, c0, c1, 2    /* Debug Feature 0 */
#define CP15_AFR0       0, c0, c1, 3    /* Auxiliary Feature 0 */
#define CP15_MMFR0      0, c0, c1, 4    /* Memory Model Feature 0 */
#define CP15_MMFR1      0, c0, c1, 5    /* Memory Model Feature 1 */
#define CP15_MMFR2      0, c0, c1, 6    /* Memory Model Feature 2 */
#define CP15_MMFR3      0, c0, c1, 7    /* Memory Model Feature 3 */

#define CP15_ISAR0      0, c0, c2, 0    /* ISA Feature 0 */
#define CP15_ISAR1      0, c0, c2, 1    /* ISA Feature 1 */
#define CP15_ISAR2      0, c0, c2, 2    /* ISA Feature 2 */
#define CP15_ISAR3      0, c0, c2, 3    /* ISA Feature 3 */
#define CP15_ISAR4      0, c0, c2, 4    /* ISA Feature 4 */
#define CP15_ISAR5      0, c0, c2, 5    /* ISA Feature 5 */

#define CP15_SCTLR      0, c1, c0, 0    /* System Control Register */
#define CP15_ACTLR      0, c1, c0, 1    /* Auxiliary Control Register */
#define CP15_CPACR      0, c1, c0, 2    /* Coprocessor Access Control */

#define CP15_TTBR0      0, c2, c0, 0    /* Translation Table Base 0 */
#define CP15_TTBR1      0, c2, c0, 1    /* Translation Table Base 1 */
#define CP15_TTBCR      0, c2, c0, 2    /* TTB Control Register */

#define CP15_DACR       0, c3, c0, 0    /* Domain Access Control */
#define CP15_DFSR       0, c5, c0, 0    /* Data Fault Status */
#define CP15_IFSR       0, c5, c0, 1    /* Instruction Fault Status */
#define CP15_DFAR       0, c6, c0, 0    /* Data Fault Address */
#define CP15_IFAR       0, c6, c0, 2    /* Instruction Fault Address */

#define CP15_ICIALLU    0, c7, c5, 0    /* I-cache invalidate all */
#define CP15_ICIMVAU    0, c7, c5, 1    /* I-cache invalidate by VA */
#define CP15_BPIMVA     0, c7, c5, 7    /* Branch predictor inval by VA */
#define CP15_BPIALL     0, c7, c5, 6    /* Branch predictor inval all */
#define CP15_DCISW      0, c7, c6, 2    /* D-cache invalidate by set/way */
#define CP15_DCCISW     0, c7, c14, 2   /* D-cache clean&inval by set/way */
#define CP15_DCCMVAC    0, c7, c10, 1   /* D-cache clean by VA to PoC */
#define CP15_DCIMVAC    0, c7, c6, 1    /* D-cache invalidate by VA */
#define CP15_DCCIMVAC   0, c7, c14, 1   /* D-cache clean&inval by VA */
#define CP15_DCIMVA     0, c7, c6, 1    /* D-cache invalidate by MVA */

#define CP15_TLBIALL    0, c8, c7, 0    /* TLB Invalidate All */
#define CP15_TLBIMVA    0, c8, c7, 1    /* TLB Invalidate by VA */

#define CP15_VBAR       0, c12, c0, 0   /* Vector Base Address Register */
#define CP15_MVBAR      0, c12, c0, 1   /* Monitor Vector Base Address */

#define CP15_CSSELR     0, c0, c0, 0    /* Cache Size Selection Register */
#define CCSELR_L1DATA   0               /* Level 1 Data cache */
#define CCSELR_L1INST   1               /* Level 1 Instruction cache */
#define CCSELR_L2       2               /* Level 2 cache */

/* SCTLR bits */
#define SCTLR_M         (1 << 0)        /* MMU enable */
#define SCTLR_A         (1 << 1)        /* Alignment check enable */
#define SCTLR_C         (1 << 2)        /* Data cache enable */
#define SCTLR_CP15BEN   (1 << 5)        /* CP15 barrier enable */
#define SCTLR_I         (1 << 12)       /* Instruction cache enable */
#define SCTLR_V         (1 << 13)       /* High vectors */
#define SCTLR_Z         (1 << 11)       /* Branch prediction enable */
#define SCTLR_FI        (1 << 21)       /* Fast interrupt config */
#define SCTLR_U         (1 << 22)       /* Unaligned access */
#define SCTLR_VE        (1 << 24)       /* Interrupt vectors enable */
#define SCTLR_TE        (1 << 30)       /* Thumb exception enable */

/* Cache Line size */
#define CACHE_LINE_MIN  16
#define CACHE_LINE_MAX  64

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Informasi CPU yang terdeteksi */
static hal_cpu_info_t g_cpu_info;

/* State interupsi tersimpan */
static tak_bertanda32_t g_irq_state;

/* Cache line size */
static tak_bertanda32_t g_cache_line_size;

/*
 * ============================================================================
 * FUNGSI BANTUAN CP15 (CP15 HELPER FUNCTIONS)
 * ============================================================================
 */

/*
 * cp15_read
 * ---------
 * Baca register CP15.
 *
 * Parameter:
 *   op1, c1, c2, op2 - Opcode CP15
 *
 * Return: Nilai register
 */
static inline tak_bertanda32_t cp15_read(tak_bertanda32_t op1,
    tak_bertanda32_t c1, tak_bertanda32_t c2, tak_bertanda32_t op2)
{
    tak_bertanda32_t val;
    __asm__ __volatile__(
        "mrc p15, %1, %0, %2, %3, %4"
        : "=r"(val)
        : "i"(op1), "i"(c1), "i"(c2), "i"(op2)
    );
    return val;
}

/*
 * cp15_write
 * ----------
 * Tulis register CP15.
 *
 * Parameter:
 *   op1, c1, c2, op2 - Opcode CP15
 *   val - Nilai yang akan ditulis
 */
static inline void cp15_write(tak_bertanda32_t op1, tak_bertanda32_t c1,
    tak_bertanda32_t c2, tak_bertanda32_t op2, tak_bertanda32_t val)
{
    __asm__ __volatile__(
        "mcr p15, %0, %1, %2, %3, %4"
        :
        : "i"(op1), "r"(val), "i"(c1), "i"(c2), "i"(op2)
        : "memory"
    );
}

/*
 * ============================================================================
 * FUNGSI INISIALISASI CPU (CPU INITIALIZATION FUNCTIONS)
 * ============================================================================
 */

/*
 * cpu_deteksi_vendor
 * ------------------
 * Deteksi vendor CPU dari implementer code.
 *
 * Parameter:
 *   implementer - Kode implementer dari MIDR
 *
 * Return: String nama vendor
 */
static const char *cpu_deteksi_vendor(tak_bertanda8_t implementer)
{
    switch (implementer) {
        case 0x41: return "ARM";
        case 0x42: return "Broadcom";
        case 0x43: return "Cavium";
        case 0x44: return "DEC";
        case 0x46: return "Fujitsu";
        case 0x48: return "HiSilicon";
        case 0x49: return "Infineon";
        case 0x4D: return "Motorola";
        case 0x4E: return "NVIDIA";
        case 0x50: return "APM";
        case 0x51: return "Qualcomm";
        case 0x53: return "Samsung";
        case 0x56: return "Marvell";
        case 0x61: return "Apple";
        case 0x69: return "Intel";
        default:   return "Unknown";
    }
}

/*
 * cpu_deteksi_arch
 * ----------------
 * Deteksi arsitektur CPU dari MIDR.
 *
 * Parameter:
 *   arch - Architecture code dari MIDR
 *
 * Return: String nama arsitektur
 */
static const char *cpu_deteksi_arch(tak_bertanda8_t arch)
{
    switch (arch) {
        case 0x0: return "ARMv3";
        case 0x1: return "ARMv4";
        case 0x2: return "ARMv4T";
        case 0x3: return "ARMv5";
        case 0x4: return "ARMv5T";
        case 0x5: return "ARMv5TE";
        case 0x6: return "ARMv5TEJ";
        case 0x7: return "ARMv6";
        case 0xF: return "ARMv7";
        default:  return "Unknown";
    }
}

/*
 * cpu_baca_cache_info
 * -------------------
 * Baca informasi cache dari CTR dan CCSIDR.
 *
 * Parameter:
 *   info - Pointer ke struktur info CPU
 */
static void cpu_baca_cache_info(hal_cpu_info_t *info)
{
    tak_bertanda32_t ctr;
    tak_bertanda32_t ccsidr;
    tak_bertanda32_t linesize;
    tak_bertanda32_t assoc;
    tak_bertanda32_t sets;
    tak_bertanda32_t size;

    /* Baca Cache Type Register */
    ctr = cp15_read(CP15_CTR);

    /* Extract cache line size dari CTR */
    linesize = ((ctr >> 0) & 0xF);
    if (linesize == 0) {
        linesize = CACHE_LINE_MIN;
    } else {
        linesize = 4 << linesize;  /* 2^(n+2) words, 1 word = 4 bytes */
    }
    g_cache_line_size = linesize;
    info->cache_line = linesize;

    /* Baca L1 Data Cache info */
    cp15_write(CP15_CSSELR, CCSELR_L1DATA);
    __asm__ __volatile__("isb");

    ccsidr = cp15_read(0, c0, c0, 0);  /* CCSIDR */

    /* Parse CCSIDR */
    linesize = ((ccsidr >> 0) & 0x7) + 4;    /* Line size log2 */
    assoc = ((ccsidr >> 3) & 0x3FF) + 1;     /* Associativity */
    sets = ((ccsidr >> 13) & 0x7FFF) + 1;    /* Number of sets */

    /* Hitung ukuran cache: sets * assoc * line_size */
    size = sets * assoc * (1 << linesize);
    info->cache_l1 = size / 1024;  /* Dalam KB */

    /* Baca L2 Cache info jika ada */
    cp15_write(CP15_CSSELR, CCSELR_L2);
    __asm__ __volatile__("isb");

    ccsidr = cp15_read(0, c0, c0, 0);

    if (ccsidr != 0) {
        linesize = ((ccsidr >> 0) & 0x7) + 4;
        assoc = ((ccsidr >> 3) & 0x3FF) + 1;
        sets = ((ccsidr >> 13) & 0x7FFF) + 1;
        size = sets * assoc * (1 << linesize);
        info->cache_l2 = size / 1024;  /* Dalam KB */
    } else {
        info->cache_l2 = 0;
    }

    info->cache_l3 = 0;  /* ARMv7 biasanya tidak ada L3 */
}

/*
 * hal_cpu_init
 * ------------
 * Inisialisasi subsistem CPU ARMv7.
 *
 * Return: Status operasi
 */
status_t hal_cpu_init(void)
{
    tak_bertanda32_t midr;
    tak_bertanda32_t mpidr;
    tak_bertanda32_t pfr0;
    tak_bertanda32_t pfr1;
    tak_bertanda8_t implementer;
    tak_bertanda8_t variant;
    tak_bertanda8_t arch;
    tak_bertanda16_t partno;
    tak_bertanda8_t revision;
    const char *vendor_str;

    /* Baca Main ID Register (MIDR) */
    midr = cp15_read(CP15_MIDR);

    /* Parse MIDR */
    implementer = (midr >> 24) & 0xFF;
    variant = (midr >> 20) & 0xF;
    arch = (midr >> 16) & 0xF;
    partno = (midr >> 4) & 0xFFF;
    revision = midr & 0xF;

    /* Dapatkan nama vendor */
    vendor_str = cpu_deteksi_vendor(implementer);

    /* Isi struktur info CPU */
    g_cpu_info.id = 0;
    g_cpu_info.apic_id = 0;
    kernel_strncpy(g_cpu_info.vendor, vendor_str, 12);
    g_cpu_info.vendor[12] = '\0';

    /* Format brand string */
    kernel_snprintf(g_cpu_info.brand, 48, "%s %s r%dp%d",
        vendor_str,
        cpu_deteksi_arch(arch),
        variant,
        revision);

    g_cpu_info.family = arch;
    g_cpu_info.model = partno;
    g_cpu_info.stepping = revision;
    g_cpu_info.features = 0;

    /* Baca Processor Feature Registers */
    pfr0 = cp15_read(CP15_PFR0);
    pfr1 = cp15_read(CP15_PFR1);

    /* Deteksi fitur dari PFR0 */
    if ((pfr0 >> 12) & 0xF) {
        g_cpu_info.features |= (1 << 0);  /* VFP */
    }
    if ((pfr0 >> 16) & 0xF) {
        g_cpu_info.features |= (1 << 1);  /* Advanced SIMD */
    }

    /* Baca Multiprocessor Affinity Register */
    mpidr = cp15_read(CP15_MPIDR);
    g_cpu_info.apic_id = mpidr & 0x03;  /* CPU ID dalam cluster */

    /* Deteksi jumlah core (Single CPU assumption untuk sekarang) */
    g_cpu_info.cores = 1;
    g_cpu_info.threads = 1;

    /* Baca info cache */
    cpu_baca_cache_info(&g_cpu_info);

    /* Default frekuensi (akan diupdate oleh timer init) */
    g_cpu_info.freq_mhz = 0;

    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI KONTROL CPU (CPU CONTROL FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_cpu_halt
 * ------------
 * Hentikan CPU (tidak pernah return).
 */
void hal_cpu_halt(void)
{
    /* Disable interupsi */
    __asm__ __volatile__("cpsid aif");

    /* Loop forever dengan WFI */
    while (1) {
        __asm__ __volatile__("wfi");
    }
}

/*
 * hal_cpu_reset
 * -------------
 * Reset CPU.
 *
 * Parameter:
 *   hard - Jika BENAR, lakukan hard reset
 */
void hal_cpu_reset(bool_t hard)
{
    (void)hard;  /* Tidak digunakan untuk sekarang */

    /* Disable interupsi */
    __asm__ __volatile__("cpsid aif");

    /*
     * Reset melalui sistem register.
     * Ini adalah contoh untuk QEMU virt machine.
     * Di hardware nyata, gunakan watchdog atau PSCI.
     */
    hal_mmio_write_32((void *)0x04000000, 0x84000009);  /* PSCI SYSTEM_RESET */

    /* Jika gagal, loop forever */
    while (1) {
        __asm__ __volatile__("wfi");
    }
}

/*
 * hal_cpu_get_id
 * --------------
 * Dapatkan ID CPU saat ini.
 *
 * Return: ID CPU
 */
tak_bertanda32_t hal_cpu_get_id(void)
{
    tak_bertanda32_t mpidr;

    mpidr = cp15_read(CP15_MPIDR);
    return mpidr & 0x03;
}

/*
 * hal_cpu_get_info
 * ----------------
 * Dapatkan informasi CPU.
 *
 * Parameter:
 *   info - Pointer ke buffer untuk menyimpan informasi
 *
 * Return: Status operasi
 */
status_t hal_cpu_get_info(hal_cpu_info_t *info)
{
    if (info == NULL) {
        return STATUS_PARAM_INVALID;
    }

    kernel_memcpy(info, &g_cpu_info, sizeof(hal_cpu_info_t));
    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI INTERUPSI CPU (CPU INTERRUPT FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_cpu_enable_interrupts
 * -------------------------
 * Aktifkan interupsi CPU.
 */
void hal_cpu_enable_interrupts(void)
{
    __asm__ __volatile__("cpsie aif");
}

/*
 * hal_cpu_disable_interrupts
 * --------------------------
 * Nonaktifkan interupsi CPU.
 */
void hal_cpu_disable_interrupts(void)
{
    __asm__ __volatile__("cpsid aif");
}

/*
 * hal_cpu_save_interrupts
 * -----------------------
 * Simpan state interupsi dan nonaktifkan.
 *
 * Return: State interupsi sebelumnya
 */
tak_bertanda32_t hal_cpu_save_interrupts(void)
{
    tak_bertanda32_t cpsr;

    /* Baca CPSR */
    __asm__ __volatile__(
        "mrs %0, cpsr"
        : "=r"(cpsr)
    );

    /* Disable interupsi */
    __asm__ __volatile__("cpsid aif");

    /* Kembalikan state sebelumnya */
    return cpsr;
}

/*
 * hal_cpu_restore_interrupts
 * --------------------------
 * Restore state interupsi.
 *
 * Parameter:
 *   state - State yang disimpan dari save_interrupts
 */
void hal_cpu_restore_interrupts(tak_bertanda32_t state)
{
    tak_bertanda32_t cpsr;

    /* Baca CPSR saat ini */
    __asm__ __volatile__(
        "mrs %0, cpsr"
        : "=r"(cpsr)
    );

    /* Preserve mode bits dan restore IRQ/FIQ bits */
    cpsr = (cpsr & ~0x1C0) | (state & 0x1C0);

    /* Tulis CPSR */
    __asm__ __volatile__(
        "msr cpsr_c, %0"
        :
        : "r"(cpsr)
    );
}

/*
 * ============================================================================
 * FUNGSI TLB (TLB FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_cpu_invalidate_tlb
 * ----------------------
 * Invalidate TLB untuk alamat tertentu.
 *
 * Parameter:
 *   addr - Alamat virtual
 */
void hal_cpu_invalidate_tlb(void *addr)
{
    tak_bertanda32_t va = (tak_bertanda32_t)(tak_bertanda32_t)addr;

    /* Invalidate TLB entry by MVA */
    __asm__ __volatile__(
        "mcr p15, 0, %0, c8, c7, 1"
        :
        : "r"(va)
        : "memory"
    );

    /* Data synchronization barrier */
    __asm__ __volatile__("dsb");
    __asm__ __volatile__("isb");
}

/*
 * hal_cpu_invalidate_tlb_all
 * --------------------------
 * Invalidate seluruh TLB.
 */
void hal_cpu_invalidate_tlb_all(void)
{
    /* Invalidate entire TLB */
    __asm__ __volatile__(
        "mcr p15, 0, %0, c8, c7, 0"
        :
        : "r"(0)
        : "memory"
    );

    /* Data synchronization barrier */
    __asm__ __volatile__("dsb");
    __asm__ __volatile__("isb");
}

/*
 * ============================================================================
 * FUNGSI CACHE (CACHE FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_cpu_cache_flush
 * -------------------
 * Flush cache CPU.
 */
void hal_cpu_cache_flush(void)
{
    tak_bertanda32_t ccsidr;
    tak_bertanda32_t sets;
    tak_bertanda32_t ways;
    tak_bertanda32_t line_size;
    tak_bertanda32_t way_shift;
    tak_bertanda32_t set_shift;
    tak_bertanda32_t way;
    tak_bertanda32_t set;

    /* Clean dan invalidate L1 Data Cache by set/way */

    /* Select L1 Data Cache */
    cp15_write(CP15_CSSELR, CCSELR_L1DATA);
    __asm__ __volatile__("isb");

    /* Read CCSIDR */
    ccsidr = cp15_read(0, c0, c0, 0);

    /* Calculate parameters */
    line_size = 1 << (((ccsidr >> 0) & 0x7) + 4);
    sets = ((ccsidr >> 13) & 0x7FFF) + 1;
    ways = ((ccsidr >> 3) & 0x3FF) + 1;

    /* Calculate shifts for set/way format */
    way_shift = 32 - __builtin_clz(ways - 1);
    set_shift = line_size;

    /* Clean and invalidate each line */
    for (way = 0; way < ways; way++) {
        for (set = 0; set < sets; set++) {
            tak_bertanda32_t val = (way << way_shift) |
                                   (set << set_shift);
            __asm__ __volatile__(
                "mcr p15, 0, %0, c7, c14, 2"
                :
                : "r"(val)
                : "memory"
            );
        }
    }

    /* Invalidate entire I-cache */
    __asm__ __volatile__(
        "mcr p15, 0, %0, c7, c5, 0"
        :
        : "r"(0)
        : "memory"
    );

    /* Branch predictor invalidate all */
    __asm__ __volatile__(
        "mcr p15, 0, %0, c7, c5, 6"
        :
        : "r"(0)
        : "memory"
    );

    /* Data synchronization barrier */
    __asm__ __volatile__("dsb");
    __asm__ __volatile__("isb");
}

/*
 * hal_cpu_cache_disable
 * ---------------------
 * Nonaktifkan cache CPU.
 */
void hal_cpu_cache_disable(void)
{
    tak_bertanda32_t sctlr;

    /* Baca SCTLR */
    sctlr = cp15_read(CP15_SCTLR);

    /* Flush cache terlebih dahulu */
    hal_cpu_cache_flush();

    /* Disable caches */
    sctlr &= ~(SCTLR_C | SCTLR_I);

    /* Tulis SCTLR */
    cp15_write(CP15_SCTLR, sctlr);
    __asm__ __volatile__("isb");
}

/*
 * hal_cpu_cache_enable
 * --------------------
 * Aktifkan cache CPU.
 */
void hal_cpu_cache_enable(void)
{
    tak_bertanda32_t sctlr;

    /* Baca SCTLR */
    sctlr = cp15_read(CP15_SCTLR);

    /* Enable caches dan branch prediction */
    sctlr |= (SCTLR_C | SCTLR_I | SCTLR_Z);

    /* Tulis SCTLR */
    cp15_write(CP15_SCTLR, sctlr);
    __asm__ __volatile__("isb");
}

/*
 * ============================================================================
 * FUNGSI MMIO (MMIO HELPER FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_mmio_read_8
 * ---------------
 * Baca byte dari memory-mapped I/O.
 *
 * Parameter:
 *   addr - Alamat MMIO
 *
 * Return: Nilai yang dibaca
 */
tak_bertanda8_t hal_mmio_read_8(void *addr)
{
    tak_bertanda8_t val;
    volatile tak_bertanda8_t *ptr = (volatile tak_bertanda8_t *)addr;

    __asm__ __volatile__("dsb");
    val = *ptr;
    __asm__ __volatile__("dmb");

    return val;
}

/*
 * hal_mmio_write_8
 * ----------------
 * Tulis byte ke memory-mapped I/O.
 *
 * Parameter:
 *   addr  - Alamat MMIO
 *   value - Nilai yang ditulis
 */
void hal_mmio_write_8(void *addr, tak_bertanda8_t value)
{
    volatile tak_bertanda8_t *ptr = (volatile tak_bertanda8_t *)addr;

    __asm__ __volatile__("dsb");
    *ptr = value;
    __asm__ __volatile__("dmb");
}

/*
 * hal_mmio_read_16
 * ----------------
 * Baca word dari memory-mapped I/O.
 *
 * Parameter:
 *   addr - Alamat MMIO
 *
 * Return: Nilai yang dibaca
 */
tak_bertanda16_t hal_mmio_read_16(void *addr)
{
    tak_bertanda16_t val;
    volatile tak_bertanda16_t *ptr = (volatile tak_bertanda16_t *)addr;

    __asm__ __volatile__("dsb");
    val = *ptr;
    __asm__ __volatile__("dmb");

    return val;
}

/*
 * hal_mmio_write_16
 * -----------------
 * Tulis word ke memory-mapped I/O.
 *
 * Parameter:
 *   addr  - Alamat MMIO
 *   value - Nilai yang ditulis
 */
void hal_mmio_write_16(void *addr, tak_bertanda16_t value)
{
    volatile tak_bertanda16_t *ptr = (volatile tak_bertanda16_t *)addr;

    __asm__ __volatile__("dsb");
    *ptr = value;
    __asm__ __volatile__("dmb");
}

/*
 * hal_mmio_read_32
 * ----------------
 * Baca dword dari memory-mapped I/O.
 *
 * Parameter:
 *   addr - Alamat MMIO
 *
 * Return: Nilai yang dibaca
 */
tak_bertanda32_t hal_mmio_read_32(void *addr)
{
    tak_bertanda32_t val;
    volatile tak_bertanda32_t *ptr = (volatile tak_bertanda32_t *)addr;

    __asm__ __volatile__("dsb");
    val = *ptr;
    __asm__ __volatile__("dmb");

    return val;
}

/*
 * hal_mmio_write_32
 * -----------------
 * Tulis dword ke memory-mapped I/O.
 *
 * Parameter:
 *   addr  - Alamat MMIO
 *   value - Nilai yang ditulis
 */
void hal_mmio_write_32(void *addr, tak_bertanda32_t value)
{
    volatile tak_bertanda32_t *ptr = (volatile tak_bertanda32_t *)addr;

    __asm__ __volatile__("dsb");
    *ptr = value;
    __asm__ __volatile__("dmb");
}

/*
 * hal_mmio_read_64
 * ----------------
 * Baca qword dari memory-mapped I/O.
 *
 * Parameter:
 *   addr - Alamat MMIO
 *
 * Return: Nilai yang dibaca
 */
tak_bertanda64_t hal_mmio_read_64(void *addr)
{
    tak_bertanda64_t val;
    volatile tak_bertanda64_t *ptr = (volatile tak_bertanda64_t *)addr;

    __asm__ __volatile__("dsb");
    val = *ptr;
    __asm__ __volatile__("dmb");

    return val;
}

/*
 * hal_mmio_write_64
 * -----------------
 * Tulis qword ke memory-mapped I/O.
 *
 * Parameter:
 *   addr  - Alamat MMIO
 *   value - Nilai yang ditulis
 */
void hal_mmio_write_64(void *addr, tak_bertanda64_t value)
{
    volatile tak_bertanda64_t *ptr = (volatile tak_bertanda64_t *)addr;

    __asm__ __volatile__("dsb");
    *ptr = value;
    __asm__ __volatile__("dmb");
}

/*
 * ============================================================================
 * FUNGSI I/O PORT (I/O PORT FUNCTIONS - NOT APPLICABLE ON ARM)
 * ============================================================================
 * ARM tidak menggunakan port I/O seperti x86, semua akses perangkat
 * dilakukan melalui memory-mapped I/O.
 */

/*
 * hal_io_read_8
 * -------------
 * Tidak didukung pada ARM, gunakan MMIO.
 */
tak_bertanda8_t hal_io_read_8(tak_bertanda16_t port)
{
    (void)port;
    return 0;
}

/*
 * hal_io_write_8
 * --------------
 * Tidak didukung pada ARM, gunakan MMIO.
 */
void hal_io_write_8(tak_bertanda16_t port, tak_bertanda8_t value)
{
    (void)port;
    (void)value;
}

/*
 * hal_io_read_16
 * --------------
 * Tidak didukung pada ARM, gunakan MMIO.
 */
tak_bertanda16_t hal_io_read_16(tak_bertanda16_t port)
{
    (void)port;
    return 0;
}

/*
 * hal_io_write_16
 * ---------------
 * Tidak didukung pada ARM, gunakan MMIO.
 */
void hal_io_write_16(tak_bertanda16_t port, tak_bertanda16_t value)
{
    (void)port;
    (void)value;
}

/*
 * hal_io_read_32
 * --------------
 * Tidak didukung pada ARM, gunakan MMIO.
 */
tak_bertanda32_t hal_io_read_32(tak_bertanda16_t port)
{
    (void)port;
    return 0;
}

/*
 * hal_io_write_32
 * ---------------
 * Tidak didukung pada ARM, gunakan MMIO.
 */
void hal_io_write_32(tak_bertanda16_t port, tak_bertanda32_t value)
{
    (void)port;
    (void)value;
}

/*
 * hal_io_delay
 * ------------
 * Delay I/O (approx 1 microsecond).
 */
void hal_io_delay(void)
{
    /* Busy wait delay approximately 1 microsecond */
    volatile tak_bertanda32_t i;
    for (i = 0; i < 100; i++) {
        __asm__ __volatile__("nop");
    }
}

/*
 * ============================================================================
 * FUNGSI UTILITAS CPU (CPU UTILITY FUNCTIONS)
 * ============================================================================
 */

/*
 * cpu_get_cache_line_size
 * -----------------------
 * Dapatkan ukuran cache line.
 *
 * Return: Ukuran cache line dalam byte
 */
tak_bertanda32_t cpu_get_cache_line_size(void)
{
    return g_cache_line_size;
}

/*
 * cpu_clean_dcache_range
 * ----------------------
 * Clean (flush) data cache untuk range alamat.
 *
 * Parameter:
 *   mulai - Alamat awal
 *   akhir - Alamat akhir
 */
void cpu_clean_dcache_range(void *mulai, void *akhir)
{
    tak_bertanda32_t line_size = g_cache_line_size;
    tak_bertanda32_t addr;

    /* Align ke cache line */
    addr = (tak_bertanda32_t)(tak_bertanda32_t)mulai;
    addr &= ~(line_size - 1);

    /* Clean setiap cache line */
    while (addr < (tak_bertanda32_t)(tak_bertanda32_t)akhir) {
        __asm__ __volatile__(
            "mcr p15, 0, %0, c7, c10, 1"
            :
            : "r"(addr)
            : "memory"
        );
        addr += line_size;
    }

    __asm__ __volatile__("dsb");
}

/*
 * cpu_invalidate_dcache_range
 * ---------------------------
 * Invalidate data cache untuk range alamat.
 *
 * Parameter:
 *   mulai - Alamat awal
 *   akhir - Alamat akhir
 */
void cpu_invalidate_dcache_range(void *mulai, void *akhir)
{
    tak_bertanda32_t line_size = g_cache_line_size;
    tak_bertanda32_t addr;

    /* Align ke cache line */
    addr = (tak_bertanda32_t)(tak_bertanda32_t)mulai;
    addr &= ~(line_size - 1);

    /* Invalidate setiap cache line */
    while (addr < (tak_bertanda32_t)(tak_bertanda32_t)akhir) {
        __asm__ __volatile__(
            "mcr p15, 0, %0, c7, c6, 1"
            :
            : "r"(addr)
            : "memory"
        );
        addr += line_size;
    }

    __asm__ __volatile__("dsb");
}

/*
 * cpu_clean_invalidate_dcache_range
 * ---------------------------------
 * Clean dan invalidate data cache untuk range alamat.
 *
 * Parameter:
 *   mulai - Alamat awal
 *   akhir - Alamat akhir
 */
void cpu_clean_invalidate_dcache_range(void *mulai, void *akhir)
{
    tak_bertanda32_t line_size = g_cache_line_size;
    tak_bertanda32_t addr;

    /* Align ke cache line */
    addr = (tak_bertanda32_t)(tak_bertanda32_t)mulai;
    addr &= ~(line_size - 1);

    /* Clean dan invalidate setiap cache line */
    while (addr < (tak_bertanda32_t)(tak_bertanda32_t)akhir) {
        __asm__ __volatile__(
            "mcr p15, 0, %0, c7, c14, 1"
            :
            : "r"(addr)
            : "memory"
        );
        addr += line_size;
    }

    __asm__ __volatile__("dsb");
}

/*
 * cpu_invalidate_icache
 * ---------------------
 * Invalidate seluruh instruction cache.
 */
void cpu_invalidate_icache(void)
{
    /* Invalidate entire I-cache */
    __asm__ __volatile__(
        "mcr p15, 0, %0, c7, c5, 0"
        :
        : "r"(0)
        : "memory"
    );

    /* Invalidate branch predictor */
    __asm__ __volatile__(
        "mcr p15, 0, %0, c7, c5, 6"
        :
        : "r"(0)
        : "memory"
    );

    __asm__ __volatile__("isb");
}

/*
 * cpu_invalidate_icache_range
 * ---------------------------
 * Invalidate instruction cache untuk range alamat.
 *
 * Parameter:
 *   mulai - Alamat awal
 *   akhir - Alamat akhir
 */
void cpu_invalidate_icache_range(void *mulai, void *akhir)
{
    tak_bertanda32_t line_size = g_cache_line_size;
    tak_bertanda32_t addr;

    /* Align ke cache line */
    addr = (tak_bertanda32_t)(tak_bertanda32_t)mulai;
    addr &= ~(line_size - 1);

    /* Invalidate setiap cache line */
    while (addr < (tak_bertanda32_t)(tak_bertanda32_t)akhir) {
        __asm__ __volatile__(
            "mcr p15, 0, %0, c7, c5, 1"
            :
            : "r"(addr)
            : "memory"
        );
        addr += line_size;
    }

    __asm__ __volatile__("isb");
}

/*
 * cpu_data_memory_barrier
 * -----------------------
 * Data memory barrier.
 */
void cpu_data_memory_barrier(void)
{
    __asm__ __volatile__("dmb sy");
}

/*
 * cpu_data_sync_barrier
 * ---------------------
 * Data synchronization barrier.
 */
void cpu_data_sync_barrier(void)
{
    __asm__ __volatile__("dsb sy");
}

/*
 * cpu_instruction_sync_barrier
 * ----------------------------
 * Instruction synchronization barrier.
 */
void cpu_instruction_sync_barrier(void)
{
    __asm__ __volatile__("isb sy");
}

/*
 * cpu_get_current_mode
 * --------------------
 * Dapatkan mode CPU saat ini.
 *
 * Return: Mode CPSR (0x10=User, 0x11=FIQ, 0x12=IRQ, 0x13=SVC,
 *         0x17=Abort, 0x1B=Undefined, 0x1F=System)
 */
tak_bertanda32_t cpu_get_current_mode(void)
{
    tak_bertanda32_t cpsr;

    __asm__ __volatile__(
        "mrs %0, cpsr"
        : "=r"(cpsr)
    );

    return cpsr & 0x1F;
}

/*
 * cpu_set_vector_base
 * -------------------
 * Set vector base address.
 *
 * Parameter:
 *   base - Alamat base vector table
 */
void cpu_set_vector_base(void *base)
{
    cp15_write(CP15_VBAR, (tak_bertanda32_t)(tak_bertanda32_t)base);
    __asm__ __volatile__("isb");
}

/*
 * cpu_get_vector_base
 * -------------------
 * Dapatkan vector base address.
 *
 * Return: Alamat base vector table
 */
void *cpu_get_vector_base(void)
{
    return (void *)cp15_read(CP15_VBAR);
}
