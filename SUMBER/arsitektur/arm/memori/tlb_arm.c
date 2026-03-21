/*
 * PIGURA OS - TLB_ARM.C
 * ---------------------
 * Implementasi operasi TLB untuk ARM (32-bit).
 *
 * Arsitektur: ARM (ARMv4/v5/v6 - 32-bit)
 * Versi: 1.0
 */

#include "../../../inti/types.h"
#include "../../../inti/konstanta.h"
#include "../../../inti/hal/hal.h"

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

/* CP15 TLB operation opcodes */
#define TLB_INVAL_ALL           0, c8, c7, 0    /* Invalidate All */
#define TLB_INVAL_ALL_NS        0, c8, c3, 0    /* Invalidate All Non-secure */
#define TLB_INVAL_MVA           0, c8, c7, 1    /* Invalidate by MVA */
#define TLB_INVAL_MVA_ASID      0, c8, c7, 2    /* Invalidate by MVA + ASID */
#define TLB_INVAL_ASID          0, c8, c7, 2    /* Invalidate by ASID */

/* Data and instruction TLB operations */
#define TLB_INVAL_I_ALL         0, c8, c5, 0    /* Invalidate I-TLB All */
#define TLB_INVAL_D_ALL         0, c8, c6, 0    /* Invalidate D-TLB All */
#define TLB_INVAL_I_MVA         0, c8, c5, 1    /* Invalidate I-TLB by MVA */
#define TLB_INVAL_D_MVA         0, c8, c6, 1    /* Invalidate D-TLB by MVA */

/*
 * ============================================================================
 * FUNGSI BANTUAN CP15 (CP15 HELPER FUNCTIONS)
 * ============================================================================
 */

/*
 * tlb_cp15_write
 * --------------
 * Tulis register CP15 untuk TLB operations.
 *
 * Parameter:
 *   op1, c1, c2, op2 - Opcode CP15
 *   val - Nilai yang akan ditulis
 */
static inline void tlb_cp15_write(tak_bertanda32_t op1,
    tak_bertanda32_t c1, tak_bertanda32_t c2, tak_bertanda32_t op2,
    tak_bertanda32_t val)
{
    __asm__ __volatile__(
        "mcr p15, %0, %1, %2, %3, %4"
        :
        : "i"(op1), "r"(val), "i"(c1), "i"(c2), "i"(op2)
        : "memory"
    );
}

/*
 * tlb_barrier
 * -----------
 * Data synchronization barrier untuk TLB operations.
 */
static inline void tlb_barrier(void)
{
    /* Data synchronization barrier */
    __asm__ __volatile__("mcr p15, 0, r0, c7, c10, 4");

    /* Instruction synchronization barrier */
    __asm__ __volatile__("mcr p15, 0, r0, c7, c5, 4");
}

/*
 * ============================================================================
 * FUNGSI INVALIDATE TLB (TLB INVALIDATE FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_tlb_invalidate_all
 * ----------------------
 * Invalidate seluruh TLB (both instruction dan data).
 *
 * Return: Status operasi
 */
status_t hal_tlb_invalidate_all(void)
{
    /* Invalidate entire unified TLB */
    tlb_cp15_write(TLB_INVAL_ALL, 0);

    /* Barrier */
    tlb_barrier();

    return STATUS_BERHASIL;
}

/*
 * hal_tlb_invalidate_all_non_secure
 * ---------------------------------
 * Invalidate seluruh TLB non-secure.
 *
 * Return: Status operasi
 */
status_t hal_tlb_invalidate_all_non_secure(void)
{
    /* Invalidate all non-secure TLB entries */
    tlb_cp15_write(TLB_INVAL_ALL_NS, 0);

    /* Barrier */
    tlb_barrier();

    return STATUS_BERHASIL;
}

/*
 * hal_tlb_invalidate_by_addr
 * --------------------------
 * Invalidate TLB entry untuk alamat tertentu.
 *
 * Parameter:
 *   addr - Alamat virtual
 *
 * Return: Status operasi
 */
status_t hal_tlb_invalidate_by_addr(void *addr)
{
    tak_bertanda32_t va = (tak_bertanda32_t)(tak_bertanda32_t)addr;

    /* Invalidate TLB entry by MVA (Modified Virtual Address) */
    tlb_cp15_write(TLB_INVAL_MVA, va);

    /* Barrier */
    tlb_barrier();

    return STATUS_BERHASIL;
}

/*
 * hal_tlb_invalidate_by_asid
 * --------------------------
 * Invalidate TLB entries untuk ASID tertentu.
 *
 * Parameter:
 *   asid - Address Space ID
 *
 * Return: Status operasi
 */
status_t hal_tlb_invalidate_by_asid(tak_bertanda32_t asid)
{
    /* Invalidate all entries with given ASID */
    tlb_cp15_write(TLB_INVAL_ASID, asid & 0xFF);

    /* Barrier */
    tlb_barrier();

    return STATUS_BERHASIL;
}

/*
 * hal_tlb_invalidate_by_addr_asid
 * -------------------------------
 * Invalidate TLB entry untuk alamat dan ASID tertentu.
 *
 * Parameter:
 *   addr - Alamat virtual
 *   asid - Address Space ID
 *
 * Return: Status operasi
 */
status_t hal_tlb_invalidate_by_addr_asid(void *addr, tak_bertanda32_t asid)
{
    tak_bertanda32_t va = (tak_bertanda32_t)(tak_bertanda32_t)addr;
    tak_bertanda32_t val;

    /* Combine MVA and ASID */
    val = (va & ~0xFF) | (asid & 0xFF);

    /* Invalidate TLB entry by MVA + ASID */
    tlb_cp15_write(TLB_INVAL_MVA_ASID, val);

    /* Barrier */
    tlb_barrier();

    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI INVALIDATE INSTRUCTION TLB (INSTRUCTION TLB FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_tlb_invalidate_i_all
 * ------------------------
 * Invalidate seluruh instruction TLB.
 *
 * Return: Status operasi
 */
status_t hal_tlb_invalidate_i_all(void)
{
    /* Invalidate entire I-TLB */
    tlb_cp15_write(TLB_INVAL_I_ALL, 0);

    /* Barrier */
    tlb_barrier();

    return STATUS_BERHASIL;
}

/*
 * hal_tlb_invalidate_i_by_addr
 * ----------------------------
 * Invalidate instruction TLB entry untuk alamat tertentu.
 *
 * Parameter:
 *   addr - Alamat virtual
 *
 * Return: Status operasi
 */
status_t hal_tlb_invalidate_i_by_addr(void *addr)
{
    tak_bertanda32_t va = (tak_bertanda32_t)(tak_bertanda32_t)addr;

    /* Invalidate I-TLB entry by MVA */
    tlb_cp15_write(TLB_INVAL_I_MVA, va);

    /* Barrier */
    tlb_barrier();

    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI INVALIDATE DATA TLB (DATA TLB FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_tlb_invalidate_d_all
 * ------------------------
 * Invalidate seluruh data TLB.
 *
 * Return: Status operasi
 */
status_t hal_tlb_invalidate_d_all(void)
{
    /* Invalidate entire D-TLB */
    tlb_cp15_write(TLB_INVAL_D_ALL, 0);

    /* Barrier */
    tlb_barrier();

    return STATUS_BERHASIL;
}

/*
 * hal_tlb_invalidate_d_by_addr
 * ----------------------------
 * Invalidate data TLB entry untuk alamat tertentu.
 *
 * Parameter:
 *   addr - Alamat virtual
 *
 * Return: Status operasi
 */
status_t hal_tlb_invalidate_d_by_addr(void *addr)
{
    tak_bertanda32_t va = (tak_bertanda32_t)(tak_bertanda32_t)addr;

    /* Invalidate D-TLB entry by MVA */
    tlb_cp15_write(TLB_INVAL_D_MVA, va);

    /* Barrier */
    tlb_barrier();

    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI INISIALISASI (INITIALIZATION FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_tlb_init
 * ------------
 * Inisialisasi subsistem TLB.
 *
 * Return: Status operasi
 */
status_t hal_tlb_init(void)
{
    /* Invalidate semua TLB saat startup */
    hal_tlb_invalidate_all();

    return STATUS_BERHASIL;
}

/*
 * hal_tlb_shutdown
 * ----------------
 * Shutdown subsistem TLB.
 *
 * Return: Status operasi
 */
status_t hal_tlb_shutdown(void)
{
    /* Invalidate semua TLB saat shutdown */
    hal_tlb_invalidate_all();

    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI UTILITAS (UTILITY FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_tlb_flush_range
 * -------------------
 * Flush TLB untuk range alamat.
 *
 * Parameter:
 *   mulai - Alamat awal
 *   akhir - Alamat akhir
 *   step  - Langkah iterasi (biasanya ukuran halaman)
 *
 * Return: Status operasi
 */
status_t hal_tlb_flush_range(void *mulai, void *akhir, tak_bertanda32_t step)
{
    tak_bertanda32_t addr;

    if (step == 0) {
        step = UKURAN_HALAMAN;
    }

    for (addr = (tak_bertanda32_t)(tak_bertanda32_t)mulai;
         addr < (tak_bertanda32_t)(tak_bertanda32_t)akhir;
         addr += step) {
        hal_tlb_invalidate_by_addr((void *)addr);
    }

    /* Final barrier */
    tlb_barrier();

    return STATUS_BERHASIL;
}

/*
 * hal_tlb_flush_kernel
 * --------------------
 * Flush TLB untuk kernel space.
 *
 * Return: Status operasi
 */
status_t hal_tlb_flush_kernel(void)
{
    /* Invalidate entire TLB untuk kernel */
    hal_tlb_invalidate_all();

    return STATUS_BERHASIL;
}

/*
 * hal_tlb_flush_user
 * ------------------
 * Flush TLB untuk user space dengan ASID tertentu.
 *
 * Parameter:
 *   asid - Address Space ID
 *
 * Return: Status operasi
 */
status_t hal_tlb_flush_user(tak_bertanda32_t asid)
{
    /* Invalidate all entries with given ASID */
    hal_tlb_invalidate_by_asid(asid);

    return STATUS_BERHASIL;
}
