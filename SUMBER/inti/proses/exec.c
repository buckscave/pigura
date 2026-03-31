/*
 * PIGURA OS - EXEC.C
 * -------------------
 * Implementasi eksekusi program baru.
 *
 * Berkas ini berisi fungsi-fungsi untuk memuat dan menjalankan
 * program baru, menggantikan image proses saat ini.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Tidak menggunakan fitur C99/C11
 * - Semua fungsi diimplementasikan lengkap
 * - Batas 80 karakter per baris
 *
 * Versi: 3.0
 * Tanggal: 2025
 */

#include "../kernel.h"
#include "proses.h"

/*
 * ============================================================================
 * FUNGSI STUB (STUB FUNCTIONS)
 * ============================================================================
 * Fungsi-fungsi file system yang akan diimplementasikan di VFS.
 */

/* Tipe berkas - alias untuk file_descriptor */
typedef struct file_descriptor berkas_t;

/* Flag berkas */
#ifndef BERKAS_BACA
#define BERKAS_BACA             FILE_BACA
#endif

/* Stub fungsi berkas - implementasi sementara */
berkas_t *berkas_buka(const char *path, tak_bertanda32_t flags)
{
    /* TODO: Implementasi dengan VFS */
    (void)path;
    (void)flags;
    return NULL;
}

ukuran_t berkas_baca(berkas_t *berkas, void *buf, ukuran_t count)
{
    /* TODO: Implementasi dengan VFS */
    (void)berkas;
    (void)buf;
    (void)count;
    return 0;
}

void berkas_tutup(berkas_t *berkas)
{
    /* TODO: Implementasi dengan VFS */
    (void)berkas;
}

/* Stub fungsi VM */
status_t vm_alloc_at(vm_descriptor_t *vm, ukuran_t size,
                            tak_bertanda32_t flags, alamat_virtual_t addr)
{
    /* TODO: Implementasi dengan VM */
    (void)vm;
    (void)size;
    (void)flags;
    (void)addr;
    return STATUS_TIDAK_DUKUNG;
}

/* Stub fungsi environment */
static char *getenv(const char *name)
{
    /* TODO: Implementasi dengan environment */
    (void)name;
    return NULL;
}

/* Stub fungsi string - strtok_r implementation */
static char *kernel_strtok_r(char *str, const char *delim, char **saveptr)
{
    char *token;
    
    if (str == NULL) {
        str = *saveptr;
    }
    
    /* Skip leading delimiters */
    while (*str && kernel_strchr(delim, *str) != NULL) {
        str++;
    }
    
    if (*str == '\0') {
        *saveptr = str;
        return NULL;
    }
    
    token = str;
    
    /* Find end of token */
    while (*str && kernel_strchr(delim, *str) == NULL) {
        str++;
    }
    
    if (*str) {
        *str++ = '\0';
    }
    
    *saveptr = str;
    
    return token;
}

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

/* Nama proses maksimum */
#ifndef NAMA_PROSES_MAKS
#define NAMA_PROSES_MAKS        32
#endif

/* Status format invalid */
#ifndef STATUS_FORMAT_INVALID
#define STATUS_FORMAT_INVALID   STATUS_TIDAK_DUKUNG
#endif

/* Ukuran buffer untuk path */
#define EXEC_PATH_MAX           PATH_MAX

/* Ukuran buffer untuk argumen */
#define EXEC_ARG_MAX            MAKS_ARGC
#define EXEC_ARG_LEN_MAX        MAKS_ARGV_LEN

/* Ukuran environment */
#define EXEC_ENV_MAX            64
#define EXEC_ENV_LEN_MAX        256

/* Alignment untuk stack user */
#define STACK_ALIGN             16

/* Magic binary types */
#define BINARY_TYPE_ELF         0x7F454C46  /* 0x7F 'E' 'L' 'F' */
#define BINARY_TYPE_SCRIPT      0x23212F   /* "#!/" */

/* Flag exec */
#define EXEC_FLAG_NONE          0x00
#define EXEC_FLAG_SUGID         0x01

/* Batas ukuran binary */
#define MAX_BINARY_SIZE         (64 * 1024 * 1024)  /* 64 MB */
#define MAX_ARGS_SIZE           (128 * 1024)        /* 128 KB */
#define MAX_ENV_SIZE            (32 * 1024)         /* 32 KB */

/*
 * ============================================================================
 * STRUKTUR DATA LOKAL (LOCAL DATA STRUCTURES)
 * ============================================================================
 */

/* Struktur argumen exec */
typedef struct {
    char *argv[EXEC_ARG_MAX];       /* Array argumen */
    tak_bertanda32_t argc;          /* Jumlah argumen */
    char *envp[EXEC_ENV_MAX];       /* Array environment */
    tak_bertanda32_t envc;          /* Jumlah environment */
    char arg_buf[EXEC_ARG_MAX * EXEC_ARG_LEN_MAX];
    char env_buf[EXEC_ENV_MAX * EXEC_ENV_LEN_MAX];
    ukuran_t arg_total;
    ukuran_t env_total;
} exec_args_t;

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Statistik exec */
static struct {
    tak_bertanda64_t total;
    tak_bertanda64_t successful;
    tak_bertanda64_t failed;
    tak_bertanda64_t elf_loaded;
    tak_bertanda64_t script_loaded;
    tak_bertanda64_t time_spent;
} exec_stats = {0, 0, 0, 0, 0, 0};

/* Status inisialisasi */
static bool_t exec_initialized = SALAH;

/* Lock */
static spinlock_t exec_lock;

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * cek_binary_type
 * ---------------
 * Cek tipe binary dari file.
 *
 * Parameter:
 *   path - Path ke file
 *   info - Pointer untuk menyimpan info binary
 *
 * Return: Status operasi
 */
static status_t cek_binary_type(const char *path, binary_info_t *info)
{
    berkas_t *berkas;
    tak_bertanda8_t header[16];
    ukuran_t bytes_read;
    
    if (path == NULL || info == NULL) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Buka file */
    berkas = berkas_buka(path, BERKAS_BACA);
    if (berkas == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    /* Baca header */
    bytes_read = berkas_baca(berkas, header, sizeof(header));
    berkas_tutup(berkas);
    
    if (bytes_read < 4) {
        return STATUS_FORMAT_INVALID;
    }
    
    /* Cek ELF magic */
    if (header[0] == 0x7F && header[1] == 'E' &&
        header[2] == 'L' && header[3] == 'F') {
        info->type = BINARY_TYPE_ELF;
        return STATUS_BERHASIL;
    }
    
    /* Cek shebang */
    if (header[0] == '#' && header[1] == '!') {
        info->type = BINARY_TYPE_SCRIPT;
        return STATUS_BERHASIL;
    }
    
    return STATUS_FORMAT_INVALID;
}

/*
 * parse_args
 * ----------
 * Parse argumen dari array.
 *
 * Parameter:
 *   args - Pointer ke struktur exec_args_t
 *   argv - Array string argumen
 *   envp - Array string environment
 *
 * Return: Status operasi
 */
static status_t parse_args(exec_args_t *args, char *const argv[],
                           char *const envp[])
{
    tak_bertanda32_t i;
    ukuran_t offset;
    ukuran_t len;
    
    if (args == NULL) {
        return STATUS_PARAM_INVALID;
    }
    
    kernel_memset(args, 0, sizeof(exec_args_t));
    offset = 0;
    
    /* Parse argv */
    if (argv != NULL) {
        for (i = 0; argv[i] != NULL && i < EXEC_ARG_MAX - 1; i++) {
            len = kernel_strlen(argv[i]);
            if (len >= EXEC_ARG_LEN_MAX) {
                len = EXEC_ARG_LEN_MAX - 1;
            }
            
            if (offset + len + 1 >= sizeof(args->arg_buf)) {
                break;
            }
            
            kernel_strncpy(&args->arg_buf[offset], argv[i], len);
            args->argv[i] = &args->arg_buf[offset];
            offset += len + 1;
        }
        args->argc = i;
        args->arg_total = offset;
    }
    
    offset = 0;
    
    /* Parse envp */
    if (envp != NULL) {
        for (i = 0; envp[i] != NULL && i < EXEC_ENV_MAX - 1; i++) {
            len = kernel_strlen(envp[i]);
            if (len >= EXEC_ENV_LEN_MAX) {
                len = EXEC_ENV_LEN_MAX - 1;
            }
            
            if (offset + len + 1 >= sizeof(args->env_buf)) {
                break;
            }
            
            kernel_strncpy(&args->env_buf[offset], envp[i], len);
            args->envp[i] = &args->env_buf[offset];
            offset += len + 1;
        }
        args->envc = i;
        args->env_total = offset;
    }
    
    return STATUS_BERHASIL;
}

/*
 * setup_user_stack
 * ----------------
 * Setup stack user dengan argumen.
 *
 * Parameter:
 *   proses    - Pointer ke proses
 *   args      - Pointer ke argumen
 *   stack_top - Alamat atas stack
 *
 * Return: Stack pointer baru
 */
static alamat_virtual_t setup_user_stack(proses_t *proses, exec_args_t *args,
                                         alamat_virtual_t stack_top)
{
    tak_bertanda32_t *stack;
    char *str_ptr;
    ukuran_t total_size;
    ukuran_t str_size;
    tak_bertanda32_t i;
    alamat_virtual_t result;
    
    if (proses == NULL || args == NULL) {
        return 0;
    }
    
    /* Hitung total ukuran yang dibutuhkan untuk strings */
    str_size = args->arg_total + args->env_total;
    
    /* Hitung total ukuran untuk pointers */
    total_size = sizeof(tak_bertanda32_t);  /* argc */
    total_size += (args->argc + 1) * sizeof(char *);  /* argv + NULL */
    total_size += (args->envc + 1) * sizeof(char *);  /* envp + NULL */
    total_size += str_size;
    
    /* Ratakan ke alignment */
    total_size = RATAKAN_ATAS(total_size, STACK_ALIGN);
    
    /* Alokasikan di stack */
    stack = (tak_bertanda32_t *)(stack_top - total_size);
    result = (alamat_virtual_t)stack;
    
    /* Area untuk strings */
    str_ptr = (char *)stack + (args->argc + args->envc + 4) *
              sizeof(tak_bertanda32_t);
    
    /* Build stack frame */
    i = 0;
    
    /* Push argc */
    stack[i++] = args->argc;
    
    /* Push argv pointers */
    {
        tak_bertanda32_t j;
        for (j = 0; j < args->argc; j++) {
            stack[i++] = (tak_bertanda32_t)(alamat_ptr_t)str_ptr;
            kernel_strncpy(str_ptr, args->argv[j],
                           kernel_strlen(args->argv[j]) + 1);
            str_ptr += kernel_strlen(args->argv[j]) + 1;
        }
    }
    stack[i++] = 0;  /* NULL terminator untuk argv */
    
    /* Push envp pointers */
    {
        tak_bertanda32_t j;
        for (j = 0; j < args->envc; j++) {
            stack[i++] = (tak_bertanda32_t)(alamat_ptr_t)str_ptr;
            kernel_strncpy(str_ptr, args->envp[j],
                           kernel_strlen(args->envp[j]) + 1);
            str_ptr += kernel_strlen(args->envp[j]) + 1;
        }
    }
    stack[i++] = 0;  /* NULL terminator untuk envp */
    
    return result;
}

/*
 * cleanup_old_image
 * -----------------
 * Bersihkan image lama proses.
 *
 * Parameter:
 *   proses - Pointer ke proses
 */
static void cleanup_old_image(proses_t *proses)
{
    tak_bertanda32_t i;
    
    if (proses == NULL) {
        return;
    }
    
    /* Hapus address space lama */
    if (proses->vm != NULL) {
        vm_destroy_address_space(proses->vm);
        proses->vm = NULL;
    }
    
    /* Tutup file descriptors kecuali stdin/stdout/stderr */
    for (i = 3; i < FD_MAKS_PER_PROSES; i++) {
        if (proses->fd_table[i] != NULL) {
            berkas_tutup(proses->fd_table[i]);
            proses->fd_table[i] = NULL;
        }
    }
    
    /* Clear signal handlers */
    kernel_memset(proses->signal_handlers, 0,
                  sizeof(proses->signal_handlers));
    proses->signal_pending = 0;
    proses->signal_ignored = 0;
    proses->signal_mask = 0;
    
    /* Reset thread context */
    if (proses->main_thread != NULL) {
        if (proses->main_thread->context != NULL) {
            context_hancurkan(proses->main_thread->context);
            proses->main_thread->context = NULL;
        }
        if (proses->main_thread->stack_base != NULL) {
            thread_bebaskan_stack(proses->main_thread->stack_ptr,
                                   proses->main_thread->stack_size, BENAR);
            proses->main_thread->stack_base = NULL;
            proses->main_thread->stack_ptr = NULL;
        }
    }
    
    /* Reset memory statistik */
    kernel_memset(&proses->mem_stat, 0, sizeof(statistik_memori_t));
    kernel_memset(&proses->cpu_stat, 0, sizeof(statistik_cpu_t));
    kernel_memset(&proses->io_stat, 0, sizeof(statistik_io_t));
}

/*
 * load_elf_binary
 * ----------------
 * Load binary ELF.
 *
 * Parameter:
 *   path   - Path ke file
 *   proses - Pointer ke proses
 *   info   - Pointer untuk info binary
 *
 * Return: Status operasi
 */
static status_t load_elf_binary(const char *path, proses_t *proses,
                                binary_info_t *info)
{
    status_t status;
    
    if (path == NULL || proses == NULL || info == NULL) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Load ELF menggunakan elf loader */
    status = elf_load(path, proses, info);
    
    if (status != STATUS_BERHASIL) {
        return status;
    }
    
    exec_stats.elf_loaded++;
    
    return STATUS_BERHASIL;
}

/*
 * load_script
 * -----------
 * Load dan eksekusi script.
 *
 * Parameter:
 *   path - Path ke script
 *   args - Argumen exec
 *   proses - Pointer ke proses
 *
 * Return: Status operasi
 */
static status_t load_script(const char *path, exec_args_t *args,
                            proses_t *proses)
{
    berkas_t *berkas;
    char shebang[EXEC_PATH_MAX];
    char interpreter[EXEC_PATH_MAX];
    char *interp_args[EXEC_ARG_MAX];
    tak_bertanda32_t interp_argc;
    ukuran_t bytes_read;
    tak_bertanda32_t i;
    status_t status;
    char *start;
    char *end;
    
    if (path == NULL || args == NULL || proses == NULL) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Buka script */
    berkas = berkas_buka(path, BERKAS_BACA);
    if (berkas == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    /* Baca shebang line */
    bytes_read = berkas_baca(berkas, shebang, sizeof(shebang) - 1);
    berkas_tutup(berkas);
    
    if (bytes_read < 3) {
        return STATUS_FORMAT_INVALID;
    }
    
    shebang[bytes_read] = '\0';
    
    /* Parse shebang */
    if (shebang[0] != '#' || shebang[1] != '!') {
        return STATUS_FORMAT_INVALID;
    }
    
    /* Skip "#!" */
    start = shebang + 2;
    
    /* Skip whitespace */
    while (*start == ' ' || *start == '\t') {
        start++;
    }
    
    /* Extract interpreter path */
    end = start;
    while (*end != '\0' && *end != '\n' && *end != ' ' &&
           *end != '\t' && (tak_bertanda32_t)(end - start) < EXEC_PATH_MAX - 1) {
        end++;
    }
    
    *end = '\0';
    kernel_strncpy(interpreter, start, EXEC_PATH_MAX - 1);
    interpreter[EXEC_PATH_MAX - 1] = '\0';
    
    /* Build argument baru: interpreter script arg1 arg2 ... */
    interp_argc = 0;
    interp_args[interp_argc++] = interpreter;
    interp_args[interp_argc++] = (char *)path;
    
    /* Copy argument asli */
    for (i = 1; i < args->argc && interp_argc < EXEC_ARG_MAX - 1; i++) {
        interp_args[interp_argc++] = args->argv[i];
    }
    interp_args[interp_argc] = NULL;
    
    /* Update args */
    args->argc = interp_argc;
    for (i = 0; i < interp_argc; i++) {
        args->argv[i] = interp_args[i];
    }
    
    /* Rekursif exec interpreter */
    status = do_exec(interpreter, (char *const *)interp_args,
                     (char *const *)args->envp);
    
    exec_stats.script_loaded++;
    
    return status;
}

/*
 * ============================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
 * ============================================================================
 */

/*
 * exec_init
 * ---------
 * Inisialisasi subsistem exec.
 *
 * Return: Status operasi
 */
status_t exec_init(void)
{
    if (exec_initialized) {
        return STATUS_SUDAH_ADA;
    }
    
    kernel_memset(&exec_stats, 0, sizeof(exec_stats));
    spinlock_init(&exec_lock);
    
    exec_initialized = BENAR;
    
    kernel_printf("[EXEC] Exec subsystem initialized\n");
    
    return STATUS_BERHASIL;
}

/*
 * do_exec
 * -------
 * Eksekusi program baru.
 *
 * Parameter:
 *   path - Path ke program
 *   argv - Array argumen
 *   envp - Array environment
 *
 * Return: Tidak return jika berhasil, status error jika gagal
 */
status_t do_exec(const char *path, char *const argv[], char *const envp[])
{
    proses_t *proses;
    binary_info_t bin_info;
    exec_args_t args;
    alamat_virtual_t entry;
    alamat_virtual_t stack_top;
    alamat_virtual_t stack_ptr;
    status_t status;
    tak_bertanda64_t start_time;
    
    if (!exec_initialized || path == NULL) {
        return STATUS_PARAM_INVALID;
    }
    
    start_time = hal_timer_get_ticks();
    
    exec_stats.total++;
    
    proses = proses_dapat_saat_ini();
    if (proses == NULL) {
        exec_stats.failed++;
        return STATUS_GAGAL;
    }
    
    /* Cek tipe binary */
    status = cek_binary_type(path, &bin_info);
    if (status != STATUS_BERHASIL) {
        exec_stats.failed++;
        return status;
    }
    
    /* Parse argumen */
    status = parse_args(&args, argv, envp);
    if (status != STATUS_BERHASIL) {
        exec_stats.failed++;
        return status;
    }
    
    /* Set nama proses */
    kernel_strncpy(proses->nama, path, NAMA_PROSES_MAKS - 1);
    proses->nama[NAMA_PROSES_MAKS - 1] = '\0';
    
    /* Handle script */
    if (bin_info.type == BINARY_TYPE_SCRIPT) {
        return load_script(path, &args, proses);
    }
    
    /* Cleanup image lama */
    cleanup_old_image(proses);
    
    /* Buat address space baru */
    proses->vm = vm_create_address_space();
    if (proses->vm == NULL) {
        exec_stats.failed++;
        return STATUS_MEMORI_HABIS;
    }
    
    /* Load binary ELF */
    status = load_elf_binary(path, proses, &bin_info);
    if (status != STATUS_BERHASIL) {
        vm_destroy_address_space(proses->vm);
        proses->vm = NULL;
        exec_stats.failed++;
        return status;
    }
    
    entry = bin_info.entry;
    
    /* Alokasikan stack user */
    stack_top = ALAMAT_STACK_USER + UKURAN_STACK_USER;
    status = vm_alloc_at(proses->vm, UKURAN_STACK_USER,
                         VMA_FLAG_READ | VMA_FLAG_WRITE | VMA_FLAG_STACK,
                         ALAMAT_STACK_USER);
    
    if (status != STATUS_BERHASIL) {
        vm_destroy_address_space(proses->vm);
        proses->vm = NULL;
        exec_stats.failed++;
        return STATUS_MEMORI_HABIS;
    }
    
    /* Setup stack dengan argumen */
    stack_ptr = setup_user_stack(proses, &args, stack_top);
    
    /* Buat context baru */
    if (proses->main_thread == NULL) {
        proses->main_thread = thread_alokasi();
        if (proses->main_thread == NULL) {
            vm_destroy_address_space(proses->vm);
            proses->vm = NULL;
            exec_stats.failed++;
            return STATUS_MEMORI_HABIS;
        }
        proses->threads = proses->main_thread;
        proses->thread_count = 1;
    }
    
    proses->main_thread->context = context_buat(entry, (void *)stack_ptr,
                                                BENAR);
    
    if (proses->main_thread->context == NULL) {
        vm_destroy_address_space(proses->vm);
        proses->vm = NULL;
        exec_stats.failed++;
        return STATUS_MEMORI_HABIS;
    }
    
    /* Set CR3 */
    context_set_cr3(proses->main_thread->context, proses->vm->cr3);
    
    /* Reset status */
    proses->status = PROSES_STATUS_JALAN;
    proses->main_thread->status = THREAD_STATUS_JALAN;
    
    /* Update statistik */
    exec_stats.successful++;
    exec_stats.time_spent += (hal_timer_get_ticks() - start_time);
    
    /* Switch ke context baru */
    context_switch_to(proses->main_thread->context);
    
    /* Tidak seharusnya sampai sini */
    return STATUS_BERHASIL;
}

/*
 * sys_execve
 * ----------
 * System call execve.
 *
 * Parameter:
 *   path - Path ke program
 *   argv - Array argumen
 *   envp - Array environment
 *
 * Return: Tidak return jika berhasil
 */
tanda32_t sys_execve(const char *path, char *const argv[],
                     char *const envp[])
{
    status_t status;
    
    /* Validasi pointer user */
    if (!validasi_string_user(path)) {
        return -ERROR_FAULT;
    }
    
    if (argv != NULL && !validasi_pointer_user(argv)) {
        return -ERROR_FAULT;
    }
    
    if (envp != NULL && !validasi_pointer_user(envp)) {
        return -ERROR_FAULT;
    }
    
    status = do_exec(path, argv, envp);
    
    /* Jika sampai sini, berarti gagal */
    return -(tanda32_t)status;
}

/*
 * sys_execvp
 * ----------
 * System call execvp (search in PATH).
 *
 * Parameter:
 *   file - Nama file atau path
 *   argv - Array argumen
 *
 * Return: Tidak return jika berhasil
 */
tanda32_t sys_execvp(const char *file, char *const argv[])
{
    char path[EXEC_PATH_MAX];
    char *env_path;
    char *dir;
    char *saveptr;
    status_t status;
    
    if (file == NULL || argv == NULL) {
        return -ERROR_INVALID;
    }
    
    /* Jika mengandung '/', gunakan langsung */
    if (kernel_strchr(file, '/') != NULL) {
        return sys_execve(file, argv, NULL);
    }
    
    /* Cari di PATH environment */
    env_path = getenv("PATH");
    if (env_path == NULL) {
        env_path = "/bin:/usr/bin:/usr/local/bin";
    }
    
    /* Parse PATH */
    dir = kernel_strtok_r(env_path, ":", &saveptr);
    
    while (dir != NULL) {
        /* Build full path */
        kernel_snprintf(path, EXEC_PATH_MAX, "%s/%s", dir, file);
        
        /* Coba exec */
        status = do_exec(path, argv, NULL);
        
        /* Jika file ditemukan */
        if (status != STATUS_TIDAK_DITEMUKAN) {
            return -(tanda32_t)status;
        }
        
        dir = kernel_strtok_r(NULL, ":", &saveptr);
    }
    
    return -ERROR_FILE_TIDAK_ADA;
}

/*
 * exec_dapat_statistik
 * ---------------------
 * Dapatkan statistik exec.
 *
 * Parameter:
 *   total      - Pointer untuk total exec
 *   successful - Pointer untuk exec berhasil
 *   failed     - Pointer untuk exec gagal
 *   elf        - Pointer untuk ELF loaded
 */
void exec_dapat_statistik(tak_bertanda64_t *total,
                          tak_bertanda64_t *successful,
                          tak_bertanda64_t *failed,
                          tak_bertanda64_t *elf)
{
    if (total != NULL) {
        *total = exec_stats.total;
    }
    
    if (successful != NULL) {
        *successful = exec_stats.successful;
    }
    
    if (failed != NULL) {
        *failed = exec_stats.failed;
    }
    
    if (elf != NULL) {
        *elf = exec_stats.elf_loaded;
    }
}

/*
 * exec_print_stats
 * ----------------
 * Print statistik exec.
 */
void exec_print_stats(void)
{
    kernel_printf("\n[EXEC] Statistik Exec:\n");
    kernel_printf("========================================\n");
    kernel_printf("  Total Execs     : %lu\n", exec_stats.total);
    kernel_printf("  Successful      : %lu\n", exec_stats.successful);
    kernel_printf("  Failed          : %lu\n", exec_stats.failed);
    kernel_printf("  ELF Loaded      : %lu\n", exec_stats.elf_loaded);
    kernel_printf("  Scripts Loaded  : %lu\n", exec_stats.script_loaded);
    kernel_printf("  Time Spent      : %lu ticks\n", exec_stats.time_spent);
    kernel_printf("  Success Rate    : %lu%%\n",
                  exec_stats.total > 0 ?
                  (exec_stats.successful * 100 / exec_stats.total) : 0);
    kernel_printf("========================================\n");
}
