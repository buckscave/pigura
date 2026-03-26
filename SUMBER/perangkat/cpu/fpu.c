/*
 * PIGURA OS - FPU.C
 * ==================
 * Implementasi FPU management.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "cpu.h"

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

static bool_t g_fpu_initialized = SALAH;
static bool_t g_fpu_available = SALAH;

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

status_t cpu_fpu_init(void)
{
    if (g_fpu_initialized) {
        return STATUS_SUDAH_ADA;
    }
    
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    {
        /* Cek FPU via CPUID */
        if (!cpu_fitur_cek(CPU_FEAT_FPU)) {
            g_fpu_available = SALAH;
            return STATUS_TIDAK_DUKUNG;
        }
        
        g_fpu_available = BENAR;
        
        /* Read/Write CR0 - x86_64 uses 64-bit CR0 */
#if defined(__x86_64__)
        {
            tak_bertanda64_t cr0;
            __asm__ __volatile__ ("mov %%cr0, %0" : "=r"(cr0));
            cr0 &= ~(1ULL << 2);  /* Clear EM */
            cr0 |= (1ULL << 1);   /* Set MP */
            cr0 &= ~(1ULL << 3);  /* Clear TS */
            __asm__ __volatile__ ("mov %0, %%cr0" : : "r"(cr0));
        }
#else
        {
            tak_bertanda32_t cr0;
            __asm__ __volatile__ ("mov %%cr0, %0" : "=r"(cr0));
            cr0 &= ~(1U << 2);  /* Clear EM */
            cr0 |= (1U << 1);   /* Set MP */
            cr0 &= ~(1U << 3);  /* Clear TS */
            __asm__ __volatile__ ("mov %0, %%cr0" : : "r"(cr0));
        }
#endif
        
        /* Init FPU */
        __asm__ __volatile__ ("fninit");
    }
#elif defined(__aarch64__)
    {
        tak_bertanda64_t cpacr;
        
        /* Enable SIMD/FP */
        __asm__ __volatile__ (
            "mrs %0, cpacr_el1"
            : "=r"(cpacr)
        );
        
        cpacr |= (3 << 20);  /* Enable FP/SIMD */
        
        __asm__ __volatile__ (
            "msr cpacr_el1, %0"
            :
            : "r"(cpacr)
        );
        
        g_fpu_available = BENAR;
    }
#endif
    
    g_fpu_initialized = BENAR;
    
    return STATUS_BERHASIL;
}

bool_t cpu_fpu_available(void)
{
    return g_fpu_available;
}

status_t cpu_fpu_save(void *buffer)
{
    if (buffer == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (!g_fpu_available) {
        return STATUS_TIDAK_DUKUNG;
    }
    
#if defined(ARSITEKTUR_X86)
    __asm__ __volatile__ (
        "fsave %0"
        : "=m"(*(char *)buffer)
    );
#elif defined(ARSITEKTUR_X86_64)
    __asm__ __volatile__ (
        "fxsave %0"
        : "=m"(*(char *)buffer)
    );
#elif defined(__aarch64__)
    /* Save FP registers */
#endif
    
    return STATUS_BERHASIL;
}

status_t cpu_fpu_restore(void *buffer)
{
    if (buffer == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (!g_fpu_available) {
        return STATUS_TIDAK_DUKUNG;
    }
    
#if defined(ARSITEKTUR_X86)
    __asm__ __volatile__ (
        "frstor %0"
        :
        : "m"(*(char *)buffer)
    );
#elif defined(ARSITEKTUR_X86_64)
    __asm__ __volatile__ (
        "fxrstor %0"
        :
        : "m"(*(char *)buffer)
    );
#elif defined(__aarch64__)
    /* Restore FP registers */
#endif
    
    return STATUS_BERHASIL;
}
