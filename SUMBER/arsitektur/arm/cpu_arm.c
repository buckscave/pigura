/*
 * PIGURA OS - CPU_ARM.C
 * ---------------------
 * Implementasi fungsi CPU untuk arsitektur ARM (32-bit).
 *
 * Berkas ini berisi fungsi-fungsi untuk mengelola CPU ARM,
 * termasuk inisialisasi, identifikasi, dan kontrol CPU.
 *
 * Arsitektur: ARM (ARMv4/v5/v6 - 32-bit)
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

/* CP15 register opcodes untuk ARM */
#define CP15_ID                 0, c0, c0, 0    /* Main ID Register */
#define CP15_CACHE_TYPE         0, c0, c0, 1    /* Cache Type Register */
#define CP15_TCM_TYPE           0, c0, c0, 2    /* TCM Type Register */
#define CP15_CTRL               0, c1, c0, 0    /* Control Register */
#define CP15_AUX_CTRL           0, c1, c0, 1    /* Auxiliary Control */
#define CP15_CP15_DISABLE       0, c1, c0, 2    /* CP15 Disable */
#define CP15_TTB0               0, c2, c0, 0    /* Translation Table Base 0 */
#define CP15_TTB1               0, c2, c0, 1    /* Translation Table Base 1 */
#define CP15_TTB_CTRL           0, c2, c0, 2    /* Translation Table Base Ctrl */
#define CP15_DOMAIN             0, c3, c0, 0    /* Domain Access Control */
#define CP15_FSR                0, c5, c0, 0    /* Fault Status Register */
#define CP15_FAR                0, c6, c0, 0    /* Fault Address Register */
#define CP15_CACHE_OPS          0, c7, c0, 0    /* Cache Operations */
#define CP15_TLB_OPS            0, c8, c0, 0    /* TLB Operations */
#define CP15_PID                0, c13, c0, 0   /* Process ID Register */

/* Control Register bits */
#define CTRL_MMU                (1 << 0)        /* MMU enable */
#define CTRL_ALIGN              (1 << 1)        /* Alignment fault enable */
#define CTRL_DCACHE             (1 << 2)        /* Data cache enable */
#define CTRL_BIG_ENDIAN         (1 << 7)        /* Big endian mode */
#define CTRL_SYSTEM             (1 << 8)        /* System protection */
#define CTRL_ROM                (1 << 9)        /* ROM protection */
#define CTRL_ICACHE             (1 << 12)       /* Instruction cache enable */
#define CTRL_ALT_VECTOR         (1 << 13)       /* High exception vectors */
#define CTRL_ROUND_ROBIN        (1 << 14)       /* Round-robin replacement */
#define CTRL_UNALIGNED          (1 << 22)       /* Unaligned access enable */

/* Processor modes */
#define MODE_USR                0x10            /* User mode */
#define MODE_FIQ                0x11            /* FIQ mode */
#define MODE_IRQ                0x12            /* IRQ mode */
#define MODE_SVC                0x13            /* Supervisor mode */
#define MODE_ABT                0x17            /* Abort mode */
#define MODE_UND                0x1B            /* Undefined mode */
#define MODE_SYS                0x1F            /* System mode */

/* CPSR bits */
#define CPSR_IRQ_DISABLE        (1 << 7)        /* IRQ disable */
#define CPSR_FIQ_DISABLE        (1 << 6)        /* FIQ disable */

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
static tak_bertanda32_t g_cache_line_size = 32;

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
 *   implementer - Kode implementer dari ID register
 *
 * Return: String nama vendor
 */
static const char *cpu_deteksi_vendor(tak_bertanda8_t implementer)
{
    switch (implementer) {
        case 0x41: return "ARM";
        case 0x42: return "Broadcom";
        case 0x44: return "DEC";
        case 0x4D: return "Motorola";
        case 0x51: return "Qualcomm";
        case 0x56: return "Marvell";
        case 0x69: return "Intel";
        default:   return "Unknown";
    }
}

/*
 * cpu_deteksi_arch
 * ----------------
 * Deteksi arsitektur CPU dari ID register.
 *
 * Parameter:
 *   arch - Architecture code
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
        default:  return "Unknown";
    }
}

/*
 * hal_cpu_init
 * ------------
 * Inisialisasi subsistem CPU ARM.
 *
 * Return: Status operasi
 */
status_t hal_cpu_init(void)
{
    tak_bertanda32_t id_reg;
    tak_bertanda32_t cache_type;
    tak_bertanda8_t implementer;
    tak_bertanda8_t variant;
    tak_bertanda8_t arch;
    tak_bertanda16_t partno;
    tak_bertanda8_t revision;
    const char *vendor_str;

    /* Baca Main ID Register */
    id_reg = cp15_read(CP15_ID);

    /* Parse ID Register */
    implementer = (id_reg >> 24) & 0xFF;
    variant = (id_reg >> 20) & 0xF;
    arch = (id_reg >> 16) & 0xF;
    partno = (id_reg >> 4) & 0xFFF;
    revision = id_reg & 0xF;

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

    /* Baca Cache Type Register */
    cache_type = cp15_read(CP15_CACHE_TYPE);

    /* Extract cache info jika ada */
    if (cache_type != 0) {
        g_cache_line_size = 1 << (((cache_type >> 0) & 0x7) + 3);
        g_cpu_info.cache_line = g_cache_line_size;
    } else {
        g_cpu_info.cache_line = 32;
    }

    /* Default nilai cache */
    g_cpu_info.cache_l1 = 16;
    g_cpu_info.cache_l2 = 0;
    g_cpu_info.cache_l3 = 0;

    /* Default frekuensi */
    g_cpu_info.freq_mhz = 0;

    /* Single core assumption */
    g_cpu_info.cores = 1;
    g_cpu_info.threads = 1;

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
    __asm__ __volatile__("cpsid if");

    /* Loop forever */
    while (1) {
        __asm__ __volatile__("mcr p15, 0, r0, c7, c0, 4");
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
    (void)hard;

    /* Disable interupsi */
    __asm__ __volatile__("cpsid if");

    /* Reset melalui watchdog atau system register */
    /* Contoh untuk sistem tertentu */
    hal_mmio_write_32((void *)0x10000000, 0x1);

    while (1) {
        __asm__ __volatile__("mcr p15, 0, r0, c7, c0, 4");
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
    return 0;
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
    __asm__ __volatile__("cpsie if");
}

/*
 * hal_cpu_disable_interrupts
 * --------------------------
 * Nonaktifkan interupsi CPU.
 */
void hal_cpu_disable_interrupts(void)
{
    __asm__ __volatile__("cpsid if");
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
    __asm__ __volatile__("cpsid if");

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

    /* Restore IRQ/FIQ bits */
    cpsr = (cpsr & ~0xC0) | (state & 0xC0);

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
    (void)addr;

    /* Invalidate TLB */
    cp15_write(0, c8, c7, 1, (tak_bertanda32_t)addr);

    /* Data synchronization barrier */
    __asm__ __volatile__("mcr p15, 0, r0, c7, c10, 4");
}

/*
 * hal_cpu_invalidate_tlb_all
 * --------------------------
 * Invalidate seluruh TLB.
 */
void hal_cpu_invalidate_tlb_all(void)
{
    /* Invalidate entire TLB */
    cp15_write(0, c8, c7, 0, 0);

    /* Data synchronization barrier */
    __asm__ __volatile__("mcr p15, 0, r0, c7, c10, 4");
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
    tak_bertanda32_t ctr;
    tak_bertanda32_t dcache_line;
    tak_bertanda32_t dcache_size;
    tak_bertanda32_t seg;
    tak_bertanda32_t way;
    tak_bertanda32_t addr;

    /* Baca Cache Type Register */
    ctr = cp15_read(CP15_CACHE_TYPE);

    /* Extract D-cache parameters */
    dcache_line = 1 << (((ctr >> 12) & 0x7) + 3);
    dcache_size = (((ctr >> 18) & 0xF) + 1) * 1024;

    /* Clean and invalidate D-cache by set/way */
    seg = dcache_size / (dcache_line * 4);

    for (way = 0; way < 4; way++) {
        for (addr = 0; addr < dcache_size; addr += dcache_line) {
            tak_bertanda32_t val = addr | (way << 30) | (seg << 5);
            __asm__ __volatile__(
                "mcr p15, 0, %0, c7, c14, 2"
                :
                : "r"(val)
                : "memory"
            );
        }
    }

    /* Invalidate I-cache */
    cp15_write(0, c7, c5, 0, 0);

    /* Data synchronization barrier */
    __asm__ __volatile__("mcr p15, 0, r0, c7, c10, 4");
}

/*
 * hal_cpu_cache_disable
 * ---------------------
 * Nonaktifkan cache CPU.
 */
void hal_cpu_cache_disable(void)
{
    tak_bertanda32_t ctrl;

    /* Flush cache terlebih dahulu */
    hal_cpu_cache_flush();

    /* Baca Control Register */
    ctrl = cp15_read(CP15_CTRL);

    /* Disable caches */
    ctrl &= ~(CTRL_DCACHE | CTRL_ICACHE);

    /* Tulis Control Register */
    cp15_write(CP15_CTRL, ctrl);
}

/*
 * hal_cpu_cache_enable
 * --------------------
 * Aktifkan cache CPU.
 */
void hal_cpu_cache_enable(void)
{
    tak_bertanda32_t ctrl;

    /* Baca Control Register */
    ctrl = cp15_read(CP15_CTRL);

    /* Enable caches */
    ctrl |= (CTRL_DCACHE | CTRL_ICACHE);

    /* Tulis Control Register */
    cp15_write(CP15_CTRL, ctrl);
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

    val = *ptr;
    __asm__ __volatile__("mcr p15, 0, r0, c7, c10, 5");

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

    *ptr = value;
    __asm__ __volatile__("mcr p15, 0, r0, c7, c10, 5");
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

    val = *ptr;
    __asm__ __volatile__("mcr p15, 0, r0, c7, c10, 5");

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

    *ptr = value;
    __asm__ __volatile__("mcr p15, 0, r0, c7, c10, 5");
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

    val = *ptr;
    __asm__ __volatile__("mcr p15, 0, r0, c7, c10, 5");

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

    *ptr = value;
    __asm__ __volatile__("mcr p15, 0, r0, c7, c10, 5");
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

    val = *ptr;
    __asm__ __volatile__("mcr p15, 0, r0, c7, c10, 5");

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

    *ptr = value;
    __asm__ __volatile__("mcr p15, 0, r0, c7, c10, 5");
}

/*
 * ============================================================================
 * FUNGSI I/O PORT (I/O PORT FUNCTIONS - NOT APPLICABLE ON ARM)
 * ============================================================================
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
 * cpu_get_current_mode
 * --------------------
 * Dapatkan mode CPU saat ini.
 *
 * Return: Mode CPSR
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
