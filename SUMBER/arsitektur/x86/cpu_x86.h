/*
 * PIGURA OS - CPU_X86.H
 * ----------------------
 * Header untuk fungsi-fungsi CPU spesifik arsitektur x86/x86_64.
 *
 * Berkas ini berisi deklarasi fungsi inline untuk operasi CPU
 * yang spesifik untuk arsitektur x86 dan x86_64.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan GCC extensions
 * - Menggunakan __inline__ untuk kompatibilitas C89
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#ifndef ARSITEKTUR_X86_CPU_X86_H
#define ARSITEKTUR_X86_CPU_X86_H

#include "../../inti/types.h"

/*
 * ===========================================================================
 * UNDEF MAKRO DARI KERNEL.H (UNDEFINE KERNEL.H MACROS)
 * ===========================================================================
 * Jika kernel.h sudah di-include sebelum berkas ini, makro-makro di bawah
 * mungkin sudah didefinisikan. Kita perlu menghapusnya agar bisa
 * mendefinisikan fungsi inline dengan nama yang sama.
 */

#ifdef inb
#undef inb
#endif

#ifdef outb
#undef outb
#endif

#ifdef inw
#undef inw
#endif

#ifdef outw
#undef outw
#endif

#ifdef inl
#undef inl
#endif

#ifdef outl
#undef outl
#endif

#ifdef io_delay
#undef io_delay
#endif

#ifdef cpu_halt
#undef cpu_halt
#endif

#ifdef cpu_enable_irq
#undef cpu_enable_irq
#endif

#ifdef cpu_disable_irq
#undef cpu_disable_irq
#endif

#ifdef cpu_save_flags
#undef cpu_save_flags
#endif

#ifdef cpu_restore_flags
#undef cpu_restore_flags
#endif

#ifdef cpu_invlpg
#undef cpu_invlpg
#endif

#ifdef cpu_reload_cr3
#undef cpu_reload_cr3
#endif

#ifdef cpu_read_cr0
#undef cpu_read_cr0
#endif

#ifdef cpu_write_cr0
#undef cpu_write_cr0
#endif

#ifdef cpu_read_cr2
#undef cpu_read_cr2
#endif

#ifdef cpu_read_cr3
#undef cpu_read_cr3
#endif

#ifdef cpu_write_cr3
#undef cpu_write_cr3
#endif

#ifdef cpu_nop
#undef cpu_nop
#endif

/*
 * ===========================================================================
 * FUNGSI I/O PORT (I/O PORT FUNCTIONS)
 * ===========================================================================
 * Fungsi-fungsi untuk mengakses port I/O pada arsitektur x86.
 * Port I/O adalah mekanisme komunikasi hardware yang unik untuk x86.
 */

/*
 * inb - Baca byte dari port I/O
 * @port: Nomor port I/O
 *
 * Return: Nilai byte yang dibaca
 */
static __inline__ tak_bertanda8_t inb(tak_bertanda16_t port)
{
    tak_bertanda8_t value;
    __asm__ __volatile__(
        "inb %1, %0"
        : "=a"(value)
        : "Nd"(port)
    );
    return value;
}

/*
 * outb - Tulis byte ke port I/O
 * @port: Nomor port I/O
 * @value: Nilai byte yang akan ditulis
 */
static __inline__ void outb(tak_bertanda16_t port, tak_bertanda8_t value)
{
    __asm__ __volatile__(
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

/*
 * inw - Baca word dari port I/O
 * @port: Nomor port I/O
 *
 * Return: Nilai word yang dibaca
 */
static __inline__ tak_bertanda16_t inw(tak_bertanda16_t port)
{
    tak_bertanda16_t value;
    __asm__ __volatile__(
        "inw %1, %0"
        : "=a"(value)
        : "Nd"(port)
    );
    return value;
}

/*
 * outw - Tulis word ke port I/O
 * @port: Nomor port I/O
 * @value: Nilai word yang akan ditulis
 */
static __inline__ void outw(tak_bertanda16_t port, tak_bertanda16_t value)
{
    __asm__ __volatile__(
        "outw %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

/*
 * inl - Baca dword dari port I/O
 * @port: Nomor port I/O
 *
 * Return: Nilai dword yang dibaca
 */
static __inline__ tak_bertanda32_t inl(tak_bertanda16_t port)
{
    tak_bertanda32_t value;
    __asm__ __volatile__(
        "inl %1, %0"
        : "=a"(value)
        : "Nd"(port)
    );
    return value;
}

/*
 * outl - Tulis dword ke port I/O
 * @port: Nomor port I/O
 * @value: Nilai dword yang akan ditulis
 */
static __inline__ void outl(tak_bertanda16_t port, tak_bertanda32_t value)
{
    __asm__ __volatile__(
        "outl %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

/*
 * io_delay - Delay I/O kira-kira 1 mikrodetik
 *
 * Menggunakan port 0x80 (POST code port) untuk delay.
 * Port ini aman untuk digunakan karena tidak terhubung ke perangkat penting.
 */
static __inline__ void io_delay(void)
{
    outb(0x80, 0);
}

/*
 * ===========================================================================
 * FUNGSI KONTROL CPU (CPU CONTROL FUNCTIONS)
 * ===========================================================================
 * Fungsi-fungsi untuk mengontrol perilaku CPU.
 */

/*
 * cpu_halt - Hentikan CPU sampai interupsi terjadi
 *
 * Menggunakan instruksi HLT untuk menghentikan CPU.
 * CPU akan berhenti sampai interupsi terjadi.
 */
static __inline__ void cpu_halt(void)
{
    __asm__ __volatile__("hlt");
}

/*
 * cpu_enable_irq - Aktifkan interupsi (enable interrupts)
 *
 * Menggunakan instruksi STI untuk mengaktifkan interupsi.
 */
static __inline__ void cpu_enable_irq(void)
{
    __asm__ __volatile__("sti");
}

/*
 * cpu_disable_irq - Nonaktifkan interupsi (disable interrupts)
 *
 * Menggunakan instruksi CLI untuk menonaktifkan interupsi.
 */
static __inline__ void cpu_disable_irq(void)
{
    __asm__ __volatile__("cli");
}

/*
 * cpu_save_flags - Simpan flags register
 *
 * Return: Nilai flags register
 *
 * Menyimpan nilai flags register dan menonaktifkan interupsi.
 */
static __inline__ tak_bertanda32_t cpu_save_flags(void)
{
    tak_bertanda32_t flags;
    __asm__ __volatile__(
        "pushfl\n\t"
        "popl %0\n\t"
        "cli"
        : "=r"(flags)
    );
    return flags;
}

/*
 * cpu_restore_flags - Restore flags register
 * @flags: Nilai flags yang akan di-restore
 *
 * Mengembalikan nilai flags register ke nilai yang disimpan.
 */
static __inline__ void cpu_restore_flags(tak_bertanda32_t flags)
{
    __asm__ __volatile__(
        "pushl %0\n\t"
        "popfl"
        :
        : "r"(flags)
    );
}

/*
 * ===========================================================================
 * FUNGSI MANAJEMEN MEMORI (MEMORY MANAGEMENT FUNCTIONS)
 * ===========================================================================
 * Fungsi-fungsi untuk mengelola memori dan TLB.
 */

/*
 * cpu_invlpg - Invalidate TLB entry untuk satu halaman
 * @addr: Alamat virtual halaman yang akan di-invalidate
 *
 * Menggunakan instruksi INVLPG untuk menghapus entry TLB
 * untuk halaman yang mengandung alamat yang diberikan.
 */
static __inline__ void cpu_invlpg(void *addr)
{
    __asm__ __volatile__(
        "invlpg (%0)"
        :
        : "r"(addr)
        : "memory"
    );
}

/*
 * cpu_reload_cr3 - Reload CR3 register (invalidate semua TLB)
 *
 * Me-reload CR3 register yang akan menginvalidate seluruh TLB.
 */
static __inline__ void cpu_reload_cr3(void)
{
    tak_bertanda32_t cr3;
    __asm__ __volatile__(
        "movl %%cr3, %0\n\t"
        "movl %0, %%cr3"
        : "=r"(cr3)
        :
        : "memory"
    );
}

/*
 * cpu_read_cr0 - Baca CR0 register
 *
 * Return: Nilai CR0 register
 */
static __inline__ tak_bertanda32_t cpu_read_cr0(void)
{
    tak_bertanda32_t cr0;
    __asm__ __volatile__(
        "movl %%cr0, %0"
        : "=r"(cr0)
    );
    return cr0;
}

/*
 * cpu_write_cr0 - Tulis CR0 register
 * @value: Nilai yang akan ditulis ke CR0
 */
static __inline__ void cpu_write_cr0(tak_bertanda32_t value)
{
    __asm__ __volatile__(
        "movl %0, %%cr0"
        :
        : "r"(value)
    );
}

/*
 * cpu_read_cr2 - Baca CR2 register (alamat page fault)
 *
 * Return: Nilai CR2 register (alamat yang menyebabkan page fault)
 */
static __inline__ tak_bertanda32_t cpu_read_cr2(void)
{
    tak_bertanda32_t cr2;
    __asm__ __volatile__(
        "movl %%cr2, %0"
        : "=r"(cr2)
    );
    return cr2;
}

/*
 * cpu_read_cr3 - Baca CR3 register (page directory base)
 *
 * Return: Nilai CR3 register (alamat fisik page directory)
 */
static __inline__ tak_bertanda32_t cpu_read_cr3(void)
{
    tak_bertanda32_t cr3;
    __asm__ __volatile__(
        "movl %%cr3, %0"
        : "=r"(cr3)
    );
    return cr3;
}

/*
 * cpu_write_cr3 - Tulis CR3 register
 * @value: Nilai yang akan ditulis ke CR3 (alamat fisik page directory)
 */
static __inline__ void cpu_write_cr3(tak_bertanda32_t value)
{
    __asm__ __volatile__(
        "movl %0, %%cr3"
        :
        : "r"(value)
    );
}

/*
 * cpu_read_cr4 - Baca CR4 register
 *
 * Return: Nilai CR4 register
 */
static __inline__ tak_bertanda32_t cpu_read_cr4(void)
{
    tak_bertanda32_t cr4;
    __asm__ __volatile__(
        "movl %%cr4, %0"
        : "=r"(cr4)
    );
    return cr4;
}

/*
 * cpu_write_cr4 - Tulis CR4 register
 * @value: Nilai yang akan ditulis ke CR4
 */
static __inline__ void cpu_write_cr4(tak_bertanda32_t value)
{
    __asm__ __volatile__(
        "movl %0, %%cr4"
        :
        : "r"(value)
    );
}

/*
 * ===========================================================================
 * FUNGSI BARRIER (BARRIER FUNCTIONS)
 * ===========================================================================
 */

/*
 * cpu_memory_barrier - Memory barrier
 *
 * Memastikan semua operasi memori sebelumnya selesai
 * sebelum operasi memori berikutnya dimulai.
 */
static __inline__ void cpu_memory_barrier(void)
{
    __asm__ __volatile__("" ::: "memory");
}

/*
 * cpu_sfence - Store fence
 *
 * Memastikan semua operasi store sebelumnya selesai
 * sebelum operasi store berikutnya.
 */
static __inline__ void cpu_sfence(void)
{
    __asm__ __volatile__("sfence" ::: "memory");
}

/*
 * cpu_lfence - Load fence
 *
 * Memastikan semua operasi load sebelumnya selesai
 * sebelum operasi load berikutnya.
 */
static __inline__ void cpu_lfence(void)
{
    __asm__ __volatile__("lfence" ::: "memory");
}

/*
 * cpu_mfence - Memory fence
 *
 * Memastikan semua operasi memori sebelumnya selesai
 * sebelum operasi memori berikutnya.
 */
static __inline__ void cpu_mfence(void)
{
    __asm__ __volatile__("mfence" ::: "memory");
}

/*
 * ===========================================================================
 * FUNGSI CACHE (CACHE FUNCTIONS)
 * ===========================================================================
 */

/*
 * cpu_clflush - Cache line flush
 * @addr: Alamat memori yang akan di-flush dari cache
 */
static __inline__ void cpu_clflush(void *addr)
{
    __asm__ __volatile__(
        "clflush %0"
        :
        : "m"(*(char *)addr)
    );
}

/*
 * cpu_wbinvd - Write back and invalidate cache
 *
 * Menulis semua data cache yang termodifikasi ke memori
 * dan menginvalidate semua cache internal.
 */
static __inline__ void cpu_wbinvd(void)
{
    __asm__ __volatile__("wbinvd");
}

/*
 * cpu_invd - Invalidate cache without write back
 *
 * Menginvalidate semua cache internal tanpa menulis ke memori.
 * Hati-hati: bisa menyebabkan kehilangan data!
 */
static __inline__ void cpu_invd(void)
{
    __asm__ __volatile__("invd");
}

/*
 * ===========================================================================
 * FUNGSI LAIN-LAIN (MISCELLANEOUS FUNCTIONS)
 * ===========================================================================
 */

/*
 * cpu_cpuid - Jalankan instruksi CPUID
 * @leaf: EAX input (function)
 * @subleaf: ECX input (sub-function)
 * @eax: Pointer untuk menyimpan hasil EAX
 * @ebx: Pointer untuk menyimpan hasil EBX
 * @ecx: Pointer untuk menyimpan hasil ECX
 * @edx: Pointer untuk menyimpan hasil EDX
 */
static __inline__ void cpu_cpuid(tak_bertanda32_t leaf,
                                 tak_bertanda32_t subleaf,
                                 tak_bertanda32_t *eax,
                                 tak_bertanda32_t *ebx,
                                 tak_bertanda32_t *ecx,
                                 tak_bertanda32_t *edx)
{
    __asm__ __volatile__(
        "cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(leaf), "c"(subleaf)
    );
}

/*
 * cpu_read_tsc - Baca Time Stamp Counter
 *
 * Return: Nilai TSC (jumlah siklus CPU sejak reset)
 */
static __inline__ tak_bertanda64_t cpu_read_tsc(void)
{
    tak_bertanda32_t low, high;
    __asm__ __volatile__(
        "rdtsc"
        : "=a"(low), "=d"(high)
    );
    return ((tak_bertanda64_t)high << 32) | low;
}

/*
 * cpu_pause - Pause untuk spin-wait loop
 *
 * Memberikan hint ke CPU bahwa sedang dalam spin-wait loop,
 * memungkinkan CPU untuk mengoptimalkan penggunaan daya.
 */
static __inline__ void cpu_pause(void)
{
    __asm__ __volatile__("pause");
}

/*
 * cpu_nop - No operation
 */
static __inline__ void cpu_nop(void)
{
    __asm__ __volatile__("nop");
}

#endif /* ARSITEKTUR_X86_CPU_X86_H */
