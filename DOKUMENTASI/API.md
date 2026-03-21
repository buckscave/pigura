# API REFERENCE PIGURA OS

## Daftar Isi

1. [Pigura C90](#pigura-c90)
2. [Kernel API](#kernel-api)
3. [LibC API](#libc-api)
4. [LibPigura API](#libpigura-api)
5. [HAL API](#hal-api)
6. [VFS API](#vfs-api)
7. [System Calls](#system-calls)

---

## Pigura C90

### Definisi

Pigura C90 adalah standar bahasa khusus untuk Pigura OS:

```
PIGURA C90 = C89 Core + POSIX Safe Functions
```

### Filosofi

Setiap tugas hanya memiliki **SATU fungsi** - versi aman:

| Dihapus (Unsafe) | Tersedia (Safe) |
|------------------|-----------------|
| `sprintf` | `snprintf` |
| `strcpy` | `strncpy` |
| `strcat` | `strncat` |
| `gets` | `fgets` |
| `strtok` | `strtok_r` |
| `strerror` | `strerror_r` |

### Tipe Data Kustom

Pigura OS mendefinisikan tipe data sendiri dalam bahasa Indonesia:

```c
/* Tipe bertanda */
typedef signed char        tanda8_t;
typedef signed short       tanda16_t;
typedef signed int         tanda32_t;
typedef signed long long   tanda64_t;

/* Tipe tak bertanda */
typedef unsigned char      tak_bertanda8_t;
typedef unsigned short     tak_bertanda16_t;
typedef unsigned int       tak_bertanda32_t;
typedef unsigned long long tak_bertanda64_t;

/* Alias portabel */
typedef tak_bertanda8_t    byte_t;
typedef tak_bertanda16_t   kata_t;
typedef tak_bertanda32_t   dwita_t;
typedef tak_bertanda64_t   kwita_t;

/* Tipe ukuran */
typedef unsigned long      ukuran_t;
typedef signed long        jarak_t;

/* Alamat memori */
typedef tak_bertanda64_t   alamat_fisik_t;
typedef tak_bertanda64_t   alamat_virtual_t;

/* Proses */
typedef tak_bertanda32_t   pid_t;
typedef tak_bertanda32_t   tid_t;
typedef tak_bertanda32_t   uid_t;
typedef tak_bertanda32_t   gid_t;

/* Boolean */
typedef enum {
    SALAH = 0,
    BENAR = 1
} bool_t;

/* Status operasi */
typedef enum {
    STATUS_BERHASIL = 0,
    STATUS_GAGAL = -1,
    STATUS_MEMORI_HABIS = -2,
    STATUS_PARAM_INVALID = -3,
    STATUS_TIDAK_DITEMUKAN = -4,
    STATUS_AKSES_DITOLAK = -5
} status_t;
```

---

## Kernel API

### Fungsi Utama

#### kernel_main

Entry point utama kernel.

```c
void kernel_main(tak_bertanda32_t magic, multiboot_info_t *bootinfo);
```

| Parameter | Tipe | Deskripsi |
|-----------|------|-----------|
| magic | `tak_bertanda32_t` | Magic number multiboot |
| bootinfo | `multiboot_info_t*` | Pointer ke info bootloader |

#### kernel_init

Inisialisasi kernel.

```c
status_t kernel_init(multiboot_info_t *bootinfo);
```

**Return:** `STATUS_BERHASIL` jika sukses

#### kernel_start

Mulai operasi normal kernel.

```c
void kernel_start(void) __attribute__((noreturn));
```

#### kernel_shutdown

Shutdown kernel dengan aman.

```c
void kernel_shutdown(int reboot) __attribute__((noreturn));
```

| Parameter | Tipe | Deskripsi |
|-----------|------|-----------|
| reboot | `int` | 0 = shutdown, 1 = reboot |

### Output Console

#### kernel_printf

Print formatted string ke console kernel.

```c
int kernel_printf(const char *format, ...);
```

| Parameter | Tipe | Deskripsi |
|-----------|------|-----------|
| format | `const char*` | Format string |
| ... | - | Argumen variadic |

**Return:** Jumlah karakter yang ditulis

#### kernel_printk

Print ke kernel log dengan level.

```c
int kernel_printk(int level, const char *format, ...);
```

| Parameter | Tipe | Deskripsi |
|-----------|------|-----------|
| level | `int` | Level log (1=error, 2=warn, 3=info, 4=debug) |
| format | `const char*` | Format string |

**Makro helper:**

```c
#define log_error(fmt, ...) kernel_printk(1, "[ERROR] " fmt, ##__VA_ARGS__)
#define log_warn(fmt, ...)  kernel_printk(2, "[WARN] " fmt, ##__VA_ARGS__)
#define log_info(fmt, ...)  kernel_printk(3, "[INFO] " fmt, ##__VA_ARGS__)
#define log_debug(fmt, ...) kernel_printk(4, "[DEBUG] " fmt, ##__VA_ARGS__)
```

#### kernel_puts

Print string ke console.

```c
int kernel_puts(const char *str);
```

#### kernel_putchar

Print satu karakter.

```c
int kernel_putchar(int c);
```

### Memory Management

#### kmalloc

Alokasikan memori dari kernel heap.

```c
void *kmalloc(ukuran_t size);
```

| Parameter | Tipe | Deskripsi |
|-----------|------|-----------|
| size | `ukuran_t` | Ukuran dalam byte |

**Return:** Pointer ke memori, atau NULL jika gagal

#### kfree

Bebaskan memori.

```c
void kfree(void *ptr);
```

#### kcalloc

Alokasi memori dan clear.

```c
void *kcalloc(ukuran_t num, ukuran_t size);
```

#### krealloc

Realokasi memori.

```c
void *krealloc(void *ptr, ukuran_t size);
```

### Paging

#### paging_init

Inisialisasi sistem paging.

```c
status_t paging_init(tak_bertanda64_t mem_size);
```

#### paging_map_page

Map satu halaman.

```c
status_t paging_map_page(alamat_virtual_t vaddr, alamat_fisik_t paddr,
                         tak_bertanda32_t flags);
```

| Parameter | Tipe | Deskripsi |
|-----------|------|-----------|
| vaddr | `alamat_virtual_t` | Alamat virtual |
| paddr | `alamat_fisik_t` | Alamat fisik |
| flags | `tak_bertanda32_t` | Permission flags |

**Flags:**

```c
#define PAGE_PRESENT    0x001
#define PAGE_WRITABLE   0x002
#define PAGE_USER       0x004
#define PAGE_NOEXEC     0x200
```

#### paging_unmap_page

Unmap satu halaman.

```c
status_t paging_unmap_page(alamat_virtual_t vaddr);
```

### Process Management

#### proses_init

Inisialisasi subsistem proses.

```c
status_t proses_init(void);
```

#### proses_create

Buat proses baru.

```c
pid_t proses_create(const char *nama, pid_t ppid, tak_bertanda32_t flags);
```

| Parameter | Tipe | Deskripsi |
|-----------|------|-----------|
| nama | `const char*` | Nama proses |
| ppid | `pid_t` | Parent PID |
| flags | `tak_bertanda32_t` | Flags pembuatan |

**Return:** PID proses baru, atau `PID_INVALID` jika gagal

#### proses_exit

Exit proses.

```c
status_t proses_exit(pid_t pid, tanda32_t exit_code);
```

#### proses_cari

Cari proses berdasarkan PID.

```c
proses_t *proses_cari(pid_t pid);
```

#### proses_get_current

Dapatkan proses saat ini.

```c
proses_t *proses_get_current(void);
```

---

## LibC API

### stdio.h

#### snprintf

Format string ke buffer dengan batas ukuran.

```c
int snprintf(char *str, ukuran_t size, const char *format, ...);
```

| Parameter | Tipe | Deskripsi |
|-----------|------|-----------|
| str | `char*` | Buffer output |
| size | `ukuran_t` | Ukuran buffer |
| format | `const char*` | Format string |

**Return:** Jumlah karakter yang akan ditulis (tanpa null terminator)

#### vsnprintf

Format string ke buffer dengan va_list.

```c
int vsnprintf(char *str, ukuran_t size, const char *format, va_list ap);
```

#### printf

Print ke stdout.

```c
int printf(const char *format, ...);
```

#### fprintf

Print ke file stream.

```c
int fprintf(FILE *stream, const char *format, ...);
```

#### fopen

Buka file.

```c
FILE *fopen(const char *pathname, const char *mode);
```

| Mode | Deskripsi |
|------|-----------|
| "r" | Baca |
| "w" | Tulis (truncate) |
| "a" | Append |
| "r+" | Baca dan tulis |
| "w+" | Baca dan tulis (truncate) |
| "a+" | Baca dan append |

#### fclose

Tutup file.

```c
int fclose(FILE *stream);
```

#### fread

Baca dari file.

```c
ukuran_t fread(void *ptr, ukuran_t size, ukuran_t nmemb, FILE *stream);
```

#### fwrite

Tulis ke file.

```c
ukuran_t fwrite(const void *ptr, ukuran_t size, ukuran_t nmemb, FILE *stream);
```

#### fgets

Baca string dari file (safe).

```c
char *fgets(char *s, int size, FILE *stream);
```

#### fputs

Tulis string ke file.

```c
int fputs(const char *s, FILE *stream);
```

### stdlib.h

#### malloc

Alokasi memori dinamis.

```c
void *malloc(ukuran_t size);
```

#### free

Bebaskan memori.

```c
void free(void *ptr);
```

#### atoi

Konversi string ke integer.

```c
int atoi(const char *nptr);
```

#### strtol

Konversi string ke long dengan error handling.

```c
long strtol(const char *nptr, char **endptr, int base);
```

| Parameter | Tipe | Deskripsi |
|-----------|------|-----------|
| nptr | `const char*` | String input |
| endptr | `char**` | Pointer ke karakter pertama yang tidak dikonversi |
| base | `int` | Basis numerik (2-36, atau 0 untuk auto-detect) |

#### rand

Generate angka random.

```c
int rand(void);
```

#### srand

Seed random generator.

```c
void srand(unsigned int seed);
```

#### qsort

Quick sort.

```c
void qsort(void *base, ukuran_t nmemb, ukuran_t size,
           int (*compar)(const void *, const void *));
```

#### bsearch

Binary search.

```c
void *bsearch(const void *key, const void *base, ukuran_t nmemb,
              ukuran_t size, int (*compar)(const void *, const void *));
```

#### exit

Terminasi program.

```c
void exit(int status) __attribute__((noreturn));
```

### string.h

#### strncpy

Copy string dengan batas (safe).

```c
char *strncpy(char *dest, const char *src, ukuran_t n);
```

**Catatan:** Selalu null-terminate jika ada ruang.

#### strncat

Concatenate string dengan batas (safe).

```c
char *strncat(char *dest, const char *src, ukuran_t n);
```

#### strlen

Hitung panjang string.

```c
ukuran_t strlen(const char *s);
```

#### strcmp

Compare dua string.

```c
int strcmp(const char *s1, const char *s2);
```

#### strncmp

Compare dua string dengan batas.

```c
int strncmp(const char *s1, const char *s2, ukuran_t n);
```

#### strchr

Cari karakter pertama dalam string.

```c
char *strchr(const char *s, int c);
```

#### strrchr

Cari karakter terakhir dalam string.

```c
char *strrchr(const char *s, int c);
```

#### strstr

Cari substring.

```c
char *strstr(const char *haystack, const char *needle);
```

#### strtok_r

Tokenize string (reentrant).

```c
char *strtok_r(char *str, const char *delim, char **saveptr);
```

#### memset

Set memori dengan nilai.

```c
void *memset(void *s, int c, ukuran_t n);
```

#### memcpy

Copy memori.

```c
void *memcpy(void *dest, const void *src, ukuran_t n);
```

#### memmove

Move memori (handles overlap).

```c
void *memmove(void *dest, const void *src, ukuran_t n);
```

#### memcmp

Compare memori.

```c
int memcmp(const void *s1, const void *s2, ukuran_t n);
```

### ctype.h

```c
int isalpha(int c);    // Alfabet?
int isdigit(int c);    // Digit?
int isalnum(int c);    // Alfanumerik?
int isspace(int c);    // Whitespace?
int isupper(int c);    // Huruf besar?
int islower(int c);    // Huruf kecil?
int isprint(int c);    // Printable?
int iscntrl(int c);    // Control character?
int isgraph(int c);    // Graphical?
int ispunct(int c);    // Punctuation?
int isxdigit(int c);   // Hex digit?

int tolower(int c);    // Konversi ke huruf kecil
int toupper(int c);    // Konversi ke huruf besar
```

### unistd.h

```c
/* I/O */
ssize_t read(int fd, void *buf, ukuran_t count);
ssize_t write(int fd, const void *buf, ukuran_t count);
int close(int fd);

/* File */
int open(const char *pathname, int flags, ...);
int creat(const char *pathname, mode_t mode);
int unlink(const char *pathname);
int access(const char *pathname, int mode);

/* Process */
pid_t fork(void);
int execve(const char *pathname, char *const argv[], char *const envp[]);
pid_t getpid(void);
pid_t getppid(void);

/* Directory */
int chdir(const char *path);
char *getcwd(char *buf, ukuran_t size);

/* Time */
unsigned int sleep(unsigned int seconds);
int usleep(useconds_t usec);

/* Duplication */
int dup(int oldfd);
int dup2(int oldfd, int newfd);
```

---

## LibPigura API

### Kanvas

#### kanvas_buat

Buat kanvas baru.

```c
kanvas_t *kanvas_buat(int lebar, int tinggi);
```

#### kanvas_hapus

Hapus kanvas.

```c
void kanvas_hapus(kanvas_t *kanvas);
```

#### kanvas_ubahsuai

Ubah ukuran kanvas.

```c
status_t kanvas_ubahsuai(kanvas_t *kanvas, int lebar_baru, int tinggi_baru);
```

### Rendering

#### render_titik

Gambar titik.

```c
void render_titik(kanvas_t *kanvas, int x, int y, tak_bertanda32_t warna);
```

#### render_garis

Gambar garis.

```c
void render_garis(kanvas_t *kanvas, int x1, int y1, int x2, int y2,
                  tak_bertanda32_t warna);
```

#### render_kotak

Gambar kotak.

```c
void render_kotak(kanvas_t *kanvas, int x, int y, int lebar, int tinggi,
                  tak_bertanda32_t warna, bool_t isi);
```

#### render_lingkaran

Gambar lingkaran.

```c
void render_lingkaran(kanvas_t *kanvas, int cx, int cy, int radius,
                      tak_bertanda32_t warna, bool_t isi);
```

#### render_teks

Render teks.

```c
void render_teks(kanvas_t *kanvas, int x, int y, const char *teks,
                 font_t *font, tak_bertanda32_t warna);
```

### Font

#### font_muat

Muat font dari file.

```c
font_t *font_muat(const char *path);
```

#### font_hapus

Hapus font.

```c
void font_hapus(font_t *font);
```

---

## HAL API

### CPU Operations

```c
void hal_cpu_halt(void);
void hal_cpu_reset(void);
tak_bertanda32_t hal_cpu_get_id(void);
void hal_cpu_enable_irq(void);
void hal_cpu_disable_irq(void);
```

### Memory Operations

```c
status_t hal_memori_init(tak_bertanda64_t mem_size);
alamat_fisik_t hal_memori_alloc_page(void);
void hal_memori_free_page(alamat_fisik_t addr);
status_t hal_memori_map(alamat_virtual_t vaddr, alamat_fisik_t paddr,
                        tak_bertanda32_t flags);
```

### Interrupt Operations

```c
status_t hal_interupsi_init(void);
status_t hal_interupsi_register(tak_bertanda32_t vector,
                                 handler_interupsi_t handler);
void hal_interupsi_enable(tak_bertanda32_t vector);
void hal_interupsi_disable(tak_bertanda32_t vector);
```

### Timer Operations

```c
status_t hal_timer_init(tak_bertanda32_t frequency);
tak_bertanda64_t hal_timer_get_ticks(void);
void hal_timer_delay(tak_bertanda32_t ms);
```

### Console Operations

```c
status_t hal_console_init(void);
void hal_console_putchar(char c);
void hal_console_puts(const char *str);
void hal_console_clear(void);
```

---

## VFS API

### Filesystem Registration

```c
status_t vfs_register(filesystem_t *fs);
status_t vfs_unregister(const char *name);
```

### Mount Operations

```c
status_t vfs_mount(const char *source, const char *target,
                   const char *fstype, tak_bertanda32_t flags);
status_t vfs_umount(const char *target);
```

### File Operations

```c
int vfs_open(const char *pathname, int flags, mode_t mode);
ssize_t vfs_read(int fd, void *buf, ukuran_t count);
ssize_t vfs_write(int fd, const void *buf, ukuran_t count);
int vfs_close(int fd);
off_t vfs_lseek(int fd, off_t offset, int whence);
```

### Directory Operations

```c
int vfs_mkdir(const char *pathname, mode_t mode);
int vfs_rmdir(const char *pathname);
int vfs_chdir(const char *path);
DIR *vfs_opendir(const char *name);
struct dirent *vfs_readdir(DIR *dirp);
int vfs_closedir(DIR *dirp);
```

---

## System Calls

### Daftar System Call

| ID | Nama | Deskripsi |
|----|------|-----------|
| 0 | sys_read | Baca dari file descriptor |
| 1 | sys_write | Tulis ke file descriptor |
| 2 | sys_open | Buka file |
| 3 | sys_close | Tutup file descriptor |
| 4 | sys_fork | Duplikasi proses |
| 5 | sys_exec | Eksekusi program |
| 6 | sys_exit | Terminasi proses |
| 7 | sys_getpid | Dapatkan process ID |
| 8 | sys_wait | Tunggu child process |
| 9 | sys_kill | Kirim signal |
| 10 | sys_mmap | Map memori |
| 11 | sys_munmap | Unmap memori |
| 12 | sys_brk | Ubah break pointer |
| 13 | sys_ioctl | Device control |
| 14 | sys_dup | Duplikasi fd |
| 15 | sys_dup2 | Duplikasi fd ke fd tertentu |
| 16 | sys_pipe | Buat pipe |
| 17 | sys_mkdir | Buat direktori |
| 18 | sys_rmdir | Hapus direktori |
| 19 | sys_unlink | Hapus file |
| 20 | sys_chdir | Ubah direktori kerja |
| 21 | sys_getcwd | Dapatkan direktori kerja |
| 22 | sys_time | Dapatkan waktu |
| 23 | sys_sleep | Sleep proses |
| 24 | sys_yield | Yield CPU |

### Cara Pemanggilan Per Arsitektur

#### x86 (32-bit)

```assembly
; System call x86
mov eax, syscall_number
mov ebx, arg1
mov ecx, arg2
mov edx, arg3
int 0x80
; Return value di eax
```

#### x86_64

```assembly
; System call x86_64
mov rax, syscall_number
mov rdi, arg1
mov rsi, arg2
mov rdx, arg3
mov r10, arg4
mov r8,  arg5
mov r9,  arg6
syscall
; Return value di rax
```

#### ARM/ARMv7

```assembly
; System call ARM
mov r0, syscall_number
mov r1, arg1
mov r2, arg2
mov r3, arg3
swi #0
; Return value di r0
```

#### ARM64

```assembly
; System call ARM64
mov x8, syscall_number
mov x0, arg1
mov x1, arg2
mov x2, arg3
mov x3, arg4
mov x4, arg5
mov x5, arg6
svc #0
; Return value di x0
```

---

## Error Codes

```c
#define ESUCCES        0    /* Sukses */
#define EPERM          1    /* Operation not permitted */
#define ENOENT         2    /* No such file or directory */
#define ESRCH          3    /* No such process */
#define EINTR          4    /* Interrupted system call */
#define EIO            5    /* I/O error */
#define ENXIO          6    /* No such device or address */
#define E2BIG          7    /* Argument list too long */
#define ENOEXEC        8    /* Exec format error */
#define EBADF          9    /* Bad file descriptor */
#define ECHILD        10    /* No child process */
#define EAGAIN        11    /* Try again */
#define ENOMEM        12    /* Out of memory */
#define EACCES        13    /* Permission denied */
#define EFAULT        14    /* Bad address */
#define ENOTBLK       15    /* Block device required */
#define EBUSY         16    /* Device or resource busy */
#define EEXIST        17    /* File exists */
#define EXDEV         18    /* Cross-device link */
#define ENODEV        19    /* No such device */
#define ENOTDIR       20    /* Not a directory */
#define EISDIR        21    /* Is a directory */
#define EINVAL        22    /* Invalid argument */
#define ENFILE        23    /* File table overflow */
#define EMFILE        24    /* Too many open files */
#define ENOTTY        25    /* Not a typewriter */
#define EFBIG         27    /* File too large */
#define ENOSPC        28    /* No space left on device */
#define ESPIPE        29    /* Illegal seek */
#define EROFS         30    /* Read-only file system */
#define EMLINK        31    /* Too many links */
#define EPIPE         32    /* Broken pipe */
#define EDOM          33    /* Math argument out of domain */
#define ERANGE        34    /* Math result not representable */
```

---

## Ringkasan

API Pigura OS dirancang dengan prinsip:

1. **Aman** - Semua fungsi string/memory bounded
2. **Konsisten** - Penamaan dan return value seragam
3. **Minimalis** - Hanya fungsi yang diperlukan
4. **Portabel** - Abstraksi per-arsitektur via HAL
5. **Indonesia** - Penamaan dalam bahasa Indonesia