/*
 * PIGURA OS - TSS x86
 * --------------------
 * Implementasi Task State Segment untuk arsitektur x86.
 *
 * Berkas ini berisi fungsi-fungsi untuk mengelola TSS yang
 * diperlukan untuk task switching dan privilege level switching.
 *
 * Arsitektur: x86 (32-bit)
 * Versi: 1.0
 */

#include "../../../inti/kernel.h"

/*
 * ============================================================================
 * KONSTANTA TSS
 * ============================================================================
 */

/* Ukuran TSS minimal */
#define TSS_UKURAN_MIN          104

/* Selector TSS */
#define TSS_SELECTOR            0x28

/* TSS type */
#define TSS_TYPE_AVAILABLE      0x89
#define TSS_TYPE_BUSY           0x8B

/*
 * ============================================================================
 * STRUKTUR TSS
 * ============================================================================
 */

/* Task State Segment (104 byte) */
struct tss {
    tak_bertanda16_t link;          /* 0:  Previous task link */
    tak_bertanda16_t link_h;

    tak_bertanda32_t esp0;          /* 4:  ESP ring 0 */
    tak_bertanda16_t ss0;           /* 8:  SS ring 0 */
    tak_bertanda16_t ss0_h;

    tak_bertanda32_t esp1;          /* 12: ESP ring 1 */
    tak_bertanda16_t ss1;           /* 16: SS ring 1 */
    tak_bertanda16_t ss1_h;

    tak_bertanda32_t esp2;          /* 20: ESP ring 2 */
    tak_bertanda16_t ss2;           /* 24: SS ring 2 */
    tak_bertanda16_t ss2_h;

    tak_bertanda32_t cr3;           /* 28: CR3 (page directory) */
    tak_bertanda32_t eip;           /* 32: EIP */
    tak_bertanda32_t eflags;        /* 36: EFLAGS */

    tak_bertanda32_t eax;           /* 40: EAX */
    tak_bertanda32_t ecx;           /* 44: ECX */
    tak_bertanda32_t edx;           /* 48: EDX */
    tak_bertanda32_t ebx;           /* 52: EBX */

    tak_bertanda32_t esp;           /* 56: ESP */
    tak_bertanda32_t ebp;           /* 60: EBP */
    tak_bertanda32_t esi;           /* 64: ESI */
    tak_bertanda32_t edi;           /* 68: EDI */

    tak_bertanda16_t es;            /* 72: ES */
    tak_bertanda16_t es_h;

    tak_bertanda16_t cs;            /* 74: CS */
    tak_bertanda16_t cs_h;

    tak_bertanda16_t ss;            /* 76: SS */
    tak_bertanda16_t ss_h;

    tak_bertanda16_t ds;            /* 78: DS */
    tak_bertanda16_t ds_h;

    tak_bertanda16_t fs;            /* 80: FS */
    tak_bertanda16_t fs_h;

    tak_bertanda16_t gs;            /* 82: GS */
    tak_bertanda16_t gs_h;

    tak_bertanda16_t ldt;           /* 84: LDT selector */
    tak_bertanda16_t ldt_h;

    tak_bertanda16_t trap;          /* 86: Trap flag */
    tak_bertanda16_t iomap_base;    /* 88: I/O map base address */
} __attribute__((packed));

/*
 * ============================================================================
 * VARIABEL GLOBAL
 * ============================================================================
 */

/* TSS kernel */
static struct tss g_tss_kernel
    __attribute__((aligned(4)));

/* TSS terinisialisasi */
static bool_t g_tss_diinisialisasi = SALAH;

/*
 * ============================================================================
 * FUNGSI PUBLIC
 * ============================================================================
 */

/*
 * tss_init
 * --------
 * Menginisialisasi TSS kernel.
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t tss_init(void)
{
    kernel_memset(&g_tss_kernel, 0, sizeof(struct tss));

    /* Set I/O map base di luar TSS (tidak ada I/O bitmap) */
    g_tss_kernel.iomap_base = sizeof(struct tss);

    /* Set SS0 ke kernel data selector */
    g_tss_kernel.ss0 = GDT_SELECTOR_DATA;

    /* ESP0 akan diset saat switch ke user mode */
    g_tss_kernel.esp0 = 0;

    /* Set CR3 (akan diset nanti) */
    g_tss_kernel.cr3 = 0;

    /* Set CS ke kernel code selector */
    g_tss_kernel.cs = GDT_SELECTOR_KODE;

    /* Set segment register */
    g_tss_kernel.ds = GDT_SELECTOR_DATA;
    g_tss_kernel.es = GDT_SELECTOR_DATA;
    g_tss_kernel.fs = GDT_SELECTOR_DATA;
    g_tss_kernel.gs = GDT_SELECTOR_DATA;
    g_tss_kernel.ss = GDT_SELECTOR_DATA;

    /* Link ke NULL */
    g_tss_kernel.link = 0;

    g_tss_diinisialisasi = BENAR;

    kernel_printf("[TSS] TSS kernel diinisialisasi\n");

    return STATUS_BERHASIL;
}

/*
 * tss_load
 * --------
 * Memuat TSS ke CPU menggunakan TR (Task Register).
 */
void tss_load(void)
{
    tak_bertanda16_t selector;

    selector = TSS_SELECTOR;

    __asm__ __volatile__(
        "ltr %0\n\t"
        :
        : "r"(selector)
    );
}

/*
 * tss_set_esp0
 * ------------
 * Mengatur ESP0 (stack pointer ring 0).
 *
 * Parameter:
 *   esp0 - Alamat stack ring 0
 */
void tss_set_esp0(tak_bertanda32_t esp0)
{
    g_tss_kernel.esp0 = esp0;
}

/*
 * tss_get_esp0
 * ------------
 * Mendapatkan ESP0.
 *
 * Return:
 *   Alamat stack ring 0
 */
tak_bertanda32_t tss_get_esp0(void)
{
    return g_tss_kernel.esp0;
}

/*
 * tss_set_cr3
 * -----------
 * Mengatur CR3 (page directory).
 *
 * Parameter:
 *   cr3 - Alamat page directory
 */
void tss_set_cr3(tak_bertanda32_t cr3)
{
    g_tss_kernel.cr3 = cr3;
}

/*
 * tss_get_cr3
 * -----------
 * Mendapatkan CR3.
 *
 * Return:
 *   Alamat page directory
 */
tak_bertanda32_t tss_get_cr3(void)
{
    return g_tss_kernel.cr3;
}

/*
 * tss_set_ss0
 * -----------
 * Mengatur SS0 (stack segment ring 0).
 *
 * Parameter:
 *   ss0 - Selector stack segment
 */
void tss_set_ss0(tak_bertanda16_t ss0)
{
    g_tss_kernel.ss0 = ss0;
}

/*
 * tss_get
 * -------
 * Mendapatkan pointer ke TSS.
 *
 * Return:
 *   Pointer ke TSS kernel
 */
void *tss_get(void)
{
    return &g_tss_kernel;
}

/*
 * tss_get_size
 * ------------
 * Mendapatkan ukuran TSS.
 *
 * Return:
 *   Ukuran TSS dalam byte
 */
ukuran_t tss_get_size(void)
{
    return sizeof(struct tss);
}

/*
 * tss_flush
 * ---------
 * Flush/reload TR.
 */
void tss_flush(void)
{
    __asm__ __volatile__(
        "ltr %0\n\t"
        :
        : "r"((tak_bertanda16_t)TSS_SELECTOR)
    );
}

/*
 * tss_set_iomap_base
 * ------------------
 * Mengatur base I/O permission map.
 *
 * Parameter:
 *   base - Offset I/O map
 */
void tss_set_iomap_base(tak_bertanda16_t base)
{
    g_tss_kernel.iomap_base = base;
}

/*
 * tss_set_io_permission
 * ---------------------
 * Mengatur permission untuk port I/O.
 *
 * Parameter:
 *   port       - Nomor port
 *   permission - Permission (0=allow, 1=block)
 */
void tss_set_io_permission(tak_bertanda16_t port, bool_t permission)
{
    tak_bertanda8_t *iomap;
    tak_bertanda32_t byte_offset;
    tak_bertanda32_t bit_offset;

    if (!g_tss_diinisialisasi) {
        return;
    }

    /* I/O map dimulai setelah TSS */
    iomap = (tak_bertanda8_t *)(&g_tss_kernel) + 
            g_tss_kernel.iomap_base;

    byte_offset = port / 8;
    bit_offset = port % 8;

    if (permission) {
        /* Set bit (block) */
        iomap[byte_offset] |= (1 << bit_offset);
    } else {
        /* Clear bit (allow) */
        iomap[byte_offset] &= ~(1 << bit_offset);
    }
}

/*
 * tss_enable_io
 * -------------
 * Mengaktifkan akses I/O untuk semua port.
 */
void tss_enable_io(void)
{
    tak_bertanda32_t i;
    tak_bertanda8_t *iomap;

    if (!g_tss_diinisialisasi) {
        return;
    }

    iomap = (tak_bertanda8_t *)(&g_tss_kernel) + 
            g_tss_kernel.iomap_base;

    /* Clear semua bit (allow all) */
    for (i = 0; i < 8192; i++) {
        iomap[i] = 0x00;
    }
}

/*
 * tss_disable_io
 * --------------
 * Menonaktifkan akses I/O untuk semua port.
 */
void tss_disable_io(void)
{
    tak_bertanda32_t i;
    tak_bertanda8_t *iomap;

    if (!g_tss_diinisialisasi) {
        return;
    }

    iomap = (tak_bertanda8_t *)(&g_tss_kernel) + 
            g_tss_kernel.iomap_base;

    /* Set semua bit (block all) */
    for (i = 0; i < 8192; i++) {
        iomap[i] = 0xFF;
    }
}
