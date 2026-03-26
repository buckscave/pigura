/*
 * PIGURA OS - CACHE.C
 * =====================
 * Implementasi cache controller.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "cpu.h"

/*
 * ===========================================================================
 * KONSTANTA DAN TIPE
 * ===========================================================================
 */

/* Konstanta cache */
#define CACHE_LEVEL_MAKS        8
#define CACHE_CTRL_MAGIC        0x43414348  /* "CACH" */

/* Tipe cache controller */
typedef struct {
    tak_bertanda32_t magic;
    bool_t enabled;
    tak_bertanda32_t level_count;
    tak_bertanda32_t level[CACHE_LEVEL_MAKS];
    ukuran_t ukuran[CACHE_LEVEL_MAKS];
    tak_bertanda32_t line_size[CACHE_LEVEL_MAKS];
    tak_bertanda32_t ways[CACHE_LEVEL_MAKS];
    tak_bertanda32_t tipe[CACHE_LEVEL_MAKS];
} cache_controller_t;

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

static cache_controller_t g_cache_ctrl;
static bool_t g_cache_initialized = SALAH;

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

status_t cache_init(void)
{
    if (g_cache_initialized) {
        return STATUS_SUDAH_ADA;
    }
    
    /* Manual memset */
    {
        tak_bertanda32_t i;
        for (i = 0; i < sizeof(cache_controller_t); i++) {
            ((char*)&g_cache_ctrl)[i] = 0;
        }
    }
    
    /* Deteksi cache via CPUID untuk x86 */
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    {
        tak_bertanda32_t eax, ebx, ecx, edx;
        tak_bertanda32_t i;
        
        /* Cek max CPUID leaf */
        cpu_cpuid(0, 0, &eax, NULL, NULL, NULL);
        
        if (eax >= 4) {
            /* Deterministic Cache Parameters */
            for (i = 0; i < CACHE_LEVEL_MAKS; i++) {
                tak_bertanda32_t level;
                
                cpu_cpuid(4, i, &eax, &ebx, &ecx, &edx);
                
                level = (eax >> 5) & 0x07;
                if (level == 0) {
                    break;
                }
                
                g_cache_ctrl.level_count = level;
                g_cache_ctrl.level[level - 1] = level;
                g_cache_ctrl.ukuran[level - 1] = 
                    ((ebx >> 22) + 1) *        /* Ways */
                    (((ebx >> 12) & 0x3FF) + 1) * /* Partitions */
                    ((ebx & 0x0FFF) + 1) *      /* Line size */
                    (ecx + 1);                  /* Sets */
                g_cache_ctrl.line_size[level - 1] = (ebx & 0x0FFF) + 1;
                g_cache_ctrl.ways[level - 1] = ((ebx >> 22) & 0x3FF) + 1;
                g_cache_ctrl.tipe[level - 1] = eax & 0x1F;
            }
        }
    }
#endif
    
    g_cache_ctrl.magic = CACHE_CTRL_MAGIC;
    g_cache_ctrl.enabled = BENAR;
    g_cache_initialized = BENAR;
    
    return STATUS_BERHASIL;
}

status_t cache_enable(tak_bertanda32_t level)
{
    if (!g_cache_initialized) {
        return STATUS_GAGAL;
    }
    
    if (level < 1 || level > CACHE_LEVEL_MAKS) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Enable cache via CR0 */
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    {
#if defined(__x86_64__)
        tak_bertanda64_t cr0;
        __asm__ __volatile__ ("mov %%cr0, %0" : "=r"(cr0));
        cr0 &= ~(1ULL << 30);  /* Clear CD */
        cr0 &= ~(1ULL << 29);  /* Clear NW */
        __asm__ __volatile__ ("mov %0, %%cr0" : : "r"(cr0));
#else
        tak_bertanda32_t cr0;
        __asm__ __volatile__ ("mov %%cr0, %0" : "=r"(cr0));
        cr0 &= ~(1U << 30);  /* Clear CD */
        cr0 &= ~(1U << 29);  /* Clear NW */
        __asm__ __volatile__ ("mov %0, %%cr0" : : "r"(cr0));
#endif
    }
#endif
    
    return STATUS_BERHASIL;
}

status_t cache_disable(tak_bertanda32_t level)
{
    if (!g_cache_initialized) {
        return STATUS_GAGAL;
    }
    
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    {
#if defined(__x86_64__)
        tak_bertanda64_t cr0;
        __asm__ __volatile__ ("mov %%cr0, %0" : "=r"(cr0));
        cr0 |= (1ULL << 30);  /* Set CD */
        __asm__ __volatile__ ("mov %0, %%cr0" : : "r"(cr0));
#else
        tak_bertanda32_t cr0;
        __asm__ __volatile__ ("mov %%cr0, %0" : "=r"(cr0));
        cr0 |= (1U << 30);  /* Set CD */
        __asm__ __volatile__ ("mov %0, %%cr0" : : "r"(cr0));
#endif
    }
#endif
    
    return STATUS_BERHASIL;
}

status_t cache_flush(tak_bertanda32_t level)
{
    if (!g_cache_initialized) {
        return STATUS_GAGAL;
    }
    
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    /* WBINVD instruction */
    __asm__ __volatile__ ("wbinvd");
#endif
    
    return STATUS_BERHASIL;
}

status_t cache_invalidate(tak_bertanda32_t level)
{
    if (!g_cache_initialized) {
        return STATUS_GAGAL;
    }
    
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    /* INVD instruction */
    __asm__ __volatile__ ("invd");
#endif
    
    return STATUS_BERHASIL;
}

status_t cache_info(tak_bertanda32_t level, cache_controller_t *ctrl)
{
    if (ctrl == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (!g_cache_initialized) {
        return STATUS_GAGAL;
    }
    
    /* Manual memcpy */
    {
        tak_bertanda32_t i;
        for (i = 0; i < sizeof(cache_controller_t); i++) {
            ((char*)ctrl)[i] = ((char*)&g_cache_ctrl)[i];
        }
    }
    
    return STATUS_BERHASIL;
}
