/*
 * PIGURA OS - KONSTANTA.H
 * ========================
 * Definisi konstanta sistem untuk kernel Pigura OS.
 *
 * Berkas ini berisi semua konstanta yang digunakan di seluruh kernel,
 * termasuk konstanta memori, proses, interupsi, dan sistem lainnya.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Tidak menggunakan fitur C99/C11
 * - Semua konstanta didefinisikan secara eksplisit
 *
 * Versi: 2.0
 * Tanggal: 2025
 */

#ifndef INTI_KONSTANTA_H
#define INTI_KONSTANTA_H

/*
 * ===========================================================================
 * INCLUDE
 * ===========================================================================
 */

#include "types.h"
#include "config.h"

/*
 * ===========================================================================
 * KONSTANTA MEMORI (MEMORY CONSTANTS)
 * ===========================================================================
 * Konstanta-konstanta yang berkaitan dengan manajemen memori.
 */

/* Mask dan shift untuk operasi halaman */
#define MASK_HALAMAN 0xFFFFF000UL
#define MASK_HALAMAN_BESAR 0xFFE00000UL
#define SHIFT_HALAMAN 12
#define SHIFT_HALAMAN_BESAR 21

/* Alamat memori khusus - x86/x86_64 */
#define ALAMAT_NULL 0x00000000UL
#define ALAMAT_IDENTITAS_MULAI 0x00000000UL
#define ALAMAT_IDENTITAS_AKHIR 0x00400000UL

/* Alamat kernel (higher half kernel) */
#define ALAMAT_KERNEL_MULAI 0xC0000000UL
#define ALAMAT_KERNEL_AKHIR 0xFFFFFFFFUL

/* Alamat user space */
#define ALAMAT_USER_MULAI 0x00400000UL
#define ALAMAT_USER_AKHIR 0xBFFFFFFFUL
#define ALAMAT_STACK_USER 0xBFFF0000UL
#define ALAMAT_HEAP_MULAI 0x00800000UL

/* Alamat video memory */
#define ALAMAT_VGA_TEXT 0xB8000UL
#define ALAMAT_VGA_GRAPHICS 0xA0000UL

/* Alamat BIOS area */
#define ALAMAT_BIOS_ROM 0xF0000UL
#define ALAMAT_BIOS_EXT 0xE0000UL

/* Alamat DMA */
#define ALAMAT_DMA_MULAI 0x00000UL
#define ALAMAT_DMA_AKHIR 0x100000UL

/* Ukuran stack */
#define UKURAN_STACK_KERNEL (8UL * 1024UL)
#define UKURAN_STACK_USER (1UL * 1024UL * 1024UL)
#define UKURAN_STACK_GUARD UKURAN_HALAMAN

/* Ukuran heap kernel */
#define UKURAN_HEAP_KERNEL_AWAL (4UL * 1024UL * 1024UL)

/* Batas alokasi */
#define MAKS_ALOKASI_KECIL 256
#define MAKS_ALOKASI_SEDANG 4096
#define MAKS_ALOKASI_BESAR (1024UL * 1024UL)

/* Alignment memori */
#define ALIGN_CACHE_LINE 64
#define ALIGN_PAGE UKURAN_HALAMAN

/*
 * ===========================================================================
 * KONSTANTA PROSES (PROCESS CONSTANTS)
 * ===========================================================================
 * Konstanta-konstanta yang berkaitan dengan manajemen proses.
 */

/* Batas proses */
#define MAKS_PROSES CONFIG_MAKS_PROSES
#define MAKS_THREAD_PER_PROSES CONFIG_MAKS_THREAD
#define MAKS_FD_PER_PROSES CONFIG_MAKS_FD

/* Alias untuk kompatibilitas dengan proses.h */
#define PROSES_MAKS MAKS_PROSES
#define THREAD_MAKS_PER_PROSES MAKS_THREAD_PER_PROSES
#define FD_MAKS_PER_PROSES MAKS_FD_PER_PROSES

/* Batas argumen */
#define MAKS_ARGC CONFIG_MAKS_ARGC
#define MAKS_ARGV_LEN CONFIG_MAKS_ARGV_LEN
#define MAKS_ENV_LEN 256
#define MAKS_PATH_LEN CONFIG_PATH_MAX
#define MAKS_NAMA_PROSES 32

/* Nilai khusus PID */
#define PID_KERNEL 0
#define PID_INIT 1
#define PID_IDLE 2

/* Flag proses */
#define PROSES_FLAG_KERNEL 0x0001
#define PROSES_FLAG_DAEMON 0x0002
#define PROSES_FLAG_TRACED 0x0004
#define PROSES_FLAG_STOPPED 0x0008
#define PROSES_FLAG_ZOMBIE 0x0010

/* Quantum scheduler (dalam tick) */
#define QUANTUM_DEFAULT CONFIG_SCHEDULER_QUANTUM
#define QUANTUM_REALTIME 20
#define QUANTUM_TINGGI 15
#define QUANTUM_NORMAL 10
#define QUANTUM_RENDAH 5
#define QUANTUM_IDLE 1

/*
 * ===========================================================================
 * KONSTANTA SIGNAL (SIGNAL CONSTANTS)
 * ===========================================================================
 * Konstanta untuk penanganan signal.
 */

#define MAKS_SIGNAL 32

/* Signal standar */
#define SIGNAL_INVALID 0
#define SIGNAL_HUP 1
#define SIGNAL_INT 2
#define SIGNAL_QUIT 3
#define SIGNAL_ILL 4
#define SIGNAL_TRAP 5
#define SIGNAL_ABRT 6
#define SIGNAL_BUS 7
#define SIGNAL_FPE 8
#define SIGNAL_KILL 9
#define SIGNAL_USR1 10
#define SIGNAL_SEGV 11
#define SIGNAL_USR2 12
#define SIGNAL_PIPE 13
#define SIGNAL_ALRM 14
#define SIGNAL_TERM 15
#define SIGNAL_STKFLT 16
#define SIGNAL_CHILD 17
#define SIGCHLD     SIGNAL_CHILD  /* Alias kompatibilitas POSIX */
#define SIGNAL_CONT 18
#define SIGNAL_STOP 19
#define SIGNAL_TSTP 20
#define SIGNAL_TTIN 21
#define SIGNAL_TTOU 22
#define SIGNAL_URG 23
#define SIGNAL_XCPU 24
#define SIGNAL_XFSZ 25
#define SIGNAL_VTALRM 26
#define SIGNAL_PROF 27
#define SIGNAL_WINCH 28
#define SIGNAL_IO 29
#define SIGNAL_PWR 30
#define SIGNAL_SYS 31

/* Signal action flags */
#define SIGNAL_ACT_DEFAULT 0
#define SIGNAL_ACT_IGNORE 1
#define SIGNAL_ACT_HANDLER 2

/*
 * ===========================================================================
 * KONSTANTA INTERUPSI (INTERRUPT CONSTANTS)
 * ===========================================================================
 * Konstanta untuk sistem interupsi.
 * CATATAN: Konstanta spesifik arsitektur ada di dalam guard kondisional.
 */

/* Jumlah vektor interupsi - umum untuk semua arsitektur */
#define JUMLAH_ISR 256
#define JUMLAH_EXCEPTION 32

/* Jumlah IRQ - berbeda per arsitektur */
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    #define JUMLAH_IRQ 16
    #define JUMLAH_IRQ_APIC 24
#elif defined(ARSITEKTUR_ARM) || defined(ARSITEKTUR_ARMV7) || defined(ARSITEKTUR_ARM64)
    #define JUMLAH_IRQ 128  /* ARM GIC mendukung lebih banyak IRQ */
#endif

/*
 * ---------------------------------------------------------------------------
 * KONSTANTA INTERUPSI x86/x86_64
 * ---------------------------------------------------------------------------
 */
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)

/* Vektor exception CPU */
#define VEKTOR_DE 0    /* Divide Error */
#define VEKTOR_DB 1    /* Debug Exception */
#define VEKTOR_NMI 2   /* Non-Maskable Interrupt */
#define VEKTOR_BP 3    /* Breakpoint */
#define VEKTOR_OF 4    /* Overflow */
#define VEKTOR_BR 5    /* BOUND Range Exceeded */
#define VEKTOR_UD 6    /* Invalid Opcode */
#define VEKTOR_NM 7    /* Device Not Available */
#define VEKTOR_DF 8    /* Double Fault */
#define VEKTOR_CSO 9   /* Coprocessor Segment Overrun */
#define VEKTOR_TS 10   /* Invalid TSS */
#define VEKTOR_NP 11   /* Segment Not Present */
#define VEKTOR_SS 12   /* Stack-Segment Fault */
#define VEKTOR_GP 13   /* General Protection */
#define VEKTOR_PF 14   /* Page Fault */
#define VEKTOR_MF 16   /* x87 FPU Error */
#define VEKTOR_AC 17   /* Alignment Check */
#define VEKTOR_MC 18   /* Machine Check */
#define VEKTOR_XM 19   /* SIMD Exception */
#define VEKTOR_VE 20   /* Virtualization Exception */
#define VEKTOR_CP 21   /* Control Protection Exception */

/* Vektor IRQ (remapped) */
#define VEKTOR_IRQ_MULAI 32
#define VEKTOR_TIMER 32
#define VEKTOR_KEYBOARD 33
#define VEKTOR_CASCADE 34
#define VEKTOR_COM2 35
#define VEKTOR_COM1 36
#define VEKTOR_LPT1 37
#define VEKTOR_FLOPPY 38
#define VEKTOR_RTC 40
#define VEKTOR_ACPI 41
#define VEKTOR_MOUSE 44
#define VEKTOR_FPU 45
#define VEKTOR_PRIMARY_ATA 46
#define VEKTOR_SECONDARY_ATA 47

/* Vektor syscall */
#define VEKTOR_SYSCALL 128

/* Vektor user-defined */
#define VEKTOR_USER_MULAI 200

/* Prioritas interupsi */
#define PRIORITAS_NMI 0
#define PRIORITAS_EXCEPTION 1
#define PRIORITAS_TIMER 2
#define PRIORITAS_KEYBOARD 3
#define PRIORITAS_DISK 4
#define PRIORITAS_NETWORK 5
#define PRIORITAS_MOUSE 6
#define PRIORITAS_USER 7

/*
 * ===========================================================================
 * KONSTANTA TIMER (TIMER CONSTANTS)
 * ===========================================================================
 * Konstanta untuk sistem timer.
 */

/* Frekuensi timer */
#define FREKUENSI_TIMER CONFIG_TIMER_FREQ
#define MILIDETIK_PER_TICK CONFIG_MS_PER_TICK

/* Frekuensi PIT (Programmable Interval Timer) */
#define FREKUENSI_PIT 1193182UL
#define PIT_CHANNEL_0 0x40
#define PIT_CHANNEL_1 0x41
#define PIT_CHANNEL_2 0x42
#define PIT_COMMAND 0x43

/* Command PIT */
#define PIT_CMD_BINARY 0x00
#define PIT_CMD_BCD 0x01
#define PIT_CMD_MODE_0 0x00  /* Interrupt on terminal count */
#define PIT_CMD_MODE_1 0x02  /* Hardware retriggerable one-shot */
#define PIT_CMD_MODE_2 0x04  /* Rate generator */
#define PIT_CMD_MODE_3 0x06  /* Square wave mode */
#define PIT_CMD_MODE_4 0x08  /* Software triggered strobe */
#define PIT_CMD_MODE_5 0x0A  /* Hardware triggered strobe */
#define PIT_CMD_LATCH 0x00
#define PIT_CMD_LSB 0x10
#define PIT_CMD_MSB 0x20
#define PIT_CMD_BOTH 0x30

/* PIT divisor untuk berbagai frekuensi */
#define PIT_DIVISOR_100HZ (FREKUENSI_PIT / 100)
#define PIT_DIVISOR_1000HZ (FREKUENSI_PIT / 1000)

/*
 * ===========================================================================
 * KONSTANTA PORT I/O (I/O PORT CONSTANTS)
 * ===========================================================================
 * Konstanta untuk akses port I/O.
 */

/* Port PIC (Programmable Interrupt Controller) */
#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA 0xA1

/* Command PIC */
#define PIC_CMD_EOI 0x20  /* End of Interrupt */
#define PIC_CMD_ICW1_INIT 0x10
#define PIC_CMD_ICW1_ICW4 0x01
#define PIC_CMD_ICW4_8086 0x01

/* PIC IRQ mask */
#define PIC_IRQ_MASK_ALL 0xFF

/* Port keyboard */
#define PORT_KEYBOARD_DATA 0x60
#define PORT_KEYBOARD_STATUS 0x64
#define PORT_KEYBOARD_COMMAND 0x64

/* Keyboard commands */
#define KEYBOARD_CMD_SET_LEDS 0xED
#define KEYBOARD_CMD_ECHO 0xEE
#define KEYBOARD_CMD_SET_SCANCODE 0xF0
#define KEYBOARD_CMD_IDENTIFY 0xF2
#define KEYBOARD_CMD_SET_RATE 0xF3
#define KEYBOARD_CMD_ENABLE 0xF4
#define KEYBOARD_CMD_DISABLE 0xF5
#define KEYBOARD_CMD_RESET 0xFF

/* Port serial */
#define PORT_COM1 0x3F8
#define PORT_COM2 0x2F8
#define PORT_COM3 0x3E8
#define PORT_COM4 0x2E8

/* Serial register offsets */
#define SERIAL_DATA 0
#define SERIAL_INT_ENABLE 1
#define SERIAL_FIFO_CTRL 2
#define SERIAL_LINE_CTRL 3
#define SERIAL_MODEM_CTRL 4
#define SERIAL_LINE_STATUS 5
#define SERIAL_MODEM_STATUS 6
#define SERIAL_SCRATCH 7

/* Port VGA */
#define PORT_VGA_CRTC_INDEX 0x3D4
#define PORT_VGA_CRTC_DATA 0x3D5
#define PORT_VGA_MISC 0x3C2
#define PORT_VGA_SEQ_INDEX 0x3C4
#define PORT_VGA_SEQ_DATA 0x3C5
#define PORT_VGA_GC_INDEX 0x3CE
#define PORT_VGA_GC_DATA 0x3CF
#define PORT_VGA_ATTR_INDEX 0x3C0
#define PORT_VGA_ATTR_DATA 0x3C1

/* Port CMOS/RTC */
#define PORT_CMOS_INDEX 0x70
#define PORT_CMOS_DATA 0x71

/* CMOS registers */
#define CMOS_SECOND 0x00
#define CMOS_MINUTE 0x02
#define CMOS_HOUR 0x04
#define CMOS_DAY 0x07
#define CMOS_MONTH 0x08
#define CMOS_YEAR 0x09
#define CMOS_CENTURY 0x32
#define CMOS_STATUS_A 0x0A
#define CMOS_STATUS_B 0x0B
#define CMOS_STATUS_C 0x0C
#define CMOS_STATUS_D 0x0D

/* Port PCI */
#define PORT_PCI_CONFIG_ADDRESS 0xCF8
#define PORT_PCI_CONFIG_DATA 0xCFC

/*
 * ===========================================================================
 * KONSTANTA GDT (GLOBAL DESCRIPTOR TABLE)
 * ===========================================================================
 * Konstanta untuk GDT pada arsitektur x86/x86_64.
 */

/* Selector segment */
#define SELECTOR_NULL 0x00
#define SELECTOR_KERNEL_CODE 0x08
#define SELECTOR_KERNEL_DATA 0x10
#define SELECTOR_USER_CODE 0x18
#define SELECTOR_USER_DATA 0x20
#define SELECTOR_TSS 0x28

/* Descriptor types */
#define GDT_TYPE_DATA_RO 0x00  /* Data read-only */
#define GDT_TYPE_DATA_RW 0x02  /* Data read-write */
#define GDT_TYPE_DATA_RWA 0x06 /* Data read-write, accessed */
#define GDT_TYPE_CODE_EO 0x0A  /* Code execute-only */
#define GDT_TYPE_CODE_ER 0x0A  /* Code execute-read */
#define GDT_TYPE_CODE_ERA 0x0E /* Code execute-read, accessed */

/* Flags GDT */
#define GDT_FLAG_PRESENT 0x80
#define GDT_FLAG_DPL_0 0x00
#define GDT_FLAG_DPL_3 0x60
#define GDT_FLAG_S_SYSTEM 0x00
#define GDT_FLAG_S_CODE_DATA 0x10
#define GDT_FLAG_32BIT 0x40
#define GDT_FLAG_16BIT 0x00
#define GDT_FLAG_4K_GRAN 0x80
#define GDT_FLAG_BYTE_GRAN 0x00

/* Jumlah entri GDT */
#define GDT_ENTRI 8

/*
 * ===========================================================================
 * KONSTANTA IDT (INTERRUPT DESCRIPTOR TABLE)
 * ===========================================================================
 * Konstanta untuk IDT pada arsitektur x86/x86_64.
 */

/* Flag IDT */
#define IDT_FLAG_PRESENT 0x80
#define IDT_FLAG_DPL_0 0x00
#define IDT_FLAG_DPL_3 0x60
#define IDT_FLAG_TYPE_INTERRUPT 0x0E
#define IDT_FLAG_TYPE_TRAP 0x0F
#define IDT_FLAG_TYPE_TASK 0x05

/* Jumlah entri IDT */
#define IDT_ENTRI 256

/*
 * ===========================================================================
 * KONSTANTA TSS (TASK STATE SEGMENT)
 * ===========================================================================
 * Konstanta untuk TSS pada arsitektur x86.
 */

/* Ukuran TSS minimum */
#define TSS_UKURAN_MIN 104

/* TSS IO bitmap offset */
#define TSS_IO_BITMAP_OFFSET 102

#endif /* ARSITEKTUR_X86 || ARSITEKTUR_X86_64 */

/*
 * ===========================================================================
 * KONSTANTA SYSCALL (SYSTEM CALL CONSTANTS)
 * ===========================================================================
 * Konstanta untuk system call - berlaku untuk semua arsitektur.
 */

/* Nomor syscall */
#define SYS_READ 0
#define SYS_WRITE 1
#define SYS_OPEN 2
#define SYS_CLOSE 3
#define SYS_FORK 4
#define SYS_EXEC 5
#define SYS_EXIT 6
#define SYS_GETPID 7
#define SYS_GETPPID 8
#define SYS_BRK 9
#define SYS_SBRK 10
#define SYS_MMAP 11
#define SYS_MUNMAP 12
#define SYS_SLEEP 13
#define SYS_USLEEP 14
#define SYS_GETCWD 15
#define SYS_CHDIR 16
#define SYS_MKDIR 17
#define SYS_RMDIR 18
#define SYS_UNLINK 19
#define SYS_STAT 20
#define SYS_FSTAT 21
#define SYS_LSEEK 22
#define SYS_DUP 23
#define SYS_DUP2 24
#define SYS_PIPE 25
#define SYS_WAIT 26
#define SYS_WAITPID 27
#define SYS_KILL 28
#define SYS_SIGNAL 29
#define SYS_SIGACTION 30
#define SYS_TIME 31
#define SYS_GETTIME 32
#define SYS_YIELD 33
#define SYS_GETUID 34
#define SYS_GETGID 35
#define SYS_SETUID 36
#define SYS_SETGID 37
#define SYS_IOCTL 38
#define SYS_READDIR 39
#define SYS_GETINFO 40
#define SYS_JUMLAH 64

/* Argumen syscall maksimum */
#define SYSCALL_ARG_MAKS 6

/*
 * ===========================================================================
 * KONSTANTA ERROR (ERROR CONSTANTS)
 * ===========================================================================
 * Konstanta kode error standar.
 */

/* Error code standar */
#define ERROR_TIDAK_ADA 0
#define ERROR_PERMISI 1
#define ERROR_FILE_TIDAK_ADA 2
#define ERROR_PROSES_TIDAK_ADA 3
#define ERROR_INTR 4
#define ERROR_IO 5
#define ERROR_NO_MEMORI 6
#define ERROR_BUSY 7
#define ERROR_EXIST 8
#define ERROR_NOTDIR 9
#define ERROR_ISDIR 10
#define ERROR_INVALID 11
#define ERROR_NOSPC 12
#define ERROR_ROFS 13
#define ERROR_NAMETOOLONG 14
#define ERROR_FAULT 15
#define ERROR_DEADLK 16
#define ERROR_NOSYS 17
#define ERROR_NOTEMPTY 18
#define ERROR_LOOP 19
#define ERROR_OVERFLOW 20
#define ERROR_PIPE 21
#define ERROR_RANGE 22
#define ERROR_NOCHILD 23      /* Tidak ada child proses */
#define ERROR_AGAIN 24        /* Coba lagi */
#define ERROR_NOTSUP 25       /* Tidak didukung */
#define ERROR_ALREADY 26      /* Sudah ada/terjadi */

/*
 * ===========================================================================
 * KONSTANTA EXIT STATUS (EXIT STATUS CONSTANTS)
 * ===========================================================================
 * Konstanta untuk encoding exit status.
 */

/* Exit status encoding */
#define EXIT_SIGNAL_SHIFT       8       /* Shift untuk signal number */
#define EXIT_SIGNAL_CORE        0x80    /* Flag core dump */

/* Codes untuk si_code (waitid) */
#define CLD_EXITED      1       /* Child exited normally */
#define CLD_KILLED      2       /* Child killed by signal */
#define CLD_DUMPED      3       /* Child dumped core */
#define CLD_TRAPPED     4       /* Child traced trap */
#define CLD_STOPPED     5       /* Child stopped */
#define CLD_CONTINUED   6       /* Child continued */

/*
 * ===========================================================================
 * KONSTANTA FILE SYSTEM (FILE SYSTEM CONSTANTS)
 * ===========================================================================
 * Konstanta untuk operasi filesystem.
 */

/* Flag akses file */
#define FILE_BACA 0x01
#define FILE_TULIS 0x02
#define FILE_BACA_TULIS 0x03
#define FILE_BUAT 0x100
#define FILE_TRUNCATE 0x200
#define FILE_APPEND 0x400
#define FILE_EXCL 0x800
#define FILE_NOCTTY 0x1000
#define FILE_NONBLOCK 0x2000
#define FILE_SYNC 0x4000
#define FILE_DIRECTORY 0x10000

/* Mode file (permission bits) */
#define MODE_ISUID 0x0800
#define MODE_ISGID 0x0400
#define MODE_ISVTX 0x0200
#define MODE_EXEC_OWNER 0x0040
#define MODE_WRITE_OWNER 0x0080
#define MODE_READ_OWNER 0x0100
#define MODE_EXEC_GROUP 0x0008
#define MODE_WRITE_GROUP 0x0010
#define MODE_READ_GROUP 0x0020
#define MODE_EXEC_OTHER 0x0001
#define MODE_WRITE_OTHER 0x0002
#define MODE_READ_OTHER 0x0004

/* Mask untuk permission */
#define MODE_PERM_MASK 0x01FF

/* Tipe file */
#define FILE_REGULER 0x0000
#define FILE_DIREKTORI 0x4000
#define FILE_CHAR 0x2000
#define FILE_BLOCK 0x6000
#define FILE_PIPE 0x1000
#define FILE_LINK 0xA000
#define FILE_SOCKET 0xC000

/* Mask untuk tipe file */
#define FILE_TYPE_MASK 0xF000

/* Seek whence */
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

/* Dirent type */
#define DIRENT_UNKNOWN 0
#define DIRENT_REGULER 1
#define DIRENT_DIREKTORI 2
#define DIRENT_CHAR 3
#define DIRENT_BLOCK 4
#define DIRENT_PIPE 5
#define DIRENT_LINK 6
#define DIRENT_SOCKET 7

/*
 * ===========================================================================
 * KONSTANTA MULTIBOOT (MULTIBOOT CONSTANTS)
 * ===========================================================================
 * Konstanta untuk multiboot protocol.
 */

/* Magic number multiboot */
#define MULTIBOOT_MAGIC 0x2BADB002
#define MULTIBOOT_MAGIC2 0x1BADB002

/* Multiboot flags */
#define MULTIBOOT_FLAG_MEM 0x001
#define MULTIBOOT_FLAG_DEVICE 0x002
#define MULTIBOOT_FLAG_CMDLINE 0x004
#define MULTIBOOT_FLAG_MODS 0x008
#define MULTIBOOT_FLAG_AOUT 0x010
#define MULTIBOOT_FLAG_ELF 0x020
#define MULTIBOOT_FLAG_MMAP 0x040
#define MULTIBOOT_FLAG_DRIVES 0x080
#define MULTIBOOT_FLAG_CFGTBL 0x100
#define MULTIBOOT_FLAG_LOADER 0x200

/* Tipe memory map */
#define MMAP_TYPE_RAM 1
#define MMAP_TYPE_RESERVED 2
#define MMAP_TYPE_ACPI 3
#define MMAP_TYPE_NVS 4
#define MMAP_TYPE_UNUSABLE 5

/*
 * ===========================================================================
 * KONSTANTA VGA (VGA CONSTANTS)
 * ===========================================================================
 * Konstanta untuk kontrol VGA.
 */

/* Mode VGA */
#define VGA_MODE_TEXT 0x03
#define VGA_MODE_13H 0x13

/* Dimensi layar text */
#define VGA_KOLOM 80
#define VGA_BARIS 25

/* Alamat buffer VGA */
#define VGA_BUFFER 0xB8000

/* Warna VGA */
#define VGA_HITAM 0x00
#define VGA_BIRU 0x01
#define VGA_HIJAU 0x02
#define VGA_CYAN 0x03
#define VGA_MERAH 0x04
#define VGA_MAGENTA 0x05
#define VGA_COKLAT 0x06
#define VGA_ABU_ABU 0x07
#define VGA_ABU_TERANG 0x08
#define VGA_BIRU_TERANG 0x09
#define VGA_HIJAU_TERANG 0x0A
#define VGA_CYAN_TERANG 0x0B
#define VGA_MERAH_TERANG 0x0C
#define VGA_MAGENTA_TERANG 0x0D
#define VGA_KUNING 0x0E
#define VGA_PUTIH 0x0F

/* Makro warna */
#define VGA_ENTRY(baru, latar) (((latar) << 4) | (baru))
#define VGA_ATTR(fg, bg) (((bg) << 4) | (fg))

/*
 * ===========================================================================
 * KONSTANTA MAGIC DAN SIGNATURE (MAGIC AND SIGNATURES)
 * ===========================================================================
 * Konstanta magic number untuk validasi.
 */

/* Magic number untuk validasi */
#define MAGIC_KERNEL 0x50494755  /* "PIGU" */
#define MAGIC_PROSES 0x50524F43  /* "PROC" */
#define MAGIC_MEMORY 0x4D454D20  /* "MEM " */
#define MAGIC_FILE 0x46494C45    /* "FILE" */
#define MAGIC_BUFFER 0x42554646  /* "BUFF" */
#define MAGIC_HANDLE 0x48414E44  /* "HAND" */

/* Alias untuk kompatibilitas */
#define PROSES_MAGIC MAGIC_PROSES

/* Signature ELF */
#define ELF_MAGIC 0x464C457F  /* 0x7F 'E' 'L' 'F' */

/* Signature executable */
#define EXE_MAGIC_MZ 0x5A4D   /* MZ header */
#define EXE_MAGIC_PE 0x00004550  /* PE\0\0 */

/*
 * ===========================================================================
 * KONSTANTA WAKTU (TIME CONSTANTS)
 * ===========================================================================
 * Konstanta untuk operasi waktu.
 */

/* Detik per menit/jam/hari */
#define DETIK_PER_MENIT 60
#define DETIK_PER_JAM 3600
#define DETIK_PER_HARI 86400

/* Hari per minggu */
#define HARI_PER_MINGGU 7

/* Bulan per tahun */
#define BULAN_PER_TAHUN 12

/* Epoch year */
#define EPOCH_YEAR 1970

/* Leap year calculation */
#define TAHUN_KABAT(tahun) \
    (((tahun) % 4 == 0 && (tahun) % 100 != 0) || (tahun) % 400 == 0)

/*
 * ===========================================================================
 * KONSTANTA ACPI (ACPI CONSTANTS)
 * ===========================================================================
 * Konstanta untuk ACPI.
 */

/* ACPI signatures */
#define ACPI_RSDP_SIGNATURE "RSD PTR "
#define ACPI_RSDT_SIGNATURE "RSDT"
#define ACPI_XSDT_SIGNATURE "XSDT"
#define ACPI_MADT_SIGNATURE "APIC"
#define ACPI_FADT_SIGNATURE "FACP"
#define ACPI_DSDT_SIGNATURE "DSDT"
#define ACPI_SSDT_SIGNATURE "SSDT"
#define ACPI_HPET_SIGNATURE "HPET"

/* ACPI address */
#define ACPI_RSDP_SCAN_MULAI 0xE0000
#define ACPI_RSDP_SCAN_AKHIR 0xFFFFF

/*
 * ===========================================================================
 * KONSTANTA PCI (PCI CONSTANTS)
 * ===========================================================================
 * Konstanta untuk PCI bus.
 */

/* PCI configuration space */
#define PCI_CONFIG_VENDOR 0x00
#define PCI_CONFIG_DEVICE 0x02
#define PCI_CONFIG_COMMAND 0x04
#define PCI_CONFIG_STATUS 0x06
#define PCI_CONFIG_REVISION 0x08
#define PCI_CONFIG_CLASS 0x09
#define PCI_CONFIG_HEADER 0x0E
#define PCI_CONFIG_BAR0 0x10
#define PCI_CONFIG_BAR1 0x14
#define PCI_CONFIG_BAR2 0x18
#define PCI_CONFIG_BAR3 0x1C
#define PCI_CONFIG_BAR4 0x20
#define PCI_CONFIG_BAR5 0x24
#define PCI_CONFIG_INTERRUPT 0x3C

/* PCI class codes */
#define PCI_CLASS_STORAGE 0x01
#define PCI_CLASS_NETWORK 0x02
#define PCI_CLASS_DISPLAY 0x03
#define PCI_CLASS_MULTIMEDIA 0x04
#define PCI_CLASS_MEMORY 0x05
#define PCI_CLASS_BRIDGE 0x06
#define PCI_CLASS_SERIAL 0x0C

/* PCI vendor */
#define PCI_VENDOR_INTEL 0x8086
#define PCI_VENDOR_AMD 0x1022
#define PCI_VENDOR_NVIDIA 0x10DE
#define PCI_VENDOR_VIA 0x1106

/*
 * ===========================================================================
 * KONSTANTA USB (USB CONSTANTS)
 * ===========================================================================
 * Konstanta untuk USB.
 */

/* USB descriptor types */
#define USB_DESC_DEVICE 1
#define USB_DESC_CONFIGURATION 2
#define USB_DESC_STRING 3
#define USB_DESC_INTERFACE 4
#define USB_DESC_ENDPOINT 5

/* USB classes */
#define USB_CLASS_AUDIO 1
#define USB_CLASS_STORAGE 8
#define USB_CLASS_HID 3
#define USB_CLASS_HUB 9

/*
 * ===========================================================================
 * KONSTANTA LIMITS (LIMITS CONSTANTS)
 * ===========================================================================
 * Konstanta batas sistem.
 */

/* Maximum path length */
#define PATH_MAX CONFIG_PATH_MAX

/* Maximum name length */
#define NAME_MAX CONFIG_NAME_MAX

/* Maximum open files */
#define OPEN_MAX CONFIG_MAKS_OPEN_FILES

/* Maximum hostname length */
#define HOST_NAME_MAX 64

/* Maximum login name */
#define LOGIN_NAME_MAX 32

/* Maximum arguments */
#define ARG_MAX (MAKS_ARGC * MAKS_ARGV_LEN)

/* Maximum environment */
#define ENV_MAX (32 * 256)

#endif /* INTI_KONSTANTA_H */
