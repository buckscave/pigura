/*
 * PIGURA LIBC - ERRNO.H
 * ======================
 * Definisi kode error sistem sesuai standar C89 dan POSIX.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 */

#ifndef LIBC_ERRNO_H
#define LIBC_ERRNO_H

/* ============================================================
 * VARIABEL ERNO
 * ============================================================
 * Variabel global yang menyimpan kode error terakhir dari
 * fungsi sistem. Nilai diset ke 0 saat program dimulai atau
 * setelah pemanggilan fungsi yang berhasil (kecuali dokumen-
 * tasinya menyatakan sebaliknya).
 *
 * Deklarasi extern untuk variabel errno yang didefinisikan
 * di errno.c
 */
extern int errno;

/* ============================================================
 * KODE ERROR DASAR (C89)
 * ============================================================
 */

/* Tidak ada error */
#define EDOM        1    /* Argumen di luar domain fungsi matematika */
#define ERANGE      2    /* Hasil di luar jangkauan tipe target */
#define EILSEQ      3    /* Sequence byte ilegal (multibyte/wide char) */

/* ============================================================
 * KODE ERROR POSIX
 * ============================================================
 */

/* Error umum */
#define ENOENT      4    /* Tidak ada file atau direktori */
#define ESRCH       5    /* Tidak ada proses dengan PID tersebut */
#define EINTR       6    /* Panggilan sistem terinterupsi */
#define EIO         7    /* I/O error */
#define ENXIO       8    /* Tidak ada device atau alamat */
#define E2BIG       9    /* Daftar argumen terlalu panjang */
#define ENOEXEC     10   /* Format exec error */
#define EBADF       11   /* File descriptor tidak valid */
#define ECHILD      12   /* Tidak ada proses child */
#define EAGAIN      13   /* Resource tidak tersedia, coba lagi */
#define ENOMEM      14   /* Memori tidak cukup */
#define EACCES      15   /* Akses ditolak */
#define EFAULT      16   /* Alamat memori tidak valid */

/* Error device/block */
#define ENOTBLK     17   /* Bukan block device */
#define EBUSY       18   /* Device atau resource sibuk */
#define EEXIST      19   /* File sudah ada */
#define EXDEV       20   /* Link melintasi device */
#define ENODEV      21   /* Operasi tidak didukung device */
#define ENOTDIR     22   /* Bukan direktori */
#define EISDIR      23   /* Adalah direktori */
#define EINVAL      24   /* Argumen tidak valid */
#define ENFILE      25   /* Tabel file penuh di sistem */
#define EMFILE      26   /* Terlalu banyak file terbuka proses */
#define ENOTTY      27   /* Ioctl tidak sesuai device */

/* Error storage */
#define ETXTBSY     28   /* File teks sibuk */
#define EFBIG       29   /* File terlalu besar */
#define ENOSPC      30   /* Tidak ada ruang di device */
#define ESPIPE      31   /* Seek tidak valid pada pipe */
#define EROFS       32   /* Filesystem read-only */
#define EMLINK      33   /* Terlalu banyak link */
#define EPIPE       34   /* Pipe rusak */

/* Error matematika tambahan */
#define EOVERFLOW   35   /* Nilai terlalu besar untuk tipe data */
#define EUNDERFLOW  36   /* Nilai terlalu kecil untuk tipe data */

/* Error jaringan */
#define ENOTSOCK    40   /* Operasi socket pada non-socket */
#define EDESTADDRREQ 41  /* Alamat tujuan diperlukan */
#define EMSGSIZE    42   /* Pesan terlalu panjang */
#define EPROTOTYPE  43   /* Protocol salah untuk socket */
#define ENOPROTOOPT 44   /* Opsi protocol tidak tersedia */
#define EPROTONOSUPPORT 45 /* Protocol tidak didukung */
#define ESOCKTNOSUPPORT 46 /* Tipe socket tidak didukung */
#define EOPNOTSUPP  47   /* Operasi tidak didukung */
#define EPFNOSUPPORT 48  /* Protocol family tidak didukung */
#define EAFNOSUPPORT 49  /* Address family tidak didukung */
#define EADDRINUSE  50   /* Alamat sudah digunakan */
#define EADDRNOTAVAIL 51 /* Alamat tidak tersedia */
#define ENETDOWN    52   /* Network down */
#define ENETUNREACH 53   /* Network unreachable */
#define ENETRESET   54   /* Network reset */
#define ECONNABORTED 55  /* Koneksi aborted */
#define ECONNRESET  56   /* Koneksi reset */
#define ENOBUFS     57   /* Tidak ada buffer space */
#define EISCONN     58   /* Sudah terkoneksi */
#define ENOTCONN    59   /* Tidak terkoneksi */
#define ESHUTDOWN   60   /* Tidak dapat kirim setelah shutdown */
#define ETIMEDOUT   61   /* Connection timed out */
#define ECONNREFUSED 62  /* Connection refused */
#define ELOOP       63   /* Terlalu banyak symbolic link */
#define ENAMETOOLONG 64  /* Nama file terlalu panjang */

/* Error filesystem tambahan */
#define ENOTEMPTY   65   /* Direktori tidak kosong */
#define EDQUOT      66   /* Disk quota exceeded */

/* Error proses */
#define EIDRM       70   /* Identifier dihapus */
#define ECHRNG      71   /* Channel number di luar jangkauan */
#define EL2NSYNC    72   /* Level 2 tidak sinkron */
#define EL3HLT      73   /* Level 3 halted */
#define EL3RST      74   /* Level 3 reset */
#define ELNRNG      75   /* Link number di luar jangkauan */
#define EUNATCH     76   /* Protocol driver tidak terpasang */
#define ENOCSI      77   /* Tidak ada CSI structure */
#define EL2HLT      78   /* Level 2 halted */
#define EBADE       79   /* Invalid exchange */
#define EBADR       80   /* Invalid request descriptor */
#define EXFULL      81   /* Exchange full */
#define ENOANO      82   /* Tidak ada anode */
#define EBADRQC     83   /* Invalid request code */
#define EBADSLT     84   /* Invalid slot */

/* Error khusus */
#define EDEADLK    85    /* Resource deadlock */
#define ENOLCK     86    /* Tidak ada record lock tersedia */
#define ENOSYS     87    /* Fungsi tidak diimplementasikan */
#define ENOMSG     88    /* Tidak ada pesan dengan tipe yang diminta */
#define EPROTO     89    /* Protocol error */

/* Error lainnya */
#define EMULTIHOP  90    /* Multihop attempted */
#define EBADMSG    91    /* Bukan data message */
#define ENODATA    92    /* Tidak ada data tersedia */
/* EOVERFLOW sudah didefinisikan di atas (35) */
#define ENOLINK    94    /* Link rusak */
/* EILSEQ sudah didefinisikan di atas (3) */
#define ENOTRECOVERABLE 96 /* State tidak recoverable */
#define EOWNERDEAD 97    /* Owner mati */
#define ERFKILL    98    /* RF-kill active */
#define EHWPOISON  99    /* Memory page hardware error */

/* Error tambahan untuk kompatibilitas */
#define EPERM           1    /* Operation not permitted (alias) */
#define EWOULDBLOCK     EAGAIN  /* Resource temporarily unavailable */
#define EDEADLOCK       EDEADLK /* Resource deadlock (alias) */

/* Error Linux tambahan */
#define EBFONT          100  /* Bad font file format */
#define ENOSTR          101  /* Device not a stream */
#define ETIME           102  /* Timer expired */
#define ENOSR           103  /* Out of streams resources */
#define ENONET          104  /* Machine is not on network */
#define ENOPKG          105  /* Package not installed */
#define EREMOTE         106  /* Object is remote */
#define EADV            107  /* Advertise error */
#define ESRMNT          108  /* Srmount error */
#define ECOMM           109  /* Communication error on send */
#define EDOTDOT         110  /* RFS specific error */
#define ENOTUNIQ        111  /* Name not unique on network */
#define EBADFD          112  /* File descriptor in bad state */
#define EREMCHG         113  /* Remote address changed */
#define ELIBACC         114  /* Cannot access shared library */
#define ELIBBAD         115  /* Accessing corrupted shared library */
#define ELIBSCN         116  /* .lib section in a.out corrupted */
#define ELIBMAX         117  /* Too many shared libraries */
#define ELIBEXEC        118  /* Cannot exec shared library directly */
#define ERESTART        119  /* Interrupted syscall should be restarted */
#define ESTRPIPE        120  /* Streams pipe error */
#define EUSERS          121  /* Too many users */
#define EHOSTDOWN       122  /* Host is down */
#define EHOSTUNREACH    123  /* No route to host */
#define EALREADY        124  /* Operation already in progress */
#define EINPROGRESS     125  /* Operation now in progress */
#define ESTALE          126  /* Stale file handle */
#define EUCLEAN         127  /* Structure needs cleaning */
#define ENOTNAM         128  /* Not a XENIX named type file */
#define ENAVAIL         129  /* No XENIX semaphores available */
#define EISNAM          130  /* Is a named type file */
#define EREMOTEIO       131  /* Remote I/O error */
#define ENOMEDIUM       132  /* No medium found */
#define EMEDIUMTYPE     133  /* Wrong medium type */
#define ECANCELED       134  /* Operation canceled */
#define ENOKEY          135  /* Required key not available */
#define EKEYEXPIRED     136  /* Key has expired */
#define EKEYREVOKED     137  /* Key has been revoked */
#define EKEYREJECTED    138  /* Key was rejected by service */

/* Error Pigura OS spesifik */
#define EPIGURA_BASE   200  /* Base untuk error khusus Pigura */

/* ============================================================
 * MAKRO PEMBANTU
 * ============================================================
 */

/* Jumlah total kode error yang dikenal */
#define EMAX        256

/* Cek apakah kode error valid */
#define IS_VALID_ERRNO(e) ((e) >= 0 && (e) < EMAX)

/* Reset errno ke 0 */
#define ERRNO_RESET() (errno = 0)

/* Set errno jika kondisi benar */
#define ERRNO_SET_IF(cond, val) do { if (cond) errno = (val); } while(0)

#endif /* LIBC_ERRNO_H */
