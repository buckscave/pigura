/*
 * PIGURA OS - CPU.H
 * ==================
 * Header untuk manajemen CPU dan fitur-fitur prosesor.
 *
 * Berkas ini mendefinisikan interface untuk deteksi CPU,
 * manajemen cache, fitur SIMD, dan dukungan multi-core.
 *
 * Kepatuhan standar:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Mendukung semua arsitektur: x86, x86_64, ARM, ARMv7, ARM64
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#ifndef PERANGKAT_CPU_H
#define PERANGKAT_CPU_H

/*
 * ===========================================================================
 * INCLUDE
 * ===========================================================================
 */

#include "../../inti/types.h"
#include "../../inti/config.h"

/*
 * ===========================================================================
 * KONSTANTA CPU
 * ===========================================================================
 */

/* Versi modul CPU */
#define CPU_VERSI_MAJOR 1
#define CPU_VERSI_MINOR 0
#define CPU_VERSI_PATCH 0

/* Magic number untuk validasi */
#define CPU_MAGIC           0x43505500  /* "CPU\0" */
#define CPU_KONTEKS_MAGIC   0x43504B54  /* "CPKT" */
#define CPU_CORE_MAGIC      0x43504352  /* "CPCR" */

/* Batas sistem */
#define CPU_NAMA_PANJANG_MAKS      64
#define CPU_VENDOR_PANJANG_MAKS    32
#define CPU_SERI_PANJANG_MAKS      48
#define CPU_FEAT_PANJANG_MAKS      128
#define CPU_MAKS_CORE              256
#define CPU_MAKS_CACHE             16
#define CPU_MAKS_TOPOLOGI_LEVEL    8

/* Tipe cache */
#define CPU_CACHE_TIPE_TIDAK_ADA   0
#define CPU_CACHE_TIPE_L1_DATA     1
#define CPU_CACHE_TIPE_L1_INSTR    2
#define CPU_CACHE_TIPE_L1_UNIFIED  3
#define CPU_CACHE_TIPE_L2          4
#define CPU_CACHE_TIPE_L3          5
#define CPU_CACHE_TIPE_L4          6
#define CPU_CACHE_TIPE_TLB_DATA    7
#define CPU_CACHE_TIPE_TLB_INSTR   8
#define CPU_CACHE_TIPE_PREFETCH    9

/* Flag fitur CPU (bitmask) */
#define CPU_FEAT_FPU               0x00000001UL
#define CPU_FEAT_VME               0x00000002UL
#define CPU_FEAT_DE                0x00000004UL
#define CPU_FEAT_PSE               0x00000008UL
#define CPU_FEAT_TSC               0x00000010UL
#define CPU_FEAT_MSR               0x00000020UL
#define CPU_FEAT_PAE               0x00000040UL
#define CPU_FEAT_MCE               0x00000080UL
#define CPU_FEAT_CX8               0x00000100UL
#define CPU_FEAT_APIC              0x00000200UL
#define CPU_FEAT_SEP               0x00000800UL
#define CPU_FEAT_MTRR              0x00001000UL
#define CPU_FEAT_PGE               0x00002000UL
#define CPU_FEAT_MCA               0x00004000UL
#define CPU_FEAT_CMOV              0x00008000UL
#define CPU_FEAT_PAT               0x00010000UL
#define CPU_FEAT_PSE36             0x00020000UL
#define CPU_FEAT_PSN               0x00040000UL
#define CPU_FEAT_CLFSH             0x00080000UL
#define CPU_FEAT_DS                0x00200000UL
#define CPU_FEAT_ACPI              0x00400000UL
#define CPU_FEAT_MMX               0x00800000UL
#define CPU_FEAT_FXSR              0x01000000UL
#define CPU_FEAT_SSE               0x02000000UL
#define CPU_FEAT_SSE2              0x04000000UL
#define CPU_FEAT_SS                0x08000000UL
#define CPU_FEAT_HTT               0x10000000UL
#define CPU_FEAT_TM                0x20000000UL
#define CPU_FEAT_PBE               0x80000000UL

/* Flag fitur extended (bitmask 64-bit) */
#define CPU_FEAT_SSE3              0x0000000100000000ULL
#define CPU_FEAT_PCLMULQDQ         0x0000000200000000ULL
#define CPU_FEAT_DTES64            0x0000000400000000ULL
#define CPU_FEAT_MONITOR           0x0000000800000000ULL
#define CPU_FEAT_DS_CPL            0x0000001000000000ULL
#define CPU_FEAT_VMX               0x0000002000000000ULL
#define CPU_FEAT_SMX               0x0000004000000000ULL
#define CPU_FEAT_EST               0x0000008000000000ULL
#define CPU_FEAT_TM2               0x0000010000000000ULL
#define CPU_FEAT_SSSE3             0x0000020000000000ULL
#define CPU_FEAT_CID               0x0000040000000000ULL
#define CPU_FEAT_FMA               0x0000100000000000ULL
#define CPU_FEAT_CX16              0x0000200000000000ULL
#define CPU_FEAT_XTPR              0x0000400000000000ULL
#define CPU_FEAT_PDCM              0x0000800000000000ULL
#define CPU_FEAT_PCID              0x0002000000000000ULL
#define CPU_FEAT_DCA               0x0004000000000000ULL
#define CPU_FEAT_SSE4_1            0x0008000000000000ULL
#define CPU_FEAT_SSE4_2            0x0010000000000000ULL
#define CPU_FEAT_X2APIC            0x0020000000000000ULL
#define CPU_FEAT_MOVBE             0x0040000000000000ULL
#define CPU_FEAT_POPCNT            0x0080000000000000ULL
#define CPU_FEAT_TSC_DEADLINE      0x0100000000000000ULL
#define CPU_FEAT_AES               0x0200000000000000ULL
#define CPU_FEAT_XSAVE             0x0400000000000000ULL
#define CPU_FEAT_OSXSAVE           0x0800000000000000ULL
#define CPU_FEAT_AVX               0x1000000000000000ULL
#define CPU_FEAT_F16C              0x2000000000000000ULL
#define CPU_FEAT_RDRAND            0x4000000000000000ULL

/* Flag fitur ARM */
#define CPU_ARM_FEAT_VFP           0x00000001UL
#define CPU_ARM_FEAT_NEON          0x00000002UL
#define CPU_ARM_FEAT_VFPV3         0x00000004UL
#define CPU_ARM_FEAT_VFPV4         0x00000008UL
#define CPU_ARM_FEAT_IDIVA         0x00000010UL
#define CPU_ARM_FEAT_IDIVT         0x00000020UL
#define CPU_ARM_FEAT_AES           0x00000040UL
#define CPU_ARM_FEAT_PMULL         0x00000080UL
#define CPU_ARM_FEAT_SHA1          0x00000100UL
#define CPU_ARM_FEAT_SHA2          0x00000200UL
#define CPU_ARM_FEAT_CRC32         0x00000400UL
#define CPU_ARM_FEAT_ATOMIC        0x00000800UL
#define CPU_ARM_FEAT_FP16          0x00001000UL
#define CPU_ARM_FEAT_SVE           0x00002000UL

/* Vendor CPU */
typedef enum {
    CPU_VENDOR_TIDAK_DIKETAHUI = 0,
    CPU_VENDOR_INTEL           = 1,
    CPU_VENDOR_AMD             = 2,
    CPU_VENDOR_VIA             = 3,
    CPU_VENDOR_TRANSMETA       = 4,
    CPU_VENDOR_CYRIX           = 5,
    CPU_VENDOR_CENTAUR         = 6,
    CPU_VENDOR_NSC             = 7,
    CPU_VENDOR_RISE            = 8,
    CPU_VENDOR_SIS             = 9,
    CPU_VENDOR_UMC             = 10,
    CPU_VENDOR_NEXGEN          = 11,
    CPU_VENDOR_ARM             = 20,
    CPU_VENDOR_ARM_DEPRECATED  = 21,
    CPU_VENDOR_QUALCOMM        = 22,
    CPU_VENDOR_SAMSUNG         = 23,
    CPU_VENDOR_APPLE           = 24,
    CPU_VENDOR_NVIDIA          = 25,
    CPU_VENDOR_MARVELL         = 26,
    CPU_VENDOR_BROADCOM        = 27,
    CPU_VENDOR_MEDIATEK        = 28,
    CPU_VENDOR_HISILICON       = 29,
    CPU_VENDOR_ALLWINNER       = 30,
    CPU_VENDOR_ROCKCHIP        = 31,
    CPU_VENDOR_AMLOGIC         = 32
} cpu_vendor_t;

/* Tipe arsitektur */
typedef enum {
    CPU_ARSITEKTUR_TIDAK_DIKETAHUI = 0,
    CPU_ARSITEKTUR_X86             = 1,
    CPU_ARSITEKTUR_X86_64          = 2,
    CPU_ARSITEKTUR_ARMV6           = 3,
    CPU_ARSITEKTUR_ARMV7           = 4,
    CPU_ARSITEKTUR_ARMV7A          = 5,
    CPU_ARSITEKTUR_ARMV7R          = 6,
    CPU_ARSITEKTUR_ARMV7M          = 7,
    CPU_ARSITEKTUR_ARMV8A          = 8,
    CPU_ARSITEKTUR_ARMV8_1A        = 9,
    CPU_ARSITEKTUR_ARMV8_2A        = 10,
    CPU_ARSITEKTUR_ARMV8_3A        = 11,
    CPU_ARSITEKTUR_ARMV8_4A        = 12,
    CPU_ARSITEKTUR_ARMV9A          = 13
} cpu_arsitektur_t;

/* Status CPU */
typedef enum {
    CPU_STATUS_TIDAK_ADA       = 0,
    CPU_STATUS_TERDETEKSI     = 1,
    CPU_STATUS_DIIDENTIFIKASI = 2,
    CPU_STATUS_SIAP           = 3,
    CPU_STATUS_ERROR          = 4,
    CPU_STATUS_DISABLE        = 5
} cpu_status_t;

/* Level topologi */
typedef enum {
    CPU_TOPOLOGI_LEVEL_TIDAK_ADA = 0,
    CPU_TOPOLOGI_LEVEL_SMT       = 1,
    CPU_TOPOLOGI_LEVEL_CORE      = 2,
    CPU_TOPOLOGI_LEVEL_DIE       = 3,
    CPU_TOPOLOGI_LEVEL_SOCKET    = 4,
    CPU_TOPOLOGI_LEVEL_NUMA      = 5
} cpu_topologi_level_t;

/*
 * ===========================================================================
 * STRUKTUR CACHE
 * ===========================================================================
 */

typedef struct {
    tak_bertanda32_t tipe;          /* Tipe cache */
    tak_bertanda32_t level;         /* Level (1-4) */
    ukuran_t ukuran;                /* Ukuran dalam byte */
    ukuran_t line_size;             /* Ukuran cache line */
    tak_bertanda32_t ways;          /* Associativity */
    tak_bertanda32_t sets;          /* Jumlah sets */
    bool_t inclusive;               /* Apakah inclusive */
    bool_t write_back;              /* Apakah write-back */
} cpu_cache_t;

/*
 * ===========================================================================
 * STRUKTUR CORE CPU
 * ===========================================================================
 */

typedef struct {
    tak_bertanda32_t magic;         /* Magic number */
    tak_bertanda32_t id;            /* ID core */
    tak_bertanda32_t apic_id;       /* APIC ID (x86) */
    tak_bertanda32_t socket_id;     /* ID socket */
    tak_bertanda32_t die_id;        /* ID die */
    tak_bertanda32_t core_id;       /* ID core dalam socket */
    tak_bertanda32_t thread_id;     /* ID thread (SMT) */
    tak_bertanda32_t numa_node;     /* NUMA node */
    
    cpu_status_t status;            /* Status core */
    tak_bertanda32_t frekuensi_mhz; /* Frekuensi dalam MHz */
    tak_bertanda64_t fitur;         /* Feature flags */
    tak_bertanda64_t fitur_ext;     /* Extended feature flags */
    
    cpu_cache_t cache[CPU_MAKS_CACHE];
    tak_bertanda32_t jumlah_cache;
} cpu_core_t;

/*
 * ===========================================================================
 * STRUKTUR CPU UTAMA
 * ===========================================================================
 */

typedef struct {
    tak_bertanda32_t magic;         /* Magic number */
    
    /* Identifikasi */
    char nama[CPU_NAMA_PANJANG_MAKS];
    char vendor_str[CPU_VENDOR_PANJANG_MAKS];
    char seri[CPU_SERI_PANJANG_MAKS];
    cpu_vendor_t vendor;
    cpu_arsitektur_t arsitektur;
    
    /* Informasi keluarga */
    tak_bertanda32_t family;
    tak_bertanda32_t model;
    tak_bertanda32_t stepping;
    tak_bertanda32_t extended_family;
    tak_bertanda32_t extended_model;
    
    /* Fitur */
    tak_bertanda64_t fitur;         /* Feature flags */
    tak_bertanda64_t fitur_ext;     /* Extended feature flags */
    tak_bertanda32_t fitur_arm;     /* ARM feature flags */
    char fitur_str[CPU_FEAT_PANJANG_MAKS];
    
    /* Frekuensi */
    tak_bertanda32_t frekuensi_mhz;     /* Frekuensi nominal */
    tak_bertanda32_t frekuensi_max_mhz; /* Frekuensi maksimum */
    tak_bertanda32_t frekuensi_min_mhz; /* Frekuensi minimum */
    tak_bertanda32_t fsb_mhz;           /* Front-side bus */
    
    /* Cache */
    cpu_cache_t cache[CPU_MAKS_CACHE];
    tak_bertanda32_t jumlah_cache;
    ukuran_t cache_l1d;
    ukuran_t cache_l1i;
    ukuran_t cache_l2;
    ukuran_t cache_l3;
    
    /* Core */
    cpu_core_t core[CPU_MAKS_CORE];
    tak_bertanda32_t jumlah_core;
    tak_bertanda32_t jumlah_thread;
    tak_bertanda32_t jumlah_socket;
    tak_bertanda32_t jumlah_numa;
    
    /* TLB */
    ukuran_t tlb_data_4k;
    ukuran_t tlb_data_2m;
    ukuran_t tlb_data_4m;
    ukuran_t tlb_instr_4k;
    ukuran_t tlb_instr_2m;
    
    /* Status */
    cpu_status_t status;
    bool_t diinisialisasi;
    bool_t mendukung_64bit;
    bool_t mendukung_smp;
    bool_t mendukung_virtualisasi;
    
} cpu_info_t;

/*
 * ===========================================================================
 * STRUKTUR KONTEKS CPU
 * ===========================================================================
 */

typedef struct {
    tak_bertanda32_t magic;
    cpu_info_t info;
    
    /* Statistik */
    tak_bertanda64_t total_cycles;
    tak_bertanda64_t idle_cycles;
    tak_bertanda64_t jiffies;
    
    /* Status */
    bool_t diinisialisasi;
    bool_t smp_aktif;
    bool_t apic_aktif;
    
} cpu_konteks_t;

/*
 * ===========================================================================
 * VARIABEL GLOBAL
 * ===========================================================================
 */

extern cpu_konteks_t g_cpu_konteks;
extern bool_t g_cpu_diinisialisasi;

/*
 * ===========================================================================
 * FUNGSI INISIALISASI
 * ===========================================================================
 */

/*
 * cpu_init - Inisialisasi subsistem CPU
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t cpu_init(void);

/*
 * cpu_shutdown - Matikan subsistem CPU
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t cpu_shutdown(void);

/*
 * cpu_deteksi - Deteksi dan identifikasi CPU
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t cpu_deteksi(void);

/*
 * cpu_konteks_dapatkan - Dapatkan konteks CPU
 *
 * Return: Pointer ke konteks atau NULL
 */
cpu_konteks_t *cpu_konteks_dapatkan(void);

/*
 * cpu_info_dapatkan - Dapatkan informasi CPU
 *
 * Return: Pointer ke info atau NULL
 */
cpu_info_t *cpu_info_dapatkan(void);

/*
 * ===========================================================================
 * FUNGSI DETEKSI FITUR
 * ===========================================================================
 */

/*
 * cpu_fitur_cek - Cek apakah fitur tersedia
 *
 * Parameter:
 *   fitur - Flag fitur yang dicari
 *
 * Return: BENAR jika fitur tersedia
 */
bool_t cpu_fitur_cek(tak_bertanda64_t fitur);

/*
 * cpu_fitur_cek_ext - Cek apakah fitur extended tersedia
 *
 * Parameter:
 *   fitur - Flag fitur extended yang dicari
 *
 * Return: BENAR jika fitur tersedia
 */
bool_t cpu_fitur_cek_ext(tak_bertanda64_t fitur);

/*
 * cpu_fitur_string - Buat string fitur
 *
 * Parameter:
 *   buffer - Buffer output
 *   ukuran - Ukuran buffer
 *
 * Return: Panjang string
 */
tak_bertanda32_t cpu_fitur_string(char *buffer, ukuran_t ukuran);

/*
 * ===========================================================================
 * FUNGSI FREKUENSI
 * ===========================================================================
 */

/*
 * cpu_frekuensi_dapatkan - Dapatkan frekuensi CPU saat ini
 *
 * Return: Frekuensi dalam MHz
 */
tak_bertanda32_t cpu_frekuensi_dapatkan(void);

/*
 * cpu_frekuensi_max - Dapatkan frekuensi maksimum
 *
 * Return: Frekuensi maksimum dalam MHz
 */
tak_bertanda32_t cpu_frekuensi_max(void);

/*
 * cpu_frekuensi_min - Dapatkan frekuensi minimum
 *
 * Return: Frekuensi minimum dalam MHz
 */
tak_bertanda32_t cpu_frekuensi_min(void);

/*
 * cpu_tsc_frekuensi - Dapatkan frekuensi TSC
 *
 * Return: Frekuensi TSC dalam Hz
 */
tak_bertanda64_t cpu_tsc_frekuensi(void);

/*
 * ===========================================================================
 * FUNGSI CACHE
 * ===========================================================================
 */

/*
 * cpu_cache_info - Dapatkan informasi cache
 *
 * Parameter:
 *   level - Level cache (1-3)
 *   tipe  - Tipe cache (data/instr/unified)
 *
 * Return: Pointer ke cache atau NULL
 */
cpu_cache_t *cpu_cache_info(tak_bertanda32_t level, tak_bertanda32_t tipe);

/*
 * cpu_cache_flush - Flush cache
 *
 * Parameter:
 *   addr - Alamat awal (NULL untuk semua)
 *   ukuran - Ukuran region
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t cpu_cache_flush(void *addr, ukuran_t ukuran);

/*
 * cpu_cache_invalidate - Invalidate cache
 *
 * Parameter:
 *   addr - Alamat awal
 *   ukuran - Ukuran region
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t cpu_cache_invalidate(void *addr, ukuran_t ukuran);

/*
 * ===========================================================================
 * FUNGSI TSC/TIMER
 * ===========================================================================
 */

/*
 * cpu_tsc_baca - Baca Time Stamp Counter
 *
 * Return: Nilai TSC
 */
tak_bertanda64_t cpu_tsc_baca(void);

/*
 * cpu_tsc_delay - Delay menggunakan TSC
 *
 * Parameter:
 *   cycles - Jumlah cycles
 */
void cpu_tsc_delay(tak_bertanda64_t cycles);

/*
 * cpu_delay_us - Delay dalam microdetik
 *
 * Parameter:
 *   us - Jumlah microdetik
 */
void cpu_delay_us(tak_bertanda32_t us);

/*
 * cpu_delay_ms - Delay dalam milidetik
 *
 * Parameter:
 *   ms - Jumlah milidetik
 */
void cpu_delay_ms(tak_bertanda32_t ms);

/*
 * ===========================================================================
 * FUNGSI UTILITAS
 * ===========================================================================
 */

/*
 * cpu_vendor_nama - Dapatkan nama vendor
 *
 * Parameter:
 *   vendor - Kode vendor
 *
 * Return: String nama vendor
 */
const char *cpu_vendor_nama(cpu_vendor_t vendor);

/*
 * cpu_arsitektur_nama - Dapatkan nama arsitektur
 *
 * Parameter:
 *   arsitektur - Kode arsitektur
 *
 * Return: String nama arsitektur
 */
const char *cpu_arsitektur_nama(cpu_arsitektur_t arsitektur);

/*
 * cpu_status_nama - Dapatkan nama status
 *
 * Parameter:
 *   status - Kode status
 *
 * Return: String nama status
 */
const char *cpu_status_nama(cpu_status_t status);

/*
 * cpu_cetak_info - Cetak informasi CPU
 */
void cpu_cetak_info(void);

/*
 * cpu_cetak_fitur - Cetak daftar fitur
 */
void cpu_cetak_fitur(void);

/*
 * cpu_cetak_cache - Cetak informasi cache
 */
void cpu_cetak_cache(void);

/*
 * ===========================================================================
 * FUNGSI HAL (Hardware Abstraction)
 * ===========================================================================
 */

/*
 * cpu_halt - Hentikan CPU
 */
void cpu_halt(void);

/*
 * cpu_reset - Reset CPU
 *
 * Parameter:
 *   cold - BENAR untuk cold reset
 *
 * Return: Tidak return jika berhasil
 */
void cpu_reset(bool_t cold) TIDAK_RETURN;

/*
 * cpu_disable_irq - Disable interrupts
 */
void cpu_disable_irq(void);

/*
 * cpu_enable_irq - Enable interrupts
 */
void cpu_enable_irq(void);

/*
 * cpu_nop - No operation
 */
void cpu_nop(void);

/*
 * cpu_pause - Pause hint (spin-wait)
 */
void cpu_pause(void);

/*
 * ===========================================================================
 * FUNGSI SMP (Multi-core)
 * ===========================================================================
 */

/*
 * cpu_smp_init - Inisialisasi SMP
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t cpu_smp_init(void);

/*
 * cpu_smp_jumlah - Dapatkan jumlah CPU
 *
 * Return: Jumlah CPU online
 */
tak_bertanda32_t cpu_smp_jumlah(void);

/*
 * cpu_smp_id - Dapatkan ID CPU saat ini
 *
 * Return: ID CPU
 */
tak_bertanda32_t cpu_smp_id(void);

/*
 * cpu_smp_boot - Boot CPU tertentu
 *
 * Parameter:
 *   cpu_id - ID CPU
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t cpu_smp_boot(tak_bertanda32_t cpu_id);

/*
 * ===========================================================================
 * FUNGSI APIC
 * ===========================================================================
 */

/*
 * cpu_apic_init - Inisialisasi APIC
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t cpu_apic_init(void);

/*
 * cpu_apic_id - Dapatkan APIC ID
 *
 * Return: APIC ID
 */
tak_bertanda32_t cpu_apic_id(void);

/*
 * cpu_apic_enable - Enable APIC
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t cpu_apic_enable(void);

/*
 * ===========================================================================
 * FUNGSI SIMD (SSE/AVX/NEON)
 * ===========================================================================
 */

/*
 * cpu_fpu_init - Inisialisasi FPU
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t cpu_fpu_init(void);

/*
 * cpu_sse_init - Inisialisasi SSE
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t cpu_sse_init(void);

/*
 * cpu_avx_init - Inisialisasi AVX
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t cpu_avx_init(void);

/*
 * cpu_fpu_save - Simpan state FPU
 *
 * Parameter:
 *   buffer - Buffer untuk menyimpan state
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t cpu_fpu_save(void *buffer);

/*
 * cpu_fpu_restore - Restore state FPU
 *
 * Parameter:
 *   buffer - Buffer yang berisi state
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t cpu_fpu_restore(void *buffer);

/*
 * ===========================================================================
 * FUNGSI ACPI
 * ===========================================================================
 */

/*
 * cpu_acpi_init - Inisialisasi ACPI
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t cpu_acpi_init(void);

/*
 * cpu_acpi_shutdown - Shutdown via ACPI
 *
 * Return: Tidak return jika berhasil
 */
void cpu_acpi_shutdown(void) TIDAK_RETURN;

/*
 * cpu_acpi_reboot - Reboot via ACPI
 *
 * Return: Tidak return jika berhasil
 */
void cpu_acpi_reboot(void) TIDAK_RETURN;

/*
 * ===========================================================================
 * FUNGSI TOPOLOGI
 * ===========================================================================
 */

/*
 * cpu_topologi_init - Inisialisasi deteksi topologi
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t cpu_topologi_init(void);

/*
 * cpu_topologi_level - Dapatkan level topologi
 *
 * Parameter:
 *   cpu_id - ID CPU
 *   level  - Level topologi
 *
 * Return: ID pada level tersebut
 */
tak_bertanda32_t cpu_topologi_level(tak_bertanda32_t cpu_id,
                                     cpu_topologi_level_t level);

/*
 * ===========================================================================
 * FUNGSI CPUID (x86/x86_64)
 * ===========================================================================
 */

/*
 * cpu_cpuid - Eksekusi instruksi CPUID
 *
 * Parameter:
 *   leaf    - EAX input
 *   subleaf - ECX input (untuk leaf 0x04, 0x0B, 0x0D)
 *   eax     - Pointer untuk output EAX
 *   ebx     - Pointer untuk output EBX
 *   ecx     - Pointer untuk output ECX
 *   edx     - Pointer untuk output EDX
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t cpu_cpuid(tak_bertanda32_t leaf, tak_bertanda32_t subleaf,
                   tak_bertanda32_t *eax, tak_bertanda32_t *ebx,
                   tak_bertanda32_t *ecx, tak_bertanda32_t *edx);

/*
 * cpu_cpuid_string - Baca string CPUID
 *
 * Parameter:
 *   leaf   - Leaf CPUID (0x00 atau 0x80000000)
 *   buffer - Buffer output
 *   ukuran - Ukuran buffer
 *
 * Return: Panjang string
 */
tak_bertanda32_t cpu_cpuid_string(tak_bertanda32_t leaf, char *buffer,
                                   ukuran_t ukuran);

/*
 * ===========================================================================
 * FUNGSI MSR (Model Specific Registers)
 * ===========================================================================
 */

/*
 * cpu_msr_baca - Baca MSR
 *
 * Parameter:
 *   msr - Alamat MSR
 *   low - Pointer untuk low 32-bit
 *   high - Pointer untuk high 32-bit
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t cpu_msr_baca(tak_bertanda32_t msr, tak_bertanda32_t *low,
                       tak_bertanda32_t *high);

/*
 * cpu_msr_tulis - Tulis MSR
 *
 * Parameter:
 *   msr - Alamat MSR
 *   low - Low 32-bit
 *   high - High 32-bit
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t cpu_msr_tulis(tak_bertanda32_t msr, tak_bertanda32_t low,
                        tak_bertanda32_t high);

/*
 * cpu_msr_baca64 - Baca MSR 64-bit
 *
 * Parameter:
 *   msr - Alamat MSR
 *
 * Return: Nilai 64-bit
 */
tak_bertanda64_t cpu_msr_baca64(tak_bertanda32_t msr);

/*
 * cpu_msr_tulis64 - Tulis MSR 64-bit
 *
 * Parameter:
 *   msr - Alamat MSR
 *   nilai - Nilai 64-bit
 */
void cpu_msr_tulis64(tak_bertanda32_t msr, tak_bertanda64_t nilai);

#endif /* PERANGKAT_CPU_H */
