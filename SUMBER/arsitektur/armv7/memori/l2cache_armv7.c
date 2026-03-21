/*
 * PIGURA OS - L2CACHE_ARMV7.C
 * ----------------------------
 * Implementasi operasi L2 Cache untuk ARMv7.
 *
 * Berkas ini berisi fungsi-fungsi untuk mengelola L2 cache
 * pada prosesor ARMv7 Cortex-A series.
 *
 * Arsitektur: ARMv7 (Cortex-A series)
 * Versi: 1.0
 */

#include "../../../inti/types.h"
#include "../../../inti/konstanta.h"
#include "../../../inti/hal/hal.h"

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

/* PL310 L2 Cache Controller registers (QEMU virt) */
#define L2C_BASE            0x0A000000

/* PL310 Register offsets */
#define L2C_CACHE_ID        0x000       /* Cache ID Register */
#define L2C_CACHE_TYPE      0x004       /* Cache Type Register */
#define L2C_CTRL            0x100       /* Control Register */
#define L2C_AUX_CTRL        0x104       /* Auxiliary Control Register */
#define L2C_TAG_RAM_CTRL    0x108       /* Tag RAM Control Register */
#define L2C_DATA_RAM_CTRL   0x10C       /* Data RAM Control Register */
#define L2C_EV_COUNTER_CTRL 0x200       /* Event Counter Control */
#define L2C_EV_COUNTER1_CFG 0x204       /* Event Counter 1 Config */
#define L2C_EV_COUNTER0_CFG 0x208       /* Event Counter 0 Config */
#define L2C_EV_COUNTER1     0x20C       /* Event Counter 1 Value */
#define L2C_EV_COUNTER0     0x210       /* Event Counter 0 Value */
#define L2C_INT_MASK        0x214       /* Interrupt Mask Register */
#define L2C_INT_MASK_STATUS 0x218       /* Interrupt Mask Status */
#define L2C_INT_RAW_STATUS  0x21C       /* Interrupt Raw Status */
#define L2C_INT_CLEAR       0x220       /* Interrupt Clear Register */
#define L2C_CACHE_SYNC      0x730       /* Cache Sync Register */
#define L2C_INV_LINE_PA     0x770       /* Invalidate by PA */
#define L2C_INV_WAY         0x77C       /* Invalidate by Way */
#define L2C_CLEAN_LINE_PA   0x7B0       /* Clean by PA */
#define L2C_CLEAN_LINE_IDX  0x7B8       /* Clean by Index */
#define L2C_CLEAN_WAY       0x7BC       /* Clean by Way */
#define L2C_CLEAN_INV_LINE_PA 0x7F0     /* Clean & Invalidate by PA */
#define L2C_CLEAN_INV_LINE_IDX 0x7F8    /* Clean & Invalidate by Index */
#define L2C_CLEAN_INV_WAY   0x7FC       /* Clean & Invalidate by Way */
#define L2C_LOCKDOWN_D_WAY  0x900       /* Data Lockdown */
#define L2C_LOCKDOWN_I_WAY  0x904       /* Instruction Lockdown */
#define L2C_LOCKDOWN_EN     0x950       /* Lockdown Enable */
#define L2C_ADDR_FILTERING  0xC00       /* Address Filtering Start */

/* Control Register bits */
#define L2C_CTRL_ENABLE     (1 << 0)    /* L2 cache enable */

/* Auxiliary Control Register bits */
#define L2C_AUX_EARLY_BRESP (1 << 30)   /* Early BRESP enable */
#define L2C_AUX_IPREFETCH   (1 << 29)   /* Instruction prefetch enable */
#define L2C_AUX_DPREFETCH   (1 << 28)   /* Data prefetch enable */
#define L2C_AUX_NS_LOCKDOWN (1 << 26)   /* Non-secure lockdown enable */
#define L2C_AUX_NS_INT_CTRL (1 << 25)   /* Non-secure interrupt control */
#define L2C_AUX_DATA_LATENCY_SHIFT 0    /* Data RAM latency */
#define L2C_AUX_TAG_LATENCY_SHIFT  8    /* Tag RAM latency */

/* Default latencies */
#define L2C_LATENCY_1_CYCLE  0x0
#define L2C_LATENCY_2_CYCLES 0x1
#define L2C_LATENCY_3_CYCLES 0x2
#define L2C_LATENCY_4_CYCLES 0x3
#define L2C_LATENCY_5_CYCLES 0x4
#define L2C_LATENCY_6_CYCLES 0x5
#define L2C_LATENCY_7_CYCLES 0x6
#define L2C_LATENCY_8_CYCLES 0x7

/* Way mask (assuming 8-way, 512KB cache) */
#define L2C_WAYS            8
#define L2C_WAY_MASK        0xFF

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* L2 cache base address */
static volatile tak_bertanda32_t *g_l2c_base = NULL;

/* L2 cache info */
static tak_bertanda32_t g_l2c_size = 0;
static tak_bertanda32_t g_l2c_line_size = 32;
static tak_bertanda32_t g_l2c_ways = 8;

/* Flag inisialisasi */
static bool_t g_l2c_initialized = SALAH;
static bool_t g_l2c_enabled = SALAH;

/*
 * ============================================================================
 * FUNGSI ACESS REGISTER (REGISTER ACCESS FUNCTIONS)
 * ============================================================================
 */

/*
 * l2c_read
 * --------
 * Baca register L2 cache controller.
 *
 * Parameter:
 *   offset - Offset register
 *
 * Return: Nilai register
 */
static inline tak_bertanda32_t l2c_read(tak_bertanda32_t offset)
{
    return hal_mmio_read_32((void *)((tak_bertanda32_t)g_l2c_base + offset));
}

/*
 * l2c_write
 * ---------
 * Tulis register L2 cache controller.
 *
 * Parameter:
 *   offset - Offset register
 *   value  - Nilai yang ditulis
 */
static inline void l2c_write(tak_bertanda32_t offset,
    tak_bertanda32_t value)
{
    hal_mmio_write_32((void *)((tak_bertanda32_t)g_l2c_base + offset), value);
}

/*
 * l2c_sync
 * --------
 * Sinkronisasi operasi L2 cache.
 */
static void l2c_sync(void)
{
    /* Write to Cache Sync register */
    l2c_write(L2C_CACHE_SYNC, 0);

    /* Wait for completion */
    while (l2c_read(L2C_CACHE_SYNC) & 1) {
        /* Busy wait */
    }
}

/*
 * ============================================================================
 * FUNGSI KONTROL L2 CACHE (L2 CACHE CONTROL FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_l2c_enable
 * --------------
 * Aktifkan L2 cache.
 *
 * Return: Status operasi
 */
status_t hal_l2c_enable(void)
{
    tak_bertanda32_t ctrl;

    if (g_l2c_base == NULL) {
        return STATUS_TIDAK_DUKUNG;
    }

    /* Baca Control Register */
    ctrl = l2c_read(L2C_CTRL);

    /* Cek apakah sudah enabled */
    if (ctrl & L2C_CTRL_ENABLE) {
        return STATUS_BERHASIL;
    }

    /* Enable L2 cache */
    ctrl |= L2C_CTRL_ENABLE;
    l2c_write(L2C_CTRL, ctrl);

    /* Sync */
    l2c_sync();

    g_l2c_enabled = BENAR;

    return STATUS_BERHASIL;
}

/*
 * hal_l2c_disable
 * ---------------
 * Nonaktifkan L2 cache.
 *
 * Return: Status operasi
 */
status_t hal_l2c_disable(void)
{
    tak_bertanda32_t ctrl;

    if (g_l2c_base == NULL) {
        return STATUS_TIDAK_DUKUNG;
    }

    /* Flush cache terlebih dahulu */
    hal_l2c_flush_all();

    /* Baca Control Register */
    ctrl = l2c_read(L2C_CTRL);

    /* Disable L2 cache */
    ctrl &= ~L2C_CTRL_ENABLE;
    l2c_write(L2C_CTRL, ctrl);

    /* Sync */
    l2c_sync();

    g_l2c_enabled = SALAH;

    return STATUS_BERHASIL;
}

/*
 * hal_l2c_is_enabled
 * ------------------
 * Cek apakah L2 cache aktif.
 *
 * Return: BENAR jika aktif
 */
bool_t hal_l2c_is_enabled(void)
{
    if (g_l2c_base == NULL) {
        return SALAH;
    }

    return (l2c_read(L2C_CTRL) & L2C_CTRL_ENABLE) ? BENAR : SALAH;
}

/*
 * ============================================================================
 * FUNGSI INVALIDATE L2 CACHE (L2 CACHE INVALIDATE FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_l2c_invalidate_all
 * ----------------------
 * Invalidate seluruh L2 cache.
 *
 * Return: Status operasi
 */
status_t hal_l2c_invalidate_all(void)
{
    if (g_l2c_base == NULL) {
        return STATUS_TIDAK_DUKUNG;
    }

    /* Invalidate all ways */
    l2c_write(L2C_INV_WAY, L2C_WAY_MASK);

    /* Wait for completion */
    while (l2c_read(L2C_INV_WAY) & L2C_WAY_MASK) {
        /* Busy wait */
    }

    /* Sync */
    l2c_sync();

    return STATUS_BERHASIL;
}

/*
 * hal_l2c_invalidate_range
 * ------------------------
 * Invalidate range alamat di L2 cache.
 *
 * Parameter:
 *   mulai - Alamat awal
 *   akhir - Alamat akhir
 *
 * Return: Status operasi
 */
status_t hal_l2c_invalidate_range(void *mulai, void *akhir)
{
    tak_bertanda32_t addr;

    if (g_l2c_base == NULL) {
        return STATUS_TIDAK_DUKUNG;
    }

    /* Align ke cache line */
    addr = (tak_bertanda32_t)(tak_bertanda32_t)mulai;
    addr &= ~(g_l2c_line_size - 1);

    /* Invalidate setiap cache line */
    while (addr < (tak_bertanda32_t)(tak_bertanda32_t)akhir) {
        l2c_write(L2C_INV_LINE_PA, addr);
        addr += g_l2c_line_size;
    }

    /* Sync */
    l2c_sync();

    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI CLEAN L2 CACHE (L2 CACHE CLEAN FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_l2c_clean_all
 * -----------------
 * Clean seluruh L2 cache.
 *
 * Return: Status operasi
 */
status_t hal_l2c_clean_all(void)
{
    if (g_l2c_base == NULL) {
        return STATUS_TIDAK_DUKUNG;
    }

    /* Clean all ways */
    l2c_write(L2C_CLEAN_WAY, L2C_WAY_MASK);

    /* Wait for completion */
    while (l2c_read(L2C_CLEAN_WAY) & L2C_WAY_MASK) {
        /* Busy wait */
    }

    /* Sync */
    l2c_sync();

    return STATUS_BERHASIL;
}

/*
 * hal_l2c_clean_range
 * -------------------
 * Clean range alamat di L2 cache.
 *
 * Parameter:
 *   mulai - Alamat awal
 *   akhir - Alamat akhir
 *
 * Return: Status operasi
 */
status_t hal_l2c_clean_range(void *mulai, void *akhir)
{
    tak_bertanda32_t addr;

    if (g_l2c_base == NULL) {
        return STATUS_TIDAK_DUKUNG;
    }

    /* Align ke cache line */
    addr = (tak_bertanda32_t)(tak_bertanda32_t)mulai;
    addr &= ~(g_l2c_line_size - 1);

    /* Clean setiap cache line */
    while (addr < (tak_bertanda32_t)(tak_bertanda32_t)akhir) {
        l2c_write(L2C_CLEAN_LINE_PA, addr);
        addr += g_l2c_line_size;
    }

    /* Sync */
    l2c_sync();

    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI FLUSH L2 CACHE (L2 CACHE FLUSH FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_l2c_flush_all
 * -----------------
 * Flush (clean dan invalidate) seluruh L2 cache.
 *
 * Return: Status operasi
 */
status_t hal_l2c_flush_all(void)
{
    if (g_l2c_base == NULL) {
        return STATUS_TIDAK_DUKUNG;
    }

    /* Clean and invalidate all ways */
    l2c_write(L2C_CLEAN_INV_WAY, L2C_WAY_MASK);

    /* Wait for completion */
    while (l2c_read(L2C_CLEAN_INV_WAY) & L2C_WAY_MASK) {
        /* Busy wait */
    }

    /* Sync */
    l2c_sync();

    return STATUS_BERHASIL;
}

/*
 * hal_l2c_flush_range
 * -------------------
 * Flush (clean dan invalidate) range alamat di L2 cache.
 *
 * Parameter:
 *   mulai - Alamat awal
 *   akhir - Alamat akhir
 *
 * Return: Status operasi
 */
status_t hal_l2c_flush_range(void *mulai, void *akhir)
{
    tak_bertanda32_t addr;

    if (g_l2c_base == NULL) {
        return STATUS_TIDAK_DUKUNG;
    }

    /* Align ke cache line */
    addr = (tak_bertanda32_t)(tak_bertanda32_t)mulai;
    addr &= ~(g_l2c_line_size - 1);

    /* Clean dan invalidate setiap cache line */
    while (addr < (tak_bertanda32_t)(tak_bertanda32_t)akhir) {
        l2c_write(L2C_CLEAN_INV_LINE_PA, addr);
        addr += g_l2c_line_size;
    }

    /* Sync */
    l2c_sync();

    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI INFORMASI (INFORMATION FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_l2c_get_size
 * ----------------
 * Dapatkan ukuran L2 cache.
 *
 * Return: Ukuran dalam KB
 */
tak_bertanda32_t hal_l2c_get_size(void)
{
    return g_l2c_size;
}

/*
 * hal_l2c_get_line_size
 * ---------------------
 * Dapatkan ukuran cache line.
 *
 * Return: Ukuran dalam bytes
 */
tak_bertanda32_t hal_l2c_get_line_size(void)
{
    return g_l2c_line_size;
}

/*
 * hal_l2c_get_ways
 * ----------------
 * Dapatkan jumlah ways.
 *
 * Return: Jumlah ways
 */
tak_bertanda32_t hal_l2c_get_ways(void)
{
    return g_l2c_ways;
}

/*
 * ============================================================================
 * FUNGSI INISIALISASI (INITIALIZATION FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_l2c_init
 * ------------
 * Inisialisasi L2 cache controller.
 *
 * Return: Status operasi
 */
status_t hal_l2c_init(void)
{
    tak_bertanda32_t cache_id;
    tak_bertanda32_t cache_type;
    tak_bertanda32_t aux_ctrl;

    /* Set base address */
    g_l2c_base = (volatile tak_bertanda32_t *)L2C_BASE;

    /* Baca Cache ID Register */
    cache_id = l2c_read(L2C_CACHE_ID);

    /* Verifikasi ini adalah PL310 */
    if (((cache_id >> 24) & 0xFF) != 0x00) {
        g_l2c_base = NULL;
        return STATUS_TIDAK_DUKUNG;
    }

    /* Baca Cache Type Register untuk ukuran */
    cache_type = l2c_read(L2C_CACHE_TYPE);

    /* Extract cache info */
    g_l2c_size = (((cache_type >> 20) & 0xF) + 1) * 128;  /* dalam KB */
    g_l2c_ways = 8;  /* PL310 selalu 8-way */

    /* Disable cache terlebih dahulu */
    hal_l2c_disable();

    /* Invalidate semua */
    hal_l2c_invalidate_all();

    /* Configure Auxiliary Control Register */
    aux_ctrl = 0;

    /* Enable early BRESP */
    aux_ctrl |= L2C_AUX_EARLY_BRESP;

    /* Enable prefetching */
    aux_ctrl |= L2C_AUX_IPREFETCH | L2C_AUX_DPREFETCH;

    /* Set default latencies */
    aux_ctrl |= (L2C_LATENCY_3_CYCLES << L2C_AUX_DATA_LATENCY_SHIFT);
    aux_ctrl |= (L2C_LATENCY_3_CYCLES << L2C_AUX_TAG_LATENCY_SHIFT);

    l2c_write(L2C_AUX_CTRL, aux_ctrl);

    /* Enable L2 cache */
    hal_l2c_enable();

    g_l2c_initialized = BENAR;

    return STATUS_BERHASIL;
}

/*
 * hal_l2c_shutdown
 * ----------------
 * Shutdown L2 cache.
 *
 * Return: Status operasi
 */
status_t hal_l2c_shutdown(void)
{
    if (g_l2c_base == NULL) {
        return STATUS_BERHASIL;
    }

    /* Flush dan disable */
    hal_l2c_flush_all();
    hal_l2c_disable();

    g_l2c_initialized = SALAH;
    g_l2c_base = NULL;

    return STATUS_BERHASIL;
}

/*
 * hal_l2c_is_initialized
 * ----------------------
 * Cek apakah L2 cache sudah diinisialisasi.
 *
 * Return: BENAR jika sudah
 */
bool_t hal_l2c_is_initialized(void)
{
    return g_l2c_initialized;
}
