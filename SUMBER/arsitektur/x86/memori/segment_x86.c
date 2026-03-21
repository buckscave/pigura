/*
 * PIGURA OS - SEGMENT x86
 * -----------------------
 * Implementasi manajemen segmen untuk arsitektur x86.
 *
 * Berkas ini berisi fungsi-fungsi untuk mengelola segment register
 * dan selector pada arsitektur x86 dalam protected mode.
 *
 * Arsitektur: x86 (32-bit)
 * Versi: 1.0
 */

#include "../../../inti/kernel.h"

/*
 * ============================================================================
 * KONSTANTA SEGMENT
 * ============================================================================
 */

/* Selector */
#define SELECTOR_NULL          0x00
#define SELECTOR_KERNEL_KODE   0x08
#define SELECTOR_KERNEL_DATA   0x10
#define SELECTOR_USER_KODE     0x18
#define SELECTOR_USER_DATA     0x20
#define SELECTOR_TSS          0x28

/* RPL (Requestor Privilege Level) */
#define RPL_KERNEL             0
#define RPL_USER               3

/* TI (Table Indicator) */
#define TI_GDT                 0
#define TI_LDT                 1

/*
 * ============================================================================
 * STRUKTUR DATA
 * ============================================================================
 */

/* Segment selector bit-field */
typedef union {
    struct {
        tak_bertanda16_t rpl: 2;
        tak_bertanda16_t ti: 1;
        tak_bertanda16_t index: 13;
    } bit;
    tak_bertanda16_t nilai;
} seg_selector_t;

/*
 * ============================================================================
 * FUNGSI PUBLIC
 * ============================================================================
 */

/*
 * segment_init
 * ------------
 * Menginisialisasi segment register.
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t segment_init(void)
{
    /* Set segment register ke kernel data selector */
    __asm__ __volatile__(
        "movw %0, %%ax\n\t"
        "movw %%ax, %%ds\n\t"
        "movw %%ax, %%es\n\t"
        "movw %%ax, %%fs\n\t"
        "movw %%ax, %%gs\n\t"
        "movw %%ax, %%ss\n\t"
        :
        : "i"(SELECTOR_KERNEL_DATA)
        : "ax"
    );

    return STATUS_BERHASIL;
}

/*
 * segment_set_ds
 * --------------
 * Set data segment register.
 *
 * Parameter:
 *   selector - Nilai selector
 */
void segment_set_ds(tak_bertanda16_t selector)
{
    __asm__ __volatile__(
        "movw %0, %%ds\n\t"
        :
        : "r"(selector)
    );
}

/*
 * segment_set_es
 * --------------
 * Set extra segment register.
 *
 * Parameter:
 *   selector - Nilai selector
 */
void segment_set_es(tak_bertanda16_t selector)
{
    __asm__ __volatile__(
        "movw %0, %%es\n\t"
        :
        : "r"(selector)
    );
}

/*
 * segment_set_fs
 * --------------
 * Set FS segment register.
 *
 * Parameter:
 *   selector - Nilai selector
 */
void segment_set_fs(tak_bertanda16_t selector)
{
    __asm__ __volatile__(
        "movw %0, %%fs\n\t"
        :
        : "r"(selector)
    );
}

/*
 * segment_set_gs
 * --------------
 * Set GS segment register.
 *
 * Parameter:
 *   selector - Nilai selector
 */
void segment_set_gs(tak_bertanda16_t selector)
{
    __asm__ __volatile__(
        "movw %0, %%gs\n\t"
        :
        : "r"(selector)
    );
}

/*
 * segment_set_ss
 * --------------
 * Set stack segment register.
 *
 * Parameter:
 *   selector - Nilai selector
 */
void segment_set_ss(tak_bertanda16_t selector)
{
    __asm__ __volatile__(
        "movw %0, %%ss\n\t"
        :
        : "r"(selector)
    );
}

/*
 * segment_get_cs
 * --------------
 * Get code segment register.
 *
 * Return:
 *   Nilai CS
 */
tak_bertanda16_t segment_get_cs(void)
{
    tak_bertanda16_t cs;

    __asm__ __volatile__(
        "movw %%cs, %0\n\t"
        : "=r"(cs)
    );

    return cs;
}

/*
 * segment_get_ds
 * --------------
 * Get data segment register.
 *
 * Return:
 *   Nilai DS
 */
tak_bertanda16_t segment_get_ds(void)
{
    tak_bertanda16_t ds;

    __asm__ __volatile__(
        "movw %%ds, %0\n\t"
        : "=r"(ds)
    );

    return ds;
}

/*
 * segment_get_es
 * --------------
 * Get extra segment register.
 *
 * Return:
 *   Nilai ES
 */
tak_bertanda16_t segment_get_es(void)
{
    tak_bertanda16_t es;

    __asm__ __volatile__(
        "movw %%es, %0\n\t"
        : "=r"(es)
    );

    return es;
}

/*
 * segment_get_ss
 * --------------
 * Get stack segment register.
 *
 * Return:
 *   Nilai SS
 */
tak_bertanda16_t segment_get_ss(void)
{
    tak_bertanda16_t ss;

    __asm__ __volatile__(
        "movw %%ss, %0\n\t"
        : "=r"(ss)
    );

    return ss;
}

/*
 * segment_buat_selector
 * ---------------------
 * Membuat selector dari index dan RPL.
 *
 * Parameter:
 *   index - Index GDT/LDT
 *   ti    - Table indicator (0=GDT, 1=LDT)
 *   rpl   - Requestor privilege level
 *
 * Return:
 *   Nilai selector
 */
tak_bertanda16_t segment_buat_selector(tak_bertanda16_t index,
                                        tak_bertanda16_t ti,
                                        tak_bertanda16_t rpl)
{
    seg_selector_t sel;

    sel.bit.index = index & 0x1FFF;
    sel.bit.ti = ti & 0x01;
    sel.bit.rpl = rpl & 0x03;

    return sel.nilai;
}

/*
 * segment_get_index
 * -----------------
 * Mendapatkan index dari selector.
 *
 * Parameter:
 *   selector - Nilai selector
 *
 * Return:
 *   Index GDT/LDT
 */
tak_bertanda16_t segment_get_index(tak_bertanda16_t selector)
{
    seg_selector_t sel;

    sel.nilai = selector;

    return sel.bit.index;
}

/*
 * segment_get_rpl
 * ---------------
 * Mendapatkan RPL dari selector.
 *
 * Parameter:
 *   selector - Nilai selector
 *
 * Return:
 *   RPL
 */
tak_bertanda16_t segment_get_rpl(tak_bertanda16_t selector)
{
    seg_selector_t sel;

    sel.nilai = selector;

    return sel.bit.rpl;
}

/*
 * segment_get_ti
 * --------------
 * Mendapatkan table indicator dari selector.
 *
 * Parameter:
 *   selector - Nilai selector
 *
 * Return:
 *   TI (0=GDT, 1=LDT)
 */
tak_bertanda16_t segment_get_ti(tak_bertanda16_t selector)
{
    seg_selector_t sel;

    sel.nilai = selector;

    return sel.bit.ti;
}

/*
 * segment_set_kernel
 * ------------------
 * Set semua segment register ke kernel selector.
 */
void segment_set_kernel(void)
{
    segment_set_ds(SELECTOR_KERNEL_DATA);
    segment_set_es(SELECTOR_KERNEL_DATA);
    segment_set_fs(SELECTOR_KERNEL_DATA);
    segment_set_gs(SELECTOR_KERNEL_DATA);
    segment_set_ss(SELECTOR_KERNEL_DATA);
}

/*
 * segment_set_user
 * ----------------
 * Set segment register ke user selector.
 *
 * Parameter:
 *   kode - Selector kode
 *   data - Selector data
 */
void segment_set_user(tak_bertanda16_t kode, tak_bertanda16_t data)
{
    segment_set_ds(data);
    segment_set_es(data);
    segment_set_fs(data);
    segment_set_gs(data);
    segment_set_ss(data);
}

/*
 * segment_get_kernel_kode
 * -----------------------
 * Mendapatkan selector kode kernel.
 *
 * Return:
 *   Selector kode kernel
 */
tak_bertanda16_t segment_get_kernel_kode(void)
{
    return SELECTOR_KERNEL_KODE;
}

/*
 * segment_get_kernel_data
 * -----------------------
 * Mendapatkan selector data kernel.
 *
 * Return:
 *   Selector data kernel
 */
tak_bertanda16_t segment_get_kernel_data(void)
{
    return SELECTOR_KERNEL_DATA;
}

/*
 * segment_get_user_kode
 * ---------------------
 * Mendapatkan selector kode user.
 *
 * Return:
 *   Selector kode user
 */
tak_bertanda16_t segment_get_user_kode(void)
{
    return SELECTOR_USER_KODE;
}

/*
 * segment_get_user_data
 * ---------------------
 * Mendapatkan selector data user.
 *
 * Return:
 *   Selector data user
 */
tak_bertanda16_t segment_get_user_data(void)
{
    return SELECTOR_USER_DATA;
}
