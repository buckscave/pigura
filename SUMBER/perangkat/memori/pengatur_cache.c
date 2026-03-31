/*
 * PIGURA OS - PENGATUR_CACHE.C
 * =============================
 * Implementasi cache controller untuk Pigura OS.
 *
 * Berkas ini mengimplementasikan fungsi-fungsi untuk mengelola
 * CPU cache (L1, L2, L3, L4) pada berbagai arsitektur.
 *
 * Kepatuhan standar:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Mendukung semua arsitektur (x86, x86_64, ARM, ARMv7, ARM64)
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "memori.h"
#include "../../inti/kernel.h"

/*
 * ===========================================================================
 * KONSTANTA LOKAL
 * ===========================================================================
 */

/* x86 CPUID leaf untuk cache info */
#define CPUID_CACHE_INFO        0x02
#define CPUID_CACHE_PARAMS      0x04
#define CPUID_EXT_FEATURES      0x80000000
#define CPUID_EXT_CACHE_INFO    0x80000006

/* Cache type values dari CPUID */
#define CACHE_TYPE_NULL         0
#define CACHE_TYPE_DATA         1
#define CACHE_TYPE_INSTRUCTION  2
#define CACHE_TYPE_UNIFIED      3

/* x86 MSR addresses */
#define MSR_IA32_PLATFORM_INFO  0x000000CE
#define MSR_IA32_TSC_AUX        0x000000C000010C
#define MSR_IA32_CR_PAT         0x00000277

/* x86 CR0 cache bits */
#define CR0_CD                  0x40000000
#define CR0_NW                  0x20000000

/* Cache line size default */
#define CACHE_LINE_SIZE_DEFAULT 64

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

/* Cache controller global */
static cache_controller_t g_cache_controller;

/* Status inisialisasi */
static bool_t g_cache_initialized = SALAH;

/* Default cache configurations */
static const ukuran_t g_default_l1_size = 32 * 1024;    /* 32 KB */
static const ukuran_t g_default_l2_size = 256 * 1024;   /* 256 KB */
static const ukuran_t g_default_l3_size = 8 * 1024 * 1024;  /* 8 MB */

/*
 * ===========================================================================
 * FUNGSI INTERNAL - x86
 * ===========================================================================
 */

#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || \
    defined(_M_X64)

/*
 * cpuid - Execute CPUID instruction
 *
 * Parameter:
 *   leaf - EAX input value
 *   subleaf - ECX input value
 *   eax - Output EAX
 *   ebx - Output EBX
 *   ecx - Output ECX
 *   edx - Output EDX
 */
static void cpuid(tak_bertanda32_t leaf, tak_bertanda32_t subleaf,
                  tak_bertanda32_t *eax, tak_bertanda32_t *ebx,
                  tak_bertanda32_t *ecx, tak_bertanda32_t *edx)
{
    tak_bertanda32_t a, b, c, d;

    __asm__ __volatile__ (
        "cpuid"
        : "=a"(a), "=b"(b), "=c"(c), "=d"(d)
        : "a"(leaf), "c"(subleaf)
    );

    if (eax != NULL) *eax = a;
    if (ebx != NULL) *ebx = b;
    if (ecx != NULL) *ecx = c;
    if (edx != NULL) *edx = d;
}

/*
 * cache_detect_x86 - Deteksi cache pada x86/x86_64
 *
 * Parameter:
 *   ctrl - Pointer ke controller
 */
static void cache_detect_x86(cache_controller_t *ctrl)
{
    tak_bertanda32_t eax, ebx, ecx, edx;
    tak_bertanda32_t level;
    tak_bertanda32_t tipe;
    tak_bertanda32_t ways;
    tak_bertanda32_t line_size;
    tak_bertanda32_t partitions;
    tak_bertanda32_t sets;
    ukuran_t ukuran;

    /* Cek apakah CPUID mendukung cache parameters */
    cpuid(0, 0, &eax, NULL, NULL, NULL);

    if (eax >= CPUID_CACHE_PARAMS) {
        /* Gunakan Deterministic Cache Parameters */
        level = 0;

        while (level < CACHE_LEVEL_MAKS) {
            cpuid(CPUID_CACHE_PARAMS, level, &eax, &ebx, &ecx, &edx);

            tipe = eax & 0x1F;

            if (tipe == CACHE_TYPE_NULL) {
                break;
            }

            /* Parse cache info */
            level = (eax >> 5) & 0x07;
            ways = ((ebx >> 22) & 0x3FF) + 1;
            partitions = ((ebx >> 12) & 0x3FF) + 1;
            line_size = (ebx & 0x0FFF) + 1;
            sets = ecx + 1;

            ukuran = ways * partitions * line_size * sets;

            /* Store ke controller */
            if (level > 0 && level <= CACHE_LEVEL_MAKS) {
                ctrl->ukuran[level - 1] = ukuran;
                ctrl->line_size[level - 1] = line_size;
                ctrl->ways[level - 1] = ways;
                ctrl->tipe[level - 1] = tipe;

                if (tipe == CACHE_TYPE_DATA) {
                    ctrl->policy[level - 1] = CACHE_POLICY_WRITE_BACK;
                } else if (tipe == CACHE_TYPE_INSTRUCTION) {
                    ctrl->policy[level - 1] = CACHE_POLICY_WRITE_THROUGH;
                } else {
                    ctrl->policy[level - 1] = CACHE_POLICY_WRITE_BACK;
                }

                if (level > ctrl->level_count) {
                    ctrl->level_count = level;
                }
            }

            level++;
        }
    } else {
        /* Gunakan CPUID leaf 0x02 atau default */
        cpuid(CPUID_CACHE_INFO, 0, &eax, &ebx, &ecx, &edx);

        /* Default values */
        ctrl->level_count = 3;
        ctrl->ukuran[0] = g_default_l1_size;
        ctrl->ukuran[1] = g_default_l2_size;
        ctrl->ukuran[2] = g_default_l3_size;
        ctrl->line_size[0] = CACHE_LINE_SIZE_DEFAULT;
        ctrl->line_size[1] = CACHE_LINE_SIZE_DEFAULT;
        ctrl->line_size[2] = CACHE_LINE_SIZE_DEFAULT;
        ctrl->ways[0] = 8;
        ctrl->ways[1] = 8;
        ctrl->ways[2] = 16;
        ctrl->tipe[0] = CACHE_TYPE_DATA;
        ctrl->tipe[1] = CACHE_TYPE_UNIFIED;
        ctrl->tipe[2] = CACHE_TYPE_UNIFIED;
        ctrl->policy[0] = CACHE_POLICY_WRITE_BACK;
        ctrl->policy[1] = CACHE_POLICY_WRITE_BACK;
        ctrl->policy[2] = CACHE_POLICY_WRITE_BACK;
    }
}

/*
 * cache_flush_x86 - Flush cache pada x86
 *
 * Parameter:
 *   level - Level cache
 */
static void cache_flush_x86(tak_bertanda32_t level)
{
    (void)level;
    /* Wbinvd flush semua cache */
    __asm__ __volatile__ ("wbinvd" : : : "memory");
}

/*
 * cache_invalidate_x86 - Invalidate cache pada x86
 *
 * Parameter:
 *   level - Level cache
 */
static void cache_invalidate_x86(tak_bertanda32_t level)
{
    (void)level;
    /* Invd invalidate tanpa writeback */
    __asm__ __volatile__ ("invd" : : : "memory");
}

/*
 * cache_enable_x86 - Enable cache pada x86
 *
 * Parameter:
 *   level - Level cache
 */
static void cache_enable_x86(tak_bertanda32_t level)
{
    (void)level;
    tak_bertanda32_t cr0;

    /* Baca CR0 */
    __asm__ __volatile__ (
        "mov %%cr0, %0"
        : "=r"(cr0)
    );

    /* Clear CD dan NW bits */
    cr0 &= ~(CR0_CD | CR0_NW);

    /* Tulis CR0 */
    __asm__ __volatile__ (
        "mov %0, %%cr0"
        :
        : "r"(cr0)
    );
}

/*
 * cache_disable_x86 - Disable cache pada x86
 *
 * Parameter:
 *   level - Level cache
 */
static void cache_disable_x86(tak_bertanda32_t level)
{
    tak_bertanda32_t cr0;

    /* Baca CR0 */
    __asm__ __volatile__ (
        "mov %%cr0, %0"
        : "=r"(cr0)
    );

    /* Set CD dan clear NW */
    cr0 |= CR0_CD;
    cr0 &= ~CR0_NW;

    /* Tulis CR0 */
    __asm__ __volatile__ (
        "mov %0, %%cr0"
        :
        : "r"(cr0)
    );

    /* Flush cache */
    cache_flush_x86(level);
}

#endif /* x86/x86_64 */

/*
 * ===========================================================================
 * FUNGSI INTERNAL - ARM
 * ===========================================================================
 */

#if defined(__arm__) || defined(__aarch64__) || defined(_M_ARM)

/*
 * cache_detect_arm - Deteksi cache pada ARM
 *
 * Parameter:
 *   ctrl - Pointer ke controller
 */
static void cache_detect_arm(cache_controller_t *ctrl)
{
    tak_bertanda32_t ctr;
    tak_bertanda32_t csselr;
    tak_bertanda32_t ccsidr;
    tak_bertanda32_t line_size;
    tak_bertanda32_t ways;
    tak_bertanda32_t sets;
    ukuran_t ukuran;

    /* Baca CTR (Cache Type Register) */
    __asm__ __volatile__ (
        "mrc p15, 0, %0, c0, c0, 1"
        : "=r"(ctr)
    );

    /* Default untuk ARM */
    ctrl->level_count = 2;
    ctrl->ukuran[0] = g_default_l1_size;
    ctrl->ukuran[1] = g_default_l2_size;
    ctrl->line_size[0] = CACHE_LINE_SIZE_DEFAULT;
    ctrl->line_size[1] = CACHE_LINE_SIZE_DEFAULT;
    ctrl->ways[0] = 4;
    ctrl->ways[1] = 8;
    ctrl->tipe[0] = CACHE_TYPE_DATA;
    ctrl->tipe[1] = CACHE_TYPE_UNIFIED;
    ctrl->policy[0] = CACHE_POLICY_WRITE_BACK;
    ctrl->policy[1] = CACHE_POLICY_WRITE_BACK;
}

/*
 * cache_flush_arm - Flush cache pada ARM
 *
 * Parameter:
 *   level - Level cache
 */
static void cache_flush_arm(tak_bertanda32_t level)
{
    /* Clean dan invalidate seluruh D-cache */
    __asm__ __volatile__ (
        "mcr p15, 0, r0, c7, c10, 0\n"  /* Clean D-cache */
        "mcr p15, 0, r0, c7, c6, 0\n"   /* Invalidate D-cache */
        : : : "r0", "memory"
    );
}

/*
 * cache_invalidate_arm - Invalidate cache pada ARM
 *
 * Parameter:
 *   level - Level cache
 */
static void cache_invalidate_arm(tak_bertanda32_t level)
{
    /* Invalidate seluruh I-cache dan D-cache */
    __asm__ __volatile__ (
        "mcr p15, 0, r0, c7, c6, 0\n"   /* Invalidate D-cache */
        "mcr p15, 0, r0, c7, c5, 0\n"   /* Invalidate I-cache */
        : : : "r0", "memory"
    );
}

/*
 * cache_enable_arm - Enable cache pada ARM
 *
 * Parameter:
 *   level - Level cache
 */
static void cache_enable_arm(tak_bertanda32_t level)
{
    tak_bertanda32_t sctlr;

    /* Baca SCTLR */
    __asm__ __volatile__ (
        "mrc p15, 0, %0, c1, c0, 0"
        : "=r"(sctlr)
    );

    /* Set I (bit 12) dan C (bit 2) */
    sctlr |= (1 << 12) | (1 << 2);

    /* Tulis SCTLR */
    __asm__ __volatile__ (
        "mcr p15, 0, %0, c1, c0, 0"
        :
        : "r"(sctlr)
    );
}

/*
 * cache_disable_arm - Disable cache pada ARM
 *
 * Parameter:
 *   level - Level cache
 */
static void cache_disable_arm(tak_bertanda32_t level)
{
    tak_bertanda32_t sctlr;

    /* Flush dulu */
    cache_flush_arm(level);

    /* Baca SCTLR */
    __asm__ __volatile__ (
        "mrc p15, 0, %0, c1, c0, 0"
        : "=r"(sctlr)
    );

    /* Clear I (bit 12) dan C (bit 2) */
    sctlr &= ~((1 << 12) | (1 << 2));

    /* Tulis SCTLR */
    __asm__ __volatile__ (
        "mcr p15, 0, %0, c1, c0, 0"
        :
        : "r"(sctlr)
    );
}

#endif /* ARM */

/*
 * ===========================================================================
 * FUNGSI INTERNAL - GENERIC
 * ===========================================================================
 */

/*
 * cache_detect_generic - Deteksi cache generic
 *
 * Parameter:
 *   ctrl - Pointer ke controller
 */
static void cache_detect_generic(cache_controller_t *ctrl)
{
    /* Default values */
    ctrl->level_count = 2;
    ctrl->ukuran[0] = g_default_l1_size;
    ctrl->ukuran[1] = g_default_l2_size;
    ctrl->line_size[0] = CACHE_LINE_SIZE_DEFAULT;
    ctrl->line_size[1] = CACHE_LINE_SIZE_DEFAULT;
    ctrl->ways[0] = 4;
    ctrl->ways[1] = 8;
    ctrl->tipe[0] = CACHE_TYPE_UNIFIED;
    ctrl->tipe[1] = CACHE_TYPE_UNIFIED;
    ctrl->policy[0] = CACHE_POLICY_WRITE_BACK;
    ctrl->policy[1] = CACHE_POLICY_WRITE_BACK;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

/*
 * cache_init - Inisialisasi cache controller
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t cache_init(void)
{
    cache_controller_t *ctrl;

    if (g_cache_initialized) {
        return STATUS_SUDAH_ADA;
    }

    ctrl = &g_cache_controller;

    /* Reset controller */
    kernel_memset(ctrl, 0, sizeof(cache_controller_t));

    /* Set magic */
    ctrl->magic = CACHE_CTRL_MAGIC;

    /* Deteksi cache berdasarkan arsitektur */
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || \
    defined(_M_X64)
    cache_detect_x86(ctrl);
#elif defined(__arm__) || defined(__aarch64__) || defined(_M_ARM)
    cache_detect_arm(ctrl);
#else
    cache_detect_generic(ctrl);
#endif

    /* Set enabled */
    ctrl->enabled = BENAR;
    ctrl->write_back = BENAR;

    g_cache_initialized = BENAR;

    return STATUS_BERHASIL;
}

/*
 * cache_enable - Aktifkan cache
 *
 * Parameter:
 *   level - Level cache (1-3, 0 untuk semua)
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t cache_enable(tak_bertanda32_t level)
{
    if (!g_cache_initialized) {
        return STATUS_PARAM_INVALID;
    }

    if (level > CACHE_LEVEL_MAKS) {
        return STATUS_PARAM_INVALID;
    }

    /* Enable berdasarkan arsitektur */
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || \
    defined(_M_X64)
    cache_enable_x86(level);
#elif defined(__arm__) || defined(__aarch64__) || defined(_M_ARM)
    cache_enable_arm(level);
#endif

    g_cache_controller.enabled = BENAR;

    return STATUS_BERHASIL;
}

/*
 * cache_disable - Nonaktifkan cache
 *
 * Parameter:
 *   level - Level cache
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t cache_disable(tak_bertanda32_t level)
{
    if (!g_cache_initialized) {
        return STATUS_PARAM_INVALID;
    }

    if (level > CACHE_LEVEL_MAKS) {
        return STATUS_PARAM_INVALID;
    }

    /* Disable berdasarkan arsitektur */
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || \
    defined(_M_X64)
    cache_disable_x86(level);
#elif defined(__arm__) || defined(__aarch64__) || defined(_M_ARM)
    cache_disable_arm(level);
#endif

    g_cache_controller.enabled = SALAH;

    return STATUS_BERHASIL;
}

/*
 * cache_flush - Flush cache
 *
 * Parameter:
 *   level - Level cache (0 untuk semua)
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t cache_flush(tak_bertanda32_t level)
{
    if (!g_cache_initialized) {
        return STATUS_PARAM_INVALID;
    }

    /* Flush berdasarkan arsitektur */
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || \
    defined(_M_X64)
    cache_flush_x86(level);
#elif defined(__arm__) || defined(__aarch64__) || defined(_M_ARM)
    cache_flush_arm(level);
#endif

    return STATUS_BERHASIL;
}

/*
 * cache_invalidate - Invalidate cache
 *
 * Parameter:
 *   level - Level cache (0 untuk semua)
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t cache_invalidate(tak_bertanda32_t level)
{
    if (!g_cache_initialized) {
        return STATUS_PARAM_INVALID;
    }

    /* Invalidate berdasarkan arsitektur */
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || \
    defined(_M_X64)
    cache_invalidate_x86(level);
#elif defined(__arm__) || defined(__aarch64__) || defined(_M_ARM)
    cache_invalidate_arm(level);
#endif

    return STATUS_BERHASIL;
}

/*
 * cache_info - Dapatkan info cache
 *
 * Parameter:
 *   level - Level cache (1-3)
 *   ctrl - Buffer untuk info
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t cache_info(tak_bertanda32_t level, cache_controller_t *ctrl)
{
    if (ctrl == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (!g_cache_initialized) {
        return STATUS_PARAM_INVALID;
    }

    if (level == 0 || level > CACHE_LEVEL_MAKS) {
        /* Copy seluruh controller */
        kernel_memcpy(ctrl, &g_cache_controller, sizeof(cache_controller_t));
    } else {
        /* Copy info level tertentu */
        kernel_memcpy(ctrl, &g_cache_controller, sizeof(cache_controller_t));
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI TAMBAHAN
 * ===========================================================================
 */

/*
 * cache_ukuran - Dapatkan ukuran cache
 *
 * Parameter:
 *   level - Level cache (1-3)
 *
 * Return: Ukuran cache dalam byte
 */
ukuran_t cache_ukuran(tak_bertanda32_t level)
{
    if (!g_cache_initialized) {
        return 0;
    }

    if (level == 0 || level > CACHE_LEVEL_MAKS) {
        return 0;
    }

    return g_cache_controller.ukuran[level - 1];
}

/*
 * cache_line_size - Dapatkan ukuran cache line
 *
 * Parameter:
 *   level - Level cache (1-3)
 *
 * Return: Ukuran cache line dalam byte
 */
ukuran_t cache_line_size(tak_bertanda32_t level)
{
    if (!g_cache_initialized) {
        return CACHE_LINE_SIZE_DEFAULT;
    }

    if (level == 0 || level > CACHE_LEVEL_MAKS) {
        return CACHE_LINE_SIZE_DEFAULT;
    }

    return g_cache_controller.line_size[level - 1];
}

/*
 * cache_flush_range - Flush range alamat
 *
 * Parameter:
 *   addr - Alamat awal
 *   ukuran - Ukuran range
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t cache_flush_range(void *addr, ukuran_t ukuran)
{
    tak_bertanda8_t *ptr;
    tak_bertanda8_t *akhir;
    ukuran_t line_size;

    if (!g_cache_initialized) {
        return STATUS_PARAM_INVALID;
    }

    if (addr == NULL || ukuran == 0) {
        return STATUS_PARAM_INVALID;
    }

    line_size = cache_line_size(1);

    /* Align ke cache line */
    ptr = (tak_bertanda8_t *)((uintptr_t)addr & ~(line_size - 1));
    akhir = (tak_bertanda8_t *)addr + ukuran;

    /* Flush per cache line */
    while (ptr < akhir) {
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || \
    defined(_M_X64)
        /* CLFLUSH */
        __asm__ __volatile__ (
            "clflush %0"
            :
            : "m"(*ptr)
            : "memory"
        );
#elif defined(__arm__) || defined(__aarch64__)
        /* Clean by VA */
        __asm__ __volatile__ (
            "mcr p15, 0, %0, c7, c10, 1"
            :
            : "r"(ptr)
            : "memory"
        );
#endif
        ptr += line_size;
    }

    return STATUS_BERHASIL;
}

/*
 * cache_invalidate_range - Invalidate range alamat
 *
 * Parameter:
 *   addr - Alamat awal
 *   ukuran - Ukuran range
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t cache_invalidate_range(void *addr, ukuran_t ukuran)
{
    tak_bertanda8_t *ptr;
    tak_bertanda8_t *akhir;
    ukuran_t line_size;

    if (!g_cache_initialized) {
        return STATUS_PARAM_INVALID;
    }

    if (addr == NULL || ukuran == 0) {
        return STATUS_PARAM_INVALID;
    }

    line_size = cache_line_size(1);

    /* Align ke cache line */
    ptr = (tak_bertanda8_t *)((uintptr_t)addr & ~(line_size - 1));
    akhir = (tak_bertanda8_t *)addr + ukuran;

    /* Invalidate per cache line */
    while (ptr < akhir) {
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || \
    defined(_M_X64)
        /* CLFLUSH juga invalidate */
        __asm__ __volatile__ (
            "clflush %0"
            :
            : "m"(*ptr)
            : "memory"
        );
#elif defined(__arm__) || defined(__aarch64__)
        /* Invalidate by VA */
        __asm__ __volatile__ (
            "mcr p15, 0, %0, c7, c6, 1"
            :
            : "r"(ptr)
            : "memory"
        );
#endif
        ptr += line_size;
    }

    return STATUS_BERHASIL;
}

/*
 * cache_total_ukuran - Dapatkan total ukuran semua cache
 *
 * Return: Total ukuran cache dalam byte
 */
ukuran_t cache_total_ukuran(void)
{
    ukuran_t total;
    tak_bertanda32_t i;

    if (!g_cache_initialized) {
        return 0;
    }

    total = 0;
    for (i = 0; i < g_cache_controller.level_count; i++) {
        total += g_cache_controller.ukuran[i];
    }

    return total;
}
