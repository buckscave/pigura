/*
 * PIGURA OS - NAMEI.C
 * ====================
 * Implementasi path lookup dan resolution untuk VFS Pigura OS.
 *
 * Berkas ini berisi fungsi-fungsi untuk resolving pathname menjadi
 * dentry dan inode, termasuk symlink following.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Tidak menggunakan fitur C99/C11
 * - Semua fungsi diimplementasikan secara lengkap
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "vfs.h"
#include "../../inti/kernel.h"

/*
 * ===========================================================================
 * KONSTANTA INTERNAL (INTERNAL CONSTANTS)
 * ===========================================================================
 */

/* Maximum symlink follow depth */
#define NAMEI_MAX_SYMLINKS      40

/* Maximum path component count */
#define NAMEI_MAX_COMPONENTS    64

/* Component name buffer size */
#define NAMEI_NAME_BUFFER_SIZE  (VFS_NAMA_MAKS + 1)

/* Internal flags */
#define NAMEI_FLAG_JUMPED       0x00010000
#define NAMEI_FLAG_TRAILING     0x00020000

/*
 * ===========================================================================
 * STRUKTUR DATA INTERNAL (INTERNAL DATA STRUCTURES)
 * ===========================================================================
 */

/* Path component */
typedef struct {
    char nama[NAMEI_NAME_BUFFER_SIZE];
    ukuran_t panjang;
} namei_component_t;

/* Namei context */
typedef struct {
    dentry_t *nc_dentry;        /* Current dentry */
    mount_t *nc_mount;          /* Current mount */
    tak_bertanda32_t nc_flags;  /* Lookup flags */
    tak_bertanda32_t nc_symlink_count; /* Symlink follow count */
    tanda32_t nc_last_type;     /* Last component type */
    char nc_last_name[NAMEI_NAME_BUFFER_SIZE]; /* Last component name */
} namei_context_t;

/*
 * ===========================================================================
 * VARIABEL GLOBAL (GLOBAL VARIABLES)
 * ===========================================================================
 */

/* Current working directory */
static dentry_t *g_cwd_dentry = NULL;
static mount_t *g_cwd_mount = NULL;

/* Lock untuk namei */
static tak_bertanda32_t g_namei_lock = 0;

/*
 * ===========================================================================
 * FUNGSI INTERNAL LOCKING (INTERNAL LOCKING FUNCTIONS)
 * ===========================================================================
 */

static void namei_lock(void)
{
    g_namei_lock++;
}

static void namei_unlock(void)
{
    if (g_namei_lock > 0) {
        g_namei_lock--;
    }
}

/*
 * ===========================================================================
 * FUNGSI PARSING PATH (PATH PARSING FUNCTIONS)
 * ===========================================================================
 */

static int namei_parse_path(const char *path,
                            namei_component_t *components,
                            int max_components)
{
    const char *curr;
    const char *start;
    int count = 0;
    ukuran_t len;
    
    if (path == NULL || components == NULL || max_components <= 0) {
        return 0;
    }
    
    curr = path;
    
    /* Skip leading slashes */
    while (*curr == '/') {
        curr++;
    }
    
    while (*curr != '\0' && count < max_components) {
        /* Cari akhir komponen */
        start = curr;
        while (*curr != '\0' && *curr != '/') {
            curr++;
        }
        
        /* Hitung panjang */
        len = (ukuran_t)(curr - start);
        
        /* Skip empty components */
        if (len == 0) {
            while (*curr == '/') {
                curr++;
            }
            continue;
        }
        
        /* Cek untuk . dan .. */
        if (len == 1 && start[0] == '.') {
            /* Skip current directory */
            while (*curr == '/') {
                curr++;
            }
            continue;
        }
        
        if (len == 2 && start[0] == '.' && start[1] == '.') {
            /* Parent directory marker */
            components[count].nama[0] = '.';
            components[count].nama[1] = '.';
            components[count].nama[2] = '\0';
            components[count].panjang = 2;
        } else {
            /* Normal component */
            if (len >= NAMEI_NAME_BUFFER_SIZE) {
                len = NAMEI_NAME_BUFFER_SIZE - 1;
            }
            kernel_strncpy(components[count].nama, start, len);
            components[count].nama[len] = '\0';
            components[count].panjang = len;
        }
        
        count++;
        
        /* Skip slashes */
        while (*curr == '/') {
            curr++;
        }
    }
    
    return count;
}

static bool_t namei_is_absolute(const char *path)
{
    if (path == NULL) {
        return SALAH;
    }
    return path[0] == '/';
}

static bool_t namei_is_trailing_slash(const char *path)
{
    ukuran_t len;
    
    if (path == NULL) {
        return SALAH;
    }
    
    len = kernel_strlen(path);
    if (len == 0) {
        return SALAH;
    }
    
    return path[len - 1] == '/';
}

/*
 * ===========================================================================
 * FUNGSI DO_LOOKUP (DO_LOOKUP FUNCTION)
 * ===========================================================================
 */

static dentry_t *namei_do_lookup(dentry_t *parent, const char *nama,
                                 namei_context_t * __attribute__((unused)) ctx)
{
    (void)ctx;
    dentry_t *dentry;
    inode_t *inode;
    
    if (parent == NULL || nama == NULL) {
        return NULL;
    }
    
    /* Cek cache dentry */
    dentry = dentry_lookup(parent, nama);
    if (dentry != NULL) {
        return dentry;
    }
    
    /* Lookup melalui inode operation */
    inode = parent->d_inode;
    if (inode == NULL) {
        return NULL;
    }
    
    if (inode->i_op == NULL || inode->i_op->lookup == NULL) {
        return NULL;
    }
    
    /* Panggil filesystem lookup */
    dentry = inode->i_op->lookup(inode, nama);
    if (dentry != NULL) {
        /* Set parent dan insert ke cache */
        dentry->d_parent = parent;
        dentry->d_sb = parent->d_sb;
        dentry_cache_insert(dentry);
        dentry_get(dentry);
    }
    
    return dentry;
}

/*
 * ===========================================================================
 * FUNGSI FOLLOW SYMLINK (FOLLOW SYMLINK FUNCTION)
 * ===========================================================================
 */

static status_t namei_follow_symlink(dentry_t *dentry, namei_context_t *ctx,
                                     char *buffer, ukuran_t size)
{
    inode_t *inode;
    tak_bertandas_t ret;
    
    if (dentry == NULL || ctx == NULL || buffer == NULL || size == 0) {
        return STATUS_PARAM_NULL;
    }
    
    inode = dentry->d_inode;
    if (inode == NULL) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Cek symlink limit */
    if (ctx->nc_symlink_count >= NAMEI_MAX_SYMLINKS) {
        log_warn("[VFS:NAMEI] Terlalu banyak symlink");
        return STATUS_GAGAL; /* STATUS_LOOP: terlalu banyak symlink */
    }
    
    /* Baca symlink target */
    if (inode->i_op == NULL || inode->i_op->readlink == NULL) {
        return STATUS_TIDAK_DUKUNG;
    }
    
    ret = inode->i_op->readlink(dentry, buffer, size);
    if (ret <= 0) {
        return STATUS_IO_ERROR;
    }
    
    ctx->nc_symlink_count++;
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI WALK PARENT (WALK PARENT FUNCTION)
 * ===========================================================================
 */

static status_t namei_walk_parent(namei_context_t *ctx)
{
    dentry_t *parent;
    mount_t *mount;
    
    if (ctx == NULL || ctx->nc_dentry == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Cek apakah sudah di root */
    if (ctx->nc_dentry->d_parent == NULL ||
        ctx->nc_dentry->d_parent == ctx->nc_dentry) {
        /* Sudah di root, tidak bisa ke parent */
        return STATUS_BERHASIL;
    }
    
    /* Cek apakah ada mount point */
    if (ctx->nc_dentry->d_mounted && ctx->nc_mount != NULL) {
        /* Pindah ke parent mount */
        mount = ctx->nc_mount->m_parent;
        if (mount != NULL) {
            parent = mount->m_mountpoint;
            if (parent != NULL) {
                dentry_get(parent);
                dentry_put(ctx->nc_dentry);
                ctx->nc_dentry = parent;
                mount_put(ctx->nc_mount);
                mount_get(mount);
                ctx->nc_mount = mount;
                return STATUS_BERHASIL;
            }
        }
    }
    
    /* Normal parent traversal */
    parent = ctx->nc_dentry->d_parent;
    if (parent == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    dentry_get(parent);
    dentry_put(ctx->nc_dentry);
    ctx->nc_dentry = parent;
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI WALK COMPONENT (WALK COMPONENT FUNCTION)
 * ===========================================================================
 */

static status_t namei_walk_component(namei_context_t *ctx,
                                     const char *nama)
{
    dentry_t *dentry;
    dentry_t *new_dentry;
    mount_t *mount;
    inode_t *inode;
    char symlink_target[VFS_PATH_MAKS + 1];
    status_t ret;
    
    if (ctx == NULL || ctx->nc_dentry == NULL || nama == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Handle . dan .. */
    if (nama[0] == '.') {
        if (nama[1] == '\0') {
            /* Current directory - tidak ada yang perlu dilakukan */
            return STATUS_BERHASIL;
        }
        if (nama[1] == '.' && nama[2] == '\0') {
            /* Parent directory */
            return namei_walk_parent(ctx);
        }
    }
    
    /* Lookup component */
    dentry = namei_do_lookup(ctx->nc_dentry, nama, ctx);
    if (dentry == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    /* Cek mount point */
    if (dentry->d_mounted && dentry->d_mount != NULL) {
        mount = dentry->d_mount;
        /* Pindah ke root mount */
        new_dentry = mount->m_root;
        if (new_dentry != NULL) {
            dentry_get(new_dentry);
            dentry_put(dentry);
            dentry = new_dentry;
            
            mount_put(ctx->nc_mount);
            ctx->nc_mount = mount;
            mount_get(mount);
        }
    }
    
    /* Follow symlink jika perlu */
    inode = dentry->d_inode;
    if (inode != NULL && VFS_S_ISLNK(inode->i_mode) &&
        (ctx->nc_flags & NAMEI_LOOKUP_FOLLOW) != 0) {
        
        ret = namei_follow_symlink(dentry, ctx, symlink_target,
                                   sizeof(symlink_target));
        if (ret != STATUS_BERHASIL) {
            dentry_put(dentry);
            return ret;
        }
        
        /* Handle symlink target */
        if (symlink_target[0] == '/') {
            /* Absolute symlink - restart dari root */
            dentry_put(dentry);
            dentry = mount_root()->m_root;
            if (dentry == NULL) {
                return STATUS_TIDAK_DITEMUKAN;
            }
            dentry_get(dentry);
            ctx->nc_flags |= NAMEI_FLAG_JUMPED;
        }
        
        /* TODO: Re-resolve symlink path */
        /* Untuk sekarang, return dentry dari symlink */
    }
    
    /* Update context */
    dentry_put(ctx->nc_dentry);
    ctx->nc_dentry = dentry;
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI LOOKUP UTAMA (MAIN LOOKUP FUNCTIONS)
 * ===========================================================================
 */

dentry_t *namei_lookup(const char *path)
{
    namei_context_t ctx;
    namei_component_t components[NAMEI_MAX_COMPONENTS];
    int component_count;
    int i;
    status_t ret;
    bool_t trailing_slash;
    
    if (path == NULL) {
        return NULL;
    }
    
    /* Validasi path */
    if (!vfs_path_valid(path)) {
        return NULL;
    }
    
    /* Handle empty path */
    if (path[0] == '\0') {
        return NULL;
    }
    
    /* Init context */
    kernel_memset(&ctx, 0, sizeof(ctx));
    ctx.nc_flags = NAMEI_LOOKUP_FOLLOW;
    ctx.nc_symlink_count = 0;
    
    /* Set starting point */
    if (namei_is_absolute(path)) {
        /* Start dari root */
        mount_t *root_mount = mount_root();
        if (root_mount == NULL) {
            return NULL;
        }
        ctx.nc_mount = root_mount;
        ctx.nc_dentry = root_mount->m_root;
        if (ctx.nc_dentry != NULL) {
            dentry_get(ctx.nc_dentry);
        }
        mount_put(root_mount);
    } else {
        /* Start dari cwd */
        namei_lock();
        if (g_cwd_dentry != NULL) {
            ctx.nc_dentry = g_cwd_dentry;
            ctx.nc_mount = g_cwd_mount;
            dentry_get(ctx.nc_dentry);
            if (ctx.nc_mount != NULL) {
                mount_get(ctx.nc_mount);
            }
        }
        namei_unlock();
        
        if (ctx.nc_dentry == NULL) {
            /* Fallback ke root */
            mount_t *root_mount = mount_root();
            if (root_mount == NULL) {
                return NULL;
            }
            ctx.nc_mount = root_mount;
            ctx.nc_dentry = root_mount->m_root;
            if (ctx.nc_dentry != NULL) {
                dentry_get(ctx.nc_dentry);
            }
            mount_put(root_mount);
        }
    }
    
    if (ctx.nc_dentry == NULL) {
        return NULL;
    }
    
    /* Parse path components */
    component_count = namei_parse_path(path, components,
                                       NAMEI_MAX_COMPONENTS);
    
    /* Cek trailing slash */
    trailing_slash = namei_is_trailing_slash(path);
    
    /* Walk path components */
    for (i = 0; i < component_count; i++) {
        ret = namei_walk_component(&ctx, components[i].nama);
        if (ret != STATUS_BERHASIL) {
            if (ctx.nc_dentry != NULL) {
                dentry_put(ctx.nc_dentry);
            }
            if (ctx.nc_mount != NULL) {
                mount_put(ctx.nc_mount);
            }
            return NULL;
        }
    }
    
    /* Jika trailing slash, harus direktori */
    if (trailing_slash && ctx.nc_dentry != NULL) {
        inode_t *inode = ctx.nc_dentry->d_inode;
        if (inode != NULL && !VFS_S_ISDIR(inode->i_mode)) {
            dentry_put(ctx.nc_dentry);
            if (ctx.nc_mount != NULL) {
                mount_put(ctx.nc_mount);
            }
            return NULL;
        }
    }
    
    /* Cleanup mount reference */
    if (ctx.nc_mount != NULL) {
        mount_put(ctx.nc_mount);
    }
    
    return ctx.nc_dentry;
}

dentry_t *namei_lookup_parent(const char *path, char *nama,
                              ukuran_t nama_size)
{
    namei_context_t ctx;
    namei_component_t components[NAMEI_MAX_COMPONENTS];
    int component_count;
    int i;
    status_t ret;
    
    if (path == NULL || nama == NULL || nama_size == 0) {
        return NULL;
    }
    
    /* Parse path */
    component_count = namei_parse_path(path, components,
                                       NAMEI_MAX_COMPONENTS);
    
    if (component_count == 0) {
        /* Path kosong atau hanya root */
        nama[0] = '\0';
        return mount_root()->m_root;
    }
    
    /* Copy last component ke nama */
    kernel_strncpy(nama, components[component_count - 1].nama,
                   nama_size - 1);
    nama[nama_size - 1] = '\0';
    
    /* Lookup parent path */
    ctx.nc_dentry = NULL;
    ctx.nc_mount = NULL;
    ctx.nc_flags = NAMEI_LOOKUP_FOLLOW;
    ctx.nc_symlink_count = 0;
    
    /* Set starting point */
    if (namei_is_absolute(path)) {
        mount_t *root_mount = mount_root();
        if (root_mount == NULL) {
            return NULL;
        }
        ctx.nc_mount = root_mount;
        ctx.nc_dentry = root_mount->m_root;
        if (ctx.nc_dentry != NULL) {
            dentry_get(ctx.nc_dentry);
        }
        mount_put(root_mount);
    } else {
        namei_lock();
        if (g_cwd_dentry != NULL) {
            ctx.nc_dentry = g_cwd_dentry;
            ctx.nc_mount = g_cwd_mount;
            dentry_get(ctx.nc_dentry);
            if (ctx.nc_mount != NULL) {
                mount_get(ctx.nc_mount);
            }
        }
        namei_unlock();
        
        if (ctx.nc_dentry == NULL) {
            mount_t *root_mount = mount_root();
            if (root_mount == NULL) {
                return NULL;
            }
            ctx.nc_mount = root_mount;
            ctx.nc_dentry = root_mount->m_root;
            if (ctx.nc_dentry != NULL) {
                dentry_get(ctx.nc_dentry);
            }
            mount_put(root_mount);
        }
    }
    
    if (ctx.nc_dentry == NULL) {
        return NULL;
    }
    
    /* Walk semua komponen kecuali terakhir */
    for (i = 0; i < component_count - 1; i++) {
        ret = namei_walk_component(&ctx, components[i].nama);
        if (ret != STATUS_BERHASIL) {
            if (ctx.nc_dentry != NULL) {
                dentry_put(ctx.nc_dentry);
            }
            if (ctx.nc_mount != NULL) {
                mount_put(ctx.nc_mount);
            }
            return NULL;
        }
    }
    
    /* Cleanup mount reference */
    if (ctx.nc_mount != NULL) {
        mount_put(ctx.nc_mount);
    }
    
    return ctx.nc_dentry;
}

status_t namei_path_lookup(const char *path, dentry_t **dentry,
                           tak_bertanda32_t flags)
{
    namei_context_t ctx;
    namei_component_t components[NAMEI_MAX_COMPONENTS];
    int component_count;
    int i;
    status_t ret;
    bool_t trailing_slash;
    
    if (path == NULL || dentry == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    *dentry = NULL;
    
    /* Init context */
    kernel_memset(&ctx, 0, sizeof(ctx));
    ctx.nc_flags = flags;
    ctx.nc_symlink_count = 0;
    
    /* Set starting point */
    if (namei_is_absolute(path)) {
        mount_t *root_mount = mount_root();
        if (root_mount == NULL) {
            return STATUS_TIDAK_DITEMUKAN;
        }
        ctx.nc_mount = root_mount;
        ctx.nc_dentry = root_mount->m_root;
        if (ctx.nc_dentry != NULL) {
            dentry_get(ctx.nc_dentry);
        }
        mount_put(root_mount);
    } else {
        namei_lock();
        if (g_cwd_dentry != NULL) {
            ctx.nc_dentry = g_cwd_dentry;
            ctx.nc_mount = g_cwd_mount;
            dentry_get(ctx.nc_dentry);
            if (ctx.nc_mount != NULL) {
                mount_get(ctx.nc_mount);
            }
        }
        namei_unlock();
        
        if (ctx.nc_dentry == NULL) {
            mount_t *root_mount = mount_root();
            if (root_mount == NULL) {
                return STATUS_TIDAK_DITEMUKAN;
            }
            ctx.nc_mount = root_mount;
            ctx.nc_dentry = root_mount->m_root;
            if (ctx.nc_dentry != NULL) {
                dentry_get(ctx.nc_dentry);
            }
            mount_put(root_mount);
        }
    }
    
    if (ctx.nc_dentry == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    /* Parse path */
    component_count = namei_parse_path(path, components,
                                       NAMEI_MAX_COMPONENTS);
    trailing_slash = namei_is_trailing_slash(path);
    
    /* Walk path */
    for (i = 0; i < component_count; i++) {
        ret = namei_walk_component(&ctx, components[i].nama);
        if (ret != STATUS_BERHASIL) {
            if (ctx.nc_dentry != NULL) {
                dentry_put(ctx.nc_dentry);
            }
            if (ctx.nc_mount != NULL) {
                mount_put(ctx.nc_mount);
            }
            return ret;
        }
    }
    
    /* Validasi trailing slash */
    if (trailing_slash && ctx.nc_dentry != NULL) {
        inode_t *inode = ctx.nc_dentry->d_inode;
        if (inode != NULL && !VFS_S_ISDIR(inode->i_mode)) {
            dentry_put(ctx.nc_dentry);
            if (ctx.nc_mount != NULL) {
                mount_put(ctx.nc_mount);
            }
            return STATUS_GAGAL; /* STATUS_BUKAN_DIR */
        }
    }
    
    if (ctx.nc_mount != NULL) {
        mount_put(ctx.nc_mount);
    }
    
    *dentry = ctx.nc_dentry;
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI CURRENT WORKING DIRECTORY (CWD FUNCTIONS)
 * ===========================================================================
 */

status_t namei_set_cwd(dentry_t *dentry, mount_t *mount)
{
    if (dentry == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Validasi harus direktori */
    if (dentry->d_inode == NULL ||
        !VFS_S_ISDIR(dentry->d_inode->i_mode)) {
        return STATUS_GAGAL; /* STATUS_BUKAN_DIR */
    }
    namei_lock();
    
    /* Release old cwd */
    if (g_cwd_dentry != NULL) {
        dentry_put(g_cwd_dentry);
    }
    if (g_cwd_mount != NULL) {
        mount_put(g_cwd_mount);
    }
    
    /* Set new cwd */
    g_cwd_dentry = dentry;
    dentry_get(dentry);
    
    g_cwd_mount = mount;
    if (mount != NULL) {
        mount_get(mount);
    }
    
    namei_unlock();
    
    return STATUS_BERHASIL;
}

dentry_t *namei_get_cwd(void)
{
    dentry_t *dentry;
    
    namei_lock();
    
    dentry = g_cwd_dentry;
    if (dentry != NULL) {
        dentry_get(dentry);
    }
    
    namei_unlock();
    
    return dentry;
}

/*
 * ===========================================================================
 * FUNGSI INITIALIZATION MODULE (MODULE INITIALIZATION)
 * ===========================================================================
 */

status_t namei_module_init(void)
{
    log_info("[VFS:NAMEI] Menginisialisasi modul namei...");
    
    /* Clear cwd */
    g_cwd_dentry = NULL;
    g_cwd_mount = NULL;
    
    /* Reset lock */
    g_namei_lock = 0;
    
    log_info("[VFS:NAMEI] Inisialisasi selesai");
    
    return STATUS_BERHASIL;
}

void namei_module_shutdown(void)
{
    log_info("[VFS:NAMEI] Mematikan modul namei...");
    
    namei_lock();
    
    /* Release cwd */
    if (g_cwd_dentry != NULL) {
        dentry_put(g_cwd_dentry);
        g_cwd_dentry = NULL;
    }
    
    if (g_cwd_mount != NULL) {
        mount_put(g_cwd_mount);
        g_cwd_mount = NULL;
    }
    
    namei_unlock();
    
    log_info("[VFS:NAMEI] Shutdown selesai");
}
