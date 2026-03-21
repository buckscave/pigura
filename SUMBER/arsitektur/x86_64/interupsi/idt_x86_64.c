/*
 * PIGURA OS - IDT x86_64
 * -----------------------
 * Implementasi Interrupt Descriptor Table untuk x86_64.
 *
 * Berkas ini berisi implementasi fungsi-fungsi untuk setup dan
 * manajemen IDT pada arsitektur x86_64.
 *
 * Arsitektur: x86_64 (64-bit)
 * Versi: 1.0
 */

#include "../../inti/kernel.h"

/*
 * ============================================================================
 * KONSTANTA IDT
 * ============================================================================
 */

/* Jumlah entri IDT */
#define IDT_ENTRI_JUMLAH       256

/* Flag IDT */
#define IDT_PRESENT            0x80
#define IDT_DPL_0              0x00
#define IDT_DPL_3              0x60
#define IDT_TYPE_INTERRUPT     0x0E
#define IDT_TYPE_TRAP          0x0F

/*
 * ============================================================================
 * STRUKTUR DATA
 * ============================================================================
 */

/* Entry IDT x86_64 (16 byte) */
struct idt_entry_x86_64 {
    tak_bertanda16_t offset_bawah;  /* Offset 15:0 */
    tak_bertanda16_t selector;      /* Segment selector */
    tak_bertanda8_t  ist;           /* IST (Interrupt Stack Table) */
    tak_bertanda8_t  type_attr;     /* Type dan attributes */
    tak_bertanda16_t offset_tengah; /* Offset 31:16 */
    tak_bertanda32_t offset_atas;   /* Offset 63:32 */
    tak_bertanda32_t reserved;      /* Reserved, harus 0 */
} __attribute__((packed));

/* Register IDT */
struct idt_ptr_x86_64 {
    tak_bertanda16_t limit;         /* Batas (ukuran - 1) */
    tak_bertanda64_t base;          /* Alamat base IDT */
} __attribute__((packed));

/*
 * ============================================================================
 * VARIABEL GLOBAL
 * ============================================================================
 */

/* IDT */
static struct idt_entry_x86_64 g_idt[IDT_ENTRI_JUMLAH]
    __attribute__((aligned(16)));

/* Pointer IDT */
static struct idt_ptr_x86_64 g_idt_ptr;

/* Flag inisialisasi */
static bool_t g_idt_diinisialisasi = SALAH;

/*
 * ============================================================================
 * FUNGSI INTERNAL
 * ============================================================================
 */

/*
 * _idt_set_entry
 * --------------
 * Mengatur satu entry IDT.
 *
 * Parameter:
 *   index    - Index entry (0-255)
 *   handler  - Alamat handler (64-bit)
 *   selector - Segment selector
 *   type     - Tipe gate (interrupt/trap)
 *   dpl      - Descriptor Privilege Level
 */
static void _idt_set_entry(tak_bertanda32_t index,
                            tak_bertanda64_t handler,
                            tak_bertanda16_t selector,
                            tak_bertanda8_t type,
                            tak_bertanda8_t dpl)
{
    struct idt_entry_x86_64 *entry;

    if (index >= IDT_ENTRI_JUMLAH) {
        return;
    }

    entry = &g_idt[index];

    entry->offset_bawah = (tak_bertanda16_t)(handler & 0xFFFF);
    entry->offset_tengah = (tak_bertanda16_t)((handler >> 16) & 0xFFFF);
    entry->offset_atas = (tak_bertanda32_t)((handler >> 32) & 0xFFFFFFFF);
    entry->selector = selector;
    entry->ist = 0;
    entry->type_attr = IDT_PRESENT | dpl | type;
    entry->reserved = 0;
}

/*
 * _idt_load
 * ---------
 * Memuat IDT ke register IDTR.
 */
static void _idt_load(void)
{
    g_idt_ptr.limit = (tak_bertanda16_t)(sizeof(g_idt) - 1);
    g_idt_ptr.base = (tak_bertanda64_t)&g_idt;

    __asm__ __volatile__(
        "lidt %0\n\t"
        :
        : "m"(g_idt_ptr)
    );
}

/*
 * ============================================================================
 * FUNGSI PUBLIC
 * ============================================================================
 */

/*
 * idt_x86_64_init
 * ---------------
 * Inisialisasi IDT untuk x86_64.
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t idt_x86_64_init(void)
{
    kernel_printf("[IDT-x86_64] Menginisialisasi IDT...\n");

    /* Clear IDT */
    kernel_memset(&g_idt, 0, sizeof(g_idt));

    /* Load IDT */
    _idt_load();

    g_idt_diinisialisasi = BENAR;

    kernel_printf("[IDT-x86_64] IDT siap (256 entri)\n");

    return STATUS_BERHASIL;
}

/*
 * idt_x86_64_set_handler
 * ----------------------
 * Mengatur handler untuk vektor interupsi tertentu.
 *
 * Parameter:
 *   vector   - Nomor vektor (0-255)
 *   handler  - Alamat handler
 *   type     - Tipe gate
 *   dpl      - Descriptor Privilege Level
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t idt_x86_64_set_handler(tak_bertanda32_t vector,
                                 tak_bertanda64_t handler,
                                 tak_bertanda8_t type,
                                 tak_bertanda8_t dpl)
{
    if (vector >= IDT_ENTRI_JUMLAH) {
        return STATUS_PARAM_INVALID;
    }

    _idt_set_entry(vector, handler, SELECTOR_KERNEL_CODE, type, dpl);

    return STATUS_BERHASIL;
}

/*
 * idt_x86_64_set_interrupt
 * ------------------------
 * Mengatur interrupt gate (DPL 0).
 *
 * Parameter:
 *   vector  - Nomor vektor
 *   handler - Alamat handler
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t idt_x86_64_set_interrupt(tak_bertanda32_t vector,
                                   tak_bertanda64_t handler)
{
    return idt_x86_64_set_handler(vector, handler, IDT_TYPE_INTERRUPT,
                                   IDT_DPL_0);
}

/*
 * idt_x86_64_set_trap
 * -------------------
 * Mengatur trap gate (DPL 0).
 *
 * Parameter:
 *   vector  - Nomor vektor
 *   handler - Alamat handler
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t idt_x86_64_set_trap(tak_bertanda32_t vector,
                              tak_bertanda64_t handler)
{
    return idt_x86_64_set_handler(vector, handler, IDT_TYPE_TRAP,
                                   IDT_DPL_0);
}

/*
 * idt_x86_64_set_user_interrupt
 * -----------------------------
 * Mengatur interrupt gate (DPL 3 - user accessible).
 *
 * Parameter:
 *   vector  - Nomor vektor
 *   handler - Alamat handler
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t idt_x86_64_set_user_interrupt(tak_bertanda32_t vector,
                                        tak_bertanda64_t handler)
{
    return idt_x86_64_set_handler(vector, handler, IDT_TYPE_INTERRUPT,
                                   IDT_DPL_3);
}

/*
 * idt_x86_64_clear
 * ----------------
 * Menghapus handler untuk vektor tertentu.
 *
 * Parameter:
 *   vector - Nomor vektor
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t idt_x86_64_clear(tak_bertanda32_t vector)
{
    if (vector >= IDT_ENTRI_JUMLAH) {
        return STATUS_PARAM_INVALID;
    }

    kernel_memset(&g_idt[vector], 0, sizeof(struct idt_entry_x86_64));

    return STATUS_BERHASIL;
}

/*
 * idt_x86_64_reload
 * -----------------
 * Memuat ulang IDT.
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t idt_x86_64_reload(void)
{
    if (!g_idt_diinisialisasi) {
        return STATUS_GAGAL;
    }

    _idt_load();

    return STATUS_BERHASIL;
}

/*
 * idt_x86_64_get_base
 * -------------------
 * Mendapatkan alamat base IDT.
 *
 * Return:
 *   Alamat base IDT
 */
tak_bertanda64_t idt_x86_64_get_base(void)
{
    return (tak_bertanda64_t)&g_idt;
}

/*
 * idt_x86_64_get_limit
 * --------------------
 * Mendapatkan batas IDT.
 *
 * Return:
 *   Batas IDT
 */
tak_bertanda16_t idt_x86_64_get_limit(void)
{
    return (tak_bertanda16_t)(sizeof(g_idt) - 1);
}

/*
 * idt_x86_64_apakah_siap
 * ----------------------
 * Mengecek apakah IDT sudah diinisialisasi.
 *
 * Return:
 *   BENAR jika sudah, SALAH jika belum
 */
bool_t idt_x86_64_apakah_siap(void)
{
    return g_idt_diinisialisasi;
}
