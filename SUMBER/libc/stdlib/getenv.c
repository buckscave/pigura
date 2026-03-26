/*
 * PIGURA LIBC - STDLIB/GETENV.C
 * ==============================
 * Implementasi fungsi environment variable.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 *
 * Fungsi yang diimplementasikan:
 *   - getenv()    : Dapatkan nilai environment variable
 *   - setenv()    : Set nilai environment variable (POSIX)
 *   - unsetenv()  : Hapus environment variable (POSIX)
 *   - putenv()    : Set environment variable (POSIX)
 *   - clearenv()  : Hapus semua environment variables (BSD)
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* ============================================================
 * KONFIGURASI
 * ============================================================
 */

/* Maksimum jumlah environment variables */
#define ENV_MAX_ENTRIES   256

/* Maksimum panjang nama variable */
#define ENV_MAX_NAMELEN   256

/* Maksimum panjang nilai variable */
#define ENV_MAX_VALUELEN  4096

/* ============================================================
 * VARIABEL GLOBAL
 * ============================================================
 */

/* Pointer ke environment (dari kernel/startup) */
extern char **environ;

/* Flag untuk menandai apakah environment sudah diinisialisasi */
static int env_initialized = 0;

/* Environment buffer internal (jika tidak ada dari kernel) */
static char *env_internal[ENV_MAX_ENTRIES];

/* ============================================================
 * FUNGSI PEMBANTU
 * ============================================================
 */

/*
 * env_init - Inisialisasi environment
 */
static void env_init(void) {
    if (env_initialized) {
        return;
    }

    /* Jika environ tidak diset, gunakan buffer internal */
    if (environ == NULL) {
        environ = env_internal;
        env_internal[0] = NULL;
    }

    env_initialized = 1;
}

/*
 * find_env - Cari environment variable berdasarkan nama
 *
 * Parameter:
 *   name - Nama variable
 *   len  - Panjang nama (output, jika ditemukan)
 *
 * Return: Index variable, atau -1 jika tidak ditemukan
 */
static int find_env(const char *name, size_t *len) {
    char **env;
    size_t name_len;
    int i;

    if (name == NULL || environ == NULL) {
        return -1;
    }

    name_len = strlen(name);

    /* Cari nama yang tidak mengandung '=' */
    for (i = 0; name[i] != '\0'; i++) {
        if (name[i] == '=') {
            return -1;
        }
    }

    for (i = 0, env = environ; *env != NULL; env++, i++) {
        if (strncmp(*env, name, name_len) == 0 &&
            (*env)[name_len] == '=') {
            if (len != NULL) {
                *len = name_len;
            }
            return i;
        }
    }

    return -1;
}

/*
 * count_env - Hitung jumlah environment variables
 */
static int count_env(void) {
    int count = 0;

    if (environ == NULL) {
        return 0;
    }

    while (environ[count] != NULL) {
        count++;
    }

    return count;
}

/*
 * alloc_env_string - Alokasikan string "name=value"
 *
 * Parameter:
 *   name  - Nama variable
 *   value - Nilai variable
 *
 * Return: String yang dialokasikan, atau NULL jika gagal
 */
static char *alloc_env_string(const char *name, const char *value) {
    size_t name_len, value_len, total_len;
    char *str;

    name_len = strlen(name);
    value_len = strlen(value);
    total_len = name_len + 1 + value_len + 1;  /* '=' + '\0' */

    str = (char *)malloc(total_len);
    if (str == NULL) {
        return NULL;
    }

    memcpy(str, name, name_len);
    str[name_len] = '=';
    memcpy(str + name_len + 1, value, value_len + 1);

    return str;
}

/* ============================================================
 * IMPLEMENTASI FUNGSI UTAMA
 * ============================================================
 */

/*
 * getenv - Dapatkan nilai environment variable
 *
 * Parameter:
 *   name - Nama environment variable
 *
 * Return: Nilai variable, atau NULL jika tidak ada
 *
 * Catatan: Jangan memodifikasi string yang dikembalikan.
 *          Pointer menjadi invalid setelah setenv/putenv/unsetenv.
 */
char *getenv(const char *name) {
    int idx;
    size_t name_len;

    env_init();

    if (name == NULL || environ == NULL) {
        return NULL;
    }

    idx = find_env(name, &name_len);
    if (idx < 0) {
        return NULL;
    }

    /* Return pointer ke nilai (setelah '=') */
    return environ[idx] + name_len + 1;
}

/*
 * setenv - Set nilai environment variable (POSIX)
 *
 * Parameter:
 *   name      - Nama variable
 *   value     - Nilai variable
 *   overwrite - Overwrite jika sudah ada (1 = ya, 0 = tidak)
 *
 * Return: 0 jika berhasil, -1 jika gagal
 *
 * Catatan: Membuat copy dari name dan value.
 */
int setenv(const char *name, const char *value, int overwrite) {
    int idx;
    int count;
    char *new_entry;
    size_t name_len;

    env_init();

    /* Validasi parameter */
    if (name == NULL || value == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Validasi nama tidak mengandung '=' */
    if (strchr(name, '=') != NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Validasi panjang */
    if (strlen(name) > ENV_MAX_NAMELEN) {
        errno = EINVAL;
        return -1;
    }

    /* Cari variable yang sudah ada */
    idx = find_env(name, &name_len);

    if (idx >= 0) {
        /* Variable sudah ada */
        if (!overwrite) {
            return 0;  /* Jangan overwrite */
        }

        /* Buat entry baru */
        new_entry = alloc_env_string(name, value);
        if (new_entry == NULL) {
            errno = ENOMEM;
            return -1;
        }

        /* Free entry lama dan ganti */
        free(environ[idx]);
        environ[idx] = new_entry;

        return 0;
    }

    /* Variable baru */
    count = count_env();

    /* Cek kapasitas */
    if (count >= ENV_MAX_ENTRIES - 1) {
        errno = ENOMEM;
        return -1;
    }

    /* Buat entry baru */
    new_entry = alloc_env_string(name, value);
    if (new_entry == NULL) {
        errno = ENOMEM;
        return -1;
    }

    /* Tambahkan ke array */
    environ[count] = new_entry;
    environ[count + 1] = NULL;

    return 0;
}

/*
 * unsetenv - Hapus environment variable (POSIX)
 *
 * Parameter:
 *   name - Nama variable yang dihapus
 *
 * Return: 0 jika berhasil, -1 jika gagal
 */
int unsetenv(const char *name) {
    int idx;
    int count;
    size_t name_len;

    env_init();

    /* Validasi parameter */
    if (name == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Validasi nama tidak mengandung '=' */
    if (strchr(name, '=') != NULL) {
        errno = EINVAL;
        return -1;
    }

    idx = find_env(name, &name_len);
    if (idx < 0) {
        /* Variable tidak ada, bukan error */
        return 0;
    }

    /* Free entry */
    free(environ[idx]);

    /* Geser entry berikutnya */
    count = count_env();
    for (; idx < count - 1; idx++) {
        environ[idx] = environ[idx + 1];
    }
    environ[count - 1] = NULL;

    return 0;
}

/*
 * putenv - Set environment variable (POSIX)
 *
 * Parameter:
 *   string - String "name=value"
 *
 * Return: 0 jika berhasil, non-zero jika gagal
 *
 * Catatan: Berbeda dengan setenv, putenv TIDAK membuat copy.
 *          String yang diberikan menjadi bagian dari environment.
 *          Jangan free atau modifikasi string setelah putenv!
 */
int putenv(char *string) {
    char *eq;
    int idx;
    int count;
    size_t name_len;

    env_init();

    /* Validasi parameter */
    if (string == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Cari '=' dalam string */
    eq = strchr(string, '=');
    if (eq == NULL || eq == string) {
        errno = EINVAL;
        return -1;
    }

    name_len = eq - string;

    /* Cari variable yang sudah ada */
    for (idx = 0; environ[idx] != NULL; idx++) {
        if (strncmp(environ[idx], string, name_len) == 0 &&
            environ[idx][name_len] == '=') {
            /* Variable sudah ada, ganti */
            environ[idx] = string;
            return 0;
        }
    }

    /* Variable baru */
    count = count_env();

    if (count >= ENV_MAX_ENTRIES - 1) {
        errno = ENOMEM;
        return -1;
    }

    environ[count] = string;
    environ[count + 1] = NULL;

    return 0;
}

/*
 * clearenv - Hapus semua environment variables (BSD)
 *
 * Return: 0 jika berhasil, non-zero jika gagal
 */
int clearenv(void) {
    int i;

    env_init();

    if (environ == NULL) {
        return 0;
    }

    /* Free semua entry (kecuali jika menggunakan environ dari kernel) */
    if (environ != env_internal) {
        for (i = 0; environ[i] != NULL; i++) {
            free(environ[i]);
            environ[i] = NULL;
        }
    }

    environ[0] = NULL;

    return 0;
}

/* ============================================================
 * FUNGSI TAMBAHAN (NON-STANDAR)
 * ============================================================
 */

/*
 * getenv_s - Versi aman dari getenv (C11)
 *
 * Parameter:
 *   lenmax  - Maksimum karakter yang bisa disimpan
 *   value   - Buffer untuk nilai
 *   len     - Pointer untuk menyimpan panjang nilai
 *   name    - Nama variable
 *
 * Return: 0 jika berhasil, error code jika gagal
 */
int getenv_s(size_t *len, char *value, size_t lenmax, const char *name) {
    char *val;
    size_t val_len;

    if (name == NULL) {
        if (len != NULL) {
            *len = 0;
        }
        if (value != NULL && lenmax > 0) {
            value[0] = '\0';
        }
        return 0;  /* Atau return error? */
    }

    val = getenv(name);
    if (val == NULL) {
        if (len != NULL) {
            *len = 0;
        }
        if (value != NULL && lenmax > 0) {
            value[0] = '\0';
        }
        return 0;
    }

    val_len = strlen(val);

    if (len != NULL) {
        *len = val_len;
    }

    if (value != NULL) {
        if (val_len >= lenmax) {
            /* Buffer terlalu kecil */
            if (lenmax > 0) {
                value[0] = '\0';
            }
            return -1;  /* ERANGE */
        }
        memcpy(value, val, val_len + 1);
    }

    return 0;
}

/*
 * setenv_s - Versi aman dari setenv (C11)
 */
int setenv_s(const char *name, const char *value, int overwrite) {
    return setenv(name, value, overwrite);
}

/*
 * secure_getenv - Versi aman dari getenv untuk program setuid (GNU)
 *
 * Return NULL jika program berjalan dengan privilege berbeda
 * dari user yang menjalankannya.
 */
char *secure_getenv(const char *name) {
    /* Untuk implementasi sederhana, sama dengan getenv.
     * Implementasi penuh harus mengecek getuid() == geteuid(). */
    return getenv(name);
}
