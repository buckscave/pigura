/*
 * PIGURA OS - KONSTANTA.H
 * -----------------------
 * Definisi konstanta sistem untuk kernel Pigura OS.
 *
 * Berkas ini berisi semua konstanta yang digunakan di seluruh kernel,
 * termasuk konstanta memori, proses, interupsi, dan sistem lainnya.
 *
 * Versi: 1.0
 * Tanggal: 2025
 *
 * CATATAN: Jangan definisikan tipe data di berkas ini.
 *          Gunakan types.h untuk tipe data.
 */

#ifndef INTI_KONSTANTA_H
#define INTI_KONSTANTA_H

/* Include tipe dasar */
#include "types.h"

/* Include konfigurasi */
#include "config.h"

/*
 * ============================================================================
 * KONSTANTA MEMORI (MEMORY CONSTANTS)
 * ============================================================================
 */

/* Ukuran halaman besar (2 MB huge page) */
#define UKURAN_HALAMAN_BESAR    (2UL * 1024UL * 1024UL)

/* Ukuran halaman raksasa (1 GB) */
#define UKURAN_HALAMAN_RAKSASA  (1UL * 1024UL * 1024UL * 1024UL)

/* Mask dan shift untuk operasi halaman */
#define MASK_HALAMAN            0xFFFFF000UL
#define MASK_HALAMAN_BESAR      0xFFE00000UL
#define SHIFT_HALAMAN           12

/* Alamat memori khusus - x86/x86_64 */
#define ALAMAT_NULL             0x00000000UL
#define ALAMAT_IDENTITAS_MULAI  0x00000000UL
#define ALAMAT_IDENTITAS_AKHIR  0x00400000UL   /* 4 MB */
#define ALAMAT_KERNEL_MULAI     0xC0000000UL   /* 3 GB (higher half) */
#define ALAMAT_KERNEL_AKHIR     0xFFFFFFFFUL
#define ALAMAT_USER_MULAI       0x00400000UL   /* 4 MB */
#define ALAMAT_USER_AKHIR       0xBFFFFFFFUL   /* 3 GB - 1 */
#define ALAMAT_STACK_USER       0xBFFF0000UL
#define ALAMAT_HEAP_MULAI       0x00800000UL   /* 8 MB */

/* Ukuran stack */
#define UKURAN_STACK_KERNEL     (8UL * 1024UL)     /* 8 KB */
#define UKURAN_STACK_USER       (1UL * 1024UL * 1024UL) /* 1 MB */
#define UKURAN_STACK_GUARD      UKURAN_HALAMAN

/* Ukuran heap kernel */
#define UKURAN_HEAP_KERNEL_AWAL (4UL * 1024UL * 1024UL) /* 4 MB */

/* Batas alokasi */
#define MAKS_ALOKASI_KECIL      256
#define MAKS_ALOKASI_SEDANG     4096
#define MAKS_ALOKASI_BESAR      (1024UL * 1024UL)

/*
 * ============================================================================
 * KONSTANTA PROSES (PROCESS CONSTANTS)
 * ============================================================================
 */

/* Batas proses */
#define MAKS_PROSES             CONFIG_MAKS_PROSES
#define MAKS_THREAD_PER_PROSES  CONFIG_MAKS_THREAD
#define MAKS_FD_PER_PROSES      CONFIG_MAKS_FD

/* Batas argumen */
#define MAKS_ARGC               32
#define MAKS_ARGV_LEN           256
#define MAKS_ENV_LEN            256
#define MAKS_PATH_LEN           256
#define MAKS_NAMA_PROSES        32

/* Nilai khusus PID */
#define PID_KERNEL              0
#define PID_INIT                1
#define PID_IDLE                2

/* Quantum scheduler (dalam tick) */
#define QUANTUM_DEFAULT         CONFIG_SCHEDULER_QUANTUM
#define QUANTUM_REALTIME        20
#define QUANTUM_RENDAH          5

/*
 * ============================================================================
 * KONSTANTA SIGNAL (SIGNAL CONSTANTS)
 * ============================================================================
 */

#define MAKS_SIGNAL             32
#define SIGNAL_KILL             9
#define SIGNAL_TERM             15
#define SIGNAL_STOP             19
#define SIGNAL_CONT             18
#define SIGNAL_SEGFAULT         11
#define SIGNAL_ALARM            14
#define SIGNAL_CHILD            17

/*
 * ============================================================================
 * KONSTANTA INTERUPSI - x86/x86_64 (INTERRUPT CONSTANTS)
 * ============================================================================
 */

/* Jumlah vektor interupsi */
#define JUMLAH_ISR              256
#define JUMLAH_IRQ              16
#define JUMLAH_IRQ_APIC         24

/* Vektor interupsi khusus */
#define VEKTOR_DE               0    /* Divide Error */
#define VEKTOR_DB               1    /* Debug Exception */
#define VEKTOR_NMI              2    /* Non-Maskable Interrupt */
#define VEKTOR_BP               3    /* Breakpoint */
#define VEKTOR_OF               4    /* Overflow */
#define VEKTOR_BR               5    /* BOUND Range Exceeded */
#define VEKTOR_UD               6    /* Invalid Opcode */
#define VEKTOR_NM               7    /* Device Not Available */
#define VEKTOR_DF               8    /* Double Fault */
#define VEKTOR_TS               10   /* Invalid TSS */
#define VEKTOR_NP               11   /* Segment Not Present */
#define VEKTOR_SS               12   /* Stack-Segment Fault */
#define VEKTOR_GP               13   /* General Protection */
#define VEKTOR_PF               14   /* Page Fault */
#define VEKTOR_MF               16   /* x87 FPU Error */
#define VEKTOR_AC               17   /* Alignment Check */
#define VEKTOR_MC               18   /* Machine Check */
#define VEKTOR_XM               19   /* SIMD Exception */

/* Vektor IRQ */
#define VEKTOR_IRQ_MULAI        32
#define VEKTOR_TIMER            32
#define VEKTOR_KEYBOARD         33
#define VEKTOR_CASCADE          34
#define VEKTOR_COM2             35
#define VEKTOR_COM1             36
#define VEKTOR_LPT1             37
#define VEKTOR_FLOPPY           38
#define VEKTOR_RTC              40
#define VEKTOR_ACPI             41
#define VEKTOR_MOUSE            44
#define VEKTOR_FPU              45
#define VEKTOR_PRIMARY_ATA      46
#define VEKTOR_SECONDARY_ATA    47

/* Vektor syscall */
#define VEKTOR_SYSCALL          128  /* 0x80 */

/* Prioritas interupsi */
#define PRIORITAS_NMI           0
#define PRIORITAS_TIMER         1
#define PRIORITAS_KEYBOARD      2
#define PRIORITAS_DISK          3
#define PRIORITAS_NETWORK       4
#define PRIORITAS_MOUSE         5
#define PRIORITAS_USER          6

/*
 * ============================================================================
 * KONSTANTA TIMER (TIMER CONSTANTS)
 * ============================================================================
 */

/* Frekuensi timer */
#define FREKUENSI_TIMER         100   /* 100 Hz = 10ms per tick */
#define MILIDETIK_PER_TICK      (1000 / FREKUENSI_TIMER)

/* Frekuensi PIT (Programmable Interval Timer) */
#define FREKUENSI_PIT           1193182UL
#define PIT_CHANNEL_0           0x40
#define PIT_CHANNEL_1           0x41
#define PIT_CHANNEL_2           0x42
#define PIT_COMMAND             0x43

/* Command PIT */
#define PIT_CMD_BINARY          0x00
#define PIT_CMD_BCD             0x01
#define PIT_CMD_MODE_0          0x00  /* Interrupt on terminal count */
#define PIT_CMD_MODE_1          0x02  /* Hardware retriggerable one-shot */
#define PIT_CMD_MODE_2          0x04  /* Rate generator */
#define PIT_CMD_MODE_3          0x06  /* Square wave mode */
#define PIT_CMD_MODE_4          0x08  /* Software triggered strobe */
#define PIT_CMD_MODE_5          0x0A  /* Hardware triggered strobe */
#define PIT_CMD_LATCH           0x00
#define PIT_CMD_LSB             0x10
#define PIT_CMD_MSB             0x20
#define PIT_CMD_BOTH            0x30

/*
 * ============================================================================
 * KONSTANTA PORT I/O (I/O PORT CONSTANTS)
 * ============================================================================
 */

/* Port PIC (Programmable Interrupt Controller) */
#define PIC1_COMMAND            0x20
#define PIC1_DATA               0x21
#define PIC2_COMMAND            0xA0
#define PIC2_DATA               0xA1

/* Command PIC */
#define PIC_CMD_EOI             0x20  /* End of Interrupt */
#define PIC_CMD_ICW1_INIT       0x10
#define PIC_CMD_ICW1_ICW4       0x01
#define PIC_CMD_ICW4_8086       0x01

/* Port keyboard */
#define PORT_KEYBOARD_DATA      0x60
#define PORT_KEYBOARD_STATUS    0x64
#define PORT_KEYBOARD_COMMAND   0x64

/* Port serial */
#define PORT_COM1               0x3F8
#define PORT_COM2               0x2F8
#define PORT_COM3               0x3E8
#define PORT_COM4               0x2E8

/* Port VGA */
#define PORT_VGA_CRTC_INDEX     0x3D4
#define PORT_VGA_CRTC_DATA      0x3D5
#define PORT_VGA_MISC           0x3C2
#define PORT_VGA_SEQ_INDEX      0x3C4
#define PORT_VGA_SEQ_DATA       0x3C5
#define PORT_VGA_GC_INDEX       0x3CE
#define PORT_VGA_GC_DATA        0x3CF
#define PORT_VGA_ATTR_INDEX     0x3C0
#define PORT_VGA_ATTR_DATA      0x3C1

/* Port CMOS/RTC */
#define PORT_CMOS_INDEX         0x70
#define PORT_CMOS_DATA          0x71

/*
 * ============================================================================
 * KONSTANTA GDT (GLOBAL DESCRIPTOR TABLE)
 * ============================================================================
 */

/* Selector segment */
#define SELECTOR_NULL           0x00
#define SELECTOR_KERNEL_CODE    0x08
#define SELECTOR_KERNEL_DATA    0x10
#define SELECTOR_USER_CODE      0x18
#define SELECTOR_USER_DATA      0x20
#define SELECTOR_TSS            0x28

/* Descriptor types */
#define GDT_TYPE_DATA_RO        0x00  /* Data read-only */
#define GDT_TYPE_DATA_RW        0x02  /* Data read-write */
#define GDT_TYPE_DATA_RWA       0x06  /* Data read-write, accessed */
#define GDT_TYPE_CODE_EO        0x0A  /* Code execute-only */
#define GDT_TYPE_CODE_ER        0x0A  /* Code execute-read */
#define GDT_TYPE_CODE_ERA       0x0E  /* Code execute-read, accessed */

/* Flags GDT */
#define GDT_FLAG_PRESENT        0x80
#define GDT_FLAG_DPL_0          0x00
#define GDT_FLAG_DPL_3          0x60
#define GDT_FLAG_S_SYSTEM       0x00  /* System segment */
#define GDT_FLAG_S_CODE_DATA    0x10  /* Code or data segment */
#define GDT_FLAG_32BIT          0x40  /* 32-bit segment */
#define GDT_FLAG_16BIT          0x00  /* 16-bit segment */
#define GDT_FLAG_4K_GRAN        0x80  /* 4KB granularity */
#define GDT_FLAG_BYTE_GRAN      0x00  /* Byte granularity */

/* Jumlah entri GDT */
#define GDT_ENTRI               8

/*
 * ============================================================================
 * KONSTANTA IDT (INTERRUPT DESCRIPTOR TABLE)
 * ============================================================================
 */

/* Flag IDT */
#define IDT_FLAG_PRESENT        0x80
#define IDT_FLAG_DPL_0          0x00
#define IDT_FLAG_DPL_3          0x60
#define IDT_FLAG_TYPE_INTERRUPT 0x0E
#define IDT_FLAG_TYPE_TRAP      0x0F
#define IDT_FLAG_TYPE_TASK      0x05

/* Jumlah entri IDT */
#define IDT_ENTRI               256

/*
 * ============================================================================
 * KONSTANTA TSS (TASK STATE SEGMENT)
 * ============================================================================
 */

/* Ukuran TSS */
#define TSS_UKURAN_MIN          104

/*
 * ============================================================================
 * KONSTANTA SYSCALL (SYSTEM CALL CONSTANTS)
 * ============================================================================
 */

/* Nomor syscall */
#define SYS_READ                0
#define SYS_WRITE               1
#define SYS_OPEN                2
#define SYS_CLOSE               3
#define SYS_FORK                4
#define SYS_EXEC                5
#define SYS_EXIT                6
#define SYS_GETPID              7
#define SYS_GETPPID             8
#define SYS_BRK                 9
#define SYS_SBRK                10
#define SYS_MMAP                11
#define SYS_MUNMAP              12
#define SYS_SLEEP               13
#define SYS_USLEEP              14
#define SYS_GETCWD              15
#define SYS_CHDIR               16
#define SYS_MKDIR               17
#define SYS_RMDIR               18
#define SYS_UNLINK              19
#define SYS_STAT                20
#define SYS_FSTAT               21
#define SYS_LSEEK               22
#define SYS_DUP                 23
#define SYS_DUP2                24
#define SYS_PIPE                25
#define SYS_WAIT                26
#define SYS_WAITPID             27
#define SYS_KILL                28
#define SYS_SIGNAL              29
#define SYS_SIGACTION           30
#define SYS_TIME                31
#define SYS_GETTIME             32
#define SYS_YIELD               33
#define SYS_JUMLAH              64

/* Argumen syscall maksimum */
#define SYSCALL_ARG_MAKS        6

/*
 * ============================================================================
 * KONSTANTA ERROR (ERROR CONSTANTS)
 * ============================================================================
 */

/* Error code standar */
#define ERROR_TIDAK_ADA         0       /* Tidak ada error */
#define ERROR_PERMISI           1       /* Permission denied */
#define ERROR_FILE_TIDAK_ADA    2       /* File tidak ditemukan */
#define ERROR_PROSES_TIDAK_ADA  3       /* Proses tidak ditemukan */
#define ERROR_INTR              4       /* Interrupted */
#define ERROR_IO                5       /* I/O error */
#define ERROR_NO_MEMORI         6       /* Out of memory */
#define ERROR_BUSY              7       /* Resource busy */
#define ERROR_EXIST             8       /* File sudah ada */
#define ERROR_NOTDIR            9       /* Bukan direktori */
#define ERROR_ISDIR             10      /* Adalah direktori */
#define ERROR_INVALID           11      /* Invalid argument */
#define ERROR_NOSPC             12      /* No space left */
#define ERROR_ROFS              13      /* Read-only filesystem */
#define ERROR_NAMETOOLONG       14      /* Nama terlalu panjang */
#define ERROR_FAULT             15      /* Bad address */
#define ERROR_DEADLK            16      /* Resource deadlock */
#define ERROR_NOSYS             17      /* Function not implemented */

/*
 * ============================================================================
 * KONSTANTA FILE SYSTEM (FILE SYSTEM CONSTANTS)
 * ============================================================================
 */

/* Flag akses file */
#define FILE_BACA               0x01
#define FILE_TULIS              0x02
#define FILE_BACA_TULIS         0x03
#define FILE_BUAT               0x100
#define FILE_TRUNCATE           0x200
#define FILE_APPEND             0x400
#define FILE_EXCL               0x800

/* Mode file */
#define MODE_EXEC_OWNER         0x0400
#define MODE_WRITE_OWNER        0x0200
#define MODE_READ_OWNER         0x0100
#define MODE_EXEC_GROUP         0x0040
#define MODE_WRITE_GROUP        0x0020
#define MODE_READ_GROUP         0x0010
#define MODE_EXEC_OTHER         0x0001
#define MODE_WRITE_OTHER        0x0002
#define MODE_READ_OTHER         0x0004

/* Tipe file */
#define FILE_REGULER            0x0000
#define FILE_DIREKTORI          0x4000
#define FILE_CHAR               0x2000
#define FILE_BLOCK              0x6000
#define FILE_PIPE               0x1000
#define FILE_LINK               0xA000
#define FILE_SOCKET             0xC000

/* Seek whence */
#define SEEK_SET                0
#define SEEK_CUR                1
#define SEEK_END                2

/*
 * ============================================================================
 * KONSTANTA MULTIBOOT (MULTIBOOT CONSTANTS)
 * ============================================================================
 */

/* Magic number multiboot */
#define MULTIBOOT_MAGIC         0x2BADB002
#define MULTIBOOT_MAGIC2        0x1BADB002

/* Flag multiboot */
#define MULTIBOOT_FLAG_MEM      0x001
#define MULTIBOOT_FLAG_DEVICE   0x002
#define MULTIBOOT_FLAG_CMDLINE  0x004
#define MULTIBOOT_FLAG_MODS     0x008
#define MULTIBOOT_FLAG_AOUT     0x010
#define MULTIBOOT_FLAG_ELF      0x020
#define MULTIBOOT_FLAG_MMAP     0x040
#define MULTIBOOT_FLAG_DRIVES   0x080
#define MULTIBOOT_FLAG_CFGTBL   0x100
#define MULTIBOOT_FLAG_LOADER   0x200

/* Tipe memory map */
#define MMAP_TYPE_RAM           1
#define MMAP_TYPE_RESERVED      2
#define MMAP_TYPE_ACPI          3
#define MMAP_TYPE_NVS           4
#define MMAP_TYPE_UNUSABLE      5

/*
 * ============================================================================
 * KONSTANTA VGA (VGA CONSTANTS)
 * ============================================================================
 */

/* Mode VGA */
#define VGA_MODE_TEXT           0x03
#define VGA_MODE_13H            0x13

/* Dimensi layar text */
#define VGA_KOLOM               80
#define VGA_BARIS               25

/* Alamat buffer VGA */
#define VGA_BUFFER              0xB8000

/* Warna VGA */
#define VGA_HITAM               0x00
#define VGA_BIRU                0x01
#define VGA_HIJAU               0x02
#define VGA_CYAN                0x03
#define VGA_MERAH               0x04
#define VGA_MAGENTA             0x05
#define VGA_COKLAT              0x06
#define VGA_ABU_ABU             0x07
#define VGA_ABU_TERANG          0x08
#define VGA_BIRU_TERANG         0x09
#define VGA_HIJAU_TERANG        0x0A
#define VGA_CYAN_TERANG         0x0B
#define VGA_MERAH_TERANG        0x0C
#define VGA_MAGENTA_TERANG      0x0D
#define VGA_KUNING              0x0E
#define VGA_PUTIH               0x0F

/* Makro warna */
#define VGA_ENTRY(baru, latar)  (((latar) << 4) | (baru))
#define VGA_ATTR(fg, bg)        (((bg) << 4) | (fg))

/*
 * ============================================================================
 * KONSTANTA MAGIC DAN SIGNATURE (MAGIC AND SIGNATURES)
 * ============================================================================
 */

/* Magic number untuk validasi struktur */
#define MAGIC_KERNEL            0x50494755  /* "PIGU" */
#define MAGIC_PROSES            0x50524F43  /* "PROC" */
#define MAGIC_MEMORY            0x4D454D20  /* "MEM " */
#define MAGIC_FILE              0x46494C45  /* "FILE" */

/* Signature ELF */
#define ELF_MAGIC               0x464C457F  /* 0x7F 'E' 'L' 'F' */

#endif /* INTI_KONSTANTA_H */
