/*
 * PIGURA LIBC - SYS/MMAN.H
 * =========================
 * Header untuk memory management sesuai standar POSIX.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 */

#ifndef LIBC_SYS_MMAN_H
#define LIBC_SYS_MMAN_H

/* ============================================================
 * DEPENDENSI
 * ============================================================
 */
#include <sys/types.h>

/* ============================================================
 * NULL POINTER
 * ============================================================
 */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* ============================================================
 * KONSTANTA UNTUK mmap() PROTECTION
 * ============================================================
 */

#define PROT_NONE       0x00   /* Tidak ada akses */
#define PROT_READ       0x01   /* Bisa dibaca */
#define PROT_WRITE      0x02   /* Bisa ditulis */
#define PROT_EXEC       0x04   /* Bisa dieksekusi */
#define PROT_GROWSDOWN  0x08   /* Extend lock ke bawah */
#define PROT_GROWSUP    0x10   /* Extend lock ke atas */
#define PROT_SEM        0x20   /* Semaphore operations */

/* ============================================================
 * KONSTANTA UNTUK mmap() FLAGS
 * ============================================================
 */

/* Sharing flags (harus salah satu) */
#define MAP_SHARED      0x01    /* Share mapping */
#define MAP_PRIVATE     0x02    /* Private copy-on-write mapping */
#define MAP_SHARED_VALIDATE 0x03  /* Shared dengan validasi flag */

/* Mapping flags */
#define MAP_FIXED       0x10    /* Interpret addr exactly */
#define MAP_ANONYMOUS   0x20    /* Mapping tanpa file */
#define MAP_ANON        MAP_ANONYMOUS  /* Alias BSD */

/* Flags tambahan (Linux/Platform-specific) */
#define MAP_GROWSDOWN   0x0100  /* Stack-like segment */
#define MAP_DENYWRITE   0x0800  /* ETXTBSY on write */
#define MAP_EXECUTABLE  0x1000  /* Mark as executable */
#define MAP_LOCKED      0x2000  /* Lock pages in memory */
#define MAP_NORESERVE   0x4000  /* Don't reserve swap space */
#define MAP_POPULATE    0x8000  /* Populate page tables */
#define MAP_NONBLOCK    0x10000 /* Non-blocking */
#define MAP_STACK       0x20000 /* Allocate from stack segment */
#define MAP_HUGETLB     0x40000 /* Huge page allocation */
#define MAP_SYNC        0x80000 /* Synchronous mapping */
#define MAP_FIXED_NOREPLACE 0x100000 /* MAP_FIXED tapi tidak overlap */
#define MAP_UNINITIALIZED 0x4000000  /* Don't clear pages */

/* Address untuk mmap */
#define MAP_FAILED      ((void *)-1)

/* ============================================================
 * KONSTANTA UNTUK msync() FLAGS
 * ============================================================
 */

#define MS_SYNC         0x01    /* Synchronous memory sync */
#define MS_ASYNC        0x02    /* Asynchronous memory sync */
#define MS_INVALIDATE   0x04    /* Invalidate mappings */

/* ============================================================
 * KONSTANTA UNTUK mlock() FLAGS
 * ============================================================
 */

#define MCL_CURRENT     0x01    /* Lock all currently mapped */
#define MCL_FUTURE      0x02    /* Lock future mappings */
#define MCL_ONFAULT     0x04    /* Lock pages on fault */

/* ============================================================
 * KONSTANTA UNTUK madvise() ADVICE
 * ============================================================
 */

#define MADV_NORMAL     0   /* Normal access pattern */
#define MADV_RANDOM     1   /* Random access pattern */
#define MADV_SEQUENTIAL 2   /* Sequential access pattern */
#define MADV_WILLNEED   3   /* Will need these pages */
#define MADV_DONTNEED   4   /* Don't need these pages */
#define MADV_FREE       8   /* Can free pages */
#define MADV_REMOVE     9   /* Remove mapping */
#define MADV_DONTFORK   10  /* Don't inherit across fork */
#define MADV_DOFORK     11  /* Do inherit across fork */
#define MADV_MERGEABLE  12  /* KSM mergeable */
#define MADV_UNMERGEABLE 13 /* KSM unmergeable */
#define MADV_HUGEPAGE   14  /* Enable huge pages */
#define MADV_NOHUGEPAGE 15  /* Disable huge pages */
#define MADV_DONTDUMP   16  /* Exclude from core dump */
#define MADV_DODUMP     17  /* Include in core dump */
#define MADV_WIPEONFORK 18  /* Zero on fork */
#define MADV_KEEPONFORK 19  /* Don't zero on fork */
#define MADV_COLD       20  /* Deactivate pages */
#define MADV_PAGEOUT    21  /* Reclaim pages */
#define MADV_HWPOISON   100 /* Hardware poisoned */

/* ============================================================
 * KONSTANTA UNTUK mprotect() FLAGS
 * ============================================================
 */

/* Sama seperti PROT_* di atas */

/* ============================================================
 * DEKLARASI FUNGSI
 * ============================================================
 */

/*
 * mmap - Map file atau anonymous memory
 *
 * Parameter:
 *   addr   - Alamat yang diinginkan (boleh NULL)
 *   length - Panjang mapping
 *   prot   - Protection flags (PROT_*)
 *   flags  - Mapping flags (MAP_*)
 *   fd     - File descriptor (-1 untuk anonymous)
 *   offset - Offset dalam file
 *
 * Return: Pointer ke mapped area, atau MAP_FAILED jika error
 */
extern void *mmap(void *addr, size_t length, int prot, int flags,
                  int fd, off_t offset);

/*
 * mmap64 - mmap dengan offset 64-bit
 */
extern void *mmap64(void *addr, size_t length, int prot, int flags,
                    int fd, off_t offset);

/*
 * munmap - Unmap memory region
 *
 * Parameter:
 *   addr   - Alamat mapping
 *   length - Panjang region
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int munmap(void *addr, size_t length);

/*
 * mprotect - Ubah protection memory
 *
 * Parameter:
 *   addr   - Alamat region
 *   len    - Panjang region
 *   prot   - Protection baru
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int mprotect(void *addr, size_t len, int prot);

/*
 * msync - Sinkronkan memory dengan file
 *
 * Parameter:
 *   addr   - Alamat mapping
 *   length - Panjang region
 *   flags  - Flags (MS_SYNC, MS_ASYNC, MS_INVALIDATE)
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int msync(void *addr, size_t length, int flags);

/*
 * mlock - Lock pages di memory
 *
 * Parameter:
 *   addr   - Alamat awal
 *   len    - Panjang region
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int mlock(const void *addr, size_t len);

/*
 * munlock - Unlock pages
 *
 * Parameter:
 *   addr   - Alamat awal
 *   len    - Panjang region
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int munlock(const void *addr, size_t len);

/*
 * mlockall - Lock semua memory proses
 *
 * Parameter:
 *   flags - Flags (MCL_CURRENT, MCL_FUTURE, MCL_ONFAULT)
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int mlockall(int flags);

/*
 * munlockall - Unlock semua memory
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int munlockall(void);

/*
 * madvise - Beri advice tentang memory usage
 *
 * Parameter:
 *   addr   - Alamat awal
 *   length - Panjang region
 *   advice - Advice (MADV_*)
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int madvise(void *addr, size_t length, int advice);

/*
 * mincore - Cek apakah pages di memory
 *
 * Parameter:
 *   addr    - Alamat awal
 *   length  - Panjang region
 *   vec     - Buffer untuk hasil (1 byte per page)
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int mincore(void *addr, size_t length, unsigned char *vec);

/*
 * remap_file_pages - Remap file pages (Linux-specific)
 *
 * Parameter:
 *   addr    - Alamat mapping
 *   size    - Ukuran region
 *   prot    - Protection
 *   pgoff   - Page offset
 *   flags   - Flags
 *
 * Return: Alamat jika berhasil, MAP_FAILED jika error
 */
extern void *remap_file_pages(void *addr, size_t size, int prot,
                              size_t pgoff, int flags);

/*
 * memfd_create - Buat anonymous file (Linux-specific)
 *
 * Parameter:
 *   name  - Nama file
 *   flags - Flags (MFD_*)
 *
 * Return: File descriptor, atau -1 jika error
 */
extern int memfd_create(const char *name, unsigned int flags);

/* Flags untuk memfd_create */
#define MFD_CLOEXEC       0x0001U
#define MFD_ALLOW_SEALING 0x0002U
#define MFD_HUGETLB       0x0004U
#define MFD_NOEXEC_SEAL   0x0008U

/*
 * mremap - Remap memory region (Linux-specific)
 *
 * Parameter:
 *   old_addr - Alamat lama
 *   old_size - Ukuran lama
 *   new_size - Ukuran baru
 *   flags    - Flags
 *   ...      - new_addr jika MREMAP_FIXED
 *
 * Return: Alamat baru, atau MAP_FAILED jika error
 */
extern void *mremap(void *old_addr, size_t old_size, size_t new_size,
                    int flags, ...);

/* Flags untuk mremap */
#define MREMAP_MAYMOVE    1
#define MREMAP_FIXED      2
#define MREMAP_DONTUNMAP  4

/*
 * pkey_mprotect - Memory protection dengan protection key
 *
 * Parameter:
 *   addr - Alamat region
 *   len  - Panjang region
 *   prot - Protection
 *   pkey - Protection key
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int pkey_mprotect(void *addr, size_t len, int prot, int pkey);

/*
 * pkey_alloc - Alokasi protection key
 *
 * Parameter:
 *   flags   - Flags (harus 0)
 *   init_val - Initial value (PKEY_DISABLE_*)
 *
 * Return: Key jika berhasil, -1 jika error
 */
extern int pkey_alloc(unsigned int flags, unsigned int init_val);

/*
 * pkey_free - Bebaskan protection key
 *
 * Parameter:
 *   pkey - Protection key
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int pkey_free(int pkey);

/* Flags untuk pkey */
#define PKEY_DISABLE_ACCESS  0x1
#define PKEY_DISABLE_WRITE   0x2

/* ============================================================
 * SHARED MEMORY (POSIX)
 * ============================================================
 */

/*
 * shm_open - Buka shared memory object
 *
 * Parameter:
 *   name  - Nama object
 *   oflag - Open flags
 *   mode  - Permission mode
 *
 * Return: File descriptor, atau -1 jika error
 */
extern int shm_open(const char *name, int oflag, mode_t mode);

/*
 * shm_unlink - Hapus shared memory object
 *
 * Parameter:
 *   name - Nama object
 *
 * Return: 0 jika berhasil, -1 jika error
 */
extern int shm_unlink(const char *name);

#endif /* LIBC_SYS_MMAN_H */
