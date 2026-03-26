/*
 * PIGURA OS - SSE.C
 * ==================
 * Implementasi SSE (Streaming SIMD Extensions).
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

static bool_t g_sse_initialized = SALAH;
static bool_t g_sse_available = SALAH;

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

status_t cpu_sse_init(void)
{
    if (g_sse_initialized) {
        return STATUS_SUDAH_ADA;
    }
    
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    {
        /* Cek SSE support */
        if (!cpu_fitur_cek(CPU_FEAT_SSE)) {
            g_sse_available = SALAH;
            return STATUS_TIDAK_DUKUNG;
        }
        
        g_sse_available = BENAR;
        
        /* Configure CR0 - x86_64 uses 64-bit CR0 */
#if defined(__x86_64__)
        {
            tak_bertanda64_t cr0;
            __asm__ __volatile__ ("mov %%cr0, %0" : "=r"(cr0));
            cr0 &= ~(1ULL << 2);  /* Clear EM */
            cr0 |= (1ULL << 1);   /* Set MP */
            __asm__ __volatile__ ("mov %0, %%cr0" : : "r"(cr0));
        }
#else
        {
            tak_bertanda32_t cr0;
            __asm__ __volatile__ ("mov %%cr0, %0" : "=r"(cr0));
            cr0 &= ~(1U << 2);  /* Clear EM */
            cr0 |= (1U << 1);   /* Set MP */
            __asm__ __volatile__ ("mov %0, %%cr0" : : "r"(cr0));
        }
#endif
        
        /* Configure CR4 - x86_64 uses 64-bit CR4 */
#if defined(__x86_64__)
        {
            tak_bertanda64_t cr4;
            __asm__ __volatile__ ("mov %%cr4, %0" : "=r"(cr4));
            cr4 |= (1ULL << 9);   /* OSFXSR */
            cr4 |= (1ULL << 10);  /* OSXMMEXCPT */
            __asm__ __volatile__ ("mov %0, %%cr4" : : "r"(cr4));
        }
#else
        {
            tak_bertanda32_t cr4;
            __asm__ __volatile__ ("mov %%cr4, %0" : "=r"(cr4));
            cr4 |= (1U << 9);   /* OSFXSR */
            cr4 |= (1U << 10);  /* OSXMMEXCPT */
            __asm__ __volatile__ ("mov %0, %%cr4" : : "r"(cr4));
        }
#endif
    }
#endif
    
    g_sse_initialized = BENAR;
    
    return STATUS_BERHASIL;
}

bool_t cpu_sse_available(void)
{
    return g_sse_available;
}

status_t cpu_sse_save(void *buffer)
{
    if (buffer == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (!g_sse_available) {
        return STATUS_TIDAK_DUKUNG;
    }
    
#if defined(ARSITEKTUR_X86_64)
    __asm__ __volatile__ (
        "fxsave %0"
        : "=m"(*(char *)buffer)
        :
        : "memory"
    );
#endif
    
    return STATUS_BERHASIL;
}

status_t cpu_sse_restore(void *buffer)
{
    if (buffer == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (!g_sse_available) {
        return STATUS_TIDAK_DUKUNG;
    }
    
#if defined(ARSITEKTUR_X86_64)
    __asm__ __volatile__ (
        "fxrstor %0"
        :
        : "m"(*(char *)buffer)
        : "memory"
    );
#endif
    
    return STATUS_BERHASIL;
}
