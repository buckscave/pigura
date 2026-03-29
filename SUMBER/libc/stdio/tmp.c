/*
 * PIGURA LIBC - STDIO/TMP.C
 * ============================
 * Implementasi fungsi tmpfile dan tmpnam.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

/* ============================================================
 * KONSTANTA
 * ============================================================
 */

/* Prefix untuk file temporary */
#define TMP_PREFIX      "/tmp/tmp_"
#define TMP_PREFIX_LEN  9

/* Counter untuk nama unik */
static unsigned long _tmp_counter = 0;

/* Buffer internal untuk tmpnam */
static char _tmpnam_buffer[L_tmpnam];

/* ============================================================
 * _GENERATE_TMPNAME (Internal)
 * ============================================================
 * Generate nama file temporary yang unik.
 *
 * Parameter:
 *   buffer - Buffer untuk menyimpan nama
 *
 * Return: Pointer ke buffer
 */
static char *_generate_tmpname(char *buffer) {
    unsigned long num;
    int i;
    int len;
    static const char digits[] = "0123456789abcdef";

    /* Salin prefix */
    strncpy(buffer, TMP_PREFIX, L_tmpnam - 1);
    buffer[L_tmpnam - 1] = '\0';
    len = TMP_PREFIX_LEN;

    /* Tambahkan counter dan PID untuk keunikan */
    num = _tmp_counter++;

    /* Tambahkan PID */
    num ^= (unsigned long)getpid() << 16;

    /* Konversi ke hex */
    for (i = 0; i < 8; i++) {
        buffer[len++] = digits[(num >> (28 - i * 4)) & 0xf];
    }

    buffer[len] = '\0';

    return buffer;
}

/* ============================================================
 * TMPFILE
 * ============================================================
 * Buat file temporary.
 *
 * Return: Pointer ke FILE, atau NULL jika gagal
 */
FILE *tmpfile(void) {
    char pathname[L_tmpnam];
    FILE *fp;
    int fd;
    int attempts;

    /* Coba buat file temporary */
    attempts = 0;

    while (attempts < TMP_MAX) {
        /* Generate nama unik */
        _generate_tmpname(pathname);

        /* Coba buat file dengan O_EXCL untuk memastikan unik */
        fd = open(pathname, O_RDWR | O_CREAT | O_EXCL, 0600);

        if (fd >= 0) {
            /* Berhasil buat file */
            break;
        }

        /* File sudah ada, coba lagi */
        if (errno == EEXIST) {
            attempts++;
            continue;
        }

        /* Error lain */
        return NULL;
    }

    if (attempts >= TMP_MAX) {
        errno = EEXIST;
        return NULL;
    }

    /* Buat stream dari file descriptor */
    fp = fdopen(fd, "w+b");

    if (fp == NULL) {
        close(fd);
        unlink(pathname);
        return NULL;
    }

    /* Hapus file segera setelah dibuka */
    /* File akan tetap ada selama masih terbuka */
    unlink(pathname);

    return fp;
}

/* ============================================================
 * TMPNAM
 * ============================================================
 * Buat nama file temporary.
 *
 * Parameter:
 *   s - Buffer untuk nama (boleh NULL)
 *
 * Return: Nama file temporary
 */
char *tmpnam(char *s) {
    char *buffer;
    int attempts;

    /* Gunakan buffer internal jika tidak disediakan */
    if (s == NULL) {
        buffer = _tmpnam_buffer;
    } else {
        buffer = s;
    }

    /* Coba generate nama yang belum ada */
    attempts = 0;

    while (attempts < TMP_MAX) {
        /* Generate nama unik */
        _generate_tmpname(buffer);

        /* Cek apakah file sudah ada */
        if (access(buffer, F_OK) != 0) {
            /* File tidak ada, nama valid */
            if (errno == ENOENT) {
                return buffer;
            }
        }

        /* File ada atau error, coba lagi */
        attempts++;
    }

    /* Gagal menemukan nama unik */
    if (s == NULL) {
        return NULL;
    }

    errno = EEXIST;
    return NULL;
}

/* ============================================================
 * TEMPnam (BSD Extension)
 * ============================================================
 * Buat nama file temporary dengan direktori tertentu.
 *
 * Parameter:
 *   dir - Direktori (NULL untuk default)
 *   pfx - Prefix nama (boleh NULL)
 *
 * Return: Nama file temporary (perlu di-free)
 */
char *tempnam(const char *dir, const char *pfx) {
    char *buffer;
    char *tmpdir;
    size_t len;
    unsigned long num;
    int i;
    int attempts;
    static const char digits[] = "0123456789abcdef";

    /* Tentukan direktori */
    if (dir != NULL && access(dir, W_OK) == 0) {
        tmpdir = (char *)dir;
    } else {
        /* Coba dari environment */
        tmpdir = getenv("TMPDIR");
        if (tmpdir == NULL) {
            tmpdir = "/tmp";
        }
    }

    /* Alokasikan buffer */
    len = strlen(tmpdir) + 1 + (pfx ? strlen(pfx) : 3) + 8 + 1;
    buffer = (char *)malloc(len);

    if (buffer == NULL) {
        return NULL;
    }

    /* Bangun nama */
    len = strlen(tmpdir);
    strncpy(buffer, tmpdir, len);
    buffer[len] = '\0';

    /* Tambahkan separator jika perlu */
    if (len > 0 && buffer[len - 1] != '/') {
        buffer[len++] = '/';
    }

    /* Tambahkan prefix */
    if (pfx != NULL) {
        size_t pfx_len = strlen(pfx);
        size_t copy_len = (pfx_len < 5) ? pfx_len : 5;
        strncpy(buffer + len, pfx, copy_len);
        len += copy_len;
    } else {
        strncpy(buffer + len, "tmp", 3);
        len += 3;
    }

    buffer[len] = '\0';  /* Null-terminate sebelum uniqueness loop */

    /* Loop untuk menemukan nama yang belum ada */
    attempts = 0;
    while (attempts < TMP_MAX) {
        /* Tambahkan angka unik */
        num = _tmp_counter++;
        num ^= (unsigned long)getpid() << 16;

        for (i = 0; i < 8; i++) {
            buffer[len++] = digits[(num >> (28 - i * 4)) & 0xf];
        }
        buffer[len] = '\0';

        /* Cek apakah nama sudah ada */
        if (access(buffer, F_OK) != 0 && errno == ENOENT) {
            return buffer;
        }

        /* Reset posisi untuk percobaan berikutnya */
        len -= 8;
        attempts++;
    }

    /* Gagal menemukan nama unik */
    free(buffer);
    errno = EEXIST;
    return NULL;
}

/* ============================================================
 * MKTEMP (BSD Extension)
 * ============================================================
 * Buat nama file temporary unik.
 *
 * Parameter:
 *   template - Template nama dengan XXXXXX
 *
 * Return: Template yang dimodifikasi, atau NULL jika gagal
 */
char *mktemp(char *template) {
    char *p;
    size_t len;
    unsigned long num;
    int i;
    int attempts;
    static const char digits[] = "0123456789abcdef";

    if (template == NULL) {
        return NULL;
    }

    len = strlen(template);

    /* Cari XXXXXX di akhir */
    if (len < 6) {
        return NULL;
    }

    p = template + len - 6;

    if (strcmp(p, "XXXXXX") != 0) {
        return NULL;
    }

    /* Ganti XXXXXX dengan angka unik */
    attempts = 0;

    while (attempts < TMP_MAX) {
        num = _tmp_counter++;
        num ^= (unsigned long)getpid() << 16;
        num ^= (unsigned long)attempts;

        for (i = 0; i < 6; i++) {
            p[i] = digits[(num >> (20 - i * 4)) & 0xf];
        }

        /* Cek apakah nama sudah ada */
        if (access(template, F_OK) != 0 && errno == ENOENT) {
            return template;
        }

        attempts++;
    }

    /* Gagal */
    template[0] = '\0';
    return NULL;
}

/* ============================================================
 * MKSTEMP (BSD Extension)
 * ============================================================
 * Buat dan buka file temporary unik.
 *
 * Parameter:
 *   template - Template nama dengan XXXXXX
 *
 * Return: File descriptor, atau -1 jika gagal
 */
int mkstemp(char *template) {
    char *p;
    size_t len;
    unsigned long num;
    int i;
    int fd;
    int attempts;
    static const char digits[] = "0123456789abcdef";

    if (template == NULL) {
        return -1;
    }

    len = strlen(template);

    if (len < 6) {
        return -1;
    }

    p = template + len - 6;

    if (strcmp(p, "XXXXXX") != 0) {
        return -1;
    }

    attempts = 0;

    while (attempts < TMP_MAX) {
        num = _tmp_counter++;
        num ^= (unsigned long)getpid() << 16;
        num ^= (unsigned long)attempts;

        for (i = 0; i < 6; i++) {
            p[i] = digits[(num >> (20 - i * 4)) & 0xf];
        }

        /* Coba buat file */
        fd = open(template, O_RDWR | O_CREAT | O_EXCL, 0600);

        if (fd >= 0) {
            return fd;
        }

        if (errno != EEXIST) {
            return -1;
        }

        attempts++;
    }

    return -1;
}
