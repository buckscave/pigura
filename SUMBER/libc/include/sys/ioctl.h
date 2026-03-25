/*
 * PIGURA LIBC - SYS/IOCTL.H
 * ==========================
 * Header untuk I/O control sesuai standar POSIX.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 */

#ifndef LIBC_SYS_IOCTL_H
#define LIBC_SYS_IOCTL_H

/* ============================================================
 * DEPENDENSI
 * ============================================================
 */
#include <sys/types.h>

/* ============================================================
 * DEKLARASI FUNGSI
 * ============================================================
 */

/*
 * ioctl - I/O control
 *
 * Parameter:
 *   fd   - File descriptor
 *   request - Request code
 *   ...  - Argumen (tergantung request)
 *
 * Return: Biasanya 0 jika berhasil, -1 jika error.
 *         Beberapa request mengembalikan nilai lain.
 */
extern int ioctl(int fd, unsigned long request, ...);

/* ============================================================
 * TERMINAL I/O CONSTANTS (Termios)
 * ============================================================
 */

/* TCGETS/TCSETS - Termios structure */
#define TCGETS      0x5401
#define TCSETS      0x5402
#define TCSETSW     0x5403
#define TCSETSF     0x5404

/* TCGETA/TCSETA - Termios structure (BSD style) */
#define TCGETA      0x5405
#define TCSETA      0x5406
#define TCSETAW     0x5407
#define TCSETAF     0x5408

/* TCSBRK - Break */
#define TCSBRK      0x5409

/* TCXONC - Flow control */
#define TCXONC      0x540A
#define TCOOFF      0   /* Suspend output */
#define TCOON       1   /* Restart output */
#define TCIOFF      2   /* Send STOP char */
#define TCION       3   /* Send START char */

/* TCFLSH - Flush */
#define TCFLSH      0x540B
#define TCIFLUSH    0   /* Flush input */
#define TCOFLUSH    1   /* Flush output */
#define TCIOFLUSH   2   /* Flush both */

/* TIOCEXCL - Exclusive use */
#define TIOCEXCL    0x540C

/* TIOCNXCL - Clear exclusive use */
#define TIOCNXCL    0x540D

/* TIOCSCTTY - Set controlling terminal */
#define TIOCSCTTY   0x540E

/* TIOCGPGRP - Get process group */
#define TIOCGPGRP   0x540F

/* TIOCSPGRP - Set process group */
#define TIOCSPGRP   0x5410

/* TIOCOUTQ - Output queue size */
#define TIOCOUTQ    0x5411

/* TIOCSTI - Simulate terminal input */
#define TIOCSTI     0x5412

/* TIOCGWINSZ - Get window size */
#define TIOCGWINSZ  0x5413

/* TIOCSWINSZ - Set window size */
#define TIOCSWINSZ  0x5414

/* TIOCMGET - Get modem status */
#define TIOCMGET    0x5415

/* TIOCMBIS - Set modem bits */
#define TIOCMBIS    0x5416

/* TIOCMBIC - Clear modem bits */
#define TIOCMBIC    0x5417

/* TIOCMSET - Set modem status */
#define TIOCMSET    0x5418

/* Modem bits */
#define TIOCM_LE    0x001   /* Line enable */
#define TIOCM_DTR   0x002   /* Data terminal ready */
#define TIOCM_RTS   0x004   /* Request to send */
#define TIOCM_ST    0x008   /* Secondary transmit */
#define TIOCM_SR    0x010   /* Secondary receive */
#define TIOCM_CTS   0x020   /* Clear to send */
#define TIOCM_CAR   0x040   /* Carrier detect */
#define TIOCM_RNG   0x080   /* Ring indicator */
#define TIOCM_DSR   0x100   /* Data set ready */
#define TIOCM_CD    TIOCM_CAR
#define TIOCM_RI    TIOCM_RNG

/* TIOCGSOFTCAR - Get software carrier */
#define TIOCGSOFTCAR 0x5419

/* TIOCSSOFTCAR - Set software carrier */
#define TIOCSSOFTCAR 0x541A

/* TIOCLINUX - Linux-specific */
#define TIOCLINUX   0x541C

/* TIOCCONS - Redirect console */
#define TIOCCONS    0x541D

/* TIOCGSERIAL - Get serial info */
#define TIOCGSERIAL 0x541E

/* TIOCSSERIAL - Set serial info */
#define TIOCSSERIAL 0x541F

/* TIOCPKT - Packet mode */
#define TIOCPKT     0x5420

/* Packet mode values */
#define TIOCPKT_DATA       0
#define TIOCPKT_FLUSHREAD  1
#define TIOCPKT_FLUSHWRITE 2
#define TIOCPKT_STOP       4
#define TIOCPKT_START      8
#define TIOCPKT_NOSTOP    16
#define TIOCPKT_DOSTOP    32
#define TIOCPKT_IOCTL     64

/* FIONREAD - Bytes available to read */
#define FIONREAD    0x541B

/* FIONBIO - Non-blocking I/O */
#define FIONBIO     0x5421

/* FIOASYNC - Asynchronous I/O */
#define FIOASYNC    0x5422

/* TIOCSERCONFIG - Serial config */
#define TIOCSERCONFIG 0x5423

/* TIOCSERGWILD - Get wild receiver mode */
#define TIOCSERGWILD 0x5424

/* TIOCSERSWILD - Set wild receiver mode */
#define TIOCSERSWILD 0x5425

/* TIOCGLCKTRMIOS - Lock termios */
#define TIOCGLCKTRMIOS 0x5426

/* TIOCSLCKTRMIOS - Unlock termios */
#define TIOCSLCKTRMIOS 0x5427

/* TIOCSERGSTRUCT - Get serial structure */
#define TIOCSERGSTRUCT 0x5428

/* TIOCSERGETLSR - Get line status register */
#define TIOCSERGETLSR 0x5429
#define TIOCSER_TEMT    0x01   /* Transmitter empty */

/* TIOCSERGETMULTI - Get multiport config */
#define TIOCSERGETMULTI 0x542A

/* TIOCSERSETMULTI - Set multiport config */
#define TIOCSERSETMULTI 0x542B

/* ============================================================
 * STRUKTUR WINDOW SIZE
 * ============================================================
 */

struct winsize {
    unsigned short ws_row;      /* Rows (characters) */
    unsigned short ws_col;      /* Columns (characters) */
    unsigned short ws_xpixel;   /* Width (pixels) */
    unsigned short ws_ypixel;   /* Height (pixels) */
};

/* ============================================================
 * STRUKTUR SERIAL
 * ============================================================
 */

struct serial_struct {
    int     type;
    int     line;
    int     port;
    int     irq;
    int     flags;
    int     xmit_fifo_size;
    int     custom_divisor;
    int     baud_base;
    unsigned short close_delay;
    char    io_type;
    char    reserved_char[1];
    int     hub6;
    unsigned short closing_wait;
    unsigned short closing_wait2;
    unsigned char *iomem_base;
    unsigned short iomem_reg_shift;
    unsigned int port_high;
    unsigned long iomap_base;
};

/* Serial types */
#define PORT_UNKNOWN    0
#define PORT_8250       1
#define PORT_16450      2
#define PORT_16550      3
#define PORT_16550A     4
#define PORT_CIRRUS     5
#define PORT_16650      6
#define PORT_16650V2    7
#define PORT_16750      8
#define PORT_STARTECH   9
#define PORT_16C950     10
#define PORT_16654      11
#define PORT_16850      12
#define PORT_RSA        13

/* Serial flags */
#define ASYNC_SPD_MASK      0x1030
#define ASYNC_SPD_HI        0x0010
#define ASYNC_SPD_VHI       0x0020
#define ASYNC_SPD_CUST      0x0030
#define ASYNC_SKIP_TEST     0x0040
#define ASYNC_AUTO_IRQ      0x0080
#define ASYNC_SESSION_LOCKOUT 0x0100
#define ASYNC_PGRP_LOCKOUT  0x0200
#define ASYNC_CALLOUT_NOHUP 0x0400
#define ASYNC_HARDPPS_CD    0x0800
#define ASYNC_SPD_SHI       0x1000
#define ASYNC_LOW_LATENCY   0x2000
#define ASYNC_BUGGY_UART    0x4000

/* ============================================================
 * BLOCK I/O CONSTANTS
 * ============================================================
 */

/* BLKROSET - Set read-only */
#define BLKROSET    _IO(0x12, 93)

/* BLKROGET - Get read-only status */
#define BLKROGET    _IO(0x12, 94)

/* BLKRRPART - Re-read partition table */
#define BLKRRPART   _IO(0x12, 95)

/* BLKGETSIZE - Get device size (in 512-byte sectors) */
#define BLKGETSIZE  _IO(0x12, 96)

/* BLKFLSBUF - Flush buffer cache */
#define BLKFLSBUF   _IO(0x12, 97)

/* BLKRASET - Set read-ahead */
#define BLKRASET    _IO(0x12, 98)

/* BLKRAGET - Get read-ahead */
#define BLKRAGET    _IO(0x12, 99)

/* BLKFRASET - Set filesystem read-ahead */
#define BLKFRASET   _IO(0x12, 100)

/* BLKFRAGET - Get filesystem read-ahead */
#define BLKFRAGET   _IO(0x12, 101)

/* BLKSECTSET - Set max sectors per request */
#define BLKSECTSET  _IO(0x12, 102)

/* BLKSECTGET - Get max sectors per request */
#define BLKSECTGET  _IO(0x12, 103)

/* BLKSSZGET - Get hardware sector size */
#define BLKSSZGET   _IO(0x12, 104)

/* BLKBSZGET - Get block size */
#define BLKBSZGET   _IO(0x12, 112)

/* BLKBSZSET - Set block size */
#define BLKBSZSET   _IO(0x12, 113)

/* BLKGETSIZE64 - Get device size in bytes */
#define BLKGETSIZE64 _IO(0x12, 114)

/* ============================================================
 * MAKRO UNTUK MEMBUAT IOCTL REQUEST
 * ============================================================
 */

/* Direction bits */
#define _IOC_NONE   0U
#define _IOC_READ   1U
#define _IOC_WRITE  2U

/* Size bits */
#define _IOC_SIZEBITS   14
#define _IOC_DIRBITS    2
#define _IOC_NRBITS     8
#define _IOC_TYPEBITS   8

/* Masks */
#define _IOC_NRMASK     ((1 << _IOC_NRBITS) - 1)
#define _IOC_TYPEMASK   ((1 << _IOC_TYPEBITS) - 1)
#define _IOC_SIZEMASK   ((1 << _IOC_SIZEBITS) - 1)
#define _IOC_DIRMASK    ((1 << _IOC_DIRBITS) - 1)

/* Shifts */
#define _IOC_NRSHIFT    0
#define _IOC_TYPESHIFT  (_IOC_NRSHIFT + _IOC_NRBITS)
#define _IOC_SIZESHIFT  (_IOC_TYPESHIFT + _IOC_TYPEBITS)
#define _IOC_DIRSHIFT   (_IOC_SIZESHIFT + _IOC_SIZEBITS)

/* Makro untuk membangun ioctl number */
#define _IOC(dir, type, nr, size) \
    (((dir) << _IOC_DIRSHIFT) | \
     ((type) << _IOC_TYPESHIFT) | \
     ((nr) << _IOC_NRSHIFT) | \
     ((size) << _IOC_SIZESHIFT))

/* Convenience macros */
#define _IO(type, nr)       _IOC(_IOC_NONE, (type), (nr), 0)
#define _IOR(type, nr, size) _IOC(_IOC_READ, (type), (nr), sizeof(size))
#define _IOW(type, nr, size) _IOC(_IOC_WRITE, (type), (nr), sizeof(size))
#define _IOWR(type, nr, size) _IOC(_IOC_READ|_IOC_WRITE, (type), (nr), sizeof(size))

/* Decode macros */
#define _IOC_DIR(nr)     (((nr) >> _IOC_DIRSHIFT) & _IOC_DIRMASK)
#define _IOC_TYPE(nr)    (((nr) >> _IOC_TYPESHIFT) & _IOC_TYPEMASK)
#define _IOC_NR(nr)      (((nr) >> _IOC_NRSHIFT) & _IOC_NRMASK)
#define _IOC_SIZE(nr)    (((nr) >> _IOC_SIZESHIFT) & _IOC_SIZEMASK)

/* Check direction macros */
#define _IOC_NONE(nr)    (_IOC_DIR(nr) == _IOC_NONE)
#define _IOC_READ(nr)    (_IOC_DIR(nr) & _IOC_READ)
#define _IOC_WRITE(nr)   (_IOC_DIR(nr) & _IOC_WRITE)

#endif /* LIBC_SYS_IOCTL_H */
