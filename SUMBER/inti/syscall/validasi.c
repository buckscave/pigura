/*
 * PIGURA OS - VALIDASI.C
 * -----------------------
 * Implementasi validasi parameter system call.
 *
 * Berkas ini berisi fungsi-fungsi untuk memvalidasi parameter
 * yang diberikan oleh user space ke system call.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../kernel.h"

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

/* Batas alokasi maksimum (untuk mencegah DoS) */
#define MAX_ALLOC_SIZE          (128UL * 1024UL * 1024UL)  /* 128 MB */

/* Batas string maksimum */
#define MAX_STRING_SIZE         (4UL * 1024UL)  /* 4 KB */

/* Batas array maksimum */
#define MAX_ARRAY_COUNT         (16UL * 1024UL)  /* 16K elements */

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Statistik validasi */
static struct {
    tak_bertanda64_t pointer_checks;
    tak_bertanda64_t string_checks;
    tak_bertanda64_t buffer_checks;
    tak_bertanda64_t failures;
} validasi_stats = {0};

/* Status inisialisasi */
static bool_t validasi_initialized = SALAH;

/* Alamat batas user space */
static alamat_virtual_t user_space_start = ALAMAT_USER_MULAI;
static alamat_virtual_t user_space_end = ALAMAT_USER_AKHIR;

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * is_user_address
 * ---------------
 * Cek apakah alamat berada dalam range user space.
 *
 * Parameter:
 *   addr - Alamat yang dicek
 *
 * Return: BENAR jika valid user address
 */
static bool_t is_user_address(alamat_virtual_t addr)
{
    return (addr >= user_space_start && addr < user_space_end);
}

/*
 * is_page_accessible
 * ------------------
 * Cek apakah halaman dapat diakses.
 *
 * Parameter:
 *   addr   - Alamat dalam halaman
 *   write  - Apakah akses tulis
 *
 * Return: BENAR jika dapat diakses
 */
static bool_t is_page_accessible(alamat_virtual_t addr, bool_t write)
{
    proses_t *proses;
    alamat_fisik_t phys;
    tak_bertanda32_t flags;

    proses = proses_get_current();
    if (proses == NULL || proses->vm == NULL) {
        return SALAH;
    }

    /* Query VM untuk info mapping */
    if (!vm_query(proses->vm, addr, &phys, &flags, NULL)) {
        return SALAH;
    }

    /* Cek permission */
    if (write && !(flags & VMA_FLAG_WRITE)) {
        return SALAH;
    }

    return BENAR;
}

/*
 * check_page_range
 * ----------------
 * Cek range halaman.
 *
 * Parameter:
 *   start  - Alamat awal
 *   len    - Panjang
 *   write  - Apakah akses tulis
 *
 * Return: BENAR jika seluruh range valid
 */
static bool_t check_page_range(alamat_virtual_t start, ukuran_t len,
                               bool_t write)
{
    alamat_virtual_t addr;
    alamat_virtual_t end;

    /* Handle wraparound */
    if (start + len < start) {
        return SALAH;
    }

    end = start + len;

    /* Align ke halaman */
    addr = RATAKAN_BAWAH(start, UKURAN_HALAMAN);

    while (addr < end) {
        if (!is_user_address(addr)) {
            return SALAH;
        }

        if (!is_page_accessible(addr, write)) {
            return SALAH;
        }

        addr += UKURAN_HALAMAN;
    }

    return BENAR;
}

/*
 * ============================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
 * ============================================================================
 */

/*
 * validasi_init
 * -------------
 * Inisialisasi subsistem validasi.
 *
 * Return: Status operasi
 */
status_t validasi_init(void)
{
    if (validasi_initialized) {
        return STATUS_SUDAH_ADA;
    }

    kernel_memset(&validasi_stats, 0, sizeof(validasi_stats));
    validasi_initialized = BENAR;

    kernel_printf("[VALIDASI] Parameter validator initialized\n");

    return STATUS_BERHASIL;
}

/*
 * validasi_pointer_user
 * ---------------------
 * Validasi pointer dari user space.
 *
 * Parameter:
 *   ptr - Pointer yang divalidasi
 *
 * Return: BENAR jika valid
 */
bool_t validasi_pointer_user(const void *ptr)
{
    alamat_virtual_t addr;

    validasi_stats.pointer_checks++;

    if (ptr == NULL) {
        validasi_stats.failures++;
        return SALAH;
    }

    addr = (alamat_virtual_t)(alamat_ptr_t)ptr;

    if (!is_user_address(addr)) {
        validasi_stats.failures++;
        return SALAH;
    }

    /* Cek minimal halaman pertama */
    if (!is_page_accessible(addr, SALAH)) {
        validasi_stats.failures++;
        return SALAH;
    }

    return BENAR;
}

/*
 * validasi_pointer_user_write
 * ---------------------------
 * Validasi pointer writable dari user space.
 *
 * Parameter:
 *   ptr - Pointer yang divalidasi
 *
 * Return: BENAR jika valid dan writable
 */
bool_t validasi_pointer_user_write(void *ptr)
{
    alamat_virtual_t addr;

    validasi_stats.pointer_checks++;

    if (ptr == NULL) {
        validasi_stats.failures++;
        return SALAH;
    }

    addr = (alamat_virtual_t)(alamat_ptr_t)ptr;

    if (!is_user_address(addr)) {
        validasi_stats.failures++;
        return SALAH;
    }

    /* Cek writable */
    if (!is_page_accessible(addr, BENAR)) {
        validasi_stats.failures++;
        return SALAH;
    }

    return BENAR;
}

/*
 * validasi_buffer_user
 * --------------------
 * Validasi buffer dari user space.
 *
 * Parameter:
 *   buf    - Pointer ke buffer
 *   len    - Panjang buffer
 *   write  - Apakah buffer harus writable
 *
 * Return: BENAR jika valid
 */
bool_t validasi_buffer_user(const void *buf, ukuran_t len, bool_t write)
{
    alamat_virtual_t addr;

    validasi_stats.buffer_checks++;

    /* NULL dengan panjang 0 valid */
    if (buf == NULL && len == 0) {
        return BENAR;
    }

    /* NULL dengan panjang > 0 invalid */
    if (buf == NULL) {
        validasi_stats.failures++;
        return SALAH;
    }

    /* Len 0 valid */
    if (len == 0) {
        return BENAR;
    }

    /* Cek overflow */
    addr = (alamat_virtual_t)(alamat_ptr_t)buf;
    if (addr + len < addr) {
        validasi_stats.failures++;
        return SALAH;
    }

    /* Cek range user */
    if (!check_page_range(addr, len, write)) {
        validasi_stats.failures++;
        return SALAH;
    }

    return BENAR;
}

/*
 * validasi_string_user_internal
 * ------------------------------
 * Validasi string dari user space dengan panjang maksimum.
 * Fungsi internal yang digunakan oleh validasi_string_user.
 *
 * Parameter:
 *   str - Pointer ke string
 *   max_len - Panjang maksimum
 *
 * Return: BENAR jika valid
 */
static bool_t validasi_string_user_internal(const char *str, ukuran_t max_len)
{
    alamat_virtual_t addr;
    ukuran_t len;
    ukuran_t checked;
    const char *p;

    if (str == NULL) {
        validasi_stats.failures++;
        return SALAH;
    }

    /* Gunakan default max jika 0 */
    if (max_len == 0) {
        max_len = MAX_STRING_SIZE;
    }

    addr = (alamat_virtual_t)(alamat_ptr_t)str;

    /* Cek alamat awal */
    if (!is_user_address(addr)) {
        validasi_stats.failures++;
        return SALAH;
    }

    /* Cek akses halaman pertama */
    if (!is_page_accessible(addr, SALAH)) {
        validasi_stats.failures++;
        return SALAH;
    }

    /* Scan string karakter per karakter */
    len = 0;
    checked = 0;
    p = str;

    while (len < max_len) {
        /* Cek batas halaman setiap UKURAN_HALAMAN karakter */
        if (checked >= UKURAN_HALAMAN) {
            if (!is_user_address((alamat_virtual_t)(alamat_ptr_t)p)) {
                validasi_stats.failures++;
                return SALAH;
            }
            if (!is_page_accessible((alamat_virtual_t)(alamat_ptr_t)p,
                                    SALAH)) {
                validasi_stats.failures++;
                return SALAH;
            }
            checked = 0;
        }

        /* Cek null terminator */
        if (*p == '\0') {
            return BENAR;
        }

        len++;
        checked++;
        p++;
    }

    /* String terlalu panjang */
    validasi_stats.failures++;
    return SALAH;
}

/*
 * validasi_string_user
 * --------------------
 * Validasi string dari user space.
 *
 * Parameter:
 *   str - Pointer ke string
 *
 * Return: BENAR jika valid
 */
bool_t validasi_string_user(const char *str)
{
    validasi_stats.string_checks++;
    return validasi_string_user_internal(str, MAX_STRING_SIZE);
}

/*
 * validasi_string_user_n
 * ----------------------
 * Validasi string dari user space dengan panjang maksimum tertentu.
 *
 * Parameter:
 *   str - Pointer ke string
 *   max_len - Panjang maksimum
 *
 * Return: BENAR jika valid
 */
bool_t validasi_string_user_n(const char *str, ukuran_t max_len)
{
    validasi_stats.string_checks++;
    return validasi_string_user_internal(str, max_len);
}

/*
 * validasi_string_array_user
 * --------------------------
 * Validasi array string dari user space.
 *
 * Parameter:
 *   arr     - Pointer ke array
 *   max_count - Jumlah maksimum elemen
 *
 * Return: BENAR jika valid
 */
bool_t validasi_string_array_user(char *const arr[], ukuran_t max_count)
{
    ukuran_t i;

    if (arr == NULL) {
        return SALAH;
    }

    if (max_count == 0) {
        max_count = MAX_ARRAY_COUNT;
    }

    /* Validasi pointer array */
    if (!validasi_pointer_user(arr)) {
        return SALAH;
    }

    /* Validasi setiap string */
    for (i = 0; i < max_count; i++) {
        /* Validasi pointer ke string */
        if (!validasi_pointer_user(&arr[i])) {
            return SALAH;
        }

        /* NULL menandakan akhir array */
        if (arr[i] == NULL) {
            return BENAR;
        }

        /* Validasi string itu sendiri */
        if (!validasi_string_user_n(arr[i], 0)) {
            return SALAH;
        }
    }

    /* Array tidak di-NULL-terminate dalam batas */
    return SALAH;
}

/*
 * validasi_size
 * -------------
 * Validasi ukuran alokasi.
 *
 * Parameter:
 *   size - Ukuran yang divalidasi
 *
 * Return: BENAR jika valid
 */
bool_t validasi_size(ukuran_t size)
{
    if (size > MAX_ALLOC_SIZE) {
        validasi_stats.failures++;
        return SALAH;
    }

    return BENAR;
}

/*
 * validasi_fd
 * -----------
 * Validasi file descriptor.
 *
 * Parameter:
 *   fd - File descriptor
 *
 * Return: BENAR jika valid
 */
bool_t validasi_fd(tanda32_t fd)
{
    if (fd < 0 || fd >= MAKS_FD_PER_PROSES) {
        validasi_stats.failures++;
        return SALAH;
    }

    return BENAR;
}

/*
 * validasi_signal
 * ---------------
 * Validasi nomor signal.
 *
 * Parameter:
 *   sig - Nomor signal
 *
 * Return: BENAR jika valid
 */
bool_t validasi_signal(tanda32_t sig)
{
    if (sig < 1 || sig > MAKS_SIGNAL) {
        validasi_stats.failures++;
        return SALAH;
    }

    return BENAR;
}

/*
 * validasi_pid
 * ------------
 * Validasi Process ID.
 *
 * Parameter:
 *   pid - Process ID
 *
 * Return: BENAR jika valid
 */
bool_t validasi_pid(pid_t pid)
{
    /* pid > 0 adalah proses spesifik */
    /* pid == -1 adalah any process */
    /* pid == 0 adalah proses group */
    /* pid < -1 adalah proses group spesifik */

    if ((tanda32_t)pid > (tanda32_t)CONFIG_MAKS_PROSES) {
        validasi_stats.failures++;
        return SALAH;
    }

    return BENAR;
}

/*
 * validasi_path
 * -------------
 * Validasi path string.
 *
 * Parameter:
 *   path - Pointer ke path
 *
 * Return: BENAR jika valid
 */
bool_t validasi_path(const char *path)
{
    ukuran_t len;

    if (!validasi_string_user_n(path, MAKS_PATH_LEN)) {
        return SALAH;
    }

    len = kernel_strlen(path);

    /* Path tidak boleh kosong */
    if (len == 0) {
        validasi_stats.failures++;
        return SALAH;
    }

    /* Path harus dimulai dengan '/' */
    if (path[0] != '/') {
        /* Relative path - cek current working directory */
        proses_t *proses = proses_get_current();
        if (proses == NULL || proses->cwd[0] == '\0') {
            validasi_stats.failures++;
            return SALAH;
        }
    }

    return BENAR;
}

/*
 * validasi_mode
 * -------------
 * Validasi mode/permission.
 *
 * Parameter:
 *   mode - Mode yang divalidasi
 *
 * Return: BENAR jika valid
 */
bool_t validasi_mode(mode_t mode)
{
    /* Mode hanya boleh memiliki bit permission yang valid */
    if (mode & ~07777) {
        validasi_stats.failures++;
        return SALAH;
    }

    return BENAR;
}

/*
 * validasi_flags
 * --------------
 * Validasi flags.
 *
 * Parameter:
 *   flags      - Flags yang divalidasi
 *   valid_mask - Mask flag yang valid
 *
 * Return: BENAR jika valid
 */
bool_t validasi_flags(tak_bertanda32_t flags, tak_bertanda32_t valid_mask)
{
    if (flags & ~valid_mask) {
        validasi_stats.failures++;
        return SALAH;
    }

    return BENAR;
}

/*
 * validasi_copy_from_user
 * -----------------------
 * Copy data dari user space dengan validasi.
 *
 * Parameter:
 *   dest   - Buffer destination (kernel)
 *   src    - Source buffer (user)
 *   len    - Panjang data
 *
 * Return: Jumlah byte yang dicopy, atau -1 jika error
 */
tak_bertandas_t validasi_copy_from_user(void *dest, const void *src,
                                         ukuran_t len)
{
    if (!validasi_buffer_user(src, len, SALAH)) {
        return -1;
    }

    kernel_memcpy(dest, src, len);

    return (tak_bertandas_t)len;
}

/*
 * validasi_copy_to_user
 * ---------------------
 * Copy data ke user space dengan validasi.
 *
 * Parameter:
 *   dest   - Buffer destination (user)
 *   src    - Source buffer (kernel)
 *   len    - Panjang data
 *
 * Return: Jumlah byte yang dicopy, atau -1 jika error
 */
tak_bertandas_t validasi_copy_to_user(void *dest, const void *src,
                                       ukuran_t len)
{
    if (!validasi_buffer_user(dest, len, BENAR)) {
        return -1;
    }

    kernel_memcpy(dest, src, len);

    return (tak_bertandas_t)len;
}

/*
 * validasi_get_stats
 * ------------------
 * Dapatkan statistik validasi.
 *
 * Parameter:
 *   pointer  - Pointer checks
 *   string   - String checks
 *   buffer   - Buffer checks
 *   failures - Failure count
 */
void validasi_get_stats(tak_bertanda64_t *pointer, tak_bertanda64_t *string,
                        tak_bertanda64_t *buffer, tak_bertanda64_t *failures)
{
    if (pointer != NULL) {
        *pointer = validasi_stats.pointer_checks;
    }

    if (string != NULL) {
        *string = validasi_stats.string_checks;
    }

    if (buffer != NULL) {
        *buffer = validasi_stats.buffer_checks;
    }

    if (failures != NULL) {
        *failures = validasi_stats.failures;
    }
}

/*
 * validasi_print_stats
 * --------------------
 * Print statistik validasi.
 */
void validasi_print_stats(void)
{
    kernel_printf("\n[VALIDASI] Statistik Validasi:\n");
    kernel_printf("========================================\n");
    kernel_printf("  Pointer Checks : %lu\n",
                  validasi_stats.pointer_checks);
    kernel_printf("  String Checks  : %lu\n",
                  validasi_stats.string_checks);
    kernel_printf("  Buffer Checks  : %lu\n",
                  validasi_stats.buffer_checks);
    kernel_printf("  Failures       : %lu\n",
                  validasi_stats.failures);
    kernel_printf("  Success Rate   : %lu%%\n",
                  validasi_stats.pointer_checks +
                  validasi_stats.string_checks +
                  validasi_stats.buffer_checks > 0 ?
                  ((validasi_stats.pointer_checks +
                    validasi_stats.string_checks +
                    validasi_stats.buffer_checks -
                    validasi_stats.failures) * 100) /
                  (validasi_stats.pointer_checks +
                   validasi_stats.string_checks +
                   validasi_stats.buffer_checks) : 100);
    kernel_printf("========================================\n");
}
