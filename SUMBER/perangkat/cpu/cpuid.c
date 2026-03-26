/*
 * PIGURA OS - CPUID.C
 * ====================
 * Implementasi fungsi CPUID untuk x86/x86_64.
 *
 * Berkas ini mengimplementasikan fungsi-fungsi untuk membaca
 * informasi CPU melalui instruksi CPUID.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "cpu.h"

/*
 * ===========================================================================
 * DETEKSI ARSITEKTUR
 * ===========================================================================
 */

#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

/* Cache untuk hasil CPUID */
static tak_bertanda32_t g_cpuid_max_basic = 0;
static tak_bertanda32_t g_cpuid_max_ext = 0;
static bool_t g_cpuid_initialized = SALAH;

/*
 * ===========================================================================
 * FUNGSI INTERNAL
 * ===========================================================================
 */

/*
 * cpuid_execute - Eksekusi instruksi CPUID
 */
static void cpuid_execute(tak_bertanda32_t leaf,
                           tak_bertanda32_t subleaf,
                           tak_bertanda32_t *eax,
                           tak_bertanda32_t *ebx,
                           tak_bertanda32_t *ecx,
                           tak_bertanda32_t *edx)
{
#if defined(__x86_64__)
    __asm__ __volatile__ (
        "cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(leaf), "c"(subleaf)
        : "memory"
    );
#else
    __asm__ __volatile__ (
        "pushl %%ebx\n"
        "cpuid\n"
        "movl %%ebx, %1\n"
        "popl %%ebx"
        : "=a"(*eax), "=r"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(leaf), "c"(subleaf)
        : "memory"
    );
#endif
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

/*
 * cpuid_init - Inisialisasi cache CPUID
 */
status_t cpuid_init(void)
{
    tak_bertanda32_t eax, ebx, ecx, edx;
    
    if (g_cpuid_initialized) {
        return STATUS_SUDAH_ADA;
    }
    
    /* Dapatkan max basic leaf */
    cpuid_execute(0, 0, &g_cpuid_max_basic, &ebx, &ecx, &edx);
    
    /* Dapatkan max extended leaf */
    cpuid_execute(0x80000000, 0, &g_cpuid_max_ext, &ebx, &ecx, &edx);
    
    g_cpuid_initialized = BENAR;
    
    return STATUS_BERHASIL;
}

/*
 * cpuid_get_vendor - Dapatkan vendor string
 */
status_t cpuid_get_vendor(char *vendor, ukuran_t len)
{
    tak_bertanda32_t eax, ebx, ecx, edx;
    
    if (vendor == NULL || len < 13) {
        return STATUS_PARAM_INVALID;
    }
    
    cpuid_execute(0, 0, &eax, &ebx, &ecx, &edx);
    
    /* Susun vendor string */
    vendor[0] = (char)(ebx & 0xFF);
    vendor[1] = (char)((ebx >> 8) & 0xFF);
    vendor[2] = (char)((ebx >> 16) & 0xFF);
    vendor[3] = (char)((ebx >> 24) & 0xFF);
    vendor[4] = (char)(edx & 0xFF);
    vendor[5] = (char)((edx >> 8) & 0xFF);
    vendor[6] = (char)((edx >> 16) & 0xFF);
    vendor[7] = (char)((edx >> 24) & 0xFF);
    vendor[8] = (char)(ecx & 0xFF);
    vendor[9] = (char)((ecx >> 8) & 0xFF);
    vendor[10] = (char)((ecx >> 16) & 0xFF);
    vendor[11] = (char)((ecx >> 24) & 0xFF);
    vendor[12] = '\0';
    
    return STATUS_BERHASIL;
}

/*
 * cpuid_get_processor_name - Dapatkan nama prosesor
 */
status_t cpuid_get_processor_name(char *name, ukuran_t len)
{
    tak_bertanda32_t eax, ebx, ecx, edx;
    tak_bertanda32_t i;
    char *p;
    
    if (name == NULL || len < 49) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Cek apakah extended leaf tersedia */
    if (g_cpuid_max_ext < 0x80000004) {
        name[0] = '\0';
        return STATUS_TIDAK_DUKUNG;
    }
    
    p = name;
    
    /* Leaf 0x80000002 */
    cpuid_execute(0x80000002, 0, &eax, &ebx, &ecx, &edx);
    *((tak_bertanda32_t *)p) = eax;
    *((tak_bertanda32_t *)(p + 4)) = ebx;
    *((tak_bertanda32_t *)(p + 8)) = ecx;
    *((tak_bertanda32_t *)(p + 12)) = edx;
    p += 16;
    
    /* Leaf 0x80000003 */
    cpuid_execute(0x80000003, 0, &eax, &ebx, &ecx, &edx);
    *((tak_bertanda32_t *)p) = eax;
    *((tak_bertanda32_t *)(p + 4)) = ebx;
    *((tak_bertanda32_t *)(p + 8)) = ecx;
    *((tak_bertanda32_t *)(p + 12)) = edx;
    p += 16;
    
    /* Leaf 0x80000004 */
    cpuid_execute(0x80000004, 0, &eax, &ebx, &ecx, &edx);
    *((tak_bertanda32_t *)p) = eax;
    *((tak_bertanda32_t *)(p + 4)) = ebx;
    *((tak_bertanda32_t *)(p + 8)) = ecx;
    *((tak_bertanda32_t *)(p + 12)) = edx;
    p += 16;
    
    *p = '\0';
    
    /* Trim leading spaces */
    p = name;
    while (*p == ' ') {
        p++;
    }
    
    if (p != name) {
        i = 0;
        while (p[i] != '\0') {
            name[i] = p[i];
            i++;
        }
        name[i] = '\0';
    }
    
    return STATUS_BERHASIL;
}

/*
 * cpuid_get_features - Dapatkan feature flags
 */
status_t cpuid_get_features(tak_bertanda32_t *edx_features,
                             tak_bertanda32_t *ecx_features)
{
    tak_bertanda32_t eax, ebx, ecx, edx;
    
    if (!g_cpuid_initialized) {
        cpuid_init();
    }
    
    if (g_cpuid_max_basic < 1) {
        return STATUS_TIDAK_DUKUNG;
    }
    
    cpuid_execute(1, 0, &eax, &ebx, &ecx, &edx);
    
    if (edx_features != NULL) {
        *edx_features = edx;
    }
    if (ecx_features != NULL) {
        *ecx_features = ecx;
    }
    
    return STATUS_BERHASIL;
}

/*
 * cpuid_get_extended_features - Dapatkan extended features
 */
status_t cpuid_get_extended_features(tak_bertanda32_t *edx_ext,
                                      tak_bertanda32_t *ecx_ext)
{
    tak_bertanda32_t eax, ebx, ecx, edx;
    
    if (!g_cpuid_initialized) {
        cpuid_init();
    }
    
    if (g_cpuid_max_ext < 0x80000001) {
        return STATUS_TIDAK_DUKUNG;
    }
    
    cpuid_execute(0x80000001, 0, &eax, &ebx, &ecx, &edx);
    
    if (edx_ext != NULL) {
        *edx_ext = edx;
    }
    if (ecx_ext != NULL) {
        *ecx_ext = ecx;
    }
    
    return STATUS_BERHASIL;
}

/*
 * cpuid_get_signature - Dapatkan CPU signature
 */
status_t cpuid_get_signature(tak_bertanda32_t *stepping,
                              tak_bertanda32_t *model,
                              tak_bertanda32_t *family)
{
    tak_bertanda32_t eax, ebx, ecx, edx;
    tak_bertanda32_t sig;
    
    if (!g_cpuid_initialized) {
        cpuid_init();
    }
    
    if (g_cpuid_max_basic < 1) {
        return STATUS_TIDAK_DUKUNG;
    }
    
    cpuid_execute(1, 0, &eax, &ebx, &ecx, &edx);
    sig = eax;
    
    if (stepping != NULL) {
        *stepping = sig & 0x0F;
    }
    
    if (model != NULL) {
        *model = (sig >> 4) & 0x0F;
        if (*family == 0x0F || *family == 6) {
            *model |= ((sig >> 16) & 0x0F) << 4;
        }
    }
    
    if (family != NULL) {
        *family = (sig >> 8) & 0x0F;
        if (*family == 0x0F) {
            *family += (sig >> 20) & 0xFF;
        }
    }
    
    return STATUS_BERHASIL;
}

/*
 * cpuid_get_cache_info - Dapatkan informasi cache
 */
status_t cpuid_get_cache_info(tak_bertanda32_t level,
                               ukuran_t *size,
                               tak_bertanda32_t *line_size,
                               tak_bertanda32_t *ways)
{
    tak_bertanda32_t eax, ebx, ecx, edx;
    tak_bertanda32_t i = 0;
    tak_bertanda32_t cache_level;
    
    if (!g_cpuid_initialized) {
        cpuid_init();
    }
    
    if (g_cpuid_max_basic < 4) {
        return STATUS_TIDAK_DUKUNG;
    }
    
    /* Iterasi melalui cache parameters */
    do {
        cpuid_execute(4, i, &eax, &ebx, &ecx, &edx);
        
        cache_level = (eax >> 5) & 0x07;
        
        if (cache_level == level) {
            if (size != NULL) {
                tak_bertanda32_t ways_num = ((ebx >> 22) & 0x3FF) + 1;
                tak_bertanda32_t partitions = ((ebx >> 12) & 0x3FF) + 1;
                tak_bertanda32_t line_len = (ebx & 0x0FFF) + 1;
                tak_bertanda32_t sets = ecx + 1;
                
                *size = ways_num * partitions * line_len * sets;
            }
            
            if (line_size != NULL) {
                *line_size = (ebx & 0x0FFF) + 1;
            }
            
            if (ways != NULL) {
                *ways = ((ebx >> 22) & 0x3FF) + 1;
            }
            
            return STATUS_BERHASIL;
        }
        
        i++;
    } while (cache_level != 0);
    
    return STATUS_TIDAK_DITEMUKAN;
}

/*
 * cpuid_get_tlb_info - Dapatkan informasi TLB
 */
status_t cpuid_get_tlb_info(tak_bertanda32_t *entries_4k,
                             tak_bertanda32_t *entries_2m)
{
    tak_bertanda32_t eax, ebx, ecx, edx;
    
    if (!g_cpuid_initialized) {
        cpuid_init();
    }
    
    if (g_cpuid_max_basic < 2) {
        return STATUS_TIDAK_DUKUNG;
    }
    
    /* Leaf 2 berisi TLB info dalam format Intel */
    cpuid_execute(2, 0, &eax, &ebx, &ecx, &edx);
    
    /* Parsing TLB info sangat kompleks dan berbeda per vendor */
    /* Untuk sekarang, gunakan nilai default */
    if (entries_4k != NULL) {
        *entries_4k = 64;  /* Default */
    }
    if (entries_2m != NULL) {
        *entries_2m = 32;  /* Default */
    }
    
    return STATUS_BERHASIL;
}

/*
 * cpuid_get_apic_id - Dapatkan APIC ID
 */
tak_bertanda32_t cpuid_get_apic_id(void)
{
    tak_bertanda32_t eax, ebx, ecx, edx;
    
    if (!g_cpuid_initialized) {
        cpuid_init();
    }
    
    if (g_cpuid_max_basic < 1) {
        return 0;
    }
    
    cpuid_execute(1, 0, &eax, &ebx, &ecx, &edx);
    
    return (ebx >> 24) & 0xFF;
}

/*
 * cpuid_has_feature - Cek apakah fitur tersedia
 */
bool_t cpuid_has_feature(tak_bertanda32_t feature_bit)
{
    tak_bertanda32_t edx, ecx;
    
    if (cpuid_get_features(&edx, &ecx) != STATUS_BERHASIL) {
        return SALAH;
    }
    
    /* Cek di EDX (bit 0-31) atau ECX (bit 32-63) */
    if (feature_bit < 32) {
        return (edx & (1UL << feature_bit)) ? BENAR : SALAH;
    } else {
        return (ecx & (1UL << (feature_bit - 32))) ? BENAR : SALAH;
    }
}

/*
 * cpuid_has_extended_feature - Cek extended feature
 */
bool_t cpuid_has_extended_feature(tak_bertanda32_t feature_bit)
{
    tak_bertanda32_t edx, ecx;
    
    if (cpuid_get_extended_features(&edx, &ecx) != STATUS_BERHASIL) {
        return SALAH;
    }
    
    if (feature_bit < 32) {
        return (edx & (1UL << feature_bit)) ? BENAR : SALAH;
    } else {
        return (ecx & (1UL << (feature_bit - 32))) ? BENAR : SALAH;
    }
}

/*
 * cpuid_get_max_leafs - Dapatkan max leaf values
 */
void cpuid_get_max_leafs(tak_bertanda32_t *max_basic,
                          tak_bertanda32_t *max_ext)
{
    if (!g_cpuid_initialized) {
        cpuid_init();
    }
    
    if (max_basic != NULL) {
        *max_basic = g_cpuid_max_basic;
    }
    if (max_ext != NULL) {
        *max_ext = g_cpuid_max_ext;
    }
}

/*
 * cpuid_check_vendor - Cek vendor tertentu
 */
bool_t cpuid_check_vendor(const char *vendor_str)
{
    char vendor[13];
    ukuran_t i;
    
    if (cpuid_get_vendor(vendor, sizeof(vendor)) != STATUS_BERHASIL) {
        return SALAH;
    }
    
    /* Bandingkan string */
    i = 0;
    while (vendor_str[i] != '\0' && vendor[i] != '\0') {
        if (vendor_str[i] != vendor[i]) {
            return SALAH;
        }
        i++;
    }
    
    return vendor_str[i] == '\0' && vendor[i] == '\0';
}

#else /* !x86 && !x86_64 */

/* Stub untuk arsitektur non-x86 */

status_t cpuid_init(void)
{
    return STATUS_TIDAK_DUKUNG;
}

status_t cpuid_get_vendor(char *vendor, ukuran_t len)
{
    return STATUS_TIDAK_DUKUNG;
}

status_t cpuid_get_processor_name(char *name, ukuran_t len)
{
    return STATUS_TIDAK_DUKUNG;
}

status_t cpuid_get_features(tak_bertanda32_t *edx,
                             tak_bertanda32_t *ecx)
{
    return STATUS_TIDAK_DUKUNG;
}

status_t cpuid_get_extended_features(tak_bertanda32_t *edx,
                                      tak_bertanda32_t *ecx)
{
    return STATUS_TIDAK_DUKUNG;
}

status_t cpuid_get_signature(tak_bertanda32_t *stepping,
                              tak_bertanda32_t *model,
                              tak_bertanda32_t *family)
{
    return STATUS_TIDAK_DUKUNG;
}

status_t cpuid_get_cache_info(tak_bertanda32_t level,
                               ukuran_t *size,
                               tak_bertanda32_t *line_size,
                               tak_bertanda32_t *ways)
{
    return STATUS_TIDAK_DUKUNG;
}

tak_bertanda32_t cpuid_get_apic_id(void)
{
    return 0;
}

bool_t cpuid_has_feature(tak_bertanda32_t feature_bit)
{
    return SALAH;
}

bool_t cpuid_has_extended_feature(tak_bertanda32_t feature_bit)
{
    return SALAH;
}

void cpuid_get_max_leafs(tak_bertanda32_t *max_basic,
                          tak_bertanda32_t *max_ext)
{
    if (max_basic != NULL) {
        *max_basic = 0;
    }
    if (max_ext != NULL) {
        *max_ext = 0;
    }
}

bool_t cpuid_check_vendor(const char *vendor_str)
{
    return SALAH;
}

#endif /* ARSITEKTUR_X86 || ARSITEKTUR_X86_64 */
