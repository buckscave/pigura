/*
 * PIGURA OS - AVX.C
 * ==================
 * Implementasi AVX (Advanced Vector Extensions).
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

static bool_t g_avx_initialized = SALAH;
static bool_t g_avx_available = SALAH;

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

status_t cpu_avx_init(void)
{
    if (g_avx_initialized) {
        return STATUS_SUDAH_ADA;
    }
    
#if defined(ARSITEKTUR_X86_64)
    {
        tak_bertanda64_t xcr0;
        tak_bertanda32_t xcr0_low;
        
        /* Cek AVX support via CPUID */
        if (!cpu_fitur_cek_ext(CPU_FEAT_AVX)) {
            g_avx_available = SALAH;
            return STATUS_TIDAK_DUKUNG;
        }
        
        /* Cek OSXSAVE */
        if (!cpu_fitur_cek_ext(CPU_FEAT_OSXSAVE)) {
            g_avx_available = SALAH;
            return STATUS_TIDAK_DUKUNG;
        }
        
        g_avx_available = BENAR;
        
        /* Enable SSE first */
        cpu_sse_init();
        
        /* Configure CR4 for XSAVE - x86_64 uses 64-bit CR4 */
        {
            tak_bertanda64_t cr4;
            __asm__ __volatile__ ("mov %%cr4, %0" : "=r"(cr4));
            cr4 |= (1ULL << 18);  /* OSXSAVE */
            __asm__ __volatile__ ("mov %0, %%cr4" : : "r"(cr4));
        }
        
        /* Enable AVX via XCR0 */
        __asm__ __volatile__ (
            "xgetbv"
            : "=a"(xcr0_low), "=d"(xcr0)
            : "c"(0)
        );
        
        xcr0 |= (1ULL << 1);  /* Enable SSE */
        xcr0 |= (1ULL << 2);  /* Enable AVX */
        
        __asm__ __volatile__ (
            "xsetbv"
            :
            : "a"(xcr0_low), "d"((tak_bertanda32_t)(xcr0 >> 32)), "c"(0)
        );
    }
#endif
    
    g_avx_initialized = BENAR;
    
    return STATUS_BERHASIL;
}

bool_t cpu_avx_available(void)
{
    return g_avx_available;
}

status_t cpu_avx_save(void *buffer)
{
    if (buffer == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (!g_avx_available) {
        return STATUS_TIDAK_DUKUNG;
    }
    
#if defined(ARSITEKTUR_X86_64)
    __asm__ __volatile__ (
        "xsave %0"
        : "=m"(*(char *)buffer)
        : "a"(0xFFFFFFFF), "d"(0xFFFFFFFF)
        : "memory"
    );
#endif
    
    return STATUS_BERHASIL;
}

status_t cpu_avx_restore(void *buffer)
{
    if (buffer == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (!g_avx_available) {
        return STATUS_TIDAK_DUKUNG;
    }
    
#if defined(ARSITEKTUR_X86_64)
    __asm__ __volatile__ (
        "xrstor %0"
        :
        : "m"(*(char *)buffer), "a"(0xFFFFFFFF), "d"(0xFFFFFFFF)
        : "memory"
    );
#endif
    
    return STATUS_BERHASIL;
}
