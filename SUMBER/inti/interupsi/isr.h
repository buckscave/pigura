/*
 * PIGURA OS - ISR.H
 * ==================
 * Deklarasi Interrupt Service Routine (ISR).
 *
 * Berkas ini berisi deklarasi fungsi-fungsi untuk manajemen ISR,
 * termasuk pendaftaran handler, pengelolaan vector, dan
 * operasi terkait interupsi lainnya.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Tidak menggunakan fitur C99/C11
 * - Semua fungsi dideklarasikan secara eksplisit
 *
 * Versi: 2.0
 * Tanggal: 2025
 */

#ifndef INTI_INTERUPSI_ISR_H
#define INTI_INTERUPSI_ISR_H

/*
 * ===========================================================================
 * INCLUDE
 * ===========================================================================
 */

#include "../types.h"
#include "../konstanta.h"
#include "../panic.h"

/*
 * ===========================================================================
 * KONSTANTA ISR (ISR CONSTANTS)
 * ===========================================================================
 */

/* Jumlah total vector ISR */
#define ISR_JUMLAH_VECTOR JUMLAH_ISR

/* Jumlah exception */
#define ISR_JUMLAH_EXCEPTION JUMLAH_EXCEPTION

/* Jumlah IRQ */
#define ISR_JUMLAH_IRQ JUMLAH_IRQ

/* Flag ISR handler */
#define ISR_FLAG_RESERVED 0x01
#define ISR_FLAG_EXCEPTION 0x02
#define ISR_FLAG_IRQ 0x04
#define ISR_FLAG_SYSCALL 0x08
#define ISR_FLAG_USER 0x10
#define ISR_FLAG_TRAP 0x20
#define ISR_FLAG_ACTIVE 0x40

/* Prioritas ISR */
#define ISR_PRIORITAS_RENDAH 0
#define ISR_PRIORITAS_NORMAL 1
#define ISR_PRIORITAS_TINGGI 2
#define ISR_PRIORITAS_KRITIS 3

/*
 * ===========================================================================
 * TIPE DATA ISR (ISR DATA TYPES)
 * ===========================================================================
 */

/* Tipe handler ISR */
typedef void (*isr_handler_t)(register_context_t *ctx);

/* Tipe handler IRQ */
typedef void (*irq_handler_t)(tak_bertanda32_t irq, register_context_t *ctx);

/* Struktur deskriptor ISR */
typedef struct {
    isr_handler_t handler;          /* Handler function */
    tak_bertanda32_t hitung;        /* Counter interrupt */
    tak_bertanda8_t bendera;        /* Flags */
    tak_bertanda8_t prioritas;      /* Priority level */
    const char *nama;               /* Handler name */
    void *data;                     /* User data */
} isr_deskriptor_t;

/* Struktur tabel ISR */
typedef struct {
    isr_deskriptor_t entri[ISR_JUMLAH_VECTOR];
    tak_bertanda32_t total_hitung;
    tak_bertanda32_t exception_hitung;
    tak_bertanda32_t irq_hitung;
} isr_tabel_t;

/*
 * ===========================================================================
 * DEKLARASI FUNGSI INISIALISASI (INITIALIZATION FUNCTIONS)
 * ===========================================================================
 */

/*
 * isr_init
 * --------
 * Inisialisasi sistem ISR.
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   Status error jika gagal
 */
status_t isr_init(void);

/*
 * isr_reset
 * ---------
 * Reset semua handler ISR ke default.
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t isr_reset(void);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI HANDLER (HANDLER FUNCTIONS)
 * ===========================================================================
 */

/*
 * isr_set_handler
 * ---------------
 * Set handler untuk vector ISR tertentu.
 *
 * Parameter:
 *   vector  - Nomor vector (0-255)
 *   handler - Fungsi handler
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   STATUS_PARAM_JARAK jika vector tidak valid
 */
status_t isr_set_handler(tak_bertanda32_t vector, isr_handler_t handler);

/*
 * isr_get_handler
 * ---------------
 * Dapatkan handler untuk vector ISR tertentu.
 *
 * Parameter:
 *   vector - Nomor vector
 *
 * Return:
 *   Pointer ke handler, atau NULL jika tidak ada
 */
isr_handler_t isr_get_handler(tak_bertanda32_t vector);

/*
 * isr_clear_handler
 * -----------------
 * Hapus handler untuk vector ISR tertentu.
 *
 * Parameter:
 *   vector - Nomor vector
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t isr_clear_handler(tak_bertanda32_t vector);

/*
 * isr_set_handler_cek
 * -------------------
 * Set handler dengan pengecekan jika sudah ada.
 *
 * Parameter:
 *   vector  - Nomor vector
 *   handler - Fungsi handler
 *   force   - Jika BENAR, overwrite handler yang ada
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   STATUS_SUDAH_ADA jika sudah ada handler dan force=SALAH
 */
status_t isr_set_handler_cek(tak_bertanda32_t vector, isr_handler_t handler,
                              bool_t force);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI IRQ (IRQ FUNCTIONS)
 * ===========================================================================
 */

/*
 * isr_irq_set_handler
 * -------------------
 * Set handler untuk IRQ tertentu.
 *
 * Parameter:
 *   irq     - Nomor IRQ (0-15 atau 0-23 untuk APIC)
 *   handler - Fungsi handler
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t isr_irq_set_handler(tak_bertanda32_t irq, irq_handler_t handler);

/*
 * isr_irq_get_handler
 * -------------------
 * Dapatkan handler untuk IRQ tertentu.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return:
 *   Pointer ke handler, atau NULL jika tidak ada
 */
irq_handler_t isr_irq_get_handler(tak_bertanda32_t irq);

/*
 * isr_irq_clear_handler
 * ---------------------
 * Hapus handler untuk IRQ tertentu.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t isr_irq_clear_handler(tak_bertanda32_t irq);

/*
 * isr_irq_to_vector
 * -----------------
 * Konversi IRQ ke vector number.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return:
 *   Nomor vector
 */
tak_bertanda32_t isr_irq_to_vector(tak_bertanda32_t irq);

/*
 * isr_vector_to_irq
 * -----------------
 * Konversi vector ke IRQ number.
 *
 * Parameter:
 *   vector - Nomor vector
 *
 * Return:
 *   Nomor IRQ, atau -1 jika bukan IRQ vector
 */
tanda32_t isr_vector_to_irq(tak_bertanda32_t vector);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI DISPATCHER (DISPATCHER FUNCTIONS)
 * ===========================================================================
 */

/*
 * isr_dispatch
 * ------------
 * Dispatch interrupt ke handler yang sesuai.
 * Dipanggil dari assembly entry point.
 *
 * Parameter:
 *   ctx - Pointer ke konteks register
 */
void isr_dispatch(register_context_t *ctx);

/*
 * isr_exception_dispatch
 * ----------------------
 * Dispatch exception ke handler yang sesuai.
 *
 * Parameter:
 *   ctx - Pointer ke konteks register
 */
void isr_exception_dispatch(register_context_t *ctx);

/*
 * isr_irq_dispatch
 * ----------------
 * Dispatch IRQ ke handler yang sesuai.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *   ctx - Pointer ke konteks register
 */
void isr_irq_dispatch(tak_bertanda32_t irq, register_context_t *ctx);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI COUNTER (COUNTER FUNCTIONS)
 * ===========================================================================
 */

/*
 * isr_get_count
 * -------------
 * Dapatkan jumlah interrupt untuk vector tertentu.
 *
 * Parameter:
 *   vector - Nomor vector
 *
 * Return:
 *   Jumlah interrupt
 */
tak_bertanda64_t isr_get_count(tak_bertanda32_t vector);

/*
 * isr_get_total_count
 * -------------------
 * Dapatkan total jumlah interrupt.
 *
 * Return:
 *   Total jumlah interrupt
 */
tak_bertanda64_t isr_get_total_count(void);

/*
 * isr_get_exception_count
 * -----------------------
 * Dapatkan total jumlah exception.
 *
 * Return:
 *   Total jumlah exception
 */
tak_bertanda64_t isr_get_exception_count(void);

/*
 * isr_get_irq_count
 * -----------------
 * Dapatkan total jumlah IRQ.
 *
 * Return:
 *   Total jumlah IRQ
 */
tak_bertanda64_t isr_get_irq_count(void);

/*
 * isr_reset_count
 * ---------------
 * Reset counter untuk vector tertentu.
 *
 * Parameter:
 *   vector - Nomor vector
 */
void isr_reset_count(tak_bertanda32_t vector);

/*
 * isr_reset_all_count
 * -------------------
 * Reset semua counter.
 */
void isr_reset_all_count(void);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI UTILITY (UTILITY FUNCTIONS)
 * ===========================================================================
 */

/*
 * isr_vector_is_valid
 * -------------------
 * Cek apakah vector valid.
 *
 * Parameter:
 *   vector - Nomor vector
 *
 * Return:
 *   BENAR jika valid
 */
bool_t isr_vector_is_valid(tak_bertanda32_t vector);

/*
 * isr_vector_is_exception
 * -----------------------
 * Cek apakah vector adalah exception.
 *
 * Parameter:
 *   vector - Nomor vector
 *
 * Return:
 *   BENAR jika exception
 */
bool_t isr_vector_is_exception(tak_bertanda32_t vector);

/*
 * isr_vector_is_irq
 * -----------------
 * Cek apakah vector adalah IRQ.
 *
 * Parameter:
 *   vector - Nomor vector
 *
 * Return:
 *   BENAR jika IRQ
 */
bool_t isr_vector_is_irq(tak_bertanda32_t vector);

/*
 * isr_vector_is_reserved
 * ----------------------
 * Cek apakah vector reserved.
 *
 * Parameter:
 *   vector - Nomor vector
 *
 * Return:
 *   BENAR jika reserved
 */
bool_t isr_vector_is_reserved(tak_bertanda32_t vector);

/*
 * isr_set_name
 * ------------
 * Set nama untuk vector.
 *
 * Parameter:
 *   vector - Nomor vector
 *   nama   - Nama handler
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t isr_set_name(tak_bertanda32_t vector, const char *nama);

/*
 * isr_get_name
 * ------------
 * Dapatkan nama vector.
 *
 * Parameter:
 *   vector - Nomor vector
 *
 * Return:
 *   Pointer ke nama, atau NULL jika tidak ada
 */
const char *isr_get_name(tak_bertanda32_t vector);

/*
 * isr_print_stats
 * ---------------
 * Print statistik ISR.
 */
void isr_print_stats(void);

/*
 * isr_print_handlers
 * ------------------
 * Print daftar handler yang terdaftar.
 */
void isr_print_handlers(void);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI EXCEPTION HANDLER (EXCEPTION HANDLER FUNCTIONS)
 * ===========================================================================
 */

/*
 * isr_exception_handler_default
 * -----------------------------
 * Handler default untuk exception yang tidak ditangani.
 *
 * Parameter:
 *   ctx - Pointer ke konteks register
 */
void isr_exception_handler_default(register_context_t *ctx);

/*
 * isr_divide_error_handler
 * ------------------------
 * Handler untuk divide error (vector 0).
 *
 * Parameter:
 *   ctx - Pointer ke konteks register
 */
void isr_divide_error_handler(register_context_t *ctx);

/*
 * isr_debug_handler
 * -----------------
 * Handler untuk debug exception (vector 1).
 *
 * Parameter:
 *   ctx - Pointer ke konteks register
 */
void isr_debug_handler(register_context_t *ctx);

/*
 * isr_nmi_handler
 * ---------------
 * Handler untuk NMI (vector 2).
 *
 * Parameter:
 *   ctx - Pointer ke konteks register
 */
void isr_nmi_handler(register_context_t *ctx);

/*
 * isr_breakpoint_handler
 * ----------------------
 * Handler untuk breakpoint (vector 3).
 *
 * Parameter:
 *   ctx - Pointer ke konteks register
 */
void isr_breakpoint_handler(register_context_t *ctx);

/*
 * isr_overflow_handler
 * --------------------
 * Handler untuk overflow (vector 4).
 *
 * Parameter:
 *   ctx - Pointer ke konteks register
 */
void isr_overflow_handler(register_context_t *ctx);

/*
 * isr_bound_handler
 * -----------------
 * Handler untuk BOUND range exceeded (vector 5).
 *
 * Parameter:
 *   ctx - Pointer ke konteks register
 */
void isr_bound_handler(register_context_t *ctx);

/*
 * isr_invalid_opcode_handler
 * --------------------------
 * Handler untuk invalid opcode (vector 6).
 *
 * Parameter:
 *   ctx - Pointer ke konteks register
 */
void isr_invalid_opcode_handler(register_context_t *ctx);

/*
 * isr_device_not_available_handler
 * --------------------------------
 * Handler untuk device not available (vector 7).
 *
 * Parameter:
 *   ctx - Pointer ke konteks register
 */
void isr_device_not_available_handler(register_context_t *ctx);

/*
 * isr_double_fault_handler
 * ------------------------
 * Handler untuk double fault (vector 8).
 *
 * Parameter:
 *   ctx - Pointer ke konteks register
 */
void isr_double_fault_handler(register_context_t *ctx);

/*
 * isr_invalid_tss_handler
 * -----------------------
 * Handler untuk invalid TSS (vector 10).
 *
 * Parameter:
 *   ctx - Pointer ke konteks register
 */
void isr_invalid_tss_handler(register_context_t *ctx);

/*
 * isr_segment_not_present_handler
 * -------------------------------
 * Handler untuk segment not present (vector 11).
 *
 * Parameter:
 *   ctx - Pointer ke konteks register
 */
void isr_segment_not_present_handler(register_context_t *ctx);

/*
 * isr_stack_segment_handler
 * -------------------------
 * Handler untuk stack segment fault (vector 12).
 *
 * Parameter:
 *   ctx - Pointer ke konteks register
 */
void isr_stack_segment_handler(register_context_t *ctx);

/*
 * isr_general_protection_handler
 * ------------------------------
 * Handler untuk general protection fault (vector 13).
 *
 * Parameter:
 *   ctx - Pointer ke konteks register
 */
void isr_general_protection_handler(register_context_t *ctx);

/*
 * isr_page_fault_handler
 * ----------------------
 * Handler untuk page fault (vector 14).
 *
 * Parameter:
 *   ctx - Pointer ke konteks register
 */
void isr_page_fault_handler(register_context_t *ctx);

/*
 * isr_fpu_handler
 * ---------------
 * Handler untuk x87 FPU error (vector 16).
 *
 * Parameter:
 *   ctx - Pointer ke konteks register
 */
void isr_fpu_handler(register_context_t *ctx);

/*
 * isr_alignment_check_handler
 * ---------------------------
 * Handler untuk alignment check (vector 17).
 *
 * Parameter:
 *   ctx - Pointer ke konteks register
 */
void isr_alignment_check_handler(register_context_t *ctx);

/*
 * isr_machine_check_handler
 * -------------------------
 * Handler untuk machine check (vector 18).
 *
 * Parameter:
 *   ctx - Pointer ke konteks register
 */
void isr_machine_check_handler(register_context_t *ctx);

/*
 * isr_simd_handler
 * ----------------
 * Handler untuk SIMD exception (vector 19).
 *
 * Parameter:
 *   ctx - Pointer ke konteks register
 */
void isr_simd_handler(register_context_t *ctx);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI IRQ HANDLER DEFAULT (DEFAULT IRQ HANDLERS)
 * ===========================================================================
 */

/*
 * isr_timer_handler
 * -----------------
 * Handler untuk timer interrupt (IRQ 0).
 *
 * Parameter:
 *   ctx - Pointer ke konteks register
 */
void isr_timer_handler(register_context_t *ctx);

/*
 * isr_keyboard_handler
 * --------------------
 * Handler untuk keyboard interrupt (IRQ 1).
 *
 * Parameter:
 *   ctx - Pointer ke konteks register
 */
void isr_keyboard_handler(register_context_t *ctx);

/*
 * isr_cascade_handler
 * -------------------
 * Handler untuk cascade interrupt (IRQ 2).
 *
 * Parameter:
 *   ctx - Pointer ke konteks register
 */
void isr_cascade_handler(register_context_t *ctx);

/*
 * isr_serial_handler
 * ------------------
 * Handler untuk serial port interrupt.
 *
 * Parameter:
 *   ctx - Pointer ke konteks register
 */
void isr_serial_handler(register_context_t *ctx);

/*
 * isr_rtc_handler
 * ---------------
 * Handler untuk RTC interrupt (IRQ 8).
 *
 * Parameter:
 *   ctx - Pointer ke konteks register
 */
void isr_rtc_handler(register_context_t *ctx);

/*
 * isr_mouse_handler
 * -----------------
 * Handler untuk mouse interrupt (IRQ 12).
 *
 * Parameter:
 *   ctx - Pointer ke konteks register
 */
void isr_mouse_handler(register_context_t *ctx);

/*
 * isr_fpu_handler_irq
 * -------------------
 * Handler untuk FPU interrupt (IRQ 13).
 *
 * Parameter:
 *   ctx - Pointer ke konteks register
 */
void isr_fpu_handler_irq(register_context_t *ctx);

/*
 * isr_ata_primary_handler
 * -----------------------
 * Handler untuk primary ATA interrupt (IRQ 14).
 *
 * Parameter:
 *   ctx - Pointer ke konteks register
 */
void isr_ata_primary_handler(register_context_t *ctx);

/*
 * isr_ata_secondary_handler
 * -------------------------
 * Handler untuk secondary ATA interrupt (IRQ 15).
 *
 * Parameter:
 *   ctx - Pointer ke konteks register
 */
void isr_ata_secondary_handler(register_context_t *ctx);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI SPURIOUS (SPURIOUS FUNCTIONS)
 * ===========================================================================
 */

/*
 * isr_spurious_handler
 * --------------------
 * Handler untuk spurious interrupt.
 *
 * Parameter:
 *   ctx - Pointer ke konteks register
 */
void isr_spurious_handler(register_context_t *ctx);

/*
 * isr_is_spurious
 * ---------------
 * Cek apakah interrupt adalah spurious.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return:
 *   BENAR jika spurious
 */
bool_t isr_is_spurious(tak_bertanda32_t irq);

/*
 * ===========================================================================
 * VARIABEL GLOBAL (GLOBAL VARIABLES)
 * ===========================================================================
 */

/* Tabel ISR global */
extern isr_tabel_t g_isr_tabel;

/* Counter spurious interrupt */
extern tak_bertanda32_t g_isr_spurious_count;

#endif /* INTI_INTERUPSI_ISR_H */
