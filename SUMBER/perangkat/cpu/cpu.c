/*
 * PIGURA OS - CPU.C
 * =================
 * Implementasi manajemen CPU dan fitur-fitur prosesor.
 *
 * Berkas ini mengimplementasikan fungsi-fungsi utama untuk deteksi
 * CPU, manajemen cache, dan dukungan multi-core.
 *
 * Kepatuhan standar:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Mendukung semua arsitektur: x86, x86_64, ARM, ARMv7, ARM64
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "cpu.h"
#include "../../inti/hal/hal.h"

/*
 * ===========================================================================
 * VARIABEL GLOBAL
 * ===========================================================================
 */

cpu_konteks_t g_cpu_konteks;
bool_t g_cpu_diinisialisasi = SALAH;

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

/* Buffer untuk pesan error */
static char g_cpu_error_msg[256];

/* Flag untuk fitur yang sudah dideteksi */
static bool_t g_fitur_dideteksi __attribute__((unused)) = SALAH;

/*
 * ===========================================================================
 * FUNGSI INTERNAL
 * ===========================================================================
 */

/*
 * cpu_set_error - Set pesan error
 */
static void cpu_set_error(const char *pesan)
{
    ukuran_t i;
    
    if (pesan == NULL) {
        return;
    }
    
    i = 0;
    while (pesan[i] != '\0' && i < 255) {
        g_cpu_error_msg[i] = pesan[i];
        i++;
    }
    g_cpu_error_msg[i] = '\0';
}

/*
 * cpu_validasi_init - Validasi apakah sudah diinisialisasi
 */
static bool_t cpu_validasi_init(void)
{
    if (!g_cpu_diinisialisasi) {
        cpu_set_error("CPU belum diinisialisasi");
        return SALAH;
    }
    
    if (g_cpu_konteks.magic != CPU_KONTEKS_MAGIC) {
        cpu_set_error("Konteks CPU tidak valid");
        return SALAH;
    }
    
    return BENAR;
}

/*
 * cpu_inisialisasi_info - Inisialisasi struktur info CPU
 */
static void cpu_inisialisasi_info(cpu_info_t *info)
{
    tak_bertanda32_t i;
    
    if (info == NULL) {
        return;
    }
    
    /* Reset semua field */
    for (i = 0; i < CPU_NAMA_PANJANG_MAKS; i++) {
        info->nama[i] = '\0';
    }
    for (i = 0; i < CPU_VENDOR_PANJANG_MAKS; i++) {
        info->vendor_str[i] = '\0';
    }
    for (i = 0; i < CPU_SERI_PANJANG_MAKS; i++) {
        info->seri[i] = '\0';
    }
    for (i = 0; i < CPU_FEAT_PANJANG_MAKS; i++) {
        info->fitur_str[i] = '\0';
    }
    
    info->vendor = CPU_VENDOR_TIDAK_DIKETAHUI;
    info->arsitektur = CPU_ARSITEKTUR_TIDAK_DIKETAHUI;
    info->family = 0;
    info->model = 0;
    info->stepping = 0;
    info->extended_family = 0;
    info->extended_model = 0;
    info->fitur = 0;
    info->fitur_ext = 0;
    info->fitur_arm = 0;
    info->frekuensi_mhz = 0;
    info->frekuensi_max_mhz = 0;
    info->frekuensi_min_mhz = 0;
    info->fsb_mhz = 0;
    info->jumlah_cache = 0;
    info->cache_l1d = 0;
    info->cache_l1i = 0;
    info->cache_l2 = 0;
    info->cache_l3 = 0;
    info->jumlah_core = 1;
    info->jumlah_thread = 1;
    info->jumlah_socket = 1;
    info->jumlah_numa = 1;
    info->tlb_data_4k = 0;
    info->tlb_data_2m = 0;
    info->tlb_data_4m = 0;
    info->tlb_instr_4k = 0;
    info->tlb_instr_2m = 0;
    info->status = CPU_STATUS_TIDAK_ADA;
    info->diinisialisasi = SALAH;
    info->mendukung_64bit = SALAH;
    info->mendukung_smp = SALAH;
    info->mendukung_virtualisasi = SALAH;
    
    /* Reset cache */
    for (i = 0; i < CPU_MAKS_CACHE; i++) {
        info->cache[i].tipe = CPU_CACHE_TIPE_TIDAK_ADA;
        info->cache[i].level = 0;
        info->cache[i].ukuran = 0;
        info->cache[i].line_size = 0;
        info->cache[i].ways = 0;
        info->cache[i].sets = 0;
        info->cache[i].inclusive = SALAH;
        info->cache[i].write_back = SALAH;
    }
    
    /* Reset core */
    for (i = 0; i < CPU_MAKS_CORE; i++) {
        info->core[i].magic = 0;
        info->core[i].id = 0;
        info->core[i].apic_id = 0;
        info->core[i].socket_id = 0;
        info->core[i].die_id = 0;
        info->core[i].core_id = 0;
        info->core[i].thread_id = 0;
        info->core[i].numa_node = 0;
        info->core[i].status = CPU_STATUS_TIDAK_ADA;
        info->core[i].frekuensi_mhz = 0;
        info->core[i].fitur = 0;
        info->core[i].fitur_ext = 0;
        info->core[i].jumlah_cache = 0;
    }
}

/*
 * ===========================================================================
 * FUNGSI DETEKSI ARSITEKTUR
 * ===========================================================================
 */

/*
 * cpu_deteksi_arsitektur - Deteksi arsitektur saat ini
 */
static cpu_arsitektur_t cpu_deteksi_arsitektur(void)
{
#if defined(__x86_64__) || defined(_M_X64)
    return CPU_ARSITEKTUR_X86_64;
#elif defined(__i386__) || defined(_M_IX86)
    return CPU_ARSITEKTUR_X86;
#elif defined(__aarch64__)
    return CPU_ARSITEKTUR_ARMV8A;
#elif defined(__arm__)
    /* Deteksi ARM version */
    #if defined(__ARM_ARCH_7A__)
        return CPU_ARSITEKTUR_ARMV7A;
    #elif defined(__ARM_ARCH_7R__)
        return CPU_ARSITEKTUR_ARMV7R;
    #elif defined(__ARM_ARCH_7M__)
        return CPU_ARSITEKTUR_ARMV7M;
    #elif defined(__ARM_ARCH_6__)
        return CPU_ARSITEKTUR_ARMV6;
    #else
        return CPU_ARSITEKTUR_ARMV7;
    #endif
#else
    return CPU_ARSITEKTUR_TIDAK_DIKETAHUI;
#endif
}

/*
 * ===========================================================================
 * FUNGSI DETEKSI x86/x86_64
 * ===========================================================================
 */

#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)

/*
 * cpu_cpuid_x86 - Eksekusi CPUID (x86/x86_64)
 */
static void cpu_cpuid_x86(tak_bertanda32_t leaf, tak_bertanda32_t subleaf,
                          tak_bertanda32_t *eax, tak_bertanda32_t *ebx,
                          tak_bertanda32_t *ecx, tak_bertanda32_t *edx)
{
    tak_bertanda32_t a, b, c, d;
    
    /* Inline assembly untuk CPUID */
#if defined(__x86_64__)
    __asm__ __volatile__ (
        "cpuid"
        : "=a"(a), "=b"(b), "=c"(c), "=d"(d)
        : "a"(leaf), "c"(subleaf)
        : "memory"
    );
#else
    __asm__ __volatile__ (
        "cpuid"
        : "=a"(a), "=b"(b), "=c"(c), "=d"(d)
        : "a"(leaf), "c"(subleaf)
        : "memory"
    );
#endif
    
    if (eax != NULL) {
        *eax = a;
    }
    if (ebx != NULL) {
        *ebx = b;
    }
    if (ecx != NULL) {
        *ecx = c;
    }
    if (edx != NULL) {
        *edx = d;
    }
}

/*
 * cpu_deteksi_vendor_x86 - Deteksi vendor CPU (x86)
 */
static cpu_vendor_t cpu_deteksi_vendor_x86(tak_bertanda32_t ebx,
                                            tak_bertanda32_t ecx,
                                            tak_bertanda32_t edx)
{
    char vendor[13];
    tak_bertanda32_t *p;
    
    /* Ekstrak vendor string dari register */
    p = (tak_bertanda32_t *)vendor;
    *p++ = ebx;
    *p++ = edx;
    *p++ = ecx;
    vendor[12] = '\0';
    
    /* Identifikasi vendor */
    if (vendor[0] == 'G' && vendor[1] == 'e') {
        /* GenuineIntel */
        return CPU_VENDOR_INTEL;
    } else if (vendor[0] == 'A' && vendor[1] == 'u') {
        /* AuthenticAMD */
        return CPU_VENDOR_AMD;
    } else if (vendor[0] == 'V' && vendor[1] == 'I') {
        /* VIA */
        return CPU_VENDOR_VIA;
    } else if (vendor[0] == 'C' && vendor[1] == 'e') {
        /* CentaurHauls */
        return CPU_VENDOR_CENTAUR;
    } else if (vendor[0] == 'T' && vendor[1] == 'r') {
        /* Transmeta */
        return CPU_VENDOR_TRANSMETA;
    } else if (vendor[0] == 'C' && vendor[1] == 'y') {
        /* CyrixInstead */
        return CPU_VENDOR_CYRIX;
    }
    
    return CPU_VENDOR_TIDAK_DIKETAHUI;
}

/*
 * cpu_deteksi_fitur_x86 - Deteksi fitur CPU (x86)
 */
static void cpu_deteksi_fitur_x86(cpu_info_t *info)
{
    tak_bertanda32_t eax, ebx, ecx, edx;
    tak_bertanda32_t max_leaf;
    
    if (info == NULL) {
        return;
    }
    
    /* Leaf 0x01 - Feature flags */
    cpu_cpuid_x86(1, 0, &eax, &ebx, &ecx, &edx);
    
    /* EDX features */
    info->fitur = edx;
    
    /* ECX features (extended) */
    info->fitur_ext = ecx;
    info->fitur_ext <<= 32;
    
    /* Family, Model, Stepping */
    info->stepping = eax & 0x0F;
    info->model = (eax >> 4) & 0x0F;
    info->family = (eax >> 8) & 0x0F;
    
    /* Extended family dan model */
    if (info->family == 0x0F) {
        info->extended_family = (eax >> 20) & 0xFF;
        info->extended_model = (eax >> 16) & 0x0F;
        info->family += info->extended_family;
        info->model += (info->extended_model << 4);
    }
    
    /* Deteksi 64-bit support */
    /* Cek extended CPUID */
    cpu_cpuid_x86(0x80000000, 0, &max_leaf, NULL, NULL, NULL);
    
    if (max_leaf >= 0x80000001) {
        tak_bertanda32_t ext_edx;
        cpu_cpuid_x86(0x80000001, 0, NULL, NULL, NULL, &ext_edx);
        
        /* Long mode (64-bit) support */
        if (ext_edx & (1 << 29)) {
            info->mendukung_64bit = BENAR;
        }
    }
    
    /* Cache info dari leaf 0x02 atau 0x04 */
    if (max_leaf >= 4) {
        tak_bertanda32_t cache_level = 0;
        tak_bertanda32_t i = 0;
        
        do {
            cpu_cpuid_x86(4, i, &eax, &ebx, &ecx, NULL);
            cache_level = (eax >> 5) & 0x07;
            
            if (cache_level != 0 && i < CPU_MAKS_CACHE) {
                info->cache[i].tipe = eax & 0x1F;
                info->cache[i].level = cache_level;
                info->cache[i].ways = ((ebx >> 22) & 0x3FF) + 1;
                info->cache[i].sets = ecx + 1;
                info->cache[i].line_size = (ebx & 0x0FFF) + 1;
                info->cache[i].ukuran = info->cache[i].ways *
                                        ((ebx >> 12) & 0x3FF) *
                                        info->cache[i].line_size *
                                        info->cache[i].sets;
                
                /* Set cache total */
                if (cache_level == 1) {
                    if (info->cache[i].tipe == 1) {
                        info->cache_l1d = info->cache[i].ukuran;
                    } else if (info->cache[i].tipe == 2) {
                        info->cache_l1i = info->cache[i].ukuran;
                    }
                } else if (cache_level == 2) {
                    info->cache_l2 = info->cache[i].ukuran;
                } else if (cache_level == 3) {
                    info->cache_l3 = info->cache[i].ukuran;
                }
                
                i++;
            }
            i++;
        } while (cache_level != 0 && i < CPU_MAKS_CACHE);
        
        info->jumlah_cache = i;
    }
}

#endif /* ARSITEKTUR_X86 || ARSITEKTUR_X86_64 */

/*
 * ===========================================================================
 * FUNGSI DETEKSI ARM
 * ===========================================================================
 */

#if defined(__arm__) || defined(__aarch64__)

/*
 * cpu_deteksi_arm - Deteksi CPU ARM
 */
static void cpu_deteksi_arm(cpu_info_t *info)
{
    if (info == NULL) {
        return;
    }
    
    info->vendor = CPU_VENDOR_ARM;
    info->arsitektur = cpu_deteksi_arsitektur();
    
    /* Baca Main ID Register (MIDR) */
#if defined(__aarch64__)
    tak_bertanda64_t midr;
    __asm__ __volatile__ (
        "mrs %0, midr_el1"
        : "=r"(midr)
    );
    
    info->implementer = (midr >> 24) & 0xFF;
    info->variant = (midr >> 20) & 0x0F;
    info->architecture = (midr >> 16) & 0x0F;
    info->partnum = (midr >> 4) & 0xFFF;
    info->revision = midr & 0x0F;
#else
    tak_bertanda32_t midr;
    __asm__ __volatile__ (
        "mrc p15, 0, %0, c0, c0, 0"
        : "=r"(midr)
    );
    
    info->implementer = (midr >> 24) & 0xFF;
    info->variant = (midr >> 20) & 0x0F;
    info->architecture = (midr >> 16) & 0x0F;
    info->partnum = (midr >> 4) & 0xFFF;
    info->revision = midr & 0x0F;
#endif
    
    /* Identifikasi vendor berdasarkan implementer */
    if (info->implementer == 0x41) {
        info->vendor = CPU_VENDOR_ARM;
    } else if (info->implementer == 0x51) {
        info->vendor = CPU_VENDOR_QUALCOMM;
    } else if (info->implementer == 0x53) {
        info->vendor = CPU_VENDOR_SAMSUNG;
    } else if (info->implementer == 0x41) {
        info->vendor = CPU_VENDOR_APPLE;
    } else if (info->implementer == 0x4E) {
        info->vendor = CPU_VENDOR_NVIDIA;
    } else if (info->implementer == 0x56) {
        info->vendor = CPU_VENDOR_MARVELL;
    }
    
    /* Deteksi fitur ARM */
#if defined(__aarch64__)
    tak_bertanda64_t id_aa64pfr0;
    __asm__ __volatile__ (
        "mrs %0, id_aa64pfr0_el1"
        : "=r"(id_aa64pfr0)
    );
    
    /* SIMD/FP support */
    if (((id_aa64pfr0 >> 16) & 0x0F) != 0x0F) {
        info->fitur_arm |= CPU_ARM_FEAT_VFP;
        info->fitur_arm |= CPU_ARM_FEAT_NEON;
    }
    
    /* SVE support */
    if (((id_aa64pfr0 >> 32) & 0x0F) != 0x0F) {
        info->fitur_arm |= CPU_ARM_FEAT_SVE;
    }
#else
    /* ARMv7 feature detection */
    tak_bertanda32_t fpsid;
    __asm__ __volatile__ (
        "mrc p15, 0, %0, c0, c1, 0"
        : "=r"(fpsid)
    );
    
    if (fpsid & (1 << 0)) {
        info->fitur_arm |= CPU_ARM_FEAT_VFP;
    }
#endif
    
    info->mendukung_64bit = BENAR;
}

#endif /* __arm__ || __aarch64__ */

/*
 * ===========================================================================
 * FUNGSI FREKUENSI
 * ===========================================================================
 */

/*
 * cpu_hitung_frekuensi - Hitung frekuensi CPU
 */
static tak_bertanda32_t cpu_hitung_frekuensi(void)
{
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    tak_bertanda64_t tsc1, tsc2;
    tak_bertanda64_t freq;
    
    /* Baca TSC */
    tsc1 = cpu_tsc_baca();
    
    /* Delay sekitar 1ms */
    cpu_delay_ms(1);
    
    tsc2 = cpu_tsc_baca();
    
    /* Hitung frekuensi */
    if (tsc2 > tsc1) {
        freq = (tsc2 - tsc1) * 1000;
        return (tak_bertanda32_t)(freq / 1000000);  /* MHz */
    }
#else
    /* ARM: baca dari timer */
    /* TODO: Implementasi untuk ARM */
#endif
    
    return 0;
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

status_t cpu_init(void)
{
    cpu_info_t *info;
    
    if (g_cpu_diinisialisasi) {
        return STATUS_SUDAH_ADA;
    }
    
    /* Inisialisasi konteks */
    g_cpu_konteks.magic = 0;
    g_cpu_konteks.total_cycles = 0;
    g_cpu_konteks.idle_cycles = 0;
    g_cpu_konteks.jiffies = 0;
    g_cpu_konteks.diinisialisasi = SALAH;
    g_cpu_konteks.smp_aktif = SALAH;
    g_cpu_konteks.apic_aktif = SALAH;
    
    /* Inisialisasi info */
    info = &g_cpu_konteks.info;
    cpu_inisialisasi_info(info);
    
    /* Deteksi arsitektur */
    info->arsitektur = cpu_deteksi_arsitektur();
    
    /* Deteksi CPU berdasarkan arsitektur */
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    {
        tak_bertanda32_t eax, ebx, ecx, edx;
        tak_bertanda32_t max_leaf;
        char *p;
        tak_bertanda32_t i;
        
        /* Dapatkan max CPUID leaf */
        cpu_cpuid_x86(0, 0, &max_leaf, &ebx, &ecx, &edx);
        
        /* Dapatkan vendor string */
        p = info->vendor_str;
        *((tak_bertanda32_t *)p) = ebx;
        *((tak_bertanda32_t *)(p + 4)) = edx;
        *((tak_bertanda32_t *)(p + 8)) = ecx;
        info->vendor_str[12] = '\0';
        
        /* Deteksi vendor */
        info->vendor = cpu_deteksi_vendor_x86(ebx, ecx, edx);
        
        /* Deteksi fitur */
        cpu_deteksi_fitur_x86(info);
        
        /* Dapatkan nama CPU */
        if (max_leaf >= 0x80000004) {
            p = info->nama;
            for (i = 0; i < 3; i++) {
                cpu_cpuid_x86(0x80000002 + i, 0, &eax, &ebx, &ecx, &edx);
                *((tak_bertanda32_t *)(p + i * 16)) = eax;
                *((tak_bertanda32_t *)(p + i * 16 + 4)) = ebx;
                *((tak_bertanda32_t *)(p + i * 16 + 8)) = ecx;
                *((tak_bertanda32_t *)(p + i * 16 + 12)) = edx;
            }
            info->nama[48] = '\0';
        }
    }
#elif defined(__arm__) || defined(__aarch64__)
    cpu_deteksi_arm(info);
#endif
    
    /* Hitung frekuensi */
    info->frekuensi_mhz = cpu_hitung_frekuensi();
    
    /* Set status */
    info->status = CPU_STATUS_TERDETEKSI;
    info->diinisialisasi = BENAR;
    
    /* Finalisasi konteks */
    g_cpu_konteks.magic = CPU_KONTEKS_MAGIC;
    g_cpu_diinisialisasi = BENAR;
    
    /* Inisialisasi subsistem lain */
    cpu_fpu_init();
    
    return STATUS_BERHASIL;
}

status_t cpu_shutdown(void)
{
    if (!g_cpu_diinisialisasi) {
        return STATUS_BERHASIL;
    }
    
    /* Reset konteks */
    g_cpu_konteks.magic = 0;
    g_cpu_konteks.diinisialisasi = SALAH;
    
    g_cpu_diinisialisasi = SALAH;
    
    return STATUS_BERHASIL;
}

status_t cpu_deteksi(void)
{
    if (!g_cpu_diinisialisasi) {
        return cpu_init();
    }
    
    return STATUS_BERHASIL;
}

cpu_konteks_t *cpu_konteks_dapatkan(void)
{
    if (!g_cpu_diinisialisasi) {
        return NULL;
    }
    
    if (g_cpu_konteks.magic != CPU_KONTEKS_MAGIC) {
        return NULL;
    }
    
    return &g_cpu_konteks;
}

cpu_info_t *cpu_info_dapatkan(void)
{
    if (!g_cpu_diinisialisasi) {
        return NULL;
    }
    
    return &g_cpu_konteks.info;
}

/*
 * ===========================================================================
 * FUNGSI FITUR
 * ===========================================================================
 */

bool_t cpu_fitur_cek(tak_bertanda64_t fitur)
{
    cpu_info_t *info;
    
    if (!g_cpu_diinisialisasi) {
        return SALAH;
    }
    
    info = &g_cpu_konteks.info;
    
    /* Cek di fitur standar */
    if ((info->fitur & fitur) != 0) {
        return BENAR;
    }
    
    return SALAH;
}

bool_t cpu_fitur_cek_ext(tak_bertanda64_t fitur)
{
    cpu_info_t *info;
    
    if (!g_cpu_diinisialisasi) {
        return SALAH;
    }
    
    info = &g_cpu_konteks.info;
    
    /* Cek di fitur extended */
    if ((info->fitur_ext & fitur) != 0) {
        return BENAR;
    }
    
    return SALAH;
}

tak_bertanda32_t cpu_fitur_string(char *buffer, ukuran_t ukuran)
{
    cpu_info_t *info;
    ukuran_t pos = 0;
    
    if (buffer == NULL || ukuran == 0) {
        return 0;
    }
    
    if (!g_cpu_diinisialisasi) {
        buffer[0] = '\0';
        return 0;
    }
    
    info = &g_cpu_konteks.info;
    
    /* FPU */
    if ((info->fitur & CPU_FEAT_FPU) && pos < ukuran - 5) {
        buffer[pos++] = 'F';
        buffer[pos++] = 'P';
        buffer[pos++] = 'U';
        buffer[pos++] = ' ';
    }
    
    /* MMX */
    if ((info->fitur & CPU_FEAT_MMX) && pos < ukuran - 5) {
        buffer[pos++] = 'M';
        buffer[pos++] = 'M';
        buffer[pos++] = 'X';
        buffer[pos++] = ' ';
    }
    
    /* SSE */
    if ((info->fitur & CPU_FEAT_SSE) && pos < ukuran - 5) {
        buffer[pos++] = 'S';
        buffer[pos++] = 'S';
        buffer[pos++] = 'E';
        buffer[pos++] = ' ';
    }
    
    /* SSE2 */
    if ((info->fitur & CPU_FEAT_SSE2) && pos < ukuran - 6) {
        buffer[pos++] = 'S';
        buffer[pos++] = 'S';
        buffer[pos++] = 'E';
        buffer[pos++] = '2';
        buffer[pos++] = ' ';
    }
    
    /* SSE3 */
    if ((info->fitur_ext & CPU_FEAT_SSE3) && pos < ukuran - 6) {
        buffer[pos++] = 'S';
        buffer[pos++] = 'S';
        buffer[pos++] = 'E';
        buffer[pos++] = '3';
        buffer[pos++] = ' ';
    }
    
    /* AVX */
    if ((info->fitur_ext & CPU_FEAT_AVX) && pos < ukuran - 5) {
        buffer[pos++] = 'A';
        buffer[pos++] = 'V';
        buffer[pos++] = 'X';
        buffer[pos++] = ' ';
    }
    
    buffer[pos] = '\0';
    
    return (tak_bertanda32_t)pos;
}

/*
 * ===========================================================================
 * FUNGSI FREKUENSI
 * ===========================================================================
 */

tak_bertanda32_t cpu_frekuensi_dapatkan(void)
{
    if (!g_cpu_diinisialisasi) {
        return 0;
    }
    
    return g_cpu_konteks.info.frekuensi_mhz;
}

tak_bertanda32_t cpu_frekuensi_max(void)
{
    if (!g_cpu_diinisialisasi) {
        return 0;
    }
    
    return g_cpu_konteks.info.frekuensi_max_mhz;
}

tak_bertanda32_t cpu_frekuensi_min(void)
{
    if (!g_cpu_diinisialisasi) {
        return 0;
    }
    
    return g_cpu_konteks.info.frekuensi_min_mhz;
}

/*
 * ===========================================================================
 * FUNGSI TSC
 * ===========================================================================
 */

tak_bertanda64_t cpu_tsc_baca(void)
{
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    tak_bertanda32_t low, high;
    
    __asm__ __volatile__ (
        "rdtsc"
        : "=a"(low), "=d"(high)
    );
    
    return ((tak_bertanda64_t)high << 32) | low;
#elif defined(__aarch64__)
    tak_bertanda64_t val;
    __asm__ __volatile__ (
        "mrs %0, cntvct_el0"
        : "=r"(val)
    );
    return val;
#else
    return 0;
#endif
}

void cpu_tsc_delay(tak_bertanda64_t cycles)
{
    tak_bertanda64_t start, end;
    
    start = cpu_tsc_baca();
    end = start + cycles;
    
    while (cpu_tsc_baca() < end) {
        cpu_nop();
    }
}

void cpu_delay_us(tak_bertanda32_t us)
{
    tak_bertanda64_t freq;
    tak_bertanda64_t cycles;
    
    freq = cpu_tsc_frekuensi();
    if (freq == 0) {
        freq = 1000000000ULL;  /* Default 1 GHz */
    }
    
    cycles = (freq * us) / 1000000;
    cpu_tsc_delay(cycles);
}

void cpu_delay_ms(tak_bertanda32_t ms)
{
    tak_bertanda64_t freq;
    tak_bertanda64_t cycles;
    
    freq = cpu_tsc_frekuensi();
    if (freq == 0) {
        freq = 1000000000ULL;
    }
    
    cycles = (freq * ms) / 1000;
    cpu_tsc_delay(cycles);
}

tak_bertanda64_t cpu_tsc_frekuensi(void)
{
    static tak_bertanda64_t cached_freq = 0;
    
    if (cached_freq != 0) {
        return cached_freq;
    }
    
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    /* Coba baca dari CPUID 0x15 */
    tak_bertanda32_t eax, ebx, ecx, edx;
    
    cpu_cpuid_x86(0x15, 0, &eax, &ebx, &ecx, &edx);
    
    if (eax != 0 && ebx != 0) {
        /* TSC frequency = denominator * core crystal clock */
        cached_freq = ((tak_bertanda64_t)ebx * ecx) / eax;
        return cached_freq;
    }
    
    /* Fallback: gunakan frekuensi yang terdeteksi */
    if (g_cpu_diinisialisasi) {
        cached_freq = (tak_bertanda64_t)g_cpu_konteks.info.frekuensi_mhz * 1000000;
    }
#elif defined(__aarch64__)
    tak_bertanda64_t freq;
    __asm__ __volatile__ (
        "mrs %0, cntfrq_el0"
        : "=r"(freq)
    );
    cached_freq = freq;
#endif
    
    return cached_freq;
}

/*
 * ===========================================================================
 * FUNGSI HAL
 * ===========================================================================
 */

void cpu_halt(void)
{
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    __asm__ __volatile__("hlt");
#elif defined(__arm__) || defined(__aarch64__)
    __asm__ __volatile__("wfi");
#else
    while (1) {
        /* Do nothing */
    }
#endif
}

void cpu_reset(bool_t cold)
{
    /* Disable interrupts */
    cpu_disable_irq();
    
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    /* Keyboard controller reset */
    if (cold) {
        /* Try ACPI reset first */
        cpu_acpi_reboot();
    }
    
    /* Fallback: keyboard controller */
    {
        tak_bertanda8_t temp;
        
        /* Disable interrupts */
        __asm__ __volatile__("cli");
        
        /* Clear keyboard buffer */
        do {
            temp = hal_io_read_8(0x64);
            if (temp & 1) {
                hal_io_read_8(0x60);
            }
        } while (temp & 2);
        
        /* Reset */
        hal_io_write_8(0x64, 0xFE);
    }
#endif
    
    /* Should not reach here */
    while (1) {
        cpu_halt();
    }
}

void cpu_disable_irq(void)
{
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    __asm__ __volatile__("cli");
#elif defined(__arm__)
    __asm__ __volatile__("cpsid i");
#elif defined(__aarch64__)
    __asm__ __volatile__("msr daifset, #2");
#endif
}

void cpu_enable_irq(void)
{
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    __asm__ __volatile__("sti");
#elif defined(__arm__)
    __asm__ __volatile__("cpsie i");
#elif defined(__aarch64__)
    __asm__ __volatile__("msr daifclr, #2");
#endif
}

void cpu_nop(void)
{
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    __asm__ __volatile__("nop");
#elif defined(__arm__) || defined(__aarch64__)
    __asm__ __volatile__("nop");
#endif
}

void cpu_pause(void)
{
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    __asm__ __volatile__("pause");
#elif defined(__arm__) || defined(__aarch64__)
    __asm__ __volatile__("yield");
#endif
}

/*
 * ===========================================================================
 * FUNGSI UTILITAS
 * ===========================================================================
 */

const char *cpu_vendor_nama(cpu_vendor_t vendor)
{
    switch (vendor) {
    case CPU_VENDOR_INTEL:
        return "Intel";
    case CPU_VENDOR_AMD:
        return "AMD";
    case CPU_VENDOR_VIA:
        return "VIA";
    case CPU_VENDOR_TRANSMETA:
        return "Transmeta";
    case CPU_VENDOR_CYRIX:
        return "Cyrix";
    case CPU_VENDOR_CENTAUR:
        return "Centaur";
    case CPU_VENDOR_NSC:
        return "NSC";
    case CPU_VENDOR_RISE:
        return "Rise";
    case CPU_VENDOR_SIS:
        return "SiS";
    case CPU_VENDOR_UMC:
        return "UMC";
    case CPU_VENDOR_NEXGEN:
        return "NexGen";
    case CPU_VENDOR_ARM:
        return "ARM";
    case CPU_VENDOR_QUALCOMM:
        return "Qualcomm";
    case CPU_VENDOR_SAMSUNG:
        return "Samsung";
    case CPU_VENDOR_APPLE:
        return "Apple";
    case CPU_VENDOR_NVIDIA:
        return "NVIDIA";
    case CPU_VENDOR_MARVELL:
        return "Marvell";
    case CPU_VENDOR_BROADCOM:
        return "Broadcom";
    case CPU_VENDOR_MEDIATEK:
        return "MediaTek";
    case CPU_VENDOR_HISILICON:
        return "HiSilicon";
    case CPU_VENDOR_ALLWINNER:
        return "Allwinner";
    case CPU_VENDOR_ROCKCHIP:
        return "Rockchip";
    case CPU_VENDOR_AMLOGIC:
        return "Amlogic";
    default:
        return "Tidak Diketahui";
    }
}

const char *cpu_arsitektur_nama(cpu_arsitektur_t arsitektur)
{
    switch (arsitektur) {
    case CPU_ARSITEKTUR_X86:
        return "x86";
    case CPU_ARSITEKTUR_X86_64:
        return "x86_64";
    case CPU_ARSITEKTUR_ARMV6:
        return "ARMv6";
    case CPU_ARSITEKTUR_ARMV7:
        return "ARMv7";
    case CPU_ARSITEKTUR_ARMV7A:
        return "ARMv7-A";
    case CPU_ARSITEKTUR_ARMV7R:
        return "ARMv7-R";
    case CPU_ARSITEKTUR_ARMV7M:
        return "ARMv7-M";
    case CPU_ARSITEKTUR_ARMV8A:
        return "ARMv8-A";
    case CPU_ARSITEKTUR_ARMV8_1A:
        return "ARMv8.1-A";
    case CPU_ARSITEKTUR_ARMV8_2A:
        return "ARMv8.2-A";
    case CPU_ARSITEKTUR_ARMV8_3A:
        return "ARMv8.3-A";
    case CPU_ARSITEKTUR_ARMV8_4A:
        return "ARMv8.4-A";
    case CPU_ARSITEKTUR_ARMV9A:
        return "ARMv9-A";
    default:
        return "Tidak Diketahui";
    }
}

const char *cpu_status_nama(cpu_status_t status)
{
    switch (status) {
    case CPU_STATUS_TIDAK_ADA:
        return "Tidak Ada";
    case CPU_STATUS_TERDETEKSI:
        return "Terdeteksi";
    case CPU_STATUS_DIIDENTIFIKASI:
        return "Teridentifikasi";
    case CPU_STATUS_SIAP:
        return "Siap";
    case CPU_STATUS_ERROR:
        return "Error";
    case CPU_STATUS_DISABLE:
        return "Disable";
    default:
        return "Tidak Diketahui";
    }
}

void cpu_cetak_info(void)
{
    cpu_info_t *info;
    
    if (!g_cpu_diinisialisasi) {
        return;
    }
    
    info = &g_cpu_konteks.info;
    
    /* Print header */
    /* Format: Vendor Model @ Frequency */
    /* Contoh: Intel(R) Core(TM) i7-8700K @ 3700MHz */
}

void cpu_cetak_fitur(void)
{
    char buffer[256];
    
    if (!g_cpu_diinisialisasi) {
        return;
    }
    
    cpu_fitur_string(buffer, sizeof(buffer));
    /* Print features */
}

void cpu_cetak_cache(void)
{
    cpu_info_t *info;
    tak_bertanda32_t i;
    
    if (!g_cpu_diinisialisasi) {
        return;
    }
    
    info = &g_cpu_konteks.info;
    
    for (i = 0; i < info->jumlah_cache; i++) {
        /* Print cache info */
    }
}

/*
 * ===========================================================================
 * FUNGSI FPU
 * ===========================================================================
 */

status_t cpu_fpu_init(void)
{
    if (!g_cpu_diinisialisasi) {
        return STATUS_GAGAL;
    }
    
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    /* Cek apakah FPU tersedia */
    if (!cpu_fitur_cek(CPU_FEAT_FPU)) {
        return STATUS_TIDAK_DUKUNG;
    }
    
    /* Enable FPU */
    {
#if defined(__x86_64__)
        tak_bertanda64_t cr0;
        
        __asm__ __volatile__ (
            "mov %%cr0, %0"
            : "=r"(cr0)
        );
        
        /* Clear EM bit, Set MP bit */
        cr0 &= ~(1ULL << 2);
        cr0 |= (1ULL << 1);
        
        __asm__ __volatile__ (
            "mov %0, %%cr0"
            :
            : "r"(cr0)
        );
#else
        tak_bertanda32_t cr0;
        
        __asm__ __volatile__ (
            "mov %%cr0, %0"
            : "=r"(cr0)
        );
        
        /* Clear EM bit, Set MP bit */
        cr0 &= ~(1U << 2);
        cr0 |= (1U << 1);
        
        __asm__ __volatile__ (
            "mov %0, %%cr0"
            :
            : "r"(cr0)
        );
#endif
    }
#endif
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI CACHE
 * ===========================================================================
 */

status_t cpu_cache_flush(void *addr, ukuran_t ukuran)
{
    if (!g_cpu_diinisialisasi) {
        return STATUS_GAGAL;
    }
    
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    /* CLFLUSH untuk x86 */
    if (cpu_fitur_cek(CPU_FEAT_CLFSH)) {
        tak_bertanda8_t *ptr = (tak_bertanda8_t *)addr;
        ukuran_t i;
        
        for (i = 0; i < ukuran; i += 64) {
            __asm__ __volatile__ (
                "clflush %0"
                :
                : "m"(ptr[i])
            );
        }
        
        return STATUS_BERHASIL;
    }
#elif defined(__aarch64__)
    /* Clean to point of coherency */
    __asm__ __volatile__ (
        "dc cvac, %0"
        :
        : "r"(addr)
        : "memory"
    );
#endif
    
    return STATUS_BERHASIL;
}

status_t cpu_cache_invalidate(void *addr, ukuran_t ukuran)
{
    (void)addr;
    (void)ukuran;
    if (!g_cpu_diinisialisasi) {
        return STATUS_GAGAL;
    }
    
#if defined(__aarch64__)
    __asm__ __volatile__ (
        "dc ivac, %0"
        :
        : "r"(addr)
        : "memory"
    );
#endif
    
    return STATUS_BERHASIL;
}

cpu_cache_t *cpu_cache_info(tak_bertanda32_t level, tak_bertanda32_t tipe)
{
    cpu_info_t *info;
    tak_bertanda32_t i;
    
    if (!g_cpu_diinisialisasi) {
        return NULL;
    }
    
    info = &g_cpu_konteks.info;
    
    for (i = 0; i < info->jumlah_cache; i++) {
        if (info->cache[i].level == level) {
            if (tipe == 0 || info->cache[i].tipe == tipe) {
                return &info->cache[i];
            }
        }
    }
    
    return NULL;
}

/*
 * ===========================================================================
 * FUNGSI SMP
 * ===========================================================================
 */

status_t cpu_smp_init(void)
{
    if (!g_cpu_diinisialisasi) {
        return STATUS_GAGAL;
    }
    
    /* SMP init akan dilakukan oleh subsistem SMP */
    return STATUS_BERHASIL;
}

tak_bertanda32_t cpu_smp_jumlah(void)
{
    if (!g_cpu_diinisialisasi) {
        return 1;
    }
    
    return g_cpu_konteks.info.jumlah_core;
}

tak_bertanda32_t cpu_smp_id(void)
{
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    tak_bertanda32_t id;
    __asm__ __volatile__ (
        "mov $1, %%eax\n"
        "cpuid\n"
        "shr $24, %%ebx\n"
        "mov %%ebx, %0"
        : "=r"(id)
        :
        : "eax", "ebx", "ecx", "edx"
    );
    return id;
#elif defined(__aarch64__)
    tak_bertanda64_t mpidr;
    __asm__ __volatile__ (
        "mrs %0, mpidr_el1"
        : "=r"(mpidr)
    );
    return (tak_bertanda32_t)(mpidr & 0xFF);
#else
    return 0;
#endif
}

/*
 * ===========================================================================
 * FUNGSI MSR
 * ===========================================================================
 */

#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)

status_t cpu_msr_baca(tak_bertanda32_t msr, tak_bertanda32_t *low,
                       tak_bertanda32_t *high)
{
    if (!cpu_fitur_cek(CPU_FEAT_MSR)) {
        return STATUS_TIDAK_DUKUNG;
    }
    
    __asm__ __volatile__ (
        "rdmsr"
        : "=a"(*low), "=d"(*high)
        : "c"(msr)
    );
    
    return STATUS_BERHASIL;
}

status_t cpu_msr_tulis(tak_bertanda32_t msr, tak_bertanda32_t low,
                        tak_bertanda32_t high)
{
    if (!cpu_fitur_cek(CPU_FEAT_MSR)) {
        return STATUS_TIDAK_DUKUNG;
    }
    
    __asm__ __volatile__ (
        "wrmsr"
        :
        : "a"(low), "d"(high), "c"(msr)
    );
    
    return STATUS_BERHASIL;
}

tak_bertanda64_t cpu_msr_baca64(tak_bertanda32_t msr)
{
    tak_bertanda32_t low, high;
    
    if (cpu_msr_baca(msr, &low, &high) != STATUS_BERHASIL) {
        return 0;
    }
    
    return ((tak_bertanda64_t)high << 32) | low;
}

void cpu_msr_tulis64(tak_bertanda32_t msr, tak_bertanda64_t nilai)
{
    cpu_msr_tulis(msr, (tak_bertanda32_t)(nilai & 0xFFFFFFFF),
                  (tak_bertanda32_t)(nilai >> 32));
}

#endif /* x86/x86_64 */

/*
 * ===========================================================================
 * FUNGSI CPUID
 * ===========================================================================
 */

status_t cpu_cpuid(tak_bertanda32_t leaf, tak_bertanda32_t subleaf,
                   tak_bertanda32_t *eax, tak_bertanda32_t *ebx,
                   tak_bertanda32_t *ecx, tak_bertanda32_t *edx)
{
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    cpu_cpuid_x86(leaf, subleaf, eax, ebx, ecx, edx);
    return STATUS_BERHASIL;
#else
    return STATUS_TIDAK_DUKUNG;
#endif
}
