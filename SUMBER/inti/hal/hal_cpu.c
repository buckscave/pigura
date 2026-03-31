/*
 * PIGURA OS - HAL_CPU.C
 * ---------------------
 * Implementasi fungsi CPU untuk Hardware Abstraction Layer.
 *
 * Berkas ini berisi implementasi fungsi-fungsi yang berkaitan dengan
 * operasi CPU seperti deteksi CPU, kontrol interrupt, dan operasi cache.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 *
 * Versi: 2.0
 * Tanggal: 2025
 */

#include "hal.h"
#include "../kernel.h"

/*
 * Untuk arsitektur x86/x86_64, include header CPU-specific
 */
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
#include "../arsitektur/x86/cpu_x86.h"
#endif

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

/* CPUID feature flags - EDX */
#define CPUID_FEAT_EDX_FPU      (1U << 0)
#define CPUID_FEAT_EDX_VME      (1U << 1)
#define CPUID_FEAT_EDX_DE       (1U << 2)
#define CPUID_FEAT_EDX_PSE      (1U << 3)
#define CPUID_FEAT_EDX_TSC      (1U << 4)
#define CPUID_FEAT_EDX_MSR      (1U << 5)
#define CPUID_FEAT_EDX_PAE      (1U << 6)
#define CPUID_FEAT_EDX_MCE      (1U << 7)
#define CPUID_FEAT_EDX_CX8      (1U << 8)
#define CPUID_FEAT_EDX_APIC     (1U << 9)
#define CPUID_FEAT_EDX_SEP      (1U << 11)
#define CPUID_FEAT_EDX_MTRR     (1U << 12)
#define CPUID_FEAT_EDX_PGE      (1U << 13)
#define CPUID_FEAT_EDX_MCA      (1U << 14)
#define CPUID_FEAT_EDX_CMOV     (1U << 15)
#define CPUID_FEAT_EDX_PAT      (1U << 16)
#define CPUID_FEAT_EDX_PSE36    (1U << 17)
#define CPUID_FEAT_EDX_PSN      (1U << 18)
#define CPUID_FEAT_EDX_CLFSH    (1U << 19)
#define CPUID_FEAT_EDX_DS       (1U << 21)
#define CPUID_FEAT_EDX_ACPI     (1U << 22)
#define CPUID_FEAT_EDX_MMX      (1U << 23)
#define CPUID_FEAT_EDX_FXSR     (1U << 24)
#define CPUID_FEAT_EDX_SSE      (1U << 25)
#define CPUID_FEAT_EDX_SSE2     (1U << 26)
#define CPUID_FEAT_EDX_SS       (1U << 27)
#define CPUID_FEAT_EDX_HTT      (1U << 28)
#define CPUID_FEAT_EDX_TM       (1U << 29)
#define CPUID_FEAT_EDX_IA64     (1U << 30)
#define CPUID_FEAT_EDX_PBE      (1U << 31)

/* CPUID extended feature flags - ECX */
#define CPUID_FEAT_ECX_SSE3     (1U << 0)
#define CPUID_FEAT_ECX_PCLMUL   (1U << 1)
#define CPUID_FEAT_ECX_DTES64   (1U << 2)
#define CPUID_FEAT_ECX_MONITOR  (1U << 3)
#define CPUID_FEAT_ECX_DS_CPL   (1U << 4)
#define CPUID_FEAT_ECX_VMX      (1U << 5)
#define CPUID_FEAT_ECX_SMX      (1U << 6)
#define CPUID_FEAT_ECX_EST      (1U << 7)
#define CPUID_FEAT_ECX_TM2      (1U << 8)
#define CPUID_FEAT_ECX_SSSE3    (1U << 9)
#define CPUID_FEAT_ECX_CID      (1U << 10)
#define CPUID_FEAT_ECX_FMA      (1U << 12)
#define CPUID_FEAT_ECX_CX16     (1U << 13)
#define CPUID_FEAT_ECX_XTPR     (1U << 14)
#define CPUID_FEAT_ECX_PDCM     (1U << 15)
#define CPUID_FEAT_ECX_PCID     (1U << 17)
#define CPUID_FEAT_ECX_DCA      (1U << 18)
#define CPUID_FEAT_ECX_SSE41    (1U << 19)
#define CPUID_FEAT_ECX_SSE42    (1U << 20)
#define CPUID_FEAT_ECX_X2APIC   (1U << 21)
#define CPUID_FEAT_ECX_MOVBE    (1U << 22)
#define CPUID_FEAT_ECX_POPCNT   (1U << 23)
#define CPUID_FEAT_ECX_TSC_DL   (1U << 24)
#define CPUID_FEAT_ECX_AES      (1U << 25)
#define CPUID_FEAT_ECX_XSAVE    (1U << 26)
#define CPUID_FEAT_ECX_OSXSAVE  (1U << 27)
#define CPUID_FEAT_ECX_AVX      (1U << 28)

/*
 * ============================================================================
 * STRUKTUR DATA LOKAL (LOCAL DATA STRUCTURES)
 * ============================================================================
 */

/* Struktur untuk menyimpan hasil CPUID */
typedef struct {
    tak_bertanda32_t eax;
    tak_bertanda32_t ebx;
    tak_bertanda32_t ecx;
    tak_bertanda32_t edx;
} cpuid_result_t;

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Informasi CPU yang terdeteksi */
static hal_cpu_info_t cpu_info_local;

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * cpuid
 * -----
 * Jalankan instruksi CPUID.
 *
 * Parameter:
 *   leaf    - EAX input (function)
 *   subleaf - ECX input (sub-function untuk leaf tertentu)
 *   result  - Pointer untuk menyimpan hasil
 */
static void cpuid(tak_bertanda32_t leaf, tak_bertanda32_t subleaf,
                  cpuid_result_t *result)
{
    __asm__ __volatile__(
        "cpuid"
        : "=a"(result->eax),
          "=b"(result->ebx),
          "=c"(result->ecx),
          "=d"(result->edx)
        : "a"(leaf),
          "c"(subleaf)
    );
}

/*
 * cpu_detect_vendor
 * -----------------
 * Deteksi vendor CPU.
 */
static void cpu_detect_vendor(hal_cpu_info_t *info)
{
    cpuid_result_t res;

    cpuid(0, 0, &res);

    /* Vendor string disimpan dalam EBX, EDX, ECX */
    *((tak_bertanda32_t *)(info->vendor + 0)) = res.ebx;
    *((tak_bertanda32_t *)(info->vendor + 4)) = res.edx;
    *((tak_bertanda32_t *)(info->vendor + 8)) = res.ecx;
    info->vendor[12] = '\0';
}

/*
 * cpu_detect_brand
 * ----------------
 * Deteksi brand string CPU.
 */
static void cpu_detect_brand(hal_cpu_info_t *info)
{
    cpuid_result_t res;
    tak_bertanda32_t i;

    /* Brand string didapatkan dari leaf 0x80000002-0x80000004 */
    for (i = 0; i < 3; i++) {
        cpuid(0x80000002 + i, 0, &res);

        *((tak_bertanda32_t *)(info->brand + (i * 16) + 0)) = res.eax;
        *((tak_bertanda32_t *)(info->brand + (i * 16) + 4)) = res.ebx;
        *((tak_bertanda32_t *)(info->brand + (i * 16) + 8)) = res.ecx;
        *((tak_bertanda32_t *)(info->brand + (i * 16) + 12)) = res.edx;
    }
    info->brand[48] = '\0';

    /* Trim spasi di awal dan akhir */
    {
        char *start = info->brand;
        char *end = info->brand + 47;

        /* Trim spasi di awal */
        while (*start == ' ') {
            start++;
        }

        /* Trim spasi di akhir */
        while (end > start && *end == ' ') {
            end--;
        }
        *(end + 1) = '\0';

        /* Pindahkan ke awal buffer jika perlu */
        if (start != info->brand) {
            kernel_memmove(info->brand, start, kernel_strlen(start) + 1);
        }
    }
}

/*
 * cpu_detect_features
 * -------------------
 * Deteksi fitur CPU.
 */
static void cpu_detect_features(hal_cpu_info_t *info)
{
    cpuid_result_t res;

    cpuid(1, 0, &res);

    /* Simpan feature flags */
    info->features = res.edx;
    info->features_ext = res.ecx;

    /* Deteksi family, model, stepping */
    info->stepping = res.eax & 0xF;
    info->model = (res.eax >> 4) & 0xF;
    info->family = (res.eax >> 8) & 0xF;

    /* Extended family dan model untuk family >= 0xF */
    if (info->family == 0xF) {
        info->family += (res.eax >> 20) & 0xFF;
        info->model += ((res.eax >> 16) & 0xF) << 4;
    }

    /* Cache line size */
    info->cache_line = (res.ebx >> 8) & 0xFF;

    /* Deteksi jumlah core jika hyperthreading */
    if (res.edx & CPUID_FEAT_EDX_HTT) {
        info->threads = (res.ebx >> 16) & 0xFF;
    } else {
        info->threads = 1;
    }
    info->cores = 1; /* Perlu leaf 0x0B untuk akurat */
}

/*
 * cpu_detect_cache
 * ----------------
 * Deteksi informasi cache CPU.
 */
static void cpu_detect_cache(hal_cpu_info_t *info)
{
    cpuid_result_t res;

    /* Gunakan leaf 0x80000005 untuk L1 cache (AMD) atau leaf 2 (Intel) */
    cpuid(0x80000005, 0, &res);

    /* L1 data cache (dalam KB) */
    info->cache_l1_data = ((res.ecx >> 24) & 0xFF) * 1024;
    /* L1 instruction cache (dalam KB) */
    info->cache_l1_inst = ((res.edx >> 24) & 0xFF) * 1024;

    /* Gunakan leaf 0x80000006 untuk L2 dan L3 cache */
    cpuid(0x80000006, 0, &res);

    /* L2 cache (dalam KB) */
    info->cache_l2 = ((res.ecx >> 16) & 0xFFFF) * 1024;
    /* L3 cache (dalam KB) */
    info->cache_l3 = ((res.edx >> 18) & 0x3FFF) * 512 * 1024;

    /* Jika tidak ada informasi dari extended leaf, gunakan default */
    if (info->cache_l1_data == 0) {
        /* Default untuk Intel */
        info->cache_l1_data = 32 * 1024;   /* 32 KB */
        info->cache_l1_inst = 32 * 1024;   /* 32 KB */
        info->cache_l2 = 256 * 1024;       /* 256 KB */
        info->cache_l3 = 8 * 1024 * 1024;  /* 8 MB */
    }
}

/*
 * cpu_detect_frequency
 * --------------------
 * Estimasi frekuensi CPU.
 * Metode: hitung siklus CPU dalam interval tertentu menggunakan PIT.
 */
static void cpu_detect_frequency(hal_cpu_info_t *info)
{
    tak_bertanda64_t tsc_start;
    tak_bertanda64_t tsc_end;
    tak_bertanda32_t i;
    tak_bertanda32_t freq_khz;

    /* Baca TSC awal */
    __asm__ __volatile__("rdtsc" : "=A"(tsc_start));

    /* Delay kira-kira 50ms menggunakan busy wait */
    for (i = 0; i < 5000000; i++) {
        __asm__ __volatile__("nop");
    }

    /* Baca TSC akhir */
    __asm__ __volatile__("rdtsc" : "=A"(tsc_end));

    /* Estimasi frekuensi dalam kHz (sangat kasar) */
    freq_khz = (tak_bertanda32_t)((tsc_end - tsc_start) / 50);

    /* Batasi ke nilai yang wajar jika estimasi terlalu jauh */
    if (freq_khz < 100000) {
        freq_khz = 1000000;     /* Default 1 GHz */
    } else if (freq_khz > 10000000) {
        freq_khz = 3000000;     /* Default 3 GHz */
    }

    info->freq_khz = freq_khz;
}

/*
 * cpu_do_reset
 * ------------
 * Reset CPU menggunakan keyboard controller.
 */
static void cpu_do_reset(void)
{
    tak_bertanda8_t temp;

    /* Disable interrupt */
    hal_cpu_disable_interrupts();

    /* Clear keyboard buffer */
    do {
        temp = inb(0x64);
        if (temp & 0x01) {
            (void)inb(0x60);
        }
    } while (temp & 0x02);

    /* Reset CPU via keyboard controller */
    outb(0x64, 0xFE);  /* Pulse reset line */

    /* Jika tidak berhasil, infinite loop */
    for (;;) {
        cpu_halt();
    }
}

/*
 * cpu_do_triple_fault
 * -------------------
 * Reset CPU menggunakan triple fault.
 */
static void cpu_do_triple_fault(void)
{
    /* Load IDTR dengan NULL pointer untuk memicu triple fault */
    struct {
        tak_bertanda16_t limit;
        tak_bertanda32_t base;
    } __attribute__((packed)) null_idt = {0, 0};

    __asm__ __volatile__(
        "lidt %0\n\t"
        "int $3"
        :
        : "m"(null_idt)
    );

    /* Tidak akan sampai sini */
    for (;;) {
        cpu_halt();
    }
}

/*
 * ============================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_cpu_init
 * ------------
 * Inisialisasi subsistem CPU.
 */
status_t hal_cpu_init(void)
{
    hal_cpu_info_t *info = &g_hal_state.cpu;

    kernel_memset(info, 0, sizeof(hal_cpu_info_t));

    /* Deteksi informasi CPU */
    cpu_detect_vendor(info);
    cpu_detect_brand(info);
    cpu_detect_features(info);
    cpu_detect_cache(info);
    cpu_detect_frequency(info);

    /* Set ID CPU */
    info->id = 0;
    info->apic_id = 0;

    /* Copy ke local */
    kernel_memcpy(&cpu_info_local, info, sizeof(hal_cpu_info_t));

    return STATUS_BERHASIL;
}

/*
 * hal_cpu_halt
 * ------------
 * Hentikan CPU.
 */
void hal_cpu_halt(void)
{
    cpu_halt();
    for (;;) {
        /* Tidak akan sampai sini */
    }
}

/*
 * hal_cpu_reset
 * -------------
 * Reset CPU.
 */
void hal_cpu_reset(bool_t hard)
{
    if (hard) {
        /* Triple fault untuk hard reset */
        cpu_do_triple_fault();
    } else {
        /* Keyboard controller reset */
        cpu_do_reset();
    }
    __builtin_unreachable();
}

/*
 * hal_cpu_get_id
 * --------------
 * Dapatkan ID CPU saat ini.
 */
tak_bertanda32_t hal_cpu_get_id(void)
{
    return cpu_info_local.id;
}

/*
 * hal_cpu_get_info
 * ----------------
 * Dapatkan informasi CPU.
 */
status_t hal_cpu_get_info(hal_cpu_info_t *info)
{
    if (info == NULL) {
        return STATUS_PARAM_NULL;
    }

    kernel_memcpy(info, &cpu_info_local, sizeof(hal_cpu_info_t));

    return STATUS_BERHASIL;
}

/*
 * hal_cpu_enable_interrupts
 * -------------------------
 * Aktifkan interupsi.
 */
void hal_cpu_enable_interrupts(void)
{
    cpu_enable_irq();
}

/*
 * hal_cpu_disable_interrupts
 * --------------------------
 * Nonaktifkan interupsi.
 */
void hal_cpu_disable_interrupts(void)
{
    cpu_disable_irq();
}

/*
 * hal_cpu_save_interrupts
 * -----------------------
 * Simpan state interupsi.
 */
tak_bertanda32_t hal_cpu_save_interrupts(void)
{
    return cpu_save_flags();
}

/*
 * hal_cpu_restore_interrupts
 * --------------------------
 * Restore state interupsi.
 */
void hal_cpu_restore_interrupts(tak_bertanda32_t state)
{
    cpu_restore_flags(state);
}

/*
 * hal_cpu_invalidate_tlb
 * ----------------------
 * Invalidate TLB untuk satu halaman.
 */
void hal_cpu_invalidate_tlb(void *addr)
{
    cpu_invlpg(addr);
}

/*
 * hal_cpu_invalidate_tlb_all
 * --------------------------
 * Invalidate seluruh TLB.
 */
void hal_cpu_invalidate_tlb_all(void)
{
    cpu_reload_cr3();
}

/*
 * hal_cpu_cache_flush
 * -------------------
 * Flush cache CPU.
 */
void hal_cpu_cache_flush(void)
{
    /* wbinvd memflush semua cache internal ke memori */
    __asm__ __volatile__("wbinvd" ::: "memory");
}

/*
 * hal_cpu_cache_disable
 * ---------------------
 * Nonaktifkan cache CPU.
 */
void hal_cpu_cache_disable(void)
{
    tak_bertanda32_t cr0;

    cr0 = cpu_read_cr0();

    /* Set CD (Cache Disable) dan NW (Not Write-through) bits */
    cr0 |= (1U << 30) | (1U << 29);

    cpu_write_cr0(cr0);

    /* Flush cache setelah disable */
    hal_cpu_cache_flush();
}

/*
 * hal_cpu_cache_enable
 * --------------------
 * Aktifkan cache CPU.
 */
void hal_cpu_cache_enable(void)
{
    tak_bertanda32_t cr0;

    /* Flush cache sebelum enable */
    hal_cpu_cache_flush();

    cr0 = cpu_read_cr0();

    /* Clear CD dan NW bits */
    cr0 &= ~((1U << 30) | (1U << 29));

    cpu_write_cr0(cr0);
}
