/* ----------------------------------------------------------------------------------------------
 * pigura inti: papan_klip.c - papan klip (clipboard) cross-platform
 * ----------------------------------------------------------------------------------------------
 * Implementasi pg_papan_klip_t dengan tiga backend:
 *   1. X11 (XStoreBytes / XFetchBytes pada XA_CUT_BUFFER0).
 *      Legacy tapi sederhana; tidak butuh event loop untuk transfer.
 *   2. Win32 (OpenClipboard + SetClipboardData(CF_TEXT, ...)).
 *   3. Fallback in-memory buffer (untuk uji-sendiri tanpa display).
 *
 * Singleton per app: pg_papan_klip_buka(layar) yang dipanggil
 * berulang dengan layar yang sama mengembalikan instance yang sama
 * dengan refcount internal. Tutup harus dipanggil sekali per buka.
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/papan_klip.h"
#include "pigura/layar.h"
#include "pigura/galat.h"
#include "pigura/catat.h"

#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------ struct */

struct pg_papan_klip {
        int    refcount;
        void  *native;       /* Display* (X11), HWND (Win32), NULL */
        unsigned long win;   /* Window ID (X11) */
        /* Fallback in-memory buffer (selalu ada; dipakai bila native
         * NULL atau tidak bisa dipakai). */
        char  *buf;
        int    buf_len;
};

/* Singleton global - instance pertama dipakai sampai refcount turun
 * ke 0. Ini menyederhanakan pola pemakaian demo (buka/tutup bisa
 * berulang tanpa kebocoran). */
static pg_papan_klip_t *g_papan_klip = NULL;

/* ------------------------------------------------------------ helper */

/* Salin teks ke heap (NUL-terminated). */
static char *pg_papan_klip_dup(const char *s, int n)
{
        char *out;
        if (n < 0) n = 0;
        out = (char *)malloc((size_t)n + 1);
        if (!out) return NULL;
        if (s && n > 0)
                memcpy(out, s, (size_t)n);
        out[n] = 0;
        return out;
}

/* ------------------------------------------------------------ backend */

#if defined(_WIN32)
/* ============================ Win32 ============================ */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static pg_galat pg_papan_klip_tulis_native(pg_papan_klip_t *k,
                                             const char *teks,
                                             int n)
{
        HANDLE h;
        char *p;
        (void)k;
        if (!OpenClipboard(NULL)) {
                pg_set_galat(PG_GALAT_UMUM, "OpenClipboard gagal");
                return PG_GALAT_UMUM;
        }
        EmptyClipboard();
        if (n <= 0) {
                CloseClipboard();
                return PG_OK;
        }
        /* CF_TEXT butuh buffer dengan \r\n line endings dan NUL
         * terminator. Untuk simplicity v0.1, salin apa adanya. */
        h = GlobalAlloc(GMEM_MOVEABLE, (size_t)n + 1);
        if (!h) {
                CloseClipboard();
                pg_set_galat(PG_GALAT_MEMORI, "GlobalAlloc gagal");
                return PG_GALAT_MEMORI;
        }
        p = (char *)GlobalLock(h);
        if (!p) {
                GlobalFree(h);
                CloseClipboard();
                pg_set_galat(PG_GALAT_UMUM, "GlobalLock gagal");
                return PG_GALAT_UMUM;
        }
        memcpy(p, teks, (size_t)n);
        p[n] = 0;
        GlobalUnlock(h);
        if (!SetClipboardData(CF_TEXT, h)) {
                GlobalFree(h);
                CloseClipboard();
                pg_set_galat(PG_GALAT_UMUM, "SetClipboardData gagal");
                return PG_GALAT_UMUM;
        }
        CloseClipboard();
        return PG_OK;
}

static char *pg_papan_klip_baca_native(pg_papan_klip_t *k)
{
        HANDLE h;
        const char *p;
        char *out;
        int n;
        (void)k;
        if (!OpenClipboard(NULL)) return NULL;
        h = GetClipboardData(CF_TEXT);
        if (!h) {
                CloseClipboard();
                return NULL;
        }
        p = (const char *)GlobalLock(h);
        if (!p) {
                CloseClipboard();
                return NULL;
        }
        n = (int)strlen(p);
        out = pg_papan_klip_dup(p, n);
        GlobalUnlock(h);
        CloseClipboard();
        return out;
}

#else
/* ============================ X11 / fallback ============================ */

/* Deteksi X11 pada waktu kompilasi. Bila backend X11 aktif, kita
 * pakai XStoreBytes/XFetchBytes. Header X11 di-include hanya bila
 * tersedia. */
#if defined(HAVE_X11)
#  include <X11/Xlib.h>
#endif

/* Tulis teks ke clipboard native. Bila native (Display*) NULL,
 * tulis ke fallback in-memory buffer. */
static pg_galat pg_papan_klip_tulis_native(pg_papan_klip_t *k,
                                             const char *teks,
                                             int n)
{
#if defined(HAVE_X11)
        Display *d = (Display *)k->native;
        if (d) {
                XStoreBytes(d, teks ? teks : "", n);
                XFlush(d);
                return PG_OK;
        }
#endif
        /* Fallback in-memory. */
        if (k->buf) {
                free(k->buf);
                k->buf = NULL;
                k->buf_len = 0;
        }
        if (n > 0 && teks) {
                k->buf = pg_papan_klip_dup(teks, n);
                if (!k->buf) {
                        pg_set_galat(PG_GALAT_MEMORI,
                                     "papan_klip: OOM");
                        return PG_GALAT_MEMORI;
                }
                k->buf_len = n;
        }
        return PG_OK;
}

/* Baca teks dari clipboard native. Bila native NULL, baca dari
 * fallback in-memory. */
static char *pg_papan_klip_baca_native(pg_papan_klip_t *k)
{
#if defined(HAVE_X11)
        Display *d = (Display *)k->native;
        if (d) {
                char *raw;
                int n;
                char *out;
                raw = XFetchBytes(d, &n);
                if (!raw || n <= 0) {
                        if (raw) XFree(raw);
                        return NULL;
                }
                out = pg_papan_klip_dup(raw, n);
                XFree(raw);
                return out;
        }
#endif
        /* Fallback in-memory. */
        if (!k->buf || k->buf_len <= 0) return NULL;
        return pg_papan_klip_dup(k->buf, k->buf_len);
}

#endif /* _WIN32 */

/* ------------------------------------------------------------ API */

pg_papan_klip_t *pg_papan_klip_buka(struct pg_layar *layar)
{
        pg_papan_klip_t *k;
        /* Singleton: bila sudah ada, tingkatkan refcount. */
        if (g_papan_klip) {
                g_papan_klip->refcount++;
                return g_papan_klip;
        }
        k = (pg_papan_klip_t *)calloc(1, sizeof(*k));
        if (!k) {
                pg_set_galat(PG_GALAT_MEMORI, "papan_klip: OOM");
                return NULL;
        }
        k->refcount = 1;
        /* Ambil handle native dari layar bila ada. */
        if (layar) {
                k->native = pg_layar_handle_native(layar);
                k->win    = pg_layar_jendela_id(layar);
        }
        g_papan_klip = k;
        pg_debug_log("papan_klip: buka (native=%s)",
                     k->native ? "ya" : "tidak");
        return k;
}

void pg_papan_klip_tutup(pg_papan_klip_t *k)
{
        if (!k) return;
        if (k != g_papan_klip) return; /* bukan singleton */
        k->refcount--;
        if (k->refcount > 0) return;
        /* Lepaskan semua resource. */
        if (k->buf) {
                free(k->buf);
                k->buf = NULL;
        }
        free(k);
        g_papan_klip = NULL;
        pg_debug_log("papan_klip: tutup");
}

pg_galat pg_papan_klip_tulis_teks(pg_papan_klip_t *k,
                                    const char *teks)
{
        int n;
        if (!k) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
        n = teks ? (int)strlen(teks) : 0;
        return pg_papan_klip_tulis_native(k, teks, n);
}

char *pg_papan_klip_baca_teks(pg_papan_klip_t *k)
{
        if (!k) return NULL;
        return pg_papan_klip_baca_native(k);
}
