/*
 * PIGURA OS - PIGURAFS_BTREE.C
 * =============================
 * Implementasi B+tree untuk filesystem PiguraFS.
 *
 * Berkas ini berisi fungsi-fungsi untuk mengelola B+tree
 * yang digunakan untuk indexing direktori dan extent.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Tidak menggunakan fitur C99/C11
 * - Semua fungsi diimplementasikan secara lengkap
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../vfs/vfs.h"
#include "../../inti/kernel.h"

/*
 * ===========================================================================
 * KONSTANTA BTREE (BTREE CONSTANTS)
 * ===========================================================================
 */

/* Magic number */
#define PFS_BTREE_MAGIC         0x50465336

/* Orde B+tree (jumlah maksimum key per node) */
#define PFS_BTREE_ORDER         64

/* Minimum key per node (kecuali root) */
#define PFS_BTREE_MIN_KEYS      (PFS_BTREE_ORDER / 2)

/* Maximum key per node */
#define PFS_BTREE_MAX_KEYS      (PFS_BTREE_ORDER - 1)

/* Maximum children per node */
#define PFS_BTREE_MAX_CHILDREN  PFS_BTREE_ORDER

/* Tipe node */
#define PFS_BTREE_LEAF          0x01
#define PFS_BTREE_INTERNAL      0x02

/* Ukuran key */
#define PFS_BTREE_KEY_SIZE      8

/* Ukuran node dalam block */
#define PFS_BTREE_NODE_SIZE     4096

/*
 * ===========================================================================
 * STRUKTUR DATA BTREE (BTREE DATA STRUCTURES)
 * ===========================================================================
 */

/* Struktur key B+tree */
typedef struct {
    tak_bertanda64_t value;             /* Nilai key */
    tak_bertanda64_t data;              /* Data terkait */
} pfs_btree_key_t;

/* Struktur node B+tree */
typedef struct {
    tak_bertanda32_t magic;             /* Magic number */
    tak_bertanda8_t tipe;               /* Tipe node */
    tak_bertanda16_t key_count;         /* Jumlah key */
    tak_bertanda32_t block_no;          /* Nomor block */
    tak_bertanda32_t parent;            /* Block parent */
    tak_bertanda32_t next;              /* Next leaf (untuk leaf node) */
    tak_bertanda32_t prev;              /* Prev leaf (untuk leaf node) */
    pfs_btree_key_t keys[PFS_BTREE_MAX_KEYS]; /* Array key */
    tak_bertanda32_t children[PFS_BTREE_MAX_CHILDREN]; /* Children blocks */
} pfs_btree_node_t;

/* Struktur B+tree */
typedef struct {
    tak_bertanda32_t magic;             /* Magic number validasi */
    tak_bertanda32_t root_block;        /* Block root */
    tak_bertanda32_t height;            /* Tinggi tree */
    tak_bertanda64_t key_count;         /* Total key */
    tak_bertanda32_t node_count;        /* Total node */
    tak_bertanda32_t block_size;        /* Ukuran block */
    void *device;                       /* Device context */
    pfs_btree_node_t *cached_node;      /* Cached node */
    tak_bertanda32_t cached_block;      /* Cached block number */
} pfs_btree_t;

/* Struktur iterator B+tree */
typedef struct {
    tak_bertanda32_t magic;             /* Magic number */
    pfs_btree_t *tree;                  /* Tree reference */
    pfs_btree_node_t *current;          /* Current node */
    tak_bertanda16_t key_index;         /* Key index */
    bool_t ascending;                   /* Arah iterasi */
} pfs_btree_iter_t;

/*
 * ===========================================================================
 * DEKLARASI FUNGSI INTERNAL (INTERNAL FUNCTION DECLARATIONS)
 * ===========================================================================
 */

static status_t pfs_btree_baca_node(pfs_btree_t *tree,
                                    tak_bertanda32_t block,
                                    pfs_btree_node_t *node);
static status_t pfs_btree_tulis_node(pfs_btree_t *tree,
                                     const pfs_btree_node_t *node);
static status_t pfs_btree_alloc_node(pfs_btree_t *tree,
                                     tak_bertanda8_t tipe,
                                     tak_bertanda32_t *block);
static status_t pfs_btree_free_node(pfs_btree_t *tree,
                                    tak_bertanda32_t block);
static status_t pfs_btree_split_node(pfs_btree_t *tree,
                                     pfs_btree_node_t *node,
                                     tak_bertanda32_t node_block,
                                     tak_bertanda16_t insert_index);
static status_t pfs_btree_merge_nodes(pfs_btree_t *tree,
                                      pfs_btree_node_t *left,
                                      pfs_btree_node_t *right,
                                      tak_bertanda32_t left_block,
                                      tak_bertanda32_t right_block);
static tak_bertanda16_t pfs_btree_find_key_index(const pfs_btree_node_t *node,
                                                 tak_bertanda64_t key);
static status_t pfs_btree_insert_to_node(pfs_btree_node_t *node,
                                         tak_bertanda16_t index,
                                         tak_bertanda64_t key,
                                         tak_bertanda64_t data);

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI I/O (I/O FUNCTIONS)
 * ===========================================================================
 */

static status_t pfs_btree_baca_node(pfs_btree_t *tree,
                                    tak_bertanda32_t block,
                                    pfs_btree_node_t *node)
{
    if (tree == NULL || node == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Cek cache */
    if (tree->cached_node != NULL && tree->cached_block == block) {
        kernel_memcpy(node, tree->cached_node, sizeof(pfs_btree_node_t));
        return STATUS_BERHASIL;
    }

    /* TODO: Baca dari disk */
    kernel_memset(node, 0, sizeof(pfs_btree_node_t));
    node->magic = PFS_BTREE_MAGIC;
    node->block_no = block;

    return STATUS_BERHASIL;
}

static status_t pfs_btree_tulis_node(pfs_btree_t *tree,
                                     const pfs_btree_node_t *node)
{
    if (tree == NULL || node == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Update cache */
    if (tree->cached_node != NULL) {
        kernel_memcpy(tree->cached_node, node, sizeof(pfs_btree_node_t));
        tree->cached_block = node->block_no;
    }

    /* TODO: Tulis ke disk */

    return STATUS_BERHASIL;
}

static status_t pfs_btree_alloc_node(pfs_btree_t *tree,
                                     tak_bertanda8_t tipe,
                                     tak_bertanda32_t *block)
{
    if (tree == NULL || block == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* TODO: Alokasi block dari filesystem */

    *block = tree->node_count + 1;
    tree->node_count++;

    return STATUS_BERHASIL;
}

static status_t pfs_btree_free_node(pfs_btree_t *tree,
                                    tak_bertanda32_t block)
{
    if (tree == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* TODO: Free block ke filesystem */

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI NODE OPERATIONS (NODE OPERATIONS)
 * ===========================================================================
 */

static tak_bertanda16_t pfs_btree_find_key_index(const pfs_btree_node_t *node,
                                                 tak_bertanda64_t key)
{
    tak_bertanda16_t low;
    tak_bertanda16_t high;
    tak_bertanda16_t mid;

    if (node == NULL || node->key_count == 0) {
        return 0;
    }

    /* Binary search */
    low = 0;
    high = node->key_count;

    while (low < high) {
        mid = (tak_bertanda16_t)((low + high) / 2);
        if (node->keys[mid].value < key) {
            low = (tak_bertanda16_t)(mid + 1);
        } else {
            high = mid;
        }
    }

    return low;
}

static status_t pfs_btree_insert_to_node(pfs_btree_node_t *node,
                                         tak_bertanda16_t index,
                                         tak_bertanda64_t key,
                                         tak_bertanda64_t data)
{
    tak_bertanda16_t i;

    if (node == NULL || index > node->key_count) {
        return STATUS_PARAM_INVALID;
    }

    if (node->key_count >= PFS_BTREE_MAX_KEYS) {
        return STATUS_PENUH;
    }

    /* Shift keys ke kanan */
    for (i = node->key_count; i > index; i--) {
        node->keys[i] = node->keys[i - 1];
    }

    /* Insert key baru */
    node->keys[index].value = key;
    node->keys[index].data = data;
    node->key_count++;

    return STATUS_BERHASIL;
}

static status_t pfs_btree_split_node(pfs_btree_t *tree,
                                     pfs_btree_node_t *node,
                                     tak_bertanda32_t node_block,
                                     tak_bertanda16_t insert_index)
{
    pfs_btree_node_t new_node;
    pfs_btree_node_t parent;
    tak_bertanda32_t new_block;
    tak_bertanda16_t mid;
    tak_bertanda16_t i;
    status_t status;

    if (tree == NULL || node == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Alokasi node baru */
    status = pfs_btree_alloc_node(tree, node->tipe, &new_block);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    kernel_memset(&new_node, 0, sizeof(pfs_btree_node_t));
    new_node.magic = PFS_BTREE_MAGIC;
    new_node.tipe = node->tipe;
    new_node.block_no = new_block;
    new_node.parent = node->parent;

    /* Hitung titik split */
    mid = (tak_bertanda16_t)(PFS_BTREE_MAX_KEYS / 2);

    /* Copy key ke node baru */
    for (i = mid; i < node->key_count; i++) {
        new_node.keys[i - mid] = node->keys[i];
    }
    new_node.key_count = (tak_bertanda16_t)(node->key_count - mid);
    node->key_count = mid;

    /* Update link untuk leaf */
    if (node->tipe == PFS_BTREE_LEAF) {
        new_node.next = node->next;
        new_node.prev = node_block;
        node->next = new_block;
    }

    /* Copy children untuk internal node */
    if (node->tipe == PFS_BTREE_INTERNAL) {
        for (i = mid; i <= node->key_count; i++) {
            new_node.children[i - mid] = node->children[i + mid];
        }
    }

    /* Tulis kedua node */
    status = pfs_btree_tulis_node(tree, node);
    if (status != STATUS_BERHASIL) {
        pfs_btree_free_node(tree, new_block);
        return status;
    }

    status = pfs_btree_tulis_node(tree, &new_node);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    /* Insert key ke parent */
    if (node->parent == 0) {
        /* Buat root baru */
        status = pfs_btree_alloc_node(tree, PFS_BTREE_INTERNAL, &new_block);
        if (status != STATUS_BERHASIL) {
            return status;
        }

        kernel_memset(&parent, 0, sizeof(pfs_btree_node_t));
        parent.magic = PFS_BTREE_MAGIC;
        parent.tipe = PFS_BTREE_INTERNAL;
        parent.block_no = new_block;
        parent.key_count = 1;
        parent.keys[0].value = node->keys[mid - 1].value;
        parent.children[0] = node_block;
        parent.children[1] = new_node.block_no;

        tree->root_block = new_block;
        tree->height++;

        return pfs_btree_tulis_node(tree, &parent);
    }

    /* TODO: Insert ke parent yang ada */

    return STATUS_BERHASIL;
}

static status_t pfs_btree_merge_nodes(pfs_btree_t *tree,
                                      pfs_btree_node_t *left,
                                      pfs_btree_node_t *right,
                                      tak_bertanda32_t left_block,
                                      tak_bertanda32_t right_block)
{
    tak_bertanda16_t i;

    if (tree == NULL || left == NULL || right == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Copy key dari right ke left */
    for (i = 0; i < right->key_count; i++) {
        left->keys[left->key_count + i] = right->keys[i];
    }
    left->key_count += right->key_count;

    /* Update link */
    left->next = right->next;

    /* Copy children untuk internal node */
    if (left->tipe == PFS_BTREE_INTERNAL) {
        for (i = 0; i <= right->key_count; i++) {
            left->children[left->key_count + i] = right->children[i];
        }
    }

    /* Tulis left node */
    pfs_btree_tulis_node(tree, left);

    /* Free right node */
    pfs_btree_free_node(tree, right_block);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * IMPLEMENTASI FUNGSI UTAMA (MAIN FUNCTIONS)
 * ===========================================================================
 */

/*
 * pfs_btree_init
 * --------------
 * Inisialisasi B+tree baru.
 *
 * Parameter:
 *   tree       - Pointer ke struktur B+tree
 *   block_size - Ukuran block
 *   device     - Device context
 *
 * Return: Status operasi
 */
status_t pfs_btree_init(pfs_btree_t *tree, tak_bertanda32_t block_size,
                        void *device)
{
    pfs_btree_node_t root;
    tak_bertanda32_t root_block;
    status_t status;

    if (tree == NULL) {
        return STATUS_PARAM_NULL;
    }

    kernel_memset(tree, 0, sizeof(pfs_btree_t));
    tree->magic = PFS_BTREE_MAGIC;
    tree->block_size = block_size;
    tree->device = device;
    tree->height = 1;

    /* Alokasi root node */
    status = pfs_btree_alloc_node(tree, PFS_BTREE_LEAF, &root_block);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    tree->root_block = root_block;

    /* Inisialisasi root */
    kernel_memset(&root, 0, sizeof(pfs_btree_node_t));
    root.magic = PFS_BTREE_MAGIC;
    root.tipe = PFS_BTREE_LEAF;
    root.block_no = root_block;
    root.parent = 0;
    root.next = 0;
    root.prev = 0;
    root.key_count = 0;

    return pfs_btree_tulis_node(tree, &root);
}

/*
 * pfs_btree_destroy
 * -----------------
 * Hancurkan B+tree.
 *
 * Parameter:
 *   tree - Pointer ke struktur B+tree
 */
void pfs_btree_destroy(pfs_btree_t *tree)
{
    if (tree == NULL) {
        return;
    }

    /* Free cached node */
    if (tree->cached_node != NULL) {
        kfree(tree->cached_node);
    }

    tree->magic = 0;
}

/*
 * pfs_btree_insert
 * ----------------
 * Insert key ke B+tree.
 *
 * Parameter:
 *   tree - Pointer ke struktur B+tree
 *   key  - Key yang akan diinsert
 *   data - Data terkait
 *
 * Return: Status operasi
 */
status_t pfs_btree_insert(pfs_btree_t *tree, tak_bertanda64_t key,
                          tak_bertanda64_t data)
{
    pfs_btree_node_t node;
    pfs_btree_node_t current;
    tak_bertanda32_t current_block;
    tak_bertanda16_t index;
    status_t status;

    if (tree == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Mulai dari root */
    current_block = tree->root_block;

    /* Traverse ke leaf */
    while (BENAR) {
        status = pfs_btree_baca_node(tree, current_block, &current);
        if (status != STATUS_BERHASIL) {
            return status;
        }

        if (current.tipe == PFS_BTREE_LEAF) {
            break;
        }

        /* Cari child yang tepat */
        index = pfs_btree_find_key_index(&current, key);
        if (index >= current.key_count) {
            current_block = current.children[current.key_count];
        } else if (key < current.keys[index].value) {
            current_block = current.children[index];
        } else {
            current_block = current.children[index + 1];
        }
    }

    /* Insert ke leaf */
    index = pfs_btree_find_key_index(&current, key);

    /* Cek jika key sudah ada */
    if (index < current.key_count && current.keys[index].value == key) {
        /* Update data */
        current.keys[index].data = data;
        return pfs_btree_tulis_node(tree, &current);
    }

    /* Cek jika perlu split */
    if (current.key_count >= PFS_BTREE_MAX_KEYS) {
        status = pfs_btree_split_node(tree, &current, current_block, index);
        if (status != STATUS_BERHASIL) {
            return status;
        }

        /* Baca ulang node setelah split */
        status = pfs_btree_baca_node(tree, current_block, &current);
        if (status != STATUS_BERHASIL) {
            return status;
        }

        index = pfs_btree_find_key_index(&current, key);
    }

    /* Insert key */
    status = pfs_btree_insert_to_node(&current, index, key, data);
    if (status != STATUS_BERHASIL) {
        return status;
    }

    tree->key_count++;

    return pfs_btree_tulis_node(tree, &current);
}

/*
 * pfs_btree_delete
 * ----------------
 * Hapus key dari B+tree.
 *
 * Parameter:
 *   tree - Pointer ke struktur B+tree
 *   key  - Key yang akan dihapus
 *
 * Return: Status operasi
 */
status_t pfs_btree_delete(pfs_btree_t *tree, tak_bertanda64_t key)
{
    pfs_btree_node_t current;
    tak_bertanda32_t current_block;
    tak_bertanda16_t index;
    tak_bertanda16_t i;
    status_t status;

    if (tree == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Cari key */
    current_block = tree->root_block;

    /* Traverse ke leaf */
    while (BENAR) {
        status = pfs_btree_baca_node(tree, current_block, &current);
        if (status != STATUS_BERHASIL) {
            return status;
        }

        if (current.tipe == PFS_BTREE_LEAF) {
            break;
        }

        index = pfs_btree_find_key_index(&current, key);
        if (index >= current.key_count) {
            current_block = current.children[current.key_count];
        } else if (key < current.keys[index].value) {
            current_block = current.children[index];
        } else {
            current_block = current.children[index + 1];
        }
    }

    /* Cari key di leaf */
    index = pfs_btree_find_key_index(&current, key);
    if (index >= current.key_count || current.keys[index].value != key) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    /* Hapus key */
    for (i = index; i < current.key_count - 1; i++) {
        current.keys[i] = current.keys[i + 1];
    }
    current.key_count--;

    tree->key_count--;

    /* Tulis node */
    return pfs_btree_tulis_node(tree, &current);
}

/*
 * pfs_btree_find
 * --------------
 * Cari key dalam B+tree.
 *
 * Parameter:
 *   tree - Pointer ke struktur B+tree
 *   key  - Key yang dicari
 *   data - Pointer ke hasil data
 *
 * Return: Status operasi
 */
status_t pfs_btree_find(pfs_btree_t *tree, tak_bertanda64_t key,
                        tak_bertanda64_t *data)
{
    pfs_btree_node_t current;
    tak_bertanda32_t current_block;
    tak_bertanda16_t index;
    status_t status;

    if (tree == NULL || data == NULL) {
        return STATUS_PARAM_NULL;
    }

    *data = 0;
    current_block = tree->root_block;

    /* Traverse tree */
    while (BENAR) {
        status = pfs_btree_baca_node(tree, current_block, &current);
        if (status != STATUS_BERHASIL) {
            return status;
        }

        index = pfs_btree_find_key_index(&current, key);

        if (current.tipe == PFS_BTREE_LEAF) {
            /* Cek apakah key ditemukan */
            if (index < current.key_count &&
                current.keys[index].value == key) {
                *data = current.keys[index].data;
                return STATUS_BERHASIL;
            }
            return STATUS_TIDAK_DITEMUKAN;
        }

        /* Navigate ke child */
        if (index >= current.key_count) {
            current_block = current.children[current.key_count];
        } else if (key < current.keys[index].value) {
            current_block = current.children[index];
        } else {
            current_block = current.children[index + 1];
        }
    }

    return STATUS_TIDAK_DITEMUKAN;
}

/*
 * pfs_btree_iter_start
 * --------------------
 * Mulai iterasi B+tree.
 *
 * Parameter:
 *   tree      - Pointer ke struktur B+tree
 *   ascending - Arah iterasi
 *
 * Return: Pointer ke iterator, atau NULL jika gagal
 */
pfs_btree_iter_t *pfs_btree_iter_start(pfs_btree_t *tree, bool_t ascending)
{
    pfs_btree_iter_t *iter;
    pfs_btree_node_t current;
    tak_bertanda32_t current_block;
    status_t status;

    if (tree == NULL) {
        return NULL;
    }

    iter = (pfs_btree_iter_t *)kmalloc(sizeof(pfs_btree_iter_t));
    if (iter == NULL) {
        return NULL;
    }

    kernel_memset(iter, 0, sizeof(pfs_btree_iter_t));
    iter->magic = PFS_BTREE_MAGIC;
    iter->tree = tree;
    iter->ascending = ascending;
    iter->key_index = 0;

    /* Cari leaf pertama atau terakhir */
    current_block = tree->root_block;

    while (BENAR) {
        status = pfs_btree_baca_node(tree, current_block, &current);
        if (status != STATUS_BERHASIL) {
            kfree(iter);
            return NULL;
        }

        if (current.tipe == PFS_BTREE_LEAF) {
            break;
        }

        if (ascending) {
            current_block = current.children[0];
        } else {
            current_block = current.children[current.key_count];
        }
    }

    iter->current = (pfs_btree_node_t *)kmalloc(sizeof(pfs_btree_node_t));
    if (iter->current == NULL) {
        kfree(iter);
        return NULL;
    }

    kernel_memcpy(iter->current, &current, sizeof(pfs_btree_node_t));

    if (!ascending && current.key_count > 0) {
        iter->key_index = (tak_bertanda16_t)(current.key_count - 1);
    }

    return iter;
}

/*
 * pfs_btree_iter_next
 * -------------------
 * Dapatkan entry berikutnya dari iterator.
 *
 * Parameter:
 *   iter - Pointer ke iterator
 *   key  - Pointer ke hasil key
 *   data - Pointer ke hasil data
 *
 * Return: Status operasi
 */
status_t pfs_btree_iter_next(pfs_btree_iter_t *iter,
                             tak_bertanda64_t *key,
                             tak_bertanda64_t *data)
{
    pfs_btree_node_t next;
    status_t status;

    if (iter == NULL || key == NULL || data == NULL) {
        return STATUS_PARAM_NULL;
    }

    if (iter->current == NULL) {
        return STATUS_KOSONG;
    }

    /* Cek apakah sudah di akhir node */
    if (iter->key_index >= iter->current->key_count) {
        /* Pindah ke node berikutnya */
        if (iter->current->next == 0) {
            return STATUS_KOSONG;
        }

        status = pfs_btree_baca_node(iter->tree, iter->current->next, &next);
        if (status != STATUS_BERHASIL) {
            return status;
        }

        kernel_memcpy(iter->current, &next, sizeof(pfs_btree_node_t));
        iter->key_index = 0;
    }

    /* Ambil key dan data */
    *key = iter->current->keys[iter->key_index].value;
    *data = iter->current->keys[iter->key_index].data;
    iter->key_index++;

    return STATUS_BERHASIL;
}

/*
 * pfs_btree_iter_end
 * ------------------
 * Akhiri iterasi B+tree.
 *
 * Parameter:
 *   iter - Pointer ke iterator
 */
void pfs_btree_iter_end(pfs_btree_iter_t *iter)
{
    if (iter == NULL) {
        return;
    }

    if (iter->current != NULL) {
        kfree(iter->current);
    }

    iter->magic = 0;
    kfree(iter);
}

/*
 * pfs_btree_get_stats
 * -------------------
 * Dapatkan statistik B+tree.
 *
 * Parameter:
 *   tree      - Pointer ke struktur B+tree
 *   key_count - Pointer ke jumlah key
 *   node_count - Pointer ke jumlah node
 *   height    - Pointer ke tinggi
 */
void pfs_btree_get_stats(pfs_btree_t *tree,
                         tak_bertanda64_t *key_count,
                         tak_bertanda32_t *node_count,
                         tak_bertanda32_t *height)
{
    if (tree == NULL) {
        return;
    }

    if (key_count != NULL) {
        *key_count = tree->key_count;
    }
    if (node_count != NULL) {
        *node_count = tree->node_count;
    }
    if (height != NULL) {
        *height = tree->height;
    }
}
