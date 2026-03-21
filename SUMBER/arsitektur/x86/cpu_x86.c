/*
 * PIGURA OS - CPU x86
 * --------------------
 * Implementasi fungsi-fungsi CPU untuk arsitektur x86.
 *
 * Berkas ini berisi fungsi-fungsi untuk deteksi dan pengelolaan CPU
 * termasuk pembacaan informasi CPUID, deteksi fitur, dan operasi
 * CPU dasar seperti halt dan reset.
 *
 * Arsitektur: x86 (32-bit)
 * Versi: 1.0
 */

#include "../../inti/kernel.h"

/*
 * ============================================================================
 * KONSTANTA CPUID
 * ============================================================================
 */

/* Leaf CPUID */
#define CPUID_LEAF_VENDOR       0x00000000
#define CPUID_LEAF_FEATURES     0x00000001
#define CPUID_LEAF_CACHE        0x00000002
#define CPUID_LEAF_SERIAL       0x00000003
#define CPUID_LEAF_EXTENDED     0x80000000
#define CPUID_LEAF_BRAND        0x80000002
#define CPUID_LEAF_BRAND_CONT   0x80000003
#define CPUID_LEAF_BRAND_END    0x80000004

/* Bit fitur EDX (leaf 0x01) */
#define CPUID_FEAT_EDX_FPU      (1 << 0)
#define CPUID_FEAT_EDX_VME      (1 << 1)
#define CPUID_FEAT_EDX_DE       (1 << 2)
#define CPUID_FEAT_EDX_PSE      (1 << 3)
#define CPUID_FEAT_EDX_TSC      (1 << 4)
#define CPUID_FEAT_EDX_MSR      (1 << 5)
#define CPUID_FEAT_EDX_PAE      (1 << 6)
#define CPUID_FEAT_EDX_MCE      (1 << 7)
#define CPUID_FEAT_EDX_CX8      (1 << 8)
#define CPUID_FEAT_EDX_APIC     (1 << 9)
#define CPUID_FEAT_EDX_SEP      (1 << 11)
#define CPUID_FEAT_EDX_MTRR     (1 << 12)
#define CPUID_FEAT_EDX_PGE      (1 << 13)
#define CPUID_FEAT_EDX_MCA      (1 << 14)
#define CPUID_FEAT_EDX_CMOV     (1 << 15)
#define CPUID_FEAT_EDX_PAT      (1 << 16)
#define CPUID_FEAT_EDX_PSE36    (1 << 17)
#define CPUID_FEAT_EDX_PSN      (1 << 18)
#define CPUID_FEAT_EDX_CLFSH    (1 << 19)
#define CPUID_FEAT_EDX_DS       (1 << 21)
#define CPUID_FEAT_EDX_ACPI     (1 << 22)
#define CPUID_FEAT_EDX_MMX      (1 << 23)
#define CPUID_FEAT_EDX_FXSR     (1 << 24)
#define CPUID_FEAT_EDX_SSE      (1 << 25)
#define CPUID_FEAT_EDX_SSE2     (1 << 26)
#define CPUID_FEAT_EDX_SS       (1 << 27)
#define CPUID_FEAT_EDX_HTT      (1 << 28)
#define CPUID_FEAT_EDX_TM       (1 << 29)
#define CPUID_FEAT_EDX_PBE      (1 << 31)

/* Bit fitur ECX (leaf 0x01) */
#define CPUID_FEAT_ECX_SSE3     (1 << 0)
#define CPUID_FEAT_ECX_PCLMUL   (1 << 1)
#define CPUID_FEAT_ECX_DTES64   (1 << 2)
#define CPUID_FEAT_ECX_MONITOR  (1 << 3)
#define CPUID_FEAT_ECX_DS_CPL   (1 << 4)
#define CPUID_FEAT_ECX_VMX      (1 << 5)
#define CPUID_FEAT_ECX_SMX      (1 << 6)
#define CPUID_FEAT_ECX_EST      (1 << 7)
#define CPUID_FEAT_ECX_TM2      (1 << 8)
#define CPUID_FEAT_ECX_SSSE3    (1 << 9)
#define CPUID_FEAT_ECX_CID      (1 << 10)
#define CPUID_FEAT_ECX_FMA      (1 << 12)
#define CPUID_FEAT_ECX_CX16     (1 << 13)
#define CPUID_FEAT_ECX_ETPRD    (1 << 14)
#define CPUID_FEAT_ECX_PDCM     (1 << 15)
#define CPUID_FEAT_ECX_PCID     (1 << 17)
#define CPUID_FEAT_ECX_DCA      (1 << 18)
#define CPUID_FEAT_ECX_SSE4_1   (1 << 19)
#define CPUID_FEAT_ECX_SSE4_2   (1 << 20)
#define CPUID_FEAT_ECX_X2APIC   (1 << 21)
#define CPUID_FEAT_ECX_MOVBE    (1 << 22)
#define CPUID_FEAT_ECX_POPCNT   (1 << 23)
#define CPUID_FEAT_ECX_TSC_DL   (1 << 24)
#define CPUID_FEAT_ECX_AES      (1 << 25)
#define CPUID_FEAT_ECX_XSAVE    (1 << 26)
#define CPUID_FEAT_ECX_OSXSAVE  (1 << 27)
#define CPUID_FEAT_ECX_AVX      (1 << 28)

/* Extended features EDX (leaf 0x80000001) */
#define CPUID_EXT_EDX_FPU       (1 << 0)
#define CPUID_EXT_EDX_VME       (1 << 1)
#define CPUID_EXT_EDX_DE        (1 << 2)
#define CPUID_EXT_EDX_PSE       (1 << 3)
#define CPUID_EXT_EDX_TSC       (1 << 4)
#define CPUID_EXT_EDX_MSR       (1 << 5)
#define CPUID_EXT_EDX_PAE       (1 << 6)
#define CPUID_EXT_EDX_MCE       (1 << 7)
#define CPUID_EXT_EDX_CX8       (1 << 8)
#define CPUID_EXT_EDX_APIC      (1 << 9)
#define CPUID_EXT_EDX_SYSCALL   (1 << 11)
#define CPUID_EXT_EDX_MTRR      (1 << 12)
#define CPUID_EXT_EDX_PGE       (1 << 13)
#define CPUID_EXT_EDX_MCA       (1 << 14)
#define CPUID_EXT_EDX_CMOV      (1 << 15)
#define CPUID_EXT_EDX_PAT       (1 << 16)
#define CPUID_EXT_EDX_PSE36     (1 << 17)
#define CPUID_EXT_EDX_NX        (1 << 20)
#define CPUID_EXT_EDX_MMXEXT    (1 << 22)
#define CPUID_EXT_EDX_MMX       (1 << 23)
#define CPUID_EXT_EDX_FXSR      (1 << 24)
#define CPUID_EXT_EDX_FXSR_OPT  (1 << 25)
#define CPUID_EXT_EDX_PDPE1GB   (1 << 26)
#define CPUID_EXT_EDX_RDTSCP    (1 << 27)
#define CPUID_EXT_EDX_LM        (1 << 29)
#define CPUID_EXT_EDX_3DNOWEXT  (1 << 30)
#define CPUID_EXT_EDX_3DNOW     (1 << 31)

/* Extended features ECX (leaf 0x80000001) */
#define CPUID_EXT_ECX_LAHF_LM   (1 << 0)
#define CPUID_EXT_ECX_CMP_LEG   (1 << 1)
#define CPUID_EXT_ECX_SVM       (1 << 2)
#define CPUID_EXT_ECX_EXTAPIC   (1 << 3)
#define CPUID_EXT_ECX_CR8_LEG   (1 << 4)
#define CPUID_EXT_ECX_ABM       (1 << 5)
#define CPUID_EXT_ECX_SSE4A     (1 << 6)
#define CPUID_EXT_ECX_MISALIGNSSE (1 << 7)
#define CPUID_EXT_ECX_3DNOWPREF (1 << 8)
#define CPUID_EXT_ECX_OSVW      (1 << 9)
#define CPUID_EXT_ECX_IBS       (1 << 10)
#define CPUID_EXT_ECX_XOP       (1 << 11)
#define CPUID_EXT_ECX_SKINIT    (1 << 12)
#define CPUID_EXT_ECX_WDT       (1 << 13)

/* MSR addresses */
#define MSR_IA32_P5_MC_ADDR     0x00000000
#define MSR_IA32_P5_MC_TYPE     0x00000001
#define MSR_IA32_TSC            0x00000010
#define MSR_IA32_PLATFORM_ID    0x00000017
#define MSR_IA32_APIC_BASE      0x0000001B
#define MSR_IA32_EBL_CR_POWERON 0x0000002A
#define MSR_IA32_FEATURE_CONTROL 0x0000003A
#define MSR_IA32_BIOS_UPDT_TRIG 0x00000079
#define MSR_IA32_BIOS_SIGN_ID   0x0000008B
#define MSR_IA32_MTRRCAP        0x000000FE
#define MSR_IA32_SYSENTER_CS    0x00000174
#define MSR_IA32_SYSENTER_ESP   0x00000175
#define MSR_IA32_SYSENTER_EIP   0x00000176
#define MSR_IA32_MCG_CAP        0x00000179
#define MSR_IA32_MCG_STATUS     0x0000017A
#define MSR_IA32_MCG_CTL        0x0000017B
#define MSR_IA32_PERFEVTSEL0    0x00000186
#define MSR_IA32_PERFEVTSEL1    0x00000187
#define MSR_IA32_PERF_STATUS    0x00000198
#define MSR_IA32_PERF_CTL       0x00000199
#define MSR_IA32_MTRR_PHYSBASE0 0x00000200
#define MSR_IA32_MTRR_PHYSMASK0 0x00000201
#define MSR_IA32_MTRR_FIX64K_00000 0x00000250
#define MSR_IA32_MTRR_FIX16K_80000 0x00000258
#define MSR_IA32_MTRR_FIX16K_A0000 0x00000259
#define MSR_IA32_MTRR_FIX4K_C0000 0x00000268
#define MSR_IA32_PAT            0x00000277
#define MSR_IA32_MTRR_DEF_TYPE  0x000002FF
#define MSR_IA32_DEBUGCTLMSR    0x000001D9
#define MSR_IA32_LASTBRANCHFROMIP 0x000001DB
#define MSR_IA32_LASTBRANCHTOIP 0x000001DC
#define MSR_IA32_LASTINTFROMIP  0x000001DD
#define MSR_IA32_LASTINTTOIP    0x000001DE

/*
 * ============================================================================
 * VARIABEL GLOBAL
 * ============================================================================
 */

/* Informasi CPU yang terdeteksi */
static hal_cpu_info_t g_info_cpu;

/* Flag CPUID tersedia */
static bool_t g_cpoid_tersedia = SALAH;

/* Leaf maksimal CPUID */
static tak_bertanda32_t g_cpoid_max_leaf = 0;
static tak_bertanda32_t g_cpoid_max_ext = 0;

/*
 * ============================================================================
 * FUNGSI INTERNAL
 * ============================================================================
 */

/*
 * _cpuid
 * ------
 * Menjalankan instruksi CPUID dan menyimpan hasilnya.
 *
 * Parameter:
 *   leaf - Nilai EAX input (leaf/level)
 *   sub  - Nilai ECX input (sub-leaf)
 *   eax  - Pointer untuk menyimpan EAX output
 *   ebx  - Pointer untuk menyimpan EBX output
 *   ecx  - Pointer untuk menyimpan ECX output
 *   edx  - Pointer untuk menyimpan EDX output
 */
static void _cpuid(tak_bertanda32_t leaf, tak_bertanda32_t sub,
                   tak_bertanda32_t *eax, tak_bertanda32_t *ebx,
                   tak_bertanda32_t *ecx, tak_bertanda32_t *edx)
{
    tak_bertanda32_t eax_val;
    tak_bertanda32_t ebx_val;
    tak_bertanda32_t ecx_val;
    tak_bertanda32_t edx_val;

    __asm__ __volatile__(
        "cpuid\n\t"
        : "=a"(eax_val), "=b"(ebx_val), "=c"(ecx_val), "=d"(edx_val)
        : "a"(leaf), "c"(sub)
    );

    if (eax != NULL) {
        *eax = eax_val;
    }
    if (ebx != NULL) {
        *ebx = ebx_val;
    }
    if (ecx != NULL) {
        *ecx = ecx_val;
    }
    if (edx != NULL) {
        *edx = edx_val;
    }
}

/*
 * _deteksi_cpoid
 * --------------
 * Mendeteksi apakah CPUID tersedia dengan mencoba flip bit 21
 * dari EFLAGS. Jika bit bisa di-flip, CPUID tersedia.
 *
 * Return:
 *   BENAR jika CPUID tersedia, SALAH jika tidak
 */
static bool_t _deteksi_cpoid(void)
{
    tak_bertanda32_t flags1;
    tak_bertanda32_t flags2;

    /* Baca EFLAGS */
    __asm__ __volatile__(
        "pushfl\n\t"
        "popl %0\n\t"
        : "=r"(flags1)
    );

    /* Flip bit 21 (ID bit) */
    flags2 = flags1 ^ (1 << 21);

    /* Tulis kembali */
    __asm__ __volatile__(
        "pushl %0\n\t"
        "popfl\n\t"
        :
        : "r"(flags2)
    );

    /* Baca lagi */
    __asm__ __volatile__(
        "pushfl\n\t"
        "popl %0\n\t"
        : "=r"(flags2)
    );

    /* Jika bit 21 berubah, CPUID tersedia */
    if ((flags1 ^ flags2) & (1 << 21)) {
        return BENAR;
    }

    return SALAH;
}

/*
 * _baca_vendor
 * ------------
 * Membaca string vendor CPU dari CPUID leaf 0.
 *
 * Parameter:
 *   vendor - Buffer untuk menyimpan string vendor (min 13 byte)
 */
static void _baca_vendor(char *vendor)
{
    tak_bertanda32_t ebx;
    tak_bertanda32_t ecx;
    tak_bertanda32_t edx;

    if (vendor == NULL) {
        return;
    }

    _cpuid(CPUID_LEAF_VENDOR, 0, &g_cpoid_max_leaf, &ebx, &ecx, &edx);

    /* Vendor string disimpan di EBX, EDX, ECX (urutan aneh) */
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
}

/*
 * _baca_brand
 * -----------
 * Membaca string brand CPU dari CPUID extended leaf.
 *
 * Parameter:
 *   brand - Buffer untuk menyimpan string brand (min 49 byte)
 */
static void _baca_brand(char *brand)
{
    tak_bertanda32_t eax;
    tak_bertanda32_t ebx;
    tak_bertanda32_t ecx;
    tak_bertanda32_t edx;
    tak_bertanda32_t *ptr;

    if (brand == NULL) {
        return;
    }

    /* Brand string dari leaf 0x80000002, 0x80000003, 0x80000004 */
    ptr = (tak_bertanda32_t *)brand;

    _cpuid(CPUID_LEAF_BRAND, 0, &eax, &ebx, &ecx, &edx);
    *ptr++ = eax;
    *ptr++ = ebx;
    *ptr++ = ecx;
    *ptr++ = edx;

    _cpuid(CPUID_LEAF_BRAND_CONT, 0, &eax, &ebx, &ecx, &edx);
    *ptr++ = eax;
    *ptr++ = ebx;
    *ptr++ = ecx;
    *ptr++ = edx;

    _cpuid(CPUID_LEAF_BRAND_END, 0, &eax, &ebx, &ecx, &edx);
    *ptr++ = eax;
    *ptr++ = ebx;
    *ptr++ = ecx;
    *ptr++ = edx;

    brand[48] = '\0';
}

/*
 * _baca_signature
 * ---------------
 * Membaca signature CPU (family, model, stepping) dari CPUID.
 */
static void _baca_signature(void)
{
    tak_bertanda32_t eax;
    tak_bertanda32_t ebx;
    tak_bertanda32_t ecx;
    tak_bertanda32_t edx;

    _cpuid(CPUID_LEAF_FEATURES, 0, &eax, &ebx, &ecx, &edx);

    /* Signature di EAX */
    g_info_cpu.stepping = eax & 0x0F;
    g_info_cpu.model = (eax >> 4) & 0x0F;
    g_info_cpu.family = (eax >> 8) & 0x0F;

    /* Extended model dan family */
    if (g_info_cpu.family == 0x0F) {
        g_info_cpu.family += (eax >> 20) & 0xFF;
        g_info_cpu.model += ((eax >> 16) & 0x0F) << 4;
    }

    /* Informasi tambahan dari EBX */
    g_info_cpu.apic_id = (eax >> 24) & 0xFF;
}

/*
 * _baca_fitur
 * -----------
 * Membaca fitur CPU dari CPUID.
 */
static void _baca_fitur(void)
{
    tak_bertanda32_t eax;
    tak_bertanda32_t ebx;
    tak_bertanda32_t ecx;
    tak_bertanda32_t edx;
    tak_bertanda32_t ext_edx;

    _cpuid(CPUID_LEAF_FEATURES, 0, &eax, &ebx, &ecx, &edx);

    /* Simpan feature flags */
    g_info_cpu.features = edx;

    /* Baca extended features jika tersedia */
    if (g_cpoid_max_ext >= 0x80000001) {
        _cpuid(0x80000001, 0, &eax, &ebx, &ecx, &ext_edx);

        /* Gabungkan fitur extended */
        if (ext_edx & CPUID_EXT_EDX_NX) {
            g_info_cpu.features |= (1 << 31); /* NX bit */
        }
        if (ext_edx & CPUID_EXT_EDX_SYSCALL) {
            g_info_cpu.features |= (1 << 30); /* SYSCALL */
        }
        if (ext_edx & CPUID_EXT_EDX_RDTSCP) {
            g_info_cpu.features |= (1 << 29); /* RDTSCP */
        }
    }
}

/*
 * _baca_cache
 * -----------
 * Membaca informasi cache dari CPUID.
 */
static void _baca_cache(void)
{
    tak_bertanda32_t eax;
    tak_bertanda32_t ebx;
    tak_bertanda32_t ecx;
    tak_bertanda32_t edx;

    /* Default values */
    g_info_cpu.cache_line = 64;
    g_info_cpu.cache_l1 = 32;
    g_info_cpu.cache_l2 = 256;
    g_info_cpu.cache_l3 = 8192;

    /* Coba baca dari CPUID leaf 0x02 (Intel) */
    if (g_cpoid_max_leaf >= 2) {
        _cpuid(2, 0, &eax, &ebx, &ecx, &edx);

        /* Parse cache info (sederhana) */
        /* Format bervariasi per vendor, ini implementasi sederhana */
        g_info_cpu.cache_line = 64;
    }

    /* Coba baca dari extended leaf 0x80000005/6 (AMD) */
    if (g_cpoid_max_ext >= 0x80000005) {
        _cpuid(0x80000005, 0, &eax, &ebx, &ecx, &edx);

        /* L1 cache dari ECX (AMD) */
        g_info_cpu.cache_l1 = ((ecx >> 24) & 0xFF); /* L1 D-cache KB */
    }

    if (g_cpoid_max_ext >= 0x80000006) {
        _cpuid(0x80000006, 0, &eax, &ebx, &ecx, &edx);

        /* L2 cache dari ECX (AMD) */
        g_info_cpu.cache_l2 = ((ecx >> 16) & 0xFFFF); /* L2 KB */
    }
}

/*
 * _deteksi_cores
 * --------------
 * Mendeteksi jumlah core dan thread CPU.
 */
static void _deteksi_cores(void)
{
    tak_bertanda32_t eax;
    tak_bertanda32_t ebx;
    tak_bertanda32_t ecx;
    tak_bertanda32_t edx;

    /* Default single core */
    g_info_cpu.cores = 1;
    g_info_cpu.threads = 1;

    /* Intel: leaf 0x0B jika tersedia */
    if (g_cpoid_max_leaf >= 0x0B) {
        _cpuid(0x0B, 0, &eax, &ebx, &ecx, &edx);
        if (ebx & 0xFFFF) {
            g_info_cpu.threads = (tak_bertanda8_t)(ebx & 0xFFFF);
        }

        _cpuid(0x0B, 1, &eax, &ebx, &ecx, &edx);
        if (ebx & 0xFFFF) {
            tak_bertanda32_t threads_total = ebx & 0xFFFF;
            if (g_info_cpu.threads > 0) {
                g_info_cpu.cores = (tak_bertanda8_t)
                    (threads_total / g_info_cpu.threads);
            }
        }
    }

    /* AMD: extended leaf 0x80000008 */
    if (g_cpoid_max_ext >= 0x80000008) {
        _cpuid(0x80000008, 0, &eax, &ebx, &ecx, &edx);
        g_info_cpu.cores = (tak_bertanda8_t)((ecx & 0xFF) + 1);
    }
}

/*
 * _deteksi_frekuensi
 * ------------------
 * Mendeteksi frekuensi CPU (estimasi).
 */
static void _deteksi_frekuensi(void)
{
    tak_bertanda32_t eax;
    tak_bertanda32_t ebx;
    tak_bertanda32_t ecx;
    tak_bertanda32_t edx;

    /* Default 1 GHz */
    g_info_cpu.freq_mhz = 1000;

    /* Coba baca dari brand string */
    /* Contoh: "Intel(R) Core(TM) i5-7200U CPU @ 2.50GHz" */
    if (g_info_cpu.brand[0] != '\0') {
        char *ghz = kernel_strstr(g_info_cpu.brand, "GHz");
        if (ghz != NULL) {
            /* Cari angka sebelum "GHz" */
            char *ptr = ghz - 1;
            while (ptr > g_info_cpu.brand && 
                   (*ptr < '0' || *ptr > '9') && *ptr != '.') {
                ptr--;
            }
            if (ptr > g_info_cpu.brand) {
                /* Parse frekuensi */
                tak_bertanda32_t ghz_val = 0;
                tak_bertanda32_t mhz_val = 0;
                char *end = ghz - 1;

                while (end > g_info_cpu.brand && 
                       ((*end >= '0' && *end <= '9') || *end == '.')) {
                    end--;
                }
                end++;

                /* Parse nilai */
                while (end < ghz && *end >= '0' && *end <= '9') {
                    ghz_val = ghz_val * 10 + (*end - '0');
                    end++;
                }
                if (*end == '.') {
                    end++;
                    while (end < ghz && *end >= '0' && *end <= '9') {
                        mhz_val = mhz_val * 10 + (*end - '0');
                        end++;
                    }
                }

                g_info_cpu.freq_mhz = ghz_val * 1000 + mhz_val * 100;
            }
        }
    }

    /* Coba baca dari CPUID leaf 0x15 (Intel) */
    if (g_cpoid_max_leaf >= 0x15) {
        _cpuid(0x15, 0, &eax, &ebx, &ecx, &edx);
        if (ebx != 0 && eax != 0) {
            /* Core crystal clock frequency */
            tak_bertanda32_t denom = eax;
            tak_bertanda32_t numer = ebx;
            tak_bertanda32_t crystal = ecx; /* Hz */

            if (denom != 0 && crystal != 0) {
                g_info_cpu.freq_mhz = 
                    (tak_bertanda32_t)((numer * crystal) / 
                                       (denom * 1000000));
            }
        }
    }
}

/*
 * _baca_msr
 * ---------
 * Membaca Model Specific Register (MSR).
 *
 * Parameter:
 *   msr - Alamat MSR
 *
 * Return:
 *   Nilai MSR (64-bit)
 */
static tak_bertanda64_t _baca_msr(tak_bertanda32_t msr)
{
    tak_bertanda32_t low;
    tak_bertanda32_t high;

    __asm__ __volatile__(
        "rdmsr\n\t"
        : "=a"(low), "=d"(high)
        : "c"(msr)
    );

    return ((tak_bertanda64_t)high << 32) | low;
}

/*
 * _tulis_msr
 * ----------
 * Menulis ke Model Specific Register (MSR).
 *
 * Parameter:
 *   msr   - Alamat MSR
 *   nilai - Nilai yang akan ditulis (64-bit)
 */
static void _tulis_msr(tak_bertanda32_t msr, tak_bertanda64_t nilai)
{
    tak_bertanda32_t low = (tak_bertanda32_t)(nilai & 0xFFFFFFFF);
    tak_bertanda32_t high = (tak_bertanda32_t)(nilai >> 32);

    __asm__ __volatile__(
        "wrmsr\n\t"
        :
        : "c"(msr), "a"(low), "d"(high)
    );
}

/*
 * ============================================================================
 * FUNGSI PUBLIC
 * ============================================================================
 */

/*
 * hal_cpu_init
 * ------------
 * Menginisialisasi subsistem CPU.
 *
 * Fungsi ini mendeteksi dan mengumpulkan informasi CPU.
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t hal_cpu_init(void)
{
    kernel_memset(&g_info_cpu, 0, sizeof(hal_cpu_info_t));

    /* Deteksi CPUID support */
    g_cpoid_tersedia = _deteksi_cpoid();

    if (!g_cpoid_tersedia) {
        kernel_printf("[CPU] CPUID tidak tersedia\n");
        return STATUS_GAGAL;
    }

    /* Baca max extended leaf */
    _cpuid(CPUID_LEAF_EXTENDED, 0, &g_cpoid_max_ext, 
           NULL, NULL, NULL);

    /* Baca informasi CPU */
    _baca_vendor(g_info_cpu.vendor);
    _baca_signature();
    _baca_fitur();
    _baca_cache();
    _deteksi_cores();

    /* Baca brand string jika tersedia */
    if (g_cpoid_max_ext >= CPUID_LEAF_BRAND) {
        _baca_brand(g_info_cpu.brand);
    }

    _deteksi_frekuensi();

    g_info_cpu.id = 0; /* Single CPU untuk saat ini */

    return STATUS_BERHASIL;
}

/*
 * hal_cpu_halt
 * ------------
 * Menghentikan CPU secara permanen.
 */
void hal_cpu_halt(void)
{
    cpu_disable_irq();
    while (1) {
        cpu_halt();
    }
}

/*
 * hal_cpu_reset
 * -------------
 * Mereset CPU.
 *
 * Parameter:
 *   keras - Jika BENAR, lakukan hard reset
 */
void hal_cpu_reset(bool_t keras)
{
    if (keras) {
        /* Hard reset via keyboard controller */
        tak_bertanda8_t temp;

        cpu_disable_irq();

        /* Flush keyboard buffer */
        do {
            temp = inb(0x64);
            if (temp & 0x01) {
                inb(0x60);
            }
        } while (temp & 0x02);

        /* Pulse reset line */
        outb(0x64, 0xFE); /* Reset command */

        /* Wait */
        cpu_halt();
    } else {
        /* Soft reset via BIOS */
        tak_bertanda32_t *warm_reset_flag = 
            (tak_bertanda32_t *)0x0472;
        *warm_reset_flag = 0x1234;

        /* Jump to reset vector */
        __asm__ __volatile__(
            "ljmp $0xFFFF, $0x0000\n\t"
        );
    }

    /* Tidak akan sampai sini */
    while (1) {
        cpu_halt();
    }
}

/*
 * hal_cpu_get_id
 * --------------
 * Mendapatkan ID CPU saat ini.
 *
 * Return:
 *   ID CPU
 */
tak_bertanda32_t hal_cpu_get_id(void)
{
    return g_info_cpu.id;
}

/*
 * hal_cpu_get_info
 * ----------------
 * Mendapatkan informasi CPU.
 *
 * Parameter:
 *   info - Pointer ke buffer untuk menyimpan informasi
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t hal_cpu_get_info(hal_cpu_info_t *info)
{
    if (info == NULL) {
        return STATUS_PARAM_INVALID;
    }

    kernel_memcpy(info, &g_info_cpu, sizeof(hal_cpu_info_t));

    return STATUS_BERHASIL;
}

/*
 * hal_cpu_enable_interrupts
 * -------------------------
 * Mengaktifkan interupsi CPU.
 */
void hal_cpu_enable_interrupts(void)
{
    cpu_enable_irq();
}

/*
 * hal_cpu_disable_interrupts
 * --------------------------
 * Menonaktifkan interupsi CPU.
 */
void hal_cpu_disable_interrupts(void)
{
    cpu_disable_irq();
}

/*
 * hal_cpu_save_interrupts
 * -----------------------
 * Menyimpan state interupsi dan menonaktifkannya.
 *
 * Return:
 *   State interupsi sebelumnya
 */
tak_bertanda32_t hal_cpu_save_interrupts(void)
{
    return cpu_save_flags();
}

/*
 * hal_cpu_restore_interrupts
 * --------------------------
 * Mengembalikan state interupsi.
 *
 * Parameter:
 *   state - State yang disimpan
 */
void hal_cpu_restore_interrupts(tak_bertanda32_t state)
{
    cpu_restore_flags(state);
}

/*
 * hal_cpu_invalidate_tlb
 * ----------------------
 * Menginvalidate TLB untuk alamat tertentu.
 *
 * Parameter:
 *   addr - Alamat virtual
 */
void hal_cpu_invalidate_tlb(void *addr)
{
    cpu_invlpg(addr);
}

/*
 * hal_cpu_invalidate_tlb_all
 * --------------------------
 * Menginvalidate seluruh TLB.
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
    /* WBINVD - writeback and invalidate cache */
    __asm__ __volatile__("wbinvd\n\t");
}

/*
 * hal_cpu_cache_disable
 * ---------------------
 * Menonaktifkan cache CPU.
 */
void hal_cpu_cache_disable(void)
{
    tak_bertanda32_t cr0;

    cr0 = cpu_read_cr0();

    /* Set CD (cache disable) dan NW (not write-through) */
    cr0 |= (1 << 30) | (1 << 29);

    cpu_write_cr0(cr0);
}

/*
 * hal_cpu_cache_enable
 * --------------------
 * Mengaktifkan cache CPU.
 */
void hal_cpu_cache_enable(void)
{
    tak_bertanda32_t cr0;

    cr0 = cpu_read_cr0();

    /* Clear CD dan NW */
    cr0 &= ~((1 << 30) | (1 << 29));

    cpu_write_cr0(cr0);
}

/*
 * cpu_get_vendor
 * --------------
 * Mendapatkan string vendor CPU.
 *
 * Return:
 *   Pointer ke string vendor
 */
const char *cpu_get_vendor(void)
{
    return g_info_cpu.vendor;
}

/*
 * cpu_get_brand
 * -------------
 * Mendapatkan string brand CPU.
 *
 * Return:
 *   Pointer ke string brand
 */
const char *cpu_get_brand(void)
{
    return g_info_cpu.brand;
}

/*
 * cpu_has_feature
 * ---------------
 * Mengecek apakah CPU memiliki fitur tertentu.
 *
 * Parameter:
 *   fitur - Flag fitur yang dicek
 *
 * Return:
 *   BENAR jika fitur tersedia
 */
bool_t cpu_has_feature(tak_bertanda32_t fitur)
{
    return (g_info_cpu.features & fitur) ? BENAR : SALAH;
}

/*
 * cpu_get_frequency
 * -----------------
 * Mendapatkan frekuensi CPU dalam MHz.
 *
 * Return:
 *   Frekuensi CPU
 */
tak_bertanda32_t cpu_get_frequency(void)
{
    return g_info_cpu.freq_mhz;
}

/*
 * cpu_read_tsc
 * ------------
 * Membaca Time Stamp Counter.
 *
 * Return:
 *   Nilai TSC (64-bit)
 */
tak_bertanda64_t cpu_read_tsc(void)
{
    tak_bertanda32_t low;
    tak_bertanda32_t high;

    __asm__ __volatile__(
        "rdtsc\n\t"
        : "=a"(low), "=d"(high)
    );

    return ((tak_bertanda64_t)high << 32) | low;
}

/*
 * cpu_delay_us
 * ------------
 * Delay dalam mikrodetik menggunakan busy-wait.
 *
 * Parameter:
 *   us - Jumlah mikrodetik
 */
void cpu_delay_us(tak_bertanda32_t us)
{
    tak_bertanda64_t start;
    tak_bertanda64_t end;
    tak_bertanda64_t delay;

    if (g_info_cpu.freq_mhz == 0) {
        return;
    }

    start = cpu_read_tsc();
    delay = (tak_bertanda64_t)g_info_cpu.freq_mhz * us;

    do {
        end = cpu_read_tsc();
    } while ((end - start) < delay);
}

/*
 * cpu_delay_ms
 * ------------
 * Delay dalam milidetik menggunakan busy-wait.
 *
 * Parameter:
 *   ms - Jumlah milidetik
 */
void cpu_delay_ms(tak_bertanda32_t ms)
{
    cpu_delay_us(ms * 1000);
}
