/*
 * PIGURA OS - CPU_ARM64.C
 * -----------------------
 * Implementasi fungsi CPU untuk arsitektur ARM64 (AArch64).
 *
 * Berkas ini berisi fungsi-fungsi untuk mengelola CPU ARM64,
 * termasuk inisialisasi, identifikasi, dan kontrol CPU.
 *
 * Arsitektur: ARM64/AArch64 (64-bit)
 * Versi: 1.0
 */

#include "../../inti/kernel.h"

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

/* System register opcodes untuk ARM64 */

/* ID registers */
#define REG_MIDR_EL1        "midr_el1"
#define REG_MPIDR_EL1       "mpidr_el1"
#define REG_ID_AA64PFR0_EL1 "id_aa64pfr0_el1"
#define REG_ID_AA64PFR1_EL1 "id_aa64pfr1_el1"
#define REG_ID_AA64DFR0_EL1 "id_aa64dfr0_el1"
#define REG_ID_AA64ISAR0_EL1 "id_aa64isar0_el1"
#define REG_ID_AA64ISAR1_EL1 "id_aa64isar1_el1"
#define REG_ID_AA64MMFR0_EL1 "id_aa64mmfr0_el1"
#define REG_ID_AA64MMFR1_EL1 "id_aa64mmfr1_el1"
#define REG_ID_AA64MMFR2_EL1 "id_aa64mmfr2_el1"

/* Control registers */
#define REG_SCTLR_EL1       "sctlr_el1"
#define REG_SCTLR_EL2       "sctlr_el2"
#define REG_SCTLR_EL3       "sctlr_el3"

/* Translation table registers */
#define REG_TTBR0_EL1       "ttbr0_el1"
#define REG_TTBR1_EL1       "ttbr1_el1"
#define REG_TCR_EL1         "tcr_el1"

/* Exception handling registers */
#define REG_VBAR_EL1        "vbar_el1"
#define REG_VBAR_EL2        "vbar_el2"
#define REG_VBAR_EL3        "vbar_el3"
#define REG_ELR_EL1         "elr_el1"
#define REG_ESR_EL1         "esr_el1"
#define REG_FAR_EL1         "far_el1"

/* Current EL register */
#define REG_CURRENT_EL      "CurrentEL"

/* DAIF mask bits */
#define DAIF_D              (1 << 3)  /* Debug exception mask */
#define DAIF_A              (1 << 2)  /* SError interrupt mask */
#define DAIF_I              (1 << 1)  /* IRQ mask */
#define DAIF_F              (1 << 0)  /* FIQ mask */

/* SCTLR bits */
#define SCTLR_M             (1 << 0)  /* MMU enable */
#define SCTLR_A             (1 << 1)  /* Alignment check */
#define SCTLR_C             (1 << 2)  /* Data cache */
#define SCTLR_SA            (1 << 3)  /* Stack alignment check */
#define SCTLR_I             (1 << 12) /* Instruction cache */
#define SCTLR_NXE           (1 << 19) /* No-execute enable */

/* Implementer codes */
#define IMPLEMENTER_ARM     0x41  /* 'A' - ARM Ltd */
#define IMPLEMENTER_BROADCOM 0x42
#define IMPLEMENTER_CAVIUM  0x43
#define IMPLEMENTER_DEC     0x44
#define IMPLEMENTER_FUJITSU 0x46
#define IMPLEMENTER_INFINEON 0x49
#define IMPLEMENTER_MOTOROLA 0x4D
#define IMPLEMENTER_NVIDIA  0x4E
#define IMPLEMENTER_AMCC    0x50
#define IMPLEMENTER_QUALCOMM 0x51
#define IMPLEMENTER_MARVELL 0x56
#define IMPLEMENTER_APPLE   0x61  /* 'a' */
#define IMPLEMENTER_INTEL   0x69  /* 'i' */

/*
 * ============================================================================
 * VARIABEL GLOBAL (GLOBAL VARIABLES)
 * ============================================================================
 */

/* CPU info global */
static cpu_info_arm64_t g_cpu_info;
static bool_t g_cpu_diinisalisasi = SALAH;

/*
 * ============================================================================
 * FUNGSI BANTUAN INLINE (INLINE HELPER FUNCTIONS)
 * ============================================================================
 */

/*
 * _baca_system_reg
 * ----------------
 * Baca system register 64-bit.
 */
static inline tak_bertanda64_t _baca_system_reg(const char *reg)
{
    tak_bertanda64_t val;

    if (kernel_strcmp(reg, "midr_el1") == 0) {
        __asm__ __volatile__("mrs %0, midr_el1" : "=r"(val));
    } else if (kernel_strcmp(reg, "mpidr_el1") == 0) {
        __asm__ __volatile__("mrs %0, mpidr_el1" : "=r"(val));
    } else if (kernel_strcmp(reg, "CurrentEL") == 0) {
        __asm__ __volatile__("mrs %0, CurrentEL" : "=r"(val));
    } else if (kernel_strcmp(reg, "sctlr_el1") == 0) {
        __asm__ __volatile__("mrs %0, sctlr_el1" : "=r"(val));
    } else if (kernel_strcmp(reg, "tcr_el1") == 0) {
        __asm__ __volatile__("mrs %0, tcr_el1" : "=r"(val));
    } else if (kernel_strcmp(reg, "ttbr0_el1") == 0) {
        __asm__ __volatile__("mrs %0, ttbr0_el1" : "=r"(val));
    } else if (kernel_strcmp(reg, "ttbr1_el1") == 0) {
        __asm__ __volatile__("mrs %0, ttbr1_el1" : "=r"(val));
    } else if (kernel_strcmp(reg, "vbar_el1") == 0) {
        __asm__ __volatile__("mrs %0, vbar_el1" : "=r"(val));
    } else if (kernel_strcmp(reg, "id_aa64pfr0_el1") == 0) {
        __asm__ __volatile__("mrs %0, id_aa64pfr0_el1" : "=r"(val));
    } else if (kernel_strcmp(reg, "id_aa64isar0_el1") == 0) {
        __asm__ __volatile__("mrs %0, id_aa64isar0_el1" : "=r"(val));
    } else if (kernel_strcmp(reg, "id_aa64mmfr0_el1") == 0) {
        __asm__ __volatile__("mrs %0, id_aa64mmfr0_el1" : "=r"(val));
    } else if (kernel_strcmp(reg, "cntfrq_el0") == 0) {
        __asm__ __volatile__("mrs %0, cntfrq_el0" : "=r"(val));
    } else if (kernel_strcmp(reg, "cntvct_el0") == 0) {
        __asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(val));
    } else if (kernel_strcmp(reg, "ctr_el0") == 0) {
        __asm__ __volatile__("mrs %0, ctr_el0" : "=r"(val));
    } else {
        val = 0;
    }

    return val;
}

/*
 * _tulis_system_reg
 * -----------------
 * Tulis system register 64-bit.
 */
static inline void _tulis_system_reg(const char *reg, tak_bertanda64_t val)
{
    if (kernel_strcmp(reg, "sctlr_el1") == 0) {
        __asm__ __volatile__("msr sctlr_el1, %0" : : "r"(val) : "memory");
    } else if (kernel_strcmp(reg, "tcr_el1") == 0) {
        __asm__ __volatile__("msr tcr_el1, %0" : : "r"(val) : "memory");
    } else if (kernel_strcmp(reg, "ttbr0_el1") == 0) {
        __asm__ __volatile__("msr ttbr0_el1, %0" : : "r"(val) : "memory");
    } else if (kernel_strcmp(reg, "ttbr1_el1") == 0) {
        __asm__ __volatile__("msr ttbr1_el1, %0" : : "r"(val) : "memory");
    } else if (kernel_strcmp(reg, "vbar_el1") == 0) {
        __asm__ __volatile__("msr vbar_el1, %0" : : "r"(val) : "memory");
    } else if (kernel_strcmp(reg, "elr_el1") == 0) {
        __asm__ __volatile__("msr elr_el1, %0" : : "r"(val) : "memory");
    } else if (kernel_strcmp(reg, "sp_el0") == 0) {
        __asm__ __volatile__("msr sp_el0, %0" : : "r"(val) : "memory");
    }

    /* ISB untuk memastikan write selesai */
    __asm__ __volatile__("isb");
}

/*
 * ============================================================================
 * FUNGSI IDENTIFIKASI CPU (CPU IDENTIFICATION FUNCTIONS)
 * ============================================================================
 */

/*
 * cpu_identifikasi
 * ----------------
 * Identifikasi CPU ARM64.
 */
status_t cpu_identifikasi(cpu_info_arm64_t *info)
{
    tak_bertanda64_t midr;
    tak_bertanda64_t mpidr;
    tak_bertanda64_t pfr0;
    tak_bertanda64_t isar0;
    tak_bertanda64_t mmfr0;

    if (info == NULL) {
        return STATUS_PARAM_INVALID;
    }

    /* Baca Main ID Register */
    midr = _baca_system_reg("midr_el1");

    /* Parse MIDR */
    info->implementer = (tak_bertanda8_t)((midr >> 24) & 0xFF);
    info->variant = (tak_bertanda8_t)((midr >> 20) & 0xF);
    info->architecture = (tak_bertanda8_t)((midr >> 16) & 0xF);
    info->part_num = (tak_bertanda16_t)((midr >> 4) & 0xFFF);
    info->revision = (tak_bertanda8_t)(midr & 0xF);

    /* Baca MPIDR */
    mpidr = _baca_system_reg("mpidr_el1");

    info->mpidr = mpidr;
    info->affinity0 = (tak_bertanda8_t)(mpidr & 0xFF);
    info->affinity1 = (tak_bertanda8_t)((mpidr >> 8) & 0xFF);
    info->affinity2 = (tak_bertanda8_t)((mpidr >> 16) & 0xFF);
    info->affinity3 = (tak_bertanda8_t)((mpidr >> 32) & 0xFF);
    info->mt = (mpidr & (1ULL << 24)) ? BENAR : SALAH;
    info->u = (mpidr & (1ULL << 30)) ? BENAR : SALAH;

    /* Baca feature registers */
    pfr0 = _baca_system_reg("id_aa64pfr0_el1");
    isar0 = _baca_system_reg("id_aa64isar0_el1");
    mmfr0 = _baca_system_reg("id_aa64mmfr0_el1");

    /* Parse PFR0 - Processor Feature Register 0 */
    info->el0 = (tak_bertanda8_t)(pfr0 & 0xF);
    info->el1 = (tak_bertanda8_t)((pfr0 >> 4) & 0xF);
    info->el2 = (tak_bertanda8_t)((pfr0 >> 8) & 0xF);
    info->el3 = (tak_bertanda8_t)((pfr0 >> 12) & 0xF);
    info->fp = ((pfr0 >> 16) & 0xF) != 0 ? BENAR : SALAH;
    info->advsimd = ((pfr0 >> 20) & 0xF) != 0 ? BENAR : SALAH;
    info->gic = (tak_bertanda8_t)((pfr0 >> 24) & 0xF);
    info->ras = ((pfr0 >> 28) & 0xF) != 0 ? BENAR : SALAH;
    info->sve = ((pfr0 >> 32) & 0xF) != 0 ? BENAR : SALAH;

    /* Parse ISAR0 - Instruction Set Attribute Register 0 */
    info->aes = (tak_bertanda8_t)((isar0 >> 4) & 0xF);
    info->pmull = (tak_bertanda8_t)((isar0 >> 8) & 0xF);
    info->sha1 = (tak_bertanda8_t)((isar0 >> 12) & 0xF);
    info->sha2 = (tak_bertanda8_t)((isar0 >> 16) & 0xF);
    info->crc32 = (tak_bertanda8_t)((isar0 >> 20) & 0xF);
    info->atomics = (tak_bertanda8_t)((isar0 >> 24) & 0xF);
    info->tme = (tak_bertanda8_t)((isar0 >> 28) & 0xF);
    info->rdm = (tak_bertanda8_t)((isar0 >> 32) & 0xF);
    info->sha3 = (tak_bertanda8_t)((isar0 >> 36) & 0xF);
    info->sm3 = (tak_bertanda8_t)((isar0 >> 40) & 0xF);
    info->sm4 = (tak_bertanda8_t)((isar0 >> 44) & 0xF);
    info->dp = (tak_bertanda8_t)((isar0 >> 48) & 0xF);
    info->fhm = (tak_bertanda8_t)((isar0 >> 52) & 0xF);
    info->bf16 = (tak_bertanda8_t)((isar0 >> 56) & 0xF);
    info->i8mm = (tak_bertanda8_t)((isar0 >> 60) & 0xF);

    /* Parse MMFR0 - Memory Model Feature Register 0 */
    info->parange = (tak_bertanda8_t)(mmfr0 & 0xF);
    info->asid_bits = (tak_bertanda8_t)((mmfr0 >> 4) & 0xF);
    info->bigend = (tak_bertanda8_t)((mmfr0 >> 8) & 0xF);
    info->sns_mem = (tak_bertanda8_t)((mmfr0 >> 12) & 0xF);
    info->bigend_el0 = (tak_bertanda8_t)((mmfr0 >> 16) & 0xF);
    info->tgran16 = (tak_bertanda8_t)((mmfr0 >> 20) & 0xF);
    info->tgran64 = (tak_bertanda8_t)((mmfr0 >> 24) & 0xF);
    info->tgran4 = (tak_bertanda8_t)((mmfr0 >> 28) & 0xF);
    info->tgran16_2 = (tak_bertanda8_t)((mmfr0 >> 32) & 0xF);
    info->tgran64_2 = (tak_bertanda8_t)((mmfr0 >> 36) & 0xF);
    info->tgran4_2 = (tak_bertanda8_t)((mmfr0 >> 40) & 0xF);
    info->exs = (tak_bertanda8_t)((mmfr0 >> 44) & 0xF);

    /* Hitung physical address size */
    switch (info->parange) {
        case 0: info->pa_bits = 32; break;
        case 1: info->pa_bits = 36; break;
        case 2: info->pa_bits = 40; break;
        case 3: info->pa_bits = 42; break;
        case 4: info->pa_bits = 44; break;
        case 5: info->pa_bits = 48; break;
        case 6: info->pa_bits = 52; break;
        default: info->pa_bits = 48; break;
    }

    return STATUS_BERHASIL;
}

/*
 * cpu_dapatkan_nama_vendor
 * ------------------------
 * Dapatkan nama vendor CPU.
 */
const char *cpu_dapatkan_nama_vendor(tak_bertanda8_t implementer)
{
    switch (implementer) {
        case IMPLEMENTER_ARM:
            return "ARM";
        case IMPLEMENTER_BROADCOM:
            return "Broadcom";
        case IMPLEMENTER_CAVIUM:
            return "Cavium";
        case IMPLEMENTER_DEC:
            return "DEC";
        case IMPLEMENTER_FUJITSU:
            return "Fujitsu";
        case IMPLEMENTER_NVIDIA:
            return "NVIDIA";
        case IMPLEMENTER_QUALCOMM:
            return "Qualcomm";
        case IMPLEMENTER_MARVELL:
            return "Marvell";
        case IMPLEMENTER_APPLE:
            return "Apple";
        case IMPLEMENTER_INTEL:
            return "Intel";
        default:
            return "Unknown";
    }
}

/*
 * cpu_dapatkan_nama_part
 * ----------------------
 * Dapatkan nama part CPU.
 */
const char *cpu_dapatkan_nama_part(tak_bertanda8_t implementer,
                                    tak_bertanda16_t part_num)
{
    if (implementer == IMPLEMENTER_ARM) {
        switch (part_num) {
            case 0xD01: return "Cortex-A32";
            case 0xD02: return "Cortex-A34";
            case 0xD03: return "Cortex-A53";
            case 0xD04: return "Cortex-A35";
            case 0xD05: return "Cortex-A55";
            case 0xD06: return "Cortex-A65";
            case 0xD07: return "Cortex-A57";
            case 0xD08: return "Cortex-A72";
            case 0xD09: return "Cortex-A73";
            case 0xD0A: return "Cortex-A75";
            case 0xD0B: return "Cortex-A76";
            case 0xD0C: return "Neoverse-N1";
            case 0xD0D: return "Cortex-A77";
            case 0xD0E: return "Cortex-A76AE";
            case 0xD0F: return "AEMv8";
            case 0xD40: return "Neoverse-V1";
            case 0xD41: return "Cortex-A78";
            case 0xD42: return "Cortex-A78AE";
            case 0xD43: return "Cortex-A65AE";
            case 0xD44: return "Cortex-X1";
            case 0xD46: return "Cortex-A510";
            case 0xD47: return "Cortex-A710";
            case 0xD48: return "Cortex-X2";
            case 0xD49: return "Neoverse-N2";
            case 0xD4A: return "Neoverse-E1";
            case 0xD4B: return "Cortex-A78C";
            default: return "Unknown ARM";
        }
    }

    if (implementer == IMPLEMENTER_APPLE) {
        switch (part_num) {
            case 0x022: return "M1 (Firestorm)";
            case 0x023: return "M1 (Icestorm)";
            case 0x024: return "M1 Pro/Max (Firestorm)";
            case 0x025: return "M1 Pro/Max (Icestorm)";
            default: return "Apple Silicon";
        }
    }

    return "Unknown";
}

/*
 * ============================================================================
 * FUNGSI KONTROL CPU (CPU CONTROL FUNCTIONS)
 * ============================================================================
 */

/*
 * cpu_init
 * --------
 * Inisialisasi CPU.
 */
status_t cpu_init(void)
{
    if (g_cpu_diinisalisasi) {
        return STATUS_BERHASIL;
    }

    /* Identifikasi CPU */
    cpu_identifikasi(&g_cpu_info);
    g_cpu_diinisalisasi = BENAR;

    kernel_printf("[CPU-ARM64] Vendor: %s\n",
        cpu_dapatkan_nama_vendor(g_cpu_info.implementer));
    kernel_printf("[CPU-ARM64] Part: %s\n",
        cpu_dapatkan_nama_part(g_cpu_info.implementer,
                               g_cpu_info.part_num));
    kernel_printf("[CPU-ARM64] Rev: %u.%u\n",
        g_cpu_info.variant, g_cpu_info.revision);
    kernel_printf("[CPU-ARM64] PA bits: %u\n", g_cpu_info.pa_bits);

    return STATUS_BERHASIL;
}

/*
 * cpu_halt
 * --------
 * Hentikan CPU (WFI).
 */
void cpu_halt(void)
{
    __asm__ __volatile__("wfi");
}

/*
 * cpu_pause
 * ---------
 * Pause CPU (YIELD).
 */
void cpu_pause(void)
{
    __asm__ __volatile__("yield");
}

/*
 * cpu_enable_irq
 * --------------
 * Aktifkan IRQ.
 */
void cpu_enable_irq(void)
{
    __asm__ __volatile__(
        "msr DAIFClr, #0x2\n\t"
        "isb"
        :
        :
        : "memory"
    );
}

/*
 * cpu_disable_irq
 * ---------------
 * Nonaktifkan IRQ.
 */
void cpu_disable_irq(void)
{
    __asm__ __volatile__(
        "msr DAIFSet, #0x2\n\t"
        "isb"
        :
        :
        : "memory"
    );
}

/*
 * cpu_enable_fiq
 * --------------
 * Aktifkan FIQ.
 */
void cpu_enable_fiq(void)
{
    __asm__ __volatile__(
        "msr DAIFClr, #0x1\n\t"
        "isb"
        :
        :
        : "memory"
    );
}

/*
 * cpu_disable_fiq
 * ---------------
 * Nonaktifkan FIQ.
 */
void cpu_disable_fiq(void)
{
    __asm__ __volatile__(
        "msr DAIFSet, #0x1\n\t"
        "isb"
        :
        :
        : "memory"
    );
}

/*
 * cpu_enable_serror
 * -----------------
 * Aktifkan SError.
 */
void cpu_enable_serror(void)
{
    __asm__ __volatile__(
        "msr DAIFClr, #0x4\n\t"
        "isb"
        :
        :
        : "memory"
    );
}

/*
 * cpu_disable_serror
 * ------------------
 * Nonaktifkan SError.
 */
void cpu_disable_serror(void)
{
    __asm__ __volatile__(
        "msr DAIFSet, #0x4\n\t"
        "isb"
        :
        :
        : "memory"
    );
}

/*
 * cpu_enable_debug
 * ----------------
 * Aktifkan debug exception.
 */
void cpu_enable_debug(void)
{
    __asm__ __volatile__(
        "msr DAIFClr, #0x8\n\t"
        "isb"
        :
        :
        : "memory"
    );
}

/*
 * cpu_disable_debug
 * -----------------
 * Nonaktifkan debug exception.
 */
void cpu_disable_debug(void)
{
    __asm__ __volatile__(
        "msr DAIFSet, #0x8\n\t"
        "isb"
        :
        :
        : "memory"
    );
}

/*
 * cpu_get_daif
 * ------------
 * Baca DAIF register.
 */
tak_bertanda64_t cpu_get_daif(void)
{
    tak_bertanda64_t daif;

    __asm__ __volatile__(
        "mrs %0, DAIF"
        : "=r"(daif)
    );

    return daif;
}

/*
 * cpu_set_daif
 * ------------
 * Tulis DAIF register.
 */
void cpu_set_daif(tak_bertanda64_t daif)
{
    __asm__ __volatile__(
        "msr DAIF, %0\n\t"
        "isb"
        :
        : "r"(daif)
        : "memory"
    );
}

/*
 * cpu_get_current_el
 * ------------------
 * Dapatkan current exception level.
 */
tak_bertanda32_t cpu_get_current_el(void)
{
    tak_bertanda64_t currentel;

    __asm__ __volatile__(
        "mrs %0, CurrentEL"
        : "=r"(currentel)
    );

    return (tak_bertanda32_t)((currentel >> 2) & 0x3);
}

/*
 * cpu_get_sctlr
 * -------------
 * Baca SCTLR_EL1.
 */
tak_bertanda64_t cpu_get_sctlr(void)
{
    return _baca_system_reg("sctlr_el1");
}

/*
 * cpu_set_sctlr
 * -------------
 * Tulis SCTLR_EL1.
 */
void cpu_set_sctlr(tak_bertanda64_t sctlr)
{
    _tulis_system_reg("sctlr_el1", sctlr);
}

/*
 * cpu_get_tcr
 * -----------
 * Baca TCR_EL1.
 */
tak_bertanda64_t cpu_get_tcr(void)
{
    return _baca_system_reg("tcr_el1");
}

/*
 * cpu_set_tcr
 * -----------
 * Tulis TCR_EL1.
 */
void cpu_set_tcr(tak_bertanda64_t tcr)
{
    _tulis_system_reg("tcr_el1", tcr);
}

/*
 * cpu_get_ttbr0
 * -------------
 * Baca TTBR0_EL1.
 */
tak_bertanda64_t cpu_get_ttbr0(void)
{
    return _baca_system_reg("ttbr0_el1");
}

/*
 * cpu_set_ttbr0
 * -------------
 * Tulis TTBR0_EL1.
 */
void cpu_set_ttbr0(tak_bertanda64_t ttbr0)
{
    _tulis_system_reg("ttbr0_el1", ttbr0);
}

/*
 * cpu_get_ttbr1
 * -------------
 * Baca TTBR1_EL1.
 */
tak_bertanda64_t cpu_get_ttbr1(void)
{
    return _baca_system_reg("ttbr1_el1");
}

/*
 * cpu_set_ttbr1
 * -------------
 * Tulis TTBR1_EL1.
 */
void cpu_set_ttbr1(tak_bertanda64_t ttbr1)
{
    _tulis_system_reg("ttbr1_el1", ttbr1);
}

/*
 * cpu_get_vbar
 * ------------
 * Baca VBAR_EL1.
 */
tak_bertanda64_t cpu_get_vbar(void)
{
    return _baca_system_reg("vbar_el1");
}

/*
 * cpu_set_vbar
 * ------------
 * Tulis VBAR_EL1.
 */
void cpu_set_vbar(tak_bertanda64_t vbar)
{
    _tulis_system_reg("vbar_el1", vbar);
}

/*
 * cpu_get_mpidr
 * -------------
 * Baca MPIDR_EL1.
 */
tak_bertanda64_t cpu_get_mpidr(void)
{
    return _baca_system_reg("mpidr_el1");
}

/*
 * cpu_get_midr
 * ------------
 * Baca MIDR_EL1.
 */
tak_bertanda64_t cpu_get_midr(void)
{
    return _baca_system_reg("midr_el1");
}

/*
 * ============================================================================
 * FUNGSI MMU (MMU FUNCTIONS)
 * ============================================================================
 */

/*
 * cpu_mmu_enable
 * --------------
 * Aktifkan MMU.
 */
void cpu_mmu_enable(void)
{
    tak_bertanda64_t sctlr;

    sctlr = cpu_get_sctlr();
    sctlr |= SCTLR_M;
    cpu_set_sctlr(sctlr);
}

/*
 * cpu_mmu_disable
 * ---------------
 * Nonaktifkan MMU.
 */
void cpu_mmu_disable(void)
{
    tak_bertanda64_t sctlr;

    sctlr = cpu_get_sctlr();
    sctlr &= ~SCTLR_M;
    cpu_set_sctlr(sctlr);
}

/*
 * cpu_cache_enable
 * ----------------
 * Aktifkan cache.
 */
void cpu_cache_enable(void)
{
    tak_bertanda64_t sctlr;

    sctlr = cpu_get_sctlr();
    sctlr |= (SCTLR_C | SCTLR_I);
    cpu_set_sctlr(sctlr);
}

/*
 * cpu_cache_disable
 * -----------------
 * Nonaktifkan cache.
 */
void cpu_cache_disable(void)
{
    tak_bertanda64_t sctlr;

    sctlr = cpu_get_sctlr();
    sctlr &= ~(SCTLR_C | SCTLR_I);
    cpu_set_sctlr(sctlr);
}

/*
 * cpu_tlb_invalidate_all
 * ----------------------
 * Invalidate semua TLB.
 */
void cpu_tlb_invalidate_all(void)
{
    __asm__ __volatile__(
        "tlbi VMALLE1IS\n\t"
        "dsb ish\n\t"
        "isb"
        :
        :
        : "memory"
    );
}

/*
 * cpu_tlb_invalidate_va
 * ---------------------
 * Invalidate TLB untuk virtual address.
 */
void cpu_tlb_invalidate_va(tak_bertanda64_t va)
{
    __asm__ __volatile__(
        "tlbi VAAE1, %0\n\t"
        "dsb sy\n\t"
        "isb"
        :
        : "r"(va >> 12)
        : "memory"
    );
}

/*
 * cpu_tlb_invalidate_asid
 * -----------------------
 * Invalidate TLB untuk ASID.
 */
void cpu_tlb_invalidate_asid(tak_bertanda8_t asid)
{
    __asm__ __volatile__(
        "tlbi ASIDE1, %0\n\t"
        "dsb sy\n\t"
        "isb"
        :
        : "r"((tak_bertanda64_t)asid)
        : "memory"
    );
}

/*
 * cpu_branch_predictor_invalidate
 * --------------------------------
 * Invalidate branch predictor.
 */
void cpu_branch_predictor_invalidate(void)
{
    __asm__ __volatile__(
        "ic IALLUIS\n\t"
        "dsb ish\n\t"
        "isb"
        :
        :
        : "memory"
    );
}

/*
 * ============================================================================
 * FUNGSI BARRIER (BARRIER FUNCTIONS)
 * ============================================================================
 */

/*
 * cpu_dsb
 * -------
 * Data Synchronization Barrier.
 */
void cpu_dsb(void)
{
    __asm__ __volatile__("dsb sy");
}

/*
 * cpu_dmb
 * -------
 * Data Memory Barrier.
 */
void cpu_dmb(void)
{
    __asm__ __volatile__("dmb sy");
}

/*
 * cpu_isb
 * -------
 * Instruction Synchronization Barrier.
 */
void cpu_isb(void)
{
    __asm__ __volatile__("isb");
}

/*
 * ============================================================================
 * FUNGSI TIMER (TIMER FUNCTIONS)
 * ============================================================================
 */

/*
 * cpu_get_timer_freq
 * ------------------
 * Dapatkan frekuensi timer.
 */
tak_bertanda64_t cpu_get_timer_freq(void)
{
    return _baca_system_reg("cntfrq_el0");
}

/*
 * cpu_get_timer_count
 * -------------------
 * Dapatkan counter timer.
 */
tak_bertanda64_t cpu_get_timer_count(void)
{
    return _baca_system_reg("cntvct_el0");
}

/*
 * cpu_get_cycle_count
 * -------------------
 * Dapatkan cycle count (PMU).
 */
tak_bertanda64_t cpu_get_cycle_count(void)
{
    tak_bertanda64_t pmccntr;

    /* Enable PMU */
    __asm__ __volatile__(
        "msr PMCR_EL0, %0\n\t"
        "msr PMCNTENSET_EL0, %1"
        :
        : "r"(0x1), "r"(0x80000000)
        : "memory"
    );

    __asm__ __volatile__(
        "mrs %0, PMCCNTR_EL0"
        : "=r"(pmccntr)
    );

    return pmccntr;
}

/*
 * ============================================================================
 * FUNGSI CACHE (CACHE FUNCTIONS)
 * ============================================================================
 */

/*
 * cpu_get_cache_line_size
 * -----------------------
 * Dapatkan ukuran cache line.
 */
tak_bertanda32_t cpu_get_cache_line_size(void)
{
    tak_bertanda64_t ctr;
    tak_bertanda32_t dminline;

    ctr = _baca_system_reg("ctr_el0");
    dminline = (tak_bertanda32_t)(ctr & 0xF);

    /* Size = 4 << dminline */
    return (4 << dminline);
}

/*
 * cpu_cache_clean_va
 * ------------------
 * Clean cache untuk virtual address.
 */
void cpu_cache_clean_va(void *va)
{
    tak_bertanda64_t line_size;
    tak_bertanda64_t addr;

    line_size = (tak_bertanda64_t)cpu_get_cache_line_size();
    addr = (tak_bertanda64_t)va & ~(line_size - 1);

    __asm__ __volatile__(
        "dc cvau, %0"
        :
        : "r"(addr)
        : "memory"
    );
}

/*
 * cpu_cache_invalidate_va
 * -----------------------
 * Invalidate cache untuk virtual address.
 */
void cpu_cache_invalidate_va(void *va)
{
    tak_bertanda64_t line_size;
    tak_bertanda64_t addr;

    line_size = (tak_bertanda64_t)cpu_get_cache_line_size();
    addr = (tak_bertanda64_t)va & ~(line_size - 1);

    __asm__ __volatile__(
        "dc ivau, %0"
        :
        : "r"(addr)
        : "memory"
    );
}

/*
 * cpu_cache_clean_invalidate_va
 * -----------------------------
 * Clean dan invalidate cache untuk virtual address.
 */
void cpu_cache_clean_invalidate_va(void *va)
{
    tak_bertanda64_t line_size;
    tak_bertanda64_t addr;

    line_size = (tak_bertanda64_t)cpu_get_cache_line_size();
    addr = (tak_bertanda64_t)va & ~(line_size - 1);

    __asm__ __volatile__(
        "dc civac, %0"
        :
        : "r"(addr)
        : "memory"
    );
}

/*
 * cpu_icache_invalidate_va
 * ------------------------
 * Invalidate instruction cache untuk virtual address.
 */
void cpu_icache_invalidate_va(void *va)
{
    tak_bertanda64_t line_size;
    tak_bertanda64_t addr;

    line_size = (tak_bertanda64_t)cpu_get_cache_line_size();
    addr = (tak_bertanda64_t)va & ~(line_size - 1);

    __asm__ __volatile__(
        "ic ivau, %0"
        :
        : "r"(addr)
        : "memory"
    );
}

/*
 * ============================================================================
 * FUNGSI ATOMIC (ATOMIC FUNCTIONS)
 * ============================================================================
 */

/*
 * cpu_atomic_add
 * --------------
 * Atomic add.
 */
tak_bertanda32_t cpu_atomic_add(volatile tak_bertanda32_t *addr,
                                 tak_bertanda32_t val)
{
    tak_bertanda32_t result;

    __asm__ __volatile__(
        "ldadd %w2, %w0, [%1]"
        : "=&r"(result)
        : "r"(addr), "r"(val)
        : "memory"
    );

    return result;
}

/*
 * cpu_atomic_swap
 * ---------------
 * Atomic swap.
 */
tak_bertanda32_t cpu_atomic_swap(volatile tak_bertanda32_t *addr,
                                  tak_bertanda32_t val)
{
    tak_bertanda32_t result;

    __asm__ __volatile__(
        "swp %w2, %w0, [%1]"
        : "=&r"(result)
        : "r"(addr), "r"(val)
        : "memory"
    );

    return result;
}

/*
 * cpu_atomic_compare_swap
 * -----------------------
 * Atomic compare and swap.
 */
bool_t cpu_atomic_compare_swap(volatile tak_bertanda32_t *addr,
                                tak_bertanda32_t expected,
                                tak_bertanda32_t desired)
{
    tak_bertanda32_t result;

    __asm__ __volatile__(
        "1:\n\t"
        "ldxr %w0, [%1]\n\t"
        "cmp %w0, %w2\n\t"
        "b.ne 2f\n\t"
        "stxr %w0, %w3, [%1]\n\t"
        "cbnz %w0, 1b\n\t"
        "mov %w0, #1\n\t"
        "b 3f\n\t"
        "2:\n\t"
        "mov %w0, #0\n\t"
        "3:"
        : "=&r"(result)
        : "r"(addr), "r"(expected), "r"(desired)
        : "memory", "cc"
    );

    return result == 1 ? BENAR : SALAH;
}

/*
 * ============================================================================
 * FUNGSI SPINLOCK (SPINLOCK FUNCTIONS)
 * ============================================================================
 */

/*
 * cpu_spinlock_lock
 * -----------------
 * Acquire spinlock.
 */
void cpu_spinlock_lock(volatile tak_bertanda32_t *lock)
{
    tak_bertanda32_t tmp;

    __asm__ __volatile__(
        "1:\n\t"
        "ldaxr %w0, [%1]\n\t"
        "cbnz %w0, 1b\n\t"
        "stxr %w0, %w2, [%1]\n\t"
        "cbnz %w0, 1b"
        : "=&r"(tmp)
        : "r"(lock), "r"(1)
        : "memory"
    );
}

/*
 * cpu_spinlock_unlock
 * -------------------
 * Release spinlock.
 */
void cpu_spinlock_unlock(volatile tak_bertanda32_t *lock)
{
    __asm__ __volatile__(
        "stlr wzr, [%0]"
        :
        : "r"(lock)
        : "memory"
    );
}

/*
 * cpu_spinlock_trylock
 * --------------------
 * Try to acquire spinlock.
 */
bool_t cpu_spinlock_trylock(volatile tak_bertanda32_t *lock)
{
    tak_bertanda32_t tmp;
    tak_bertanda32_t result;

    __asm__ __volatile__(
        "ldaxr %w0, [%2]\n\t"
        "cbnz %w0, 1f\n\t"
        "stxr %w0, %w3, [%2]\n\t"
        "1:\n\t"
        "eor %w1, %w0, #1"
        : "=&r"(tmp), "=&r"(result)
        : "r"(lock), "r"(1)
        : "memory"
    );

    return result ? BENAR : SALAH;
}
