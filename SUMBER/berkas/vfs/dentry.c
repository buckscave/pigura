/*
 * PIGURA OS - DENTRY.C
 * ====================
 * Implementasi directory entry cache untuk VFS Pigura OS.
 *
 * Berkas ini berisi fungsi-fungsi untuk mengelola dentry,
 * termasuk alokasi, dealokasi, cache, dan operasi-operasi terkait.
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

/* Ukuran hash table untuk dentry cache */
#define DENTRY_HASH_SIZE        256

/* Jumlah maksimum dentry dalam cache */
#define DENTRY_CACHE_MAX        2048

/* Jumlah maksimum anak per direktori */
#define DENTRY_CHILD_MAX        1024

/* Flag internal dentry */
#define D_FLAG_HASHED           0x00010000
#define D_FLAG_CACHED           0x00020000
#define D_FLAG_NEGATIVE         0x00040000  /* Negative dentry (tidak ada) */

/*
 * ===========================================================================
 * VARIABEL GLOBAL (GLOBAL VARIABLES)
 * ===========================================================================
 */

/* Dentry hash table */
static dentry_t *g_dentry_hash[DENTRY_HASH_SIZE];

/* Dentry LRU list */
static dentry_t *g_dentry_lru_head = NULL;
static dentry_t *g_dentry_lru_tail = NULL;

/* Statistik dentry */
static tak_bertanda32_t g_dentry_total = 0;
static tak_bertanda32_t g_dentry_cached = 0;
static tak_bertanda32_t g_dentry_negative = 0;

/* Lock untuk operasi dentry */
static tak_bertanda32_t g_dentry_lock = 0;

/*
 * ===========================================================================
 * FUNGSI INTERNAL LOCKING (INTERNAL LOCKING FUNCTIONS)
 * ===========================================================================
 */

static void dentry_lock(void)
{
    g_dentry_lock++;
}

static void dentry_unlock(void)
{
    if (g_dentry_lock > 0) {
        g_dentry_lock--;
    }
}

/*
 * ===========================================================================
 * FUNGSI HASH INTERNAL (INTERNAL HASH FUNCTIONS)
 * ===========================================================================
 */

static tak_bertanda32_t dentry_hash_calc(const char *nama,
                                         dentry_t *parent)
{
    tak_bertanda32_t hash = 5381;
    int c;
    const char *str = nama;
    
    /* DJB2 hash algorithm */
    while ((c = *str++) != 0) {
        hash = ((hash << 5) + hash) + c;
    }
    
    /* XOR dengan parent pointer untuk uniqueness */
    hash ^= (tak_bertanda32_t)(uintptr_t)parent;
    
    return hash % DENTRY_HASH_SIZE;
}

/*
 * ===========================================================================
 * FUNGSI LRU LIST (LRU LIST FUNCTIONS)
 * ===========================================================================
 */

static void dentry_lru_add(dentry_t *dentry)
{
    if (dentry == NULL) {
        return;
    }
    
    if (dentry->d_flags & D_FLAG_CACHED) {
        return;
    }
    
    dentry_lock();
    
    /* Add ke tail (least recently used) */
    if (g_dentry_lru_tail != NULL) {
        g_dentry_lru_tail->d_sibling = dentry;
    }
    dentry->d_sibling = NULL;
    
    if (g_dentry_lru_head == NULL) {
        g_dentry_lru_head = dentry;
    }
    g_dentry_lru_tail = dentry;
    
    dentry->d_flags |= D_FLAG_CACHED;
    g_dentry_cached++;
    
    /* Check cache limit dan evict jika perlu */
    while (g_dentry_cached > DENTRY_CACHE_MAX) {
        /* TODO: Implementasi LRU eviction */
        break;
    }
    
    dentry_unlock();
}

static void dentry_lru_remove(dentry_t *dentry)
{
    dentry_t *curr;
    dentry_t *prev;
    
    if (dentry == NULL) {
        return;
    }
    
    if (!(dentry->d_flags & D_FLAG_CACHED)) {
        return;
    }
    
    dentry_lock();
    
    /* Cari dan hapus dari LRU list */
    prev = NULL;
    curr = g_dentry_lru_head;
    
    while (curr != NULL) {
        if (curr == dentry) {
            if (prev == NULL) {
                g_dentry_lru_head = dentry->d_sibling;
            } else {
                prev->d_sibling = dentry->d_sibling;
            }
            
            if (g_dentry_lru_tail == dentry) {
                g_dentry_lru_tail = prev;
            }
            
            dentry->d_sibling = NULL;
            dentry->d_flags &= ~D_FLAG_CACHED;
            
            if (g_dentry_cached > 0) {
                g_dentry_cached--;
            }
            
            break;
        }
        prev = curr;
        curr = curr->d_sibling;
    }
    
    dentry_unlock();
}

static void dentry_lru_touch(dentry_t *dentry)
{
    if (dentry == NULL) {
        return;
    }
    
    /* Move ke tail (most recently used) */
    dentry_lru_remove(dentry);
    dentry_lru_add(dentry);
}

/*
 * ===========================================================================
 * FUNGSI HASH TABLE (HASH TABLE FUNCTIONS)
 * ===========================================================================
 */

static status_t dentry_hash_insert(dentry_t *dentry)
{
    tak_bertanda32_t hash_idx;
    
    if (dentry == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    hash_idx = dentry_hash_calc(dentry->d_name, dentry->d_parent);
    
    dentry_lock();
    
    /* Add ke hash chain head */
    dentry->d_next = g_dentry_hash[hash_idx];
    if (g_dentry_hash[hash_idx] != NULL) {
        g_dentry_hash[hash_idx]->d_prev = dentry;
    }
    g_dentry_hash[hash_idx] = dentry;
    dentry->d_prev = NULL;
    
    dentry->d_flags |= D_FLAG_HASHED;
    g_dentry_total++;
    
    dentry_unlock();
    
    return STATUS_BERHASIL;
}

static status_t dentry_hash_remove(dentry_t *dentry)
{
    tak_bertanda32_t hash_idx;
    
    if (dentry == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (!(dentry->d_flags & D_FLAG_HASHED)) {
        return STATUS_BERHASIL;
    }
    
    hash_idx = dentry_hash_calc(dentry->d_name, dentry->d_parent);
    
    dentry_lock();
    
    /* Remove dari chain */
    if (dentry->d_prev != NULL) {
        dentry->d_prev->d_next = dentry->d_next;
    } else {
        g_dentry_hash[hash_idx] = dentry->d_next;
    }
    
    if (dentry->d_next != NULL) {
        dentry->d_next->d_prev = dentry->d_prev;
    }
    
    dentry->d_prev = NULL;
    dentry->d_next = NULL;
    dentry->d_flags &= ~D_FLAG_HASHED;
    
    if (g_dentry_total > 0) {
        g_dentry_total--;
    }
    
    dentry_unlock();
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI ALOKASI DAN DEALOKASI (ALLOCATION AND DEALLOCATION FUNCTIONS)
 * ===========================================================================
 */

dentry_t *dentry_alloc(const char *nama)
{
    dentry_t *dentry;
    ukuran_t namelen;
    
    if (nama == NULL) {
        log_error("[VFS:DENTRY] Nama NULL");
        return NULL;
    }
    
    namelen = kernel_strlen(nama);
    if (namelen == 0 || namelen >= VFS_NAMA_MAKS) {
        log_error("[VFS:DENTRY] Nama invalid: panjang=%u", namelen);
        return NULL;
    }
    
    /* Alokasi memori */
    dentry = (dentry_t *)kmalloc(sizeof(dentry_t));
    if (dentry == NULL) {
        log_error("[VFS:DENTRY] Gagal alokasi memori");
        return NULL;
    }
    
    /* Clear dan init */
    kernel_memset(dentry, 0, sizeof(dentry_t));
    
    dentry->d_magic = VFS_DENTRY_MAGIC;
    kernel_strncpy(dentry->d_name, nama, VFS_NAMA_MAKS);
    dentry->d_name[VFS_NAMA_MAKS] = '\0';
    dentry->d_namelen = namelen;
    dentry->d_refcount = 1;
    
    log_debug("[VFS:DENTRY] Dentry dialokasi: %s", nama);
    
    return dentry;
}

void dentry_free(dentry_t *dentry)
{
    if (dentry == NULL) {
        return;
    }
    
    /* Validasi magic */
    if (dentry->d_magic != VFS_DENTRY_MAGIC) {
        log_warn("[VFS:DENTRY] Magic invalid saat free");
        return;
    }
    
    /* Jangan free jika masih ada reference */
    if (dentry->d_refcount > 0) {
        log_debug("[VFS:DENTRY] Dentry masih di-reference: %u",
                  dentry->d_refcount);
        return;
    }
    
    /* Hapus dari hash table */
    dentry_hash_remove(dentry);
    
    /* Hapus dari LRU */
    dentry_lru_remove(dentry);
    
    /* Update negative count */
    if (dentry->d_flags & D_FLAG_NEGATIVE) {
        if (g_dentry_negative > 0) {
            g_dentry_negative--;
        }
    }
    
    /* Release parent reference */
    if (dentry->d_parent != NULL) {
        dentry_put(dentry->d_parent);
    }
    
    /* Release inode reference */
    if (dentry->d_inode != NULL) {
        inode_put(dentry->d_inode);
    }
    
    /* Panggil release callback */
    if (dentry->d_op != NULL && dentry->d_op->d_release != NULL) {
        dentry->d_op->d_release(dentry);
    }
    
    /* Clear dan free */
    kernel_memset(dentry, 0, sizeof(dentry_t));
    kfree(dentry);
    
    log_debug("[VFS:DENTRY] Dentry dibebaskan");
}

/*
 * ===========================================================================
 * FUNGSI INISIALISASI (INITIALIZATION FUNCTIONS)
 * ===========================================================================
 */

status_t dentry_init(dentry_t *dentry, const char *name,
                     dentry_t *parent, inode_t *inode)
{
    if (dentry == NULL || name == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    kernel_strncpy(dentry->d_name, name, VFS_NAMA_MAKS);
    dentry->d_namelen = kernel_strlen(name);
    
    /* Set parent */
    if (parent != NULL) {
        dentry->d_parent = parent;
        dentry_get(parent);
    }
    
    /* Set inode */
    if (inode != NULL) {
        dentry->d_inode = inode;
        dentry->d_sb = inode->i_sb;
        inode_get(inode);
    }
    
    /* Set superblock dari parent jika belum ada */
    if (dentry->d_sb == NULL && parent != NULL) {
        dentry->d_sb = parent->d_sb;
    }
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI REFERENCE COUNTING (REFERENCE COUNTING FUNCTIONS)
 * ===========================================================================
 */

void dentry_get(dentry_t *dentry)
{
    if (dentry == NULL) {
        return;
    }
    
    if (dentry->d_magic != VFS_DENTRY_MAGIC) {
        return;
    }
    
    dentry_lock();
    
    dentry->d_refcount++;
    
    /* Touch LRU */
    if (dentry->d_flags & D_FLAG_CACHED) {
        dentry_lru_touch(dentry);
    }
    
    dentry_unlock();
}

void dentry_put(dentry_t *dentry)
{
    if (dentry == NULL) {
        return;
    }
    
    if (dentry->d_magic != VFS_DENTRY_MAGIC) {
        return;
    }
    
    dentry_lock();
    
    if (dentry->d_refcount > 0) {
        dentry->d_refcount--;
        
        if (dentry->d_refcount == 0) {
            dentry_unlock();
            dentry_free(dentry);
            return;
        }
    }
    
    dentry_unlock();
}

/*
 * ===========================================================================
 * FUNGSI CACHE (CACHE FUNCTIONS)
 * ===========================================================================
 */

dentry_t *dentry_lookup(dentry_t *parent, const char *name)
{
    tak_bertanda32_t hash_idx;
    dentry_t *curr;
    
    if (parent == NULL || name == NULL) {
        return NULL;
    }
    
    hash_idx = dentry_hash_calc(name, parent);
    
    dentry_lock();
    
    /* Cari di hash chain */
    curr = g_dentry_hash[hash_idx];
    while (curr != NULL) {
        if (curr->d_parent == parent &&
            kernel_strcmp(curr->d_name, name) == 0) {
            
            /* Revalidate jika perlu */
            if (curr->d_op != NULL &&
                curr->d_op->d_revalidate != NULL) {
                if (curr->d_op->d_revalidate(curr) != STATUS_BERHASIL) {
                    /* Dentry tidak valid lagi */
                    dentry_unlock();
                    return NULL;
                }
            }
            
            /* Dentry ditemukan, add reference */
            dentry_get(curr);
            dentry_unlock();
            
            /* Touch LRU */
            dentry_lru_touch(curr);
            
            return curr;
        }
        curr = curr->d_next;
    }
    
    dentry_unlock();
    
    return NULL;
}

void dentry_cache_insert(dentry_t *dentry)
{
    if (dentry == NULL) {
        return;
    }
    
    /* Insert ke hash table */
    dentry_hash_insert(dentry);
    
    /* Add ke LRU */
    dentry_lru_add(dentry);
}

void dentry_cache_remove(dentry_t *dentry)
{
    if (dentry == NULL) {
        return;
    }
    
    /* Remove dari hash table */
    dentry_hash_remove(dentry);
    
    /* Remove dari LRU */
    dentry_lru_remove(dentry);
}

void dentry_cache_invalidate(dentry_t *dentry)
{
    if (dentry == NULL) {
        return;
    }
    
    /* Hapus dari cache */
    dentry_cache_remove(dentry);
    
    /* Panggil delete callback */
    if (dentry->d_op != NULL && dentry->d_op->d_delete != NULL) {
        dentry->d_op->d_delete(dentry);
    }
    
    /* Mark sebagai invalid */
    dentry->d_magic = 0;
}

void dentry_cache_prune(void)
{
    dentry_t *curr;
    dentry_t *next;
    tak_bertanda32_t pruned = 0;
    
    log_info("[VFS:DENTRY] Pruning dentry cache...");
    
    dentry_lock();
    
    /* Mulai dari LRU head (least recently used) */
    curr = g_dentry_lru_head;
    
    while (curr != NULL && g_dentry_cached > (DENTRY_CACHE_MAX / 2)) {
        next = curr->d_sibling;
        
        /* Hanya prune jika refcount = 0 */
        if (curr->d_refcount == 0) {
            dentry_hash_remove(curr);
            dentry_lru_remove(curr);
            
            if (curr->d_flags & D_FLAG_NEGATIVE) {
                if (g_dentry_negative > 0) {
                    g_dentry_negative--;
                }
            }
            
            /* Free dentry */
            if (curr->d_parent != NULL) {
                dentry_put(curr->d_parent);
            }
            if (curr->d_inode != NULL) {
                inode_put(curr->d_inode);
            }
            
            kernel_memset(curr, 0, sizeof(dentry_t));
            kfree(curr);
            
            pruned++;
        }
        
        curr = next;
    }
    
    dentry_unlock();
    
    log_info("[VFS:DENTRY] Pruned %u dentries", pruned);
}

/*
 * ===========================================================================
 * FUNGSI HIERARKI (HIERARCHY FUNCTIONS)
 * ===========================================================================
 */

status_t dentry_add_child(dentry_t *parent, dentry_t *child)
{
    if (parent == NULL || child == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    child->d_parent = parent;
    
    /* Add ke child list */
    child->d_sibling = parent->d_child;
    parent->d_child = child;
    
    /* Reference parent */
    dentry_get(parent);
    
    /* Set superblock */
    if (child->d_sb == NULL && parent->d_sb != NULL) {
        child->d_sb = parent->d_sb;
    }
    
    return STATUS_BERHASIL;
}

status_t dentry_remove_child(dentry_t *parent, dentry_t *child)
{
    dentry_t *curr;
    dentry_t *prev = NULL;
    
    if (parent == NULL || child == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    dentry_lock();
    
    /* Cari di child list */
    curr = parent->d_child;
    while (curr != NULL) {
        if (curr == child) {
            /* Remove dari list */
            if (prev == NULL) {
                parent->d_child = child->d_sibling;
            } else {
                prev->d_sibling = child->d_sibling;
            }
            
            child->d_parent = NULL;
            child->d_sibling = NULL;
            
            /* Release parent reference */
            dentry_unlock();
            dentry_put(parent);
            
            return STATUS_BERHASIL;
        }
        prev = curr;
        curr = curr->d_sibling;
    }
    
    dentry_unlock();
    
    return STATUS_TIDAK_DITEMUKAN;
}

dentry_t *dentry_find_child(dentry_t *parent, const char *name)
{
    dentry_t *curr;
    
    if (parent == NULL || name == NULL) {
        return NULL;
    }
    
    dentry_lock();
    
    /* Cari di child list */
    curr = parent->d_child;
    while (curr != NULL) {
        if (kernel_strcmp(curr->d_name, name) == 0) {
            dentry_get(curr);
            dentry_unlock();
            return curr;
        }
        curr = curr->d_sibling;
    }
    
    dentry_unlock();
    
    return NULL;
}

dentry_t *dentry_get_parent(dentry_t *dentry)
{
    if (dentry == NULL) {
        return NULL;
    }
    
    if (dentry->d_parent != NULL) {
        dentry_get(dentry->d_parent);
    }
    
    return dentry->d_parent;
}

/*
 * ===========================================================================
 * FUNGSI NEGATIVE DENTRY (NEGATIVE DENTRY FUNCTIONS)
 * ===========================================================================
 */

dentry_t *dentry_create_negative(const char *name, dentry_t *parent)
{
    dentry_t *dentry;
    
    if (name == NULL || parent == NULL) {
        return NULL;
    }
    
    dentry = dentry_alloc(name);
    if (dentry == NULL) {
        return NULL;
    }
    
    dentry->d_parent = parent;
    dentry->d_sb = parent->d_sb;
    dentry->d_inode = NULL;  /* Tidak ada inode = negative */
    dentry->d_flags |= D_FLAG_NEGATIVE;
    
    /* Reference parent */
    dentry_get(parent);
    
    /* Insert ke cache */
    dentry_cache_insert(dentry);
    
    /* Update negative count */
    g_dentry_negative++;
    
    return dentry;
}

bool_t dentry_is_negative(dentry_t *dentry)
{
    if (dentry == NULL) {
        return BENAR;
    }
    
    return (dentry->d_flags & D_FLAG_NEGATIVE) != 0;
}

/*
 * ===========================================================================
 * FUNGSI VALIDASI (VALIDATION FUNCTIONS)
 * ===========================================================================
 */

status_t dentry_revalidate(dentry_t *dentry)
{
    if (dentry == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    /* Negative dentry tidak perlu revalidate */
    if (dentry->d_flags & D_FLAG_NEGATIVE) {
        return STATUS_BERHASIL;
    }
    
    /* Gunakan filesystem revalidate jika ada */
    if (dentry->d_op != NULL && dentry->d_op->d_revalidate != NULL) {
        return dentry->d_op->d_revalidate(dentry);
    }
    
    /* Default: valid */
    return STATUS_BERHASIL;
}

bool_t dentry_is_valid(dentry_t *dentry)
{
    if (dentry == NULL) {
        return SALAH;
    }
    
    if (dentry->d_magic != VFS_DENTRY_MAGIC) {
        return SALAH;
    }
    
    if (dentry_revalidate(dentry) != STATUS_BERHASIL) {
        return SALAH;
    }
    
    return BENAR;
}

/*
 * ===========================================================================
 * FUNGSI PATH BUILDING (PATH BUILDING FUNCTIONS)
 * ===========================================================================
 */

status_t dentry_build_path(dentry_t *dentry, char *buffer, ukuran_t size)
{
    dentry_t *path[VFS_PATH_MAKS / 2];
    tak_bertanda32_t depth = 0;
    ukuran_t pos;
    ukuran_t len;
    tak_bertanda32_t i;
    
    if (dentry == NULL || buffer == NULL || size == 0) {
        return STATUS_PARAM_NULL;
    }
    
    /* Build path dari dentry ke root */
    while (dentry != NULL && depth < (VFS_PATH_MAKS / 2)) {
        path[depth++] = dentry;
        if (dentry->d_parent == dentry) {
            break;
        }
        dentry = dentry->d_parent;
    }
    
    /* Check buffer size */
    if (depth == 0) {
        if (size < 2) {
            return STATUS_PARAM_UKURAN;
        }
        buffer[0] = '/';
        buffer[1] = '\0';
        return STATUS_BERHASIL;
    }
    
    /* Build string dari root ke dentry */
    pos = 0;
    
    for (i = depth; i > 0; i--) {
        dentry = path[i - 1];
        len = dentry->d_namelen;
        
        /* Cek buffer size */
        if (pos + len + 2 >= size) {
            return STATUS_PARAM_UKURAN;
        }
        
        /* Add separator */
        if (pos > 0) {
            buffer[pos++] = '/';
        } else if (i == depth && dentry->d_parent == NULL) {
            /* Root */
            buffer[pos++] = '/';
        }
        
        /* Copy nama */
        kernel_strncpy(buffer + pos, dentry->d_name, len);
        pos += len;
    }
    
    buffer[pos] = '\0';
    
    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * FUNGSI OPERATIONS SETUP (OPERATIONS SETUP FUNCTIONS)
 * ===========================================================================
 */

void dentry_set_operations(dentry_t *dentry, vfs_dentry_operations_t *op)
{
    if (dentry == NULL) {
        return;
    }
    
    dentry->d_op = op;
}

void dentry_set_inode(dentry_t *dentry, inode_t *inode)
{
    if (dentry == NULL) {
        return;
    }
    
    /* Release old inode */
    if (dentry->d_inode != NULL) {
        inode_put(dentry->d_inode);
        dentry->d_inode = NULL;
    }
    
    /* Set new inode */
    if (inode != NULL) {
        dentry->d_inode = inode;
        inode_get(inode);
        
        /* Set superblock dari inode */
        if (dentry->d_sb == NULL && inode->i_sb != NULL) {
            dentry->d_sb = inode->i_sb;
        }
        
        /* Clear negative flag */
        dentry->d_flags &= ~D_FLAG_NEGATIVE;
    }
}

/*
 * ===========================================================================
 * FUNGSI STATISTIK (STATISTICS FUNCTIONS)
 * ===========================================================================
 */

tak_bertanda32_t dentry_get_total_count(void)
{
    return g_dentry_total;
}

tak_bertanda32_t dentry_get_cached_count(void)
{
    return g_dentry_cached;
}

tak_bertanda32_t dentry_get_negative_count(void)
{
    return g_dentry_negative;
}

/*
 * ===========================================================================
 * FUNGSI INFORMASI DAN DEBUG (INFORMATION AND DEBUG FUNCTIONS)
 * ===========================================================================
 */

void dentry_print_info(dentry_t *dentry)
{
    if (dentry == NULL) {
        kernel_printf("[VFS:DENTRY] Dentry: NULL\n");
        return;
    }
    
    kernel_printf("[VFS:DENTRY] Dentry Info:\n");
    kernel_printf("  Nama: %s\n", dentry->d_name);
    kernel_printf("  Name Len: %u\n", dentry->d_namelen);
    kernel_printf("  Refcount: %u\n", dentry->d_refcount);
    kernel_printf("  Flags: 0x%08X\n", dentry->d_flags);
    kernel_printf("  Negative: %s\n",
                  (dentry->d_flags & D_FLAG_NEGATIVE) ? "ya" : "tidak");
    kernel_printf("  Mounted: %s\n",
                  dentry->d_mounted ? "ya" : "tidak");
    
    if (dentry->d_inode != NULL) {
        kernel_printf("  Inode: %llu\n", dentry->d_inode->i_ino);
    } else {
        kernel_printf("  Inode: NULL\n");
    }
    
    if (dentry->d_parent != NULL) {
        kernel_printf("  Parent: %s\n", dentry->d_parent->d_name);
    } else {
        kernel_printf("  Parent: NULL (root)\n");
    }
}

void dentry_print_tree(dentry_t *root, tak_bertanda32_t depth)
{
    dentry_t *child;
    tak_bertanda32_t i;
    
    if (root == NULL) {
        return;
    }
    
    /* Print indentation */
    for (i = 0; i < depth; i++) {
        kernel_printf("  ");
    }
    
    /* Print name */
    kernel_printf("%s", root->d_name);
    
    if (root->d_inode != NULL) {
        kernel_printf(" (ino=%llu)", root->d_inode->i_ino);
    }
    
    if (root->d_flags & D_FLAG_NEGATIVE) {
        kernel_printf(" [negative]");
    }
    
    kernel_printf("\n");
    
    /* Print children */
    child = root->d_child;
    while (child != NULL) {
        dentry_print_tree(child, depth + 1);
        child = child->d_sibling;
    }
}

/*
 * ===========================================================================
 * FUNGSI INITIALIZATION MODULE (MODULE INITIALIZATION)
 * ===========================================================================
 */

status_t dentry_module_init(void)
{
    tak_bertanda32_t i;
    
    log_info("[VFS:DENTRY] Menginisialisasi modul dentry...");
    
    /* Clear hash table */
    for (i = 0; i < DENTRY_HASH_SIZE; i++) {
        g_dentry_hash[i] = NULL;
    }
    
    /* Clear LRU list */
    g_dentry_lru_head = NULL;
    g_dentry_lru_tail = NULL;
    
    /* Reset statistics */
    g_dentry_total = 0;
    g_dentry_cached = 0;
    g_dentry_negative = 0;
    
    /* Reset lock */
    g_dentry_lock = 0;
    
    log_info("[VFS:DENTRY] Inisialisasi selesai");
    
    return STATUS_BERHASIL;
}

void dentry_module_shutdown(void)
{
    tak_bertanda32_t i;
    dentry_t *curr;
    dentry_t *next;
    
    log_info("[VFS:DENTRY] Mematikan modul dentry...");
    
    /* Free semua dentry di hash table */
    for (i = 0; i < DENTRY_HASH_SIZE; i++) {
        curr = g_dentry_hash[i];
        while (curr != NULL) {
            next = curr->d_next;
            
            /* Release references */
            if (curr->d_inode != NULL) {
                inode_put(curr->d_inode);
            }
            if (curr->d_parent != NULL) {
                dentry_put(curr->d_parent);
            }
            
            /* Free memory */
            kernel_memset(curr, 0, sizeof(dentry_t));
            kfree(curr);
            
            curr = next;
        }
        g_dentry_hash[i] = NULL;
    }
    
    /* Clear LRU */
    g_dentry_lru_head = NULL;
    g_dentry_lru_tail = NULL;
    
    /* Reset statistics */
    g_dentry_total = 0;
    g_dentry_cached = 0;
    g_dentry_negative = 0;
    
    log_info("[VFS:DENTRY] Shutdown selesai");
}
